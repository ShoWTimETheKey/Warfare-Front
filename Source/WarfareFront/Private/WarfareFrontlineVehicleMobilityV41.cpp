#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFRRUCampaignV82.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlinePacingV24.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"

bool FrontlineReadHeldArmourGoalV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,FVector& Goal);
bool FrontlineReadHeldArmourLegV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,FVector& Goal,FVector& Leg);
bool FrontlineUseLegacyArmourYieldV84411();
bool FrontlineArmourYieldDirectPathClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Goal,const AFrontlineTank* PreviewTank=nullptr,const FTransform* PreviewPose=nullptr,FString* Failure=nullptr);
bool FrontlineReadBlockedArmourLegsV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const FVector& Goal,TArray<FVector>& Legs);
bool FrontlinePlanCooperativeFloorRouteV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const FVector& Goal,const FTransform* PreviewPose,TArray<FVector>& Path,bool GeometryOnly=false);
bool FrontlineCooperativeFloorRouteClearV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const TArray<FVector>& Path,const FTransform* PreviewPose=nullptr);

namespace
{
bool LegacyStaticBodyDetourV85=false;
struct FStaticPoseKeyV86
{
    FVector Point=FVector::ZeroVector;float Yaw=0,Seconds=0;FTransform Previous=FTransform::Identity;
    bool operator==(const FStaticPoseKeyV86& Other)const
    {
        return Point==Other.Point&&Yaw==Other.Yaw&&Seconds==Other.Seconds
            &&Previous.GetTranslation()==Other.Previous.GetTranslation()
            &&Previous.GetRotation()==Other.Previous.GetRotation()
            &&Previous.GetScale3D()==Other.Previous.GetScale3D();
    }
    friend uint32 GetTypeHash(const FStaticPoseKeyV86& K)
    {
        uint32 H=HashCombine(GetTypeHash(K.Point),HashCombine(GetTypeHash(K.Yaw),GetTypeHash(K.Seconds)));
        H=HashCombine(H,GetTypeHash(K.Previous.GetTranslation()));H=HashCombine(H,GetTypeHash(K.Previous.GetScale3D()));
        const FQuat Q=K.Previous.GetRotation();
        return HashCombine(H,HashCombine(HashCombine(GetTypeHash(Q.X),GetTypeHash(Q.Y)),HashCombine(GetTypeHash(Q.Z),GetTypeHash(Q.W))));
    }
};
struct FStaticPoseResultV86 { bool Clear=false;FTransform Supported=FTransform::Identity; };
}

bool AFrontlineTank::StaticBodyDetourPoseClearV85(const FVector& Position,float Yaw,float Seconds,const FTransform* FromPose,FTransform* OutSupportedPose) const
{
    if(Position.ContainsNaN()||!FMath::IsFinite(Yaw)
        ||(FromPose&&FromPose->ContainsNaN()))return false;
    const FTransform Pose=GroundPoseForContactV8218(Position,Yaw,Seconds,FromPose);
    if(OutSupportedPose)*OutSupportedPose=Pose;
    if(FMath::Abs(Position.Y)>980.f||!CanOccupyV48(Position,Yaw,Seconds,FromPose,&Pose)||!Body||!Body->GetStaticMesh())return false;
    const FTransform BodyLocal=Body->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    const FBox WholeBody=Body->GetStaticMesh()->GetBoundingBox().TransformBy(BodyLocal*Pose);
    if(Pose.ContainsNaN()||!WholeBody.IsValid||WholeBody.Min.Y<-FrontlineBattlefield::HalfFrontage
        ||WholeBody.Max.Y>FrontlineBattlefield::HalfFrontage)return false;
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld());if(!Fleet)return false;
    // Soft normal routes are preserved. A recovery manoeuvre, however, may
    // not open its escape by driving through a neighbouring hull or wreck.
    for(const AFrontlineTank* Other:Fleet->GetVehicles())if(IsValid(Other)&&Other!=this&&!Other->IsHidden())
    {
        const float Old=ArmourOverlapV8311(FromPose?FromPose->GetLocation():GetActorLocation(),
            FromPose?float(FromPose->Rotator().Yaw):HeadingYawV37,Other,12.f);
        const float Next=ArmourOverlapV8311(Position,Yaw,Other,12.f);
        if(Next>.01f&&(Old<=.01f||Next>=Old-.01f))return false;
    }
    return true;
}

bool AFrontlineTank::ReadCooperativeOpeningGuideV86(const AWarfareFrontlineSoldier* Unit,const FVector& Goal,FVector& Guide) const
{
    if(!Battle||!IsOperational()||IsImmobilized()||IsHeldForFieldRepairV63()||IsHidden()
        ||FrontlineUseLegacyArmourYieldV84411()||WarfareFRRUCampaignV82::Active(Battle)
        ||!IsValid(Unit)||!Unit->IsAlive()||Unit->IsHidden()||Unit->Battle.Get()!=Battle.Get()||Unit->Team!=Team
        ||StaticBodyOpeningTrenchV86==INDEX_NONE||!StaticBodyDetourV85.IsEmpty()||StaticBodyDetourOwnerV85.Get()!=Unit)return false;
    const auto* Held=Battle->FindSquad(Unit->SquadId);
    const auto* Opening=CooperativeOpeningsV86.Find(const_cast<AWarfareFrontlineSoldier*>(Unit));
    if(!Held||Held->bAdvancing||Held->Trench!=StaticBodyOpeningTrenchV86||!Opening||Opening->Acknowledged
        ||Opening->Encounter!=Battle->GetEncounterId()||Opening->Trench!=Held->Trench
        ||FVector::DistSquared2D(Goal,Opening->Goal)>FMath::Square(.01f)
        ||FVector::DistSquared2D(Unit->Feet(),Opening->Leg)<=FMath::Square(.01f)
        ||!FrontlineArmourYieldDirectPathClearV84411(Unit,Opening->Leg))return false;
    Guide=Opening->Leg;return true;
}

void AFrontlineTank::CancelCooperativeOpeningGuideV86(const AWarfareFrontlineSoldier* Unit,const FVector& Goal)
{
    if(!Unit||StaticBodyDetourOwnerV85.Get()!=Unit||StaticBodyOpeningTrenchV86==INDEX_NONE)return;
    const auto* Opening=CooperativeOpeningsV86.Find(const_cast<AWarfareFrontlineSoldier*>(Unit));
    if(!Opening||FVector::DistSquared2D(Goal,Opening->Goal)>FMath::Square(.01f))return;
    CooperativeOpeningsV86.Remove(StaticBodyDetourOwnerV85);StaticBodyOpeningTrenchV86=INDEX_NONE;
    StaticBodyDetourRetryAtV85=0;RouteDecisionAtV37=0;
}

bool AFrontlineTank::RefreshInfantryRecoveryWitnessV86() const
{
    auto& Witness=InfantryRecoveryWitnessV86;
    if(!Battle||!Witness.Valid||Witness.Encounter!=Battle->GetEncounterId()||Witness.At>Battle->Elapsed
        ||Battle->Elapsed-Witness.At>4.25f)return false;
    const FTransform Actual=GetActorTransform();
    if(Witness.From.Equals(Actual,0.f))return true;
    // Re-resolve only the already published first backing intention while its
    // planar origin is unchanged. Ground settling and checked stationary yaw
    // must not leave a four-second-old supported hull as a live witness.
    // Actual travel invalidates it; this never creates a future route or extends
    // the publication lease, and every consumer still tests its current body.
    if(Witness.From.GetLocation().X!=Actual.GetLocation().X||Witness.From.GetLocation().Y!=Actual.GetLocation().Y)
    {Witness.Valid=false;return false;}
    if(Actual.ContainsNaN()||Witness.LocalStep.ContainsNaN()||!FMath::IsFinite(Witness.Turn)
        ||!Body||!Body->GetStaticMesh())return false;
    const FVector Step=FRotator(0,HeadingYawV37,0).RotateVector(FVector(Witness.LocalStep.X,Witness.LocalStep.Y,0));
    const FVector Point=Actual.GetLocation()+Step;
    const float Yaw=HeadingYawV37+Witness.Turn;
    const FTransform Supported=GroundPoseForContactV8218(Point,Yaw,Witness.Seconds,&Actual);
    if(Supported.ContainsNaN()||FMath::Abs(Point.Y)>980.f)return false;
    const FTransform BodyLocal=Body->GetComponentTransform().GetRelativeTransform(Actual);
    const FBox Whole=Body->GetStaticMesh()->GetBoundingBox().TransformBy(BodyLocal*Supported);
    if(!Whole.IsValid||Whole.Min.Y<-FrontlineBattlefield::HalfFrontage||Whole.Max.Y>FrontlineBattlefield::HalfFrontage)return false;
    Witness.From=Actual;Witness.Supported=Supported;
    return true;
}

bool AFrontlineTank::HasInfantryRecoveryContactV86(const AWarfareFrontlineSoldier* Unit) const
{
    const auto& Witness=InfantryRecoveryWitnessV86;
    if(!Battle||!IsOperational()||IsImmobilized()||IsHeldForFieldRepairV63()||IsHidden()
        ||FrontlineUseLegacyArmourYieldV84411()||bNationalGermanGroundV82||bRailConvoyV32||WarfareFRRUCampaignV82::Active(Battle)
        ||!IsValid(Unit)||!Unit->IsAlive()||Unit->IsHidden()||Unit->Battle.Get()!=Battle.Get()||Unit->Team!=Team
        ||!RefreshInfantryRecoveryWitnessV86()||!Witness.From.Equals(GetActorTransform(),0.f))return false;
    const auto* Held=Battle->FindSquad(Unit->SquadId);
    return Held&&!Held->bAdvancing&&Battle->Trenches.IsValidIndex(Held->Trench)
        &&!BlocksInfantryAtV8218(Unit->Feet(),GetActorLocation(),HeadingYawV37,34.f,0.f,Unit)
        &&BlocksInfantryAtV8218(Unit->Feet(),Witness.Supported.GetLocation(),float(Witness.Supported.Rotator().Yaw),34.f,
            Witness.Seconds,Unit,&Witness.From,&Witness.Supported);
}

bool AFrontlineTank::IsInfantryRecoveryStationClearV86(const AWarfareFrontlineSoldier* Unit,const FVector& Station) const
{
    if(!HasInfantryRecoveryContactV86(Unit))return true;
    const auto& Witness=InfantryRecoveryWitnessV86;
    for(const FVector2D Margin:{FVector2D::ZeroVector,FVector2D(-3,-3),FVector2D(-3,3),FVector2D(3,-3),FVector2D(3,3)})
    {
        FVector Point=Station+FVector(Margin.X,Margin.Y,0);
        Point.Z=FMath::Max(Station.Z,double(FrontlineInfantryV45::SurfaceHeight(Battle,Point)));
        if(BlocksInfantryAtV8218(Point,Witness.Supported.GetLocation(),float(Witness.Supported.Rotator().Yaw),34.f,
            Witness.Seconds,Unit,&Witness.From,&Witness.Supported))return false;
    }
    if(FVector::DistSquared2D(Unit->Feet(),Station)<=FMath::Square(10.f)
        &&BlocksInfantryAtV8218(Unit->Feet(),Witness.Supported.GetLocation(),float(Witness.Supported.Rotator().Yaw),34.f,
            Witness.Seconds,Unit,&Witness.From,&Witness.Supported))return false;
    return true;
}

bool AFrontlineTank::PrepareStaticBodyDetourV85(float Speed,float Seconds,AWarfareFrontlineSoldier* PassingOwner)
{
    if(LegacyStaticBodyDetourV85||!Battle||bNationalGermanGroundV82||bRailConvoyV32||IsImmobilized()
        ||IsHeldForFieldRepairV63()||WarfareFRRUCampaignV82::Active(Battle)||!FMath::IsFinite(Speed)||Speed<=1.f
        ||!FMath::IsFinite(Seconds)||Seconds<=0.f
        ||Speed*Seconds<=.001f
        ||Battle->Elapsed<StaticBodyDetourRetryAtV85||!StaticBodyDetourV85.IsEmpty())return false;
    FString ReviewUser;
    const bool PrivateReview=Battle->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(ReviewUser);
    TArray<TSharedPtr<FJsonValue>> TrialReceipts;
    if(PrivateReview)
    {StaticBodyDetourAttemptV86=MakeShared<FJsonObject>();StaticBodyDetourAttemptV86->SetNumberField(TEXT("host_seconds"),Battle->Elapsed);
        StaticBodyDetourAttemptV86->SetStringField(TEXT("start"),GetActorTransform().ToString());}
    ON_SCOPE_EXIT{if(PrivateReview)StaticBodyDetourAttemptV86->SetArrayField(TEXT("trials"),TrialReceipts);};
    const int32 Dir=Battle->GetTeamDirection(Team);const FVector Start=GetActorLocation();
    const float ForwardYaw=Dir>0?0.f:180.f;
    if(FRotator(0,HeadingYawV37,0).Vector().X*Dir<.35f)return false;
    AWarfareFrontlineSoldier* Blocker=nullptr;float Closest=MAX_flt;
    if(PassingOwner)
    {
        const auto* Held=IsValid(PassingOwner)?Battle->FindSquad(PassingOwner->SquadId):nullptr;
        if(!Held||Held->bAdvancing||!Battle->Trenches.IsValidIndex(Held->Trench)||!PassingOwner->IsAlive()
            ||PassingOwner->IsHidden()||PassingOwner->Team!=Team||PassingOwner->Battle.Get()!=Battle.Get())return false;
        Blocker=PassingOwner;
    }
    for(AWarfareFrontlineSoldier* U:Battle->Units)if(!PassingOwner)
    {
        if(!IsValid(U)||!U->IsAlive()||U->IsHidden()||U->Team!=Team||U->GetMotionSpeed()>15.f)continue;
        const auto* Held=Battle->FindSquad(U->SquadId);
        if(!Held||Held->bAdvancing||!Battle->Trenches.IsValidIndex(Held->Trench))continue;
        const float Ahead=float((U->Feet().X-Start.X)*Dir);
        const float BodyGap=FMath::Abs(Ahead);
        // On a bank the supported rear may sweep a man behind the centre.
        // Eligibility belongs to the actual next rendered hull, not to its nose.
        if(BodyGap>FootprintHalfLengthV37()+180.f||BodyGap>=Closest
            ||!BlocksInfantryAtV8218(U->Feet(),Start+FRotator(0,HeadingYawV37,0).Vector()*(Speed*Seconds),HeadingYawV37,34.f,Seconds,U))continue;
        Blocker=U;Closest=BodyGap;
    }
    if(!Blocker){if(PrivateReview)StaticBodyDetourAttemptV86->SetStringField(TEXT("reason"),TEXT("no_eligible_actual_forward_body"));return false;}
    if(PrivateReview)
    {StaticBodyDetourAttemptV86->SetNumberField(TEXT("blocker_squad"),Blocker->SquadId);StaticBodyDetourAttemptV86->SetNumberField(TEXT("blocker_member"),Blocker->MemberIndex);
        StaticBodyDetourAttemptV86->SetStringField(TEXT("blocker_feet"),Blocker->Feet().ToString());}
    // These are finite trajectories, never a forced relocation. A stationary
    // main gun and wire-bound infantry cannot accept the trench-yield lease;
    // the vehicle must make room for its own first turn instead.
    StaticBodyDetourRetryAtV85=Battle->Elapsed+4.f;
    InfantryRecoveryWitnessV86.Valid=false;
    bool StraightRecoveryWitnessV86=false;
    const float Side=FMath::Abs(Start.Y-Blocker->Feet().Y)>20.f?FMath::Sign(float(Start.Y-Blocker->Feet().Y)):
        ((Serial+Team)%2?1.f:-1.f);
    const float Steering=SteeringRateV8311(Speed,false);
    const float MaximumBack=FMath::Min(650.f,FootprintHalfLengthV37()+FootprintHalfWidthV8311());
    TArray<FStaticBodyDetourPoseV85> OpeningPath;FVector OpeningLeg=FVector::ZeroVector,OpeningGoal=FVector::ZeroVector,OpeningGuided=FVector::ZeroVector;
    TArray<FVector> OpeningLegs;
    AWarfareFrontlineSoldier* OpeningOwner=Blocker;
    // A planted man already at his firing bay can be the nearest hull contact.
    // The same next-hull witness can also touch a second man with a real blocked
    // step. Keep the complete route owner unchanged, but let that actual step
    // own cooperative opening; no additional vehicle trajectories are searched.
    if(!PassingOwner)
    {
        FVector Goal,Guided;TArray<FVector> Tried;
        const auto HasCurrentBlockedStep=[&](AWarfareFrontlineSoldier* Unit)
        {
            Tried.Reset();
            return FrontlineReadHeldArmourLegV86(Unit,this,Goal,Guided)
                &&FrontlineReadBlockedArmourLegsV86(Unit,this,Goal,Tried)
                &&Tried.ContainsByPredicate([&](const FVector& Leg)
                    {return !FrontlineArmourYieldDirectPathClearV84411(Unit,Leg);});
        };
        const bool PrimaryHasStep=HasCurrentBlockedStep(Blocker);
        if(!PrimaryHasStep)
        {
            float Nearest=MAX_flt;
            for(AWarfareFrontlineSoldier* U:Battle->Units)
            {
                if(!IsValid(U)||!U->IsAlive()||U->IsHidden()||U->Team!=Team||U->GetMotionSpeed()>15.f)continue;
                const float Gap=FMath::Abs(float(U->Feet().X-Start.X));
                if(Gap>FootprintHalfLengthV37()+180.f||Gap>=Nearest
                    ||!BlocksInfantryAtV8218(U->Feet(),Start+FRotator(0,HeadingYawV37,0).Vector()*(Speed*Seconds),HeadingYawV37,34.f,Seconds,U)
                    ||!HasCurrentBlockedStep(U))continue;
                OpeningOwner=U;Nearest=Gap;
            }
        }
    }
    const auto* OpeningHeld=Battle->FindSquad(OpeningOwner->SquadId);
    const auto* PreviousOpening=CooperativeOpeningsV86.Find(OpeningOwner);
    const FVector PriorLeg=PreviousOpening?PreviousOpening->Leg-PreviousOpening->From:FVector::ZeroVector;
    const bool PriorLegCompleted=!PreviousOpening||PreviousOpening->Acknowledged||(OpeningHeld&&PreviousOpening->Trench!=OpeningHeld->Trench)
        ||FVector::DotProduct(OpeningOwner->Feet()-PreviousOpening->From,PriorLeg.GetSafeNormal2D())>=PriorLeg.Size2D()-.01;
    bool OpeningEligible=!FrontlineUseLegacyArmourYieldV84411()&&OpeningHeld&&!OpeningHeld->bAdvancing&&Battle->Trenches.IsValidIndex(OpeningHeld->Trench)
        &&OpeningOwner->GetMotionSpeed()<=15.f&&PriorLegCompleted;
    if(OpeningEligible)
    {
        // Read the existing leased shoulder, or the man's actual held bay.
        // The cooperative opening never invents an infantry order/endpoint.
        OpeningEligible=FrontlineReadHeldArmourLegV86(OpeningOwner,this,OpeningGoal,OpeningGuided);
        FVector Leg=OpeningGuided-OpeningOwner->Feet();Leg.Z=0;
        OpeningEligible=OpeningEligible&&Leg.Size2D()>.01;
        if(OpeningEligible)
        {
            OpeningLeg=OpeningOwner->Feet()+Leg.GetSafeNormal2D()*FMath::Min(24.,Leg.Size2D());
            OpeningLeg.Z=FrontlineInfantryV45::SurfaceHeight(Battle,OpeningLeg);
            FrontlineReadBlockedArmourLegsV86(OpeningOwner,this,OpeningGoal,OpeningLegs);
            for(FVector& TriedLeg:OpeningLegs)
            {
                FVector Direction=TriedLeg-OpeningOwner->Feet();Direction.Z=0;
                if(Direction.Size2D()>24.){TriedLeg=OpeningOwner->Feet()+Direction.GetSafeNormal2D()*24.;
                    TriedLeg.Z=FrontlineInfantryV45::SurfaceHeight(Battle,TriedLeg);}
            }
            OpeningLegs.AddUnique(OpeningLeg);
            OpeningLegs.RemoveAll([&](const FVector& TriedLeg){return FrontlineArmourYieldDirectPathClearV84411(OpeningOwner,TriedLeg);});
            OpeningEligible=!OpeningLegs.IsEmpty();
        }
    }
    TArray<FVector> OpeningFloorPath,GeometryFloorPath;
    // Geometry-only exploration never grants a step: the complete path must
    // subsequently clear this actual hull at a checked recovery pose. The
    // shared floor graph keeps the original goal and permits necessary bends.
    if(OpeningEligible&&!FrontlinePlanCooperativeFloorRouteV86(OpeningOwner,this,OpeningGoal,nullptr,GeometryFloorPath,true))
        OpeningEligible=false;
    if(PrivateReview)
    {
        StaticBodyDetourAttemptV86->SetBoolField(TEXT("opening_eligible"),OpeningEligible);
        StaticBodyDetourAttemptV86->SetNumberField(TEXT("opening_owner_squad"),OpeningOwner->SquadId);
        StaticBodyDetourAttemptV86->SetNumberField(TEXT("opening_owner_member"),OpeningOwner->MemberIndex);
        TArray<TSharedPtr<FJsonValue>> LegReceipts;
        for(const FVector& TriedLeg:OpeningLegs)
        {
            FString Failure;const bool Clear=FrontlineArmourYieldDirectPathClearV84411(OpeningOwner,TriedLeg,nullptr,nullptr,&Failure);
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("leg"),TriedLeg.ToString());R->SetBoolField(TEXT("current_clear"),Clear);
            R->SetStringField(TEXT("first_current_reject"),Failure);LegReceipts.Add(MakeShared<FJsonValueObject>(R));
        }
        StaticBodyDetourAttemptV86->SetArrayField(TEXT("opening_tried_legs"),LegReceipts);
    }
    // This synchronous attempt moves no actor or terrain. Exact support keys
    // include the preceding pitch/roll as well as location, heading and host
    // step; repeated reverse prefixes may reuse the identical predicate.
    TMap<FStaticPoseKeyV86,FStaticPoseResultV86> PoseCache;uint32 PoseQueries=0,PoseCacheHits=0;
    ON_SCOPE_EXIT{if(PrivateReview){StaticBodyDetourAttemptV86->SetNumberField(TEXT("pose_queries"),PoseQueries);
        StaticBodyDetourAttemptV86->SetNumberField(TEXT("unique_pose_predicates"),PoseCache.Num());
        StaticBodyDetourAttemptV86->SetNumberField(TEXT("exact_pose_cache_hits"),PoseCacheHits);}};
    for(float InitialTurn:{0.f,3.f,-3.f,6.f,-6.f})
    for(bool StraightReverse:{false,true})for(float Sign:{Side,-Side})for(float Angle:{20.f,30.f,40.f})for(float Back:{120.f,240.f,360.f,480.f,600.f})
    {
        // Wheeled reverse already keeps its heading and never pivots. The
        // second StraightReverse pass is precisely the same trajectory.
        if(StraightReverse&&IsWheeledV8218())continue;
        if(InitialTurn!=0.f&&(!StraightReverse||IsWheeledV8218()
            ||FRotator(0,HeadingYawV37+InitialTurn,0).Vector().X*Dir<.35f))continue;
        if(Back>MaximumBack)continue;
        TArray<FStaticBodyDetourPoseV85> Path;FVector P=Start;float Yaw=HeadingYawV37;
        TSharedPtr<FJsonObject> Trial;
        if(PrivateReview)
        {Trial=MakeShared<FJsonObject>();Trial->SetNumberField(TEXT("turn_deg"),Angle*Sign);Trial->SetNumberField(TEXT("reverse_cm"),Back);
            Trial->SetBoolField(TEXT("straight_reverse_before_turn"),StraightReverse);
            Trial->SetNumberField(TEXT("stationary_turn_before_reverse_deg"),InitialTurn);
            TrialReceipts.Add(MakeShared<FJsonValueObject>(Trial));}
        const float TargetYaw=ForwardYaw+Sign*Dir*Angle;float Remaining=Back;bool Clear=true;
        const bool Wheeled=IsWheeledV8218();FTransform PathPose=GetActorTransform();
        const auto AcceptPose=[&](const FVector& Point,float Heading,float SegmentSeconds)
        {
            // Every chassis consumes each short waypoint on a real host frame.
            // Carry its accepted support pose forward, exactly as FitGround does;
            // predicting every future point from the original tilt opens a
            // false trench route which the visible hull later cannot follow.
            const float SupportSeconds=SegmentSeconds;
            const FTransform* Previous=&PathPose;
            const FStaticPoseKeyV86 Key{Point,Heading,SupportSeconds,PathPose};++PoseQueries;
            auto* Cached=PoseCache.Find(Key);
            if(Cached)++PoseCacheHits;
            else
            {
                FStaticPoseResultV86 Result;
                Result.Clear=StaticBodyDetourPoseClearV85(Point,Heading,SupportSeconds,Previous,&Result.Supported);
                Cached=&PoseCache.Add(Key,Result);
            }
            if(!Cached->Clear)
            {
                // A real first curved backing support can touch the man who
                // is already safe for ordinary forward motion. Publish that
                // first-frame intention only as a fallback: the original
                // straight-first witness retains priority if it also blocks.
                // This is no accepted pose; both actors retain every real gate.
                if(Path.IsEmpty()&&InitialTurn==0.f&&(!InfantryRecoveryWitnessV86.Valid
                    ||(StraightReverse&&!StraightRecoveryWitnessV86))
                    &&!Cached->Supported.ContainsNaN()&&Body&&Body->GetStaticMesh()&&FMath::Abs(Point.Y)<=980.f)
                {
                    const FTransform BodyLocal=Body->GetComponentTransform().GetRelativeTransform(GetActorTransform());
                    const FBox Whole=Body->GetStaticMesh()->GetBoundingBox().TransformBy(BodyLocal*Cached->Supported);
                    if(Whole.IsValid&&Whole.Min.Y>=-FrontlineBattlefield::HalfFrontage&&Whole.Max.Y<=FrontlineBattlefield::HalfFrontage)
                    {
                        bool ActualHeldBodyBlocks=false;
                        for(const AWarfareFrontlineSoldier* Unit:Battle->Units)
                        {
                            if(!IsValid(Unit)||!Unit->IsAlive()||Unit->IsHidden()||Unit->Team!=Team)continue;
                            const auto* Held=Battle->FindSquad(Unit->SquadId);
                            if(!Held||Held->bAdvancing||!Battle->Trenches.IsValidIndex(Held->Trench))continue;
                            if(!BlocksInfantryAtV8218(Unit->Feet(),GetActorLocation(),HeadingYawV37,34.f,0.f,Unit)
                                &&BlocksInfantryAtV8218(Unit->Feet(),Point,Heading,34.f,SupportSeconds,Unit,Previous,&Cached->Supported))
                            {ActualHeldBodyBlocks=true;break;}
                        }
                        if(ActualHeldBodyBlocks)
                        {InfantryRecoveryWitnessV86={GetActorTransform(),Cached->Supported,Battle->GetEncounterId(),Battle->Elapsed,SupportSeconds,true};
                            const FVector LocalStep=FRotator(0,-HeadingYawV37,0).RotateVector(Point-GetActorLocation());
                            InfantryRecoveryWitnessV86.LocalStep=FVector2D(LocalStep.X,LocalStep.Y);
                            InfantryRecoveryWitnessV86.Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,Heading);
                            StraightRecoveryWitnessV86=StraightReverse;}
                    }
                }
                if(Trial)
                {
                    Trial->SetStringField(TEXT("first_rejected_point"),Point.ToString());Trial->SetNumberField(TEXT("first_rejected_heading"),Heading);
                    Trial->SetNumberField(TEXT("accepted_poses_before_rejection"),Path.Num());Trial->SetBoolField(TEXT("centre_in_field"),FMath::Abs(Point.Y)<=980.f);
                    Trial->SetStringField(TEXT("actual_predicted_pose"),Cached->Supported.ToString());
                    TArray<TSharedPtr<FJsonValue>> Bodies;
                    for(const AWarfareFrontlineSoldier* U:Battle->Units)if(IsValid(U)&&U->IsAlive()&&!U->IsHidden()
                        &&BlocksInfantryAtV8218(U->Feet(),Point,Heading,34.f,SupportSeconds,U,Previous,&Cached->Supported)
                        &&!RelievesExistingInfantryContactV842(U->Feet(),Point,Heading,34.f,SupportSeconds,U,Previous,&Cached->Supported))
                    {auto BodyReceipt=MakeShared<FJsonObject>();BodyReceipt->SetNumberField(TEXT("squad"),U->SquadId);BodyReceipt->SetNumberField(TEXT("member"),U->MemberIndex);
                        BodyReceipt->SetStringField(TEXT("feet"),U->Feet().ToString());Bodies.Add(MakeShared<FJsonValueObject>(BodyReceipt));}
                    Trial->SetArrayField(TEXT("actual_body_blockers"),Bodies);
                }
                return false;
            }
            PathPose=Cached->Supported;
            return !PathPose.ContainsNaN();
        };
        const auto CaptureOpening=[&]()
        {
            if(!OpeningEligible||!OpeningPath.IsEmpty()
                ||!FrontlineCooperativeFloorRouteClearV86(OpeningOwner,this,GeometryFloorPath,&PathPose))return;
            FVector Leg=GeometryFloorPath[0]-OpeningOwner->Feet();Leg.Z=0;
            if(Leg.Size2D()<=.01)return;
            OpeningLeg=OpeningOwner->Feet()+Leg.GetSafeNormal2D()*FMath::Min(24.,Leg.Size2D());
            OpeningLeg.Z=FrontlineInfantryV45::SurfaceHeight(Battle,OpeningLeg);
            if(!FrontlineArmourYieldDirectPathClearV84411(OpeningOwner,OpeningLeg,this,&PathPose))return;
            OpeningFloorPath=GeometryFloorPath;OpeningPath=Path;
        };
        // A three-point recovery needs a checked stationary first turn when
        // both translating directions are blocked. These small alternatives
        // stay inside the same forward domain and original reverse budget.
        const float FirstYaw=HeadingYawV37+InitialTurn;
        while(InitialTurn!=0.f&&FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,FirstYaw))>.01f&&Clear)
        {
            const float Next=FMath::FixedTurn(Yaw,FirstYaw,SteeringRateV8311(0.f,true)*Seconds);
            Clear=AcceptPose(P,Next,Seconds);
            if(Clear){Yaw=Next;Path.Add({P,Yaw,false});CaptureOpening();}
            else if(!Path.IsEmpty())
            {
                // A safe initial pivot need not reach the requested small angle.
                // Its last checked pose is a valid start for the same bounded
                // reverse. Never store or execute the rejected next pose.
                if(Trial){Trial->SetBoolField(TEXT("stationary_pivot_stopped_at_checked_prefix"),true);
                    Trial->SetNumberField(TEXT("actual_initial_pivot_deg"),FMath::FindDeltaAngleDegrees(HeadingYawV37,Yaw));}
                Clear=true;break;
            }
        }
        if(!Clear)continue;
        while(Remaining>.01f&&Clear)
        {
            const float Step=FMath::Min(FMath::Min(10.f,Speed*Seconds),Remaining),DT=Seconds;
            // A car needs space for its forward steering arc. Turning toward
            // that same heading in reverse first cancels the lateral clearance
            // when it drives forward again; it is not a usable bypass curve.
            const float NextYaw=Wheeled||StraightReverse?Yaw:FMath::FixedTurn(Yaw,TargetYaw,Steering*DT);
            const float MidYaw=Yaw+FMath::FindDeltaAngleDegrees(Yaw,NextYaw)*.5f;
            const FVector Next=P-FRotator(0,MidYaw,0).Vector()*Step;
            Clear=AcceptPose(Next,NextYaw,DT);
            if(Clear)
            {
                P=Next;Yaw=NextYaw;Path.Add({P,Yaw,true});Remaining-=Step;
                // Save only a real opening of the currently blocked held leg.
                // It is a prefix of an existing bounded reverse candidate,
                // fully checked pose by pose, not a claimed complete bypass.
                // Neighbour bodies and centimetre floor/hull sweeps remain
                // unchanged; only this vehicle uses its predicted end pose.
                CaptureOpening();
                if(PrivateReview&&OpeningEligible&&OpeningPath.IsEmpty()&&Remaining<=.01f)
                {
                    TArray<TSharedPtr<FJsonValue>> RejectedLegs;
                    for(const FVector& TriedLeg:OpeningLegs)
                    {
                        FString Failure;FrontlineArmourYieldDirectPathClearV84411(OpeningOwner,TriedLeg,this,&PathPose,&Failure);
                        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("leg"),TriedLeg.ToString());R->SetStringField(TEXT("first_reject"),Failure);
                        RejectedLegs.Add(MakeShared<FJsonValueObject>(R));
                    }
                    StaticBodyDetourAttemptV86->SetArrayField(TEXT("last_opening_prefix_refusals"),RejectedLegs);
                    StaticBodyDetourAttemptV86->SetNumberField(TEXT("last_opening_prefix_reverse_cm"),Back);
                }
            }
        }
        if(!Clear)continue;
        // Tracked vehicles retain their existing stationary angular rate.
        while(!Wheeled&&FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,TargetYaw))>.01f&&Clear)
        {
            const float Next=FMath::FixedTurn(Yaw,TargetYaw,SteeringRateV8311(0.f,true)*Seconds);
            Clear=AcceptPose(P,Next,Seconds);
            if(Clear){Yaw=Next;Path.Add({P,Yaw,false});}
        }
        if(!Clear)continue;
        // Clearing only the diagonal projection is insufficient when this
        // finite trajectory straightens again. Use the whole model envelope.
        const float Tail=FootprintHalfLengthV37()+FootprintHalfWidthV8311();
        const float Exit=float(Blocker->Feet().X)*Dir+Tail+90.f;
        const TArray<FStaticBodyDetourPoseV85> Approach=Path;
        const FVector ForwardStart=P;const float ForwardStartYaw=Yaw;const FTransform ForwardStartPose=PathPose;
        bool Complete=false;
        // Keep the original straight exit as the cheapest first candidate.
        // A long hull can have a clear shoulder but meet another held man or
        // the field edge before its rear clears. Try bounded return arcs
        // after model-derived clearance distances. Every pose keeps the same
        // support, body, field, travel and steering gates.
        const float Returns[]={MAX_flt,0.f,FootprintHalfWidthV8311()+70.f,FootprintHalfLengthV37()};
        for(int32 ReturnIndex=0;ReturnIndex<UE_ARRAY_COUNT(Returns)&&!Complete;++ReturnIndex)
        {
            Path=Approach;P=ForwardStart;Yaw=ForwardStartYaw;PathPose=ForwardStartPose;Clear=true;
            double ForwardCm=0.;
            if(Trial)Trial->SetNumberField(TEXT("return_after_forward_cm"),ReturnIndex?Returns[ReturnIndex]:-1.f);
            while(float(P.X)*Dir<Exit&&Clear&&ForwardCm<2200.)
            {
                // Preserve the old 220 x 10 cm distance budget at every FPS.
                const float Step=FMath::Min(FMath::Min(10.f,Speed*Seconds),float(2200.-ForwardCm)),DT=Seconds;
                const bool Returning=ReturnIndex&&ForwardCm>=Returns[ReturnIndex];
                const float NextYaw=(Wheeled||Returning)?FMath::FixedTurn(Yaw,Returning?ForwardYaw:TargetYaw,Steering*DT):Yaw;
                const float MidYaw=Yaw+FMath::FindDeltaAngleDegrees(Yaw,NextYaw)*.5f;
                const FVector Next=P+FRotator(0,MidYaw,0).Vector()*Step;
                Clear=AcceptPose(Next,NextYaw,DT);
                if(Clear){P=Next;Yaw=NextYaw;Path.Add({P,Yaw,false});ForwardCm+=Step;}
            }
            Complete=Clear&&float(P.X)*Dir>=Exit;
        }
        if(!Complete)continue;
        StaticBodyDetourV85=MoveTemp(Path);StaticBodyDetourIndexV85=0;StaticBodyDetourBlockedV85=0;
        StaticBodyOpeningTrenchV86=INDEX_NONE;
        StaticBodyDetourOwnerV85=Blocker;StaticBodyDetourOwnerFeetV85=Blocker->Feet();
        if(PrivateReview){StaticBodyDetourAttemptV86->SetStringField(TEXT("reason"),TEXT("accepted_complete_checked_route"));Trial->SetBoolField(TEXT("accepted"),true);}
        bBypassInfantryPassingV84412=false;BypassPhaseV839=0;DesiredLaneV37=P.Y;return true;
    }
    if(!OpeningPath.IsEmpty())
    {
        StaticBodyDetourV85=MoveTemp(OpeningPath);StaticBodyDetourIndexV85=0;StaticBodyDetourBlockedV85=0;
        StaticBodyDetourOwnerV85=OpeningOwner;StaticBodyDetourOwnerFeetV85=OpeningOwner->Feet();StaticBodyOpeningTrenchV86=OpeningHeld->Trench;
        CooperativeOpeningsV86.Add(OpeningOwner,FCooperativeOpeningV86{OpeningOwner->Feet(),OpeningLeg,OpeningGoal,OpeningGuided,OpeningHeld->Trench,false,Battle->GetEncounterId(),OpeningFloorPath});
        BypassInfantryAttemptsV84411.Add(OpeningOwner,OpeningOwner->Feet());
        bBypassInfantryPassingV84412=false;BypassPhaseV839=0;
        if(PrivateReview)
        {
            StaticBodyDetourAttemptV86->SetStringField(TEXT("reason"),TEXT("accepted_checked_cooperative_reverse_opening"));
            StaticBodyDetourAttemptV86->SetStringField(TEXT("opening_original_feet"),OpeningOwner->Feet().ToString());
            StaticBodyDetourAttemptV86->SetStringField(TEXT("opening_actual_held_goal"),OpeningGoal.ToString());
            StaticBodyDetourAttemptV86->SetStringField(TEXT("opening_checked_leg"),OpeningLeg.ToString());
            TArray<TSharedPtr<FJsonValue>> FloorRoute;
            for(const FVector& Point:OpeningFloorPath)FloorRoute.Add(MakeShared<FJsonValueString>(Point.ToString()));
            StaticBodyDetourAttemptV86->SetArrayField(TEXT("opening_complete_floor_path"),FloorRoute);
            StaticBodyDetourAttemptV86->SetStringField(TEXT("opening_terminal"),StaticBodyDetourV85.Last().Position.ToString());
            StaticBodyDetourAttemptV86->SetNumberField(TEXT("opening_trench"),StaticBodyOpeningTrenchV86);
            StaticBodyDetourAttemptV86->SetNumberField(TEXT("opening_checked_poses"),StaticBodyDetourV85.Num());
            double ActualReverse=0.;FVector Previous=Start;
            for(const auto& Pose:StaticBodyDetourV85){ActualReverse+=FVector::Dist2D(Previous,Pose.Position);Previous=Pose.Position;}
            StaticBodyDetourAttemptV86->SetNumberField(TEXT("opening_checked_reverse_cm"),ActualReverse);
        }
        return true;
    }
    if(PrivateReview)StaticBodyDetourAttemptV86->SetStringField(TEXT("reason"),TEXT("all_bounded_routes_physically_rejected"));
    return false;
}

float AFrontlineTank::AdvanceStaticBodyDetourV85(float Distance,float Seconds)
{
    if(!StaticBodyDetourV85.IsValidIndex(StaticBodyDetourIndexV85)||!Battle||Distance<=0.f||Seconds<=0.f)return 0.f;
    const FVector Before=GetActorLocation();const auto& Goal=StaticBodyDetourV85[StaticBodyDetourIndexV85];
    const float Travel=float(FVector::Dist2D(Before,Goal.Position));
    const float Turn=FMath::FindDeltaAngleDegrees(HeadingYawV37,Goal.Heading);
    const float Rate=SteeringRateV8311(Travel>.001f?Distance/Seconds:0.f,Travel<=.001f);
    float Fraction=Travel>.001f?FMath::Min(1.f,Distance/Travel):1.f;
    if(FMath::Abs(Turn)>.001f)Fraction=FMath::Min(Fraction,Rate*Seconds/FMath::Abs(Turn));
    const FVector P=FMath::Lerp(Before,Goal.Position,Fraction);const float Yaw=HeadingYawV37+Turn*Fraction;
    if(StaticBodyDetourPoseClearV85(P,Yaw,Seconds))
    {
        HeadingYawV37=Yaw;FitGroundV35(P,Seconds);
        const float Moved=float(FVector::Dist2D(Before,GetActorLocation()));bBackingStepV839=Goal.Reverse&&Moved>.01f;
        if(Moved>.01f||FMath::Abs(Turn*Fraction)>.001f)StaticBodyDetourBlockedV85=0;else StaticBodyDetourBlockedV85+=Seconds;
        if(FVector::Dist2D(GetActorLocation(),Goal.Position)<.01
            &&FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingYawV37,Goal.Heading))<.01f)++StaticBodyDetourIndexV85;
        if(!StaticBodyDetourV85.IsValidIndex(StaticBodyDetourIndexV85))
        {StaticBodyDetourV85.Reset();StaticBodyDetourIndexV85=0;RouteStallSecondsV837=0;RouteDecisionAtV37=0;}
        return Moved;
    }
    StaticBodyDetourBlockedV85+=Seconds;
    // An arriving soldier or hull invalidates the remaining plan. Retry normal
    // local routing rather than retaining a stale path or forcing the step.
    if(StaticBodyDetourBlockedV85>=.5f)
    {if(StaticBodyOpeningTrenchV86!=INDEX_NONE)CooperativeOpeningsV86.Remove(StaticBodyDetourOwnerV85);
        StaticBodyDetourV85.Reset();StaticBodyDetourIndexV85=0;StaticBodyOpeningTrenchV86=INDEX_NONE;RouteDecisionAtV37=0;StaticBodyDetourRetryAtV85=Battle->Elapsed+2.f;}
    return 0.f;
}

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunMobilityChecksV41()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;bool Good=true;
    auto Check=[&](const FString& Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetBoolField(TEXT("success"),Good);};
    auto* B=Battle();FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("armour_mobility_requires_isolated_review"),false);return Report;}
    const auto Saved=Vehicles;const auto Encounter=LastEncounter;const float Clock=B->Elapsed,EnemyClock=NextEnemyAt;
    const int32 Serial=NextSerial,Count0=DeploymentCountV37[0],Count1=DeploymentCountV37[1];
    const float Morale0=B->Morale[0],Morale1=B->Morale[1];const auto Phase=B->Phase;const FString Message=B->LastMessage;
    float Ready[2][int32(EFrontlineVehicle::Count)];for(int32 T=0;T<2;++T)for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)Ready[T][I]=ReadyAt[T][I];
    const float Cycle[2]={ArmourCycleSecondsV40[0],ArmourCycleSecondsV40[1]};
    TSet<AActor*> Effects;for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)Effects.Add(*It);
    Vehicles.Empty();LastEncounter=B->GetEncounterId();B->Phase=EFrontlinePhase::Fighting;
    ON_SCOPE_EXIT
    {
        for(AFrontlineTank* V:Vehicles)if(IsValid(V))V->Destroy();Vehicles=Saved;LastEncounter=Encounter;NextSerial=Serial;NextEnemyAt=EnemyClock;
        for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)if(!Effects.Contains(*It))It->Destroy();
        DeploymentCountV37[0]=Count0;DeploymentCountV37[1]=Count1;B->Elapsed=Clock;B->Phase=Phase;B->LastMessage=Message;B->Morale[0]=Morale0;B->Morale[1]=Morale1;
        for(int32 T=0;T<2;++T){ArmourCycleSecondsV40[T]=Cycle[T];for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)ReadyAt[T][I]=Ready[T][I];}
    };
    for(int32 Team=0;Team<2;++Team)
    {
        const auto Roster=GetRoster(Team);bool Recruited=Roster.Num()==3,Shared=true;
        for(int32 I=0;I<6&&Roster.Num()==3;++I)
        {
            ResetReadinessForDebug(Team);const int32 Before=Vehicles.Num();const bool Success=Recruit(Roster[I%Roster.Num()].Type,Team);Recruited&=Success&&Vehicles.Num()==Before+1;
            if(Success)Shared&=!Recruit(Roster[(I+1)%Roster.Num()].Type,Team)&&Vehicles.Num()==Before+1;
        }
        Check(FString::Printf(TEXT("army%d_can_recruit_six_live_tanks_without_population_cap"),Team),Recruited);
        Check(FString::Printf(TEXT("army%d_still_shares_one_model_cooldown"),Team),Shared);
        AFrontlineTank* Wrecked=nullptr;AFrontlineTank* Follower=nullptr;
        for(AFrontlineTank* V:Vehicles)if(V->Team==Team){if(!Wrecked)Wrecked=V;else if(!Follower)Follower=V;V->SetActorLocation(FVector(-15000-Team*4000,V->Serial*800,0));}
        if(!Wrecked||!Follower)continue;
        const int32 Dir=B->GetTeamDirection(Team);
        Wrecked->SetActorLocation(FVector(0,-300,0));Wrecked->ReceiveModuleHit(EFrontlineTankModule::Hull,Wrecked->GetMaximumHullIntegrity()+1);
        bool NoCollision=!Wrecked->GetActorEnableCollision();TInlineComponentArray<UPrimitiveComponent*> Components(Wrecked);
        for(const UPrimitiveComponent* C:Components)NoCollision&=C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->GetGenerateOverlapEvents();
        Check(FString::Printf(TEXT("army%d_wreck_all_primitive_collision_is_disabled"),Team),NoCollision);
        Follower->HeadingYawV37=Dir>0?0:180;Follower->PreferredLaneV37=Follower->DesiredLaneV37=-300;Follower->RouteDecisionAtV37=0;
        Follower->SetActorLocation(FVector(-1000*Dir,-300,0));Follower->FitGroundV35(Follower->GetActorLocation(),0);
        bool Bounded=true;for(int32 I=0;I<110;++I)
        {
            const FVector Before=Follower->GetActorLocation();Follower->AdvanceV37(20,.1f);const FVector After=Follower->GetActorLocation();
            Bounded&=!After.ContainsNaN()&&(After.X-Before.X)*Dir>=-.01&&FVector::Dist2D(Before,After)<=20.01&&FMath::Abs(After.Y+300)<1;
        }
        Check(FString::Printf(TEXT("army%d_live_tank_passes_wreck_without_detour_or_reverse"),Team),Bounded&&Follower->GetActorLocation().X*Dir>1000);
        Follower->SetActorLocation(FVector(-19000-Team*4000,Follower->Serial*800,0));
    }
    return Report;
}

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunMobilityChecksV842()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    auto Check=[&](const FString& Name,bool Passed){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Passed;};
    ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);Report->SetBoolField(TEXT("success"),Good);};
    auto* B=Battle();FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV842Review"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("v842_requires_disposable_no_save_review"),false);return Report;}
    const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const int32 Serial=NextSerial;
    const int32 Counts[2]={DeploymentCountV37[0],DeploymentCountV37[1]};const auto Message=B->LastMessage;
    const auto Encounter=LastEncounter;const float Clock=B->Elapsed;
    TArray<AFrontlineTank*> Fixtures;AWarfareFrontlineSoldier* Person=nullptr;
    Vehicles.Empty();B->Units.Empty();LastEncounter=B->GetEncounterId();
    ON_SCOPE_EXIT{if(IsValid(Person))Person->Destroy();for(auto* V:Fixtures)if(IsValid(V))V->Destroy();
        Vehicles=SavedVehicles;B->Units=SavedUnits;NextSerial=Serial;LastEncounter=Encounter;B->Elapsed=Clock;B->LastMessage=Message;
        DeploymentCountV37[0]=Counts[0];DeploymentCountV37[1]=Counts[1];};
    const int32 Trench=B->Trenches.Num()/2;
    Check(TEXT("v842_authored_trench_present"),B->Trenches.IsValidIndex(Trench));if(!B->Trenches.IsValidIndex(Trench))return Report;
    const EFrontlineVehicle Models[]={EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkIVFemale,EFrontlineVehicle::MarkVMale,
        EFrontlineVehicle::MarkVStar,EFrontlineVehicle::Whippet,EFrontlineVehicle::A7V,EFrontlineVehicle::A7VGretchen,
        EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::BeuteMarkIVFemale,EFrontlineVehicle::Austin1918};
    for(int32 Team=0;Team<2;++Team)for(int32 N=0;N<UE_ARRAY_COUNT(Models);++N)
    {
        auto* V=Spawn(Models[N],Team,false);if(!V){Check(TEXT("v842_actual_model_spawn"),false);continue;}
        Fixtures.Add(V);Vehicles={V};const int32 Dir=B->GetTeamDirection(Team);const float Lane=N%2?300.f:-300.f;
        const float Centre=FrontlineFieldV32::TrenchCentreX(Trench,Lane);
        V->HeadingYawV37=Dir>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=Lane;V->RouteDecisionAtV37=0;
        V->SetActorLocation(FVector(Centre-Dir*1100.f,Lane,0));V->FitGroundV35(V->GetActorLocation(),0);
        const FVector Start=V->GetActorLocation();bool Finite=true,Bounded=true;float MaxIdle=0.f,Idle=0.f;
        for(int32 I=0;I<180&&float((V->GetActorLocation().X-Centre)*Dir)<1150.f;++I)
        {
            const FVector Before=V->GetActorLocation();V->AdvanceV37(20.f,.1f);const FVector After=V->GetActorLocation();
            const float Travel=float(FVector::Dist2D(Before,After));Idle=Travel<.01f?Idle+.1f:0.f;MaxIdle=FMath::Max(MaxIdle,Idle);
            Finite&=!After.ContainsNaN()&&!V->GetActorTransform().ContainsNaN();
            Bounded&=Travel<=20.01f&&FMath::Abs(After.Y)<=980.1f&&(After.X-Before.X)*Dir>=-.01f;
            B->Elapsed+=.1f;
        }
        const bool Crossed=(V->GetActorLocation().X-Centre)*Dir>1100.f;
        auto C=MakeShared<FJsonObject>();C->SetNumberField(TEXT("model"),int32(Models[N]));C->SetNumberField(TEXT("team"),Team);
        C->SetStringField(TEXT("start"),Start.ToString());C->SetStringField(TEXT("end"),V->GetActorLocation().ToString());C->SetNumberField(TEXT("max_idle_seconds"),MaxIdle);
        C->SetBoolField(TEXT("crossed"),Crossed);Cases.Add(MakeShared<FJsonValueObject>(C));
        Check(FString::Printf(TEXT("v842_model%d_direction%d_continuous_actual_trench_crossing"),int32(Models[N]),Team),Finite&&Bounded&&Crossed);
        V->Destroy();Vehicles.Empty();
    }
    // A stationary friendly man is an actual body obstacle, not a fake wall.
    // The normal physical route must find a usable shoulder without contact.
    Person=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
    if(Person)
    {
        Person->Initialize(B,842001,0,EFrontlineRole::Rifle,0);Person->SetActorTickEnabled(false);B->Units={Person};
        const float Centre=FrontlineFieldV32::TrenchCentreX(Trench,300.f);
        const FVector Feet(Centre+260.f,300.f,FrontlineInfantryV45::SurfaceHeight(B,FVector(Centre+260.f,300.f,0)));
        Person->SetActorLocation(Feet+FVector(0,0,88.f));
        auto* V=Spawn(EFrontlineVehicle::MarkIVMale,0,false);
        if(V)
        {
            Fixtures.Add(V);Vehicles={V};const int32 Dir=B->GetTeamDirection(0);V->HeadingYawV37=Dir>0?0.f:180.f;
            V->PreferredLaneV37=V->DesiredLaneV37=300.f;V->RouteDecisionAtV37=0;
            V->SetActorLocation(FVector(Centre-Dir*1100.f,300.f,0));V->FitGroundV35(V->GetActorLocation(),0);
            bool NoNewIntersection=true;
            for(int32 I=0;I<300&&(V->GetActorLocation().X-Centre)*Dir<1200.f;++I)
            {V->AdvanceV37(20.f,.1f);B->Elapsed+=.1f;NoNewIntersection&=!V->BlocksInfantryAtV8218(Person->Feet(),V->GetActorLocation(),V->HeadingYawV37);}
            Check(TEXT("v842_trench_bank_body_is_avoided_without_overlap_or_permanent_stall"),NoNewIntersection&&(V->GetActorLocation().X-Centre)*Dir>1100.f);
            // An already enclosed torso may move only toward a nearer face.
            B->Units.Empty();V->SetActorLocation(FVector(Centre-900.f,300.f,0));V->HeadingYawV37=0;V->FitGroundV35(V->GetActorLocation(),0);
            const FVector Enclosed=V->GetActorTransform().TransformPosition(FVector(-300.f,0,130.f))-FVector(0,0,72.5f);
            const FVector P=V->GetActorLocation();
            Check(TEXT("v842_existing_overlap_can_relieve_but_cannot_deepen_or_teleport"),V->BlocksInfantryAtV8218(Enclosed,P,0)
                &&V->RelievesExistingInfantryContactV842(Enclosed,P+FVector(20,0,0),0,34,.1f)
                &&!V->RelievesExistingInfantryContactV842(Enclosed,P-FVector(20,0,0),0,34,.1f)
                &&!V->RelievesExistingInfantryContactV842(Enclosed,P+FVector(200,0,0),0,34,.1f));
        }
        else Check(TEXT("v842_contact_fixture_spawn"),false);
    }
    else Check(TEXT("v842_actual_infantry_fixture_spawn"),false);
    Report->SetStringField(TEXT("scope"),TEXT("Actual production meshes, current trench heightfield, grounded AdvanceV37 planner and reciprocal standing-body envelope. Does not simulate a full battle or assert observed combat halts are defects."));
    return Report;
}
#include "WarfareFrontlineArmourMobilityReviewV8445.inl"

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunMobilityChecksV85()
{
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;
    const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;};
    ON_SCOPE_EXIT{LegacyStaticBodyDetourV85=false;Report->SetBoolField(TEXT("success"),Good);Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("cases"),Cases);};
    auto* B=Battle();FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FPaths::NormalizeDirectoryName(User);
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V85/Runtime"));FPaths::NormalizeDirectoryName(Root);
    const FString Stamp=User.StartsWith(Root+TEXT("/"),ESearchCase::IgnoreCase)?User.Mid(Root.Len()+1):FString();
    const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&!FPaths::IsRelative(User)&&!User.Contains(TEXT(".."))&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
    Check(TEXT("v85_field_detour_exact_disposable_ukde_world"),Safe);if(!Safe)return Report;
    const auto SavedVehicles=Vehicles;const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;
    const auto SavedSmoke=B->SmokeZones;const auto SavedTracers=B->Tracers;const auto Phase=B->Phase;const auto Message=B->LastMessage;
    const auto Encounter=LastEncounter;const float Clock=B->Elapsed,EnemyAt=NextEnemyAt;const int32 Serial=NextSerial,Winner=B->BattleOutroWinner;
    const bool Paused=UGameplayStatics::IsGamePaused(B);
    TArray<AFrontlineTank*> Tanks;TArray<AWarfareFrontlineSoldier*> People;
    struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden;};TArray<FHidden> Hidden;
    for(AFrontlineTank* V:SavedVehicles)if(IsValid(V)){Hidden.Add({V,V->IsHidden()});V->SetActorHiddenInGame(true);}
    for(AWarfareFrontlineSoldier* U:SavedUnits)if(IsValid(U)){Hidden.Add({U,U->IsHidden()});U->SetActorHiddenInGame(true);}
    const auto Clear=[&]{for(auto* V:Tanks)if(IsValid(V))V->Destroy();for(auto* U:People)if(IsValid(U))U->Destroy();Tanks.Reset();People.Reset();Vehicles.Reset();B->Units.Reset();B->Squads.Reset();B->SmokeZones.Reset();B->Tracers.Reset();B->Elapsed=1000;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;LastEncounter=B->GetEncounterId();NextEnemyAt=MAX_flt;FrontlineMobilityV41::Reset(B);};
    ON_SCOPE_EXIT{Clear();Vehicles=SavedVehicles;B->Units=SavedUnits;B->Squads=SavedSquads;B->SmokeZones=SavedSmoke;B->Tracers=SavedTracers;
        B->Phase=Phase;B->LastMessage=Message;B->Elapsed=Clock;B->BattleOutroWinner=Winner;LastEncounter=Encounter;NextEnemyAt=EnemyAt;NextSerial=Serial;
        for(const auto& H:Hidden)if(auto* A=H.Actor.Get())A->SetActorHiddenInGame(H.Hidden);UGameplayStatics::SetGamePaused(B,Paused);};
    UGameplayStatics::SetGamePaused(B,false);
    // Select authored features by deterministic object name, never by whether
    // a trial route succeeds. The field height and mesh support remain intact.
    struct FFeature{FString Name;FVector Position;};TArray<FFeature> Wires,Trees;
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Parts(*It);
        for(auto* C:Parts)if(C&&C->IsVisible()&&C->GetStaticMesh())
        {
            const FString Mesh=C->GetStaticMesh()->GetName();const bool Wire=Mesh.Contains(TEXT("Wire_Coil"));
            const bool Tree=Mesh.Contains(TEXT("SplinteredTrunk"))||Mesh.Contains(TEXT("SplitStump"));if(!Wire&&!Tree)continue;
            const auto Add=[&](FVector P,int32 Index){if(FMath::Abs(P.X)<FrontlinePacingV24::CombatHalfLength()-1200.f&&FMath::Abs(P.Y)<(Wire?1100.f:3500.f))(Wire?Wires:Trees).Add({C->GetPathName()+FString::Printf(TEXT("_%05d"),Index),P});};
            if(auto* I=Cast<UInstancedStaticMeshComponent>(C)){for(int32 N=0;N<I->GetInstanceCount();++N){FTransform T;if(I->GetInstanceTransform(N,T,true))Add(T.GetLocation(),N);}}
            else Add(C->GetComponentLocation(),0);
        }
    }
    const auto Order=[](const FFeature& A,const FFeature& E){return A.Name<E.Name;};Wires.Sort(Order);Trees.Sort(Order);
    Check(TEXT("v85_actual_wire_feature_present"),!Wires.IsEmpty());Check(TEXT("v85_actual_tree_feature_present"),!Trees.IsEmpty());
    if(Wires.IsEmpty()||Trees.IsEmpty()||B->Trenches.IsEmpty())return Report;
    const FVector Anchors[]={Wires[0].Position,Trees[0].Position,FVector(FrontlineFieldV32::TrenchCentreX(B->Trenches.Num()/2,0.f),0,0)};
    const TCHAR* Names[]={TEXT("wire_approach"),TEXT("tree_line_adjacent_playable_lane"),TEXT("trench_approach")};
    const double Started=FPlatformTime::Seconds();int32 Witnesses=0,Trials=0;
    for(int32 Team=0;Team<2;++Team)for(int32 Context=0;Context<3;++Context)
        for(auto Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkVStar,EFrontlineVehicle::A7V,EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918})
    {
        bool OldCrossed=false;float OldMaxIdle=0;
        for(int32 Trial=0;Trial<2;++Trial)
        {
            Clear();++Trials;LegacyStaticBodyDetourV85=Trial==0;const int32 Dir=B->GetTeamDirection(Team);
            const FVector Anchor(Anchors[Context].X,FMath::Clamp(float(Anchors[Context].Y),-500.f,500.f),0);
            auto* V=GetWorld()->SpawnActor<AFrontlineTank>();auto* U=GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
            if(!V||!U){Check(TEXT("v85_field_actual_actor_spawn"),false);if(V)V->Destroy();if(U)U->Destroy();continue;}
            Tanks.Add(V);People.Add(U);Vehicles.Add(V);B->Units.Add(U);
            const bool Configured=V->Configure(B,Type,Team,++NextSerial);Check(TEXT("v85_field_actual_body_assets"),Configured&&V->AreAssetsReady());if(!Configured)continue;
            V->SetActorTickEnabled(false);V->HeadingYawV37=Dir>0?0.f:180.f;V->PreferredLaneV37=V->DesiredLaneV37=Anchor.Y;V->bEntered=true;
            V->FitGroundV35(Anchor-FVector(Dir*(V->FootprintHalfLengthV37()+60.f),0,0),0);
            U->Initialize(B,8505000+Trials,Team,EFrontlineRole::Rifle,0);U->SetActorTickEnabled(false);U->bFieldEntered=true;U->ReturnToIdle();
            const FVector Feet(Anchor.X,Anchor.Y,FrontlineInfantryV45::SurfaceHeight(B,Anchor));U->SetActorLocation(Feet+FVector(0,0,88));
            U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(.1f,false);U->Soldier->RefreshBoneTransforms();
            const FVector Start=V->GetActorLocation();float Idle=0,MaxIdle=0,Reverse=0;bool Finite=true,Bounded=true,MoveBounded=true,TurnBounded=true,ClearBody=true,Detour=false;TArray<TSharedPtr<FJsonValue>> Samples;
            TSharedPtr<FJsonObject> FirstViolation;
            const auto ActualBodyClear=[&]
            {
                const FTransform Hull=V->Body->GetComponentTransform();const FVector Scale=Hull.GetScale3D().GetAbs();
                const FVector Vertical=Hull.InverseTransformVector(FVector(0,0,72.5f));
                const FBox Box=V->Body->GetStaticMesh()->GetBoundingBox().ExpandBy(FVector(FMath::Abs(Vertical.X)+34.f/FMath::Max(.01,Scale.X),FMath::Abs(Vertical.Y)+34.f/FMath::Max(.01,Scale.Y),FMath::Abs(Vertical.Z)));
                return !Box.IsInsideOrOn(Hull.InverseTransformPosition(U->Feet()+FVector(0,0,72.5f)));
            };
            ClearBody=ActualBodyClear();float Time=0;
            for(int32 F=0;F<1400&&V->RearProgressV49()<float(Feet.X)*Dir+90.f;++F)
            {
                const float DT=F%11==0?.1f:.05f;const FVector Before=V->GetActorLocation();const float BeforeYaw=V->HeadingYawV37;
                B->Elapsed+=DT;Time+=DT;V->Simulate(DT);const float Moved=float(FVector::Dist2D(Before,V->GetActorLocation()));
                Idle=V->MobilityRequestedV839>.01f&&Moved<.01f?Idle+DT:0.f;MaxIdle=FMath::Max(MaxIdle,Idle);
                const float Back=float((Before.X-V->GetActorLocation().X)*Dir);if(Back>0)Reverse+=Back;
                const bool Stationary=Moved<.01f;const float Cap=V->SteeringRateV8311(Stationary?0.f:V->MobilityRequestedV839/DT,Stationary);
                const float ActualTurn=FMath::Abs(FMath::FindDeltaAngleDegrees(BeforeYaw,V->HeadingYawV37));
                const bool FrameMoveBounded=Moved<=V->MobilityRequestedV839+.01f;
                const bool FrameTurnBounded=ActualTurn<=Cap*DT+.01f;
                const bool FrameFinite=!V->GetActorTransform().ContainsNaN(),FrameBodyClear=ActualBodyClear();
                MoveBounded&=FrameMoveBounded;TurnBounded&=FrameTurnBounded;Bounded&=FrameMoveBounded&&FrameTurnBounded;
                Finite&=FrameFinite;ClearBody&=FrameBodyClear;Detour|=!V->StaticBodyDetourV85.IsEmpty();
                if(!FirstViolation&&(!FrameMoveBounded||!FrameTurnBounded||!FrameFinite||!FrameBodyClear))
                {
                    FirstViolation=V->DescribeMobilityV839();FirstViolation->SetNumberField(TEXT("frame"),F);
                    FirstViolation->SetNumberField(TEXT("actual_step_cm"),Moved);FirstViolation->SetNumberField(TEXT("actual_turn_deg"),ActualTurn);
                    FirstViolation->SetNumberField(TEXT("turn_budget_deg"),Cap*DT);FirstViolation->SetBoolField(TEXT("move_bounded"),FrameMoveBounded);
                    FirstViolation->SetBoolField(TEXT("turn_bounded"),FrameTurnBounded);FirstViolation->SetBoolField(TEXT("finite"),FrameFinite);
                    FirstViolation->SetBoolField(TEXT("actual_body_clear"),FrameBodyClear);
                }
                if(F%40==0)Samples.Add(MakeShared<FJsonValueObject>(V->DescribeMobilityV839()));
            }
            const bool Crossed=V->RearProgressV49()>float(Feet.X)*Dir+80.f;
            const FString Name=FString::Printf(TEXT("v85_static_body_t%d_%s_model%d"),Team,Names[Context],int32(Type));
            if(Trial==0){OldCrossed=Crossed;OldMaxIdle=MaxIdle;}
            else
            {
                if(!OldCrossed&&OldMaxIdle>3.f&&Crossed)++Witnesses;
                Check(Name+TEXT("_production_tick_crosses_fixed_stationary_body"),Crossed&&Finite&&Bounded&&ClearBody);
                Check(Name+TEXT("_bounded_recovery_without_body_overlap_or_teleport"),MaxIdle<5.f&&Reverse<655.f);
            }
            auto J=V->DescribeMobilityV839();J->SetStringField(TEXT("case"),Name);J->SetBoolField(TEXT("previous_short_arcs_only"),Trial==0);
            J->SetStringField(TEXT("authored_feature"),Context==0?Wires[0].Name:Context==1?Trees[0].Name:TEXT("middle_trench"));
            J->SetStringField(TEXT("authored_feature_world_position"),Anchors[Context].ToString());J->SetStringField(TEXT("fixed_lane_anchor"),Anchor.ToString());
            J->SetStringField(TEXT("fixed_start"),Start.ToString());J->SetStringField(TEXT("fixed_body_feet"),Feet.ToString());
            J->SetNumberField(TEXT("seconds"),Time);J->SetNumberField(TEXT("max_requested_idle"),MaxIdle);J->SetNumberField(TEXT("actual_reverse_cm"),Reverse);
            J->SetBoolField(TEXT("used_bounded_detour"),Detour);J->SetBoolField(TEXT("crossed"),Crossed);J->SetBoolField(TEXT("actual_rendered_hull_clear_of_standing_body"),ClearBody);
            J->SetBoolField(TEXT("finite"),Finite);J->SetBoolField(TEXT("move_bounded"),MoveBounded);J->SetBoolField(TEXT("turn_bounded"),TurnBounded);
            if(FirstViolation)J->SetObjectField(TEXT("first_violation"),FirstViolation);
            J->SetArrayField(TEXT("samples"),Samples);Cases.Add(MakeShared<FJsonValueObject>(J));
        }
    }
    Check(TEXT("v85_fixed_field_trench_two_directions_five_models_paired_cases"),Trials==60);
    Check(TEXT("v85_old_short_arc_deadlock_reproduced_and_removed"),Witnesses>0);
    Report->SetNumberField(TEXT("old_new_deadlock_witnesses"),Witnesses);Report->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Started);
    Report->SetStringField(TEXT("scope"),TEXT("UK/DE actual authored wire/tree-line/middle-trench terrain, two directions and five actual chassis. Fixed initial stationary friendly body, actual Simulate and renderer-body envelope. No route-success placement search, no motion teleport, no changed travel speed or infantry orders. Trees/wire are not asserted to be hard hull colliders; nearby infantry is the contact fixture."));
    return Report;
}
