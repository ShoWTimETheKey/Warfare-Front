#include "WarfareFrontlineEnvironmentV42.h"
#include "WarfareFrontlineEnvironmentV40.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineGroundContactV8449.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineTerrainProfilesV51.h"
#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFrontlineArtV68.h"
#include "WarfareFrontlineEmbersV27.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineFieldSitesV59.h"
#include "WarfareFrontlineSandbagDescriptorV87.h"
#include "WarfareFRRUCampaignV82.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "MeshUVChannelInfo.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "SceneInterface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "UObject/StrongObjectPtr.h"
#include "StaticMeshResources.h"
#include "Misc/Paths.h"

namespace
{
constexpr int32 MaximumInstances=550;
constexpr int32 LegacyMaximumInstancesV84412=430;
constexpr float RailY=-1130.f,RailHalfWidth=290.f;
struct FOriginalBankV42
{
    TStrongObjectPtr<UStaticMesh> Mesh;
    TArray<TStrongObjectPtr<UMaterialInterface>> Materials;
    FTransform Transform=FTransform::Identity;
    ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;
    EComponentMobility::Type Mobility=EComponentMobility::Static;
    bool Timber=false,Navigation=false;
};
struct FFieldV42
{
    struct FCorridorRepair {int32 Mesh=0;FVector Before,After;};
    TWeakObjectPtr<AActor> Owner;
    TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> Groups;
    TMap<TWeakObjectPtr<UStaticMeshComponent>,bool> Hidden;
    TMap<TWeakObjectPtr<UStaticMeshComponent>,FOriginalBankV42> OriginalBanks;
    int32 Instances=0,RailSkipped=0,LandscapeInstancesV50=0,LandmarkInstancesV54=0,CanalBankDetailsV55=0;
    int32 OutskirtsInstancesV8449=0;
    int32 MeninLegacyFallenHiddenV59=0;
    int32 LegacyTimberRelocatedV8448=0,LegacyTimberHiddenV8448=0;
    float ArmourAcrossV8448=510.f;
    bool TimberBaselineV8448=false;
    bool PropsV8449=false;
    bool BagsV84412=false;
    bool BagsV87=false;
    int32 FiringBaysV84412=0;
    struct FBagPlacedV84412 {int32 Group=0,Instance=0,Variant=0;FTransform Transform;TArray<int32> Supports;};
    TArray<FBagPlacedV84412> BagsPlacedV84412;
    TWeakObjectPtr<UStaticMeshComponent> RootContactsV8449;
    int32 RootPatchesV8449=0,RootColorsMissingV8449=0,RootUnsafeV8449=0;
    float RootColorDistanceV8449=0,RootSecondsV8449=0;
    TArray<FVector> RootEdgeSamplesV8449;
    TArray<FTransform> MeninDebrisV62;
    TArray<FCorridorRepair> CorridorRepairsV8449;
    bool Ready=false,Finite=true;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FFieldV42> Fields;
#include "WarfareFrontlineTimberContactV76.inl"
#include "WarfareFrontlineTimberContactV8448.inl"
#include "WarfareFrontlinePropContactV8449.inl"
#include "WarfareFrontlineSandbagContactV84412.inl"
// V76 preserves accepted bag topology and replaces only its paired surface.
// Operational boards, dugouts and track remain the proven shared assets.
const TCHAR* Names[]={TEXT("SM_FiringBay_A_V76"),TEXT("SM_FiringBay_B_V76"),TEXT("SM_FiringBay_C_V76"),
    TEXT("SM_RepairRevetment_A_V42"),TEXT("SM_RepairRevetment_B_V42"),
    TEXT("SM_RootedApron_A_V42"),TEXT("SM_RootedApron_B_V42"),TEXT("SM_CollapseScatter_V42"),TEXT("SM_FiringStores_V42"),
    TEXT("SM_Duckboards_A_V40"),TEXT("SM_Duckboards_B_V40"),TEXT("SM_Duckboards_C_V40"),
    TEXT("SM_Dugout_V40"),TEXT("SM_DugoutCollapsed_V40"),TEXT("SM_Rail_A_V40"),TEXT("SM_Rail_B_V40"),TEXT("SM_Rail_C_V40"),
    TEXT("SM_SplinteredTrunk_A_V8448"),TEXT("SM_SplitStump_A_V8448"),TEXT("SM_FallenTimber_A_V8448"),
    TEXT("SM_SplinteredTrunk_B_V8448"),TEXT("SM_SplitStump_B_V8448"),TEXT("SM_FallenTimber_B_V8448")};
const TCHAR* BaselineTimberNamesV8448[]={
    TEXT("SM_SplinteredTrunk_A_V76"),TEXT("SM_SplitStump_A_V76"),TEXT("SM_FallenTimber_A_V76"),
    TEXT("SM_SplinteredTrunk_B_V76"),TEXT("SM_SplitStump_B_V76"),TEXT("SM_FallenTimber_B_V76")};
const TCHAR* PropNamesV8449[]={TEXT("SM_FiringBay_A_V8449"),TEXT("SM_FiringBay_B_V8449"),TEXT("SM_FiringBay_C_V8449")};
const TCHAR* BagNamesV84412[]={TEXT("SM_Sandbag_A_V84412"),TEXT("SM_Sandbag_B_V84412"),TEXT("SM_Sandbag_C_V84412"),
    TEXT("SM_Sandbag_D_V84412"),TEXT("SM_Sandbag_E_V84412"),TEXT("SM_Sandbag_F_V84412")};
bool IsBagV84412(int32 I){return I<3||(I>=23&&I<26);}
int32 BagGroupV84412(int32 Variant){return Variant<3?Variant:Variant+20;}
int32 BagVariantV84412(int32 Group){return Group<3?Group:Group-20;}
bool BagBaselineAllowedV84412(const AWarfareFrontlineGameMode* B)
{
    FString Directory;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV84412BagBaseline"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Directory);
}
bool BagTopHeightV84412(int32 Variant,const FTransform& T,float X,float Y,float& Height)
{
    // Bounded visual support query, called only during construction/review.
    // These 256 triangles follow the authored sack shoulder, not its AABB.
    bool Found=false;Height=-MAX_flt;
    for(const auto& Face:SandbagTopIndicesV84412)
    {
        const FVector A=T.TransformPosition(SandbagTopV84412[Variant][Face[0]]);
        const FVector C=T.TransformPosition(SandbagTopV84412[Variant][Face[1]]);
        const FVector D=T.TransformPosition(SandbagTopV84412[Variant][Face[2]]);
        const double Den=(C.Y-D.Y)*(A.X-D.X)+(D.X-C.X)*(A.Y-D.Y);
        if(FMath::Abs(Den)<.00001)continue;
        const double U=((C.Y-D.Y)*(X-D.X)+(D.X-C.X)*(Y-D.Y))/Den;
        const double V=((D.Y-A.Y)*(X-D.X)+(A.X-D.X)*(Y-D.Y))/Den;
        if(U<-.00001||V<-.00001||U+V>1.00001)continue;
        Height=FMath::Max(Height,float(U*A.Z+V*C.Z+(1-U-V)*D.Z));Found=true;
    }
    return Found;
}
const FVector* BagFeetV87(const FFieldV42& State,int32 Variant)
{
    return State.BagsV87?FrontlineSandbagDescriptorV87::Feet(FrontlineSandbagDescriptorV87::Variant(Variant)):SandbagFeetV84412[Variant];
}
bool BagTopHeightV87(const FFieldV42& State,int32 Variant,const FTransform& T,float X,float Y,float& Height)
{
    return State.BagsV87?FrontlineSandbagDescriptorV87::TopHeight(FrontlineSandbagDescriptorV87::Variant(Variant),T,X,Y,Height)
        :BagTopHeightV84412(Variant,T,X,Y,Height);
}
bool IsTimberNameV8448(const FString& Name)
{return Name.Contains(TEXT("SplinteredTrunk"))||Name.Contains(TEXT("SplitStump"))||Name.Contains(TEXT("FallenTimber"))||Name==TEXT("SM_ShatteredTrunk");}
bool IsGroundedFireFragmentV8448(const UStaticMeshComponent* Component)
{
    // These three deliberately flattened fragments support the same tagged
    // residual fire. Promoting only its log would detach wood from the flame.
    // Purpose and actual dimensions both qualify; a full-sized misplaced tree
    // does not acquire this exception by sharing a material or an asset name.
    const AActor* Actor=Component?Component->GetOwner():nullptr;
    const UStaticMesh* Mesh=Component?Component->GetStaticMesh():nullptr;
    if(!Actor||!Actor->ActorHasTag(TEXT("V27BoundedBurningTimber"))||!Mesh)return false;
    const FString Name=Mesh->GetName();
    if(Name!=TEXT("SM_FallenTimber_V12")&&Name!=TEXT("SM_BrokenRevetment_B")&&Name!=TEXT("SM_BrokenRevetment_C"))return false;
    const FBox Bounds=Mesh->GetBoundingBox().TransformBy(Component->GetComponentTransform());
    const FVector Size=Bounds.GetSize();
    return Bounds.IsValid&&Size.Z<=45.f&&FMath::Max(Size.X,Size.Y)<=160.f;
}
bool TimberBaselineAllowedV8448(const AWarfareFrontlineGameMode* B)
{
    FString Directory;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV8448TreeBaseline"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8448Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Directory);
}
float ArmourAcrossEnvelopeV8448()
{
    // The movement planner can change between all four real lanes. Protect its
    // swept heading envelope using the displayed long/wide hull geometry rather
    // than a trunk centre or the old 190 cm landscape margin alone.
    float Across=177.f;
    for(const TCHAR* Name:{TEXT("SM_MarkVStar"),TEXT("SM_A7V"),TEXT("SM_Austin1918")})
        if(auto* Mesh=FrontlineVehicleArtV836::ResolveMesh(Name))
        {
            const FBox Bounds=Mesh->GetBoundingBox();
            const float Long=FMath::Max(FMath::Abs(float(Bounds.Min.X)),FMath::Abs(float(Bounds.Max.X)));
            const float Wide=FMath::Max(177.f,FMath::Max(FMath::Abs(float(Bounds.Min.Y)),FMath::Abs(float(Bounds.Max.Y))));
            for(float Yaw:{0.f,20.f,40.f,50.f})
                Across=FMath::Max(Across,Long*FMath::Sin(FMath::DegreesToRadians(Yaw))+Wide*FMath::Cos(FMath::DegreesToRadians(Yaw)));
        }
    return Across+75.f;
}
bool TimberPassageClearV8448(const FBox& Box,float ArmourAcross,bool Rail)
{
    if(!Box.IsValid)return false;
    // Whole transformed geometry is tested, including branches and fallen-log
    // ends. No collision is enabled: decoration never vetoes vehicle movement.
    if(Box.Min.X<FrontlineBattlefield::GroundHalfLength&&Box.Max.X> -FrontlineBattlefield::GroundHalfLength)
        for(float Lane:{-980.f,-900.f,-300.f,300.f,900.f,980.f})
            if(Box.Min.Y<Lane+ArmourAcross&&Box.Max.Y>Lane-ArmourAcross)return false;
    if(Rail&&Box.Min.Y<RailY+RailHalfWidth+80.f&&Box.Max.Y>RailY-RailHalfWidth-80.f)return false;
    for(int32 Trench=0;Trench<FrontlineBattlefield::TrenchCount;++Trench)
        for(int32 Sample=0;Sample<7;++Sample)
        {
            const float Y=FMath::Lerp(Box.Min.Y,Box.Max.Y,float(Sample)/6.f);
            if(FMath::Abs(Y)>FrontlineBattlefield::HalfFrontage+190.f)continue;
            const float X=FrontlineFieldV32::TrenchCentreX(Trench,Y);
            if(Box.Min.X<X+460.f&&Box.Max.X>X-460.f)return false;
        }
    return true;
}
float H(float X,float Y){return AWarfareFrontlineGround::HeightAt(X,Y);}
FTransform TimberSupportV76(int32 ContactIndex,const FVector& XY,const FQuat& AuthoredRotation,const FVector& Scale,bool Baseline=false)
{
    const FVector* Contacts=Baseline?TimberContactsV76[ContactIndex]:TimberContactsV8448[ContactIndex];
    FQuat Rotation=AuthoredRotation;
    if(ContactIndex%3==2)
    {
        // A fallen trunk bears on its continuous body, not a thin downward
        // branch. Fit its longitudinal pitch before resolving body contact.
        FVector Along=AuthoredRotation.RotateVector(FVector::ForwardVector);Along.Z=0;
        Along=Along.GetSafeNormal();if(Along.IsNearlyZero())Along=FVector::ForwardVector;
        const FQuat Horizontal=FRotationMatrix::MakeFromXZ(Along,FVector::UpVector).ToQuat();
        const FVector Left=Horizontal.RotateVector(Contacts[0]*Scale);
        const FVector Right=Horizontal.RotateVector(Contacts[4]*Scale);
        const double Span=FMath::Max(1.,(Right-Left).Size2D());
        const double Slope=FMath::Clamp((H(XY.X+Right.X,XY.Y+Right.Y)-H(XY.X+Left.X,XY.Y+Left.Y))/Span,-.55,.55);
        Rotation=FRotationMatrix::MakeFromXZ(FVector(Along.X,Along.Y,Slope),FVector::UpVector).ToQuat();
    }
    float Base=-MAX_flt;const FTransform Shape(Rotation,FVector::ZeroVector,Scale);
    for(int32 I=0;I<5;++I)
    {const FVector Q=Shape.TransformPosition(Contacts[I]);Base=FMath::Max(Base,H(XY.X+Q.X,XY.Y+Q.Y)-Q.Z);}
    return FTransform(Rotation,FVector(XY.X,XY.Y,Base-5.f),Scale);
}
float Yaw(int32 T,float Y){return -FMath::RadiansToDegrees(FMath::Atan((FrontlineFieldV32::TrenchCentreX(T,Y+12)-FrontlineFieldV32::TrenchCentreX(T,Y-12))/24.f));}
bool IsRail(int32 I){return I>=14&&I<17;}
const TCHAR* AssetVersion(int32 I,bool Baseline=false,bool Props=false,bool Bags=false){return Bags&&(I==12||I==13)?TEXT("V84413/Props"):Bags&&IsBagV84412(I)?TEXT("V84412/Props"):I<3?(Props?TEXT("V8449/Props"):TEXT("V76")):I<9?TEXT("V42"):I<17?TEXT("V40"):Baseline?TEXT("V76"):TEXT("V8448");}
bool RoofMaterialV84413(const UMaterialInterface* Mat)
{
    if(!Mat)return false;const FString P=Mat->GetPathName();
    return P.StartsWith(TEXT("/Game/Warfare/Field/V84413/Props/Materials/M_Hessian_V84413"))
        ||P.StartsWith(TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Timber_V84412"))
        ||P.StartsWith(TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Rust_V84412"));
}
UMaterialInterface* WornHessianV84413(UMaterialInterface* Mat,bool Enabled)
{
    if(Enabled&&Mat&&Mat->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Hessian_V84412")))
        return LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84413/Props/Materials/M_Hessian_V84413.M_Hessian_V84413"));
    return Mat;
}
float MeninRoadAxisV59(float X){return -300.f+1.20f*X+145.f*FMath::Sin(X/1950.f+.42f);}
bool IsMeninLegacyFallenV59(const AActor* Actor,const UStaticMeshComponent* Component)
{
    if(!Actor||!Actor->IsA<AStaticMeshActor>()||!Component||!Component->GetStaticMesh())return false;
    const FVector P=Component->GetComponentLocation();
    // The V12 level actor predates operation-specific trenches. Its 1.45x,
    // horizontal 4.2 m mesh now cuts across Menin's right-hand parapet.
    // Its source label is "Enemy fallen tree"; actor labels are editor-only,
    // so use the cooked actor name, original tag, mesh and authored XY.
    return Component->GetStaticMesh()->GetFName()==FName(TEXT("SM_FallenTimber_V12"))
        &&Actor->ActorHasTag(FName(TEXT("WarfareFrontline")))
        &&Actor->GetName().StartsWith(TEXT("StaticMeshActor"))
        &&FMath::Abs(P.X-4060.f)<80.f&&FMath::Abs(P.Y-1260.f)<80.f;
}
bool MeninTimberClearV59(const FBox& Box)
{
    const FVector Mid=Box.GetCenter();
    const float Across=Mid.Y-MeninRoadAxisV59(Mid.X);
    const float South=925.f+90.f*FMath::Sin(Mid.X/1580.f+.8f);
    const float North=-865.f+100.f*FMath::Sin(Mid.X/1760.f+1.4f);
    if(FMath::Abs(Mid.Y)>4900.f||FMath::Min(FMath::Abs(Across-South),FMath::Abs(Across-North))>560.f)return false;
    // Upright silhouettes belong beyond the soldier lanes. Low stumps can
    // remain in the ruined foreground without becoming a wall of trees.
    if(Box.GetSize().Z>180.f&&Box.Min.Y<FrontlineBattlefield::HalfFrontage+180.f
        &&Box.Max.Y> -FrontlineBattlefield::HalfFrontage-180.f)return false;
    // Check the whole transformed mesh, including a fallen log's far end.
    // The route and trenches bend inside a wide timber AABB, so sample more
    // than its centre before claiming the protected passage is clear.
    for(int32 Sample=0;Sample<5;++Sample)
    {
        const float X=FMath::Lerp(Box.Min.X,Box.Max.X,float(Sample)/4.f);
        const float Road=MeninRoadAxisV59(X);
        if(Box.Min.Y<Road+710.f&&Box.Max.Y>Road-710.f)return false;
    }
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
        for(int32 Sample=0;Sample<5;++Sample)
        {
            const float Y=FMath::Lerp(Box.Min.Y,Box.Max.Y,float(Sample)/4.f);
            const float Trench=FrontlineFieldV32::TrenchCentreX(T,Y);
            if(Box.Min.X<Trench+440.f&&Box.Max.X>Trench-440.f)return false;
        }
    return true;
}
bool MeninDebrisCorridorClearV62(const FBox& Box)
{
    // A scattered 2 m wreck remains outside the complete road/trench width,
    // including its long plank ends. Decoration must not redraw the route as
    // a continuous barricade or mask men in an active trench.
    if(Box.Min.X< -7000.f||Box.Max.X>7000.f||Box.Min.Y< -2550.f||Box.Max.Y>2550.f)return false;
    for(int32 Sample=0;Sample<5;++Sample)
    {
        const float X=FMath::Lerp(Box.Min.X,Box.Max.X,float(Sample)/4.f);
        const float Road=MeninRoadAxisV59(X);
        if(Box.Min.Y<Road+690.f&&Box.Max.Y>Road-690.f)return false;
    }
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
        for(int32 Sample=0;Sample<5;++Sample)
        {
            const float Y=FMath::Lerp(Box.Min.Y,Box.Max.Y,float(Sample)/4.f);
            const float Trench=FrontlineFieldV32::TrenchCentreX(T,Y);
            if(Box.Min.X<Trench+420.f&&Box.Max.X>Trench-420.f)return false;
        }
    return true;
}
bool MeninDebrisDryV62(const FBox& Box)
{
    const FVector2D Samples[]={FVector2D(Box.Min.X,Box.Min.Y),FVector2D(Box.Max.X,Box.Min.Y),
        FVector2D(Box.Min.X,Box.Max.Y),FVector2D(Box.Max.X,Box.Max.Y),
        FVector2D((Box.Min.X+Box.Max.X)*.5f,(Box.Min.Y+Box.Max.Y)*.5f)};
    for(const auto& P:Samples)
    {
        float Level=0,Depth=0;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth)&&Depth>2.f)return false;
    }
    return true;
}
}

bool FrontlineEnvironmentV42::FieldBaselineV8449(const AWarfareFrontlineGameMode* B)
{
    FString Directory;
    return B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV8449FieldBaseline"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8449Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(Directory);
}

void FrontlineEnvironmentV42::FinishContactPropsV8449(AWarfareFrontlineGameMode* B)
{
    auto* S=Fields.Find(B);
    if(!B||!S||!S->Ready||!S->PropsV8449||WarfareFRRUCampaignV82::Active(B)||FieldBaselineV8449(B))return;
    AWarfareFrontlineGround* Ground=nullptr;
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(It->Surface){Ground=*It;break;}
    if(!Ground||!Ground->Surface||!Ground->Surface->GetMaterial(0))return;
    UMaterialInterface* ActualGround=Ground->Surface->GetMaterial(0);
    if(auto* Existing=S->RootContactsV8449.Get())
    {Existing->SetMaterial(0,ActualGround);return;}
    const double Started=FPlatformTime::Seconds();
    struct FRootRequest {FVector Position;float Radius,Height,Phase;};
    TArray<FRootRequest> Roots;
    auto Request=[&](UStaticMeshComponent* C,const FTransform& T,int32 Kind)
    {
        if(!C||!C->GetStaticMesh()||Kind%3==2)return;
        const FVector Base=T.GetLocation();const FVector Scale=T.GetScale3D();
        float Radius=30.f;
        for(int32 I=0;I<5;++I)
        {const FVector P=TimberContactsV8448[Kind][I]*Scale;Radius=FMath::Max(Radius,float(P.Size2D())+18.f);}
        Radius=FMath::Clamp(Radius,35.f,105.f);
        const FBox Foot(Base-FVector(Radius*1.24f,Radius*1.24f,0),Base+FVector(Radius*1.24f,Radius*1.24f,30));
        const auto* M=FrontlineCampaignV32::CurrentOperation(B);
        if(!TimberPassageClearV8448(Foot,S->ArmourAcrossV8448,M&&M->bArmoredTrain))return;
        Roots.Add({Base,Radius,FMath::Clamp(13.f*float(Scale.Z),9.f,21.f),float(Base.X*.013+Base.Y*.017)});
    };
    for(int32 G=17;G<FMath::Min(23,S->Groups.Num());++G)if(auto* C=S->Groups[G].Get())
        for(int32 I=0;I<C->GetInstanceCount();++I){FTransform T;C->GetInstanceTransform(I,T,true);Request(C,T,G-17);}
    for(const auto& Pair:S->OriginalBanks)if(Pair.Value.Timber)
        if(auto* C=Pair.Key.Get();C&&C->IsVisible()&&C->GetStaticMesh())
        {
            const FString Name=C->GetStaticMesh()->GetName();
            for(int32 K=0;K<6;++K)if(Name==Names[K+17]){Request(C,C->GetComponentTransform(),K);break;}
        }
    if(Roots.IsEmpty())return;
    // Read the real installed source triangles. Refined shore/earthwork cells
    // are not a regular point grid, so nearest-vertex guesses cannot recover
    // their topology or the exact physical wet/snow masks.
    if(!FrontlineGroundContactV8449::Prepare(Ground))
    {S->RootColorsMissingV8449+=Roots.Num();return;}
    FMeshDescription Description;FStaticMeshAttributes Attributes(Description);Attributes.Register();
    auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();
    auto Tangents=Attributes.GetVertexInstanceTangents();auto Signs=Attributes.GetVertexInstanceBinormalSigns();
    auto UVs=Attributes.GetVertexInstanceUVs();auto Colors=Attributes.GetVertexInstanceColors();UVs.SetNumChannels(1);
    const FPolygonGroupID Group=Description.CreatePolygonGroup();Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Ground");
    TArray<FVector3f> NormalByVertex;TArray<FVector4f> ColorByVertex;
    auto AddVertex=[&](const FRootRequest& R,float T,float Angle)
    {
        const float Shape=1.f+.12f*FMath::Sin(Angle*3+R.Phase)+.065f*FMath::Sin(Angle*7-R.Phase);
        const float X=R.Position.X+FMath::Cos(Angle)*R.Radius*T*Shape;
        const float Y=R.Position.Y+FMath::Sin(Angle)*R.Radius*T*Shape*.91f;
        // Discontinuous deposits cover root valleys; the outer rim falls below
        // the actual ground, with no brown circle, decal edge or opacity mask.
        const float Lobe=.58f+.42f*FMath::Square(FMath::Sin(Angle*2+R.Phase));
        const float Rise=R.Height*FMath::Square(1-T)*Lobe-2.f*FMath::Pow(T,6.f);
        FVector GroundPoint(X,Y,H(X,Y)),GroundNormal(0,0,1);FLinearColor Physical(.3f,0,.45f,0);
        if(!FrontlineGroundContactV8449::Sample(Ground,X,Y,GroundPoint,GroundNormal,&Physical))
            ++S->RootColorsMissingV8449;
        const FVertexID V=Description.CreateVertex();Positions[V]=FVector3f(X,Y,GroundPoint.Z+Rise);
        NormalByVertex.Add(FVector3f(GroundNormal));
        ColorByVertex.Add(FVector4f(Physical.R,Physical.G,Physical.B,Physical.A));
        if(T>.99f)S->RootEdgeSamplesV8449.Add(FVector(Positions[V]));return V;
    };
    auto Triangle=[&](FVertexID A,FVertexID Z,FVertexID C)
    {
        TArray<FVertexInstanceID> Corners;
        const FVector3f N=FVector3f::CrossProduct(Positions[Z]-Positions[A],Positions[C]-Positions[A]).GetSafeNormal();
        for(FVertexID V:{A,Z,C})
        {
            const FVertexInstanceID VI=Description.CreateVertexInstance(V);Corners.Add(VI);
            Normals[VI]=(NormalByVertex[V.GetValue()]*.6f+N*.4f).GetSafeNormal();
            Tangents[VI]=(FVector3f(1,0,0)-Normals[VI]*Normals[VI].X).GetSafeNormal();Signs[VI]=1.f;
            UVs.Set(VI,0,FVector2f(Positions[V].X/145.f,Positions[V].Y/145.f));Colors[VI]=ColorByVertex[V.GetValue()];
        }
        Description.CreatePolygon(Group,Corners);
    };
    constexpr int32 Segments=24,Rings=4;
    for(const auto& R:Roots)
    {
        const FVertexID Centre=AddVertex(R,0,0);TArray<FVertexID> Vertices;
        for(int32 Ring=1;Ring<=Rings;++Ring)for(int32 A=0;A<Segments;++A)Vertices.Add(AddVertex(R,float(Ring)/Rings,A*2*PI/Segments));
        for(int32 A=0;A<Segments;++A)Triangle(Centre,Vertices[A],Vertices[(A+1)%Segments]);
        for(int32 Ring=1;Ring<Rings;++Ring)for(int32 A=0;A<Segments;++A)
        {
            const int32 L=(Ring-1)*Segments+A,N=(Ring-1)*Segments+(A+1)%Segments;
            const int32 U=Ring*Segments+A,V=Ring*Segments+(A+1)%Segments;
            Triangle(Vertices[L],Vertices[U],Vertices[V]);Triangle(Vertices[L],Vertices[V],Vertices[N]);
        }
    }
    UStaticMesh* Mesh=NewObject<UStaticMesh>(S->Owner.Get(),NAME_None,RF_Transient);
    Mesh->bSupportRayTracing=true;Mesh->bAllowCPUAccess=true;
    FStaticMaterial Slot(ActualGround,TEXT("Ground"));Slot.UVChannelData=FMeshUVChannelInfo(145.f);
    Slot.UVChannelData.bOverrideDensities=true;Slot.UVChannelData.LocalUVDensities[0]=145.f;Mesh->GetStaticMaterials().Add(Slot);
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=true;Params.bBuildSimpleCollision=false;
    Params.bAllowCpuAccess=true;Params.bMarkPackageDirty=false;
    TArray<const FMeshDescription*> Descriptions;Descriptions.Add(&Description);
    if(!Mesh->BuildFromMeshDescriptions(Descriptions,Params)){S->RootColorsMissingV8449+=1;return;}
    auto* C=NewObject<UStaticMeshComponent>(S->Owner.Get());S->Owner->AddInstanceComponent(C);C->SetupAttachment(S->Owner->GetRootComponent());
    C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetMaterial(0,ActualGround);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCastShadow(true);C->SetReceivesDecals(true);
    C->ComponentTags.Add(TEXT("WF_RootCollar_V8449"));C->RegisterComponent();C->SetWorldTransform(FTransform::Identity);
    S->RootContactsV8449=C;S->RootPatchesV8449=Roots.Num();S->RootSecondsV8449=FPlatformTime::Seconds()-Started;
}

void FrontlineEnvironmentV42::Reset(AWarfareFrontlineGameMode* B)
{
    FrontlineEnvironmentV40::Reset(B);
    if(auto* S=Fields.Find(B))
    {
        for(const auto& Pair:S->Hidden)if(auto* C=Pair.Key.Get())C->SetVisibility(Pair.Value);
        for(const auto& Pair:S->OriginalBanks)if(auto* C=Pair.Key.Get();C&&Pair.Value.Mesh.Get())
        {
            C->SetStaticMesh(Pair.Value.Mesh.Get());C->EmptyOverrideMaterials();
            for(int32 I=0;I<Pair.Value.Materials.Num();++I)C->SetMaterial(I,Pair.Value.Materials[I].Get());
            C->SetWorldTransform(Pair.Value.Transform);
            if(Pair.Value.Timber)
            {C->SetCollisionEnabled(Pair.Value.Collision);C->SetCanEverAffectNavigation(Pair.Value.Navigation);C->SetMobility(Pair.Value.Mobility);}
        }
        for(const auto& W:S->Groups)if(auto* C=W.Get())C->DestroyComponent();
        if(auto* C=S->RootContactsV8449.Get())C->DestroyComponent();
    }
    Fields.Remove(B);
    for(auto It=Fields.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}

bool FrontlineEnvironmentV42::Build(AWarfareFrontlineGameMode* B,AActor* Owner)
{
    if(!B||!Owner||!Owner->GetRootComponent())return false;
    // Restoration must precede FieldV32's active-trench visibility/contact fit.
    // A repeated call for this owner may reuse its complete pack, never reveal
    // inactive earthworks or append a duplicate set of instanced groups.
    if(const auto* Existing=Fields.Find(B))return Existing->Owner.Get()==Owner&&Existing->Ready;
    auto& S=Fields.Add(B);S.Owner=Owner;
    S.TimberBaselineV8448=TimberBaselineAllowedV8448(B);
    S.PropsV8449=!WarfareFRRUCampaignV82::Active(B)&&!FieldBaselineV8449(B);
    S.BagsV84412=S.PropsV8449&&!BagBaselineAllowedV84412(B);
    S.ArmourAcrossV8448=S.TimberBaselineV8448?510.f:ArmourAcrossEnvelopeV8448();
    for(auto It=Fields.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline"))){FrontlineFieldV32::RefreshBankContactsV42(B);return false;}
    UStaticMesh* BankMeshes[3]={};
    for(int32 I=0;I<3;++I)
    {
        const FString N=FString::Printf(TEXT("SM_TrenchEarth_%c_V42"),'A'+I);
        BankMeshes[I]=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Warfare/Field/V42/Meshes/")+N+TEXT(".")+N));
        if(!BankMeshes[I]||!BankMeshes[I]->GetMaterial(0)||!BankMeshes[I]->GetMaterial(0)->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V42/Materials/")))return false;
    }
    UStaticMesh* Meshes[UE_ARRAY_COUNT(Names)+3]={};
    const int32 GroupCount=UE_ARRAY_COUNT(Names)+(S.BagsV84412?3:0);
    TArray<UStaticMesh*> FormalBagMeshes;UMaterialInterface* FormalBagMaterial=nullptr;
#if WITH_EDITOR
    FString FormalReviewDir;
    if(S.BagsV84412&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV87FormalBags"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(FormalReviewDir))
        S.BagsV87=FrontlineSandbagDescriptorV87::TryLoad(FormalBagMeshes,FormalBagMaterial);
#endif
    for(int32 I=0;I<GroupCount;++I)
    {
        const FString Version=AssetVersion(I,S.TimberBaselineV8448,S.PropsV8449,S.BagsV84412);
        const FString N=S.BagsV84412&&I==12?TEXT("SM_Dugout_V84413"):
            S.BagsV84412&&I==13?TEXT("SM_DugoutCollapsed_V84413"):
            S.BagsV84412&&IsBagV84412(I)?BagNamesV84412[BagVariantV84412(I)]:
            S.TimberBaselineV8448&&I>=17?BaselineTimberNamesV8448[I-17]:I<3&&S.PropsV8449?PropNamesV8449[I]:Names[I];
        Meshes[I]=S.BagsV87&&IsBagV84412(I)?FormalBagMeshes[FrontlineSandbagDescriptorV87::Variant(BagVariantV84412(I))]
            :LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Warfare/Field/")+Version+TEXT("/Meshes/")+N+TEXT(".")+N));
        if(!Meshes[I]||Meshes[I]->GetStaticMaterials().IsEmpty())return false;
        for(const auto& M:Meshes[I]->GetStaticMaterials())
        {
            const bool Earth=M.MaterialInterface&&M.MaterialInterface->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42"));
            const bool Formal=S.BagsV87&&IsBagV84412(I)&&M.MaterialInterface==FormalBagMaterial;
            if(!M.MaterialInterface||(!Formal&&!Earth&&!(S.BagsV84412&&(I==12||I==13)&&RoofMaterialV84413(M.MaterialInterface))&&!M.MaterialInterface->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/")+Version+TEXT("/Materials/"))))return false;
        }
    }
    for(int32 I=0;I<GroupCount;++I)
    {
        auto* Mesh=Meshes[I];
        auto* C=NewObject<UInstancedStaticMeshComponent>(Owner);Owner->AddInstanceComponent(C);C->SetupAttachment(Owner->GetRootComponent());
        C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(true);C->SetCastShadow(true);C->SetCullDistances(16000,22000);
        if(S.BagsV84412&&((IsBagV84412(I)&&!S.BagsV87)||I==12||I==13))for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
        {auto* Worn=WornHessianV84413(C->GetMaterial(Slot),true);if(!Worn)return false;C->SetMaterial(Slot,Worn);}
        C->ComponentTags.Add(TEXT("WF_Environment_V42"));C->RegisterComponent();S.Groups.Add(C);
    }
    const auto* Mission=FrontlineCampaignV32::CurrentOperation(B);const int32 Id=Mission?Mission->Id:0;
    const bool Rail=Mission&&Mission->bArmoredTrain,Marsh=Mission&&Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Marsh;
    const bool Village=Mission&&Mission->TerrainStyleV32==EFrontlineMissionTerrainV32::Village;
    if(!S.TimberBaselineV8448)for(int32 I=17;I<FMath::Min(23,S.Groups.Num());++I)if(auto* Group=S.Groups[I].Get())
        for(int32 M=0;M<Group->GetNumMaterials();++M)
            if(auto* Surface=UMaterialInstanceDynamic::Create(Group->GetMaterial(M),Owner))
            {Surface->SetScalarParameterValue(TEXT("WeatherWetness"),Mission?Mission->Wetness:.58f);Group->SetMaterial(M,Surface);}
    if(S.PropsV8449)for(int32 I=0;I<S.Groups.Num();++I)if(IsBagV84412(I)||(S.BagsV84412&&(I==12||I==13)))if(auto* Group=S.Groups[I].Get())
        for(int32 Slot=0;Slot<Group->GetNumMaterials();++Slot)
            if(Group->GetMaterial(Slot)&&(Group->GetMaterial(Slot)->GetPathName().StartsWith(S.BagsV84412?TEXT("/Game/Warfare/Field/V84412/Props/Materials/"):TEXT("/Game/Warfare/Field/V8449/Props/Materials/"))
                ||(S.BagsV87&&IsBagV84412(I)&&FrontlineSandbagDescriptorV87::IsMaterial(Group->GetMaterial(Slot)))
                ||(S.BagsV84412&&RoofMaterialV84413(Group->GetMaterial(Slot)))))
                if(auto* Surface=UMaterialInstanceDynamic::Create(Group->GetMaterial(Slot),Owner))
                {Surface->SetScalarParameterValue(TEXT("WeatherWetness"),Mission?Mission->Wetness:.58f);
                 Surface->SetScalarParameterValue(TEXT("SnowCoverage"),Id==10?.44f:0.f);
                 Surface->SetVectorParameterValue(TEXT("SoilTint"),TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,Id).Soil);
                 Group->SetMaterial(Slot,Surface);}
    auto Add=[&](int32 M,FVector P,FRotator R=FRotator::ZeroRotator,FVector Scale=FVector::OneVector)
    {
        FTransform T(R,P,Scale);if(T.ContainsNaN()){S.Finite=false;return;}
        if(M>=5&&M<=8&&!WarfareFRRUCampaignV82::Active(B))
        {
            // Low() protected the centre only. In Villers, two rotated rear
            // deposits still extended into the projected trench passage. Fit
            // the complete displayed footprint onto its original bank side;
            // preserve the prop and leave all collision/gameplay unchanged.
            const FVector Before=P;bool Moved=false;
            for(int32 Pass=0;Pass<FrontlineBattlefield::TrenchCount+1;++Pass)
            {
                const FBox Box=Meshes[M]->GetBoundingBox().TransformBy(T);bool Adjusted=false;
                for(int32 K=0;K<FrontlineBattlefield::TrenchCount;++K)
                {
                    float Left=MAX_flt,Right=-MAX_flt;
                    const int32 Steps=FMath::Max(2,FMath::CeilToInt(float(Box.GetSize().Y)/32.f));
                    for(int32 N=0;N<=Steps;++N)
                    {
                        const float Y=FMath::Lerp(float(Box.Min.Y),float(Box.Max.Y),float(N)/Steps);
                        const float X=FrontlineFieldV32::TrenchCentreX(K,Y);
                        Left=FMath::Min(Left,X-82.f);Right=FMath::Max(Right,X+82.f);
                    }
                    if(Box.Min.X>=Right+25.f||Box.Max.X<=Left-25.f)continue;
                    const float Centre=FrontlineFieldV32::TrenchCentreX(K,P.Y);
                    const float Shift=P.X<Centre?Left-25.f-float(Box.Max.X):Right+25.f-float(Box.Min.X);
                    const float OldHeight=H(P.X,P.Y);P.X+=Shift;P.Z+=H(P.X,P.Y)-OldHeight;
                    T.SetLocation(P);Moved=true;Adjusted=true;break;
                }
                if(!Adjusted)break;
            }
            if(Moved)S.CorridorRepairsV8449.Add({M,Before,P});
        }
        if(Rail&&!IsRail(M))
        {
            const FBox Box=Meshes[M]->GetBoundingBox().TransformBy(T);
            if(Box.Min.Y<RailY+RailHalfWidth&&Box.Max.Y>RailY-RailHalfWidth){++S.RailSkipped;return;}
        }
        if(S.Instances>=(S.BagsV84412?MaximumInstances:LegacyMaximumInstancesV84412))return;
        S.Groups[M]->AddInstance(T,true);++S.Instances;
    };
    // Replace only the visible source surface, retaining component transforms.
    // The exact exported V42 triangles are rebound to the same contact cache.
    for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        if(It->Sandbags){S.Hidden.Add(It->Sandbags.Get(),It->Sandbags->IsVisible());It->Sandbags->SetVisibility(false);}
    TArray<UStaticMeshComponent*> BankFits[FrontlineBattlefield::MaxTrenchCount*2];
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        if(*It==Owner||Cast<AWarfareFrontlineSoldier>(*It))continue;
        TArray<UStaticMeshComponent*> Cs;It->GetComponents(Cs);
        for(auto* C:Cs)if(C->GetStaticMesh()&&C->IsVisible())
        {
            const FString N=C->GetStaticMesh()->GetName();
            if(Id==7&&IsMeninLegacyFallenV59(*It,C))
            {
                S.Hidden.Add(C,C->IsVisible());C->SetVisibility(false);
                ++S.MeninLegacyFallenHiddenV59;
                continue;
            }
            const TCHAR* LegacyTimber[]={TEXT("SM_SplinteredTrunk_V12"),TEXT("SM_SplitStump_V12"),TEXT("SM_FallenTimber_V12"),TEXT("SM_ShatteredTrunk")};
            int32 LegacyKind=INDEX_NONE;
            for(int32 K=0;K<UE_ARRAY_COUNT(LegacyTimber);++K)if(N==LegacyTimber[K]){LegacyKind=K;break;}
            // The prior check promoted static V12 trunks outside the instanced
            // landscape inventory. Include every already-derived static timber
            // component as well; its full visible bounds use the same route gate.
            if(LegacyKind==INDEX_NONE&&IsTimberNameV8448(N))
                LegacyKind=N.Contains(TEXT("SplitStump"))?1:N.Contains(TEXT("FallenTimber"))?2:0;
            if(LegacyKind!=INDEX_NONE&&!IsGroundedFireFragmentV8448(C))
            {
                auto& Original=S.OriginalBanks.FindOrAdd(C);Original.Mesh.Reset(C->GetStaticMesh());Original.Transform=C->GetComponentTransform();
                for(int32 I=0;I<C->GetNumMaterials();++I)Original.Materials.Emplace(C->GetMaterial(I));
                Original.Timber=true;Original.Collision=C->GetCollisionEnabled();Original.Navigation=C->CanEverAffectNavigation();Original.Mobility=C->Mobility;
                // Registered static level props reject SetStaticMesh after
                // BeginPlay. Change mobility before either binding or fitting,
                // and restore their original mobility only on scene reset.
                C->SetMobility(EComponentMobility::Movable);
                int32 Kind=LegacyKind<3?LegacyKind:0;
                // The early branched mesh was also laid on its side. The new
                // fallen asset is authored along X, so preserve that visible
                // direction rather than rotating an upright root into the air.
                const FVector OldAxis=C->GetComponentQuat().RotateVector(FVector::UpVector);
                if(LegacyKind==3&&FMath::Abs(OldAxis.Z)<.45f)
                {
                    Kind=2;const FVector Along=FVector(OldAxis.X,OldAxis.Y,0).GetSafeNormal();
                    C->SetWorldRotation(FRotationMatrix::MakeFromXZ(Along,FVector::UpVector).ToQuat());
                }
                const int32 Variant=GetTypeHash(It->GetName())%2,MeshIndex=17+Kind+Variant*3;
                C->SetStaticMesh(Meshes[MeshIndex]);
                if(C->GetStaticMesh()!=Meshes[MeshIndex])
                {S.Finite=false;UE_LOG(LogTemp,Error,TEXT("WF_V8448_TIMBER_BINDING_REJECTED component=%s"),*C->GetPathName());continue;}
                C->EmptyOverrideMaterials();
                for(int32 I=0;I<Meshes[MeshIndex]->GetStaticMaterials().Num();++I)C->SetMaterial(I,Meshes[MeshIndex]->GetMaterial(I));
                FTransform Supported=TimberSupportV76(Kind+Variant*3,C->GetComponentLocation(),C->GetComponentQuat(),C->GetComponentScale(),S.TimberBaselineV8448);
                if(!S.TimberBaselineV8448)
                {
                    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
                    FBox Box=Meshes[MeshIndex]->GetBoundingBox().TransformBy(Supported);
                    if(!TimberPassageClearV8448(Box,S.ArmourAcrossV8448,Rail))
                    {
                        const uint32 Salt=GetTypeHash(It->GetName());
                        const float Side=Kind==0||Supported.GetLocation().Y<0.f?-1.f:1.f;
                        const float Shoulder=980.f+S.ArmourAcrossV8448+Box.GetExtent().Y+140.f+float(Salt%4)*67.f;
                        bool Seated=false;
                        for(int32 Attempt=0;Attempt<6&&!Seated;++Attempt)
                        {
                            const FVector Position(Supported.GetLocation().X+float(int32((Salt+uint32(Attempt))%3)-1)*91.f,
                                Side*(Shoulder+Attempt*140.f),0);
                            const FTransform Candidate=TimberSupportV76(Kind+Variant*3,Position,Supported.GetRotation(),Supported.GetScale3D());
                            if(TimberPassageClearV8448(Meshes[MeshIndex]->GetBoundingBox().TransformBy(Candidate),S.ArmourAcrossV8448,Rail))
                            {Supported=Candidate;Seated=true;++S.LegacyTimberRelocatedV8448;}
                        }
                        if(!Seated)
                        {S.Hidden.Add(C,C->IsVisible());C->SetVisibility(false);++S.LegacyTimberHiddenV8448;}
                    }
                    for(int32 M=0;M<C->GetNumMaterials();++M)
                        if(auto* Surface=UMaterialInstanceDynamic::Create(C->GetMaterial(M),Owner))
                        {Surface->SetScalarParameterValue(TEXT("WeatherWetness"),Mission?Mission->Wetness:.58f);C->SetMaterial(M,Surface);}
                }
                C->SetWorldTransform(Supported);
            }
            for(int32 K=0;K<3;++K)if(N==FString::Printf(TEXT("SM_TrenchEarth_%c"),'A'+K))
            {
                auto& Original=S.OriginalBanks.FindOrAdd(C);Original.Mesh.Reset(C->GetStaticMesh());Original.Transform=C->GetComponentTransform();
                for(int32 I=0;I<C->GetNumMaterials();++I)Original.Materials.Emplace(C->GetMaterial(I));
                C->SetStaticMesh(BankMeshes[K]);
                C->SetMaterial(0,BankMeshes[K]->GetMaterial(0));C->SetVisibility(true);
                const FVector P=C->GetComponentLocation();int32 T=0;
                for(int32 Candidate=1;Candidate<FrontlineBattlefield::TrenchCount;++Candidate)
                    if(FMath::Abs(P.X-FrontlineFieldV32::TrenchCentreX(Candidate,P.Y))<FMath::Abs(P.X-FrontlineFieldV32::TrenchCentreX(T,P.Y)))T=Candidate;
                BankFits[T*2+(P.X>FrontlineFieldV32::TrenchCentreX(T,P.Y)?1:0)].Add(C);
            }
            if((N.StartsWith(TEXT("SM_BrokenRevetment_"))||N.Contains(TEXT("Duckboard")))&&!IsGroundedFireFragmentV8448(C))
            {S.Hidden.Add(C,C->IsVisible());C->SetVisibility(false);}
        }
    }
    // The old 4–6m rigid banks were tangent-fitted at their centers only. They
    // bridged sharp traverses and exposed tall cut ends above the receding floor.
    // Reuse their components as short chord-fitted fragments across the active
    // frontage; the continuous terrain remains beneath every repaired opening.
    constexpr float Stations[]={-1290,-1035,-805,-600,-400,-200,0,200,400,600,805,1035};
    for(int32 G=0;G<UE_ARRAY_COUNT(BankFits);++G)
    {
        auto& Cs=BankFits[G];Cs.Sort([](const UStaticMeshComponent& A,const UStaticMeshComponent& C){return A.GetComponentLocation().Y<C.GetComponentLocation().Y;});
        const int32 Trench=G/2,Side=G%2?1:-1;
        for(int32 I=0;I<Cs.Num();++I)
        {
            auto* C=Cs[I];const FVector Previous=C->GetComponentLocation();FVector Scale=C->GetComponentScale();
            const float Offset=FMath::Clamp(FMath::Abs(Previous.X-FrontlineFieldV32::TrenchCentreX(Trench,Previous.Y)),150.f,195.f);
            const float Y=(I<UE_ARRAY_COUNT(Stations)?Stations[I]:FMath::Lerp(-1290.f,1290.f,float(I)/FMath::Max(1,Cs.Num()-1)))*((Trench+G%2)%2?-1.f:1.f);
            Scale.Y=.38f;const float Half=C->GetStaticMesh()->GetBoundingBox().GetExtent().Y*Scale.Y;
            const float Before=FrontlineFieldV32::TrenchCentreX(Trench,Y-Half),After=FrontlineFieldV32::TrenchCentreX(Trench,Y+Half);
            const float A=-FMath::RadiansToDegrees(FMath::Atan2(After-Before,2.f*Half))+(Side<0?180.f:0.f);
            float Z=H(FrontlineFieldV32::TrenchCentreX(Trench,Y)+Side*300.f,Y)-9.f;
            if(FMath::Abs(Y)>720.f)
                Z=FMath::Min3(Z,H(Before+Side*300.f,Y-Half)-11.5f,H(After+Side*300.f,Y+Half)-11.5f);
            C->SetWorldTransform(FTransform(FRotator(0,A,0),FVector(FrontlineFieldV32::TrenchCentreX(Trench,Y)+Side*Offset,Y,Z),Scale));
        }
    }
    FrontlineFieldV32::RefreshBankContactsV42(B);
    auto Surface=[&](float X,float Y)
    {
        float Height=H(X,Y),BankHeight=0.f;
        if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,FVector(X,Y,Height),BankHeight))Height=FMath::Max(Height,BankHeight);
        return Height;
    };
    AWarfareFrontlineGround* BagGroundV84412=nullptr;
    if(S.BagsV84412)for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
        if(It->Surface){BagGroundV84412=*It;break;}
    if(S.BagsV84412&&!FrontlineGroundContactV8449::Prepare(BagGroundV84412))S.Finite=false;
    auto BagSurfaceV84412=[&](float X,float Y)
    {
        FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;float Height=H(X,Y),BankHeight=0;
        if(BagGroundV84412&&FrontlineGroundContactV8449::Sample(BagGroundV84412,X,Y,Actual,Normal))Height=Actual.Z;
        else S.Finite=false;
        if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,FVector(X,Y,Height),BankHeight))Height=FMath::Max(Height,BankHeight);
        return Height;
    };
    auto PlaceBagV84412=[&](int32 Variant,const FVector& Centre,float Angle,const TArray<int32>& Supports)->int32
    {
        const int32 Group=BagGroupV84412(Variant);const bool Upper=!Supports.IsEmpty();
        FTransform Best;float BestScore=MAX_flt;
        // Fit each short sack, not an entire rigid 1.8 m bank. The tiny local
        // adjustment seeks continuous support without changing a firing bay.
        for(int32 Attempt=0;Attempt<5;++Attempt)
        {
            const FVector Offset=FRotator(0,Angle,0).RotateVector(FVector(Attempt==1?5.f:Attempt==2?-5.f:0.f,
                Attempt==3?5.f:Attempt==4?-5.f:0.f,0));
            const FVector XY=Centre+Offset;
            const FVector N(-FMath::Clamp((BagSurfaceV84412(XY.X+18,XY.Y)-BagSurfaceV84412(XY.X-18,XY.Y))/36.f,-.34f,.34f),
                -FMath::Clamp((BagSurfaceV84412(XY.X,XY.Y+18)-BagSurfaceV84412(XY.X,XY.Y-18))/36.f,-.34f,.34f),1);
            // A settled upper sack follows the actual bearing sack's pitch and
            // roll. It must not bridge the narrowed, unsupported end mouths.
            FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,N.GetSafeNormal())*FQuat(FVector::UpVector,FMath::DegreesToRadians(Angle));
            if(S.BagsV87&&(Variant%2))Q*=FQuat(FVector::UpVector,PI);
            if(Supports.Num()==1&&S.BagsPlacedV84412.IsValidIndex(Supports[0]))
            {
                const auto& Lower=S.BagsPlacedV84412[Supports[0]];Q=Lower.Transform.GetRotation();
                // Rigid variation belongs to the instance, not another copy of
                // the same geometry. Preserve the bearing sack's bank tilt.
                if(S.BagsV87&&(Variant%2)!=(Lower.Variant%2))Q*=FQuat(FVector::UpVector,PI);
            }
            float Base=-MAX_flt;float SupportHeights[9]={};
            for(int32 Foot=0;Foot<9;++Foot)
            {
                const FVector P=Q.RotateVector(BagFeetV87(S,Variant)[Foot])+FVector(XY.X,XY.Y,0);
                float Height=BagSurfaceV84412(P.X,P.Y);
                for(int32 Index:Supports)if(S.BagsPlacedV84412.IsValidIndex(Index))
                {
                    const auto& Lower=S.BagsPlacedV84412[Index];float Top=0;
                    if(BagTopHeightV87(S,Lower.Variant,Lower.Transform,P.X,P.Y,Top))Height=FMath::Max(Height,Top);
                }
                SupportHeights[Foot]=Height;Base=FMath::Max(Base,Height-float(P.Z));
            }
            const float Embed=Upper?4.5f:2.5f;Base-=Embed;
            const FTransform Fit(Q,FVector(XY.X,XY.Y,Base));float Low=MAX_flt,High=-MAX_flt;
            for(int32 Foot=0;Foot<9;++Foot)
            {const float Gap=Fit.TransformPosition(BagFeetV87(S,Variant)[Foot]).Z-SupportHeights[Foot];Low=FMath::Min(Low,Gap);High=FMath::Max(High,Gap);}
            const float Score=High-Low+.015f*Offset.Size2D();
            if(Score<BestScore){BestScore=Score;Best=Fit;}
        }
        auto* Component=S.Groups[Group].Get();if(!Component)return INDEX_NONE;
        const int32 Instance=Component->GetInstanceCount();
        Add(Group,Best.GetLocation(),Best.Rotator());
        if(Component->GetInstanceCount()==Instance)return INDEX_NONE;
        FFieldV42::FBagPlacedV84412 Placed;Placed.Group=Group;Placed.Instance=Instance;Placed.Variant=Variant;
        Placed.Transform=Best;Placed.Supports=Supports;
        return S.BagsPlacedV84412.Add(MoveTemp(Placed));
    };
    FRandomStream R(421917+Id*197);
    auto Low=[&](int32 M,float X,float Y,float A,float Size)
    {
        for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
            if(FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y))<370.f)return;
        const FVector N(-((H(X+28,Y)-H(X-28,Y))/56.f),-((H(X,Y+28)-H(X,Y-28))/56.f),1);
        FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,N.GetSafeNormal())*FQuat(FVector::UpVector,FMath::DegreesToRadians(A));
        Add(M,FVector(X,Y,H(X,Y)-5.f),Q.Rotator(),FVector(Size,Size,.85f));
    };
    for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
    {
        // A traversable central tread, with individual repair gaps. The passage
        // remains independent of the sparse firing points on the outer banks.
        const float Bias=((Id+T)%3-1)*39.f;
        for(int32 J=0;J<17;++J)
        {
            if((J+T*3+Id)%11==3)continue;
            const float Y=-1270.f+J*157.f+Bias,X=FrontlineFieldV32::TrenchCentreX(T,Y);
            Add(9+(J+T+Id)%3,FVector(X,Y,H(X,Y)+1),FRotator(0,Yaw(T,Y),0));
        }
        for(int32 Side:{-1,1})
        {
            // Three discontinuous firing bays per side; the intervening earth
            // is intentionally exposed. No second continuous sack/revetment line.
            for(int32 Bay=0;Bay<3;++Bay)
            {
                const float Y=-850.f+Bay*850.f+Bias+Side*91.f+R.FRandRange(-85,85);
                const float X=FrontlineFieldV32::TrenchCentreX(T,Y),A=Yaw(T,Y)+(Side<0?180.f:0.f);
                const int32 Variant=(Bay+T+Id+(Side>0?1:0))%3;
                const float BagX=X+Side*235.f,BagYaw=A+R.FRandRange(-7,7);
                if(S.BagsV84412)
                {
                    const float LongYaw=BagYaw+90.f;
                    const FVector Along=FRotator(0,LongYaw,0).RotateVector(FVector::ForwardVector);
                    const FVector Across=FRotator(0,LongYaw,0).RotateVector(FVector::RightVector);
                    // Three bounded repair patterns: a low short run, one
                    // reinforced shoulder, or a compact asymmetric two-tier
                    // heap. Five sacks retain the firing bay's material density.
                    const int32 Layout=(Bay+T+Id+(Side>0?1:0))%3;
                    const int32 BaseCount=Layout==0?5:Layout==1?4:3;
                    const float Spacing=Layout==0?54.f:58.f;
                    TArray<int32> Supports;
                    for(int32 Sack=0;Sack<BaseCount;++Sack)
                    {
                        const int32 V=(Bay+T*2+Id+Sack+(Side>0?2:0))%6;
                        const float AlongOffset=(Sack-(BaseCount-1)*.5f)*Spacing;
                        const int32 Placed=PlaceBagV84412(V,FVector(BagX,Y,0)+Along*AlongOffset+Across*((Sack%3-1)*2.f),
                            LongYaw+(Sack-(BaseCount-1)*.5f)*2.8f,TArray<int32>());
                        if(Placed!=INDEX_NONE)Supports.Add(Placed);
                    }
                    if(Supports.Num()==BaseCount)for(int32 Sack=0;Sack<5-BaseCount;++Sack)
                    {
                        const int32 Bearing=Layout==1?1+(Bay+T)%2:(Sack+(Bay+T)%2);
                        const int32 SupportIndex=Supports[Bearing];
                        const auto& Lower=S.BagsPlacedV84412[SupportIndex];
                        PlaceBagV84412((Bay+T+Id+Sack+3)%6,Lower.Transform.GetLocation(),
                            Lower.Transform.Rotator().Yaw,TArray<int32>{SupportIndex});
                    }
                    S.FiringBaysV84412+=!Supports.IsEmpty();
                }
                else
                {
                const FVector Along=FRotator(0,BagYaw,0).RotateVector(FVector(0,70,0));
                const float Before=Surface(BagX-Along.X,Y-Along.Y),After=Surface(BagX+Along.X,Y+Along.Y);
                const FVector N(-FMath::Clamp((Surface(BagX+28,Y)-Surface(BagX-28,Y))/56.f,-.26f,.26f),
                    -FMath::Clamp((Surface(BagX,Y+55)-Surface(BagX,Y-55))/110.f,-.26f,.26f),1);
                const FQuat BagQ=FQuat::FindBetweenNormals(FVector::UpVector,N.GetSafeNormal())*FQuat(FVector::UpVector,FMath::DegreesToRadians(BagYaw));
                const float BagBaseZ=BagQ.RotateVector(FVector(0,0,Meshes[Variant]->GetBoundingBox().Min.Z)).Z;
                const float BagSupport=(Surface(BagX,Y)*2.f+Before+After)*.25f;
                float Base=BagSupport-BagBaseZ-4.f;
                if(S.PropsV8449)
                {
                    Base=-MAX_flt;
                    for(int32 Foot=0;Foot<9;++Foot)
                    {const FVector P=BagQ.RotateVector(PropContactsV8449[Variant][Foot]);
                     Base=FMath::Max(Base,Surface(BagX+P.X,Y+P.Y)-float(P.Z));}
                    Base-=3.f;
                }
                Add(Variant,FVector(BagX,Y,Base),BagQ.Rotator());
                }
                Add(3+(Bay+T)%2,FVector(X+Side*132.f,Y,H(X,Y)+4.f),FRotator(0,A,0),FVector(1,1,(FrontlineBattlefield::TrenchCount%2==1&&T==FrontlineBattlefield::CentreTrench)?.70f:.87f));
                // Rooted collapse skirts live outside the authoritative lip and
                // cannot become fake support surfaces beneath planted bipods.
                Low(5+(Variant%2),X+Side*(410.f+R.FRandRange(0,65)),Y+Side*128.f,A+Side*19.f,R.FRandRange(.88f,1.12f));
                if(Bay==1&&(T+Side+Id)%2==0)
                    Low(8,X+Side*435.f,Y+230.f,A,R.FRandRange(.90f,1.05f));
            }
            // A collapsed opening has rubble and broken timber, never a mound
            // of identical sacks or a broad corrugated sheet pasted on the mud.
            const float CY=Side*(1250.f+R.FRandRange(-90,85)),CX=FrontlineFieldV32::TrenchCentreX(T,CY);
            Low(7,CX+Side*440.f,CY,Yaw(T,CY)+R.FRandRange(-28,28),.92f);
        }
        if((T+Id)%2==0)
        {
            const int32 Side=T%2?1:-1;const float Y=Side*(1470.f+R.FRandRange(-60,60)),X=FrontlineFieldV32::TrenchCentreX(T,Y);
            Add(12+((T+Id)%3==0?1:0),FVector(X+Side*158.f,Y,H(X,Y)+2.f),FRotator(0,Yaw(T,Y)+(Side<0?180.f:0.f),0));
        }
    }
    // Broken root/clay transitions follow interrupted portions of actual crater
    // rims, with low remains grouped as a blast deposit rather than repeated dots.
    const FVector4 Scars[]={FVector4(-1700,180,460,290),FVector4(1550,-170,265,195),FVector4(-4550,600,330,245),
        FVector4(4700,-650,460,220),FVector4(-1250,1500,570,310),FVector4(2200,2200,340,230),
        FVector4(-800,-980,470,235),FVector4(1850,-1500,315,200),FVector4(-2250,1080,270,160),
        FVector4(3650,1450,440,260),FVector4(-5800,1500,350,180),FVector4(600,-2500,430,205)};
    for(int32 I=0;I<UE_ARRAY_COUNT(Scars);++I)
    {
        const FVector4& C=Scars[I];const float Start=R.FRandRange(0,6.28318f);
        for(int32 J=0;J<(Marsh?4:3);++J)
        {
            const float A=Start+J*.61f+R.FRandRange(-.13f,.13f);
            Low(5+(I+J)%2,C.X+FMath::Cos(A)*C.Z,C.Y+FMath::Sin(A)*C.W,FMath::RadiansToDegrees(A)+90.f,R.FRandRange(.85f,1.15f));
        }
        if((I+Id)%2==0)Low(7,C.X+C.Z*.9f,C.Y+C.W*.32f,R.FRandRange(0,360),Village?1.12f:.91f);
    }
    // Local service areas, not equally spaced supplies across no-man's-land.
    for(int32 I=0;I<(Village?9:6);++I)
    {
        const float X=R.FRandRange(-7900,7900),Y=(I%2?1.f:-1.f)*R.FRandRange(1710,2390);
        Low(I%3?7:8,X,Y,R.FRandRange(0,360),R.FRandRange(.90f,1.14f));
    }
    if(Rail)for(int32 I=-26;I<=26;++I)Add(14+(I+27)%3,FVector(I*500.f,RailY,30.f));
    // V54's regional forms are physical, and their set dressing must occupy
    // those same coordinates. Small reusable authored fragments keep draw cost
    // bounded; no invisible barriers are introduced in a soldier or tank lane.
    // The previous random crater scatter alone made every operation read alike.
    const auto Style=Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland;
    auto LandmarkLow=[&](int32 Mesh,float X,float Y,float Angle,float Size,float MinimumMask,bool bWaterLevel=false)
    {
        const auto Feature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y);
        const float Mask=Style==EFrontlineMissionTerrainV32::Village?FMath::Max(Feature.Ruins,Feature.RuinCellar):
            Style==EFrontlineMissionTerrainV32::Marsh?Feature.MarshChannel:Feature.RailShoulder;
        if(Mask<MinimumMask)return;
        const int32 Before=S.Instances;
        if(bWaterLevel)
        {
            bool Clear=true;
            for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
                Clear&=FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y))>=370.f;
            if(Clear)
            {
                float Level=0,Depth=0;
                const float Z=AWarfareFrontlineGround::QueryWaterSurfaceV51(X,Y,Level,Depth)&&Depth>2.f?Level+1.f:H(X,Y)-3.f;
                Add(Mesh,FVector(X,Y,Z),FRotator(0,Angle,0),FVector(Size,Size,.83f));
            }
        }
        else Low(Mesh,X,Y,Angle,Size);
        S.LandmarkInstancesV54+=S.Instances-Before;
    };
    auto LandmarkRetaining=[&](int32 Mesh,float X,float Y,float Angle,float Length,float Height,float MinimumMask)
    {
        const auto Feature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y);
        const float Mask=Style==EFrontlineMissionTerrainV32::Village?FMath::Max(Feature.Ruins,Feature.RuinCellar):Feature.RailShoulder;
        if(Mask<MinimumMask)return;
        // The timber face is only a short exposed remnant. Keep its whole
        // length outside an active trench, and seat its foot at the lower end
        // of the uneven berm so it cannot hang over a crater.
        const float Radians=FMath::DegreesToRadians(Angle),DX=-FMath::Sin(Radians)*85.f*Length,DY=FMath::Cos(Radians)*85.f*Length;
        for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
            for(float End:{-1.f,0.f,1.f})
                if(FMath::Abs(X+End*DX-FrontlineFieldV32::TrenchCentreX(T,Y+End*DY))<420.f)return;
        const float Z=FMath::Min3(H(X-DX,Y-DY),H(X,Y),H(X+DX,Y+DY))-8.f;
        const int32 Before=S.Instances;
        Add(Mesh,FVector(X,Y,Z),FRotator(0,Angle,0),FVector(1.f,Length,Height));
        S.LandmarkInstancesV54+=S.Instances-Before;
    };
    if(Style==EFrontlineMissionTerrainV32::Village)
    {
        const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
        const float OffsetA=float((Id*7)%5-2)*132.f,OffsetB=float((Id*11)%7-3)*73.f;
        struct FCellar{float X,Y,HalfX,HalfY,Angle;};
        const FCellar GeneralCellars[]={
            {-.48f*Home+OffsetA,-270.f+OffsetB,590.f,360.f,.19f+Id*.013f},
            {.46f*Home-OffsetB,390.f+OffsetA*.45f,460.f,420.f,-.24f},
            {-.18f*Home+OffsetA*.5f,-2290.f+OffsetB,690.f,360.f,.10f}};
        const auto RoadY=[](float X){return 90.f+.085f*X+125.f*FMath::Sin(X/1590.f+.4f);};
        const FCellar VillersCellars[]={
            {-.50f*Home,RoadY(-.50f*Home)+1280.f,810.f,460.f,.11f},
            {-.03f*Home,RoadY(-.03f*Home)-1330.f,700.f,430.f,-.17f},
            {.43f*Home,RoadY(.43f*Home)+1240.f,590.f,510.f,.23f}};
        const FCellar* Cellars=Id==13?VillersCellars:GeneralCellars;
        for(int32 Bld=0;Bld<3;++Bld)
        {
            const auto& C=Cellars[Bld];const float Sine=FMath::Sin(C.Angle),Cosine=FMath::Cos(C.Angle);
            // Ruptured masonry/earth and timbers form interrupted rectangular
            // perimeter lines. The sunken centre stays open for troop passage.
            for(int32 K=0;K<8;++K)
            {
                if((K+Bld+Id)%5==1)continue; // shell-breached wall gaps
                const float U=(K%4<2?-1.f:1.f)*C.HalfX*(K%2?.72f:.92f);
                const float V=(K/4?-1.f:1.f)*C.HalfY*(.78f+.11f*(K%2));
                const float X=C.X+U*Cosine-V*Sine,Y=C.Y+U*Sine+V*Cosine;
                // Every third section retains a buried rooted-clay skirt;
                // those raised fragments give the cellar a continuous-enough
                // outline at the strategic camera without a tidy intact wall.
                LandmarkLow((K+Bld)%3==0?5+(K+Bld)%2:7,X,Y,FMath::RadiansToDegrees(C.Angle)+K*17.f,1.16f,.09f);
            }
            if(Bld<2)
            {
                // Short surviving cellar lining on the far lip. The gaps
                // reveal the hollow and leave the broken entry unblocked.
                for(int32 Span=-1;Span<=1;++Span)
                {
                    if((Span+Bld+Id+3)%5==1)continue;
                    const float U=Span*C.HalfX*.64f,V=-C.HalfY*.87f;
                    const float X=C.X+U*Cosine-V*Sine,Y=C.Y+U*Sine+V*Cosine;
                    LandmarkRetaining(3+(Span+Bld+3)%2,X,Y,FMath::RadiansToDegrees(C.Angle)-90.f,1.18f,1.03f,.10f);
                }
            }
            // The near lip and two broken returns make a legible rectangular
            // cellar at the normal battle camera. No intact wall or roof spans
            // the traversable sunken middle.
            for(int32 Span=-2;Span<=1;++Span)
            {
                if((Span+Bld+Id+4)%7==1)continue;
                const float U=(Span+.5f)*C.HalfX*.46f,V=C.HalfY*.84f;
                LandmarkRetaining(3+(Span+Bld+4)%2,C.X+U*Cosine-V*Sine,C.Y+U*Sine+V*Cosine,
                    FMath::RadiansToDegrees(C.Angle)-90.f,1.26f,1.12f,.11f);
            }
            for(int32 Side:{-1,1})for(int32 Span:{-1,1})
            {
                if((Side+Span+Bld+Id+4)%6==1)continue;
                const float U=Side*C.HalfX*.84f,V=Span*C.HalfY*.40f;
                LandmarkRetaining(3+(Bld+(Side>0?1:0))%2,C.X+U*Cosine-V*Sine,C.Y+U*Sine+V*Cosine,
                    FMath::RadiansToDegrees(C.Angle),1.12f,1.02f,.11f);
            }
            // A fractured plank remnant follows the actual cellar lip rather
            // than floating over a random crater or covering its hollow.
            const float U=-.28f*C.HalfX,V=-.81f*C.HalfY;
            LandmarkLow(9+(Bld+Id)%3,C.X+U*Cosine-V*Sine,C.Y+U*Sine+V*Cosine,
                FMath::RadiansToDegrees(C.Angle)+90.f,1.03f,.08f);
            if(Bld==2)
            {
                // This rear structure is outside active frontage: a visible
                // broken shell connects the low cellar footprint to the town
                // photograph without a tall prop clipping advancing soldiers.
                const float X=C.X+C.HalfX*.36f,Y=C.Y-85.f;
                const int32 Before=S.Instances;
                Add(13,FVector(X,Y,H(X,Y)-4.f),FRotator(0,FMath::RadiansToDegrees(C.Angle)-21.f,0),FVector(1.12f,1.12f,1.12f));
                S.LandmarkInstancesV54+=S.Instances-Before;
            }
        }
    }
    else if(Style==EFrontlineMissionTerrainV32::Marsh)
    {
        const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
        if(Id==12)
        {
            // Half-buried timber marks the actual Lys banks and the two
            // shallow openings, instead of drawing generic Ypres drainage
            // fragments across a different geographic section.
            for(int32 I=0;I<21;++I)
            {
                const float Y=-5770.f+I*385.f;
                const float Centre=-.45f*Home+.19f*Y+145.f*FMath::Sin(Y/830.f+.35f);
                if(FMath::Abs(Y+720.f)<360.f||FMath::Abs(Y-690.f)<390.f)continue;
                for(int32 Side:{-1,1})
                {
                    if((I+(Side>0?2:0))%5==1)continue;
                    const float X=Centre+Side*(965.f+float((I*71)%5-2)*35.f);
                    LandmarkLow(7,X,Y,18.f+I*31.f+Side*12.f,.80f+.04f*(I%4),.13f);
                    if(I%4==0)LandmarkLow(9+(I+(Side>0?1:0))%3,X+Side*115.f,Y+83.f,I*29.f,.83f,.12f);
                }
            }
        }
        else
        {
        const float OffsetB=float((Id*11)%7-3)*73.f;
        for(int32 I=0;I<11;++I)
        {
            const float X=FMath::Lerp(-Home*.94f,Home*.94f,float(I)/10.f)+((I*71+Id*13)%5-2)*47.f;
            const float Centre=-90.f+OffsetB+205.f*FMath::Sin(X/1590.f+Id*.29f)+.045f*X;
            if(I%3!=1)LandmarkLow(9+(I+Id)%3,X,Centre+(I%2?82.f:-80.f),19.f+(I%4)*23.f,.94f,.13f,true);
            // Broken bank timber/deposits follow both bends, preserving the
            // wet centre as the obvious low route rather than generic specks.
            LandmarkLow(7,X+115.f,Centre+(I%2?405.f:-405.f),I*37.f,.96f,.10f);
        }
        }
    }
    else if(Style==EFrontlineMissionTerrainV32::Canal&&Id==16)
    {
        // The photographed bridge belongs to the same physical St Quentin
        // cutting. V42's rooted earth skirts and broken timber revetments have
        // scanned mud/wood/rust materials; seat sparse, overlapping remnants
        // on its actual crests, not a repeated row of clean primitive blocks.
        // This bearing must match StQuentinSectionV55 in Ground.cpp.
        const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
        auto Axis=[&](float Y){return 600.f+.24f*Home-.70f*(Y+1450.f)+90.f*FMath::Sin(Y/1450.f+1.1f);};
        auto ClearBank=[&](float X,float Y,float Margin)
        {
            for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
                if(FMath::Abs(X-FrontlineFieldV32::TrenchCentreX(T,Y))<Margin)return false;
            return true;
        };
        auto ClearBreaches=[](float Y)
        {
            // Ground.cpp grades both openings inside the playable frontage.
            return FMath::Abs(Y-850.f)>=510.f&&FMath::Abs(Y+850.f)>=530.f;
        };
        const int32 BeforeCanal=S.Instances;
        for(int32 I=0;I<29;++I)
        {
            const float Y=-4550.f+I*215.f+R.FRandRange(-24.f,24.f);
            // The two graded breaches remain conspicuously open. Near active
            // trenches the large-form protection removes the bank altogether;
            // leave those portions free of decorative, unsupported timber.
            if(!ClearBreaches(Y))continue;
            const float Centre=Axis(Y);
            const float TangentYaw=FMath::RadiansToDegrees(FMath::Atan(.70f-(90.f/1450.f)*FMath::Cos(Y/1450.f+1.1f)));
            for(int32 Side:{-1,1})
            {
                if((I+(Side>0?3:0))%7==2)continue;
                const float X=Centre+Side*(1470.f+R.FRandRange(-72.f,72.f));
                // The actual local crest, not a mask measured at the canal
                // centre, decides whether this bank fragment has earth under it.
                if(!ClearBank(X,Y,440.f)||H(X,Y)-H(X-Side*250.f,Y)<5.f)continue;
                const int32 Before=S.Instances;
                Low(5+(I+(Side>0?1:0))%2,X,Y,TangentYaw+R.FRandRange(-19.f,19.f),R.FRandRange(1.27f,1.43f));
                S.LandmarkInstancesV54+=S.Instances-Before;
                if(I%3==0||I%5==1)
                {
                    const float Y2=Y+137.f,X2=Axis(Y2)+Side*(1500.f+R.FRandRange(-49.f,49.f));
                    if(ClearBreaches(Y2)
                        &&ClearBank(X2,Y2,440.f)&&H(X2,Y2)-H(X2-Side*240.f,Y2)>5.f)
                    {
                        const int32 PairBefore=S.Instances;
                        Low(5+(I+1+(Side>0?1:0))%2,X2,Y2,TangentYaw+R.FRandRange(-24.f,24.f),R.FRandRange(1.03f,1.19f));
                        S.LandmarkInstancesV54+=S.Instances-PairBefore;
                    }
                }
            }
        }
        // Three separate surviving revetment runs: the old wood/rust mesh is
        // already sculpted and weathered, while intervening bare slopes and
        // the two crossings remain obvious. None becomes a continuous fence.
        struct FCanalTimberRun{float Y,Side,Length;};
        const FCanalTimberRun TimberRuns[]={
            {-3850.f,-1.f,1.04f},{-3520.f,-1.f,.91f},
            {-1970.f,1.f,1.02f},{-1730.f,1.f,.88f},
            {-210.f,1.f,1.01f},{105.f,1.f,.90f}};
        for(int32 I=0;I<UE_ARRAY_COUNT(TimberRuns);++I)
        {
            const auto& Run=TimberRuns[I];const float Y=Run.Y;
            if(!ClearBreaches(Y))continue;
            const float WX=Axis(Y)+Run.Side*1230.f;
            const float Angle=FMath::Atan(.70f-(90.f/1450.f)*FMath::Cos(Y/1450.f+1.1f))
                +FMath::DegreesToRadians(I%2?5.f:-7.f);
            const float DX=-FMath::Sin(Angle)*81.f*Run.Length,DY=FMath::Cos(Angle)*81.f*Run.Length;
            if(!ClearBank(WX-DX,Y-DY,460.f)||!ClearBank(WX,Y,460.f)
                ||!ClearBank(WX+DX,Y+DY,460.f))continue;
            if(H(WX,Y)-H(WX-Run.Side*260.f,Y)<5.f)continue;
            const float Z=FMath::Min3(H(WX-DX,Y-DY),H(WX,Y),H(WX+DX,Y+DY))-10.f;
            const int32 Before=S.Instances;
            Add(3+I%2,FVector(WX,Y,Z),FRotator(0,FMath::RadiansToDegrees(Angle),0),FVector(1.f,Run.Length,.85f));
            S.LandmarkInstancesV54+=S.Instances-Before;
        }
        S.CanalBankDetailsV55=S.Instances-BeforeCanal;
    }
    else if(Style==EFrontlineMissionTerrainV32::RailCutting)
    {
        const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
        // Only a few low, twisted remnants survive on the cut shoulders.
        // Unequal clusters and long bare gaps keep them from reading as a
        // second pair of straight rails or a repeated line of dark blocks.
        struct FBankRemnant{float Along,Side,Shoulder,Yaw,Length,Height;};
        const FBankRemnant Remnants[]={
            {-1.02f,-1.f, 44.f,-54.f,.77f,.62f},
            {-.63f, 1.f,-36.f,-25.f,.88f,.68f},
            {-.36f, 1.f, 51.f,-69.f,.67f,.56f},
            {-.26f, 1.f,-21.f,-38.f,.91f,.62f},
            {.12f,-1.f, 68.f,-67.f,.75f,.58f},
            {.59f, 1.f,-32.f,-28.f,.82f,.65f},
            {.88f,-1.f, 15.f,-56.f,.70f,.55f}};
        for(int32 I=0;I<UE_ARRAY_COUNT(Remnants);++I)
        {
            const auto& Remnant=Remnants[I];
            const float X=Remnant.Along*Home;
            const float Axis=RailY+47.f*FMath::Sin(X/1370.f+Id*.17f);
            const float Y=Axis+Remnant.Side*(465.f+Remnant.Shoulder);
            LandmarkRetaining(3+I%2,X,Y,Remnant.Yaw,Remnant.Length,Remnant.Height,.13f);
        }
        for(int32 I=0;I<15;++I)
        {
            const float X=FMath::Lerp(-Home-1200.f,Home+1200.f,float(I)/14.f)+((I*59+Id*17)%7-3)*31.f;
            const float Axis=RailY+47.f*FMath::Sin(X/1370.f+Id*.17f);
            const float Side=I%2?1.f:-1.f,Y=Axis+Side*(490.f+float(I%3)*32.f);
            // Continuous operating rails remain clear. Ballast and snapped
            // sleepers instead occupy the elevated cut shoulders on both sides.
            LandmarkLow(I%4==0?5+I%2:7,X,Y,19.f+I*27.f,1.08f,.13f);
            if(I%3==0)LandmarkLow(9+(I+Id)%3,X+105.f,Y+Side*47.f,Side*64.f+I*11.f,.94f,.09f);
        }
        // The operation's two small rear service shelters connect the thin
        // running rails with the destroyed rail-side landscape. Their entire
        // authored bounds stay beyond the active soldier frontage.
        for(int32 I=0;I<2;++I)
        {
            const float X=(I?+.49f:-.38f)*Home,Y=-1960.f-I*130.f;
            const int32 Before=S.Instances;
            Add(13,FVector(X,Y,H(X,Y)-5.f),FRotator(0,I?11.f:-16.f,0),FVector(.95f,1.f,.88f));
            S.LandmarkInstancesV54+=S.Instances-Before;
        }
    }
    // Regional silhouette bridges the playable ground and distant landscape.
    // Six physically scaled fractured variants share paired bark/endgrain.
    // Instancing adds no bodies, ticks or collision.
    FRandomStream Landscape(501917+Id*317+int32(Style)*997);
    const float Extent=FMath::Min(FrontlineBattlefield::GoalHalfLength+950.f,9600.f);
    auto Timber=[&](int32 M,float X,float Y,float Size,float YawAngle,float Lean=0.f,float Stretch=1.f,bool bMeninGully=false)
    {
        // Preserve the accepted variant sequence: older Villers builds counted
        // rear debris before trees. Keep that parity seed without conflating
        // two different resource families in the tree-count invariant.
        if((S.LandscapeInstancesV50+S.OutskirtsInstancesV8449+Id)%2)M+=3;
        const FVector Scale(Size,Size*.93f,Size*Stretch);
        const FRotator AuthoredRotation(Lean,YawAngle,Landscape.FRandRange(-4,4));
        const FTransform Support=TimberSupportV76(M-17,FVector(X,Y,0),AuthoredRotation.Quaternion(),Scale,S.TimberBaselineV8448);
        const FRotator Rotation=Support.Rotator();const FVector P=Support.GetLocation();
        const FBox Box=Meshes[M]->GetBoundingBox().TransformBy(Support);
        // Other operations keep entire silhouettes beyond the active frontage.
        // Menin places them by its gully bounds and road/trench clearance.
        if(bMeninGully)
        {if(!MeninTimberClearV59(Box))return;}
        else if(Box.Min.Y<FrontlineBattlefield::HalfFrontage+190.f&&Box.Max.Y> -FrontlineBattlefield::HalfFrontage-190.f)return;
        if(!S.TimberBaselineV8448&&!TimberPassageClearV8448(Box,S.ArmourAcrossV8448,Rail))return;
        const int32 Before=S.Instances;Add(M,P,Rotation,Scale);S.LandscapeInstancesV50+=S.Instances-Before;
    };
    switch(Style)
    {
        case EFrontlineMissionTerrainV32::Farmland:
        case EFrontlineMissionTerrainV32::Village:
        {
            if(Id==13)
            {
                // Fractured, non-colliding outskirts occupy the same rear
                // relief as the Village profile. A staggered handful of
                // existing scanned earth/timber modules gives the matte a
                // physical foreground without erecting duplicate flat walls.
                const FVector2D Outskirts[]={
                    {-6340.f,-4350.f},{-4860.f,-4790.f},{-3520.f,-4470.f},
                    {-1550.f,-5070.f},{-110.f,-4560.f},{1700.f,-5000.f},
                    {3840.f,-4480.f},{5320.f,-4890.f},{6820.f,-4310.f}};
                for(int32 I=0;I<UE_ARRAY_COUNT(Outskirts);++I)
                {
                    const float X=Outskirts[I].X,Y=Outskirts[I].Y;
                    const int32 Before=S.Instances;
                    Add(7,FVector(X,Y,H(X,Y)-11.f),FRotator(0,19.f+I*41.f,0),
                        FVector(1.12f+.12f*(I%3),1.03f+.10f*(I%2),.75f));
                    Add(5+I%2,FVector(X+210.f,Y-95.f,H(X+210.f,Y-95.f)-17.f),
                        FRotator(0,74.f+I*29.f,0),FVector(1.16f,1.06f,.76f));
                    if(I%3==1)
                        Add(13,FVector(X-230.f,Y+78.f,H(X-230.f,Y+78.f)-15.f),
                            FRotator(0,I*53.f,0),FVector(.82f,.87f,.64f));
                    S.OutskirtsInstancesV8449+=S.Instances-Before;
                }
            }
            // Surviving spacing suggests former orchards and garden boundaries;
            // gaps, split stumps and windthrown logs break any repeated fence.
            const int32 Rows=Style==EFrontlineMissionTerrainV32::Village?3:2;
            for(int32 Row=0;Row<Rows;++Row)for(int32 I=0;I<12;++I)
            {
                if((I+Row*3+Id)%5==1)continue;
                const float X=FMath::Lerp(-Extent,Extent,float(I)/11.f)+Landscape.FRandRange(-130,130);
                const float Y=-1890.f-Row*625.f+X*.028f+Landscape.FRandRange(-100,100);
                const bool Stump=(I+Row+Id)%4==0;
                Timber(Stump?18:17,X,Y,Landscape.FRandRange(.78f,1.32f),Landscape.FRandRange(0,360),Landscape.FRandRange(-13,13));
                if((I+Row)%4==2)Timber(19,X+180,Y-180,Landscape.FRandRange(.72f,1.06f),25.f+Landscape.FRandRange(-28,28));
            }
            break;
        }
        case EFrontlineMissionTerrainV32::Marsh:
        {
            // Thin leaning remains cluster along drowned drainage margins.
            for(int32 Cluster=0;Cluster<8;++Cluster)
            {
                const float X=FMath::Lerp(-Extent,Extent,(Cluster+.35f)/8.f),Y=-2090.f-Landscape.FRandRange(0,1050);
                for(int32 I=0;I<3;++I)Timber((I+Cluster)%4==0?18:17,X+I*235.f+Landscape.FRandRange(-70,70),Y-I*130.f,
                    Landscape.FRandRange(.67f,.92f),Landscape.FRandRange(0,360),Landscape.FRandRange(-17,17),1.35f);
                Timber(19,X-165,Y+120,Landscape.FRandRange(.73f,1.13f),Landscape.FRandRange(-28,28));
            }
            break;
        }
        case EFrontlineMissionTerrainV32::Canal:
        {
            // The interrupted bank-side avenue follows the water's long line.
            for(int32 I=0;I<18;++I)
            {
                if((I+Id)%5==1)continue;
                const float X=FMath::Lerp(-Extent,Extent,(I+.5f)/18.f),Y=-2150.f+X*.042f;
                Timber(I%4==0?18:17,X+Landscape.FRandRange(-80,80),Y+Landscape.FRandRange(-95,95),Landscape.FRandRange(.83f,1.29f),Landscape.FRandRange(0,360),Landscape.FRandRange(-9,9));
                if(I%3==0)Timber(19,X+120,Y-620,Landscape.FRandRange(.82f,1.18f),Landscape.FRandRange(55,120));
            }
            break;
        }
        case EFrontlineMissionTerrainV32::Ridge:
        case EFrontlineMissionTerrainV32::RailCutting:
        {
            if(Id==7)
            {
                // V59's oblique road has two drainage gullies. The visible
                // remnants form a few ruined woodland pockets, with pauses
                // between them; alternating sides at fixed intervals put most
                // of the old candidates outside the useful ground.
                struct FMeninStand{float X;int32 Side,Count;};
                const FMeninStand Stands[]={
                    {-4520.f, 1,2},
                    {-2810.f, 1,4},{-2130.f, 1,5},{-1490.f, 1,4},
                    {-1670.f,-1,3},
                    { -110.f, 1,4},{  650.f,-1,5},
                    { 1350.f,-1,4},{ 1630.f, 1,3},
                    { 2130.f,-1,4},{ 2780.f,-1,5},
                    { 4510.f,-1,2}};
                for(int32 Stand=0;Stand<UE_ARRAY_COUNT(Stands);++Stand)
                {
                    const auto& Site=Stands[Stand];
                    for(int32 I=0;I<Site.Count;++I)
                    {
                        // A staggered handful around each original stand reads
                        // as a broken treeline, rather than scattered confetti.
                        const float X=Site.X+(float(I)-float(Site.Count-1)*.5f)*92.f
                            +Landscape.FRandRange(-65.f,65.f);
                        const float Gully=Site.Side>0
                            ?925.f+90.f*FMath::Sin(X/1580.f+.8f)
                            :-865.f+100.f*FMath::Sin(X/1760.f+1.4f);
                        const float Y=MeninRoadAxisV59(X)+Gully
                            +Site.Side*Landscape.FRandRange(175.f,305.f)+Landscape.FRandRange(-75.f,75.f);
                        const bool SoldierLane=FMath::Abs(Y)<FrontlineBattlefield::HalfFrontage+270.f;
                        // The old horizontal V12 trunk exposes a dark,
                        // funnel-shaped end cap at tactical zoom and can
                        // visually bridge a trench even with clear AABBs.
                        // Menin uses the low fractured stump and sparse
                        // upright remnant until that mesh is re-authored.
                        const int32 M=SoldierLane||I%3!=0?18:17;
                        Timber(M,X,Y,M==17?Landscape.FRandRange(.72f,1.03f):Landscape.FRandRange(.75f,1.06f),
                            Landscape.FRandRange(0.f,360.f),M==17?Landscape.FRandRange(-16.f,16.f):Landscape.FRandRange(-6.f,6.f),
                            M==17?Landscape.FRandRange(.92f,1.12f):1.f,true);
                    }
                }
                break;
            }
            // Chalk crests stay open. A few shattered hedgerow clusters sit in
            // the hollows; the cutting uses more low ballast-side timber.
            const bool Cutting=Style==EFrontlineMissionTerrainV32::RailCutting;
            for(int32 I=0;I<(Cutting?12:8);++I)
            {
                const float X=Landscape.FRandRange(-Extent,Extent),Y=-2050.f-Landscape.FRandRange(0,1400);
                Timber(I%3==0?17:18,X,Y,Landscape.FRandRange(.68f,1.05f),Landscape.FRandRange(0,360),Landscape.FRandRange(-11,11));
                if(I%2==0)Timber(19,X+230,Y-130,Landscape.FRandRange(.72f,1.02f),Landscape.FRandRange(-22,22));
            }
            break;
        }
    }
    // Low foreground remnants frame the far flanks; tall trunks never pass in
    // front of infantry or mask the trench controls at the normal play camera.
    if(Id!=7)for(int32 Side:{-1,1})for(int32 I=0;I<2;++I)
        Timber(I?19:18,Side*(Extent*.83f+I*180),2020+I*240,Landscape.FRandRange(.68f,.89f),Landscape.FRandRange(0,360));
    if(Id==7&&FParse::Param(FCommandLine::Get(),TEXT("WFV62MeninDebris")))
    {
        // Separate, opt-in composition study: existing authored PBR timber,
        // rust and earth in one low Nanite scatter mesh. Unequal blast pockets
        // replace a regular grid; quiet gaps preserve the silhouettes of the
        // road and trenches. No actor ticks, collision or new texture reads.
        struct FDebrisPocket{float X,Y;int32 Pieces;};
        const FDebrisPocket Pockets[]={
            {-6150,-1540,4},{-5340,690,5},{-4490,1900,3},
            {-3480,-760,5},{-2810,1720,3},{-2070,-1740,4},
            {-1320,920,4},{-380,-1880,5},{410,1690,4},
            {1280,-950,5},{2120,1070,3},{2810,-1810,5},
            {3670,750,4},{4460,-790,5},{5360,1670,4},{6220,-1320,3}};
        FRandomStream Scatter(6201917);
        UStaticMesh* ScatterMesh=Meshes[7];
        for(const auto& Pocket:Pockets)
        {
            int32 Placed=0;
            for(int32 Attempt=0;Attempt<Pocket.Pieces*6&&Placed<Pocket.Pieces;++Attempt)
            {
                const float Angle=Scatter.FRandRange(0.f,2.f*PI);
                const float Radius=FMath::Sqrt(Scatter.FRand())*Scatter.FRandRange(125.f,400.f);
                const float X=Pocket.X+FMath::Cos(Angle)*Radius;
                const float Y=Pocket.Y+FMath::Sin(Angle)*Radius*.78f;
                const float Size=Scatter.FRandRange(.62f,1.08f);
                const FVector Scale(Size,Size*Scatter.FRandRange(.85f,1.18f),Scatter.FRandRange(.78f,.94f));
                const float SlopeX=(H(X+50.f,Y)-H(X-50.f,Y))/100.f;
                const float SlopeY=(H(X,Y+50.f)-H(X,Y-50.f))/100.f;
                const FVector Normal(-SlopeX,-SlopeY,1.f);
                const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Normal.GetSafeNormal())
                    *FQuat(FVector::UpVector,Scatter.FRandRange(-PI,PI));
                const FBox Unseated=ScatterMesh->GetBoundingBox().TransformBy(FTransform(Rotation,FVector(X,Y,0),Scale));
                if(!MeninDebrisCorridorClearV62(Unseated)||!MeninDebrisDryV62(Unseated))continue;
                const float Heights[]={H(Unseated.Min.X,Unseated.Min.Y),H(Unseated.Min.X,Unseated.Max.Y),
                    H(Unseated.Max.X,Unseated.Min.Y),H(Unseated.Max.X,Unseated.Max.Y),H(X,Y)};
                float Lowest=Heights[0],Highest=Heights[0];
                for(float Height:Heights){Lowest=FMath::Min(Lowest,Height);Highest=FMath::Max(Highest,Height);}
                // Seat the lowest board into the clay; steep slopes would
                // otherwise leave a recognisable floating aggregate mesh.
                if(Highest-Lowest>29.f)continue;
                const FVector Position(X,Y,Lowest-Unseated.Min.Z-3.f);
                const FTransform Transform(Rotation,Position,Scale);
                const int32 Before=S.Instances;
                Add(7,Position,Rotation.Rotator(),Scale);
                if(S.Instances>Before){S.MeninDebrisV62.Add(Transform);++Placed;}
            }
        }
    }
    // Weather may have created these persistent fires on the menu/previous
    // profile. The final operation height field is now installed.
    FrontlineEmbersV27::ReseatGround(B->GetWorld());
    S.Ready=true;
    UE_LOG(LogTemp,Display,TEXT("WF_V42_ENVIRONMENT_READY mission=%d instances=%d canal_bank_details_v55=%d groups=%d no_new_collision=1 exact_v42_bank_contacts=1"),Id,S.Instances,S.CanalBankDetailsV55,S.Groups.Num());
    return true;
}

TSharedRef<FJsonObject> FrontlineEnvironmentV42::RunChecks(AWarfareFrontlineGameMode* B)
{
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV42Baseline")))
    {auto J=FrontlineEnvironmentV40::RunChecks(B);J->SetBoolField(TEXT("baseline_v40"),true);return J;}
    auto J=MakeShared<FJsonObject>();const auto* S=Fields.Find(B);const auto* Mission=B?FrontlineCampaignV32::CurrentOperation(B):nullptr;
    const int32 ExpectedRail=Mission&&Mission->bArmoredTrain?53:0;
    int32 Count=0,Bags=0,Rails=0,Collision=0,Materials=0,Obstructions=0,Corridor=0,Trees=0,LandscapeInPlay=0,MeninUnsafe=0;
    int32 MeninLegacyFallenMatched=0,MeninLegacyFallenVisible=0,MeninDebrisUnsafeV62=0;
    int32 StaticTreeCountV8448=0,StaticTreeUnsafeV8448=0,InstanceTreeUnsafeV8448=0,StaticTreeCollisionV8448=0,StaticTreeBindingV8448=0;
    int64 TreeTrianglesV8448=0;float CameraScoreV8448=MAX_flt;FVector CameraFocusV8448=FVector::ZeroVector;
    TArray<TSharedPtr<FJsonValue>> StaticTreeInventoryV8448,GroundedFireFragmentsV8448;
    int32 FireFragmentUnsafeV8448=0;
    auto AuditTimber=[&](UStaticMeshComponent* Component,const FTransform& Transform,bool Static)
    {
        if(!Component||!Component->GetStaticMesh())return;
        const UStaticMesh* Mesh=Component->GetStaticMesh();const FBox Box=Mesh->GetBoundingBox().TransformBy(Transform);
        const bool Clear=TimberPassageClearV8448(Box,S?S->ArmourAcrossV8448:510.f,ExpectedRail>0);
        const bool Binding=Mesh->GetPathName().StartsWith(S&&S->TimberBaselineV8448?TEXT("/Game/Warfare/Field/V76/Meshes/"):TEXT("/Game/Warfare/Field/V8448/Meshes/"));
        if(Static){++StaticTreeCountV8448;StaticTreeUnsafeV8448+=!Clear;StaticTreeCollisionV8448+=Component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;StaticTreeBindingV8448+=!Binding;}
        else InstanceTreeUnsafeV8448+=!Clear;
        const FStaticMeshRenderData* Data=Mesh->GetRenderData();
        const int32 Triangles=Data&&!Data->LODResources.IsEmpty()?Data->LODResources[0].GetNumTriangles():0;
        TreeTrianglesV8448+=Triangles;
        // Placement-space focus is reusable for the matched old/new camera;
        // changing silhouette height must not silently reframe the comparison.
        const FVector Origin=Transform.GetLocation();const float Score=FMath::Abs(float(Origin.X))+.3f*FMath::Abs(float(Origin.Y));
        if(!Static&&Score<CameraScoreV8448){CameraScoreV8448=Score;CameraFocusV8448=Origin+FVector(0,0,150);}
        if(Static)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("component"),Component->GetPathName());
            Row->SetStringField(TEXT("mesh"),Mesh->GetPathName());Row->SetBoolField(TEXT("whole_bounds_passage_clear"),Clear);
            Row->SetBoolField(TEXT("expected_mesh_binding"),Binding);
            Row->SetNumberField(TEXT("render_triangles"),Triangles);
            Row->SetNumberField(TEXT("x"),Origin.X);Row->SetNumberField(TEXT("y"),Origin.Y);Row->SetNumberField(TEXT("z"),Origin.Z);
            Row->SetNumberField(TEXT("min_y"),Box.Min.Y);Row->SetNumberField(TEXT("max_y"),Box.Max.Y);
            StaticTreeInventoryV8448.Add(MakeShared<FJsonValueObject>(Row));
        }
    };
    if(Mission&&Mission->Id==7)for(TActorIterator<AStaticMeshActor> It(B->GetWorld());It;++It)
        if(auto* C=It->GetStaticMeshComponent();IsMeninLegacyFallenV59(*It,C))
        {
            ++MeninLegacyFallenMatched;
            MeninLegacyFallenVisible+=C->IsVisible()?1:0;
        }
    int32 PromotedEarth=0,EarthParameters=0;bool Finite=true;
    // TerrainV42 deliberately promotes only authored earth slots to the V44
    // photographed bank shader. Validate that exact binding and its operation
    // values; timber, cloth, rails and metal must keep their authored ancestry.
    auto* AuthoredEarth=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V42/Materials/MI_EarthBank_V42.MI_EarthBank_V42"));
    auto* DisplayedEarth=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V76/Materials/MI_EarthBank_V76.MI_EarthBank_V76"));
    if(!DisplayedEarth)DisplayedEarth=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V44/Materials/MI_EarthBank_V44.MI_EarthBank_V44"));
    if(!DisplayedEarth)DisplayedEarth=AuthoredEarth;
    // Match the current production bank selector, including the accepted V839
    // replacement. A correct MID parent is not an obsolete V76 binding error.
    DisplayedEarth=FrontlineArtV68::ResolveBankMaterial(DisplayedEarth);
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV839EnvironmentalBaseline")))
        if(auto* Current=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V839/Materials/MI_EarthBank_V839.MI_EarthBank_V839")))DisplayedEarth=Current;
    const auto Palette=TerrainPaletteV51(Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland,Mission?Mission->Id:0);
    TArray<TSharedPtr<FJsonValue>> Inventory;
    if(S)for(int32 G=0;G<S->Groups.Num();++G)if(auto* C=S->Groups[G].Get())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("mesh"),C->GetStaticMesh()?C->GetStaticMesh()->GetPathName():TEXT("missing"));
        Row->SetNumberField(TEXT("instances"),C->GetInstanceCount());Count+=C->GetInstanceCount();if(IsBagV84412(G))Bags+=C->GetInstanceCount();if(IsRail(G))Rails+=C->GetInstanceCount();if(G>=17&&G<23)Trees+=C->GetInstanceCount();
        Collision+=C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;
        TArray<TSharedPtr<FJsonValue>> Paths;
        for(int32 M=0;M<C->GetNumMaterials();++M)
        {
            const FString Path=C->GetMaterial(M)?C->GetMaterial(M)->GetPathName():TEXT("missing");Paths.Add(MakeShared<FJsonValueString>(Path));
            auto* Expected=C->GetStaticMesh()?C->GetStaticMesh()->GetMaterial(M):nullptr;
            const bool Formal=S&&S->BagsV87&&IsBagV84412(G)&&FrontlineSandbagDescriptorV87::IsMaterial(Expected);
            const bool Authored=Expected&&(Formal||Expected==AuthoredEarth||(S&&S->BagsV84412&&(G==12||G==13)&&RoofMaterialV84413(Expected))||Expected->GetPathName().StartsWith(FString(TEXT("/Game/Warfare/Field/"))+AssetVersion(G,S&&S->TimberBaselineV8448,S&&S->PropsV8449,S&&S->BagsV84412)+TEXT("/Materials/")));
            const bool EarthSlot=Expected&&Expected==AuthoredEarth;
            const UMaterialInterface* Required=Formal?Expected:EarthSlot?DisplayedEarth:WornHessianV84413(Expected,S&&S->BagsV84412);
            const UMaterialInterface* Resolved=C->GetMaterial(M);bool ExactParent=false;
            for(int32 Depth=0;Resolved&&Depth<12;++Depth)
            {if(Resolved==Required){ExactParent=true;break;}const auto* Instance=Cast<UMaterialInstance>(Resolved);Resolved=Instance?Instance->Parent.Get():nullptr;}
            Materials+=!Authored||!ExactParent;
            if(EarthSlot)
            {
                PromotedEarth+=ExactParent&&DisplayedEarth!=AuthoredEarth;
                float Wet=0,Chalk=0;const auto* Displayed=C->GetMaterial(M);
                const bool Parameters=Displayed&&Displayed->GetScalarParameterValue(FMaterialParameterInfo(TEXT("WeatherWetness")),Wet)
                    &&Displayed->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ChalkExposure")),Chalk)
                    &&FMath::IsNearlyEqual(Wet,Mission?Mission->Wetness:.58f,.001f)&&FMath::IsNearlyEqual(Chalk,Palette.Chalk,.001f);
                EarthParameters+=!Parameters;
            }
        }
        Row->SetArrayField(TEXT("materials"),Paths);Inventory.Add(MakeShared<FJsonValueObject>(Row));
        for(int32 I=0;I<C->GetInstanceCount();++I)
        {
            FTransform T;C->GetInstanceTransform(I,T,true);Finite&=!T.ContainsNaN()&&T.GetScale3D().GetAbsMin()>.5f;
            if(!C->GetStaticMesh())continue;
            const FBox Box=C->GetStaticMesh()->GetBoundingBox().TransformBy(T);
            if(G>=17&&G<23)
            {
                AuditTimber(C,T,false);
                LandscapeInPlay+=Box.Min.Y<FrontlineBattlefield::HalfFrontage+190.f&&Box.Max.Y> -FrontlineBattlefield::HalfFrontage-190.f;
                if(Mission&&Mission->Id==7)MeninUnsafe+=!MeninTimberClearV59(Box);
            }
            if(ExpectedRail&&!IsRail(G))Obstructions+=Box.Min.Y<RailY+RailHalfWidth&&Box.Max.Y>RailY-RailHalfWidth;
            // Clay/root/debris/stores are placed wholly outside the central
            // walking passage; treads and bank-retaining walls have separate roles.
            if(G>=5&&G<=8)for(int32 K=0;K<FrontlineBattlefield::TrenchCount;++K)
            {
                const float CX=FrontlineFieldV32::TrenchCentreX(K,T.GetLocation().Y);
                Corridor+=Box.Min.X<CX+82.f&&Box.Max.X>CX-82.f;
            }
        }
    }
    if(B)for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)if(C&&C->IsVisible()&&!It->IsHidden()&&!C->IsA<UInstancedStaticMeshComponent>()&&C->GetStaticMesh())
        {
            if(IsGroundedFireFragmentV8448(C))
            {
                const FBox Box=C->GetStaticMesh()->GetBoundingBox().TransformBy(C->GetComponentTransform());
                const FVector Centre=Box.GetCenter();const float Ground=AWarfareFrontlineGround::HeightAt(Centre.X,Centre.Y);
                const bool Grounded=FMath::Abs(float(Box.Min.Z-Ground))<=35.f&&Box.Max.Z-Ground<=65.f;
                const bool NoCollision=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
                FireFragmentUnsafeV8448+=!Grounded||!NoCollision;
                auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("component"),C->GetPathName());
                Row->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());Row->SetBoolField(TEXT("grounded"),Grounded);
                Row->SetBoolField(TEXT("collision_disabled"),NoCollision);Row->SetNumberField(TEXT("height_cm"),Box.GetSize().Z);
                Row->SetNumberField(TEXT("bottom_from_ground_cm"),Box.Min.Z-Ground);Row->SetNumberField(TEXT("top_from_ground_cm"),Box.Max.Z-Ground);
                GroundedFireFragmentsV8448.Add(MakeShared<FJsonValueObject>(Row));
            }
            else if(IsTimberNameV8448(C->GetStaticMesh()->GetName()))AuditTimber(C,C->GetComponentTransform(),true);
        }
    }
    if(S)for(const auto& Transform:S->MeninDebrisV62)
    {
        const UStaticMesh* Mesh=S->Groups.IsValidIndex(7)&&S->Groups[7].IsValid()?S->Groups[7]->GetStaticMesh().Get():nullptr;
        if(!Mesh){++MeninDebrisUnsafeV62;continue;}
        const FBox Box=Mesh->GetBoundingBox().TransformBy(Transform);
        MeninDebrisUnsafeV62+=!MeninDebrisCorridorClearV62(Box)||!MeninDebrisDryV62(Box);
    }
    const auto Contacts=FrontlineFieldV32::RunBankContactChecksV42(B);
    const auto Style=Mission?Mission->TerrainStyleV32:EFrontlineMissionTerrainV32::Farmland;
    const bool LandmarkExpected=Style==EFrontlineMissionTerrainV32::Village||Style==EFrontlineMissionTerrainV32::Marsh||Style==EFrontlineMissionTerrainV32::RailCutting;
    const bool TimberClearV8448=S&&(S->TimberBaselineV8448||(StaticTreeUnsafeV8448==0&&InstanceTreeUnsafeV8448==0&&StaticTreeCollisionV8448==0&&StaticTreeBindingV8448==0))&&FireFragmentUnsafeV8448==0;
    const bool OK=S&&S->Ready&&S->Owner.IsValid()&&Count==S->Instances&&Count<=(S->BagsV84412?MaximumInstances:LegacyMaximumInstancesV84412)&&Bags>0&&Bags<=(S->BagsV84412?150:30)&&Rails==ExpectedRail&&Collision==0&&Materials==0&&EarthParameters==0&&Obstructions==0&&Corridor==0&&Finite&&MeninDebrisUnsafeV62==0&&Trees==S->LandscapeInstancesV50&&Trees>0&&(Mission&&Mission->Id==7?MeninUnsafe==0&&MeninLegacyFallenVisible==0&&MeninLegacyFallenMatched<=1:LandscapeInPlay==0)&&(!LandmarkExpected||S->LandmarkInstancesV54>0)&&Contacts->GetBoolField(TEXT("success"))&&TimberClearV8448;
    J->SetObjectField(TEXT("bank_contacts"),Contacts);
    J->SetBoolField(TEXT("success"),OK);J->SetBoolField(TEXT("baseline_v40"),false);J->SetNumberField(TEXT("instances"),Count);J->SetNumberField(TEXT("budget"),S&&S->BagsV84412?MaximumInstances:LegacyMaximumInstancesV84412);
    J->SetNumberField(TEXT("firing_bays"),S&&S->BagsV84412?S->FiringBaysV84412:Bags);J->SetNumberField(TEXT("rail_modules"),Rails);J->SetNumberField(TEXT("expected_rail_modules"),ExpectedRail);
    J->SetNumberField(TEXT("collision_components"),Collision);J->SetNumberField(TEXT("invalid_materials"),Materials);J->SetNumberField(TEXT("rail_obstructions"),Obstructions);
    J->SetNumberField(TEXT("exact_promoted_earth_bindings"),PromotedEarth);J->SetNumberField(TEXT("incorrect_earth_parameters"),EarthParameters);
    J->SetNumberField(TEXT("walking_corridor_obstructions"),Corridor);J->SetBoolField(TEXT("finite_transforms"),Finite);J->SetArrayField(TEXT("inventory"),Inventory);
    TArray<TSharedPtr<FJsonValue>> CorridorRepairs;
    if(S)for(const auto& Repair:S->CorridorRepairsV8449)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("mesh"),Names[Repair.Mesh]);
        Row->SetNumberField(TEXT("from_x"),Repair.Before.X);Row->SetNumberField(TEXT("from_y"),Repair.Before.Y);
        Row->SetNumberField(TEXT("to_x"),Repair.After.X);Row->SetNumberField(TEXT("to_y"),Repair.After.Y);
        CorridorRepairs.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetArrayField(TEXT("walking_corridor_placement_repairs"),CorridorRepairs);
    J->SetNumberField(TEXT("regional_landscape_instances"),Trees);J->SetNumberField(TEXT("landscape_inside_play_frontage"),LandscapeInPlay);
    J->SetNumberField(TEXT("recorded_tree_instances"),S?S->LandscapeInstancesV50:0);
    J->SetNumberField(TEXT("regional_outskirts_instances"),S?S->OutskirtsInstancesV8449:0);
    J->SetBoolField(TEXT("timber_v8448_photographed"),S&&!S->TimberBaselineV8448);
    J->SetBoolField(TEXT("timber_v8448_isolated_baseline"),S&&S->TimberBaselineV8448);
    J->SetNumberField(TEXT("timber_v8448_armour_swept_half_width_cm"),S?S->ArmourAcrossV8448:0.f);
    J->SetNumberField(TEXT("timber_v8448_visible_static_components"),StaticTreeCountV8448);
    J->SetNumberField(TEXT("timber_v8448_static_route_obstructions"),StaticTreeUnsafeV8448);
    J->SetNumberField(TEXT("timber_v8448_instance_route_obstructions"),InstanceTreeUnsafeV8448);
    J->SetNumberField(TEXT("timber_v8448_static_collision_components"),StaticTreeCollisionV8448);
    J->SetNumberField(TEXT("timber_v8448_static_binding_errors"),StaticTreeBindingV8448);
    J->SetNumberField(TEXT("timber_v8448_legacy_relocated"),S?S->LegacyTimberRelocatedV8448:0);
    J->SetNumberField(TEXT("timber_v8448_legacy_hidden"),S?S->LegacyTimberHiddenV8448:0);
    J->SetNumberField(TEXT("timber_v8448_visible_render_triangles"),double(TreeTrianglesV8448));
    J->SetArrayField(TEXT("timber_v8448_static_inventory"),StaticTreeInventoryV8448);
    J->SetArrayField(TEXT("grounded_fire_fragments"),GroundedFireFragmentsV8448);
    J->SetNumberField(TEXT("grounded_fire_fragments_unsafe"),FireFragmentUnsafeV8448);
    auto Focus=MakeShared<FJsonObject>();Focus->SetNumberField(TEXT("x"),CameraFocusV8448.X);Focus->SetNumberField(TEXT("y"),CameraFocusV8448.Y);Focus->SetNumberField(TEXT("z"),CameraFocusV8448.Z);
    J->SetObjectField(TEXT("timber_v8448_camera_focus"),Focus);
    // One explicit review readback covers actual bound resources and the
    // original render proxy. This never runs in normal scene drawing/ticks.
    TSet<UMaterialInterface*> PropMaterials;TArray<TSharedPtr<FJsonValue>> BagContacts;
    int32 BagSupportErrorsV8449=0;FVector BagFocusV8449=FVector::ZeroVector;float BagFocusScore=MAX_flt;
    AWarfareFrontlineGround* BagGroundV84412=nullptr;
    if(B)for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(It->Surface){BagGroundV84412=*It;break;}
    TArray<TSharedPtr<FJsonValue>> BagMaskInventoryV84412;
    if(S)for(int32 G=0;G<S->Groups.Num();++G)if(IsBagV84412(G))if(auto* C=S->Groups[G].Get())
    {
        for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)if(auto* M=C->GetMaterial(Slot))PropMaterials.Add(M);
        if(S->BagsV84412&&C->GetStaticMesh()&&C->GetStaticMesh()->GetRenderData()&&C->GetStaticMesh()->GetRenderData()->LODResources.Num())
        {
            const auto& Colors=C->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.ColorVertexBuffer;
            TArray<FColor> Values;Colors.GetVertexColors(Values);double Soil=0,Wet=0;
            for(const FColor& Color:Values){Soil+=double(Color.R)/255.;Wet+=double(Color.G)/255.;}
            auto MaskRow=MakeShared<FJsonObject>();MaskRow->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());
            MaskRow->SetNumberField(TEXT("colour_vertices"),Values.Num());MaskRow->SetNumberField(TEXT("linear_soil_mask_mean"),Values.IsEmpty()?0:Soil/Values.Num());
            MaskRow->SetNumberField(TEXT("linear_wet_mask_mean"),Values.IsEmpty()?0:Wet/Values.Num());
            MaskRow->SetStringField(TEXT("decode"),TEXT("Authored LINEAR FBX vertex mask; direct UNORM read, no second sRGB decode."));
            BagMaskInventoryV84412.Add(MakeShared<FJsonValueObject>(MaskRow));
        }
        for(int32 I=0;I<C->GetInstanceCount();++I)
        {
            FTransform T;C->GetInstanceTransform(I,T,true);float Low=MAX_flt,High=-MAX_flt,Contact=MAX_flt;
            const auto* Placed=S->BagsPlacedV84412.FindByPredicate([G,I](const auto& P){return P.Group==G&&P.Instance==I;});
            const bool Upper=Placed&&!Placed->Supports.IsEmpty();bool Sampled=true;
            if(S->BagsV84412)for(int32 Foot=0;Foot<9;++Foot)
            {
                const FVector P=T.TransformPosition(BagFeetV87(*S,BagVariantV84412(G))[Foot]);
                FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;float Bank=0,Ground=H(P.X,P.Y);
                const bool Found=BagGroundV84412&&FrontlineGroundContactV8449::Sample(BagGroundV84412,P.X,P.Y,Actual,Normal);
                Sampled&=Found;if(Found)Ground=Actual.Z;
                if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,FVector(P.X,P.Y,Ground),Bank))Ground=FMath::Max(Ground,Bank);
                if(Placed)for(int32 Index:Placed->Supports)if(S->BagsPlacedV84412.IsValidIndex(Index))
                {
                    const auto& Lower=S->BagsPlacedV84412[Index];FTransform Support;float Top=0;
                    if(S->Groups.IsValidIndex(Lower.Group)&&S->Groups[Lower.Group].IsValid()
                        &&S->Groups[Lower.Group]->GetInstanceTransform(Lower.Instance,Support,true)
                        &&BagTopHeightV87(*S,Lower.Variant,Support,P.X,P.Y,Top))Ground=FMath::Max(Ground,Top);
                }
                const float Delta=P.Z-Ground;Low=FMath::Min(Low,Delta);High=FMath::Max(High,Delta);
                Contact=FMath::Min(Contact,FMath::Abs(Delta+(Upper?4.5f:2.5f)));
            }
            else if(S->PropsV8449)for(int32 Foot=0;Foot<9;++Foot)
            {
                const FVector P=T.TransformPosition(PropContactsV8449[G][Foot]);float Bank=0,Ground=H(P.X,P.Y);
                if(FrontlineFieldV32::TryEarthworkSurfaceV36(B,FVector(P.X,P.Y,Ground),Bank))Ground=FMath::Max(Ground,Bank);
                const float Delta=P.Z-Ground;Low=FMath::Min(Low,Delta);High=FMath::Max(High,Delta);Contact=FMath::Min(Contact,FMath::Abs(Delta+3.f));
            }
            const bool Good=S->BagsV84412?(Placed&&Sampled&&Low>=-(Upper?4.58f:2.58f)&&High<=6.f&&Contact<=.08f):
                !S->PropsV8449||(Low>=-3.08f&&Contact<=.08f);BagSupportErrorsV8449+=!Good;
            const FVector P=T.GetLocation();const float Score=FMath::Abs(float(P.X))+.05f*FMath::Abs(float(P.Y));
            if(Score<BagFocusScore){BagFocusScore=Score;BagFocusV8449=P+FVector(0,0,30);}
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());
            Row->SetBoolField(TEXT("upper_sack"),Upper);Row->SetBoolField(TEXT("actual_indexed_ground_sampled"),Sampled);
            Row->SetNumberField(TEXT("support_sacks"),Placed?Placed->Supports.Num():0);
            Row->SetBoolField(TEXT("actual_ground_support"),Good);Row->SetNumberField(TEXT("min_foot_ground_cm"),S->PropsV8449?Low:0);
            Row->SetNumberField(TEXT("max_foot_gap_cm"),S->PropsV8449?High:0);BagContacts.Add(MakeShared<FJsonValueObject>(Row));
        }
    }
    int32 RootEdgeErrors=0;bool RootBound=!S||!S->PropsV8449;FString RootParent;
    if(S&&S->PropsV8449)
    {
        AWarfareFrontlineGround* RootGround=nullptr;
        for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(It->Surface){RootGround=*It;break;}
        for(const FVector& P:S->RootEdgeSamplesV8449)
        {
            FVector Actual=FVector::ZeroVector,Normal=FVector::UpVector;
            const bool Sampled=RootGround&&FrontlineGroundContactV8449::Sample(RootGround,P.X,P.Y,Actual,Normal);
            RootEdgeErrors+=!Sampled||FMath::Abs(float(P.Z-Actual.Z)+2.f)>.08f;
        }
        if(auto* C=S->RootContactsV8449.Get())
        {
            auto* M=C->GetMaterial(0);if(M)PropMaterials.Add(M);RootParent=GetPathNameSafe(M);
            for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
                if(It->Surface&&It->Surface->GetMaterial(0)==M)RootBound=true;
        }
    }
    if(B)for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)if(C->IsVisible()&&C->ComponentHasTag(TEXT("WF_Prop_V8449")))
            for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)if(auto* M=C->GetMaterial(Slot))PropMaterials.Add(M);
    }
    struct FObservedPropV8449{bool Original=false,NoFallback=false,Valid=false;FString Name;};
    const TArray<UMaterialInterface*> MaterialList=PropMaterials.Array();TArray<FObservedPropV8449> Render;Render.SetNum(MaterialList.Num());
    TArray<const FMaterialRenderProxy*> Proxies;for(auto* M:MaterialList)Proxies.Add(M->GetRenderProxy());
    const auto Feature=B&&B->GetWorld()?B->GetWorld()->GetFeatureLevel():GMaxRHIFeatureLevel;
    ENQUEUE_RENDER_COMMAND(WarfareFieldPropReadbackV8449)([&Render,Proxies,Feature](FRHICommandListImmediate&)
    {
        for(int32 I=0;I<Proxies.Num();++I)if(Proxies[I])if(const FMaterial* Direct=Proxies[I]->GetMaterialNoFallback(Feature))
        {
            auto& R=Render[I];R.Original=true;const FMaterial& Actual=Proxies[I]->GetIncompleteMaterialWithFallback(Feature);
            R.NoFallback=&Actual==Direct&&!Direct->IsDefaultMaterial();R.Name=Actual.GetFriendlyName();
            const auto* Map=Direct->GetRenderingThreadShaderMap();R.Valid=Map&&Map->IsValidForRendering();
        }
    });
    FlushRenderingCommands();int32 ShaderErrors=0;TArray<TSharedPtr<FJsonValue>> ActualShaders;
    for(int32 I=0;I<MaterialList.Num();++I)
    {
        const auto& R=Render[I];ShaderErrors+=!(R.Original&&R.NoFallback&&R.Valid);
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("bound_material"),MaterialList[I]->GetPathName());
        Row->SetStringField(TEXT("actual_render_material"),R.Name);Row->SetBoolField(TEXT("original_proxy_present"),R.Original);
        Row->SetBoolField(TEXT("render_without_default_fallback"),R.NoFallback);Row->SetBoolField(TEXT("render_shader_valid"),R.Valid);
        ActualShaders.Add(MakeShared<FJsonValueObject>(Row));
    }
    const auto Sites=FrontlineFieldSitesV59::DescribeVisualsV8449(B);
    auto Props=MakeShared<FJsonObject>();Props->SetBoolField(TEXT("candidate"),S&&S->PropsV8449);
    Props->SetBoolField(TEXT("sandbags_v84412"),S&&S->BagsV84412);
    Props->SetBoolField(TEXT("formal_tripo_sandbags_v87"),S&&S->BagsV87);
    Props->SetBoolField(TEXT("worn_hessian_and_roof_v84413"),S&&S->BagsV84412);
    Props->SetBoolField(TEXT("bag_isolated_baseline_v8449"),S&&S->PropsV8449&&!S->BagsV84412);
    Props->SetArrayField(TEXT("bag_linear_masks_v84412"),BagMaskInventoryV84412);
    Props->SetNumberField(TEXT("bag_instances_v84412"),S?S->BagsPlacedV84412.Num():0);
    Props->SetStringField(TEXT("bag_support_contract"),S&&S->BagsV84412?TEXT("Single closed sacks. Nine foundation feet fit indexed ground/bank; upper nine feet fit actual lower-sack transforms. Max gap <=6cm."):TEXT("V8449 isolated baseline: legacy nine feet, one-contact check retained for comparison."));
    AWarfareFrontlineGround* ContactReceiver=nullptr;
    if(B)for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)if(It->Surface){ContactReceiver=*It;break;}
    Props->SetObjectField(TEXT("actual_contact_receiver"),FrontlineGroundContactV8449::Describe(ContactReceiver));
    Props->SetNumberField(TEXT("bag_ground_contact_errors"),BagSupportErrorsV8449);Props->SetArrayField(TEXT("bag_actual_contacts"),BagContacts);
    Props->SetNumberField(TEXT("root_patches"),S?S->RootPatchesV8449:0);Props->SetBoolField(TEXT("root_actual_ground_mid_shared"),RootBound);
    Props->SetStringField(TEXT("root_actual_material"),RootParent);Props->SetNumberField(TEXT("root_physical_color_missing"),S?S->RootColorsMissingV8449:0);
    Props->SetStringField(TEXT("root_color_sampling"),TEXT("Actual installed source indexed triangle; barycentric physical RGBA and geometric normal."));
    Props->SetNumberField(TEXT("root_max_nearest_color_distance_cm"),S?S->RootColorDistanceV8449:0);Props->SetNumberField(TEXT("root_edge_contact_errors"),RootEdgeErrors);
    Props->SetNumberField(TEXT("root_build_seconds"),S?S->RootSecondsV8449:0);Props->SetNumberField(TEXT("actual_render_shader_errors"),ShaderErrors);
    Props->SetArrayField(TEXT("actual_render_materials"),ActualShaders);Props->SetObjectField(TEXT("facilities"),Sites);
    auto BF=MakeShared<FJsonObject>();BF->SetNumberField(TEXT("x"),BagFocusV8449.X);BF->SetNumberField(TEXT("y"),BagFocusV8449.Y);BF->SetNumberField(TEXT("z"),BagFocusV8449.Z);
    Props->SetObjectField(TEXT("bag_camera_focus"),BF);
    const bool PropsOK=S&&BagSupportErrorsV8449==0&&RootBound&&S->RootColorsMissingV8449==0&&RootEdgeErrors==0&&ShaderErrors==0&&Sites->GetBoolField(TEXT("success"));
    Props->SetBoolField(TEXT("success"),PropsOK);J->SetObjectField(TEXT("field_props_v8449"),Props);
    J->SetBoolField(TEXT("success"),OK&&PropsOK);
    J->SetNumberField(TEXT("menin_road_or_trench_timber_obstructions"),MeninUnsafe);
    J->SetNumberField(TEXT("menin_legacy_fallen_matched"),MeninLegacyFallenMatched);
    J->SetNumberField(TEXT("menin_legacy_fallen_hidden"),S?S->MeninLegacyFallenHiddenV59:0);
    J->SetNumberField(TEXT("menin_legacy_fallen_visible"),MeninLegacyFallenVisible);
    J->SetNumberField(TEXT("menin_debris_v62_instances"),S?S->MeninDebrisV62.Num():0);
    J->SetNumberField(TEXT("menin_debris_v62_unsafe"),MeninDebrisUnsafeV62);
    J->SetNumberField(TEXT("v54_landmark_instances"),S?S->LandmarkInstancesV54:0);
    J->SetBoolField(TEXT("v54_landmark_expected"),LandmarkExpected);
    J->SetStringField(TEXT("scope"),TEXT("Installed instanced meshes, exact authored material ancestry, transformed clearance and imported V42 bank/contact bounds. HeightAt and movement are unaltered. Visual acceptance requires actual renderer captures."));
    return J;
}
