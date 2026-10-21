void RunEngineerFrontageChecksV84412(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    const bool Safe=FrontlineGameplayFocusedV84412::Allowed(B)&&B->Trenches.Num()>=2;
    Check(TEXT("v84412_engineer_frontage_strict_disposable_ukde_profile"),Safe);if(!Safe)return;
    FFrontlineEngineerMineV833Review Armour(B);
    Check(TEXT("v84412_engineer_frontage_actual_fleet_scope"),Armour.Fleet!=nullptr);if(!Armour.Fleet)return;
    const auto SavedFields=Battles;const auto SavedKits=Units;const auto SavedUnits=B->Units;const auto SavedSquads=B->Squads;
    const float Clock=B->Elapsed,FF=B->FriendlyFireDamage,Outro=B->BattleOutroElapsed;const auto Phase=B->Phase;
    const int32 Winner=B->BattleOutroWinner;const auto R0=B->CombatRecordV65[0],R1=B->CombatRecordV65[1];
    TArray<AWarfareFrontlineSoldier*> Made;TSet<TWeakObjectPtr<AWarfareFrontlineImpact>> Impacts;
    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)Impacts.Add(*I);
    ON_SCOPE_EXIT
    {
        FrontlineMobilityV41::Reset(B);for(auto* U:Made)if(IsValid(U))U->Destroy();
        for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(!Impacts.Contains(*I))I->Destroy();
        Battles=SavedFields;Units=SavedKits;B->Units=SavedUnits;B->Squads=SavedSquads;B->Elapsed=Clock;
        B->FriendlyFireDamage=FF;B->CombatRecordV65[0]=R0;B->CombatRecordV65[1]=R1;
        B->Phase=Phase;B->BattleOutroWinner=Winner;B->BattleOutroElapsed=Outro;
    };
    FrontlineAchievementsV76::WithScopedCombatEventsV839(B,[&]
    {
        for(int32 Team=0;Team<2;++Team)
        {
            B->Units.Reset();B->Squads.Reset();Units.Reset();B->Elapsed=1000.f;B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->BattleOutroElapsed=0.f;
            auto& S=Battles.FindOrAdd(B);S=FState();S.Encounter=B->GetEncounterId();S.TrainSpawned=true;S.HazardAt=B->Elapsed;
            const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team),Next=Home+Dir;
            const FVector Centre((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,500.f,0);
            auto MakeMine=[&](float Y)
            {FHazard H;H.Mine=true;H.Radius=FrontlineMineV8214::TriggerRadiusCm;H.Centre=FVector(Centre.X,Y,0);H.Centre.Z=FrontlineInfantryV45::SurfaceHeight(B,H.Centre)+2.f;return H;};
            S.Hazards={MakeMine(875.f)};
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_actual_registered_worker"),Team),U!=nullptr);if(!U)continue;
            Made.Add(U);U->Initialize(B,84412000+Team,Team,EFrontlineRole::Pioneer,0);U->SetActorTickEnabled(false);
            U->bFieldEntered=true;U->FormationOffset=FVector::ZeroVector;U->ReturnToIdle();
            FVector Foot=Centre-FVector(Dir*FrontlineMineV8214::WorkStandOffCmV833,150.f,0);Foot.Z=FrontlineInfantryV45::SurfaceHeight(B,Foot);
            U->SetActorLocation(Foot+FVector(0,0,88.f));U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));B->Units={U};
            FFrontlineSquad Q;Q.Id=U->SquadId;Q.Team=Team;Q.Role=EFrontlineRole::Pioneer;Q.bAdvancing=true;Q.Destination=Next;Q.Members={U};B->Squads={Q};
            FVector Destination=B->SoldierDestination(U);
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_initial_real_y_search_exceeds_old_400cm"),Team),
                FMath::Abs(S.Hazards[0].Centre.Y-U->Feet().Y)>FrontlineMineV8214::SearchFrontageCmV833
                &&FMath::Abs(S.Hazards[0].Centre.Y-U->Feet().Y)<=MineSearchFrontageV84412(B)
                &&HasMineApproachV833(U)&&HasReachableHazardV52(U)&&Units.FindChecked(U).MineIndexV833==0);
            Units.Remove(U);S.Hazards={MakeMine(500.f),MakeMine(1100.f),MakeMine(2300.f)};
            Destination=B->SoldierDestination(U);
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_reserves_nearby_real_charge"),Team),HasMineApproachV833(U)&&Units.FindChecked(U).MineIndexV833==0);
            TickHazardsV8219(B,S,0.f);
            for(int32 F=0;F<100&&!S.Hazards[0].Cleared;++F){B->Elapsed+=.05f;TickHazardsV8219(B,S,.05f);}
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_first_charge_real_work_records_ground"),Team),S.Hazards[0].Cleared&&S.ClearanceBurstsV8311==1&&S.Hazards[0].ClearedSquadV84412==U->SquadId);
            Destination=B->SoldierDestination(U);
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_reuses_worked_ground_beyond_initial_600cm_lane"),Team),
                FMath::Abs(S.Hazards[1].Centre.Y-U->Feet().Y)>MineSearchFrontageV84412(B)
                &&HasMineApproachV833(U)&&Units.FindChecked(U).MineIndexV833==1&&!S.Hazards[1].Cleared);
            const float Untouched=S.Hazards[1].Work;
            for(int32 F=0;F<20;++F){B->Elapsed+=.05f;TickHazardsV8219(B,S,.05f);}
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_lateral_discovery_does_not_clear_at_distance"),Team),S.Hazards[1].Work==Untouched&&!S.Hazards[1].Cleared&&S.ClearanceBurstsV8311==1);
            bool Approached=false;
            for(int32 F=0;F<240&&!S.Hazards[1].Cleared;++F)
            {
                B->Elapsed+=.05f;U->Tick(.05f);TickHazardsV8219(B,S,.05f);
                if(S.Hazards[1].Work>0.f)Approached|=FVector::DistSquared2D(U->Feet(),S.Hazards[1].Centre)<=FMath::Square(FrontlineMineV8214::WorkReachCmV833);
            }
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_real_lateral_movement_stable_work_and_second_fx"),Team),Approached&&S.Hazards[1].Cleared&&S.ClearanceBurstsV8311==2&&U->IsAlive()&&S.TriggeredMines==0);
            Check(FString::Printf(TEXT("v84412_engineer_team_%d_remote_y_lane_stays_unknown_and_armed"),Team),!MineWithinWorkedFrontageV84412(U,S,S.Hazards[2].Centre)&&!S.Hazards[2].Cleared&&S.Hazards[2].Work==0.f);
            U->Destroy();
        }
    });
}
