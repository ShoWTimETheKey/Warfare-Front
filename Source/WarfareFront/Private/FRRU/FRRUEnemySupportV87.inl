// Native companies use the existing UK/DE observed-support policy. These are
// decision weights/observation snapshots, never extra damage or supply grants.
namespace
{
struct FEnemyMaskObservationV87 {float SeenAt=0;bool Sealed=false;};
struct FEnemyArmourObservationV87 {FVector Point=FVector::ZeroVector;float SeenAt=0;};
struct FEnemySupportPlanV87
{
    FGuid Encounter;bool Pending=false;float LastClock=0,AssessAt=22,ObservedAt=0,RegisteredAt=0,ExecuteAt=0;
    EFrontlineSupport Kind=EFrontlineSupport::Count;FVector Point=FVector::ZeroVector;FString Basis;
    TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FEnemyMaskObservationV87> Masks;
    TMap<TWeakObjectPtr<AFrontlineTank>,FEnemyArmourObservationV87> Armour;
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FEnemySupportPlanV87> EnemySupportPlansV87;

bool ObserveEnemyArmourV87(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank,FVector& ObservedPoint)
{
    // Match the established UK/DE observation range and obstruction test. A
    // vehicle outside a real enemy observer's LOS never becomes a candidate.
    if(!B||!Tank||Tank->Team!=0||!Tank->IsOperational()||!Tank->HasEnteredField())return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV87ObservedArmour),true,Tank);
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U)Query.AddIgnoredActor(U);
    for(const AWarfareFrontlineSoldier* Observer:B->Units)if(Observer&&Observer->Team==1&&Observer->IsActiveCombatant())
    {
        const FVector Point=Tank->ObservedArmourAimPointV837(Observer->Feet());FHitResult Hit;
        if(FVector::Dist2D(Observer->Feet(),Point)<=6000.f*B->EncounterVisibilityScale()
            &&B->DirectFireMultiplier(Observer->Feet(),Point)>=.9f
            &&!B->GetWorld()->LineTraceSingleByChannel(Hit,Observer->Feet()+FVector(0,0,145),Point,ECC_Visibility,Query))
        {ObservedPoint=Point;return true;}
    }
    return false;
}

void TickEnemySupportV87(AWarfareFrontlineGameMode* B,FWorld& R,const TArray<AWarfareFrontlineSoldier*>& Seen)
{
    if(!B||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE||R.State.objective.finished)return;
    for(auto I=EnemySupportPlansV87.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
    auto& Plan=EnemySupportPlansV87.FindOrAdd(B);const float Now=B->Elapsed;
    if(Plan.Encounter!=B->GetEncounterId()||Now<Plan.LastClock)
    {Plan=FEnemySupportPlanV87();Plan.Encounter=B->GetEncounterId();Plan.AssessAt=float(R.EnemySupportAt);}
    Plan.LastClock=Now;
    auto& Tactics=FrontlineDifficultyV30::State(B);const auto& D=FrontlineDifficultyV30::Doctrine(B->GetCampaignDifficulty());
    // Only the caller's CURRENTLY visible soldiers may reveal mask state.
    // Remembered contacts retain that observed value, never current actor data.
    int32 SeenMarksmen=0;bool SeenGun=false;
    for(auto* U:Seen)if(U&&U->Team==0&&U->IsActiveCombatant())
    {
        Plan.Masks.Add(U,{Now,FrontlineMotionV40::MaskSealed(U)});
        SeenMarksmen+=U->Role==EFrontlineRole::Marksman;SeenGun|=U->IsAutomaticWeapon()&&U->ShotsDischarged>0;
    }
    for(auto I=Plan.Masks.CreateIterator();I;++I)if(Now-I.Value().SeenAt>D.MemorySeconds)I.RemoveCurrent();
    if(WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::AntiTank,1))
        if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* Tank:Fleet->GetVehicles())
        {FVector Point;if(ObserveEnemyArmourV87(B,Tank,Point))Plan.Armour.Add(Tank,{Point,Now});}
    for(auto I=Plan.Armour.CreateIterator();I;++I)if(Now-I.Value().SeenAt>D.MemorySeconds)I.RemoveCurrent();

    if(Plan.Pending)
    {
        if(Now<Plan.ExecuteAt)return;
        const bool Fresh=Now-Plan.ObservedAt<D.MemorySeconds;
        // A report must be current when the battery order is registered. Once
        // accepted, transmission is not cancelled by the observer forgetting
        // that contact. The gun still fires at the original, dispersed point.
        const bool Registered=FMath::IsFinite(Plan.RegisteredAt)&&FMath::IsFinite(Plan.ObservedAt)
            &&Plan.RegisteredAt>=Plan.ObservedAt&&Plan.RegisteredAt-Plan.ObservedAt<D.MemorySeconds
            &&Plan.RegisteredAt<=Plan.ExecuteAt&&!Plan.Point.ContainsNaN();
        const bool Coordinated=Now-Tactics.LastSupportAt>=D.FireCoordinationGap;
        const bool Available=WarfareFRRUEquipmentV82::SupportAvailable(B,Plan.Kind,1);
        const bool Ready=Available&&B->SupportBlockReason(Plan.Kind,1).IsEmpty();
        const bool Safe=B->IsFireSupportAreaSafe(Plan.Point,Plan.Kind,1);
        const bool Legal=Registered&&Coordinated&&Ready&&Safe;
        // Execution reads neither a stored target actor nor a replacement
        // contact: losing LOS or moving away cannot make this point follow.
        const bool Fired=Legal&&B->RequestSupport(Plan.Kind,Plan.Point,1);
        if(Fired){Tactics.LastSupportAt=Now;++Tactics.PlannedFireMissions;}
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ENEMY_SUPPORT_EXECUTED mission=%s encounter=%s t=%.3f support=%d fresh=%d legal=%d fired=%d basis=%s x=%.3f y=%.3f observed_at=%.3f registered_at=%.3f dispatch_lag=%.3f registered=%d coordinated=%d available=%d ready=%d safe=%d"),
            *R.Mission,*Plan.Encounter.ToString(),Now,int32(Plan.Kind),int32(Fresh),int32(Legal),int32(Fired),*Plan.Basis,Plan.Point.X,Plan.Point.Y,Plan.ObservedAt,
            Plan.RegisteredAt,Now-Plan.ExecuteAt,int32(Registered),int32(Coordinated),int32(Available),int32(Ready),int32(Safe));
        Plan.Pending=false;Plan.AssessAt=Now+R.EnemyRandom.FRandRange(D.AssessMin,D.AssessMax);return;
    }
    if(Now<Plan.AssessAt||Now-Tactics.LastSupportAt<D.FireCoordinationGap)return;
    Plan.AssessAt=Now+R.EnemyRandom.FRandRange(D.AssessMin,D.AssessMax);
    double Best=0;EFrontlineSupport BestKind=EFrontlineSupport::Count;FString Basis;
    FrontlineDifficultyV30::FContact Registration;
    const auto Offer=[&](EFrontlineSupport Kind,double Score,const FrontlineDifficultyV30::FContact& C,const TCHAR* Reason,bool Extended=true)
    {
        if(Score>Best&&(!Extended||Score>D.MinimumTargetValue)&&WarfareFRRUEquipmentV82::SupportAvailable(B,Kind,1)
            &&B->SupportBlockReason(Kind,1).IsEmpty()&&B->IsFireSupportAreaSafe(C.Position,Kind,1))
        {Best=Score;BestKind=Kind;Registration=C;Basis=Reason;}
    };
    for(const auto& Item:Plan.Armour)
    {
        FrontlineDifficultyV30::FContact C;C.Position=Item.Value.Point;C.LastSeen=Item.Value.SeenAt;
        Offer(EFrontlineSupport::AntiTank,6.f*FrontlineDifficultyV30::Confidence(C,Now,D),C,TEXT("observed_armour"));
    }
    const FrontlineDifficultyV30::FContact* ScreenThreat=nullptr;float ScreenThreatScore=0;
    for(const auto& C:Tactics.Contacts)
    {
        const float Confidence=FrontlineDifficultyV30::Confidence(C,Now,D);if(Confidence<=0)continue;
        const float Value=FrontlineDifficultyV30::TargetScore(C,Tactics.Contacts,Now,D);
        if(Value>ScreenThreatScore){ScreenThreat=&C;ScreenThreatScore=Value;}
        float Fixed=0,Moving=0,UnmaskedFixed=0,Nearby=0;
        for(const auto& N:Tactics.Contacts)
        {
            const float Certainty=FrontlineDifficultyV30::Confidence(N,Now,D);
            if(FVector::DistSquared2D(N.Position,C.Position)<FMath::Square(520.f))Nearby+=Certainty;
            if(FMath::Abs(N.Position.X-C.Position.X)>1000||FMath::Abs(N.Position.Y-C.Position.Y)>650)continue;
            if(N.Moving)Moving+=Certainty;
            else
            {
                Fixed+=Certainty;
                const auto* Mask=Plan.Masks.Find(N.Identity);
                if(Mask&&!Mask->Sealed&&Now-Mask->SeenAt<D.MemorySeconds)UnmaskedFixed+=Certainty;
            }
        }
        if(Value>D.MinimumTargetValue)
        {
            wfexp::tactical::ObservedFireTarget T;T.importance=Value;T.confidence=Confidence;T.nearbyWeight=Nearby;
            T.moving=C.Moving;T.entrenched=C.Entrenched;T.firingMachinegun=C.Machinegun&&C.Fired;
            Offer(EFrontlineSupport::Mortar,wfexp::tactical::ObservedSupportScore(wfexp::tactical::ObservedFireKind::Mortar,T),C,TEXT("observed_trench_or_gun"),false);
            Offer(EFrontlineSupport::Artillery,wfexp::tactical::ObservedSupportScore(wfexp::tactical::ObservedFireKind::FieldArtillery,T),C,TEXT("observed_concentration"),false);
            Offer(EFrontlineSupport::AirRecon,wfexp::tactical::ObservedSupportScore(wfexp::tactical::ObservedFireKind::Shrapnel,T),C,TEXT("observed_infantry_airburst"),false);
        }
        // These density/weight rules are the established ServiceFireV84415
        // consumers. National/date availability remains a separate hard gate.
        if(Confidence>.35f)
        {
            if(UnmaskedFixed>=4)Offer(EFrontlineSupport::Gas,(UnmaskedFixed*.90f+(C.Entrenched?1.f:0.f))*Confidence,C,TEXT("observed_unmasked_fixed_group"));
            if(Moving>=4)Offer(EFrontlineSupport::AirStrike,Moving*1.10f*Confidence,C,TEXT("observed_moving_group"));
            if(Fixed>=4)Offer(EFrontlineSupport::CreepingBarrage,(Fixed*.85f+(C.Machinegun?1.f:0.f))*Confidence,C,TEXT("observed_fixed_group"));
            if(Fixed+Moving>=6)Offer(EFrontlineSupport::AirBombing,(Fixed+Moving)*.95f*Confidence,C,TEXT("observed_large_group"));
            if(Fixed>=3)Offer(EFrontlineSupport::AntiTank,Fixed*.75f*Confidence,C,TEXT("observed_fixed_infantry"));
        }
    }
    if(ScreenThreat&&WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::Smoke,1)
        &&B->SupportBlockReason(EFrontlineSupport::Smoke,1).IsEmpty())
    {
        const AWarfareFrontlineSoldier* Forward=nullptr;
        for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==1&&U->IsActiveCombatant()
            &&((U->bMoving&&U->Suppression>D.SmokePressure)||(!U->bMoving&&SeenGun&&U->Suppression>D.SmokePressure)
                ||(SeenMarksmen>=2&&U->Suppression>.12f))
            &&(!Forward||U->Feet().X*B->GetTeamDirection(1)>Forward->Feet().X*B->GetTeamDirection(1)))Forward=U;
        if(Forward)
        {
            const FVector Screen=Forward->Feet()+FVector(B->GetTeamDirection(1)*600.f,0,0);
            if(B->DirectFireMultiplier(Forward->Feet(),Screen)>=.95f)
            {
                auto C=*ScreenThreat;C.Position=Screen;C.ObservedVelocity=FVector::ZeroVector;C.Moving=false;C.LastSeen=Now;
                Offer(EFrontlineSupport::Smoke,ScreenThreatScore,C,TEXT("own_exposed_pressed_section"));
            }
        }
    }
    if(BestKind==EFrontlineSupport::Count)return;
    const float ObservationAge=Now-Registration.LastSeen;
    if(!FMath::IsFinite(ObservationAge)||ObservationAge<0||ObservationAge>=D.MemorySeconds)return;
    const FVector Point=FrontlineDifficultyV30::RegisteredPoint(Registration,Now,D,R.EnemyRandom);
    if(!B->IsFireSupportAreaSafe(Point,BestKind,1))return;
    Plan.Kind=BestKind;Plan.Point=Point;Plan.ObservedAt=Registration.LastSeen;Plan.RegisteredAt=Now;Plan.Basis=Basis;
    Plan.ExecuteAt=Now+R.EnemyRandom.FRandRange(D.ReactionMin,D.ReactionMax);Plan.Pending=true;
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_ENEMY_SUPPORT_PLANNED mission=%s encounter=%s difficulty=%d t=%.3f support=%d score=%.3f basis=%s execute_at=%.3f observed_at=%.3f x=%.3f y=%.3f"),
        *R.Mission,*Plan.Encounter.ToString(),int32(B->GetCampaignDifficulty()),Now,int32(BestKind),Best,*Basis,Plan.ExecuteAt,Plan.ObservedAt,Point.X,Point.Y);
}
}
