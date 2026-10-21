#include "WarfareFrontlineLockV39.h"
#include "WarfareFrontline.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderingThread.h"
#include "UObject/StrongObjectPtr.h"
#include "BatchedElements.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "GlobalShader.h"
#include "SimpleElementShaders.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "TextureResource.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

class FFrontlineLockPSV39 final : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFrontlineLockPSV39);
public:
    FFrontlineLockPSV39()=default;
    explicit FFrontlineLockPSV39(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FGlobalShader(Initializer)
    {
        LockAtlas.Bind(Initializer.ParameterMap,TEXT("LockAtlas"));
        LockAtlasSampler.Bind(Initializer.ParameterMap,TEXT("LockAtlasSampler"));
    }
    void SetParameters(FRHIBatchedShaderParameters& Parameters,const FTexture* Texture)
    {SetTextureParameter(Parameters,LockAtlas,LockAtlasSampler,Texture);}
private:
    LAYOUT_FIELD(FShaderResourceParameter,LockAtlas);
    LAYOUT_FIELD(FShaderResourceParameter,LockAtlasSampler);
};
IMPLEMENT_GLOBAL_SHADER(FFrontlineLockPSV39,"/Project/Private/WarfareLockV39.usf","Main",SF_Pixel);

namespace
{
class FFrontlineLockParametersV39 final : public FBatchedElementParameters
{
public:
    virtual void BindShaders(FRHICommandList& RHICmdList,FGraphicsPipelineStateInitializer& PSO,
        ERHIFeatureLevel::Type FeatureLevel,const FMatrix& Transform,const float,const FMatrix&,const FTexture* Texture) override
    {
        TShaderMapRef<FSimpleElementVS> VS(GetGlobalShaderMap(FeatureLevel));
        TShaderMapRef<FFrontlineLockPSV39> PS(GetGlobalShaderMap(FeatureLevel));
        // Scene translucent materials multiply destination alpha by 1-srcA.
        // The HUD is an RGBA layer: accumulate coverage as srcA+dstA*(1-srcA).
        // RGB and alpha share the exact same silhouette, including its holes.
        PSO.BlendState=TStaticBlendState<CW_RGBA,BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_One,BF_InverseSourceAlpha>::GetRHI();
        PSO.BoundShaderState.VertexDeclarationRHI=GSimpleElementVertexDeclaration.VertexDeclarationRHI;
        PSO.BoundShaderState.VertexShaderRHI=VS.GetVertexShader();PSO.BoundShaderState.PixelShaderRHI=PS.GetPixelShader();
        PSO.PrimitiveType=PT_TriangleList;RHICmdList.ApplyCachedRenderTargets(PSO);SetGraphicsPipelineState(RHICmdList,PSO,0);
        SetShaderParametersLegacyVS(RHICmdList,VS,Transform);SetShaderParametersLegacyPS(RHICmdList,PS,Texture);
    }
};
}

void FrontlineLockV39::Draw(UCanvas* Canvas,UTexture2D* Atlas,const FVector2D& Position,const FVector2D& Size)
{
    static TStrongObjectPtr<UTexture2D> StoresLock(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V66/T_StoresLock.T_StoresLock")));
    Atlas=StoresLock.Get();
    if(!Canvas||!Canvas->Canvas||!Atlas||!Atlas->GetResource()||Size.X<=0||Size.Y<=0)return;
    // The source cell is square; letterboxing a caller's rectangle must never
    // stretch the lock. All current roster callers already request squares.
    const double Side=FMath::Min(Size.X,Size.Y);
    const FVector2D Origin=Position+(Size-FVector2D(Side,Side))*.5;
    // Native alpha includes the shackle opening; there is no colour-key mask.
    // Equal canvas padding preserves centring at every roster size.
    FCanvasTileItem Tile(Origin,Atlas->GetResource(),FVector2D(Side,Side),
        FVector2D(0,0),FVector2D(1,1),FLinearColor::White);
    static TRefCountPtr<FFrontlineLockParametersV39> Parameters=new FFrontlineLockParametersV39;
    Tile.BatchedElementParameters=Parameters.GetReference();Tile.BlendMode=SE_BLEND_AlphaBlend;
    Canvas->DrawItem(Tile);
}

TSharedRef<FJsonObject> FrontlineLockV39::RunCoverageChecks(UWorld* World)
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Formats;bool Success=true;
    auto Check=[&](const FString& Name,bool Pass)
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("pass"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(J));Success&=Pass;
    };
    FString UserDir;auto* B=World?World->GetAuthGameMode<AWarfareFrontlineGameMode>():nullptr;
    const bool Isolated=B&&B->IsProgressionVolatile()&&(FParse::Param(FCommandLine::Get(),TEXT("WFV39Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV66Review")))&&
        FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    Check(TEXT("isolated_volatile_render_probe"),Isolated);
    if(!Isolated){Report->SetBoolField(TEXT("success"),false);Report->SetArrayField(TEXT("checks"),Checks);return Report;}
    TStrongObjectPtr<UTexture2D> Atlas(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V66/T_StoresLock.T_StoresLock")));
#if WITH_EDITOR
    // The first editor load can return a placeholder while UI texture DDC is built.
    FAssetCompilingManager::Get().FinishAllCompilation();
#endif
    FlushRenderingCommands();
    Check(TEXT("painted_atlas_loaded"),Atlas.IsValid()&&Atlas->GetResource());
    if(!Atlas.IsValid()||!Atlas->GetResource()){Report->SetBoolField(TEXT("success"),false);Report->SetArrayField(TEXT("checks"),Checks);return Report;}
    constexpr int32 Width=256,Height=128;
    const float Origins[]={8,120,205},Sizes[]={96,58,32};
    const FVector2D MetalSamples[]={FVector2D(.5,.55),FVector2D(.34,.65),FVector2D(.5,.09)};
    const FVector2D OpenSamples[]={FVector2D(.05,.12),FVector2D(.5,.25),FVector2D(.9,.15)};
    for(int32 Format=0;Format<2;++Format)
    {
        TArray<FLinearColor> Pixels[2];bool ReadOK=true;
        for(int32 Background=0;Background<2;++Background)
        {
            TStrongObjectPtr<UTextureRenderTarget2D> Target(NewObject<UTextureRenderTarget2D>(GetTransientPackage()));
            Target->ClearColor=Background?FLinearColor(.18f,.4f,.7f,1):FLinearColor::Transparent;
            Target->InitCustomFormat(Width,Height,Format?PF_FloatRGBA:PF_B8G8R8A8,Format!=0);
            Target->UpdateResourceImmediate(true);
            FTextureRenderTargetResource* Resource=Target->GameThread_GetRenderTargetResource();
            FCanvas RenderCanvas(Resource,nullptr,World,World->GetFeatureLevel());RenderCanvas.Clear(Target->ClearColor);
            TStrongObjectPtr<UCanvas> Surface(NewObject<UCanvas>(GetTransientPackage()));Surface->Init(Width,Height,nullptr,&RenderCanvas);
            for(int32 I=0;I<3;++I)Draw(Surface.Get(),Atlas.Get(),FVector2D(Origins[I],16),FVector2D(Sizes[I],Sizes[I]));
            RenderCanvas.Flush_GameThread();FlushRenderingCommands();
            FReadSurfaceDataFlags Flags(RCM_MinMax);Flags.SetLinearToGamma(false);
            ReadOK&=Resource->ReadLinearColorPixels(Pixels[Background],Flags)&&Pixels[Background].Num()==Width*Height;
            Surface->Canvas=nullptr;
        }
        const FString Prefix=Format?TEXT("linear_rgba16f"):TEXT("sdr_rgba8");Check(Prefix+TEXT("_gpu_readback"),ReadOK);
        if(!ReadOK)continue;
        float MinimumMetalAlpha=1,MaximumOpenAlpha=0,MaximumCoreRGBDelta=0,MinimumOpaqueAlpha=1;
        for(int32 I=0;I<3;++I)
        {
            auto Index=[&](FVector2D P){return FMath::FloorToInt(16+P.Y*Sizes[I])*Width+FMath::FloorToInt(Origins[I]+P.X*Sizes[I]);};
            for(const auto& Point:MetalSamples)
            {
                const auto& A=Pixels[0][Index(Point)];const auto& C=Pixels[1][Index(Point)];
                MinimumMetalAlpha=FMath::Min(MinimumMetalAlpha,A.A);
                MaximumCoreRGBDelta=FMath::Max(MaximumCoreRGBDelta,FMath::Max3(FMath::Abs(A.R-C.R),FMath::Abs(A.G-C.G),FMath::Abs(A.B-C.B)));
            }
            for(const auto& Point:OpenSamples)MaximumOpenAlpha=FMath::Max(MaximumOpenAlpha,Pixels[0][Index(Point)].A);
        }
        for(const auto& Pixel:Pixels[1])MinimumOpaqueAlpha=FMath::Min(MinimumOpaqueAlpha,Pixel.A);
        Check(Prefix+TEXT("_metal_opaque_at_96_58_32_pixels"),MinimumMetalAlpha>.995f);
        Check(Prefix+TEXT("_openings_and_exterior_transparent"),MaximumOpenAlpha<.005f);
        Check(Prefix+TEXT("_metal_rgb_independent_of_background"),MaximumCoreRGBDelta<.005f);
        Check(Prefix+TEXT("_opaque_destination_coverage_preserved"),MinimumOpaqueAlpha>.995f);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("format"),Prefix);J->SetNumberField(TEXT("minimum_metal_alpha"),MinimumMetalAlpha);
        J->SetNumberField(TEXT("maximum_open_alpha"),MaximumOpenAlpha);J->SetNumberField(TEXT("maximum_core_rgb_difference"),MaximumCoreRGBDelta);
        J->SetNumberField(TEXT("minimum_opaque_destination_alpha"),MinimumOpaqueAlpha);Formats.Add(MakeShared<FJsonValueObject>(J));
    }
    Report->SetBoolField(TEXT("success"),Success);Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("formats"),Formats);
    return Report;
}
