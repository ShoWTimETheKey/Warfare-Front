// Included after WarfareFrontlineMachinegunReviewV8450.inl by the disposable
// review dispatcher. This exercises the production helper, not a second model.
struct FFrontlineCrewV84411Review
{
    static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
    {
        auto Result=MakeShared<FJsonObject>();FString User;
        const bool Allowed=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
            &&FParse::Param(FCommandLine::Get(),TEXT("WFV8450Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty()&&!FPaths::IsRelative(User);
        Check(TEXT("v84411_crew_disposable_ukde_only"),Allowed);if(!Allowed)return Result;
        FFrontlineMachinegunV8450Review::FContext C(B);TArray<TSharedPtr<FJsonValue>> Cases;
        for(int32 Team:{0,1})for(int32 Context:{0,1,2})
        {
            C.Clear();const int32 Dir=B->GetTeamDirection(Team),Home=B->GetHomeTrenchIndex(Team);
            const int32 Next=FMath::Clamp(Home+Dir,0,B->Trenches.Num()-1),Id=8441100+Team*100+Context*10;
            const FString Prefix=FString::Printf(TEXT("v84411_crew_t%d_c%d_"),Team,Context);
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("team"),Team);Row->SetNumberField(TEXT("context"),Context);
            const FVector Start((B->Trenches[Home].X+B->Trenches[Next].X)*.5f,0,0);
            FFrontlineSquad Q;Q.Id=Id;Q.Team=Team;Q.Role=EFrontlineRole::Lewis;Q.Slot=1;Q.Trench=INDEX_NONE;
            Q.LastPassedTrench=Home;Q.Destination=Next;Q.bAdvancing=true;B->Squads.Add(Q);
            auto* G=FFrontlineMachinegunV8450Review::Make(C,Id,Team,0,Start);
            auto* U=FFrontlineMachinegunV8450Review::Make(C,Id,Team,1,Start+FVector(-Dir*150.f,200.f,0));
            auto* E=FFrontlineMachinegunV8450Review::Make(C,Id+500,1-Team,0,Start+FVector(Dir*650.f,0,0),true);
            const bool Made=G&&U&&E;Check(Prefix+TEXT("actual_gunner_helper_and_contact"),Made);if(!Made)continue;
            B->FindSquad(Id)->Members.Add(G);B->FindSquad(Id)->Members.Add(U);
            for(int32 I=0;I<140;++I){B->Elapsed+=.05f;FFrontlineMachinegunV8450Review::Tick(G,.05f);}
            auto* Gun=FrontlineMachinegunsV30::Get(G);
            const bool Supported=Gun&&Gun->IsSupported()&&!Gun->IsTrench();Check(Prefix+TEXT("ordinary_tick_really_deploys_ground_gun"),Supported);if(!Supported)continue;
            bool Reform=false;const FVector Station=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
            Check(Prefix+TEXT("support_station_initially_requires_arrival"),Reform&&FrontlineMachinegunsV30::HasGroundStationV828(U));
            const FVector Front=FRotator(0,Gun->FormationYawV50(),0).Vector();
            // A legal 64 cm body gap permits 8 cm of approach, then prevents
            // entering the 32 cm station circle. Old code called the remaining
            // 36 cm "progress" forever because it was inside the 64 cm hold band.
            const FVector From=Station+Front*44.f;
            U->SetActorLocation(FFrontlineMachinegunV8450Review::Ground(B,From)+FVector(0,0,88));
            U->bAtDestinationV43=Context==1;U->SetActorRotation(FRotator(0,Dir>0?0.f:180.f,0));
            if(Context==2){U->WeaponPhase=EFrontlineWeaponPhase::Reload;U->ReloadDurationSecondsV82=10.f;}
            auto* Blocker=FFrontlineMachinegunV8450Review::Make(C,Id+700,Team,0,Station-Front*20.f,true);
            Check(Prefix+TEXT("station_blocker_is_a_real_body_at_legal_initial_gap"),Blocker&&FVector::Dist2D(U->Feet(),Blocker->Feet())>=63.9f);if(!Blocker)continue;
            FrontlineMobilityV41::Reset(B);FFrontlineMachinegunV8450Review::Pose(U,0);
            const int32 BeginPacks=FrontlineMachinegunsV30::Get(U)->Packs;
            float Clock=0,NextSample=0;int32 GoalChanges=0,RearSteps=0,Frame=0;FVector LastFeet=U->Feet(),LastGoal=Station;
            TArray<TSharedPtr<FJsonValue>> Samples;bool Finite=true,Reselected=false,Seated=false;float MinPeerGap=MAX_flt,ChangedAt=-1.f;
            // The explicit reload pause case does not run production Update,
            // which would independently finish a synthetic reload timer.
            if(Context==2)
            {
                for(int32 I=0;I<120;++I){B->Elapsed+=.05f;const FVector P=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);
                    Finite&=P.Equals(Station,.01f);}
                Check(Prefix+TEXT("real_reload_pause_does_not_invalidate_station"),Finite);U->WeaponPhase=EFrontlineWeaponPhase::Ready;
            }
            while(Clock<14.f)
            {
                const float DTs[]={1.f/120.f,1.f/60.f,1.f/30.f,.1f};const float DT=DTs[Frame++%4];Clock+=DT;B->Elapsed+=DT;
                FFrontlineMachinegunV8450Review::Tick(G,DT);FFrontlineMachinegunV8450Review::Tick(U,DT);
                const FVector Goal=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform),Feet=U->Feet();
                if(FVector::DistSquared2D(Goal,LastGoal)>FMath::Square(5.f)){++GoalChanges;if(ChangedAt<0)ChangedAt=Clock;}
                Reselected|=FVector::DistSquared2D(Goal,Station)>FMath::Square(65.f);
                Seated|=Reselected&&!Reform&&!U->bMoving;
                if(Reselected&&FVector::DotProduct(Feet-LastFeet,Front)<-.05f)++RearSteps;
                MinPeerGap=FMath::Min(MinPeerGap,float(FVector::Dist2D(Feet,Blocker->Feet())));
                Finite&=!Feet.ContainsNaN()&&!Goal.ContainsNaN()&&FMath::IsFinite(U->GetMotionSpeed());
                if(Clock>=NextSample)
                {
                    auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seconds"),Clock);R->SetStringField(TEXT("feet"),Feet.ToString());
                    R->SetStringField(TEXT("goal"),Goal.ToString());R->SetNumberField(TEXT("error_cm"),FVector::Dist2D(Feet,Goal));
                    R->SetBoolField(TEXT("reform"),Reform);R->SetBoolField(TEXT("at_destination"),U->bAtDestinationV43);R->SetBoolField(TEXT("moving"),U->bMoving);
                    R->SetStringField(TEXT("animation"),GetNameSafe(U->Playing));R->SetNumberField(TEXT("packs"),FrontlineMachinegunsV30::Get(U)->Packs-BeginPacks);
                    Samples.Add(MakeShared<FJsonValueObject>(R));NextSample=Clock+.25f;
                }
                LastFeet=Feet;LastGoal=Goal;
            }
            Check(Prefix+TEXT("unreachable_32_to_64cm_station_is_reselected"),Reselected&&ChangedAt>0.f&&ChangedAt<9.f);
            Check(Prefix+TEXT("replacement_is_seated_without_teleport_or_peer_overlap"),Finite&&Seated&&MinPeerGap>=55.4f);
            Check(Prefix+TEXT("failed_rearward_station_does_not_repeat_reverse_march"),RearSteps==0);
            Check(Prefix+TEXT("replacement_does_not_repeat_pack_deploy"),FrontlineMachinegunsV30::Get(U)->Packs-BeginPacks<=2&&GoalChanges<=2);
            Row->SetNumberField(TEXT("goal_changes"),GoalChanges);Row->SetNumberField(TEXT("first_reselection_seconds"),ChangedAt);Row->SetNumberField(TEXT("reverse_steps_after_reselection"),RearSteps);
            Row->SetNumberField(TEXT("min_actual_peer_gap_cm"),MinPeerGap);Row->SetNumberField(TEXT("packs"),FrontlineMachinegunsV30::Get(U)->Packs-BeginPacks);
            Row->SetBoolField(TEXT("seated"),Seated);Row->SetArrayField(TEXT("timeline"),Samples);Cases.Add(MakeShared<FJsonValueObject>(Row));
            Check(Prefix+TEXT("checkpoint_written"),FFrontlineMachinegunV8450Review::Checkpoint(User,Prefix,Row));
        }
        Result->SetArrayField(TEXT("cases"),Cases);Result->SetStringField(TEXT("scope"),TEXT("UK/DE actual production helper Tick, peer body obstruction, inherited arrival latch, and real weapon-phase pause. No armour or FR/RU campaign acceptance."));return Result;
    }
};

TSharedRef<FJsonObject> RunFrontlineCrewChecksV84411(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineCrewV84411Review::Run(B,Check);}
