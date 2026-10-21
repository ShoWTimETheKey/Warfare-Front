// Included by SupportV28.cpp: reviews use the actual mission and hit methods.
namespace FrontlineAirSupportReviewV8220
{
bool AppendMetric(AWarfareFrontlineGameMode* B,const TCHAR* Group,TSharedRef<FJsonObject> Metric)
{
    FString Dir;if(!FrontlineEffectsReviewSafetyV8216::Allowed(B)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir))return false;
    const FString Path=Dir/TEXT("air-support-metrics.json");TSharedPtr<FJsonObject> Document;
    if(FPaths::FileExists(Path))
    {
        FString Prior;if(!FFileHelper::LoadFileToString(Prior,*Path)
            ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Prior),Document)||!Document.IsValid())return false;
    }
    else Document=MakeShared<FJsonObject>();
    const TArray<TSharedPtr<FJsonValue>>* PriorRows=nullptr;TArray<TSharedPtr<FJsonValue>> Rows;
    if(Document->TryGetArrayField(Group,PriorRows))Rows=*PriorRows;
    Metric->SetNumberField(TEXT("battle_elapsed"),B->Elapsed);Metric->SetNumberField(TEXT("committed_clock"),B->CommittedSupportTimeV8217());
    Metric->SetNumberField(TEXT("phase"),int32(B->Phase));Metric->SetNumberField(TEXT("player_faction"),int32(B->GetPlayerFaction()));
    Rows.Add(MakeShared<FJsonValueObject>(Metric));Document->SetArrayField(Group,Rows);
    Document->SetStringField(TEXT("scope"),TEXT("Actual disposable V8220 production aviation calls. No player archive or production gameplay parameter writes."));
    FString JSON;if(!FJsonSerializer::Serialize(Document.ToSharedRef(),TJsonWriterFactory<>::Create(&JSON)))return false;
    return FFileHelper::SaveStringToFile(JSON,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}
struct FFrontlineAirSupportV8220Review
{
    static bool Allowed(AWarfareFrontlineGameMode* B)
    {
        FString Dir;return FrontlineEffectsReviewSafetyV8216::Allowed(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV8220Review"))&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
    }
    static void Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        Check(TEXT("v8220_air_support_requires_exact_disposable_nosave_profile"),Allowed(B));if(!Allowed(B))return;
        auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        Check(TEXT("v8220_air_support_has_actual_field_and_vehicle_runtime"),Fleet&&B->Trenches.Num()>=2);if(!Fleet||B->Trenches.Num()<2)return;
        const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;
        const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Vehicles=Fleet->Vehicles;
        const auto Phase=B->Phase;const float Clock=B->Elapsed,Outro=B->BattleOutroElapsed;
        const int32 Winner=B->BattleOutroWinner,Serial=Fleet->NextSerial,Commands=B->ExtendedSupportCommands[2];
        const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];
        const auto Random=B->Random;const auto Tactics=FrontlineDifficultyV30::State(B);
        const int32 Cas0=B->Casualties[0],Cas1=B->Casualties[1];
        const float Damage0=B->DamageDealt[0],Damage1=B->DamageDealt[1],Morale0=B->Morale[0],Morale1=B->Morale[1];
        const float Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;const FString Message=B->LastMessage;
        auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>();const bool AudioActive=Audio&&Audio->IsBattleActive();
        TSet<TWeakObjectPtr<AActor>> Prior;for(TActorIterator<AActor> I(B->GetWorld());I;++I)Prior.Add(*I);
        auto ClearAdded=[&]()
        {
            TArray<AActor*> Added;for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(!Prior.Contains(*I))Added.Add(*I);
            for(auto* A:Added){if(Audio)Audio->UpdateAircraftAudio(int32(A->GetUniqueID()),A->GetActorLocation(),0.f,false);A->Destroy();}
        };
        ON_SCOPE_EXIT
        {
            ClearAdded();B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;B->Strikes=Strikes;B->SmokeZones=Smoke;
            Fleet->Vehicles=Vehicles;Fleet->NextSerial=Serial;B->Phase=Phase;B->Elapsed=Clock;B->BattleOutroElapsed=Outro;B->BattleOutroWinner=Winner;
            B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->Casualties[0]=Cas0;B->Casualties[1]=Cas1;
            B->DamageDealt[0]=Damage0;B->DamageDealt[1]=Damage1;B->Morale[0]=Morale0;B->Morale[1]=Morale1;
            B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;B->LastMessage=Message;B->Random=Random;
            B->ExtendedSupportCommands[2]=Commands;FrontlineDifficultyV30::State(B)=Tactics;
            if(Audio)Audio->SetBattleActive(AudioActive);
        };
        auto ResetCase=[&]()
        {
            ClearAdded();B->Units.Empty();B->Squads.Empty();B->Strikes.Empty();B->SmokeZones.Empty();Fleet->Vehicles.Empty();
            B->Trenches=Trenches;B->Phase=EFrontlinePhase::Fighting;B->Elapsed=100.f;B->BattleOutroElapsed=0;B->BattleOutroWinner=INDEX_NONE;
            B->CombatRecordV65[0]=FFrontlineCombatRecordV65();B->CombatRecordV65[1]=FFrontlineCombatRecordV65();
            B->Casualties[0]=B->Casualties[1]=0;B->DamageDealt[0]=B->DamageDealt[1]=B->FriendlyFireDamage=B->PeakSuppression=0;
            B->Morale[0]=B->Morale[1]=100;B->ExtendedSupportCommands[2]=0;B->Random.Initialize(8220001);
        };
        auto Make=[&](int32 Team,const FVector& Feet,int32 Trench=INDEX_NONE)
        {
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
            const int32 Id=822000+B->Units.Num();U->Initialize(B,Id,Team,EFrontlineRole::Rifle,0);
            U->SetActorTickEnabled(false);U->bFieldEntered=true;U->bMoving=false;U->Suppression=0;U->SetActorLocation(Feet+FVector(0,0,88));
            B->Units.Add(U);FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Trench=Trench;Q.Members.Add(U);B->Squads.Add(Q);return U;
        };
        const int32 Trench=FMath::Clamp(FrontlineBattlefield::CentreTrench,0,B->Trenches.Num()-2);
        const int32 Dir=B->GetTeamDirection(0);
        const float OpenX=(Trenches[Trench].X+Trenches[Trench+1].X)*.5f;
        const FVector Open(OpenX,0,AWarfareFrontlineGround::HeightAt(OpenX,0));
        struct FResult {int32 Rounds=0,Kills=0,Credited=0;bool Done=false;};
        auto RunStrafe=[&](float Step,bool Covered,bool Ending,bool Armour,int32 SeedOffset)
        {
            ResetCase();B->ExtendedSupportCommands[2]=SeedOffset;
            FVector Target=Covered?FVector(Trenches[Trench].X,0,AWarfareFrontlineGround::HeightAt(Trenches[Trench].X,0)):Open;
            B->Trenches[Trench].Owner=1;
            for(int32 I=0;I<32;++I)
            {
                const float Y=(I/2-7.5f)*160.f;
                const float X=(Covered?FrontlineFieldV32::TrenchCentreX(Trench,Y):Target.X)+(I%2?35.f:-35.f);
                Make(1,FVector(X,Y,AWarfareFrontlineGround::HeightAt(X,Y)),Covered?Trench:INDEX_NONE);
            }
            bool ActualCover=true;for(AWarfareFrontlineSoldier* U:B->Units)ActualCover&=U&&(Covered?B->CoverFor(U)>.5f:B->CoverFor(U)==0.f);
            Check(FString::Printf(TEXT("v8220_strafe_actual_%s_fixture_%.2f_seed%d"),Covered?TEXT("trench"):TEXT("open"),Step,SeedOffset),ActualCover&&B->Units.Num()==32);
            auto* Tank=Armour?Fleet->Spawn(B->GetFactionForTeam(1)==EFrontlineFaction::German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,1,true):nullptr;
            float Hull=0;
            if(Tank){Tank->SetActorLocation(Target);Tank->FitGroundV35(Target,0);Tank->bEntered=true;Tank->SetActorTickEnabled(false);Hull=Tank->HullIntegrity;}
            auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            const bool Ready=Air&&Air->Configure(B,EFrontlineSupport::AirStrike,Target,0);
            Check(TEXT("v8220_strafe_configures_registered_actual_aircraft"),Ready&&Air->Visuals.Num()==1&&Air->Visuals[0]->IsRegistered()&&Air->Visuals[0]->GetStaticMesh());
            FResult Result;if(!Ready)return Result;
            const float End=Air->RequestedAt+Air->Arrival+16.f;
            const float Freeze=Air->RequestedAt+Air->Arrival+3.f;
            for(int32 Frame=1;100.f+Frame*Step<=End+Step*.5f;++Frame)
            {
                const float Time=FMath::Min(End,100.f+Frame*Step);
                if(Ending&&Time>=Freeze){B->Elapsed=Freeze;B->Phase=EFrontlinePhase::Victory;B->BattleOutroWinner=0;B->BattleOutroElapsed=Time-Freeze;}
                else B->Elapsed=Time;
                Air->Tick(Step);const int32 Before=Air->RoundsFired,Pending=Air->PendingStrafe.Num();Air->Tick(Step);
                if(Before!=Air->RoundsFired||Pending!=Air->PendingStrafe.Num())ActualCover=false;
            }
            Result.Rounds=Air->RoundsFired;Result.Kills=B->Casualties[1];
            Result.Credited=B->CombatRecordV65[0].SupportSources[int32(EFrontlineSupport::AirStrike)].Infantry;
            Result.Done=Air->PendingStrafe.IsEmpty()&&Air->StrafeSlotsV8220[0]==32&&Air->StrafeSlotsV8220[1]==32&&ActualCover;
            Check(FString::Printf(TEXT("v8220_strafe_%s_%.2f_real_arrival_credit_and_same_frame_dedup"),Ending?TEXT("ending"):TEXT("fighting"),Step),Result.Done&&Result.Rounds>=40&&Result.Rounds<=64&&Result.Kills>0&&Result.Credited==Result.Kills&&B->Casualties[0]==0);
            if(Armour)Check(TEXT("v8220_strafe_actual_heavy_armour_does_not_lose_hull_to_aircraft_mg"),Tank&&Tank->AreAssetsReady()&&Tank->IsOperational()&&Tank->HullIntegrity==Hull);
            UE_LOG(LogTemp,Display,TEXT("WF_V8220_STRAFE step=%.2f cover=%d ending=%d seed=%d nominal_legacy_rounds=%d actual_rounds=%d actual_kills=%d credited=%d pending=%d"),Step,Covered,Ending,SeedOffset,2*(FMath::FloorToInt(1.55f/FMath::Max(.05f,Step))+1),Result.Rounds,Result.Kills,Result.Credited,Air->PendingStrafe.Num());
            auto Metric=MakeShared<FJsonObject>();Metric->SetNumberField(TEXT("game_step_seconds"),Step);Metric->SetBoolField(TEXT("covered"),Covered);
            Metric->SetBoolField(TEXT("ending"),Ending);Metric->SetBoolField(TEXT("armour"),Armour);Metric->SetNumberField(TEXT("seed_offset"),SeedOffset);
            Metric->SetNumberField(TEXT("actual_rounds"),Result.Rounds);Metric->SetNumberField(TEXT("actual_kills"),Result.Kills);
            Metric->SetNumberField(TEXT("credited_kills"),Result.Credited);Metric->SetNumberField(TEXT("pending_impacts"),Air->PendingStrafe.Num());
            Metric->SetBoolField(TEXT("completed_and_same_frame_deduplicated"),Result.Done);
            FrontlineAirSupportReviewV8220::AppendMetric(B,TEXT("strafe_cases"),Metric);
            return Result;
        };
        const bool Registered=FrontlineFieldSitesV59::WithAirCoverReviewV8220(B,INDEX_NONE,[&](const FVector&)
        {
            const auto Fine=RunStrafe(.05f,false,false,false,0),Medium=RunStrafe(.2f,false,false,false,0),Fast=RunStrafe(.4f,false,false,false,0);
            Check(TEXT("v8220_strafe_actual_005_020_040_steps_preserve_rounds_and_casualties"),Fine.Done&&Medium.Done&&Fast.Done&&Fine.Rounds==Medium.Rounds&&Fine.Rounds==Fast.Rounds&&Fine.Kills==Medium.Kills&&Fine.Kills==Fast.Kills);
            Check(TEXT("v8220_strafe_recovers_coarse_step_lost_ammunition_without_clearing_broad_infantry_fixture"),Fast.Rounds>2*8&&Fast.Kills>=4&&Fast.Kills<32);
            int32 OpenKills=Fine.Kills,CoveredKills=RunStrafe(.2f,true,false,false,0).Kills;
            for(int32 Seed=1;Seed<=2;++Seed){OpenKills+=RunStrafe(.2f,false,false,false,Seed).Kills;CoveredKills+=RunStrafe(.2f,true,false,false,Seed).Kills;}
            // Real trench floors and assigned squads, rather than a synthetic cover multiplier.
            Check(TEXT("v8220_strafe_has_real_trench_casualties_and_retains_protection"),CoveredKills>0&&OpenKills>CoveredKills);
            UE_LOG(LogTemp,Display,TEXT("WF_V8220_STRAFE_COVER seeds=3 open_kills=%d trench_kills=%d units_per_case=32"),OpenKills,CoveredKills);
            auto CoverMetric=MakeShared<FJsonObject>();CoverMetric->SetNumberField(TEXT("seed_count"),3);CoverMetric->SetNumberField(TEXT("units_per_case"),32);
            CoverMetric->SetNumberField(TEXT("open_kills"),OpenKills);CoverMetric->SetNumberField(TEXT("trench_kills"),CoveredKills);
            FrontlineAirSupportReviewV8220::AppendMetric(B,TEXT("strafe_cover_totals"),CoverMetric);
            RunStrafe(.2f,false,true,true,0);
            ResetCase();auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
            if(Air&&Air->Configure(B,EFrontlineSupport::AirStrike,Open,0))
            {
                // Discharge one actual scheduled round and observe its real queue.
                B->Elapsed=Air->RequestedAt+Air->Arrival+1.35f;Air->Strafe(B->Elapsed-Air->RequestedAt,.05f);
                const bool Shot=!Air->PendingStrafe.IsEmpty();
                Check(TEXT("v8220_strafe_actual_forward_muzzle_and_absolute_arrival_queue"),Shot);
                if(Shot)
                {
                    const auto Round=Air->PendingStrafe.Last();const float Local=Round.LaunchedAtV8220-Air->RequestedAt-Air->Arrival;
                    const auto Flight=FrontlineSupportArtV40::AttackFlight(Open,Dir,Local,0);
                    Check(TEXT("v8220_strafe_sampled_muzzle_and_ballistic_arrival_are_discharge_based"),Round.Origin.Equals(FTransform(Flight.Rotation,Flight.Position).TransformPosition(FVector(287,-31,49)),.1f)&&Round.ImpactAt>Round.LaunchedAtV8220&&Round.ImpactAt-Round.LaunchedAtV8220<.2f);
                    B->Elapsed=Air->PendingStrafe[0].ImpactAt-.0001f;Air->ResolveStrafeRounds();const int32 Pending=Air->PendingStrafe.Num();
                    B->Elapsed=Round.ImpactAt+.001f;Air->ResolveStrafeRounds();int32 Impacts=0;
                    for(TActorIterator<AWarfareFrontlineImpact> I(B->GetWorld());I;++I)if(!Prior.Contains(*I)&&!I->IsActorBeingDestroyed()&&I->GetBlastProfileIndex()==3)++Impacts;
                    Check(TEXT("v8220_strafe_empty_ground_has_real_delayed_directional_impacts"),Pending>0&&Air->PendingStrafe.Num()<Pending&&Impacts>0&&B->Casualties[0]+B->Casualties[1]==0);
                }
            }
            ResetCase();if(Audio)Audio->SetBattleActive(true);
            FrontlineSupportV32::RunAirBombingChecksV8220(B,Check,[&](){B->TickCommittedSupportV8217();});
            ResetCase();bool HiddenAudio=false;
            const bool AAGun=FrontlineFieldSitesV59::WithAirCoverReviewV8220(B,1,[&](const FVector& Gun)
            {
                FVector Aim=Gun+FVector(Dir*4000.f,0,0);Aim.Z=AWarfareFrontlineGround::HeightAt(Aim.X,Aim.Y);
                auto* Driven=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
                if(!Driven||!Driven->Configure(B,EFrontlineSupport::AirStrike,Aim,0))return;
                for(int32 Frame=1;Frame<=110&&!Driven->bDrivenOffV59;++Frame)
                {B->Elapsed=Driven->RequestedAt+Driven->Arrival-3.f+Frame*.05f;Driven->Tick(.05f);}
                const bool ActualInterception=Driven->bDrivenOffV59;
                B->Elapsed=Driven->RequestedAt+Driven->Arrival+13.9f;Driven->Tick(.2f);
                HiddenAudio=ActualInterception&&Driven->Visuals.Num()==1&&!Driven->Visuals[0]->IsVisible()
                    &&Audio&&!Audio->DescribeAircraftAudioV8218(int32(Driven->GetUniqueID()))->GetBoolField(TEXT("playing"));
            });
            Check(TEXT("v8220_strafe_actual_aa_driven_off_hidden_return_has_no_ghost_engine_audio"),AAGun&&HiddenAudio);
        });
        Check(TEXT("v8220_air_checks_restore_registered_field_sites"),Registered);
    }
    static bool Capture(AWarfareFrontlineGameMode* B,int32 Mode)
    {
        if(!Allowed(B))return false;if(Mode==1)return FrontlineSupportV32::SetupAirBombingCaptureV8220(B);
        const FName Tag(TEXT("V8220AirCapture"));
        for(TActorIterator<AActor> I(B->GetWorld());I;++I)if(I->ActorHasTag(Tag))I->Destroy();
        const FVector Target(0,-850,AWarfareFrontlineGround::HeightAt(0,-850));
        auto* Air=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        if(!Air||!Air->Configure(B,EFrontlineSupport::AirStrike,Target,0)){if(Air)Air->Destroy();return false;}
        Air->Tags.Add(Tag);Air->SetActorTickEnabled(false);
        const auto F=FrontlineSupportArtV40::AttackFlight(Target,B->GetTeamDirection(0),2.5f,0);
        Air->Visuals[0]->SetVisibility(true);Air->Visuals[0]->SetWorldTransform(FTransform(F.Rotation,F.Position));
        const FVector Muzzle=FTransform(F.Rotation,F.Position).TransformPosition(FVector(287,-31,49));FVector Point;float Duration=0;
        if(!FrontlineSupportArtV40::ForwardGroundShot(Muzzle,F.Rotation.Vector(),Point,Duration))return false;
        auto* Round=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();
        if(Round){Round->Tags.Add(Tag);Round->SetCommittedSupportV8217();Round->SetGroundImpactEnabled(false);Round->Configure(B,Muzzle,Point,B->CommittedSupportTimeV8217(),B->CommittedSupportTimeV8217()+Duration,false,true);Round->SetActorTickEnabled(false);Round->UpdateVisual(B->CommittedSupportTimeV8217()+Duration*.65f);Round->SetLifeSpan(20.f);}
        if(auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Point,FRotator::ZeroRotator)){Impact->Tags.Add(Tag);Impact->ConfigureStrafeStrike(Point-Muzzle);Impact->SetActorTickEnabled(false);Impact->Tick(.08f);Impact->SetLifeSpan(20.f);}
        Air->SetLifeSpan(20.f);return Round&&Round->IsVisualReady();
    }
};
void RunAirSupportChecksV8220(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{FFrontlineAirSupportV8220Review::Run(B,Check);}
bool SetupAirSupportCaptureV8220(AWarfareFrontlineGameMode* B,int32 Mode)
{return FFrontlineAirSupportV8220Review::Capture(B,Mode);}
