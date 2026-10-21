// Member of the existing private review friend; no game runtime entry point.
static TSharedRef<FJsonObject> RunAntiArmourV833(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 auto Result=MakeShared<FJsonObject>();bool Good=true;TArray<TSharedPtr<FJsonValue>> Rows;
 const bool FocusedNo44=FParse::Param(FCommandLine::Get(),TEXT("WFV8310UKRifleGrenades"));
 const auto Assert=[&](const FString& Name,bool Pass){Good&=Pass;Check(Name,Pass);};
 FString User;
 const bool Safe=B&&B->GetWorld()&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV833Review"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)
  &&!User.IsEmpty()&&!FPaths::IsRelative(User)&&!WarfareFRRUCampaignV82::Active(B);
 Assert(TEXT("v833_antiarmour_disposable_ukde_world"),Safe);if(!Safe){Result->SetBoolField(TEXT("success"),false);return Result;}
 auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 Assert(TEXT("v833_antiarmour_actual_vehicle_service"),Fleet!=nullptr);if(!Fleet){Result->SetBoolField(TEXT("success"),false);return Result;}
 const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;const auto Vehicles=Fleet->Vehicles;
 const auto Runs=B->CampaignRuns;const auto Faction=B->PlayerFaction;const auto Active=B->ActiveCampaignId;
 const auto Bound=B->BoundCampaignId,BoundEncounter=B->BoundCampaignEncounterId;const int32 BoundMission=B->BoundCampaignMission;
 const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;const auto Tracers=B->Tracers;const auto Phase=B->Phase;const auto Random=B->Random;
 const float Clock=B->Elapsed,Peak=B->PeakSuppression,Friendly=B->FriendlyFireDamage;
 const int32 Winner=B->BattleOutroWinner,Shots=B->ShotsFired,Thrown=B->GrenadesThrown,NextId=B->NextSquadId,Serial=Fleet->NextSerial;
 const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];const FString Message=B->LastMessage;
 const int32 Casualties[2]={B->Casualties[0],B->Casualties[1]};const float Morale[2]={B->Morale[0],B->Morale[1]},Damage[2]={B->DamageDealt[0],B->DamageDealt[1]};
 TSet<AActor*> Existing;for(TActorIterator<AActor> It(B->GetWorld());It;++It)Existing.Add(*It);
 struct FHidden{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHidden> Hidden;
 const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
 for(AWarfareFrontlineSoldier* U:Units)Hide(U);for(AFrontlineTank* V:Vehicles)Hide(V);
 const auto Clear=[&]()
 {
  // Include ordnance and impact actors created by ordinary weapon code.
  for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(!Existing.Contains(*It))It->Destroy();
  B->Units.Reset();B->Squads.Reset();B->Strikes.Reset();B->SmokeZones.Reset();B->Tracers.Reset();Fleet->Vehicles.Reset();
  FrontlineSpecialistsV41::Reset(B);FrontlineMobilityV41::Reset(B);FrontlineMotionV41::Reset(B);
 };
 ON_SCOPE_EXIT
 {
  Clear();B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;Fleet->Vehicles=Vehicles;Fleet->NextSerial=Serial;
  B->CampaignRuns=Runs;B->PlayerFaction=Faction;B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=BoundEncounter;B->BoundCampaignMission=BoundMission;
  B->Strikes=Strikes;B->SmokeZones=Smoke;B->Tracers=Tracers;B->Phase=Phase;B->Random=Random;B->Elapsed=Clock;B->PeakSuppression=Peak;B->FriendlyFireDamage=Friendly;
  B->BattleOutroWinner=Winner;B->ShotsFired=Shots;B->GrenadesThrown=Thrown;B->NextSquadId=NextId;B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->LastMessage=Message;
  for(int32 T=0;T<2;++T){B->Casualties[T]=Casualties[T];B->Morale[T]=Morale[T];B->DamageDealt[T]=Damage[T];}
  for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
  Result->SetBoolField(TEXT("success"),Good);Result->SetArrayField(TEXT("cases"),Rows);
 };
 B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;B->Elapsed=1000;
 const auto Ground=[&](const FVector& P){return FVector(P.X,P.Y,FrontlineInfantryV45::SurfaceHeight(B,P));};
 const auto Pose=[](AWarfareFrontlineSoldier* U,float DT)
 {
  if(!U||!U->Soldier)return;
  U->Soldier->HandleExistingParallelEvaluationTask(true,true);U->Soldier->TickAnimation(DT,false);U->Soldier->RefreshBoneTransforms();
  if(auto* C=FrontlineMachinegunsV30::Get(U))C->TickComponent(DT,LEVELTICK_All,nullptr);
 };
 const auto Make=[&](int32 Team,int32 Id,EFrontlineRole Role,int32 Member,const FVector& P,int32 Seed)
 {
  auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)return U;
  U->Initialize(B,Id,Team,Role,Member);U->SetActorTickEnabled(false);U->bFieldEntered=true;
  U->SetActorLocation(Ground(P)+FVector(0,0,88));U->SetActorRotation(FRotator(0,B->GetTeamDirection(Team)>0?0.f:180.f,0));
  U->MotionRandom.Initialize(Seed+Member*101+Team*197);U->ReturnToIdle();U->AimSettleClock=U->DepartureClock=0;
  U->SetMotion(U->GetAimMotion());B->Units.Add(U);Pose(U,.6f);return U;
 };
 // Both factions keep their real weapon and roster. Stormtroopers are an
 // explicit upgraded German case, not another role ID or a free rifle bonus.
 struct FCase{EFrontlineRole Role;bool Storm;const TCHAR* Name;};
 const FCase Cases[]={{EFrontlineRole::Rifle,false,TEXT("rifle_control")},{EFrontlineRole::Mills,false,TEXT("grenadiers")},
  {EFrontlineRole::RifleGrenadier,false,TEXT("rifle_grenadiers")},{EFrontlineRole::AntiTankRifle,false,TEXT("tankgewehr")},
  {EFrontlineRole::Mills,true,TEXT("stormtroopers")}};
 int32 CaseId=833000;int32 ContactCases=0,NonRifleDamagingCases=0;float RifleHullDamage=0;
 TMap<FString,int32> CoveredWeaponFires,CoveredWeaponDamagingCases;int32 PairedAPShots=0,PairedAPHits=0;
 for(int32 National=0;National<2;++National)
 {
  if(FocusedNo44&&National!=0)continue;
  B->PlayerFaction=National?EFrontlineFaction::German:EFrontlineFaction::British;
  FFrontlineCampaignProgress Run;Run.Id=FGuid::NewGuid();Run.Faction=B->PlayerFaction;Run.SelectedMission=Run.CompletedMissions=19;
  B->CampaignRuns={Run};B->ActiveCampaignId=B->BoundCampaignId=Run.Id;B->BoundCampaignEncounterId=B->EncounterId;B->BoundCampaignMission=19;
  const int32 Dir=B->GetTeamDirection(0),Home=B->GetHomeTrenchIndex(0);
  Assert(FString::Printf(TEXT("v833_%s_has_actual_home_trench"),National?TEXT("de"):TEXT("uk")),B->Trenches.IsValidIndex(Home));
  if(!B->Trenches.IsValidIndex(Home))continue;
  for(const auto& Spec:Cases)
  {
   if(FocusedNo44&&Spec.Role!=EFrontlineRole::RifleGrenadier)continue;
   // Only issued national roles: No.23 for Britain, Tankgewehr for Germany.
   if((National==0&&(Spec.Role==EFrontlineRole::AntiTankRifle||Spec.Storm))||(National==1&&Spec.Role==EFrontlineRole::RifleGrenadier))continue;
   B->CampaignRuns[0].Research.TechnologyMask=Spec.Storm?1u<<12:0;
   const EFrontlineVehicle TankTypesUK[]={EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIVFemale};
   const EFrontlineVehicle TankTypesDE[]={EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::Whippet};
   for(int32 Model=0;Model<2;++Model)for(int32 Context=0;Context<5;++Context)for(int32 SeedIndex=0;SeedIndex<2;++SeedIndex)
   {
    if(FocusedNo44&&Context!=0&&Context!=3)continue;
    if(Context==4&&Spec.Role!=EFrontlineRole::AntiTankRifle)continue;
    // Additional contexts must not change the old case's identity-derived
    // scatter, reaction, cadence or armour serial. They are separate trials.
    const int32 LegacyCaseId=CaseId,LegacyTankSerial=Fleet->NextSerial;
    ON_SCOPE_EXIT {if(Context>=3){CaseId=LegacyCaseId;Fleet->NextSerial=LegacyTankSerial;}};
    Clear();B->Trenches=Trenches;B->Trenches[Home].Owner=0;B->Elapsed=1000;B->Random.Initialize(83317+SeedIndex*1549);
    for(int32 T=0;T<2;++T){B->Morale[T]=100;B->Casualties[T]=0;B->DamageDealt[T]=0;B->CombatRecordV65[T]={};}
    const int32 Id=++CaseId;FFrontlineSquad Section;Section.Id=Id;Section.Team=0;Section.Role=Spec.Role;Section.Slot=1;
    const bool Covered=Context==1||Context>=3;
    Section.Trench=Covered?Home:INDEX_NONE;Section.Destination=Covered?Home:INDEX_NONE;
    Section.bAdvancing=!Covered;Section.bReinforcement=false;B->Squads.Add(Section);
    const float Lane=SeedIndex?300.f:-300.f;
    // Open approach lives on the real rear apron. Held cases use the actual
    // trench firing slots, while the third case closes on immobilised armour.
    const FVector Base(B->Trenches[Home].X-(Covered?0.f:Dir*1800.f),Lane,0);
    const int32 Count=B->RoleSize(Spec.Role,0);TArray<AWarfareFrontlineSoldier*> Men;
    for(int32 M=0;M<Count;++M)
     if(auto* U=Make(0,Id,Spec.Role,M,Base+FVector(-Dir*(M%2)*100.f,(M-(Count-1)*.5f)*105.f,0),83317+SeedIndex*1549+M*31))
     {B->FindSquad(Id)->Members.Add(U);Men.Add(U);}
    if(Context==4)
    {
     const int32 SecondId=++CaseId;auto Second=Section;Second.Id=SecondId;Second.Slot=2;Second.Members.Reset();B->Squads.Add(Second);
     for(int32 M=0;M<Count;++M)
      if(auto* U=Make(0,SecondId,Spec.Role,M,Base+FVector(-Dir*(M%2)*100.f,550.f+(M-(Count-1)*.5f)*105.f,0),83417+SeedIndex*1549+M*31))
      {B->FindSquad(SecondId)->Members.Add(U);Men.Add(U);}
    }
    if(Covered)for(auto* U:Men){bool Reform=false;const auto P=FrontlineMachinegunsV30::Destination(U,B->SoldierDestination(U),Reform);U->SetActorLocation(Ground(P)+FVector(0,0,88));Pose(U,.6f);}
    const auto Type=National?TankTypesDE[Model]:TankTypesUK[Model];auto* Tank=Fleet->Spawn(Type,1,true);
    const int32 ExpectedCount=Count*(Context==4?2:1);
    const bool Fixtures=Men.Num()==ExpectedCount&&Tank&&Tank->AreAssetsReady();Assert(TEXT("v833_antiarmour_actual_roster_and_armour_assets"),Fixtures);
    if(!Fixtures)continue;
    Tank->HeadingYawV37=Dir>0?180.f:0.f;Tank->SetActorRotation(FRotator(0,Tank->HeadingYawV37,0));Tank->bEntered=true;
    const float Separation=FocusedNo44?FrontlineSpecialistsV41::AntiTankRifleGrenadeContactCmV8310*B->EncounterVisibilityScale():Context>=3?(Spec.Role==EFrontlineRole::AntiTankRifle?5600.f:Spec.Role==EFrontlineRole::RifleGrenadier?3400.f:1850.f)
        :Context==1?2500.f:Context==2?1050.f:2300.f;
    Tank->FitGroundV35(Base+FVector(Dir*Separation,0,0),0);Tank->PreferredLaneV37=Tank->DesiredLaneV37=Lane;Tank->RouteDecisionAtV37=B->Elapsed+60;
    if(Context==2)Tank->TrackIntegrity[0]=0; // Combat continues; no artificial invulnerability.
    for(float& C:Tank->GunClock)C=0;for(float& C:Tank->CannonClock)C=0;
    Tank->Random.Initialize(83329+SeedIndex*1553);
    // An ordinary two-man enemy rifle screen exposes armour-target starvation.
    if(Context==1)
    {
     const int32 ScreenId=++CaseId;FFrontlineSquad Screen;Screen.Id=ScreenId;Screen.Team=1;Screen.Role=EFrontlineRole::Rifle;Screen.bAdvancing=true;Screen.Trench=INDEX_NONE;
     B->Squads.Add(Screen);
     for(int32 M=0;M<2;++M)if(auto* U=Make(1,ScreenId,EFrontlineRole::Rifle,M,Base+FVector(Dir*1200.f,(M?1.f:-1.f)*420.f,0),83397+SeedIndex*1549))B->FindSquad(ScreenId)->Members.Add(U);
    }
    // A paid friendly rifle section draws fire at a separate frontage. Actual
    // cover, casualty rolls, body avoidance and tank targeting remain active;
    // this is combined infantry support, not an invulnerable target dummy.
    // The long-ranged ATR case also exposes its old hard 54 m acquisition cap.
    int32 SupportId=INDEX_NONE;
    if(Context>=3||FocusedNo44)
    {
     SupportId=++CaseId;FFrontlineSquad Support;Support.Id=SupportId;Support.Team=0;Support.Role=EFrontlineRole::Rifle;Support.bAdvancing=true;Support.Trench=INDEX_NONE;Support.Destination=INDEX_NONE;B->Squads.Add(Support);
     for(int32 M=0;M<B->RoleSize(EFrontlineRole::Rifle,0);++M)
      if(auto* U=Make(0,SupportId,EFrontlineRole::Rifle,M,Base+FVector(Dir*(Separation-900.f),(M%2?500.f:-500.f)+(M/2)*100.f,0),83437+SeedIndex*1549))B->FindSquad(SupportId)->Members.Add(U);
    }
    int32 InitialSight=0;for(auto* U:Men)InitialSight+=Fleet->AcquireInfantryTarget(U)==Tank;
    const float PrimaryArmourDistance=Men.IsEmpty()?0.f:float(FVector::Dist2D(Men[0]->Feet(),Tank->ObservedArmourAimPointV837(Men[0]->Feet())));
    float FormationMin=MAX_flt,FormationMax=-MAX_flt;
    for(auto* U:Men){const float X=(U->MemberIndex%2?36.f:-36.f)+U->FormationOffset.X;FormationMin=FMath::Min(FormationMin,X);FormationMax=FMath::Max(FormationMax,X);}
    const float AuthoredFormationSpan=FormationMax-FormationMin;
    // Production offsets are +/-36cm plus a personal +/-29cm. Use the actual
    // current span and the existing220cm contact hysteresis as the allowance,
    // rather than inventing a metre-scale formation width for this three-man crew.
    const float CompanionAdvanceAllowance=AuthoredFormationSpan+220.f;
    float MaximumCompanionAhead=0;int32 CompanionContactFrames=0;
    const bool StandardThreeManFormation=Men.Num()==3&&AuthoredFormationSpan<=130.01f;
    const float Hull=Tank->HullIntegrity,Left=Tank->TrackIntegrity[0],Right=Tank->TrackIntegrity[1],StartX=Tank->GetActorLocation().X;
    const int32 TankMG=Tank->MachinegunShots,TankCannon=Tank->CannonShots;float FirstDamage=-1,FirstFire=-1,FirstKill=-1;
    bool ClearBodies=true,Finite=true;int32 Frames=0;float DeferredDrainSeconds=0;
    float LeadDeath=-1;int32 LeadObserved=0,LeadReady=0,LeadSupported=0,LeadClear=0,LeadBlocked=0;
    TArray<TSharedPtr<FJsonValue>> BlockedMuzzleSamples;
    constexpr float Step=.05f,Duration=36.f;
    for(;Frames<720;++Frames)
    {
     B->Elapsed+=Step;
     for(AWarfareFrontlineSoldier* U:B->Units)if(IsValid(U)){U->Tick(Step);Pose(U,Step);}
     B->TickCommittedSupportV8217();FrontlineSpecialistsV41::Tick(B);Tank->Simulate(Step);
     if(!Men.IsEmpty())
     {
      auto* Lead=Men[0];if(!Lead->IsAlive()&&LeadDeath<0)LeadDeath=(Frames+1)*Step;
      if(FocusedNo44&&Context==0&&Lead->IsActiveCombatant()&&Lead->bEngaging&&!Lead->bMoving&&Fleet->AcquireInfantryTarget(Lead)==Tank)
      {
       ++CompanionContactFrames;
       for(int32 M=1;M<Men.Num();++M)if(Men[M]->IsActiveCombatant())
        MaximumCompanionAhead=FMath::Max(MaximumCompanionAhead,float((Men[M]->Feet().X-Lead->Feet().X)*Dir));
      }
      if(Lead->IsAlive()&&Spec.Role==EFrontlineRole::AntiTankRifle)
       if(auto* Gun=FrontlineMachinegunsV30::Get(Lead))
       {
        const bool Observed=Gun->ObserveArmourV48()==Tank;LeadObserved+=Observed;
        LeadReady+=Lead->WeaponPhase==EFrontlineWeaponPhase::Ready;LeadSupported+=Gun->IsSupported();
        const bool SupportedReadyObserved=Observed&&Gun->CanFire()&&Lead->WeaponPhase==EFrontlineWeaponPhase::Ready;
        const bool MuzzleClear=SupportedReadyObserved&&Gun->HasArmourFiringLineV48(Tank);LeadClear+=MuzzleClear;
        if(SupportedReadyObserved&&!MuzzleClear)
        {
         ++LeadBlocked;
         if(BlockedMuzzleSamples.Num()<2&&(BlockedMuzzleSamples.IsEmpty()||LeadBlocked>20))
         {
          FCollisionQueryParams Query(SCENE_QUERY_STAT(V833BlockedATRBed),true);Query.AddIgnoredActor(Tank);
          for(const AWarfareFrontlineSoldier* Other:B->Units)if(Other)Query.AddIgnoredActor(Other);
          const FVector Muzzle=Lead->GetMuzzleLocation(),Aim=Tank->AimPoint(),Foot=Lead->Feet();FHitResult Hit;
          const bool Wall=Lead->GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Aim,ECC_Visibility,Query);
          const FVector Bipod=Lead->Soldier->DoesSocketExist(TEXT("WF_Bipod"))?Lead->Soldier->GetSocketLocation(TEXT("WF_Bipod")):FVector::ZeroVector;
          auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("seconds"),(Frames+1)*Step);
          Sample->SetStringField(TEXT("feet"),Foot.ToString());Sample->SetStringField(TEXT("muzzle"),Muzzle.ToString());Sample->SetStringField(TEXT("bipod"),Bipod.ToString());Sample->SetStringField(TEXT("target"),Aim.ToString());
          Sample->SetStringField(TEXT("animation"),GetNameSafe(Lead->Playing));Sample->SetNumberField(TEXT("muzzle_above_feet_cm"),Muzzle.Z-Foot.Z);
          Sample->SetNumberField(TEXT("bank_intrusion_cm"),FrontlineInfantryV45::BankIntrusion(Lead,Foot));
          Sample->SetBoolField(TEXT("physical_wall"),Wall);Sample->SetStringField(TEXT("wall_actor"),Wall?GetNameSafe(Hit.GetActor()):TEXT(""));Sample->SetStringField(TEXT("wall_component"),Wall?GetNameSafe(Hit.GetComponent()):TEXT(""));
          if(Wall){Sample->SetStringField(TEXT("wall_point"),Hit.ImpactPoint.ToString());Sample->SetNumberField(TEXT("wall_distance_cm"),FVector::Dist(Muzzle,Hit.ImpactPoint));}
          BlockedMuzzleSamples.Add(MakeShared<FJsonValueObject>(Sample));
         }
        }
       }
     }
     for(auto* U:Men)if(U->IsActiveCombatant()){ClearBodies&=!Tank->BlocksInfantryAtV8218(U->Feet(),Tank->GetActorLocation(),Tank->HeadingYawV37);Finite&=!U->Feet().ContainsNaN();}
     int32 ShotCount=0,Living=0;for(auto* U:Men){ShotCount+=U->ShotsDischarged+U->GrenadesReleased;Living+=U->IsAlive();}
     if(FirstFire<0&&ShotCount>0)FirstFire=(Frames+1)*Step;
     if(FirstDamage<0&&(Tank->HullIntegrity<Hull||Tank->TrackIntegrity[0]<Left||Tank->TrackIntegrity[1]<Right))FirstDamage=(Frames+1)*Step;
     if(!Tank->IsOperational()){FirstKill=(Frames+1)*Step;break;}
     if(!Living&&B->Strikes.IsEmpty())
     {
      // A crew's death cannot erase the AP round already in flight this same
      // frame. Consume its bounded <=120 ms journey with ordinary hull travel
      // before recording the final damage; never force a hit or freeze a tank.
      for(int32 Drain=0;Drain<3;++Drain)
      {
       B->Elapsed+=Step;DeferredDrainSeconds+=Step;FrontlineSpecialistsV41::Tick(B);Tank->Simulate(Step);
       if(FirstDamage<0&&(Tank->HullIntegrity<Hull||Tank->TrackIntegrity[0]<Left||Tank->TrackIntegrity[1]<Right))FirstDamage=(Frames+1)*Step+DeferredDrainSeconds;
       if(!Tank->IsOperational()&&FirstKill<0)FirstKill=(Frames+1)*Step+DeferredDrainSeconds;
      }
      break;
     }
    }
    int32 Living=0,ShotsMade=0,Throws=0,Deployments=0,Packs=0;float Travel=0;
    for(auto* U:Men){Living+=U->IsAlive();ShotsMade+=U->ShotsDischarged;Throws+=U->GrenadesReleased;Travel+=U->DistanceTravelled;if(auto* C=FrontlineMachinegunsV30::Get(U)){Deployments+=C->Deployments;Packs+=C->Packs;}}
    const float HullLoss=Hull-Tank->HullIntegrity,TrackLoss=Left+Right-Tank->TrackIntegrity[0]-Tank->TrackIntegrity[1];
    ContactCases+=InitialSight>0;if(Spec.Role==EFrontlineRole::Rifle)RifleHullDamage+=HullLoss;else NonRifleDamagingCases+=HullLoss>0||TrackLoss>0;
    const FString Key=FString::Printf(TEXT("v833_%s_%s_tank%d_context%d_seed%d"),National?TEXT("de"):TEXT("uk"),Spec.Name,int32(Type),Context,SeedIndex);
    Assert(Key+TEXT("_finite_and_no_live_hull_infantry_overlap"),Finite&&ClearBodies);
    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("case"),Key);R->SetStringField(TEXT("faction"),National?TEXT("German"):TEXT("British"));R->SetStringField(TEXT("role"),Spec.Name);
    R->SetNumberField(TEXT("armour_type"),int32(Type));R->SetStringField(TEXT("context"),Context==0?TEXT("open_approach"):Context==1?TEXT("held_trench_with_screen"):Context==2?TEXT("close_disabled_track"):Context==3?TEXT("covered_section_with_paid_rifle_screen"):TEXT("covered_two_ATR_sections_with_paid_rifle_screen"));R->SetNumberField(TEXT("seed"),SeedIndex);
    R->SetNumberField(TEXT("initial_armour_observers"),InitialSight);R->SetNumberField(TEXT("initial_hull"),Hull);R->SetNumberField(TEXT("hull_damage"),HullLoss);R->SetNumberField(TEXT("track_damage"),TrackLoss);
    if(FocusedNo44)
    {
     R->SetNumberField(TEXT("initial_primary_armour_distance_cm"),PrimaryArmourDistance);
     R->SetNumberField(TEXT("old_no44_effective_range_cm"),2800.f*B->EncounterVisibilityScale());
     R->SetNumberField(TEXT("no44_effective_range_cm"),FrontlineSpecialistsV41::AntiTankRifleGrenadeRangeCmV836*B->EncounterVisibilityScale());
     R->SetNumberField(TEXT("no23_infantry_range_cm"),FrontlineWeapons::ProfileFor(Men[0]).FireRangeCm*B->EncounterVisibilityScale());
     R->SetBoolField(TEXT("no44_issued"),FrontlineSpecialistsV41::HasAntiTankRifleGrenadeV836(Men[0]));
     R->SetBoolField(TEXT("armour_immobilised_at_start"),Context==2);
     R->SetBoolField(TEXT("actual_three_man_formation_valid"),StandardThreeManFormation);
     R->SetNumberField(TEXT("actual_authored_longitudinal_span_cm"),AuthoredFormationSpan);
     R->SetNumberField(TEXT("companion_allowance_span_plus_contact_hysteresis_cm"),CompanionAdvanceAllowance);
     R->SetNumberField(TEXT("actual_lead_armour_hold_frames"),CompanionContactFrames);
     R->SetNumberField(TEXT("maximum_live_companion_ahead_during_armour_hold_cm"),MaximumCompanionAhead);
     Assert(Key+TEXT("_actual_primary_starts_beyond_old_no44_envelope"),PrimaryArmourDistance>2800.f*B->EncounterVisibilityScale());
     if(Context==0)
     {
      Assert(Key+TEXT("_normal_three_man_formation_really_enters_armour_hold"),StandardThreeManFormation&&CompanionContactFrames>=10);
      if(StandardThreeManFormation&&CompanionContactFrames>=10)
       Assert(Key+TEXT("_companions_do_not_cross_old_ten_metre_observation_gap"),MaximumCompanionAhead<=CompanionAdvanceAllowance+.01f);
     }
    }
    R->SetNumberField(TEXT("survivors"),Living);R->SetNumberField(TEXT("roster_size"),ExpectedCount);R->SetNumberField(TEXT("shots"),ShotsMade);R->SetNumberField(TEXT("grenades"),Throws);
    R->SetNumberField(TEXT("lead_weapon_shots"),Men.IsEmpty()?0:Men[0]->ShotsDischarged);R->SetNumberField(TEXT("lead_grenades"),Men.IsEmpty()?0:Men[0]->GrenadesReleased);
    R->SetNumberField(TEXT("lead_death_seconds"),LeadDeath);R->SetNumberField(TEXT("lead_first_shot_seconds"),Men.IsEmpty()||Men[0]->FirstShotAt<0?-1:Men[0]->FirstShotAt-1000.f);
    R->SetNumberField(TEXT("lead_observed_frames"),LeadObserved);R->SetNumberField(TEXT("lead_ready_frames"),LeadReady);R->SetNumberField(TEXT("lead_supported_frames"),LeadSupported);R->SetNumberField(TEXT("lead_supported_ready_muzzle_clear_frames"),LeadClear);
    R->SetNumberField(TEXT("lead_supported_ready_observed_blocked_frames"),LeadBlocked);R->SetArrayField(TEXT("blocked_muzzle_samples"),BlockedMuzzleSamples);
    const auto AP=FrontlineSpecialistsV41::ReadArmourRoundsV833(B);
    R->SetNumberField(TEXT("ap_rounds"),AP.Fired);R->SetNumberField(TEXT("ap_physical_hull_hits"),AP.PhysicalHullHits);R->SetNumberField(TEXT("ap_world_interceptions"),AP.WorldInterceptions);
    R->SetNumberField(TEXT("ap_applied_hits"),AP.AppliedHits);R->SetNumberField(TEXT("ap_hull_damage"),AP.HullDamage);R->SetNumberField(TEXT("ap_track_damage"),AP.TrackDamage);
    R->SetNumberField(TEXT("grenade_applied_hits"),AP.GrenadeAppliedHits);R->SetNumberField(TEXT("grenade_hull_damage"),AP.GrenadeHullDamage);R->SetNumberField(TEXT("grenade_track_damage"),AP.GrenadeTrackDamage);
    const float ActualWeaponDamage=AP.HullDamage+AP.TrackDamage+AP.GrenadeHullDamage+AP.GrenadeTrackDamage;
    R->SetNumberField(TEXT("infantry_weapon_module_damage"),ActualWeaponDamage);
    if(Context==3&&Spec.Role!=EFrontlineRole::Rifle)
    {
     const FString RoleKey=FString::Printf(TEXT("%s_%s"),National?TEXT("de"):TEXT("uk"),Spec.Name);
     CoveredWeaponFires.FindOrAdd(RoleKey)+=Spec.Role==EFrontlineRole::AntiTankRifle?AP.Fired:Spec.Role==EFrontlineRole::RifleGrenadier?Men[0]->ShotsDischarged:Throws;
     CoveredWeaponDamagingCases.FindOrAdd(RoleKey)+=ActualWeaponDamage>0;
    }
    if(Context==4){PairedAPShots+=AP.Fired;PairedAPHits+=AP.AppliedHits;}
    Assert(Key+TEXT("_weapon_provenance_within_actual_module_loss"),ActualWeaponDamage<=HullLoss+TrackLoss+.05f);
    R->SetNumberField(TEXT("first_fire_seconds"),FirstFire);R->SetNumberField(TEXT("first_damage_seconds"),FirstDamage);R->SetNumberField(TEXT("kill_seconds"),FirstKill);
    R->SetNumberField(TEXT("observed_seconds"),FMath::Min((Frames+1)*Step,Duration)+DeferredDrainSeconds);R->SetNumberField(TEXT("infantry_travel_cm"),Travel);R->SetNumberField(TEXT("armour_travel_cm"),FMath::Abs(Tank->GetActorLocation().X-StartX));
    R->SetNumberField(TEXT("shared_queue_seconds"),B->EffectiveRecruitSeconds(Spec.Role,0)*(Context==4?2:1));R->SetNumberField(TEXT("tank_queue_seconds"),UFrontlineVehicleSubsystem::Definition(Type).Cooldown*FrontlinePacingV24::TempoIntervalScale);
    R->SetNumberField(TEXT("support_queue_seconds"),Context>=3||FocusedNo44?B->EffectiveRecruitSeconds(EFrontlineRole::Rifle,0):0);
    int32 SupportSurvivors=0;if(const auto* Support=B->FindSquad(SupportId))for(const auto& W:Support->Members)if(const auto* U=W.Get())SupportSurvivors+=U->IsAlive();R->SetNumberField(TEXT("support_survivors"),SupportSurvivors);
    R->SetNumberField(TEXT("tank_mg_shots"),Tank->MachinegunShots-TankMG);R->SetNumberField(TEXT("tank_cannon_shots"),Tank->CannonShots-TankCannon);R->SetNumberField(TEXT("deployments"),Deployments);R->SetNumberField(TEXT("packs"),Packs);
    R->SetBoolField(TEXT("body_contact_clear"),ClearBodies);R->SetBoolField(TEXT("storm_weapon_issued"),!Spec.Storm||(!Men.IsEmpty()&&Men[0]->IsStormtrooper()));Rows.Add(MakeShared<FJsonValueObject>(R));
    UE_LOG(LogTemp,Display,TEXT("WF_V833_ANTIARMOUR %s hull=%.1f tracks=%.1f survivors=%d/%d shots=%d grenades=%d first=%.2f tankmg=%d"),*Key,HullLoss,TrackLoss,Living,Count,ShotsMade,Throws,FirstDamage,Tank->MachinegunShots-TankMG);
   }
  }
 }
 if(!FocusedNo44)Assert(TEXT("v833_ordinary_service_rifles_never_erode_armour_hull"),FMath::IsNearlyZero(RifleHullDamage,.001f));
 Assert(TEXT("v833_actual_contact_cases_present"),ContactCases>0);
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFV833Baseline")))
 {
  // Meaningful opportunities across both actual hostile models; no demanded
  // solo tank kill, guaranteed penetration or invulnerable supporting screen.
  const TArray<FString> Roles=FocusedNo44?TArray<FString>{TEXT("uk_rifle_grenadiers")}:TArray<FString>{TEXT("uk_grenadiers"),TEXT("de_grenadiers"),TEXT("de_stormtroopers"),TEXT("uk_rifle_grenadiers"),TEXT("de_tankgewehr")};
  for(const FString& RoleKey:Roles)
  {
   Assert(TEXT("v833_paid_cover_")+RoleKey+TEXT("_special_weapon_really_fires"),CoveredWeaponFires.FindRef(RoleKey)>=4);
   Assert(TEXT("v833_paid_cover_")+RoleKey+TEXT("_damages_armour_in_multiple_cases"),CoveredWeaponDamagingCases.FindRef(RoleKey)>=2);
  }
  if(!FocusedNo44)Assert(TEXT("v833_two_paid_ATR_sections_really_fire_and_penetrate"),PairedAPShots>=4&&PairedAPHits>=2);
 }
 Result->SetNumberField(TEXT("paired_ATR_actual_shots"),PairedAPShots);Result->SetNumberField(TEXT("paired_ATR_actual_applied_hits"),PairedAPHits);
 Result->SetNumberField(TEXT("non_rifle_cases_with_real_module_damage"),NonRifleDamagingCases);
 Result->SetNumberField(TEXT("initial_contact_cases"),ContactCases);Result->SetNumberField(TEXT("rifle_control_hull_damage"),RifleHullDamage);
 Result->SetBoolField(TEXT("baseline"),FParse::Param(FCommandLine::Get(),TEXT("WFV833Baseline")));
 Result->SetStringField(TEXT("scope"),TEXT("116 fixed-seed actual production section-versus-armour encounters: both playable armies, issued roles, heavy and fast/gun variants, real rear-apron movement, held trench with enemy rifle screen, close immobilised armour, 28 covered sections with a paid friendly rifle screen (ATR at 56 m), and four paid two-ATR-section encounters. The first 84 keys match the recorded unmodified Shipping baseline. Normal soldier Tick, evaluated skeleton, grenade animation notification/projectile/ResolveStrike, deferred AP trace/penetration, and hostile tank Simulate. 36 s cap per case; actual queue cost, first fire/damage, casualties, net module loss and separately proven infantry-weapon damage are recorded. No game invulnerability, fabricated hit, duplicated DPS equation or full campaign balance claim."));
 if(FocusedNo44)Result->SetStringField(TEXT("scope"),TEXT("Eight bounded actual British No.44 sections against two opposing production armour models and two fixed seeds, in advancing rear approaches and held trenches, starting beyond the former28m envelope with a separately costed, vulnerable rifle screen. Ordinary soldier Tick, animated projectile release, committed ballistic ResolveStrike and hostile vehicle Simulate run for at most36s. Tanks are mobile and fire normally; no fabricated hits or protected crew. Reports actual formation span and live companion movement during observed armour hold, queue costs, armour-source damage, casualties and kill times. Controlled actor fixtures are not full public recruitment, campaign acceptance or proof of equal national difficulty."));
 return Result;
}
