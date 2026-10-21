#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineBlastCompositionV8221.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "WarfareFrontlineEffectsSafetyV844.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineGroundContactV8449.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineParticulateV84412.h"
#include "WarfareFrontlineFXV47.h"
#include "WarfareFrontlineMedalMotionV8216.h"
#include "WarfareFrontlineFireV33.h"
#include "WarfareFrontlineWoundFX.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineWaterV51.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

namespace
{
// A short object shutter restores fast clod motion without blurring a camera
// pan or any UI. Moving infantry shares the tightly capped object shutter;
// static scenery has zero object velocity. Its fixed shutter
// reference also avoids making 0.5x / 2x or a slow render frame smear the field.
class FFragmentShutterV835 final:public FWorldSceneViewExtension
{
public:
    FFragmentShutterV835(const FAutoRegister& AutoRegister,UWorld* World)
        :FWorldSceneViewExtension(AutoRegister,World),OwnerWorld(World) {}
    TArray<TWeakObjectPtr<AWarfareFrontlineImpact>> Impacts;
    bool bObserved=false;
    int32 AppliedViews=0;
    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily& Family,FSceneView& View) override
    {
        UWorld* W=OwnerWorld.Get();
        if(!W||View.bIsSceneCapture||W->IsPaused()||!Family.EngineShowFlags.MotionBlur
            ||Family.EngineShowFlags.PathTracing)return;
        const auto* B=Cast<AWarfareFrontlineGameMode>(W->GetAuthGameMode());
        auto* PC=Cast<AWarfareFrontlineController>(UGameplayStatics::GetPlayerController(W,0));
        if(!B||B->Phase!=EFrontlinePhase::Fighting||!PC||PC->GetViewTarget()!=PC->BattleCamera.Get())return;
        bool Moving=false;
        for(int32 I=Impacts.Num()-1;I>=0;--I)
        {
            if(!Impacts[I].IsValid()){Impacts.RemoveAtSwap(I);continue;}
            Moving|=Impacts[I]->HasFastFragmentsV835();
        }
        if(!Moving)return;
        View.bCameraMotionBlur=false;
        auto& PP=View.FinalPostProcessSettings;
        PP.MotionBlurAmount=.42f;PP.MotionBlurTargetFPS=60;PP.MotionBlurMax=.25f;
        PP.MotionBlurPerObjectSize=0.f;
        ++AppliedViews;
        if(!bObserved)
        {
            bObserved=true;
            UE_LOG(LogTemp,Display,TEXT("WF_V835_OBJECT_SHUTTER actual_battle_view=1 camera_motion=0 amount=%.2f max_screen_percent=%.2f reference_fps=%d"),PP.MotionBlurAmount,PP.MotionBlurMax,PP.MotionBlurTargetFPS);
        }
    }
private:
    TWeakObjectPtr<UWorld> OwnerWorld;
};
struct FFragmentWorldV835 {TWeakObjectPtr<UWorld> World;TSharedPtr<FFragmentShutterV835,ESPMode::ThreadSafe> View;};
TArray<FFragmentWorldV835> FragmentWorldsV835;
void RegisterFragmentShutterV835(AWarfareFrontlineImpact* Impact)
{
    auto& Worlds=FragmentWorldsV835;
    Worlds.RemoveAll([](const FFragmentWorldV835& E){return !E.World.IsValid();});
    FFragmentWorldV835* E=Worlds.FindByPredicate([&](const FFragmentWorldV835& P){return P.World==Impact->GetWorld();});
    if(!E)
    {
        auto& Added=Worlds.AddDefaulted_GetRef();Added.World=Impact->GetWorld();
        Added.View=FSceneViewExtensions::NewExtension<FFragmentShutterV835>(Impact->GetWorld());E=&Added;
    }
    E->View->Impacts.RemoveAll([](const TWeakObjectPtr<AWarfareFrontlineImpact>& P){return !P.IsValid();});
    E->View->Impacts.AddUnique(Impact);
}
int32 EffectsBudgetV8211()
{
    const float Budget=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43();
    return Budget<.5f?8:(Budget<.9f?12:16);
}
bool VolumeSlotV828(int32 I,int32 Core,int32 Count,int32 Kind)
{
    // Airburst needs depth both aloft and at the struck earth. Reserving both
    // volumes for the overhead puff left all blood-bearing ground fans flat.
    if(Kind==4)return Count>0&&(I==0||(Count>1&&I==Core));
    return Kind!=4&&Count>1?(I==0||(I>=Core-(Count-1)&&I<Core)):(I>=Core-Count&&I<Core);
}
float NearGroundCasualtyLoadV8216(float Load,float T,float Window=.64f)
{
    // Incident blood is a brief admixture in outward ground dust. Feeding the
    // full casualty load into every rising jet painted the whole blast red.
    return FMath::Clamp(Load,0.f,.92f)*.64f*FMath::Clamp(T/.02f,0.f,1.f)
        *FMath::Pow(FMath::Clamp(1.f-T/Window,0.f,1.f),1.10f);
}
FVector NearGroundCasualtyBiasV8216(const TArray<FVector>& Sources,const FVector& Origin,float Limit)
{
    if(Sources.IsEmpty())return FVector::ZeroVector;
    FVector Mean=FVector::ZeroVector;for(const FVector& P:Sources)Mean+=P;
    Mean=Mean/Sources.Num()-Origin;Mean.Z=0.f;
    return Mean.GetClampedToMaxSize(Limit)*.45f;
}
struct FBlastWaterContactV831
{
    float Ground=0.f,Lift=0.f,MaximumSurface=0.f;
    int32 WetSamples=0;
    float Height() const {return Ground+Lift;}
};
float RawContactSweepMaximumV831=0.f,SmoothedContactSweepMaximumV831=0.f;
FBlastWaterContactV831 BlastWaterContactV831(const FVector& Centre,const FVector& Dimensions)
{
    FBlastWaterContactV831 Contact;
    Contact.Ground=AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y);
    Contact.MaximumSurface=Contact.Ground;
    // A dry impact can spill across a higher adjacent pool. Sample this
    // transported cloud's local footprint, not only its dry centre. Depth is
    // from the same displayed basin query; all dry footprints remain exact.
    for(int32 I=-1;I<8;++I)
    {
        const float Angle=I*PI*.25f;
        const float X=Centre.X+(I<0?0.f:FMath::Cos(Angle)*Dimensions.X*.5f);
        const float Y=Centre.Y+(I<0?0.f:FMath::Sin(Angle)*Dimensions.Y*.5f);
        float Level=0.f,Depth=0.f;
        if(!AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)
            ||!FMath::IsFinite(Level)||!FMath::IsFinite(Depth))continue;
        ++Contact.WetSamples;
        Contact.MaximumSurface=FMath::Max(Contact.MaximumSurface,Level);
        const float Lift=FMath::Max(0.f,Level-Contact.Ground)*FMath::SmoothStep(0.f,24.f,Depth);
        Contact.Lift=FMath::Max(Contact.Lift,Lift);
    }
    return Contact;
}
float WaterContactResponseV831(float Current,float Target,float VisualDelta)
{
    // Trench/rail protection can admit a deep pool without a shallow bank.
    // Respond continuously on the existing visual clock rather than snapping
    // an entire transported cloud up when a perimeter point crosses that cut.
    const float Rate=Target>Current?40.f:20.f;
    return FMath::Lerp(Current,Target,1.f-FMath::Exp(-Rate*FMath::Max(0.f,VisualDelta)));
}
bool IsV47FXReview()
{
    FString UserDir;
    return (FParse::Param(FCommandLine::Get(),TEXT("WFV47Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV828Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8214Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV831Review")))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
FRotator BlastBillboard(const AActor* Actor)
{
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(Actor,0);
    const FRotationMatrix ViewAxes(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0));
    return FRotationMatrix::MakeFromXY(ViewAxes.GetUnitAxis(EAxis::Y),ViewAxes.GetUnitAxis(EAxis::Z)).Rotator();
}
}

TSharedRef<FJsonObject> AWarfareFrontlineImpact::DescribeFragmentShutterV835(UWorld* World)
{
    auto J=MakeShared<FJsonObject>();int32 Applied=0;
    for(const auto& E:FragmentWorldsV835)if(E.World==World&&E.View)Applied+=E.View->AppliedViews;
    J->SetNumberField(TEXT("actual_battle_views_applied"),Applied);
    J->SetNumberField(TEXT("maximum_screen_percent"),.25f);
    J->SetBoolField(TEXT("camera_motion_blur"),false);
    return J;
}

AWarfareFrontlineScorchV8449::AWarfareFrontlineScorchV8449()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SurfaceScorchV8449")));
}
AWarfareFrontlineScorchV8449* AWarfareFrontlineScorchV8449::Get(UWorld* World,bool bCreate)
{
    if(!World||FParse::Param(FCommandLine::Get(),TEXT("WFV8449FieldBaseline")))return nullptr;
    auto* B=Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode());
    if(!B||B->Phase==EFrontlinePhase::Briefing||WarfareFRRUCampaignV82::Active(B))return nullptr;
    for(TActorIterator<AWarfareFrontlineScorchV8449> It(World);It;++It)if(IsValid(*It))return *It;
    return bCreate?World->SpawnActor<AWarfareFrontlineScorchV8449>():nullptr;
}
void AWarfareFrontlineScorchV8449::BeginPlay()
{
    Super::BeginPlay();
    Battle=Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode());
    for(TActorIterator<AWarfareFrontlineGround> It(GetWorld());It;++It)
        if(It->Surface&&It->Surface->GetStaticMesh()){Ground=*It;break;}
    ScorchBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84413/Materials/M_ScorchResidue_V84413.M_ScorchResidue_V84413"));
    if(!ScorchBase)ScorchBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84411/Materials/M_SurfaceScorchVerifiedMask_V84411.M_SurfaceScorchVerifiedMask_V84411"));
    if(!ScorchBase)ScorchBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84411/Materials/M_SurfaceScorch_V84411.M_SurfaceScorch_V84411"));
    if(!ScorchBase)ScorchBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8449/Materials/M_SurfaceScorch_V8449.M_SurfaceScorch_V8449"));
    State.SetNum(StampBudget);Stamps.SetNum(StampBudget);StampMaterials.SetNum(StampBudget);
    if(auto* B=Battle.Get())LastHostTime=B->Elapsed;
    if(!ScorchBase)UE_LOG(LogTemp,Warning,TEXT("WF_V8449_SCORCH_ASSET_MISSING cosmetic_receiver_disabled=1"));
}
void AWarfareFrontlineScorchV8449::ClearStamps()
{
    for(auto& C:Stamps)if(C)C->SetVisibility(false);
    for(auto& S:State)S=FFrontlineScorchStampV8449();
    Pending.Reset();Cursor=0;
}
void AWarfareFrontlineScorchV8449::QueueImpact(const FVector& Point,int32 Profile)
{
    auto* B=Battle.Get();auto* G=Ground.Get();
    if(!B||!G||!ScorchBase||Point.ContainsNaN()||Profile<0||Profile>5||Profile==3||Profile==4
        ||WarfareFRRUCampaignV82::Active(B))return;
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);
    const int32 Operation=Mission?Mission->Id:INDEX_NONE;
    if(Operation!=BoundOperation||B->Elapsed+.1f<LastHostTime){ClearStamps();BoundOperation=Operation;}
    LastHostTime=B->Elapsed;
    const float H=AWarfareFrontlineGround::HeightAt(Point.X,Point.Y);
    // Vehicle fuel fire and elevated body effects are not ground detonations.
    // Do not turn their repeated cosmetic actors into a new field of soot.
    if(!FMath::IsFinite(H)||FMath::Abs(Point.Z-H)>40.f){++RejectedAir;return;}
    float Level=0.f,Depth=0.f;
    const bool WaterVisible=G->BasinWaterV44&&G->BasinWaterV44->IsVisible()&&G->BasinWaterV44->GetStaticMesh();
    if(WaterVisible&&AWarfareFrontlineGround::QueryWaterSurfaceV51(Point.X,Point.Y,Level,Depth)&&Depth>.8f)
    {++RejectedWater;return;}
    FFrontlineScorchStampV8449 Request;Request.Origin=FVector(Point.X,Point.Y,H);Request.Profile=Profile;
    Request.Radius=Profile==2?128.f:Profile==1?240.f:Profile==5?260.f:330.f;
    Request.Seed=int32(GetTypeHash(Point)&0x7fffffff)^(Queued*73);
    auto* Bound=G->BakedTerrainV43.Num()&&G->BakedTerrainV43[0]?G->BakedTerrainV43[0]->GetMaterial(0):G->Surface->GetMaterial(0);
    const FString SurfaceName=Bound?GetPathNameSafe(Bound->GetBaseMaterial()):FString();
    Request.bSnow=SurfaceName.Contains(TEXT("Winter"))||SurfaceName.Contains(TEXT("Snow"));
    // A barrage queues at most twelve small contact builds, one every .08s.
    // The oldest unbuilt request can yield; damage and existing smoke do not.
    if(Pending.Num()>=12)Pending.RemoveAt(0);
    Pending.Add(Request);++Queued;
}
bool AWarfareFrontlineScorchV8449::BuildStamp(const FFrontlineScorchStampV8449& Request)
{
    auto* G=Ground.Get();if(!G||!G->Surface||!G->Surface->GetStaticMesh()||!ScorchBase)return false;
    const double Start=FPlatformTime::Seconds();
    if(Request.Origin.ContainsNaN()||!FMath::IsFinite(Request.Radius)||Request.Radius<=0.f||Request.Radius>360.f)return false;
    constexpr float Lift=.35f;
    const bool WaterVisible=G->BasinWaterV44&&G->BasinWaterV44->IsVisible()&&G->BasinWaterV44->GetStaticMesh();
    const FVector2D Low(Request.Origin.X-Request.Radius,Request.Origin.Y-Request.Radius);
    const FVector2D High(Request.Origin.X+Request.Radius,Request.Origin.Y+Request.Radius);
    TArray<FFrontlineGroundTriangleV8449> Source;
    if(!FrontlineGroundContactV8449::TrianglesInBox(G,Low,High,Source)){++RejectedContact;return false;}
    int32 WaterPruned=0;
    // Exact double XY keys share only identical queries within this stamp;
    // no quantized shore or contact samples are substituted.
    TMap<FVector2D,bool> WaterSafety;
    struct FActualContact
    {
        FVector Point=FVector::ZeroVector,Normal=FVector::UpVector;
        bool bValid=false;
    };
    TMap<FVector2D,FActualContact> Contacts;
    const int32 MemoReserve=FMath::Min(Source.Num()*4,8192);
    WaterSafety.Reserve(MemoReserve);Contacts.Reserve(MemoReserve);
    auto SafePoint=[&](const FVector& P)
    {
        if(P.ContainsNaN())return false;
        const FVector2D Key(P.X,P.Y);
        if(const bool* Cached=WaterSafety.Find(Key))
        {if(!*Cached)++WaterPruned;return *Cached;}
        float Level=0.f,Depth=0.f;
        const bool Safe=!(WaterVisible&&AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth)&&Depth>.2f);
        WaterSafety.Add(Key,Safe);if(!Safe)++WaterPruned;return Safe;
    };
    auto ActualContact=[&](const FVector& At,FVector& Point,FVector& Normal)
    {
        const FVector2D Key(At.X,At.Y);
        if(const FActualContact* Cached=Contacts.Find(Key))
        {Point=Cached->Point;Normal=Cached->Normal;return Cached->bValid;}
        FActualContact Result;
        Result.bValid=FrontlineGroundContactV8449::Sample(G,At.X,At.Y,Result.Point,Result.Normal);
        Contacts.Add(Key,Result);Point=Result.Point;Normal=Result.Normal;return Result.bValid;
    };
    FMeshDescription D;FStaticMeshAttributes A(D);A.Register();
    auto Positions=A.GetVertexPositions();auto Normals=A.GetVertexInstanceNormals();auto Tangents=A.GetVertexInstanceTangents();
    auto Signs=A.GetVertexInstanceBinormalSigns();auto UV=A.GetVertexInstanceUVs();auto Colors=A.GetVertexInstanceColors();UV.SetNumChannels(1);
    const FPolygonGroupID Group=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Scorch");
    int32 Triangles=0,Clipped=0;float MaxError=0.f,MinClearance=MAX_flt,MaxPenetration=0.f;
    float MinimumZ=MAX_flt,MaximumZ=-MAX_flt;bool Clockwise=true;
    // Copy only actual receiver faces under this small footprint. Intersections
    // with its rectangle stay on each source triangle's plane; no coarse grid
    // can sink between its vertices or bridge an excavated bank.
    for(const auto& Face:Source)
    {
        FVector Normal=FVector::CrossProduct(Face.Points[1]-Face.Points[0],Face.Points[2]-Face.Points[0]).GetSafeNormal();
        if(Normal.Z<0.f)Normal=-Normal;
        if(Normal.ContainsNaN()||Normal.Z<.40f){++Clipped;continue;}
        TArray<FVector,TInlineAllocator<8>> Polygon;
        for(const auto& P:Face.Points)Polygon.Add(P);
        auto Cut=[&](int32 Axis,double Edge,bool KeepAbove)
        {
            if(Polygon.IsEmpty())return;
            TArray<FVector,TInlineAllocator<8>> Result;FVector Previous=Polygon.Last();
            bool PreviousInside=KeepAbove?Previous[Axis]>=Edge:Previous[Axis]<=Edge;
            for(const FVector& Current:Polygon)
            {
                const bool Inside=KeepAbove?Current[Axis]>=Edge:Current[Axis]<=Edge;
                if(Inside!=PreviousInside)
                {
                    const double Denominator=Current[Axis]-Previous[Axis];
                    if(FMath::Abs(Denominator)>UE_SMALL_NUMBER)
                    {const double T=FMath::Clamp((Edge-Previous[Axis])/Denominator,0.,1.);Result.Add(Previous+(Current-Previous)*T);}
                }
                if(Inside)Result.Add(Current);
                Previous=Current;PreviousInside=Inside;
            }
            Polygon=MoveTemp(Result);
        };
        Cut(0,Low.X,true);Cut(0,High.X,false);Cut(1,Low.Y,true);Cut(1,High.Y,false);
        if(Polygon.Num()<3){++Clipped;continue;}
        for(int32 Corner=1;Corner+1<Polygon.Num();++Corner)
        {
            FVector P[3]={Polygon[0],Polygon[Corner],Polygon[Corner+1]};
            double Winding=(P[1].X-P[0].X)*(P[2].Y-P[0].Y)-(P[1].Y-P[0].Y)*(P[2].X-P[0].X);
            if(FMath::Abs(Winding)<.00001){++Clipped;continue;}
            if(Winding>0.)Swap(P[1],P[2]); // UE visible ground is clockwise.
            const FVector Centre=(P[0]+P[1]+P[2])/3.f;
            bool Supported=SafePoint(P[0])&&SafePoint(P[1])&&SafePoint(P[2]);
            float LocalError=0.f,LocalClearance=MAX_flt,LocalPenetration=0.f;
            const FVector Witness[4]={Centre,(P[0]+P[1])*.5f,(P[1]+P[2])*.5f,(P[2]+P[0])*.5f};
            for(const FVector& At:Witness)
            {
                if(!Supported)break;
                FVector GroundPoint=FVector::ZeroVector,GroundNormal=FVector::UpVector;
                Supported=SafePoint(At)&&ActualContact(At,GroundPoint,GroundNormal);
                if(!Supported)break;
                const float Error=FMath::Abs(GroundPoint.Z-At.Z);
                const float Clearance=At.Z+Lift-GroundPoint.Z;
                LocalError=FMath::Max(LocalError,Error);LocalClearance=FMath::Min(LocalClearance,Clearance);
                LocalPenetration=FMath::Max(LocalPenetration,FMath::Max(0.f,-Clearance));
                Supported=Error<=1.2f&&Clearance>=.30f;
            }
            if(!Supported){++Clipped;continue;}
            for(const FVector& Point:P){MinimumZ=FMath::Min(MinimumZ,float(Point.Z));MaximumZ=FMath::Max(MaximumZ,float(Point.Z));}
            TArray<FVertexInstanceID,TInlineAllocator<3>> Corners;
            for(int32 I=0;I<3;++I)
            {
                const auto V=D.CreateVertex();Positions[V]=FVector3f(P[I]+FVector(0,0,Lift)-Request.Origin);
                const auto VI=D.CreateVertexInstance(V);Normals[VI]=FVector3f(Normal);
                Tangents[VI]=FVector3f(FVector::VectorPlaneProject(FVector::ForwardVector,Normal).GetSafeNormal());Signs[VI]=1.f;
                const FVector Local=P[I]-Request.Origin;
                UV.Set(VI,0,FVector2f(Local.X/(2.f*Request.Radius)+.5f,Local.Y/(2.f*Request.Radius)+.5f));
                Colors[VI]=FVector4f(1,1,1,1);Corners.Add(VI);
            }
            D.CreateTriangle(Group,Corners);++Triangles;
            Clockwise&=((P[1].X-P[0].X)*(P[2].Y-P[0].Y)-(P[1].Y-P[0].Y)*(P[2].X-P[0].X))<0.;
            MaxError=FMath::Max(MaxError,LocalError);MinClearance=FMath::Min(MinClearance,LocalClearance);MaxPenetration=FMath::Max(MaxPenetration,LocalPenetration);
        }
    }
    // A valid coarse receiver face is still real ground. Requiring eight
    // faces discarded complete stamps on flatter low-density terrain.
    if(Triangles<1||Triangles>Source.Num()*5||!Clockwise){++RejectedContact;return false;}
    // A second hit at the same contact replaces its old residue rather than
    // multiplying translucent black layers indefinitely.
    int32 Slot=Cursor%StampBudget;
    for(int32 I=0;I<State.Num();++I)if(State[I].Triangles>0&&State[I].bSnow==Request.bSnow
        &&FVector::DistSquared2D(State[I].Origin,Request.Origin)<FMath::Square(FMath::Min(State[I].Radius,Request.Radius)*.62f)
        &&FMath::Abs(State[I].Origin.Z-Request.Origin.Z)<20.f){Slot=I;break;}
    auto* M=UMaterialInstanceDynamic::Create(ScorchBase,this);
    M->SetScalarParameterValue(TEXT("Seed"),float(Request.Seed%317));
    M->SetScalarParameterValue(TEXT("Snow"),Request.bSnow?1.f:0.f);
    const auto* Mission=Battle.IsValid()?FrontlineCampaignV32::CurrentOperation(Battle.Get()):nullptr;
    M->SetScalarParameterValue(TEXT("Wetness"),Mission?FMath::Clamp(Mission->Wetness,0.f,1.f):.6f);
    // The opaque geometry witness rendered correctly, while both mesh-decal
    // and translucent-surface routes did not. Reuse the ground decal route
    // already used by blood and running gear; let the actual visible receiver
    // supply its shading and geometry. The checked patch bounds its depth.
    if(!FMath::IsFinite(MinimumZ)||!FMath::IsFinite(MaximumZ)){++RejectedContact;return false;}
    const float HalfDepth=FMath::Max(4.f,(MaximumZ-MinimumZ)*.5f+2.f);
    auto* C=Stamps[Slot].Get();
    if(!C)
    {
        C=NewObject<UDecalComponent>(this,*FString::Printf(TEXT("ScorchContact_%d"),Slot));C->SetupAttachment(RootComponent);
        C->SetMobility(EComponentMobility::Movable);C->SetFadeScreenSize(.00025f);C->SetSortOrder(1);C->RegisterComponent();Stamps[Slot]=C;
    }
    C->SetVisibility(false);C->SetDecalMaterial(M);
    C->SetWorldLocation(FVector(Request.Origin.X,Request.Origin.Y,(MinimumZ+MaximumZ)*.5f));
    C->SetWorldRotation(FRotationMatrix::MakeFromXZ(FVector::DownVector,FVector::ForwardVector).Rotator());
    C->SetWorldScale3D(FVector::OneVector);C->DecalSize=FVector(HalfDepth,Request.Radius,Request.Radius);
    C->MarkRenderStateDirty();C->SetVisibility(true);
    StampMaterials[Slot]=M;State[Slot]=Request;State[Slot].Triangles=Triangles;State[Slot].ClippedTriangles=Clipped;
    State[Slot].WaterSamplesPruned=WaterPruned;State[Slot].MaximumSupportError=MaxError;
    State[Slot].SourceTriangles=Source.Num();State[Slot].MinimumClearance=MinClearance;
    State[Slot].MaximumPenetration=MaxPenetration;State[Slot].bClockwiseVerified=Clockwise;
    State[Slot].ProjectionHalfDepthV84411=HalfDepth;
    State[Slot].MaximumBuildMilliseconds=float((FPlatformTime::Seconds()-Start)*1000.);++Cursor;
    if(Cursor<=2)UE_LOG(LogTemp,Display,TEXT("WF_V8449_SCORCH_BOUND slot=%d profile=%d radius_cm=%.1f triangles=%d clipped=%d support_error_cm=%.3f lift_cm=.35 build_ms=%.3f snow=%d material=%s"),
        Slot,Request.Profile,Request.Radius,Triangles,Clipped,MaxError,State[Slot].MaximumBuildMilliseconds,int32(Request.bSnow),*GetPathNameSafe(ScorchBase));
    return true;
}
void AWarfareFrontlineScorchV8449::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);auto* B=Battle.Get();if(!B||WarfareFRRUCampaignV82::Active(B))return;
    if(B->Phase==EFrontlinePhase::Briefing){ClearStamps();LastHostTime=B->Elapsed;return;}
    if(B->Elapsed+.1f<LastHostTime)ClearStamps();LastHostTime=B->Elapsed;
    BuildWait=FMath::Max(0.f,BuildWait-FMath::Max(0.f,DeltaSeconds));
    if(!Pending.IsEmpty()&&BuildWait<=0.f)
    {const auto Next=Pending[0];Pending.RemoveAt(0);BuildStamp(Next);BuildWait=.08f;}
}
TSharedRef<FJsonObject> AWarfareFrontlineScorchV8449::Describe() const
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("material"),GetPathNameSafe(ScorchBase));
    const auto* R=ScorchBase?ScorchBase->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
    const auto* ShaderMap=R?R->GetGameThreadShaderMap():nullptr;
    J->SetBoolField(TEXT("material_shader_complete"),R&&R->IsGameThreadShaderMapComplete());
    J->SetBoolField(TEXT("material_shader_valid"),ShaderMap&&ShaderMap->IsValidForRendering());
    J->SetBoolField(TEXT("material_deferred_decal"),ScorchBase&&ScorchBase->GetMaterial()&&ScorchBase->GetMaterial()->MaterialDomain==MD_DeferredDecal);
    J->SetBoolField(TEXT("material_native_ground_decal_v84411"),ScorchBase&&ScorchBase->GetMaterial()
        &&ScorchBase->GetMaterial()->MaterialDomain==MD_DeferredDecal&&ScorchBase->GetBlendMode()==BLEND_Translucent);
    J->SetNumberField(TEXT("stamp_budget"),StampBudget);J->SetNumberField(TEXT("pending"),Pending.Num());J->SetNumberField(TEXT("queued"),Queued);
    J->SetNumberField(TEXT("rejected_air"),RejectedAir);J->SetNumberField(TEXT("rejected_water"),RejectedWater);J->SetNumberField(TEXT("rejected_contact"),RejectedContact);
    J->SetNumberField(TEXT("support_lift_cm"),.35f);J->SetBoolField(TEXT("damage_or_cover_changed"),false);
    J->SetBoolField(TEXT("actual_source_triangles"),true);J->SetBoolField(TEXT("uniform_vertical_lift"),false);
    J->SetBoolField(TEXT("checked_contact_patch_vertical_lift"),true);
    J->SetStringField(TEXT("render_route"),TEXT("Native ground decal; source triangles bound the projection, not a visible overlay mesh."));
    J->SetObjectField(TEXT("actual_ground_receiver"),FrontlineGroundContactV8449::Describe(Ground.Get()));
    if(auto* CV=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DBuffer")))J->SetNumberField(TEXT("actual_dbuffer_cvar"),CV->GetInt());
    TArray<TSharedPtr<FJsonValue>> Receivers;
    if(const auto* G=Ground.Get())for(const auto& C:G->BakedTerrainV43)if(C&&C->IsVisible())
    {
        auto V=MakeShared<FJsonObject>();V->SetBoolField(TEXT("receives_decals"),C->bReceivesDecals);
        const auto* MI=C->GetMaterial(0);const auto* Base=MI?MI->GetMaterial():nullptr;
        V->SetStringField(TEXT("actual_base_material"),GetPathNameSafe(Base));
        V->SetNumberField(TEXT("actual_decal_response"),Base?int32(Base->MaterialDecalResponse):INDEX_NONE);
        Receivers.Add(MakeShared<FJsonValueObject>(V));
    }
    J->SetArrayField(TEXT("actual_visible_receiver_decal_contract"),Receivers);
    J->SetBoolField(TEXT("legacy_dry_trace_checked"),bLegacyDryTraceChecked);
    J->SetBoolField(TEXT("legacy_dry_trace_hit"),bLegacyDryTraceHit);
    J->SetNumberField(TEXT("legacy_dry_trace_normal_z"),LegacyDryTraceNormalZ);
    J->SetNumberField(TEXT("legacy_dry_trace_height_error_cm"),LegacyDryTraceErrorCm);
    TArray<TSharedPtr<FJsonValue>> Rows;const FFrontlineScorchStampV8449* Focus=nullptr;
    for(int32 I=0;I<State.Num();++I)if(State[I].Triangles>0&&Stamps[I]&&Stamps[I]->IsVisible())
    {
        auto Row=MakeShared<FJsonObject>();const auto& S=State[I];Row->SetNumberField(TEXT("slot"),I);Row->SetNumberField(TEXT("profile"),S.Profile);
        Row->SetStringField(TEXT("origin"),S.Origin.ToString());Row->SetNumberField(TEXT("radius_cm"),S.Radius);
        Row->SetNumberField(TEXT("triangles"),S.Triangles);Row->SetNumberField(TEXT("clipped_triangles"),S.ClippedTriangles);
        Row->SetNumberField(TEXT("water_samples_pruned"),S.WaterSamplesPruned);
        Row->SetNumberField(TEXT("actual_source_triangles"),S.SourceTriangles);
        Row->SetNumberField(TEXT("minimum_clearance_cm"),S.MinimumClearance);
        Row->SetNumberField(TEXT("maximum_penetration_cm"),S.MaximumPenetration);
        Row->SetBoolField(TEXT("clockwise_verified"),S.bClockwiseVerified);
        Row->SetNumberField(TEXT("support_error_cm"),S.MaximumSupportError);Row->SetNumberField(TEXT("build_ms"),S.MaximumBuildMilliseconds);
        Row->SetBoolField(TEXT("snow"),S.bSnow);Row->SetBoolField(TEXT("collision_disabled"),true);
        Row->SetNumberField(TEXT("projection_half_depth_cm"),S.ProjectionHalfDepthV84411);
        Row->SetStringField(TEXT("actual_material"),GetPathNameSafe(Stamps[I]->GetDecalMaterial()));Rows.Add(MakeShared<FJsonValueObject>(Row));
        if(S.Radius>100.f&&(!Focus||FMath::Abs(S.Origin.X)<FMath::Abs(Focus->Origin.X)))Focus=&S;
    }
    if(Focus){J->SetNumberField(TEXT("review_focus_x"),Focus->Origin.X);J->SetNumberField(TEXT("review_focus_y"),Focus->Origin.Y);J->SetNumberField(TEXT("review_focus_z"),Focus->Origin.Z);}
    J->SetArrayField(TEXT("stamps"),Rows);return J;
}
void AWarfareFrontlineScorchV8449::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString Dir;const bool Allowed=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&(FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review")))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
    Check(TEXT("v8449_scorch_review_isolated"),Allowed);if(!Allowed)return;
    auto* Receiver=Get(B->GetWorld());Check(TEXT("v8449_scorch_actual_receiver"),Receiver&&Receiver->ScorchBase&&Receiver->Ground.IsValid());if(!Receiver)return;
    const int32 CountBefore=Receiver->Queued;
    FVector P(0,700,AWarfareFrontlineGround::HeightAt(0,700));
    Receiver->QueueImpact(P,3);Receiver->QueueImpact(P+FVector(0,0,1700),0);
    Check(TEXT("v8449_scorch_bullets_and_air_do_not_stamp"),Receiver->Queued==CountBefore);
    FVector Dry=FVector::ZeroVector,Slope=FVector::ZeroVector,Wet=FVector::ZeroVector,Shore=FVector::ZeroVector;
    bool HaveDry=false,HaveSlope=false,HaveWater=false;float DryScore=MAX_flt,SlopeScore=MAX_flt;
    auto* G=Receiver->Ground.Get();const bool WaterVisible=G&&G->BasinWaterV44&&G->BasinWaterV44->IsVisible()&&G->BasinWaterV44->GetStaticMesh();
    const float TrenchX=B->Trenches.IsEmpty()?0.f:B->Trenches[B->Trenches.Num()/2].X;
    for(int32 IX=-8;IX<=8;++IX)for(int32 IY=0;IY<=14;++IY)
    {
        const float X=TrenchX+IX*90.f,Y=400.f+IY*90.f;
        const float H=AWarfareFrontlineGround::HeightAt(X,Y);if(!FMath::IsFinite(H))continue;
        float Level=0.f,Depth=0.f;const bool UnderWater=WaterVisible&&AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>.8f;
        if(UnderWater){if(!HaveWater){Wet=FVector(X,Y,H);HaveWater=true;}continue;}
        const float DX=(AWarfareFrontlineGround::HeightAt(X+4,Y)-AWarfareFrontlineGround::HeightAt(X-4,Y))/8.f;
        const float DY=(AWarfareFrontlineGround::HeightAt(X,Y+4)-AWarfareFrontlineGround::HeightAt(X,Y-4))/8.f;
        const float Incline=FMath::Sqrt(DX*DX+DY*DY),Score=FMath::Abs(X)+FMath::Abs(Y-800.f)*.20f;
        if(Incline<.15f&&Score<DryScore){Dry=FVector(X,Y,H);HaveDry=true;DryScore=Score;}
        if(Incline>.35f&&Incline<1.65f&&Score<SlopeScore){Slope=FVector(X,Y,H);HaveSlope=true;SlopeScore=Score;}
    }
    auto ActualGroundBlast=[&](const FVector& At)
    {
        auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At+FVector(0,0,3.f),FRotator::ZeroRotator);
        if(!Impact)return false;
        const int32 Before=Receiver->Queued;Impact->ConfigureBlast(false,true);
        const int32 After=Receiver->Queued;Impact->ConfigureBlast(false,false);
        const bool Single=After==Before+1&&Receiver->Queued==After;Impact->Destroy();return Single;
    };
    if(HaveDry&&G&&G->Surface)
    {
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(WFScorchLegacyDiagnosticV8449),true);
        Receiver->bLegacyDryTraceChecked=true;
        Receiver->bLegacyDryTraceHit=G->Surface->LineTraceComponent(Hit,Dry+FVector(0,0,1800),Dry-FVector(0,0,1800),Query)&&Hit.bBlockingHit;
        if(Receiver->bLegacyDryTraceHit)
        {Receiver->LegacyDryTraceNormalZ=Hit.ImpactNormal.Z;Receiver->LegacyDryTraceErrorCm=FMath::Abs(Hit.ImpactPoint.Z-Dry.Z);}
    }
    Check(TEXT("v8449_scorch_real_dry_ground_detonation_queues_once"),HaveDry&&ActualGroundBlast(Dry));
    // Build the dry witness before adding neighbouring witnesses; a nearby
    // follow-up hit may legitimately replace it through the coalescing rule.
    for(int32 I=0;I<12&&!Receiver->Pending.IsEmpty();++I)Receiver->Tick(.08f);
    bool DryBound=false;for(const auto& S:Receiver->State)DryBound|=S.Triangles>0&&FVector::DistSquared(S.Origin,Dry)<1.f;
    Check(TEXT("v8449_scorch_actual_dry_ground_mesh_survives_short_blast"),DryBound);
    if(HaveSlope)
    {
        Check(TEXT("v8449_scorch_real_bank_detonation_queues_once"),ActualGroundBlast(Slope));
        for(int32 I=0;I<12&&!Receiver->Pending.IsEmpty();++I)Receiver->Tick(.08f);
        bool Built=false;for(const auto& S:Receiver->State)
            Built|=S.Triangles>0&&S.MaximumSupportError<=1.2f&&FVector::DistSquared(S.Origin,Slope)<1.f;
        Check(TEXT("v8449_scorch_real_bank_mesh_built_and_supported"),Built);
    }
    if(HaveWater)
    {
        const int32 Rejected=Receiver->RejectedWater,Before=Receiver->Queued;
        auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Wet+FVector(0,0,3.f),FRotator::ZeroRotator);
        if(Impact){Impact->ConfigureBlast(false,true);Impact->Destroy();}
        Check(TEXT("v8449_scorch_actual_basin_does_not_receive_floating_soot"),Impact&&Receiver->RejectedWater>Rejected&&Receiver->Queued==Before);
        bool HaveShore=false;
        for(int32 R=1;R<=4&&!HaveShore;++R)for(int32 A=0;A<12&&!HaveShore;++A)
        {
            const float Angle=A*PI/6.f;const float X=Wet.X+FMath::Cos(Angle)*R*90.f,Y=Wet.Y+FMath::Sin(Angle)*R*90.f;
            float Level=0.f,Depth=0.f;if(AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>.2f)continue;
            const float H=AWarfareFrontlineGround::HeightAt(X,Y);if(!FMath::IsFinite(H))continue;
            Shore=FVector(X,Y,H);HaveShore=true;
        }
        if(HaveShore)
        {
            Check(TEXT("v8449_scorch_actual_shore_detonation_queues_once"),ActualGroundBlast(Shore));
            for(int32 I=0;I<12&&!Receiver->Pending.IsEmpty();++I)Receiver->Tick(.08f);
            bool Built=false,Clipped=false;for(const auto& S:Receiver->State)
                if(S.Triangles>0&&FVector::DistSquared(S.Origin,Shore)<1.f)
                {Built=S.MaximumSupportError<=1.2f;Clipped=S.WaterSamplesPruned>0;}
            Check(TEXT("v8449_scorch_actual_shore_mesh_built_and_supported"),Built);
            Check(TEXT("v8449_scorch_actual_shore_water_samples_removed"),Clipped);
        }
    }
    // Drain only requests issued by these actual production impact actors.
    // Destroying the short blast must not remove the persistent ground mark.
    for(int32 I=0;I<12&&!Receiver->Pending.IsEmpty();++I)Receiver->Tick(.08f);
    bool Valid=true;int32 Active=0;
    for(int32 I=0;I<Receiver->State.Num();++I)if(Receiver->State[I].Triangles>0)
    {++Active;const auto& S=Receiver->State[I];Valid&=S.MaximumSupportError<=1.2f&&Receiver->Stamps[I]
        &&Receiver->Stamps[I]->IsRegistered()&&Receiver->Stamps[I]->GetDecalMaterial()&&S.Radius<=360.f;}
    Check(TEXT("v8449_scorch_contact_is_real_bounded_and_cosmetic"),Valid&&Active>0&&Active<=StampBudget);
    bool Direct=true;for(const auto& S:Receiver->State)if(S.Triangles>0)
        Direct&=S.SourceTriangles>0&&S.SourceTriangles<=8192&&S.bClockwiseVerified&&S.MinimumClearance>=.30f&&S.MaximumPenetration<=.0001f;
    Check(TEXT("v8449_scorch_checked_source_patch_winding_and_clearance"),Direct&&Active>0);
    Check(TEXT("v8449_scorch_persistent_receiver_has_no_blast_lifespan"),Receiver->GetLifeSpan()==0.f&&Receiver->State.Num()==StampBudget&&Receiver->Stamps.Num()==StampBudget);
}

AWarfareFrontlineImpact::AWarfareFrontlineImpact()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Impact")));
    CompositionV8221=CreateDefaultSubobject<UFrontlineBlastCompositionV8221>(TEXT("BlastCompositionV8221"));
    for(int32 I=0;I<CardBudget;++I)
    {
        UStaticMeshComponent* Puff=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Dust%d"),I));
        Puff->SetupAttachment(RootComponent);
        Puff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Puff->SetCastShadow(false);Puff->SetVisibility(false);Puff->SetReceivesDecals(false);
        Puff->SetCanEverAffectNavigation(false);Puffs.Add(Puff);
    }
    Flash=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BriefFlash"));Flash->SetupAttachment(RootComponent);
    Flash->SetCollisionEnabled(ECollisionEnabled::NoCollision);Flash->SetCastShadow(false);Flash->SetReceivesDecals(false);Flash->SetVisibility(false);
    Fragments=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ThrownEarth"));Fragments->SetupAttachment(RootComponent);
    Fragments->SetCollisionEnabled(ECollisionEnabled::NoCollision);Fragments->SetCastShadow(true);
    Fragments->SetCanEverAffectNavigation(false);
    Fragments->SetHasPerInstancePrevTransforms(true);
    FlashLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("ImpactLight"));FlashLight->SetupAttachment(RootComponent);
    FlashLight->SetCastShadows(false);FlashLight->SetAttenuationRadius(620.f);
    FlashLight->SetIntensityUnits(ELightUnits::Lumens);FlashLight->SetIntensity(0.f);
    FlashLight->SetLightColor(FLinearColor(1.f,.47f,.16f));FlashLight->SetRelativeLocation(FVector(0,0,65));
}
void AWarfareFrontlineImpact::BeginPlay()
{
    Super::BeginPlay();
    UStaticMesh* Quad=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    EjectaBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V38/Materials/M_BlastEjecta_V38.M_BlastEjecta_V38"));
    RollingBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V38/Materials/M_BlastRoll_V38.M_BlastRoll_V38"));
    if(auto* E=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V47/Materials/M_BlastEjecta_V47.M_BlastEjecta_V47")))EjectaBase=E;
    if(auto* R=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V47/Materials/M_BlastRoll_V47.M_BlastRoll_V47")))RollingBase=R;
    auto* FlashBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V31/Materials/M_BlastFlash_V31.M_BlastFlash_V31"));
    if(auto* Revised=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V45/Materials/M_BlastFlash_V45.M_BlastFlash_V45")))FlashBase=Revised;
    if(!EjectaBase)EjectaBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V31/Materials/M_BlastEjecta_V31.M_BlastEjecta_V31"));
    if(!RollingBase)RollingBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V31/Materials/M_BlastRoll_V31.M_BlastRoll_V31"));
    if(!EjectaBase)EjectaBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V28/Materials/M_BlastEjecta_V28.M_BlastEjecta_V28"));
    if(!RollingBase)RollingBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V28/Materials/M_BlastRoll_V28.M_BlastRoll_V28"));
    if(!FlashBase)FlashBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V23/Materials/M_BlastFlash_V23.M_BlastFlash_V23"));
    auto* Clod=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_EarthClod_V12.SM_EarthClod_V12"));
    VolumeBaseV40=FrontlineSmokeVolumeV35::MaterialV40();
    if(auto* Dust=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8211/Materials/M_BlastVolume_V8211.M_BlastVolume_V8211")))VolumeBaseV40=Dust;
    if(auto* Dust=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8215/Materials/M_BlastVolume_V8215.M_BlastVolume_V8215")))VolumeBaseV40=Dust;
    PenetrationBaseV8215=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8215/Materials/M_PenetrationShard_V8215.M_PenetrationShard_V8215"));
    if(!Quad || !EjectaBase || !RollingBase || !FlashBase || !Clod)
    {
        UE_LOG(LogTemp,Error,TEXT("V28 impact asset missing; import V28 flipbooks and retain V23 flash before runtime review"));
        Destroy();return;
    }
    // Loading preparation warms the retained flipbooks. The new eight MiB
    // density sequence is non-streaming: no per-detonation 45-second mip pin.
    FRandomStream R(GetTypeHash(GetActorLocation())^(IsV47FXReview()?0:int32(GetUniqueID())));
    Materials.SetNum(Puffs.Num());
    for(int32 I=0;I<Puffs.Num();++I)
    {
        Puffs[I]->SetStaticMesh(Quad);
        const float A=R.FRandRange(0,2*PI);
        PuffDrift.Add(FVector(FMath::Cos(A)*R.FRandRange(65,155),FMath::Sin(A)*R.FRandRange(40,105),R.FRandRange(25,80)));
        PuffVariation.Add(R.FRandRange(.78f,1.24f));
    }
    Flash->SetStaticMesh(Quad);FlashMaterial=UMaterialInstanceDynamic::Create(FlashBase,this);Flash->SetMaterial(0,FlashMaterial);
    Fragments->SetStaticMesh(Clod);
}
void AWarfareFrontlineImpact::ConfigureBlast(bool bGrenade,bool bMortar,const FVector& IncomingDirection)
{
    bHeavyBulletV8215=bArmourBulletV8215=bStrafeBulletV8215=false;
    ConfigureProfile(bGrenade?2:(bMortar?1:0));
    StrikeDirection=FVector(IncomingDirection.X,IncomingDirection.Y,0).GetSafeNormal();
    for(auto& Fragment:Soil)Fragment.Velocity+=StrikeDirection*(bGrenade?28.f:(bMortar?65.f:115.f));
}
void AWarfareFrontlineImpact::ConfigureBulletStrike()
{ bHeavyBulletV8215=bArmourBulletV8215=bStrafeBulletV8215=false;ConfigureProfile(3); }
void AWarfareFrontlineImpact::ConfigureMineV8214(float FootprintRadiusCm)
{
    bHeavyBulletV8215=bArmourBulletV8215=bStrafeBulletV8215=false;
    MineRadiusV8214=FMath::IsFinite(FootprintRadiusCm)&&FootprintRadiusCm>0.f?FMath::Clamp(FootprintRadiusCm,300.f,900.f):600.f;
    StrikeDirection=FVector::ZeroVector;
    ConfigureProfile(5);
}
void AWarfareFrontlineImpact::ConfigureAirburstV64(float FootprintRadius,const FVector& IncomingDirection)
{
    bHeavyBulletV8215=bArmourBulletV8215=bStrafeBulletV8215=false;
    ShrapnelRadiusV8211=FMath::Clamp(FootprintRadius,100.f,1600.f);
    StrikeDirection=IncomingDirection.GetSafeNormal2D();
    ConfigureProfile(4);
}
void AWarfareFrontlineImpact::SetCasualtyDustV8211(int32 Casualties)
{
    BloodLoadV8211=FMath::Clamp(Casualties*.24f,0.f,.92f);
    if(CompositionV8221)CompositionV8221->SetCasualties(CasualtyDustSourcesV8216,Casualties);
}
void AWarfareFrontlineImpact::SetCasualtyDustV8216(const TArray<FVector>& CasualtyFeet)
{
    CasualtyDustSourcesV8216.Reset();
    for(const FVector& P:CasualtyFeet)if(!P.ContainsNaN()&&CasualtyDustSourcesV8216.Num()<16)CasualtyDustSourcesV8216.Add(P);
    SetCasualtyDustV8211(CasualtyDustSourcesV8216.Num());
    // The low fragment impacts carry casualty aerosol inside their own dust.
    // There is no red plume in the overhead airburst, nor one spawned at a man.
    if(BlastKind==4)for(int32 I=0;I<FMath::Min(ShrapnelGroundV8211.Num(),CasualtyDustSourcesV8216.Num());++I)
        ShrapnelGroundV8211[I]=CasualtyDustSourcesV8216[I]+FVector(0,0,9.f);
}
void AWarfareFrontlineImpact::ConfigureStrafeStrike(const FVector& IncomingDirection)
{
    bHeavyBulletV8215=bArmourBulletV8215=false;
    bStrafeBulletV8215=true;ConfigureProfile(3);StrikeDirection=FVector(IncomingDirection.X,IncomingDirection.Y,0).GetSafeNormal();
    for(auto& Fragment:Soil)Fragment.Velocity+=StrikeDirection*145.f;
}
void AWarfareFrontlineImpact::ConfigureHeavyBulletV8215(const FVector& IncomingDirection,bool bArmour)
{
    bStrafeBulletV8215=false;
    bHeavyBulletV8215=true;bArmourBulletV8215=bArmour;
    ConfigureProfile(3);StrikeDirection=IncomingDirection.GetSafeNormal2D();
    for(auto& Fragment:Soil)
    {
        Fragment.Velocity+=StrikeDirection*(bArmour?390.f:270.f);
    }
}
void AWarfareFrontlineImpact::ConfigureProfile(int32 Profile)
{
    if(IsActorBeingDestroyed() || !Fragments->GetStaticMesh())return;
    BlastKind=Profile;
    BloodLoadV8211=0.f;CasualtyDustSourcesV8216.Reset();
    WaterContactLiftV831.Init(0.f,Puffs.Num());WaterContactTargetV831.Init(0.f,Puffs.Num());
    WaterContactInitializedV831.Init(0,Puffs.Num());
    ParticulateSlotsV84412.Init(0,Puffs.Num());
    WaterContactGroundV831.Reset();
    const bool bGrenade=BlastKind==2,bMortar=BlastKind==1,bBullet=BlastKind==3,bMine=BlastKind==5;
    // Select the budget only after the caller supplies the profile. A fresh
    // bullet must never evict a shell during SpawnActor/BeginPlay.
    int32 Active=0,FlashLights=0,Volumes=0;AWarfareFrontlineImpact* Oldest=nullptr;
    for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)
        if(IsValid(*It) && *It!=this && !It->IsActorBeingDestroyed() && It->bProfileConfigured)
        {
            if(It->bFlashLightAllocated&&It->Age<.24f)++FlashLights;
            Volumes+=It->ReservedVolumeCountV8216();
            if((It->BlastKind==3)==bBullet){++Active;if(!Oldest || It->Age>Oldest->Age)Oldest=*It;}
        }
    if(Active>=(bBullet?BulletImpactBudget:HeavyImpactBudget))
    {
        if(Oldest && Oldest->Age>(bBullet?.15f:.75f))Oldest->Destroy();
        else{Destroy();return;}
    }
    const bool FirstProfile=!bProfileConfigured;
    bProfileConfigured=true;
    // The persistent receiver owns only a real ground detonation. The short
    // smoke actor may be rebound or retired without duplicating that mark.
    // Aerial shrapnel, smoke/gas delivery and small-arms strikes leave no soot.
    if(FirstProfile&&!bBullet&&BlastKind!=4)
        if(auto* Surface=AWarfareFrontlineScorchV8449::Get(GetWorld()))Surface->QueueImpact(GetActorLocation(),BlastKind);
    if(FirstProfile&&!bBullet)FrontlineMedalMotionV8216::NotifyBlast(GetWorld(),GetActorLocation(),
        bMine?.90f:BlastKind==4?.60f:bGrenade?.55f:bMortar?1.f:1.3f);
    // Rebinding a cosmetic profile must not spend multiple water inputs on the
    // same detonation. The Tick fallback also passes through this first bind.
    if(FirstProfile&&!bBullet&&BlastKind!=4)UFrontlineWaterResponseV51::NotifyImpact(GetWorld(),GetActorLocation(),bMine?MineRadiusV8214:bGrenade?290.f:(bMortar?620.f:1050.f),bMine?.85f:bGrenade?.48f:(bMortar?.90f:1.25f));
    bFlashLightAllocated=!bBullet&&FlashLights<FlashLightBudget;
    // Actor scaling also scaled the old local gravity and trench clearance.
    // Profiles now use centimetres directly, with the same 980 cm/s2 gravity.
    SetActorScale3D(FVector::OneVector);
    EarthPuffs=bBullet?1:(bGrenade?2:3);WashPuffs=bBullet?2:(bGrenade?3:4);
    CorePuffs=bBullet?3:(bGrenade?7:(bMortar?10:12));
    ActivePuffs=CorePuffs+(bBullet?0:(bGrenade?2:(bMortar?3:4)));
    if(bMine){EarthPuffs=3;WashPuffs=5;CorePuffs=12;ActivePuffs=CardBudget;}
    if(BlastKind==4){EarthPuffs=WashPuffs=0;CorePuffs=3;ActivePuffs=EffectsBudgetV8211();}
    const float EffectsBudget=UFrontlineRenderSettingsSubsystem::EffectsBudgetV43();
    const int32 AllowedVolumes=EffectsBudget<.5f?2:(EffectsBudget<.9f?4:VolumeBudgetV40);
    VolumePuffsV40=VolumeBaseV40&&!bBullet?FMath::Min(bGrenade||EffectsBudget<.9f?1:2,FMath::Max(0,AllowedVolumes-Volumes)):0;
    auto* VolumeProxy=VolumePuffsV40?LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")):nullptr;
    if(!VolumeProxy)VolumePuffsV40=0;
    auto* FlatProxy=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* ParticulateBase=bReviewLegacyV831?nullptr:FrontlineParticulateV84412::Material(this,0);
    auto* ParticulateCube=ParticulateBase?FrontlineParticulateV84412::Cube():nullptr;
    Soil.Reset();Fragments->SetHasPerInstancePrevTransforms(false);Fragments->ClearInstances();Fragments->SetHasPerInstancePrevTransforms(true);
    FragmentTransformsV835.Reset();PreviousFragmentTransformsV835.Reset();
    FragmentUploadBatchesV835=0;
    bReviewLegacyDebrisV835=FrontlineEffectsReviewSafetyV8216::Allowed(Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV835DebrisBaseline"));
    if(auto* Original=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V12/Meshes/SM_EarthClod_V12.SM_EarthClod_V12")))
        Fragments->SetStaticMesh(Original);
    Fragments->SetMaterial(0,nullptr);PenetrationMaterialV8215=nullptr;
    if(bArmourBulletV8215&&PenetrationBaseV8215)
    {
        PenetrationMaterialV8215=UMaterialInstanceDynamic::Create(PenetrationBaseV8215,this);
        PenetrationMaterialV8215->SetScalarParameterValue(TEXT("ImpactAge"),0.f);
        Fragments->SetMaterial(0,PenetrationMaterialV8215);
    }
    FRandomStream R(GetTypeHash(GetActorLocation())^(IsV47FXReview()?0:int32(GetUniqueID()))^(BlastKind*173));
    if(FlashMaterial)FlashMaterial->SetScalarParameterValue(TEXT("Seed"),R.FRandRange(0,6.283185f));
    for(int32 I=0;I<Puffs.Num();++I)
    {
        if(I>=ActivePuffs){Materials[I]=nullptr;Puffs[I]->SetVisibility(false);continue;}
        const bool Volume=VolumeSlotV828(I,CorePuffs,VolumePuffsV40,BlastKind);
        const bool NonSmoke=bBullet||(BlastKind==4?I>=CorePuffs:I<EarthPuffs+WashPuffs||I>=CorePuffs);
        const bool Particulate=ParticulateBase&&ParticulateCube&&NonSmoke;
        ParticulateSlotsV84412[I]=Particulate?1:0;
        Puffs[I]->SetStaticMesh(Particulate?ParticulateCube:Volume?VolumeProxy:FlatProxy);
        auto* M=UMaterialInstanceDynamic::Create(Particulate?ParticulateBase:Volume?VolumeBaseV40.Get():(I<EarthPuffs?EjectaBase.Get():RollingBase.Get()),this);
        if(Volume&&!Particulate){Puffs[I]->SetStaticMesh(VolumeProxy);M->SetScalarParameterValue(TEXT("VolumeProfile"),2.f);}
        if(Particulate)
        {
            Puffs[I]->SetVisibleInRayTracing(false);Puffs[I]->SetAffectDistanceFieldLighting(false);
            Puffs[I]->SetAffectDynamicIndirectLighting(false);
            Puffs[I]->bVisibleInReflectionCaptures=false;Puffs[I]->bVisibleInRealTimeSkyCaptures=false;
            M->SetScalarParameterValue(TEXT("IncidentMode"),0.f);
        }
        M->SetScalarParameterValue(TEXT("Density"),0.f);M->SetScalarParameterValue(TEXT("Seed"),R.FRandRange(0,6.283185f));
        Puffs[I]->SetMaterial(0,M);Materials[I]=M;Puffs[I]->SetVisibility(false);
    }
    const int32 FullCount=bMine?80:bBullet?(bHeavyBulletV8215?16:bStrafeBulletV8215?9:5):(bGrenade?18:(bMortar?30:44));
    const int32 Count=FMath::Max(bBullet?3:8,FMath::RoundToInt(FullCount*(EffectsBudget<.5f?.48f:(EffectsBudget<.9f?.73f:1.f))));
    for(int32 I=0;I<(BlastKind==4?0:Count);++I)
    {
        auto& F=Soil.AddDefaulted_GetRef();
        // Unequal low fans carry wet soil outwards; a smaller fraction rises
        // high. Clods and their dust wakes share this same physical trajectory.
        const float Angle=R.FRandRange(0,2*PI),Speed=R.FRandRange(bGrenade?130.f:180.f,bGrenade?350.f:(bMortar?485.f:660.f));
        F.Position=FVector(R.FRandRange(-20,20),R.FRandRange(-20,20),18);
        const float Lift=bGrenade?R.FRandRange(240.f,510.f):R.FRandRange(bMortar?330.f:390.f,bMortar?820.f:1090.f);
        F.Velocity=FVector(FMath::Cos(Angle)*Speed,FMath::Sin(Angle)*Speed*.82f,Lift*(I%3==0?.75f:1.f));
        if(bBullet)F.Velocity*=bHeavyBulletV8215?FVector(.75f,.75f,.75f):bStrafeBulletV8215?FVector(.55f,.55f,.60f):FVector(.35f,.35f,.48f);
        F.AngularVelocity=FVector(R.FRandRange(-210,210),R.FRandRange(-210,210),R.FRandRange(-210,210));
        F.Scale=FVector(R.FRandRange(.35f,1.2f),R.FRandRange(.35f,.9f),R.FRandRange(.45f,1.35f));
        F.Scale*=bGrenade?.52f:(bMortar?.78f:1.f);
        if(bBullet)F.Scale*=bHeavyBulletV8215?.29f:bStrafeBulletV8215?.24f:.18f;
        if(bArmourBulletV8215)F.Scale*=FVector(.23f,.23f,1.65f);
        if(I%5==0)F.Scale*=1.25f;
        F.Scale*=FVector(I%2?1.13f:.83f,I%2?.80f:1.1f,1.f);
        if(bMine)
        {
            const float Bearing=R.FRandRange(0.f,2.f*PI);
            const bool HighJet=I%4==0;
            const float MineSpeed=R.FRandRange(HighJet?120.f:340.f,HighJet?330.f:720.f);
            const float MineLift=R.FRandRange(HighJet?900.f:300.f,HighJet?1320.f:920.f);
            F.Velocity=FVector(FMath::Cos(Bearing)*MineSpeed,FMath::Sin(Bearing)*MineSpeed*.88f,MineLift);
            F.Scale*=HighJet?1.24f:.90f;
        }
        Fragments->AddInstance(FTransform(F.Rotation,F.Position,F.Scale));
    }
    // Include the longest per-card variation and delayed start. No visible
    // late billow is removed when the actor's bounded lifetime finishes.
    SetLifeSpan(bMine?6.2f:BlastKind==4?3.2f:bBullet?(bHeavyBulletV8215?1.55f:bStrafeBulletV8215?.80f:.9f):(bGrenade?3.4f:(bMortar?8.2f:10.8f)));
    ShrapnelGroundV8211.Reset();ShrapnelArrivalV8211.Reset();
    if(BlastKind==4)
    {
        // One bounded actor: a dense overhead puff, fast fragment fan and
        // staggered ground strikes matching the actual horizontal footprint.
        for(int32 I=CorePuffs;I<ActivePuffs;++I)
        {
            const float Fraction=float(I-CorePuffs+.5f)/FMath::Max(1,ActivePuffs-CorePuffs);
            const float A=I*2.399963f+R.FRandRange(-.15f,.15f);
            const float Radius=ShrapnelRadiusV8211*FMath::Sqrt(Fraction)*.91f;
            FVector Q=GetActorLocation()+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0);
            Q.Z=AWarfareFrontlineGround::HeightAt(Q.X,Q.Y)+10;
            ShrapnelGroundV8211.Add(Q);ShrapnelArrivalV8211.Add(.09f+.08f*Fraction);
            auto& F=Soil.AddDefaulted_GetRef();F.Position=FVector::ZeroVector;
            F.Scale=FVector(.028f,.028f,.38f);
            Fragments->AddInstance(FTransform(F.Rotation,F.Position,F.Scale));
        }
    }
    Age=0;bFirstTick=true;ConfiguredAt=FPlatformTime::Seconds();
    if(CompositionV8221)
    {
        CompositionV8221->Configure(BlastKind,GetActorLocation(),BlastKind==5?MineRadiusV8214:ShrapnelRadiusV8211,!bReviewLegacyV831);
        if(CompositionV8221->IsReady())
        {
            Fragments->SetStaticMesh(UFrontlineBlastCompositionV8221::SoilMesh());
            Fragments->SetMaterial(0,UFrontlineBlastCompositionV8221::SoilMaterial());
        }
        // Layer-isolated captures identify the retained smoke Cube, not the
        // flat wash, as the hard-edged patch. Its derivative preserves the
        // transported field and softens density at opaque surface contact.
        // Every flat card and Earth volume retains its original parent.
        if(CompositionV8221->UsesArtResetV831())
        {
            // Resolve the actual displayed surface once per profile. Analytic
            // hydrology alone must not lift a cloud above an absent mesh.
            for(TActorIterator<AWarfareFrontlineGround> G(GetWorld());G;++G)
                if(!G->IsHidden()&&G->BasinWaterV44&&G->BasinWaterV44->IsRegistered()
                    &&G->BasinWaterV44->IsVisible()&&G->BasinWaterV44->GetStaticMesh()&&G->BasinWaterV44->GetMaterial(0))
                {WaterContactGroundV831=*G;break;}
            auto* Envelope=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84411/Materials/M_BlastVolumeEnvelope_V84411.M_BlastVolumeEnvelope_V84411"));
            if(!Envelope)Envelope=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V831/Materials/M_BlastVolumeEnvelope_V831.M_BlastVolumeEnvelope_V831"));
            if(Envelope)
                for(int32 I=EarthPuffs;I<ActivePuffs;++I)
                    if(VolumeSlotV828(I,CorePuffs,VolumePuffsV40,BlastKind)&&!ParticulateSlotsV84412[I])
                    {
                        float ExistingSeed=0.f;
                        Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Seed")),ExistingSeed);
                        auto* M=UMaterialInstanceDynamic::Create(Envelope,this);
                        M->SetScalarParameterValue(TEXT("VolumeProfile"),2.f);
                        M->SetScalarParameterValue(TEXT("Density"),0.f);M->SetScalarParameterValue(TEXT("Seed"),ExistingSeed);
                        Puffs[I]->SetMaterial(0,M);Materials[I]=M;
                    }
        }
    }
    if(Fragments->GetStaticMesh())
    {
        const FVector Diameter=Fragments->GetStaticMesh()->GetBounds().BoxExtent*2.f;
        // The authored mesh is a 24 cm clod. Most ejected earth is a 2–7 cm
        // chip, with sparse larger pieces. Fit actual mesh bounds instead of
        // relying on FBX units or the original emitter's arbitrary scale.
        for(int32 I=0;I<Soil.Num();++I)
        {
            auto& F=Soil[I];
            if(!bReviewLegacyDebrisV835&&BlastKind!=3&&BlastKind!=4)
            {
                const float Limit=BlastKind==2?7.f:BlastKind==1?10.f:BlastKind==5?10.f:12.f;
                const bool Large=I%19==0;
                const float Desired=Limit*(Large?R.FRandRange(.92f,1.32f):R.FRandRange(.18f,.60f));
                F.Scale*=Desired/FMath::Max(.001f,(Diameter*F.Scale).GetMax());
            }
            const FTransform Pose(F.Rotation,F.Position,F.Scale);
            FragmentTransformsV835.Add(Pose);PreviousFragmentTransformsV835.Add(Pose);
        }
        if(!FragmentTransformsV835.IsEmpty())
            Fragments->BatchUpdateInstancesTransforms(0,FragmentTransformsV835,PreviousFragmentTransformsV835,false,false,false);
    }
    if(!bReviewLegacyDebrisV835)RegisterFragmentShutterV835(this);
    UpdateFlash(BlastBillboard(this));
    if(CompositionV8221&&CompositionV8221->ReplacesFlatFlash())Flash->SetVisibility(false);
    FrontlineEffectsSafetyV844::Guard(this);
}
int32 AWarfareFrontlineImpact::ReservedVolumeCountV8216() const
{
    int32 Count=0;
    for(int32 I=0;I<ActivePuffs;++I)if(VolumeSlotV828(I,CorePuffs,VolumePuffsV40,BlastKind))
    {
        float Delay=0,Life=0;const bool Earth=I<EarthPuffs,Wash=I>=EarthPuffs&&I<EarthPuffs+WashPuffs;
        const float V=PuffVariation.IsValidIndex(I)?PuffVariation[I]:1.f;
        if(BlastKind==4)
        {const bool Overhead=I<CorePuffs;Delay=Overhead?I*.018f:ShrapnelArrivalV8211[I-CorePuffs];Life=Overhead?2.7f:1.25f;}
        else if(BlastKind==5)
        {Delay=Earth?.008f:.045f;Life=(Earth?1.18f:Wash?1.65f:4.30f)*V;}
        else
        {
            Delay=Earth?.008f:.045f;
            Life=BlastKind==2?(Earth?.52f*V:Wash?.95f:2.35f*V):
                Earth?(BlastKind==1?.85f:1.08f)*V:Wash?(BlastKind==1?1.6f:2.1f):(BlastKind==1?5.9f:7.8f)*V;
        }
        // Keep future delayed puffs reserved, but release a completed earth
        // volume while its actor still owns long-lived fragments/smoke. The
        // old ten-second reservation made later salvos regress to flat cards.
        Count+=Age<Delay+Life;
    }
    return Count;
}
bool AWarfareFrontlineImpact::IsVisualReady() const
{ return !IsActorBeingDestroyed() && bProfileConfigured && FlashMaterial && Materials.Num()==CardBudget && Materials[0] && Fragments->GetStaticMesh(); }
bool AWarfareFrontlineImpact::IsFlashVisible() const
{ return Flash && Flash->IsVisible(); }
int32 AWarfareFrontlineImpact::GetVisibleSmokeCount() const
{ int32 N=0;for(const auto& P:Puffs)if(P&&P->IsVisible())++N;return N; }
void AWarfareFrontlineImpact::UpdateFlash(const FRotator& Billboard)
{
    if(BlastKind==3)
    {
        const float Amount=bArmourBulletV8215?FMath::Square(FMath::Max(0.f,1.f-Age/.055f)):0.f;
        Flash->SetVisibility(Amount>0);Flash->SetRelativeLocation(FVector(0,0,12));Flash->SetWorldRotation(Billboard);
        Flash->SetRelativeScale3D(FVector(.42f,.30f,1));
        if(FlashMaterial){FlashMaterial->SetScalarParameterValue(TEXT("Density"),Amount);FlashMaterial->SetScalarParameterValue(TEXT("BurstAge"),Age);}
        FlashLight->SetIntensity(0.f);return;
    }
    if(BlastKind==5)
    {
        const float Life=.09f,Amount=FMath::Exp(-FMath::Max(0.f,Age-.008f)*32.f)*FMath::Clamp((Life-Age)/.025f,0.f,1.f);
        const float Expansion=.66f+.42f*(1.f-FMath::Exp(-Age*55.f));
        Flash->SetVisibility(Age<Life);Flash->SetRelativeLocation(FVector(0,0,30.f));Flash->SetWorldRotation(Billboard);
        Flash->SetRelativeScale3D(FVector(3.4f,.98f,1.f)*FVector(Expansion,Expansion,1.f));
        if(FlashMaterial){FlashMaterial->SetScalarParameterValue(TEXT("Density"),Amount);FlashMaterial->SetScalarParameterValue(TEXT("BurstAge"),Age);}
        FlashLight->SetIntensity(bFlashLightAllocated?3600.f*Amount:0.f);FlashLight->SetAttenuationRadius(MineRadiusV8214);
        FlashLight->SetLightColor(FLinearColor(1.f,.43f,.105f));FlashLight->SetRelativeLocation(FVector(0,0,38.f));
        FlashLight->SetCastShadows(false);FlashLight->SetVolumetricScatteringIntensity(0.f);return;
    }
    const bool bGrenade=BlastKind==2||BlastKind==4,bMortar=BlastKind==1;
    const float FlashLife=bGrenade?.11f:(bMortar?.14f:.16f);
    const float FlashAmount=FMath::Exp(-FMath::Max(0.f,Age-.012f)*26.f)*FMath::Clamp((FlashLife-Age)/.035f,0.f,1.f);
    const float Expansion=.72f+.38f*(1-FMath::Exp(-Age*48.f));
    Flash->SetVisibility(Age<FlashLife);Flash->SetRelativeLocation(FVector(0,0,bGrenade?52.f:74.f));
    Flash->SetWorldRotation(Billboard);Flash->SetRelativeScale3D((bGrenade?FVector(1.65f,1.12f,1):(bMortar?FVector(2.7f,1.85f,1):FVector(4.15f,2.65f,1)))*FVector(Expansion,Expansion,1));
    if(FlashMaterial){FlashMaterial->SetScalarParameterValue(TEXT("Density"),FlashAmount);FlashMaterial->SetScalarParameterValue(TEXT("BurstAge"),Age);}
    FlashLight->SetIntensity(bFlashLightAllocated?(bGrenade?2200.f:(bMortar?4200.f:7200.f))*FlashAmount:0.f);
    FlashLight->SetAttenuationRadius(bGrenade?380.f:(bMortar?620.f:880.f));
    FlashLight->SetLightColor(FMath::Lerp(FLinearColor(1.f,.68f,.31f),FLinearColor(1.f,.27f,.035f),FMath::Clamp(Age*6.f,0.f,1.f)));
    // Three short local lights at most. Density bakes supply internal shade;
    // no transient shadow-map rebuild for a tenth-of-a-second impulse.
    FlashLight->SetCastShadows(false);
    // Native fog integrates lighting temporally; short impulses otherwise
    // leave a dark/light trail. The authored smoke carries its own brief heat.
    FlashLight->SetVolumetricScatteringIntensity(0.f);
}
void AWarfareFrontlineImpact::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ON_SCOPE_EXIT { FrontlineEffectsSafetyV844::Guard(this); };
    if(!bProfileConfigured)ConfigureBlast(false,false);
    if(IsActorBeingDestroyed() || !bProfileConfigured)return;
    float VisualDelta=DeltaSeconds;
    if(bFirstTick)
    {
        // Actors spawned during a long engine frame receive that frame's full
        // delta, including time before they existed. CoreTicker spawning can
        // also precede World::Tick, so subtracting WorldTime alone has the same
        // problem. Only the first partial frame is limited to time since birth;
        // later frames retain normal game time, pause and speed behaviour.
        const float Dilation=GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation()*CustomTimeDilation;
        const float SinceBirth=float(FMath::Max(0.,FPlatformTime::Seconds()-ConfiguredAt))*Dilation;
        VisualDelta=FMath::Clamp(SinceBirth,0.f,DeltaSeconds);bFirstTick=false;
        UE_LOG(LogTemp,Display,TEXT("WF_V23_BLAST_BIRTH kind=%d engine_delta=%.4f post_birth_delta=%.4f"),BlastKind,DeltaSeconds,VisualDelta);
    }
    Age+=VisualDelta;
    const FRotator Billboard=BlastBillboard(this);
    const bool bGrenade=BlastKind==2,bMortar=BlastKind==1,bBullet=BlastKind==3;
    const float FlashLife=bGrenade?.11f:(bMortar?.14f:.16f);
    UpdateFlash(Billboard);
    if(CompositionV8221&&CompositionV8221->IsReady())
    {
        const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
        CompositionV8221->UpdateAt(Age,(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector());
        if(CompositionV8221->ReplacesFlatFlash())Flash->SetVisibility(false);
    }
    if(BlastKind==4)
    {
        const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
        const FVector View=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
        for(int32 I=0;I<ActivePuffs;++I)
        {
            if(!Materials.IsValidIndex(I)||!Materials[I])continue;
            const bool Overhead=I<CorePuffs;const int32 GroundIndex=I-CorePuffs;
            const float Delay=Overhead?I*.018f:ShrapnelArrivalV8211[GroundIndex];
            const float T=Age-Delay,Life=Overhead?2.7f:1.25f;
            if(T<0||T>=Life){Puffs[I]->SetVisibility(false);continue;}
            const float Expansion=1-FMath::Exp(-T*(Overhead?5.5f:15.f));
            const float Width=(Overhead?105.f+400.f*Expansion:58.f+170.f*Expansion)*PuffVariation[I];
            const float Height=Overhead?Width*.91f:(80.f+100.f*Expansion)*(1-T/Life);
            const float Density=FMath::Clamp(T/.025f,0.f,1.f)*FMath::Pow(1-T/Life,Overhead?1.f:1.2f)*(Overhead?.90f:.86f);
            const FVector Centre=Overhead?GetActorLocation()+FVector((I-1)*27.f,T*24.f,T*24.f):ShrapnelGroundV8211[GroundIndex]+FVector(T*17,0,Height*.28f);
            Puffs[I]->SetVisibility(Density>.005f);
            if(ParticulateSlotsV84412[I])
                FrontlineParticulateV84412::Update(Puffs[I],Materials[I],Centre,FVector(Width,Width*.94f,Height),View,Density,T);
            else if(VolumeSlotV828(I,CorePuffs,VolumePuffsV40,BlastKind))
                FrontlineSmokeVolumeV35::Update(Puffs[I],Materials[I],Centre,FVector(Width,Width*.94f,Height),View,Density,T);
            else
            {
                Puffs[I]->SetWorldLocation(Centre);Puffs[I]->SetWorldRotation(Billboard);
                Puffs[I]->SetRelativeScale3D(FVector(Width/100.f,Height/100.f,1));
                Materials[I]->SetScalarParameterValue(TEXT("FramePhase"),FMath::Clamp(T/Life,0.f,.99f));
            }
            Materials[I]->SetScalarParameterValue(TEXT("Density"),Density);
            Materials[I]->SetScalarParameterValue(TEXT("Age"),T);
            Materials[I]->SetScalarParameterValue(TEXT("FlashHeat"),Overhead?FMath::Exp(-T*40.f)*.06f:0.f);
            const bool CasualtyGround=!(CompositionV8221&&CompositionV8221->IsReady())&&!Overhead&&(CasualtyDustSourcesV8216.IsEmpty()
                ||GroundIndex<CasualtyDustSourcesV8216.Num());
            const float Blood=CasualtyGround?NearGroundCasualtyLoadV8216(BloodLoadV8211,T,.46f):0.f;
            Materials[I]->SetScalarParameterValue(TEXT("BloodLoadV8211"),Blood);
            Materials[I]->SetVectorParameterValue(TEXT("DustColor"),Overhead?FLinearColor(.063f,.060f,.053f):
                FMath::Lerp(FLinearColor(.079f,.067f,.054f),FLinearColor(.09f,.012f,.006f),Blood*.22f));
        }
        for(int32 I=0;I<Soil.Num();++I)
        {
            const float T=Age/ShrapnelArrivalV8211[I];const bool Visible=T<1.f;
            const FVector Delta=ShrapnelGroundV8211[I]-GetActorLocation();
            const FQuat Rotation=FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).ToQuat();
            FragmentTransformsV835[I]=FTransform(Rotation,Delta*FMath::Min(T,1.f),Visible?Soil[I].Scale:FVector::ZeroVector);
        }
        if(Age>3.2f)
        {
            // The airburst shards have long since reached the ground. A hitch
            // or explicit cosmetic reconfiguration must not retain zero-scale
            // instances and their previous-transform buffer until destruction.
            Soil.Reset();Fragments->SetHasPerInstancePrevTransforms(false);Fragments->ClearInstances();Fragments->SetHasPerInstancePrevTransforms(true);
            FragmentTransformsV835.Reset();PreviousFragmentTransformsV835.Reset();
        }
        else CommitFragmentTransformsV835();
        return;
    }
    if(BlastKind==5){UpdateMineV8214(VisualDelta,Billboard);return;}
    UpdateSoil(VisualDelta);
    for(int32 I=0;I<Puffs.Num() && I<Materials.Num();++I)
    {
        if(I>=ActivePuffs){Puffs[I]->SetVisibility(false);continue;}
        if(I>=CorePuffs)continue; // Reserved slots follow actual ballistic clods below.
        const bool bEarth=I<EarthPuffs,bGroundWash=I>=EarthPuffs && I<EarthPuffs+WashPuffs;
        // The new oriented particulate fan owns thrown earth. The retained
        // wash/rolling smoke keeps its original density, timing and shader.
        if(bEarth&&BlastKind!=3&&CompositionV8221&&CompositionV8221->IsReady())
        {Puffs[I]->SetVisibility(false);Materials[I]->SetScalarParameterValue(TEXT("BloodLoadV8211"),0.f);continue;}
        const float Variation=PuffVariation[I];
        const bool VolumeLayer=VolumeSlotV828(I,CorePuffs,VolumePuffsV40,BlastKind);
        const float Delay=VolumeLayer?(bEarth?.008f:.045f):
            (bEarth?I*.012f*Variation:(bGroundWash?.028f+(I-EarthPuffs)*.021f:.09f+(I-EarthPuffs-WashPuffs)*.088f*Variation));
        const float T=Age-Delay,Life=bBullet?(bEarth?(bHeavyBulletV8215?.45f:bStrafeBulletV8215?.33f:.28f):(bHeavyBulletV8215?1.35f:bStrafeBulletV8215?.72f:.73f)):(bGrenade?(bEarth?.52f*Variation:(bGroundWash?.95f:2.35f*Variation)):
            (bEarth?(bMortar?.85f:1.08f)*Variation:(bGroundWash?(bMortar?1.6f:2.1f):(bMortar?5.9f:7.8f)*Variation)));
        if(T<0 || T>Life){Puffs[I]->SetVisibility(false);continue;}
        Puffs[I]->SetVisibility(true);Puffs[I]->SetWorldRotation(Billboard);
        const float Phase=FMath::Clamp(T/Life,0.f,1.f);
        FVector Position;float Width,Height,Density;
        if(bBullet)
        {
            // Small ground strike, not a miniature explosive shell. The short
            // jet collapses into two low, irregular dust wisps with no light.
            Position=PuffDrift[I]*T*.23f+StrikeDirection*T*(bEarth?150.f:48.f)+FVector(0,0,bEarth?18+190*T-260*T*T:12+12*T);
            Width=(bEarth?48+90*T:100+160*T)*Variation;
            Height=(bEarth?125+60*T:35+32*T)*Variation;
            Density=FMath::Clamp(T*60.f,0.f,1.f)*FMath::Pow(1-Phase,1.15f)*(bEarth?.95f:.55f);
            const float Weight=bHeavyBulletV8215?2.10f:bStrafeBulletV8215?1.62f:1.f;
            Width*=Weight;Height*=Weight;Density*=bHeavyBulletV8215?1.30f:bStrafeBulletV8215?1.12f:1.f;
        }
        else if(bGrenade)
        {
            // Fragmentation grenade: asymmetric local dirt kick and settling
            // ground haze. There is no artillery stem/cap or sustained fireball.
            if(bEarth)
            {
                const float Fan=I==0?1.f:.67f;
                Position=PuffDrift[I]*(.02f+T*2.2f)+FVector(0,0,35+(290+I*37)*T-310*T*T);
                Width=(105+185*FMath::Sqrt(T))*Variation;Height=(230+190*T)*Variation*Fan;
                Density=FMath::Clamp(T*60.f,0.f,1.f)*FMath::Pow(1-Phase,.95f)*1.38f;
            }
            else if(bGroundWash)
            {
                const float Push=1-FMath::Exp(-T*8.f);
                Position=PuffDrift[I]*(.02f+Push*1.9f)+FVector(T*18,0,26+15*T);
                Width=(105+170*FMath::Sqrt(T))*Variation;Height=(48+34*T)*Variation;
                Density=FMath::Clamp(T*38.f,0.f,1.f)*FMath::Pow(1-Phase,1.25f)*.76f;
            }
            else
            {
                Position=PuffDrift[I]*(.15f+T*.37f)+FVector(T*31,0,40+36*(1-FMath::Exp(-T)));
                Width=(140+155*FMath::Sqrt(T))*Variation;Height=(76+29*T)*Variation;
                Density=FMath::Clamp(T*16.f,0.f,1.f)*FMath::Pow(1-Phase,1.65f)*.54f;
            }
        }
        else if(bEarth)
        {
            // Authored evolving density grows and tears apart over 16 true
            // volume frames; the unequal jets clear the parapet then collapse.
            const float JetShape=I==0?1.f:(I==1?.57f:.74f);
            Position=PuffDrift[I]*(.04f+T*3.15f)+StrikeDirection*T*150.f+FVector(0,0,58+(540+I*59)*T-430*T*T);
            Width=(150+275*FMath::Sqrt(T))*Variation;Height=(390+400*T)*Variation*JetShape;
            Density=FMath::Clamp(T*65.f,0.f,1.f)*FMath::Pow(1-Phase,.92f)*(I==0?1.62f:1.26f);
        }
        else if(bGroundWash)
        {
            // Low outward soil pressure is separate from the vertical ejecta.
            // Unequal directions and timing avoid a row of identical columns.
            const float Push=1-FMath::Exp(-T*7.5f);
            Position=PuffDrift[I]*(.05f+Push*3.1f)+FVector(T*27,0,35+26*T);
            Width=(165+340*FMath::Sqrt(T))*Variation;Height=(74+65*T)*Variation*(I%2?.81f:1.15f);
            Density=FMath::Clamp(T*52.f,0.f,1.f)*FMath::Pow(1-Phase,1.18f)*.78f;
        }
        else
        {
            // Dark, broad entrained earth rolls from the crater outwards.
            // Unequal lobes separate as momentum dies; no shared stem/cap.
            const float Roll=1-FMath::Exp(-T*1.65f),Curl=FMath::Sin(T*.9f+Variation*4.f);
            Position=PuffDrift[I]*(.18f+Roll*1.45f+T*.13f)+FVector(T*39.f,Curl*19.f,55+65*Roll);
            const float Footprint=(300+360*FMath::Sqrt(T))*Variation;
            Width=Footprint*(1.12f+.08f*(I%2));Height=Footprint*(.50f+.045f*(I%3));
            Density=FMath::Clamp(T*16.f,0.f,1.f)*FMath::Pow(1-Phase,1.08f)*(I%2?1.06f:1.30f);
        }
        if(bMortar){Position*=.86f;Width*=.87f;Height*=.86f;Density*=.96f;}
        else if(!bBullet)
        {
            // Widen the near-ground impulse to match the larger gameplay
            // footprint. Grenades keep a compact low burst, not a taller cap.
            const float Breadth=bGrenade?1.08f:1.12f;
            Position.X*=Breadth;Position.Y*=Breadth;Width*=Breadth;
        }
        // Once real rolling density is available, the residual billboard
        // slots become thin entrained wisps instead of opaque oval balloons.
        if(VolumePuffsV40>0&&!bEarth&&!bGroundWash&&!VolumeLayer)
        {Height*=.40f;Position.Z*=.45f;Width*=1.15f;Density*=.42f;}
        // The transported core supplies the dense ejecta. Its older flat side
        // fans should remain torn wisps, not mask that volume with dark slabs.
        if(VolumePuffsV40>0&&bEarth&&!VolumeLayer)
        {Density*=.32f;Height*=.82f;}
        const bool BloodCarrier=!(CompositionV8221&&CompositionV8221->IsReady())&&!bBullet&&((VolumeLayer&&!bEarth)
            ||(bGroundWash&&(I-EarthPuffs)%2==0));
        if(BloodCarrier)Position+=NearGroundCasualtyBiasV8216(CasualtyDustSourcesV8216,GetActorLocation(),bGrenade?160.f:350.f);
        if(bEarth)
        {
            // One dominant irregular jet and unequal lateral fans; never a
            // mirrored pair of tall, identically orange upright fingers.
            const float Lean=bBullet?FMath::Clamp(float(StrikeDirection.X)*18.f,-18.f,18.f):
                FMath::Clamp(float(PuffDrift[I].X)*.36f+float(PuffDrift[I].Y)*.24f,-57.f,57.f);
            Puffs[I]->SetWorldRotation(Billboard.Quaternion()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Lean)));
        }
        else if(!bGroundWash)
            Puffs[I]->SetWorldRotation(Billboard.Quaternion()*FQuat(FVector::UpVector,FMath::DegreesToRadians(FMath::Sin(Variation*11.f)*9.f+T*(I%2?2.3f:-1.7f))));
        if(bGroundWash)
        {
            Puffs[I]->SetWorldRotation(Billboard.Quaternion()*FQuat(FVector::UpVector,FMath::DegreesToRadians(FMath::Sin(Variation*13.f)*8.f)));
            const FVector World=GetActorLocation()+Position;
            Position.Z+=AWarfareFrontlineGround::HeightAt(World.X,World.Y)-GetActorLocation().Z;
        }
        const bool Particulate=ParticulateSlotsV84412[I]!=0;
        if(Particulate&&!VolumeLayer)
        {
            const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
            const FVector View=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
            FrontlineParticulateV84412::Update(Puffs[I],Materials[I],GetActorLocation()+Position,
                FVector(Width,Width*.88f,Height),View,Density,T);
        }
        else if(VolumeLayer)
        {
            // The actual transported support is fitted into this physical
            // envelope. Runtime expansion must not be multiplied by a sparse
            // source box's expansion again, which hid the early dust impulse.
            // A rounded aftermath lobe sits above the outward ground wash.
            // Its envelope stays bounded independently of the ejecta column.
            const float VolumeHeight=bEarth?Height:FMath::Clamp(Height,bGrenade?90.f:140.f,bGrenade?240.f:420.f);
            const FVector Dimensions(Width*(bEarth?1.f:1.12f),Width*(bEarth?.88f:.92f),VolumeHeight);
            FVector Centre=GetActorLocation()+Position;
            const bool FitWaterContact=!bEarth&&CompositionV8221&&CompositionV8221->UsesArtResetV831();
            const float ContactHeight=FitWaterContact?FitWaterContactV831(I,Centre,Dimensions,VisualDelta):
                AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y);
            Centre.Z=ContactHeight+Dimensions.Z*.44f;
            const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
            const FVector View=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
            if(Particulate)FrontlineParticulateV84412::Update(Puffs[I],Materials[I],Centre,Dimensions,View,Density*1.15f,T);
            else FrontlineSmokeVolumeV35::Update(Puffs[I],Materials[I],Centre,Dimensions,View,Density*1.15f,T);
        }
        else
        {
            Puffs[I]->SetRelativeLocation(Position);Puffs[I]->SetRelativeScale3D(FVector(Width/100,Height/100,1));
            if(bEarth&&!bBullet)
            {
                const FRotationMatrix Basis(Billboard);
                const FVector Right=Basis.GetUnitAxis(EAxis::X),Up=Basis.GetUnitAxis(EAxis::Y),Normal=Basis.GetUnitAxis(EAxis::Z);
                const float Tilt=FMath::Clamp(FVector::DotProduct(PuffDrift[I],Right)*.0035f,-.43f,.43f);
                const FVector Long=(Up+Right*Tilt).GetSafeNormal();
                Puffs[I]->SetWorldRotation(FRotationMatrix::MakeFromXY(FVector::CrossProduct(Long,Normal).GetSafeNormal(),Long).Rotator());
            }
        }
        Materials[I]->SetScalarParameterValue(TEXT("Density"),Density*(VolumeLayer?1.15f:1.f));
        Materials[I]->SetScalarParameterValue(TEXT("Age"),T);
        const float Blood=BloodCarrier?NearGroundCasualtyLoadV8216(BloodLoadV8211,T):0.f;
        Materials[I]->SetScalarParameterValue(TEXT("BloodLoadV8211"),Blood);
        Materials[I]->SetScalarParameterValue(TEXT("FramePhase"),Phase);
        const float Heat=bBullet?0.f:FMath::Exp(-FMath::Max(0.f,Age-.008f)*(bGrenade?48.f:36.f))*(bEarth?.10f:(bGroundWash?.16f:.025f));
        Materials[I]->SetScalarParameterValue(TEXT("FlashHeat"),Heat);
        // Blast dust scatters little light in this overcast scene. These are
        // linear dark grey-brown values, not the old bright smoke-screen tint.
        const FLinearColor SoilColor=bEarth?FLinearColor(.038f,.034f,.029f):(bGroundWash?FLinearColor(.064f,.056f,.046f):(I%2?FLinearColor(.070f,.066f,.060f):FLinearColor(.096f,.088f,.076f)));
        const float Tint=VolumeLayer?0.f:Blood*.36f;
        Materials[I]->SetVectorParameterValue(TEXT("DustColor"),bArmourBulletV8215?FLinearColor(.12f,.115f,.10f):FMath::Lerp(SoilColor,FLinearColor(.16f,.015f,.004f),Tint));
        if(Density<.012f)Puffs[I]->SetVisibility(false);
        if(I==0 && Age>=.10f && Age-VisualDelta<.10f)
            UE_LOG(LogTemp,Display,TEXT("WF_V23_BLAST kind=%d cards=%d pieces=%d flash_life=%.3f density=%.3f local_z_cm=%.1f card_cm=%.1fx%.1f"),
                BlastKind,ActivePuffs,Soil.Num(),FlashLife,Density,Position.Z,Width,Height);
    }
    UpdateClodTrails(Billboard);
}

float AWarfareFrontlineImpact::FitWaterContactV831(int32 PuffIndex,const FVector& Centre,const FVector& Dimensions,float VisualDelta)
{
    const auto* Ground=WaterContactGroundV831.Get();
    const auto* Surface=Ground?Ground->BasinWaterV44.Get():nullptr;
    if(!Ground||Ground->IsHidden()||!Surface||!Surface->IsRegistered()||!Surface->IsVisible()
        ||!Surface->GetStaticMesh()||!Surface->GetMaterial(0))
    {
        if(WaterContactLiftV831.IsValidIndex(PuffIndex))WaterContactLiftV831[PuffIndex]=0.f;
        if(WaterContactTargetV831.IsValidIndex(PuffIndex))WaterContactTargetV831[PuffIndex]=0.f;
        if(WaterContactInitializedV831.IsValidIndex(PuffIndex))WaterContactInitializedV831[PuffIndex]=0;
        return AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y);
    }
    const auto Contact=BlastWaterContactV831(Centre,Dimensions);
    if(!WaterContactLiftV831.IsValidIndex(PuffIndex)||!WaterContactTargetV831.IsValidIndex(PuffIndex)
        ||!WaterContactInitializedV831.IsValidIndex(PuffIndex))return Contact.Height();
    WaterContactTargetV831[PuffIndex]=Contact.Lift;
    if(!WaterContactInitializedV831[PuffIndex])
    {
        // A newly emitted cloud already overlapping water starts at its true
        // local support; subsequent boundary crossings have a finite response.
        WaterContactLiftV831[PuffIndex]=Contact.Lift;WaterContactInitializedV831[PuffIndex]=1;
    }
    else WaterContactLiftV831[PuffIndex]=WaterContactResponseV831(WaterContactLiftV831[PuffIndex],Contact.Lift,VisualDelta);
    return Contact.Ground+WaterContactLiftV831[PuffIndex];
}

void AWarfareFrontlineImpact::UpdateMineV8214(float VisualDelta,const FRotator& Billboard)
{
    UpdateSoil(VisualDelta);
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    const FVector View=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
    const float Bearing=FMath::Atan2(float(PuffDrift[0].Y),float(PuffDrift[0].X));
    for(int32 I=0;I<CorePuffs;++I)
    {
        if(!Materials.IsValidIndex(I)||!Materials[I])continue;
        const bool Earth=I<EarthPuffs,Wash=I>=EarthPuffs&&I<EarthPuffs+WashPuffs;
        if(Earth&&CompositionV8221&&CompositionV8221->IsReady())
        {Puffs[I]->SetVisibility(false);Materials[I]->SetScalarParameterValue(TEXT("BloodLoadV8211"),0.f);continue;}
        const bool Volume=VolumeSlotV828(I,CorePuffs,VolumePuffsV40,BlastKind);
        const float Variation=PuffVariation[I];
        // A transported base must accompany the early outward impulse. The
        // ordinary late smoke slot did not start until .336 s, leaving the
        // first casualty aerosol entirely on cards despite its reserved cube.
        const float Delay=Volume?(Earth?.008f:.045f):
            (Earth?I*.018f:.032f+(Wash?(I-EarthPuffs)*.016f:.10f+(I-EarthPuffs-WashPuffs)*.068f));
        const float T=Age-Delay,Life=Earth?1.18f*Variation:Wash?1.65f*Variation:4.30f*Variation;
        if(T<0.f||T>=Life){Puffs[I]->SetVisibility(false);continue;}
        const float Phase=FMath::Clamp(T/Life,0.f,1.f);
        FVector Position;float Width=0,Height=0,Density=0;
        if(Earth)
        {
            const float Fan=I==0?1.f:I==1?.71f:.53f;
            const float Growth=1.f-FMath::Exp(-T*8.f);
            Position=PuffDrift[I]*(.025f+T*2.4f)+FVector(T*17.f,0,32.f+(475.f+I*40.f)*T-300.f*T*T);
            Width=(175.f+270.f*FMath::Sqrt(T))*Variation;
            Height=(180.f+470.f*Growth)*(1.f-.42f*Phase)*Variation*Fan;
            Density=FMath::Clamp(T*65.f,0.f,1.f)*FMath::Pow(1.f-Phase,.86f)*(I==0?1.62f:1.02f);
            if(VolumePuffsV40>1&&!Volume)Density*=.16f;
        }
        else if(Wash)
        {
            // Each torn lobe travels across the same six-metre blast footprint.
            // A short stagger and different widths preserve gaps and ground detail.
            const float A=Bearing+(I-EarthPuffs)*2.399963f,Push=1.f-FMath::Exp(-T*7.0f);
            const FVector Radial(FMath::Cos(A),FMath::Sin(A),0);
            Position=Radial*(MineRadiusV8214*.66f*Push*(.86f+.12f*Variation))+FVector(T*17.f,0,26.f+T*12.f);
            Width=(140.f+225.f*Push)*Variation;
            Height=(62.f+66.f*Push)*Variation*(I%2?.83f:1.09f);
            Density=FMath::Clamp(T*48.f,0.f,1.f)*FMath::Pow(1.f-Phase,1.23f)*.83f;
        }
        else
        {
            const float Roll=1.f-FMath::Exp(-T*1.7f),Curl=FMath::Sin(T*.91f+Variation*5.f);
            Position=PuffDrift[I]*(.12f+Roll*.95f+T*.09f)+FVector(T*28.f,Curl*22.f,40.f+55.f*Roll);
            Width=(280.f+510.f*Roll)*Variation;
            Height=(145.f+300.f*Roll)*Variation;
            Density=FMath::Clamp(T*19.f,0.f,1.f)*FMath::Pow(1.f-Phase,1.13f)*(I%2?1.06f:.93f)*.70f;
            if(VolumePuffsV40>0&&!Volume){Height*=.32f;Density*=.40f;Position.Z*=.48f;}
        }
        const bool BloodCarrier=!(CompositionV8221&&CompositionV8221->IsReady())&&((Volume&&!Earth)||(Wash&&(I-EarthPuffs)%2==0));
        if(BloodCarrier)Position+=NearGroundCasualtyBiasV8216(CasualtyDustSourcesV8216,GetActorLocation(),350.f);
        Puffs[I]->SetVisibility(Density>.012f);
        const bool Particulate=ParticulateSlotsV84412[I]!=0;
        if(Particulate&&!Volume)
            FrontlineParticulateV84412::Update(Puffs[I],Materials[I],GetActorLocation()+Position,
                FVector(Width,Width*.94f,Height),View,Density,T);
        else if(Volume)
        {
            const float VolumeHeight=Earth?Height:FMath::Clamp(Width*.56f,140.f,400.f);
            const FVector Dimensions(Width*(Earth?1.f:1.16f),Width*(Earth?.82f:.94f),VolumeHeight);
            FVector Centre=GetActorLocation()+Position;
            const bool FitWaterContact=!Earth&&CompositionV8221&&CompositionV8221->UsesArtResetV831();
            const float ContactHeight=FitWaterContact?FitWaterContactV831(I,Centre,Dimensions,VisualDelta):
                AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y);
            Centre.Z=ContactHeight+VolumeHeight*.46f;
            if(Particulate)FrontlineParticulateV84412::Update(Puffs[I],Materials[I],Centre,Dimensions,View,Density*1.15f,T);
            else FrontlineSmokeVolumeV35::Update(Puffs[I],Materials[I],Centre,Dimensions,View,Density*1.15f,T);
        }
        else
        {
            if(Wash)
            {
                const FVector World=GetActorLocation()+Position;
                Position.Z+=AWarfareFrontlineGround::HeightAt(World.X,World.Y)-GetActorLocation().Z;
            }
            const float Lean=Earth?FMath::Clamp(float(PuffDrift[I].X)*.24f+float(PuffDrift[I].Y)*.17f,-38.f,38.f):FMath::Sin(Variation*13.f)*7.f;
            Puffs[I]->SetWorldRotation(Billboard.Quaternion()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Lean)));
            Puffs[I]->SetRelativeLocation(Position);Puffs[I]->SetRelativeScale3D(FVector(Width/100.f,Height/100.f,1));
        }
        Materials[I]->SetScalarParameterValue(TEXT("Density"),Density*(Volume?1.15f:1.f));
        Materials[I]->SetScalarParameterValue(TEXT("Age"),T);Materials[I]->SetScalarParameterValue(TEXT("FramePhase"),Phase);
        Materials[I]->SetScalarParameterValue(TEXT("FlashHeat"),FMath::Exp(-T*48.f)*(Earth?.09f:Wash?.15f:.025f));
        const float Blood=BloodCarrier?NearGroundCasualtyLoadV8216(BloodLoadV8211,T,.60f):0.f;
        Materials[I]->SetScalarParameterValue(TEXT("BloodLoadV8211"),Blood);
        const FLinearColor Colour=Earth?FLinearColor(.036f,.032f,.027f):Wash?FLinearColor(.064f,.054f,.043f):FLinearColor(.073f,.067f,.057f);
        Materials[I]->SetVectorParameterValue(TEXT("DustColor"),Volume?Colour:FMath::Lerp(Colour,FLinearColor(.16f,.015f,.004f),Blood*.36f));
    }
    UpdateClodTrails(Billboard);
}

void AWarfareFrontlineImpact::UpdateSoil(float VisualDelta)
{
    const bool bBullet=BlastKind==3,bGrenade=BlastKind==2,bMortar=BlastKind==1;
    // Bounded visual integration only. There are no Chaos bodies, traces,
    // support hits, damage radii or navigation obstacles in this simulation.
    const float SoilLife=BlastKind==5?4.4f:bBullet?(bHeavyBulletV8215?1.12f:bStrafeBulletV8215?.85f:.55f):(bGrenade?2.8f:(bMortar?4.8f:5.5f));
    if(PenetrationMaterialV8215)PenetrationMaterialV8215->SetScalarParameterValue(TEXT("ImpactAge"),Age);
    if(Age>SoilLife && !Soil.IsEmpty())
    {Soil.Reset();Fragments->SetHasPerInstancePrevTransforms(false);Fragments->ClearInstances();Fragments->SetHasPerInstancePrevTransforms(true);
     FragmentTransformsV835.Reset();PreviousFragmentTransformsV835.Reset();}
    for(int32 I=0;I<Soil.Num();++I)
    {
        auto& F=Soil[I];
        if(!F.bLanded)
        {
            float Remaining=VisualDelta;
            while(Remaining>0)
            {
                const float Dt=FMath::Min(Remaining,1.f/120.f);Remaining-=Dt;
                F.Position+=F.Velocity*Dt+FVector(0,0,-490)*Dt*Dt;F.Velocity.Z-=980*Dt;
                F.Rotation+=FRotator(F.AngularVelocity.Y*Dt,F.AngularVelocity.Z*Dt,F.AngularVelocity.X*Dt);
                const FVector W=GetActorTransform().TransformPosition(F.Position);
                const float Floor=AWarfareFrontlineGround::HeightAt(W.X,W.Y)+3;
                if(W.Z<=Floor)
                {
                    F.Position=GetActorTransform().InverseTransformPosition(FVector(W.X,W.Y,Floor));
                    if(!bBullet&&F.Bounces==0&&F.Velocity.Z< -240.f)
                    {
                        // A single wet, low bounce; most energy stays in the mud.
                        F.Velocity*=FVector(.32f,.32f,-.16f);F.AngularVelocity*=.35f;++F.Bounces;
                    }
                    else{F.bLanded=true;break;}
                }
            }
        }
        const float Fade=FMath::Clamp((SoilLife-Age)/.8f,0.f,1.f);
        FragmentTransformsV835[I]=FTransform(F.Rotation,F.Position,F.Scale*Fade);
    }
    CommitFragmentTransformsV835();
}
void AWarfareFrontlineImpact::CommitFragmentTransformsV835()
{
    if(FragmentTransformsV835.IsEmpty())return;
    // UE 5.8 tracks dirty instance transforms itself. Recreating the entire
    // render proxy here discards history and defeats per-instance velocity.
    Fragments->BatchUpdateInstancesTransforms(0,FragmentTransformsV835,PreviousFragmentTransformsV835,false,false,false);
    PreviousFragmentTransformsV835=FragmentTransformsV835;++FragmentUploadBatchesV835;
}
bool AWarfareFrontlineImpact::HasFastFragmentsV835() const
{
    if(bReviewLegacyDebrisV835||!bProfileConfigured||!IsActorTickEnabled()||IsActorBeingDestroyed()||IsHidden())return false;
    if(BlastKind==4)return ShrapnelArrivalV8211.ContainsByPredicate([&](float Arrival){return Age<Arrival;});
    return Soil.ContainsByPredicate([](const FFrontlineSoilFragment& F){return !F.bLanded&&F.Velocity.SizeSquared()>160000.f;});
}

void AWarfareFrontlineImpact::UpdateClodTrails(const FRotator& Billboard)
{
    const float Window=BlastKind==2?.62f:(BlastKind==1?1.12f:1.45f);
    const FRotationMatrix Axes(Billboard);
    const FVector Right=Axes.GetUnitAxis(EAxis::X),Up=Axes.GetUnitAxis(EAxis::Y);
    const FVector Normal=Axes.GetUnitAxis(EAxis::Z);
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    const FVector View=(Camera?Camera->GetCameraRotation():FRotator(-16,-90,0)).Vector();
    for(int32 I=CorePuffs;I<ActivePuffs;++I)
    {
        const int32 K=Soil.IsEmpty()?INDEX_NONE:((I-CorePuffs)*7+2)%Soil.Num();
        if(!Soil.IsValidIndex(K)||Age<.025f||Age>Window)
        {Puffs[I]->SetVisibility(false);continue;}
        const auto& F=Soil[K];
        if(F.bLanded){Puffs[I]->SetVisibility(false);continue;}
        const FVector Projected=Right*FVector::DotProduct(F.Velocity,Right)+Up*FVector::DotProduct(F.Velocity,Up);
        const float Speed=Projected.Size(),TrailSeconds=FMath::Min(Age,BlastKind==2?.11f:.17f);
        const float Length=FMath::Clamp(Speed*TrailSeconds,26.f,BlastKind==2?88.f:185.f);
        const FVector Long=Speed>1.f?Projected/Speed:Up,Across=FVector::CrossProduct(Long,Normal).GetSafeNormal();
        const bool PowderReset=CompositionV8221&&CompositionV8221->UsesArtResetV831();
        const float Width=(BlastKind==2?13.f:22.f)*(1+Age*.55f)*PuffVariation[I]*(PowderReset?.43f:1.f);
        const float Density=1.12f*FMath::Clamp(Age*25.f,0.f,1.f)*FMath::Pow(1-Age/Window,.75f)*(PowderReset?.34f:1.f);
        const bool Particulate=ParticulateSlotsV84412[I]!=0;
        if(Particulate)
        {
            // A ballistic powder wake follows its actual 3-D velocity. Camera
            // axes remain confined to the preserved legacy billboard branch.
            const FVector Axis=GetActorTransform().TransformVectorNoScale(F.Velocity).GetSafeNormal(.0001f,FVector::UpVector);
            const FVector Side=FVector::CrossProduct(FVector::UpVector,Axis).GetSafeNormal(.0001f,FVector::RightVector);
            const FVector LocalUp=FVector::CrossProduct(Axis,Side);
            const float PhysicalLength=FMath::Clamp(float(F.Velocity.Size())*TrailSeconds,26.f,BlastKind==2?88.f:185.f);
            const FVector LocalDimensions(Width,Width,PhysicalLength);
            const FVector Bounds=(Side.GetAbs()*LocalDimensions.X+LocalUp.GetAbs()*LocalDimensions.Y+Axis.GetAbs()*LocalDimensions.Z)*1.24f;
            const FVector Centre=GetActorTransform().TransformPosition(F.Position)-Axis*PhysicalLength*.43f;
            Materials[I]->SetVectorParameterValue(TEXT("EjectionAxis"),FLinearColor(Axis));
            Materials[I]->SetVectorParameterValue(TEXT("JetRadii"),FLinearColor(LocalDimensions*.5f));
            FrontlineParticulateV84412::ApplySceneLighting(GetWorld(),Materials[I]);
            FrontlineSmokeVolumeV35::Update(Puffs[I],Materials[I],Centre,Bounds,
                View,Density,Age);
        }
        else
        {
            Puffs[I]->SetVisibility(true);
            Puffs[I]->SetWorldRotation(FRotationMatrix::MakeFromXY(Across,Long).Rotator());
            Puffs[I]->SetRelativeLocation(F.Position-Long*Length*.43f);
            Puffs[I]->SetRelativeScale3D(FVector(Width/100,Length/100,1));
            Materials[I]->SetScalarParameterValue(TEXT("Density"),Density);
        }
        Materials[I]->SetScalarParameterValue(TEXT("FramePhase"),FMath::Clamp(.12f+Age/Window*.64f,0.f,1.f));
        Materials[I]->SetScalarParameterValue(TEXT("FlashHeat"),0.f);
        Materials[I]->SetVectorParameterValue(TEXT("DustColor"),PowderReset?FLinearColor(.065f,.061f,.056f):FLinearColor(.036f,.032f,.027f));
    }
}

struct FFrontlineImpactV38Review
{
    static bool AllowedV47(const AWarfareFrontlineGameMode* B)
    { return B&&B->IsProgressionVolatile()&&IsV47FXReview(); }
    static bool Allowed(const AWarfareFrontlineGameMode* B)
    {
        FString UserDir;
        return B&&B->IsProgressionVolatile()&&(FParse::Param(FCommandLine::Get(),TEXT("WFV38Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV45Review")))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    }
    static AWarfareFrontlineImpact* Spawn(AWarfareFrontlineGameMode* B,int32 Kind,float X)
    {
        const FVector At(X,420,AWarfareFrontlineGround::HeightAt(X,420));
        auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
        if(!A)return nullptr;
        if(Kind==3)A->ConfigureStrafeStrike(FVector(1,.3,-1));
        else A->ConfigureBlast(Kind==2,Kind==1,FVector(.8,.2,-1));
        if(!A->IsVisualReady()){A->Destroy();return nullptr;}
        A->SetActorTickEnabled(false);A->Tick(0);return A;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
    {
        const bool Safe=Allowed(B);Check(TEXT("V38 impact review requires isolated volatile profile"),Safe);if(!Safe)return;
        const int32 Units=B->Units.Num(),Strikes=B->Strikes.Num();const float Clock=B->Elapsed;
        TArray<AWarfareFrontlineImpact*> Actors;
        ON_SCOPE_EXIT{for(auto* A:Actors)if(IsValid(A))A->Destroy();};
        bool Ready=true,Profiles=true,Finite=true,Trails=true,Grounded=true,Quiet=true;
        for(int32 K=0;K<4;++K)
        {
            auto* A=Spawn(B,K,-1500+K*1000);Ready&=A!=nullptr;if(!A)continue;Actors.Add(A);
            Profiles&=A->ActivePuffs==(K==3?3:(K==2?9:(K==1?13:16)))&&A->Soil.Num()==(K==3?5:(K==2?18:(K==1?30:44)));
            A->Tick(.08f);
            Profiles&=A->IsFlashVisible()==(K!=3)&&A->GetVisibleSmokeCount()>0;
            bool HasTrail=K==3;
            for(int32 I=A->CorePuffs;I<A->ActivePuffs;++I)HasTrail|=A->Puffs[I]->IsVisible();
            Trails&=HasTrail;
            for(int32 I=0;I<A->ActivePuffs;++I)
                Finite&=A->Materials[I]&&!A->Puffs[I]->GetRelativeLocation().ContainsNaN()
                    &&!A->Puffs[I]->GetRelativeScale3D().ContainsNaN()
                    &&A->Puffs[I]->GetRelativeScale3D().GetMin()>0;
            A->Tick(.52f);Quiet&=!A->IsFlashVisible()&&A->FlashLight->Intensity==0;
            for(const auto& F:A->Soil)
            {
                const FVector W=A->GetActorTransform().TransformPosition(F.Position);
                Grounded&=!W.ContainsNaN()&&W.Z>=AWarfareFrontlineGround::HeightAt(W.X,W.Y)+2.8f&&F.Bounces<=1;
            }
            A->Tick(10.5f);Quiet&=A->GetVisibleSmokeCount()==0&&A->Soil.IsEmpty();
        }
        Check(TEXT("V38 actual four impact profiles retain assets and exact bounded counts"),Ready&&Profiles);
        Check(TEXT("V38 cosmetic fragments carry finite moving soil trails"),Finite&&Trails);
        Check(TEXT("V38 clods stay on terrain and allow at most one wet bounce"),Grounded);
        int32 BurstCount=0,Lit=0;bool Shadowless=true;
        for(int32 I=0;I<6;++I)if(auto* A=Spawn(B,0,-1800+I*650))
        {Actors.Add(A);++BurstCount;Lit+=A->FlashLight->Intensity>0;Shadowless&=!A->FlashLight->CastShadows;}
        Check(TEXT("V38 simultaneous six-shell burst respects three shadowless light ceiling"),BurstCount==6&&Lit<=AWarfareFrontlineImpact::FlashLightBudget&&Shadowless);
        int32 Volumes=0;bool Proxies=true;
        for(auto* A:Actors)if(IsValid(A)&&!A->IsActorBeingDestroyed())
        {
            Volumes+=A->ReservedVolumeCountV8216();
            for(int32 I=0;I<A->CorePuffs;++I)if(VolumeSlotV828(I,A->CorePuffs,A->VolumePuffsV40,A->BlastKind))
                Proxies&=A->Puffs[I]->GetStaticMesh()&&A->Puffs[I]->GetStaticMesh()->GetName()==TEXT("Cube")&&A->Materials[I];
        }
        Check(TEXT("V40 explosive aftermath volume budget uses at most eight existing proxy slots"),Volumes>0&&Volumes<=AWarfareFrontlineImpact::VolumeBudgetV40&&Proxies);
        Check(TEXT("V38 hot core and late smoke finish within their bounded lifetime"),Quiet);
        Check(TEXT("V38 cosmetic review leaves troops strikes and battle clock unchanged"),B->Units.Num()==Units&&B->Strikes.Num()==Strikes&&B->Elapsed==Clock);
    }
    static bool Fixture(AWarfareFrontlineGameMode* B,float PhaseSeconds)
    {
        if(!Allowed(B)||!FMath::IsFinite(PhaseSeconds)||PhaseSeconds<0||PhaseSeconds>8.f)return false;
        const FName Tag(TEXT("V38ImpactPhaseFixture"));
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)
            if(It->ActorHasTag(Tag))It->Destroy();
        bool Ready=true;
        // Left: grenade; middle: mortar; right: field shell. Existing soldiers
        // and terrain supply scale; this path does not apply any blast damage.
        for(int32 I=0;I<3;++I)
        {
            auto* A=Spawn(B,2-I,-1450+I*1450);Ready&=A!=nullptr;if(!A)continue;
            A->Tags.Add(Tag);A->SetLifeSpan(20.f);A->Tick(PhaseSeconds);
        }
        return Ready;
    }
    static bool FixtureV47(AWarfareFrontlineGameMode* B,float Seconds)
    {
        if(!AllowedV47(B)||!FMath::IsFinite(Seconds)||Seconds<0||Seconds>6)return false;
        const FName Tag(TEXT("V47TemporalImpact"));
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)
            if(It->ActorHasTag(Tag))It->Destroy();
        bool Ready=true;
        for(int32 I=0;I<2;++I)
        {
            auto* A=Spawn(B,I==0?2:0,I==0?-1250:1250);Ready&=A!=nullptr;if(!A)continue;
            A->Tags.Add(Tag);A->SetLifeSpan(25.f);
            // Integrate the production cosmetic trajectory at bounded real
            // time steps, then freeze; no material node uses absolute Time.
            float Remaining=Seconds;
            while(Remaining>.00001f){const float Dt=FMath::Min(Remaining,1.f/120.f);A->Tick(Dt);Remaining-=Dt;}
        }
        return Ready;
    }
    static TSharedRef<FJsonObject> DescribeV47(AWarfareFrontlineGameMode* B)
    {
        auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("isolated"),AllowedV47(B));
        if(!AllowedV47(B))return Result;
        TArray<TSharedPtr<FJsonValue>> Actors;
        for(TActorIterator<AWarfareFrontlineImpact> It(B->GetWorld());It;++It)
        {
            auto* A=*It;if(!IsValid(A)||A->IsActorBeingDestroyed()||!A->ActorHasTag(TEXT("V47TemporalImpact")))continue;
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("profile"),A->BlastKind);
            Row->SetNumberField(TEXT("visual_age_seconds"),A->Age);Row->SetBoolField(TEXT("frozen"),!A->IsActorTickEnabled());
            Row->SetBoolField(TEXT("flash_visible"),A->IsFlashVisible());Row->SetNumberField(TEXT("light_lumens"),A->FlashLight->Intensity);
            Row->SetNumberField(TEXT("visible_layers"),A->GetVisibleSmokeCount());Row->SetNumberField(TEXT("ballistic_clods"),A->Soil.Num());
            Row->SetStringField(TEXT("ejecta_material"),A->EjectaBase?A->EjectaBase->GetPathName():TEXT("missing"));
            Row->SetStringField(TEXT("volume_material"),A->VolumeBaseV40?A->VolumeBaseV40->GetPathName():TEXT("missing"));
            bool Finite=true,Grounded=true;
            for(const auto& F:A->Soil)
            {
                const FVector W=A->GetActorTransform().TransformPosition(F.Position);
                Finite&=!W.ContainsNaN()&&!F.Velocity.ContainsNaN()&&!F.Rotation.ContainsNaN();
                Grounded&=W.Z>=AWarfareFrontlineGround::HeightAt(W.X,W.Y)+2.8f&&F.Bounces<=1;
            }
            Row->SetBoolField(TEXT("finite_clods"),Finite);Row->SetBoolField(TEXT("grounded_one_bounce"),Grounded);
            TArray<TSharedPtr<FJsonValue>> Volumes;
            for(int32 I=0;I<A->CorePuffs;++I)if(VolumeSlotV828(I,A->CorePuffs,A->VolumePuffsV40,A->BlastKind))
            {
                auto V=MakeShared<FJsonObject>();auto* C=A->Puffs[I].Get();auto* M=A->Materials[I].Get();
                V->SetBoolField(TEXT("visible"),C->IsVisible());V->SetStringField(TEXT("dimensions_cm"),(C->GetComponentScale()*100).ToString());
                V->SetBoolField(TEXT("secondary_ray_excluded"),!C->bVisibleInRayTracing&&!C->bAffectDistanceFieldLighting&&!C->CastShadow);
                for(const TCHAR* Name:{TEXT("Age"),TEXT("QualitySteps"),TEXT("Density"),TEXT("VolumeProfile")})
                {float Value=0;if(M->GetScalarParameterValue(FMaterialParameterInfo(Name),Value))V->SetNumberField(Name,Value);}
                UTexture* Texture=nullptr;
                if(M->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityVolume")),Texture)&&Texture)
                {
                    V->SetStringField(TEXT("density_atlas"),Texture->GetPathName());
                    if(auto* T=Cast<UTexture2D>(Texture))
                    {V->SetNumberField(TEXT("atlas_width"),T->GetSizeX());V->SetNumberField(TEXT("atlas_height"),T->GetSizeY());V->SetBoolField(TEXT("render_resource"),T->GetResource()!=nullptr);}
                }
                Volumes.Add(MakeShared<FJsonValueObject>(V));
            }
            Row->SetArrayField(TEXT("volumes"),Volumes);Actors.Add(MakeShared<FJsonValueObject>(Row));
        }
        Result->SetArrayField(TEXT("actual_impacts"),Actors);Result->SetNumberField(TEXT("count"),Actors.Num());
        Result->SetNumberField(TEXT("effects_budget"),UFrontlineRenderSettingsSubsystem::EffectsBudgetV43());
        return Result;
    }
    static void RunV47(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
    {
        const bool Safe=AllowedV47(B);Check(TEXT("V47 FX checks use an isolated volatile profile"),Safe);if(!Safe)return;
        const int32 Units=B->Units.Num(),Strikes=B->Strikes.Num();const float Clock=B->Elapsed;
        bool Ready=true,Early=true,Quiet=true,Finite=true,Grounded=true,Atlas=true,Proxy=true,Bounds=true,EarlyVolume=true;
        for(int32 K=0;K<4;++K)
        {
            auto* A=Spawn(B,K,-1800+K*1200);Ready&=A!=nullptr;if(!A)continue;
            ON_SCOPE_EXIT{if(IsValid(A))A->Destroy();};
            Ready&=A->EjectaBase->GetPathName().Contains(TEXT("/V47/"))&&A->RollingBase->GetPathName().Contains(TEXT("/V47/"));
            const int32 InitialClods=A->Soil.Num();Bounds&=InitialClods>0&&InitialClods<=(K==3?5:44)&&A->ActivePuffs<=16;
            for(int32 Step=0;Step<300;++Step)
            {
                A->Tick(1.f/120.f);
                if(Step==6)Early&=A->IsFlashVisible()==(K!=3)&&A->GetVisibleSmokeCount()>0;
                if(Step==29&&K!=3)
                    for(int32 I=0;I<A->CorePuffs;++I)if(VolumeSlotV828(I,A->CorePuffs,A->VolumePuffsV40,A->BlastKind))
                        EarlyVolume&=A->Puffs[I]->IsVisible();
                if(Step==100)Quiet&=!A->IsFlashVisible()&&A->FlashLight->Intensity==0;
                for(const auto& F:A->Soil)
                {
                    const FVector W=A->GetActorTransform().TransformPosition(F.Position);
                    Finite&=!W.ContainsNaN()&&!F.Velocity.ContainsNaN();
                    Grounded&=W.Z>=AWarfareFrontlineGround::HeightAt(W.X,W.Y)+2.8f&&F.Bounces<=1;
                }
            }
            for(int32 I=0;I<A->CorePuffs;++I)if(VolumeSlotV828(I,A->CorePuffs,A->VolumePuffsV40,A->BlastKind))
            {
                auto* C=A->Puffs[I].Get();auto* M=A->Materials[I].Get();UTexture* T=nullptr;
                const bool ThisProxy=!C->bVisibleInRayTracing&&!C->bAffectDistanceFieldLighting&&!C->CastShadow&&!C->bVisibleInReflectionCaptures;
                const bool HasAtlas=M->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityVolume")),T);
                const auto* Tex=Cast<UTexture2D>(T);
                const bool ThisAtlas=HasAtlas&&Tex&&Tex->GetPathName().Contains(TEXT("T_FlowDensity64_V47"))&&Tex->GetSizeX()==4096&&Tex->GetSizeY()==2048;
                Proxy&=ThisProxy;Atlas&=ThisAtlas;
                UE_LOG(LogTemp,Display,TEXT("WF_V47_FX_BINDING_CHECK profile=%d slot=%d age=%.4f material=%s atlas=%s size=%dx%d rt=%d df=%d shadow=%d reflection_capture=%d atlas_ok=%d proxy_ok=%d"),
                    K,I,A->Age,*M->GetPathName(),*GetPathNameSafe(T),Tex?Tex->GetSizeX():0,Tex?Tex->GetSizeY():0,
                    C->bVisibleInRayTracing,C->bAffectDistanceFieldLighting,C->CastShadow,C->bVisibleInReflectionCaptures,ThisAtlas,ThisProxy);
                Bounds&=C->GetComponentScale().GetMin()>0&&!C->GetComponentLocation().ContainsNaN();
            }
            A->Tick(10.6f);Quiet&=A->GetVisibleSmokeCount()==0&&A->Soil.IsEmpty();
        }
        Check(TEXT("V47 actual impact profiles use connected derivative materials and bounded layers"),Ready&&Bounds);
        Check(TEXT("V47 short hot impulse exists early and ends before the residual smoke"),Early&&Quiet);
        Check(TEXT("V47 actual shell and grenade density accompanies the first quarter-second impulse"),EarlyVolume);
        Check(TEXT("V47 actual thrown soil remains finite and obeys terrain plus one wet bounce"),Finite&&Grounded);
        Check(TEXT("V47 real volume materials resolve the correct animated density atlas"),Atlas);
        Check(TEXT("V47 real volume proxies never occlude secondary rays or reflection captures"),Proxy);
        Check(TEXT("V47 cosmetic validation leaves combat actors strikes and clock unchanged"),B->Units.Num()==Units&&B->Strikes.Num()==Strikes&&B->Elapsed==Clock);
    }
};

void FrontlineImpactV38::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{FFrontlineImpactV38Review::Run(B,Check);}
bool FrontlineImpactV38::SetupVisualFixture(AWarfareFrontlineGameMode* B,float PhaseSeconds)
{return FFrontlineImpactV38Review::Fixture(B,PhaseSeconds);}

void FrontlineImpactV40::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{
    FFrontlineImpactV38Review::Run(B,Check);
    Check(TEXT("V40 smoke volume material is available"),FrontlineSmokeVolumeV35::MaterialV40()!=nullptr);
}
bool FrontlineImpactV40::SetupVisualFixture(AWarfareFrontlineGameMode* B,float PhaseSeconds)
{return FFrontlineImpactV38Review::Fixture(B,PhaseSeconds);}

bool FrontlineFXV47::SetupTemporalFixture(AWarfareFrontlineGameMode* B,float AgeSeconds)
{return FFrontlineImpactV38Review::FixtureV47(B,AgeSeconds);}
TSharedRef<FJsonObject> FrontlineFXV47::Describe(AWarfareFrontlineGameMode* B)
{return FFrontlineImpactV38Review::DescribeV47(B);}
void FrontlineFXV47::RunFocusedChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{FFrontlineImpactV38Review::RunV47(B,Check);}


void AWarfareFrontlineImpact::RunChecksV8216(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B);
    Check(TEXT("v8216_effects_require_absolute_project_saved_disposable_nosave_profile"),Safe);if(!Safe)return;
    const auto Units=B->Units;const auto Squads=B->Squads;const auto Phase=B->Phase;const float Clock=B->Elapsed;
    const int32 Winner=B->BattleOutroWinner,Cas0=B->Casualties[0],Cas1=B->Casualties[1];const float Outro=B->BattleOutroElapsed;
    const float M0=B->Morale[0],M1=B->Morale[1],D0=B->DamageDealt[0],D1=B->DamageDealt[1],Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
    const auto C0=B->CombatRecordV65[0],C1=B->CombatRecordV65[1];const auto Message=B->LastMessage;
    TSet<TWeakObjectPtr<AActor>> Prior;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Prior.Add(*I);
    ON_SCOPE_EXIT{
        TArray<AActor*> Added;for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Prior.Contains(*I))Added.Add(*I);
        for(auto* A:Added)if(IsValid(A))A->Destroy();
        B->Units=Units;B->Squads=Squads;B->Phase=Phase;B->Elapsed=Clock;B->BattleOutroWinner=Winner;B->BattleOutroElapsed=Outro;
        B->Casualties[0]=Cas0;B->Casualties[1]=Cas1;B->Morale[0]=M0;B->Morale[1]=M1;B->DamageDealt[0]=D0;B->DamageDealt[1]=D1;
        B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;B->CombatRecordV65[0]=C0;B->CombatRecordV65[1]=C1;B->LastMessage=Message;
    };
    B->Units.Reset();B->Squads.Reset();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Morale[0]=B->Morale[1]=100;
    auto Make=[&](int32 Id,EFrontlineRole UnitRole,FVector Feet){
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
        U->Initialize(B,Id,0,UnitRole,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->SetActorLocation(Feet+FVector(0,0,88));
        B->Units.Add(U);FFrontlineSquad S;S.Id=Id;S.Team=0;S.Role=UnitRole;S.Members.Add(U);B->Squads.Add(S);return U;
    };
    auto* AT=Make(821601,EFrontlineRole::AntiTankRifle,FVector(0,-1200,2000));
    Check(TEXT("v8216_Tankgewehr_actual_profile_has_6480cm_range_and_5040cm_contact"),AT&&FMath::IsNearlyEqual(FrontlineWeapons::ProfileFor(AT).FireRangeCm,6480.f)
        &&FMath::IsNearlyEqual(FrontlineWeapons::ProfileFor(AT).AdvanceContactCm,5040.f));
    int32 Id=821610;
    const auto Admits=[&](int32 N,bool Heavy){const uint32 K=uint32(N)*92821u^68917u^283923u;return Heavy?((K*1664525u+1013904223u)%100u)<82u:(K%100u)<64u;};
    while(!Admits(Id,true))++Id;
    auto* HeavyVictim=Make(Id,EFrontlineRole::Rifle,FVector(-300,-800,2000));
    auto* Wound=AWarfareFrontlineWoundFX::Get(B->GetWorld());
    Check(TEXT("v8216_effects_review_clears_only_disposable_prior_cosmetic_fixture"),Wound&&Wound->ResetFocusedReviewV8216());
    Check(TEXT("v8216_disposable_review_waits_for_exact_role_async_pair_preload"),Wound&&HeavyVictim&&Wound->WaitForPreparedRoleV8216(HeavyVictim->Soldier->GetSkeletalMeshAsset()));
    if(HeavyVictim)HeavyVictim->ReceiveHit(1,0,1,FVector(1,.2f,.08f),EFrontlineDamageKind::HeavyBullet,1);
    Check(TEXT("v8216_heavy_round_limb_appearance_changes_at_hit_before_authored_fall_physics"),Wound&&HeavyVictim&&!HeavyVictim->IsAlive()
        &&HeavyVictim->GetDeathState().bSeverAttempted&&!HeavyVictim->GetDeathState().bPhysicsStarted&&Wound->HasLimbLossV8216(HeavyVictim));
    ++Id;while(!Admits(Id,false))++Id;
    auto* BlastVictim=Make(Id,EFrontlineRole::Rifle,FVector(300,-800,2000));const int32 BodyMist=Wound?Wound->ActiveBodyMistV8216():0;
    if(BlastVictim)BlastVictim->ReceiveHit(1,0,1,FVector(.8f,-.4f,.5f),EFrontlineDamageKind::Artillery,.95f);
    Check(TEXT("v8216_major_explosion_can_detach_visual_limb_on_committed_hit"),Wound&&BlastVictim&&Wound->HasLimbLossV8216(BlastVictim)
        &&BlastVictim->GetDeathState().Age==0&&BlastVictim->GetDeathState().IncomingDirection.Z>.1f);
    Check(TEXT("v8216_explosive_casualty_does_not_spawn_torso_blood_fountain"),Wound&&Wound->ActiveBodyMistV8216()==BodyMist);
    // Use the actual strike resolver, not a cosmetic blood-load setter. This
    // disposable fixture is outside all live field sites and tank positions.
    const FVector RealHitPoint(42000,6500,2400);
    auto* RealHit=Make(821680,EFrontlineRole::Rifle,RealHitPoint);
    auto* RadiusMiss=Make(821681,EFrontlineRole::Rifle,RealHitPoint+FVector(240,0,0));
    FFrontlineStrike Actual;Actual.Team=1;Actual.Target=RealHitPoint;Actual.Origin=RealHitPoint+FVector(-300,0,1600);Actual.Radius=200;
    const int32 MistBeforeResolve=Wound?Wound->ActiveBodyMistV8216():0;
    if(RealHit)B->ResolveFieldStrikeV32(Actual);
    AWarfareFrontlineImpact* Committed=nullptr;
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)
        if(I->GetActorLocation().Equals(RealHitPoint,.01f))Committed=*I;
    Check(TEXT("v8216_actual_shell_resolution_kills_struck_target_and_leaves_radius_miss_alive"),RealHit&&RadiusMiss&&!RealHit->IsAlive()&&RadiusMiss->IsAlive()
        &&Actual.bResolved&&Committed&&Committed->CasualtyDustSourcesV8216.Num()==1&&Committed->CasualtyDustSourcesV8216[0].Equals(RealHitPoint,.01f));
    if(Committed)
    {
        Committed->bFirstTick=false;Committed->Tick(.20f);bool BoundBlood=false,MainEarthClean=true,LowVolume=false;
        for(int32 I=0;I<Committed->CorePuffs;++I)
            if(VolumeSlotV828(I,Committed->CorePuffs,Committed->VolumePuffsV40,Committed->BlastKind))
            {
                float Load=0;const bool Read=Committed->Materials[I]&&Committed->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BloodLoadV8211")),Load);
                BoundBlood|=Read&&Load>0;
                if(I<Committed->EarthPuffs)MainEarthClean&=Read&&Load==0;
                else if(Read&&Load>0){const FVector Dimensions=Committed->Puffs[I]->GetComponentScale()*100.f;
                    LowVolume|=Dimensions.Z<=235.01f&&Dimensions.X>Dimensions.Z*1.4f&&Load<Committed->BloodLoadV8211;}
            }
        const bool IndependentIncident=Committed->CompositionV8221&&Committed->CompositionV8221->IsReady();
        if(IndependentIncident)
        {
            BoundBlood=Committed->DescribeCompositionV8221()->GetNumberField(TEXT("visible_incident_aerosol"))>0;
            LowVolume=BoundBlood;
            Committed->CheckCompositionV8221(Check);
        }
        Check(IndependentIncident?TEXT("v8221_actual_shell_casualty_blood_has_a_single_explosion_incident_producer"):
            TEXT("v8216_actual_shell_casualty_blood_is_bound_to_true_volume_earth_plume"),BoundBlood
            &&Wound&&Wound->ActiveBodyMistV8216()==MistBeforeResolve);
        Check(IndependentIncident?TEXT("v8221_actual_blast_keeps_retained_grey_smoke_free_of_duplicate_casualty_tint"):
            TEXT("v8216_actual_blast_keeps_rising_earth_grey_and_blood_inside_low_outward_volume"),MainEarthClean&&LowVolume);
        Committed->Tick(.60f);bool ExpiredBlood=true;
        for(int32 I=0;I<Committed->CorePuffs;++I)if(Committed->Materials[I])
        {float Load=0;if(Committed->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BloodLoadV8211")),Load))ExpiredBlood&=Load==0;}
        Check(TEXT("v8216_actual_incident_blood_fades_before_persistent_grey_dust_and_flash_stays_short"),ExpiredBlood
            &&Committed->GetVisibleSmokeCount()>0&&!Committed->IsFlashVisible()&&Committed->FlashLight->Intensity==0);
    }
    FFrontlineStrike Empty=Actual;Empty.bResolved=false;Empty.Target=RealHitPoint+FVector(1800,0,0);Empty.Origin=Empty.Target+FVector(0,0,1600);
    B->ResolveFieldStrikeV32(Empty);AWarfareFrontlineImpact* EmptyImpact=nullptr;
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(I->GetActorLocation().Equals(Empty.Target,.01f))EmptyImpact=*I;
    Check(TEXT("v8216_actual_shell_miss_retains_earth_effect_and_has_no_casualty_blood"),EmptyImpact&&EmptyImpact->CasualtyDustSourcesV8216.IsEmpty()
        &&EmptyImpact->BloodLoadV8211==0&&RadiusMiss&&RadiusMiss->IsAlive());
    // Every imported role uses a geometry-matched body/fragment pair; missing
    // variant names are reported explicitly, never hidden by the old calf.
    for(int32 Choice=0;Choice<3;++Choice)
    {
        static const TCHAR* Labels[]={TEXT("CalfRight"),TEXT("ForearmLeft"),TEXT("ThighLeft")};
        int32 VariantId=821690+Choice*10;
        while(!Admits(VariantId,true)||((uint32(VariantId)*73856093u)%3u)!=uint32(Choice))++VariantId;
        auto* Victim=Make(VariantId,EFrontlineRole::Rifle,RealHitPoint+FVector(Choice*160,-450,0));
        if(Victim)Victim->ReceiveHit(1,0,1,FVector(1,.18f,.05f),EFrontlineDamageKind::HeavyBullet,1);
        const FString ActualVariant=Wound?Wound->LossVariantV8216(Victim):FString();
        Check(FString::Printf(TEXT("v8216_true_mesh_separation_at_hit_variant_%s"),Labels[Choice]),ActualVariant.Contains(Labels[Choice])&&ActualVariant.EndsWith(TEXT("_V8216")));
        UE_LOG(LogTemp,Display,TEXT("WF_V8216_VARIANT_ACTUAL requested=%s actual=%s"),Labels[Choice],*ActualVariant);
    }
    auto* Air=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(FVector(300,-800,2450),FRotator::ZeroRotator);
    if(Air)
    {
        Air->ConfigureAirburstV64(1200,FVector(1,0,-1));Air->SetCasualtyDustV8216({FVector(300,-800,2000)});Air->bFirstTick=false;Air->Tick(.20f);
        float Aloft=1,AtGround=0;bool AloftRead=Air->Materials.IsValidIndex(0)&&Air->Materials[0]
            &&Air->Materials[0]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BloodLoadV8211")),Aloft);
        const int32 Ground=Air->CorePuffs;
        const bool GroundRead=Air->Materials.IsValidIndex(Ground)&&Air->Materials[Ground]
            &&Air->Materials[Ground]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BloodLoadV8211")),AtGround);
        const bool IndependentIncident=Air->CompositionV8221&&Air->CompositionV8221->IsReady();
        const bool IncidentVisible=IndependentIncident?AtGround==0&&Air->DescribeCompositionV8221()->GetNumberField(TEXT("visible_incident_aerosol"))>0:AtGround>0;
        if(IndependentIncident)Air->CheckCompositionV8221(Check);
        Check(IndependentIncident?TEXT("v8221_airburst_casualty_aerosol_uses_fragment_incident_and_preserves_grey_overhead_smoke"):
            TEXT("v8216_airburst_casualty_aerosol_lives_inside_struck_ground_dust_not_overhead_puff"),AloftRead&&Aloft==0&&GroundRead&&IncidentVisible
            &&Air->ShrapnelGroundV8211.Num()>0&&Air->ShrapnelGroundV8211[0].Equals(FVector(300,-800,2009),.01f));
        Check(TEXT("v8216_airburst_has_true_volume_at_overhead_and_first_ground_strike"),Air->VolumePuffsV40==2
            &&VolumeSlotV828(0,Air->CorePuffs,2,4)&&VolumeSlotV828(Air->CorePuffs,Air->CorePuffs,2,4));
    }
    auto* Mine=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(RealHitPoint+FVector(4200,0,0),FRotator::ZeroRotator);
    if(Mine)
    {
        Mine->ConfigureMineV8214(600.f);Mine->SetCasualtyDustV8216({Mine->GetActorLocation()+FVector(80,40,0)});
        Mine->bFirstTick=false;Mine->Tick(.04f);bool AwaitingLow=false;
        for(int32 I=Mine->EarthPuffs;I<Mine->CorePuffs;++I)if(VolumeSlotV828(I,Mine->CorePuffs,Mine->VolumePuffsV40,Mine->BlastKind))
            AwaitingLow|=!Mine->Puffs[I]->IsVisible();
        Check(TEXT("v8216_mine_ground_volume_remains_hidden_before_45ms_release"),AwaitingLow);
        Mine->Tick(.16f);bool LowBlood=false,HighGrey=true;
        for(int32 I=0;I<Mine->CorePuffs;++I)if(VolumeSlotV828(I,Mine->CorePuffs,Mine->VolumePuffsV40,Mine->BlastKind))
        {
            float Load=0;const bool Read=Mine->Materials[I]&&Mine->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BloodLoadV8211")),Load);
            if(I<Mine->EarthPuffs)HighGrey&=Read&&Load==0;
            else{const FVector Dimensions=Mine->Puffs[I]->GetComponentScale()*100.f;LowBlood|=Read&&Load>0&&Mine->Puffs[I]->IsVisible()&&Dimensions.Z<=220.01f&&Dimensions.X>Dimensions.Z*1.4f;}
        }
        const bool IndependentIncident=Mine->CompositionV8221&&Mine->CompositionV8221->IsReady();
        if(IndependentIncident)
        {
            LowBlood=Mine->DescribeCompositionV8221()->GetNumberField(TEXT("visible_incident_aerosol"))>0;
            Mine->CheckCompositionV8221(Check);
        }
        Check(IndependentIncident?TEXT("v8221_mine_casualty_aerosol_has_single_incident_producer_and_preserves_radius_and_grey_smoke"):
            TEXT("v8216_mine_low_casualty_aerosol_keeps_six_metre_footprint_and_grey_rising_soil"),LowBlood&&HighGrey&&Mine->MineRadiusV8214==600.f);
    }
    auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld());
    if(Fire&&AT)
    {
        for(int32 I=0;I<24;++I){Fire->EmitClothing(AT,.05f,1.f);Fire->EmitBurningWreck(FVector(1000,600,2000),.05f,1.45f,2.f,true,1.f);}
        const auto V=Fire->VisualStateV8214();
        Check(TEXT("v8216_burn_uses_bone_attached_flames_source_oriented_ribbons_and_true_volume_soot"),Fire->IsReady()
            &&V.AttachedClothing>0&&V.SourceRibbonsV8216>0&&V.VolumeSootV8216>0&&V.VolumeSootV8216<=AFrontlineFuelFireV33::VolumeBudgetV8216
            &&V.PoolUsed<=V.PoolCapacity&&V.PoolCapacity==304);
    }
    if(B->Trenches.Num()>0)
    {
        int32 Trench=INDEX_NONE;for(int32 I=0;I<B->Trenches.Num();++I)if(B->Trenches[I].Owner==0){Trench=I;break;}
        if(Trench!=INDEX_NONE)
        {
            const FVector P(FrontlineFieldV32::TrenchCentreX(Trench,0),0,2200);auto* Protected=Make(821660,EFrontlineRole::Rifle,P);
            if(Protected)
            {
                FFrontlineStrike Strike;Strike.bShrapnelV64=true;Strike.Target=P+FVector(0,0,430);Strike.Radius=1200;
                auto* Squad=B->FindSquad(Protected->SquadId);Squad->Trench=Trench;const float Cover=B->CoverFor(Protected),Sheltered=B->ExplosionHitChance(Strike,Protected);
                Squad->Trench=INDEX_NONE;const float Open=B->ExplosionHitChance(Strike,Protected);
                Check(TEXT("v8216_shrapnel_trench_protection_uses_actual_cover_with_open_ground_unchanged"),Cover>0&&Open>.85f
                    &&FMath::IsNearlyEqual(Sheltered,Open*(1-Cover*.80f),.001f));
            }
        }
    }
}

TSharedRef<FJsonObject> AWarfareFrontlineImpact::DescribeCompositionV8221() const
{
    auto J=CompositionV8221?CompositionV8221->Describe():MakeShared<FJsonObject>();
    J->SetNumberField(TEXT("retained_smoke_components_visible"),GetVisibleSmokeCount());
    J->SetNumberField(TEXT("ballistic_earth_instances"),Fragments?Fragments->GetInstanceCount():0);
    J->SetStringField(TEXT("actual_soil_mesh"),Fragments?GetPathNameSafe(Fragments->GetStaticMesh()):FString());
    if(Fragments&&Fragments->GetStaticMesh())
    {
        const FVector Bounds=Fragments->GetStaticMesh()->GetBounds().BoxExtent*2;
        J->SetNumberField(TEXT("soil_mesh_extent_cm_x"),Bounds.X);J->SetNumberField(TEXT("soil_mesh_extent_cm_y"),Bounds.Y);J->SetNumberField(TEXT("soil_mesh_extent_cm_z"),Bounds.Z);
    }
    J->SetStringField(TEXT("retained_volume_base"),GetPathNameSafe(VolumeBaseV40));
    int32 NewDust=0,VisibleDust=0,PreservedSmoke=0,VisiblePreservedSmoke=0;
    TArray<TSharedPtr<FJsonValue>> DustBindings;
    for(int32 I=0;I<ActivePuffs;++I)if(Puffs.IsValidIndex(I)&&Puffs[I]&&Materials.IsValidIndex(I)&&Materials[I])
    {
        const bool New=ParticulateSlotsV84412.IsValidIndex(I)&&ParticulateSlotsV84412[I]!=0;
        if(New)
        {
            ++NewDust;VisibleDust+=Puffs[I]->IsVisible();
            auto L=MakeShared<FJsonObject>();L->SetNumberField(TEXT("slot"),I);L->SetBoolField(TEXT("visible"),Puffs[I]->IsVisible());
            L->SetStringField(TEXT("material"),GetPathNameSafe(Materials[I]->GetBaseMaterial()));
            L->SetStringField(TEXT("mesh"),GetPathNameSafe(Puffs[I]->GetStaticMesh()));
            L->SetStringField(TEXT("world_dimensions_cm"),(Puffs[I]->GetComponentScale()*100.f).ToString());
            DustBindings.Add(MakeShared<FJsonValueObject>(L));
        }
        else if(BlastKind==4?I<CorePuffs:I>=EarthPuffs+WashPuffs&&I<CorePuffs)
        {++PreservedSmoke;VisiblePreservedSmoke+=Puffs[I]->IsVisible();}
    }
    J->SetNumberField(TEXT("particulate_puff_bindings_v84412"),NewDust);
    J->SetNumberField(TEXT("visible_particulate_puffs_v84412"),VisibleDust);
    J->SetNumberField(TEXT("preserved_smoke_puff_bindings_v84412"),PreservedSmoke);
    J->SetNumberField(TEXT("visible_preserved_smoke_puffs_v84412"),VisiblePreservedSmoke);
    J->SetArrayField(TEXT("particulate_puff_details_v84412"),DustBindings);
    J->SetNumberField(TEXT("fragment_upload_batches_v835"),FragmentUploadBatchesV835);
    J->SetNumberField(TEXT("fragment_previous_transforms_v835"),Fragments?Fragments->PerInstancePrevTransform.Num():0);
    TArray<double> Sizes;
    if(Fragments&&Fragments->GetStaticMesh())
    {
        const FVector Diameter=Fragments->GetStaticMesh()->GetBounds().BoxExtent*2.f;
        for(const auto& F:Soil)Sizes.Add((Diameter*F.Scale).GetMax());
    }
    Sizes.Sort();
    J->SetNumberField(TEXT("fragment_max_diameter_cm_v835"),Sizes.IsEmpty()?0:Sizes.Last());
    J->SetNumberField(TEXT("fragment_median_diameter_cm_v835"),Sizes.IsEmpty()?0:Sizes[Sizes.Num()/2]);
    return J;
}
void AWarfareFrontlineImpact::RunDebrisChecksV835(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV835Review"));
    Check(TEXT("v835_debris_requires_disposable_actual_world"),Safe);if(!Safe)return;
    const int32 Units=B->Units.Num(),Strikes=B->Strikes.Num();const float Clock=B->Elapsed;
    const FVector At(400,720,AWarfareFrontlineGround::HeightAt(400,720));
    auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
    Check(TEXT("v835_debris_production_actor_created"),A!=nullptr);if(!A)return;
    ON_SCOPE_EXIT {if(IsValid(A))A->Destroy();};
    A->SetActorTickEnabled(false);
    for(int32 Profile:{0,1,2,4,5,3})
    {
        const FString Name=FString::Printf(TEXT("v835_debris_profile_%d_"),Profile);
        if(Profile==5)A->ConfigureMineV8214(600.f);
        else if(Profile==4)A->ConfigureAirburstV64(1200.f,FVector(.8,.2,-1));
        else if(Profile==3)A->ConfigureHeavyBulletV8215(FVector(.8,.2,-1));
        else A->ConfigureBlast(Profile==2,Profile==1,FVector(.8,.2,-1));
        Check(Name+TEXT("previous_array_matches_instances_after_reconfiguration"),A->Soil.Num()==A->Fragments->GetInstanceCount()
            &&A->Soil.Num()==A->FragmentTransformsV835.Num()&&A->Soil.Num()==A->PreviousFragmentTransformsV835.Num()
            &&A->Soil.Num()==A->Fragments->PerInstancePrevTransform.Num());
        if(Profile!=3&&Profile!=4)
        {
            const auto J=A->DescribeCompositionV8221();
            const float Limit=Profile==2?7.f:Profile==1?10.f:Profile==5?10.f:12.f;
            Check(Name+TEXT("authored_mesh_real_extent_has_fine_chips_and_sparse_clods"),J->GetNumberField(TEXT("fragment_max_diameter_cm_v835"))<=Limit*1.321f
                &&J->GetNumberField(TEXT("fragment_median_diameter_cm_v835"))<Limit*.61f);
        }
        A->bFirstTick=false;A->Tick(.03f);const auto Prior=A->FragmentTransformsV835;
        A->Tick(.016f);bool Exact=!Prior.IsEmpty(),Moved=false;
        for(int32 I=0;I<Prior.Num();++I)
        {
            Exact&=A->Fragments->PerInstancePrevTransform[I].Equals(Prior[I].ToMatrixWithScale(),.0001f);
            Moved|=!A->FragmentTransformsV835[I].Equals(Prior[I],.0001f);
        }
        Check(Name+TEXT("second_tick_submits_exact_previous_transform_and_moving_current"),Exact&&Moved&&A->FragmentUploadBatchesV835==2);
    }
    A->ConfigureBlast(false,false);A->bFirstTick=false;A->Tick(6.f);
    Check(TEXT("v835_expired_debris_clears_current_previous_and_instances"),A->Soil.IsEmpty()&&A->FragmentTransformsV835.IsEmpty()
        &&A->PreviousFragmentTransformsV835.IsEmpty()&&A->Fragments->GetInstanceCount()==0&&A->Fragments->PerInstancePrevTransform.IsEmpty());
    Check(TEXT("v835_frozen_review_does_not_apply_stale_object_shutter"),!A->HasFastFragmentsV835());
    A->ConfigureHeavyBulletV8215(FVector(.8,.2,-1),true);bool InitialHistory=true;
    for(int32 I=0;I<A->Soil.Num();++I)
    {
        const FTransform Expected(A->Soil[I].Rotation,A->Soil[I].Position,A->Soil[I].Scale);
        InitialHistory&=A->FragmentTransformsV835[I].Equals(Expected,.0001f)
            &&A->Fragments->PerInstancePrevTransform[I].Equals(Expected.ToMatrixWithScale(),.0001f);
    }
    Check(TEXT("v835_armour_penetration_shape_is_bound_before_initial_velocity_history"),InitialHistory&&!A->Soil.IsEmpty());
    Check(TEXT("v835_debris_checks_preserve_gameplay_state"),Units==B->Units.Num()&&Strikes==B->Strikes.Num()&&Clock==B->Elapsed);
}
void AWarfareFrontlineImpact::CheckCompositionV8221(TFunctionRef<void(const FString&,bool)> Check) const
{
    if(CompositionV8221)CompositionV8221->Check(Check);
    else Check(TEXT("v8221_blast_composition_component_exists"),false);
    if(CompositionV8221&&CompositionV8221->IsReady())
    {
        bool LegacyBloodCleared=true;int32 Read=0;
        for(int32 I=0;I<ActivePuffs;++I)if(Materials.IsValidIndex(I)&&Materials[I])
        {
            float Load=0;if(Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("BloodLoadV8211")),Load))
            {++Read;LegacyBloodCleared&=FMath::Abs(Load)<.0001f;}
        }
        Check(TEXT("v8221_incident_aerosol_has_exclusive_blood_ownership_with_legacy_tint_cleared"),Read>0&&LegacyBloodCleared);
        if(BlastKind!=4)
        {
            bool LegacyEarthHidden=true;for(int32 I=0;I<EarthPuffs;++I)LegacyEarthHidden&=!Puffs[I]->IsVisible();
            Check(TEXT("v8221_oriented_throw_replaces_old_upright_earth_only"),LegacyEarthHidden);
        }
    }
}

void AWarfareFrontlineImpact::RunReconfigurationChecksV8221(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B);
    Check(TEXT("v8221_blast_reconfigure_requires_strict_disposable_actual_world"),Safe);if(!Safe)return;
    const int32 Units=B->Units.Num(),Strikes=B->Strikes.Num();const float Clock=B->Elapsed;
    const FVector At(400,720,AWarfareFrontlineGround::HeightAt(400,720));
    auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(At,FRotator::ZeroRotator);
    Check(TEXT("v8221_blast_reconfiguration_real_actor_created"),A!=nullptr);if(!A)return;
    ON_SCOPE_EXIT {if(IsValid(A))A->Destroy();};
    A->SetActorTickEnabled(false);
    auto Binding=[&](bool Shell)
    {
        bool Correct=true;for(int32 I=0;I<A->ActivePuffs;++I)
        {
            const bool Volume=(Shell&&VolumeSlotV828(I,A->CorePuffs,A->VolumePuffsV40,A->BlastKind))||A->ParticulateSlotsV84412[I]!=0;
            const FString Path=GetPathNameSafe(A->Puffs[I]->GetStaticMesh());
            Correct&=Path==FString(Volume?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Engine/BasicShapes/Plane.Plane"));
            if(A->ParticulateSlotsV84412[I])Correct&=GetPathNameSafe(A->Materials[I]->GetBaseMaterial())
                ==TEXT("/Game/Warfare/Field/V84413/Materials/M_MineralDustVolume_V84413.M_MineralDustVolume_V84413");
        }
        return Correct;
    };
    A->ConfigureBlast(false,false);A->SetCasualtyDustV8216({At});A->Tick(0);A->Tick(.20f);
    Check(TEXT("v8221_HE_actual_volume_and_new_soil_binding"),A->IsVisualReady()&&Binding(true)
        &&A->Fragments->GetStaticMesh()==UFrontlineBlastCompositionV8221::SoilMesh()&&A->CompositionV8221->IsReady());
    A->ConfigureBulletStrike();A->Tick(0);A->Tick(.20f);
    Check(A->ParticulateSlotsV84412[0]?TEXT("v84412_HE_to_bullet_restores_dedicated_dust_and_legacy_nonexplosive_soil"):
        TEXT("v8221_HE_to_bullet_restores_flat_proxies_and_legacy_nonexplosive_soil"),A->IsVisualReady()&&Binding(false)
        &&GetPathNameSafe(A->Fragments->GetStaticMesh())==TEXT("/Game/Warfare/Field/V12/Meshes/SM_EarthClod_V12.SM_EarthClod_V12")
        &&!A->CompositionV8221->IsReady()&&A->BloodLoadV8211==0&&A->CasualtyDustSourcesV8216.IsEmpty());
    A->ConfigureBlast(false,false);A->Tick(0);A->Tick(.20f);
    Check(TEXT("v8221_bullet_to_HE_restores_volumes_and_new_soil_without_old_casualty_context"),A->IsVisualReady()&&Binding(true)
        &&A->CompositionV8221->IsReady()&&A->BloodLoadV8211==0&&A->CasualtyDustSourcesV8216.IsEmpty());
    A->CheckCompositionV8221(Check);A->Tick(1.10f);A->CheckCompositionV8221(Check);
    // The persistent fire actor deliberately lives away from the incident: this
    // exercises the real world/local boundary, reset path, and bounded overlap.
    auto* F=B->GetWorld()->SpawnActor<AFrontlineFuelFireV33>(FVector(-1200,-800,0),FRotator::ZeroRotator);
    Check(TEXT("v8221_fuel_fixture_real_production_fire_actor_created"),F&&F->IsReady());
    if(F&&F->IsReady())
    {
        ON_SCOPE_EXIT {if(IsValid(F))F->Destroy();};
        F->SetActorTickEnabled(false);
        auto Counts=[&]()
        {
            TArray<UFrontlineBlastCompositionV8221*> Cs;F->GetComponents(Cs);
            TArray<UStaticMeshComponent*> Render;F->GetComponents(Render);
            return TPair<int32,int32>(Cs.Num(),Render.Num());
        };
        const auto Baseline=Counts();const FVector FuelAt=At+FVector(930,460,154);
        F->FuelBurst(FuelAt,12.f);
        TArray<UFrontlineBlastCompositionV8221*> Cs;F->GetComponents(Cs);
        Check(TEXT("v8221_fuel_burst_calls_actual_new_impulse_hook"),Cs.Num()==Baseline.Key+1);
        if(!Cs.IsEmpty())
        {
            auto* C=Cs.Last();C->UpdateAt(.15f,FVector(0,-1,-.25f).GetSafeNormal());C->Check(Check);
            const auto J=C->Describe();
            Check(TEXT("v8221_airborne_fuel_has_real_volume_fire_and_no_soil_or_blood"),J->GetNumberField(TEXT("profile"))==6
                &&J->GetNumberField(TEXT("visible_true_volume_fire"))>0
                &&J->GetNumberField(TEXT("visible_true_volume_earth"))==0
                &&J->GetNumberField(TEXT("visible_incident_aerosol"))==0);
        }
        F->Clear();
        Check(TEXT("v8221_fuel_actual_fire_clear_removes_impulse_and_all_registered_proxies"),Counts()==Baseline);
        for(int32 I=0;I<9;++I)F->FuelBurst(FuelAt+FVector(I*53,0,0),12.f);
        Check(TEXT("v8221_fuel_overlap_is_bounded_to_eight_independent_impulses"),Counts().Key==Baseline.Key+8);
        F->Clear();
        Check(TEXT("v8221_fuel_clear_after_overlap_restores_component_counts"),Counts()==Baseline);
        F->FuelBurst(FuelAt,12.f);Cs.Reset();F->GetComponents(Cs);
        if(Cs.Num()==Baseline.Key+1)
        {
            auto* C=Cs.Last();C->TickComponent(0.f,LEVELTICK_All,nullptr);C->TickComponent(1.11f,LEVELTICK_All,nullptr);
        }
        Check(TEXT("v8221_fuel_lifetime_cleanup_releases_component_and_all_render_proxies"),Counts()==Baseline);
    }
    else if(F)F->Destroy();
    Check(TEXT("v8221_cosmetic_reconfiguration_never_changes_units_strikes_or_host_clock"),Units==B->Units.Num()&&Strikes==B->Strikes.Num()&&Clock==B->Elapsed);
}

struct FFrontlineBlastReviewV831
{
    static bool Allowed(const AWarfareFrontlineGameMode* B)
    {return FrontlineEffectsReviewSafetyV8216::Allowed(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV831Review"));}
    static void Clear(AWarfareFrontlineGameMode* B)
    {
        if(!Allowed(B))return;
        for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)
            if(I->ActorHasTag(TEXT("BlastArtReviewV831")))I->Destroy();
    }
    static AWarfareFrontlineImpact* Fixture(AWarfareFrontlineGameMode* B,int32 Profile,float Seconds,bool Legacy,const FVector& Origin,int32 Casualties)
    {
        if(!Allowed(B)||!FMath::IsFinite(Seconds)||Seconds<0||Seconds>4||Origin.ContainsNaN()
            ||(Profile!=0&&Profile!=1&&Profile!=2&&Profile!=4&&Profile!=5))return nullptr;
        auto* A=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Origin,FRotator::ZeroRotator);
        if(!A)return nullptr;
        A->Tags.Add(TEXT("BlastArtReviewV831"));A->bReviewLegacyV831=Legacy;
        if(Profile==5)A->ConfigureMineV8214(600.f);
        else if(Profile==4)A->ConfigureAirburstV64(1200.f,FVector(.8,.2,-1));
        else A->ConfigureBlast(Profile==2,Profile==1,FVector(.8,.2,-1));
        if(!A->IsVisualReady()){A->Destroy();return nullptr;}
        TArray<FVector> Feet;
        for(int32 I=0;I<FMath::Clamp(Casualties,0,3);++I)
        {
            const FVector P=Origin+FVector(I*75.f-55.f,I%2?55.f:-50.f,0);
            Feet.Add(FVector(P.X,P.Y,AWarfareFrontlineGround::HeightAt(P.X,P.Y)));
        }
        A->SetCasualtyDustV8216(Feet);
        A->SetActorTickEnabled(false);A->SetLifeSpan(45.f);A->bFirstTick=false;
        A->Tick(0);
        float Remaining=Seconds;
        while(Remaining>.00001f){const float Dt=FMath::Min(Remaining,1.f/120.f);A->Tick(Dt);Remaining-=Dt;}
        return A;
    }
    static TSharedRef<FJsonObject> Describe(AWarfareFrontlineGameMode* B)
    {
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("strict_disposable_review"),Allowed(B));if(!Allowed(B))return J;
        J->SetNumberField(TEXT("actual_shore_raw_sweep_max_step_cm"),RawContactSweepMaximumV831);
        J->SetNumberField(TEXT("actual_shore_visual_response_max_step_cm"),SmoothedContactSweepMaximumV831);
        TArray<TSharedPtr<FJsonValue>> Actors;
        for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)
        {
            auto* A=*I;if(!IsValid(A)||A->IsActorBeingDestroyed()||!A->ActorHasTag(TEXT("BlastArtReviewV831")))continue;
            auto Row=A->DescribeCompositionV8221();Row->SetBoolField(TEXT("frozen"),!A->IsActorTickEnabled());
            Row->SetBoolField(TEXT("legacy_requested"),A->bReviewLegacyV831);Row->SetNumberField(TEXT("light_lumens"),A->FlashLight->Intensity);
            Row->SetNumberField(TEXT("exact_age_s"),A->Age);Row->SetNumberField(TEXT("mine_radius_cm"),A->MineRadiusV8214);
            bool Finite=true,Grounded=true;for(const auto& F:A->Soil)
            {
                const FVector W=A->GetActorTransform().TransformPosition(F.Position);
                Finite&=!F.Position.ContainsNaN()&&!F.Velocity.ContainsNaN()&&!F.Scale.ContainsNaN();
                Grounded&=W.Z>=AWarfareFrontlineGround::HeightAt(W.X,W.Y)+2.8f;
            }
            Row->SetBoolField(TEXT("finite_discrete_debris"),Finite);Row->SetBoolField(TEXT("debris_above_ground"),Grounded);
            TArray<TSharedPtr<FJsonValue>> Smoke;
            for(int32 K=0;K<A->ActivePuffs;++K)if(A->Materials.IsValidIndex(K)&&A->Materials[K])
            {
                auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("slot"),K);
                M->SetStringField(TEXT("material"),GetPathNameSafe(A->Materials[K]->GetBaseMaterial()));
                M->SetBoolField(TEXT("visible"),A->Puffs[K]&&A->Puffs[K]->IsVisible());
                if(auto* C=A->Puffs[K].Get())
                {
                    M->SetStringField(TEXT("actual_proxy"),GetPathNameSafe(C->GetStaticMesh()));
                    M->SetStringField(TEXT("world_centre_cm"),C->GetComponentLocation().ToString());
                    M->SetStringField(TEXT("world_dimensions_cm"),(C->GetComponentScale()*100.f).ToString());
                    M->SetStringField(TEXT("world_rotation"),C->GetComponentRotation().ToString());
                    M->SetStringField(TEXT("actual_component_bounds_extent_cm"),C->Bounds.BoxExtent.ToString());
                    if(C->GetStaticMesh())M->SetStringField(TEXT("actual_local_mesh_extent_cm"),C->GetStaticMesh()->GetBounds().BoxExtent.ToString());
                    M->SetStringField(TEXT("actual_component_material"),GetPathNameSafe(C->GetMaterial(0)));
                    M->SetBoolField(TEXT("actual_component_matches_parameter_owner"),C->GetMaterial(0)==A->Materials[K]);
                    if(K>=A->EarthPuffs&&VolumeSlotV828(K,A->CorePuffs,A->VolumePuffsV40,A->BlastKind)&&A->BlastKind!=4)
                    {
                        const auto Contact=BlastWaterContactV831(C->GetComponentLocation(),C->GetComponentScale()*100.f);
                        M->SetNumberField(TEXT("water_footprint_sample_count"),Contact.WetSamples);
                        M->SetNumberField(TEXT("water_footprint_ground_cm"),Contact.Ground);
                        M->SetNumberField(TEXT("water_footprint_raw_lift_cm"),Contact.Lift);
                        M->SetNumberField(TEXT("water_footprint_max_surface_cm"),Contact.MaximumSurface);
                        M->SetNumberField(TEXT("water_footprint_smoothed_lift_cm"),A->WaterContactLiftV831.IsValidIndex(K)?A->WaterContactLiftV831[K]:0.f);
                        M->SetNumberField(TEXT("water_footprint_stored_target_cm"),A->WaterContactTargetV831.IsValidIndex(K)?A->WaterContactTargetV831[K]:0.f);
                        const auto* Ground=A->WaterContactGroundV831.Get();const auto* Surface=Ground?Ground->BasinWaterV44.Get():nullptr;
                        M->SetBoolField(TEXT("water_footprint_actual_surface_bound"),Ground&&!Ground->IsHidden()&&Surface&&Surface->IsRegistered()
                            &&Surface->IsVisible()&&Surface->GetStaticMesh()&&Surface->GetMaterial(0));
                        M->SetBoolField(TEXT("water_footprint_fitting_enabled"),A->CompositionV8221&&A->CompositionV8221->UsesArtResetV831());
                    }
                    for(const TCHAR* Name:{TEXT("VolumeCenter"),TEXT("VolumeRadii"),TEXT("ViewForward"),TEXT("DustColor")})
                    {
                        FLinearColor Value=FLinearColor::Black;const bool Found=A->Materials[K]->GetVectorParameterValue(FMaterialParameterInfo(Name),Value);
                        auto Param=MakeShared<FJsonObject>();Param->SetBoolField(TEXT("found"),Found);
                        Param->SetNumberField(TEXT("x"),Value.R);Param->SetNumberField(TEXT("y"),Value.G);Param->SetNumberField(TEXT("z"),Value.B);
                        M->SetObjectField(Name,Param);
                    }
                    for(const TCHAR* Name:{TEXT("Density"),TEXT("Age"),TEXT("Seed"),TEXT("VolumeProfile")})
                    {
                        float Value=0;const bool Found=A->Materials[K]->GetScalarParameterValue(FMaterialParameterInfo(Name),Value);
                        M->SetBoolField(FString(Name)+TEXT("_found"),Found);M->SetNumberField(Name,Value);
                    }
                }
                Smoke.Add(MakeShared<FJsonValueObject>(M));
            }
            Row->SetArrayField(TEXT("retained_smoke_bindings"),Smoke);Actors.Add(MakeShared<FJsonValueObject>(Row));
        }
        J->SetArrayField(TEXT("actors"),Actors);return J;
    }
    static int32 HideLayer(AWarfareFrontlineGameMode* B,int32 Layer)
    {
        if(!Allowed(B)||Layer<1||Layer>5)return INDEX_NONE;
        int32 Hidden=0;
        for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)
        {
            auto* A=*I;
            if(!IsValid(A)||A->IsActorBeingDestroyed()||!A->ActorHasTag(TEXT("BlastArtReviewV831"))||A->IsActorTickEnabled())continue;
            auto Hide=[&](UStaticMeshComponent* C)
            {if(C){Hidden+=C->IsVisible();C->SetVisibility(false);}};
            if(Layer==1||Layer==2||Layer==4||Layer==5)
            {
                for(int32 K=0;K<A->ActivePuffs;++K)
                {
                    auto* C=A->Puffs[K].Get();if(!C||!C->GetStaticMesh())continue;
                    const bool Cube=GetPathNameSafe(C->GetStaticMesh())==TEXT("/Engine/BasicShapes/Cube.Cube");
                    if(Layer==5)
                    {
                        // Raster coverage diagnostic: never alter the bound MID's
                        // physical centre/radii, density, age or flow atlas.
                        if(Cube&&C->IsVisible()){C->SetWorldScale3D(C->GetComponentScale()*1.5f);++Hidden;}
                    }
                    else if(Layer==4)
                    {
                        if(Cube&&C->IsVisible())if(auto* MID=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))
                        {MID->SetScalarParameterValue(TEXT("Density"),0.f);++Hidden;}
                    }
                    else if(Cube==(Layer==2))Hide(C);
                }
            }
            else
            {
                // These actual registered proxies are owned by this impact.
                // Names originate in CompositionV8221::Configure; splinter
                // and solid-soil instances are deliberately not included.
                TInlineComponentArray<UStaticMeshComponent*> Components(A);
                for(auto* C:Components)if(C)
                {
                    const FString Name=C->GetName();
                    if(Name.StartsWith(TEXT("BlastCoreV8221_"))||Name.StartsWith(TEXT("IncidentAerosolV8221_"))
                        ||Name.StartsWith(TEXT("ThrownEarthJetV8221_")))Hide(C);
                }
            }
        }
        return Hidden;
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        const bool Safe=Allowed(B);Check(TEXT("v831_blast_checks_require_strict_disposable_profile"),Safe);if(!Safe)return;
        const int32 Units=B->Units.Num(),Strikes=B->Strikes.Num();const float Clock=B->Elapsed;
        ON_SCOPE_EXIT{Clear(B);};
        // Actual dry-centre/adjacent-pool regression. The review battlefield
        // contains this shore; it exposed the otherwise invisible water-depth
        // contact cut. None of these queries changes hydrology or combat.
        const FVector ContactCentre(0.f,250.f,0.f);
        const auto DryContact=BlastWaterContactV831(ContactCentre,FVector(2.f,2.f,100.f));
        Check(TEXT("v831_water_contact_all_dry_footprint_preserves_exact_ground_height"),DryContact.WetSamples==0
            &&DryContact.Lift==0.f&&DryContact.Height()==AWarfareFrontlineGround::HeightAt(0.f,250.f));
        const auto CrossContact=BlastWaterContactV831(ContactCentre,FVector(560.f,460.f,150.f));
        Check(TEXT("v831_water_contact_dry_centre_samples_higher_adjacent_pool"),CrossContact.WetSamples>0&&CrossContact.Lift>0.f);
        Check(TEXT("v831_water_contact_finite_height_bounded_by_actual_sampled_surface"),FMath::IsFinite(CrossContact.Height())
            &&CrossContact.Height()<=CrossContact.MaximumSurface+.001f);
        const auto WetContact=BlastWaterContactV831(FVector(280.f,250.f,0.f),FVector(2.f,2.f,100.f));
        Check(TEXT("v831_water_contact_deep_water_support_matches_true_surface"),WetContact.WetSamples==9
            &&FMath::IsNearlyEqual(WetContact.Height(),WetContact.MaximumSurface,.001f));
        float MaximumStep=0.f,MaximumResponseStep=0.f,Response=0.f,PriorSupport=DryContact.Height();
        bool ResponseFinite=true,ResponseBounded=true,ResponseSpeedBounded=true;
        constexpr float ContactDt=1.f/120.f;
        for(int32 N=2;N<=640;++N)
        {
            const float Radius=N*.5f;
            const auto Contact=BlastWaterContactV831(ContactCentre,FVector(Radius*2.f,Radius*2.f*.82f,150.f));
            MaximumStep=FMath::Max(MaximumStep,FMath::Abs(Contact.Height()-PriorSupport));PriorSupport=Contact.Height();
            const float Next=WaterContactResponseV831(Response,Contact.Lift,ContactDt);
            MaximumResponseStep=FMath::Max(MaximumResponseStep,FMath::Abs(Next-Response));
            ResponseFinite&=FMath::IsFinite(Next);
            ResponseBounded&=Next>=FMath::Min(Response,Contact.Lift)-.001f&&Next<=FMath::Max(Response,Contact.Lift)+.001f;
            ResponseSpeedBounded&=FMath::Abs(Next-Response)<=40.f*FMath::Max(Response,Contact.Lift)*ContactDt+.001f;
            Response=Next;
        }
        RawContactSweepMaximumV831=MaximumStep;SmoothedContactSweepMaximumV831=MaximumResponseStep;
        UE_LOG(LogTemp,Display,TEXT("WF_V831_WATER_CONTACT_SWEEP raw_max_step_cm=%.3f smoothed_max_step_cm=%.3f dt=%.6f raw_is_diagnostic_not_continuity_acceptance"),MaximumStep,MaximumResponseStep,ContactDt);
        Check(TEXT("v831_water_contact_actual_shore_response_finite_and_bounded_by_real_support_targets"),ResponseFinite&&ResponseBounded);
        Check(TEXT("v831_water_contact_actual_shore_response_has_finite_visual_clock_speed"),ResponseSpeedBounded&&MaximumResponseStep<=MaximumStep*.5f+.001f);
        const FVector At(960,700,AWarfareFrontlineGround::HeightAt(960,700));
        for(int32 Kind:{0,1,2,4,5})
        {
            Clear(B);
            auto* A=Fixture(B,Kind,.20f,true,At+(Kind==4?FVector(0,0,430):FVector::ZeroVector),2);
            const bool Old=A&&A->CompositionV8221&&A->CompositionV8221->IsReady()&&!A->CompositionV8221->UsesArtResetV831();
            TArray<FString> OldSmoke;TArray<bool> OldVisibility;TArray<FVector> OldFlow;
            const int32 OldClods=A?A->Soil.Num():-1;
            if(A)for(int32 I=0;I<A->ActivePuffs;++I)if(A->Materials[I])
            {
                OldSmoke.Add(GetPathNameSafe(A->Materials[I]->GetBaseMaterial()));OldVisibility.Add(A->Puffs[I]->IsVisible());
                float Seed=0,Density=0,Phase=0;
                A->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Seed")),Seed);
                A->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Density);
                A->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Age")),Phase);
                OldFlow.Add(FVector(Seed,Density,Phase));
            }
            Clear(B);A=Fixture(B,Kind,.20f,false,At+(Kind==4?FVector(0,0,430):FVector::ZeroVector),2);
            const bool New=A&&A->CompositionV8221&&A->CompositionV8221->IsReady()&&A->CompositionV8221->UsesArtResetV831();
            Check(FString::Printf(TEXT("v831_profile_%d_has_both_exact_old_and_new_short_composition"),Kind),Old&&New);
            if(A&&Kind==0)
            {
                auto* Ground=A->WaterContactGroundV831.Get();auto* Surface=Ground?Ground->BasinWaterV44.Get():nullptr;
                Check(TEXT("v831_water_contact_resolves_actual_displayed_surface_once"),Surface&&Surface->IsRegistered()&&Surface->IsVisible());
                if(Surface)
                {
                    const bool Visible=Surface->IsVisible();
                    {ON_SCOPE_EXIT{Surface->SetVisibility(Visible);};Surface->SetVisibility(false);
                    const float Missing=A->FitWaterContactV831(0,ContactCentre,FVector(560,460,150),ContactDt);
                    Check(TEXT("v831_water_contact_hidden_actual_binding_uses_exact_old_ground"),Missing==AWarfareFrontlineGround::HeightAt(ContactCentre.X,ContactCentre.Y));}
                }
            }
            bool SmokeSame=A&&OldSmoke.Num()==A->ActivePuffs;
            bool EnvelopeBound=true,FlowSame=true;
            if(A&&SmokeSame)for(int32 I=0;I<A->ActivePuffs;++I)
            {
                const bool Derivative=I>=A->EarthPuffs&&VolumeSlotV828(I,A->CorePuffs,A->VolumePuffsV40,A->BlastKind);
                const FString Base=GetPathNameSafe(A->Materials[I]->GetBaseMaterial());
                SmokeSame&=A->Puffs[I]->IsVisible()==OldVisibility[I]&&(Derivative||Base==OldSmoke[I]);
                EnvelopeBound&=!Derivative||Base==TEXT("/Game/Warfare/Field/V831/Materials/M_BlastVolumeEnvelope_V831.M_BlastVolumeEnvelope_V831");
                if(Derivative)
                {
                    float Seed=0,Density=0,Phase=0;
                    A->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Seed")),Seed);
                    A->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Density);
                    A->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Age")),Phase);
                    FlowSame&=OldFlow.IsValidIndex(I)&&OldFlow[I].Equals(FVector(Seed,Density,Phase),.0001f);
                }
            }
            Check(FString::Printf(TEXT("v831_profile_%d_preserves_other_smoke_parents_visibility_and_discrete_debris_count"),Kind),SmokeSame&&A->Soil.Num()==OldClods);
            Check(FString::Printf(TEXT("v831_profile_%d_only_non_earth_cubes_bind_actual_proxy_envelope_derivative"),Kind),EnvelopeBound);
            Check(FString::Printf(TEXT("v831_profile_%d_preserves_retained_volume_density_age_and_seed"),Kind),FlowSame);
            if(A)
            {
                A->CheckCompositionV8221(Check);
                Check(FString::Printf(TEXT("v831_profile_%d_has_exact_frozen_age_and_no_actor_scale_warp"),Kind),FMath::Abs(A->Age-.20f)<.0001f&&!A->IsActorTickEnabled()&&A->GetActorScale3D().Equals(FVector::OneVector));
                A->Tick(1.05f);A->CheckCompositionV8221(Check);
                Check(FString::Printf(TEXT("v831_profile_%d_short_layers_expire_without_erasing_smoke"),Kind),A->GetVisibleSmokeCount()>0
                    &&A->DescribeCompositionV8221()->GetNumberField(TEXT("visible_true_volume_fire"))==0
                    &&A->DescribeCompositionV8221()->GetNumberField(TEXT("visible_true_volume_earth"))==0);
            }
            Clear(B);A=Fixture(B,Kind,.20f,false,At+(Kind==4?FVector(0,0,430):FVector::ZeroVector),0);
            Check(FString::Printf(TEXT("v831_profile_%d_empty_ground_has_no_explosive_blood"),Kind),A
                &&A->DescribeCompositionV8221()->GetNumberField(TEXT("visible_incident_aerosol"))==0);
        }
        Check(TEXT("v831_blast_art_checks_leave_troops_strikes_and_host_clock_unchanged"),Units==B->Units.Num()&&Strikes==B->Strikes.Num()&&Clock==B->Elapsed);
    }
};
AWarfareFrontlineImpact* FrontlineImpactV831::SetupVisualFixture(AWarfareFrontlineGameMode* B,int32 Profile,float PhaseSeconds,bool bLegacy,const FVector& Origin,int32 Casualties)
{return FFrontlineBlastReviewV831::Fixture(B,Profile,PhaseSeconds,bLegacy,Origin,Casualties);}
TSharedRef<FJsonObject> FrontlineImpactV831::DescribeVisualFixture(AWarfareFrontlineGameMode* B)
{return FFrontlineBlastReviewV831::Describe(B);}
void FrontlineImpactV831::ClearVisualFixture(AWarfareFrontlineGameMode* B)
{FFrontlineBlastReviewV831::Clear(B);}
int32 FrontlineImpactV831::HideVisualFixtureLayer(AWarfareFrontlineGameMode* B,int32 Layer)
{return FFrontlineBlastReviewV831::HideLayer(B,Layer);}
void FrontlineImpactV831::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineBlastReviewV831::Run(B,Check);}
