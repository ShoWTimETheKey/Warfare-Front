namespace FrontlineArmourBlastV844
{
// Review changes this only around synchronous, disposable production calls.
// No clock lease, damage cap, fictional invulnerability or save field is added.
static bool BaselineForReview=false;
struct FResponse {float Energy=1.f,Modules=1.f;};
bool Applies(const AWarfareFrontlineGameMode* B,const FFrontlineKillSourceV65& Source)
{
 if(!B||BaselineForReview)return false;
 return Source.Support==int32(EFrontlineSupport::Artillery)||Source.Support==int32(EFrontlineSupport::Mortar)
  ||Source.Support==int32(EFrontlineSupport::CreepingBarrage)||Source.Support==int32(EFrontlineSupport::AirBombing);
}
FResponse Response(EFrontlineVehicle Type,EFrontlineSupport Support)
{
 // Compressed game responses, not historical steel thickness measurements.
 // Heavy hulls resist nearby HE; thin road cars still fear a close burst.
 float Coat=Type==EFrontlineVehicle::Austin1918?.78f:Type==EFrontlineVehicle::Whippet?.74f:
  Type==EFrontlineVehicle::MarkVStar?.40f:Type==EFrontlineVehicle::MarkVMale?.42f:
  Type==EFrontlineVehicle::RailConvoy?.50f:A7VFamily(Type)?.48f:.46f;
 const bool Light=Type==EFrontlineVehicle::Austin1918||Type==EFrontlineVehicle::Whippet;
 FResponse R;R.Energy=Coat;R.Modules=Light?.82f:.62f;
 if(Support==EFrontlineSupport::Mortar){R.Energy*=Light?.65f:.55f;R.Modules=Light?.72f:.48f;}
 else if(Support==EFrontlineSupport::CreepingBarrage){R.Energy*=.90f;}
 else if(Support==EFrontlineSupport::AirBombing){R.Energy=FMath::Min(.78f,Coat*1.40f);R.Modules=Light?.90f:.80f;}
 return R;
}
FResponse Response(const AFrontlineTank* Vehicle,EFrontlineSupport Support)
{
 // Native actors deliberately carry Type::Count. Resolve the existing blast
 // response by their real chassis class, never the enum's Mark IV fallback.
 EFrontlineVehicle Type=Vehicle->Type;
 if(const auto* Native=WarfareFRRUArmourV82::Definition(Vehicle))
  Type=Native->Rail?EFrontlineVehicle::RailConvoy:Native->Wheeled?EFrontlineVehicle::Austin1918:
   Native->Id==TEXT("fr.ft")?EFrontlineVehicle::Whippet:EFrontlineVehicle::MarkIVMale;
 return Response(Type,Support);
}
}

TSharedRef<FJsonObject> UFrontlineVehicleSubsystem::RunSupportArmourBalanceChecksV844()
{
 auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Matrix,Salvos;bool Good=true;
 const auto Check=[&](const FString& Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));Good&=Pass;};
 ON_SCOPE_EXIT{Report->SetArrayField(TEXT("checks"),Checks);Report->SetArrayField(TEXT("single_round_matrix"),Matrix);Report->SetArrayField(TEXT("actual_resolved_salvos"),Salvos);Report->SetBoolField(TEXT("success"),Good);};
 auto* B=Battle();FString User;
 const bool Safe=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV844Review"))&&FrontlineEffectsReviewSafetyV8216::Allowed(B)
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!User.IsEmpty();
 Check(TEXT("v844_support_balance_disposable_available_campaign_guard"),Safe);if(!Safe)return Report;
 const auto SavedVehicles=Vehicles;const auto Units=B->Units;const auto Phase=B->Phase;const auto Encounter=LastEncounter;
 const int32 Serial=NextSerial,Count0=DeploymentCountV37[0],Count1=DeploymentCountV37[1],Outro=B->BattleOutroWinner;
 const float Clock=B->Elapsed,M0=B->Morale[0],M1=B->Morale[1];const auto Message=B->LastMessage;
 const auto Stats0=B->CombatRecordV65[0],Stats1=B->CombatRecordV65[1];
 TArray<AFrontlineTank*> Owned;TSet<AActor*> BeforeActors;for(TActorIterator<AActor> It(GetWorld());It;++It)BeforeActors.Add(*It);
 Vehicles.Empty();B->Units.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;LastEncounter=B->GetEncounterId();
 ON_SCOPE_EXIT{for(auto* V:Owned)if(IsValid(V))V->Destroy();Vehicles=SavedVehicles;B->Units=Units;B->Phase=Phase;LastEncounter=Encounter;
  NextSerial=Serial;DeploymentCountV37[0]=Count0;DeploymentCountV37[1]=Count1;B->BattleOutroWinner=Outro;B->Elapsed=Clock;
  B->Morale[0]=M0;B->Morale[1]=M1;B->LastMessage=Message;B->CombatRecordV65[0]=Stats0;B->CombatRecordV65[1]=Stats1;
  for(TActorIterator<AWarfareFrontlineImpact> It(GetWorld());It;++It)if(!BeforeActors.Contains(*It))It->Destroy();};
 const EFrontlineSupport Supports[]={EFrontlineSupport::Mortar,EFrontlineSupport::Artillery,EFrontlineSupport::CreepingBarrage,EFrontlineSupport::AirBombing,EFrontlineSupport::AntiTank};
 const auto Power=[](EFrontlineSupport S){return S==EFrontlineSupport::Mortar?230.f:S==EFrontlineSupport::Artillery?440.f:S==EFrontlineSupport::CreepingBarrage?420.f:S==EFrontlineSupport::AirBombing?500.f:260.f;};
 const auto Radius=[](EFrontlineSupport S){return S==EFrontlineSupport::Mortar?365.f:S==EFrontlineSupport::Artillery?600.f:S==EFrontlineSupport::CreepingBarrage?780.f:S==EFrontlineSupport::AirBombing?820.f:300.f;};
 const auto Make=[&](EFrontlineVehicle Type){AFrontlineTank* V=nullptr;
  if(Type==EFrontlineVehicle::RailConvoy)
  {
   // A damage receiver fixture is not an allocation order. The production
   // Spawn gate correctly refuses a train on this non-rail operation.
   V=GetWorld()->SpawnActor<AFrontlineTank>();
   if(!V||!V->Configure(B,EFrontlineVehicle::A7V,1,NextSerial++)||!V->ConfigureRailV32())
   {if(IsValid(V))V->Destroy();return static_cast<AFrontlineTank*>(nullptr);}
  }
  else V=Spawn(Type,0,false);
  if(!V)return V;Owned.Add(V);Vehicles={V};V->bEntered=true;
  V->HeadingYawV37=0;const float X=B->Trenches.IsEmpty()?-3000.f:B->Trenches[B->Trenches.Num()/2].X-1150.f;
  V->SetActorLocation(FVector(X,-700.f,0));V->FitGroundV35(V->GetActorLocation(),0);V->SetActorTickEnabled(false);return V;};
 bool NonIncrease=true,PhysicalFringeOnly=true,EdgeTapers=true,APUnchanged=true,CommonBothTeams=true;
 for(int32 Model=0;Model<int32(EFrontlineVehicle::Count);++Model)
 {
  auto* V=Make(EFrontlineVehicle(Model));if(!V){Check(TEXT("v844_actual_armour_model_assets"),false);continue;}
  const float OriginalHull=V->MaximumHullIntegrity,RailEngine=V->RailEngineIntegrityV837_;const uint8 RailMask=V->RailSectionMaskV837;
  float RailCars[5];for(int32 I=0;I<5;++I)RailCars[I]=V->RailCarIntegrityV837_[I];
  const FTransform Hull=V->Body->GetComponentTransform();const auto Box=V->Body->GetStaticMesh()->GetBoundingBox();
  const auto ResetTransfer=[&](){V->HullIntegrity=5000.f;V->TrackIntegrity[0]=V->TrackIntegrity[1]=5000.f;for(float& G:V->GunIntegrity)G=5000.f;
   V->RailEngineIntegrityV837_=RailEngine;V->RailSectionMaskV837=RailMask;for(int32 I=0;I<5;++I)V->RailCarIntegrityV837_[I]=RailCars[I];};
  for(auto Support:Supports)for(float Band:{0.f,.5f,.95f,1.02f})
  {
   float Loss[2]={},Track[2]={};
   for(int32 Mode=0;Mode<2;++Mode)
   {
    TGuardValue<bool> Baseline(FrontlineArmourBlastV844::BaselineForReview,Mode==0);
    // High fixture integrity measures the complete transfer without a wreck
    // changing the receiver. Production MaxHull and all real salvo tests remain.
    ResetTransfer();
    FVector P=Hull.TransformPosition(FVector(0,Box.Max.Y+Radius(Support)*Band,Box.Min.Z));
    ApplyBlast(P,Radius(Support),Power(Support),1-V->Team,FFrontlineKillSourceV65::FireSupport(1-V->Team,int32(Support)));
    Loss[Mode]=5000.f-V->HullIntegrity;Track[Mode]=10000.f-V->TrackIntegrity[0]-V->TrackIntegrity[1];
   }
   const bool GeometricFringe=Band>.9f&&Band<1.f&&Support!=EFrontlineSupport::AntiTank
    &&Loss[0]==0.f&&Track[0]==0.f&&(Loss[1]>0.f||Track[1]>0.f);
   if(GeometricFringe)
    PhysicalFringeOnly&=Loss[1]<=OriginalHull*.003f&&Track[1]<=.54f;
   else NonIncrease&=Loss[1]<=Loss[0]+.02f&&Track[1]<=Track[0]+.02f;
   if(Band>1.f&&Support!=EFrontlineSupport::AntiTank)EdgeTapers&=Loss[1]==0&&Track[1]==0;
   if(Support==EFrontlineSupport::AntiTank)APUnchanged&=FMath::IsNearlyEqual(Loss[0],Loss[1],.02f)&&FMath::IsNearlyEqual(Track[0],Track[1],.02f);
   auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("model"),Model);Row->SetStringField(TEXT("model_name"),Definition(EFrontlineVehicle(Model)).Name);Row->SetNumberField(TEXT("support"),int32(Support));Row->SetNumberField(TEXT("radius_band"),Band);Row->SetNumberField(TEXT("production_hull_integrity"),OriginalHull);
   Row->SetNumberField(TEXT("old_hull_damage"),Loss[0]);Row->SetNumberField(TEXT("new_hull_damage"),Loss[1]);Row->SetNumberField(TEXT("old_track_damage"),Track[0]);Row->SetNumberField(TEXT("new_track_damage"),Track[1]);
   Row->SetBoolField(TEXT("corrected_physical_fringe_previously_outside_legacy_envelope"),GeometricFringe);
   Row->SetNumberField(TEXT("actual_hull_half_width_cm"),FMath::Max(FMath::Abs(Box.Min.Y),FMath::Abs(Box.Max.Y)));
   Row->SetNumberField(TEXT("legacy_uniform_hull_half_width_cm"),190.f);
   Row->SetNumberField(TEXT("minimum_new_same_contact_rounds_to_kill"),Loss[1]>0?FMath::CeilToInt(OriginalHull/Loss[1]):0);Matrix.Add(MakeShared<FJsonValueObject>(Row));
   if(Band==0)
   {
    ResetTransfer();
    const FVector P=Hull.TransformPosition(FVector(0,Box.Max.Y,Box.Min.Z));
    ApplyBlast(P,Radius(Support),Power(Support),V->Team,FFrontlineKillSourceV65::FireSupport(V->Team,int32(Support)));
    CommonBothTeams&=FMath::IsNearlyEqual(5000.f-V->HullIntegrity,Loss[1],.02f);
   }
  }
  V->Destroy();Vehicles.Empty();
 }
 Check(TEXT("v844_HE_damage_does_not_increase_inside_the_common_legacy_and_actual_envelopes"),NonIncrease);
 Check(TEXT("v844_corrected_physical_fringe_adds_only_bounded_minor_damage"),PhysicalFringeOnly);
 Check(TEXT("v844_support_edge_and_outside_cannot_equal_direct_hits"),EdgeTapers);
 Check(TEXT("v844_antitank_direct_support_response_and_infantry_routes_remain_unscaled"),APUnchanged);
 Check(TEXT("v844_friendly_and_enemy_support_share_same_armour_response"),CommonBothTeams);
 {
  auto* V=Make(EFrontlineVehicle::MarkVStar);
  if(V)
  {
   const float Before=V->HullIntegrity;FFrontlineStrike S;S.Team=1;S.SourceV65=FFrontlineKillSourceV65::FireSupport(1,int32(EFrontlineSupport::AirRecon));S.Target=V->AimPoint();S.Target.Z+=400;S.Radius=1200;S.bShrapnelV64=true;S.ImpactAt=B->Elapsed;
   B->ResolveFieldStrikeV32(S);Check(TEXT("v844_actual_shrapnel_strike_has_no_heavy_armour_damage"),V->HullIntegrity==Before);
   // Find genuine occlusion on this prepared map; no synthetic shield actor
   // or map-style flag may grant cover. Record the collision readback.
   const int32 Trench=B->Trenches.Num()/2;const float Centre=B->Trenches.IsValidIndex(Trench)?B->Trenches[Trench].X:0.f;
   V->SetActorLocation(FVector(Centre-550.f,300.f,0));V->FitGroundV35(V->GetActorLocation(),0);
   const FTransform H=V->Body->GetComponentTransform();const FBox Box=V->Body->GetStaticMesh()->GetBoundingBox();bool Found=false;
   for(float DX:{150.f,280.f,430.f,580.f})for(float Y:{-300.f,0.f,300.f,600.f})if(!Found)
   {
    const FVector P(Centre+DX,Y,AWarfareFrontlineGround::HeightAt(Centre+DX,Y)+2.f);const FVector L=H.InverseTransformPosition(P);
    const FVector Near(FMath::Clamp(L.X,Box.Min.X,Box.Max.X),FMath::Clamp(L.Y,Box.Min.Y,Box.Max.Y),FMath::Clamp(L.Z,Box.Min.Z,Box.Max.Z));const FVector Contact=H.TransformPosition(Near);
    if(FVector::Distance(P,Contact)<=20.f||FVector::Distance(P,Contact)>=1500.f||!GetWorld()->LineTraceTestByChannel(P+FVector(0,0,8),Contact,ECC_Visibility,SightParams(GetWorld(),nullptr,V)))continue;
    Found=true;V->HullIntegrity=V->TrackIntegrity[0]=V->TrackIntegrity[1]=5000.f;
    ApplyBlast(P,1500.f,440.f,1,FFrontlineKillSourceV65::FireSupport(1,int32(EFrontlineSupport::Artillery)));
    const float Damage=5000.f-V->HullIntegrity;
    auto Cover=MakeShared<FJsonObject>();Cover->SetStringField(TEXT("blast_origin"),P.ToString());Cover->SetStringField(TEXT("physical_hull_contact"),Contact.ToString());Cover->SetNumberField(TEXT("screened_hull_damage"),Damage);Cover->SetBoolField(TEXT("collision_blocked"),true);Report->SetObjectField(TEXT("actual_earth_screening"),Cover);
    Check(TEXT("v844_actual_bank_screens_pressure_without_complete_armour_immunity"),Damage>0&&Damage<440.f*.40f*.72f*.30f);
   }
   Report->SetBoolField(TEXT("prepared_map_has_a_sampled_earth_screening_case"),Found);V->Destroy();Vehicles.Empty();
  }
 }
 // Real ResolveStrike chains at real model integrity: three close rounds or
 // all twelve. Fixed impacts deliberately hold the tank inside the bombardment
 // after tracks fail; this is a worst-case survivability bound, not hit chance.
 bool Once=true;
 for(auto Type:{EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkVStar,EFrontlineVehicle::A7V,EFrontlineVehicle::Whippet})
  for(auto Support:Supports)for(float Band:{0.f,.5f})for(int32 Mode=0;Mode<2;++Mode)
  {
   auto* V=Make(Type);if(!V){Check(TEXT("v844_live_salvo_model_spawn"),false);continue;}
   TGuardValue<bool> Baseline(FrontlineArmourBlastV844::BaselineForReview,Mode==0);
   const float H0=V->HullIntegrity,T0=V->TrackIntegrity[0]+V->TrackIntegrity[1];const float Start=B->Elapsed;
   int32 Fired=0,DisabledAt=INDEX_NONE,KilledAt=INDEX_NONE;float KillTime=-1,DisableTime=-1,ThreeHull=0;
   const auto Box=V->Body->GetStaticMesh()->GetBoundingBox();FVector Ground=V->GetActorTransform().TransformPosition(FVector(0,Box.Max.Y+15.f+Band*Radius(Support),0));Ground.Z=AWarfareFrontlineGround::HeightAt(Ground.X,Ground.Y)+(Support==EFrontlineSupport::AntiTank?105.f:2.f);
   const float Spacing=Support==EFrontlineSupport::AntiTank?3.3f:Support==EFrontlineSupport::AirBombing?.55f:.70f;
   const float SalvoGap=Support==EFrontlineSupport::Mortar?5.8f:Support==EFrontlineSupport::CreepingBarrage?5.2f:5.4f;
   const int32 Ammunition=Support==EFrontlineSupport::AntiTank?6:12;
   for(int32 Round=0;Round<Ammunition&&V->IsOperational();++Round)
   {
    const float Since=Support==EFrontlineSupport::AntiTank||Support==EFrontlineSupport::AirBombing?Round*Spacing:(Round/3)*SalvoGap+(Round%3)*Spacing;
    B->Elapsed=Start+Since;FFrontlineStrike S;S.Team=1;S.SourceV65=FFrontlineKillSourceV65::FireSupport(1,int32(Support));S.Target=Ground;S.Radius=Radius(Support);S.ArmorPower=Power(Support);S.bMortar=Support==EFrontlineSupport::Mortar||Support==EFrontlineSupport::AntiTank;S.bDirectFire=Support==EFrontlineSupport::AntiTank;S.bAirDropped=Support==EFrontlineSupport::AirBombing;S.FireMissionId=844000+int32(Type)*100+int32(Support)*2+Mode;S.SalvoIndex=Round/3;S.RoundIndex=Round%3;S.LaunchedAt=B->Elapsed-.5f;S.ImpactAt=B->Elapsed;
    B->ResolveFieldStrikeV32(S);++Fired;const float After=V->HullIntegrity;B->ResolveFieldStrikeV32(S);
    Once&=V->HullIntegrity==After;
    if(Round==2)ThreeHull=V->HullIntegrity;
    if(V->IsImmobilized()&&DisabledAt==INDEX_NONE){DisabledAt=Fired;DisableTime=Since;}
    if(!V->IsOperational()){KilledAt=Fired;KillTime=Since;}
   }
   auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("model"),int32(Type));Row->SetNumberField(TEXT("support"),int32(Support));Row->SetNumberField(TEXT("radius_band"),Band);Row->SetBoolField(TEXT("baseline"),Mode==0);Row->SetNumberField(TEXT("initial_hull"),H0);Row->SetNumberField(TEXT("initial_tracks"),T0);Row->SetNumberField(TEXT("rounds_resolved"),Fired);Row->SetNumberField(TEXT("remaining_after_first_three"),ThreeHull);Row->SetNumberField(TEXT("remaining_hull"),V->HullIntegrity);Row->SetNumberField(TEXT("immobilized_on_round"),DisabledAt);Row->SetNumberField(TEXT("immobilized_after_seconds"),DisableTime);Row->SetNumberField(TEXT("destroyed_on_round"),KilledAt);Row->SetNumberField(TEXT("destroyed_after_seconds"),KillTime);Salvos.Add(MakeShared<FJsonValueObject>(Row));
   V->Destroy();Vehicles.Empty();B->Morale[0]=M0;B->Morale[1]=M1;
  }
 Check(TEXT("v844_one_committed_shell_cannot_double_resolve"),Once);
 Report->SetStringField(TEXT("scope"),TEXT("Actual ApplyBlast/ReceiveImpact transfer on all 13 imported configurations and real ResolveStrike chains for four representative hulls. Close, half-radius, fringe and outside single hits; real close/half-radius 3-round and 12-round timings include continued impacts after tracks fail. Fixed worst-case ground contacts do not claim natural hit rates or whole-campaign feasibility. Single-transfer integrity is 5000 only in the disposable fixture to avoid changing its material/mesh midway."));
 return Report;
}
