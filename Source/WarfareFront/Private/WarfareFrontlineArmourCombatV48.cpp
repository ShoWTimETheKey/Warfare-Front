#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineSpecialistsV41.h"
#include "WarfareFrontlineImpact.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include <cfloat>
bool FrontlineUseLegacyStandingBodyV8448();

namespace
{
bool LegacySupportedAssistantYieldV84411=false;
FBox InfantryContactBoxV8448(const AWarfareFrontlineSoldier* Unit,float Radius)
{
    const FBox Standing(FVector(-Radius,-Radius,0.f),FVector(Radius,Radius,145.f));
    if(FrontlineUseLegacyStandingBodyV8448()||!Unit||!Unit->Soldier||!Unit->Soldier->GetSkeletalMeshAsset()
        ||Unit->Soldier->GetBoneIndex(TEXT("Head"))==INDEX_NONE)return Standing;
    struct FCachedPose{uint32 Revision=MAX_uint32;FTransform Mesh;FVector Feet;FBox LowBox{ForceInit};bool Low=false;};
    static TMap<TWeakObjectPtr<const AWarfareFrontlineSoldier>,FCachedPose> Poses;
    if(Poses.Num()>512)for(auto It=Poses.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Cached=Poses.FindOrAdd(Unit);const uint32 Revision=Unit->Soldier->GetBoneTransformRevisionNumber();
    const FVector Feet=Unit->Feet();const FTransform Mesh=Unit->Soldier->GetComponentTransform();
    if(Cached.Revision!=Revision||!Cached.Mesh.Equals(Mesh,.001f)||!Cached.Feet.Equals(Feet,.001f))
    {
        Cached.Revision=Revision;Cached.Mesh=Mesh;Cached.Feet=Feet;Cached.Low=false;Cached.LowBox=FBox(ForceInit);
        float Top=-MAX_flt;
        for(const FName Name:{FName(TEXT("Head")),FName(TEXT("pelvis")),FName(TEXT("spine_03")),FName(TEXT("hand_l")),FName(TEXT("hand_r"))})
            if(Unit->Soldier->GetBoneIndex(Name)!=INDEX_NONE)
            {const FVector P=Unit->Soldier->GetBoneLocation(Name)-Feet;if(P.ContainsNaN())return Standing;Cached.LowBox+=P;Top=FMath::Max(Top,float(P.Z));}
        // Standing/contact transitions retain the original 145 cm envelope.
        // Only a genuinely rendered low pose gets its own physical body box;
        // its prone horizontal length stays protected as well as its height.
        if(Top>12.f&&Top<112.f&&Cached.LowBox.IsValid)
        {
            for(const FName Name:{FName(TEXT("foot_l")),FName(TEXT("foot_r")),FName(TEXT("calf_l")),FName(TEXT("calf_r")),FName(TEXT("upperarm_l")),FName(TEXT("upperarm_r"))})
                if(Unit->Soldier->GetBoneIndex(Name)!=INDEX_NONE)Cached.LowBox+=Unit->Soldier->GetBoneLocation(Name)-Feet;
            Cached.LowBox.Min.Z=FMath::Min(0.,Cached.LowBox.Min.Z);Cached.Low=true;
        }
    }
    return Cached.Low?Cached.LowBox.ExpandBy(FVector(Radius,Radius,20.f)):Standing;
}
FBox InfantryHullEnvelopeV8218(const UStaticMeshComponent* Body,const FTransform& Hull,const FBox& Person)
{
    if(!Body||!Body->GetStaticMesh())return FBox(ForceInit);
    // Expand by the actual standing body, not a screen-space radius. The
    // vertical projection keeps a man below a bridge tank out of its hull.
    if(Person.Min.Z>=0.f)
    {
        const FVector Vertical=Hull.InverseTransformVector(FVector(0,0,72.5f)),Scale=Hull.GetScale3D().GetAbs();
        const float Radius=float(Person.GetExtent().X);
        return Body->GetStaticMesh()->GetBoundingBox().ExpandBy(FVector(FMath::Abs(Vertical.X)+Radius/FMath::Max(.01,Scale.X),
            FMath::Abs(Vertical.Y)+Radius/FMath::Max(.01,Scale.Y),FMath::Abs(Vertical.Z)));
    }
    const FVector Half=Person.GetExtent();
    const FVector X=Hull.InverseTransformVector(FVector(Half.X,0,0)).GetAbs();
    const FVector Y=Hull.InverseTransformVector(FVector(0,Half.Y,0)).GetAbs();
    const FVector Z=Hull.InverseTransformVector(FVector(0,0,Half.Z)).GetAbs();
    const FVector Extent=X+Y+Z;
    return Body->GetStaticMesh()->GetBoundingBox().ExpandBy(Extent);
}
bool BoxEscapeProgressV833(const FBox& Box,const FVector& From,const FVector& To,const FVector& Scale)
{
    const FVector Step=To-From,PhysicalStep=Step*Scale,Centre=Box.GetCenter();
    // Infantry recovery is constrained to the ground plane. A rolled hull
    // turns that real horizontal move into small secondary local components;
    // requiring every one to improve blocks a valid outward adjustment.
    const int32 Escape=FMath::Abs(PhysicalStep.X)>=FMath::Abs(PhysicalStep.Y)?0:1;
    if(FMath::Abs(PhysicalStep[Escape])<.001||(From[Escape]-Centre[Escape])*Step[Escape]<-.00001)return false;
    double EscapeProgress=0,InwardDrift=0;
    for(int32 Axis=0;Axis<3;++Axis)
    {
        const double Before=FMath::Min(From[Axis]-Box.Min[Axis],Box.Max[Axis]-From[Axis]);
        const double After=FMath::Min(To[Axis]-Box.Min[Axis],Box.Max[Axis]-To[Axis]);
        const double Gain=(Before-After)*Scale[Axis];
        if(Axis==Escape)EscapeProgress=Gain;
        else InwardDrift+=FMath::Max(0.,-Gain);
    }
    // The chosen horizontal exit must dominate all incidental inward drift.
    // Deliberate movement toward its centre, net deeper movement and ordinary
    // outside-to-inside entry are never recovered through this branch.
    return EscapeProgress>InwardDrift+.001;
}

bool WholePassingStationClearV84411(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,
    const FVector& Point,float ShortYaw,float Seconds,float Distance,float Turn)
{
    if(!IsValid(U)||!U->Battle||!IsValid(Tank)||Point.ContainsNaN())return false;
    const auto* Hull=Cast<UStaticMeshComponent>(Tank->GetRootComponent());
    if(!Hull||!Hull->GetStaticMesh())return false;
    const FTransform Pose=Tank->GetActorTransform();const FVector Position=Pose.GetLocation();
    const float Yaw=float(Pose.Rotator().Yaw);const FVector Forward=FRotator(0,Yaw,0).Vector();
    const FTransform ActorLocal=Hull->GetComponentTransform().GetRelativeTransform(Pose);
    const FBox LocalBounds=Hull->GetStaticMesh()->GetBoundingBox().TransformBy(ActorLocal);
    if(Pose.ContainsNaN()||!LocalBounds.IsValid||LocalBounds.Min.ContainsNaN()||LocalBounds.Max.ContainsNaN())return false;
    const FVector Extent(FMath::Max(FMath::Abs(LocalBounds.Min.X),FMath::Abs(LocalBounds.Max.X)),
        FMath::Max(FMath::Abs(LocalBounds.Min.Y),FMath::Abs(LocalBounds.Max.Y)),
        FMath::Max(FMath::Abs(LocalBounds.Min.Z),FMath::Abs(LocalBounds.Max.Z)));
    const FBox Person=InfantryContactBoxV8448(Tank->UsesNationalGroundContactV82()?nullptr:U,34.f);const FVector PersonCentre=Person.GetCenter();
    const double PassingLength=FMath::Max(0.,FVector::DotProduct(Point+PersonCentre-Position,Forward)
        +Extent.Size()+Person.GetExtent().Size()+3.);
    if(!FMath::IsFinite(PassingLength))return false;
    const float Speed=FMath::Max(1.f,UFrontlineVehicleSubsystem::Definition(Tank->Type).SpeedCm);
    TArray<FVector,TInlineAllocator<5>> Stations;
    for(const FVector2D Margin:{FVector2D::ZeroVector,FVector2D(-3,-3),FVector2D(-3,3),FVector2D(3,-3),FVector2D(3,3)})
    {
        FVector P=Point+FVector(Margin.X,Margin.Y,0);
        if(FMath::Abs(P.Y)>=FrontlineBattlefield::HalfFrontage-35.f)return false;
        P.Z=FMath::Max(Point.Z,double(FrontlineInfantryV45::SurfaceHeight(U->Battle,P)));
        Stations.Add(P+PersonCentre);
    }
    const auto PoseClear=[&](const FTransform& ActorPose)
    {
        const FTransform PassingHull=ActorLocal*ActorPose;
        const FBox Envelope=InfantryHullEnvelopeV8218(Hull,PassingHull,Person);
        if(!Envelope.IsValid)return false;
        for(const FVector& Station:Stations)
            if(Envelope.IsInsideOrOn(PassingHull.InverseTransformPosition(Station)))return false;
        return true;
    };
    if(!PoseClear(Pose))return false;
    const int32 ShortSamples=FMath::Max(1,FMath::CeilToInt(Distance/20.f));
    for(float Offset:{0.f,-Turn,Turn})
    {
        const float NextYaw=ShortYaw+Offset;const FVector NextForward=FRotator(0,NextYaw,0).Vector();
        if(!PoseClear(Tank->GroundPoseForContactV8218(Position,NextYaw,Seconds)))return false;
        for(int32 I=1;I<=ShortSamples;++I)
            if(!PoseClear(Tank->GroundPoseForContactV8218(Position+NextForward*(Distance*I/ShortSamples),NextYaw,Seconds*I/ShortSamples)))return false;
        if(Tank->IsRailConvoyV32())break;
    }
    // Initial, cached and reached stations share one whole-pass eligibility.
    // Reuse only support predictions for this exact actor pose/encounter/frame;
    // each caller still tests its own actual body and five arrival positions.
    // Fixed 20 cm samples can be shared by nearby candidate endpoints, with
    // the exact final point checked separately; no sample gap exceeds 20 cm.
    struct FPassingPoses
    {
        uint64 Frame=MAX_uint64;FGuid Encounter;FTransform Actor;float Speed=0;
        EFrontlineVehicle Type=EFrontlineVehicle::MarkIVMale;bool Rail=false;
        TArray<FTransform,TInlineAllocator<64>> Poses;
    };
    static TMap<TWeakObjectPtr<const AFrontlineTank>,FPassingPoses> Passing;
    if(Passing.Num()>128)for(auto It=Passing.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    auto& Cached=Passing.FindOrAdd(Tank);const FGuid Encounter=U->Battle->GetEncounterId();
    if(Cached.Frame!=GFrameCounter||Cached.Encounter!=Encounter||Cached.Speed!=Speed||Cached.Type!=Tank->Type
        ||Cached.Rail!=Tank->IsRailConvoyV32()||!Cached.Actor.Equals(Pose,0.f))
    {Cached.Frame=GFrameCounter;Cached.Encounter=Encounter;Cached.Actor=Pose;Cached.Speed=Speed;
        Cached.Type=Tank->Type;Cached.Rail=Tank->IsRailConvoyV32();Cached.Poses.Reset();}
    const int32 WholeSamples=FMath::FloorToInt(PassingLength/20.);
    for(int32 I=1;I<=WholeSamples;++I)
    {
        const float Travel=I*20.f;
        if(!Cached.Poses.IsValidIndex(I-1))Cached.Poses.Add(Tank->GroundPoseForContactV8218(Position+Forward*Travel,Yaw,Travel/Speed));
        if(!PoseClear(Cached.Poses[I-1]))return false;
    }
    return PoseClear(Tank->GroundPoseForContactV8218(Position+Forward*PassingLength,Yaw,float(PassingLength/Speed)));
}
}

void FrontlineSetLegacyArmourYieldV84411(bool Legacy)
{
    // Internal paired witness only. Normal play never acquires this switch.
    LegacySupportedAssistantYieldV84411=Legacy&&FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
}
bool FrontlineUseLegacyArmourYieldV84411(){return LegacySupportedAssistantYieldV84411;}

bool FrontlineArmourYieldGoalBodyClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Destination)
{
    // Passing stations are physical destinations, not unreserved screen-space
    // offsets. The same body field used by each actual step must keep the
    // endpoint clear; a stationary man from the next section can occupy it.
    constexpr double Clearance=56.5;
    return FrontlineMobilityV41::VisitSweptBodies(U,Destination,Destination,Clearance,
        [&](const AWarfareFrontlineSoldier* Other)
        {
            return Other==U||!IsValid(Other)||!Other->IsAlive()||Other->IsHidden()
                ||FVector::DistSquared2D(Destination,Other->Feet())>=FMath::Square(Clearance);
        });
}

bool FrontlineArmourYieldDirectPathClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Candidate,
    const AFrontlineTank* PreviewTank=nullptr,const FTransform* PreviewPose=nullptr,FString* Failure=nullptr)
{
    if(Failure)Failure->Reset();
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden()||Candidate.ContainsNaN()
        ||(PreviewTank&&(!PreviewPose||PreviewPose->ContainsNaN())))return false;
    const FVector From=U->Feet(),Leg=Candidate-From;const double LengthSquared=Leg.SizeSquared2D();
    if(LengthSquared<.001)return true;
    if(!FrontlineMobilityV41::VisitSweptBodies(U,From,Candidate,56.,[&](const AWarfareFrontlineSoldier* Other)
    {
        if(Other==U||!IsValid(Other)||!Other->IsAlive()||Other->IsHidden())return true;
        const FVector BodyFeet=Other->Feet();
        const double T=FMath::Clamp(((BodyFeet.X-From.X)*Leg.X+(BodyFeet.Y-From.Y)*Leg.Y)/LengthSquared,0.,1.);
        return FVector::DistSquared2D(From+Leg*T,BodyFeet)>=FMath::Square(56.);
    })){if(Failure)*Failure=TEXT("neighbour_body");return false;}
    // This shared leg is the existing centimetre bank/body/hull check. An
    // empty shoulder is not evidence that the held man can approach it.
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld());
    FVector Previous=From;const int32 Steps=FMath::CeilToInt(FMath::Sqrt(LengthSquared));
    for(int32 I=1;I<=Steps;++I)
    {
        FVector Next=FMath::Lerp(From,Candidate,double(I)/Steps);
        if(!FrontlineInfantryV45::AllowsTrenchStep(U,Previous,Next))
        {if(Failure)*Failure=FString::Printf(TEXT("bank:%s->%s"),*Previous.ToString(),*Next.ToString());return false;}
        const double Floor=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);
        Next.Z=FMath::Max(Previous.Z,Floor);
        if(Fleet)for(const AFrontlineTank* Other:Fleet->GetVehicles())
            if(IsValid(Other)&&Other->IsOperational()&&!Other->IsHidden()
                &&FVector::DistSquared2D(From,Other->GetActorLocation())<FMath::Square(3600.f)
                &&Other->InfantrySweepClearanceV8218(Previous,Next,34.f,U,Other==PreviewTank?PreviewPose:nullptr)<1.f)
            {if(Failure)*Failure=FString::Printf(TEXT("hull:%d:%s->%s"),Other->Serial,*Previous.ToString(),*Next.ToString());return false;}
        Next.Z=Floor;Previous=Next;
    }
    return true;
}

bool AFrontlineTank::BlocksInfantryAtV8218(const FVector& Feet,const FVector& Position,float Yaw,float Radius,float MotionSeconds,const AWarfareFrontlineSoldier* Unit,const FTransform* FromPose,const FTransform* SupportedPose) const
{
    if(!IsOperational()||IsHidden()||!Body||!Body->GetStaticMesh())return false;
    // Candidate contact uses the same real track/wheel support as placement;
    // the centre may lie in a trench while the hull bridges above its floor.
    // Occupancy at the accepted pose uses its real component orientation.
    // A prospective move uses precisely the damped support pose that the
    // movement commit will render. An instantaneous bank fit can otherwise
    // clear a torso which the still-damping visible hull intersects.
    const bool CurrentPose=!FromPose&&MotionSeconds<=0.f&&Position.Equals(GetActorLocation(),.001f)
        &&FMath::Abs(FMath::FindDeltaAngleDegrees(float(GetActorRotation().Yaw),Yaw))<=.001f;
    const FTransform Predicted=SupportedPose?*SupportedPose:CurrentPose?GetActorTransform():GroundPoseForContactV8218(Position,Yaw,MotionSeconds,FromPose);
    // A root component's RelativeTransform is already in world space when
    // it has no attachment parent. Derive its actor-local transform from the
    // component world pose so prediction applies the actor pose exactly once.
    const FTransform ActorLocal=Body->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    const FTransform Hull=ActorLocal*Predicted;
    const FBox Person=InfantryContactBoxV8448(bNationalGermanGroundV82?nullptr:Unit,Radius);
    const FBox Box=InfantryHullEnvelopeV8218(Body,Hull,Person);
    return Box.IsValid&&Box.IsInsideOrOn(Hull.InverseTransformPosition(Feet+Person.GetCenter()));
}

bool AFrontlineTank::IsInfantryYieldStationSafeV86(const AWarfareFrontlineSoldier* U,const FVector& Destination) const
{
    if(!U||Destination.ContainsNaN()||!IsOperational()||IsHidden())return false;
    // A held man cannot reach a bank-top station by the lateral floor route.
    // Endpoint intrusion alone is misleading: at the bank top, ground height
    // has risen too and the end may pass while every intervening step rejects
    // the wall. Preserve his actual height above the same authored floor,
    // with the existing 8 cm bank allowance, at the destination's own Y.
    if(Battle&&!WarfareFRRUCampaignV82::Active(Battle))
    {
        const auto* Held=Battle->FindSquad(U->SquadId);const FVector Feet=U->Feet();
        if(Held&&!Held->bAdvancing&&Battle->Trenches.IsValidIndex(Held->Trench)
            &&FMath::Abs(float(Feet.X)-FrontlineFieldV32::TrenchCentreX(Held->Trench,Feet.Y))<=140.f)
        {
            FVector OldFloor=Feet,NewFloor=Destination;
            OldFloor.X=FrontlineFieldV32::TrenchCentreX(Held->Trench,OldFloor.Y);
            NewFloor.X=FrontlineFieldV32::TrenchCentreX(Held->Trench,NewFloor.Y);
            const double ActualHeight=Feet.Z-FrontlineInfantryV45::SurfaceHeight(Battle,OldFloor);
            const double GoalHeight=Destination.Z-FrontlineInfantryV45::SurfaceHeight(Battle,NewFloor);
            if(GoalHeight>ActualHeight+8.)return false;
        }
    }
    const FVector Position=GetActorLocation();
    if(BlocksInfantryAtV8218(Destination,Position,float(GetActorRotation().Yaw),34.f,0.f,U))return false;
    return LegacySupportedAssistantYieldV84411||(Battle&&WarfareFRRUCampaignV82::Active(Battle))||FrontlineArmourYieldGoalBodyClearV84411(U,Destination);
}

bool AFrontlineTank::IsInfantryYieldNextStepClearV86(const AWarfareFrontlineSoldier* U,const FVector& Destination) const
{
    if(!U||Destination.ContainsNaN()||!IsOperational()||IsHidden())return false;
    const float Seconds=MobilityStepV839>0.f?FMath::Clamp(MobilityStepV839,1.f/120.f,.10f):1.f/30.f;
    const float Distance=MobilityRequestedV839>0.f?MobilityRequestedV839:
        UFrontlineVehicleSubsystem::Definition(Type).SpeedCm*Seconds;
    const FVector Next=GetActorLocation()+FRotator(0,HeadingYawV37,0).Vector()*Distance;
    if(BlocksInfantryAtV8218(Destination,Next,HeadingYawV37,34.f,Seconds,U))return false;
    // A precise station still has 3/10 cm arrival hysteresis. Near it, use
    // the rendered man's real feet as well: an empty nominal destination
    // must not certify a still-blocking body a centimetre away from it.
    return FVector::DistSquared2D(U->Feet(),Destination)>FMath::Square(10.f)
        ||!BlocksInfantryAtV8218(U->Feet(),Next,HeadingYawV37,34.f,Seconds,U);
}

bool AFrontlineTank::IsInfantryYieldGoalClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Destination) const
{
    if(!IsInfantryYieldStationSafeV86(U,Destination))return false;
    const FVector Position=GetActorLocation();
    // A man can reach an empty shoulder while the next few centimetres of
    // pitched/turning hull still sweep across it. Use the exact short forward
    // arcs and damped ground poses available to AdvanceV37, not a larger fake
    // body or a horizon that forces him to abandon his trench unnecessarily.
    const float Seconds=MobilityStepV839>0.f?FMath::Clamp(MobilityStepV839,1.f/120.f,.10f):1.f/30.f;
    const float Distance=MobilityRequestedV839>0.f?MobilityRequestedV839:
        UFrontlineVehicleSubsystem::Definition(Type).SpeedCm*Seconds;
    const float Turn=bRailConvoyV32?0.f:SteeringRateV8311(Distance/Seconds,false)*Seconds;
    if(!LegacySupportedAssistantYieldV84411&&Battle&&!WarfareFRRUCampaignV82::Active(Battle))
        return WholePassingStationClearV84411(U,this,Destination,HeadingYawV37,Seconds,Distance,Turn);
    const int32 Samples=FMath::Max(1,FMath::CeilToInt(Distance/20.f));
    for(float Offset:{0.f,-Turn,Turn})
    {
        const float Yaw=HeadingYawV37+Offset;const FVector Forward=FRotator(0,Yaw,0).Vector();
        if(BlocksInfantryAtV8218(Destination,Position,Yaw,34.f,Seconds,U))return false;
        for(int32 I=1;I<=Samples;++I)
            if(BlocksInfantryAtV8218(Destination,Position+Forward*(Distance*I/Samples),Yaw,34.f,Seconds*I/Samples,U))return false;
        if(bRailConvoyV32)break; // its actual fixed rail heading never turns
    }
    return true;
}

bool FrontlineArmourYieldReachableStationClearV84411(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const FVector& Point)
{
    // The floor planner owns reachability through its checked parent chain.
    // This predicate only certifies that the final waiting station actually
    // releases this same passing hull, including the precise arrival margin.
    // An empty but disconnected shoulder must never become a movement order.
    if(!IsValid(U)||!IsValid(Tank)||!U->Battle||!U->IsAlive()||U->IsHidden()
        ||FrontlineUseLegacyArmourYieldV84411()||WarfareFRRUCampaignV82::Active(U->Battle)
        ||Tank->GetWorld()!=U->GetWorld()||Tank->Team!=U->Team||!Tank->IsOperational()
        ||Tank->IsHidden()||Tank->IsImmobilized()||Tank->IsHeldForFieldRepairV63()
        ||Point.ContainsNaN()||FVector::DistSquared2D(U->Feet(),Point)>FMath::Square(283.5f))return false;
    const auto* Held=U->Battle->FindSquad(U->SquadId);
    if(!Held||Held->bAdvancing||!U->Battle->Trenches.IsValidIndex(Held->Trench))return false;
    // Initial/cache qualification owns the same five-corner whole-pass hull
    // proof. This planner predicate adds only its local radius, held-floor and
    // actual neighbour constraints; it cannot accept a weaker reached station.
    if(!Tank->IsInfantryYieldGoalClearV84411(U,Point))return false;
    const FVector Feet=U->Feet();FVector OldFloor=Feet;
    OldFloor.X=FrontlineFieldV32::TrenchCentreX(Held->Trench,Feet.Y);
    const bool OnHeldFloor=FMath::Abs(Feet.X-OldFloor.X)<=140.;
    const double ActualHeight=Feet.Z-FrontlineInfantryV45::SurfaceHeight(U->Battle,OldFloor);
    for(const FVector2D Margin:{FVector2D::ZeroVector,FVector2D(-3,-3),FVector2D(-3,3),FVector2D(3,-3),FVector2D(3,3)})
    {
        FVector P=Point+FVector(Margin.X,Margin.Y,0);
        if(FMath::Abs(P.Y)>=FrontlineBattlefield::HalfFrontage-35.f)return false;
        P.Z=FMath::Max(Point.Z,double(FrontlineInfantryV45::SurfaceHeight(U->Battle,P)));
        FVector NewFloor=P;NewFloor.X=FrontlineFieldV32::TrenchCentreX(Held->Trench,P.Y);
        if((OnHeldFloor&&P.Z-FrontlineInfantryV45::SurfaceHeight(U->Battle,NewFloor)>ActualHeight+8.)
            ||!FrontlineArmourYieldGoalBodyClearV84411(U,P))return false;
    }
    return true;
}

bool AFrontlineTank::RelievesExistingInfantryContactV842(const FVector& Feet,const FVector& Position,float Yaw,float Radius,float MotionSeconds,const AWarfareFrontlineSoldier* Unit,const FTransform* FromPose,const FTransform* SupportedPose) const
{
    const FTransform& Previous=FromPose?*FromPose:GetActorTransform();
    if(!Body||!Body->GetStaticMesh()||Feet.ContainsNaN()||Position.ContainsNaN()
        ||FVector::Dist2D(Position,Previous.GetLocation())>80.f
        ||FMath::Abs(FMath::FindDeltaAngleDegrees(Previous.Rotator().Yaw,Yaw))>5.f)return false;
    const FTransform ActorLocal=Body->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    const FTransform OldHull=FromPose?ActorLocal*Previous:Body->GetComponentTransform();
    const FTransform NewHull=ActorLocal*(SupportedPose?*SupportedPose:GroundPoseForContactV8218(Position,Yaw,MotionSeconds,FromPose));
    const FBox Person=InfantryContactBoxV8448(bNationalGermanGroundV82?nullptr:Unit,Radius);
    const FBox OldBox=InfantryHullEnvelopeV8218(Body,OldHull,Person),NewBox=InfantryHullEnvelopeV8218(Body,NewHull,Person);
    const FVector Centre=Feet+Person.GetCenter();
    const FVector Before=OldHull.InverseTransformPosition(Centre),After=NewHull.InverseTransformPosition(Centre);
    if(!OldBox.IsValid||!NewBox.IsValid||!OldBox.IsInsideOrOn(Before))return false;
    if(!NewBox.IsInsideOrOn(After))return true;
    // Reciprocity with the infantry's outward-only recovery: an overlap
    // caused by a moving bay/ground fit may escape, never deepen or re-enter.
    // A trench lip often leaves the floor as the closest box face. Its
    // unchanged vertical depth must not veto a real horizontal exit. Compare
    // the same old envelope first so a changing support pose cannot fabricate
    // outward progress, then keep every new face's inward drift bounded by
    // that actual horizontal escape.
    if(!BoxEscapeProgressV833(OldBox,Before,After,OldHull.GetScale3D().GetAbs()))return false;
    const FVector Scale=NewHull.GetScale3D().GetAbs(),Step=After-Before;
    const int32 ExitAxis=FMath::Abs(Step.X*Scale.X)>=FMath::Abs(Step.Y*Scale.Y)?0:1;
    double ExitProgress=0,InwardDrift=0;
    for(int32 Axis=0;Axis<3;++Axis)
    {
        const double OldDepth=FMath::Min(Before[Axis]-OldBox.Min[Axis],OldBox.Max[Axis]-Before[Axis]);
        const double NewDepth=FMath::Min(After[Axis]-NewBox.Min[Axis],NewBox.Max[Axis]-After[Axis]);
        const double Gain=(OldDepth-NewDepth)*Scale[Axis];
        if(Axis==ExitAxis)ExitProgress=Gain;else InwardDrift+=FMath::Max(0.,-Gain);
    }
    return ExitProgress>InwardDrift+.10;
}

float AFrontlineTank::InfantrySweepClearanceV8218(const FVector& From,const FVector& To,float Radius,const AWarfareFrontlineSoldier* Unit,const FTransform* PreviewPose) const
{
    if(!IsOperational()||IsHidden()||!Body||!Body->GetStaticMesh())return 1.f;
    if(PreviewPose&&PreviewPose->ContainsNaN())return 0.f;
    const FBox Person=InfantryContactBoxV8448(bNationalGermanGroundV82?nullptr:Unit,Radius);
    const FTransform Hull=PreviewPose?Body->GetComponentTransform().GetRelativeTransform(GetActorTransform())*(*PreviewPose):Body->GetComponentTransform();
    const FBox Box=InfantryHullEnvelopeV8218(Body,Hull,Person);
    if(!Box.IsValid)return 1.f;
    const FVector A=Hull.InverseTransformPosition(From+Person.GetCenter());
    const FVector Z=Hull.InverseTransformPosition(To+Person.GetCenter()),D=Z-A;
    if(Box.IsInsideOrOn(A))
    {
        // An already displaced body may recover toward a valid hull face.
        // Comparing only the minimum penetration froze sideways yielding
        // whenever the floor/roof or nose was already the closest face.
        return BoxEscapeProgressV833(Box,A,Z,Hull.GetScale3D().GetAbs())?1.f:0.f;
    }
    double Enter=0,Exit=1;
    for(int32 I=0;I<3;++I)
    {
        if(FMath::Abs(D[I])<1.e-8){if(A[I]<Box.Min[I]||A[I]>Box.Max[I])return 1.f;continue;}
        double Lo=(Box.Min[I]-A[I])/D[I],Hi=(Box.Max[I]-A[I])/D[I];if(Lo>Hi)Swap(Lo,Hi);
        Enter=FMath::Max(Enter,Lo);Exit=FMath::Min(Exit,Hi);if(Enter>Exit)return 1.f;
    }
    return Enter>1||Exit<0?1.f:float(FMath::Clamp(Enter-.005,0.,1.));
}

bool AFrontlineTank::InfantryYieldPointV8218(const AWarfareFrontlineSoldier* U,FVector& Destination) const
{
    if(!U||!U->IsAlive()||!Battle||U->Team!=Team||!IsOperational()||IsImmobilized()
        ||IsHeldForFieldRepairV63()||IsHidden()||!Body||!Body->GetStaticMesh()
        ||(bRailConvoyV32&&WarfareFRRUCampaignV82::Active(Battle)))return false;
    // Do not dismantle a valid planted main gun for a predicted approach.
    // A supported rifle-bearing assistant is not that immovable gun: blocking
    // his side-step until the hull already intersects him deadlocked both
    // actors, since the vehicle correctly refuses that first intersection.
    // A body already inside the live hull must still be allowed a real exit;
    // the same vertical envelope excludes men safely beneath it in a trench.
    if(auto* Gun=FrontlineMachinegunsV30::Get(U);Gun&&Gun->IsSupported()
        &&(LegacySupportedAssistantYieldV84411||WarfareFRRUCampaignV82::Active(Battle)
            ||U->IsMachineGunner()||FrontlineSpecialistsV41::IsAntiTankGunner(U))
        &&(WarfareFRRUCampaignV82::Active(Battle)||!BlocksInfantryAtV8218(U->Feet(),GetActorLocation(),HeadingYawV37,50.f,0.f,U)))return false;
    const FTransform Hull=Body->GetComponentTransform();const FBox Bounds=Body->GetStaticMesh()->GetBoundingBox();
    const FVector P=Hull.InverseTransformPosition(U->Feet());
    const bool RecoveryWitnessV86=HasInfantryRecoveryContactV86(U);
    if(!RecoveryWitnessV86&&(P.X<Bounds.Min.X-70.f||P.X>Bounds.Max.X+390.f||FMath::Abs(P.Y)>FMath::Max(FMath::Abs(Bounds.Min.Y),FMath::Abs(Bounds.Max.Y))+50.f))return false;
    // A low trench occupant beneath the hull does not need to abandon cover.
    const FVector Ahead=GetActorLocation()+FRotator(0,HeadingYawV37,0).Vector()*FMath::Clamp(float(P.X),0.f,390.f);
    // A bank's next supported pose can sweep the rear across a man whose
    // current position is safely behind the centre. The clamped nose-only
    // prediction is exactly the current pose for him and cannot witness that
    // real contact. Include the same host step used by the vehicle commit.
    const float ContactSeconds=MobilityStepV839>0.f?FMath::Clamp(MobilityStepV839,1.f/120.f,.10f):1.f/30.f;
    const float ContactDistance=MobilityRequestedV839>0.f?MobilityRequestedV839:
        UFrontlineVehicleSubsystem::Definition(Type).SpeedCm*ContactSeconds;
    const FVector SupportedNext=GetActorLocation()+FRotator(0,HeadingYawV37,0).Vector()*ContactDistance;
    const bool SupportedNextWitness=!LegacySupportedAssistantYieldV84411&&!WarfareFRRUCampaignV82::Active(Battle)
        &&BlocksInfantryAtV8218(U->Feet(),SupportedNext,HeadingYawV37,34.f,ContactSeconds,U);
    if(!BlocksInfantryAtV8218(U->Feet(),Ahead,HeadingYawV37,50.f,0.f,U)
        &&!SupportedNextWitness&&!RecoveryWitnessV86)return false;
    // Match the same standing body centre used by the hull sweep. Choosing
    // from feet alone on a rolled hull can send the torso across its centre
    // instead of outward, creating an impossible persistent passing lease.
    const float BodyY=Hull.InverseTransformPosition(U->Feet()+InfantryContactBoxV8448(bNationalGermanGroundV82?nullptr:U,50.f).GetCenter()).Y;
    const float Side=FMath::Abs(BodyY)>.125f?(BodyY>0?1.f:-1.f):((U->MemberIndex+Serial)%2?1.f:-1.f);
    const bool PhysicalPassingV84411=!LegacySupportedAssistantYieldV84411&&!WarfareFRRUCampaignV82::Active(Battle);
    const auto* HeldSquad=Battle->FindSquad(U->SquadId);
    const bool OnHeldFloor=PhysicalPassingV84411&&HeldSquad&&!HeldSquad->bAdvancing
        &&Battle->Trenches.IsValidIndex(HeldSquad->Trench)
        &&FMath::Abs(float(U->Feet().X)-FrontlineFieldV32::TrenchCentreX(HeldSquad->Trench,U->Feet().Y))<=140.f;
    const float HeldDepth=OnHeldFloor?float(U->Feet().X)-FrontlineFieldV32::TrenchCentreX(HeldSquad->Trench,U->Feet().Y):0.f;
    const auto MakeCandidate=[&](float Sign,float ShoulderOffset,float FloorOffset)
    {
        FVector Q=P;Q.Y=Sign>0?Bounds.Max.Y+88.f+ShoulderOffset:Bounds.Min.Y-88.f-ShoulderOffset;
        if(!OnHeldFloor)Q.X+=FloorOffset;
        FVector Candidate=Hull.TransformPosition(Q);
        // A held man's trench floor follows the authored bay at his new Y.
        // Holding the hull-local X across a curved traverse put the requested
        // side station through that bay's wall. The ordinary step gate then
        // correctly refused it, leaving the hull and the stationary man to wait.
        // Preserve the man's depth in the floor corridor; the same bank and
        // body sweeps still decide whether this physical side route is usable.
        if(OnHeldFloor)Candidate.X=FrontlineFieldV32::TrenchCentreX(HeldSquad->Trench,Candidate.Y)+HeldDepth+FloorOffset;
        Candidate.Z=FrontlineInfantryV45::SurfaceHeight(Battle,Candidate);return Candidate;
    };
    const auto CandidateClear=[&](const FVector& Candidate)
    {
        return FMath::Abs(Candidate.Y)<FrontlineBattlefield::HalfFrontage-35.f
            &&(!PhysicalPassingV84411||(IsInfantryYieldGoalClearV84411(U,Candidate)
                &&FrontlineArmourYieldGoalBodyClearV84411(U,Candidate)&&IsInfantryRecoveryStationClearV86(U,Candidate)))
            &&FrontlineInfantryV45::AllowsTrenchStep(U,U->Feet(),Candidate);
    };
    if(!PhysicalPassingV84411)
    {
        // Exact frozen legacy/FRRU route for paired evidence and paused content.
        Destination=MakeCandidate(Side,0.f,0.f);if(CandidateClear(Destination))return true;
        if(bRailConvoyV32&&FMath::Abs(Destination.Y)>=FrontlineBattlefield::HalfFrontage-35.f)
        {Destination=MakeCandidate(-Side,0.f,0.f);return CandidateClear(Destination);}
        return false;
    }
    const auto TrySide=[&](float Sign,bool Direct)
    {
        float BestDistance=MAX_flt;FVector Best=FVector::ZeroVector;
        // Small, deterministic samples on the actual curved floor. Keep the
        // original shoulder margin and reject occupied or walled endpoints.
        // No movement is committed here: the existing step resolver still
        // owns every swept body, bank, hull, heading and animation transition.
        for(float ShoulderOffset:{0.f,28.f,56.f,84.f,112.f,168.f})
            for(float FloorOffset:{0.f,28.f,-28.f,56.f,-56.f,84.f,-84.f})
            {
                const FVector Candidate=MakeCandidate(Sign,ShoulderOffset,FloorOffset);
                const float Distance=float(FVector::DistSquared2D(Candidate,U->Feet()));
                if(Distance>=BestDistance||!CandidateClear(Candidate)
                    ||(Direct&&!FrontlineArmourYieldDirectPathClearV84411(U,Candidate)))continue;
                BestDistance=Distance;Best=Candidate;
            }
        if(BestDistance==MAX_flt)return false;
        Destination=Best;return true;
    };
    // The nearer shoulder may be a traverse wall. Choose the other legal
    // shoulder once when acquiring the lease; the infantry resolver still
    // checks every actual bank/body/hull leg, and its cached side stays fixed
    // until the passing hull has cleared rather than alternating each frame.
    // Prefer an actually clear physical leg over an empty station beyond a packed
    // rank. If neither shoulder has one, the existing checked local detour can
    // still approach a clear endpoint. Each acquired lease keeps its side.
    if(TrySide(Side,true)||TrySide(-Side,true))return true;
    return TrySide(Side,false)||TrySide(-Side,false);
}

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunArmourCombatChecksV48()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    auto Check=[&](const FString& Name,bool Passed)
    {auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    ON_SCOPE_EXIT
    {
        Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("duels"),Cases);Report->SetBoolField(TEXT("success"),Good);
        Report->SetStringField(TEXT("scope"),TEXT("Actual spawned vehicle meshes on current authored terrain, Simulate, collision sweeps, cannon flight and hull damage; no alternate combat model. Fixed 0.1 s steps, seeded production RNG. This fixture is not a claim about an unobserved player battle."));
    };
    auto* B=Battle();FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV48Review"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("v48_armour_requires_isolated_volatile_review"),false);return Report;}

    const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const auto SavedSmoke=B->SmokeZones;const auto SavedTracers=B->Tracers;
    const auto SavedPhase=B->Phase;const auto SavedEncounter=LastEncounter;const FString SavedMessage=B->LastMessage;
    const int32 SavedOutro=B->BattleOutroWinner,SavedShots=B->ShotsFired,SavedSerial=NextSerial;
    const int32 SavedCount[2]={DeploymentCountV37[0],DeploymentCountV37[1]};
    const float SavedMorale[2]={B->Morale[0],B->Morale[1]};
    struct FActorState { AActor* Actor;bool Hidden,Collision; };
    TArray<FActorState> ActorStates;
    auto Hide=[&](AActor* A){if(IsValid(A)){ActorStates.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
    for(AFrontlineTank* V:SavedVehicles)Hide(V);
    for(AWarfareFrontlineSoldier* U:SavedUnits)Hide(U);
    TSet<AActor*> ExistingImpacts;for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)ExistingImpacts.Add(*It);
    Vehicles.Empty();B->Units.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
    LastEncounter=B->GetEncounterId();NextSerial=4800;DeploymentCountV37[0]=DeploymentCountV37[1]=0;
    ON_SCOPE_EXIT
    {
        for(AFrontlineTank* V:Vehicles)if(IsValid(V))V->Destroy();Vehicles=SavedVehicles;
        for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)if(!ExistingImpacts.Contains(*It))It->Destroy();
        for(const auto& S:ActorStates)if(IsValid(S.Actor)){S.Actor->SetActorHiddenInGame(S.Hidden);S.Actor->SetActorEnableCollision(S.Collision);}
        B->Units=SavedUnits;B->SmokeZones=SavedSmoke;B->Tracers=SavedTracers;B->Phase=SavedPhase;B->BattleOutroWinner=SavedOutro;
        B->LastMessage=SavedMessage;B->ShotsFired=SavedShots;LastEncounter=SavedEncounter;NextSerial=SavedSerial;
        for(int32 T=0;T<2;++T){B->Morale[T]=SavedMorale[T];DeploymentCountV37[T]=SavedCount[T];}
    };
    auto Clear=[&](){for(AFrontlineTank* V:Vehicles)if(IsValid(V))V->Destroy();Vehicles.Empty();B->SmokeZones.Empty();};
    auto Place=[&](AFrontlineTank* V,float X,float Y)
    {
        V->HeadingYawV37=B->GetTeamDirection(V->Team)>0?0.f:180.f;
        V->PreferredLaneV37=V->DesiredLaneV37=Y;V->bEntered=true;
        V->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)));
        V->FitGroundV35(V->GetActorLocation(),0);
    };
    auto Duel=[&](int32 FirstTeam,bool AlreadyPassed)
    {
        Clear();auto* A=Spawn(EFrontlineVehicle::A7V,FirstTeam,true);auto* M=Spawn(EFrontlineVehicle::MarkIVMale,1-FirstTeam,true);
        const FString Name=FString::Printf(TEXT("v48_team%d_%s"),FirstTeam,AlreadyPassed?TEXT("reacquires_passed_armour"):TEXT("opposing_lane_duel"));
        if(!A||!M){Check(Name+TEXT("_assets"),false);return;}
        const float Dir=B->GetTeamDirection(A->Team),X=AlreadyPassed?700.f:-1200.f,Y=AlreadyPassed?700.f:300.f;
        Place(A,X*Dir,-Y);Place(M,-X*Dir,Y);
        const float ABefore=A->HullIntegrity,MBefore=M->HullIntegrity;float FirstA=-1,FirstM=-1;bool ClearHulls=true,Bounded=true,Crossed=false;
        const double InitialSide=(M->GetActorLocation().X-A->GetActorLocation().X)*Dir;
        for(int32 Step=0;Step<90;++Step)
        {
            const FVector AP=A->GetActorLocation(),MP=M->GetActorLocation();A->Simulate(.1f);M->Simulate(.1f);
            if(A->CannonShots>0&&FirstA<0)FirstA=(Step+1)*.1f;if(M->CannonShots>0&&FirstM<0)FirstM=(Step+1)*.1f;
            ClearHulls&=A->CanOccupyV48(A->GetActorLocation(),A->HeadingYawV37)&&M->CanOccupyV48(M->GetActorLocation(),M->HeadingYawV37);
            Bounded&=FVector::Dist2D(AP,A->GetActorLocation())<=14.501&&FVector::Dist2D(MP,M->GetActorLocation())<=13.001;
            if(!AlreadyPassed&&(M->GetActorLocation().X-A->GetActorLocation().X)*Dir*InitialSide<0)Crossed=true;
        }
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetNumberField(TEXT("a7v_first_shot_seconds"),FirstA);J->SetNumberField(TEXT("mark_iv_first_shot_seconds"),FirstM);
        J->SetNumberField(TEXT("a7v_hull_loss"),ABefore-A->HullIntegrity);J->SetNumberField(TEXT("mark_iv_hull_loss"),MBefore-M->HullIntegrity);
        J->SetNumberField(TEXT("a7v_cannon_shots"),A->CannonShots);J->SetNumberField(TEXT("mark_iv_cannon_shots"),M->CannonShots);J->SetBoolField(TEXT("crossed_before_engagement"),Crossed);Cases.Add(MakeShared<FJsonValueObject>(J));
        Check(Name+TEXT("_both_fire_and_real_flight_damages_both_hulls"),FirstA>0&&FirstA<=8&&FirstM>0&&FirstM<=8&&A->HullIntegrity<ABefore&&M->HullIntegrity<MBefore);
        Check(Name+TEXT("_bounded_motion_no_live_hull_overlap_or_uncontested_crossing"),ClearHulls&&Bounded&&!Crossed);
    };
    for(int32 Team=0;Team<2;++Team){Duel(Team,false);Duel(Team,true);}

    Clear();auto* A=Spawn(EFrontlineVehicle::A7V,0,true);auto* M=Spawn(EFrontlineVehicle::MarkIVMale,1,true);
    if(!A||!M){Check(TEXT("v48_control_pair_assets"),false);return Report;}
    const float Dir=B->GetTeamDirection(A->Team);Place(A,-1000*Dir,-300);Place(M,1000*Dir,300);
    A->CannonClock[0]=0;for(int32 I=0;I<20&&A->CannonShots==0;++I)A->Simulate(.1f);
    const bool SawAndFired=A->CannonShots>0;for(auto& S:A->Shells)if(S.Visual.IsValid())S.Visual->DestroyComponent();A->Shells.Empty();
    const FVector HeldAt=A->GetActorLocation();const int32 BeforeSmoke=A->CannonShots;
    FFrontlineSmokeZone Zone;Zone.Centre=(A->AimPoint()+M->AimPoint())*.5;Zone.Radius=1500;Zone.StartedAt=B->Elapsed;Zone.ExpiresAt=B->Elapsed+20;B->SmokeZones.Add(Zone);
    A->CannonClock[0]=0;for(int32 I=0;I<8;++I)A->Simulate(.1f);
    Check(TEXT("v48_observed_contact_briefly_holds_without_firing_or_tracking_through_smoke"),SawAndFired&&A->CannonShots==BeforeSmoke&&A->GetActorLocation().Equals(HeldAt,.001)&&A->ArmourContactAgeV48>=.79f);
    B->SmokeZones.Empty();A->ArmourContactV48.Reset();A->ArmourContactAgeV48=2;A->CannonClock[0]=0;
    M->AddActorWorldOffset(FVector(0,0,1800));const int32 BeforeHigh=A->CannonShots;
    for(int32 I=0;I<25;++I)A->Simulate(.1f);
    Check(TEXT("v48_cannon_cannot_fire_at_target_beyond_real_elevation_limit"),A->CannonShots==BeforeHigh&&A->Cannons[0]->GetRelativeRotation().Pitch<=18.001);
    Place(M,1000*Dir,300);for(int32 I=0;I<25&&A->CannonShots==BeforeHigh;++I)A->Simulate(.1f);
    Check(TEXT("v48_cannon_resumes_when_real_pitch_and_muzzle_sight_recover"),A->CannonShots>BeforeHigh);
    float MinImpactZ=FLT_MAX,MaxImpactZ=-FLT_MAX;bool FiniteShells=true;
    for(int32 I=0;I<8;++I)
    {
        A->FireCannon(0,M->AimPoint(),M);
        if(A->Shells.IsEmpty()){FiniteShells=false;continue;}
        const auto& S=A->Shells.Last();FiniteShells&=!S.Impact.ContainsNaN()&&S.Duration>=.12f&&S.Duration<=.34f;
        MinImpactZ=FMath::Min(MinImpactZ,float(S.Impact.Z));MaxImpactZ=FMath::Max(MaxImpactZ,float(S.Impact.Z));
    }
    Report->SetNumberField(TEXT("armour_shell_vertical_impact_span_cm"),MaxImpactZ-MinImpactZ);
    Check(TEXT("v48_actual_seeded_armour_shells_have_vertical_dispersion_and_finite_flight"),FiniteShells&&MaxImpactZ-MinImpactZ>1.f);

    Clear();auto* W=Spawn(EFrontlineVehicle::Whippet,0,true);auto* Threat=Spawn(EFrontlineVehicle::A7V,1,true);
    if(!W||!Threat){Check(TEXT("v48_whippet_pair_assets"),false);return Report;}
    Place(W,-900*Dir,-300);Place(Threat,900*Dir,300);const FVector WAt=W->GetActorLocation();
    for(int32 I=0;I<30;++I)W->Simulate(.1f);
    Check(TEXT("v48_machinegun_only_whippet_stops_for_armour_threat_without_invented_cannon"),W->Cannons.IsEmpty()&&W->CannonShots==0&&W->GetActorLocation().Equals(WAt,.001));
    // An out-of-arc turn beside another live hull must not rotate through it.
    Place(W,0,-275);Place(Threat,0,275);const float Heading=Threat->HeadingYawV37;
    const FVector Behind=Threat->GetActorLocation()-FRotator(0,Heading,0).Vector()*2000;
    const bool InitiallyClear=Threat->CanOccupyV48(Threat->GetActorLocation(),Heading);
    const bool TurnAllowed=Threat->FaceArmourV48(Behind,.1f);
    Check(TEXT("v48_complete_hull_turn_sweep_refuses_clipping_an_adjacent_live_tank"),InitiallyClear&&!TurnAllowed&&Threat->HeadingYawV37==Heading);
    return Report;
}
