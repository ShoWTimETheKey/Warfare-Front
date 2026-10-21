#include "WarfareFrontlineBlockedFireProbeV87.h"
#if WITH_EDITOR
#include "WarfareFrontline.h"
#include "WarfareFrontlineMachinegunsV30.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFRRUPresentationV82.h"
#include "WarfareFRRUWorldV82.h"
#include "FRRU/Native/Expansion.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace FrontlineBlockedFireProbeV87
{
namespace
{
struct FWatch
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Unit,Contact,TickTarget,DischargeTarget;
    int32 Shots=0,Attempts=0;double ContactSince=-1,ShotChangedAt=0,ObservedAt=-1,AttemptAt=-1;
    bool Stops=false,WantsMovement=false,CrewReform=false,AtDestination=false;
    FVector Destination=FVector::ZeroVector;
    float ShotClock=0,AimSettleClock=0,FirePoseClock=0,DepartureClock=0;
    FString TickReason=TEXT("contact_observed_before_stance_update"),DischargeReason=TEXT("never_called");
    TMap<FString,int32> Reasons;
};
FGuid Encounter;TWeakObjectPtr<const AWarfareFrontlineGameMode> WatchedBattle;
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FWatch> Watches;
TSharedPtr<FJsonObject> Pending;
bool NormalPersistenceIsolationV87()
{
    // Reuse the authoritative normal-review boundary in full. This adds no
    // NoSave exception and cannot redirect a player repository or Continue.
    static const bool Allowed=[]()
    {
        FString User,Mode,Nation;
        return FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview"))
            &&FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceMode="),Mode)&&Mode==TEXT("start")
            &&FParse::Value(FCommandLine::Get(),TEXT("FRRUPersistenceNation="),Nation)&&Nation==TEXT("fr")
            &&WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(User);
    }();return Allowed;
}
void Bind(const AWarfareFrontlineGameMode* Battle)
{
    if(WatchedBattle.Get()==Battle&&Encounter==Battle->GetEncounterId())return;
    WatchedBattle=Battle;Encounter=Battle->GetEncounterId();Watches.Reset();Pending.Reset();
}
FWatch* Watch(AWarfareFrontlineSoldier* Unit)
{
    if(!Unit||!Enabled(Unit->Battle))return nullptr;
    Bind(Unit->Battle);return Watches.Find(TWeakObjectPtr<AWarfareFrontlineSoldier>(Unit));
}
TSharedRef<FJsonObject> Identity(const AWarfareFrontlineSoldier* Unit)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("present"),IsValid(Unit));if(!IsValid(Unit))return J;
    J->SetNumberField(TEXT("squad"),Unit->SquadId);J->SetNumberField(TEXT("member"),Unit->MemberIndex);
    J->SetNumberField(TEXT("team"),Unit->Team);J->SetNumberField(TEXT("role"),int32(Unit->Role));
    J->SetBoolField(TEXT("alive"),Unit->IsAlive());J->SetBoolField(TEXT("active"),Unit->IsActiveCombatant());
    J->SetStringField(TEXT("feet"),Unit->Feet().ToString());return J;
}
TSharedRef<FJsonObject> Weapon(const wfexp::Weapon* Value)
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("resolved"),Value!=nullptr);if(!Value)return J;
    J->SetStringField(TEXT("id"),UTF8_TO_TCHAR(Value->id));J->SetStringField(TEXT("kind"),UTF8_TO_TCHAR(Value->kind));
    J->SetNumberField(TEXT("fireRangeCm"),Value->stats.fireRangeCm);J->SetNumberField(TEXT("advanceContactCm"),Value->stats.advanceContactCm);return J;
}
TSharedRef<FJsonObject> TargetState(const AWarfareFrontlineSoldier* Unit,const AWarfareFrontlineSoldier* Target)
{
    auto J=Identity(Target);if(!IsValid(Target))return J;const auto* B=Unit->Battle.Get();
    const float Visibility=B->EncounterVisibilityScale(),Concealment=Unit->Role!=EFrontlineRole::Mills&&B->DirectFireMultiplier(Unit->Feet(),Target->Feet())<1.f?.65f:1.f;
    J->SetNumberField(TEXT("distance2DCm"),FVector::Dist2D(Unit->Feet(),Target->Feet()));
    J->SetNumberField(TEXT("actualFireRangeDistanceCm"),FrontlineWeapons::DirectFireRangeDistanceCm(Unit,Target->Feet()));
    J->SetNumberField(TEXT("actualFireRangeLimitCm"),FrontlineWeapons::ProfileFor(Unit).FireRangeCm*Visibility);
    J->SetNumberField(TEXT("advanceContactLimitBeforeRetentionCm"),FrontlineWeapons::AdvanceContactRangeFor(Unit)*Visibility*Concealment*B->AssaultContactScale(Unit));
    J->SetNumberField(TEXT("concealmentContactScale"),Concealment);J->SetNumberField(TEXT("directFireMultiplier"),B->DirectFireMultiplier(Unit->Feet(),Target->Feet()));
    J->SetBoolField(TEXT("unitInOwnSmoke"),B->IsInFriendlySmoke(Unit->Feet(),Unit->Team));
    J->SetBoolField(TEXT("targetInOwnSmoke"),B->IsInFriendlySmoke(Target->Feet(),Target->Team));
    J->SetBoolField(TEXT("observationSight"),B->HasSight(Unit,Target));J->SetBoolField(TEXT("nationalActualMuzzleFiringLine"),WarfareFRRUWorldV82::FiringLine(Unit,Target));
    if(const auto* Gun=FrontlineMachinegunsV30::Get(Unit))
    {J->SetBoolField(TEXT("supportedMuzzleFiringLine"),Gun->HasInfantryFiringLine(Target));J->SetBoolField(TEXT("gunCoversTargetYaw"),Gun->CoversYaw((Target->Feet()-Unit->Feet()).Rotation().Yaw));}
    return J;
}
TSharedRef<FJsonObject> MemberState(AWarfareFrontlineSoldier* Unit)
{
    auto J=Identity(Unit);if(!IsValid(Unit))return J;const auto* B=Unit->Battle.Get();
    J->SetBoolField(TEXT("isMachineGunner"),Unit->IsMachineGunner());J->SetBoolField(TEXT("recoveredMachineGun"),Unit->bRecoveredMachineGun);
    J->SetBoolField(TEXT("automaticWeapon"),Unit->IsAutomaticWeapon());J->SetBoolField(TEXT("fieldEntered"),Unit->bFieldEntered);
    J->SetBoolField(TEXT("moving"),Unit->bMoving);J->SetBoolField(TEXT("engaging"),Unit->bEngaging);
    J->SetNumberField(TEXT("shotsDischarged"),Unit->ShotsDischarged);J->SetNumberField(TEXT("roundsRemaining"),Unit->RoundsRemaining);
    J->SetNumberField(TEXT("weaponPhase"),int32(Unit->WeaponPhase));J->SetNumberField(TEXT("suppression"),Unit->Suppression);
    J->SetNumberField(TEXT("motionSpeed"),Unit->GetMotionSpeed());J->SetStringField(TEXT("muzzle"),Unit->GetMuzzleLocation().ToString());
    float FeetFloor=0,MuzzleFloor=0;const FVector Muzzle=Unit->GetMuzzleLocation();
    const bool HasFeetFloor=WarfareFRRUWorldV82::TryHeight(B,Unit->Feet().X,Unit->Feet().Y,FeetFloor);
    const bool HasMuzzleFloor=WarfareFRRUWorldV82::TryHeight(B,Muzzle.X,Muzzle.Y,MuzzleFloor);
    J->SetBoolField(TEXT("feetFloorResolved"),HasFeetFloor);J->SetNumberField(TEXT("feetFloorZ"),FeetFloor);
    J->SetBoolField(TEXT("muzzleFloorResolved"),HasMuzzleFloor);J->SetNumberField(TEXT("muzzleFloorZ"),MuzzleFloor);
    J->SetNumberField(TEXT("muzzleHeightAboveLocalFloorCm"),Muzzle.Z-MuzzleFloor);
    const auto* Role=wfexp::RoleFor(static_cast<wfexp::Faction>(uint8(WarfareFRRUCampaignV82::Nation(Unit))),int32(Unit->Role),WarfareFRRUCampaignV82::CalendarDate(B));
    J->SetObjectField(TEXT("rolePrimaryWeapon"),Weapon(Role&&Role->weaponId?wfexp::FindWeapon(Role->weaponId):nullptr));
    J->SetObjectField(TEXT("roleEscortWeapon"),Weapon(Role&&Role->escortWeaponId?wfexp::FindWeapon(Role->escortWeaponId):nullptr));
    const auto* Equipment=WarfareFRRUEquipmentV82::Resolve(Unit);J->SetStringField(TEXT("actualResolvedWeaponId"),Equipment?Equipment->WeaponId:TEXT("UNRESOLVED"));
    const auto& Profile=FrontlineWeapons::ProfileFor(Unit);J->SetStringField(TEXT("actualProfileWeaponId"),Profile.WeaponId);
    J->SetNumberField(TEXT("actualProfileFireRangeCm"),Profile.FireRangeCm);J->SetNumberField(TEXT("actualProfileAdvanceContactCm"),Profile.AdvanceContactCm);
    J->SetNumberField(TEXT("effectiveAdvanceContactRangeCm"),FrontlineWeapons::AdvanceContactRangeFor(Unit));
    J->SetNumberField(TEXT("visibilityScale"),B->EncounterVisibilityScale());J->SetNumberField(TEXT("assaultContactScale"),B->AssaultContactScale(Unit));
    if(const FWatch* W=Watches.Find(TWeakObjectPtr<AWarfareFrontlineSoldier>(Unit)))
    {
        J->SetNumberField(TEXT("continuousStoppingContactSinceHost"),W->ContactSince);J->SetNumberField(TEXT("shotsLastChangedAtHost"),W->ShotChangedAt);
        J->SetNumberField(TEXT("lastObservedAtHost"),W->ObservedAt);J->SetBoolField(TEXT("stopsForContact"),W->Stops);
        J->SetBoolField(TEXT("wantsMovement"),W->WantsMovement);J->SetBoolField(TEXT("crewReform"),W->CrewReform);J->SetBoolField(TEXT("atDestination"),W->AtDestination);
        J->SetStringField(TEXT("destination"),W->Destination.ToString());J->SetStringField(TEXT("latestTickGate"),W->TickReason);
        J->SetNumberField(TEXT("shotClock"),W->ShotClock);J->SetNumberField(TEXT("aimSettleClock"),W->AimSettleClock);
        J->SetNumberField(TEXT("firePoseClock"),W->FirePoseClock);J->SetNumberField(TEXT("departureClock"),W->DepartureClock);
        J->SetNumberField(TEXT("actualDischargeCallsInNoShotWindow"),W->Attempts);J->SetNumberField(TEXT("lastActualDischargeAtHost"),W->AttemptAt);
        J->SetStringField(TEXT("lastActualDischargeResult"),W->DischargeReason);auto Reasons=MakeShared<FJsonObject>();
        for(const auto& Pair:W->Reasons)Reasons->SetNumberField(Pair.Key,Pair.Value);J->SetObjectField(TEXT("actualDischargeResultsInNoShotWindow"),Reasons);
        J->SetObjectField(TEXT("advanceContactTarget"),TargetState(Unit,W->Contact.Get()));
        J->SetObjectField(TEXT("actualTickFireTarget"),TargetState(Unit,W->TickTarget.Get()));
        J->SetObjectField(TEXT("lastActualDischargeTarget"),TargetState(Unit,W->DischargeTarget.Get()));
    }
    J->SetObjectField(TEXT("focusTarget"),TargetState(Unit,Unit->GetObservedInfantryTargetV844()));
    if(const auto* Gun=FrontlineMachinegunsV30::Get(Unit))
    {
        J->SetNumberField(TEXT("gunState"),int32(Gun->State));J->SetBoolField(TEXT("gunCanFireReadOnly"),Gun->CanFire());
        J->SetBoolField(TEXT("gunHasAssets"),Gun->HasAssets());J->SetBoolField(TEXT("gunCoversActorYaw"),Gun->CoversYaw(Unit->GetActorRotation().Yaw));
        J->SetNumberField(TEXT("gunBlockedShotAttempts"),Gun->BlockedShotAttempts);J->SetNumberField(TEXT("gunPacks"),Gun->Packs);J->SetNumberField(TEXT("gunDeployments"),Gun->Deployments);
        J->SetObjectField(TEXT("gunActualPoseAndClocks"),Gun->DescribeTrenchFiringSectorV844());J->SetObjectField(TEXT("gunPhysicalSupport"),Gun->DescribeSupportContactsV8447());
        int32 Contacts=0;J->SetBoolField(TEXT("nationalGunSupportContactsReady"),WarfareFRRUPresentationV82::GunSupportContacts(Unit,Contacts));J->SetNumberField(TEXT("nationalGunSupportContacts"),Contacts);
    }
    if(Unit->Soldier)if(auto* Node=Unit->Soldier->GetSingleNodeInstance())
    {J->SetNumberField(TEXT("animationTime"),Node->GetCurrentTime());J->SetNumberField(TEXT("animationLength"),Node->GetLength());J->SetNumberField(TEXT("animationPlayRate"),Node->GetPlayRate());}
    J->SetObjectField(TEXT("actualMovement"),WarfareFRRUWorldV82::MovementDiagnosis(Unit,true));return J;
}
}
bool Requested(){static const bool Value=FParse::Param(FCommandLine::Get(),TEXT("FRRUBlockedFireProbe"));return Value;}
bool Isolation()
{
    if(!Requested())return false;
    static const bool Allowed=[]()
    {
        if(NormalPersistenceIsolationV87())return true;
        for(const TCHAR* Flag:{TEXT("WFRenderNoSave"),TEXT("WFFRRUV82Review"),TEXT("WFFRRUV82WorldReview"),TEXT("WFFRRUV82NaturalReview")})
            if(!FParse::Param(FCommandLine::Get(),Flag))return false;
        if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82VisualCaptureOnly")))return false;
        FString Case,User;if(!FParse::Value(FCommandLine::Get(),TEXT("FRRUNaturalCase="),Case)||(Case!=TEXT("mort-hotchkiss")&&Case!=TEXT("mort-ordinary"))
            ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User)||User.Contains(TEXT("..")))return false;
        FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
        if(!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase))return false;
        User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
        const FString Prefix=Project+TEXT("/Saved/FRRU/V82NaturalReview/")+Case+TEXT("/");
        if(!User.StartsWith(Prefix,ESearchCase::IgnoreCase)||!User.EndsWith(TEXT("/User"),ESearchCase::IgnoreCase))return false;
        const FString Token=User.Mid(Prefix.Len(),User.Len()-Prefix.Len()-5);FGuid Run;
        return Token.Len()==32&&FGuid::ParseExact(Token,EGuidFormats::Digits,Run)&&Run.IsValid();
    }();return Allowed;
}
bool Enabled(const AWarfareFrontlineGameMode* Battle)
{return Isolation()&&Battle&&(Battle->IsProgressionVolatile()||NormalPersistenceIsolationV87())&&WarfareFRRUCampaignV82::Active(Battle)
    &&WarfareFRRUCampaignV82::Nation(Battle)==EFRRUNationV81::French&&WarfareFRRUCampaignV82::MissionId(Battle)==TEXT("fr.mort_homme");}
void Observe(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Contact,bool StopsForContact,bool WantsMovement,bool CrewReform,bool AtDestination,const FVector& Destination)
{
    if(!Unit||!Enabled(Unit->Battle)||Unit->Team!=0||(Unit->Role!=EFrontlineRole::Lewis&&Unit->Role!=EFrontlineRole::MobileGun))return;
    Bind(Unit->Battle);auto& W=Watches.FindOrAdd(TWeakObjectPtr<AWarfareFrontlineSoldier>(Unit));const double Host=Unit->Battle->Elapsed;
    const auto* Group=Unit->Battle->FindSquad(Unit->SquadId);
    const bool Stopping=Unit->IsActiveCombatant()&&Group&&Group->bAdvancing&&!Group->bHoldPosition&&IsValid(Contact)&&StopsForContact&&!WantsMovement;
    if(W.ObservedAt<0||W.Shots!=Unit->ShotsDischarged)
    {W.ShotChangedAt=Host;W.Shots=Unit->ShotsDischarged;W.Attempts=0;W.Reasons.Reset();}
    if(!Stopping)W.ContactSince=-1;else if(W.ContactSince<0)W.ContactSince=Host;
    W.Unit=Unit;W.Contact=Contact;W.Stops=Stopping;W.ObservedAt=Host;W.WantsMovement=WantsMovement;W.CrewReform=CrewReform;W.AtDestination=AtDestination;W.Destination=Destination;
    W.TickReason=TEXT("contact_observed_before_stance_update");
}
void TickGate(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Target,const TCHAR* Reason,float ShotClock,float AimSettleClock,float FirePoseClock,float DepartureClock)
{
    if(auto* W=Watch(Unit))
    {W->TickTarget=Target;W->TickReason=Reason;W->ShotClock=ShotClock;W->AimSettleClock=AimSettleClock;W->FirePoseClock=FirePoseClock;W->DepartureClock=DepartureClock;}
}
void Discharge(AWarfareFrontlineSoldier* Unit,AWarfareFrontlineSoldier* Target,AFrontlineTank* Vehicle,const TCHAR* Reason)
{
    if(auto* W=Watch(Unit))
    {
        W->AttemptAt=Unit->Battle->Elapsed;W->DischargeTarget=Target;W->DischargeReason=Reason;++W->Attempts;++W->Reasons.FindOrAdd(Reason);
        if(Unit->ShotsDischarged!=W->Shots)
        {W->ShotChangedAt=Unit->Battle->Elapsed;W->Shots=Unit->ShotsDischarged;W->Attempts=0;W->Reasons.Reset();}
        // The allowed cases diagnose infantry stopping contact. Armour is
        // identified separately rather than treated as an infantry target.
        if(Vehicle)W->DischargeReason+=TEXT(":armour_target");
    }
}
TSharedPtr<FJsonObject> Failure(const AWarfareFrontlineGameMode* Battle)
{
    if(!Enabled(Battle)||Battle->Phase!=EFrontlinePhase::Fighting)return nullptr;Bind(Battle);if(Pending)return Pending;
    for(const auto& Pair:Watches)
    {
        const auto& W=Pair.Value;auto* Unit=W.Unit.Get();
        if(!Unit||!Unit->IsActiveCombatant()||!W.Stops||W.ContactSince<0||Battle->Elapsed-W.ObservedAt>1.f||Unit->ShotsDischarged!=W.Shots)continue;
        // Bearers may legitimately remain in support behind cover while the
        // living operator works the gun. A recovered weapon makes its new
        // operator IsMachineGunner(), so the authoritative gun remains watched.
        // Keep every crew member's actual gates in the operator's diagnostic.
        if(!Unit->IsMachineGunner()&&FrontlineMachinegunsV30::LivingGunner(Unit))continue;
        const double NoShotSeconds=Battle->Elapsed-FMath::Max(W.ContactSince,W.ShotChangedAt);if(NoShotSeconds<10.)continue;
        Pending=MakeShared<FJsonObject>();Pending->SetStringField(TEXT("revision"),TEXT("v87.blocked_fire_probe.2"));
        Pending->SetStringField(TEXT("scope"),TEXT("Explicit WITH_EDITOR isolated NoSave or strictly qualified normal-start observation only; no fire, clock, order, placement or balance changes. A 10-host-second stalled contact is diagnostic failure, not proof of its cause."));
        Pending->SetStringField(TEXT("encounter"),Encounter.ToString(EGuidFormats::Digits));Pending->SetNumberField(TEXT("hostElapsed"),Battle->Elapsed);
        Pending->SetNumberField(TEXT("continuousContactWithoutActualDischargeSeconds"),NoShotSeconds);Pending->SetObjectField(TEXT("triggerMember"),Identity(Unit));
        Pending->SetObjectField(TEXT("actualObjective"),WarfareFRRUWorldV82::ObjectiveDiagnosis(Battle));TArray<TSharedPtr<FJsonValue>> Crew;
        for(const auto& Member:Battle->Units)if(Member&&Member->Team==Unit->Team&&Member->SquadId==Unit->SquadId)
            Crew.Add(MakeShared<FJsonValueObject>(MemberState(Member)));
        Pending->SetArrayField(TEXT("allActualCrewMembersIncludingCasualties"),Crew);
        FString Text;FJsonSerializer::Serialize(Pending.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_BLOCKED_FIRE_PROBE %s"),*Text);return Pending;
    }return nullptr;
}
}
#endif
