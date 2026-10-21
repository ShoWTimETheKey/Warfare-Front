#include "WarfareFrontlineArtV68.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineMaterialV27.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFrontlineGround.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
TAutoConsoleVariable<int32> GSoldiersV68(TEXT("wf.Art.V68.Soldiers"),1,TEXT("V68 garment construction and cloth/canvas/leather response. 0 restores V67 for review A/B."));
TAutoConsoleVariable<int32> GTerrainV68(TEXT("wf.Art.V68.Terrain"),1,TEXT("V68 damp-earth bank response. 0 restores V67 for review A/B."));
TAutoConsoleVariable<int32> GLightV68(TEXT("wf.Art.V68.Light"),1,TEXT("V68 key/fill arrangement. 0 restores V67 for review A/B."));
// Same-process review (V68 runs 2026-09-29): the authored rear-quarter key
// (yaw 36-58) left every camera-facing uniform in sky fill and returned a
// specular sheet from wet banks; a front-left key (-70) flattened crater
// relief. -40 turns each operation's key into a cross light from the left:
// relief keeps its side shadows and the visible side of each soldier is lit.
TAutoConsoleVariable<float> GSunYawV68(TEXT("wf.Art.V68.SunYawOffset"),-40.f,TEXT("Degrees added to each operation's authored sun yaw when V68 light is enabled."));
TAutoConsoleVariable<float> GSunPitchV68(TEXT("wf.Art.V68.SunPitchOffset"),0.f,TEXT("Degrees added to the authored sun pitch (negative is higher) when V68 light is enabled."));
TAutoConsoleVariable<float> GSunScaleV68(TEXT("wf.Art.V68.SunScale"),1.f,TEXT("Multiplier on the operation sun intensity when V68 light is enabled."));
TAutoConsoleVariable<float> GSkyScaleV68(TEXT("wf.Art.V68.SkyScale"),1.f,TEXT("Multiplier on the operation sky fill when V68 light is enabled."));

bool Baseline(){return FParse::Param(FCommandLine::Get(),TEXT("WFV68Baseline"));}
// V69: V68's broad art replacement was not accepted by the player. Keep its
// evidence and isolated comparison harness, but never opt normal play into it
// through a stale console setting. V67 remains the production appearance.
bool ReviewCandidate(){return FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview"))&&!Baseline();}
const TCHAR* CharacterRootV67=TEXT("/Game/Warfare/Characters/V67/");
const TCHAR* CharacterRootV68=TEXT("/Game/Warfare/Characters/V68/");

template<typename T> T* CachedLoad(const FString& Path)
{
    // A missing optional V68 asset is a normal fallback, not an error to retry
    // on every spawn. Cache both hits and misses for the process lifetime.
    static TMap<FString,TWeakObjectPtr<UObject>> Hits;static TSet<FString> Misses;
    if(Misses.Contains(Path))return nullptr;
    if(const auto* Found=Hits.Find(Path))if(Found->IsValid())return Cast<T>(Found->Get());
    T* Loaded=LoadObject<T>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(Loaded)Hits.Add(Path,Loaded);else Misses.Add(Path);
    return Loaded;
}
FString AssetPath(const FString& Folder,const FString& Name){return Folder+Name+TEXT(".")+Name;}
}

bool FrontlineArtV68::SoldiersEnabled(){return ReviewCandidate()&&GSoldiersV68.GetValueOnGameThread()!=0;}
bool FrontlineArtV68::TerrainEnabled(){return ReviewCandidate()&&GTerrainV68.GetValueOnGameThread()!=0;}
bool FrontlineArtV68::LightEnabled(){return ReviewCandidate()&&GLightV68.GetValueOnGameThread()!=0;}

FString FrontlineArtV68::SourceMeshName(const FString& Name)
{
    if(!Name.EndsWith(TEXT("_V68"))&&!Name.EndsWith(TEXT("_V67")))return Name;
    if(Name.Contains(TEXT("British_RifleGrenadier_"))||Name.Contains(TEXT("German_AntiTankRifle_")))
        return Name.LeftChop(4)+TEXT("_V41");
    return Name.LeftChop(4)+TEXT("_V34");
}

USkeletalMesh* FrontlineArtV68::ResolveMesh(USkeletalMesh* Source)
{
    if(!Source||!SoldiersEnabled())return Source;
    const FString Original=Source->GetName();
    if(!Source->GetPathName().StartsWith(CharacterRootV67)||!Original.EndsWith(TEXT("_V67")))return Source;
    const bool Casualty=Original.StartsWith(TEXT("SK_Casualty_"));
    const FString LivingName=Original.Replace(TEXT("SK_Casualty_"),TEXT("SK_")).LeftChop(4)+TEXT("_V68");
    const FString CasualtyName=LivingName.Replace(TEXT("SK_"),TEXT("SK_Casualty_"));
    auto* Living=CachedLoad<USkeletalMesh>(AssetPath(FString(CharacterRootV68)+TEXT("Meshes/"),LivingName));
    auto* Dead=CachedLoad<USkeletalMesh>(AssetPath(FString(CharacterRootV68)+TEXT("Casualties/"),CasualtyName));
    // A living body must never be paired with an older wounded body, and the
    // shared skeleton keeps animation, sockets and physics contracts intact.
    if(Living&&Dead&&Living->GetSkeleton()==Source->GetSkeleton()&&Dead->GetSkeleton()==Source->GetSkeleton())return Casualty?Dead:Living;
    static TSet<FString> Reported;
    if(!Reported.Contains(Original)){Reported.Add(Original);UE_LOG(LogTemp,Display,TEXT("WF_V68_WARDROBE_KEEPS_V67 %s living=%d casualty=%d"),*Original,Living!=nullptr,Dead!=nullptr);}
    return Source;
}

UMaterialInterface* FrontlineArtV68::ResolveMaterial(UMaterialInterface* Source)
{
    if(!Source||!SoldiersEnabled())return Source;
    const FString Path=Source->GetPathName();
    // V67 garments and retained V34 skin/weapon/badge graphs may both receive
    // a same-named V68 response. Absent V68 names keep the accepted binding.
    if(!Path.StartsWith(CharacterRootV67+FString(TEXT("Materials/")))&&!Path.StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/")))return Source;
    if(auto* Material=CachedLoad<UMaterialInterface>(AssetPath(FString(CharacterRootV68)+TEXT("Materials/"),Source->GetName())))return Material;
    return Source;
}

UMaterialInterface* FrontlineArtV68::ResolveBankMaterial(UMaterialInterface* Source)
{
    if(!TerrainEnabled())return Source;
    if(auto* Bank=CachedLoad<UMaterialInterface>(TEXT("/Game/Warfare/Field/V68/Materials/MI_EarthBank_V68.MI_EarthBank_V68")))return Bank;
    return Source;
}

void FrontlineArtV68::AdjustLight(float& Pitch,float& Yaw,float& SunIntensity,float& SkyIntensity)
{
    if(!LightEnabled())return;
    Yaw+=GSunYawV68.GetValueOnGameThread();
    Pitch=FMath::Clamp(Pitch+GSunPitchV68.GetValueOnGameThread(),-70.f,-8.f);
    SunIntensity*=FMath::Clamp(GSunScaleV68.GetValueOnGameThread(),.5f,1.5f);
    SkyIntensity*=FMath::Clamp(GSkyScaleV68.GetValueOnGameThread(),.5f,1.5f);
}

UMaterialInterface* FrontlineArtV68::ResolveGroundMaterial(UMaterialInterface* Source)
{
    // Strict baseline only: with FieldFaciesAmountV68=0 the V68 graph reproduces
    // V67, so the terrain toggle can A/B the facies without swapping materials.
    // Unaccepted candidate: its material import never completed (see
    // Docs/V68/ART-REVIEW.md). Opt-in only; production keeps the V67 soil.
    if(!ReviewCandidate()||!FParse::Param(FCommandLine::Get(),TEXT("WFV68Facies")))return Source;
    if(!Source||!Source->GetName().Contains(TEXT("GroundMud")))return Source;
    if(auto* Soil=CachedLoad<UMaterialInterface>(TEXT("/Game/Warfare/Field/V68/Materials/MI_GroundMud_V68.MI_GroundMud_V68")))return Soil;
    return Source;
}

namespace
{
TArray<float> BoxBlur(const TArray<float>& In,int32 W,int32 H,int32 R)
{
    // Two separable running-sum passes, edge clamped; applied twice by callers
    // that need a smoother (near-Gaussian) response.
    if(R<=0)return In;
    TArray<float> Tmp;Tmp.SetNumUninitialized(In.Num());TArray<float> Out;Out.SetNumUninitialized(In.Num());
    for(int32 Y=0;Y<H;++Y)
    {
        double Sum=0;for(int32 K=-R;K<=R;++K)Sum+=In[Y*W+FMath::Clamp(K,0,W-1)];
        for(int32 X=0;X<W;++X)
        {
            Tmp[Y*W+X]=float(Sum/(2*R+1));
            Sum+=In[Y*W+FMath::Clamp(X+R+1,0,W-1)]-In[Y*W+FMath::Clamp(X-R,0,W-1)];
        }
    }
    for(int32 X=0;X<W;++X)
    {
        double Sum=0;for(int32 K=-R;K<=R;++K)Sum+=Tmp[FMath::Clamp(K,0,H-1)*W+X];
        for(int32 Y=0;Y<H;++Y)
        {
            Out[Y*W+X]=float(Sum/(2*R+1));
            Sum+=Tmp[FMath::Clamp(Y+R+1,0,H-1)*W+X]-Tmp[FMath::Clamp(Y-R,0,H-1)*W+X];
        }
    }
    return Out;
}
float Smooth(float A,float B,float X){const float T=FMath::Clamp((X-A)/(B-A),0.f,1.f);return T*T*(3-2*T);}
}

void FrontlineArtV68::BindFieldFacies(AWarfareFrontlineGround* Ground,UMaterialInstanceDynamic* GroundMID)
{
    if(!ReviewCandidate()||!FParse::Param(FCommandLine::Get(),TEXT("WFV68Facies"))||!Ground||!GroundMID)return;
    float Probe=-1;
    if(!GroundMID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FieldFaciesAmountV68")),Probe))return;
    // The rendered field: union of the twenty baked tile bounds, clamped to the
    // depth the tactical camera can reach (the painted matte covers the rest).
    FBox Box(ForceInit);
    for(const auto& T:Ground->BakedTerrainV43)if(T)Box+=T->Bounds.GetBox();
    if(!Box.IsValid)Box=FBox(FVector(-14500,-5600,0),FVector(14500,2600,0));
    const float X0=Box.Min.X,X1=Box.Max.X,Y0=FMath::Max(Box.Min.Y,-7200.),Y1=FMath::Min(Box.Max.Y,4200.);
    const int32 W=FMath::Clamp(int32((X1-X0)/40.f),64,1024),H=FMath::Clamp(int32((Y1-Y0)/40.f),16,384);
    const float DX=(X1-X0)/W,DY=(Y1-Y0)/H;
    const double Start=FPlatformTime::Seconds();
    TArray<float> Hm;Hm.SetNumUninitialized(W*H);
    for(int32 Y=0;Y<H;++Y)for(int32 X=0;X<W;++X)
        Hm[Y*W+X]=AWarfareFrontlineGround::HeightAt(X0+(X+.5f)*DX,Y0+(Y+.5f)*DY);
    const auto Blur2=[&](const TArray<float>& In,float Cm){const int32 R=FMath::Max(1,FMath::RoundToInt(Cm/DX*.5f));return BoxBlur(BoxBlur(In,W,H,R),W,H,R);};
    const TArray<float> Local=Blur2(Hm,90.f),Crater=Blur2(Hm,650.f),Broad=Blur2(Hm,2200.f);
    TArray<FColor> Pixels;Pixels.SetNumUninitialized(W*H);
    for(int32 Y=0;Y<H;++Y)for(int32 X=0;X<W;++X)
    {
        const int32 I=Y*W+X;
        const auto At=[&](int32 XX,int32 YY){return Local[FMath::Clamp(YY,0,H-1)*W+FMath::Clamp(XX,0,W-1)];};
        const float Relief=Hm[I]-Crater[I];
        const float Laplace=(At(X-1,Y)+At(X+1,Y)-2*At(X,Y))/(DX*DX)+(At(X,Y-1)+At(X,Y+1)-2*At(X,Y))/(DY*DY);
        const float Gx=(At(X+1,Y)-At(X-1,Y))/(2*DX),Gy=(At(X,Y+1)-At(X,Y-1))/(2*DY);
        // A 2.5 m crater 80 cm deep: bowl relief about -60 cm, Laplacian +0.005/cm;
        // its thrown-out lip is 15-30 cm proud and strongly convex.
        const float Bowl=Smooth(10.f,50.f,-Relief)*(.55f+.45f*Smooth(-.0005f,.004f,Laplace));
        const float Rim=Smooth(4.f,22.f,Relief)*Smooth(.0006f,.006f,-Laplace);
        const float Steep=Smooth(.35f,.95f,FMath::Sqrt(Gx*Gx+Gy*Gy));
        const float Low=Smooth(12.f,70.f,Broad[I]-Crater[I]);
        Pixels[I]=FColor(uint8(Bowl*255.f+.5f),uint8(Rim*255.f+.5f),uint8(Steep*255.f+.5f),uint8(Low*255.f+.5f));
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV68ArtReview")))
    {
        // Review evidence: the exact mask bound to the ground (RGB = bowl, rim, cut).
        TArray<FColor> Preview=Pixels;for(auto& P:Preview)P.A=255;
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Gameplay/V68/Review/facies");IFileManager::Get().MakeDirectory(*Dir,true);
        FFileHelper::CreateBitmap(*(Dir/FString::Printf(TEXT("facies-%dx%d-%08x.bmp"),W,H,GetTypeHash(FString::SanitizeFloat(Hm[W*H/2])))),W,H,Preview.GetData());
    }
    UTexture2D* Texture=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8,TEXT("T_FieldFacies_V68"));
    if(!Texture)return;
    Texture->SRGB=false;Texture->Filter=TF_Bilinear;Texture->AddressX=Texture->AddressY=TA_Clamp;
    Texture->CompressionSettings=TC_VectorDisplacementmap;Texture->NeverStream=true;
    FTexture2DMipMap& Mip=Texture->GetPlatformData()->Mips[0];
    void* Data=Mip.BulkData.Lock(LOCK_READ_WRITE);FMemory::Memcpy(Data,Pixels.GetData(),Pixels.Num()*sizeof(FColor));Mip.BulkData.Unlock();
    Texture->UpdateResource();
    GroundMID->SetTextureParameterValue(TEXT("FieldFaciesV68"),Texture);
    GroundMID->SetVectorParameterValue(TEXT("FieldFaciesFrameV68"),FLinearColor(X0,Y0,1.f/(X1-X0),1.f/(Y1-Y0)));
    GroundMID->SetScalarParameterValue(TEXT("FieldFaciesAmountV68"),TerrainEnabled()?1.f:0.f);
    UE_LOG(LogTemp,Display,TEXT("WF_V68_FIELD_FACIES size=%dx%d cell_cm=%.1fx%.1f frame=(%.0f,%.0f)-(%.0f,%.0f) seconds=%.3f amount=%d"),
        W,H,DX,DY,X0,Y0,X1,Y1,FPlatformTime::Seconds()-Start,TerrainEnabled());
}

void FrontlineArtV68::ApplyWorld(UWorld* World)
{
    if(!ReviewCandidate()||!World)return;
    int32 Swapped=0,Rebound=0,Banks=0;
    for(TActorIterator<AWarfareFrontlineSoldier> It(World);It;++It)
    {
        AWarfareFrontlineSoldier* Unit=*It;if(!Unit||!Unit->Soldier)continue;
        USkeletalMesh* Current=Unit->Soldier->GetSkeletalMeshAsset();if(!Current)continue;
        USkeletalMesh* V67=Current;
        if(Current->GetPathName().StartsWith(CharacterRootV68)&&Current->GetName().EndsWith(TEXT("_V68")))
        {
            const FString Name=Current->GetName().LeftChop(4)+TEXT("_V67");
            const FString Folder=Name.StartsWith(TEXT("SK_Casualty_"))?TEXT("Casualties/"):TEXT("Meshes/");
            if(auto* Accepted=CachedLoad<USkeletalMesh>(AssetPath(FString(CharacterRootV67)+Folder,Name)))V67=Accepted;
        }
        USkeletalMesh* Target=ResolveMesh(V67);
        if(Target&&Target!=Current&&Target->GetSkeleton()==Current->GetSkeleton())
        {
            // Retain the evaluated pose and anim instance: same skeleton, same
            // bone transforms, only the skinned surface changes.
            Unit->Soldier->SetSkinnedAssetAndUpdate(Target,false);
            Unit->Soldier->RefreshBoneTransforms();++Swapped;
        }
        FrontlineMaterialV27::Apply(Unit);++Rebound;
    }
    for(TActorIterator<AActor> It(World);It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
        for(auto* C:Meshes)
        {
            const bool Bank=C->GetStaticMesh()&&C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_TrenchEarth_"));
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
            {
                auto* Existing=C->GetMaterial(Slot);const auto* Base=Existing?Existing->GetBaseMaterial():nullptr;
                const bool Earth=Base&&(Base->GetName().Contains(TEXT("EarthBank_V42"))||Base->GetName().Contains(TEXT("EarthBank_V44"))||Base->GetName().Contains(TEXT("EarthBank_V68")));
                if(!Bank&&!Earth)continue;
                auto* V44=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V44/Materials/MI_EarthBank_V44.MI_EarthBank_V44"));
                auto* Wanted=ResolveBankMaterial(V44);if(!Wanted)continue;
                float Wet=.58f,Chalk=0.f;
                if(Existing){Existing->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Wet);Existing->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ChalkExposure")),Chalk);}
                if(auto* MID=C->CreateDynamicMaterialInstance(Slot,Wanted))
                {MID->SetScalarParameterValue(TEXT("WeatherWetness"),Wet);MID->SetScalarParameterValue(TEXT("ChalkExposure"),Chalk);++Banks;}
            }
        }
    }
    for(TActorIterator<AWarfareFrontlineGround> It(World);It;++It)
        if(It->Surface)if(auto* MID=Cast<UMaterialInstanceDynamic>(It->Surface->GetMaterial(0)))
        {
            float Probe=-1;
            if(MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FieldFaciesAmountV68")),Probe))
            {MID->SetScalarParameterValue(TEXT("FieldFaciesAmountV68"),TerrainEnabled()?1.f:0.f);It->SyncTerrainMaterialV43();}
        }
    for(TActorIterator<AWarfareFrontlineWeather> It(World);It;++It)It->RebindGroundMaterial();
    UE_LOG(LogTemp,Display,TEXT("WF_V68_ART_APPLY soldiers=%d terrain=%d light=%d swapped_meshes=%d rebound_soldiers=%d bank_slots=%d"),
        SoldiersEnabled(),TerrainEnabled(),LightEnabled(),Swapped,Rebound,Banks);
}

static FAutoConsoleCommandWithWorldAndArgs GApplyArtV68(TEXT("wf.Art.V68.Apply"),
    TEXT("Rebind existing soldiers, earth banks and light to the current wf.Art.V68.* toggles. Review A/B only; no saves, assets or gameplay state change."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&,UWorld* World){FrontlineArtV68::ApplyWorld(World);}));
