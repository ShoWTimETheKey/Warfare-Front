#include "WarfareFrontlineSupportV26.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineBattlefield.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

TSharedRef<FJsonObject> RunFrontlineV26SupportChecks(AWarfareFrontlineGameMode* B)
{
    const auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;bool Passed=true;
    auto Check=[&](const TCHAR* Name,bool Pass,const FString& Detail=FString())
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);J->SetStringField(TEXT("detail"),Detail);
        Checks.Add(MakeShared<FJsonValueObject>(J));Passed&=Pass;UE_LOG(LogTemp,Display,TEXT("WF_V26_SUPPORT %s %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
    };
    FString UserDir;
    const bool Isolated=B && FParse::Param(FCommandLine::Get(),TEXT("WFV26Review")) &&
        FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) && !UserDir.IsEmpty();
    Check(TEXT("explicit_isolated_support_fixture"),Isolated);
    if(!Isolated){Report->SetBoolField(TEXT("success"),false);Report->SetArrayField(TEXT("checks"),Checks);return Report;}
    using namespace FrontlineFireSupportV26;
    bool Timing=true,Groups=true,Scatter=true,Repeatable=true,Variable=false,Observed=true,Surveyed=true,ExtraRound=true;
    double MinimumX=DBL_MAX,MaximumX=-DBL_MAX,MinimumY=DBL_MAX,MaximumY=-DBL_MAX;int32 Samples=0;
    for(int32 Type=0;Type<2;++Type)for(int32 Seed=0;Seed<12;++Seed)
    {
        const auto Base=Plan(Type!=0,191726+Seed),Again=Plan(Type!=0,191726+Seed),Other=Plan(Type!=0,191727+Seed);
        const auto Faster=Plan(Type!=0,191726+Seed,true),Tighter=Plan(Type!=0,191726+Seed,false,true),Extra=Plan(Type!=0,191726+Seed,false,false,true);
        const auto Extent=ScatterExtent(Type!=0),Range=DelayRange(Type!=0);
        Groups&=Base.Num()==Salvos*NormalRoundsPerSalvo;ExtraRound&=Extra.Num()==Salvos*NormalRoundsPerSalvo+1 && Extra.Last().Salvo==Salvos-1 && Extra.Last().Round==3;
        Timing&=Base[0].ImpactDelay>=Range.X && Base[0].ImpactDelay<=Range.Y && Base.Last().ImpactDelay<40;
        for(int32 I=0;I<Base.Num();++I)
        {
            const auto& S=Base[I];++Samples;
            Groups&=S.Salvo==I/3 && S.Round==I%3;
            Timing&=S.ImpactDelay-S.TerminalFlight>5 && S.WarningLead>0 && S.WarningLead<S.TerminalFlight;
            if(I)Timing&=S.ImpactDelay>Base[I-1].ImpactDelay;
            if(I && I%3==0)Timing&=S.ImpactDelay-Base[I-3].ImpactDelay>=4.999f;
            Scatter&=FMath::Square(S.Offset.X/Extent.X)+FMath::Square(S.Offset.Y/Extent.Y)<=1.00001;
            Repeatable&=S.Offset.Equals(Again[I].Offset,.0001) && FMath::IsNearlyEqual(S.ImpactDelay,Again[I].ImpactDelay,.0001f);
            Variable|=!S.Offset.Equals(Other[I].Offset,10.);
            Observed&=FMath::IsNearlyEqual(S.ImpactDelay-Faster[I].ImpactDelay,2.f,.0001f) && S.Offset.Equals(Faster[I].Offset,.0001);
            if(Type)Surveyed&=(S.Offset*.6).Equals(Tighter[I].Offset,.0001);
            MinimumX=FMath::Min(MinimumX,S.Offset.X);MaximumX=FMath::Max(MaximumX,S.Offset.X);
            MinimumY=FMath::Min(MinimumY,S.Offset.Y);MaximumY=FMath::Max(MaximumY,S.Offset.Y);
        }
    }
    Check(TEXT("four_separate_three_round_salvos"),Groups);
    Check(TEXT("long_wait_and_separated_salvo_schedule"),Timing);
    Check(TEXT("random_scatter_is_bounded_and_materially_wider"),Scatter && MaximumX-MinimumX>1100 && MaximumY-MinimumY>400,
        FString::Printf(TEXT("%d rounds, observed X span %.1fcm Y span %.1fcm"),Samples,MaximumX-MinimumX,MaximumY-MinimumY));
    Check(TEXT("same_seed_reproducible_different_orders_vary"),Repeatable && Variable);
    Check(TEXT("observation_removes_only_two_seconds"),Observed);
    Check(TEXT("german_survey_keeps_forty_percent_scatter_reduction"),Surveyed);
    Check(TEXT("smoke_coordination_adds_one_final_round_only"),ExtraRound);
    Check(TEXT("edge_reflection_does_not_stack_impacts_on_boundary"),ReflectInside(12080,12000)==11920 && ReflectInside(12140,12000)==11860 && ReflectInside(-12140,12000)==-11860);

    // Invoke the same public order APIs used by the player and AI, with existing
    // research intact. Save/restore only local fixture state; never world-tick.
    const auto OriginalPhase=B->Phase;const float OriginalTime=B->Elapsed;
    const auto OriginalStrikes=B->Strikes;const auto OriginalZones=B->SmokeZones;const FString OriginalMessage=B->LastMessage;
    const int32 OriginalArtillery=B->ArtilleryCommands,OriginalMortar=B->MortarCommands,OriginalSmoke=B->SmokeCommands;
    const float ArtyReady[2]={B->ArtilleryReadyAt[0],B->ArtilleryReadyAt[1]},MortarReady[2]={B->MortarReadyAt[0],B->MortarReadyAt[1]},SmokeReady[2]={B->SmokeReadyAt[0],B->SmokeReadyAt[1]};
    TSet<AWarfareFrontlineSmoke*> ExistingSmoke;for(TActorIterator<AWarfareFrontlineSmoke> It(B->GetWorld());It;++It)ExistingSmoke.Add(*It);
    B->Phase=EFrontlinePhase::Fighting;B->Elapsed=1000;B->Strikes.Empty();B->SmokeZones.Empty();
    B->ArtilleryReadyAt[0]=B->MortarReadyAt[0]=B->SmokeReadyAt[0]=0;
    const FVector Target(6000,0,0);const auto ArtyDelay=B->FireSupportDelayRange(EFrontlineSupport::Artillery,Target);
    const bool Arty=B->RequestArtillery(Target);
    bool ArtyShape=Arty && B->Strikes.Num()==Salvos*NormalRoundsPerSalvo;
    for(const auto& S:B->Strikes)ArtyShape&=!S.bGrenade && !S.bMortar && !S.bResolved && S.FireMissionId>=100000 && S.Radius==FieldGunBlastRadius && S.ImpactAt>=1000+ArtyDelay.X && S.LaunchedAt>1000;
    Check(TEXT("actual_artillery_order_commits_twelve_delayed_rounds"),ArtyShape);
    const int32 Queued=B->Strikes.Num();const float ReadyAt=B->ArtilleryReadyAt[0];
    Check(TEXT("repeat_order_cannot_bypass_support_preparation"),!B->RequestArtillery(Target) && B->Strikes.Num()==Queued && B->ArtilleryReadyAt[0]==ReadyAt);
    B->Strikes.Empty();const auto MortarDelay=B->FireSupportDelayRange(EFrontlineSupport::Mortar,Target);
    const bool Mortar=B->RequestMortar(Target);bool MortarShape=Mortar && B->Strikes.Num()==Salvos*NormalRoundsPerSalvo;
    for(const auto& S:B->Strikes)MortarShape&=S.bMortar && !S.bGrenade && !S.bResolved && S.FireMissionId>=200000 && S.Radius==MortarBlastRadius && S.ImpactAt>=1000+MortarDelay.X;
    Check(TEXT("actual_mortar_order_commits_twelve_delayed_rounds"),MortarShape);
    const auto Fixed=B->Strikes;B->Elapsed+=1;
    bool FixedTargets=B->Strikes.Num()==Fixed.Num();for(int32 I=0;I<Fixed.Num();++I)FixedTargets&=B->Strikes[I].Target==Fixed[I].Target && B->Strikes[I].ImpactAt==Fixed[I].ImpactAt;
    Check(TEXT("orders_store_fixed_world_targets_without_actor_locks"),FixedTargets);
    B->Elapsed=1000;const bool Smoke=B->RequestSmoke(Target);
    const bool SmokeZone=Smoke && B->SmokeZones.Num()==1;
    bool Deferred=false,Deployed=false,Expired=false;
    if(SmokeZone)
    {
        const auto Zone=B->SmokeZones[0];const FVector From=Target-FVector(1000,0,0),To=Target+FVector(1000,0,0);
        Deferred=Zone.StartedAt>=1000+SmokeDelayMin && Zone.StartedAt<=1000+SmokeDelayMax && FMath::IsNearlyEqual(Zone.ExpiresAt-Zone.StartedAt,SmokeDuration,.001f)
            && !B->IsInFriendlySmoke(Target,0) && B->DirectFireMultiplier(From,To)==1.f;
        B->Elapsed=Zone.StartedAt+.2f;Deployed=B->IsInFriendlySmoke(Target,0) && B->DirectFireMultiplier(From,To)==.22f;
        B->Elapsed=Zone.ExpiresAt+.01f;Expired=!B->IsInFriendlySmoke(Target,0) && B->DirectFireMultiplier(From,To)==1.f;
    }
    Check(TEXT("smoke_has_no_cover_before_actual_arrival"),Deferred);
    Check(TEXT("smoke_cover_uses_current_actual_deployment_window"),Deployed && Expired);

    B->Elapsed=1000;B->ArtilleryReadyAt[1]=B->MortarReadyAt[1]=0;
    auto* Friendly=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Target+FVector(950,0,88),FRotator::ZeroRotator);
    bool AISafe=false;
    if(Friendly)
    {
        Friendly->Team=1;Friendly->Health=100;Friendly->bFieldEntered=true;B->Units.Add(Friendly);
        const int32 Before=B->Strikes.Num();
        AISafe=!B->IsFireSupportAreaSafe(Target,EFrontlineSupport::Artillery,1) && !B->RequestArtillery(Target,1)
            && !B->RequestMortar(Target,1) && B->Strikes.Num()==Before && B->ArtilleryReadyAt[1]==0 && B->MortarReadyAt[1]==0;
        B->Units.Remove(Friendly);Friendly->Destroy();
    }
    Check(TEXT("ai_rejects_friendly_in_full_scatter_footprint"),AISafe,TEXT("Friendly950cm from aim centre: beyond old600cm check, inside revised mission footprint."));
    for(TActorIterator<AWarfareFrontlineSmoke> It(B->GetWorld());It;++It)if(!ExistingSmoke.Contains(*It))It->Destroy();
    B->Phase=OriginalPhase;B->Elapsed=OriginalTime;B->Strikes=OriginalStrikes;B->SmokeZones=OriginalZones;B->LastMessage=OriginalMessage;
    B->ArtilleryCommands=OriginalArtillery;B->MortarCommands=OriginalMortar;B->SmokeCommands=OriginalSmoke;
    for(int32 Team=0;Team<2;++Team){B->ArtilleryReadyAt[Team]=ArtyReady[Team];B->MortarReadyAt[Team]=MortarReady[Team];B->SmokeReadyAt[Team]=SmokeReady[Team];}
    Report->SetBoolField(TEXT("success"),Passed);Report->SetArrayField(TEXT("checks"),Checks);Report->SetNumberField(TEXT("planned_round_samples"),Samples);
    Report->SetStringField(TEXT("scope"),TEXT("24 deterministic production-planner missions plus real player/AI order and smoke-window API checks. No world ticking, resolving damage, visual playback or claim that UI hides every impact; UI rendering needs its separate actual review. Existing research retained, gameplay fixture state restored, no player save calls."));
    return Report;
}
