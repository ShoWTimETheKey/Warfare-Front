#include "WarfareFrontlineUIMotionV83.h"
#include "WarfareFrontlineReportV839.h"
#include "WarfareFrontlineArchiveToneV833.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineUnlocksV53.h"
#include "WarfareFrontStartupV67.h"
#include "WarfareFrontCursorV67.h"
#include "WarfareFrontStartupReviewV67.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineArmourSurfaceV837.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/SecureHash.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFRRUContentV82.h"
#include "WarfareFRRUVauxCompositionV87.h"
#include "WarfareFRRUTextV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUArmourV82.h"
#include "FRRU/Native/UE/ExpansionArchiveUE.h"
#include "FRRU/Native/UE/ExpansionTacticalArchiveUE.h"
#include "FRRU/Native/TacticalRuntime.h"
#include "FRRU/FRRUVehicleNavCandidateV87.h"
#include "FRRU/FRRUArtV82.generated.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMedalsV8215.h"
#include "WarfareFrontlineMedalRibbonV8446.h"
#include "WarfareFrontlineUIReviewAccessV8447.h"
#include "WarfareFrontlineTrenchMotionV8447Checks.h"
#include "WarfareFrontlineBalanceV50.h"
#include "WarfareFrontlineArmourKillTallyV8220.h"
#include "WarfareFrontlineArmourWreckV8220.h"
#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFrontlineBlastCompositionV8221.h"
#include "WarfareFrontlineArmourValourV8221.h"
#include "WarfareFrontlineRibbonRackV8222.h"
#include "WarfareFrontlineFireTimeV8221.h"
#include "WarfareFrontlineAirSupportV8220.h"
#include "WarfareFrontlineMedalMotionV8216.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineNewUnlockV8215.h"
#include "WarfareFrontlineAwardMotionV8217.h"
#include "WarfareFrontlineFrontsV76.h"
#include "WarfareFrontlineConsoleV79.h"
#include "WarfareFrontlineUIAssetsV8213.h"
#include "WarfareFrontlineConsoleLayoutV79.h"
#include "WarfareFrontlineReviewV76.h"
#include "WarfareFrontlineGameplayReviewV75.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineHUDV45.h"
#include "WarfareFrontlineLockV39.h"
#include "WarfareFrontlineUpdatesV36.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineSupportV32.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFrontlineSoldierSurfaceV62.h"
#include "WarfareFrontlineSoldierSurfaceV67.h"
#include "WarfareFrontlineFilmV32.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV51.h"
#include "WarfareFrontlineCampaignV52.h"
#include "WarfareFrontlineWarRoomV52.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineTerrainV42.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineVoiceContext.h"
#include "WarfareFrontlineCameraInput.h"
#include "WarfareFrontlineV25HDRReview.h"
#include "WarfareFrontlineCampaignV27.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineCombatV63.h"
#include "WarfareFrontlineCombatV64.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineDifficultyV30.h"
#include "WarfareFrontlineReviewMGV30.h"
#include "WarfareFrontlineArtMetadataV30.inl"
#include "WarfareFrontlinePresentationPolicyV37.inl"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponentBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Fonts/FontCache.h"
#include "EngineFontServices.h"
#include "Dom/JsonObject.h"
namespace FrontlineTargetReuseV834 { TSharedPtr<FJsonObject> Review(); }
namespace FrontlineSupportReuseV834 { TSharedPtr<FJsonObject> Review(); }
#include "CoreGlobals.h"
#include "DynamicRHI.h"
#include "RHI.h"
#include "RenderTimer.h"
#include "RenderCommandFence.h"
#include "SceneView.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformMemory.h"
#include "Misc/Crc.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/Canvas.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Internationalization/BreakIterator.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Sound/SoundWave.h"
#include "WarfareFrontlineMapSceneV61.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#include "BatchedElements.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "HDRHelper.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "SimpleElementShaders.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "Math/ScaleMatrix.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "TextureResource.h"
#include "UObject/GCObject.h"
#include <limits>

// Use the pixel entry paired with FSimpleElementVS. FScreenPS is not compatible:
// its TEXCOORD0 uses noperspective interpolation, unlike SimpleElementVS.
// This binding reuses the engine's existing HLSL, with no new image/material.
class FFrontlineCanvasCoveragePS final : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFrontlineCanvasCoveragePS);
public:
    FFrontlineCanvasCoveragePS() = default;
    explicit FFrontlineCanvasCoveragePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"));
        InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));
        TextureComponentReplicate.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicate"));
        TextureComponentReplicateAlpha.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicateAlpha"));
    }
    void SetParameters(FRHIBatchedShaderParameters& Parameters,const FTexture* Texture)
    {
        SetTextureParameter(Parameters,InTexture,InTextureSampler,Texture);
        SetShaderValue(Parameters,TextureComponentReplicate,FVector4f(0,0,0,0));
        SetShaderValue(Parameters,TextureComponentReplicateAlpha,FVector4f(0,0,0,1));
    }
private:
    LAYOUT_FIELD(FShaderResourceParameter,InTexture);
    LAYOUT_FIELD(FShaderResourceParameter,InTextureSampler);
    LAYOUT_FIELD(FShaderParameter,TextureComponentReplicate);
    LAYOUT_FIELD(FShaderParameter,TextureComponentReplicateAlpha);
};
IMPLEMENT_GLOBAL_SHADER(FFrontlineCanvasCoveragePS,"/Engine/Private/SimpleElementPixelShader.usf","Main",SF_Pixel);
// Flag color and torn-cloth coverage are evaluated by one pixel invocation.
// No white alpha repair can fill a hole back in on the HDR Slate texture.
class FFrontlineFlagPS final : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFrontlineFlagPS);
public:
    FFrontlineFlagPS()=default;
    explicit FFrontlineFlagPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FGlobalShader(Initializer)
    {
        FlagCloth.Bind(Initializer.ParameterMap,TEXT("FlagCloth"));FlagClothSampler.Bind(Initializer.ParameterMap,TEXT("FlagClothSampler"));
        Absorption.Bind(Initializer.ParameterMap,TEXT("Absorption"));AbsorptionSampler.Bind(Initializer.ParameterMap,TEXT("AbsorptionSampler"));
        Spill.Bind(Initializer.ParameterMap,TEXT("Spill"));SpillSampler.Bind(Initializer.ParameterMap,TEXT("SpillSampler"));
        StainState.Bind(Initializer.ParameterMap,TEXT("StainState"));ClothTime.Bind(Initializer.ParameterMap,TEXT("ClothTime"));SplatterSeed.Bind(Initializer.ParameterMap,TEXT("SplatterSeed"));
    }
    void SetParameters(FRHIBatchedShaderParameters& Parameters,const FTexture* Flag,const FTexture* Mask,const FTexture* Liquid,const FVector4f& State,float Time,float Seed)
    {
        SetTextureParameter(Parameters,FlagCloth,FlagClothSampler,Flag);SetTextureParameter(Parameters,Absorption,AbsorptionSampler,Mask);SetTextureParameter(Parameters,Spill,SpillSampler,Liquid);
        SetShaderValue(Parameters,StainState,State);SetShaderValue(Parameters,ClothTime,Time);SetShaderValue(Parameters,SplatterSeed,Seed);
    }
private:
    LAYOUT_FIELD(FShaderResourceParameter,FlagCloth);LAYOUT_FIELD(FShaderResourceParameter,FlagClothSampler);
    LAYOUT_FIELD(FShaderResourceParameter,Absorption);LAYOUT_FIELD(FShaderResourceParameter,AbsorptionSampler);
    LAYOUT_FIELD(FShaderResourceParameter,Spill);LAYOUT_FIELD(FShaderResourceParameter,SpillSampler);
    LAYOUT_FIELD(FShaderParameter,StainState);LAYOUT_FIELD(FShaderParameter,ClothTime);LAYOUT_FIELD(FShaderParameter,SplatterSeed);
};
IMPLEMENT_GLOBAL_SHADER(FFrontlineFlagPS,"/Project/Private/WarfareFlagV33.usf","Main",SF_Pixel);


namespace
{
const FLinearColor Paper(.88f,.85f,.77f), Muted(.57f,.62f,.64f), Dim(.22f,.29f,.30f);
FLinearColor NationalConsolePaintV8213(int32 Nation)
{
    static const FLinearColor Paints[]={FLinearColor(.44f,.47f,.33f),FLinearColor(.35f,.45f,.50f),FLinearColor(.38f,.52f,.67f),FLinearColor(.58f,.43f,.25f)};
    return Paints[FMath::Clamp(Nation,0,3)];
}
const FLinearColor NoticeInfoV75(.78f,.66f,.41f),NoticeWarningV75(.89f,.54f,.24f),NoticeErrorV75(.82f,.30f,.25f);
FLinearColor NoticeColourV75(const FString& Message)
{
    if(Message.Contains(TEXT("失败"))||Message.Contains(TEXT("错误"))||Message.Contains(TEXT("误伤"))||Message.Contains(TEXT("失守"))||Message.Contains(TEXT("failed"))||Message.Contains(TEXT("error"))||Message.Contains(TEXT("friendly fire")))return NoticeErrorV75;
    if(Message.Contains(TEXT("未"))||Message.Contains(TEXT("不能"))||Message.Contains(TEXT("等待"))||Message.Contains(TEXT("准备中"))||Message.Contains(TEXT("unavailable"))||Message.Contains(TEXT("cannot"))||Message.Contains(TEXT("rearming")))return NoticeWarningV75;
    return NoticeInfoV75;
}
const FLinearColor Allied(.55f,.72f,.75f), Opposing(.76f,.44f,.37f), Amber(.73f,.60f,.38f);
const FLinearColor Panel(.018f,.024f,.028f,.98f), Border(.075f,.090f,.10f,.75f);
constexpr float DockTop=852.f;
#include "WarfareFrontlineEtchedGlyphs.inl"
#include "WarfareFrontlineControlGlyphsV31.inl"
TWeakObjectPtr<AHUD> LastUIInputOwner;
uint64 LastUIInputFrame=MAX_uint64;

// A national illustration may be a whole painting or a selected atlas cell.
// The binding is presentation-only: saved role and technology identities stay intact.
struct FFrontlineArtRegionV71
{
    TObjectPtr<UTexture2D> Texture=nullptr;
    int32 Slot=INDEX_NONE,Columns=1,Rows=1;
    FIntRect ContentRectV87{0,0,0,0}; // Optional measured content, in source pixels.
};
struct FFrontlineArtwork : FGCObject
{
    TObjectPtr<UTexture2D> Keyart[2]={},BasicPortraits[6]={},AchievementBadgesV76[FrontlineAchievementsV76::Count]={};
    TObjectPtr<UTexture2D> IllustrationsV32=nullptr,OperationsV32=nullptr,ArchivePhotosV32=nullptr;
    TObjectPtr<UTexture2D> LateMapsV32[4]={};
    TObjectPtr<UTexture2D> CommandSurface=nullptr,CommandPaper=nullptr;
    TObjectPtr<UTexture2D> ConsoleV79[4][13]={};
    TObjectPtr<UMaterialInterface> ConsolePigmentV8213=nullptr;
    TObjectPtr<UMaterialInstanceDynamic> ConsolePigmentsV8213[4][3]={};
    TObjectPtr<UTexture2D> FRRUArmourAtlasV825=nullptr,FRRUArmourResearchAtlasV87=nullptr;
    bool NationalConsoleReadyV82[4]={true,true,true,true};
    TObjectPtr<UTexture2D> MapsV30[6]={},CommandersV30[2]={},GermanBackgroundV30=nullptr,EmblemsV30=nullptr;
    TObjectPtr<UTexture2D> CommandSkinV31=nullptr,ControlFontV31=nullptr,CambraiCounterV31=nullptr;
    TObjectPtr<UTexture2D> ArchivePhotosV35[4]={};
    TObjectPtr<UTexture2D> CommandersV37=nullptr;
    TObjectPtr<UMaterialInterface> LockCutoutV35=nullptr;
    TObjectPtr<UFont> Stencil=nullptr,ControlRuntimeV62=nullptr;
    TObjectPtr<UFont> ControlSemiBoldV8210,TextV8210,TextMediumV8210,TextSemiBoldV8210,TextItalicV8210,TypedV8210,StencilV8210,KaiV8210,BlackletterV8210;
    TObjectPtr<UFont> RegularMixedV8210,MediumMixedV8210,KaiMixedV8210,MediumControlMixedV8210;
    TObjectPtr<UTexture2D> NVIDIAOfficial=nullptr;
    TObjectPtr<UTexture2D> TechnologyPaintings[2][16]={},SupportPaintings[6]={},SpecialistPortraits[4]={},Flags[2]={},UpgradedPortraits[2]={};
    TObjectPtr<UTexture2D> OfficerPortraits[2]={};
    TObjectPtr<UTexture2D> SupportPaintingsV70[2][6]={};
    FFrontlineArtRegionV71 RolePaintingsV71[2][int32(EFrontlineRole::Count)]={};
    // Scoped marksman, stormtrooper and commissioned officer respectively.
    FFrontlineArtRegionV71 RoleVariantsV71[2][3]={};
    FFrontlineArtRegionV71 TechnologyPaintingsV71[2][28]={};
    FFrontlineArtRegionV71 ArmourTechnologyPaintingsV71[2][9]={};
    FFrontlineArtRegionV71 SupportPaintingsV71[2][int32(EFrontlineSupport::Count)]={};
    FFrontlineArtRegionV71 MenuCoversV71[2]={},BackgroundsV71[2]={};
    TObjectPtr<UTexture2D> BrandWordmarkV72=nullptr,BrandEmblemV72=nullptr;
    FFrontlineArtRegionV71 MenuCoversV72[2]={},RolePaintingsV72[2][int32(EFrontlineRole::Count)]={},RoleVariantsV72[2][3]={};
    TObjectPtr<UMaterialInterface> ArchivePhotoMaterialV72=nullptr;
    TMap<int32,TObjectPtr<UMaterialInstanceDynamic>> ArchivePhotosV72;
    FFrontlineArtRegionV71 ArchivePaintingsV74[2][37]={};
    TObjectPtr<UTexture2D> DeskV74=nullptr,FieldBoardV74=nullptr,OrderCardV74=nullptr,CommandKeyV74=nullptr,IndexTabV74=nullptr;
    TObjectPtr<UMaterialInterface> PaintingMaterialV73=nullptr;
    TMap<UTexture2D*,TObjectPtr<UMaterialInstanceDynamic>> PaintingsV73;
    TMap<UTexture2D*,bool> PaintingClassificationV73;
    TObjectPtr<UMaterialInterface> FlagClothMaterial=nullptr;
    TObjectPtr<UTexture2D> FlagAbsorption=nullptr,FlagSplashesV32=nullptr;
    TObjectPtr<UMaterialInterface> GrayscalePhotoMaterial=nullptr;
    TObjectPtr<UMaterialInterface> MedalLockedMaterialV8217=nullptr;
    TMap<UTexture2D*,TObjectPtr<UMaterialInstanceDynamic>> GrayscalePhotos;
    TObjectPtr<UMaterialInstanceDynamic> CasualtyFlags[2]={};
    TObjectPtr<UMaterialInstanceDynamic> ReviewFlags[8]={};
    float FlagLosses[2]={},FlagTime[2]={};
    float FlagTargetLosses[2]={},FlagPreviousLosses[2]={};
    double FlagUpdatedAt[2]={},FlagImpactAt[2]={};
    uint64 FlagDrawFrame[2]={MAX_uint64,MAX_uint64};
    TObjectPtr<UMaterialInterface> EtchedMaterial=nullptr;
    TObjectPtr<UTexture2D> EtchedTitleTexture=nullptr;
    TObjectPtr<UTexture2D> EtchedGlyphTexture=nullptr;
    TObjectPtr<UMaterialInstanceDynamic> EtchedTints[14]={};
    TObjectPtr<UFont> Sans=nullptr; TObjectPtr<UFont> Serif=nullptr;
    TObjectPtr<UFont> ChineseMediumV45=nullptr;
    FFrontlineArtwork()
    {
        auto Texture=[](const TCHAR* Name) { return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V09/%s.%s"),Name,Name)); };
        Keyart[0]=Texture(TEXT("T_Keyart1917")); Keyart[1]=Texture(TEXT("T_Keyart1944"));
        auto V32=[](const TCHAR* Name){return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V32/%s.%s"),Name,Name));};
        const TCHAR* V35Photos[]={TEXT("Relief"),TEXT("Gunline"),TEXT("Armour"),TEXT("Aftermath")};
        CommandersV37=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V37/T_Commanders_Late_V37.T_Commanders_Late_V37"));
        for(int32 I=0;I<4;++I)ArchivePhotosV35[I]=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V35/T_Archive_%s_V35.T_Archive_%s_V35"),V35Photos[I],V35Photos[I]));
        LockCutoutV35=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V38/M_LockCoverage_V38.M_LockCoverage_V38"));
        // Legacy reviews retain this reference. Runtime locks use the native
        // V39 RGBA renderer; never fall back to brightness-keyed transparency.
        FlagSplashesV32=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V33/T_FlagSplashes_V33.T_FlagSplashes_V33"));IllustrationsV32=V32(TEXT("T_Illustrations_V32"));ArchivePhotosV32=V32(TEXT("T_ArchivePhotos_V32"));
        const TCHAR* LateMapNames[]={TEXT("Scarpe"),TEXT("Hindenburg"),TEXT("Selle"),TEXT("Sambre")};
        for(int32 I=0;I<4;++I)LateMapsV32[I]=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V32/Maps/T_Map_%s_V32.T_Map_%s_V32"),LateMapNames[I],LateMapNames[I]));
        auto V30=[](const TCHAR* Name){return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V30/%s.%s"),Name,Name));};
        const TCHAR* Maps[]={TEXT("T_Map_Arras"),TEXT("T_Map_Messines"),TEXT("T_Map_Ypres"),TEXT("T_Map_Cambrai"),TEXT("T_Map_Michael"),TEXT("T_Map_Amiens")};
        for(int32 I=0;I<6;++I)MapsV30[I]=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V31/Maps/%s.%s"),Maps[I],Maps[I]));
        CambraiCounterV31=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V31/Maps/T_Map_Cambrai_Counterattack.T_Map_Cambrai_Counterattack"));
        CommandSkinV31=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V33/T_CommandSkin_V33.T_CommandSkin_V33"));
        ControlFontV31=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V31/T_ControlGlyphs_V31.T_ControlGlyphs_V31"));
        NVIDIAOfficial=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/Distribution/T_NVIDIA_Official.T_NVIDIA_Official"));
        CommandersV30[0]=V30(TEXT("T_Commanders_British"));CommandersV30[1]=V30(TEXT("T_Commanders_German"));
        GermanBackgroundV30=V30(TEXT("T_Background_German"));EmblemsV30=V30(TEXT("T_Emblems"));
        CommandSurface=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V16/T_ConsoleIron_V16.T_ConsoleIron_V16"));
        CommandPaper=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V16/T_CommandPaper_V16.T_CommandPaper_V16"));
        ControlRuntimeV62=LoadObject<UFont>(nullptr,TEXT("/Game/Warfare/UI/V62/F_Control.F_Control"));
        Stencil=LoadObject<UFont>(nullptr,TEXT("/Game/Warfare/UI/V13/F_Stencil.F_Stencil"));
        auto PaintingV15=[](const FString& Name) { return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V15/%s.%s"),*Name,*Name)); };
        const TCHAR* PortraitNames[]={TEXT("T_Portrait_British_Rifle"),TEXT("T_Portrait_British_Lewis"),TEXT("T_Portrait_British_Mills"),TEXT("T_Portrait_German_Rifle"),TEXT("T_Portrait_German_MG"),TEXT("T_Portrait_German_Assault")};
        auto PortraitV16=[](const TCHAR* Name) {return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V16/%s.%s"),Name,Name));};
        for (int32 I=0;I<6;++I) BasicPortraits[I]=PortraitV16(PortraitNames[I]);
        EtchedMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V30/M_EtchedLabel_V30.M_EtchedLabel_V30"));
        if(!EtchedMaterial)EtchedMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V15/M_EtchedLabel_V15.M_EtchedLabel_V15"));
        EtchedTitleTexture=V30(TEXT("T_EtchedTitleRelief"));
        EtchedGlyphTexture=V30(TEXT("T_EtchedGlyphRelief"));
        auto Painting=[](const TCHAR* Name) { return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V14/%s.%s"),Name,Name)); };
        for (int32 F=0;F<2;++F) for (int32 I=0;I<12;++I)
            TechnologyPaintings[F][I]=PaintingV15(FString::Printf(TEXT("T_Tech_%s_%02d"),F?TEXT("German"):TEXT("British"),I));
        auto PaintingV23=[](const FString& Name){return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V23/%s.%s"),*Name,*Name));};
        TechnologyPaintings[0][8]=PaintingV23(TEXT("T_Tech_British_08"));
        for(int32 F=0;F<2;++F)for(int32 I=12;I<16;++I)
            TechnologyPaintings[F][I]=PaintingV23(FString::Printf(TEXT("T_Tech_%s_%02d"),F?TEXT("German"):TEXT("British"),I));
        UpgradedPortraits[0]=PaintingV23(TEXT("T_Portrait_British_Sniper_V23"));
        UpgradedPortraits[1]=PaintingV23(TEXT("T_Portrait_German_Stormtrooper_V23"));
        const TCHAR* SupportNames[]={TEXT("T_Support_Artillery"),TEXT("T_Support_Smoke"),TEXT("T_Support_Mortar")};
        for (int32 I=0;I<3;++I) SupportPaintings[I]=Painting(SupportNames[I]);
        const TCHAR* NewSupportNames[]={TEXT("T_Support_Gas"),TEXT("T_Support_AntiTank"),TEXT("T_Support_AirStrike")};
        for(int32 I=0;I<3;++I)SupportPaintings[I+3]=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V28/%s.%s"),NewSupportNames[I],NewSupportNames[I]));
        // Authored national identities: the old shared AT card depicted a French crew.
        const TCHAR* SupportV70[][3]={
            {TEXT("British"),TEXT("AntiTank"),TEXT("4")},
            {TEXT("German"),TEXT("AntiTank"),TEXT("4")},
            {TEXT("German"),TEXT("Mortar"),TEXT("2")},
            {TEXT("German"),TEXT("Gas"),TEXT("3")},
            {TEXT("German"),TEXT("AirStrike"),TEXT("5")}};
        for(const auto& Entry:SupportV70)
        {
            const int32 F=FCString::Strcmp(Entry[0],TEXT("German"))==0?1:0;
            const FString Name=FString::Printf(TEXT("T_Support_%s_%s_V70"),Entry[0],Entry[1]);
            SupportPaintingsV70[F][FCString::Atoi(Entry[2])]=LoadObject<UTexture2D>(nullptr,
                *FString::Printf(TEXT("/Game/Warfare/UI/V70/%s.%s"),*Name,*Name));
        }
        const TCHAR* SpecialistNames[]={TEXT("T_Portrait_British_Sniper"),TEXT("T_Portrait_British_Officer"),TEXT("T_Portrait_German_Sniper"),TEXT("T_Portrait_German_Officer")};
        for (int32 I=0;I<4;++I) SpecialistPortraits[I]=PortraitV16(SpecialistNames[I]);
        Flags[0]=Painting(TEXT("T_Flag_British")); Flags[1]=Painting(TEXT("T_Flag_German"));
        for(int32 F=0;F<2;++F)
        {
            const FString Name=FString::Printf(TEXT("T_Portrait_%s_Officer_V24"),F?TEXT("German"):TEXT("British"));
            OfficerPortraits[F]=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V24/Portraits/%s.%s"),*Name,*Name));
        }
        TechnologyPaintings[0][9]=OfficerPortraits[0];TechnologyPaintings[1][6]=OfficerPortraits[1];
        FlagClothMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V26/Flags/M_CasualtyFlag_V26.M_CasualtyFlag_V26"));
        FlagAbsorption=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V24/Flags/T_FlagAbsorption_V24.T_FlagAbsorption_V24"));
        GrayscalePhotoMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V26/Photos/M_GrayscalePhoto_V26.M_GrayscalePhoto_V26"));
        MedalLockedMaterialV8217=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V8218/M_MedalLocked_V8218.M_MedalLocked_V8218"));
        Sans=LoadObject<UFont>(nullptr,TEXT("/Game/Warfare/UI/V09/F_UI.F_UI"));
        const TCHAR* ConsolePartsV79[]={TEXT("Case"),TEXT("Well"),TEXT("Key"),TEXT("Tab"),TEXT("Field"),TEXT("Gauge"),TEXT("Fill"),TEXT("Lock"),TEXT("Settings"),TEXT("Manual"),TEXT("Research")};
        for(int32 F=0;F<2;++F)for(int32 Part=0;Part<11;++Part)
        {
            const FString Name=FString::Printf(TEXT("T_Console_%s_%s_V79"),F?TEXT("German"):TEXT("British"),ConsolePartsV79[Part]);
            ConsoleV79[F][Part]=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V79/%s.%s"),*Name,*Name));
        }
        const TCHAR* ConsolePartsV80[]={TEXT("Case"),TEXT("Well"),TEXT("Key"),TEXT("Tab"),TEXT("Field"),TEXT("Crest"),TEXT("Terminal")};
        for(int32 F=0;F<2;++F)for(int32 P=0;P<7;++P)
        {
            const FString Name=FString::Printf(TEXT("T_Console_%s_%s_V80"),F?TEXT("German"):TEXT("British"),ConsolePartsV80[P]);
            auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V80/%s.%s"),*Name,*Name));
            if(T)ConsoleV79[F][P<5?P:P+6]=T;
        }
        const TCHAR* ConsolePartsV81[]={TEXT("Case"),TEXT("Gauge"),TEXT("Fill")};
        for(int32 F=0;F<2;++F)for(int32 P=0;P<3;++P)
        {
            const FString Name=FString::Printf(TEXT("T_Console_%s_%s_V81"),F?TEXT("German"):TEXT("British"),ConsolePartsV81[P]);
            if(auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V81/%s.%s"),*Name,*Name)))ConsoleV79[F][P==0?0:P+4]=T;
        }
        const TCHAR* NationalConsoleNamesV82[]={TEXT("French"),TEXT("Russian")};
        const TCHAR* NationalConsolePartsV82[]={TEXT("Case"),TEXT("Well"),TEXT("Key"),TEXT("Tab"),TEXT("Field"),TEXT("Gauge"),TEXT("Fill"),TEXT("Lock"),TEXT("Settings"),TEXT("Manual"),TEXT("Research"),TEXT("Crest"),TEXT("Terminal")};
        for(int32 F=2;F<4;++F)for(int32 P=0;P<13;++P)
        {
            ConsoleV79[F][P]=nullptr;
            if(P>=5&&P!=11)continue;
            const FString Name=FString::Printf(TEXT("T_Console_%s_%s_V82"),NationalConsoleNamesV82[F-2],NationalConsolePartsV82[P]);
            auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/UI/Console/%s.%s"),*Name,*Name));
            if(T)ConsoleV79[F][P]=T;
            if(P<5||P==11)NationalConsoleReadyV82[F]&=T!=nullptr;
        }
        auto LoadUIV825=[](const FString& Name){return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V825/%s.%s"),*Name,*Name));};
        for(int32 F=2;F<4;++F)for(int32 P=5;P<13;++P)
        {
            const FString Name=FString::Printf(TEXT("T_Console_%s_%s_V825"),NationalConsoleNamesV82[F-2],NationalConsolePartsV82[P]);
            ConsoleV79[F][P]=LoadUIV825(Name);NationalConsoleReadyV82[F]&=ConsoleV79[F][P]!=nullptr;
        }
        ConsoleV79[1][8]=LoadUIV825(TEXT("T_Console_German_Settings_V825"));
        // A field chronometer belongs beside the battle clock. Its metalwork
        // follows each console, replacing the unrelated electrical terminal.
        ConsoleV79[0][12]=LoadUIV825(TEXT("T_Console_French_Terminal_V825"));
        ConsoleV79[1][12]=LoadUIV825(TEXT("T_Console_Russian_Terminal_V825"));
        FRRUArmourAtlasV825=LoadUIV825(TEXT("T_Armour_FRRU_V825"));
        FRRUArmourResearchAtlasV87=FrontlineUIAssetsV8213::Texture(TEXT("/Game/FRRUIntegration/FormalV82/UI/T_ArmourResearch_FRRU_V87.T_ArmourResearch_FRRU_V87"));
        ConsolePigmentV8213=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V8213/M_ConsolePigment_V8213.M_ConsolePigment_V8213"));
        const float PaintGainsV8213[]={2.2f,1.1f,1.4f};
        for(int32 Nation=0;Nation<4;++Nation)if(ConsolePigmentV8213&&ConsoleV79[Nation][6])
        {
            for(int32 Paint=0;Paint<3;++Paint)
            {
                const FLinearColor Ink=Paint?FLinearColor(.54f,.20f,.19f):NationalConsolePaintV8213(Nation);
                const float Peak=FMath::Max3(Ink.R,Ink.G,Ink.B);
                auto* M=UMaterialInstanceDynamic::Create(ConsolePigmentV8213,GetTransientPackage());
                M->SetTextureParameterValue(TEXT("Photo"),ConsoleV79[Nation][6]);
                M->SetVectorParameterValue(TEXT("Pigment"),FLinearColor(Ink.R/Peak,Ink.G/Peak,Ink.B/Peak,1));
                M->SetScalarParameterValue(TEXT("PaintGain"),PaintGainsV8213[Paint]);
                ConsolePigmentsV8213[Nation][Paint]=M;
            }
        }
        ChineseMediumV45=LoadObject<UFont>(nullptr,TEXT("/Game/Warfare/UI/V45/F_UIChinese.F_UIChinese"));
        Serif=LoadObject<UFont>(nullptr,TEXT("/Game/Warfare/UI/V09/F_Title.F_Title"));
        auto TypographyFontV8210=[](const TCHAR* Name){return LoadObject<UFont>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V8210/%s.%s"),Name,Name));};
        ControlSemiBoldV8210=TypographyFontV8210(TEXT("F_ControlSemiBold"));
        TextV8210=TypographyFontV8210(TEXT("F_Text"));TextMediumV8210=TypographyFontV8210(TEXT("F_TextMedium"));
        TextSemiBoldV8210=TypographyFontV8210(TEXT("F_TextSemiBold"));TextItalicV8210=TypographyFontV8210(TEXT("F_TextItalic"));
        TypedV8210=TypographyFontV8210(TEXT("F_Typed"));StencilV8210=TypographyFontV8210(TEXT("F_StencilUI"));
        KaiV8210=TypographyFontV8210(TEXT("F_Kai"));BlackletterV8210=TypographyFontV8210(TEXT("F_Blackletter"));
        auto MixedFontV8210=[&](UFont* CJK,UFont* Latin,const TCHAR* Name)->UFont*
        {
            if(!CJK||!Latin)return CJK;
            auto* Result=DuplicateObject<UFont>(CJK,GetTransientPackage(),FName(Name));
            auto& Composite=Result->GetMutableInternalCompositeFont();
            FCompositeSubFont Pair;Pair.Typeface=Latin->GetInternalCompositeFont().DefaultTypeface;
            Pair.CharacterRanges.Add(FInt32Range::Inclusive(0x0020,0x024f));
            Pair.CharacterRanges.Add(FInt32Range::Inclusive(0x0400,0x052f));
            Pair.CharacterRanges.Add(FInt32Range::Inclusive(0x2000,0x206f));
            Composite.SubTypefaces.Insert(Pair,0);
            return Result;
        };
        RegularMixedV8210=MixedFontV8210(Sans,TextV8210,TEXT("RegularMixedV8210"));
        MediumMixedV8210=MixedFontV8210(ChineseMediumV45,TextMediumV8210,TEXT("MediumMixedV8210"));
        MediumControlMixedV8210=MixedFontV8210(ChineseMediumV45,ControlRuntimeV62,TEXT("MediumControlMixedV8210"));
        KaiMixedV8210=MixedFontV8210(KaiV8210,TextItalicV8210,TEXT("KaiMixedV8210"));
        // Runtime symbol fallback is additive; authored assets stay untouched.
        if(Sans)for(UFont* Face:{ControlSemiBoldV8210.Get(),TextV8210.Get(),TextMediumV8210.Get(),TextSemiBoldV8210.Get(),TextItalicV8210.Get(),TypedV8210.Get(),StencilV8210.Get()})
            if(Face){FCompositeSubFont Symbols;Symbols.Typeface=Sans->GetInternalCompositeFont().DefaultTypeface;Symbols.CharacterRanges.Add(FInt32Range::Inclusive(0x2190,0x21ff));Face->GetMutableInternalCompositeFont().SubTypefaces.Insert(Symbols,0);}

        auto BindV71=[](FFrontlineArtRegionV71& Region,const FString& Name,int32 Slot=INDEX_NONE,int32 Columns=1,int32 Rows=1)
        {
            auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V71/%s.%s"),*Name,*Name));
            if(T)Region={T,Slot,Columns,Rows};
        };
        struct FRoleBindingV71 {EFrontlineRole Role;const TCHAR* Suffix;};
        const FRoleBindingV71 GermanRoles[]={
            {EFrontlineRole::Rifle,TEXT("Rifle")},{EFrontlineRole::Lewis,TEXT("MG")},
            {EFrontlineRole::Mills,TEXT("Assault")},{EFrontlineRole::Marksman,TEXT("Marksman")},
            {EFrontlineRole::Officer,TEXT("NCO")},{EFrontlineRole::Flamethrower,TEXT("Flamethrower")},
            {EFrontlineRole::Pioneer,TEXT("Pioneer")},{EFrontlineRole::AntiTankRifle,TEXT("AntiTankRifle")},
            {EFrontlineRole::MobileGun,TEXT("MobileGun")},{EFrontlineRole::Signals,TEXT("Signals")}};
        for(const auto& Entry:GermanRoles)
            BindV71(RolePaintingsV71[1][int32(Entry.Role)],FString::Printf(TEXT("T_Portrait_German_%s_V71"),Entry.Suffix));
        BindV71(RoleVariantsV71[1][1],TEXT("T_Portrait_German_Stormtrooper_V71"));
        BindV71(RoleVariantsV71[1][2],TEXT("T_Portrait_German_Officer_V71"));
        BindV71(RoleVariantsV71[0][2],TEXT("T_Portrait_British_Officer_V71"));
        BindV71(RolePaintingsV71[0][int32(EFrontlineRole::MobileGun)],TEXT("T_Portrait_British_MobileGun_V71"));
        BindV71(RolePaintingsV71[0][int32(EFrontlineRole::Signals)],TEXT("T_Portrait_British_Signals_V71"));
        struct FSupportBindingV71 {int32 Faction;EFrontlineSupport Support;const TCHAR* Name;};
        const FSupportBindingV71 NationalSupport[]={
            {0,EFrontlineSupport::AntiTank,TEXT("T_Support_British_AntiTank_V71")},
            {1,EFrontlineSupport::AntiTank,TEXT("T_Support_German_AntiTank_V71")},
            {1,EFrontlineSupport::Mortar,TEXT("T_Support_German_Mortar_V71")},
            {1,EFrontlineSupport::Artillery,TEXT("T_Support_German_Artillery_V71")},
            {1,EFrontlineSupport::AirStrike,TEXT("T_Support_German_AirStrike_V71")},
            {1,EFrontlineSupport::AirRecon,TEXT("T_Support_German_Shrapnel_V71")}};
        for(const auto& Entry:NationalSupport)BindV71(SupportPaintingsV71[Entry.Faction][int32(Entry.Support)],Entry.Name);
        const int32 TechnologyIds[4][4]={{0,1,2,3},{4,5,7,8},{9,10,11,13},{14,15,24,26}};
        for(int32 Sheet=0;Sheet<4;++Sheet)for(int32 Cell=0;Cell<4;++Cell)
            BindV71(TechnologyPaintingsV71[1][TechnologyIds[Sheet][Cell]],FString::Printf(TEXT("T_Tech_German_%c_V71"),TCHAR('A'+Sheet)),Cell,2,2);
        TechnologyPaintingsV71[1][6]=RoleVariantsV71[1][2];
        TechnologyPaintingsV71[1][12]=RoleVariantsV71[1][1];
        TechnologyPaintingsV71[0][9]=RoleVariantsV71[0][2];
        const int32 ArmourIds[]={3,4,6,7};
        for(int32 Cell=0;Cell<4;++Cell)BindV71(ArmourTechnologyPaintingsV71[1][ArmourIds[Cell]],TEXT("T_ArmourTech_German_V71"),Cell,2,2);
        BindV71(MenuCoversV71[1],TEXT("T_MainMenu_German_V71"));
        BindV71(BackgroundsV71[1],TEXT("T_Background_German_V71"));
        auto LoadV72=[](const FString& Name){return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V72/%s.%s"),*Name,*Name),nullptr,LOAD_NoWarn);};
        BrandWordmarkV72=LoadV72(TEXT("T_Brand_Wordmark_V72"));BrandEmblemV72=LoadV72(TEXT("T_Brand_Emblem_V72"));
        MenuCoversV72[0]={LoadV72(TEXT("T_MainMenu_British_V72"))};MenuCoversV72[1]={LoadV72(TEXT("T_MainMenu_German_V72"))};
        for(const auto& Entry:GermanRoles)RolePaintingsV72[1][int32(Entry.Role)]={LoadV72(FString::Printf(TEXT("T_Portrait_German_%s_V72"),Entry.Suffix))};
        RoleVariantsV72[1][1]={LoadV72(TEXT("T_Portrait_German_Stormtrooper_V72"))};RoleVariantsV72[1][2]={LoadV72(TEXT("T_Portrait_German_Officer_V72"))};
        RoleVariantsV72[0][2]={LoadV72(TEXT("T_Portrait_British_Officer_V72"))};
        RolePaintingsV72[0][int32(EFrontlineRole::MobileGun)]={LoadV72(TEXT("T_Portrait_British_MobileGun_V72"))};
        RolePaintingsV72[0][int32(EFrontlineRole::Signals)]={LoadV72(TEXT("T_Portrait_British_Signals_V72"))};
        auto LoadV73=[](const FString& Name){return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/UI/V73/%s.%s"),*Name,*Name),nullptr,LOAD_NoWarn);};
        BrandWordmarkV72=LoadV73(TEXT("T_Brand_Wordmark_V73"));BrandEmblemV72=LoadV73(TEXT("T_Brand_Emblem_V73"));
        for(const auto& Entry:GermanRoles)
            if(Entry.Role!=EFrontlineRole::Officer)
                RolePaintingsV72[1][int32(Entry.Role)]={LoadV73(FString::Printf(TEXT("T_Portrait_German_%s_V73"),Entry.Suffix))};
        RoleVariantsV72[1][1]={LoadV73(TEXT("T_Portrait_German_Stormtrooper_V73"))};
        ArchivePhotoMaterialV72=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V84413/M_ResearchPhoto_V84413.M_ResearchPhoto_V84413"),nullptr,LOAD_NoWarn);
        if(!ArchivePhotoMaterialV72)ArchivePhotoMaterialV72=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V833/M_ArchivePhoto_V833.M_ArchivePhoto_V833"),nullptr,LOAD_NoWarn);
        PaintingMaterialV73=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/UI/V75/M_CinematicPainting_V75.M_CinematicPainting_V75"),nullptr,LOAD_NoWarn);
        #include "WarfareFrontlineArchiveBindingsV75.inl"
        #include "WarfareFrontlineArtworkBindingsV8215.inl"
#include "WarfareFrontlineArtworkBindingsV851.inl"
#include "WarfareFrontlineArtworkBindingsV86.inl"
        auto RegisterPainting=[&](UTexture2D* T){if(T)PaintingClassificationV73.Add(T,true);};
        RegisterPainting(FRRUArmourAtlasV825);
        for(auto T:Keyart)RegisterPainting(T);for(auto T:BasicPortraits)RegisterPainting(T);
        RegisterPainting(IllustrationsV32);RegisterPainting(ArchivePhotosV32);RegisterPainting(GermanBackgroundV30);RegisterPainting(CommandersV37);
        for(auto T:ArchivePhotosV35)RegisterPainting(T);for(auto T:CommandersV30)RegisterPainting(T);
        for(auto& Nation:TechnologyPaintings)for(auto T:Nation)RegisterPainting(T);
        for(auto T:SupportPaintings)RegisterPainting(T);for(auto T:SpecialistPortraits)RegisterPainting(T);
        for(auto T:UpgradedPortraits)RegisterPainting(T);for(auto T:OfficerPortraits)RegisterPainting(T);
        for(auto& Nation:SupportPaintingsV70)for(auto T:Nation)RegisterPainting(T);
        for(auto& Nation:RolePaintingsV71)for(auto& Art:Nation)RegisterPainting(Art.Texture);
        for(auto& Nation:RoleVariantsV71)for(auto& Art:Nation)RegisterPainting(Art.Texture);
        for(auto& Nation:TechnologyPaintingsV71)for(auto& Art:Nation)RegisterPainting(Art.Texture);
        for(auto& Nation:ArmourTechnologyPaintingsV71)for(auto& Art:Nation)RegisterPainting(Art.Texture);
        for(auto& Nation:SupportPaintingsV71)for(auto& Art:Nation)RegisterPainting(Art.Texture);
        for(auto& Art:MenuCoversV71)RegisterPainting(Art.Texture);for(auto& Art:BackgroundsV71)RegisterPainting(Art.Texture);
        for(auto& Art:MenuCoversV72)RegisterPainting(Art.Texture);
        for(auto& Nation:RolePaintingsV72)for(auto& Art:Nation)RegisterPainting(Art.Texture);
        for(auto& Nation:RoleVariantsV72)for(auto& Art:Nation)RegisterPainting(Art.Texture);
    }
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override
    {
        for(auto& T:AchievementBadgesV76)Collector.AddReferencedObject(T);
        for (auto& T:Keyart) Collector.AddReferencedObject(T);
        for(auto& T:ArchivePhotosV35)Collector.AddReferencedObject(T);
        Collector.AddReferencedObject(CommandersV37);
        Collector.AddReferencedObject(LockCutoutV35);
        Collector.AddReferencedObject(IllustrationsV32);Collector.AddReferencedObject(OperationsV32);Collector.AddReferencedObject(ArchivePhotosV32);
        for(auto& T:LateMapsV32)Collector.AddReferencedObject(T);
        for(auto& T:MapsV30)Collector.AddReferencedObject(T);
        for(auto& T:CommandersV30)Collector.AddReferencedObject(T);
        Collector.AddReferencedObject(CommandSkinV31);Collector.AddReferencedObject(ControlFontV31);Collector.AddReferencedObject(CambraiCounterV31);Collector.AddReferencedObject(ChineseMediumV45);
        Collector.AddReferencedObject(NVIDIAOfficial);
        for(auto& Nation:ConsoleV79)for(auto& T:Nation)Collector.AddReferencedObject(T);
        Collector.AddReferencedObject(ConsolePigmentV8213);
        for(auto& Nation:ConsolePigmentsV8213)for(auto& M:Nation)Collector.AddReferencedObject(M);
        Collector.AddReferencedObject(FRRUArmourAtlasV825);
        Collector.AddReferencedObject(FRRUArmourResearchAtlasV87);
        Collector.AddReferencedObject(GermanBackgroundV30);Collector.AddReferencedObject(EmblemsV30);
        for (auto& Nation:TechnologyPaintings) for (auto& T:Nation) Collector.AddReferencedObject(T);
        for (auto& T:BasicPortraits) Collector.AddReferencedObject(T);
        for (auto& T:EtchedTints) Collector.AddReferencedObject(T);
        Collector.AddReferencedObject(EtchedMaterial);
        Collector.AddReferencedObject(EtchedTitleTexture);
        Collector.AddReferencedObject(EtchedGlyphTexture);
        for (auto& T:SupportPaintings) Collector.AddReferencedObject(T);
        for(auto& Nation:SupportPaintingsV70)for(auto& T:Nation)Collector.AddReferencedObject(T);
        for(auto& Nation:RolePaintingsV71)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Nation:RoleVariantsV71)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Nation:TechnologyPaintingsV71)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Nation:ArmourTechnologyPaintingsV71)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Nation:SupportPaintingsV71)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Art:MenuCoversV71)Collector.AddReferencedObject(Art.Texture);
        for(auto& Art:BackgroundsV71)Collector.AddReferencedObject(Art.Texture);
        Collector.AddReferencedObject(BrandWordmarkV72);Collector.AddReferencedObject(BrandEmblemV72);Collector.AddReferencedObject(ArchivePhotoMaterialV72);
        for(auto& Art:MenuCoversV72)Collector.AddReferencedObject(Art.Texture);
        for(auto& Nation:RolePaintingsV72)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Nation:RoleVariantsV72)for(auto& Art:Nation)Collector.AddReferencedObject(Art.Texture);
        for(auto& Pair:ArchivePhotosV72)Collector.AddReferencedObject(Pair.Value);
        for(auto& Nation:ArchivePaintingsV74)for(auto& Card:Nation)Collector.AddReferencedObject(Card.Texture);
        Collector.AddReferencedObject(DeskV74);Collector.AddReferencedObject(FieldBoardV74);Collector.AddReferencedObject(OrderCardV74);Collector.AddReferencedObject(CommandKeyV74);Collector.AddReferencedObject(IndexTabV74);
        Collector.AddReferencedObject(PaintingMaterialV73);
        for(auto& Pair:PaintingsV73)Collector.AddReferencedObject(Pair.Value);
        for (auto& T:SpecialistPortraits) Collector.AddReferencedObject(T);
        for (auto& T:UpgradedPortraits) Collector.AddReferencedObject(T);
        for (auto& T:Flags) Collector.AddReferencedObject(T);
        for (auto& T:OfficerPortraits) Collector.AddReferencedObject(T);
        for (auto& M:CasualtyFlags) Collector.AddReferencedObject(M);
        for (auto& M:ReviewFlags) Collector.AddReferencedObject(M);
        Collector.AddReferencedObject(FlagClothMaterial);
        Collector.AddReferencedObject(GrayscalePhotoMaterial);Collector.AddReferencedObject(MedalLockedMaterialV8217);Collector.AddReferencedObject(FlagAbsorption);Collector.AddReferencedObject(FlagSplashesV32);
        for(auto& Pair:GrayscalePhotos)Collector.AddReferencedObject(Pair.Value);
        Collector.AddReferencedObject(CommandSurface);
        Collector.AddReferencedObject(CommandPaper);
        Collector.AddReferencedObject(ControlRuntimeV62); Collector.AddReferencedObject(Sans); Collector.AddReferencedObject(Serif);
        Collector.AddReferencedObject(Stencil);
        for(auto* F:{&ControlSemiBoldV8210,&TextV8210,&TextMediumV8210,&TextSemiBoldV8210,&TextItalicV8210,&TypedV8210,&StencilV8210,&KaiV8210,&BlackletterV8210,&RegularMixedV8210,&MediumMixedV8210,&KaiMixedV8210,&MediumControlMixedV8210})Collector.AddReferencedObject(*F);
    }
    UMaterialInstanceDynamic* LabelMaterial(const FLinearColor& Color,bool Large=false)
    {
        if (!EtchedMaterial) return nullptr;
        // Immutable draw palettes avoid a canvas batch observing a later tint.
        const FLinearColor Palette[]={Paper,Muted,Amber,Allied,Opposing,Dim,FLinearColor(.32f,.35f,.36f)};
        int32 Closest=0;float Distance=MAX_flt;
        for (int32 I=0;I<UE_ARRAY_COUNT(Palette);++I)
        { const float D=FMath::Square(Color.R-Palette[I].R)+FMath::Square(Color.G-Palette[I].G)+FMath::Square(Color.B-Palette[I].B); if (D<Distance) { Closest=I;Distance=D; } }
        const int32 Index=Closest+(Large?7:0);
        if (!EtchedTints[Index])
        {
            EtchedTints[Index]=UMaterialInstanceDynamic::Create(EtchedMaterial,GetTransientPackage());
            EtchedTints[Index]->SetVectorParameterValue(TEXT("LetterMetal"),Palette[Closest]);
            EtchedTints[Index]->SetScalarParameterValue(TEXT("Wear"),Large?.65f:0.f);
            if(Large && EtchedTitleTexture)EtchedTints[Index]->SetTextureParameterValue(TEXT("GlyphRelief"),EtchedTitleTexture);
            else if(EtchedGlyphTexture)EtchedTints[Index]->SetTextureParameterValue(TEXT("GlyphRelief"),EtchedGlyphTexture);
        }
        return EtchedTints[Index];
    }
    virtual FString GetReferencerName() const override { return TEXT("FrontlinePresentationArtwork"); }
};
FFrontlineArtwork& Artwork() { static FFrontlineArtwork Assets; return Assets; }

struct FFrontlineFactionSelectionV82
{
    int32 Selected=INDEX_NONE,Hover=INDEX_NONE,HoldSource=0;
    bool Visible=false,FreshInput=false,EnterDown=false,MouseDown=false,CommitPainted=false;
    bool PointerOwnsSelectionV826=true,PointerSeenV826=false;
    FVector2D LastPointerV826=FVector2D::ZeroVector;
    double LastTick=-1,HoldAt=-1,CommitAt=-1;
    uint64 LayoutFrame=MAX_uint64,PaintedFrame=MAX_uint64;
    float Widths[4]={480,480,480,480},CommitWidths[4]={480,480,480,480};
    FString Notice;
};
namespace UIMotionReviewV83
{
    double TooltipAge=-1,BannerAge=-1,NumberAge=-1;
    bool HoldMorale=false;
    bool Active=false;int32 HDRCompositeFrames=0;
}
#include "WarfareFrontlineTooltipMotionV83.inl"
struct FFrontlineScreenState
{
    TWeakObjectPtr<APlayerController> Owner;
    int32 Dossier=0,DeploymentTabV63=0;
    int32 AutoRoleV63=INDEX_NONE,AutoVehicleV63=INDEX_NONE;
    FString AutoArmourStableV82;
    FGuid AutoEncounterV63;
    FVector SupportSealV63=FVector::ZeroVector;FVector2D SupportExtentV63=FVector2D::ZeroVector;double SupportSealAtV63=-100.;
    EFrontlineDifficulty PendingDifficulty=EFrontlineDifficulty::Rookie;
    FFrontlineFactionSelectionV82 FactionSelectionV82;
    bool Menu=false,Help=false,Credits=false,bPausedByOverlay=false;
    bool ManualTermsV829=false,ManualBattleV841=false;
    int32 ManualBattleTopicV841=0;
    bool AchievementsV76=false;int32 AchievementFocusV76=0;
    float AchievementScrollV82=0,AchievementScrollTargetV82=0;
    int32 AchievementToastV81=INDEX_NONE;double AchievementToastAtV81=-100.;
    int32 MedalToastV8217=INDEX_NONE;uint8 MedalToastGradeV8217=0,MedalToastRepeatsV8217=0;uint16 MedalToastAttachmentsV8217=0;
    bool ExitConfirmV61=false;
    bool Settings=false,Technology=false,bPausedForAuxOverlay=false;
    int32 SettingsTab=0,TechnologyFocus=0,ArchiveOffset=0,UnitIntroductionPage=0,TechnologyPage=0;
    FGuid ResearchCampaignV72;
    TMap<int32,double> ResearchRevealsV72;
    FGuid NationalResearchCampaignV82;
    TMap<FString,double> NationalResearchRevealsV82;
    TSet<FGuid> ArchiveSelection;
    TArray<FGuid> ArchivePendingDelete;
    bool FRRUArchivesV82=false;
    TArray<int32> FRRUSelectedSquadsV82;
    FGuid FRRUSelectionEncounterV82;
    FString FRRUTargetV82,FRRUAlternateV82;
    bool FRRUOptionsOpenV82=false;int32 FRRUVariantV82=0,FRRUResearchPageV87=0;
    FString ArchiveNotice;
    bool HistoricalDispatch=false,Deployment=false;
    bool CampaignEndingV51=false;
    FTooltipStateV83 TooltipV83;
    FGuid ReportEncounterV83;
    double ReportVisibleAtV83=-1,EndingVisibleAtV83=-1;
    bool EndingEnteredV83=false;
    int32 PreviousPageV35=INDEX_NONE;
    double PageChangedAtV35=-10.;
    bool SmallTransitionV35=false;
    bool PhotoTransitionV37=false;double LastPhotoAtV37=-10.;
    TWeakObjectPtr<UTexture2D> TransitionPhotoV37;
    FrontlinePresentationV37::FPhotoClock PhotoClockV37;
    bool TransitionPhotoPresentedV37=false;
    FGuid AcceptedLoadoutCampaign;int32 AcceptedLoadoutMission=INDEX_NONE;
    FGuid AcceptedNationalIntroCampaignV827;FString AcceptedNationalIntroMissionV827;
    bool NationalAssignmentNoticeV87=false;
    bool Preparing=false,PreparationPainted=false,UnitIntroduction=false;
    double PreparationStarted=0,PreparationFirstSceneAt=0,PreparationLastDrawAt=-1.;
    float PreparationVisualSeconds=0.f;
    uint64 PreparationDrawFrame=MAX_uint64;int32 PreparationSceneFrames=0;
    int32 HoverTrench=INDEX_NONE;double HoverTrenchSeenAt=0;
    FString PreparationError;
    TArray<TWeakObjectPtr<UTexture2D>> PreparationTextures;
    TArray<TWeakObjectPtr<UPrimitiveComponent>> PreparationWarmersV43;
    bool Performance=false,PreferencesLoaded=false;
    bool VideoPending=false;
    bool FrameRateDragging=false,FrameRateFocused=false,FrameRateKeyHeld=false;
    double FrameRateNextRepeat=0;
    FIntPoint DisplayResolution=FIntPoint(2560,1440),PriorResolution=FIntPoint(2560,1440);
    EWindowMode::Type DisplayMode=EWindowMode::Windowed,PriorMode=EWindowMode::Windowed;
    TArray<FIntPoint> ResolutionChoices;
    double VideoDeadline=0,PerfLast=0,PerfUpdate=0,PerfFrameSum=0,PerfGpuSum=0;
    int32 PerfFrames=0,PerfGpuFrames=0;
    float PerfFrameMs=0,PerfGpuMs=0,PerfMemoryMB=0;
    float PerfPresentFPS=0;bool PerfPresentValid=false;
    TArray<float> FrameHistory;
    bool bAssetNoticeDismissed=false;
    FString LastDispatch,LastLoggedAssetFailure;
    TArray<TPair<float,FString>> Dispatches;
};
namespace MedalWallV8215 { bool Selected(APlayerController* P);bool Wheel(APlayerController* P,int32 Direction); }
FFrontlineScreenState& ScreenState(APlayerController* PC)
{
    static FFrontlineScreenState State;
    if (State.Owner.Get()!=PC) { State=FFrontlineScreenState(); State.Owner=PC; }
    if (!State.PreferencesLoaded && GConfig)
    {
        GConfig->GetBool(TEXT("Warfare.Display"),TEXT("PerformanceMonitor"),State.Performance,GGameUserSettingsIni);
        State.PreferencesLoaded=true;
    }
    return State;
}

extern TOptional<FVector2D> V28ReviewPointer;
namespace HonoursListV82
{
constexpr float X=92,Top=208,Width=970,Height=728,Stride=104,RowHeight=98;
float Maximum(){return FMath::Max(0.f,FrontlineAchievementsV76::Count*Stride-Height);}
bool Visible(const FFrontlineScreenState& S,int32 Index)
{
    if(Index<0||Index>=FrontlineAchievementsV76::Count)return false;
    const float Y=Top+Index*Stride-S.AchievementScrollV82;
    return Y>=Top-.01f&&Y+RowHeight<=Top+Height+.01f;
}
void Reveal(FFrontlineScreenState& S,int32 Index,bool Snap=false)
{
    S.AchievementFocusV76=FMath::Clamp(Index,0,FrontlineAchievementsV76::Count-1);
    const float Y=S.AchievementFocusV76*Stride;
    float Target=S.AchievementScrollTargetV82;
    if(Y<Target)Target=Y;else if(Y+RowHeight>Target+Height)Target=Y+Stride-Height;
    S.AchievementScrollTargetV82=FMath::Clamp(Target,0.f,Maximum());
    if(Snap)S.AchievementScrollV82=S.AchievementScrollTargetV82;
}
void Advance(FFrontlineScreenState& S)
{
    S.AchievementScrollTargetV82=FMath::Clamp(S.AchievementScrollTargetV82,0.f,Maximum());
    // Whole-entry steps keep every primitive inside the viewport on both
    // SDR Canvas and HDR Slate DebugCanvas, which forbids game-thread flushes.
    S.AchievementScrollV82=S.AchievementScrollTargetV82;
}
bool Wheel(AWarfareFrontlineController* PC,int32 Direction)
{
    auto& S=ScreenState(PC);if(!S.AchievementsV76)return false;
    if(MedalWallV8215::Selected(PC))return MedalWallV8215::Wheel(PC,Direction);
    int32 W=0,H=0;PC->GetViewportSize(W,H);float MX=0,MY=0;
    const float Scale=FMath::Min(W/1920.f,H/1080.f);
    if(Scale<=0||(!PC->GetMousePosition(MX,MY)&&!V28ReviewPointer.IsSet()))return true;
    // All wheel events belong to the open page; the sheet itself stays still.
    const FVector2D P=V28ReviewPointer.IsSet()?V28ReviewPointer.GetValue():FVector2D((MX-(W-1920*Scale)*.5f)/Scale,(MY-(H-1080*Scale)*.5f)/Scale);
    if(P.X>=X&&P.X<=X+Width+24&&P.Y>=Top&&P.Y<=Top+Height)
        S.AchievementScrollTargetV82=FMath::Clamp(S.AchievementScrollTargetV82+Direction*Stride,0.f,Maximum());
    return true;
}
}

AWarfareFrontlineGameMode* FrontlineFor(const UObject* Object)
{
    return Cast<AWarfareFrontlineGameMode>(UGameplayStatics::GetGameMode(Object));
}

bool HasEnded(const AWarfareFrontlineGameMode* Battle)
{
    return Battle && !Battle->IsBattleOutroActive() && (Battle->Phase==EFrontlinePhase::Victory || Battle->Phase==EFrontlinePhase::Defeat);
}

FString TrenchName(int32 Index)
{
    return FString::Printf(TEXT("第 %d 道战壕"),Index+1);
}

FLinearColor TeamColor(int32 Team)
{
    return Team==0?Allied:Team==1?Opposing:Muted;
}

FString ClockText(float Seconds)
{
    const int32 Whole=FMath::Max(0,FMath::FloorToInt(Seconds));
    return FString::Printf(TEXT("%02d:%02d"),Whole/60,Whole%60);
}

// All screen layout uses a centred 1920 x 1080 canvas, including mouse hit boxes.
// Font size is based on the actual engine font height rather than a monitor DPI assumption.
// Surface-material tiles use scene-translucency alpha: dstA *= 1-srcA.
// Slate's SDR UI texture instead needs coverage: srcA + dstA*(1-srcA).
// Immediately after each such tile, add its original coverage to alpha only.
// The registered PS uses the matching engine SimpleElement shader entry. RGB is
// excluded from the render-target write mask throughout this correction pass.
struct FFrontlineImagePlacement
{
    float X,Y,W,H,U,V,UW,VH;
};
bool FrontlineTextureReady(const UTexture2D* Texture)
{
    if(!IsValid(Texture)||!Texture->GetResource()
        ||Texture->GetSizeX()<=0||Texture->GetSizeY()<=0)return false;
#if WITH_EDITOR
    // Asynchronous editor compilation may expose a temporary default texture.
    // Cooked games have no texture compiler and do not expose this editor API.
    if(Texture->IsDefaultTexture())return false;
#endif
    return true;
}
FFrontlineImagePlacement PlaceFrontlineImage(const UTexture2D* Texture,float X,float Y,float W,float H,
    float U=0,float V=0,float UW=1,float VH=1,bool Contain=false)
{
    FFrontlineImagePlacement P{X,Y,W,H,U,V,UW,VH};
    if(!Texture||Texture->GetSizeX()<=0||Texture->GetSizeY()<=0||W<=0||H<=0||UW<=0||VH<=0)return P;
    const float SourceAspect=float(Texture->GetSizeX())*UW/FMath::Max(1.f,float(Texture->GetSizeY())*VH),BoxAspect=W/H;
    if(Contain)
    {
        if(SourceAspect>BoxAspect){P.H=W/SourceAspect;P.Y+=(H-P.H)*.5f;}
        else {P.W=H*SourceAspect;P.X+=(W-P.W)*.5f;}
    }
    else if(SourceAspect>BoxAspect){P.UW=UW*BoxAspect/SourceAspect;P.U+=(UW-P.UW)*.5f;}
    else {P.VH=VH*SourceAspect/BoxAspect;P.V+=(VH-P.VH)*.5f;}
    return P;
}
TOptional<FVector2D> V28ReviewPointer;
TOptional<FVector> V28ReviewAim;
void StartFrontlineV28UIReview(AWarfareFrontlineController* Controller);
void StartFrontlineV30Review(AWarfareFrontlineController* Controller);
void StartFrontlineV31Review(AWarfareFrontlineController* Controller);
void StartFrontlineV32Review(AWarfareFrontlineController* Controller);
void StartFrontlineV33Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8214EffectsReview(AWarfareFrontlineController* Controller);
void StartFrontlineV8215Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8216Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8217Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8218Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8219Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8220Review(AWarfareFrontlineController* Controller);
void StartFrontlineV8221Review(AWarfareFrontlineController* Controller);
bool DecorationModalV8216(APlayerController* P);
void StartFrontlineV35Review(AWarfareFrontlineController* Controller);
void StartFrontlineV36UIReview(AWarfareFrontlineController* Controller);
void StartFrontlineV37Review(AWarfareFrontlineController* Controller);
void StartFrontlineV38Review(AWarfareFrontlineController* Controller);
void StartFrontlineV39Review(AWarfareFrontlineController* Controller);
void StartFrontlineV40Review(AWarfareFrontlineController* Controller);
bool V36TypographyAuditActive=false;
TMap<FString,TSharedPtr<FJsonObject>> V36TypographyAudit;
bool V45HUDAuditActive=false;
// Disposable V66 diagnostic scope; neither marker changes the render path.
bool V66HUDModernAudit=false;
int32 V66HUDHoverIndex=0;
FrontlineLocalizationV35::ELanguage V45AuditLanguage=FrontlineLocalizationV35::ELanguage::English;
TMap<FString,TSharedPtr<FJsonObject>> V45PrimaryDraws,V45ChineseDraws;
TSet<FString> V45InvalidChineseBreaks;
// Disposable native-font audit only; normal drawing has no diagnostic allocation.
// FString's default map equality is case-insensitive; rendered glyphs and rows are not.
template<typename ValueType>
struct FTextCacheKeyFuncsV825 : TDefaultMapKeyFuncs<FString,ValueType,false>
{
    using KeyInitType=typename TDefaultMapKeyFuncs<FString,ValueType,false>::KeyInitType;
    static bool Matches(KeyInitType A,KeyInitType B){return A.Equals(B,ESearchCase::CaseSensitive);}
    static uint32 GetKeyHash(KeyInitType Key){return FCrc::MemCrc32(*Key,Key.Len()*sizeof(TCHAR));}
};
template<typename ValueType>
using TTextCacheV825=TMap<FString,ValueType,FDefaultSetAllocator,FTextCacheKeyFuncsV825<ValueType>>;
// Fixed-size FIFO eviction preserves other warm entries. A dirty glyph entry
// replaces itself without inserting a duplicate key into the eviction ring.
template<typename ValueType,int32 Capacity>
struct TBoundedTextCacheV8213
{
    TTextCacheV825<ValueType> Values;TArray<FString> Ring;int32 Oldest=0;
    ValueType* Find(const FString& Key){return Values.Find(Key);}
    void Add(const FString& Key,const ValueType& Value)
    {
        if(FrontlineUIAssetsV8213::BaselineMode())
        {if(Values.Num()>=Capacity)Values.Reset();Values.Add(Key,Value);return;}
        if(Values.Contains(Key)){Values.Add(Key,Value);return;}
        if(Ring.Num()<Capacity)Ring.Add(Key);
        else {Values.Remove(Ring[Oldest]);Ring[Oldest]=Key;Oldest=(Oldest+1)%Capacity;}
        Values.Add(Key,Value);
    }
};
// Retain layout, not pixels: live colours, anchors and tooltip motion still
// pass through the original Canvas renderer. Evict only the least recent
// entry on a miss rather than clearing all the warm labels at capacity.
template<typename KeyType,typename ValueType,int32 Capacity>
struct TLayoutCacheV834
{
    struct FEntry {ValueType Value;uint64 Used=0;};
    TMap<KeyType,FEntry> Entries;uint64 Clock=0;
    const ValueType* Find(const KeyType& Key)
    {if(auto* Entry=Entries.Find(Key)){Entry->Used=++Clock;return &Entry->Value;}return nullptr;}
    void Add(const KeyType& Key,const ValueType& Value)
    {
        if(auto* Entry=Entries.Find(Key)){Entry->Value=Value;Entry->Used=++Clock;return;}
        if(Entries.Num()>=Capacity)
        {
            const KeyType* Oldest=nullptr;uint64 OldestUse=MAX_uint64;
            for(const auto& Pair:Entries)if(Pair.Value.Used<OldestUse){Oldest=&Pair.Key;OldestUse=Pair.Value.Used;}
            if(Oldest){const KeyType Evicted=*Oldest;Entries.Remove(Evicted);}
        }
        Entries.Add(Key,FEntry{Value,++Clock});
    }
};
namespace FrontlineTextLayoutV834
{
struct FReview
{
    uint64 LabelHits=0,LabelMisses=0,ParagraphHits=0,ParagraphMisses=0,ParityChecks=0,ParityFailures=0;
    uint64 OpaqueMapHeldFrames=0,OpaqueMapHoldEntries=0,OpaqueMapResumeCuts=0,LastOpaqueMapHoldFrame=MAX_uint64;
    TWeakObjectPtr<AWarfareFrontlineController> OpaqueMapOwner;
    int32 LabelEntries=0,ParagraphEntries=0;
};
FReview& ReviewState(){static FReview State;return State;}
bool ReviewEnabled()
{
    static const bool Active=FParse::Param(FCommandLine::Get(),TEXT("WFV834Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));return Active;
}
bool CacheEnabled()
{static const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFUILayoutBaselineV834"));return !Baseline;}
void RecordOpaqueMapHold(AWarfareFrontlineController* PC)
{
    if(!ReviewEnabled())return;
    auto& S=ReviewState();
    if(S.OpaqueMapOwner.Get()==PC&&S.LastOpaqueMapHoldFrame==GFrameCounter)return;
    if(S.OpaqueMapOwner.Get()!=PC||S.LastOpaqueMapHoldFrame==MAX_uint64||GFrameCounter>S.LastOpaqueMapHoldFrame+1)
        ++S.OpaqueMapHoldEntries;
    ++S.OpaqueMapHeldFrames;S.OpaqueMapOwner=PC;S.LastOpaqueMapHoldFrame=GFrameCounter;
}
TSharedPtr<FJsonObject> Review()
{
    const auto& S=ReviewState();auto J=MakeShared<FJsonObject>();
    J->SetBoolField(TEXT("cache_enabled"),CacheEnabled());J->SetBoolField(TEXT("review_enabled"),ReviewEnabled());
    J->SetNumberField(TEXT("label_hits"),double(S.LabelHits));J->SetNumberField(TEXT("label_misses"),double(S.LabelMisses));
    J->SetNumberField(TEXT("paragraph_hits"),double(S.ParagraphHits));J->SetNumberField(TEXT("paragraph_misses"),double(S.ParagraphMisses));
    J->SetNumberField(TEXT("same_frame_exact_layout_checks"),double(S.ParityChecks));J->SetNumberField(TEXT("same_frame_exact_layout_failures"),double(S.ParityFailures));
    J->SetNumberField(TEXT("label_entries"),S.LabelEntries);J->SetNumberField(TEXT("paragraph_entries"),S.ParagraphEntries);
    J->SetNumberField(TEXT("opaque_map_held_frames"),double(S.OpaqueMapHeldFrames));
    J->SetNumberField(TEXT("opaque_map_hold_entries"),double(S.OpaqueMapHoldEntries));
    J->SetNumberField(TEXT("opaque_map_resume_cuts"),double(S.OpaqueMapResumeCuts));
    J->SetBoolField(TEXT("bounded"),S.LabelEntries<=512&&S.ParagraphEntries<=512);
    return J;
}
}
bool V825FontAuditActive=false;
TTextCacheV825<TSharedPtr<FJsonObject>> V825FontDraws;
TTextCacheV825<TSharedPtr<FJsonObject>> V8212ShapedFontDraws;
TSharedPtr<FJsonObject> V825FontEdges;
float V8212NativeWindowDPI=0.f,V8212ViewportCachedDPI=0.f;
int32 V8212ViewportDPIRefreshes=0;
void RecordTypographyV36(const FString& Text,float X,float Y,float W,float H,float ActualW,float ActualH,float Size,bool Truncated=false,int32 Align=-1,float DrawX=0,float DrawY=0,const FString& Face=FString(),const TArray<FString>* PaintedRows=nullptr)
{
    if(!V36TypographyAuditActive)return;
    const FString Key=FString::Printf(TEXT("%d|%s|%.1f,%.1f,%.1f,%.1f"),int32(FrontlineLocalizationV35::GetLanguage()),*Text,X,Y,W,H);
    if(V36TypographyAudit.Contains(Key))return;
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("text"),Text);J->SetNumberField(TEXT("x"),X);J->SetNumberField(TEXT("y"),Y);
    J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);J->SetNumberField(TEXT("measured_width"),ActualW);J->SetNumberField(TEXT("measured_height"),ActualH);
    J->SetNumberField(TEXT("font_size"),Size);J->SetBoolField(TEXT("truncated"),Truncated);
    J->SetBoolField(TEXT("fits"),ActualW<=W+.5f&&ActualH<=H+.5f&&X>=0&&Y>=0&&X+W<=1920.5f&&Y+H<=1080.5f);
    if(PaintedRows)
    {
        TArray<TSharedPtr<FJsonValue>> Rows;FString Joined;bool Verbatim=true;
        for(const auto& Row:*PaintedRows)
        {
            Rows.Add(MakeShared<FJsonValueString>(Row));Joined+=Row;
            if(V825FontAuditActive&&!Row.IsEmpty())
            {
                bool Found=false;for(const auto& Draw:V825FontDraws)Found|=Draw.Value->GetStringField(TEXT("text")).Equals(Row,ESearchCase::CaseSensitive);
                Verbatim&=Found;
            }
        }
        auto Compact=[](const FString& V){FString R;for(TCHAR C:V)if(!FChar::IsWhitespace(C))R.AppendChar(C);return R;};
        J->SetArrayField(TEXT("painted_rows"),Rows);
        J->SetBoolField(TEXT("painted_rows_match_authored"),!Truncated&&Compact(Joined).Equals(Compact(Text),ESearchCase::CaseSensitive));
        J->SetBoolField(TEXT("painted_rows_drawn_verbatim"),Verbatim);
    }
    if(Align>=0)
    {
        J->SetNumberField(TEXT("alignment"),Align);J->SetNumberField(TEXT("draw_x"),DrawX);J->SetNumberField(TEXT("draw_y"),DrawY);
        J->SetNumberField(TEXT("cap_baseline"),DrawY+ActualH);J->SetStringField(TEXT("font_face"),Face);
    }
    V36TypographyAudit.Add(Key,J);
}
void RunConsoleTypographyChecksV70(const TSharedRef<FJsonObject>& Layout,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto& Rows=Layout->GetArrayField(TEXT("typography"));
    TArray<TSharedPtr<FJsonObject>> Headers;bool Fits=true,Centred=true;int32 Controls=0,UnitCards=0;
    for(const auto& Row:Rows)
    {
        const auto J=Row->AsObject();double Align=0;if(!J.IsValid()||!J->TryGetNumberField(TEXT("alignment"),Align))continue;
        ++Controls;Fits&=J->GetBoolField(TEXT("fits"));const float X=J->GetNumberField(TEXT("x")),W=J->GetNumberField(TEXT("width"));
        const float ActualX=J->GetNumberField(TEXT("draw_x")),ActualW=J->GetNumberField(TEXT("measured_width"));
        if(Align==1)Centred&=FMath::Abs(ActualX+ActualW*.5f-X-W*.5f)<.02f;
        const FString Text=J->GetStringField(TEXT("text"));
        if(Text==TEXT("INFANTRY")||Text==TEXT("ARMOUR")||Text==TEXT("FIRE SUPPORT")||Text==TEXT("COMMAND"))Headers.Add(J);
        if(FMath::IsNearlyEqual(float(J->GetNumberField(TEXT("y"))),DockTop+174.f)&&FMath::IsNearlyEqual(W,110.f))
            {++UnitCards;Centred&=Align==1;}
    }
    bool HeaderRow=Headers.Num()==4;
    for(int32 I=1;I<Headers.Num();++I)
        HeaderRow&=FMath::IsNearlyEqual(float(Headers[I]->GetNumberField(TEXT("cap_baseline"))),float(Headers[0]->GetNumberField(TEXT("cap_baseline"))),.02f)
            &&Headers[I]->GetStringField(TEXT("font_face"))==Headers[0]->GetStringField(TEXT("font_face"))&&Headers[I]->GetNumberField(TEXT("alignment"))==1;
    Check(TEXT("v70_actual_hud_headers_share_face_baseline_and_centred_boxes"),HeaderRow);
    Check(TEXT("v70_actual_roster_card_captions_are_centred"),UnitCards>0&&Centred);
    Check(TEXT("v70_actual_control_glyphs_fit_their_containers"),Controls>8&&Fits);
}
bool V32FlagSheet=false;
float V32FlagAge=3.f;
bool V27ImageAuditActive=false;
TMap<FString,TSharedPtr<FJsonObject>> V27ImageAudit;
// Shared page geometry is authored in the same 1920 x 1080 canvas as hit boxes.
// Full pages reserve one header, one body and one footer rather than nesting cases.
namespace PageCanvasV73
{
constexpr float Margin=64.f,Right=1856.f,Width=1792.f;
constexpr float Header=64.f,TabY=174.f,TabH=58.f,BodyY=274.f,FooterY=936.f;
FString L(const TCHAR* EN,const TCHAR* ZH)
{return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?EN:ZH;}
bool Audit=false;
TMap<FString,TSharedPtr<FJsonObject>> Hits;
void RecordHit(float X,float Y,float W,float H,FName Name,int32 Priority)
{
    if(!Audit)return;
    const FString Key=FString::Printf(TEXT("%s|%.1f,%.1f,%.1f,%.1f"),*Name.ToString(),X,Y,W,H);
    if(Hits.Contains(Key))return;
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("command"),Name.ToString());
    J->SetNumberField(TEXT("x"),X);J->SetNumberField(TEXT("y"),Y);J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);J->SetNumberField(TEXT("priority"),Priority);
    J->SetBoolField(TEXT("fits"),W>0&&H>0&&X>=0&&Y>=0&&X+W<=1920.5f&&Y+H<=1080.5f);Hits.Add(Key,J);
}
}
void RecordV27ImagePlacement(const UTexture2D* Texture,const FFrontlineImagePlacement& P,bool Contain)
{
    if(!V27ImageAuditActive||!Texture)return;
    const FString Key=FString::Printf(TEXT("%s|%.2f,%.2f|%.1fx%.1f|%.4f,%.4f,%d"),*Texture->GetPathName(),P.X,P.Y,P.W,P.H,P.UW,P.VH,Contain);
    if(V27ImageAudit.Contains(Key))return;
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("texture"),Texture->GetPathName());J->SetStringField(TEXT("placement"),Contain?TEXT("contain"):TEXT("cover"));
    J->SetNumberField(TEXT("source_width"),Texture->GetSizeX());J->SetNumberField(TEXT("source_height"),Texture->GetSizeY());
    J->SetNumberField(TEXT("draw_x"),P.X);J->SetNumberField(TEXT("draw_y"),P.Y);
    J->SetNumberField(TEXT("draw_width"),P.W);J->SetNumberField(TEXT("draw_height"),P.H);J->SetNumberField(TEXT("uv_width"),P.UW);J->SetNumberField(TEXT("uv_height"),P.VH);
    const float SourceAspect=float(Texture->GetSizeX())*P.UW/(float(Texture->GetSizeY())*P.VH);
    J->SetNumberField(TEXT("aspect_relative_error"),FMath::Abs((P.W/P.H)/SourceAspect-1));V27ImageAudit.Add(Key,J);
}
class FFrontlineFlagParameters final : public FBatchedElementParameters
{
public:
    FFrontlineFlagParameters(const FTexture* InMask,const FTexture* InLiquid,const FVector4f& InState,float InTime,float InSeed):Mask(InMask),Liquid(InLiquid),State(InState),Time(InTime),Seed(InSeed){}
    virtual void BindShaders(FRHICommandList& RHICmdList,FGraphicsPipelineStateInitializer& PSO,
        ERHIFeatureLevel::Type FeatureLevel,const FMatrix& Transform,const float,const FMatrix&,const FTexture* Texture) override
    {
        TShaderMapRef<FSimpleElementVS> VS(GetGlobalShaderMap(FeatureLevel));TShaderMapRef<FFrontlineFlagPS> PS(GetGlobalShaderMap(FeatureLevel));
        PSO.BlendState=TStaticBlendState<CW_RGBA,BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_One,BF_InverseSourceAlpha>::GetRHI();
        PSO.BoundShaderState.VertexDeclarationRHI=GSimpleElementVertexDeclaration.VertexDeclarationRHI;
        PSO.BoundShaderState.VertexShaderRHI=VS.GetVertexShader();PSO.BoundShaderState.PixelShaderRHI=PS.GetPixelShader();PSO.PrimitiveType=PT_TriangleList;
        RHICmdList.ApplyCachedRenderTargets(PSO);SetGraphicsPipelineState(RHICmdList,PSO,0);
        SetShaderParametersLegacyVS(RHICmdList,VS,Transform);SetShaderParametersLegacyPS(RHICmdList,PS,Texture,Mask,Liquid,State,Time,Seed);
    }
private:
    const FTexture* Mask;const FTexture* Liquid;FVector4f State;float Time,Seed;
};

class FFrontlineCanvasCoverage final : public FBatchedElementParameters
{
public:
    virtual void BindShaders(FRHICommandList& RHICmdList,FGraphicsPipelineStateInitializer& PSO,
        ERHIFeatureLevel::Type FeatureLevel,const FMatrix& Transform,const float,
        const FMatrix&,const FTexture* Texture) override
    {
        TShaderMapRef<FSimpleElementVS> VS(GetGlobalShaderMap(FeatureLevel));
        TShaderMapRef<FFrontlineCanvasCoveragePS> PS(GetGlobalShaderMap(FeatureLevel));
        PSO.BlendState=TStaticBlendState<CW_ALPHA,BO_Add,BF_Zero,BF_One,BO_Add,BF_One,BF_One>::GetRHI();
        PSO.BoundShaderState.VertexDeclarationRHI=GSimpleElementVertexDeclaration.VertexDeclarationRHI;
        PSO.BoundShaderState.VertexShaderRHI=VS.GetVertexShader();
        PSO.BoundShaderState.PixelShaderRHI=PS.GetPixelShader();
        PSO.PrimitiveType=PT_TriangleList;
        RHICmdList.ApplyCachedRenderTargets(PSO);
        SetGraphicsPipelineState(RHICmdList,PSO,0);
        SetShaderParametersLegacyVS(RHICmdList,VS,Transform);
        SetShaderParametersLegacyPS(RHICmdList,PS,Texture);
    }
};

struct FMeasuredCanvasTextV62 : FCanvasTextItem
{
    using FCanvasTextItem::FCanvasTextItem;
    using FCanvasTextItem::GetTextSize;
};
struct FMeasuredCanvasShapedTextV8212 : FCanvasShapedTextItem
{
    using FCanvasShapedTextItem::FCanvasShapedTextItem;
    FVector2D PaintedOrigin=FVector2D::ZeroVector;
    virtual void DrawStringInternal(FCanvas* InCanvas,const FVector2D& DrawPos,const FLinearColor& InColor,TArrayView<FTextEffect> Effects) override
    {
        PaintedOrigin=DrawPos;
        FCanvasShapedTextItem::DrawStringInternal(InCanvas,DrawPos,InColor,Effects);
    }
};
int32 TypographyReviewPageV8210=INDEX_NONE;
enum class EFrontlineTextRoleV8210 : uint8 { Auto,Plate,Label,Headline,Text,Objective,Annotation,Inscription,Typed,Stencil,Numeral,FieldValue,DarkText,Legacy };

struct FFrontlineCanvas
{
    AHUD& HUD;
    UCanvas* Surface;
    UFont* Font;
    UFont* LargeFont;
    UFont* ChineseFont=nullptr;
    float ChineseFontHeight=16;
    float Scale=1,OX=0,OY=0,FontHeight=16,LargeFontHeight=32;
    bool bSuppressBackgroundHits=false;
    bool bGermanFinish=false;
    int32 ConsoleNationIndexV82=0;
    bool bHDRComposite=false;
    mutable int32 NextHitId=0;
    mutable int32 CursorPriorityV67=MIN_int32;
    mutable EFrontlineCursorV67 CursorV67=EFrontlineCursorV67::Idle;
    mutable FString HoverShortcut;
    mutable bool bEnglishPrimaryV45=false;
    mutable bool bPreferChineseFontV45=false;
    mutable FVector2D ShortcutAnchor=FVector2D::ZeroVector;
    FVector2D Mouse=FVector2D(-10000,-10000);
    explicit FFrontlineCanvas(AHUD& InHUD,UCanvas* Canvas,bool bModal=false,bool bInHDRComposite=false) : HUD(InHUD),Surface(Canvas),bSuppressBackgroundHits(bModal),bHDRComposite(bInHDRComposite)
    {
        if (const auto* Battle=Cast<AWarfareFrontlineGameMode>(HUD.GetWorld()->GetAuthGameMode()))
        {
            const auto Nation=FrontlineConsoleV79::ResolveDisplayNationV82(Battle,
                Battle->Phase==EFrontlinePhase::Briefing&&ScreenState(HUD.GetOwningPlayerController()).Dossier==0);
            ConsoleNationIndexV82=FMath::Clamp(int32(Nation),0,3);
            bGermanFinish=Nation==EFRRUNationV81::German||Nation==EFRRUNationV81::French;
        }
        Scale=FMath::Min(Canvas->ClipX/1920.f,Canvas->ClipY/1080.f);
        OX=(Canvas->ClipX-1920.f*Scale)*.5f; OY=(Canvas->ClipY-1080.f*Scale)*.5f;
        Font=Artwork().Sans?Artwork().Sans.Get():(GEngine?GEngine->GetMediumFont():nullptr);
        LargeFont=GEngine?GEngine->GetLargeFont():nullptr;
        ChineseFont=Artwork().ChineseMediumV45?Artwork().ChineseMediumV45.Get():Font;
        // Font line metrics are immutable between font resource changes.
        static TWeakObjectPtr<UFont> MetricFont,MetricLargeFont;
        static float CachedHeight=16.f,CachedLargeHeight=32.f;
        float W=0,H=0;
        if(MetricFont.Get()!=Font){HUD.GetTextSize(TEXT("Hg"),W,H,Font,1.f);CachedHeight=FMath::Max(H,1.f);MetricFont=Font;}
        if(MetricLargeFont.Get()!=LargeFont){HUD.GetTextSize(TEXT("Hg"),W,H,LargeFont,1.f);CachedLargeHeight=FMath::Max(H,1.f);MetricLargeFont=LargeFont;}
        FontHeight=CachedHeight;LargeFontHeight=CachedLargeHeight;
        static TWeakObjectPtr<UFont> MetricChineseFont;static float CachedChineseHeight=16;
        if(MetricChineseFont.Get()!=ChineseFont){HUD.GetTextSize(TEXT("Hg国"),W,H,ChineseFont,1.f);CachedChineseHeight=FMath::Max(H,1.f);MetricChineseFont=ChineseFont;}
        ChineseFontHeight=CachedChineseHeight;
        float MX=0,MY=0;
        if (APlayerController* Owner=HUD.GetOwningPlayerController())
            if (Owner->GetMousePosition(MX,MY)) Mouse=FVector2D((MX-OX)/Scale,(MY-OY)/Scale);
        if(V28ReviewPointer.IsSet())Mouse=V28ReviewPointer.GetValue();
        if(!bModal)if(const auto* PC=Cast<AWarfareFrontlineController>(HUD.GetOwningPlayerController()))
            if(PC->SupportTargetMode>0)CursorV67=EFrontlineCursorV67::Target;
        if(V825FontAuditActive&&!V825FontEdges.IsValid())AuditFontEdgesV825();
    }
    ~FFrontlineCanvas()
    {
        if(!HoverShortcut.IsEmpty())
        {
            TooltipV83(TooltipKeyV83(11,uint32(ShortcutAnchor.X)+uint32(ShortcutAnchor.Y)*1920),0,[&]{
            const FString Label=TEXT("快捷键  ")+HoverShortcut;
            const float W=FMath::Clamp(PlainTextWidth(Label,15)+30.f,120.f,360.f);
            const float X=FMath::Clamp(float(ShortcutAnchor.X),12.f,1908.f-W);
            const float Y=FMath::Clamp(float(ShortcutAnchor.Y)-45.f,74.f,1027.f);
            Skin(1,X,Y,W,36,6);PlainText(Label,X+15,Y+9,15,Paper);
            });
        }
        FinishTooltipsV83();
        FrontlineCursorV67::SetState(HUD.GetOwningPlayerController(),CursorV67);
    }
    void Shortcut(float X,float Y,float W,float H,const FString& Key,int32 Priority=10) const
    {
        if(!Key.IsEmpty() && (!bSuppressBackgroundHits||Priority>=40) && Hovered(X,Y,W,H))
        {HoverShortcut=Key;ShortcutAnchor=FVector2D(X,Y);}
    }
    void FitText(const FString& Value,float X,float Y,float Width,float Size=18,const FLinearColor& Ink=Paper) const
    {
        if(Width<=0||Size<=0||!FMath::IsFinite(Width)||!FMath::IsFinite(Size))return;
        const float Measured=TextWidth(Value,Size);
        const float Fitted=Measured>Width?Size*Width/FMath::Max(1.f,Measured):Size;
        Text(Value,X,Y,Fitted,Ink);
        const FString Display=DisplayText(Value);
        if(V36TypographyAuditActive||FrontlineUIAssetsV8213::BaselineMode())
            RecordTypographyV36(Display,X,Y,Width,Size*1.25f,TextWidth(Value,Fitted),Fitted*(IsControlText(Display)?1.f:OpticalTextScaleV45(Display)),Fitted);
    }
    FString DisplayText(const FString& Source) const
    {return bEnglishPrimaryV45?FrontlineLocalizationV35::English(Source):FrontlineLocalizationV35::Text(Source);}
    static bool ContainsCJKV45(const FString& Value)
    {for(TCHAR C:Value)if((C>=0x2E80&&C<=0x9FFF)||(C>=0xF900&&C<=0xFAFF)||(C>=0xFF00&&C<=0xFFEF))return true;return false;}
    static bool ClosingPunctuationV45(TCHAR C)
    {return C==0x3002||C==0xff0c||C==0x3001||C==0xff1b||C==0xff1a||C==0xff01||C==0xff1f||C==0xff09||C==0x300d||C==0x300f||C==0x3011||C==0x201d||C==0x2019;}
    float OpticalTextScaleV45(const FString& Value) const
    {return Artwork().Sans?((bPreferChineseFontV45||ContainsCJKV45(Value))?1.25f:1.20f):1.f;}
    void PrimaryLabel(const FString& Source,float X,float Y,float W,float H,float Size=18,const FLinearColor& Ink=Paper,int32 Align=0) const
    {TGuardValue<bool> EnglishScope(bEnglishPrimaryV45,true);Label(Source,X,Y,W,H,Size,Ink,Align);}
    void PrimaryCaption(const FString& Source,float X,float Y,float W,float H,float Size=13,const FLinearColor& Ink=Paper) const
    {TGuardValue<bool> EnglishScope(bEnglishPrimaryV45,true);Caption(Source,X,Y,W,H,Size,Ink);}
    void PrimaryText(const FString& Source,float X,float Y,float Size=18,const FLinearColor& Ink=Paper) const
    {TGuardValue<bool> EnglishScope(bEnglishPrimaryV45,true);Text(Source,X,Y,Size,Ink);}
    void PrimaryRight(const FString& Source,float X,float Y,float Size=18,const FLinearColor& Ink=Paper) const
    {TGuardValue<bool> EnglishScope(bEnglishPrimaryV45,true);Right(Source,X,Y,Size,Ink);}
    void PrimaryButton(float X,float Y,float W,float H,const FString& Source,const FString& Key,FName Name,bool Enabled=true,bool Selected=false,int32 Priority=10) const
    {TGuardValue<bool> EnglishScope(bEnglishPrimaryV45,true);Button(X,Y,W,H,Source,Key,Name,Enabled,Selected,Priority);}
    void AuditDrawV45(const FString& Value,float Size) const
    {
        if(!V45HUDAuditActive)return;
        if(bEnglishPrimaryV45)
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("text"),Value);
            J->SetBoolField(TEXT("contains_cjk"),ContainsCJKV45(Value));
            J->SetBoolField(TEXT("selected_chinese"),FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese);
            V45PrimaryDraws.FindOrAdd(Value)=J;
        }
        if(ContainsCJKV45(Value)&&ChineseFont)
        {
            const auto Info=RuntimeInfoV62(Value,Size,false);
            FMeasuredCanvasTextV62 Item(FVector2D::ZeroVector,FText::FromString(Value),Info,FLinearColor::White);
            const float W=Item.GetTextSize(Surface->Canvas->GetDPIScale()).X;
            const float Expected=PlainTextWidth(Value,Size);
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("text"),Value);J->SetStringField(TEXT("font"),ChineseFont->GetPathName());
            J->SetNumberField(TEXT("raster_point_size"),Info.Size);
            J->SetNumberField(TEXT("nominal_size"),Size);J->SetNumberField(TEXT("draw_width"),W/RasterUnitsV825());
            J->SetNumberField(TEXT("measure_width"),Expected);J->SetNumberField(TEXT("width_error"),FMath::Abs(W/RasterUnitsV825()-Expected));
            J->SetBoolField(TEXT("medium_face"),ChineseFont==Artwork().ChineseMediumV45&&Artwork().ChineseMediumV45!=nullptr);
            V45ChineseDraws.FindOrAdd(FString::Printf(TEXT("%s|%.2f"),*Value,Size))=J;
        }
    }
    // One cap box for all engraved controls, including buttons and captions.
    // Use the rounded output glyph size, not the requested nominal font size,
    // so neighbouring controls retain the same baseline at every UI scale.
    void ControlLabelV70(const FString& Value,float X,float Y,float W,float H,float Size,const FLinearColor& Ink,int32 Align,bool Fixed=false) const
    {
        if(Value.IsEmpty()||W<=0||H<=0||Size<=0||!FMath::IsFinite(W)||!FMath::IsFinite(H)||!FMath::IsFinite(Size))return;
        TArray<FString> Rows;
        if(Value.Contains(TEXT("\n")))Value.ParseIntoArray(Rows,TEXT("\n"),false);
        const int32 LineCount=Rows.IsEmpty()?1:Rows.Num();
        auto CapHeight=[&](float S){return RuntimeInfoV62(Value,S,true).Size*(4.f/3.f)*.810f/FMath::Max(.001f,Scale);};
        auto BlockHeight=[&](float S){return LineCount==1?CapHeight(S):CapHeight(S)+(LineCount-1)*RuntimeSizeV81(TEXT("Hg"),S,true).Y;};
        if(!Fixed)
        {
            Size*=FMath::Min(1.f,FMath::Min(W/FMath::Max(1.f,RuntimeWidthV62(Value,Size,true)),H/FMath::Max(1.f,BlockHeight(Size))));
            for(int32 I=0;I<12&&(RuntimeWidthV62(Value,Size,true)>W+.1f||BlockHeight(Size)>H+.1f);++I)Size=FMath::Max(1.f,Size-.25f);
        }
        const float TW=RuntimeWidthV62(Value,Size,true),TH=BlockHeight(Size);
        const float TX=X+(Align==1?(W-TW)*.5f:Align==2?W-TW:0.f),TY=Y+(H-TH)*.5f;
        if(LineCount==1)ControlText(Value,TX,TY,Size,Ink);
        else
        {
            const float Leading=RuntimeSizeV81(TEXT("Hg"),Size,true).Y;
            for(int32 I=0;I<Rows.Num();++I)
            {
                const float RowWidth=RuntimeWidthV62(Rows[I],Size,true);
                ControlText(Rows[I],X+(Align==1?(W-RowWidth)*.5f:Align==2?W-RowWidth:0.f),TY+I*Leading,Size,Ink);
            }
        }
        if(V36TypographyAuditActive||FrontlineUIAssetsV8213::BaselineMode())
            RecordTypographyV36(Value,X,Y,W,H,TW,TH,Size,false,Align,TX,TY,Artwork().ControlRuntimeV62->GetPathName());
    }
    float RasterUnitsV825() const
    {
        const float DPI=Surface&&Surface->Canvas?Surface->Canvas->GetDPIScale():1.f;
        return FMath::Max(.001f,Scale)*FMath::Max(.001f,FMath::IsFinite(DPI)?DPI:1.f);
    }
    void AuditFontEdgesV825() const
    {
        if(!V825FontAuditActive||V825FontEdges.IsValid())return;
        auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;bool Good=true;
        auto Check=[&](const TCHAR* Name,bool Passed){auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(C));Good&=Passed;};
        Check(TEXT("v825_empty_text_has_no_draw_bounds"),RuntimeSizeV81(TEXT(""),18).IsNearlyZero()&&RuntimeSizeV81(TEXT(""),18,true).IsNearlyZero());
        Check(TEXT("v825_nonpositive_text_size_has_no_draw_bounds"),RuntimeSizeV81(TEXT("Hg"),0).IsNearlyZero()&&RuntimeSizeV81(TEXT("Hg"),-1).IsNearlyZero()&&RuntimeSizeV81(TEXT("Hg"),0,true).IsNearlyZero());
        Check(TEXT("v825_empty_or_nonpositive_wrap_returns_no_rows"),TextRows(TEXT(""),100,18,true).IsEmpty()&&TextRows(TEXT("Hg"),0,18,true).IsEmpty()&&TextRows(TEXT("Hg"),100,-1,true).IsEmpty());
        for(int32 Case=0;Case<3;++Case)
        {
            TGuardValue<bool> FaceScope(bPreferChineseFontV45,Case==2);const bool Control=Case==1;
            const FString First=Case==2?TEXT("国军"):TEXT("Hg"),Second=Case==2?TEXT("国"):TEXT("H");
            const auto A=RuntimeSizeV81(First,18,Control),B=RuntimeSizeV81(Second,18,Control),M=RuntimeSizeV81(First+TEXT("\n")+Second,18,Control);
            const auto Cached=RuntimeSizeV81(First+TEXT("\n")+Second,18,Control);
            Check(Case==0?TEXT("v825_plain_multiline_uses_maximum_row_width_and_full_height"):Case==1?TEXT("v825_control_multiline_uses_maximum_row_width_and_full_height"):TEXT("v825_chinese_multiline_uses_one_medium_face"),FMath::Abs(M.X-FMath::Max(A.X,B.X))<.05f&&FMath::Abs(M.Y-(A.Y+B.Y))<.05f&&M.Equals(Cached,.0001));
        }
        // Compare each spelling with uncached renderer metrics in both insertion orders.
        TGuardValue<bool> LatinScope(bPreferChineseFontV45,false);
        for(int32 Face=0;Face<2;++Face)for(int32 Order=0;Order<2;++Order)
        {
            const bool Control=Face==1;const float Size=Order==0?27.125f:31.125f;
            const FString First=Order==0?TEXT("RETURN"):TEXT("return"),Second=Order==0?TEXT("return"):TEXT("RETURN");
            auto Fresh=[&](const FString& Value){FMeasuredCanvasTextV62 Item(FVector2D::ZeroVector,FText::FromString(Value),RuntimeInfoV62(Value,Size,Control),FLinearColor::White);return Item.GetTextSize(Surface->Canvas->GetDPIScale())/RasterUnitsV825();};
            const FVector2D ExpectedFirst=Fresh(First),ExpectedSecond=Fresh(Second);
            const FVector2D FirstSize=RuntimeSizeV81(First,Size,Control),SecondSize=RuntimeSizeV81(Second,Size,Control);
            const bool MetricsMatch=FMath::Abs(ExpectedFirst.X-ExpectedSecond.X)>.5f&&FirstSize.Equals(ExpectedFirst,.05)&&SecondSize.Equals(ExpectedSecond,.05)&&RuntimeSizeV81(First,Size,Control).Equals(ExpectedFirst,.05)&&RuntimeSizeV81(Second,Size,Control).Equals(ExpectedSecond,.05);
            const FString MetricsName=FString::Printf(TEXT("v825_%s_case_metrics_%s_first"),Control?TEXT("control"):TEXT("plain"),Order==0?TEXT("upper"):TEXT("lower"));Check(*MetricsName,MetricsMatch);
            const auto FirstRows=TextRows(First,600,Size,!Control),SecondRows=TextRows(Second,600,Size,!Control);
            const bool RowsMatch=FirstRows.Num()==1&&SecondRows.Num()==1&&FirstRows[0].Equals(First,ESearchCase::CaseSensitive)&&SecondRows[0].Equals(Second,ESearchCase::CaseSensitive);
            const FString RowsName=FString::Printf(TEXT("v825_%s_case_rows_%s_first"),Control?TEXT("control"):TEXT("plain"),Order==0?TEXT("upper"):TEXT("lower"));Check(*RowsName,RowsMatch);
        }
        J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);
        J->SetNumberField(TEXT("canvas_dpi"),Surface->Canvas->GetDPIScale());J->SetNumberField(TEXT("output_scale"),Scale);V825FontEdges=J;
    }
    FVector2D RuntimeSizeV81(const FString& Value,float Size,bool Control=false) const
    {
        if(Value.IsEmpty()||Size<=0||!FMath::IsFinite(Size))return FVector2D::ZeroVector;
        // Font, raster size and output scale are part of the key. A language,
        // font or resolution change cannot reuse another face's measurements.
        static TBoundedTextCacheV8213<FVector2D,2048> Cache;
        const auto Info=RuntimeInfoV62(Value,Size,Control);
        const FString Key=FString::Printf(TEXT("%p|%s|%.3f|%.5f|%.5f|%s"),Info.FontObject.Get(),*Info.TypefaceFontName.ToString(),Info.Size,Scale,Surface->Canvas->GetDPIScale(),*Value);
        if(const auto* Found=Cache.Find(Key))return *Found;
        FMeasuredCanvasTextV62 Item(FVector2D::ZeroVector,FText::FromString(Value),Info,FLinearColor::White);
        // Runtime Canvas glyphs rasterize at DPI, but their batch cancels that
        // DPI before the shared panel transform. Convert atlas pixels to the
        // same logical units as the panel, once; do not enlarge centred offsets.
        const FVector2D Result=Item.GetTextSize(Surface->Canvas->GetDPIScale())/RasterUnitsV825();
        Cache.Add(Key,Result);return Result;
    }
#include "WarfareFrontlineTypographyV8210.inl"
    // Every constrained label is measured after localization against the same
    // renderer used to draw it. Centre/right alignment never changes its box.
    void Label(const FString& Source,float X,float Y,float W,float H,float Size=18,const FLinearColor& Ink=Paper,int32 Align=0,bool Fixed=false,EFrontlineTextRoleV8210 Role=EFrontlineTextRoleV8210::Auto) const
    {
        if(!bLegacyTypographyV8210||Role!=EFrontlineTextRoleV8210::Auto)
        {const auto V=DisplayText(Source);RoleLabel(Role==EFrontlineTextRoleV8210::Auto?(bTooltipTypographyV8210&&Size>=19?EFrontlineTextRoleV8210::Headline:DefaultRoleV8210(V)):Role,Source,X,Y,W,H,Role==EFrontlineTextRoleV8210::Auto?DefaultEmV8210(V,Size):Size,Ink,Align);return;}
        const FString Value=DisplayText(Source);
        if(Value.IsEmpty()||W<=0||H<=0||Size<=0||!FMath::IsFinite(W)||!FMath::IsFinite(H)||!FMath::IsFinite(Size))return;
        if(IsControlText(Value)){ControlLabelV70(Value,X,Y,W,H,Size,Ink,Align,Fixed);return;}
        TGuardValue<bool> CJKLabelScope(bPreferChineseFontV45,bPreferChineseFontV45||ContainsCJKV45(Value));
        // Use the same rounded font size and full line bounds as the actual
        // Canvas draw. Approximate CJK optical height used to centre the wrong box.
        const float Padding=1.f;
        FVector2D Metrics=RuntimeSizeV81(Value,Size);
        if(!Fixed)
        {
            Size*=FMath::Min(1.f,FMath::Min(FMath::Max(0.f,W-2*Padding)/FMath::Max(1.f,float(Metrics.X)),FMath::Max(0.f,H-2*Padding)/FMath::Max(1.f,float(Metrics.Y))));
            if(Size<=0)return;
            for(int32 I=0;I<16;++I){Metrics=RuntimeSizeV81(Value,Size);if(Metrics.X<=W-2*Padding+.1f&&Metrics.Y<=H-2*Padding+.1f)break;Size=FMath::Max(1.f,Size-.25f);}
        }
        // The last bounded fit iteration may reduce Size; measure what we draw.
        Metrics=RuntimeSizeV81(Value,Size);
        const float TW=Metrics.X,TH=Metrics.Y;
        const float TX=X+(Align==1?(W-TW)*.5f:Align==2?W-Padding-TW:Padding),TY=Y+(H-TH)*.5f;
        if(!Value.Contains(TEXT("\n")))PlainText(Value,TX,TY,Size,Ink);
        else
        {
            TArray<FString> Rows;Value.ParseIntoArray(Rows,TEXT("\n"),false);
            const float Leading=Rows.IsEmpty()?0.f:TH/Rows.Num();
            for(int32 I=0;I<Rows.Num();++I)
            {
                const float RowWidth=RuntimeSizeV81(Rows[I],Size).X;
                PlainText(Rows[I],X+(Align==1?(W-RowWidth)*.5f:Align==2?W-Padding-RowWidth:Padding),TY+I*Leading,Size,Ink);
            }
        }
        if(V36TypographyAuditActive||FrontlineUIAssetsV8213::BaselineMode())
            RecordTypographyV36(Value,X,Y,W,H,TW,TH,Size,false,Align,TX,TY,ChineseFont&&ContainsCJKV45(Value)?ChineseFont->GetPathName():Font->GetPathName());
    }

    void Caption(const FString& Source,float X,float Y,float W,float H,float Size=13,const FLinearColor& Ink=Paper) const
    {
        if(!bLegacyTypographyV8210){RoleLabel(EFrontlineTextRoleV8210::Plate,Source,X,Y,W,H,DefaultEmV8210(DisplayText(Source),Size),Ink,1);return;}
        const FString Value=DisplayText(Source).ToUpper();
        if(IsControlText(Value))ControlLabelV70(Value,X,Y,W,H,Size,Ink,1);
        else Label(Source,X,Y,W,H,Size,Ink,1);
    }
    TArray<FString> TextRows(const FString& Value,float Width,float Size,bool Plain) const
    {
        if(Value.IsEmpty()||Width<=0||Size<=0||!FMath::IsFinite(Width)||!FMath::IsFinite(Size))return {};
        static TBoundedTextCacheV8213<TArray<FString>,512> Cache;
        const auto Info=RuntimeInfoV62(Value,Size,!Plain&&IsControlText(Value));
        const FString Key=FString::Printf(TEXT("%p|%s|%.3f|%.4f|%.5f|%.5f|%s"),Info.FontObject.Get(),*Info.TypefaceFontName.ToString(),Info.Size,Width,Scale,Surface->Canvas->GetDPIScale(),*Value);
        if(const auto* Found=Cache.Find(Key))return *Found;
        TArray<FString> Rows;
        // Value has already been localized. Wrapped fragments are never source keys.
        const bool Control=!Plain&&IsControlText(Value);
        auto Measure=[&](const FString& V){return RuntimeWidthV62(V,Size,Control);};
        // Unicode line-break boundaries keep punctuation with its phrase and
        // keep Latin names/numbers intact inside Chinese prose.
        TArray<FString> Blocks;Value.ParseIntoArray(Blocks,TEXT("\n"),false);
        for(const FString& Block:Blocks)
        {
            FString Row;auto Iterator=FBreakIterator::CreateLineBreakIterator();Iterator->SetStringRef(Block);
            int32 Begin=Iterator->ResetToBeginning();
            for(int32 End=Iterator->MoveToNext();End!=INDEX_NONE;Begin=End,End=Iterator->MoveToNext())
            {
                FString Word=Block.Mid(Begin,End-Begin);
                if(!Row.IsEmpty()&&Measure(Row+Word.TrimEnd())>Width){Rows.Add(Row.TrimEnd());Row.Empty();}
                if(Row.IsEmpty())Word.TrimStartInline();
                if(Measure(Word.TrimEnd())>Width)
                {
                    // An exceptionally long identifier still stays in its column.
                    for(TCHAR C:Word){if(!Row.IsEmpty()&&Measure(Row+FString::Chr(C))>Width){Rows.Add(Row.TrimEnd());Row.Empty();}Row.AppendChar(C);}
                }
                else Row+=Word;
            }
            Rows.Add(Row.TrimEnd());
        }
        Cache.Add(Key,Rows);return Rows;
    }
    float Paragraph(const FString& Source,float X,float Y,float W,float H,float Size=17,const FLinearColor& Ink=Muted,bool Plain=false,bool Fixed=false,EFrontlineTextRoleV8210 Role=EFrontlineTextRoleV8210::Auto) const
    {
        if(!bLegacyTypographyV8210||Role!=EFrontlineTextRoleV8210::Auto)return RoleParagraph(Role==EFrontlineTextRoleV8210::Auto?(bTooltipTypographyV8210?EFrontlineTextRoleV8210::DarkText:EFrontlineTextRoleV8210::Text):Role,Source,X,Y,W,H,Role==EFrontlineTextRoleV8210::Auto?DefaultEmV8210(DisplayText(Source),Size):Size,Ink);
        if(W<=0||H<=0||Size<=0||!FMath::IsFinite(W)||!FMath::IsFinite(H)||!FMath::IsFinite(Size))return Y;
        const FString Value=DisplayText(Source);
        const bool CJK=ContainsCJKV45(Value);
        // Latin names inside a Chinese paragraph retain the same Medium face
        // and optical size during both line breaking and final row drawing.
        TGuardValue<bool> CJKParagraphScope(bPreferChineseFontV45,bPreferChineseFontV45||CJK);
        // Select one face for the whole paragraph. An unsupported symbol must
        // not switch just one wrapped row to a thinner, differently sized font.
        // The ordinary face lacks acute S; the already cooked Oswald face has it.
        // Resolve this once for the whole paragraph, including explicitly plain tooltips.
        if((Value.Contains(TEXT("\u015A"))||Value.Contains(TEXT("\u015B")))&&IsControlText(Value))Plain=false;
        Plain=Plain||!IsControlText(Value);
        TArray<FString> Rows;float Leading=0,LineHeight=0;const float Minimum=FMath::Min(Size,CJK?15.f:13.f);
        for(;;)
        {
            Rows=TextRows(Value,W,Size,Plain);LineHeight=0;
            for(const auto& Row:Rows)LineHeight=FMath::Max(LineHeight,Plain?float(RuntimeSizeV81(Row,Size).Y):RuntimeInfoV62(Row,Size,true).Size*(4.f/3.f)*.810f/FMath::Max(.001f,Scale));
            Leading=FMath::Max(LineHeight+5.f,Size*(CJK?1.25f:1.20f)+5.f);
            if(Fixed||FMath::Max(0,Rows.Num()-1)*Leading+LineHeight<=H+.1f||Size<=Minimum)break;Size=FMath::Max(Minimum,Size-.5f);
        }
        const int32 Capacity=FMath::Max(1,1+FMath::FloorToInt((H-LineHeight)/FMath::Max(1.f,Leading)));const bool Truncated=!Fixed&&Rows.Num()>Capacity;
        if(Truncated)
        {
            Rows.SetNum(Capacity);auto& Tail=Rows.Last();
            while(!Tail.IsEmpty()&&RuntimeWidthV62(Tail+TEXT("…"),Size,!Plain)>W)Tail.LeftChopInline(1);
            Tail+=TEXT("…");
        }
        float ActualW=0;
        for(int32 I=0;I<Rows.Num();++I)
        {
            if(Plain){RuntimeTextV62(Rows[I],X,Y+I*Leading,Size,Ink,false);AuditDrawV45(Rows[I],Size);}
            else ControlText(Rows[I],X,Y+I*Leading,Size,Ink);
            ActualW=FMath::Max(ActualW,RuntimeWidthV62(Rows[I],Size,!Plain));
        }
        const float ActualH=Rows.IsEmpty()?0.f:(Rows.Num()-1)*Leading+LineHeight;
        if(V45HUDAuditActive&&CJK)for(int32 I=1;I<Rows.Num();++I)if(!Rows[I].IsEmpty()&&ClosingPunctuationV45(Rows[I][0]))V45InvalidChineseBreaks.Add(Rows[I]);
        if(V36TypographyAuditActive||FrontlineUIAssetsV8213::BaselineMode())
            RecordTypographyV36(Value,X,Y,W,H,ActualW,ActualH,Size,Truncated,-1,0,0,FString(),&Rows);
        return Y+ActualH;
    }
#include "WarfareFrontlineTooltipCanvasV83.inl"
    FVector2D Screen(float X,float Y) const { return FVector2D(OX+(X+ReplayXV83)*Scale,OY+(Y+ReplayYV83)*Scale); }
    FVector2D Local(const FVector2D& P) const { return FVector2D((P.X-OX)/Scale,(P.Y-OY)/Scale); }
    bool PointerPosition(FVector2D& Out) const {Out=Mouse;return Mouse.X>=0&&Mouse.X<=1920&&Mouse.Y>=0&&Mouse.Y<=1080;}
    bool Hovered(float X,float Y,float W,float H) const { return Mouse.X>=X && Mouse.X<=X+W && Mouse.Y>=Y && Mouse.Y<=Y+H; }
    void Rect(float X,float Y,float W,float H,const FLinearColor& Color) const
    { if(RecordingV83){OpV83(ETooltipOpV83::Rect,X,Y,W,H,Color);return;} HUD.DrawRect(MotionInkV83(Color),OX+(X+ReplayXV83)*Scale,OY+(Y+ReplayYV83)*Scale,W*Scale,H*Scale); }
    void Gradient(float X,float Y,float W,float H,const FLinearColor& Start,const FLinearColor& End,bool Vertical=false) const
    {
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Gradient,X,Y,W,H,Start);O.End=End;O.Vertical=Vertical;return;}
        if (!Surface->DefaultTexture || !Surface->DefaultTexture->GetResource()) return;
        const FLinearColor MotionStart=MotionInkV83(Start),MotionEnd=MotionInkV83(End);
        const FVector2D A=Screen(X,Y),B=Screen(X+W,Y),C=Screen(X+W,Y+H),D=Screen(X,Y+H);
        TArray<FCanvasUVTri> Triangles; Triangles.SetNum(2);
        Triangles[0].V0_Pos=A; Triangles[0].V1_Pos=B; Triangles[0].V2_Pos=C;
        Triangles[1].V0_Pos=A; Triangles[1].V1_Pos=C; Triangles[1].V2_Pos=D;
        Triangles[0].V0_Color=MotionStart; Triangles[0].V1_Color=Vertical?MotionStart:MotionEnd; Triangles[0].V2_Color=MotionEnd;
        Triangles[1].V0_Color=MotionStart; Triangles[1].V1_Color=MotionEnd; Triangles[1].V2_Color=Vertical?MotionEnd:MotionStart;
        FCanvasTriangleItem Item(Triangles,Surface->DefaultTexture->GetResource());
        Item.BlendMode=SE_BLEND_Translucent;
        Surface->DrawItem(Item);
    }
    void Line(float X,float Y,float EX,float EY,const FLinearColor& Color,float Width=1) const
    {if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Line,X,Y,EX,EY,Color);O.Width=Width;return;} HUD.DrawLine(OX+(X+ReplayXV83)*Scale,OY+(Y+ReplayYV83)*Scale,OX+(EX+ReplayXV83)*Scale,OY+(EY+ReplayYV83)*Scale,MotionInkV83(Color),Width*Scale); }
    void Triangle(const FVector2D& A,const FVector2D& B,const FVector2D& C,const FLinearColor& Ink) const
    {
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Triangle,A.X,A.Y,B.X,B.Y,Ink);O.Third=C;return;}
        if(!Surface->DefaultTexture||!Surface->DefaultTexture->GetResource())return;
        TArray<FCanvasUVTri> T;T.SetNum(1);T[0].V0_Pos=Screen(A.X,A.Y);T[0].V1_Pos=Screen(B.X,B.Y);T[0].V2_Pos=Screen(C.X,C.Y);
        T[0].V0_UV=T[0].V1_UV=T[0].V2_UV=FVector2D::ZeroVector;
        T[0].V0_Color=T[0].V1_Color=T[0].V2_Color=MotionInkV83(Ink);
        FCanvasTriangleItem Item(T,Surface->DefaultTexture->GetResource());Item.BlendMode=SE_BLEND_Translucent;Surface->DrawItem(Item);
    }
    void Box(float X,float Y,float W,float H,const FLinearColor& Fill=Panel,const FLinearColor& Edge=Border) const
    {
        if(Artwork().CommandSkinV31 && Fill.A>.4f && W>24 && H>24){Skin(1,X,Y,W,H,7);return;}
        Rect(X,Y,W,H,Fill); Line(X,Y,X+W,Y,Edge); Line(X,Y+H,X+W,Y+H,Edge);
        Line(X,Y,X,Y+H,Edge); Line(X+W,Y,X+W,Y+H,Edge);
    }
    // Rasterize at the output pixel size. A large Canvas Scale only stretches
    // the old cached glyph; it does not request a higher-resolution font glyph.
    FSlateFontInfo RuntimeInfoV62(const FString& Value,float Size,bool Control) const
    {
        const bool CJK=bPreferChineseFontV45||ContainsCJKV45(Value);
        UFont* Face=Control&&Artwork().ControlRuntimeV62?Artwork().ControlRuntimeV62.Get():CJK?ChineseFont:Font;
        auto Info=Face->GetLegacySlateFontInfo();
        // Oswald: 810-unit cap height in a 1000-unit em; Slate points are 4/3 px.
        const float Raster=Control?Size*Scale*(.75f/ .810f):Info.Size*Size*OpticalTextScaleV45(Value)/(CJK?ChineseFontHeight:FontHeight)*Scale;
        Info.Size=FMath::Max(1.f,FMath::RoundToFloat(Raster*2.f)*.5f);
        return Info;
    }
    float RuntimeWidthV62(const FString& Value,float Size,bool Control) const
    {
        return RuntimeSizeV81(Value,Size,Control).X;
    }
    void RuntimeTextV62(const FString& Value,float X,float Y,float Size,const FLinearColor& Colour,bool Control) const
    {
        if(Value.IsEmpty()||Size<=0||!FMath::IsFinite(Size))return;
        const auto Info=RuntimeInfoV62(Value,Size,Control);
        // Canvas positions by the font ascender, while layout uses visible caps.
        // Remove the font's 383-unit ascender-to-cap gap before centring labels.
        const float TopInset=Control?Info.Size*(4.f/3.f)*.383f:0.f;
        const FVector2D Position(FMath::RoundToFloat(OX+X*Scale),FMath::RoundToFloat(OY+Y*Scale-TopInset));
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Text,(Position.X-OX)/Scale,(Position.Y-OY)/Scale,0,0,Colour);O.Text=FText::FromString(Value);O.Font=Info;return;}
        FCanvasTextItem Item(Position+FVector2D(ReplayXV83*Scale,ReplayYV83*Scale),FText::FromString(Value),Info,MotionInkV83(Colour));
        Item.BlendMode=SE_BLEND_Translucent;
        Surface->DrawItem(Item);
        if(V825FontAuditActive)
        {
            const FString Key=FString::Printf(TEXT("%p|%.3f|%.5f|%.5f|%s"),Info.FontObject.Get(),Info.Size,Scale,Surface->Canvas->GetDPIScale(),*Value);
            if(!V825FontDraws.Contains(Key))
            {
                // DrawnSize comes from the actual Canvas glyph loop, not the
                // measuring service or the label's calculated alignment box.
                const FVector2D Drawn=Item.DrawnSize/RasterUnitsV825(),Measured=RuntimeSizeV81(Value,Size,Control);
                auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("text"),Value);J->SetStringField(TEXT("font"),Info.FontObject->GetPathName());
                J->SetNumberField(TEXT("raster_point_size"),Info.Size);J->SetNumberField(TEXT("canvas_dpi"),Surface->Canvas->GetDPIScale());J->SetNumberField(TEXT("output_scale"),Scale);
                J->SetBoolField(TEXT("hdr_sdr_composite"),bHDRComposite);
                J->SetNumberField(TEXT("actual_draw_width"),Drawn.X);J->SetNumberField(TEXT("actual_draw_height"),Drawn.Y);
                J->SetNumberField(TEXT("measure_width"),Measured.X);J->SetNumberField(TEXT("measure_height"),Measured.Y);
                J->SetNumberField(TEXT("width_error"),FMath::Abs(Drawn.X-Measured.X));J->SetNumberField(TEXT("height_error"),FMath::Abs(Drawn.Y-Measured.Y));
                V825FontDraws.Add(Key,J);
            }
        }
    }
    float TextWidth(const FString& Source,float Size) const
    {const FString Value=DisplayText(Source);if(!bLegacyTypographyV8210)return RoleWidth(DefaultRoleV8210(Value),Source,DefaultEmV8210(Value,Size));return RuntimeWidthV62(Value,Size,IsControlText(Value));}
    float PlainTextWidth(const FString& Source,float Size) const
    {if(!bLegacyTypographyV8210)return RoleWidth(EFrontlineTextRoleV8210::Text,Source,DefaultEmV8210(DisplayText(Source),Size));return RuntimeWidthV62(DisplayText(Source),Size,false);}
    void Text(const FString& Source,float X,float Y,float Size=18,const FLinearColor& Color=Paper,EFrontlineTextRoleV8210 Role=EFrontlineTextRoleV8210::Auto) const
    {const FString Value=DisplayText(Source);if(!bLegacyTypographyV8210||Role!=EFrontlineTextRoleV8210::Auto){RoleText(Role==EFrontlineTextRoleV8210::Auto?DefaultRoleV8210(Value):Role,Source,X,Y,Role==EFrontlineTextRoleV8210::Auto?DefaultEmV8210(Value,Size):Size,Color);return;}if(IsControlText(Value))ControlText(Value,X,Y,Size,Color);else PlainText(Value,X,Y,Size,Color);}
    bool IsControlText(const FString& Value) const
    {
        if(bPreferChineseFontV45||!Artwork().ControlRuntimeV62||Value.IsEmpty())return false;
        // The runtime Oswald face contains Polish acute S; the old V31 atlas does not.
        // Keep the existing routing for every other character and for CJK text.
        for(TCHAR C:Value)if((C>=0x2190&&C<=0x2193)||(C!='\n'&&C!=0x015A&&C!=0x015B&&!ControlGlyphV31(C)))return false;
        return true;
    }
    void ControlText(const FString& Value,float X,float Y,float Size,const FLinearColor& Color) const
    {
        AuditDrawV45(Value,Size);
        RuntimeTextV62(Value,X-.45f,Y-.45f,Size,FLinearColor(.004f,.006f,.005f,Color.A*.72f),true);
        RuntimeTextV62(Value,X,Y,Size,Color,true);
    }
    // Decorative skin UVs are nine-sliced; photographs always keep their aspect.
    void Skin(int32 Slot,float X,float Y,float W,float H,float Edge=9,FLinearColor Tint=FLinearColor::White,bool Mirror=false) const
    {
        if(Slot<4&&ConsoleSkinV79(Slot,X,Y,W,H,Edge,Tint))return;
        auto* T=Artwork().CommandSkinV31.Get();if(!FrontlineTextureReady(T)||W<=0||H<=0)return;
        struct FPixelRect {float X,Y,Z,W;};
        const FPixelRect Rects[]={ {2,2,439,409},{446,3,884,410},{892,4,1329,410},{1336,4,1770,410},
            {2,418,439,882},{447,418,884,882},{893,418,1327,882},{1337,418,1770,882} };
        const auto P=Rects[FMath::Clamp(Slot,0,7)];
        // Rivets remain entirely inside the corners. Corners are square in
        // source and destination; only undecorated straight rails stretch.
        const float Cut=Slot==0?56.f:Slot==2?55.f:Slot==3?32.f:36.f;
        const float DX=FMath::Min(Slot==0?FMath::Max(Edge,15.f):Edge,W*.3f),DY=FMath::Min(Slot==0?FMath::Max(Edge,15.f):Edge,H*.3f);
        const float SX[]={X,X+DX,X+W-DX,X+W},SY[]={Y,Y+DY,Y+H-DY,Y+H};
        const float SU[]={P.X/T->GetSizeX(),(P.X+Cut)/T->GetSizeX(),(P.Z-Cut)/T->GetSizeX(),P.Z/T->GetSizeX()};
        const float SV[]={P.Y/T->GetSizeY(),(P.Y+Cut)/T->GetSizeY(),(P.W-Cut)/T->GetSizeY(),P.W/T->GetSizeY()};
        for(int32 R=0;R<3;++R)for(int32 C=0;C<3;++C)
        {
            if((Slot==0||Slot==1) && R==1 && C==1)
            {
                // Keep paint grain square and near native density, even on
                // ultrawide rails. Mirroring joins identical edge texels.
                constexpr float Tile=256.f;
                int32 Row=0;
                for(float TY=SY[1];TY<SY[2]-.01f;TY+=Tile,++Row)
                {
                    const float TH=FMath::Min(Tile,SY[2]-TY);int32 Col=0;
                    for(float TX=SX[1];TX<SX[2]-.01f;TX+=Tile,++Col)
                    {
                        const float TW=FMath::Min(Tile,SX[2]-TX);
                        const float TU=Col%2?.225f:.025f,TV=Row%2?.95f:.55f;
                        TextureV83(T,OX+TX*Scale,OY+TY*Scale,TW*Scale,TH*Scale,TU,TV,(Col%2?-.20f:.20f)*TW/Tile,(Row%2?-.40f:.40f)*TH/Tile,Tint);
                    }
                }
            }
            else
            {
                const int32 SourceC=Mirror?2-C:C;const float StartU=Mirror?SU[SourceC+1]:SU[SourceC];
                const float SpanU=(SU[SourceC+1]-SU[SourceC])*(Mirror?-1.f:1.f);
                TextureV83(T,OX+SX[C]*Scale,OY+SY[R]*Scale,(SX[C+1]-SX[C])*Scale,(SY[R+1]-SY[R])*Scale,StartU,SV[R],SpanU,SV[R+1]-SV[R],Tint);
            }
        }
    }
    void InstrumentIcon(int32 Slot,float X,float Y,float W,float H) const
    {
        if((Slot==5||Slot==6)&&ConsoleToolV79(Slot==5?8:9,X,Y,W,H))return;
        auto* T=Artwork().CommandSkinV31.Get();if(!T)return;
        // Crop the illustration from its matching metal surround, never stretch it.
        Image(T,X,Y,W,H,FLinearColor(.92f,.93f,.92f,1),(Slot%4+.10f)*.25f,(Slot/4+.10f)*.5f,.20f,.40f,false,true);
    }
    void MaterialTile(UMaterialInterface* Material,float X,float Y,float W,float H,
        const FTexture* Coverage,float U=0,float V=0,float UW=1,float VH=1,FLinearColor Tint=FLinearColor::White) const
    {
        if(!Material)return;
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Material,X,Y,W,H,Tint);O.Material=Material;O.Coverage=Coverage;O.U=U;O.V=V;O.UW=UW;O.VH=VH;return;}
        Tint=MotionInkV83(Tint);
        const FVector2D P=Screen(X,Y),Size(W*Scale,H*Scale);
        FCanvasTileItem MaterialItem(P,Material->GetRenderProxy(),Size,FVector2D(U,V),FVector2D(U+UW,V+VH));
        MaterialItem.SetColor(Tint);Surface->DrawItem(MaterialItem);
        if(bHDRComposite && Coverage)
        {
            static TRefCountPtr<FFrontlineCanvasCoverage> Parameters=new FFrontlineCanvasCoverage;
            FCanvasTileItem Item(P,Coverage,Size,FVector2D(U,V),FVector2D(U+UW,V+VH),FLinearColor(1,1,1,Tint.A));
            Item.BatchedElementParameters=Parameters.GetReference();
            Item.BlendMode=SE_BLEND_Opaque;
            Surface->DrawItem(Item);
        }
    }
    void ReliefText(const FString& Value,float X,float Y,float Size=18,const FLinearColor& Color=Paper) const
    {
        if(IsControlText(Value))ControlText(Value,X,Y,Size,Color);else PlainText(Value,X,Y,Size,Color);
    }
    void PlainText(const FString& Source,float X,float Y,float Size=18,const FLinearColor& Color=Paper) const
    {
        const FString Value=DisplayText(Source);
        if(!bLegacyTypographyV8210){RoleText(bTooltipTypographyV8210?EFrontlineTextRoleV8210::DarkText:EFrontlineTextRoleV8210::Text,Source,X,Y,DefaultEmV8210(Value,Size),Color);return;}
        RuntimeTextV62(Value,X,Y,Size,Color,false);
        AuditDrawV45(Value,Size);
    }
    void Right(const FString& Value,float RightX,float Y,float Size=18,const FLinearColor& Color=Paper) const
    { Text(Value,RightX-TextWidth(Value,Size),Y,Size,Color); }
    void Centre(const FString& Value,float X,float Y,float Size=18,const FLinearColor& Color=Paper) const
    { Text(Value,X-TextWidth(Value,Size)*.5f,Y,Size,Color); }
    float Wrap(const FString& Source,float X,float Y,float Width,float Size=17,const FLinearColor& Color=Muted,bool Plain=false,EFrontlineTextRoleV8210 Role=EFrontlineTextRoleV8210::Auto) const
    {
        if(!bLegacyTypographyV8210||Role!=EFrontlineTextRoleV8210::Auto)return RoleParagraph(Role==EFrontlineTextRoleV8210::Auto?(bTooltipTypographyV8210?EFrontlineTextRoleV8210::DarkText:EFrontlineTextRoleV8210::Text):Role,Source,X,Y,Width,1080.f,Role==EFrontlineTextRoleV8210::Auto?DefaultEmV8210(DisplayText(Source),Size):Size,Color);
        const FString Value=DisplayText(Source);
        TGuardValue<bool> CJKWrapScope(bPreferChineseFontV45,bPreferChineseFontV45||ContainsCJKV45(Value));
        // CJK has no word spaces: break at ideographs, retaining Latin words as units.
        TArray<FString> Words; FString Latin;
        auto Closing=[](TCHAR C){return C==0x3002||C==0xff0c||C==0x3001||C==0xff1b||C==0xff1a||C==0xff01||C==0xff1f||C==0xff09||C==0x300d||C==0x300f||C==0x3011||C==0x201d;};
        for (int32 I=0;I<Value.Len();++I)
        {
            const TCHAR C=Value[I];
            if (C>=0x2E80 || FChar::IsWhitespace(C))
            {
                if (!Latin.IsEmpty()) { Words.Add(Latin); Latin.Empty(); }
                if(Closing(C)&&!Words.IsEmpty()&&Words.Last()!=TEXT("\n"))Words.Last().AppendChar(C);
                else Words.Add(FString::Chr(C));
            }
            else Latin.AppendChar(C);
        }
        if (!Latin.IsEmpty()) Words.Add(Latin);
        FString Row; float CY=Y; const float LineHeight=Size*OpticalTextScaleV45(Value)+5;
        auto RowText=[&](const FString& Row,float RowY) {if (Plain) PlainText(Row,X,RowY,Size,Color);else Text(Row,X,RowY,Size,Color);};
        for (const FString& Word:Words)
        {
            if (Word==TEXT("\n")) { RowText(Row,CY); CY+=LineHeight; Row.Empty(); continue; }
            const FString Candidate=Row+Word;
            if (!Row.IsEmpty() && (Plain?PlainTextWidth(Candidate,Size):TextWidth(Candidate,Size))>Width) { RowText(Row,CY); CY+=LineHeight; Row=Word; }
            else Row=Candidate;
        }
        if (!Row.IsEmpty()) { RowText(Row,CY); CY+=LineHeight; }
        return CY;
    }
    void Image(UTexture2D* Texture,float X,float Y,float W,float H,FLinearColor Tint=FLinearColor::White,
        float U=0,float V=0,float UW=1,float VH=1,bool bGrayscale=false,bool bContain=false) const
    {
        if(!FrontlineTextureReady(Texture)||W<=0||H<=0||UW<=0||VH<=0||Tint.A<=0)return;
        const auto P=PlaceFrontlineImage(Texture,X,Y,W,H,U,V,UW,VH,bContain);
        RecordV27ImagePlacement(Texture,P,bContain);
        X=P.X;Y=P.Y;W=P.W;H=P.H;U=P.U;V=P.V;UW=P.UW;VH=P.VH;
        if(bGrayscale)
        {
            auto& A=Artwork();
            if(!A.GrayscalePhotoMaterial)
            {
                static bool Reported=false;if(!Reported){Reported=true;UE_LOG(LogTemp,Error,TEXT("V26 grayscale photo material missing; unavailable artwork is not drawn as false color."));}
                return;
            }
            auto& M=A.GrayscalePhotos.FindOrAdd(Texture);
            if(!M){M=UMaterialInstanceDynamic::Create(A.GrayscalePhotoMaterial,GetTransientPackage());M->SetTextureParameterValue(TEXT("Photo"),Texture);}
            // One immutable material per source photo avoids late canvas batches
            // observing another card's texture. RGB is a true neutral luminance.
            MaterialTile(M,X,Y,W,H,Texture->GetResource(),U,V,UW,VH);
            return;
        }
        auto& A=Artwork();
        bool* Classified=A.PaintingClassificationV73.Find(Texture);
        if(!Classified)
        {
            const FString Name=Texture->GetName();bool Painting=false;
            for(const TCHAR* Prefix:{TEXT("T_Portrait_"),TEXT("T_MainMenu_"),TEXT("T_Keyart"),TEXT("T_Tech"),TEXT("T_Support"),TEXT("T_Illustrations"),TEXT("T_Archive"),TEXT("T_Background_"),TEXT("T_Commander"),TEXT("T_ArmourTech"),TEXT("T_Armour_"),TEXT("T_Shrapnel_V64"),TEXT("T_AttachmentPortraits_V51")})
                Painting|=Name.StartsWith(Prefix);
            Classified=&A.PaintingClassificationV73.Add(Texture,Painting);
        }
        if(*Classified&&A.PaintingMaterialV73)
        {
            auto& M=A.PaintingsV73.FindOrAdd(Texture);
            if(!M){M=UMaterialInstanceDynamic::Create(A.PaintingMaterialV73,GetTransientPackage());M->SetTextureParameterValue(TEXT("Photo"),Texture);}
            // Tint travels with the Canvas vertices. No mutable per-draw MID
            // parameters can leak between queued artwork tiles or fade states.
            MaterialTile(M,X,Y,W,H,Texture->GetResource(),U,V,UW,VH,Tint);return;
        }
        TextureV83(Texture,OX+X*Scale,OY+Y*Scale,W*Scale,H*Scale,U,V,UW,VH,Tint);
    }
    void HangingImageV8216(UTexture2D* T,float X,float Y,float W,float H,float Angle)const
    {
        if(!FrontlineTextureReady(T))return;
        auto P=PlaceFrontlineImage(T,X,Y,W,H,0,0,1,1,true);P.Y=Y;
        FCanvasTileItem Tile(Screen(P.X,P.Y),T->GetResource(),FVector2D(P.W*Scale,P.H*Scale),FLinearColor(.90f,.91f,.86f,1));
        Tile.PivotPoint=FVector2D(.5f,0);Tile.Rotation.Yaw=Angle;Tile.BlendMode=SE_BLEND_Translucent;Surface->DrawItem(Tile);
    }
    void MedalOverlayV8216(UTexture2D* T,float X,float Y,float W,float H,float Angle,float AX,float AY,float U,float V,float UW,float VH)const
    {
        if(!FrontlineTextureReady(T)||W<=0||H<=0)return;
        const auto P=PlaceFrontlineImage(T,X,Y,W,H,U,V,UW,VH,true);
        FCanvasTileItem Tile(Screen(P.X,P.Y),T->GetResource(),FVector2D(P.W*Scale,P.H*Scale),FVector2D(P.U,P.V),FVector2D(P.U+P.UW,P.V+P.VH),FLinearColor(.90f,.91f,.86f,1));
        Tile.PivotPoint=FVector2D((AX-P.X)/P.W,(AY-P.Y)/P.H);Tile.Rotation.Yaw=Angle;Tile.BlendMode=SE_BLEND_Translucent;Surface->DrawItem(Tile);
    }
    void ImageCover(UTexture2D* Texture,float X,float Y,float W,float H,FLinearColor Tint=FLinearColor::White) const
    {Image(Texture,X,Y,W,H,Tint);}
    void ImageContain(UTexture2D* Texture,float X,float Y,float W,float H,FLinearColor Tint=FLinearColor::White) const
    {Image(Texture,X,Y,W,H,Tint,0,0,1,1,false,true);}
    void ArchivePhotoV72(UTexture2D* Texture,UTexture2D* Archive,int32 CardKey,float X,float Y,float W,float H,
        float U,float V,float UW,float VH,float AU,float AV,float AUW,float AVH,float ColourReveal) const
    {
        auto& Art=Artwork();
        if(!FrontlineTextureReady(Texture)||W<=0||H<=0||UW<=0||VH<=0)return;
        // No silent return to a grayscale shader if the authored photo is missing.
        if(!FrontlineTextureReady(Archive)||!Art.ArchivePhotoMaterialV72)
        {Image(Texture,X,Y,W,H,FLinearColor::White,U,V,UW,VH,false,true);return;}
        auto& Material=Art.ArchivePhotosV72.FindOrAdd(CardKey);
        if(!Material)Material=UMaterialInstanceDynamic::Create(Art.ArchivePhotoMaterialV72,GetTransientPackage());
        Material->SetTextureParameterValue(TEXT("Photo"),Texture);
        Material->SetTextureParameterValue(TEXT("Archive"),Archive);
        Material->SetVectorParameterValue(TEXT("Region"),FLinearColor(U,V,UW,VH));
        Material->SetVectorParameterValue(TEXT("ArchiveRegion"),FLinearColor(AU,AV,AUW,AVH));
        Material->SetVectorParameterValue(TEXT("ArchiveBalance"),FrontlineArchiveToneV833::Balance(Archive,AU,AV));
        Material->SetVectorParameterValue(TEXT("Texel"),FLinearColor(1.f/Texture->GetSizeX(),1.f/Texture->GetSizeY(),1.f/Archive->GetSizeX(),1.f/Archive->GetSizeY()));
        Material->SetScalarParameterValue(TEXT("ColourReveal"),FMath::Clamp(ColourReveal,0.f,1.f));
        const bool ArmourCard=(CardKey>=64&&CardKey<80)||(CardKey>=192&&CardKey<208);
        Material->SetScalarParameterValue(TEXT("ColourChromaScale"),ArmourCard?(CardKey%128==72?1.14f:1.08f):1.f);
        const auto P=PlaceFrontlineImage(Texture,X,Y,W,H,U,V,UW,VH,true);RecordV27ImagePlacement(Texture,P,true);
        MaterialTile(Material,P.X,P.Y,P.W,P.H,Texture->GetResource(),P.U,P.V,P.UW,P.VH);
    }
    void Flag(UTexture2D* Texture,float X,float Y,float W,float H,const FVector4f& Stain,float Time,float Seed=0.f) const
    {
        auto* Mask=Artwork().FlagAbsorption.Get();auto* Liquid=Artwork().FlagSplashesV32.Get();
        if(!FrontlineTextureReady(Texture)||!FrontlineTextureReady(Mask)||!FrontlineTextureReady(Liquid))return;
        const auto P=PlaceFrontlineImage(Texture,X,Y,W,H,0,0,1,1,true);
        RecordV27ImagePlacement(Texture,P,true);
        FCanvasTileItem Tile(Screen(P.X,P.Y),Texture->GetResource(),FVector2D(P.W*Scale,P.H*Scale),FLinearColor::White);
        Tile.BatchedElementParameters=new FFrontlineFlagParameters(Mask->GetResource(),Liquid->GetResource(),Stain,Time,Seed);
        Tile.BlendMode=SE_BLEND_AlphaBlend;Surface->DrawItem(Tile);
    }
    void CooldownShade(float X,float Y,float W,float H,float ReadyFraction) const
    {
        const float Cleared=H*FMath::Clamp(ReadyFraction,0.f,1.f),Remaining=H-Cleared;
        if(Remaining<.15f) return;
        Rect(X,Y+Cleared,W,Remaining,FLinearColor(.004f,.004f,.004f,.64f));
        const float Edge=FMath::Min(4.f,Remaining);
        Gradient(X,Y+Cleared,W,Edge,FLinearColor(.30f,.30f,.30f,.20f),FLinearColor(0,0,0,0),true);
    }
    void Title(const FString& Value,float X,float Y,float Size=64,const FLinearColor& Color=Paper) const
    {
        if(!bLegacyTypographyV8210){RoleText(EFrontlineTextRoleV8210::Headline,Value,X,Y,DefaultEmV8210(DisplayText(Value),Size),Color);return;}
        if (Artwork().EtchedMaterial) {ReliefText(Value,X,Y,Size,Color);return;}
        UFont* Face=Artwork().Serif?Artwork().Serif.Get():LargeFont; float W=0,H=0;
        HUD.GetTextSize(TEXT("Hg"),W,H,Face,1.f);
        HUD.DrawText(Value,Color,OX+X*Scale,OY+Y*Scale,Face,Size*(Artwork().Serif?1.10f:1.f)/FMath::Max(H,1.f)*Scale,false);
    }
    void Stamped(const FString& Source,float X,float Y,float Size=22,const FLinearColor& Ink=Paper,bool Engraved=false) const
    {
        if(!bLegacyTypographyV8210){RoleText(Size>=64?EFrontlineTextRoleV8210::Inscription:EFrontlineTextRoleV8210::Plate,Source,X,Y,DefaultEmV8210(DisplayText(Source),Size),Ink);return;}
        const FString Value=FrontlineLocalizationV35::Text(Source);
        if(IsControlText(Value)&&Size<64)ControlText(Value,X,Y,Size,Ink);else if(IsControlText(Value))ReliefText(Value,X,Y,Size,Ink);else PlainText(Value,X,Y,Size,Ink);
    }
    void PaperSheet(float X,float Y,float W,float H) const
    {
        if(FrontlineTextureReady(Artwork().CommandSkinV31))
        {
            Rect(X+5,Y+7,W,H,FLinearColor(0,0,0,.5f));
            Skin(3,X,Y,W,H,11,bGermanFinish?FLinearColor(.85f,.88f,.88f,1):FLinearColor(.93f,.91f,.86f,1));
            // Paper fibres remain at the edges; a thin warm paper wash quiets
            // the writing area instead of stretching dark cracks behind text.
            Rect(X+13,Y+13,W-26,H-26,bGermanFinish?FLinearColor(.57f,.58f,.53f,.80f):FLinearColor(.63f,.60f,.51f,.80f));
            return;
        }
        Rect(X+8,Y+12,W,H,FLinearColor(0,0,0,.55f));
        Rect(X,Y,W,H,FLinearColor(.64f,.61f,.52f,1));
        Image(Artwork().CommandPaper,X,Y,W,H,bGermanFinish?FLinearColor(.80f,.84f,.83f,1):FLinearColor(.90f,.88f,.81f,1));
        Line(X+20,Y+19,X+W-20,Y+19,FLinearColor(.20f,.19f,.16f,.28f));
    }
    void Metal(float X,float Y,float W,float H,bool Recess=false) const
    {
        if(ConsoleSkinV79(Recess?1:0,X,Y,W,H,Recess?8.f:15.f,FLinearColor::White))return;
        if(Artwork().CommandSkinV31){Skin(Recess?1:0,X,Y,W,H,Recess?7.f:11.f,bGermanFinish?FLinearColor(.77f,.82f,.84f,1):FLinearColor(.85f,.83f,.76f,1));return;}
        // One continuous painted surface. Small controls are cut into it rather
        // than repeated miniature cases with their own bolts and bright rails.
        const FLinearColor Paint=bGermanFinish?FLinearColor(.050f,.061f,.067f,1):FLinearColor(.070f,.069f,.050f,1);
        const FLinearColor Grain=bGermanFinish?FLinearColor(.48f,.55f,.58f,Recess?.60f:.93f):FLinearColor(.59f,.57f,.46f,Recess?.60f:.93f);
        const FLinearColor WornEdge=bGermanFinish?FLinearColor(.27f,.31f,.32f,Recess?.20f:.44f):FLinearColor(.32f,.29f,.20f,Recess?.20f:.44f);
        Rect(X,Y,W,H,Recess?FLinearColor(Paint.R*.3f,Paint.G*.3f,Paint.B*.3f,.96f):Paint);
        if (W>300 && H>55)
            Image(Artwork().CommandSurface,X,Y,W,H,Grain,0,0,1,1);
        Gradient(X,Y,W,H,FLinearColor(Paint.R*3,Paint.G*3,Paint.B*3,Recess?.025f:.10f),FLinearColor(0,0,0,.24f),true);
        Line(X,Y,X+W,Y,WornEdge);
        Line(X,Y+H,X+W,Y+H,FLinearColor(0,0,0,.70f));
    }
    void Hit(float X,float Y,float W,float H,FName Name,int32 Priority=10) const
    {
        if(RecordingV83){auto& O=OpV83(ETooltipOpV83::Hit,X,Y,W,H,FLinearColor::White);O.HitName=Name;O.Priority=Priority;return;}
        if (bSuppressBackgroundHits && Priority<40) return;
        PageCanvasV73::RecordHit(X,Y,W,H,Name,Priority);
        if(Hovered(X,Y,W,H)&&Priority>=CursorPriorityV67)
        {
            CursorPriorityV67=Priority;
            CursorV67=Name==TEXT("Blocked")
                ?(W<600&&H<350?EFrontlineCursorV67::Disabled:EFrontlineCursorV67::Idle)
                :EFrontlineCursorV67::Hover;
        }
        // AHUD requires a unique name for every rectangle in the same frame.
        // The prefix remains the command; the suffix distinguishes its regions.
        const FName Region(*FString::Printf(TEXT("%s__%d"),*Name.ToString(),NextHitId++));
        HUD.AddHitBox(Screen(X,Y),FVector2D(W*Scale,H*Scale),Region,true,Priority);
    }
    // Separate orthographic objects: fixed corners, quiet writing centres.
    // Source cuts are fractions of the authored 768px width, so native image
    // dimensions can vary without moving clips or stretching corner hardware.
    bool PageSliceV74(UTexture2D* Texture,float X,float Y,float W,float H,float CutFraction,float Edge,FLinearColor Tint=FLinearColor::White) const
    {
        if(!FrontlineTextureReady(Texture)||W<=0||H<=0)return false;
        const float SW=Texture->GetSizeX(),SH=Texture->GetSizeY();
        const float Cut=FMath::Min(CutFraction*SW,FMath::Min(SW,SH)*.4f);
        const float E=FMath::Min(Edge,FMath::Min(W,H)*.35f);
        const float DX[]={X,X+E,X+W-E,X+W},DY[]={Y,Y+E,Y+H-E,Y+H};
        const float U[]={0,Cut/SW,1-Cut/SW,1},V[]={0,Cut/SH,1-Cut/SH,1};
        for(int32 Row=0;Row<3;++Row)for(int32 Col=0;Col<3;++Col)
        {
            if(Row==1&&Col==1&&Texture==ConsoleTextureV79(0)&&FrontlineTextureReady(ConsoleTextureV79(4)))
                ConsoleFieldV79(DX[Col],DY[Row],DX[Col+1]-DX[Col],DY[Row+1]-DY[Row],Tint);
            else TextureV83(Texture,OX+DX[Col]*Scale,OY+DY[Row]*Scale,(DX[Col+1]-DX[Col])*Scale,(DY[Row+1]-DY[Row])*Scale,U[Col],V[Row],U[Col+1]-U[Col],V[Row+1]-V[Row],Tint);
        }
        return true;
    }
#include "WarfareFrontlineConsoleVisualsV79.inl"
    void PageShadowV74(float X,float Y,float W,float H,float Depth=8) const
    {
        // A contact shadow belongs to the object, never to a giant black case.
        for(int32 I=3;I>=0;--I)
            Rect(X+Depth*.35f-I,Y+Depth-I,W+I*2,H+I*2,FLinearColor(0,0,0,I==0?.26f:.045f));
    }
    void PhysicalPanelV74(float X,float Y,float W,float H,bool OrderCard=false) const
    {
        PageShadowV74(X,Y,W,H,OrderCard?9.f:7.f);
        if(ConsoleSkinV79(OrderCard?1:0,X,Y,W,H,OrderCard?12.f:18.f,FLinearColor::White))return;
        auto* Texture=OrderCard?Artwork().OrderCardV74.Get():Artwork().FieldBoardV74.Get();
        if(PageSliceV74(Texture,X,Y,W,H,(OrderCard?84.f:64.f)/768.f,OrderCard?14.f:18.f))return;
        // Bounded fallback while the kit is unavailable. Relief is confined
        // to the binding; the text sits on a calm, opaque writing surface.
        Rect(X,Y,W,H,FLinearColor(.095f,.069f,.043f,1));
        Rect(X+3,Y+3,W-6,H-6,OrderCard?FLinearColor(.13f,.105f,.070f,1):FLinearColor(.080f,.093f,.066f,1));
        Rect(X+14,Y+14,W-28,H-28,OrderCard?FLinearColor(.030f,.031f,.026f,1):FLinearColor(.022f,.031f,.027f,1));
        Gradient(X+3,Y+3,W-6,8,FLinearColor(.35f,.29f,.18f,.55f),FLinearColor(0,0,0,0),true);
        Gradient(X+3,Y+H-12,W-6,9,FLinearColor(0,0,0,0),FLinearColor(0,0,0,.45f),true);
        for(float BX:{X+7,X+W-11})for(float BY:{Y+7,Y+H-11})
        {Rect(BX,BY,4,4,FLinearColor(.30f,.25f,.14f,1));Rect(BX+1,BY+2,2,1,FLinearColor(.08f,.07f,.045f,1));}
    }
    void PhysicalRailV74(float X,float Y,float W,float H) const
    {
        PageShadowV74(X,Y,W,H,4);
        if(ConsoleSkinV79(1,X,Y,W,H,9,FLinearColor::White))return;
        if(PageSliceV74(Artwork().FieldBoardV74.Get(),X,Y,W,H,64.f/768.f,12,FLinearColor(.91f,.93f,.94f,1)))return;
        Rect(X,Y,W,H,FLinearColor(.084f,.071f,.050f,1));
        Rect(X+5,Y+5,W-10,H-10,FLinearColor(.027f,.030f,.024f,1));
        Gradient(X+2,Y+2,W-4,5,FLinearColor(.35f,.29f,.18f,.55f),FLinearColor(0,0,0,0),true);
        Rect(X+3,Y+H-4,W-6,3,FLinearColor(0,0,0,.5f));
    }
    void PaperPanelV74(float X,float Y,float W,float H) const
    {
        // Existing records and temporary narrative sheets use dark ink.
        // Their paper stays opaque and light inside the same physical binding.
        PhysicalPanelV74(X,Y,W,H,true);
        Rect(X+12,Y+12,W-24,H-24,FLinearColor(.66f,.62f,.50f,1));
        ImageCover(Artwork().CommandPaper.Get(),X+12,Y+12,W-24,H-24,FLinearColor(1,1,1,.15f));
        Gradient(X+12,Y+12,W-24,7,FLinearColor(.80f,.74f,.55f,.18f),FLinearColor(0,0,0,0),true);
        Gradient(X+12,Y+H-20,W-24,8,FLinearColor(0,0,0,0),FLinearColor(.16f,.12f,.07f,.18f),true);
    }
    void PageRuleV74(float X,float Y,float W) const
    {
        if(ConsoleTextureV79(0)){ConsoleSeamV79(X,Y,X+W,Y,bGermanFinish?FLinearColor(.30f,.33f,.34f,.65f):FLinearColor(.37f,.32f,.23f,.65f));return;}
        // A shallow routed seam rather than a single modern divider line.
        Rect(X,Y,W,5,FLinearColor(.010f,.013f,.010f,.7f));
        Gradient(X,Y,W,2,FLinearColor(.28f,.24f,.15f,.32f),FLinearColor(.10f,.10f,.07f,.12f));
        Rect(X,Y+4,W,1,FLinearColor(.25f,.24f,.17f,.24f));
    }
    void PageSliderV74(float X,float Y,float W,float Position,bool Focused=false,bool MutedSlider=false) const
    {
        Position=FMath::Clamp(Position,0.f,1.f);
        if(FrontlineTextureReady(ConsoleTextureV79(5)))
        {
            ConsoleGaugeV79(X-16,Y-10,W+32,28);
            ConsoleFillV79(X,Y,W,8,Position,true,MutedSlider?Dim:Allied);
            const float HX=X+Position*W;
            PageShadowV74(HX-9,Y-10,18,28,2);
            ConsoleSliceV80(2,HX-9,Y-10,18,28,4,ConsoleTintV79(Focused?1.25f:1.08f));
            ConsoleSeamV79(HX,Y-5,HX,Y+14,FLinearColor(.43f,.43f,.36f,.85f));return;
        }
        Rect(X-4,Y-3,W+8,14,FLinearColor(.12f,.10f,.062f,1));
        Rect(X-2,Y-1,W+4,10,FLinearColor(.005f,.008f,.006f,1));
        if(Position>.002f)Gradient(X,Y,W*Position,8,MutedSlider?Dim:FLinearColor(.20f,.25f,.20f,1),MutedSlider?Dim:Allied);
        const float HX=X+Position*W;
        PageShadowV74(HX-8,Y-11,16,30,2);
        Rect(HX-8,Y-11,16,30,FLinearColor(.24f,.18f,.09f,1));
        Gradient(HX-6,Y-10,12,27,Focused?Amber:Paper,Focused?Amber*.62f:FLinearColor(.50f,.53f,.45f,1),true);
        Rect(HX-1,Y-5,2,16,FLinearColor(.10f,.11f,.08f,.72f));
    }
    void PageSurfaceV73(float X=0,float Y=0,float W=1920,float H=1080,bool PaperSurface=false,bool DefaultBoard=true,bool DefaultFooter=true) const
    {
        const bool FullPage=X==0&&Y==0&&W==1920&&H==1080;
        if(!FullPage)
        {
            if(PaperSurface)PaperPanelV74(X,Y,W,H);
            else PhysicalPanelV74(X,Y,W,H,true);
            return;
        }
        Rect(-OX/Scale,-OY/Scale,Surface->ClipX/Scale,Surface->ClipY/Scale,FLinearColor(.018f,.023f,.026f,1));
        Rect(0,0,1920,1080,FLinearColor(.028f,.033f,.036f,1));
        if(FrontlineTextureReady(ConsoleTextureV79(4)))ConsoleFieldV79(0,0,1920,1080,ConsoleTintV79(.28f));
        else if(FrontlineTextureReady(Artwork().DeskV74.Get()))ImageCover(Artwork().DeskV74.Get(),0,0,1920,1080);
        // Desk gutters separate the heading, filed tabs, reading board and
        // command rail. This remains a full page rather than a giant dialog.
        // One continuous cabinet and one calm writing plane. The former stacked
        // inset wells placed shadow bands through headings, cards and footers.
        PhysicalPanelV74(40,32,1840,1016);
        if(PaperSurface)
        {
            Rect(64,56,1792,968,FLinearColor(.66f,.62f,.50f,1));
            ImageCover(Artwork().CommandPaper.Get(),64,56,1792,968,FLinearColor(1,1,1,.11f));
        }
        if(DefaultBoard)ConsoleSeamV79(96,252,1824,252,Border*.38f);
        if(DefaultFooter)ConsoleSeamV79(96,936,1824,936,Border*.38f);
        // Heading space belongs to live type; faction craft remains in the housing.
    }
    void PageButtonV73(float X,float Y,float W,float H,const FString& Value,const FString& Key,FName Name,bool Enabled=true,bool Selected=false,int32 Priority=50) const
    {
        const bool Hover=Enabled&&(!bSuppressBackgroundHits||Priority>=40)&&Hovered(X,Y,W,H);
        const bool Pressed=Hover&&HUD.GetOwningPlayerController()&&HUD.GetOwningPlayerController()->IsInputKeyDown(EKeys::LeftMouseButton);
        const float PX=X+(Pressed?1.f:0.f),PY=Y+(Pressed?2.f:0.f);
        if(!Pressed)PageShadowV74(X,Y,W,H,3);
        const auto Tint=!Enabled?FLinearColor(.55f,.57f,.58f,1):Selected?FLinearColor(1.06f,1.07f,1.06f,1):Hover?FLinearColor(1.01f,1.03f,1.04f,1):FLinearColor(.87f,.90f,.92f,1);
        auto* KeyTexture=FrontlineTextureReady(ConsoleTextureV79(2))?ConsoleTextureV79(2):Artwork().CommandKeyV74.Get();
        if(!ConsoleSliceV80(2,PX,PY,W-(Pressed?1.f:0.f),H-(Pressed?2.f:0.f),FMath::Min(12.f,H*.21f),ConsoleTintV79()*Tint))
        {
            Rect(PX,PY,W-(Pressed?1.f:0.f),H-(Pressed?2.f:0.f),FLinearColor(.17f,.12f,.065f,1)*Tint);
            Gradient(PX+4,PY+4,W-8,H-8,FLinearColor(.085f,.073f,.048f,1)*Tint,FLinearColor(.025f,.027f,.020f,1),true);
            Rect(PX+2,PY+H-5,W-4,3,FLinearColor(0,0,0,.55f));
        }
        ConsoleSelectionV79(PX,PY,W-(Pressed?1.f:0.f),H-(Pressed?2.f:0.f),Selected,Hover);
        Label(Value,X+16,Y+(Pressed?2.f:0.f),W-32,H-(Pressed?2.f:0.f),H>=56?21.f:18.f,Enabled?Paper:Muted*.88f,1,false,bLegacyTypographyV8210?EFrontlineTextRoleV8210::Auto:EFrontlineTextRoleV8210::Plate);
        Shortcut(X,Y,W,H,Key,Priority);Hit(X,Y,W,H,Enabled?Name:FName(TEXT("Blocked")),Priority);
    }
    void PageTabV73(float X,float Y,float W,const FString& Value,FName Name,bool Selected,bool Enabled=true,int32 Priority=50) const
    {
        const bool Hover=Enabled&&(!bSuppressBackgroundHits||Priority>=40)&&Hovered(X,Y,W,PageCanvasV73::TabH);
        const bool Pressed=Hover&&HUD.GetOwningPlayerController()&&HUD.GetOwningPlayerController()->IsInputKeyDown(EKeys::LeftMouseButton);
        const float TY=Y+(Selected?0.f:4.f)+(Pressed?2.f:0.f),TH=PageCanvasV73::TabH-(Selected?0.f:4.f)-(Pressed?2.f:0.f);
        if(!Pressed)PageShadowV74(X,TY,W,TH,2);
        const auto Tint=Selected?FLinearColor(1.06f,1.07f,1.06f,1):Hover?FLinearColor(.97f,.99f,1.f,1):FLinearColor(.68f,.72f,.74f,1);
        auto* TabTexture=FrontlineTextureReady(ConsoleTextureV79(3))?ConsoleTextureV79(3):Artwork().IndexTabV74.Get();
        if(!ConsoleSliceV80(3,X,TY,W,TH,12,ConsoleTintV79()*Tint))
        {
            Rect(X,TY,W,TH,FLinearColor(.14f,.15f,.10f,1)*Tint);
            Gradient(X+4,TY+4,W-8,TH-8,FLinearColor(.070f,.083f,.057f,1)*Tint,FLinearColor(.020f,.030f,.024f,1),true);
            Rect(X+4,TY+TH-4,W-8,3,FLinearColor(0,0,0,.40f));
        }
        ConsoleSelectionV79(X,TY,W,TH,Selected,Hover);
        Label(Value,X+16,TY,W-32,TH,22,Enabled?(Selected?Paper:Muted):Muted*.88f,1,false,bLegacyTypographyV8210?EFrontlineTextRoleV8210::Auto:EFrontlineTextRoleV8210::Plate);
        Hit(X,Y,W,PageCanvasV73::TabH,Enabled?Name:FName(TEXT("Blocked")),Priority);
    }
    void Button(float X,float Y,float W,float H,const FString& Label,const FString& Shortcut,FName Name,
        bool Enabled=true,bool Selected=false,int32 Priority=10) const
    {
        const bool Hover=Enabled && (!bSuppressBackgroundHits||Priority>=40) && Hovered(X,Y,W,H);
        Skin(3,X,Y,W,H,8,Enabled?(Selected?FLinearColor(1.12f,1.12f,1.08f,1):Hover?FLinearColor::White:FLinearColor(.86f,.86f,.84f,1)):FLinearColor(.52f,.52f,.50f,1));
        const float Nominal=H>60?22.f:W<=110?16.f:18.f;
        const FLinearColor LabelColor=Enabled?Paper:FLinearColor(.40f,.43f,.42f);
        const bool Pressed=Hover&&HUD.GetOwningPlayerController()&&HUD.GetOwningPlayerController()->IsInputKeyDown(EKeys::LeftMouseButton);
        ConsoleSelectionV79(X,Y,W,H,Selected,Hover);
        this->Label(Label,X+15,Y+(Pressed?1.2f:0.f),W-30,H,Nominal,Hover?FLinearColor(.97f,.93f,.79f):LabelColor,1,false,bLegacyTypographyV8210?EFrontlineTextRoleV8210::Auto:EFrontlineTextRoleV8210::Plate);
        this->Shortcut(X,Y,W,H,Shortcut,Priority);
        // Disabled cards still consume their area, but never dispatch a gameplay order.
        Hit(X,Y,W,H,Enabled?Name:FName(TEXT("Blocked")),Priority);
    }
    void IconButton(int32 Slot,float X,float Y,float W,float H,FName Name,const FString& Tip) const
    {
        const bool Hover=!bSuppressBackgroundHits&&Hovered(X,Y,W,H);
        Skin(3,X,Y,W,H,9,Hover?FLinearColor::White:FLinearColor(.86f,.86f,.84f,1));
        const float S=FMath::Min(W,H)*.65f;InstrumentIcon(Slot,X+(W-S)*.5f,Y+(H-S)*.5f,S,S);
        Hit(X,Y,W,H,Name);
        if(Hover)TooltipV83(TooltipKeyV83(10,Slot),0,[&]{Metal(X-142,Y-50,190,40,true);PlainText(Tip,X-127,Y-40,16,Paper);});
    }
};

void ProbeFrontlineCanvasV8218Review(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,float Top);
void ProbeFrontlineCanvasV8219Review(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B);

#include "WarfareFrontlineCommanderBiographiesV31.inl"
#include "WarfareFrontlineCommanderBiographiesV32.inl"

bool FRRUStoryModalV82(const AWarfareFrontlineGameMode* B)
{return WarfareFRRUCampaignV82::Active(B)&&B->Phase==EFrontlinePhase::Fighting&&WarfareFRRUWorldV82::View(B).StoryPending;}
const EFrontlineSupport FrontlineSupportCards[]={EFrontlineSupport::Artillery,EFrontlineSupport::Smoke,EFrontlineSupport::Mortar,EFrontlineSupport::Gas,EFrontlineSupport::AntiTank,EFrontlineSupport::AirStrike,EFrontlineSupport::CreepingBarrage,EFrontlineSupport::AirRecon,EFrontlineSupport::AirBombing};
int32 SupportTargetMode(EFrontlineSupport Kind)
{switch(Kind){case EFrontlineSupport::Artillery:return 1;case EFrontlineSupport::Smoke:return 2;case EFrontlineSupport::Mortar:return 4;case EFrontlineSupport::Gas:return 5;case EFrontlineSupport::AntiTank:return 6;case EFrontlineSupport::AirStrike:return 7;case EFrontlineSupport::CreepingBarrage:return 8;case EFrontlineSupport::AirRecon:return 9;case EFrontlineSupport::AirBombing:return 10;default:return 3;}}
EFrontlineSupport SupportForTargetMode(int32 Mode)
{for(const auto Kind:FrontlineSupportCards)if(SupportTargetMode(Kind)==Mode)return Kind;return EFrontlineSupport::Rally;}
void SelectSupportCard(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,EFrontlineSupport Kind)
{
    if(FRRUStoryModalV82(B))return;
    const int32 Mode=SupportTargetMode(Kind);if(PC->SupportTargetMode==Mode){PC->CancelTargeting();return;}
    const FString Reason=B->SupportBlockReason(Kind,0);if(!Reason.IsEmpty()){B->LastMessage=Reason;return;}
    PC->SupportTargetMode=Mode;PC->bTargetingArtillery=Kind==EFrontlineSupport::Artillery;
    B->LastMessage=FString::Printf(TEXT("%s · 选择覆盖地段"),B->GetSupportName(Kind));
}
void DrawClassicSupportSight(const FFrontlineCanvas& UI,AWarfareFrontlineController* PC,const FVector& Target,const FVector2D& Extent)
{
    FVector2D C,X,Y;const FVector Centre=Target+FVector(0,0,60);
    if(!PC->ProjectWorldLocationToScreen(Centre,C)||!PC->ProjectWorldLocationToScreen(Centre+FVector(Extent.X,0,0),X)||!PC->ProjectWorldLocationToScreen(Centre+FVector(0,Extent.Y,0),Y))return;
    C=UI.Local(C);X=UI.Local(X);Y=UI.Local(Y);const float RX=FMath::Max(90.f,float(FMath::Abs(X.X-C.X))),RY=FMath::Max(37.f,float(FMath::Abs(Y.Y-C.Y)));
    static TStrongObjectPtr<UTexture2D> Stamp;
    if(!Stamp.IsValid())Stamp.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V64/T_SupportStamp_V64.T_SupportStamp_V64")));
    float Opacity=.84f;const auto& S=ScreenState(PC);
    if(PC->SupportTargetMode==0)Opacity*=1.f-FMath::Clamp(float(FPlatformTime::Seconds()-S.SupportSealAtV63)/6.f,0.f,1.f);
    // This is a circle projected onto the ground, intentionally an ellipse;
    // portrait contain/crop rules must not change its world-space footprint.
    // Restore the accepted authored stamp through Canvas's texture colour path.
    // The later material route lost its transfer function and dimmed the entire
    // ellipse; its raw linear pigment was not equivalent to this painted ink.
    if(FrontlineTextureReady(Stamp.Get()))UI.HUD.DrawTexture(Stamp.Get(),UI.OX+(C.X-RX*1.27f)*UI.Scale,UI.OY+(C.Y-RY*1.27f)*UI.Scale,RX*2.54f*UI.Scale,RY*2.54f*UI.Scale,0,0,1,1,FLinearColor(.88f,.40f,.34f,Opacity));
}

// National plates are explicit content bindings. A missing new-faction plate
// cannot quietly present a British or German soldier as its replacement.
UTexture2D* NationalTextureV82(EFRRUNationV81 Nation,const TCHAR* Kind)
{
    if(FString(Kind)==TEXT("Flag")&&(Nation==EFRRUNationV81::British||Nation==EFRRUNationV81::German))return Artwork().Flags[Nation==EFRRUNationV81::German?1:0];
    FString Prefix;switch(Nation){case EFRRUNationV81::French:Prefix=TEXT("FR");break;case EFRRUNationV81::Russian:Prefix=TEXT("RU");break;case EFRRUNationV81::AustroHungarian:Prefix=TEXT("AH");break;default:return nullptr;}
    const FString Name=TEXT("T_")+Prefix+TEXT("_")+Kind,Path=TEXT("/Game/FRRUIntegration/FormalV82/UI/")+Name+TEXT(".")+Name;
    static TMap<FString,TStrongObjectPtr<UTexture2D>> Plates;auto& Plate=Plates.FindOrAdd(Path);if(!Plate.IsValid())Plate.Reset(LoadObject<UTexture2D>(nullptr,*Path));return Plate.Get();
}
UTexture2D* NationalAtlasV82(EFRRUNationV81 Nation,const TCHAR* Kind)
{
    const TCHAR* Prefix=Nation==EFRRUNationV81::French?TEXT("FR"):Nation==EFRRUNationV81::Russian?TEXT("RU"):TEXT("AT_HU");
    const FString Name=FString(TEXT("T_"))+Prefix+TEXT("_")+Kind,Path=TEXT("/Game/FRRUIntegration/FormalV82/UI/")+Name+TEXT(".")+Name;
    static TMap<FString,TStrongObjectPtr<UTexture2D>> Atlases;auto& Atlas=Atlases.FindOrAdd(Path);if(!Atlas.IsValid())Atlas.Reset(LoadObject<UTexture2D>(nullptr,*Path));return Atlas.Get();
}
const FFRRUCampaignSummaryV82* MostRecentNationalCampaignV827(const AWarfareFrontlineGameMode* B)
{
    if(!B)return nullptr;
    // Cover and Continue share the same timestamp comparison. A resumed older
    // archive does not move to the end of the repository's insertion order.
    const FFRRUCampaignSummaryV82* Recent=nullptr;int64 Newest=0,LegacyNewest=0;
    for(const auto& Row:WarfareFRRUCampaignV82::Archives(B))if(WarfareFRRUCampaignV82::CampaignsAvailable(Row.PlayerNation))
    {FDateTime Time;if(FDateTime::ParseIso8601(*Row.UpdatedUtc,Time)&&Time.GetTicks()>Newest){Newest=Time.GetTicks();Recent=&Row;}}
    for(const auto& Row:B->GetCampaignRuns()){FDateTime Time;if(FDateTime::ParseIso8601(*Row.UpdatedUtc,Time))LegacyNewest=FMath::Max(LegacyNewest,Time.GetTicks());}
    return Recent&&Newest>LegacyNewest?Recent:nullptr;
}
// A complete, resident cover is also the last-resort loading plate.
UTexture2D* MenuCoverV63(const AWarfareFrontlineGameMode* B)
{
    if(WarfareFRRUCampaignV82::Active(B))return NationalTextureV82(WarfareFRRUCampaignV82::Nation(B),TEXT("Cover"));
    if(const auto* Recent=MostRecentNationalCampaignV827(B))return NationalTextureV82(Recent->PlayerNation,TEXT("Cover"));
    const EFrontlineFaction F=FrontlineConsoleV79::ResolveDisplayFaction(B,true);
    if(Artwork().MenuCoversV72[int32(F)].Texture)return Artwork().MenuCoversV72[int32(F)].Texture.Get();
    if(Artwork().MenuCoversV71[int32(F)].Texture)return Artwork().MenuCoversV71[int32(F)].Texture.Get();
    static TStrongObjectPtr<UTexture2D> British,German;
    auto& Cover=F==EFrontlineFaction::German?German:British;
    if(!Cover.IsValid())Cover.Reset(LoadObject<UTexture2D>(nullptr,F==EFrontlineFaction::German?
        TEXT("/Game/Warfare/UI/V63/T_MainMenu_German_V63.T_MainMenu_German_V63"):TEXT("/Game/Warfare/UI/V62/T_MainMenu_V62.T_MainMenu_V62")));
    return Cover.Get();
}
bool ContinueHomeCoveredV8214(AWarfareFrontlineController* PC,const AWarfareFrontlineGameMode* B)
{
    if(!PC||!B||FrontlineUIAssetsV8213::BaselineMode()
        ||FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfContinueBaselineV8214"))||B->Phase!=EFrontlinePhase::Briefing
        ||WarfareFRRUCampaignV82::Active(B)||!B->AssetFailure.IsEmpty())return false;
    const auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
    if(!HUD||HUD->GetPresentationPage()!=0)return false;
    int32 W=0,H=0;PC->GetViewportSize(W,H);
    if(W<=0||H<=0||int64(W)*9!=int64(H)*16)return false;
    auto* Cover=MenuCoverV63(B);auto& Art=Artwork();
    return Cover&&(Cover==Art.MenuCoversV72[0].Texture.Get()||Cover==Art.MenuCoversV72[1].Texture.Get())
        &&FrontlineTextureReady(Cover);
}
namespace FrontlineOpaqueMapV834
{
struct FHold
{
    TWeakObjectPtr<AWarfareFrontlineController> Owner;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> View;
    TWeakObjectPtr<UGameViewportClient> Viewport;
    bool Pending=false;
};
FHold& State(){static FHold S;return S;}
void Remember(AWarfareFrontlineController* PC)
{
    auto& S=State();S.Owner=PC;S.World=PC->GetWorld();S.View=PC->GetViewTarget();
    S.Viewport=PC->GetWorld()->GetGameViewport();S.Pending=true;
}
void Discard(){State()=FHold();}
void Resume(AWarfareFrontlineController* PC,const AWarfareFrontlineGameMode* B)
{
    auto& S=State();if(!S.Pending)return;
    auto* World=PC?PC->GetWorld():nullptr;
    auto* Client=World?World->GetGameViewport():nullptr;
    auto* View=PC?PC->GetViewTarget():nullptr;
    const bool SameMap=PC&&B&&S.Owner.Get()==PC&&S.World.Get()==World&&S.View.Get()==View
        &&S.Viewport.Get()==Client&&Client&&View&&View->GetWorld()==World
        &&View->ActorHasTag(TEXT("WF_MapScene_V61"))&&FrontlineMapSceneV61::IsActive(PC)
        &&B->Phase==EFrontlinePhase::Briefing&&B->GetCurrentCampaign()&&!WarfareFRRUCampaignV82::Active(B)
        &&(B->GetPlayerFaction()==EFrontlineFaction::British||B->GetPlayerFaction()==EFrontlineFaction::German);
    if(!SameMap){Discard();return;}
    // GameViewportClient consumes the cut even when it skips the world view.
    // Issue it only after MapScene.Update has kept this same view active and
    // the viewport will actually submit it, restoring the old re-entry reset.
    if(Client->bDisableWorldRendering||!FSlateApplication::IsInitialized()
        ||!FSlateApplication::Get().GetPlatformApplication()->IsAllowedToRender())return;
    if(PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
        if(FrontlineTextLayoutV834::ReviewEnabled())++FrontlineTextLayoutV834::ReviewState().OpaqueMapResumeCuts;
    }
    Discard();
}
}
#include "WarfareFrontlineRosterUIV32.inl"
#include "WarfareFrontlineIdentityV72.inl"
#include "WarfareFrontlineUIV30.inl"
void DrawFactionRibbonV8215(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,EFrontlineFaction N,float X,float Y,float W,float H);
namespace FrontlineLoadingPageV82 { void DrawOperationMapV8447(const FFrontlineCanvas&,const AWarfareFrontlineGameMode*,const FFrontlineCampaignMission*,float,float,float,float,FLinearColor); }
#include "WarfareFrontlineWarRoomV52.inl"
#include "FRRU/WarfareFRRUWarRoomUIV82.inl"
void DrawSpecialAllocationV87(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FString& Tip,FString& TipTitle);
#include "FRRU/WarfareFRRUArmourUIV82.inl"
#include "WarfareFrontlineAttachmentsV38.inl"
#include "WarfareFrontlineArmourUIV63.inl"
#include "WarfareFrontlineDeploymentUIV32.inl"
#include "WarfareFrontlineCampaignMapV23.inl"
#include "WarfareFrontlineTextReviewV829.api.inl"
#include "WarfareFrontlineOperationPreparationV16.inl"
#include "WarfareFrontlineUnitIntroductionV27.inl"
#include "WarfareFrontlineDisplay.inl"
namespace FrontlineLoadingPageV82 { void PreloadAvailable(); }
#include "WarfareFrontlineNavigationV61.inl"
#include "WarfareFrontlineFactionSelectionV82.inl"
#include "WarfareFrontlineTechnologyUI.inl"
#include "FRRU/WarfareFRRUWorldUIV82.inl"
#include "WarfareFrontlineArmourKillTallyUIV8220.inl"
#include "WarfareFrontlineMedalWallUIV8215.inl"
#include "WarfareFrontlineMedalMountV8216.inl"
bool DecorationModalV8216(APlayerController* P){if(!P)return false;auto& M=MedalMountV8216::View(P);auto& W=MedalWallV8215::View(P);if(!MedalMountV8216::Eligible(P,FrontlineFor(P)))MedalMountV8216::CloseV836(P);if(!ScreenState(P).AchievementsV76||!W.Wall)W.Inspect=false;return M.Picker||W.Inspect;}
#include "WarfareFrontlineAchievementsUIV76.inl"
#include "WarfareFrontlineUltimateArchiveUIV8215.inl"
bool EndArchiveReplayV8215(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,bool OpenRoom)
{
    if(!PC||!FrontlineUltimateArchiveV8215::Active(B))return false;
    // Settle post-result support kills before Leave retires the replay ledger.
    // A failed write must retain the result screen and its pending record.
    if(HasEnded(B)&&!FrontlineAchievementsV76::FinalizeBattleKillsV8217(B))return false;
    auto& S=ScreenState(PC);PC->CancelTargeting();FinishVideoChange(S,false);ClearPreparationWarmersV43(S);
    const bool NationalArchive=WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B);
    const auto ArchiveNation=NationalArchive?WarfareFRRUCampaignV82::ArchiveReplayNationV87(B):WarfareFRRUCampaignV82::Nation(B,0);
    const auto ArchiveFaction=B->GetPlayerFaction();const auto ArchiveDifficulty=NationalArchive?WarfareFRRUCampaignV82::ArchiveReplayDifficultyV87(B):B->GetCampaignDifficulty();
    const auto* Replay=B->GetCurrentCampaign();const int32 ArchiveMission=NationalArchive?WarfareFRRUCampaignV82::ArchiveReplayMissionV87(B):Replay?Replay->SelectedMission:0;
    FrontlineUltimateArchiveV8215::Leave(B);
    S.Menu=S.Help=S.Settings=S.Technology=S.Preparing=S.Deployment=S.HistoricalDispatch=S.UnitIntroduction=S.CampaignEndingV51=S.AchievementsV76=false;
    S.NationalAssignmentNoticeV87=false;
    S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.AcceptedLoadoutMission=INDEX_NONE;S.PreparationError.Empty();S.Dossier=1917;
    auto& History=NavigationV61::History(PC);History.Back.Empty();History.Forward.Empty();
    PC->ResetCamera();if(OpenRoom){if(NationalArchive)UltimateArchiveUIV8215::ReturnToNationalSelectionV87(PC,ArchiveNation,ArchiveMission,ArchiveDifficulty);else UltimateArchiveUIV8215::ReturnToSelectionV85(PC,ArchiveFaction,ArchiveMission,ArchiveDifficulty);}else UltimateArchiveUIV8215::Close(PC);return true;
}
#include "WarfareFrontlineReviewArchiveFlowV851.inl"
#include "WarfareFrontlineUIV24Review.inl"
#include "WarfareFrontlineUIV26Review.inl"
#include "WarfareFrontlineUIV27Review.inl"


}

void StartFrontlineV63Review(AWarfareFrontlineController* Controller);
void StartFrontlineV64Review(AWarfareFrontlineController* Controller);
void StartFrontlineV65Review(AWarfareFrontlineController* Controller);
void StartFrontlineV66Review(AWarfareFrontlineController* Controller);
void StartFrontlineV70Review(AWarfareFrontlineController* Controller);
void StartFrontlineArtV71Review(AWarfareFrontlineController* Controller);
void StartFrontlineUIV72Review(AWarfareFrontlineController* Controller);
void StartFrontlineUIV73Review(AWarfareFrontlineController* Controller);
void StartFrontlineUIV74Review(AWarfareFrontlineController* Controller);
void StartFrontlineUIV75Review(AWarfareFrontlineController* Controller);
void StartFrontlineUIPerfV8213Review(AWarfareFrontlineController* Controller);
void StartFrontlineDeathSleepReviewV8213(UWorld* World);
#include "WarfareFrontlineTrenchCombatV69.h"
void StartFrontlineV68ArtReview(AWarfareFrontlineController* Controller);
void StartFrontlineV15UIReview(AWarfareFrontlineController* Controller);
void StartFrontlineV16UIReview(AWarfareFrontlineController* Controller);
void StartFrontlineV17UIReview(AWarfareFrontlineController* Controller);
void StartFrontlineV18UIReview(AWarfareFrontlineController* Controller);
void StartFrontlineV21Review(AWarfareFrontlineController* Controller);
void StartFrontlineV23FieldReview(AWarfareFrontlineController* Controller);
void StartFrontlineV26Verification(AWarfareFrontlineController* Controller);
void StartFrontlineV27RenderReview(AWarfareFrontlineController* Controller);
void StartFrontlineV27CampaignReview(AWarfareFrontlineController* Controller);

AWarfareFrontlineController::AWarfareFrontlineController()
{
    bShowMouseCursor=true; bEnableClickEvents=true; bEnableMouseOverEvents=true;
    PrimaryActorTick.bTickEvenWhenPaused=true; bShouldPerformFullTickWhenPaused=true;
}

void AWarfareFrontlineController::BeginPlay()
{
    Super::BeginPlay();
    FrontlineUIAssetsV8213::PreloadCore();
    FrontlineLoadingPageV82::PreloadAvailable();
    if(auto* Battle=FrontlineFor(this);Battle
        &&!FrontlineUIAssetsV8213::BaselineMode()
        &&!FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfContinueBaselineV8214")))
        FrontlineMapSceneV61::PreloadContinueV8214(this);
    NavigationV61::PreloadClickSoundV8213();
    FrontlineDebugV37::Initialize(this);
    ResetCamera();
    // UE can clamp the startup window against display metrics obtained before
    // per-monitor DPI initialization. Resize then updates the settings object.
    // Reload the saved values after the real game viewport exists and apply
    // them with current metrics; explicit launch overrides still take priority.
    if (GetWorld()->WorldType==EWorldType::Game && GEngine && GEngine->GameViewport)
        if (UGameUserSettings* Settings=GEngine->GetGameUserSettings())
        {
            Settings->LoadSettings(false);
            const FIntPoint SavedSize=Settings->GetScreenResolution();
            Settings->ApplyResolutionSettings(true);
            UE_LOG(LogTemp,Display,TEXT("WF_DISPLAY_RESTORE saved=%dx%d mode=%d"),SavedSize.X,SavedSize.Y,int32(Settings->GetFullscreenMode()));
        }
    for (TActorIterator<AActor> It(GetWorld());It;++It)
        if (It->ActorHasTag(TEXT("WF_DistantMatte_V12"))) { DistantMatte=*It; break; }
    FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
    FrontlineCursorV67::Install(this);
    StartFrontlineStartupReviewV67(this);
    BattleCamera=GetWorld()->SpawnActor<ACameraActor>();
    if (BattleCamera)
    {
        UCameraComponent* Camera=BattleCamera->GetCameraComponent();
        Camera->ProjectionMode=ECameraProjectionMode::Orthographic; Camera->OrthoWidth=CameraWidth;
        Camera->bConstrainAspectRatio=false;
        // MaintainYFOV expands a 11500-wide orthographic view to 20444 at 16:9.
        // Explicitly keep the horizontal battlefield extent at the requested width.
        Camera->bOverrideAspectRatioAxisConstraint=true;
        Camera->AspectRatioAxisConstraint=AspectRatio_MaintainXFOV;
        Camera->bAutoCalculateOrthoPlanes=false; Camera->AutoPlaneShift=0;
        Camera->bUpdateOrthoPlanes=false; Camera->bUseCameraHeightAsViewTarget=false;
        Camera->OrthoNearClipPlane=1.f; Camera->OrthoFarClipPlane=40000.f;
        Camera->Overscan=0;
        const FVector NativeComposition=WarfareFRRUVauxCompositionV87::CameraOffset(FrontlineFor(this));
        const FVector Focus=CameraFocus+FVector(0,0,650)+NativeComposition;
        const FVector Location=CameraFocus+FVector(0,14000,4650)+NativeComposition;
        BattleCamera->SetActorLocation(Location); BattleCamera->SetActorRotation((Focus-Location).Rotation());
        SetViewTarget(BattleCamera);
    }
    // The explicit native parity console command is the sole test driver.
    // Its account-codec flag must not start older automatic world fixtures.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ParityReview")))return;
    // The focused ribbon fixture owns its disposable scene. Never schedule
    // the legacy achievement/operation fixtures alongside its UI captures.
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV8443RibbonReview"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8445Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8446Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8448Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84412GameplayFocused"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84413Focused")))
    {StartFrontlineV8221Review(this);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review")))
    {StartFrontlineV8221Review(this);return;}
    StartFrontlineV14AudioVerification(this);
    StartFrontlineV18AudioVerification(this);
    StartFrontlineV63Review(this);
    StartFrontlineV64Review(this);
    StartFrontlineV65Review(this);
    StartFrontlineV66Review(this);
    StartFrontlineV70Review(this);
    StartFrontlineArtV71Review(this);
    StartFrontlineUIV72Review(this);
    StartFrontlineUIV73Review(this);
    StartFrontlineUIV74Review(this);
    StartFrontlineUIV75Review(this);StartFrontlineReviewV76(this);
    StartFrontlineUIPerfV8213Review(this);
    StartFrontlineDeathSleepReviewV8213(GetWorld());
    StartFrontlineV68ArtReview(this);
    StartFrontlineV69CombatReview(this);
    StartFrontlineGameplayReviewV75(this);
    StartFrontlineV15UIReview(this);
    StartFrontlineV16UIReview(this);
    StartFrontlineV17UIReview(this);
    StartFrontlineV18UIReview(this);
    StartFrontlineV19WeatherReview(this);
    StartFrontlineV20Review(this);
    StartFrontlineV21Review(this);
    StartFrontlineV23FieldReview(this);
    StartFrontlineV24UIReview(this);
    StartFrontlineV26UIReview(this);
    StartFrontlineV27UIReview(this);
    StartFrontlineV28UIReview(this);
    StartFrontlineV30Review(this);
    StartFrontlineV31Review(this);
    StartFrontlineV32Review(this);
    StartFrontlineV33Review(this);
    StartFrontlineV8214EffectsReview(this);
    StartFrontlineV8215Review(this);
    StartFrontlineV8216Review(this);
    StartFrontlineV8217Review(this);
    StartFrontlineV8218Review(this);
    StartFrontlineV8219Review(this);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review")))StartFrontlineV8221Review(this);
    else StartFrontlineV8220Review(this);
    StartFrontlineV35Review(this);
    StartFrontlineV36UIReview(this);
    StartFrontlineV37Review(this);
    StartFrontlineV38Review(this);
    StartFrontlineV39Review(this);
    StartFrontlineV40Review(this);
    StartFrontlineV27RenderReview(this);
    StartFrontlineV27CampaignReview(this);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV26Review")))StartFrontlineV26Verification(this);
    StartFrontlineV23DisplayReview(this);
    StartFrontlineV24DisplayReview(this);
    FrontlineHDRReviewV25::Begin(GetWorld());
}

void AWarfareFrontlineController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&AWarfareFrontlineController::ClickAtCursor).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::One,IE_Pressed,this,&AWarfareFrontlineController::RecruitRifle);
    InputComponent->BindKey(EKeys::Two,IE_Pressed,this,&AWarfareFrontlineController::RecruitLewis);
    InputComponent->BindKey(EKeys::Three,IE_Pressed,this,&AWarfareFrontlineController::RecruitMills);
    InputComponent->BindKey(EKeys::Four,IE_Pressed,this,&AWarfareFrontlineController::RecruitMarksman);
    InputComponent->BindKey(EKeys::Five,IE_Pressed,this,&AWarfareFrontlineController::RecruitOfficer);
    InputComponent->BindKey(EKeys::Q,IE_Pressed,this,&AWarfareFrontlineController::ReleaseSelected);
    InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AWarfareFrontlineController::ToggleArtillery);
    InputComponent->BindKey(EKeys::C,IE_Pressed,this,&AWarfareFrontlineController::ToggleSmoke);
    InputComponent->BindKey(EKeys::V,IE_Pressed,this,&AWarfareFrontlineController::ToggleMortar);
    InputComponent->BindKey(EKeys::H,IE_Pressed,this,&AWarfareFrontlineController::ToggleHelp).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::F10,IE_Pressed,this,&AWarfareFrontlineController::TogglePerformanceMonitor).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::O,IE_Pressed,this,&AWarfareFrontlineController::ToggleSettings).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::T,IE_Pressed,this,&AWarfareFrontlineController::ToggleTechnology).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AWarfareFrontlineController::ResetCamera).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::Enter,IE_Pressed,this,&AWarfareFrontlineController::BeginBattle).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::SpaceBar,IE_Pressed,this,&AWarfareFrontlineController::TogglePause).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AWarfareFrontlineController::Restart).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&AWarfareFrontlineController::ToggleMenu).bExecuteWhenPaused=true;
    // InputCore's native Windows path receives WM_XBUTTON1 / WM_XBUTTON2.
    InputComponent->BindKey(EKeys::ThumbMouseButton,IE_Pressed,this,&AWarfareFrontlineController::MenuBackV61).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::ThumbMouseButton2,IE_Pressed,this,&AWarfareFrontlineController::MenuForwardV61).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&AWarfareFrontlineController::RightClickOrdersV63).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::MouseScrollUp,IE_Pressed,this,&AWarfareFrontlineController::ZoomIn).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::MouseScrollDown,IE_Pressed,this,&AWarfareFrontlineController::ZoomOut).bExecuteWhenPaused=true;
}

void AWarfareFrontlineController::RightClickOrdersV63()
{
    if(DecorationModalV8216(this))return;
 if(SupportTargetMode){CancelTargeting();return;}
 auto* B=FrontlineFor(this);auto& S=ScreenState(this);
 if(!B||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE||S.Menu||S.Help||S.Settings||S.Technology||S.AchievementsV76||S.Preparing||FRRUStoryModalV82(B))return;
 int32 W=0,H=0;GetViewportSize(W,H);float MX=0,MY=0;if(!GetMousePosition(MX,MY)&&!V28ReviewPointer.IsSet())return;
 const float Scale=FMath::Min(W/1920.f,H/1080.f);if(Scale<=0)return;
 FVector2D P((MX-(W-1920*Scale)*.5f)/Scale,(MY-(H-1080*Scale)*.5f)/Scale);if(V28ReviewPointer.IsSet())P=V28ReviewPointer.GetValue();
 S.AutoEncounterV63=B->GetEncounterId();
 const auto Roles=FieldRolesV32(B);
 for(int32 I=0;I<Roles.Num()&&I<5;++I)if(FrontlineConsoleLayoutV79::ContainsCard(P,FrontlineConsoleLayoutV79::InfantryX,I,DockTop))
 {if(B->IsRoleUnlocked(Roles[I]))S.AutoRoleV63=S.AutoRoleV63==int32(Roles[I])?INDEX_NONE:int32(Roles[I]);return;}
 if(WarfareFRRUCampaignV82::Active(B))
 {
  const auto R=FRRUArmourUIV82::Selected(B);
  for(int32 I=0;I<R.Num()&&I<2;++I)if(FrontlineConsoleLayoutV79::ContainsCard(P,FrontlineConsoleLayoutV79::ArmourX,I,DockTop))
  {if(WarfareFRRUArmourV82::Eligible(B,R[I],0)){S.AutoArmourStableV82=S.AutoArmourStableV82==R[I]?FString():R[I];S.AutoVehicleV63=INDEX_NONE;}return;}
  return;
 }
 if(auto* V=UFrontlineVehicleSubsystem::Get(GetWorld())){const auto R=V->GetEquippedRoster();for(int32 I=0;I<R.Num()&&I<2;++I)
 if(FrontlineConsoleLayoutV79::ContainsCard(P,FrontlineConsoleLayoutV79::ArmourX,I,DockTop))
 {S.AutoVehicleV63=S.AutoVehicleV63==int32(R[I].Type)?INDEX_NONE:int32(R[I].Type);return;}}
}

void AWarfareFrontlineController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    FactionSelectionV82::Tick(this,FrontlineFor(this));
    TickDisplay(ScreenState(this));
    TickOperationPreparation(this);
    if(FrontlineDebugV37::IsOpen(this)){bPreviewValid=false;CameraPanVelocity=CameraVerticalVelocity=0;return;}
    AWarfareFrontlineGameMode* Battle=FrontlineFor(this); if (!Battle || !BattleCamera) return;
    const auto& StoryScreen=ScreenState(this);
    if(!StoryScreen.Preparing&&!StoryScreen.Menu&&!StoryScreen.Help&&!StoryScreen.Settings&&!StoryScreen.Technology&&!StoryScreen.AchievementsV76)
        WarfareFRRUWorldV82::PauseForPendingStory(Battle);
    TickAutoOrdersV63(this,Battle);FrontlineArmourV63::Tick(Battle);
    // Real time keeps camera response independent of simulation speed.
    const float DT=FMath::Clamp(static_cast<float>(FApp::GetDeltaTime()),0.f,.05f);
    const FFrontlineScreenState& Screen=ScreenState(this);
    const AWarfareFrontlineHUD* HUD=Cast<AWarfareFrontlineHUD>(GetHUD());
    FViewport* Viewport=GetWorld()->GetGameViewport()?GetWorld()->GetGameViewport()->Viewport:nullptr;
    const bool Focused=Viewport&&Viewport->HasFocus()&&Viewport->IsForegroundWindow()&&FApp::HasFocus();
    WarRoom52::Tick(this,Focused);
    FRRUWarRoomV82::Tick(this,Focused);
    auto& Atlas=WarRoom52::State(this);
    const int32 PageV834=HUD?HUD->GetPresentationPage():INDEX_NONE;
    const bool OpaqueAuxiliaryV834=PageV834==5||PageV834==7||PageV834==8||PageV834==12||PageV834==17||PageV834==19;
    const auto* ExistingMapViewV834=GetViewTarget();
    // These pages cover the whole viewport. Keep only the already active UK/DE
    // table and its camera; restoring the battlefield here would cut the camera,
    // rebuild environment render states, and undo the same work on returning Home.
    // CoveredWorld still skips the hidden view family, including viewport margins.
    const bool CoveredTableV834=OpaqueAuxiliaryV834&&Battle->Phase==EFrontlinePhase::Briefing
        &&Battle->GetCurrentCampaign()&&!WarfareFRRUCampaignV82::Active(Battle)
        &&(Battle->GetPlayerFaction()==EFrontlineFaction::British||Battle->GetPlayerFaction()==EFrontlineFaction::German)
        &&FrontlineMapSceneV61::IsActive(this)&&ExistingMapViewV834
        &&ExistingMapViewV834->GetWorld()==GetWorld()&&ExistingMapViewV834->ActorHasTag(TEXT("WF_MapScene_V61"))
        &&FactionSelectionV82::OwnsCoveredWorldV826(this);
    if(CoveredTableV834)
    {
        FrontlineOpaqueMapV834::Remember(this);
        FrontlineTextLayoutV834::RecordOpaqueMapHold(this);
        bPreviewValid=false;CameraPanVelocity=CameraVerticalVelocity=0;return;
    }
    const bool ContinueHomeV8214=ContinueHomeCoveredV8214(this,Battle);
    if(ContinueHomeV8214)FrontlineMapSceneV61::PreloadContinueV8214(this);
    // The same table remains resident and renders behind the alpha-one cover.
    // Continue starts the ordinary geographic flight; it no longer registers
    // the entire table, toggles every environment light, and cuts the camera
    // on the first visible destination frame.
    const bool KeepTableV8214=ContinueHomeV8214&&FrontlineMapSceneV61::ContinuePreloadReadyV8214(this)
        &&FrontlineUIAssetsV8213::PreloadSettled();
    if(FrontlineMapSceneV61::Update(this,WarRoom52::Active(this)||KeepTableV8214,Atlas.Centre,Atlas.Zoom,
        Atlas.FlightDuration>0?float((FPlatformTime::Seconds()-Atlas.FlightStart)/Atlas.FlightDuration):1.f,
        Atlas.AtlasSector,Atlas.Preview,Atlas.Detail))
    {
        FrontlineOpaqueMapV834::Resume(this,Battle);
        bPreviewValid=false;CameraPanVelocity=CameraVerticalVelocity=0;return;
    }
    FrontlineOpaqueMapV834::Discard();
    FrontlineCameraInput::FInput Pan;
    Pan.bActive=Focused&&!Screen.Menu&&!Screen.Help&&!Screen.Settings&&!Screen.Technology&&!Screen.AchievementsV76&&!Screen.Preparing
        &&Battle->Phase==EFrontlinePhase::Fighting;
    Pan.bLeftKey=IsInputKeyDown(EKeys::A)||IsInputKeyDown(EKeys::Left);
    Pan.bRightKey=IsInputKeyDown(EKeys::D)||IsInputKeyDown(EKeys::Right);
    int32 VX=0,VY=0;GetViewportSize(VX,VY);Pan.Viewport=FVector2D(VX,VY);
    float MX=0,MY=0;Pan.bPointerValid=GetMousePosition(MX,MY);Pan.Mouse=FVector2D(MX,MY);
    const float UIScale=FMath::Min(VX/1920.f,VY/1080.f),UIOffsetY=(VY-1080.f*UIScale)*.5f;
    Pan.FieldTop=UIOffsetY+78.f*UIScale;Pan.FieldBottom=UIOffsetY+DockTop*UIScale;
    Pan.bOverUI=HUD&&HUD->IsPointerOverInterface();
    // Explicit tactical pause permits keyboard surveying; a parked mouse stays still.
    if(UGameplayStatics::IsGamePaused(this))Pan.bPointerValid=false;
    CameraWidth=FMath::FInterpTo(CameraWidth,TargetCameraWidth,DT,8.f);
    const double Limit=FMath::Max(0.f,FrontlinePacingV24::CombatHalfLength()-CameraWidth*.5f);
    FrontlineCameraInput::Step(FrontlineCameraInput::Axis(Pan),CameraWidth,DT,FrontlineCameraInput::AllowsMotion(Pan),Limit,CameraFocus.X,CameraPanVelocity);
    const float VerticalAxis=float(IsInputKeyDown(EKeys::S)||IsInputKeyDown(EKeys::Down))-float(IsInputKeyDown(EKeys::W)||IsInputKeyDown(EKeys::Up));
    const double VerticalLimit=WarfareFRRUCampaignV82::Active(Battle)?FMath::Max(0.f,FrontlineBattlefield::HalfFrontage-CameraWidth*.32f):FrontlineTerrainV42::CameraVerticalLimitV43(CameraWidth,VY>0?float(VX)/VY:16.f/9.f);
    FrontlineCameraInput::Step(VerticalAxis,CameraWidth*.52f,DT,Pan.bActive,VerticalLimit,CameraFocus.Y,CameraVerticalVelocity);
    const FVector NativeComposition=WarfareFRRUVauxCompositionV87::CameraOffset(Battle);
    const FVector Focus=CameraFocus+FVector(0,0,650)+NativeComposition,Location=CameraFocus+FVector(0,14000,4650)+NativeComposition;
    BattleCamera->SetActorLocation(Location); BattleCamera->SetActorRotation((Focus-Location).Rotation());
    BattleCamera->GetCameraComponent()->OrthoWidth=CameraWidth;
    // The opaque army selector owns the covered viewport. Its hidden terrain
    // cache is refreshed on the first ordinary tick after leaving the page.
    if(!FactionSelectionV82::OwnsCoveredWorldV826(this))
        FrontlineTerrainV42::UpdateStreaming(GetWorld(),Location,Focus,CameraWidth,VX);
    if (DistantMatte.IsValid())
    {
        FrontlineTerrainV42::PlaceBackdropV43(DistantMatte.Get(),CameraFocus,CameraWidth,VY>0?float(VX)/VY:16.f/9.f);
    }
    SetViewTarget(BattleCamera);
    if (Battle->Phase!=EFrontlinePhase::Fighting) CancelTargeting();
    bPreviewValid=SupportTargetMode>0
        && (!HUD || !HUD->IsPointerOverInterface()) && GroundAtCursor(ArtilleryPreview);
    if(V28ReviewAim.IsSet() && SupportTargetMode>0){bPreviewValid=true;ArtilleryPreview=V28ReviewAim.GetValue();}
    if(bPreviewValid){ArtilleryPreview.Y=FMath::Clamp(ArtilleryPreview.Y,-double(FrontlineBattlefield::HalfFrontage),double(FrontlineBattlefield::HalfFrontage));ArtilleryPreview.Z=AWarfareFrontlineGround::HeightAt(ArtilleryPreview.X,ArtilleryPreview.Y);}
}

bool AWarfareFrontlineController::GroundAtCursor(FVector& Out) const
{
    FVector Ray,Direction;
    if (!DeprojectMousePositionToWorld(Ray,Direction) || FMath::Abs(Direction.Z)<.001f) return false;
    const float T=-Ray.Z/Direction.Z; if (T<0) return false;
    FVector Point=Ray+Direction*T;
    if (FMath::Abs(Point.X)>FrontlineBattlefield::SupportHalfLength || FMath::Abs(Point.Y)>FrontlineBattlefield::HalfFrontage+600.f) return false;
    // Project to the shared terrain, ignoring soldiers and the decorative props.
    for (int32 I=0;I<4;++I)
    {
        const float Z=AWarfareFrontlineGround::HeightAt(Point.X,Point.Y);
        Point=Ray+Direction*((Z-Ray.Z)/Direction.Z);
    }
    Point.Y=FMath::Clamp(Point.Y,-double(FrontlineBattlefield::HalfFrontage),double(FrontlineBattlefield::HalfFrontage));
    Point.Z=AWarfareFrontlineGround::HeightAt(Point.X,Point.Y); Out=Point; return true;
}

void AWarfareFrontlineController::ClickAtCursor()
{
    AWarfareFrontlineGameMode* Battle=FrontlineFor(this); if (!Battle) return;
    const AWarfareFrontlineHUD* HUD=Cast<AWarfareFrontlineHUD>(GetHUD());
    if (HUD && HUD->IsPointerOverInterface()) return;
    if (Battle->Phase!=EFrontlinePhase::Fighting) return;
    if (SupportTargetMode>0)
    {
        FVector Target;
        if (GroundAtCursor(Target))
        {Target.Z=AWarfareFrontlineGround::HeightAt(Target.X,Target.Y);ConfirmSupportAtWorldPoint(Target);}
        return;
    }
    if(FRRUWorldClickV82(this,Battle))return;
    float MX=0,MY=0; if (!GetMousePosition(MX,MY)) return;
    float Best=MAX_flt; int32 Index=INDEX_NONE;
    for (int32 I=0;I<Battle->Trenches.Num();++I)
    {
        const FFrontlineTrench& Trench=Battle->Trenches[I]; if (Trench.Owner!=0) continue;
        const FVector Centre(Trench.X,0,AWarfareFrontlineGround::HeightAt(Trench.X,0)+50);
        FVector2D P; if (!ProjectWorldLocationToScreen(Centre,P)) continue;
        int32 W=0,H=0; GetViewportSize(W,H); const float S=FMath::Min(W/1920.f,H/1080.f);
        const float DX=FMath::Abs(MX-P.X),DY=FMath::Abs(MY-P.Y);
        if (DX<115*S && DY<105*S && DX+DY<Best) { Best=DX+DY; Index=I; }
    }
    if (Index!=INDEX_NONE) SelectTrench(Index);
}

void AWarfareFrontlineController::BeginBattle()
{
    if(DecorationModalV8216(this))return;
    if(ScreenState(this).ExitConfirmV61)return;
    // Enter on the new-campaign page belongs exclusively to its fresh hold.
    if(FactionSelectionV82::Visible(this,FrontlineFor(this)))return;
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
    {
        FFrontlineScreenState& S=ScreenState(this);
        if(FrontlineReportV839::Active(this,B))return;
        if(FrontlineNewUnlockV8215::Pending(B)){NewUnlockUIV8215::Confirm(this,B);return;}
        if(UltimateArchiveUIV8215::Active(this))
        {
            if(UltimateArchiveUIV8215::Hit(this,B,TEXT("V8215ArchiveBegin"))==2){S.Dossier=1917;S.Deployment=true;S.DeploymentTabV63=0;S.HistoricalDispatch=S.UnitIntroduction=false;S.AcceptedLoadoutMission=INDEX_NONE;ResetCamera();}
            return;
        }
        if(S.CampaignEndingV51)
        {
            if(B->Phase==EFrontlinePhase::Briefing||B->ContinueCampaign())
            {S.CampaignEndingV51=false;S.Dossier=1917;S.Menu=false;ResetCamera();}
            return;
        }
        if (HasEnded(B))
        {
            if(S.Technology||S.Settings||S.Help||S.AchievementsV76)return;
            if(FrontlineReportV839::Active(this,B))return;
            if(!FrontlineAchievementsV76::FinalizeBattleKillsV8217(B))return;
            if(EndArchiveReplayV8215(this,B,true))return;
            if(FrontlineCampaignV51::CanOpenEnding(B)){S.CampaignEndingV51=true;S.Menu=false;FrontlineReportV839::OpenCampaignV86(this,B);return;}
            const auto* FinishedMissionV87=B->GetEncounterCampaignMission();
            const int32 AssignmentFromV87=FinishedMissionV87&&WarfareFRRUCampaignV82::HasNewAssignmentReceiptV87(B,FinishedMissionV87->Id)?FinishedMissionV87->Id:INDEX_NONE;
            if(B->ContinueCampaign()){S.Dossier=1917;S.Menu=false;ResetCamera();ReleaseNationalAssignmentAfterReportV87(this,B,AssignmentFromV87);FrontlineNewUnlockV8215::ReleaseAfterReportV8217(B);}return;
        }
        if (B->Phase!=EFrontlinePhase::Briefing) return;
        if (S.Dossier==0&&!S.Preparing&&!S.Help&&!S.Credits&&!S.Settings&&!S.Technology&&!S.AchievementsV76)
        {S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.Dossier=1918;return;}
        if(WarfareFRRUCampaignV82::Active(B)&&!WarfareFRRUCampaignV82::CampaignsAvailable(WarfareFRRUCampaignV82::Nation(B)))
        {B->LastMessage=WarfareFRRUCampaignV82::WorldBlockReason(B);return;}
        if(WarfareFRRUCampaignV82::Active(B)&&!WarfareFRRUCampaignV82::DeploymentBlockReason(B).IsEmpty())
        {
            B->LastMessage=WarfareFRRUCampaignV82::WorldBlockReason(B);
            // An incomplete roster can still be edited; confirmation cannot
            // start loading or bypass the world/required-worker checks.
            if(!WarfareFRRUCampaignV82::DeploymentBlockReason(B).IsEmpty()&&S.Dossier==1917
                &&!S.Deployment&&!S.Preparing&&!S.Help&&!S.Settings&&!S.Technology&&!S.AchievementsV76)S.Deployment=true;
            return;
        }
        if(WarfareFRRUCampaignV82::Active(B)&&!WarfareFRRUCampaignV82::WorldReady(B))
        {B->LastMessage=WarfareFRRUCampaignV82::WorldBlockReason(B);return;}
        if (!B->AssetFailure.IsEmpty()) { S.bAssetNoticeDismissed=false; return; }
        if (S.Preparing || S.Help || S.Settings || S.Technology || S.AchievementsV76) return;
        if(WarfareFRRUCampaignV82::Active(B))
        {
            if(S.Dossier!=1917||!B->GetSelectedCampaignMission())return;
            if(S.UnitIntroduction){ConfirmUnitIntroductionV27(this,B);return;}
            if(!S.Deployment){S.Deployment=true;return;}
            S.Deployment=false;
            S.Menu=S.Help=false;
            if(!WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B)
                &&(S.AcceptedNationalIntroCampaignV827!=WarfareFRRUCampaignV82::CampaignId(B)
                ||S.AcceptedNationalIntroMissionV827!=WarfareFRRUCampaignV82::MissionId(B)))
            {
                if(!UnitIntroductionCards(B).IsEmpty())
                {S.UnitIntroduction=true;S.UnitIntroductionPage=0;return;}
                S.AcceptedNationalIntroCampaignV827=WarfareFRRUCampaignV82::CampaignId(B);
                S.AcceptedNationalIntroMissionV827=WarfareFRRUCampaignV82::MissionId(B);
            }
            BeginOperationPreparation(this,B);return;
        }
        if (S.Dossier!=1917 || !B->GetSelectedCampaignMission()) return;
        S.Menu=false; S.Help=false;
        if(!S.HistoricalDispatch&&!S.Deployment&&!S.UnitIntroduction)
        {
            auto& Map=WarRoom52::State(this);WarRoom52::Synchronize(Map,B);
            if(Map.AtlasSector>=5||Map.FlightDuration>0.)return;
            if(Map.Preview!=B->GetCurrentCampaign()->SelectedMission&&!FrontlineCampaignV52::Select(B,Map.Preview)){Map.Notice=B->CampaignMissionBlockReason(Map.Preview);return;}
            FrontlineMapSceneV61::CaptureLoadingView(this);
        }
        if(!FrontlineUltimateArchiveV8215::Active(B)&&FrontlineCampaignV52::Dispatch(B).bPending){S.HistoricalDispatch=true;return;}
        S.HistoricalDispatch=false;
        const auto L=FrontlineCampaignV32::GetLoadout(B);const auto* Run=B->GetCurrentCampaign();
        if(S.Deployment && Run){S.Deployment=false;S.AcceptedLoadoutCampaign=Run->Id;S.AcceptedLoadoutMission=Run->SelectedMission;}
        if(Run && (L.bSelectRoles||L.bSelectSupports) && (S.AcceptedLoadoutCampaign!=Run->Id||S.AcceptedLoadoutMission!=Run->SelectedMission))
        {S.Deployment=true;return;}
        // Subsequent assignments are introduced after the preceding victory.
        if(FrontlineUltimateArchiveV8215::Active(B)||(Run&&Run->CompletedMissions>0)){S.UnitIntroduction=false;BeginOperationPreparation(this,B);return;}
        if(S.UnitIntroduction)ConfirmUnitIntroductionV27(this,B);else {S.UnitIntroduction=true;S.UnitIntroductionPage=0;}
    }
}
void AWarfareFrontlineController::RecruitRifle()
{ RecruitFieldSlotV32(this,0); }
void AWarfareFrontlineController::RecruitLewis()
{ RecruitFieldSlotV32(this,1); }
void AWarfareFrontlineController::RecruitMills()
{ RecruitFieldSlotV32(this,2); }
void AWarfareFrontlineController::RecruitMarksman()
{ RecruitFieldSlotV32(this,3); }
void AWarfareFrontlineController::RecruitOfficer()
{ RecruitFieldSlotV32(this,4); }
void AWarfareFrontlineController::ReleaseSelected()
{
    if(DecorationModalV8216(this))return;
    const auto& S=ScreenState(this);if (S.Menu || S.Help || S.Settings || S.Technology || S.AchievementsV76) return;
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
        if (B->Phase==EFrontlinePhase::Fighting && B->Trenches.IsValidIndex(SelectedTrench)) B->AdvanceTrench(SelectedTrench);
}
void AWarfareFrontlineController::ToggleArtillery()
{
    if(DecorationModalV8216(this))return;
    if(FRRUStoryModalV82(FrontlineFor(this)))return;
    if (SupportTargetMode==1) { CancelTargeting(); return; }
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
    {
        const FString Reason=B->ArtilleryBlockReason();
        if (!Reason.IsEmpty()) { B->LastMessage=Reason; return; }
        SupportTargetMode=1; bTargetingArtillery=true; B->LastMessage=TEXT("选择炮击落点。炮弹延迟抵达，爆炸会伤及友军。");
    }
}
void AWarfareFrontlineController::ToggleSmoke()
{
    if(DecorationModalV8216(this))return;
    if(FRRUStoryModalV82(FrontlineFor(this)))return;
    if (SupportTargetMode==2) { CancelTargeting(); return; }
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
    {
        const FString Reason=B->SmokeBlockReason(); if (!Reason.IsEmpty()) { B->LastMessage=Reason; return; }
        SupportTargetMode=2; bTargetingArtillery=false; B->LastMessage=TEXT("选择烟幕落点，削弱双方穿烟直射，为出壕争取时间。");
    }
}
void AWarfareFrontlineController::ToggleMortar()
{
    if(DecorationModalV8216(this))return;
    if(FRRUStoryModalV82(FrontlineFor(this)))return;
    if (SupportTargetMode==4) { CancelTargeting(); return; }
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
    {
        const FString Reason=B->MortarBlockReason(); if (!Reason.IsEmpty()) { B->LastMessage=Reason; return; }
        SupportTargetMode=4; bTargetingArtillery=false; B->LastMessage=TEXT("选择轻迫击炮落点，三轮覆盖，首弹延迟抵达，注意友军。");
    }
}
void AWarfareFrontlineController::ToggleRally()
{
    if(DecorationModalV8216(this))return;
    if(FRRUStoryModalV82(FrontlineFor(this)))return;
    if (SupportTargetMode==3) { CancelTargeting(); return; }
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
    {
        const FString Reason=B->RallyBlockReason(); if (!Reason.IsEmpty()) { B->LastMessage=Reason; return; }
        SupportTargetMode=3; bTargetingArtillery=false; B->LastMessage=TEXT("选择友军集结位置，缓解受压并鼓舞士气。");
    }
}
void AWarfareFrontlineController::ConfirmSupportAtWorldPoint(const FVector& Target)
{
    if(DecorationModalV8216(this))return;
    AWarfareFrontlineGameMode* B=FrontlineFor(this);
    if (!B || SupportTargetMode<=0 || B->Phase!=EFrontlinePhase::Fighting || UGameplayStatics::IsGamePaused(this)||FRRUStoryModalV82(B)) return;
    const FFrontlineScreenState& S=ScreenState(this); if (S.Menu || S.Help || S.Settings || S.Technology || S.AchievementsV76) return;
    const bool Accepted=SupportTargetMode==3?B->RequestRally(Target):B->RequestSupport(SupportForTargetMode(SupportTargetMode),Target);
    if (Accepted) {auto& State=ScreenState(this);State.SupportSealV63=Target;State.SupportExtentV63=B->FireSupportScatterExtent(SupportForTargetMode(SupportTargetMode));State.SupportSealAtV63=FPlatformTime::Seconds();CancelTargeting();}
}
void AWarfareFrontlineController::ToggleMenu()
{
    if (SupportTargetMode>0) { CancelTargeting(); return; }
    FFrontlineScreenState& S=ScreenState(this);
    if(FrontlineReportV839::Close(this,FrontlineFor(this)))return;
    // A selector close restores its saved entry page. Do not record the
    // hidden replay page again when the ordinary navigation guard unwinds.
    if(UltimateArchiveUIV8215::Active(this)){UltimateArchiveUIV8215::BackV851(this);return;}
    NavigationV61::FRecord Record(this);
    if(FactionSelectionV82::Back(this,FrontlineFor(this)))return;
    if(S.ExitConfirmV61){S.ExitConfirmV61=false;return;}
    if(MedalMountV8216::Back(this)||MedalWallV8215::Back(this))return;
    if(auto* B=FrontlineFor(this);FrontlineNewUnlockV8215::Pending(B)){FrontlineNewUnlockV8215::Dismiss(B);return;}
    if(S.AchievementsV76){ToggleAchievementsV76();return;}
    if (S.Settings) { ToggleSettings(); return; }
    if(S.CampaignEndingV51)
    {
        if(auto* Battle=FrontlineFor(this);Battle&&(Battle->Phase==EFrontlinePhase::Briefing||Battle->ContinueCampaign()))
        {S.CampaignEndingV51=false;S.Dossier=0;S.Menu=false;ResetCamera();}
        return;
    }
    if(S.HistoricalDispatch){S.HistoricalDispatch=false;return;}
    if(S.Deployment){if(EndArchiveReplayV8215(this,FrontlineFor(this),true))return;S.Deployment=false;return;}
    if (S.UnitIntroduction) {if(S.UnitIntroductionPage>0)--S.UnitIntroductionPage;else if(!DismissNationalAssignmentV87(this,FrontlineFor(this)))S.UnitIntroduction=false;return;}
    if (S.Preparing) {if(EndArchiveReplayV8215(this,FrontlineFor(this),true))return;if(auto* B=FrontlineFor(this))B->AbandonBattleToOperations();ClearPreparationWarmersV43(S);S.Preparing=false;S.PreparationError.Empty();S.Dossier=1917;ResetCamera();return;}
    if(!S.ArchivePendingDelete.IsEmpty()){S.ArchivePendingDelete.Empty();return;}
    if (S.Technology) { ToggleTechnology(); return; }
    if (S.Help) { ToggleHelp(); return; }
    AWarfareFrontlineGameMode* B=FrontlineFor(this); if (!B) return;
    if(FrontlineReportV839::Close(this,B))return;
    if (B->Phase==EFrontlinePhase::Briefing) { if(EndArchiveReplayV8215(this,B,true))return; if(S.Dossier==0)S.ExitConfirmV61=true;else S.Dossier=0; return; }
    if (B->Phase!=EFrontlinePhase::Fighting) return;
    S.Menu=!S.Menu;
    if (S.Menu) { S.bPausedByOverlay=!UGameplayStatics::IsGamePaused(this); B->SetPaused(true); }
    else if (S.bPausedByOverlay) { WarfareFRRUWorldV82::ResumeAfterOwnedPause(B); S.bPausedByOverlay=false; }
}
void AWarfareFrontlineController::MenuBackV61(){if(FrontlineReportV839::Active(this,FrontlineFor(this))){ToggleMenu();return;}if(MedalMountV8216::Back(this)||MedalWallV8215::Back(this))return;if(UltimateArchiveUIV8215::Active(this)){UltimateArchiveUIV8215::BackV851(this);return;}if(auto* B=FrontlineFor(this);FrontlineUltimateArchiveV8215::Active(B)&&B->Phase==EFrontlinePhase::Briefing){ToggleMenu();return;}FactionSelectionV82::Cancel(this);NavigationV61::Travel(this,false);}
void AWarfareFrontlineController::MenuForwardV61(){if(FrontlineReportV839::Active(this,FrontlineFor(this)))return;if(MedalMountV8216::View(this).Picker||MedalWallV8215::View(this).Inspect)return;if(UltimateArchiveUIV8215::Active(this))return;if(auto* B=FrontlineFor(this);FrontlineUltimateArchiveV8215::Active(B)&&B->Phase==EFrontlinePhase::Briefing)return;FactionSelectionV82::Cancel(this);NavigationV61::Travel(this,true);}
void AWarfareFrontlineController::ToggleHelp()
{
    if(DecorationModalV8216(this))return;
    if(ScreenState(this).ExitConfirmV61)return;
    NavigationV61::FRecord Record(this);
    if(ScreenState(this).CampaignEndingV51)return;
    if (ScreenState(this).UnitIntroduction || ScreenState(this).Preparing || ScreenState(this).Deployment || ScreenState(this).HistoricalDispatch || ScreenState(this).Settings || ScreenState(this).Technology || ScreenState(this).AchievementsV76) return;
    FFrontlineScreenState& S=ScreenState(this); S.Help=!S.Help; S.Credits=false;
    if (S.Help) CancelTargeting();
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this)) if (B->Phase==EFrontlinePhase::Fighting && !S.Menu)
    {
        if (S.Help) { S.bPausedByOverlay=!UGameplayStatics::IsGamePaused(this); B->SetPaused(true); }
        else if (S.bPausedByOverlay) { WarfareFRRUWorldV82::ResumeAfterOwnedPause(B); S.bPausedByOverlay=false; }
    }
}
void AWarfareFrontlineController::TogglePerformanceMonitor()
{
    FFrontlineScreenState& S=ScreenState(this); S.Performance=!S.Performance;
    S.PerfLast=0; S.FrameHistory.Empty();
    if (GConfig && !FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))) { GConfig->SetBool(TEXT("Warfare.Display"),TEXT("PerformanceMonitor"),S.Performance,GGameUserSettingsIni); GConfig->Flush(false,GGameUserSettingsIni); }
}
void AWarfareFrontlineController::ToggleAchievementsV76()
{
    if(DecorationModalV8216(this))return;
    auto& S=ScreenState(this);if(S.Preparing||S.UnitIntroduction||S.CampaignEndingV51||S.Settings||S.Technology||S.Help||S.ExitConfirmV61)return;
    NavigationV61::FRecord Record(this);S.AchievementsV76=!S.AchievementsV76;CancelTargeting();
    if(S.AchievementsV76)
    {
        HonoursListV82::Reveal(S,S.AchievementFocusV76,true);
    }
    if(auto* B=FrontlineFor(this);B&&B->Phase==EFrontlinePhase::Fighting&&!S.Menu)
    {if(S.AchievementsV76){S.bPausedForAuxOverlay=!UGameplayStatics::IsGamePaused(this);B->SetPaused(true);}
     else if(S.bPausedForAuxOverlay){WarfareFRRUWorldV82::ResumeAfterOwnedPause(B);S.bPausedForAuxOverlay=false;}}
}
void AWarfareFrontlineController::ToggleSettings()
{
    if(DecorationModalV8216(this))return;
    if(ScreenState(this).ExitConfirmV61)return;
    NavigationV61::FRecord Record(this);
    if(ScreenState(this).CampaignEndingV51)return;
    FFrontlineScreenState& S=ScreenState(this); if (S.UnitIntroduction || S.Preparing || S.Technology || S.Help || S.AchievementsV76) return;
    S.Settings=!S.Settings; CancelTargeting();
    if(!S.Settings)if(auto* R=UFrontlineRenderSettingsSubsystem::Get(GetWorld()))R->SetHDRCalibrationVisible(false);
    if (S.Settings) LoadDisplayChoices(S); else FinishVideoChange(S,false);
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this)) if (B->Phase==EFrontlinePhase::Fighting && !S.Menu)
    {
        if (S.Settings) { S.bPausedForAuxOverlay=!UGameplayStatics::IsGamePaused(this); B->SetPaused(true); }
        else if (S.bPausedForAuxOverlay) { WarfareFRRUWorldV82::ResumeAfterOwnedPause(B); S.bPausedForAuxOverlay=false; }
    }
}
void AWarfareFrontlineController::ToggleTechnology()
{
    if(DecorationModalV8216(this))return;
    if(const auto* B=FrontlineFor(this);B&&HasEnded(B))return;
    if(ScreenState(this).ExitConfirmV61)return;
    NavigationV61::FRecord Record(this);
    if(ScreenState(this).CampaignEndingV51)return;
    FFrontlineScreenState& S=ScreenState(this); if (S.UnitIntroduction || S.Preparing || S.Deployment || S.HistoricalDispatch || S.Settings || S.Help || S.AchievementsV76) return;
    if (!S.Technology) if (auto* B=FrontlineFor(this))
        if (B->Phase==EFrontlinePhase::Briefing && (S.Dossier!=1917 || (!B->GetCurrentCampaign()&&!WarfareFRRUCampaignV82::Active(B)))) return;
    S.Technology=!S.Technology; CancelTargeting();
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this)) if (B->Phase==EFrontlinePhase::Fighting && !S.Menu)
    {
        if (S.Technology) { S.bPausedForAuxOverlay=!UGameplayStatics::IsGamePaused(this); B->SetPaused(true); }
        else if (S.bPausedForAuxOverlay) { WarfareFRRUWorldV82::ResumeAfterOwnedPause(B); S.bPausedForAuxOverlay=false; }
    }
}
void AWarfareFrontlineController::ResetCamera()
{
    const auto* B=FrontlineFor(this);
    SelectedTrench=INDEX_NONE;CameraFocus=FVector(B?B->GetHomeCameraFocusX():FrontlineBattlefield::TrenchX(0)+1300,0,0);
    CameraPanVelocity=CameraVerticalVelocity=0;TargetCameraWidth=6000;
}
void AWarfareFrontlineController::TogglePause()
{ if(DecorationModalV8216(this))return;const auto& S=ScreenState(this);if(S.Preparing||S.UnitIntroduction||S.Deployment||S.HistoricalDispatch||S.Menu||S.Help||S.Settings||S.Technology||S.AchievementsV76)return;if(auto* B=FrontlineFor(this))if(B->Phase==EFrontlinePhase::Fighting){if(WarfareFRRUWorldV82::PauseForPendingStory(B))return;B->SetPaused(!UGameplayStatics::IsGamePaused(this));} }
void AWarfareFrontlineController::ToggleSpeed()
{
    const auto& S=ScreenState(this); if (S.Settings || S.Technology || S.AchievementsV76 || S.Help || S.Menu) return;
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this);B&&B->Phase==EFrontlinePhase::Fighting) B->SetBattleSpeed(B->BattleSpeed<.75f?1.f:B->BattleSpeed<1.25f?1.5f:.5f);
}
void AWarfareFrontlineController::Restart()
{
    if(DecorationModalV8216(this))return;
    if(FrontlineReportV839::Active(this,FrontlineFor(this)))return;
    if(ScreenState(this).ExitConfirmV61)return;
    const auto& Overlay=ScreenState(this); if (Overlay.Settings || Overlay.Technology || Overlay.Help || Overlay.AchievementsV76) return;
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
    {const auto* M=B->GetEncounterCampaignMission();const int32 Replay=M?M->Id:INDEX_NONE;if(!B->RestartBattle())return;if(Replay!=INDEX_NONE) B->SelectCampaignMission(Replay);}
    FactionSelectionV82::Cancel(this);ScreenState(this).FactionSelectionV82=FFrontlineFactionSelectionV82();CancelTargeting();ResetCamera();
    FFrontlineScreenState& S=ScreenState(this); FinishVideoChange(S,false); ClearPreparationWarmersV43(S);S.Preparing=false; S.CampaignEndingV51=false; S.HistoricalDispatch=S.Deployment=false; S.AcceptedLoadoutMission=INDEX_NONE; S.UnitIntroduction=false; S.NationalAssignmentNoticeV87=false; S.UnitIntroductionPage=0; S.PreparationError.Empty(); S.Dossier=1917; S.Menu=false; S.Help=false; S.Settings=false; S.Technology=false;S.AchievementsV76=false; S.bPausedByOverlay=false; S.bPausedForAuxOverlay=false; S.bAssetNoticeDismissed=false; S.Dispatches.Empty(); S.LastDispatch.Empty();
    if (const auto* B=FrontlineFor(this)) if (!B->GetCurrentCampaign()&&!WarfareFRRUCampaignV82::Active(B)) S.Dossier=0;
    if(const auto* B=FrontlineFor(this);B&&FrontlineUltimateArchiveV8215::Active(B))S.Deployment=true;
}
void AWarfareFrontlineController::SelectTrench(int32 Index)
{
    if(DecorationModalV8216(this))return;
    if (AWarfareFrontlineGameMode* B=FrontlineFor(this))
        if (B->Trenches.IsValidIndex(Index) && B->Trenches[Index].Owner==0)
        { SelectedTrench=Index; CancelTargeting(); CameraFocus.X=B->Trenches[Index].X+B->GetTeamDirection(0)*1800; CameraPanVelocity=0; }
}
void AWarfareFrontlineController::CancelTargeting() { SupportTargetMode=0; bTargetingArtillery=false; bPreviewValid=false; }
void AWarfareFrontlineController::ZoomIn() { if(MedalMountV8216::Wheel(this,-1))return; if(HonoursListV82::Wheel(this,-1))return; if(!ScreenState(this).Settings&&!ScreenState(this).Help&&!ScreenState(this).Menu&&FrontlineReportV839::Wheel(this,FrontlineFor(this),-1))return; if(FRRUWarRoomV82::Active(this)){FRRUWarRoomV82::Zoom(this,1.16f);return;} if(WarRoom52::Active(this)){WarRoom52::Zoom(this,1.16f);return;} const auto& S=ScreenState(this); if (S.Preparing || S.UnitIntroduction || S.Settings || S.Technology || S.AchievementsV76 || S.Help || S.Menu) return; TargetCameraWidth=FMath::Max(3400.f,TargetCameraWidth-700.f); }
void AWarfareFrontlineController::ZoomOut() { if(MedalMountV8216::Wheel(this,1))return; if(HonoursListV82::Wheel(this,1))return; if(!ScreenState(this).Settings&&!ScreenState(this).Help&&!ScreenState(this).Menu&&FrontlineReportV839::Wheel(this,FrontlineFor(this),1))return; if(FRRUWarRoomV82::Active(this)){FRRUWarRoomV82::Zoom(this,1/1.16f);return;} if(WarRoom52::Active(this)){WarRoom52::Zoom(this,1/1.16f);return;} const auto& S=ScreenState(this); if (S.Preparing || S.UnitIntroduction || S.Settings || S.Technology || S.AchievementsV76 || S.Help || S.Menu) return; TargetCameraWidth=FMath::Min(9000.f,TargetCameraWidth+850.f); }

int32 AWarfareFrontlineHUD::GetPresentationPage() const
{
    const AWarfareFrontlineGameMode* B=FrontlineFor(this); if (!B) return 0;
    const FFrontlineScreenState& S=ScreenState(PlayerOwner);
    if(S.CampaignEndingV51)return 18;
    if(S.Preparing)return 9;
    if(S.AchievementsV76)return 19;
    if (S.Settings) return 7;
    if(S.HistoricalDispatch)return 16;
    if(S.Deployment)return 17;
    if (S.UnitIntroduction) return 15;
    if (S.Preparing) return 9;
    if (S.Technology) return 8;
    if (!B->AssetFailure.IsEmpty() && !S.bAssetNoticeDismissed) return 10;
    if (S.Help) return 5;
    if (HasEnded(B)) return 6;
    if (B->Phase==EFrontlinePhase::Briefing) return S.Dossier==1918?11:S.Dossier==1919?12:S.Dossier==1944?2:S.Dossier==1917?1:0;
    return S.Menu?4:3;
}

bool AWarfareFrontlineHUD::IsPointerOverInterface() const
{
    if (!PlayerOwner) return false;
    if (LastUIInputOwner.Get()==this && LastUIInputFrame==GFrameCounter) return true;
    if (GetPresentationPage()!=3) return true;
    float X=0,Y=0; if (!PlayerOwner->GetMousePosition(X,Y)) return false;
    return GetHitBoxAtCoordinates(FVector2D(X,Y),true)!=nullptr;
}

void AWarfareFrontlineHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    bool RealInput=PlayerOwner&&!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&!FParse::Param(FCommandLine::Get(),TEXT("unattended"));
#if WITH_EDITOR
    FString InputReviewUser;
    if(PlayerOwner&&!FParse::Param(FCommandLine::Get(),TEXT("unattended"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFAuthenticInputReviewV82"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),InputReviewUser))
    {
        FPaths::NormalizeDirectoryName(InputReviewUser);
        RealInput=InputReviewUser.StartsWith(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/FRRU/UIReview/")),ESearchCase::IgnoreCase)
            &&!InputReviewUser.Contains(TEXT(".."));
    }
#endif
    TGuardValue<bool> AuthenticInput(NavigationV61::AuthenticHUDInputV75,RealInput);
    LastUIInputOwner=this; LastUIInputFrame=GFrameCounter;
    FString Command,Region;
    if (BoxName.ToString().Split(TEXT("__"),&Command,&Region)) BoxName=FName(*Command);
    AWarfareFrontlineController* PC=Cast<AWarfareFrontlineController>(PlayerOwner);
    AWarfareFrontlineGameMode* B=FrontlineFor(this); if (!PC || !B || BoxName==TEXT("Blocked")) return;
    FFrontlineScreenState& S=ScreenState(PC);
    if(FrontlineDebugV37::IsOpen(PC))return;
    if(!S.Preparing&&!S.Settings&&!S.Technology&&!S.Help&&!S.Menu&&!S.AchievementsV76&&FrontlineReportV839::Click(PC,B,BoxName))
    {NavigationV61::Click(PC,BoxName);return;}
    if(MedalMountV8216::Hit(PC,B,BoxName)){NavigationV61::Click(PC,BoxName);return;}
    if(UltimateArchiveUIV8215::Active(PC))
    {
        NavigationV61::Click(PC,BoxName);
        const int32 Result=UltimateArchiveUIV8215::Hit(PC,B,BoxName);
        if(Result==2){S.Dossier=1917;S.Deployment=true;S.DeploymentTabV63=0;S.HistoricalDispatch=S.UnitIntroduction=false;S.AcceptedLoadoutMission=INDEX_NONE;PC->ResetCamera();}
        return;
    }
    if(FrontlineNewUnlockV8215::Pending(B)){if(NewUnlockUIV8215::Hit(PC,B,BoxName))NavigationV61::Click(PC,BoxName);return;}
    if(BoxName==TEXT("V8215ArchiveOpen")&&B->Phase==EFrontlinePhase::Briefing)
    {
        const bool InWarRoom=WarRoom52::Active(PC)||FRRUWarRoomV82::Active(PC);
        bool Available=InWarRoom&&FrontlineUltimateArchiveV8215::NationAvailableV87(B,WarfareFRRUCampaignV82::Nation(B,0));
        if(!InWarRoom)for(int32 N=0;N<4;++N)Available|=FrontlineUltimateArchiveV8215::NationAvailableV87(B,EFRRUNationV81(N));
        if(Available){NavigationV61::Click(PC,BoxName);UltimateArchiveUIV8215::Open(PC,B);}
        return;
    }
    if(FrontlineUltimateArchiveV8215::Active(B))
    {
        if(BoxName==TEXT("V32DeploymentBack")||BoxName==TEXT("PreparationBack")||BoxName==TEXT("AbandonBattle")||BoxName==TEXT("CampaignAfterBattle")){NavigationV61::Click(PC,BoxName);EndArchiveReplayV8215(PC,B,true);return;}
        if(BoxName==TEXT("MenuHome")||BoxName==TEXT("NewCampaign")||BoxName==TEXT("ContinueCampaign")||BoxName==TEXT("CampaignArchives"))if(!EndArchiveReplayV8215(PC,B,false))return;
    }
    if(FactionSelectionV82::Click(PC,B,BoxName))return;
    if(FRRUStoryModalV82(B)&&!S.Menu&&!S.Help&&!S.Settings&&!S.Technology&&!S.AchievementsV76)
    {
        const bool StoryChoiceV82=BoxName.ToString().StartsWith(TEXT("FRRUWorld:story:"));
        if(!StoryChoiceV82&&BoxName!=TEXT("Menu")&&BoxName!=TEXT("Pause"))return;
    }
    if(S.ExitConfirmV61)
    {
        if(BoxName==TEXT("ExitCancelV61")){S.ExitConfirmV61=false;NavigationV61::Click(PC,BoxName);}
        else if(BoxName==TEXT("ExitConfirmV61")&&B->Phase==EFrontlinePhase::Briefing&&S.Dossier==0)
        {UE_LOG(LogTemp,Display,TEXT("WF_V61_EXIT_CONFIRMED"));FPlatformMisc::RequestExit(false);}
        return;
    }
    // Resolve at click time as well as during HUD painting: a fast pointer can
    // leave last frame's small geographic hover hitbox before the click arrives.
    if(BoxName==TEXT("V52MapSurface")&&WarRoom52::Active(PC))
    {
        float MX=0,MY=0;int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);
        const float Scale=FMath::Min(VX/1920.f,VY/1080.f);
        if(Scale>0&&PC->GetMousePosition(MX,MY))
        {
            const FVector2D Pixel((MX-(VX-1920.f*Scale)*.5f)/Scale,(MY-(VY-1080.f*Scale)*.5f)/Scale);
            const int32 Sector=WarRoom52::AtlasSectorAtPointV61(WarRoom52::State(PC),Pixel);
            if(Sector!=INDEX_NONE)BoxName=FName(*FString::Printf(TEXT("V60Sector%d"),Sector));
        }
    }
    if(S.Technology&&BoxName.ToString().StartsWith(TEXT("V63ArmourBuy")))
    {
        NavigationV61::Click(PC,BoxName);
        const int32 Id=FCString::Atoi(*BoxName.ToString().Mid(12));
        if(FrontlineUltimateArchiveV8215::Active(B)){FrontlineUltimateArchiveV8215::SetArmourTechnologyEnabled(B,Id,!FrontlineArmourV63::Has(B,Id));return;}
        if(FrontlineArmourV63::BuyResearch(B,Id))S.ResearchRevealsV72.Add(ResearchCardKeyV72(B,Id,true),FPlatformTime::Seconds());
        return;
    }
    NavigationV61::FRecord Record(PC);
    NavigationV61::Click(PC,BoxName);
    if(MedalWallV8215::OpenWarRoomRibbonV8444(PC,B,BoxName))
    {if(!S.AchievementsV76)PC->ToggleAchievementsV76();return;}
    if(S.CampaignEndingV51)
    {
        if(BoxName==TEXT("V51EndingOperations")||BoxName==TEXT("V51EndingArchives")||BoxName==TEXT("V51EndingMenu"))
        {
            if(B->Phase==EFrontlinePhase::Briefing||B->ContinueCampaign())
            {
                S.CampaignEndingV51=false;S.Menu=S.Help=false;
                S.Dossier=BoxName==TEXT("V51EndingArchives")?1919:BoxName==TEXT("V51EndingMenu")?0:1917;
                if(BoxName==TEXT("V51EndingArchives"))S.FRRUArchivesV82=WarfareFRRUCampaignV82::Active(B);
                S.ArchiveOffset=0;S.ArchiveSelection.Empty();PC->ResetCamera();
            }
        }
        return;
    }
    if(S.Preparing){if(BoxName==TEXT("PreparationBack")&&!S.PreparationError.IsEmpty()){B->AbandonBattleToOperations();ClearPreparationWarmersV43(S);S.Preparing=false;S.PreparationError.Empty();S.Dossier=1917;PC->ResetCamera();}return;}
    // The displayed auxiliary page owns input; its underlying workflow stays intact.
    if(S.AchievementsV76)
    {
        if(MedalWallV8215::Hit(PC,BoxName))return;
        if(BoxName==TEXT("AchievementsV76")||BoxName==TEXT("AchievementsBackV76")){PC->ToggleAchievementsV76();return;}
        const FString Name=BoxName.ToString();
        if(Name.StartsWith(TEXT("AchievementV76_")))
        {
            const FString Suffix=Name.RightChop(15);
            const int32 Index=Suffix.IsNumeric()?FCString::Atoi(*Suffix):INDEX_NONE;
            if(HonoursListV82::Visible(S,Index))S.AchievementFocusV76=Index;
        }
        return;
    }
    if (S.Settings)
    {
        if(BoxName==TEXT("Settings")||BoxName==TEXT("DisplayBack")){PC->ToggleSettings();return;}
        if(BoxName==TEXT("UpdateCheck")){FrontlineUpdatesV36::Check();return;}
        if(BoxName==TEXT("UpdateOpen")){FrontlineUpdatesV36::OpenReleasePage();return;}
        if(BoxName==TEXT("LanguageEnglish")||BoxName==TEXT("LanguageChinese"))
        {FrontlineLocalizationV35::SetLanguage(BoxName==TEXT("LanguageChinese")?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);return;}
        if (BoxName==TEXT("SettingsAudio")) { S.SettingsTab=1; return; }
        if (BoxName==TEXT("SettingsVideo")) { S.SettingsTab=0; return; }
        if (BoxName==TEXT("SettingsImage")) { S.SettingsTab=2; return; }
        if (BoxName==TEXT("SettingsHDR")) { S.SettingsTab=3; return; }
        if (S.SettingsTab==2 && HandleImageTechnology(GetWorld(),BoxName)) return;
        if (S.SettingsTab==3 && HandleHDRSettings(GetWorld(),BoxName)) return;
        if (S.SettingsTab==0 && HandleFrameRate(S,GetWorld(),BoxName)) return;
        if(BoxName==TEXT("FilmGrain")){FrontlineFilmV32::SetEnabled(!FrontlineFilmV32::IsEnabled());return;}
        if(S.FrameRateDragging || S.FrameRateKeyHeld)
            if(auto* Render=UFrontlineRenderSettingsSubsystem::Get(GetWorld()))Render->CommitFrameRateLimit();
        S.FrameRateFocused=S.FrameRateDragging=S.FrameRateKeyHeld=false;
        if (UFrontlineAudioSubsystem* Audio=GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
        {
            if (BoxName==TEXT("AudioMute")) { Audio->SetMuted(!Audio->IsMuted()); return; }
            const FString AudioName=BoxName.ToString();
            for (int32 Bus=0;Bus<5;++Bus)
            {
                const EFrontlineAudioBus Channel=EFrontlineAudioBus(Bus);
                if (AudioName==FString::Printf(TEXT("AudioDown%d"),Bus)) { Audio->SetVolume(Channel,Audio->GetVolume(Channel)-.1f); return; }
                if (AudioName==FString::Printf(TEXT("AudioUp%d"),Bus)) { Audio->SetVolume(Channel,Audio->GetVolume(Channel)+.1f); return; }
                for (int32 Step=0;Step<=10;++Step)
                    if (AudioName==FString::Printf(TEXT("AudioLevel%d_%d"),Bus,Step)) { Audio->SetVolume(Channel,Step*.1f); return; }
            }
        }
        if (BoxName==TEXT("VideoApply")) ApplyVideoChange(S);
        else
        {
            if (BoxName==TEXT("ResolutionPrev") || BoxName==TEXT("ResolutionNext"))
            {
                if (S.DisplayMode!=EWindowMode::WindowedFullscreen && S.ResolutionChoices.Num())
                {
                    int32 Index=S.ResolutionChoices.IndexOfByKey(S.DisplayResolution); if (Index<0) Index=0;
                    const int32 N=S.ResolutionChoices.Num();
                    Index=(Index+(BoxName==TEXT("ResolutionNext")?1:N-1))%N; S.DisplayResolution=S.ResolutionChoices[Index];
                }
            }
            else if (BoxName==TEXT("WindowMode0")) S.DisplayMode=EWindowMode::Windowed;
            else if (BoxName==TEXT("WindowMode1"))
            { S.DisplayMode=EWindowMode::WindowedFullscreen; if (GEngine && GEngine->GetGameUserSettings()) S.DisplayResolution=GEngine->GetGameUserSettings()->GetDesktopResolution(); }
            else if (BoxName==TEXT("WindowMode2")) S.DisplayMode=EWindowMode::Fullscreen;
        }
        return;
    }
    if(!S.ArchivePendingDelete.IsEmpty())
    {
        if(BoxName==TEXT("ArchiveDeleteCancel"))S.ArchivePendingDelete.Empty();
        else if(BoxName==TEXT("ArchiveDeleteConfirm"))
        {
            const bool Deleted=S.FRRUArchivesV82?WarfareFRRUCampaignV82::Delete(B,S.ArchivePendingDelete):B->DeleteCampaignArchives(S.ArchivePendingDelete);
            if(Deleted){S.ArchiveSelection.Empty();S.ArchiveOffset=0;}
            S.ArchiveNotice=S.FRRUArchivesV82?WarfareFRRUCampaignV82::Status(B):B->LastMessage;S.ArchivePendingDelete.Empty();
        }
        return;
    }
    if(S.HistoricalDispatch)
    {
        if(BoxName==TEXT("V52DispatchBack")){S.HistoricalDispatch=false;return;}
        const FString N=BoxName.ToString();
        if(N.StartsWith(TEXT("V52DispatchChoice"))&&FrontlineCampaignV52::ResolveDispatch(B,FCString::Atoi(*N.Mid(17))))
        {S.HistoricalDispatch=false;PC->BeginBattle();}
        return;
    }
    if(FRRUArmourUIV82::Hit(BoxName,PC,B))return;
    if(S.Deployment)
    {
        const FString N=BoxName.ToString();
        if(BoxName==TEXT("V52GunFixed"))FrontlineGameplayV52::SetMobileGun(B,false);
        else if(BoxName==TEXT("V52GunMobile"))FrontlineGameplayV52::SetMobileGun(B,true);
        else if(N.StartsWith(TEXT("V32Role")))FrontlineCampaignV32::ToggleRole(B,EFrontlineRole(FCString::Atoi(*N.Mid(7))));
        else if(N.StartsWith(TEXT("V32Support")))FrontlineCampaignV32::ToggleSupport(B,EFrontlineSupport(FCString::Atoi(*N.Mid(10))));
        else if(BoxName==TEXT("V63DeploymentMain")){S.DeploymentTabV63=0;return;}
        if(BoxName==TEXT("V63DeploymentArmour")){S.DeploymentTabV63=1;return;}
        if(BoxName==TEXT("V63DeploymentResearch")){S.Deployment=false;S.Technology=true;S.TechnologyPage=2;return;}
        if(BoxName.ToString().StartsWith(TEXT("V63ArmourSelect"))){FrontlineArmourV63::ToggleSelection(B,EFrontlineVehicle(FCString::Atoi(*BoxName.ToString().Mid(15))));return;}
        if(BoxName.ToString().StartsWith(TEXT("V8218ArmourVariant"))){FrontlineArmourV63::ToggleConfigurationV8218(B,EFrontlineVehicle(FCString::Atoi(*BoxName.ToString().Mid(18))));return;}
        if(BoxName.ToString().StartsWith(TEXT("V63Requisition"))){FrontlineArmourV63::BuyRequisition(B,FCString::Atoi(*BoxName.ToString().Mid(14)));return;}
        if(BoxName==TEXT("V32DeploymentBack"))S.Deployment=false;
        else if(BoxName==TEXT("V32DeploymentConfirm"))PC->BeginBattle();
        return;
    }
    if(BoxName==TEXT("V32Deployment")){FrontlineMapSceneV61::CaptureLoadingView(PC);S.Deployment=true;return;}
    if(FRRUWarRoomV82::Hit(BoxName,PC,B))return;
    if(WarRoom52::Command(BoxName,PC,B))return;
    if (S.UnitIntroduction)
    {
        if(BoxName==TEXT("UnitIntroductionBegin"))ConfirmUnitIntroductionV27(PC,B);
        else if(BoxName==TEXT("UnitIntroductionBack")){if(S.UnitIntroductionPage>0)--S.UnitIntroductionPage;else if(!DismissNationalAssignmentV87(PC,B))S.UnitIntroduction=false;}
        return;
    }
    if (S.Preparing) { if (BoxName==TEXT("PreparationBack")) { B->AbandonBattleToOperations();ClearPreparationWarmersV43(S);S.Preparing=false;S.PreparationError.Empty();S.Dossier=1917;PC->ResetCamera(); } return; }
    if(BoxName==TEXT("AchievementsV76")||BoxName==TEXT("AchievementsBackV76")){PC->ToggleAchievementsV76();return;}
    if (BoxName==TEXT("Performance")) { PC->TogglePerformanceMonitor(); return; }
    if (BoxName==TEXT("Settings") || BoxName==TEXT("DisplayBack")) { PC->ToggleSettings(); return; }
    if (BoxName==TEXT("Technology") || BoxName==TEXT("TechnologyBack")) { PC->ToggleTechnology(); return; }
    if(BoxName==TEXT("V51CampaignRecord")&&!FrontlineUltimateArchiveV8215::Active(B)&&(WarRoom52::Active(PC)||FRRUWarRoomV82::Active(PC))&&FrontlineCampaignV51::CanOpenEnding(B))
    {S.CampaignEndingV51=true;S.Menu=S.Help=S.Settings=S.Technology=S.AchievementsV76=false;return;}
    if(WarfareFRRUCampaignV82::Active(B)&&(BoxName.ToString().StartsWith(TEXT("FRRUMission:"))||BoxName==TEXT("FRRUDeploy")))
    {
        const FString Name=BoxName.ToString();
        if(Name.StartsWith(TEXT("FRRUMission:"))&&S.Dossier==1917)WarfareFRRUCampaignV82::SelectMission(B,Name.Mid(12));
        else if(Name.StartsWith(TEXT("FRRUOption:"))&&S.Technology)PurchaseNationalResearchV82(B,Name.Mid(11));
        else if(BoxName==TEXT("FRRUDeploy"))PC->BeginBattle();
        return;
    }
    if(BoxName.ToString().StartsWith(TEXT("Difficulty")) && S.Dossier==1918)
    {S.PendingDifficulty=EFrontlineDifficulty(FMath::Clamp(FCString::Atoi(*BoxName.ToString().Mid(10)),0,2));return;}
    if (S.Technology)
    {
        const FString Name=BoxName.ToString();
        if(WarfareFRRUCampaignV82::Active(B))
        {
            if(Name==TEXT("FRRUResearchOrdersV87")||Name==TEXT("FRRUResearchArmourV87"))
            {S.FRRUResearchPageV87=Name==TEXT("FRRUResearchArmourV87")?1:0;S.TechnologyFocus=INDEX_NONE;return;}
            if(Name.StartsWith(TEXT("FRRUOption:")))PurchaseNationalResearchV82(B,Name.Mid(11));
            else if(Name.StartsWith(TEXT("FRRUOptionFocus:"))){const auto Rows=WarfareFRRUCampaignV82::Options(B);const int32 Index=Rows.IndexOfByPredicate([&](const FFRRUTacticalOptionV82& Row){return Row.Id==Name.Mid(16);});if(Index!=INDEX_NONE)S.TechnologyFocus=Index;}
            return;
        }
        if(BoxName==TEXT("V32TechPrevious")){S.TechnologyPage=0;return;}
        if(BoxName==TEXT("V32TechNext")){S.TechnologyPage=1;return;}
        if(BoxName==TEXT("V63TechArmour")){S.TechnologyPage=2;return;}
        if (Name.StartsWith(TEXT("TechBuy")))
        {
            S.TechnologyFocus=FCString::Atoi(*Name.Mid(7));
            if(FrontlineUltimateArchiveV8215::Active(B)){FrontlineUltimateArchiveV8215::SetTechnologyEnabled(B,S.TechnologyFocus,!B->HasTechnology(S.TechnologyFocus));return;}
            if(B->PurchaseTechnology(S.TechnologyFocus))S.ResearchRevealsV72.Add(ResearchCardKeyV72(B,S.TechnologyFocus,false),FPlatformTime::Seconds());
        }
        else if (Name.StartsWith(TEXT("TechFocus"))) S.TechnologyFocus=FCString::Atoi(*Name.Mid(9));
        return;
    }
    if(FRRUWorldHitV82(BoxName,PC,B))return;
    if (BoxName==TEXT("AssetBack")) { PC->Restart(); S.Dossier=0; S.bAssetNoticeDismissed=true; return; }
    if (BoxName==TEXT("Credits")) { if(!S.Help) PC->ToggleHelp(); if(S.Help) S.Credits=true; return; }
    if (BoxName==TEXT("ManualPage")) { S.Credits=false; S.ManualTermsV829=false; S.ManualBattleV841=false; return; }
    if (BoxName==TEXT("ManualTermsV829")) { S.Credits=false; S.ManualTermsV829=true; S.ManualBattleV841=false; return; }
    if (BoxName==TEXT("ManualBattleV841")) {S.Credits=false;S.ManualTermsV829=false;S.ManualBattleV841=true;return;}
    if (BoxName.ToString().StartsWith(TEXT("ManualTopicV841"))) {S.ManualBattleTopicV841=FMath::Clamp(FCString::Atoi(*BoxName.ToString().Mid(15)),0,3);return;}
    if (BoxName==TEXT("Help")) { PC->ToggleHelp(); return; }
    if (BoxName==TEXT("Back")) { if (S.Help) PC->ToggleHelp(); else PC->ToggleMenu(); return; }
    if (BoxName==TEXT("Menu") || BoxName==TEXT("Resume")) { PC->ToggleMenu(); return; }
    if (BoxName==TEXT("Restart")) { PC->Restart(); return; }
    if (BoxName==TEXT("AbandonBattle") && S.Menu && B->Phase==EFrontlinePhase::Fighting)
    {
        PC->CancelTargeting();FinishVideoChange(S,false);
        B->AbandonBattleToOperations();
        S.Menu=S.Help=S.Settings=S.Technology=S.Preparing=S.UnitIntroduction=false;
        S.HistoricalDispatch=S.Deployment=false;S.bPausedByOverlay=S.bPausedForAuxOverlay=false;
        S.UnitIntroductionPage=0;S.AcceptedLoadoutMission=INDEX_NONE;S.PreparationError.Empty();
        S.Dispatches.Empty();S.LastDispatch.Empty();S.Dossier=1917;
        PC->SelectedTrench=INDEX_NONE;PC->ResetCamera();return;
    }
    if (HasEnded(B))
    {
        if (BoxName==TEXT("ReplayEncounter")) PC->Restart();
        else if (BoxName==TEXT("CampaignAfterBattle"))
        {
            if(!FrontlineAchievementsV76::FinalizeBattleKillsV8217(B))return;
            if(EndArchiveReplayV8215(PC,B,true))return;
            if(FrontlineCampaignV51::CanOpenEnding(B)){S.CampaignEndingV51=true;S.Menu=S.Help=false;FrontlineReportV839::OpenCampaignV86(PC,B);return;}
            const auto* FinishedMissionV87=B->GetEncounterCampaignMission();
            const int32 AssignmentFromV87=FinishedMissionV87&&WarfareFRRUCampaignV82::HasNewAssignmentReceiptV87(B,FinishedMissionV87->Id)?FinishedMissionV87->Id:INDEX_NONE;
            if(B->ContinueCampaign()){S.Dossier=1917;S.Menu=false;S.Help=false;PC->ResetCamera();ReleaseNationalAssignmentAfterReportV87(PC,B,AssignmentFromV87);FrontlineNewUnlockV8215::ReleaseAfterReportV8217(B);}
        }
        return;
    }
    if (B->Phase==EFrontlinePhase::Briefing)
    {
        const FString Name=BoxName.ToString();
        if (BoxName==TEXT("QuitGame") && S.Dossier==0 && !S.Help)
        {S.ExitConfirmV61=true;return;}
        if (BoxName==TEXT("NewCampaign") || BoxName==TEXT("Dossier1917")) { S.FactionSelectionV82=FFrontlineFactionSelectionV82();S.Dossier=1918; S.bAssetNoticeDismissed=false; }
        else if (BoxName==TEXT("ContinueCampaign"))
        {
            const auto* Recent=MostRecentNationalCampaignV827(B);
            if(Recent?WarfareFRRUCampaignV82::Continue(B,Recent->Id):B->ContinueCampaign()){S.Dossier=1917;PC->ResetCamera();}
        }
        else if (BoxName==TEXT("CampaignArchives")) {S.Dossier=1919;S.ArchiveOffset=0;S.ArchiveSelection.Empty();S.ArchiveNotice.Empty();}
        else if(BoxName==TEXT("FRRUArchiveTabV82"))
        {S.FRRUArchivesV82=!S.FRRUArchivesV82;S.ArchiveOffset=0;S.ArchiveSelection.Empty();S.ArchiveNotice.Empty();}
        else if(S.FRRUArchivesV82&&S.Dossier==1919&&Name.StartsWith(TEXT("FRRUResume:")))
        {FGuid Id;if(FGuid::ParseExact(Name.Mid(11),EGuidFormats::Digits,Id)&&WarfareFRRUCampaignV82::Continue(B,Id)){S.Dossier=1917;PC->ResetCamera();}}
        else if(S.FRRUArchivesV82&&S.Dossier==1919&&Name.StartsWith(TEXT("FRRUSelect:")))
        {FGuid Id;if(FGuid::ParseExact(Name.Mid(11),EGuidFormats::Digits,Id)&&WarfareFRRUCampaignV82::Archives(B).ContainsByPredicate([&](const FFRRUCampaignSummaryV82& Row){return Row.Id==Id;})){if(S.ArchiveSelection.Contains(Id))S.ArchiveSelection.Remove(Id);else S.ArchiveSelection.Add(Id);}}
        else if(S.FRRUArchivesV82&&S.Dossier==1919&&Name.StartsWith(TEXT("FRRUDelete:")))
        {FGuid Id;if(FGuid::ParseExact(Name.Mid(11),EGuidFormats::Digits,Id)&&WarfareFRRUCampaignV82::Archives(B).ContainsByPredicate([&](const FFRRUCampaignSummaryV82& Row){return Row.Id==Id;}))S.ArchivePendingDelete={Id};}
        else if(S.FRRUArchivesV82&&S.Dossier==1919&&BoxName==TEXT("ArchiveSelectAll"))
        {const auto& Rows=WarfareFRRUCampaignV82::Archives(B);if(S.ArchiveSelection.Num()==Rows.Num())S.ArchiveSelection.Empty();else for(const auto& Row:Rows)S.ArchiveSelection.Add(Row.Id);}
        else if(S.Dossier==1919 && Name.StartsWith(TEXT("ArchiveSelect")) && BoxName!=TEXT("ArchiveSelectAll"))
        {
            FGuid Id;if(FGuid::ParseExact(Name.Mid(13),EGuidFormats::Digits,Id) && B->GetCampaignRuns().ContainsByPredicate([&](const FFrontlineCampaignProgress& R){return R.Id==Id;}))
            {if(S.ArchiveSelection.Contains(Id))S.ArchiveSelection.Remove(Id);else S.ArchiveSelection.Add(Id);}
        }
        else if(S.Dossier==1919 && BoxName==TEXT("ArchiveSelectAll"))
        {if(S.ArchiveSelection.Num()==B->GetCampaignRuns().Num())S.ArchiveSelection.Empty();else for(const auto& R:B->GetCampaignRuns())S.ArchiveSelection.Add(R.Id);}
        else if(S.Dossier==1919 && BoxName==TEXT("ArchiveDeleteSelected"))S.ArchivePendingDelete=S.ArchiveSelection.Array();
        else if(S.Dossier==1919 && Name.StartsWith(TEXT("ArchiveDeleteOne")))
        {FGuid Id;if(FGuid::ParseExact(Name.Mid(16),EGuidFormats::Digits,Id)){S.ArchivePendingDelete.Empty();S.ArchivePendingDelete.Add(Id);}}
        else if (BoxName==TEXT("ArchiveNewer")) S.ArchiveOffset=FMath::Max(0,S.ArchiveOffset-5);
        else if (BoxName==TEXT("ArchiveOlder")) S.ArchiveOffset+=5;
        else if (Name.StartsWith(TEXT("CampaignResume")))
        {const int32 I=FCString::Atoi(*Name.Mid(14));if(B->GetCampaignRuns().IsValidIndex(I) && B->ContinueCampaign(B->GetCampaignRuns()[I].Id)) {S.Dossier=1917;PC->ResetCamera();}}
        else if (Name.StartsWith(TEXT("CampaignMission"))) B->SelectCampaignMission(FCString::Atoi(*Name.Mid(15)));
        else if (BoxName==TEXT("Dossier1944")) { S.Dossier=1944; S.bAssetNoticeDismissed=false; }
        else if (BoxName==TEXT("MenuHome")) { S.Dossier=0; S.Help=false; }
        else if (BoxName==TEXT("Begin")) PC->BeginBattle();
        return;
    }
    if (B->Phase!=EFrontlinePhase::Fighting || S.Menu || S.Help || !B->AssetFailure.IsEmpty()) return;
    const FString Order=BoxName.ToString();
    if (Order.StartsWith(TEXT("TrenchFocus")) || Order.StartsWith(TEXT("TrenchEntry")))
    {
        const int32 Index=FCString::Atoi(*Order.Mid(11));
        if (B->Trenches.IsValidIndex(Index) && B->Trenches[Index].Owner==0)
        {
            PC->SelectedTrench=Index;
            if (Order.StartsWith(TEXT("TrenchEntry"))) B->SetTrenchBypass(Index,!B->Trenches[Index].bBypass);
        }
        return;
    }
    if (Order.StartsWith(TEXT("TrenchAdvance")))
    {const int32 I=FCString::Atoi(*Order.Mid(13));PC->SelectTrench(I);B->AdvanceTrench(I);return;}
    if (Order.StartsWith(TEXT("SectionAdvance")) || Order.StartsWith(TEXT("SectionHold")))
    {
        const bool Hold=Order.StartsWith(TEXT("SectionHold"));FString TrenchText,SquadText;
        if (Order.Mid(Hold?11:14).Split(TEXT("_"),&TrenchText,&SquadText))
        {
            const int32 I=FCString::Atoi(*TrenchText),Id=FCString::Atoi(*SquadText);const auto* Squad=B->FindSquad(Id);
            if (Squad) {PC->SelectedTrench=I;if(Hold) B->SetSquadHold(I,Id,!Squad->bHoldPosition);else B->AdvanceSquad(I,Id);}
        }return;
    }
    else if (BoxName==TEXT("Rifle")) PC->RecruitRifle();
    else if (BoxName==TEXT("Lewis")) PC->RecruitLewis();
    else if (BoxName==TEXT("Mills")) PC->RecruitMills();
    else if (BoxName==TEXT("Marksman")) PC->RecruitMarksman();
    else if (BoxName==TEXT("Officer")) PC->RecruitOfficer();
    else if (BoxName==TEXT("Artillery")) PC->ToggleArtillery();
    else if (BoxName==TEXT("Smoke")) PC->ToggleSmoke();
    else if (BoxName==TEXT("Mortar")) PC->ToggleMortar();
    else if (BoxName==TEXT("Gas")) SelectSupportCard(PC,B,EFrontlineSupport::Gas);
    else if (BoxName==TEXT("AntiTank")) SelectSupportCard(PC,B,EFrontlineSupport::AntiTank);
    else if (BoxName==TEXT("AirStrike")) SelectSupportCard(PC,B,EFrontlineSupport::AirStrike);
    else if (Order.StartsWith(TEXT("RecruitRole")) && Order.Mid(11).IsNumeric()) B->Recruit(EFrontlineRole(FCString::Atoi(*Order.Mid(11))));
    else if (Order.StartsWith(TEXT("SupportKind")) && Order.Mid(11).IsNumeric()) SelectSupportCard(PC,B,EFrontlineSupport(FCString::Atoi(*Order.Mid(11))));
    else if (Order.StartsWith(TEXT("Vehicle")) && Order.Mid(7).IsNumeric())
    {if(auto* V=UFrontlineVehicleSubsystem::Get(GetWorld()))V->Recruit(EFrontlineVehicle(FCString::Atoi(*Order.Mid(7))));}
    else if (BoxName==TEXT("Rally")) PC->ToggleRally();
    else if (BoxName==TEXT("Pause")) PC->TogglePause();
    else if (BoxName==TEXT("ResetCamera")) PC->ResetCamera();
    else if (Order.StartsWith(TEXT("BattleRate")) && Order.Mid(10).IsNumeric())
    {const int32 I=FCString::Atoi(*Order.Mid(10));if(I>=0&&I<3&&B->Phase==EFrontlinePhase::Fighting){const float Values[]={.5f,1.f,1.5f};B->SetBattleSpeed(Values[I]);}}
    else if (Order.StartsWith(TEXT("Trench")) && Order.Len()>6 && Order.Mid(6).IsNumeric()) PC->SelectTrench(FCString::Atoi(*Order.Mid(6)));
}

namespace
{
FString ChineseStatus(const FString& Source)
{
    static const TMap<FString,FString> Lines={
        {TEXT("Begin the battle first."),TEXT("等待开始行动")},
        {TEXT("Resume to issue orders."),TEXT("继续战斗后下令")},
        {TEXT("Eight sections are already active."),TEXT("已达 8 小队上限")},
        {TEXT("This section is still preparing."),TEXT("小队正在整备")},
        {TEXT("Select a friendly trench."),TEXT("请选择友方战壕")},
        {TEXT("This trench has no sections to release."),TEXT("壕内没有可出击的小队")},
        {TEXT("The battery is reloading."),TEXT("炮兵正在装填")},
        {TEXT("All sections in the selected trench are advancing."),TEXT("所选战壕的全部小队正在前进。")},
        {TEXT("Hold your line. The enemy is moving for the central trench."),TEXT("守住战线，敌军正在向中央战壕推进。")},
        {TEXT("A trench has fallen. Your line is under pressure."),TEXT("一处战壕失守，我军防线承压。")},
        {TEXT("Reinforce the line. Release a trench to advance."),TEXT("增援已就位。选中友壕，准备下一次前进。")},
        {TEXT("Battery firing. Clear the marked area; shells can hit friendly troops."),TEXT("炮兵已开火。立即离开标记区，炮弹会伤及友军。")}
    };
    if (const FString* Found=Lines.Find(Source)) return *Found;
    return Source;
}

void DrawMenuArtwork(const FFrontlineCanvas& UI,bool Is1944)
{
    UI.Rect(0,0,1920,1080,FLinearColor(.012f,.022f,.035f,1));
    const auto* Battle=Cast<AWarfareFrontlineGameMode>(UI.HUD.GetWorld()->GetAuthGameMode());
    const bool German=Battle && Battle->GetPlayerFaction()==EFrontlineFaction::German;
    auto& MenuArt=Artwork();UTexture2D* Backdrop=German&&MenuArt.GermanBackgroundV30?MenuArt.GermanBackgroundV30.Get():MenuArt.Keyart[Is1944?1:0].Get();
    if(!Is1944&&MenuArt.BackgroundsV71[German?1:0].Texture)Backdrop=MenuArt.BackgroundsV71[German?1:0].Texture.Get();
    UI.Image(Backdrop,0,0,1920,1080);
    // Shared edges and interpolated vertex alpha avoid overlapping translucent strips.
    for (int32 I=0;I<32;++I)
    {
        const float A=.83f*FMath::Pow(1-I/32.f,1.8f);
        const float B=.83f*FMath::Pow(1-(I+1)/32.f,1.8f);
        UI.Gradient(I*60.f,0,60,1080,FLinearColor(.005f,.012f,.022f,A),FLinearColor(.005f,.012f,.022f,B));
    }
    UI.Gradient(0,680,1920,400,FLinearColor(.005f,.012f,.022f,0),FLinearColor(.005f,.012f,.022f,.82f),true);
    DrawBrandTitleV72(UI,80,34,218,73,.65f);
    const auto* Selected=Battle?Battle->GetSelectedCampaignMission():nullptr;
    UI.Right(FString::FromInt(Is1944?1944:Selected?Selected->Year:1917),1835,47,17,Muted);
}

#include "WarfareFrontlineTermsV829.generated.inl"
#include "WarfareFrontlineManualUIV841.inl"
void DrawFieldManual(const FFrontlineCanvas& UI)
{
    const bool Credits=ScreenState(UI.HUD.GetOwningPlayerController()).Credits;
    const bool Terms=!Credits&&ScreenState(UI.HUD.GetOwningPlayerController()).ManualTermsV829;
    const bool Fieldcraft=!Credits&&ScreenState(UI.HUD.GetOwningPlayerController()).ManualBattleV841;
    UI.PageSurfaceV73(0,0,1920,1080,false,!Credits);
    if(Credits){UI.PhysicalPanelV74(80,252,1152,676);UI.PhysicalPanelV74(1272,252,552,676,true);}
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    UI.Label(Credits?WarRoom52::L(TEXT("CREDITS"),TEXT("制作与鸣谢")):WarRoom52::L(TEXT("FIELD MANUAL"),TEXT("战地手册")),72,64,1180,65,40,Paper);
    DrawBrandTitleV72(UI,1372,66,436,86);
    UI.PageTabV73(72,PageCanvasV73::TabY,364,WarRoom52::L(TEXT("OPERATIONS"),TEXT("操作")),TEXT("ManualPage"),!Credits&&!Terms&&!Fieldcraft);
    UI.PageTabV73(456,PageCanvasV73::TabY,364,WarRoom52::L(TEXT("FIELDCRAFT"),TEXT("战场")),TEXT("ManualBattleV841"),Fieldcraft);
    UI.PageTabV73(840,PageCanvasV73::TabY,364,TEXT("术语"),TEXT("ManualTermsV829"),Terms);
    UI.PageTabV73(1224,PageCanvasV73::TabY,364,WarRoom52::L(TEXT("CREDITS"),TEXT("制作与鸣谢")),TEXT("Credits"),Credits);
    UI.PageButtonV73(72,958,280,58,TEXT("返回"),TEXT("Esc"),TEXT("Back"));
    if(Credits)
    {
        UI.Label(TEXT("Unreal Engine 5"),112,282,920,46,29,Paper);
        UI.Paragraph(TEXT("Warfare Front uses Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere."),112,352,1076,108,20,Muted);
        UI.Paragraph(TEXT("Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved."),112,486,1076,66,19,Muted);
        UI.PageRuleV74(112,582,1076);
        UI.Label(TEXT("NVIDIA DLSS / Reflex"),112,610,916,46,29,Paper);
        UI.ImageContain(Artwork().NVIDIAOfficial,1118,605,68,68);
        UI.Paragraph(TEXT("NVIDIA, the NVIDIA logo, GeForce and RTX are trademarks and/or registered trademarks of NVIDIA Corporation in the U.S. and other countries."),112,682,1076,121,20,Muted);
        UI.Label(TEXT("独立历史题材作品。非 Armor Games 官方产品，亦未获其背书。"),112,842,1076,44,19,Muted);
        UI.Label(PageCanvasV73::L(TEXT("Art, motion and type"),TEXT("美术、动作与字体")),1296,282,512,46,26,Paper);
        UI.Paragraph(TEXT("Blender Studio · Poly Haven · Quaternius · CMU Motion Capture · RDW MIC / Marcin Solarz · Noto · Vernon Adams · VSCO Community Edition"),1296,352,512,194,20,Paper);
        UI.Label(PageCanvasV73::L(TEXT("Sound and licenses"),TEXT("声音与许可")),1296,594,512,46,26,Paper);
        UI.Paragraph(TEXT("录音与声音设计素材：kurt、Michel Baradari、Manuel Senfft、Kenney、HaelDB 及各位贡献者。完整署名、素材改编说明与各组件许可见随游戏提供的 THIRD-PARTY-NOTICES.md 与 Licenses 文件夹。"),1296,664,512,230,19,Muted);
        return;
    }
    if(Fieldcraft&&DrawManualFieldcraftV841(UI))return;
    if(Terms)
    {
        for(int32 I=0;I<UE_ARRAY_COUNT(TermsHeadingsV829);++I)
        {
            const float X=112+(I%3)*578,Y=292+(I/3)*214;
            UI.PageRuleV74(X,Y,536);
            // The readiness heading shares a legacy Chinese key with Upgrades.
            // Select the authored language before resolving, without reordering old aliases.
            UI.Label(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::English?TermsHeadingsEnglishV829[I]:TermsHeadingsV829[I],X,Y+18,536,38,26,Amber);
            UI.Paragraph(TermsCopyV829[I],X,Y+66,536,128,21,Paper);
        }
        return;
    }
    UI.Label(WarRoom52::L(TEXT("STANDING ORDERS"),TEXT("作战须知")),112,282,1696,50,30,Paper);
    const TCHAR* Headings[]={TEXT("01  补员与整备"),TEXT("02  战壕与前进"),TEXT("03  炮火支援"),TEXT("04  士气与胜负")};
    const TCHAR* Copy[]={
        TEXT("点击兵种肖像征募小队。步兵共用整备队列。军士提高附近友军命中率，停步时缓解压制。"),
        TEXT("每壕容纳3支小队。点击驻军头像令该队出击，或下令整壕前进。关闭入壕时，后续部队从旁通过，驻军留守；满员友壕也可通过。敌壕须先清除守军。"),
        TEXT("选择支援，再标记目标地域；炮火经过传令后抵达。野炮、烟幕与迫击炮分别整备。炮弹、毒气和扫射也会伤及友军；烟幕遮挡双方观察，无法挡住炮弹。"),
        TEXT("清除敌方末端防区，让一支小队的全部生还者或整辆战车穿过后方交通线，即可获胜。击溃敌军士气也能取胜。己方炮火、毒气与近距离装甲会动摇士气；占壕、推进和相互支援可恢复少量士气。")
    };
    for(int32 I=0;I<4;++I)
    {
        const float X=112+(I%2)*880,Y=366+(I/2)*274;
        UI.PageRuleV74(X,Y,816);
        UI.Label(Headings[I],X,Y+20,816,40,26,Amber);
        UI.Paragraph(Copy[I],X,Y+82,816,174,21,Paper);
        const TCHAR* Keys[]={TEXT("1–5"),TEXT("Q"),TEXT("E / C / V · 鼠标右键取消"),TEXT("A / D 水平 · W / S 纵深 · 滚轮缩放")};
        UI.Shortcut(X,Y,816,256,Keys[I],50);
    }
    UI.Label(PageCanvasV73::L(TEXT("Space · pause    H · manual    Esc · back"),TEXT("Space · 暂停    H · 手册    Esc · 返回")),440,958,1368,58,19,Muted,2);
    UI.Shortcut(440,958,1368,58,TEXT("F 返回后方 · Space 暂停 · H 手册"),50);
}
#include "WarfareFrontlineTrenchCommandV17.inl"
#include "WarfareFrontlineFieldHoverV8218.inl"
#include "WarfareFrontlineMedalUIReviewV8218.inl"
#include "WarfareFrontlineReportV839.inl"
#include "WarfareFrontlineBriefingUI.inl"
#include "WarfareFrontlineCampaignEndingV51.inl"
#include "WarfareFrontlineUIV28Review.inl"
}

#include "WarfareFrontlineCampaignReportReviewV86.inl"

namespace { namespace TextReviewV829 {
static bool Draw(const FFrontlineCanvas&,AWarfareFrontlineGameMode*,FFrontlineScreenState&);
} }

void AWarfareFrontlineHUD::DrawHUD()
{
    // Review cameras and other legitimate callers may freeze the controller.
    // The painted page still owns the cover lifecycle, including restoration
    // when leaving an opaque menu for the live battlefield.
    if(auto* CoverPC=Cast<AWarfareFrontlineController>(PlayerOwner))
    {
        // GameViewportClient submits its main view before calling HUD PostRender.
        // Observe that old world-render flag before Update can change it.
        FactionSelectionV82::CoveredWorldV826().NotifySubmittedWorldV834(CoverPC,Canvas);
        FactionSelectionV82::CoveredWorldV826().Update(CoverPC,FrontlineFor(this));
    }
    V39LastStatus={}; // A covered or hidden HUD must not retain a previous panel.
    Super::DrawHUD(); if (!Canvas || !PlayerOwner) return;
    // AHUD's ordinary Canvas is drawn into the already encoded scene viewport.
    // DebugCanvas is submitted as a normal Slate custom element: in HDR it is
    // rendered to the separate SDR UI texture, then mapped to the selected UI
    // luminance/gamut once by CompositeUI. Keep the accepted SDR path untouched.
    UGameViewportClient* ViewportClient=GetWorld()?GetWorld()->GetGameViewport():nullptr;
    // UE 5.8's window-DPI cache invalidation is Editor-only. Shipping can cache
    // 1 before the native window exists, or retain another monitor's scale.
    // Refresh from the engine's real native window; the already-created Canvas
    // keeps its own DPI for this frame and the engine applies the update next
    // frame. Never substitute desktop DPI for viewport/layout coordinates.
    V8212NativeWindowDPI=V8212ViewportCachedDPI=0.f;
    if(ViewportClient)
    {
        const TSharedPtr<SWindow> Window=ViewportClient->GetWindow();
        if(Window.IsValid()&&Window->GetNativeWindow().IsValid())
        {
            const float WindowDPI=Window->GetNativeWindow()->GetDPIScaleFactor();
            if(FMath::IsFinite(WindowDPI)&&WindowDPI>0.f)
            {
                V8212NativeWindowDPI=WindowDPI;
                if(!FMath::IsNearlyEqual(ViewportClient->GetDPIScale(),WindowDPI,.0001f))
                {
                    ViewportClient->RequestUpdateDPIScale();
                    ++V8212ViewportDPIRefreshes;
                }
                V8212ViewportCachedDPI=ViewportClient->GetDPIScale();
            }
        }
    }
    const bool bHDRScene=ViewportClient && ViewportClient->Viewport && ViewportClient->Viewport->GetSceneHDREnabled();
    static const auto* CompositeMode=IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.UI.CompositeMode"));
    const bool bHDRComposite=bHDRScene && GRHISupportsHDROutput && IsHDREnabled() &&
        CompositeMode && CompositeMode->GetValueOnGameThread()!=0;
    const UCanvas* SceneHUDCanvas=Canvas;
    const float SceneHUDDPI=Canvas->Canvas?Canvas->Canvas->GetDPIScale():1.f;
    const bool bCanvasReady=DebugCanvas && DebugCanvas->Canvas && Canvas->Canvas &&
        DebugCanvas->SizeX==Canvas->SizeX && DebugCanvas->SizeY==Canvas->SizeY &&
        DebugCanvas->SizeX>0 && DebugCanvas->SizeY>0;
    // A resize can expose the old debug canvas for one frame. Wait for matching
    // geometry instead of writing SDR values into PQ or accepting shifted hits.
    if(bHDRScene && (!bHDRComposite || !bCanvasReady))return;
    TGuardValue<TObjectPtr<UCanvas>> RestoreCanvas(Canvas,bHDRComposite?DebugCanvas:Canvas);
    TGuardValue<float> RestoreClipX(Canvas->ClipX,SceneHUDCanvas->ClipX);
    TGuardValue<float> RestoreClipY(Canvas->ClipY,SceneHUDCanvas->ClipY);
    TGuardValue<float> RestoreOrgX(Canvas->OrgX,SceneHUDCanvas->OrgX);
    TGuardValue<float> RestoreOrgY(Canvas->OrgY,SceneHUDCanvas->OrgY);
    // Resizing/minimizing may briefly leave a zero viewport. Do not construct
    // a zero layout scale or submit non-finite positions to the canvas.
    if(!Canvas->Canvas||!FMath::IsFinite(Canvas->ClipX)||!FMath::IsFinite(Canvas->ClipY)
        ||Canvas->ClipX<=0||Canvas->ClipY<=0)return;
    if(bHDRComposite)
    {
        // DebugCanvas includes desktop DPI scaling; our HUD coordinates and hit
        // regions already use viewport pixels. Preserve the original scale.
        const float ScaleRatio=SceneHUDCanvas->Canvas->GetDPIScale()/FMath::Max(.01f,Canvas->Canvas->GetDPIScale());
        Canvas->Canvas->PushAbsoluteTransform(FScaleMatrix(ScaleRatio));
    }
    // Disposable review only: exercise the actual Canvas glyph loop at desktop
    // DPI values without changing Windows settings, player files or UI geometry.
    TUniquePtr<FCanvas> DPIReviewCanvas;
    FCanvas* OriginalDrawCanvas=Canvas->Canvas;
    float ReviewDPI=0.f;
    if(!bHDRComposite&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&
        FParse::Param(FCommandLine::Get(),TEXT("WFV825FontLayout"))&&
        FParse::Value(FCommandLine::Get(),TEXT("WFCanvasDPIReview="),ReviewDPI)&&
        FMath::IsFinite(ReviewDPI)&&ReviewDPI>=1.f&&ReviewDPI<=3.f)
    {
        DPIReviewCanvas=MakeUnique<FCanvas>(OriginalDrawCanvas->GetRenderTarget(),nullptr,GetWorld(),
            OriginalDrawCanvas->GetFeatureLevel(),FCanvas::CDM_DeferDrawing,ReviewDPI);
        DPIReviewCanvas->SetBaseTransform(OriginalDrawCanvas->GetTransformStack().Top().GetMatrix());
        Canvas->Canvas=DPIReviewCanvas.Get();
    }
    ON_SCOPE_EXIT
    {
        FrontlineDebugV37::Draw(this,Canvas);
        FrontlineHDRReviewV25::DrawTiles(Canvas);
        if(DPIReviewCanvas){DPIReviewCanvas->Flush_GameThread();Canvas->Canvas=OriginalDrawCanvas;}
        if(bHDRComposite)Canvas->Canvas->PopTransform();
    };
    static int32 LastHDRRoute=INDEX_NONE;
    const int32 HDRRoute=bHDRComposite?1:0;
    if(LastHDRRoute!=HDRRoute)
    {
        LastHDRRoute=HDRRoute;
        UE_LOG(LogTemp,Display,TEXT("WF_HDR_HUD_ROUTE slate_sdr_composite=%d size=%dx%d clip=%.0fx%.0f canvas_dpi=%.3f source_dpi=%.3f material_coverage_alpha=%d"),
            HDRRoute,Canvas->SizeX,Canvas->SizeY,Canvas->ClipX,Canvas->ClipY,Canvas->Canvas->GetDPIScale(),SceneHUDDPI,HDRRoute);
    }
    AWarfareFrontlineGameMode* B=FrontlineFor(this);
    AWarfareFrontlineController* PC=Cast<AWarfareFrontlineController>(PlayerOwner); if (!B || !PC) return;
    FFrontlineScreenState& State=ScreenState(PC);
    const bool StoryModalV82=FRRUStoryModalV82(B);
    const bool Modal=GetPresentationPage()!=3 || !B->AssetFailure.IsEmpty()||StoryModalV82;
    const FFrontlineCanvas UI(*this,Canvas,Modal,bHDRComposite);
    UI.bLegacyTypographyV8210=GetPresentationPage()==3;
    if(UIMotionReviewV83::Active&&bHDRComposite)++UIMotionReviewV83::HDRCompositeFrames;
    static const bool StableMotion=FParse::Param(FCommandLine::Get(),TEXT("WFUIMotionStableV83"));
    UI.MotionStableV83=StableMotion&&B->IsProgressionVolatile();
    UI.StartTooltipsV83(!Modal);
    ProbeFrontlineCanvasV8218Review(UI,B,PC,DockTop);
    ON_SCOPE_EXIT{ProbeFrontlineCanvasV8219Review(UI,B);};
    if(TextReviewV829::Draw(UI,B,State))return;
    const int32 PageV35=GetPresentationPage();const double NowV35=FPlatformTime::Seconds();
    if(State.PreviousPageV35==INDEX_NONE)State.PreviousPageV35=PageV35;
    else if(State.PreviousPageV35!=PageV35)
    {
        State.PhotoTransitionV37=PageV35!=11&&State.PreviousPageV35!=11&&FrontlinePresentationV37::UsePhotoTransition(State.PreviousPageV35,PageV35,NowV35-State.LastPhotoAtV37);
        State.SmallTransitionV35=!State.PhotoTransitionV37;
        State.TransitionPhotoV37.Reset();State.PhotoClockV37={};State.TransitionPhotoPresentedV37=false;
        if(State.PhotoTransitionV37)
        {
            const auto* M=PageV35==6?B->GetEncounterCampaignMission():B->GetSelectedCampaignMission();
            if(!M)M=B->GetSelectedCampaignMission();
            const int32 Photo=PageV35==6?3:M&&M->LegacyArtV41==3?2:B->GetPlayerFaction()==EFrontlineFaction::German?1:0;
            UTexture2D* Image=Artwork().ArchivePhotosV35[Photo];
            const int32 NewPhoto=FrontlineCampaignV51::PhotoForOperation(M?M->Id:INDEX_NONE);
            if(PageV35==18)Image=FrontlineCampaignV51::Photo(3);
            else if(NewPhoto>=0&&PageV35!=6)if(auto* Themed=FrontlineCampaignV51::Photo(NewPhoto))Image=Themed;
            State.TransitionPhotoV37=Image;
        }
        // Resolve the asset once, before starting the clock. A later mission
        // selection must not swap the photograph halfway through its fade.
        State.PreviousPageV35=PageV35;State.PageChangedAtV35=FPlatformTime::Seconds();
    }
    // Start counters only after the photograph has finished being presented.
    // Auxiliary pages preserve a report's encounter clock.
    if(State.ReportEncounterV83!=B->GetEncounterId())
    {State.ReportEncounterV83=B->GetEncounterId();State.ReportVisibleAtV83=-1;}
    if(PageV35!=18&&PageV35!=5&&PageV35!=7){State.EndingEnteredV83=false;State.EndingVisibleAtV83=-1;}
    if(PageV35==18&&!State.EndingEnteredV83){State.EndingEnteredV83=true;State.EndingVisibleAtV83=-1;}
    const bool NumbersVisibleV83=!State.PhotoTransitionV37||(State.TransitionPhotoPresentedV37&&State.PhotoClockV37.VisibleSeconds>=.55f);
    if(PageV35==6){if(NumbersVisibleV83&&State.ReportVisibleAtV83<0)State.ReportVisibleAtV83=NowV35;UI.MotionNumberAgeV83=State.ReportVisibleAtV83<0?0:NowV35-State.ReportVisibleAtV83;}
    if(PageV35==18){if(NumbersVisibleV83&&State.EndingVisibleAtV83<0)State.EndingVisibleAtV83=NowV35;UI.MotionNumberAgeV83=State.EndingVisibleAtV83<0?0:NowV35-State.EndingVisibleAtV83;}
    if(B->IsProgressionVolatile()&&UIMotionReviewV83::NumberAge>=0)UI.MotionNumberAgeV83=UIMotionReviewV83::NumberAge;
    ON_SCOPE_EXIT
    {
        const double DrawNow=FPlatformTime::Seconds();
        const float Age=float(DrawNow-State.PageChangedAtV35);
        const bool SelectorCommitV844=FactionSelectionV82::SuppressPageVeilV844(DrawNow);
        if(State.PhotoTransitionV37&&!SelectorCommitV844)
        {
            UTexture2D* Image=State.TransitionPhotoV37.Get();
            const bool Ready=FrontlineTextureReady(Image);
            // An absent photo leaves the ready page usable. A pending texture
            // gets a short bounded chance to become drawable, without consuming
            // the photo cadence or blocking input behind an empty plate.
            if(!State.TransitionPhotoPresentedV37&&(!Image||Age>2.f))State.PhotoTransitionV37=false;
            else if(State.PhotoClockV37.Advance(DrawNow,Ready))
            {
                if(!State.TransitionPhotoPresentedV37)
                {
                    State.TransitionPhotoPresentedV37=true;State.LastPhotoAtV37=DrawNow;
                    UE_LOG(LogTemp,Display,TEXT("WF_PHOTO_TRANSITION page=%d image=%s"),PageV35,*Image->GetPathName());
                }
                UI.HoverShortcut.Empty();
                const float T=State.PhotoClockV37.VisibleSeconds/.55f;
                UI.ImageCover(Image,0,0,1920,1080,FLinearColor(.62f,.62f,.59f,1-FMath::SmoothStep(.1f,1.f,T)));
                if(T<.45f)UI.Hit(0,0,1920,1080,TEXT("Blocked"),1000);
            }
            else if(Ready)State.PhotoTransitionV37=false;
        }
        else if(State.SmallTransitionV35&&!SelectorCommitV844&&Age>=0&&Age<.12f)
        {
            UI.HoverShortcut.Empty();
            UI.Rect(0,0,1920,1080,FLinearColor(.004f,.006f,.006f,.10f*(1-Age/.12f)));
        }
        if(!ArmourKillTallyUIV8220::Banner(UI,PC,State))DrawAchievementBannerV81(UI,State);
        if(FParse::Param(FCommandLine::Get(),TEXT("WFV35LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV36LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV37LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV38LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV39LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV40LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV41LocalizationAudit"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45LocalizationAudit")))
        {static double LastAudit=-100.;if(NowV35-LastAudit>1.){FrontlineLocalizationV35::WriteCoverageReport();LastAudit=NowV35;}}
    };
    if(TypographyReviewPageV8210==29){UI.bLegacyTypographyV8210=false;UI.MotionStableV83=true;DrawFieldReport(UI,B);return;}
    if(TypographyReviewPageV8210==30){UI.bLegacyTypographyV8210=false;FrontlineLoadingPageV82::Draw(UI,B,State);return;}
    if(TypographyReviewPageV8210==31){UI.bLegacyTypographyV8210=false;UI.MotionStableV83=true;DrawCampaignEndingV51(UI,B);return;}
    if(V38LockCoverageReview){DrawLockCoverageReviewV38(UI);return;}
    if(V32FlagSheet)
    {
        UI.Rect(0,0,1920,1080,FLinearColor(.015f,.018f,.022f,1));
        for(int32 F=0;F<2;++F)for(int32 I=0;I<4;++I)
        {
            const float Loss[]={15,45,90,180};const float X=52+I*469,Y=110+F*444;
            UI.Flag(Artwork().Flags[F],X,Y,399,264,FVector4f(Loss[I],Loss[I]+8,Loss[I]-3,V32FlagAge),1.f,19.f+F*137.f);
            UI.Flag(Artwork().Flags[F],X,Y+291,74,49,FVector4f(Loss[I],Loss[I]+8,Loss[I]-3,V32FlagAge),1.f,19.f+F*137.f);
        }
        return;
    }
    if(V24FlagReviewStage==0){DrawV24FlagContactSheet(UI);return;}
    if(V26UIContactSheet){DrawV26UIContactSheet(UI);return;}
    if(V27UIContactSheet){DrawV27UIContactSheet(UI);return;}
    if(V28UIContactSheet){DrawV28UIContactSheet(UI);return;}
    const bool Paused=UGameplayStatics::IsGamePaused(this);
    if(FrontlineReportV839::Active(PC,B)&&FrontlineReportV839::View(PC).CampaignV86){ReportUIV839::Draw(UI,B);return;}
    if(State.CampaignEndingV51){DrawCampaignEndingV51(UI,B);DrawPerformance(UI,State);return;}
    if(State.Preparing){DrawOperationPreparation(UI,B,State);DrawPerformance(UI,State);return;}
    if(UltimateArchiveUIV8215::Active(PC)){UltimateArchiveUIV8215::Draw(UI,B);return;}
    if(FrontlineNewUnlockV8215::Pending(B)){NewUnlockUIV8215::Draw(UI,B);return;}
    if(State.AchievementsV76)
    {
        if(B&&B->Phase==EFrontlinePhase::Fighting)
        {if(!State.Menu&&!UGameplayStatics::IsGamePaused(PC))State.bPausedForAuxOverlay=true;B->SetPaused(true);}
        DrawAchievementsV76(UI);return;
    }
    if (State.Settings) { DrawDisplaySettings(UI,State); DrawPerformance(UI,State); return; }
    if(State.HistoricalDispatch){DrawHistoricalDispatchV32(UI,B);return;}
    if(State.Deployment){DrawDeploymentV32(UI,B);return;}
    if (State.UnitIntroduction) {DrawUnitIntroductionV27(UI,B,State);return;}
    if (State.Preparing) { DrawOperationPreparation(UI,B,State); DrawPerformance(UI,State); return; }
    if (State.Technology) { DrawTechnology(UI,B); DrawPerformance(UI,State); return; }
    if (!B->AssetFailure.IsEmpty() && !State.bAssetNoticeDismissed)
    {
        DrawMenuArtwork(UI,false);
        UI.Rect(0,0,1920,1080,FLinearColor(.005f,.01f,.018f,.72f));
        UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
        UI.Box(470,315,980,440,Panel,Border);
        if(State.LastLoggedAssetFailure!=B->AssetFailure)
        {UE_LOG(LogTemp,Error,TEXT("WF_PRESENTATION_ASSET_FAILURE %s"),*FrontlineLocalizationV35::English(B->AssetFailure));State.LastLoggedAssetFailure=B->AssetFailure;}
        UI.Text(TEXT("暂时无法开始行动"),518,358,34,Paper);
        UI.Wrap(TEXT("行动准备未能完成。请返回作战室后重试。"),518,430,878,23,Muted);
        UI.Button(518,616,396,68,TEXT("返回作战室"),TEXT("←"),TEXT("AssetBack"),true,false,50);
        UI.Button(936,616,396,68,TEXT("重新开始"),TEXT("R"),TEXT("Restart"),true,true,50);
        // A painted recovery page is a valid startup handoff. Requiring the
        // failed artwork here would keep this page behind the native splash.
        // The existing render acknowledgement and GPU fence still govern reveal.
        FrontlineStartupV67::NotifyMainMenuDrawn(true);
        return;
    }
    if (State.Help)
    {
        if (B->Phase==EFrontlinePhase::Briefing) DrawMenuArtwork(UI,State.Dossier==1944);
        DrawFieldManual(UI); DrawPerformance(UI,State); return;
    }
    if (B->Phase==EFrontlinePhase::Briefing)
    {
        if (State.Dossier==1918) DrawCampaignSelection(UI,B);
        else if (State.Dossier==1919) DrawCampaignArchives(UI,B);
        else if (State.Dossier) DrawDossier(UI,State.Dossier==1944,B);
        else
        {
            DrawMainMenu(UI,B);
            // Compile the already-cooked gauge material's first draw behind
            // the existing startup artwork, before a battle needs its paint.
            static uint64 PigmentWarmAtV8213=MAX_uint64;
            if(PigmentWarmAtV8213==MAX_uint64)PigmentWarmAtV8213=GFrameCounter;
            if(GFrameCounter<=PigmentWarmAtV8213+2)
                for(int32 Nation=0;Nation<4;++Nation)if(auto* Fill=Artwork().ConsoleV79[Nation][6].Get())
                    for(int32 Paint=0;Paint<3;++Paint)
                        UI.MaterialTile(Artwork().ConsolePigmentsV8213[Nation][Paint],-4096,-4096,96,12,Fill->GetResource(),0,Paint?.60f:.12f,1,.28f);
            if(FrontlineUIAssetsV8213::PreloadSettled()&&!FrontlineUIAssetsV8213::PreloadReady()&&B->AssetFailure.IsEmpty())
                B->AssetFailure=WarRoom52::L(TEXT("Interface artwork could not be loaded."),TEXT("界面原画未能载入。"));
            bool FontsReady=Artwork().Stencil&&Artwork().ControlRuntimeV62&&Artwork().Sans&&Artwork().ChineseMediumV45;
            for(UFont* Font:{Artwork().Stencil.Get(),Artwork().Sans.Get(),Artwork().ChineseMediumV45.Get()})
                if(Font)for(UTexture2D* Page:Font->Textures)FontsReady&=FrontlineTextureReady(Page);
            bool ConsoleReadyV79=Artwork().NationalConsoleReadyV82[UI.ConsoleNationIndexV82];for(auto& T:Artwork().ConsoleV79[UI.ConsoleNationIndexV82])ConsoleReadyV79&=FrontlineTextureReady(T);
            const bool ContinueReadyV8214=!ContinueHomeCoveredV8214(Cast<AWarfareFrontlineController>(PlayerOwner),B)
                ||(FrontlineMapSceneV61::ContinuePreloadSettledV8214(Cast<AWarfareFrontlineController>(PlayerOwner))
                    &&(!FrontlineMapSceneV61::ContinuePreloadReadyV8214(Cast<AWarfareFrontlineController>(PlayerOwner))
                        ||FrontlineMapSceneV61::ContinueViewWarmedV8214(Cast<AWarfareFrontlineController>(PlayerOwner))));
            FrontlineStartupV67::NotifyMainMenuDrawn(FrontlineTextureReady(MenuCoverV63(B))&&FrontlineTextureReady(Artwork().BrandWordmarkV72)&&FontsReady&&ConsoleReadyV79&&FactionSelectionV82::WarmType(UI)&&FrontlineUIAssetsV8213::PreloadSettled()&&NavigationV61::ClickSoundSettledV8213()&&ContinueReadyV8214);
        }
        if (State.Help) DrawFieldManual(UI);
        DrawPerformance(UI,State);
        return;
    }
    if (State.LastDispatch!=B->LastMessage && !B->LastMessage.IsEmpty())
    {
        State.LastDispatch=B->LastMessage;
        State.Dispatches.Insert(TPair<float,FString>(B->Elapsed,ChineseStatus(B->LastMessage)),0);
        if (State.Dispatches.Num()>3) State.Dispatches.SetNum(3);
    }

    // A single nearby trench exposes its orders. The retained pointer corridor
    // joins the trench to the panel, so reaching a portrait never closes it.
    struct FVisibleTrench {int32 Index;FVector2D Centre,Anchor;};
    TArray<FVisibleTrench,TInlineAllocator<5>> VisibleTrenches;
    FVector2D Pointer;const bool HasPointer=UI.PointerPosition(Pointer);
    for (int32 I=0;I<B->Trenches.Num();++I)
    {
        const FFrontlineTrench& Trench=B->Trenches[I]; FVector2D Point;
        if(Trench.Owner==1)continue;
        if (!PlayerOwner->ProjectWorldLocationToScreen(FVector(Trench.X,0,AWarfareFrontlineGround::HeightAt(Trench.X,0)+190),Point)) continue;
        FVector2D Head=UI.Local(Point);
        float HighestHead=Head.Y;
        for (const AWarfareFrontlineSoldier* Unit:B->Units)
            if (IsValid(Unit) && Unit->IsAlive() && Unit->Soldier && !Unit->bMoving
                && FMath::Abs(Unit->Feet().X-Trench.X)<250.f && FMath::Abs(Unit->Feet().Y)<850.f)
            {
                FVector2D Projected;
                if (PlayerOwner->ProjectWorldLocationToScreen(Unit->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,18),Projected))
                    HighestHead=FMath::Min(HighestHead,UI.Local(Projected).Y);
            }
        const FVector2D P(Head.X,FMath::Min(Head.Y-95.f,HighestHead-44.f));
        Head.Y=HighestHead;
        if (P.X<50 || P.X>1870 || P.Y<295 || P.Y>730) continue;
        VisibleTrenches.Add({I,Head,P});
    }
    const double HoverNow=FPlatformTime::Seconds();
    const FVisibleTrench* Visible=nullptr;
    if(HasPointer)
    {
        for(const auto& T:VisibleTrenches)
            if(T.Index==State.HoverTrench && TrenchPointerStays(Pointer,T.Centre,T.Anchor)){Visible=&T;break;}
        if(!Visible)
        {
            double Closest=MAX_dbl;
            for(const auto& T:VisibleTrenches)if(TrenchPointerNear(Pointer,T.Centre))
            {const double D=FVector2D::DistSquared(Pointer,T.Centre);if(D<Closest){Closest=D;Visible=&T;}}
        }
    }
    if(Visible&&!StoryModalV82&&!State.Menu&&!State.Help&&!State.Settings&&!State.Technology&&PC->SupportTargetMode==0
        &&!FieldHoverV8218::PanelOwnsPointer(UI,Visible->Anchor)&&FieldHoverV8218::HasFieldTarget(UI,B,PC,DockTop))
    {Visible=nullptr;State.HoverTrench=INDEX_NONE;State.HoverTrenchSeenAt=0;}
    if(Visible){State.HoverTrench=Visible->Index;State.HoverTrenchSeenAt=HoverNow;}
    else if(HasPointer && HoverNow-State.HoverTrenchSeenAt<.18)
        Visible=VisibleTrenches.FindByPredicate([&](const FVisibleTrench& T){return T.Index==State.HoverTrench;});
    if(Visible)
    {
        UI.Line(Visible->Anchor.X,Visible->Anchor.Y+16,Visible->Centre.X,Visible->Centre.Y-12,FLinearColor(.48f,.58f,.58f,.4f));
    }
    else State.HoverTrench=INDEX_NONE;
    for (const FFrontlineTracer& Tracer:B->Tracers)
    {
        FVector2D A,C; const FVector Flight=Tracer.End-Tracer.Start;
        const float Progress=FMath::Clamp(1-Tracer.Life/FMath::Max(.001f,Tracer.Duration),0.f,1.f);
        const FVector Head=Tracer.Start+Flight*Progress;
        const FVector Tail=Tracer.Start+Flight*FMath::Max(0.f,Progress-45.f/FMath::Max(1.f,float(Flight.Size())));
        if (PlayerOwner->ProjectWorldLocationToScreen(Tail,A) && PlayerOwner->ProjectWorldLocationToScreen(Head,C))
            DrawLine(A.X,A.Y,C.X,C.Y,FLinearColor(.9f,.80f,.58f,.8f),.9f*UI.Scale);
    }
    if(PC->SupportTargetMode>0 && PC->bPreviewValid)
    {
        const auto Kind=SupportForTargetMode(PC->SupportTargetMode);
        DrawClassicSupportSight(UI,PC,PC->ArtilleryPreview,B->FireSupportScatterExtent(Kind));
    }

    if(PC->SupportTargetMode==0&&FPlatformTime::Seconds()-State.SupportSealAtV63<6.)DrawClassicSupportSight(UI,PC,State.SupportSealV63,State.SupportExtentV63);
    bool FieldPopupClaimedV75=false;
    #include "WarfareFrontlineClassicRail.inl"

    if(!StoryModalV82&&!State.Menu&&!State.Help&&!State.Settings&&!State.Technology&&PC->SupportTargetMode==0&&!UI.Hovered(1548,DockTop-64,348,64))
    {
        FieldPopupClaimedV75=FieldHoverV8218::Draw(UI,B,PC,DockTop,Visible&&FieldHoverV8218::PanelOwnsPointer(UI,Visible->Anchor));
    }

    if(!StoryModalV82&&!Visible&&!FieldPopupClaimedV75&&!State.Menu&&!State.Help&&!State.Settings&&!State.Technology&&!UI.Hovered(1548,DockTop-64,348,64))DrawSquadStrengthV30(UI,B,PC);
    if(!StoryModalV82&&Visible)DrawTrenchCommand(UI,B,PC,Visible->Index,Visible->Anchor);
    DrawFRRUWorldV82(UI,B,PC);

    if (HasEnded(B)) {UI.bLegacyTypographyV8210=false;DrawFieldReport(UI,B);DrawPerformance(UI,State);return;}
    if (State.Menu || !B->AssetFailure.IsEmpty())
    {
        UI.bLegacyTypographyV8210=false;
        UI.Rect(0,0,1920,1080,FLinearColor(.005f,.01f,.018f,.75f));UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
        UI.PageSurfaceV73(480,204,960,672);
        {
            UI.RoleLabel(EFrontlineTextRoleV8210::Headline,B->AssetFailure.IsEmpty()?WarRoom52::L(TEXT("BATTLE PAUSED"),TEXT("战斗已暂停")):WarRoom52::L(TEXT("OPERATION UNAVAILABLE"),TEXT("行动未能开始")),528,256,864,70,39,Paper);
            UI.PageButtonV73(528,378,420,64,TEXT("继续"),TEXT("Esc"),TEXT("Resume"),B->AssetFailure.IsEmpty(),true,50);
            UI.PageButtonV73(972,378,420,64,TEXT("战地手册"),TEXT("H"),TEXT("Help"),true,false,50);
            UI.PageButtonV73(528,470,864,64,TEXT("设置"),TEXT("O"),TEXT("Settings"),true,false,50);
            UI.PageRuleV74(528,578,864);
            UI.PageButtonV73(528,626,864,64,TEXT("重新开始行动"),TEXT("R"),TEXT("Restart"),true,false,50);
            UI.PageButtonV73(528,718,864,64,TEXT("放弃战斗，返回作战室"),TEXT(""),TEXT("AbandonBattle"),true,false,50);
        }
    }
    if (State.Help) DrawFieldManual(UI);
    if(!HasEnded(B)&&!State.Menu&&!State.Help&&!State.Settings&&!State.Technology&&!State.AchievementsV76)MedalMountV8216::DrawPicker(UI,PC,B);
    DrawPerformance(UI,State);
}
#include "WarfareFrontlineFeatureReview.inl"
#include "WarfareFrontlineV30Review.inl"
#include "WarfareFrontlineV31Review.inl"
#include "WarfareFrontlineV32Review.inl"
#include "WarfareFrontlineV33Review.inl"
#include "WarfareFrontlineV8214EffectsReview.inl"
#include "WarfareFrontlineV8215Review.inl"
#include "WarfareFrontlineV8216Review.inl"
#include "WarfareFrontlineV8217Review.inl"
#include "WarfareFrontlineV8218Review.inl"
#include "WarfareFrontlineV8219Review.inl"
#include "WarfareFrontlineV8220Review.inl"
#include "WarfareFrontlineV35Review.inl"
#include "WarfareFrontlineV36UIReview.inl"
#include "WarfareFrontlineV37Review.inl"
#include "WarfareFrontlineV38Review.inl"
#include "WarfareFrontlineReviewV39.inl"
#include "WarfareFrontlineReviewV40.inl"


#include "WarfareFrontlineUIReviewV17.inl"
#include "WarfareFrontlineUIReviewV18.inl"

#include "WarfareFrontlineReviewV41.h"
namespace FrontlineReviewUIV41
{
static bool Allowed(AWarfareFrontlineController* PC=nullptr)
{
    FString ReviewDirV66;
    if((((FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75UIReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV74Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFV73Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterReview"))||FParse::Param(FCommandLine::Get(),TEXT("WFV72Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV71Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV70Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV66Review")))return FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),ReviewDirV66)&&!ReviewDirV66.IsEmpty()&&(!PC||(FrontlineFor(PC)&&FrontlineFor(PC)->IsProgressionVolatile()));
    FString Dir;return (FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview"))||FParse::Param(FCommandLine::Get(),TEXT("WFV65Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV42Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV43Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV49Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV50Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV51Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV52Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV53Review"))||(FParse::Param(FCommandLine::Get(),TEXT("WFV54Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV63Review")))||FParse::Param(FCommandLine::Get(),TEXT("WFPromoV49"))||FParse::Param(FCommandLine::Get(),TEXT("WFV64Review")))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!Dir.IsEmpty()
        &&(!PC||(FrontlineFor(PC)&&FrontlineFor(PC)->IsProgressionVolatile()));
}
void Clear(AWarfareFrontlineController* PC)
{
    if(!PC||!Allowed(PC))return;auto& S=ScreenState(PC);UGameplayStatics::SetGamePaused(PC,false);
    S.AchievementsV76=S.Menu=S.Help=S.Settings=S.Technology=S.UnitIntroduction=S.Preparing=S.Deployment=S.HistoricalDispatch=S.CampaignEndingV51=false;
    S.bPausedByOverlay=S.bPausedForAuxOverlay=false;S.AcceptedLoadoutMission=INDEX_NONE;PC->CancelTargeting();V28ReviewPointer=FVector2D(1790,110);
}
void Dossier(AWarfareFrontlineController* PC){if(!PC||!Allowed(PC))return;Clear(PC);ScreenState(PC).Dossier=1917;PC->ResetCamera();}
void HonoursV82(AWarfareFrontlineController* PC,int32 Focus,bool Banner)
{
    if(!PC||!Allowed(PC)||!FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))
        ||Focus<0||Focus>=FrontlineAchievementsV76::Count)return;
    Clear(PC);auto& S=ScreenState(PC);S.Dossier=0;
    for(int32 I=0;I<FrontlineAchievementsV76::Count;++I)
    {
        auto& Badge=Artwork().AchievementBadgesV76[I];
        if(!Badge)Badge=LoadObject<UTexture2D>(nullptr,*FrontlineAchievementsV76::BadgeObjectPathV82(I),nullptr,LOAD_NoWarn);
    }
    if(Banner){S.AchievementToastV81=Focus;S.AchievementToastAtV81=FPlatformTime::Seconds();return;}
    PC->ToggleAchievementsV76();HonoursListV82::Reveal(S,Focus,true);
}
void FactionV83(AWarfareFrontlineController* PC,int32 Army,bool Animate)
{
    if(!PC||!Allowed(PC)||!FParse::Param(FCommandLine::Get(),TEXT("WFV83FactionOnly")))return;
    Clear(PC);auto* B=FrontlineFor(PC);B->Phase=EFrontlinePhase::Briefing;
    auto& S=ScreenState(PC);S.Dossier=1918;S.FactionSelectionV82=FFrontlineFactionSelectionV82();
    auto& F=S.FactionSelectionV82;F.Visible=true;F.Selected=Army;F.LastTick=FPlatformTime::Seconds();
    if(!Animate)for(int32 I=0;I<4;++I)F.Widths[I]=Army==INDEX_NONE?480.f:I==Army?768.f:384.f;
    using namespace FactionSelectionV82::FrameReviewV83;
    Active=true;Moving=Animate;FactionSelectionV82::FrameReviewV83::Army=Army;Last=FPlatformTime::Seconds();Wall.Reset();GPU.Reset();
}
TSharedRef<FJsonObject> FactionFramesV83(){return FactionSelectionV82::FrameReviewV83::Summary();}
void HonoursInputChecksV76(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    auto* B=PC?FrontlineFor(PC):nullptr;
    auto* H=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
    const bool Isolated=PC&&B&&H&&Allowed(PC)&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)
        &&!UserDir.IsEmpty()&&!FPaths::IsRelative(UserDir);
    Check(TEXT("v76_honours_input_disposable_profile"),Isolated);
    if(!Isolated)return;
    auto& S=ScreenState(PC);auto& History=NavigationV61::History(PC);
    const auto Before=S;const auto BeforeHistory=History;const bool WasPaused=UGameplayStatics::IsGamePaused(PC);
    ON_SCOPE_EXIT{S=Before;History=BeforeHistory;B->SetPaused(WasPaused);};
    const FGuid Encounter=B->GetEncounterId();
    const auto Reset=[&](){Clear(PC);S.ExitConfirmV61=false;S.Dossier=1917;History.Back.Empty();History.Forward.Empty();};
    const auto Closed=[&](){return !S.AchievementsV76&&!S.Help&&!S.Settings&&!S.Technology
        &&!UGameplayStatics::IsGamePaused(PC)&&B->GetEncounterId()==Encounter;};
    const auto HonoursOnly=[&](){return S.AchievementsV76&&!S.Help&&!S.Settings&&!S.Technology
        &&UGameplayStatics::IsGamePaused(PC)&&S.bPausedForAuxOverlay&&B->GetEncounterId()==Encounter;};
    struct FKeyCase{const TCHAR* Key;void (AWarfareFrontlineController::*Handler)();};
    const FKeyCase Keys[]={{TEXT("h"),&AWarfareFrontlineController::ToggleHelp},
        {TEXT("o"),&AWarfareFrontlineController::ToggleSettings},
        {TEXT("t"),&AWarfareFrontlineController::ToggleTechnology},
        {TEXT("r"),&AWarfareFrontlineController::Restart}};
    for(const auto& K:Keys)
    {
        Reset();PC->ToggleAchievementsV76();(PC->*K.Handler)();
        Check(FString::Printf(TEXT("v76_honours_%s_keeps_only_visible_page"),K.Key),HonoursOnly());
        PC->ToggleMenu();Check(FString::Printf(TEXT("v76_honours_%s_then_escape_returns"),K.Key),Closed());
        PC->ToggleAchievementsV76();(PC->*K.Handler)();H->NotifyHitBoxClick(TEXT("AchievementsBackV76"));
        Check(FString::Printf(TEXT("v76_honours_%s_then_return_button_returns"),K.Key),Closed());
    }
    for(int32 I=0;I<3;++I)
    {
        Reset();(PC->*Keys[I].Handler)();
        const bool Open=I==0?S.Help:I==1?S.Settings:S.Technology;
        PC->ToggleMenu();Check(FString::Printf(TEXT("v76_normal_%s_menu_still_opens_and_returns"),Keys[I].Key),Open&&Closed());
    }
    Reset();PC->ToggleAchievementsV76();PC->MenuBackV61();const bool Back=Closed();PC->MenuForwardV61();
    const bool Forward=HonoursOnly();PC->ToggleMenu();
    Check(TEXT("v76_honours_mouse_back_forward_preserves_pause_ownership"),Back&&Forward&&Closed());
    Reset();S.Menu=true;S.bPausedByOverlay=true;B->SetPaused(true);PC->ToggleAchievementsV76();
    PC->ToggleHelp();PC->ToggleSettings();PC->ToggleTechnology();PC->Restart();H->NotifyHitBoxClick(TEXT("AchievementsBackV76"));
    Check(TEXT("v76_honours_return_preserves_existing_paused_menu"),!S.AchievementsV76&&!S.Help&&!S.Settings&&!S.Technology
        &&S.Menu&&S.bPausedByOverlay&&UGameplayStatics::IsGamePaused(PC)&&B->GetEncounterId()==Encounter);
    Reset();S.AchievementFocusV76=0;S.AchievementScrollV82=S.AchievementScrollTargetV82=0;PC->ToggleAchievementsV76();
    for(int32 Index=0;Index<FrontlineAchievementsV76::Count;++Index)
    {
        HonoursListV82::Reveal(S,Index,true);
        H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("AchievementV76_%d"),Index)));
        Check(FString::Printf(TEXT("v82_unified_honours_entry%d_reachable"),Index),S.AchievementFocusV76==Index&&HonoursListV82::Visible(S,Index)&&HonoursOnly());
        const int32 Hidden=Index<FrontlineAchievementsV76::Count/2?FrontlineAchievementsV76::Count-1:0;
        if(!HonoursListV82::Visible(S,Hidden))H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("AchievementV76_%d"),Hidden)));
        H->NotifyHitBoxClick(TEXT("AchievementV76_999"));H->NotifyHitBoxClick(TEXT("AchievementV76_bad"));H->NotifyHitBoxClick(TEXT("AchievementChapterV82_1"));
        Check(FString::Printf(TEXT("v82_unified_honours_entry%d_hidden_invalid_hits_rejected"),Index),S.AchievementFocusV76==Index);
    }
    V28ReviewPointer=FVector2D(600,500);const float BeforeCamera=FFrontlineReviewV32::HonoursWheelV82(PC);
    S.AchievementScrollV82=S.AchievementScrollTargetV82=0;
    for(int32 I=0;I<100;++I)FFrontlineReviewV32::HonoursWheelV82(PC,1);
    Check(TEXT("v82_unified_honours_wheel_clamps_at_last_row_without_camera_zoom"),S.AchievementScrollTargetV82==HonoursListV82::Maximum()&&FFrontlineReviewV32::HonoursWheelV82(PC)==BeforeCamera);
    S.AchievementScrollV82=S.AchievementScrollTargetV82;const int32 Last=FrontlineAchievementsV76::Count-1;
    H->NotifyHitBoxClick(FName(*FString::Printf(TEXT("AchievementV76_%d"),Last)));
    Check(TEXT("v82_unified_honours_wheel_reaches_final_cross_faction_entry"),S.AchievementFocusV76==Last&&HonoursListV82::Visible(S,Last));
    PC->ToggleMenu();PC->MenuBackV61();
    Check(TEXT("v82_unified_honours_navigation_restores_focus_and_scroll"),HonoursOnly()&&S.AchievementFocusV76==Last&&S.AchievementScrollTargetV82==HonoursListV82::Maximum());
    V28ReviewPointer=FVector2D(1550,500);FFrontlineReviewV32::HonoursWheelV82(PC,-1);
    Check(TEXT("v82_unified_honours_dossier_wheel_does_not_move_list_or_camera"),S.AchievementScrollTargetV82==HonoursListV82::Maximum()&&FFrontlineReviewV32::HonoursWheelV82(PC)==BeforeCamera);
    V28ReviewPointer=FVector2D(600,500);for(int32 I=0;I<100;++I)FFrontlineReviewV32::HonoursWheelV82(PC,-1);
    Check(TEXT("v82_unified_honours_wheel_clamps_at_first_row"),S.AchievementScrollTargetV82==0&&FFrontlineReviewV32::HonoursWheelV82(PC)==BeforeCamera);
}
void HonoursBadgeChecksV82(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
    if(!PC||!Allowed(PC)||!FParse::Param(FCommandLine::Get(),TEXT("WFV76Review")))return;
    for(int32 I=0;I<FrontlineAchievementsV76::Count;++I)
    {
        auto* Badge=LoadObject<UTexture2D>(nullptr,*FrontlineAchievementsV76::BadgeObjectPathV82(I),nullptr,LOAD_NoWarn);
        UE_LOG(LogTemp,Display,TEXT("WF_V82_BADGE %s loaded=%d size=%dx%d never_stream=%d resident_mips=%d mips=%d resource=%d"),
            FrontlineAchievementsV76::Definition(I).Id,Badge!=nullptr,Badge?Badge->GetSizeX():0,Badge?Badge->GetSizeY():0,
            Badge?int32(Badge->NeverStream):0,Badge?Badge->GetNumResidentMips():0,Badge?Badge->GetNumMips():0,Badge&&Badge->GetResource()!=nullptr);
        Check(FString::Printf(TEXT("v82_honours_badge_%s_resident"),FrontlineAchievementsV76::Definition(I).Id),
            FrontlineTextureReady(Badge)&&Badge->GetSizeX()>=512&&Badge->GetSizeX()==Badge->GetSizeY()
            &&(Badge->NeverStream||Badge->LODGroup==TEXTUREGROUP_UI)&&!Badge->IsStreamable()
            &&Badge->GetNumMips()>0&&Badge->GetNumResidentMips()==Badge->GetNumMips());
    }
}
void Technology(AWarfareFrontlineController* PC,int32 Page,int32 Focus){if(!PC||!Allowed(PC))return;Dossier(PC);auto& S=ScreenState(PC);S.Technology=true;S.TechnologyPage=Page;S.TechnologyFocus=Focus;}
void Deployment(AWarfareFrontlineController* PC){if(!PC||!Allowed(PC))return;Dossier(PC);ScreenState(PC).Deployment=true;}
void Pointer(FVector2D P){if(Allowed())V28ReviewPointer=P;}
void Camera(AWarfareFrontlineController* PC,float X,float Y,float W,float H,bool Cut){if(PC&&Allowed(PC))FFrontlineReviewV32::Camera(PC,X,Y,W,H,Cut);}
void BeginAudit(){if(!Allowed())return;V27ImageAudit.Empty();V36TypographyAudit.Empty();V27ImageAuditActive=V36TypographyAuditActive=true;PageCanvasV73::Hits.Empty();PageCanvasV73::Audit=FParse::Param(FCommandLine::Get(),TEXT("WFV73Review"))||(((FParse::Param(FCommandLine::Get(),TEXT("WFV76Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV75GameplayReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV75UIReview")))||FParse::Param(FCommandLine::Get(),TEXT("WFV74Review")));V825FontDraws.Empty();V8212ShapedFontDraws.Empty();V825FontEdges.Reset();V825FontAuditActive=FParse::Param(FCommandLine::Get(),TEXT("WFV825FontLayout"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));}
TSharedRef<FJsonObject> EndAudit()
{
    auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Text,Images,Checks,Hits;bool Good=Allowed();
    for(const auto& E:V36TypographyAudit){Text.Add(MakeShared<FJsonValueObject>(E.Value));Good&=E.Value->GetBoolField(TEXT("fits"))&&!E.Value->GetBoolField(TEXT("truncated"));}
    for(const auto& E:V27ImageAudit){Images.Add(MakeShared<FJsonValueObject>(E.Value));Good&=E.Value->GetNumberField(TEXT("aspect_relative_error"))<.001;}
    for(const auto& E:PageCanvasV73::Hits){Hits.Add(MakeShared<FJsonValueObject>(E.Value));Good&=E.Value->GetBoolField(TEXT("fits"));}
    auto Check=MakeShared<FJsonObject>();Check->SetStringField(TEXT("name"),TEXT("v41_affected_real_ui_text_and_image_bounds"));Check->SetBoolField(TEXT("passed"),Good);Checks.Add(MakeShared<FJsonValueObject>(Check));
    if(V825FontAuditActive)
    {
        TArray<TSharedPtr<FJsonValue>> Draws,ShapedDraws;float WidthError=0,HeightError=0,AnchorError=0,MinimumDPI=MAX_flt,MaximumDPI=0;bool AllHDRComposite=true;
        for(const auto& E:V825FontDraws){Draws.Add(MakeShared<FJsonValueObject>(E.Value));WidthError=FMath::Max(WidthError,float(E.Value->GetNumberField(TEXT("width_error"))));HeightError=FMath::Max(HeightError,float(E.Value->GetNumberField(TEXT("height_error"))));const float DPI=E.Value->GetNumberField(TEXT("canvas_dpi"));MinimumDPI=FMath::Min(MinimumDPI,DPI);MaximumDPI=FMath::Max(MaximumDPI,DPI);}
        for(const auto& E:V825FontDraws)AllHDRComposite&=E.Value->GetBoolField(TEXT("hdr_sdr_composite"));
        for(const auto& E:V8212ShapedFontDraws)
        {
            ShapedDraws.Add(MakeShared<FJsonValueObject>(E.Value));WidthError=FMath::Max(WidthError,float(E.Value->GetNumberField(TEXT("width_error"))));HeightError=FMath::Max(HeightError,float(E.Value->GetNumberField(TEXT("height_error"))));AnchorError=FMath::Max(AnchorError,float(E.Value->GetNumberField(TEXT("anchor_error"))));
            const float DPI=E.Value->GetNumberField(TEXT("canvas_dpi"));MinimumDPI=FMath::Min(MinimumDPI,DPI);MaximumDPI=FMath::Max(MaximumDPI,DPI);
            AllHDRComposite&=E.Value->GetBoolField(TEXT("hdr_sdr_composite"));
        }
        // New shaped-only pages have no legacy glyph draws. Require their actual
        // engine-loop evidence instead of allowing a self-measurement-only pass.
        const bool HasDraws=!Draws.IsEmpty()||!ShapedDraws.IsEmpty();
        const bool LegacyPassed=Draws.IsEmpty()||(V825FontEdges.IsValid()&&V825FontEdges->GetBoolField(TEXT("success")));
        auto Native=MakeShared<FJsonObject>();Native->SetStringField(TEXT("name"),TEXT("v825_actual_canvas_draw_and_measure_bounds_agree"));const bool Passed=LegacyPassed&&HasDraws&&WidthError<.05f&&HeightError<.05f&&AnchorError<.05f;Native->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(Native));Good&=Passed;
        R->SetArrayField(TEXT("font_layout_native_draws"),Draws);if(V825FontEdges.IsValid())R->SetObjectField(TEXT("font_layout_edges"),V825FontEdges);
        R->SetArrayField(TEXT("font_layout_shaped_draws"),ShapedDraws);R->SetNumberField(TEXT("font_layout_anchor_error"),AnchorError);
        R->SetBoolField(TEXT("font_layout_hdr_sdr_composite"),HasDraws&&AllHDRComposite);
        R->SetNumberField(TEXT("font_layout_native_window_dpi"),V8212NativeWindowDPI);
        R->SetNumberField(TEXT("font_layout_viewport_cached_dpi"),V8212ViewportCachedDPI);
        R->SetNumberField(TEXT("font_layout_viewport_dpi_refreshes"),V8212ViewportDPIRefreshes);
        R->SetNumberField(TEXT("font_layout_maximum_width_error"),WidthError);R->SetNumberField(TEXT("font_layout_maximum_height_error"),HeightError);
        R->SetNumberField(TEXT("font_layout_minimum_canvas_dpi"),HasDraws?MinimumDPI:0);R->SetNumberField(TEXT("font_layout_maximum_canvas_dpi"),MaximumDPI);
    }
    R->SetBoolField(TEXT("success"),Good);R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("typography"),Text);R->SetArrayField(TEXT("images"),Images);R->SetArrayField(TEXT("hitboxes"),Hits);
    V28ReviewPointer.Reset();V27ImageAuditActive=V36TypographyAuditActive=PageCanvasV73::Audit=V825FontAuditActive=false;return R;
}
}

namespace FrontlineHUDV45
{
void BeginAudit()
{
    if(!FrontlineReviewUIV41::Allowed()||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    V45PrimaryDraws.Empty();V45ChineseDraws.Empty();V45InvalidChineseBreaks.Empty();V45AuditLanguage=FrontlineLocalizationV35::GetLanguage();V45HUDAuditActive=true;
    V66HUDModernAudit=FParse::Param(FCommandLine::Get(),TEXT("WFV66Review"));V66HUDHoverIndex=0;
}
TSharedRef<FJsonObject> EndAudit()
{
    auto R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Primary,Chinese,Checks;bool Good=true;
    auto Check=[&](const TCHAR* Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    Check(TEXT("v45_isolated_actual_draw_audit_started"),V45HUDAuditActive&&FrontlineReviewUIV41::Allowed());
    Check(TEXT("v45_chinese_selection_preserved"),V45AuditLanguage==FrontlineLocalizationV35::ELanguage::Chinese&&FrontlineLocalizationV35::GetLanguage()==V45AuditLanguage);
    bool AllEnglish=true,AllMedium=true;float MaximumWidthError=0,MaximumHeightError=0,MaximumAnchorError=0;
    for(const auto& E:V45PrimaryDraws){Primary.Add(MakeShared<FJsonValueObject>(E.Value));AllEnglish&=!E.Value->GetBoolField(TEXT("contains_cjk"))&&E.Value->GetBoolField(TEXT("selected_chinese"));}
    Check(TEXT("v45_chinese_mode_primary_hud_draws_english"),V45PrimaryDraws.Num()>=12&&AllEnglish);
    if(V66HUDModernAudit)
    {
        TTextCacheV825<bool> Distinct,FirstHover,SecondHover;bool Faces=true,Glyphs=true,Finite=true;
        for(const auto& E:V45ChineseDraws)
        {
            const auto& J=E.Value;bool Actual=false;
            if(!J->TryGetBoolField(TEXT("actual_shaped_glyph_loop"),Actual))continue;
            if(!Actual){Glyphs=false;continue;}
            Chinese.Add(MakeShared<FJsonValueObject>(J));const FString Value=J->GetStringField(TEXT("text"));Distinct.Add(Value,true);
            const int32 Hover=int32(J->GetNumberField(TEXT("hover_index")));
            if(Hover==1)FirstHover.Add(Value,true);else if(Hover==2)SecondHover.Add(Value,true);
            Faces&=J->GetBoolField(TEXT("modern_cjk_face_family"));Finite&=J->GetBoolField(TEXT("bounds_finite"));
            Glyphs&=J->GetBoolField(TEXT("glyph_source_indices_valid"))&&J->GetBoolField(TEXT("visible_glyphs_valid"))
                &&J->GetBoolField(TEXT("glyph_atlas_entries_valid"))&&J->GetBoolField(TEXT("cjk_source_coverage"))
                &&J->GetBoolField(TEXT("cjk_face_has_glyphs_without_fallback"));
            MaximumWidthError=FMath::Max(MaximumWidthError,float(J->GetNumberField(TEXT("width_error"))));
            MaximumHeightError=FMath::Max(MaximumHeightError,float(J->GetNumberField(TEXT("height_error"))));
            MaximumAnchorError=FMath::Max(MaximumAnchorError,float(J->GetNumberField(TEXT("anchor_error"))));
            if(J->GetBoolField(TEXT("closing_punctuation_at_row_start")))V45InvalidChineseBreaks.Add(Value);
        }
        int32 NewSecondHoverRows=0;for(const auto& E:SecondHover)NewSecondHoverRows+=!FirstHover.Contains(E.Key);
        const bool HasChinese=Distinct.Num()>=4;
        Check(TEXT("v66_four_distinct_actual_shaped_cjk_rows"),HasChinese);
        Check(TEXT("v66_both_real_hovers_draw_distinct_chinese_explanations"),FirstHover.Num()>0&&SecondHover.Num()>0&&NewSecondHoverRows>0);
        Check(TEXT("v66_actual_cjk_draws_use_approved_modern_face_families"),HasChinese&&Faces);
        Check(TEXT("v66_actual_cjk_glyphs_sources_and_atlas_are_valid"),HasChinese&&Glyphs);
        Check(TEXT("v66_actual_cjk_draw_bounds_and_anchor_agree"),HasChinese&&Finite&&MaximumWidthError<.05f&&MaximumHeightError<.05f&&MaximumAnchorError<.05f);
        Check(TEXT("v45_chinese_closing_punctuation_keeps_preceding_glyph"),HasChinese&&V45InvalidChineseBreaks.IsEmpty());
        R->SetNumberField(TEXT("distinct_actual_chinese_rows"),Distinct.Num());R->SetNumberField(TEXT("first_hover_chinese_rows"),FirstHover.Num());
        R->SetNumberField(TEXT("second_hover_chinese_rows"),SecondHover.Num());R->SetNumberField(TEXT("second_hover_new_chinese_rows"),NewSecondHoverRows);
        R->SetStringField(TEXT("face_validation_scope"),TEXT("Actual shaped draws must use an approved V8210 CJK mixed family and valid rendered glyph/atlas data; this does not claim per-role context survived retained tooltip replay."));
    }
    else
    {
        for(const auto& E:V45ChineseDraws){Chinese.Add(MakeShared<FJsonValueObject>(E.Value));AllMedium&=E.Value->GetBoolField(TEXT("medium_face"));MaximumWidthError=FMath::Max(MaximumWidthError,float(E.Value->GetNumberField(TEXT("width_error"))));}
        Check(TEXT("v45_localized_explanation_draws_medium_cjk"),V45ChineseDraws.Num()>=4&&AllMedium);
        Check(TEXT("v45_cjk_draw_and_wrap_measurement_agree"),V45ChineseDraws.Num()>=4&&MaximumWidthError<.05f);
        Check(TEXT("v45_chinese_closing_punctuation_keeps_preceding_glyph"),V45ChineseDraws.Num()>=4&&V45InvalidChineseBreaks.IsEmpty());
        UFont* Medium=Artwork().ChineseMediumV45;
        Check(TEXT("v45_packaged_runtime_medium_face_present"),Medium&&Medium->FontCacheType==EFontCacheType::Runtime);
    }
    R->SetBoolField(TEXT("success"),Good);R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("primary_draws"),Primary);R->SetArrayField(TEXT("chinese_draws"),Chinese);
    R->SetNumberField(TEXT("maximum_draw_measure_width_error"),MaximumWidthError);
    R->SetNumberField(TEXT("maximum_draw_measure_height_error"),MaximumHeightError);R->SetNumberField(TEXT("maximum_draw_anchor_error"),MaximumAnchorError);
    R->SetNumberField(TEXT("invalid_chinese_line_breaks"),V45InvalidChineseBreaks.Num());
    R->SetStringField(TEXT("scope"),V66HUDModernAudit?TEXT("Isolated V66 actual engine glyph-loop bounds and anchors, visible Chinese explanation rows from both existing synthetic hovers, approved modern mixed font families and glyph/atlas coverage under Chinese selection; primary HUD English scope remains the existing actual draw audit. No physical mouse, pixel appearance, or per-role replay metadata claim."):TEXT("Actual primary HUD and localized explanation draw calls under Chinese selection; paired real UI typography audit checks each constrained box."));
    if(V66HUDModernAudit)V825FontAuditActive=false;V45HUDAuditActive=V66HUDModernAudit=false;V66HUDHoverIndex=0;return R;
}
}

namespace FrontlineReviewUIV51
{
void Ending(AWarfareFrontlineController* PC)
{
    if(!PC||!FrontlineReviewUIV41::Allowed(PC)||!FParse::Param(FCommandLine::Get(),TEXT("WFV51Review")))return;
    FrontlineReviewUIV41::Clear(PC);
    if(FrontlineCampaignV51::CanOpenEnding(FrontlineFor(PC)))ScreenState(PC).CampaignEndingV51=true;
}
}

#include "WarfareFrontlineWarRoomReviewV52.inl"

#include "WarfareFrontlineReviewV63.inl"
#include "WarfareFrontlineReviewV64.inl"

#include "WarfareFrontlineReviewV65.inl"
#include "WarfareFrontlineReviewV66.inl"
#include "WarfareFrontlineReviewV68.inl"

#include "WarfareFrontlineReviewV70.inl"
#include "WarfareFrontlineArtReviewV71.inl"
#include "WarfareFrontlineUIReviewV72.inl"
#include "WarfareFrontlineUIReviewV73.inl"
#include "WarfareFrontlineUIReviewV74.inl"
#include "WarfareFrontlineMotionReviewV83.inl"
#include "WarfareFrontlineUIReviewV75.inl"
#include "FRRU/WarfareFRRUNaturalReviewV82.inl"
#include "FRRU/WarfareFRRUCampaignPlayReviewV82.inl"
#include "FRRU/FRRUParityReviewV87.inl"

#include "WarfareFrontlineTextReviewV829.inl"

#include "WarfareFrontlineUIPerfV8213.inl"

#include "WarfareFrontlineV8221Review.inl"

bool StartDistributionCampaignFixtureV8222(AWarfareFrontlineController* PC)
{
    auto* B=PC?FrontlineFor(PC):nullptr;
    FString User;
    if(!B||!B->IsProgressionVolatile()
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFDistributionCheck"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)
        ||User.IsEmpty()||FPaths::IsRelative(User)||User.Contains(TEXT("..")))return false;
    User=FPaths::ConvertRelativePathToFull(User);
    if(!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),User))return false;
    auto& S=ScreenState(PC);
    if(S.Dossier!=1918||S.Menu||!B->StartNewCampaign(EFrontlineFaction::British,EFrontlineDifficulty::Rookie))return false;
    S.Dossier=1917;S.Menu=false;S.FactionSelectionV82=FFrontlineFactionSelectionV82();
    PC->ResetCamera();return true;
}
