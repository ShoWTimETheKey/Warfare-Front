#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AWarfareFrontlineGameMode::RunV20TrenchChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV20Review")))return;
    Check(TEXT("isolated_campaign_profile"),bProgressionVolatile);
    if(!bProgressionVolatile || !AssetFailure.IsEmpty())return;
    auto Reset=[&](){ResetEncounter();BeginBattle();};
    auto Add=[&](int32 Team,int32 Trench,EFrontlineRole UnitRole=EFrontlineRole::Rifle)
    {return SpawnSquad(Team,UnitRole,Trench)?Squads.Last().Id:INDEX_NONE;};
    auto Move=[&](int32 Id,float X)
    {
        if(auto* S=FindSquad(Id))for(const auto& Ref:S->Members)if(Ref.IsValid())
        {const float Y=Ref->Feet().Y;Ref->SetActorLocation(FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)+88.f));}
    };
    auto Arrive=[&](int32 Id)
    {
        if(auto* S=FindSquad(Id))for(const auto& Ref:S->Members)if(Ref.IsValid())
            Ref->SetActorLocation(SoldierDestination(Ref.Get())+FVector(0,0,88));
        UpdateSquads();
    };
    for(int32 Team=0;Team<2;++Team)
    {
        Reset();const int32 Home=Team?4:0,Next=Team?3:1,Dir=Team?-1:1;
        const int32 Walker=Add(Team,Home),Stays=Add(Team,Home,EFrontlineRole::Lewis);
        bool Filled=true;for(int32 I=0;I<3;++I)Filled&=Add(Team,Next)!=INDEX_NONE;
        const bool Ordered=AdvanceSquad(Home,Walker,Team);
        Check(FString::Printf(TEXT("team_%d_full_front_trench_does_not_block_single_section_exit"),Team),Filled&&Ordered
            &&FindSquad(Stays)&&FindSquad(Stays)->Trench==Home&&TrenchSquadCount(Next,Team)==3);
        Move(Walker,Trenches[Next].X+Dir*220.f);UpdateSquads();
        auto* S=FindSquad(Walker);
        Check(FString::Printf(TEXT("team_%d_crossed_full_trench_without_fourth_garrison"),Team),S&&S->bAdvancing&&S->Trench==INDEX_NONE
            &&S->LastPassedTrench==Next&&S->Destination==2&&TrenchSquadCount(Next,Team)==3);
        Arrive(Walker);S=FindSquad(Walker);
        Check(FString::Printf(TEXT("team_%d_enters_first_available_trench_after_crossing"),Team),S&&S->Trench==2&&!S->bAdvancing&&S->Slot>=0&&S->Slot<3);
    }
    Reset();const int32 Chain=Add(0,0);
    Trenches[2].Owner=0;
    bool Full=true;for(int32 T:{1,2})for(int32 I=0;I<3;++I)Full&=Add(0,T)!=INDEX_NONE;
    const bool Ordered=AdvanceSquad(0,Chain,0);
    Move(Chain,Trenches[2].X+220.f);UpdateSquads();
    Check(TEXT("continuous_full_friendly_lines_can_be_crossed"),Full&&Ordered&&FindSquad(Chain)&&FindSquad(Chain)->Destination==3
        &&TrenchSquadCount(1,0)==3&&TrenchSquadCount(2,0)==3);
    Arrive(Chain);
    Check(TEXT("empty_enemy_historical_trench_is_capturable"),FindSquad(Chain)&&FindSquad(Chain)->Trench==3&&Trenches[3].Owner==0);

    Reset();Trenches[1].Owner=1;TArray<int32> Converging;
    for(int32 I=0;I<6;++I)
    {
        const int32 Id=Add(0,0);auto* S=FindSquad(Id);
        // Six moving groups reach an abandoned enemy line, three formation lanes.
        // This is an explicit contention fixture; no fourth resident is created.
        if(S){S->Trench=INDEX_NONE;S->bAdvancing=true;S->Destination=1;S->LastPassedTrench=0;S->Slot=I%3;Move(Id,Trenches[1].X);Converging.Add(Id);}
    }
    UpdateSquads();int32 ForwardMoving=0;
    for(int32 Id:Converging)if(const auto* S=FindSquad(Id))if(S->bAdvancing&&S->Destination==2)++ForwardMoving;
    Check(TEXT("six_converging_sections_do_not_deadlock_empty_enemy_trench"),Converging.Num()==6&&Trenches[1].Owner==0
        &&TrenchSquadCount(1,0)==3&&ForwardMoving==3);

    Reset();const int32 InFlight=Add(0,0);AdvanceSquad(0,InFlight,0);
    // Simulate an in-flight occupancy change, independent of reservation policy.
    FindSquad(InFlight)->bAdvancing=false;
    Full=true;for(int32 I=0;I<3;++I)Full&=Add(0,1)!=INDEX_NONE;
    FindSquad(InFlight)->bAdvancing=true;UpdateSquads();
    Check(TEXT("newly_full_destination_is_replanned_during_march"),Full&&FindSquad(InFlight)->bAdvancing&&FindSquad(InFlight)->Destination==2);
    // Occupancy changes again before arrival: a living enemy, not the historical
    // owner label, must prevent the route from skipping the now-hostile line.
    for(auto& S:Squads)if(S.Trench==1){S.Team=1;for(auto& Ref:S.Members)if(Ref.IsValid())Ref->Team=1;}
    Trenches[1].Owner=1;UpdateSquads();
    const auto* March=FindSquad(InFlight);
    const FVector Halt=March&&!March->Members.IsEmpty()?SoldierDestination(March->Members[0].Get()):FVector::ZeroVector;
    Check(TEXT("enemy_occupying_intermediate_trench_interrupts_skip_route"),March&&March->Destination==1&&Halt.X<Trenches[1].X-300.f&&March->Trench==INDEX_NONE);

    Reset();const int32 Garrison=Add(0,0,EFrontlineRole::Lewis);
    const bool Closed=SetTrenchBypass(0,true,0);
    const bool Reinforced=Recruit(EFrontlineRole::Rifle,0);
    Check(TEXT("entry_gate_leaves_garrison_and_routes_new_reinforcement_onward"),Closed&&Reinforced&&FindSquad(Garrison)->Trench==0
        &&!FindSquad(Garrison)->bAdvancing&&Squads.Last().Destination==1);
    Check(TEXT("entry_gate_reopens_without_enemy_control"),SetTrenchBypass(0,false,0)&&!Trenches[0].bBypass&&!SetTrenchBypass(4,true,0));

    // Leave a representative fixture for the real HUD clicks and screenshots.
    Reset();for(EFrontlineRole UnitRole:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills})Add(0,0,UnitRole);
    for(EFrontlineRole UnitRole:{EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Marksman})Add(0,1,UnitRole);
    EnemyRecruitAt=EnemyThinkAt=EnemyFireAssessAt=MAX_flt;EnemyFireExecuteAt=0;
    for(AWarfareFrontlineSoldier* U:Units)if(U)U->SetActorTickEnabled(false);
    LastMessage.Empty();SetActorTickEnabled(false);
}
