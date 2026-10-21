#include "WarfareFrontlineMapSceneV61.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineCampaignV52.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponentBase.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RendererInterface.h"
#include "Modules/ModuleManager.h"

namespace FrontlineMapSceneV61
{
namespace
{
constexpr double Width=2200.,Height=1540.,TableZ=-100000.;
constexpr float FOV=50.f,ViewSpan=2470.f;
const FName OwnerTag(TEXT("WF_MapScene_V61"));
const TCHAR* AssetRoot=TEXT("/Game/Warfare/Campaign/V61/");
const FMapRect Sectors[]={
    {{.350,.085},{.525,.335}},{{.350,.215},{.500,.390}},
    {{.310,.270},{.530,.475}},{{.390,.225},{.540,.410}},
    {{.425,.210},{.600,.410}}};

struct FHiddenActor {TWeakObjectPtr<AActor> Actor;bool Hidden=false;};
struct FHiddenComponent {TWeakObjectPtr<USceneComponent> Component;bool Visible=false;};
struct FDisabledGrade {TWeakObjectPtr<APostProcessVolume> Volume;bool Enabled=false;};
struct FSheet
{
    TWeakObjectPtr<UStaticMeshComponent> Surface;
    TWeakObjectPtr<UMaterialInstanceDynamic> Material;
    TWeakObjectPtr<UTexture2D> Texture;
    int32 Index=INDEX_NONE;
};
struct FState
{
    TWeakObjectPtr<AWarfareFrontlineController> Owner;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> Root,PreviousView;
    TWeakObjectPtr<ACameraActor> Camera;
    TArray<FHiddenActor> HiddenActors;
    TArray<FHiddenComponent> HiddenComponents;
    TArray<FDisabledGrade> Grades;
    FSheet Atlas;
    int32 HoverSectorV64=INDEX_NONE,HoverMissionV64=INDEX_NONE;
    TArray<TWeakObjectPtr<UTexture2D>> Historical;
    TArray<TWeakObjectPtr<UStaticMeshComponent>> MissionProps,FrontThreadV64;
    TWeakObjectPtr<UMaterialInstanceDynamic> FrontInkV64;
    // Positions depend on the campaign, not on research/hover/localized labels.
    FGuid GeometryCampaignV834;
    int32 GeometryFactionV834=INDEX_NONE,GeometryMissionV834=INDEX_NONE,GeometryCountV834=INDEX_NONE;
    int32 GeometryHoverV834=INDEX_NONE;
    bool GeometryVisibleV834=false,GeometryValidV834=false;
    TArray<FVector2D> GeometryPositionsV834;
    FVector Location=FVector::ZeroVector;
    FRotator Rotation=FRotator::ZeroRotator;
    FVector2D Viewport=FVector2D(1920,1080),Offset=FVector2D::ZeroVector;
    double Scale=1.,LastEnvironmentScan=-1.,LastResidentRequest=-1.,EnteredAtV64=0.,LastSurveyRequestV75=-1.;
    bool Active=false;
    uint64 FirstRenderableFrameV8214=MAX_uint64;
};
FState Scene;
struct FContinuePreloadV8214
{
    TWeakObjectPtr<AWarfareFrontlineController> Owner;
    TSharedPtr<FStreamableHandle> Handle;
    bool Requested=false,Completed=false,Failed=false;
};
FContinuePreloadV8214 ContinuePreloadV8214;
#include "WarfareFrontlineSurveyV62.inl"
TStrongObjectPtr<UTexture2D> OperationArtworkV62;
int32 OperationArtworkIndexV62=INDEX_NONE;
TStrongObjectPtr<UTextureRenderTarget2D> LoadingViewV62;
TWeakObjectPtr<AWarfareFrontlineController> LoadingOwnerV62;
FGuid LoadingCampaignV63;
int32 LoadingMissionV63=INDEX_NONE;

FVector WorldPoint(FVector2D UV,double Z=0.)
{return FVector((UV.X-.5)*Width,(UV.Y-.5)*Height,TableZ+Z);}
FVector2D GeoUV(FVector2D LonLat)
{
    const auto Merc=[](double Lat){return FMath::Loge(FMath::Tan(PI/4.+FMath::DegreesToRadians(Lat)/2.));};
    return FVector2D((LonLat.X+2.5)/12.5,(Merc(52.)-Merc(LonLat.Y))/(Merc(52.)-Merc(46.4)));
}
template<class T>T* Asset(const FString& Name)
{return LoadObject<T>(nullptr,*(FString(AssetRoot)+Name+TEXT(".")+Name));}

void Restore()
{
    for(const auto& Entry:Scene.HiddenComponents)if(auto* C=Entry.Component.Get())C->SetVisibility(Entry.Visible,false);
    for(const auto& Entry:Scene.HiddenActors)if(auto* A=Entry.Actor.Get())A->SetActorHiddenInGame(Entry.Hidden);
    for(const auto& Entry:Scene.Grades)if(auto* P=Entry.Volume.Get())P->bEnabled=Entry.Enabled;
    Scene.HiddenComponents.Reset();Scene.HiddenActors.Reset();Scene.Grades.Reset();
    if(auto* PC=Scene.Owner.Get())
    {
        if(auto* Previous=Scene.PreviousView.Get())PC->SetViewTarget(Previous);
        if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }
    Scene.PreviousView.Reset();Scene.Active=false;
    if(auto* A=Scene.Root.Get())A->SetActorHiddenInGame(true);
    if(auto* A=Scene.Camera.Get())A->SetActorHiddenInGame(true);
    UE_LOG(LogTemp,Display,TEXT("WF_V61_MAP_SCENE restored battlefield visibility and view"));
}
void DestroyScene()
{
    if(Scene.Active)Restore();
    if(auto* A=Scene.Root.Get())A->Destroy();
    if(auto* A=Scene.Camera.Get())A->Destroy();
    Scene=FState();
    ContinuePreloadV8214=FContinuePreloadV8214();
}
void HideEnvironment()
{
    UWorld* World=Scene.World.Get();if(!World)return;
    for(TActorIterator<AActor> It(World);It;++It)
    {
        AActor* A=*It;if(A->ActorHasTag(OwnerTag))continue;
        if(auto* P=Cast<APostProcessVolume>(A))
        {
            if(!Scene.Grades.ContainsByPredicate([P](const FDisabledGrade& E){return E.Volume.Get()==P;}))
                Scene.Grades.Add({P,bool(P->bEnabled)});
            P->bEnabled=false;
        }
        TInlineComponentArray<USceneComponent*> Components(A);
        bool Environment=A->ActorHasTag(TEXT("WF_DistantMatte_V12"))||A->GetName().Contains(TEXT("SkySphere"));
        for(auto* C:Components)
        {
            if(!C->IsA<ULightComponentBase>()&&!C->IsA<UExponentialHeightFogComponent>()
                &&!C->IsA<USkyAtmosphereComponent>()&&!C->IsA<UVolumetricCloudComponent>())continue;
            Environment=true;
            if(!Scene.HiddenComponents.ContainsByPredicate([C](const FHiddenComponent& E){return E.Component.Get()==C;}))
                Scene.HiddenComponents.Add({C,C->GetVisibleFlag()});
            C->SetVisibility(false,false);
        }
        if(Environment)
        {
            if(!Scene.HiddenActors.ContainsByPredicate([A](const FHiddenActor& E){return E.Actor.Get()==A;}))
                Scene.HiddenActors.Add({A,A->IsHidden()});
            A->SetActorHiddenInGame(true);
        }
    }
}

UStaticMeshComponent* Piece(AActor* Owner,UStaticMesh* Mesh,UMaterialInterface* Material,
    const TCHAR* Name,FVector Position,FVector Size,FRotator Rotation=FRotator::ZeroRotator)
{
    auto* C=NewObject<UStaticMeshComponent>(Owner,FName(Name),RF_Transient);
    Owner->AddInstanceComponent(C);C->SetupAttachment(Owner->GetRootComponent());
    C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetMaterial(0,Material);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);
    C->SetCanEverAffectNavigation(false);C->bReceivesDecals=false;
    C->SetRelativeLocation(Position);C->SetRelativeRotation(Rotation);C->SetRelativeScale3D(Size/100.);
    C->RegisterComponent();return C;
}

// Explicit UVs avoid dependence on the Engine plane's orientation. Four
// vertices only: paper folds are material detail, not subdivided world terrain.
UStaticMesh* PaperMesh(AActor* Owner,UMaterialInterface* Material)
{
    FMeshDescription D;FStaticMeshAttributes A(D);A.Register();A.GetVertexInstanceUVs().SetNumChannels(1);
    const FPolygonGroupID Group=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Paper");
    const FVector3f Positions[]={{-50,-50,0},{50,-50,0},{-50,50,0},{50,50,0}};
    const FVector2f UV[]={{0,0},{1,0},{0,1},{1,1}};
    FVertexID V[4];for(int32 I=0;I<4;++I){V[I]=D.CreateVertex();A.GetVertexPositions()[V[I]]=Positions[I];}
    for(const auto& Indices:{FIntVector(0,2,1),FIntVector(1,2,3)})
    {
        TArray<FVertexInstanceID> Corners;
        for(int32 I:{Indices.X,Indices.Y,Indices.Z})
        {
            const auto VI=D.CreateVertexInstance(V[I]);Corners.Add(VI);
            A.GetVertexInstanceNormals()[VI]=FVector3f(0,0,1);A.GetVertexInstanceTangents()[VI]=FVector3f(1,0,0);
            A.GetVertexInstanceBinormalSigns()[VI]=1;A.GetVertexInstanceColors()[VI]=FVector4f(1,1,1,1);
            A.GetVertexInstanceUVs().Set(VI,0,UV[I]);
        }
        D.CreateTriangle(Group,Corners);
    }
    auto* Mesh=NewObject<UStaticMesh>(Owner,NAME_None,RF_Transient);Mesh->bSupportRayTracing=true;
    FStaticMaterial Slot(Material,TEXT("Paper"));Slot.UVChannelData=FMeshUVChannelInfo(100.f);
    Slot.UVChannelData.bOverrideDensities=true;Slot.UVChannelData.LocalUVDensities[0]=100.f;
    Mesh->GetStaticMaterials().Add(Slot);
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=true;Params.bBuildSimpleCollision=false;
    Params.bAllowCpuAccess=false;Params.bMarkPackageDirty=false;
    TArray<const FMeshDescription*> Descriptions;Descriptions.Add(&D);
    return Mesh->BuildFromMeshDescriptions(Descriptions,Params)?Mesh:nullptr;
}
FSheet MakeSheet(AActor* Root,UStaticMesh* Mesh,UMaterialInterface* Parent,const TCHAR* Name,float Z)
{
    FSheet S;auto* M=UMaterialInstanceDynamic::Create(Parent,Root);S.Material=M;
    M->SetScalarParameterValue(TEXT("SectorAlpha"),0);M->SetScalarParameterValue(TEXT("OperationAlpha"),0);
    M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::White);
    S.Surface=Piece(Root,Mesh,M,Name,FVector(0,0,Z),FVector(Width,Height,100));
    // One opaque paper surface samples all three geographic levels.
    S.Surface->SetCastShadow(false);S.Surface->SetVisibility(false);return S;
}
void SetBounds(UMaterialInstanceDynamic* Material,FName Parameter,const FMapRect& Bounds)
{
    if(Material)Material->SetVectorParameterValue(Parameter,FLinearColor(Bounds.Min.X,Bounds.Min.Y,Bounds.Max.X,Bounds.Max.Y));
}
bool BindSheet(FSheet& Sheet,const FString& Name,int32 Index,FName Parameter)
{
    if(Sheet.Index==Index&&Sheet.Texture.IsValid())return true;
    auto* T=Asset<UTexture2D>(Name);if(!T||!T->GetResource())return false;
    if(auto* M=Sheet.Material.Get())M->SetTextureParameterValue(Parameter,T);else return false;
    Sheet.Texture=T;Sheet.Index=Index;T->SetForceMipLevelsToBeResident(8.f);
    UE_LOG(LogTemp,Display,TEXT("WF_V61_MAP_TEXTURE asset=%s size=%dx%d mips=%d resident=%d"),
        *T->GetPathName(),T->GetSizeX(),T->GetSizeY(),T->GetNumMips(),T->GetNumResidentMips());
    return true;
}

bool Create(AWarfareFrontlineController* PC)
{
    const double Began=FPlatformTime::Seconds();
    auto* World=PC?PC->GetWorld():nullptr;if(!World)return false;
    auto* Paper=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Campaign/V77/M_Atlas_V77.M_Atlas_V77"));
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    auto* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Vehicles/V37/M_Vehicle_Wood.M_Vehicle_Wood"));
    auto* Brass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Characters/V12/Materials/M_V12_DullBrass.M_V12_DullBrass"));
    if(!Paper||!Cube||!Cylinder||!Sphere||!Wood||!Brass)return false;
    Scene.Owner=PC;Scene.World=World;
    auto* Root=World->SpawnActor<AActor>();if(!Root)return false;
    Scene.Root=Root;Root->Tags.Add(OwnerTag);Root->SetActorLocation(FVector(0,0,TableZ));
    auto* RootComponent=NewObject<USceneComponent>(Root,TEXT("MapTable"),RF_Transient);
    Root->AddInstanceComponent(RootComponent);Root->SetRootComponent(RootComponent);RootComponent->RegisterComponent();
    Root->SetActorLocation(FVector(0,0,TableZ));
    auto* Mesh=PaperMesh(Root,Paper);if(!Mesh){DestroyScene();return false;}
    Scene.Atlas=MakeSheet(Root,Mesh,Paper,TEXT("Atlas"),0);
    auto* Theatre=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/Campaign/V64/T_WesternAtlas_V64.T_WesternAtlas_V64"));
    if(!Theatre){DestroyScene();return false;}
    Scene.Atlas.Texture=Theatre;Scene.Atlas.Index=0;Scene.Atlas.Material->SetTextureParameterValue(TEXT("Atlas"),Theatre);
    Scene.Atlas.Surface->SetVisibility(true);
    Piece(Root,Cube,Wood,TEXT("WalnutDesk"),FVector(0,0,-14),FVector(4200,3000,26));
    Piece(Root,Cube,Wood,TEXT("NorthRail"),FVector(0,-840,3),FVector(2360,25,24));
    Piece(Root,Cube,Wood,TEXT("SouthRail"),FVector(0,840,3),FVector(2360,25,24));
    Piece(Root,Cube,Wood,TEXT("WestRail"),FVector(-1170,0,3),FVector(25,1680,24));
    Piece(Root,Cube,Wood,TEXT("EastRail"),FVector(1170,0,3),FVector(25,1680,24));
    // Low-profile physical objects provide depth without hiding the survey.
    Piece(Root,Cube,Brass,TEXT("SurveyRule"),FVector(-675,805,2),FVector(650,11,2),FRotator(0,-4,0));
    Piece(Root,Cylinder,Brass,TEXT("CompassCase"),FVector(1065,790,8),FVector(64,64,10));
    for(int32 I=0;I<20;++I)
    {
        const FString Prefix=FString::Printf(TEXT("Army%d"),I);
        auto* Token=Piece(Root,Cube,Wood,*(Prefix+TEXT("Block")),FVector::ZeroVector,FVector(8,5,1.5));Scene.MissionProps.Add(Token);
        auto* Pin=Piece(Root,Cylinder,Brass,*(Prefix+TEXT("Pin")),FVector::ZeroVector,FVector(.30,.30,4.8));Scene.MissionProps.Add(Pin);
        auto* Head=Piece(Root,Sphere,Brass,*(Prefix+TEXT("Head")),FVector::ZeroVector,FVector(1.6,1.6,1.6));Scene.MissionProps.Add(Head);
        auto* Flag=Piece(Root,Cube,Brass,*(Prefix+TEXT("Flag")),FVector::ZeroVector,FVector(2.8,.22,1.6));Scene.MissionProps.Add(Flag);
    }
    // A dark red plotting cord rests on the document, with real light and
    // depth. Its geographic path follows the operation's historical snapshot.
    auto* InkParent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Campaign/V64/M_MapInk_V64.M_MapInk_V64"));
    if(InkParent)
    {
        Scene.FrontInkV64=UMaterialInstanceDynamic::Create(InkParent,Root);
        for(int32 I=0;I<32;++I)
        {auto* P=Piece(Root,Cylinder,Scene.FrontInkV64.Get(),*FString::Printf(TEXT("FrontCord%d"),I),FVector::ZeroVector,FVector(.8,.8,1));P->SetCastShadow(false);Scene.FrontThreadV64.Add(P);}
    }
    const FVector LightPositions[]={FVector(-700,150,1000),FVector(700,-300,1100)};
    for(int32 I=0;I<2;++I)
    {
        auto* Light=NewObject<UPointLightComponent>(Root,*FString::Printf(TEXT("DeskLight%d"),I),RF_Transient);
        Root->AddInstanceComponent(Light);Light->SetupAttachment(RootComponent);Light->SetMobility(EComponentMobility::Movable);
        Light->bUseInverseSquaredFalloff=false;Light->SetIntensity(I?1.0f:3.5f);Light->SetAttenuationRadius(6000);
        Light->SetLightColor(I?FLinearColor(.80f,.87f,1.f):FLinearColor(1.f,.94f,.83f));
        Light->SetSourceRadius(I?75.f:35.f);
        Light->SetCastShadows(I==0);Light->SetIndirectLightingIntensity(0);Light->SetVolumetricScatteringIntensity(0);
        Light->SetRelativeLocation(LightPositions[I]);Light->RegisterComponent();
    }
    auto* Camera=World->SpawnActor<ACameraActor>();if(!Camera){DestroyScene();return false;}
    Scene.Camera=Camera;Camera->Tags.Add(OwnerTag);auto* C=Camera->GetCameraComponent();
    C->ProjectionMode=ECameraProjectionMode::Perspective;C->FieldOfView=FOV;C->bConstrainAspectRatio=false;
    C->bOverrideAspectRatioAxisConstraint=true;C->AspectRatioAxisConstraint=AspectRatio_MaintainXFOV;
    auto& PP=C->PostProcessSettings;C->PostProcessBlendWeight=1.f;
    PP.bOverride_AutoExposureMethod=true;PP.AutoExposureMethod=AEM_Manual;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP.AutoExposureApplyPhysicalCameraExposure=false;
    PP.bOverride_AutoExposureBias=true;PP.AutoExposureBias=0;
    PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;
    PP.bOverride_DepthOfFieldFstop=true;PP.DepthOfFieldFstop=32;
    PP.bOverride_DepthOfFieldFocalDistance=true;PP.DepthOfFieldFocalDistance=0;
    PP.bOverride_DepthOfFieldScale=true;PP.DepthOfFieldScale=0;
    PP.bOverride_BloomIntensity=true;PP.BloomIntensity=.08f;
    PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=.13f;
    PP.bOverride_DynamicGlobalIlluminationMethod=true;PP.DynamicGlobalIlluminationMethod=EDynamicGlobalIlluminationMethod::None;
    PP.bOverride_ReflectionMethod=true;PP.ReflectionMethod=EReflectionMethod::ScreenSpace;
    PP.bOverride_AmbientOcclusionIntensity=true;PP.AmbientOcclusionIntensity=.45f;
    PP.bOverride_AmbientOcclusionRadius=true;PP.AmbientOcclusionRadius=25;
    static bool CleanupBound=false;
    if(!CleanupBound)
    {
        FWorldDelegates::OnWorldCleanup.AddLambda([](UWorld* W,bool,bool){if(Scene.World.Get()==W)DestroyScene();});CleanupBound=true;
    }
    Root->SetActorHiddenInGame(true);
    UE_LOG(LogTemp,Display,TEXT("WF_CONTINUE_V8214 map_create_ms=%.3f primitives=%d"),
        (FPlatformTime::Seconds()-Began)*1000.,Root->GetInstanceComponents().Num());
    return true;
}
}

void PreloadContinueV8214(AWarfareFrontlineController* PC)
{
    if(!PC)return;
    auto& Cache=ContinuePreloadV8214;
    if(Cache.Owner.Get()!=PC)Cache=FContinuePreloadV8214();
    Cache.Owner=PC;if(Cache.Requested)return;Cache.Requested=true;
    TArray<FSoftObjectPath> Paths;
    for(const TCHAR* Path:{
        TEXT("/Game/Warfare/Campaign/V77/M_Atlas_V77.M_Atlas_V77"),
        TEXT("/Game/Warfare/Campaign/V64/T_WesternAtlas_V64.T_WesternAtlas_V64"),
        TEXT("/Game/Warfare/Campaign/V64/M_MapInk_V64.M_MapInk_V64"),
        TEXT("/Game/Warfare/Vehicles/V37/M_Vehicle_Wood.M_Vehicle_Wood"),
        TEXT("/Game/Warfare/Characters/V12/Materials/M_V12_DullBrass.M_V12_DullBrass"),
        TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),TEXT("/Engine/BasicShapes/Sphere.Sphere")})Paths.Emplace(Path);
    for(int32 I=0;I<6;++I)Paths.Emplace(FString::Printf(TEXT("/Game/Warfare/Campaign/V75/T_Historical_%d_V75.T_Historical_%d_V75"),I,I));
    const TWeakObjectPtr<AWarfareFrontlineController> Owner(PC);
    Cache.Handle=UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,[Paths,Owner]()
    {
        auto& Loaded=ContinuePreloadV8214;if(Loaded.Owner!=Owner)return;
        for(const auto& Path:Paths)if(!Path.ResolveObject())
        {Loaded.Failed=true;UE_LOG(LogTemp,Warning,TEXT("WF_CONTINUE_V8214 preload_missing=%s"),*Path.ToString());}
        Loaded.Completed=true;
    },FStreamableManager::DefaultAsyncLoadPriority,false,false,TEXT("WarfareContinueTableV8214"));
    if(!Cache.Handle.IsValid()){Cache.Completed=true;Cache.Failed=true;}
}
bool ContinuePreloadSettledV8214(AWarfareFrontlineController* PC)
{return ContinuePreloadV8214.Owner.Get()==PC&&ContinuePreloadV8214.Completed;}
bool ContinuePreloadReadyV8214(AWarfareFrontlineController* PC)
{return ContinuePreloadSettledV8214(PC)&&!ContinuePreloadV8214.Failed;}
bool ContinueViewWarmedV8214(AWarfareFrontlineController* PC)
{
    const auto* Client=PC&&PC->GetWorld()?PC->GetWorld()->GetGameViewport():nullptr;
    return IsActive(PC)&&Client&&!Client->bDisableWorldRendering
        &&Scene.FirstRenderableFrameV8214!=MAX_uint64&&GFrameCounter>Scene.FirstRenderableFrameV8214+1;
}

FMapRect SectorBounds(int32 Sector){return Sectors[FMath::Clamp(Sector,0,4)];}
FVector2D SectorFocus(int32 Sector)
{
    const int32 Sheet[]={0,1,2,3,5};
    const FVector2D Centres[]={{2.91,50.80},{2.86,50.29},{2.64,49.88},{3.21,50.09},{3.55,50.20}};
    return GeoUV(Centres[FMath::Clamp(Sector,0,4)]);
}
FMapRect OperationBounds(AWarfareFrontlineController* PC,int32 Mission)
{
    const auto* B=PC&&PC->GetWorld()?Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode()):nullptr;
    const auto N=FrontlineCampaignV52::GetNode(B,Mission);const FVector2D C=N.Mission?GeoUV(N.LonLat):FVector2D(.44,.32);
    return {C-FVector2D(.025,.025),C+FVector2D(.025,.025)};
}
bool IsActive(AWarfareFrontlineController* PC)
{return PC&&Scene.Owner.Get()==PC&&Scene.Active&&Scene.Root.IsValid()&&Scene.Camera.IsValid();}
bool Update(AWarfareFrontlineController* PC,bool Active,FVector2D CentreUV,float Zoom,float FlightPhase,int32 Sector,int32 Mission,float Detail)
{
    if(!PC)return false;
    if((Scene.Owner.IsValid()||Scene.World.IsValid()||Scene.Root.IsValid()||Scene.Camera.IsValid())
        &&(Scene.Owner.Get()!=PC||Scene.World.Get()!=PC->GetWorld()||!Scene.Root.IsValid()||!Scene.Camera.IsValid()))DestroyScene();
    if(!Active){if(Scene.Active)Restore();return false;}
    if(!Scene.Root.IsValid()&&!Create(PC))return false;
    if(!Scene.Camera.IsValid())return false;
    if(!Scene.Active)
    {
        Scene.PreviousView=PC->GetViewTarget();Scene.Active=true;Scene.Root->SetActorHiddenInGame(false);
        Scene.FirstRenderableFrameV8214=MAX_uint64;
        if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
        Scene.LastEnvironmentScan=-1.;Scene.LastResidentRequest=-1.;Scene.EnteredAtV64=FPlatformTime::Seconds();
        UE_LOG(LogTemp,Display,TEXT("WF_V61_MAP_SCENE entered true perspective main view"));
    }
    const double Now=FPlatformTime::Seconds();
    if(Now-Scene.LastEnvironmentScan>.5){HideEnvironment();Scene.LastEnvironmentScan=Now;}
    // Request a small collar beyond the camera before it is visible. Physical
    // pages remain in the engine's bounded VT cache; no full-atlas residency.
    if(Now-Scene.LastResidentRequest>.35&&Scene.Atlas.Texture.IsValid())
    {
        if(auto* Resource=Scene.Atlas.Texture->GetResource())if(auto* VT=Resource->GetVirtualTexture2DResource())
        {
            const double R=.72/FMath::Max(Zoom,1.f);
            const FVector2D A(FMath::Max(0.,CentreUV.X-R),FMath::Max(0.,CentreUV.Y-R));
            const FVector2D D(FMath::Min(1.,CentreUV.X+R),FMath::Min(1.,CentreUV.Y+R));
            int32 W=0,H=0;PC->GetViewportSize(W,H);const FVector2D Pixels(W,H);
            ENQUEUE_RENDER_COMMAND(WFAtlasPagesV64)([VT,A,D,Pixels](FRHICommandListImmediate& RHICmdList)
            {
                if(auto* Allocation=VT->AcquireAllocatedVT())
                    FModuleManager::GetModuleChecked<IRendererModule>(TEXT("Renderer")).RequestVirtualTextureTiles(Allocation,Pixels,FVector2D::ZeroVector,Pixels,A,D,-1);
            });
        }
        Scene.LastResidentRequest=Now;
    }
    Zoom=FMath::Clamp(Zoom,1.f,25.f);Detail=FMath::Clamp(Detail,0.f,1.f);
    Scene.Atlas.Material->SetScalarParameterValue(TEXT("MapZoomV75"),Zoom);
    // V64: one map object, one georeferenced virtual texture. Camera distance
    // selects native pages/mips; no darkening, second paper, cross-fade or
    // independently authored operation photograph participates in this view.
    Scene.Atlas.Surface->SetVisibility(true);
    PrefetchV75(PC,CentreUV,Zoom);
    auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
    const auto* Campaign=B?B->GetCurrentCampaign():nullptr;
    const FGuid CampaignId=Campaign?Campaign->Id:FGuid();
    const int32 Faction=B?int32(B->GetPlayerFaction()):INDEX_NONE;
    const int32 Count=B?B->GetCampaignMissions().Num():0;
    const bool NewGeometry=!Scene.GeometryValidV834||Scene.GeometryCampaignV834!=CampaignId
        ||Scene.GeometryFactionV834!=Faction||Scene.GeometryCountV834!=Count;
    if(NewGeometry)
    {
        Scene.GeometryPositionsV834.Reset(Count);
        for(const auto& N:FrontlineCampaignV52::Nodes(B))Scene.GeometryPositionsV834.Add(GeoUV(N.LonLat));
        Scene.GeometryCampaignV834=CampaignId;Scene.GeometryFactionV834=Faction;Scene.GeometryCountV834=Count;
    }
    const bool PropsVisible=Zoom>3.f;
    if(NewGeometry||Scene.GeometryVisibleV834!=PropsVisible||Scene.GeometryHoverV834!=Scene.HoverMissionV64)
    for(int32 I=0;I<Scene.MissionProps.Num();++I)
    {
        auto* P=Scene.MissionProps[I].Get();if(!P)continue;
        const int32 N=I/4,Part=I%4;const bool Visible=Scene.GeometryPositionsV834.IsValidIndex(N)&&PropsVisible&&Part!=0&&Part!=3;
        P->SetVisibility(Visible);if(!Visible)continue;
        const auto C=Scene.GeometryPositionsV834[N];const float K=Scene.HoverMissionV64==N?1.15f:1.f;
        const FVector Centre=WorldPoint(C)-FVector(0,0,TableZ);
        const float H=Part==0?.9f:Part==1?3.2f:Part==2?5.5f:4.9f;
        P->SetRelativeLocation(Centre+FVector(Part==3?1.6f:0,0,H));
        P->SetRelativeRotation(FRotator(0,Part==0?-8.f:0.f,0));
        const FVector Size=Part==0?FVector(3.8,2.6,.65):Part==1?FVector(.16,.16,2.3):Part==2?FVector(.80,.80,.65):FVector(2.8,.22,1.6);
        P->SetRelativeScale3D(Size*(K/100.f));
    }
    const auto* Selected=B?B->GetSelectedCampaignMission():nullptr;
    const int32 FrontMission=B&&B->GetCampaignMissions().IsValidIndex(Mission)?Mission:Selected?Selected->Id:INDEX_NONE;
    if(NewGeometry||Scene.GeometryMissionV834!=FrontMission)
    {
      const auto Front=FrontlineCampaignV52::Frontline(B,Mission);
      for(int32 I=0;I<Scene.FrontThreadV64.Num();++I)if(auto* P=Scene.FrontThreadV64[I].Get())
      {
        const bool Visible=Front.Points.IsValidIndex(I+1);P->SetVisibility(Visible);if(!Visible)continue;
        const FVector A=WorldPoint(GeoUV(Front.Points[I]),.25),D=WorldPoint(GeoUV(Front.Points[I+1]),.25);
        P->SetWorldLocation((A+D)*.5);P->SetWorldRotation(FRotationMatrix::MakeFromZ(D-A).Rotator());
        P->SetRelativeScale3D(FVector(.005,.005,FVector::Distance(A,D)/100.));
      }
    }
    Scene.GeometryValidV834=true;Scene.GeometryVisibleV834=PropsVisible;
    Scene.GeometryHoverV834=Scene.HoverMissionV64;Scene.GeometryMissionV834=FrontMission;
    int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);if(VX<1||VY<1){Restore();return false;}
    Scene.Viewport=FVector2D(VX,VY);Scene.Scale=FMath::Min(VX/1920.,VY/1080.);
    Scene.Offset=(Scene.Viewport-FVector2D(1920,1080)*Scene.Scale)*.5;
    const double Aspect=double(VX)/VY,Tan=FMath::Tan(FMath::DegreesToRadians(FOV*.5));
    const float Arc=FMath::Sin(PI*FMath::Clamp(FlightPhase,0.f,1.f));
    const float Approach=FMath::Clamp(FMath::Loge(Zoom)/FMath::Loge(20.f),0.f,1.f);
    Scene.Rotation=FRotator(FMath::Lerp(-67.f,-78.f,Approach)+Arc*1.2f,-90.f,0);
    const double Distance=ViewSpan/(Zoom*2.*Tan);
    const FVector Forward=Scene.Rotation.Vector(),Right=Scene.Rotation.RotateVector(FVector::RightVector),Up=Scene.Rotation.RotateVector(FVector::UpVector);
    const FVector2D Target=FVector2D(FMath::Lerp(960.f,660.f,FMath::SmoothStep(1.f,2.f,Zoom)),540)*Scene.Scale+Scene.Offset;
    const FVector2D NDC(Target.X/VX*2.-1.,1.-Target.Y/VY*2.);
    Scene.Location=WorldPoint(CentreUV)-Forward*Distance-Right*(NDC.X*Distance*Tan)-Up*(NDC.Y*Distance*Tan/Aspect);
    Scene.Camera->SetActorLocationAndRotation(Scene.Location,Scene.Rotation);PC->SetViewTarget(Scene.Camera.Get());
    if(auto* Client=PC->GetWorld()->GetGameViewport();Client&&!Client->bDisableWorldRendering
        &&Scene.FirstRenderableFrameV8214==MAX_uint64)Scene.FirstRenderableFrameV8214=GFrameCounter;
    return true;
}
void SetHoverV64(AWarfareFrontlineController* PC,int32 Sector,int32 Mission,FVector2D Centre,FVector2D Extent)
{
    if(!IsActive(PC))return;Scene.HoverSectorV64=Sector;Scene.HoverMissionV64=Mission;
    Scene.Atlas.Material->SetVectorParameterValue(TEXT("HoverArea"),Sector>=0?FLinearColor(Centre.X,Centre.Y,Extent.X,Extent.Y):FLinearColor(-5,-5,0,0));
}
void PrefetchV75(AWarfareFrontlineController* PC,FVector2D Centre,float Zoom)
{
    if(!PC||!Scene.Active)return;
    const double Now=FPlatformTime::Seconds();if(Now-Scene.LastSurveyRequestV75<.25)return;Scene.LastSurveyRequestV75=Now;
    if(Scene.Historical.IsEmpty())for(int32 I=0;I<6;++I)
        Scene.Historical.Add(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Campaign/V75/T_Historical_%d_V75.T_Historical_%d_V75"),I,I)));
    int32 W=0,H=0;PC->GetViewportSize(W,H);const FVector2D Pixels(W,H);
    const float R=.72f/FMath::Max(1.f,Zoom);
    for(int32 I=0;I<Scene.Historical.Num()&&I<UE_ARRAY_COUNT(SurveysV62);++I)
    {
        auto* Texture=Scene.Historical[I].Get();if(!Texture||!Texture->GetResource())continue;
        auto* VT=Texture->GetResource()->GetVirtualTexture2DResource();if(!VT)continue;
        const auto& S=SurveysV62[I];
        const FVector2D A(FMath::Max(Centre.X-R,S.Min.X),FMath::Max(Centre.Y-R,S.Min.Y));
        const FVector2D D(FMath::Min(Centre.X+R,S.Max.X),FMath::Min(Centre.Y+R,S.Max.Y));
        if(A.X>=D.X||A.Y>=D.Y)continue;
        const FVector2D Span=S.Max-S.Min,Min(S.Crop.R,S.Crop.G),Scale(S.Crop.B-S.Crop.R,S.Crop.A-S.Crop.G);
        const FVector2D SourceA=Min+(A-S.Min)/Span*Scale,SourceD=Min+(D-S.Min)/Span*Scale;
        ENQUEUE_RENDER_COMMAND(WFSurveyPagesV75)([VT,SourceA,SourceD,Pixels](FRHICommandListImmediate&)
        {
            if(auto* Allocation=VT->AcquireAllocatedVT())FModuleManager::GetModuleChecked<IRendererModule>(TEXT("Renderer")).RequestVirtualTextureTiles(Allocation,Pixels,FVector2D::ZeroVector,Pixels,SourceA,SourceD,-1);
        });
    }
}
bool IsSingleDocumentV64(AWarfareFrontlineController* PC)
{return IsActive(PC)&&Scene.Atlas.Surface.IsValid()&&Scene.Atlas.Texture.IsValid()&&Scene.Atlas.Texture->GetName()==TEXT("T_WesternAtlas_V64");}
bool IsAtlasReadyV64(AWarfareFrontlineController* PC)
{
    if(!IsSingleDocumentV64(PC))return false;
    auto* T=Scene.Atlas.Texture.Get();
#if WITH_EDITOR
    if(T->IsCompiling())return false;
#endif
    const auto* R=T->GetResource();const auto* VT=R?R->GetVirtualTexture2DResource():nullptr;
    if(!VT)return false;
    static bool Logged=false;
    if(!Logged){UE_LOG(LogTemp,Display,TEXT("WF_V64_ATLAS_READY size=%dx%d blocks=%dx%d"),VT->GetSizeX(),VT->GetSizeY(),VT->GetSizeInBlocks().X,VT->GetSizeInBlocks().Y);Logged=true;}
    return VT->GetSizeInBlocks()==FIntPoint(8,8)&&VT->GetSizeX()>=32768&&VT->GetSizeY()>=32768;
}
void SetOperationArtwork(UTexture2D* Texture,int32 Mission)
{
    if(Texture&&Texture->GetResource()){OperationArtworkV62.Reset(Texture);OperationArtworkIndexV62=Mission;Texture->SetForceMipLevelsToBeResident(12.f);}
}
bool CaptureLoadingView(AWarfareFrontlineController* PC)
{
    if(!IsActive(PC)||!Scene.Camera.IsValid())return false;
    // A rapid order before the first visible VT pages arrives uses the existing
    // faction cover fallback rather than caching the empty first frame.
    if(FPlatformTime::Seconds()-Scene.EnteredAtV64<.35)return false;
    if(auto* Atlas=Scene.Atlas.Texture.Get())
    {
#if WITH_EDITOR
        if(Atlas->IsCompiling())return false;
#endif
        if(!Atlas->GetResource()||!Atlas->GetResource()->GetVirtualTexture2DResource())return false;
    }
    else return false;
    int32 W=0,H=0;PC->GetViewportSize(W,H);if(W<1||H<1)return false;
    auto* RT=NewObject<UTextureRenderTarget2D>(PC,NAME_None,RF_Transient);
    RT->RenderTargetFormat=RTF_RGBA16f;RT->TargetGamma=1.f;RT->ClearColor=FLinearColor(.014f,.018f,.018f);
    RT->InitAutoFormat(W,H);RT->UpdateResourceImmediate(true);
    auto* Capture=NewObject<USceneCaptureComponent2D>(PC,NAME_None,RF_Transient);
    PC->AddInstanceComponent(Capture);Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
    Capture->CaptureSource=ESceneCaptureSource::SCS_FinalToneCurveHDR;Capture->TextureTarget=RT;
    Capture->FOVAngle=FOV;Capture->PostProcessSettings=Scene.Camera->GetCameraComponent()->PostProcessSettings;
    Capture->PostProcessBlendWeight=1;Capture->RegisterComponent();
    Capture->SetWorldLocationAndRotation(Scene.Location,Scene.Rotation);Capture->CaptureScene();
    Capture->DestroyComponent();LoadingViewV62.Reset(RT);LoadingOwnerV62=PC;
    const auto* B=Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode());
    const auto* R=B?B->GetCurrentCampaign():nullptr;const auto* Mission=B?B->GetSelectedCampaignMission():nullptr;
    LoadingCampaignV63=R?R->Id:FGuid();LoadingMissionV63=Mission?Mission->Id:INDEX_NONE;
    return true;
}
void RunV834GeometryChecks(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check)
{
    auto* B=PC&&PC->GetWorld()?Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode()):nullptr;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV834Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return;
    const int32 Original=B->GetCurrentCampaign()?B->GetCurrentCampaign()->SelectedMission:INDEX_NONE;
    auto Verify=[&](float Zoom,int32 Hover,int32 Mission,const TCHAR* Name)
    {
        Scene.HoverMissionV64=Hover;
        const bool Ready=Update(PC,true,FVector2D(.44,.32),Zoom,1,0,Mission,0);
        bool Good=Ready&&Scene.MissionProps.Num()==80&&Scene.FrontThreadV64.Num()==32;
        const auto Nodes=FrontlineCampaignV52::Nodes(B);
        for(int32 I=0;I<Scene.MissionProps.Num();++I)
        {
            const auto* P=Scene.MissionProps[I].Get();const int32 N=I/4,Part=I%4;
            const bool Visible=Nodes.IsValidIndex(N)&&Zoom>3.f&&Part!=0&&Part!=3;
            Good&=P&&P->GetVisibleFlag()==Visible;if(!P||!Visible)continue;
            const FVector Centre=WorldPoint(GeoUV(Nodes[N].LonLat))-FVector(0,0,TableZ);
            const FVector Position=Centre+FVector(0,0,Part==1?3.2f:5.5f);
            const FVector Scale=(Part==1?FVector(.16,.16,2.3):FVector(.80,.80,.65))*((Hover==N?1.15f:1.f)/100.f);
            Good&=P->GetRelativeLocation().Equals(Position,.00001)&&P->GetRelativeScale3D().Equals(Scale,.00001);
        }
        const auto Front=FrontlineCampaignV52::Frontline(B,Mission);
        for(int32 I=0;I<Scene.FrontThreadV64.Num();++I)
        {
            const auto* P=Scene.FrontThreadV64[I].Get();const bool Visible=Front.Points.IsValidIndex(I+1);
            Good&=P&&P->GetVisibleFlag()==Visible;if(!P||!Visible)continue;
            const FVector A=WorldPoint(GeoUV(Front.Points[I]),.25),D=WorldPoint(GeoUV(Front.Points[I+1]),.25);
            Good&=P->GetComponentLocation().Equals((A+D)*.5,.00001)
                &&P->GetComponentRotation().Equals(FRotationMatrix::MakeFromZ(D-A).Rotator(),.00001)
                &&P->GetRelativeScale3D().Equals(FVector(.005,.005,FVector::Distance(A,D)/100.),.00001);
        }
        Check(Name,Good);
    };
    Verify(2.999f,INDEX_NONE,11,TEXT("v834_map_below_zoom_threshold_matches_original"));
    Verify(3.001f,2,11,TEXT("v834_map_above_zoom_threshold_and_hover_matches_original"));
    Verify(3.001f,3,14,TEXT("v834_map_mission_and_hover_change_matches_original"));
    Verify(3.f,INDEX_NONE,14,TEXT("v834_map_exact_zoom_threshold_matches_original"));
    Update(PC,false,FVector2D(.44,.32),3.001f,1,0,14,0);
    Verify(3.001f,INDEX_NONE,14,TEXT("v834_map_return_retains_original_geometry"));
    AActor* OldRoot=Scene.Root.Get();if(OldRoot)OldRoot->Destroy();
    // Recreation resets hover in the original scene lifecycle. Exercise that
    // reset first, then apply a fresh HUD hover to the recreated scene.
    Check(TEXT("v834_map_destroyed_root_recreated_cleanly_with_hover_reset"),Update(PC,true,FVector2D(.44,.32),3.001f,1,0,11,0)
        &&Scene.Root.IsValid()&&Scene.Root.Get()!=OldRoot&&Scene.HoverMissionV64==INDEX_NONE
        &&Scene.MissionProps.Num()==80&&Scene.FrontThreadV64.Num()==32);
    Verify(3.001f,2,11,TEXT("v834_map_destroyed_root_rebuilds_all_objects_and_geometry"));
    Check(TEXT("v834_map_select_first_disposable_mission"),B->SelectCampaignMission(11));
    Verify(3.001f,INDEX_NONE,INDEX_NONE,TEXT("v834_map_invalid_id_uses_first_selected_mission"));
    Check(TEXT("v834_map_select_second_disposable_mission"),B->SelectCampaignMission(14));
    Verify(3.001f,INDEX_NONE,INDEX_NONE,TEXT("v834_map_invalid_id_refreshes_selected_mission"));
    if(Original!=INDEX_NONE)Check(TEXT("v834_map_fixture_selection_restored"),B->SelectCampaignMission(Original));
    Update(PC,false,FVector2D(.44,.32),1.f,1,0,Original,0);
}
UTextureRenderTarget2D* LoadingView(AWarfareFrontlineController* PC)
{
    const auto* B=PC?Cast<AWarfareFrontlineGameMode>(PC->GetWorld()->GetAuthGameMode()):nullptr;
    const auto* R=B?B->GetCurrentCampaign():nullptr;const auto* Mission=B?B->GetSelectedCampaignMission():nullptr;
    return LoadingOwnerV62.Get()==PC&&R&&Mission&&R->Id==LoadingCampaignV63&&Mission->Id==LoadingMissionV63?LoadingViewV62.Get():nullptr;
}
void ReleaseLoadingView(){LoadingViewV62.Reset();LoadingOwnerV62.Reset();LoadingCampaignV63.Invalidate();LoadingMissionV63=INDEX_NONE;}
FVector2D Project(AWarfareFrontlineController* PC,FVector2D UV)
{
    if(!IsActive(PC))return FVector2D(-100000,-100000);
    const FVector P=Scene.Rotation.UnrotateVector(WorldPoint(UV)-Scene.Location);
    if(P.X<=.01)return FVector2D(-100000,-100000);
    const double Tan=FMath::Tan(FMath::DegreesToRadians(FOV*.5)),Aspect=Scene.Viewport.X/Scene.Viewport.Y;
    const FVector2D Pixel((.5+.5*P.Y/(P.X*Tan))*Scene.Viewport.X,(.5-.5*P.Z/(P.X*Tan/Aspect))*Scene.Viewport.Y);
    return (Pixel-Scene.Offset)/Scene.Scale;
}
FVector2D Unproject(AWarfareFrontlineController* PC,FVector2D Screen)
{
    if(!IsActive(PC))return FVector2D(-1,-1);
    const FVector2D P=Screen*Scene.Scale+Scene.Offset;
    const double Tan=FMath::Tan(FMath::DegreesToRadians(FOV*.5)),Aspect=Scene.Viewport.X/Scene.Viewport.Y;
    const FVector Ray=Scene.Rotation.RotateVector(FVector(1,(P.X/Scene.Viewport.X*2.-1.)*Tan,(1.-P.Y/Scene.Viewport.Y*2.)*Tan/Aspect));
    if(Ray.Z>=-.00001)return FVector2D(-1,-1);
    const double T=(TableZ-Scene.Location.Z)/Ray.Z;if(T<=0)return FVector2D(-1,-1);
    const FVector Hit=Scene.Location+Ray*T;
    return FVector2D(Hit.X/Width+.5,Hit.Y/Height+.5);
}
}
