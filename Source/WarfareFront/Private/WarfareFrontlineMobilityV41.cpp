#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlineTrenchMotionV8444Checks.h"
#include "WarfareFrontlineMovementFocusedV84413.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Dom/JsonObject.h"

bool FrontlineUseLegacyArmourYieldV84411();
bool FrontlineArmourYieldGoalBodyClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Goal);
bool FrontlineArmourYieldDirectPathClearV84411(const AWarfareFrontlineSoldier* U,const FVector& Goal,const AFrontlineTank* PreviewTank=nullptr,const FTransform* PreviewPose=nullptr,FString* Failure=nullptr);
bool FrontlineArmourYieldReachableStationClearV84411(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const FVector& Point);

namespace
{
constexpr float CellSize=180.f,Clearance=56.f,SearchRadius=340.f;
struct FPerson {TWeakObjectPtr<AWarfareFrontlineSoldier> Unit;FVector Position;bool LegacyIndexed=true;};
struct FPass
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Obstacle;float Side=1,Until=0;
    int32 FullTrenchEscapeV84413=INDEX_NONE,EscapeDirectionV84413=0,EscapeDestinationV84413=INDEX_NONE;
    FVector ContactOriginV84413=FVector::ZeroVector;
    float ActualBackwardTravelV84413=0.f;
    TWeakObjectPtr<AWarfareFrontlineSoldier> ContactLegBodyV84413;
    float ContactLegTurnV84413=0.f,ContactLegRadiusV84413=0.f;
};
struct FArmourYieldV8218
{
    TWeakObjectPtr<AFrontlineTank> Tank;FVector Destination=FVector::ZeroVector;
    float SideV836=0.f;bool PathBlockedV84411=false;
    FVector OriginalFeetV86=FVector::ZeroVector;float OriginalBodyFrontV86=0.f;
    int32 OriginalTrenchV86=INDEX_NONE;
};
struct FBankDetourV81
{
    int32 Trench=INDEX_NONE;
    FVector Goal=FVector::ZeroVector,Waypoint=FVector::ZeroVector,ProgressFeet=FVector::ZeroVector,LastAttemptGoalV8444=FVector::ZeroVector;
    float RetryAt=0.f,AttemptAtV8444=0.f,StalledSecondsV8444=0.f,TotalAttemptSecondsV8444=0.f;
    uint32 DecisionV8444=0;bool Active=false;
    TArray<FVector,TInlineAllocator<24>> FloorPathV84411;
    int32 FloorPathIndexV84411=0;
    TSharedPtr<FJsonObject> LastPhysicalPlanV84411;
};
struct FBankEventV8444
{
    FrontlineTrenchMotionV8444::EBankInvalidation Reason=FrontlineTrenchMotionV8444::EBankInvalidation::None;
    uint32 Decision=0;
    FVector Goal=FVector::ZeroVector,Replacement=FVector::ZeroVector;
    float At=0.f,AttemptSeconds=0.f;bool Held=false,Validated=false;
};
struct FBlockedArmourLegV86
{
    struct FCandidate {TWeakObjectPtr<AFrontlineTank> Tank;FVector Leg=FVector::ZeroVector;};
    FVector From=FVector::ZeroVector,Goal=FVector::ZeroVector;
    int32 Trench=INDEX_NONE;float Time=-1.f,Requested=0.f;
    TArray<FCandidate,TInlineAllocator<5>> Candidates;
};
struct FTerrainRouteV54
{
    int32 Team=INDEX_NONE,Destination=INDEX_NONE,LastPassedTrench=INDEX_NONE,Slot=INDEX_NONE,Operation=INDEX_NONE;
    float StartX=0,EndX=0,StartY=0,EndY=0,OffsetY=0,CrossingT=.5f;
};
struct FLocalField
{
    FGuid Encounter;
    uint64 Frame=MAX_uint64;float Time=-1;int32 Count=-1;
    TArray<FPerson> People;TMap<FIntPoint,TArray<int32>> Cells;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,int32> Index;
    struct FTransformBinding {TWeakObjectPtr<USceneComponent> Root;FDelegateHandle Handle;};
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FTransformBinding> TransformBindings;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FPass> Passing;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FArmourYieldV8218> ArmourYieldV8218;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FBankDetourV81> BankDetoursV81;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FBankEventV8444> BankEventsV8444;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FBlockedArmourLegV86> BlockedArmourLegsV86;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,TSharedPtr<FJsonObject>> StepReceiptsV84411;
    uint32 NextBankDecisionV8444=0;
    // One decision per squad and trench crossing. The ordinary local collision
    // field still owns the final centimetres of every step.
    TMap<int32,FTerrainRouteV54> TerrainRoutesV54;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FLocalField> Fields;
struct FStepInvocationV86 {int32 Depth=0;bool Retried=false;};
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FStepInvocationV86> StepInvocationsV86;
TWeakObjectPtr<AWarfareFrontlineSoldier> TraceUnitV64;
FIntPoint Cell(const FVector& P){return FIntPoint(FMath::FloorToInt(P.X/CellSize),FMath::FloorToInt(P.Y/CellSize));}
void ClearTransformBindings(FLocalField& F)
{
    for(auto& Pair:F.TransformBindings)if(auto* Root=Pair.Value.Root.Get())Root->TransformUpdated.Remove(Pair.Value.Handle);
    F.TransformBindings.Reset();
}
void UpdateFieldPosition(FLocalField& F,AWarfareFrontlineSoldier* U,const FVector& Position)
{
    if(Position.ContainsNaN())return;
    if(const int32* Index=F.Index.Find(U))
    {
        auto& P=F.People[*Index];const auto Old=Cell(P.Position),New=Cell(Position);
        if(auto* Pass=F.Passing.Find(U);Pass&&Pass->FullTrenchEscapeV84413!=INDEX_NONE)
        {
            // Spend only accepted actor travel, never a planned step refused
            // by the facing or supported-placement gates.
            Pass->ActualBackwardTravelV84413+=FMath::Max(0.f,float((P.Position.X-Position.X)*Pass->EscapeDirectionV84413));
        }
        if(Old!=New){F.Cells.FindChecked(Old).RemoveSingleSwap(*Index);F.Cells.FindOrAdd(New).Add(*Index);}P.Position=Position;
    }
}
void TrackActualTransform(AWarfareFrontlineGameMode* B,AWarfareFrontlineSoldier* U,FLocalField& F)
{
    auto* Root=U->GetRootComponent();if(!Root)return;
    if(auto* Existing=F.TransformBindings.Find(U))
    {
        if(Existing->Root.Get()==Root)return;
        if(auto* Previous=Existing->Root.Get())Previous->TransformUpdated.Remove(Existing->Handle);
        F.TransformBindings.Remove(U);
    }
    const TWeakObjectPtr<AWarfareFrontlineGameMode> WeakBattle=B;
    const TWeakObjectPtr<AWarfareFrontlineSoldier> WeakUnit=U;
    const FDelegateHandle Handle=Root->TransformUpdated.AddLambda([WeakBattle,WeakUnit](USceneComponent*,EUpdateTransformFlags,ETeleportType)
    {
        auto* Battle=WeakBattle.Get();auto* Unit=WeakUnit.Get();if(!Battle||!Unit||Unit->Battle.Get()!=Battle)return;
        if(auto* Cached=Fields.Find(WeakBattle);Cached&&Cached->Encounter==Battle->GetEncounterId())
            UpdateFieldPosition(*Cached,Unit,Unit->Feet());
    });
    F.TransformBindings.Add(U,FLocalField::FTransformBinding{Root,Handle});
}
float RouteEnvelopeV57(float T,float CrossingT)
{
    const float C=FMath::Clamp(CrossingT,.18f,.82f);
    return T<=C?FMath::Sin(.5f*PI*T/C):FMath::Sin(.5f*PI*(1.f-T)/(1.f-C));
}
float RouteYV54(float StartY,float EndY,float Offset,float T,float CrossingT=.5f)
{
    return FMath::Lerp(StartY,EndY,T)+Offset*RouteEnvelopeV57(T,CrossingT);
}
float SelectTerrainLaneV54(float StartX,float EndX,float StartY,float EndY,
    TFunctionRef<float(float,float)> TravelScale,bool bLysFords=false,float CrossingT=.5f)
{
    if(!FMath::IsFinite(StartX)||!FMath::IsFinite(EndX)||FMath::Abs(EndX-StartX)<1200.f)return 0.f;
    const bool bSelleCauseway=!bLysFords&&FrontlineBattlefield::OperationIndex==17;
    constexpr float Fractions[]={.10f,.18f,.34f,.50f,.66f,.82f,.90f};
    constexpr float Offsets[]={0.f,330.f,-330.f,450.f,-450.f,720.f,-720.f};
    float BestOffset=0.f,BestCost=FLT_MAX,DirectCost=FLT_MAX;
    // A detour is worthwhile only if its footing gain pays for its extra
    // lateral travel. Candidate routes keep the whole formation on the field.
    for(float Offset:Offsets)
    {
        if(!bLysFords&&FMath::Abs(Offset)>450.f)continue;
        // The Lys crossings are farther apart than a normal trench bay. A
        // modest extra lateral walk is still cheaper than the flooded cut.
        float Cost=(bLysFords?.040f:bSelleCauseway?.012f:.060f)*FMath::Abs(Offset)/630.f;bool Valid=true;
        const auto AddSample=[&](float T,float Weight)
        {
            const float X=FMath::Lerp(StartX,EndX,T),Y=RouteYV54(StartY,EndY,Offset,T,CrossingT);
            if(FMath::Abs(Y)>FrontlineBattlefield::HalfFrontage-350.f){Valid=false;return;}
            const float Scale=TravelScale(X,Y);
            if(!FMath::IsFinite(Scale)||Scale<=0.f){Valid=false;return;}
            Cost+=Weight*(1.f/Scale-1.f);
        };
        const float SampleWeight=1.f/(UE_ARRAY_COUNT(Fractions)+(bLysFords?3:0));
        for(float T:Fractions){AddSample(T,SampleWeight);if(!Valid)break;}
        // The authored wash is narrower than the interval between generic
        // samples. Inspect the physical crossing itself, or two routes can
        // appear equal even when one wades and the other uses a dry ford.
        if(Valid&&bLysFords)
            for(float Delta:{-.045f,0.f,.045f})
            {AddSample(FMath::Clamp(CrossingT+Delta,.05f,.95f),SampleWeight);if(!Valid)break;}
        if(!Valid)continue;
        if(Offset==0.f)DirectCost=Cost;
        if(Cost<BestCost-.0001f){BestCost=Cost;BestOffset=Offset;}
    }
    // Close scores keep the authored squad slot. This also prevents cosmetic
    // relief noise from making two identical orders choose different tracks.
    return BestCost<DirectCost-(bLysFords?.009f:bSelleCauseway?.007f:.018f)?BestOffset:0.f;
}
float OpenGroundBlendV54(const AWarfareFrontlineGameMode* B,const FVector& At)
{
    float Nearest=FLT_MAX;
    for(int32 I=0;I<B->Trenches.Num();++I)
        Nearest=FMath::Min(Nearest,FMath::Abs(At.X-FrontlineFieldV32::TrenchCentreX(I,At.Y)));
    const float T=FMath::Clamp((Nearest-330.f)/420.f,0.f,1.f);
    return T*T*(3.f-2.f*T);
}
FVector TerrainWaypointV54(AWarfareFrontlineSoldier* U,const FVector& Destination,FLocalField& F)
{
    const auto* B=U->Battle.Get();
    const auto* Squad=B->FindSquad(U->SquadId);
    if(!U->bFieldEntered||!Squad||!Squad->bAdvancing)return Destination;
    const int32 Direction=B->GetTeamDirection(U->Team);
    const FVector From=U->Feet();
    auto& Route=F.TerrainRoutesV54.FindOrAdd(Squad->Id);
    if(Route.Team!=Squad->Team||Route.Destination!=Squad->Destination
        ||Route.LastPassedTrench!=Squad->LastPassedTrench||Route.Slot!=Squad->Slot
        ||Route.Operation!=FrontlineBattlefield::OperationIndex)
    {
        const FVector Centre=Squad->Centre();
        Route.Team=Squad->Team;Route.Destination=Squad->Destination;
        Route.LastPassedTrench=Squad->LastPassedTrench;Route.Slot=Squad->Slot;
        Route.Operation=FrontlineBattlefield::OperationIndex;
        // A trench has one canonical X. Per-member wavy-bay offsets vary by
        // over 200 cm and must not repeatedly reselect the entire squad route.
        Route.StartX=Centre.X;
        Route.EndX=B->Trenches.IsValidIndex(Squad->Destination)
            ?B->Trenches[Squad->Destination].X:Destination.X;
        Route.StartY=Centre.Y;
        Route.EndY=(Squad->Slot-1)*630.f;
        const bool bLysFords=FrontlineBattlefield::OperationIndex==12;
        // The Lys lies near one end of each leg. A symmetrical mid-leg arc
        // returns men to their slot before they have reached either ford.
        const float Home=FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
        const float RiverAxis=-.45f*FMath::Max(1900.f,Home);
        Route.CrossingT=bLysFords&&FMath::Abs(Route.EndX-Route.StartX)>1.f
            ?FMath::Clamp((RiverAxis-Route.StartX)/(Route.EndX-Route.StartX),.18f,.82f):.5f;
        Route.OffsetY=(Route.EndX-Route.StartX)*Direction>1200.f
            ?SelectTerrainLaneV54(Route.StartX,Route.EndX,Route.StartY,Route.EndY,
                [](float X,float Y){return FrontlineTerrainTacticsV54::InfantryTravelScale(FVector(X,Y,0));},
                bLysFords,Route.CrossingT)
            :0.f;
    }
    if(Route.OffsetY==0.f||(Destination.X-From.X)*Direction<=0.f)return Destination;
    const float Progress=FMath::Clamp((From.X-Route.StartX)/(Route.EndX-Route.StartX),0.f,1.f);
    const float Offset=Route.OffsetY*RouteEnvelopeV57(Progress,Route.CrossingT)
        *(FrontlineBattlefield::OperationIndex==12?1.f:OpenGroundBlendV54(B,From));
    // Look ahead along the real order, not toward a persistent target off the
    // map. The sin envelope and trench gate return men to their assigned bay.
    FVector Waypoint=Destination;
    Waypoint.X=From.X+Direction*FMath::Min(520.f,(Destination.X-From.X)*Direction);
    Waypoint.Y+=Offset;
    return Waypoint;
}
FLocalField& Field(AWarfareFrontlineGameMode* B)
{
    for(auto It=Fields.CreateIterator();It;++It)if(!It.Key().IsValid()){ClearTransformBindings(It.Value());It.RemoveCurrent();}
    auto& F=Fields.FindOrAdd(B);
    // A normal restart/abandon changes the encounter ID even when the same
    // GameMode actor persists. Drop all old passing and route decisions before
    // the first recruit of the next battle; explicit Reset still removes F.
    if(F.Encounter!=B->GetEncounterId())
    {ClearTransformBindings(F);F.Encounter=B->GetEncounterId();F.Frame=MAX_uint64;F.Passing.Reset();F.ArmourYieldV8218.Reset();F.TerrainRoutesV54.Reset();F.BankDetoursV81.Reset();F.BankEventsV8444.Reset();F.BlockedArmourLegsV86.Reset();}
    if(F.Frame==GFrameCounter&&F.Time==B->Elapsed&&F.Count==B->Units.Num())return F;
    F.Frame=GFrameCounter;F.Time=B->Elapsed;F.Count=B->Units.Num();F.People.Reset();F.Cells.Reset();F.Index.Reset();
    const bool National=WarfareFRRUCampaignV82::Active(B);
    // National body queries also see hidden passengers after their ordinary
    // field-entry state changes. Keep them indexed while aboard, and read the
    // caller's current state predicates instead of caching eligibility.
    for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)&&U->IsAlive()&&(National||(!U->IsActorBeingDestroyed()&&!U->IsHidden())))
    {
        const int32 I=F.People.Add({U,U->Feet(),!U->IsActorBeingDestroyed()&&!U->IsHidden()});F.Index.Add(U,I);F.Cells.FindOrAdd(Cell(U->Feet())).Add(I);
        TrackActualTransform(B,U,F);
    }
    for(auto It=F.TransformBindings.CreateIterator();It;++It)if(!F.Index.Contains(It.Key()))
    {if(auto* Root=It.Value().Root.Get())Root->TransformUpdated.Remove(It.Value().Handle);It.RemoveCurrent();}
    for(auto It=F.Passing.CreateIterator();It;++It)
    {
        auto* U=It.Key().Get();auto& Pass=It.Value();
        if(!IsValid(U)||!U->IsAlive()){It.RemoveCurrent();continue;}
        if(Pass.FullTrenchEscapeV84413!=INDEX_NONE)
        {
            const auto* S=B->FindSquad(U->SquadId);
            if(!S||!S->bAdvancing||S->Destination!=Pass.EscapeDestinationV84413
                ||!B->Trenches.IsValidIndex(Pass.FullTrenchEscapeV84413)
                ||FMath::Abs(U->Feet().X-FrontlineFieldV32::TrenchCentreX(Pass.FullTrenchEscapeV84413,U->Feet().Y))>180.f)
                Pass.FullTrenchEscapeV84413=INDEX_NONE;
        }
        // A transient obstacle/side timeout cannot replenish this episode's
        // accepted-travel budget while the man is still inside the same bank.
        if(Pass.FullTrenchEscapeV84413==INDEX_NONE&&Pass.Until<B->Elapsed-2)It.RemoveCurrent();
    }
    for(auto It=F.BankDetoursV81.CreateIterator();It;++It)if(!It.Key().IsValid()||!It.Key()->IsAlive())It.RemoveCurrent();
    for(auto It=F.BankEventsV8444.CreateIterator();It;++It)if(!It.Key().IsValid()||!It.Key()->IsAlive())It.RemoveCurrent();
    for(auto It=F.BlockedArmourLegsV86.CreateIterator();It;++It)
        if(!It.Key().IsValid()||!It.Key()->IsAlive()||B->Elapsed-It.Value().Time>.25f)It.RemoveCurrent();
    return F;
}

enum class EYieldLegFailureV84411:uint8 {None,Body,Bank,Hull};
struct FYieldLegFailureV84411
{
    EYieldLegFailureV84411 Kind=EYieldLegFailureV84411::None;
    FVector From=FVector::ZeroVector,To=FVector::ZeroVector,Body=FVector::ZeroVector;
    const AWarfareFrontlineSoldier* Other=nullptr;
    const AFrontlineTank* Tank=nullptr;
    double ClosestGap=0.;
};
// Planning and the vehicle's recovery predicate share this exact held-yield
// leg proof. The predicate reads cached decisions and never plans or commits
// movement; a stale spatial cache falls back to actual live body positions.
bool HeldYieldLegClearV84411(const AWarfareFrontlineSoldier* U,const FLocalField* F,
    const TArray<const AFrontlineTank*,TInlineAllocator<8>>& Armour,
    const FVector& Start,const FVector& End,FYieldLegFailureV84411* Failure=nullptr,
    const AFrontlineTank* PreviewTank=nullptr,const FTransform* PreviewPose=nullptr)
{
    FVector Leg=End-Start;Leg.Z=0;const double Length=Leg.Size2D();
    if(Length<.01)return true;
    const auto BodyClear=[&](const AWarfareFrontlineSoldier* Other)
    {
        if(!IsValid(Other)||Other==U||!Other->IsAlive()||Other->IsHidden())return true;
        const FVector Body=Other->Feet();
        const double T=FMath::Clamp(FVector::DotProduct(Body-Start,Leg)/Leg.SizeSquared2D(),0.,1.);
        const double Closest=FVector::DistSquared2D(Start+Leg*T,Body);
        if(Closest>=FMath::Square(Clearance+.5f))return true;
        FVector Away=Start-Body;Away.Z=0;const double Gap=Away.SizeSquared2D();
        const bool LeavesExistingMargin=Gap>=FMath::Square(Clearance)
            &&Gap<FMath::Square(Clearance+.5f)&&Closest>=FMath::Square(Clearance)
            &&FVector::DotProduct(Away,Leg)>=0.
            &&FVector::DistSquared2D(End,Body)>=FMath::Square(Clearance+.5f);
        if(LeavesExistingMargin)return true;
        if(Failure){Failure->Kind=EYieldLegFailureV84411::Body;Failure->From=Start;Failure->To=End;
            Failure->Body=Body;Failure->Other=Other;Failure->ClosestGap=FMath::Sqrt(Closest);}
        return false;
    };
    if(F&&F->Frame==GFrameCounter&&F->Time==U->Battle->Elapsed&&F->Count==U->Battle->Units.Num())
    {
        const FIntPoint First=Cell(FVector(FMath::Min(Start.X,End.X)-Clearance-.5f,FMath::Min(Start.Y,End.Y)-Clearance-.5f,0));
        const FIntPoint Last=Cell(FVector(FMath::Max(Start.X,End.X)+Clearance+.5f,FMath::Max(Start.Y,End.Y)+Clearance+.5f,0));
        for(int64 Y=First.Y;Y<=Last.Y;++Y)for(int64 X=First.X;X<=Last.X;++X)
            if(const auto* Bucket=F->Cells.Find(FIntPoint(int32(X),int32(Y))))
                for(int32 Index:*Bucket)if(!BodyClear(F->People[Index].Unit.Get()))return false;
    }
    else for(const AWarfareFrontlineSoldier* Other:U->Battle->Units)if(!BodyClear(Other))return false;
    FVector Previous=Start;Previous.Z=FMath::Max(Start.Z,double(FrontlineInfantryV45::SurfaceHeight(U->Battle,Start)));
    const int32 Steps=FMath::CeilToInt(Length);
    for(int32 I=1;I<=Steps;++I)
    {
        FVector Next=FMath::Lerp(Start,End,double(I)/Steps);
        if(!FrontlineInfantryV45::AllowsTrenchStep(U,Previous,Next))
        {if(Failure){Failure->Kind=EYieldLegFailureV84411::Bank;Failure->From=Previous;Failure->To=Next;}return false;}
        Next.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);Previous=Next;
    }
    Previous=Start;Previous.Z=FMath::Max(Start.Z,double(FrontlineInfantryV45::SurfaceHeight(U->Battle,Start)));
    for(int32 I=1;I<=Steps;++I)
    {
        FVector Next=FMath::Lerp(Start,End,double(I)/Steps);
        const double Floor=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);Next.Z=FMath::Max(Previous.Z,Floor);
        for(const AFrontlineTank* Tank:Armour)if(Tank->InfantrySweepClearanceV8218(Previous,Next,34.f,U,Tank==PreviewTank?PreviewPose:nullptr)<1.f)
        {if(Failure){Failure->Kind=EYieldLegFailureV84411::Hull;Failure->From=Previous;Failure->To=Next;Failure->Tank=Tank;}return false;}
        Previous=Next;Previous.Z=Floor;
    }
    return true;
}
struct FHeldFloorGraphV86
{
    TArray<FVector> Path;
    FVector Goal=FVector::ZeroVector;
    float Cost=MAX_flt;
    int32 Nodes=0,Visited=0,TestedEdges=0,TangentNodes=0;
    bool OriginalGoalReachable=false;
};
// The existing held-floor lattice is shared by ordinary movement and the
// vehicle's plan/execute contract. Callers provide the actual or preview hull;
// the same finite graph and complete physical edges select both routes.
bool FindHeldFloorGraphV86(const AWarfareFrontlineSoldier* U,const FVector& Destination,float Distance,
    float LocalRadius,bool GoalInScope,TConstArrayView<FVector> Headings,
    TFunctionRef<bool(const FVector&)> NodeClear,TFunctionRef<bool(const FVector&,const FVector&)> LegClear,
    TFunctionRef<bool(const FVector&)> ReplacementClear,FHeldFloorGraphV86& Out)
{
    Out=FHeldFloorGraphV86();Out.Goal=Destination;
    const auto* Squad=U->Battle->FindSquad(U->SquadId);if(!Squad)return false;
    const FVector From=U->Feet();
    TArray<FVector,TInlineAllocator<160>> Nodes;Nodes.Add(From);Nodes.Add(GoalInScope?Destination:From);
    int32 Grid[17][7];for(auto& Row:Grid)for(int32& Index:Row)Index=INDEX_NONE;
    const float FloorGoalY=GoalInScope?float(Destination.Y):float(From.Y)
        +FMath::Clamp(float(Destination.Y-From.Y),-LocalRadius+36.f,LocalRadius-36.f);
    const float MinimumY=FMath::Min(float(From.Y),FloorGoalY)-36.f,MaximumY=FMath::Max(float(From.Y),FloorGoalY)+36.f;
    const int32 Rows=FMath::Clamp(FMath::CeilToInt((MaximumY-MinimumY)/24.f)+1,2,17);
    const float Depth=float(From.X)-FrontlineFieldV32::TrenchCentreX(Squad->Trench,From.Y);
    FVector StartFloor=From;StartFloor.X=FrontlineFieldV32::TrenchCentreX(Squad->Trench,From.Y);
    const double FloorHeightAllowance=From.Z-FrontlineInfantryV45::SurfaceHeight(U->Battle,StartFloor)+8.;
    const float Depths[7]={-84.f,-56.f,-28.f,0.f,28.f,56.f,84.f};
    for(int32 Row=0;Row<Rows;++Row)for(int32 Column=0;Column<7;++Column)
    {
        const float Y=FMath::Lerp(MinimumY,MaximumY,float(Row)/(Rows-1));
        FVector P(FrontlineFieldV32::TrenchCentreX(Squad->Trench,Y)+Depth+Depths[Column],Y,0);
        if(FMath::Abs(Y)>FrontlineBattlefield::HalfFrontage-35.f||FVector::DistSquared2D(From,P)>FMath::Square(LocalRadius))continue;
        P.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,P);
        FVector Floor=P;Floor.X=FrontlineFieldV32::TrenchCentreX(Squad->Trench,Y);
        if(P.Z-FrontlineInfantryV45::SurfaceHeight(U->Battle,Floor)>FloorHeightAllowance||!NodeClear(P))continue;
        Grid[Row][Column]=Nodes.Add(P);
    }
    const int32 FirstTangent=Nodes.Num();
    for(int32 H=0;H<FMath::Min(7,Headings.Num());++H)for(float Radius:{4.f,8.f,Distance,24.f,48.f})
    {
        FVector P=From+Headings[H]*Radius;
        if(FMath::Abs(P.Y)>FrontlineBattlefield::HalfFrontage-35.f||FVector::DistSquared2D(From,P)>FMath::Square(LocalRadius))continue;
        P.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,P);
        FVector Floor=P;Floor.X=FrontlineFieldV32::TrenchCentreX(Squad->Trench,P.Y);
        if(P.Z-FrontlineInfantryV45::SurfaceHeight(U->Battle,Floor)>FloorHeightAllowance||!LegClear(From,P))continue;
        bool Duplicate=false;for(const FVector& Existing:Nodes)if(FVector::DistSquared2D(Existing,P)<FMath::Square(.5f)){Duplicate=true;break;}
        if(!Duplicate)Nodes.Add(P);
    }
    constexpr int32 Limit=160;
    if(Nodes.Num()>Limit)return false;
    float Cost[Limit];int32 Parent[Limit];bool Closed[Limit];int8 Edge[Limit][Limit];
    for(int32 I=0;I<Limit;++I){Cost[I]=MAX_flt;Parent[I]=INDEX_NONE;Closed[I]=false;for(int32 J=0;J<Limit;++J)Edge[I][J]=-1;}
    Cost[0]=0.f;
    const auto Link=[&](int32 A,int32 B)
    {
        if(A==B||B==INDEX_NONE||(!GoalInScope&&B==1)||Closed[B]||FVector::DistSquared2D(Nodes[A],Nodes[B])>FMath::Square(72.f))return;
        const float CandidateCost=Cost[A]+float(FVector::Dist2D(Nodes[A],Nodes[B]));if(CandidateCost>=Cost[B])return;
        if(Edge[A][B]<0){++Out.TestedEdges;Edge[A][B]=LegClear(Nodes[A],Nodes[B])?1:0;}
        if(Edge[A][B]>0){Cost[B]=CandidateCost;Parent[B]=A;}
    };
    for(int32 Iteration=0;Iteration<Nodes.Num();++Iteration)
    {
        int32 Current=INDEX_NONE;float Priority=MAX_flt;
        for(int32 I=0;I<Nodes.Num();++I)if(!Closed[I]&&Cost[I]<MAX_flt)
        {const float Value=Cost[I]+float(FVector::Dist2D(Nodes[I],Destination));if(Value<Priority){Priority=Value;Current=I;}}
        if(Current==INDEX_NONE)break;Closed[Current]=true;++Out.Visited;
        if(GoalInScope&&Current==1)break;
        if(Current==0||Current>=FirstTangent)for(int32 I=1;I<Nodes.Num();++I)Link(Current,I);
        else
        {
            Link(Current,1);for(int32 I=FirstTangent;I<Nodes.Num();++I)Link(Current,I);
            for(int32 Row=0;Row<Rows;++Row)for(int32 Column=0;Column<7;++Column)if(Grid[Row][Column]==Current)
                for(int32 DY=-1;DY<=1;++DY)for(int32 DX=-1;DX<=1;++DX)
                    if(Row+DY>=0&&Row+DY<Rows&&Column+DX>=0&&Column+DX<7)Link(Current,Grid[Row+DY][Column+DX]);
        }
    }
    int32 Station=Closed[1]?1:INDEX_NONE;Out.OriginalGoalReachable=Closed[1];
    if(Station==INDEX_NONE)
    {
        float SafeCost=MAX_flt;
        for(int32 I=2;I<Nodes.Num();++I)if(Closed[I]&&Parent[I]!=INDEX_NONE&&Cost[I]<SafeCost&&ReplacementClear(Nodes[I]))
        {SafeCost=Cost[I];Station=I;}
    }
    Out.Nodes=Nodes.Num();Out.TangentNodes=Nodes.Num()-FirstTangent;
    if(Station==INDEX_NONE)return false;
    Out.Goal=Nodes[Station];Out.Cost=Cost[Station];
    for(int32 I=Station;I!=0&&I!=INDEX_NONE;I=Parent[I])Out.Path.Insert(Nodes[I],0);
    return !Out.Path.IsEmpty();
}
}

bool FrontlineArmourYieldHasCheckedRouteV84411(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden()||FrontlineUseLegacyArmourYieldV84411()
        ||WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* F=Fields.Find(U->Battle.Get());
    const auto* Squad=U->Battle->FindSquad(U->SquadId);
    if(!F||F->Encounter!=U->Battle->GetEncounterId()||!Squad||Squad->bAdvancing
        ||!U->Battle->Trenches.IsValidIndex(Squad->Trench))return false;
    const auto* Lease=F->ArmourYieldV8218.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    const auto* Tank=Lease?Lease->Tank.Get():nullptr;
    if(!IsValid(Tank)||!Tank->IsOperational()||Tank->IsHidden()||Tank->Team!=U->Team)return false;
    const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld());
    TArray<const AFrontlineTank*,TInlineAllocator<8>> Armour;
    const FVector From=U->Feet();
    if(Fleet)for(const AFrontlineTank* Other:Fleet->GetVehicles())
        if(IsValid(Other)&&Other->IsOperational()&&!Other->IsHidden()
            &&FVector::DistSquared2D(From,Other->GetActorLocation())<FMath::Square(3600.f))Armour.Add(Other);
    const auto* Route=F->BankDetoursV81.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(Route&&Route->Active&&Route->Trench==Squad->Trench
        &&FVector::DistSquared2D(Route->Goal,Lease->Destination)<=FMath::Square(.01f))
    {
        FVector Previous=From;bool Clear=true;
        if(Route->FloorPathV84411.IsValidIndex(Route->FloorPathIndexV84411))
        {
            for(int32 I=Route->FloorPathIndexV84411;I<Route->FloorPathV84411.Num();++I)
            {const FVector Next=Route->FloorPathV84411[I];if(!HeldYieldLegClearV84411(U,F,Armour,Previous,Next)){Clear=false;break;}Previous=Next;}
        }
        else Clear=HeldYieldLegClearV84411(U,F,Armour,From,Route->Waypoint)
            &&HeldYieldLegClearV84411(U,F,Armour,Route->Waypoint,Lease->Destination);
        if(Clear)return true;
    }
    return HeldYieldLegClearV84411(U,F,Armour,From,Lease->Destination);
}

bool FrontlineTrenchMotionV8444::ReadBankRoute(const AWarfareFrontlineSoldier* U,FBankRouteEvidence& Out)
{
    Out=FBankRouteEvidence();
    if(!U||!U->Battle)return false;
    const auto* F=Fields.Find(U->Battle.Get());
    if(!F||F->Encounter!=U->Battle->GetEncounterId())return false;
    if(const auto* E=F->BankEventsV8444.Find(const_cast<AWarfareFrontlineSoldier*>(U)))
    {
        Out.LastInvalidation=E->Reason;Out.InvalidatedDecision=E->Decision;
        Out.InvalidatedGoal=E->Goal;Out.ReplacementGoal=E->Replacement;
        Out.InvalidatedAt=E->At;Out.InvalidatingAttemptSeconds=E->AttemptSeconds;
        Out.HeldAtInvalidation=E->Held;Out.ValidatedAtInvalidation=E->Validated;
    }
    const auto* R=F->BankDetoursV81.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!R)return Out.InvalidatedDecision!=0;
    Out.Active=R->Active;Out.Decision=R->DecisionV8444;Out.Trench=R->Trench;
    Out.Goal=R->Goal;Out.Waypoint=R->Waypoint;Out.ProgressFeet=R->ProgressFeet;Out.LastAttemptGoal=R->LastAttemptGoalV8444;
    Out.AttemptAt=R->AttemptAtV8444;Out.StalledSeconds=R->StalledSecondsV8444;Out.TotalAttemptSeconds=R->TotalAttemptSecondsV8444;
    return true;
}

bool FrontlineReadArmourYieldV84411(const AWarfareFrontlineSoldier* U,FVector& Destination)
{
    Destination=FVector::ZeroVector;
    FString ReviewUserV86;
    const bool PrivateV86=U&&U->Battle&&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(ReviewUserV86);
    if(!U||!U->Battle||!U->Battle->IsProgressionVolatile()
        ||(!FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))&&!FrontlineMovementFocusedV84413::Allowed(U->Battle.Get())&&!PrivateV86)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
    const auto* F=Fields.Find(U->Battle.Get());
    if(!F||F->Encounter!=U->Battle->GetEncounterId())return false;
    const auto* Lease=F->ArmourYieldV8218.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!Lease)return false;
    Destination=Lease->Destination;return true;
}

bool FrontlineReadHeldArmourGoalV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,FVector& Goal)
{
    if(!U||!U->Battle||!IsValid(Tank)||!Tank->IsOperational()||Tank->Team!=U->Team
        ||FrontlineUseLegacyArmourYieldV84411()||WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* Held=U->Battle->FindSquad(U->SquadId);
    const auto* F=Fields.Find(U->Battle.Get());
    if(!Held||Held->bAdvancing||!U->Battle->Trenches.IsValidIndex(Held->Trench)
        ||!F||F->Encounter!=U->Battle->GetEncounterId())return false;
    const auto* Lease=F->ArmourYieldV8218.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!Lease||Lease->Tank.Get()!=Tank||Lease->OriginalTrenchV86!=Held->Trench||Lease->Destination.ContainsNaN())return false;
    Goal=Lease->Destination;return true;
}

TSharedPtr<FJsonObject> FrontlineMobilityV41::DescribeArmourYieldLeaseV87(const AWarfareFrontlineSoldier* U)
{
#if WITH_EDITOR
    FString User;
    if(!U||!U->Battle||!IsInGameThread()||!U->Battle->IsProgressionVolatile()||WarfareFRRUCampaignV82::Active(U->Battle)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV87MGLeaseOnly"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!FrontlineUltimateArchiveV8215::ReviewUserDir(User))return nullptr;
    auto J=MakeShared<FJsonObject>();const auto* B=U->Battle.Get();const int32 Dir=B->GetTeamDirection(U->Team);
    J->SetStringField(TEXT("scope"),TEXT("Pure read of the current encounter's existing armour-yield lease and bank-route map; no destination service or field refresh was invoked."));
    J->SetNumberField(TEXT("team"),U->Team);J->SetNumberField(TEXT("direction"),Dir);J->SetNumberField(TEXT("elapsed"),B->Elapsed);
    const auto* F=Fields.Find(U->Battle.Get());const bool Current=F&&F->Encounter==B->GetEncounterId();J->SetBoolField(TEXT("current_encounter_field"),Current);
    const auto* Lease=Current?F->ArmourYieldV8218.Find(const_cast<AWarfareFrontlineSoldier*>(U)):nullptr;
    J->SetBoolField(TEXT("lease_present"),Lease!=nullptr);if(!Lease)return J;
    const auto Point=[](const FVector& V){auto P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("x"),V.X);P->SetNumberField(TEXT("y"),V.Y);P->SetNumberField(TEXT("z"),V.Z);return P;};
    J->SetObjectField(TEXT("destination"),Point(Lease->Destination));J->SetObjectField(TEXT("original_feet"),Point(Lease->OriginalFeetV86));
    J->SetNumberField(TEXT("original_body_front_projected_cm"),Lease->OriginalBodyFrontV86);J->SetNumberField(TEXT("original_trench"),Lease->OriginalTrenchV86);
    J->SetNumberField(TEXT("selected_side"),Lease->SideV836);J->SetBoolField(TEXT("path_blocked"),Lease->PathBlockedV84411);
    if(const auto* Tank=Lease->Tank.Get())
    {J->SetNumberField(TEXT("tank_serial"),Tank->Serial);J->SetNumberField(TEXT("tank_team"),Tank->Team);J->SetBoolField(TEXT("tank_operational"),Tank->IsOperational());
        J->SetObjectField(TEXT("tank_position"),Point(Tank->GetActorLocation()));J->SetNumberField(TEXT("tank_rear_projected_cm"),Tank->RearProgressV49());}
    const auto* Route=F->BankDetoursV81.Find(const_cast<AWarfareFrontlineSoldier*>(U));J->SetBoolField(TEXT("bank_route_present"),Route!=nullptr);
    if(Route){J->SetBoolField(TEXT("bank_route_active"),Route->Active);J->SetObjectField(TEXT("bank_route_goal"),Point(Route->Goal));
        J->SetObjectField(TEXT("bank_route_waypoint"),Point(Route->Waypoint));J->SetNumberField(TEXT("bank_route_decision"),Route->DecisionV8444);
        J->SetNumberField(TEXT("bank_route_floor_path_index"),Route->FloorPathIndexV84411);}
    return J;
#else
    return nullptr;
#endif
}

bool FrontlineReadHeldArmourLegV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,FVector& Goal,FVector& Leg)
{
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden()||!IsValid(Tank)||Tank->Team!=U->Team
        ||FrontlineUseLegacyArmourYieldV84411()||WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* Held=U->Battle->FindSquad(U->SquadId);
    if(!Held||Held->bAdvancing||!U->Battle->Trenches.IsValidIndex(Held->Trench))return false;
    const bool Leased=FrontlineReadHeldArmourGoalV86(U,Tank,Goal);
    if(!Leased)Goal=U->Battle->SoldierDestination(U);
    Leg=Goal;
    const auto* F=Fields.Find(U->Battle.Get());
    const auto* Route=F&&F->Encounter==U->Battle->GetEncounterId()?F->BankDetoursV81.Find(const_cast<AWarfareFrontlineSoldier*>(U)):nullptr;
    // Read the already selected physical first leg. Do not invent or modify
    // a bay, lease, command or route for the cooperative opening.
    if(Route&&Route->Active&&Route->Trench==Held->Trench&&!Route->Goal.ContainsNaN()&&!Route->Waypoint.ContainsNaN()
        &&(!Leased||FVector::DistSquared2D(Route->Goal,Goal)<=FMath::Square(.01f)))
    {Goal=Route->Goal;Leg=Route->Waypoint;}
    return !Goal.ContainsNaN()&&!Leg.ContainsNaN();
}

bool FrontlineReadBlockedArmourLegsV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,const FVector& Goal,TArray<FVector>& Legs)
{
    Legs.Reset();FVector CheckedGoal,Guided;
    if(!FrontlineReadHeldArmourLegV86(U,Tank,CheckedGoal,Guided)
        ||FVector::DistSquared2D(CheckedGoal,Goal)>FMath::Square(.01f))return false;
    const auto* F=Fields.Find(U->Battle.Get());
    const auto* Held=U->Battle->FindSquad(U->SquadId);
    const auto* Tried=F&&F->Encounter==U->Battle->GetEncounterId()?F->BlockedArmourLegsV86.Find(const_cast<AWarfareFrontlineSoldier*>(U)):nullptr;
    if(!Tried||!Held||Tried->Trench!=Held->Trench||U->Battle->Elapsed-Tried->Time<-.001f
        ||U->Battle->Elapsed-Tried->Time>.25f
        ||FVector::DistSquared2D(U->Feet(),Tried->From)>FMath::Square(.01f)
        ||FVector::DistSquared2D(Goal,Tried->Goal)>FMath::Square(.01f))return false;
    for(const auto& Candidate:Tried->Candidates)if(Candidate.Tank.Get()==Tank&&!Candidate.Leg.ContainsNaN())Legs.Add(Candidate.Leg);
    return !Legs.IsEmpty();
}

bool FrontlinePlanCooperativeFloorRouteV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,
    const FVector& Goal,const FTransform* PreviewPose,TArray<FVector>& Path,bool GeometryOnly=false)
{
    Path.Reset();
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden()||!IsValid(Tank)||!Tank->IsOperational()||Tank->Team!=U->Team
        ||Goal.ContainsNaN()||(PreviewPose&&PreviewPose->ContainsNaN())||FrontlineUseLegacyArmourYieldV84411()
        ||WarfareFRRUCampaignV82::Active(U->Battle))return false;
    const auto* Held=U->Battle->FindSquad(U->SquadId);
    const float Radius=SearchRadius-Clearance-.5f;
    if(!Held||Held->bAdvancing||!U->Battle->Trenches.IsValidIndex(Held->Trench)
        ||FVector::DistSquared2D(U->Feet(),Goal)>FMath::Square(Radius))return false;
    const auto* F=Fields.Find(U->Battle.Get());
    TArray<const AFrontlineTank*,TInlineAllocator<8>> Armour;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(const AFrontlineTank* Other:Fleet->GetVehicles())
        if(IsValid(Other)&&Other->IsOperational()&&!Other->IsHidden()&&(!GeometryOnly||Other!=Tank)
            &&FVector::DistSquared2D(U->Feet(),Other->GetActorLocation())<FMath::Square(3600.f))Armour.Add(Other);
    TArray<FVector,TInlineAllocator<7>> Headings;
    float Requested=24.f;
    if(const auto* Tried=F?F->BlockedArmourLegsV86.Find(const_cast<AWarfareFrontlineSoldier*>(U)):nullptr;
        Tried&&F->Encounter==U->Battle->GetEncounterId()&&Tried->Trench==Held->Trench
        &&FVector::DistSquared2D(Tried->From,U->Feet())<=FMath::Square(.01f)
        &&FVector::DistSquared2D(Tried->Goal,Goal)<=FMath::Square(.01f)&&U->Battle->Elapsed-Tried->Time<=.25f)
    {
        Requested=Tried->Requested;
        for(const auto& Candidate:Tried->Candidates)if(Candidate.Tank.Get()==Tank&&Headings.Num()<7)
            Headings.Add((Candidate.Leg-U->Feet()).GetSafeNormal2D());
    }
    const auto NodeClear=[&](const FVector& P)
    {
        for(const AFrontlineTank* Other:Armour)
            if(Other->BlocksInfantryAtV8218(P,Other->GetActorLocation(),Other->GetActorRotation().Yaw,34.f,0.f,U,nullptr,
                Other==Tank?PreviewPose:nullptr))return false;
        return true;
    };
    if(HeldYieldLegClearV84411(U,F,Armour,U->Feet(),Goal,nullptr,Tank,PreviewPose))
    {Path.Add(Goal);return true;}
    FHeldFloorGraphV86 Graph;
    const bool Found=FindHeldFloorGraphV86(U,Goal,Requested,Radius,true,MakeArrayView(Headings),NodeClear,
        [&](const FVector& Start,const FVector& End){return HeldYieldLegClearV84411(U,F,Armour,Start,End,nullptr,Tank,PreviewPose);},
        [](const FVector&){return false;},Graph); // Cooperative execution preserves the original goal; no replacement-station fallback.
    if(Found&&Graph.OriginalGoalReachable)Path=MoveTemp(Graph.Path);
    return !Path.IsEmpty();
}

bool FrontlineCooperativeFloorRouteClearV86(const AWarfareFrontlineSoldier* U,const AFrontlineTank* Tank,
    const TArray<FVector>& Path,const FTransform* PreviewPose=nullptr)
{
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden()||!IsValid(Tank)||Path.IsEmpty()
        ||(PreviewPose&&PreviewPose->ContainsNaN()))return false;
    const auto* F=Fields.Find(U->Battle.Get());
    TArray<const AFrontlineTank*,TInlineAllocator<8>> Armour;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(const AFrontlineTank* Other:Fleet->GetVehicles())
        if(IsValid(Other)&&Other->IsOperational()&&!Other->IsHidden()
            &&FVector::DistSquared2D(U->Feet(),Other->GetActorLocation())<FMath::Square(3600.f))Armour.Add(Other);
    FVector Previous=U->Feet();
    for(const FVector& Next:Path)
    {if(!HeldYieldLegClearV84411(U,F,Armour,Previous,Next,nullptr,Tank,PreviewPose))return false;Previous=Next;}
    return true;
}

bool FrontlineArmourYieldNeedsExactArrivalV86(const AWarfareFrontlineSoldier* U,const FVector& Goal)
{
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden()||Goal.ContainsNaN()
        ||FrontlineUseLegacyArmourYieldV84411()||WarfareFRRUCampaignV82::Active(U->Battle)
        ||FVector::DistSquared2D(U->Feet(),Goal)<=FMath::Square(.01f))return false;
    const auto* F=Fields.Find(U->Battle.Get());if(!F||F->Encounter!=U->Battle->GetEncounterId())return false;
    const auto* Lease=F->ArmourYieldV8218.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!Lease||FVector::DistSquared2D(Lease->Destination,Goal)>FMath::Square(.01f))return false;
    const auto* Tank=Lease->Tank.Get();FVector CheckedGoal,OpeningGuide;
    return FrontlineReadHeldArmourGoalV86(U,Tank,CheckedGoal)
        &&(Tank->ReadCooperativeOpeningGuideV86(U,Goal,OpeningGuide)||!Tank->IsInfantryYieldNextStepClearV86(U,U->Feet()));
}

TSharedPtr<FJsonObject> FrontlineReadArmourStepV84411(const AWarfareFrontlineSoldier* U)
{
    FString ReviewUserV86;
    const bool PrivateV86=U&&U->Battle&&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(ReviewUserV86);
    if(!U||!U->Battle||!U->Battle->IsProgressionVolatile()
        ||(!FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))&&!FrontlineMovementFocusedV84413::Allowed(U->Battle.Get())&&!PrivateV86)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return nullptr;
    const auto* F=Fields.Find(U->Battle.Get());
    if(!F||F->Encounter!=U->Battle->GetEncounterId())return nullptr;
    const auto* Receipt=F->StepReceiptsV84411.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    return Receipt?*Receipt:nullptr;
}

bool FrontlineReadCommittedBodyV84413(const AWarfareFrontlineSoldier* U,FVector& Position)
{
    if(!FrontlineMovementFocusedV84413::Allowed(U?U->Battle.Get():nullptr))return false;
    const auto* F=Fields.Find(U->Battle.Get());
    if(!F||F->Encounter!=U->Battle->GetEncounterId())return false;
    const auto* I=F->Index.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!I||!F->People.IsValidIndex(*I))return false;
    Position=F->People[*I].Position;return true;
}

TSharedPtr<FJsonObject> FrontlineReadFullTrenchContactV84413(const AWarfareFrontlineSoldier* U)
{
    if(!FrontlineMovementFocusedV84413::Allowed(U?U->Battle.Get():nullptr))return nullptr;
    const auto* F=Fields.Find(U->Battle.Get());if(!F||F->Encounter!=U->Battle->GetEncounterId())return nullptr;
    const auto* Pass=F->Passing.Find(const_cast<AWarfareFrontlineSoldier*>(U));
    if(!Pass||Pass->FullTrenchEscapeV84413==INDEX_NONE)return nullptr;
    auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("trench"),Pass->FullTrenchEscapeV84413);
    R->SetStringField(TEXT("contact_origin"),Pass->ContactOriginV84413.ToString());R->SetNumberField(TEXT("actual_reverse_cm"),Pass->ActualBackwardTravelV84413);
    R->SetNumberField(TEXT("reverse_limit_cm"),Clearance);return R;
}

void FrontlineMobilityV41::Reset(AWarfareFrontlineGameMode* B)
{if(auto* F=Fields.Find(B))ClearTransformBindings(*F);Fields.Remove(B);}
void FrontlineMobilityV41::InvalidateRoster(AWarfareFrontlineGameMode* B)
{if(auto* F=Fields.Find(B))F->Frame=MAX_uint64;}
bool FrontlineMobilityV41::VisitSweptBodies(const AWarfareFrontlineSoldier* U,const FVector& From,const FVector& To,double Padding,
    TFunctionRef<bool(const AWarfareFrontlineSoldier*)> Visit)
{
    if(!U||!U->Battle||From.ContainsNaN()||To.ContainsNaN()||!FMath::IsFinite(Padding)||Padding<0)return false;
    auto& F=Field(U->Battle);
    const FIntPoint First=Cell(FVector(FMath::Min(From.X,To.X)-Padding,FMath::Min(From.Y,To.Y)-Padding,0));
    const FIntPoint Last=Cell(FVector(FMath::Max(From.X,To.X)+Padding,FMath::Max(From.Y,To.Y)+Padding,0));
    for(int64 Y=First.Y;Y<=Last.Y;++Y)for(int64 X=First.X;X<=Last.X;++X)
        if(const auto* Bucket=F.Cells.Find(FIntPoint(int32(X),int32(Y))))
            for(int32 Index:*Bucket)if(const auto* Other=F.People[Index].Unit.Get();Other&&!Visit(Other))return false;
    return true;
}
void FrontlineMobilityV41::TraceNextStepV64(AWarfareFrontlineSoldier* U)
{
    if(U&&U->Battle&&U->Battle->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV64Review"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))TraceUnitV64=U;
}

bool FrontlineMobilityV41::ArmourYieldDestinationV8218(AWarfareFrontlineSoldier* U,FVector& Destination)
{
    if(!U||!U->Battle||!U->IsAlive()||U->IsHidden())return false;
    auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld());if(!Fleet)return false;
    auto& F=Field(U->Battle);
    if(auto* Cached=F.ArmourYieldV8218.Find(U))
    {
        auto* Tank=Cached->Tank.Get();FVector Check=FVector::ZeroVector;
        // Keep the chosen side. If a turn places the goal inside the actual
        // hull, the current physical safe edge supplies a replacement.
        const bool ValidTank=Tank&&Tank->IsOperational()&&Tank->Team==U->Team;
        const bool StableYieldV84411=!FrontlineUseLegacyArmourYieldV84411()
            &&!WarfareFRRUCampaignV82::Active(U->Battle);
        const auto* HeldV86=U->Battle->FindSquad(U->SquadId);
        const bool OriginalHeldAssignmentV86=HeldV86&&!HeldV86->bAdvancing
            &&HeldV86->Trench==Cached->OriginalTrenchV86&&U->Battle->Trenches.IsValidIndex(HeldV86->Trench);
        const bool CurrentStationSafeV86=ValidTank&&StableYieldV84411&&OriginalHeldAssignmentV86
            &&Tank->IsInfantryYieldStationSafeV86(U,Cached->Destination);
        const int32 DirectionV86=U->Battle->GetTeamDirection(U->Team);
        const FVector ReturnBayV86=OriginalHeldAssignmentV86?U->Battle->SoldierDestination(U):Cached->OriginalFeetV86;
        const float ReturnBodyReachV86=FMath::Max(34.f,Cached->OriginalBodyFrontV86-float(Cached->OriginalFeetV86.X)*DirectionV86);
        const float ReturnBodyFrontV86=FMath::Max(Cached->OriginalBodyFrontV86,float(ReturnBayV86.X)*DirectionV86+ReturnBodyReachV86);
        // A clear lateral shoulder is not proof that the real rear has passed
        // the original firing bay. A forecast of a future turn may fail while
        // this current station is entirely safe; sending the man back then
        // re-seals the supported hull. Retain the same assignment until its
        // real rear clears the original/actual return bay, with ordinary body
        // and floor steps still owning both actors' movement.
        const bool OriginalBayStillPassingV86=CurrentStationSafeV86
            &&Tank->RearProgressV49()<=ReturnBodyFrontV86+3.f;
        // An arrived shoulder can be empty in the current pose yet seal the
        // next supported hull on a bank. AtDestination then produces no Step
        // and hence no PathBlocked receipt. Reopen that exact witness through
        // the normal fully qualified shoulder search, retaining the original
        // return bay and all swept body/floor movement checks.
        const bool NextStationClearV86=!ValidTank||!StableYieldV84411
            ||(Tank->IsInfantryYieldNextStepClearV86(U,Cached->Destination)
                &&Tank->IsInfantryRecoveryStationClearV86(U,Cached->Destination));
        const auto* CheckedRouteV84411=F.BankDetoursV81.Find(U);
        const bool ActiveCheckedRouteV84411=CheckedRouteV84411&&CheckedRouteV84411->Active
            &&FVector::DistSquared2D(CheckedRouteV84411->Goal,Cached->Destination)<=FMath::Square(.01f);
        const bool BlockedRouteNeedsSearchV84411=Cached->PathBlockedV84411&&!ActiveCheckedRouteV84411;
        if(StableYieldV84411&&BlockedRouteNeedsSearchV84411)Cached->PathBlockedV84411=false;
        // The exact current and next supported poses suffice for retaining
        // this original bay lease. Avoid its unused whole-future proof before
        // this early return; acquisition and replacement still require it.
        if(StableYieldV84411&&!BlockedRouteNeedsSearchV84411&&OriginalBayStillPassingV86&&NextStationClearV86)
        {Destination=Cached->Destination;return true;}
        const bool HullStillPassing=ValidTank&&Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation()
            +FRotator(0,Tank->GetActorRotation().Yaw,0).Vector()*340.f,Tank->GetActorRotation().Yaw,120.f,0.f,U);
        const bool ActualTailStillPassingV84411=ValidTank&&StableYieldV84411
            &&Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation(),Tank->GetActorRotation().Yaw,120.f,0.f,U);
        const bool CachedGoalClearV84411=ValidTank&&StableYieldV84411
            &&Tank->IsInfantryYieldGoalClearV84411(U,Cached->Destination)
            &&FrontlineArmourYieldGoalBodyClearV84411(U,Cached->Destination);
        const bool RecoveryHoldingV84411=ValidTank&&StableYieldV84411&&CachedGoalClearV84411
            &&Tank->HasInfantryYieldRecoveryV84411(U);
        // Either passing envelope already retains this lease. A still-safe
        // cached goal cannot be changed below, so a new shoulder search would
        // have no effect on the decision or destination in this exact case.
        if(StableYieldV84411&&!BlockedRouteNeedsSearchV84411&&NextStationClearV86
            &&(CachedGoalClearV84411&&(HullStillPassing||ActualTailStillPassingV84411||RecoveryHoldingV84411)))
        {Destination=Cached->Destination;return true;}
        const bool CurrentPoint=ValidTank&&Tank->InfantryYieldPointV8218(U,Check);
        if(StableYieldV84411&&ValidTank&&!Tank->IsInfantryYieldStationSafeV86(U,Cached->Destination)&&!CurrentPoint)
        {F.ArmourYieldV8218.Remove(U);return false;}
        // The endpoint can stay empty after its actual approach is sealed by
        // the hull. Only a failed real held-yield step with no checked two-leg
        // route can reopen that lease, and only a fully checked physical leg
        // to the replacement can change its shoulder. A merely nearer or
        // different-side empty endpoint changes nothing.
        const bool ReplacementPathClearV84411=StableYieldV84411&&BlockedRouteNeedsSearchV84411
            &&CurrentPoint&&FrontlineArmourYieldDirectPathClearV84411(U,Check);
        if(CurrentPoint||HullStillPassing||ActualTailStillPassingV84411||RecoveryHoldingV84411||OriginalBayStillPassingV86)
        {
            // Walking around a curved bank changes the man's current depth.
            // Recomputing a still-safe goal from that depth cancels the checked
            // two-leg route every frame. Only actual hull contact relocates it.
            const bool NeedsNewGoalV84411=!StableYieldV84411||!CachedGoalClearV84411||!NextStationClearV86;
            if(ReplacementPathClearV84411||(StableYieldV84411&&(!CachedGoalClearV84411||!NextStationClearV86)&&CurrentPoint))
            {
                // A verified replacement is a new lease when the old station
                // has become unsafe. Its old shoulder cannot veto the only
                // physically clear escape; safe leases never change side here.
                const float NewSide=FVector::DotProduct(Check-U->Feet(),Tank->GetActorRightVector());
                Cached->Destination=Check;Cached->SideV836=NewSide>0.f?1.f:NewSide<0.f?-1.f:0.f;Cached->PathBlockedV84411=false;
            }
            else if(CurrentPoint&&NeedsNewGoalV84411&&(Cached->SideV836==0.f
                ||FVector::DotProduct(Check-U->Feet(),Tank->GetActorRightVector())*Cached->SideV836>0.f))Cached->Destination=Check;
            Destination=Cached->Destination;return true;
        }
        F.ArmourYieldV8218.Remove(U);
    }
    for(AFrontlineTank* Tank:Fleet->GetVehicles())if(IsValid(Tank)&&Tank->InfantryYieldPointV8218(U,Destination))
    {
        const float Side=FVector::DotProduct(Destination-U->Feet(),Tank->GetActorRightVector());
        FArmourYieldV8218 Lease;Lease.Tank=Tank;Lease.Destination=Destination;Lease.SideV836=Side>0.f?1.f:Side<0.f?-1.f:0.f;
        Lease.OriginalFeetV86=U->Feet();const int32 Direction=U->Battle->GetTeamDirection(U->Team);
        Lease.OriginalBodyFrontV86=float(Lease.OriginalFeetV86.X)*Direction+34.f;
        if(U->Soldier){const FBox ActualBody=U->Soldier->Bounds.GetBox();if(ActualBody.IsValid)
            Lease.OriginalBodyFrontV86=FMath::Max(Lease.OriginalBodyFrontV86,float(Direction>0?ActualBody.Max.X:-ActualBody.Min.X));}
        if(const auto* Held=U->Battle->FindSquad(U->SquadId);Held&&!Held->bAdvancing)Lease.OriginalTrenchV86=Held->Trench;
        F.ArmourYieldV8218.Add(U,Lease);return true;
    }
    return false;
}

bool FrontlineMobilityV41::AllowsSupportedPlacementV8448(const AWarfareFrontlineSoldier* U,const FVector& From,const FVector& To)
{
    if(!U||!U->Battle||WarfareFRRUCampaignV82::Active(U->Battle))return true;
    if(From.ContainsNaN()||To.ContainsNaN())return false;
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(const AFrontlineTank* V:Fleet->GetVehicles())
        if(IsValid(V)&&V->IsOperational()&&!V->IsHidden()
            &&FVector::DistSquared2D(From,V->GetActorLocation())<FMath::Square(3600.f)
            &&V->InfantrySweepClearanceV8218(From,To,34.f,U)<1.f)return false;
    return true;
}
FVector FrontlineMobilityV41::Step(AWarfareFrontlineSoldier* U,const FVector& Destination,float Distance,bool Advancing,float AttemptSeconds,FVector* RequiredFacing)
{
    if(RequiredFacing)*RequiredFacing=FVector::ZeroVector;
    if(!U||!U->Battle)return FVector::ZeroVector;
    const FVector From=U->Feet();if(Destination.ContainsNaN()||!FMath::IsFinite(Distance)||Distance<=0)return From;
    if(WarfareFRRUCampaignV82::Active(U->Battle))
    {
        // The national route has already selected a checked local guide.
        // Keep one steering authority; the legacy planner must not replace
        // that portal with another body's unrelated preferred passing side.
        const FVector Next=WarfareFRRUWorldV82::MovementStep(U,Destination,Distance);
        const FVector Direction=(Next-From).GetSafeNormal2D();
        if(RequiredFacing&&!Direction.IsNearlyZero())
        {
            *RequiredFacing=Direction;
            if(FVector::DotProduct(Direction,U->GetActorForwardVector())<.5f)return From;
        }
        return Next;
    }
    const bool BoundedRetryV86=!FrontlineUseLegacyArmourYieldV84411()&&!WarfareFRRUCampaignV82::Active(U->Battle);
    if(BoundedRetryV86)++StepInvocationsV86.FindOrAdd(U).Depth;
    ON_SCOPE_EXIT
    {
        if(BoundedRetryV86)if(auto* Invocation=StepInvocationsV86.Find(U))
            if(--Invocation->Depth==0)StepInvocationsV86.Remove(U);
    };
    auto& F=Field(U->Battle);
    static const bool ReviewBankV8444=FParse::Param(FCommandLine::Get(),TEXT("WFV8444Review"))||FParse::Param(FCommandLine::Get(),TEXT("WFV8447Review"));
    static const bool ReviewStepV84411=[]
    {
        FString User;
        return (FParse::Param(FCommandLine::Get(),TEXT("WFV84411Review"))
            ||(FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))&&FParse::Param(FCommandLine::Get(),TEXT("WFV86ArmourTrench"))
                &&FrontlineUltimateArchiveV8215::ReviewUserDir(User)))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
    }();
    TSharedPtr<FJsonObject> ReceiptV84411;
    if((ReviewStepV84411||FrontlineMovementFocusedV84413::Allowed(U->Battle.Get()))&&U->Battle->IsProgressionVolatile())
    {
        ReceiptV84411=MakeShared<FJsonObject>();F.StepReceiptsV84411.Add(U,ReceiptV84411);
        ReceiptV84411->SetNumberField(TEXT("at_host_seconds"),U->Battle->Elapsed);
        ReceiptV84411->SetStringField(TEXT("from"),From.ToString());ReceiptV84411->SetStringField(TEXT("destination"),Destination.ToString());
        ReceiptV84411->SetNumberField(TEXT("requested_cm"),Distance);ReceiptV84411->SetBoolField(TEXT("advancing"),Advancing);
    }
    const auto RetrySameStepV86=[&](const FVector& Goal,bool MovingForward)
    {
        if(BoundedRetryV86)
        {
            auto* Invocation=StepInvocationsV86.Find(U);
            if(!Invocation||Invocation->Retried)
            {
                if(auto* Rejected=F.BankDetoursV81.Find(U))
                {Rejected->Active=false;Rejected->FloorPathV84411.Reset();Rejected->FloorPathIndexV84411=0;
                    Rejected->RetryAt=FMath::Max(Rejected->RetryAt,U->Battle->Elapsed+.5f);}
                if(auto* Lease=F.ArmourYieldV8218.Find(U))Lease->PathBlockedV84411=true;
                if(ReceiptV84411)ReceiptV84411->SetStringField(TEXT("result_gate"),TEXT("same_step_retry_exhausted"));
                return From;
            }
            Invocation->Retried=true;
        }
        return Step(U,Goal,Distance,MovingForward,AttemptSeconds,RequiredFacing);
    };
    const auto* Squad=U->Battle->FindSquad(U->SquadId);
    const bool FiringBay=U->IsMachineGunner()?FrontlineMachinegunsV30::HasValidatedTrenchBayV69(U)
        :FrontlineInfantryV45::HasValidatedFiringStepV831(U,Destination);
    auto* YieldLeaseV84411=F.ArmourYieldV8218.Find(U);
    const auto* YieldTankV84411=YieldLeaseV84411?YieldLeaseV84411->Tank.Get():nullptr;
    // A physical yield goal belongs to the same held trench as the original
    // firing slot. It needs the checked two-leg bank route too; identifying
    // only the firing-slot coordinates denied that route at curved corners.
    const bool HeldYieldBayV84411=!FrontlineUseLegacyArmourYieldV84411()
        &&!WarfareFRRUCampaignV82::Active(U->Battle)
        &&!Advancing&&Squad&&!Squad->bAdvancing&&U->Battle->Trenches.IsValidIndex(Squad->Trench)
        &&IsValid(YieldTankV84411)&&YieldTankV84411->IsOperational()&&!YieldTankV84411->IsHidden()
        &&YieldTankV84411->Team==U->Team
        &&FVector::DistSquared2D(YieldLeaseV84411->Destination,Destination)<=FMath::Square(.01f);
    const bool ValidatedBay=FiringBay||HeldYieldBayV84411;
    auto* Detour=F.BankDetoursV81.Find(U);
    if(ReceiptV84411&&Detour&&Detour->LastPhysicalPlanV84411)
        ReceiptV84411->SetObjectField(TEXT("last_physical_route_plan"),Detour->LastPhysicalPlanV84411);
    if(Detour&&ReviewBankV8444)Detour->LastAttemptGoalV8444=Destination;
    // Keep a checked short route through its temporary outward leg. A real
    // goal/order change still releases it; ordinary marches do not acquire it.
    const bool HeldBay=!Advancing&&Squad&&!Squad->bAdvancing
        &&U->Battle->Trenches.IsValidIndex(Squad->Trench)
        &&ValidatedBay;
    // Tick passes the current foot height with its horizontal target. Support
    // integration owns Z; centimetres of slope cannot cancel the same XY route.
    const bool SameGoal=Detour&&FVector::DistSquared2D(Detour->Goal,Destination)<=FMath::Square(.01f);
    const bool RetainedBay=HeldBay&&Detour&&Detour->Active&&Detour->Trench==Squad->Trench&&SameGoal;
    // The two-leg route's body checks use this same nearby-body field. Keep a
    // yield goal and its full body margin inside that field; the original
    // firing-slot scope remains 110 cm. Waypoints are at most 72 cm away.
    const float LocalBayRadiusV84411=HeldYieldBayV84411?SearchRadius-Clearance-.5f:110.f;
    const bool YieldGoalInLocalScopeV84411=HeldYieldBayV84411
        &&FVector::DistSquared2D(From,Destination)<=FMath::Square(LocalBayRadiusV84411);
    // A leased shoulder can become remote while the passing hull redirects
    // the man. Keep the search local, rather than disabling it or connecting
    // a remote endpoint through an unchecked extension of the physical field.
    const bool LocalBay=HeldBay&&(RetainedBay||HeldYieldBayV84411
        ||FVector::DistSquared2D(From,Destination)<FMath::Square(LocalBayRadiusV84411));
    if(ReceiptV84411)
    {
        ReceiptV84411->SetBoolField(TEXT("held_yield"),HeldYieldBayV84411);ReceiptV84411->SetBoolField(TEXT("firing_bay"),FiringBay);
        ReceiptV84411->SetBoolField(TEXT("local_bay"),LocalBay);ReceiptV84411->SetBoolField(TEXT("retained_bay"),RetainedBay);
        ReceiptV84411->SetBoolField(TEXT("same_goal"),SameGoal);ReceiptV84411->SetNumberField(TEXT("local_radius_cm"),LocalBayRadiusV84411);
    }
    const auto RecordBankEvent=[&](FrontlineTrenchMotionV8444::EBankInvalidation Reason)
    {
        if(!ReviewBankV8444||!Detour)return;
        FBankEventV8444 E;E.Reason=Reason;E.Decision=Detour->DecisionV8444;
        E.Goal=Detour->Goal;E.Replacement=Destination;E.At=U->Battle->Elapsed;E.AttemptSeconds=AttemptSeconds;
        E.Held=Squad&&!Squad->bAdvancing&&U->Battle->Trenches.IsValidIndex(Squad->Trench);E.Validated=ValidatedBay;
        F.BankEventsV8444.Add(U,E);
    };
    if(Detour&&(!LocalBay||Detour->Trench!=Squad->Trench||!SameGoal))
    {
        using EReason=FrontlineTrenchMotionV8444::EBankInvalidation;
        const EReason Reason=!Squad||Advancing||Squad->bAdvancing||!U->Battle->Trenches.IsValidIndex(Squad->Trench)
            ||Detour->Trench!=Squad->Trench?EReason::AssignmentChanged:!ValidatedBay?EReason::BayUnvalidated:
            !SameGoal?EReason::GoalChanged:EReason::OutsideLocalScope;
        RecordBankEvent(Reason);F.BankDetoursV81.Remove(U);Detour=nullptr;
    }
    bool WaypointReachedV86=Detour&&Detour->Active&&FVector::DistSquared2D(From,Detour->Waypoint)<=FMath::Square(2.f);
    // A close but necessary bend cannot be skipped unless the real feet can
    // reach the next node through the same complete physical leg proof.
    if(WaypointReachedV86&&HeldYieldBayV84411&&Detour->FloorPathV84411.IsValidIndex(Detour->FloorPathIndexV84411+1))
        WaypointReachedV86=FrontlineArmourYieldDirectPathClearV84411(U,Detour->FloorPathV84411[Detour->FloorPathIndexV84411+1]);
    if(WaypointReachedV86)
    {
        RecordBankEvent(FrontlineTrenchMotionV8444::EBankInvalidation::WaypointReached);
        if(HeldYieldBayV84411&&Detour->FloorPathV84411.IsValidIndex(Detour->FloorPathIndexV84411+1))
        {++Detour->FloorPathIndexV84411;Detour->Waypoint=Detour->FloorPathV84411[Detour->FloorPathIndexV84411];}
        else Detour->Active=false;
    }
    bool BankProgressV8447=false;
    if(Detour&&Detour->Active)
    {
        if(FVector::DistSquared2D(From,Detour->ProgressFeet)>FMath::Square(.25f))
        {Detour->ProgressFeet=From;Detour->StalledSecondsV8444=0.f;BankProgressV8447=true;}
        // Count failed steering attempts, not time spent finishing the authored
        // pack/reload/deploy action before Step is called again.
        if(Detour->StalledSecondsV8444>=.75f)
        {RecordBankEvent(FrontlineTrenchMotionV8444::EBankInvalidation::AttemptStalled);Detour->Active=false;Detour->RetryAt=U->Battle->Elapsed;}
    }
    const bool NationalField=WarfareFRRUCampaignV82::Active(U->Battle);
    auto* ContactLegPassV84413=F.Passing.Find(U);
    auto* ContactLegBodyV84413=ContactLegPassV84413?ContactLegPassV84413->ContactLegBodyV84413.Get():nullptr;
    const auto* ContactLegSquadV84413=ContactLegBodyV84413?U->Battle->FindSquad(ContactLegBodyV84413->SquadId):nullptr;
    const int32 ContactLegTrenchV84413=ContactLegPassV84413?ContactLegPassV84413->FullTrenchEscapeV84413:INDEX_NONE;
    bool ContactLegSlotsV84413[FFrontlineTrench::Capacity]={};
    if(ContactLegBodyV84413)for(const auto& Resident:U->Battle->Squads)
        if(Resident.Id!=U->SquadId&&Resident.Team==U->Team&&Resident.Trench==ContactLegTrenchV84413&&Resident.LivingCount()>0
            &&Resident.Slot>=0&&Resident.Slot<FFrontlineTrench::Capacity)ContactLegSlotsV84413[Resident.Slot]=true;
    bool ContactLegFullV84413=ContactLegBodyV84413!=nullptr;
    for(bool Taken:ContactLegSlotsV84413)ContactLegFullV84413&=Taken;
    const int32 Dir=U->Battle->GetTeamDirection(U->Team);
    const bool ContactLegActiveV84413=Advancing&&!NationalField&&Squad&&Squad->bAdvancing
        &&ContactLegPassV84413&&ContactLegPassV84413->EscapeDestinationV84413==Squad->Destination
        &&U->Battle->Trenches.IsValidIndex(ContactLegTrenchV84413)&&U->Battle->Trenches[ContactLegTrenchV84413].Owner==U->Team
        &&ContactLegFullV84413&&IsValid(ContactLegBodyV84413)&&ContactLegBodyV84413->IsAlive()&&ContactLegBodyV84413->Team==U->Team
        &&ContactLegSquadV84413&&!ContactLegSquadV84413->bAdvancing&&ContactLegSquadV84413->Trench==ContactLegTrenchV84413
        &&(From.X-ContactLegBodyV84413->Feet().X)*Dir<Clearance
        &&FMath::Abs(From.X-FrontlineFieldV32::TrenchCentreX(ContactLegTrenchV84413,From.Y))<=180.f;
    if(ContactLegPassV84413&&!ContactLegActiveV84413)
        ContactLegPassV84413->ContactLegBodyV84413.Reset(); // Ending a leg does not replenish its same-bank budget.
    FVector Guided=Detour&&Detour->Active?Detour->Waypoint:Advancing&&!NationalField&&!FrontlineFieldV32::HasMineApproachV833(U)?TerrainWaypointV54(U,Destination,F):Destination;
    AFrontlineTank* OpeningGuideTankV86=nullptr;
    if(!Advancing&&!NationalField&&!FrontlineUseLegacyArmourYieldV84411())
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(U->GetWorld()))for(AFrontlineTank* Tank:Fleet->GetVehicles())
            if(IsValid(Tank)&&Tank->ReadCooperativeOpeningGuideV86(U,Destination,Guided)){OpeningGuideTankV86=Tank;break;}
    if(ContactLegActiveV84413)
    {
        FVector Away=From-ContactLegBodyV84413->Feet();Away.Z=0;Away.Normalize();
        // Follow the radius acquired with the physically checked escape,
        // rather than repeatedly walking a tangent plus outward push. That
        // spiralled farther behind the rank and exhausted its fixed budget.
        const float Arc=FMath::Min(.12f,Distance/FMath::Max(1.f,ContactLegPassV84413->ContactLegRadiusV84413))
            *ContactLegPassV84413->ContactLegTurnV84413;
        const FVector NextRadial(Away.X*FMath::Cos(Arc)-Away.Y*FMath::Sin(Arc),
            Away.X*FMath::Sin(Arc)+Away.Y*FMath::Cos(Arc),0);
        Guided=ContactLegBodyV84413->Feet()+NextRadial*ContactLegPassV84413->ContactLegRadiusV84413;
        Guided.Z=From.Z; // Support owns vertical integration; the real chord still goes through every movement gate.
        if(ReceiptV84411)
        {
            ReceiptV84411->SetBoolField(TEXT("full_friendly_trench_transit"),true);
            ReceiptV84411->SetBoolField(TEXT("contact_leg_active"),true);
            ReceiptV84411->SetNumberField(TEXT("contact_leg_body_member"),ContactLegBodyV84413->MemberIndex);
            ReceiptV84411->SetNumberField(TEXT("contact_leg_forward_gap_cm"),(From.X-ContactLegBodyV84413->Feet().X)*Dir);
            ReceiptV84411->SetNumberField(TEXT("contact_leg_radius_cm"),ContactLegPassV84413->ContactLegRadiusV84413);
            ReceiptV84411->SetStringField(TEXT("contact_origin"),ContactLegPassV84413->ContactOriginV84413.ToString());
            ReceiptV84411->SetNumberField(TEXT("actual_contact_reverse_cm"),ContactLegPassV84413->ActualBackwardTravelV84413);
        }
    }
    if(ReceiptV84411)
    {ReceiptV84411->SetStringField(TEXT("guided"),Guided.ToString());ReceiptV84411->SetBoolField(TEXT("detour_active"),Detour&&Detour->Active);ReceiptV84411->SetNumberField(TEXT("actor_yaw"),U->GetActorRotation().Yaw);}
    const bool Trace=TraceUnitV64.Get()==U;if(Trace)TraceUnitV64.Reset();
    if(Trace)UE_LOG(LogTemp,Display,TEXT("V64_STEP_START from=%s destination=%s guided=%s distance=%.4f advancing=%d"),*From.ToString(),*Destination.ToString(),*Guided.ToString(),Distance,Advancing);
    if(Trace)UE_LOG(LogTemp,Display,TEXT("V84411_STEP_SCOPE held_yield=%d held_bay=%d local_bay=%d local_radius=%.4f retained=%d same_goal=%d detour_active=%d actor_yaw=%.4f bank=%.4f"),HeldYieldBayV84411,HeldBay,LocalBay,LocalBayRadiusV84411,RetainedBay,SameGoal,Detour&&Detour->Active,U->GetActorRotation().Yaw,FrontlineInfantryV45::BankIntrusion(U,From));
    FVector Wish=Guided-From;Wish.Z=0;
    if(Advancing&&!NationalField&&!ContactLegActiveV84413&&Wish.X*Dir<0)Wish.X=0;
    // Distance belongs to the reachable direction after the advance constraint.
    // Keeping the original backward distance overshot a tiny lateral remainder
    // every tick, producing a fast left/right run around a stationary gun.
    const float Remaining=Wish.Size2D();
    if(!Wish.Normalize()){if(ReceiptV84411)ReceiptV84411->SetStringField(TEXT("result_gate"),TEXT("no_horizontal_goal"));return From;}
    Distance=FMath::Min3(Distance,Remaining,80.f);
    const FIntPoint C=Cell(From);TArray<int32,TInlineAllocator<32>> Near;
    // At most 25 nearby buckets, 96 samples per bucket and the closest 32
    // candidates. No per-soldier scan of the battlefield or arbitrary lane wall.
    for(int32 Y=-2;Y<=2;++Y)for(int32 X=-2;X<=2;++X)if(const auto* Bucket=F.Cells.Find(C+FIntPoint(X,Y)))
    {
        int32 Samples=0;
        for(int32 K=0;K<Bucket->Num()&&Samples<96;++K)
        {
            const int32 I=(*Bucket)[K];const auto& P=F.People[I];
            // Hidden native passengers share the broad-phase index, but do
            // not consume the original 96 visible-body sampling allowance.
            if(!P.LegacyIndexed)continue;++Samples;
            if(P.Unit.Get()==U||!P.Unit.IsValid()||!P.Unit->IsAlive()||FVector::DistSquared2D(From,P.Position)>FMath::Square(SearchRadius))continue;
            if(Near.Num()<32)Near.Add(I);
            else
            {
                int32 Far=0;for(int32 N=1;N<Near.Num();++N)if(FVector::DistSquared2D(From,F.People[Near[N]].Position)>FVector::DistSquared2D(From,F.People[Near[Far]].Position))Far=N;
                if(FVector::DistSquared2D(From,P.Position)<FVector::DistSquared2D(From,F.People[Near[Far]].Position))Near[Far]=I;
            }
        }
    }
    const FVector Across(-Wish.Y,Wish.X,0);int32 Blocker=INDEX_NONE;float AheadBest=240;
    if(Trace)for(int32 I:Near)UE_LOG(LogTemp,Display,TEXT("V64_STEP_NEIGHBOUR member=%d squad=%d feet=%s gap=%.2f"),F.People[I].Unit->MemberIndex,F.People[I].Unit->SquadId,*F.People[I].Position.ToString(),FVector::Dist2D(From,F.People[I].Position));
    for(int32 I:Near)
    {
        const FVector D=F.People[I].Position-From;const float Ahead=FVector::DotProduct(D,Wish);
        if(Ahead>=-8&&Ahead<AheadBest&&FMath::Abs(FVector::DotProduct(D,Across))<Clearance+24){AheadBest=Ahead;Blocker=I;}
    }
    float Side=0;
    if(Blocker!=INDEX_NONE)
    {
        const auto& Obstacle=F.People[Blocker];auto& Pass=F.Passing.FindOrAdd(U);
        if(Pass.Obstacle!=Obstacle.Unit||Pass.Until<U->Battle->Elapsed)
        {
            const float Offset=FVector::DotProduct(From-Obstacle.Position,Across);
            Pass.Side=FMath::Abs(Offset)>16?(Offset>0?1.f:-1.f):(U->GetUniqueID()<Obstacle.Unit->GetUniqueID()?-1.f:1.f);
            Pass.Obstacle=Obstacle.Unit;
        }
        Pass.Until=U->Battle->Elapsed+.9f;Side=Pass.Side;
    }
    else if(!FrontlineInfantryV45::AllowsTrenchStep(U,From,From+Wish*Distance))
    {
        // A public advance can assign another receiving slot. Its diagonal
        // wish must still leave the bank by a legal forward crossing before
        // turning toward that lane. Side=0 repeated the same rejected heading
        // five times and could pin a whole crew to the front wall indefinitely.
        if(Advancing&&!NationalField)
            Side=FVector::DotProduct(FVector(Dir,0,0),Across)>=0.f?1.f:-1.f;
        // Native squads have no legacy trench centre. A rejected forward
        // slope still needs distinct tangent trials around the actual bank;
        // otherwise Side stays zero and all five headings are identical.
        // Prefer a fully checked native tangent. The ordinary candidate loop
        // still owns body clearance and native ground/capsule acceptance.
        else if(NationalField)
        {
            const FVector Tangent=(Wish+Across*.70f).GetSafeNormal();
            Side=FrontlineInfantryV45::AllowsTrenchStep(U,From,From+Tangent*Distance)?1.f:-1.f;
        }
        // A curved legacy parapet first tries its inward trench tangent.
        else if(Squad&&U->Battle->Trenches.IsValidIndex(Squad->Trench))
        {
            const float Centre=FrontlineFieldV32::TrenchCentreX(Squad->Trench,From.Y);
            const FVector Inward(Centre-From.X,0,0);
            Side=FVector::DotProduct(Inward,Across)>=0?1.f:-1.f;
        }
    }
    const auto* FleetV8218=UFrontlineVehicleSubsystem::Get(U->GetWorld());
    TArray<const AFrontlineTank*,TInlineAllocator<8>> ArmourV8218;
    if(FleetV8218)for(const AFrontlineTank* Tank:FleetV8218->GetVehicles())
        if(IsValid(Tank)&&Tank->IsOperational()&&!Tank->IsHidden()
            &&FVector::DistSquared2D(From,Tank->GetActorLocation())<FMath::Square(3600.f))ArmourV8218.Add(Tank);
    const auto HullLegClearV84411=[&](const FVector& Start,const FVector& End)
    {
        const float Length=FVector::Dist2D(Start,End);if(Length<.01f)return true;
        FVector Previous=Start;Previous.Z=FMath::Max(Start.Z,double(FrontlineInfantryV45::SurfaceHeight(U->Battle,Previous)));
        const int32 Steps=FMath::CeilToInt(Length);
        for(int32 I=1;I<=Steps;++I)
        {
            FVector Next=FMath::Lerp(Start,End,double(I)/Steps);
            const double Floor=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);
            Next.Z=FMath::Max(Previous.Z,Floor);
            for(const AFrontlineTank* Tank:ArmourV8218)
                if(Tank->InfantrySweepClearanceV8218(Previous,Next,34.f,U)<1.f)
                {
                    if(ReceiptV84411&&!ReceiptV84411->HasTypedField<EJson::Object>(TEXT("first_route_hull_reject")))
                    {
                        auto HullReceipt=MakeShared<FJsonObject>();HullReceipt->SetStringField(TEXT("from"),Previous.ToString());HullReceipt->SetStringField(TEXT("to"),Next.ToString());
                        HullReceipt->SetNumberField(TEXT("tank_serial"),Tank->Serial);ReceiptV84411->SetObjectField(TEXT("first_route_hull_reject"),HullReceipt);
                    }
                    return false;
                }
            Previous=Next;Previous.Z=Floor;
        }
        return true;
    };
    int32 BodyRejectsV84411=0,BankRejectsV84411=0,HullRejectsV84411=0;
    const auto LegClear=[&](const FVector& Start,const FVector& End)
    {
        if(HeldYieldBayV84411)
        {
            FYieldLegFailureV84411 Failure;
            if(HeldYieldLegClearV84411(U,&F,ArmourV8218,Start,End,&Failure))return true;
            if(Failure.Kind==EYieldLegFailureV84411::Body)
            {
                ++BodyRejectsV84411;
                if(ReceiptV84411&&BodyRejectsV84411==1)
                {
                    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("from"),Failure.From.ToString());R->SetStringField(TEXT("to"),Failure.To.ToString());
                    R->SetStringField(TEXT("body"),Failure.Body.ToString());R->SetNumberField(TEXT("member"),Failure.Other->MemberIndex);R->SetNumberField(TEXT("squad"),Failure.Other->SquadId);
                    R->SetNumberField(TEXT("start_gap_cm"),FVector::Dist2D(Failure.From,Failure.Body));R->SetNumberField(TEXT("closest_gap_cm"),Failure.ClosestGap);ReceiptV84411->SetObjectField(TEXT("first_route_body_reject"),R);
                }
                if(Trace&&BodyRejectsV84411==1)UE_LOG(LogTemp,Display,TEXT("V84411_BANK_LEG_BODY_REJECT from=%s to=%s member=%d squad=%d body=%s closest_gap=%.4f start_gap=%.4f"),*Failure.From.ToString(),*Failure.To.ToString(),Failure.Other->MemberIndex,Failure.Other->SquadId,*Failure.Body.ToString(),Failure.ClosestGap,FVector::Dist2D(Failure.From,Failure.Body));
            }
            else if(Failure.Kind==EYieldLegFailureV84411::Bank)
            {
                ++BankRejectsV84411;
                if(ReceiptV84411&&BankRejectsV84411==1)
                {
                    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("from"),Failure.From.ToString());R->SetStringField(TEXT("to"),Failure.To.ToString());
                    R->SetNumberField(TEXT("old_bank_cm"),FrontlineInfantryV45::BankIntrusion(U,Failure.From));R->SetNumberField(TEXT("new_bank_cm"),FrontlineInfantryV45::BankIntrusion(U,Failure.To));ReceiptV84411->SetObjectField(TEXT("first_route_bank_reject"),R);
                }
                if(Trace&&BankRejectsV84411==1)UE_LOG(LogTemp,Display,TEXT("V84411_BANK_LEG_WALL_REJECT from=%s to=%s old_bank=%.4f new_bank=%.4f"),*Failure.From.ToString(),*Failure.To.ToString(),FrontlineInfantryV45::BankIntrusion(U,Failure.From),FrontlineInfantryV45::BankIntrusion(U,Failure.To));
            }
            else if(Failure.Kind==EYieldLegFailureV84411::Hull)
            {
                ++HullRejectsV84411;
                if(ReceiptV84411&&!ReceiptV84411->HasTypedField<EJson::Object>(TEXT("first_route_hull_reject")))
                {
                    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("from"),Failure.From.ToString());R->SetStringField(TEXT("to"),Failure.To.ToString());
                    R->SetNumberField(TEXT("tank_serial"),Failure.Tank->Serial);ReceiptV84411->SetObjectField(TEXT("first_route_hull_reject"),R);
                }
            }
            return false;
        }
        FVector Leg=End-Start;Leg.Z=0;const float Length=Leg.Size2D();
        if(Length<.01f)return true;
        const auto BodyClearV84411=[&](const AWarfareFrontlineSoldier* OtherUnit,const FVector& Other)
        {
            if(!IsValid(OtherUnit)||OtherUnit==U||!OtherUnit->IsAlive()||OtherUnit->IsHidden())return true;
            const double T=FMath::Clamp(FVector::DotProduct(Other-Start,Leg)/Leg.SizeSquared2D(),0.,1.);
            const double ClosestGapSquaredV84411=FVector::DistSquared2D(Start+Leg*T,Other);
            if(ClosestGapSquaredV84411<FMath::Square(Clearance+.5f))
            {
                FVector AwayV84411=Start-Other;AwayV84411.Z=0;
                const double StartGapSquaredV84411=AwayV84411.SizeSquared2D();
                // Real bodies retain 56 cm clearance. A man already in
                // the extra half-centimetre planning margin can leave it
                // monotonically; rejecting that legal first point denied
                // every two-leg escape from an otherwise clear contact.
                const bool LeavesExistingMarginV84411=HeldYieldBayV84411
                    &&StartGapSquaredV84411>=FMath::Square(Clearance)
                    &&StartGapSquaredV84411<FMath::Square(Clearance+.5f)
                    &&ClosestGapSquaredV84411>=FMath::Square(Clearance)
                    &&FVector::DotProduct(AwayV84411,Leg)>=0.
                    &&FVector::DistSquared2D(End,Other)>=FMath::Square(Clearance+.5f);
                if(LeavesExistingMarginV84411)return true;
                ++BodyRejectsV84411;
                if(ReceiptV84411&&BodyRejectsV84411==1)
                {
                    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("from"),Start.ToString());R->SetStringField(TEXT("to"),End.ToString());
                    R->SetStringField(TEXT("body"),Other.ToString());R->SetNumberField(TEXT("member"),OtherUnit->MemberIndex);R->SetNumberField(TEXT("squad"),OtherUnit->SquadId);
                    R->SetNumberField(TEXT("start_gap_cm"),FVector::Dist2D(Start,Other));R->SetNumberField(TEXT("closest_gap_cm"),FVector::Dist2D(Start+Leg*T,Other));ReceiptV84411->SetObjectField(TEXT("first_route_body_reject"),R);
                }
                if(Trace&&BodyRejectsV84411==1)UE_LOG(LogTemp,Display,TEXT("V84411_BANK_LEG_BODY_REJECT from=%s to=%s member=%d squad=%d body=%s closest_gap=%.4f start_gap=%.4f"),*Start.ToString(),*End.ToString(),OtherUnit->MemberIndex,OtherUnit->SquadId,*Other.ToString(),FVector::Dist2D(Start+Leg*T,Other),FVector::Dist2D(Start,Other));
                return false;
            }
            return true;
        };
        if(HeldYieldBayV84411)
        {
            // A retained multi-leg path can extend beyond this frame's local
            // short-step neighbour list. Query only each swept edge's cells;
            // its full remaining body proof must not omit a distant rank.
            if(!FrontlineMobilityV41::VisitSweptBodies(U,Start,End,Clearance+.5f,
                [&](const AWarfareFrontlineSoldier* Other){return BodyClearV84411(Other,Other->Feet());}))return false;
        }
        else for(int32 I:Near)if(!BodyClearV84411(F.People[I].Unit.Get(),F.People[I].Position))return false;
        FVector Previous=Start;Previous.Z=HeldYieldBayV84411
            ?FMath::Max(Start.Z,double(FrontlineInfantryV45::SurfaceHeight(U->Battle,Previous)))
            :FrontlineInfantryV45::SurfaceHeight(U->Battle,Previous);
        const int32 Steps=FMath::CeilToInt(Length); // at most one centimetre per physical bank test.
        for(int32 I=1;I<=Steps;++I)
        {
            FVector Next=FMath::Lerp(Start,End,double(I)/Steps);
            if(!FrontlineInfantryV45::AllowsTrenchStep(U,Previous,Next))
            {
                ++BankRejectsV84411;
                if(ReceiptV84411&&BankRejectsV84411==1)
                {
                    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("from"),Previous.ToString());R->SetStringField(TEXT("to"),Next.ToString());
                    R->SetNumberField(TEXT("old_bank_cm"),FrontlineInfantryV45::BankIntrusion(U,Previous));R->SetNumberField(TEXT("new_bank_cm"),FrontlineInfantryV45::BankIntrusion(U,Next));ReceiptV84411->SetObjectField(TEXT("first_route_bank_reject"),R);
                }
                if(Trace&&BankRejectsV84411==1)UE_LOG(LogTemp,Display,TEXT("V84411_BANK_LEG_WALL_REJECT from=%s to=%s old_bank=%.4f new_bank=%.4f"),*Previous.ToString(),*Next.ToString(),FrontlineInfantryV45::BankIntrusion(U,Previous),FrontlineInfantryV45::BankIntrusion(U,Next));
                return false;
            }
            Next.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);Previous=Next;
        }
        // A bank/body-clear point inside a passing vehicle is not a route.
        // Keep the exact production body radius and supported-height sweep
        // for both legs; ordinary firing bays and national routes retain
        // their previous planner and final physical movement gates.
        if(HeldYieldBayV84411&&!HullLegClearV84411(Start,End)){++HullRejectsV84411;return false;}
        return true;
    };
    const auto CheckedFloorPathClearV84411=[&]()
    {
        if(!Detour||!Detour->Active)return false;
        FVector Previous=From;
        if(Detour->FloorPathV84411.IsValidIndex(Detour->FloorPathIndexV84411))
        {
            for(int32 I=Detour->FloorPathIndexV84411;I<Detour->FloorPathV84411.Num();++I)
            {const FVector Next=Detour->FloorPathV84411[I];if(!LegClear(Previous,Next))return false;Previous=Next;}
            return true;
        }
        return LegClear(From,Detour->Waypoint)&&LegClear(Detour->Waypoint,Destination);
    };
    if(!OpeningGuideTankV86&&HeldYieldBayV84411&&Detour&&Detour->Active&&!CheckedFloorPathClearV84411())
    {
        // The whole remaining path belongs to this live body field and hull.
        // A changed obstacle invalidates it before any obsolete guide can
        // veto a real escape. Translation still uses the ordinary step gates.
        RecordBankEvent(FrontlineTrenchMotionV8444::EBankInvalidation::AttemptStalled);
        Detour->Active=false;Detour->RetryAt=U->Battle->Elapsed;
        if(Trace)TraceUnitV64=U;
        return RetrySameStepV86(Destination,Advancing);
    }
    if(Side==0.f)for(const AFrontlineTank* Tank:ArmourV8218)
        if(Tank->InfantrySweepClearanceV8218(From,From+Wish*FMath::Max(Distance,300.f),34.f,U)<1.f)
        {
            const float Offset=FVector::DotProduct(From-Tank->GetActorLocation(),Across);
            Side=FMath::Abs(Offset)>12.f?(Offset>0?1.f:-1.f):((U->MemberIndex+Tank->Serial)%2?1.f:-1.f);break;
        }
    float LastBodyClearV84411=0.f,LastHullClearV84411=0.f;
    auto ClearDistance=[&](const FVector& Heading)
    {
        float Allowed=Distance;
        for(int32 I:Near)
        {
            FVector Offset=From-F.People[I].Position;Offset.Z=0;
            const double Along=FVector::DotProduct(Offset,Heading),C2=Offset.SizeSquared()-FMath::Square(Clearance);
            if(C2<0){if(Along<-.01)Allowed=0;continue;}
            if(Along>=0)continue;const double Disc=Along*Along-C2;if(Disc<0)continue;
            Allowed=FMath::Min(Allowed,float(FMath::Max(0.,-Along-FMath::Sqrt(Disc)-.5)));
        }
        // Downward support is integrated after this sweep. Do not borrow a
        // trench-floor height the body has not physically reached yet.
        FVector End=From+Heading*Distance;End.Z=FMath::Max(double(From.Z),double(FrontlineInfantryV45::SurfaceHeight(U->Battle,End)));
        const float BodyAllowedV84411=Allowed;
        for(const AFrontlineTank* Tank:ArmourV8218)
            Allowed=FMath::Min(Allowed,Distance*Tank->InfantrySweepClearanceV8218(From,End,34.f,U));
        if(ContactLegActiveV84413&&Heading.X*Dir<0.f)
        {
            const float Budget=Clearance-ContactLegPassV84413->ActualBackwardTravelV84413;
            const float OriginBudget=Clearance+float((From.X-ContactLegPassV84413->ContactOriginV84413.X)*Dir);
            Allowed=FMath::Min(Allowed,FMath::Max(0.f,FMath::Min(Budget,OriginBudget))/float(-Heading.X*Dir));
        }
        LastBodyClearV84411=BodyAllowedV84411;LastHullClearV84411=Allowed;
        if(Trace)UE_LOG(LogTemp,Display,TEXT("V84411_STEP_CLEARANCE heading=%s requested=%.4f body_clear=%.4f hull_clear=%.4f"),*Heading.ToString(),Distance,BodyAllowedV84411,Allowed);
        return Allowed;
    };
    FVector Best=FVector::ZeroVector;float Score=-FLT_MAX;bool SelectedContactEscapeV84413=false;
    TArray<FVector,TInlineAllocator<7>> PhysicalYieldHeadingsV84411;
    const auto RememberYieldHeadingV84411=[&](const FVector& Heading,float Travel)
    {
        // The sweep supplies a direction, not permission to traverse a bank.
        // Its first few centimetres can be legal before a curve requires the
        // next turn. Each seeded point and complete edge still passes LegClear.
        if(!HeldYieldBayV84411||Travel<Distance*.94f||PhysicalYieldHeadingsV84411.Num()>=7)return;
        for(const FVector& Existing:PhysicalYieldHeadingsV84411)if(FVector::DotProduct(Existing,Heading)>.999f)return;
        PhysicalYieldHeadingsV84411.Add(Heading);
    };
    const auto ApproachesSlot=[&](const FVector& Candidate,float Travel)
    {
        return Advancing||Remaining>=110.f||FVector::Dist2D(Candidate,Guided)
            <Remaining-FMath::Max(.01f,Travel*.08f);
    };
    // Stable preferred passing side, then a steeper tangent. Only a genuinely
    // blocked tangent considers the opposite side; no random per-frame swaying.
    const float Turns[5]={Side*.70f,Side*1.65f,0.f,-Side*.85f,-Side*2.2f};
    TArray<TSharedPtr<FJsonValue>> CandidatesV84411;
    const bool RememberBlockedArmourLegV86=!Advancing&&!NationalField&&!FrontlineUseLegacyArmourYieldV84411()
        &&Squad&&!Squad->bAdvancing&&U->Battle->Trenches.IsValidIndex(Squad->Trench);
    if(RememberBlockedArmourLegV86)
    {
        auto& Tried=F.BlockedArmourLegsV86.FindOrAdd(U);Tried.From=From;Tried.Goal=Destination;
        Tried.Trench=Squad->Trench;Tried.Time=U->Battle->Elapsed;Tried.Requested=Distance;Tried.Candidates.Reset();
    }
    else F.BlockedArmourLegsV86.Remove(U);
    const auto RememberBlockedLegV86=[&](const FVector& Heading)
    {
        if(!RememberBlockedArmourLegV86||LastBodyClearV84411<Distance-.01f||LastHullClearV84411>=Distance-.01f
            ||F.BlockedArmourLegsV86.FindChecked(U).Candidates.Num()>=5)return;
        // An endpoint beyond a thin curved bank can be clear even though its
        // intervening shoulders cross the wall. Retain only the checked prefix
        // of this already attempted heading, using the same body/floor proof
        // as recovery. No vehicle is omitted when that prefix is later opened.
        TArray<const AFrontlineTank*,TInlineAllocator<8>> NoArmour;
        const float LegLength=FMath::Min(24.f,Distance);
        const int32 Samples=FMath::CeilToInt(LegLength);
        FVector Candidate=From+Heading*LegLength;Candidate.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Candidate);
        FYieldLegFailureV84411 Failure;
        if(!HeldYieldLegClearV84411(U,&F,NoArmour,From,Candidate,&Failure))
        {
            // Existing 56..56.5 cm margin escape is judged at the complete
            // endpoint. Only an actual intermediate bank failure may truncate
            // a leg; a one-centimetre margin escape is not a full body proof.
            if(Failure.Kind!=EYieldLegFailureV84411::Bank)return;
            Candidate=From;
            for(int32 I=1;I<=Samples;++I)
            {
                FVector Next=From+Heading*(LegLength*float(I)/Samples);
                Next.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Next);
                if(FMath::Abs(Next.Y)>FrontlineBattlefield::HalfFrontage-35.f
                    ||!HeldYieldLegClearV84411(U,&F,NoArmour,From,Next))break;
                Candidate=Next;
            }
        }
        const float CheckedLength=FVector::Dist2D(From,Candidate);
        if(CheckedLength<=.01f||!ApproachesSlot(Candidate,CheckedLength))return;
        auto& Tried=F.BlockedArmourLegsV86.FindChecked(U);
        FVector End=Candidate;End.Z=FMath::Max(From.Z,Candidate.Z);
        for(const AFrontlineTank* Tank:ArmourV8218)if(Tried.Candidates.Num()<5&&Tank->Team==U->Team
            &&Tank->InfantrySweepClearanceV8218(From,End,34.f,U)<1.f)Tried.Candidates.Add({const_cast<AFrontlineTank*>(Tank),Candidate});
    };
    for(int32 I=0;I<5;++I)
    {
        if(OpeningGuideTankV86&&I!=2)continue; // Execute the exact already-checked local leg, not a different tangent.
        if(ContactLegActiveV84413&&I!=2)continue; // The checked contact flank owns steering until its real body exit.
        FVector H=(Wish+Across*Turns[I]).GetSafeNormal();if(Advancing&&!NationalField&&!ContactLegActiveV84413&&H.X*Dir<-.001)continue;
        FVector Candidate=From+H*Distance;if(FMath::Abs(Candidate.Y)>FrontlineBattlefield::HalfFrontage-35.f)continue;
        const float Travel=ClearDistance(H);
        const bool BankAllows=FrontlineInfantryV45::AllowsTrenchStep(U,From,From+H*Travel);
        // Retain only an already attempted held step whose body/bank checks
        // permit its requested travel, but this actual hull prevents it.
        // Recovery later rechecks every centimetre against all bodies/banks.
        RememberBlockedLegV86(H);
        RememberYieldHeadingV84411(H,Travel);
        if(ReceiptV84411)
        {
            auto CandidateReceipt=MakeShared<FJsonObject>();CandidateReceipt->SetNumberField(TEXT("index"),I);CandidateReceipt->SetStringField(TEXT("heading"),H.ToString());
            CandidateReceipt->SetNumberField(TEXT("body_clear_cm"),LastBodyClearV84411);CandidateReceipt->SetNumberField(TEXT("hull_clear_cm"),LastHullClearV84411);
            CandidateReceipt->SetBoolField(TEXT("bank_allows"),BankAllows);CandidateReceipt->SetBoolField(TEXT("approaches_goal"),ApproachesSlot(From+H*Travel,Travel));
            CandidatesV84411.Add(MakeShared<FJsonValueObject>(CandidateReceipt));
        }
        if(Trace)UE_LOG(LogTemp,Display,TEXT("V64_STEP_CANDIDATE i=%d side=%.2f heading=%s travel=%.4f bank=%.4f allowed=%d approaches=%d remaining=%.4f next_distance=%.4f"),I,Side,*H.ToString(),Travel,FrontlineInfantryV45::BankIntrusion(U,From+H*Travel),BankAllows,ApproachesSlot(From+H*Travel,Travel),Remaining,FVector::Dist2D(From+H*Travel,Destination));
        if(OpeningGuideTankV86&&(!BankAllows||Travel<Distance-.01f))
        {
            OpeningGuideTankV86->CancelCooperativeOpeningGuideV86(U,Destination);
            if(ReceiptV84411)ReceiptV84411->SetStringField(TEXT("result_gate"),TEXT("cooperative_leg_changed_physically"));
            return From;
        }
        if(!BankAllows||Travel<=.01f)continue;
        // Inside an occupied trench slot, a tangent must actually approach the
        // assigned position. Otherwise its positive travel score makes a man
        // orbit his neighbour indefinitely while playing a walking clip.
        if(!ApproachesSlot(From+H*Travel,Travel))continue;
        const float Approach=I==2&&Blocker!=INDEX_NONE?Distance*.32f*(1-AheadBest/240.f):0.f;
        const float Value=Travel*(.78f+.22f*FVector::DotProduct(H,Wish))-(I>=3?.6f:0.f)-Approach;
        if(Value>Score){Score=Value;Best=H*Travel;SelectedContactEscapeV84413=ContactLegActiveV84413;}
        if(I==1&&Best.Size2D()>Distance*.94f)break;
    }
    if(!ContactLegActiveV84413&&(!Advancing||!NationalField)&&Blocker!=INDEX_NONE&&Best.Size2D()<Distance*.5f)
    {
        // At body clearance, every destination-centred heading can point
        // inside the same neighbour. Follow that body's actual tangent; a
        // small outward component keeps the safety margin from trapping the
        // actor on the circle. No collision radius or wall gate is relaxed.
        FVector Away=From-F.People[Blocker].Position;Away.Z=0;Away.Normalize();
        FVector Tangent(-Away.Y,Away.X,0);
        if(FVector::DotProduct(Tangent,Across)*Side<0.f)Tangent=-Tangent;
        auto* ContactPassV84413=F.Passing.Find(U);
        const auto* ContactUnitV84413=F.People[Blocker].Unit.Get();
        const auto* ContactSquadV84413=ContactUnitV84413?U->Battle->FindSquad(ContactUnitV84413->SquadId):nullptr;
        const int32 ContactTrenchV84413=ContactPassV84413&&ContactPassV84413->FullTrenchEscapeV84413!=INDEX_NONE
            ?ContactPassV84413->FullTrenchEscapeV84413:ContactSquadV84413?ContactSquadV84413->Trench:INDEX_NONE;
        bool HeldContactV84413=false;
        for(int32 I:Near)
        {
            const auto* Other=F.People[I].Unit.Get();const auto* OtherSquad=Other?U->Battle->FindSquad(Other->SquadId):nullptr;
            HeldContactV84413|=Other&&Other->Team==U->Team&&OtherSquad&&!OtherSquad->bAdvancing&&OtherSquad->Trench==ContactTrenchV84413
                &&FVector::DistSquared2D(From,F.People[I].Position)<=FMath::Square(Clearance+24.f);
        }
        bool ResidentSlotsV84413[FFrontlineTrench::Capacity]={};
        for(const auto& Resident:U->Battle->Squads)
            if(Resident.Id!=U->SquadId&&Resident.Team==U->Team&&Resident.Trench==ContactTrenchV84413&&Resident.LivingCount()>0
                &&Resident.Slot>=0&&Resident.Slot<FFrontlineTrench::Capacity)ResidentSlotsV84413[Resident.Slot]=true;
        bool AllResidentSlotsTakenV84413=true;
        for(bool Taken:ResidentSlotsV84413)AllResidentSlotsTakenV84413&=Taken;
        const bool FullFriendlyTransitV84413=Advancing&&!NationalField&&Squad&&Squad->bAdvancing&&HeldContactV84413
            &&U->Battle->Trenches.IsValidIndex(ContactTrenchV84413)
            &&U->Battle->Trenches[ContactTrenchV84413].Owner==U->Team
            &&U->Battle->TrenchSquadCount(ContactTrenchV84413,U->Team)>=FFrontlineTrench::Capacity
            &&AllResidentSlotsTakenV84413
            &&U->Battle->Trenches.IsValidIndex(Squad->Destination)&&(Squad->Destination-ContactTrenchV84413)*Dir>0
            &&FMath::Abs(From.X-FrontlineFieldV32::TrenchCentreX(ContactTrenchV84413,From.Y))<=180.f;
        if(ContactPassV84413&&ContactPassV84413->FullTrenchEscapeV84413!=INDEX_NONE
            &&(!Advancing||NationalField||!Squad||!Squad->bAdvancing
                ||Squad->Destination!=ContactPassV84413->EscapeDestinationV84413
                ||!U->Battle->Trenches.IsValidIndex(ContactPassV84413->FullTrenchEscapeV84413)
                ||FMath::Abs(From.X-FrontlineFieldV32::TrenchCentreX(ContactPassV84413->FullTrenchEscapeV84413,From.Y))>180.f))
            ContactPassV84413->FullTrenchEscapeV84413=INDEX_NONE;
        if(FullFriendlyTransitV84413&&ContactPassV84413&&Best.Size2D()<=.01f
            &&ContactPassV84413->FullTrenchEscapeV84413==INDEX_NONE)
        {
            ContactPassV84413->FullTrenchEscapeV84413=ContactTrenchV84413;
            ContactPassV84413->ContactOriginV84413=From;ContactPassV84413->ActualBackwardTravelV84413=0.f;
            ContactPassV84413->EscapeDirectionV84413=Dir;ContactPassV84413->EscapeDestinationV84413=Squad->Destination;
        }
        const bool BoundedContactEscapeV84413=FullFriendlyTransitV84413&&ContactPassV84413
            &&ContactPassV84413->FullTrenchEscapeV84413==ContactTrenchV84413
            &&ContactPassV84413->ActualBackwardTravelV84413<Clearance;
        if(ReceiptV84411)
        {
            ReceiptV84411->SetBoolField(TEXT("full_friendly_trench_transit"),FullFriendlyTransitV84413);
            ReceiptV84411->SetBoolField(TEXT("bounded_contact_escape"),BoundedContactEscapeV84413);
            if(ContactPassV84413){ReceiptV84411->SetStringField(TEXT("contact_origin"),ContactPassV84413->ContactOriginV84413.ToString());ReceiptV84411->SetNumberField(TEXT("actual_contact_reverse_cm"),ContactPassV84413->ActualBackwardTravelV84413);}
            TArray<TSharedPtr<FJsonValue>> ContactBodies;
            for(int32 I:Near)if(FVector::DistSquared2D(From,F.People[I].Position)<=FMath::Square(Clearance+24.f))
            {auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("squad"),F.People[I].Unit->SquadId);R->SetNumberField(TEXT("member"),F.People[I].Unit->MemberIndex);R->SetStringField(TEXT("feet"),F.People[I].Position.ToString());R->SetNumberField(TEXT("gap_cm"),FVector::Dist2D(From,F.People[I].Position));ContactBodies.Add(MakeShared<FJsonValueObject>(R));}
            ReceiptV84411->SetArrayField(TEXT("actual_contact_bodies"),ContactBodies);
        }
        for(int32 Attempt=0;Attempt<(BoundedContactEscapeV84413?5:2);++Attempt)
        {
            const bool LocalEscapeV84413=Attempt>=2;
            FVector TangentWish=Attempt==4?FVector(-Dir,0,0):((Attempt%2)?-Tangent:Tangent)+Away*.10f;
            // A marching man can meet the planted rank in a full friendly
            // trench at body contact. Destination-centred angles then all
            // enter that body, while the usable sideways escape was reserved
            // for held men. Keep the ordinary forward-only contract: remove
            // its rearward component, then sweep the real lateral/forward
            // tangent through the same body, hull and curved-bank gates.
            const FVector OriginalTangentV84413=TangentWish;
            if(Advancing&&!NationalField&&!LocalEscapeV84413&&TangentWish.X*Dir<0.f)TangentWish.X=0.f;
            const FVector H=TangentWish.GetSafeNormal();if(H.IsNearlyZero())continue;
            float Travel=ClearDistance(H);
            RememberBlockedLegV86(H);
            if(LocalEscapeV84413&&H.X*Dir<0.f)
            {
                // A full friendly rank can seal the entire forward half-plane.
                // This local contact lease permits at most one body clearance
                // of actual rearward travel; changing neighbours cannot reset it.
                const float Budget=Clearance-ContactPassV84413->ActualBackwardTravelV84413;
                const float OriginBudget=Clearance+float((From.X-ContactPassV84413->ContactOriginV84413.X)*Dir);
                Travel=FMath::Min(Travel,FMath::Max(0.f,FMath::Min(Budget,OriginBudget))/float(-H.X*Dir));
            }
            const FVector Candidate=From+H*Travel;
            RememberYieldHeadingV84411(H,Travel);
            const bool Allowed=FMath::Abs(Candidate.Y)<=FrontlineBattlefield::HalfFrontage-35.f
                &&FrontlineInfantryV45::AllowsTrenchStep(U,From,Candidate)
                &&ApproachesSlot(Candidate,Travel);
            if(ReceiptV84411)
            {
                auto TangentReceipt=MakeShared<FJsonObject>();TangentReceipt->SetNumberField(TEXT("body_tangent_attempt"),Attempt);TangentReceipt->SetStringField(TEXT("heading"),H.ToString());
                TangentReceipt->SetStringField(TEXT("original_tangent"),OriginalTangentV84413.GetSafeNormal().ToString());TangentReceipt->SetNumberField(TEXT("away_dot"),FVector::DotProduct(Away,H));TangentReceipt->SetBoolField(TEXT("local_contact_escape"),LocalEscapeV84413);
                TangentReceipt->SetNumberField(TEXT("body_clear_cm"),LastBodyClearV84411);TangentReceipt->SetNumberField(TEXT("hull_clear_cm"),LastHullClearV84411);
                TangentReceipt->SetBoolField(TEXT("bank_allows"),FrontlineInfantryV45::AllowsTrenchStep(U,From,Candidate));TangentReceipt->SetBoolField(TEXT("approaches_goal"),ApproachesSlot(Candidate,Travel));
                CandidatesV84411.Add(MakeShared<FJsonValueObject>(TangentReceipt));
            }
            if(Trace)UE_LOG(LogTemp,Display,TEXT("V64_STEP_BODY_TANGENT attempt=%d heading=%s travel=%.4f bank=%.4f allowed=%d"),Attempt,*H.ToString(),Travel,FrontlineInfantryV45::BankIntrusion(U,Candidate),Allowed);
            if(Allowed&&Travel>Best.Size2D()+.01f)
            {
                Best=H*Travel;SelectedContactEscapeV84413=LocalEscapeV84413;
                if(LocalEscapeV84413&&ContactPassV84413)
                {
                    ContactPassV84413->ContactLegBodyV84413=F.People[Blocker].Unit;
                    ContactPassV84413->ContactLegTurnV84413=FVector::DotProduct(H,FVector(-Away.Y,Away.X,0))>=0.f?1.f:-1.f;
                    ContactPassV84413->ContactLegRadiusV84413=FMath::Max(Clearance+.5f,float(FVector::Dist2D(Candidate,F.People[Blocker].Position)));
                }
                if(auto* Pass=F.Passing.Find(U))Pass->Side=FVector::DotProduct(H,Across)>=0.f?1.f:-1.f;
                break;
            }
        }
    }
    if(ContactLegActiveV84413&&Best.Size2D()<=.01f)
    {
        // Another real body or bank can seal this flank. Invalidate only the
        // physically rejected leg, then reuse the ordinary checked resolver;
        // the same-bank contact origin and spent reverse budget remain intact.
        ContactLegPassV84413->ContactLegBodyV84413.Reset();
        return RetrySameStepV86(Destination,Advancing);
    }
    if(ReceiptV84411){ReceiptV84411->SetArrayField(TEXT("actual_candidates"),CandidatesV84411);ReceiptV84411->SetBoolField(TEXT("selected_contact_escape"),SelectedContactEscapeV84413);}
    // A physically legal tangent can still orbit a sealed final leg forever.
    // Only a held hull-yield route checks that complete leg and plans a finite
    // floor path even while one short tangent remains possible.
    const bool NeedsWholeYieldRouteV84411=!OpeningGuideTankV86&&LocalBay&&HeldYieldBayV84411&&(!Detour||!Detour->Active)
        &&(!Detour||U->Battle->Elapsed>=Detour->RetryAt)
        &&YieldGoalInLocalScopeV84411&&!FrontlineArmourYieldDirectPathClearV84411(U,Destination);
    if(!OpeningGuideTankV86&&LocalBay&&(Best.Size2D()<=.01f||NeedsWholeYieldRouteV84411)
        &&(!Detour||(!Detour->Active&&U->Battle->Elapsed>=Detour->RetryAt)))
    {
        if(ReceiptV84411)ReceiptV84411->SetBoolField(TEXT("two_leg_route_attempted"),true);
        // A safe firing bay can lie around a curved inner-bank corner. Every
        // ordinary near-slot heading had to reduce final distance, trapping
        // the man until a hitch skipped the corner. Find one short waypoint;
        // both legs retain the real bank gate and full neighbour clearance.
        auto& Route=F.BankDetoursV81.FindOrAdd(U);
        Route.Trench=Squad->Trench;Route.Goal=Destination;Route.RetryAt=U->Battle->Elapsed+.5f;
        float BestLength=MAX_flt;FVector Waypoint=FVector::ZeroVector;
        FVector ResolvedRouteGoalV84411=Destination;
        TArray<FVector,TInlineAllocator<24>> PlannedFloorPathV84411;
        const float Inward=FrontlineFieldV32::TrenchCentreX(Squad->Trench,From.Y)<From.X?-1.f:1.f;
        if(!HeldYieldBayV84411||YieldGoalInLocalScopeV84411)
        for(float Radius:{24.f,48.f,72.f})for(int32 I=0;I<16;++I)
        {
            const float Angle=I*PI/8.f;
            FVector P=From+FVector(Inward*FMath::Cos(Angle),FMath::Sin(Angle),0)*Radius;
            if(FMath::Abs(P.Y)>FrontlineBattlefield::HalfFrontage-35.f)continue;
            const float Length=Radius+FVector::Dist2D(P,Destination);
            if(Length>=BestLength||!LegClear(From,P)||!LegClear(P,Destination))continue;
            BestLength=Length;Waypoint=P;
        }
        if(HeldYieldBayV84411&&BestLength==MAX_flt)
        {
            // The held floor is a curved traverse, not a free navigation grid.
            // A single corner cannot describe its successive bends or a rank
            // of real bodies. Search at most 121 authored-floor points, only
            // their adjacent rows/columns and short start/end connections.
            // Every edge uses the same full physical leg as the two-leg bay.
            // A just-clear body tangent can differ from the floor lattice by
            // only a few centimetres. Preserve the actually checked short
            // headings as start connections; they still need a full physical
            // leg and a complete route, never an away-from-goal movement waiver.
            TArray<int32,TInlineAllocator<2>> Touching;
            for(int32 I:Near)if(FVector::DistSquared2D(From,F.People[I].Position)<FMath::Square(Clearance+24.f))
            {
                if(Touching.Num()<2)Touching.Add(I);
                else
                {
                    const int32 Far=FVector::DistSquared2D(From,F.People[Touching[0]].Position)
                        >FVector::DistSquared2D(From,F.People[Touching[1]].Position)?0:1;
                    if(FVector::DistSquared2D(From,F.People[I].Position)<FVector::DistSquared2D(From,F.People[Touching[Far]].Position))Touching[Far]=I;
                }
            }
            for(int32 I:Touching)
            {
                FVector Away=From-F.People[I].Position;Away.Z=0;if(!Away.Normalize())continue;
                const FVector Tangent(-Away.Y,Away.X,0);
                for(float Sign:{1.f,-1.f})
                {
                    const FVector Heading=(Tangent*Sign+Away*.10f).GetSafeNormal();
                    const float Travel=ClearDistance(Heading);
                    RememberYieldHeadingV84411(Heading,Travel);
                }
            }
            FHeldFloorGraphV86 Graph;
            const auto NodeClear=[&](const FVector& P)
            {
                for(const AFrontlineTank* Tank:ArmourV8218)
                    if(Tank->BlocksInfantryAtV8218(P,Tank->GetActorLocation(),Tank->GetActorRotation().Yaw,34.f,0.f,U))return false;
                return true;
            };
            const bool Found=FindHeldFloorGraphV86(U,Destination,Distance,LocalBayRadiusV84411,YieldGoalInLocalScopeV84411,
                MakeArrayView(PhysicalYieldHeadingsV84411),NodeClear,LegClear,
                [&](const FVector& P){return FrontlineArmourYieldReachableStationClearV84411(U,YieldTankV84411,P);},Graph);
            if(Found)
            {
                ResolvedRouteGoalV84411=Graph.Goal;BestLength=Graph.Cost;Waypoint=Graph.Path[0];
                PlannedFloorPathV84411.Append(Graph.Path);
                if(!Graph.OriginalGoalReachable)
                {
                    const float LeaseSide=FVector::DotProduct(ResolvedRouteGoalV84411-From,YieldTankV84411->GetActorRightVector());
                    YieldLeaseV84411->Destination=ResolvedRouteGoalV84411;
                    YieldLeaseV84411->SideV836=LeaseSide>0.f?1.f:LeaseSide<0.f?-1.f:0.f;
                    YieldLeaseV84411->PathBlockedV84411=false;Route.Goal=ResolvedRouteGoalV84411;
                }
            }
            if(ReceiptV84411)
            {
                ReceiptV84411->SetNumberField(TEXT("floor_graph_nodes"),Graph.Nodes);ReceiptV84411->SetNumberField(TEXT("floor_graph_visited"),Graph.Visited);
                ReceiptV84411->SetNumberField(TEXT("floor_graph_tested_edges"),Graph.TestedEdges);ReceiptV84411->SetBoolField(TEXT("floor_graph_found"),Found);
                ReceiptV84411->SetBoolField(TEXT("floor_graph_original_goal_reachable"),Graph.OriginalGoalReachable);
                ReceiptV84411->SetBoolField(TEXT("floor_graph_original_goal_in_local_scope"),YieldGoalInLocalScopeV84411);
                ReceiptV84411->SetBoolField(TEXT("floor_graph_replaced_unreachable_station"),Found&&!Graph.OriginalGoalReachable);
                ReceiptV84411->SetStringField(TEXT("floor_graph_resolved_station"),ResolvedRouteGoalV84411.ToString());
                ReceiptV84411->SetNumberField(TEXT("floor_graph_physical_start_headings"),PhysicalYieldHeadingsV84411.Num());
                ReceiptV84411->SetNumberField(TEXT("floor_graph_tangent_nodes"),Graph.TangentNodes);
                TArray<TSharedPtr<FJsonValue>> Points;for(const FVector& P:PlannedFloorPathV84411)Points.Add(MakeShared<FJsonValueString>(P.ToString()));
                ReceiptV84411->SetArrayField(TEXT("floor_graph_path"),Points);
            }
        }
        if(Trace)UE_LOG(LogTemp,Display,TEXT("V84411_BANK_ROUTE_RESULT found=%d waypoint=%s length=%.4f body_rejects=%d bank_rejects=%d"),BestLength<MAX_flt,*Waypoint.ToString(),BestLength,BodyRejectsV84411,BankRejectsV84411);
        if(ReceiptV84411)
        {
            ReceiptV84411->SetBoolField(TEXT("two_leg_route_found"),BestLength<MAX_flt);ReceiptV84411->SetStringField(TEXT("route_waypoint"),Waypoint.ToString());
            ReceiptV84411->SetNumberField(TEXT("route_body_rejects"),BodyRejectsV84411);ReceiptV84411->SetNumberField(TEXT("route_bank_rejects"),BankRejectsV84411);
            ReceiptV84411->SetNumberField(TEXT("route_hull_rejects"),HullRejectsV84411);
            auto Plan=MakeShared<FJsonObject>();Plan->SetNumberField(TEXT("at_host_seconds"),U->Battle->Elapsed);
            for(const auto& Pair:ReceiptV84411->Values)
            {
                const FString PlanKey(*Pair.Key);
                if(PlanKey.StartsWith(TEXT("floor_graph_"))||PlanKey.StartsWith(TEXT("first_route_"))
                    ||PlanKey.StartsWith(TEXT("route_"))||PlanKey==TEXT("two_leg_route_found"))Plan->SetField(Pair.Key,Pair.Value);
            }
            Route.LastPhysicalPlanV84411=Plan;ReceiptV84411->SetObjectField(TEXT("last_physical_route_plan"),Plan);
        }
        if(HeldYieldBayV84411&&YieldLeaseV84411&&BestLength>=MAX_flt)
        {
            // The real short-step loop and every checked two-leg route failed.
            // This is physical approach evidence, not an endpoint preference.
            YieldLeaseV84411->PathBlockedV84411=true;
            if(ReceiptV84411)ReceiptV84411->SetBoolField(TEXT("yield_route_blocked_evidence"),true);
        }
        if(BestLength<MAX_flt)
        {
            Route.Waypoint=Waypoint;Route.Active=true;Route.ProgressFeet=From;
            Route.FloorPathV84411.Reset();Route.FloorPathIndexV84411=0;
            if(HeldYieldBayV84411)
            {
                if(PlannedFloorPathV84411.IsEmpty())
                {Route.FloorPathV84411.Add(Waypoint);Route.FloorPathV84411.Add(Destination);}
                else Route.FloorPathV84411=MoveTemp(PlannedFloorPathV84411);
            }
            Route.AttemptAtV8444=U->Battle->Elapsed;Route.StalledSecondsV8444=0.f;
            if(ReviewBankV8444)
            {Route.LastAttemptGoalV8444=ResolvedRouteGoalV84411;Route.TotalAttemptSecondsV8444=0.f;}
            Route.DecisionV8444=++F.NextBankDecisionV8444;
            bool FreshRouteClearV86=true;
            if(HeldYieldBayV84411)
            {
                FVector Previous=From;
                for(const FVector& Next:Route.FloorPathV84411)
                {if(!LegClear(Previous,Next)){FreshRouteClearV86=false;break;}Previous=Next;}
            }
            if(!FreshRouteClearV86)
            {
                Route.Active=false;Route.FloorPathV84411.Reset();Route.FloorPathIndexV84411=0;
                if(YieldLeaseV84411)YieldLeaseV84411->PathBlockedV84411=true;
                if(ReceiptV84411)ReceiptV84411->SetBoolField(TEXT("new_route_failed_commit_proof"),true);
            }
            else
            {
            // The normal step resolver owns translation, speed and animation.
            if(Trace)TraceUnitV64=U;
            return RetrySameStepV86(ResolvedRouteGoalV84411,false); // the same attempt, not another frame of steering.
            }
        }
    }
    if(RequiredFacing&&Best.SizeSquared2D()>.0001f)
    {
        *RequiredFacing=Best.GetSafeNormal2D();
        if(ReceiptV84411)
        {ReceiptV84411->SetStringField(TEXT("required_facing"),RequiredFacing->ToString());ReceiptV84411->SetNumberField(TEXT("facing_dot"),FVector::DotProduct(*RequiredFacing,U->GetActorForwardVector()));}
        if(Trace)UE_LOG(LogTemp,Display,TEXT("V84411_STEP_FACING direction=%s actor_forward=%s dot=%.4f turns_before_walk=%d"),*RequiredFacing->ToString(),*U->GetActorForwardVector().ToString(),FVector::DotProduct(*RequiredFacing,U->GetActorForwardVector()),FVector::DotProduct(*RequiredFacing,U->GetActorForwardVector())<.5f);
        // A forward walk/run clip cannot describe a body moving through its
        // own back. Turn in place toward the accepted bank/body tangent first.
        // A local bay may still require a legal rearward leg; its collision
        // rules and destination are unchanged. Turning is no failed steering.
        if(FVector::DotProduct(*RequiredFacing,U->GetActorForwardVector())<.5f)
        {if(ReceiptV84411)ReceiptV84411->SetStringField(TEXT("result_gate"),TEXT("turn_before_walk"));return From;}
    }
    if(Detour&&Detour->Active&&FMath::IsFinite(AttemptSeconds)&&AttemptSeconds>0.f)
    {
        if(ReviewBankV8444)Detour->TotalAttemptSecondsV8444+=AttemptSeconds;
        if(!BankProgressV8447)Detour->StalledSecondsV8444+=AttemptSeconds;
        Detour->AttemptAtV8444=U->Battle->Elapsed;
    }
    FVector To=From+Best;To.Z=From.Z;
    if(HeldYieldBayV84411&&YieldLeaseV84411&&Best.Size2D()>.01f)YieldLeaseV84411->PathBlockedV84411=false;
    if(ReceiptV84411)
    {ReceiptV84411->SetStringField(TEXT("result_gate"),Best.SizeSquared2D()>.0001f?TEXT("physical_step" ):TEXT("no_physical_candidate"));ReceiptV84411->SetStringField(TEXT("proposed_to"),To.ToString());ReceiptV84411->SetNumberField(TEXT("proposed_cm"),Best.Size2D());}
    if(Trace)UE_LOG(LogTemp,Display,TEXT("V64_STEP_RESULT to=%s delta=%s score=%.4f"),*To.ToString(),*Best.ToString(),Score);
    // Both hosts publish only an accepted actor transform. The subsequent
    // support/hull integration may refuse this candidate; publishing it here
    // made later squad members avoid a body position that never existed.
    return To;
}

void FrontlineMobilityV41::RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    FString UserDir;
    if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV41Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())
    {Check(TEXT("mobility_requires_isolated_review"),false);return;}
    const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;const float Clock=B->Elapsed;
    TArray<AWarfareFrontlineSoldier*> Fixtures;B->Units.Empty();B->Squads.Empty();
    ON_SCOPE_EXIT{for(auto* U:Fixtures)if(IsValid(U))U->Destroy();B->Units=SavedUnits;B->Squads=SavedSquads;B->Elapsed=Clock;Reset(B);};
    for(int32 I=0;I<16;++I)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
        U->Initialize(B,94100+I,0,EFrontlineRole::Rifle,I%6);U->SetActorTickEnabled(false);B->Units.Add(U);Fixtures.Add(U);
    }
    Check(TEXT("sixteen_real_infantry_actors_for_local_movement"),Fixtures.Num()==16);if(Fixtures.Num()!=16)return;
    bool Finite=true,Forward=true,Bounded=true,Separated=true,Progress=true;
    for(int32 Team=0;Team<2;++Team)
    {
        const int32 Dir=B->GetTeamDirection(Team);Reset(B);
        for(int32 I=0;I<16;++I)
        {
            auto* U=Fixtures[I];U->Team=Team;const float X=I<12?(-1000-(I/4)*145)*Dir:0;
            const float Y=(I%4-1.5f)*145;U->SetActorLocation(FVector(X,Y,288));
        }
        for(int32 Frame=0;Frame<160;++Frame)
        {
            B->Elapsed+=.05f;
            for(int32 I=0;I<12;++I)
            {
                auto* U=Fixtures[I];const FVector Before=U->Feet();const FVector Next=Step(U,FVector(1400*Dir,(I%4-1.5f)*145,200),19.5f,true);
                Finite&=!Next.ContainsNaN();Forward&=(Next.X-Before.X)*Dir>=-.001;Bounded&=FVector::Dist2D(Before,Next)<=19.501;
                U->SetActorLocation(Next+FVector(0,0,88));
            }
            for(int32 I=0;I<16;++I)for(int32 J=I+1;J<16;++J)Separated&=FVector::DistSquared2D(Fixtures[I]->Feet(),Fixtures[J]->Feet())>=FMath::Square(55.4f);
        }
        for(int32 I=0;I<12;++I)Progress&=Fixtures[I]->Feet().X*Dir>450.f;
    }
    Check(TEXT("crowded_passage_both_directions_is_finite_forward_and_speed_bounded"),Finite&&Forward&&Bounded);
    Check(TEXT("crowded_passage_preserves_body_clearance"),Separated);
    Check(TEXT("all_twelve_movers_pass_four_stationary_neighbours"),Progress);
    auto* U=Fixtures[0];auto* Other=Fixtures[1];
    B->Units={U,Other};U->Team=0;Other->Team=1;U->SetActorLocation(FVector(0,0,288));Other->SetActorLocation(FVector(200,800,288));Reset(B);
    const int32 Dir=B->GetTeamDirection(0);const FVector Next=Step(U,U->Feet()+FVector(1000*Dir,0,0),20,true);
    Check(TEXT("enemy_in_another_lane_does_not_create_a_900cm_invisible_wall"),FMath::IsNearlyEqual(float((Next.X-U->Feet().X)*Dir),20.f,.001f));
    FFrontlineSquad S;S.Id=U->SquadId;S.Team=0;S.Trench=0;S.Slot=1;S.Members.Add(U);S.Members.Add(Other);B->Squads.Add(S);
    U->Role=EFrontlineRole::Rifle;const float Y=B->SoldierDestination(U).Y;U->Role=EFrontlineRole::Marksman;
    Check(TEXT("role_replacement_does_not_move_original_formation_lane"),FMath::IsNearlyEqual(Y,float(B->SoldierDestination(U).Y),.001f));
    U->Role=EFrontlineRole::Flamethrower;U->MemberIndex=0;U->bMoving=false;
    Check(TEXT("flame_operator_bullet_probability_and_pressure_are_modestly_reduced"),FMath::IsNearlyEqual(B->IncomingBulletHitChanceScale(U),.88f)&&FMath::IsNearlyEqual(B->IncomingBulletPressureScale(U),.90f)&&U->Health==100);
    U->MemberIndex=1;Check(TEXT("flame_rifle_escort_does_not_gain_operator_protection"),B->IncomingBulletHitChanceScale(U)==1.f);
}

void FrontlineMobilityV41::RunTerrainRouteChecksV54(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto Flat=[](float,float){return 1.f;};
    const auto Marsh=[](float,float Y){return FMath::Abs(Y)<350.f?.73f:1.f;};
    const auto Canal=[](float X,float){return FMath::Abs(X)<550.f?.79f:1.f;};
    const auto EdgeMarsh=[](float,float Y){return FMath::Abs(Y-700.f)<330.f?.73f:1.f;};
    Check(TEXT("v54_flat_route_keeps_authored_slot_in_both_directions"),
        SelectTerrainLaneV54(-2000,2000,0,0,Flat)==0.f
        &&SelectTerrainLaneV54(2000,-2000,0,0,Flat)==0.f);
    const float East=SelectTerrainLaneV54(-2000,2000,0,0,Marsh);
    const float West=SelectTerrainLaneV54(2000,-2000,0,0,Marsh);
    Check(TEXT("v54_both_factions_select_same_dry_marsh_bypass"),
        East==450.f&&West==East);
    Check(TEXT("v54_continuous_canal_cut_is_crossed_without_futile_lateral_detour"),
        SelectTerrainLaneV54(-2000,2000,0,0,Canal)==0.f);
    Check(TEXT("v54_route_keeps_formation_inside_frontage"),
        SelectTerrainLaneV54(-2000,2000,700,700,EdgeMarsh)<0.f
        &&FMath::Abs(RouteYV54(700,700,-450,.5f))<FrontlineBattlefield::HalfFrontage-350.f);
    Check(TEXT("v54_short_trench_to_trench_leg_stays_in_assigned_slot"),
        SelectTerrainLaneV54(-500,500,0,0,Marsh)==0.f);
    const auto FordCut=[](float X,float Y){return FMath::Abs(X+300.f)<450.f&&FMath::Abs(Y)<480.f?.72f:1.f;};
    const float LysEast=SelectTerrainLaneV54(-2000,0,0,0,FordCut,true,.82f);
    const float LysWest=SelectTerrainLaneV54(0,-2000,0,0,FordCut,true,.18f);
    Check(TEXT("v57_lys_route_uses_a_real_end_of_leg_ford_in_both_directions"),
        FMath::Abs(LysEast)==720.f&&FMath::Abs(LysWest)==720.f
        &&FMath::Abs(RouteYV54(0,0,LysEast,.82f,.82f))>650.f
        &&FMath::IsNearlyZero(RouteYV54(0,0,LysEast,1.f,.82f),.01f));
    const auto Blend=[](float Dist)
    {
        const float T=FMath::Clamp((Dist-330.f)/420.f,0.f,1.f);
        return T*T*(3.f-2.f*T);
    };
    Check(TEXT("v54_tactical_deviation_fades_at_trench_earthworks"),
        Blend(300.f)==0.f&&Blend(750.f)==1.f&&FMath::Abs(RouteYV54(0,0,630,0.f))<.001f
        &&FMath::Abs(RouteYV54(0,0,630,1.f))<.001f);
    {
        int32 PreviousOp=0;EFrontlineMissionTerrainV32 PreviousStyle;
        FrontlineFieldV32::TerrainProfileV43(PreviousOp,PreviousStyle);
        const float PreviousDepth=FrontlineBattlefield::OperationDepthScale;
        const int32 PreviousOperation=FrontlineBattlefield::OperationIndex;
        ON_SCOPE_EXIT
        {
            FrontlineFieldV32::TerrainProfileV43(PreviousOp,PreviousStyle,true);
            FrontlineBattlefield::ConfigureOperationDepth(PreviousDepth,PreviousOperation);
        };
        FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(17),17);
        int32 SelleOp=17;auto SelleStyle=EFrontlineMissionTerrainV32::RailCutting;
        FrontlineFieldV32::TerrainProfileV43(SelleOp,SelleStyle,true);
        const float Home=FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
        const float Opposing=FrontlineBattlefield::TrenchX(1);
        const auto Actual=[](float X,float Y){return FrontlineTerrainTacticsV54::InfantryTravelScale(FVector(X,Y,0.f));};
        const float SelleEast=SelectTerrainLaneV54(-Home,Opposing,0.f,0.f,Actual);
        const float SelleWest=SelectTerrainLaneV54(Opposing,-Home,0.f,0.f,Actual);
        UE_LOG(LogTemp,Display,TEXT("WF_V57_SELLE_ROUTE home=%.1f opposing=%.1f east=%.1f west=%.1f"),Home,Opposing,SelleEast,SelleWest);
        Check(TEXT("v56_selle_both_factions_select_the_physical_dry_causeway"),SelleEast>0.f&&SelleWest==SelleEast);
    }
    {
        int32 PreviousOp=0;EFrontlineMissionTerrainV32 PreviousStyle;
        FrontlineFieldV32::TerrainProfileV43(PreviousOp,PreviousStyle);
        const float PreviousDepth=FrontlineBattlefield::OperationDepthScale;
        const int32 PreviousOperation=FrontlineBattlefield::OperationIndex;
        ON_SCOPE_EXIT
        {
            FrontlineFieldV32::TerrainProfileV43(PreviousOp,PreviousStyle,true);
            FrontlineBattlefield::ConfigureOperationDepth(PreviousDepth,PreviousOperation);
        };
        FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(12),12);
        int32 LysOp=12;auto LysStyle=EFrontlineMissionTerrainV32::Marsh;
        FrontlineFieldV32::TerrainProfileV43(LysOp,LysStyle,true);
        const float Home=FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
        const float Middle=FrontlineBattlefield::TrenchX(1);
        const auto Actual=[](float X,float Y){return FrontlineTerrainTacticsV54::InfantryTravelScale(FVector(X,Y,0.f));};
        const float RiverAxis=-.45f*FMath::Max(1900.f,Home);
        const float CrossingEast=FMath::Clamp((RiverAxis+Home)/(Middle+Home),.18f,.82f);
        const float CrossingWest=FMath::Clamp((RiverAxis-Middle)/(-Home-Middle),.18f,.82f);
        const float LysRouteEast=SelectTerrainLaneV54(-Home,Middle,0.f,0.f,Actual,true,CrossingEast);
        const float LysRouteWest=SelectTerrainLaneV54(Middle,-Home,0.f,0.f,Actual,true,CrossingWest);
        UE_LOG(LogTemp,Display,TEXT("WF_V58_LYS_ROUTE east=%.0f west=%.0f center_scale=%.3f north_scale=%.3f south_scale=%.3f crossing_east=%.3f crossing_west=%.3f"),
            LysRouteEast,LysRouteWest,Actual(RiverAxis,0.f),Actual(RiverAxis,720.f),Actual(RiverAxis,-720.f),CrossingEast,CrossingWest);
        Check(TEXT("v57_lys_actual_wash_makes_both_armies_select_a_dry_ford"),
            FMath::Abs(LysRouteEast)>=450.f&&FMath::Abs(LysRouteWest)>=450.f);
    }
}
