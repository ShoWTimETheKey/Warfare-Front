#include "WarfareFrontlineArmourV63.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFrontline.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUArmourV82.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineCampaignV32.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "WarfareFrontlineDebugConsoleV37.h"
#include "WarfareFrontlineInfantryV45.h"
#include "WarfareFrontlineMobilityV41.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineMoraleV36.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"

namespace
{
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,TWeakObjectPtr<AFrontlineTank>> PioneerEscortsV63;
TSet<TWeakObjectPtr<AFrontlineTank>> EscortAllocatedV8445;
struct FPioneerEgressV8445 {TWeakObjectPtr<AFrontlineTank> Wreck;FVector Destination=FVector::ZeroVector;};
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FPioneerEgressV8445> PioneerEgressV8445;
struct FExitDiagnosticV8445
{
 TMap<FName,int32> Rejected;TArray<TSharedPtr<FJsonValue>> Samples;TSet<FName> Sampled;TSet<FString> AnalyticGroundComponents;
 int32 Candidates=0,GroundAccepted=0,PathAccepted=0;
 void Reject(FName Reason,const FVector& P,const AActor* Actor=nullptr,const UPrimitiveComponent* Component=nullptr,const FHitResult* Hit=nullptr)
 {
  ++Rejected.FindOrAdd(Reason);
  if(Samples.Num()>=6||Sampled.Contains(Reason))return;Sampled.Add(Reason);
  auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("reason"),Reason.ToString());J->SetStringField(TEXT("feet"),P.ToString());
  if(Actor){J->SetStringField(TEXT("actor"),Actor->GetPathName());J->SetStringField(TEXT("actor_class"),Actor->GetClass()->GetPathName());}
  if(Component){J->SetStringField(TEXT("component"),Component->GetPathName());J->SetStringField(TEXT("component_class"),Component->GetClass()->GetPathName());}
  if(Hit){J->SetBoolField(TEXT("start_penetrating"),Hit->bStartPenetrating);J->SetNumberField(TEXT("penetration_depth_cm"),Hit->PenetrationDepth);J->SetStringField(TEXT("impact_point"),Hit->ImpactPoint.ToString());J->SetStringField(TEXT("impact_normal"),Hit->ImpactNormal.ToString());}
  Samples.Add(MakeShared<FJsonValueObject>(J));
 }
 TSharedRef<FJsonObject> Json() const
 {
  auto J=MakeShared<FJsonObject>(),R=MakeShared<FJsonObject>();for(const auto& Pair:Rejected)R->SetNumberField(Pair.Key.ToString(),Pair.Value);
  J->SetNumberField(TEXT("candidate_count"),Candidates);J->SetNumberField(TEXT("ground_clear_count"),GroundAccepted);J->SetNumberField(TEXT("path_clear_count"),PathAccepted);
  TArray<TSharedPtr<FJsonValue>> Ground;for(const auto& Path:AnalyticGroundComponents)Ground.Add(MakeShared<FJsonValueString>(Path));
  J->SetObjectField(TEXT("rejections"),R);J->SetArrayField(TEXT("first_rejection_samples"),Samples);J->SetArrayField(TEXT("analytic_ground_components"),Ground);return J;
 }
};
// Populated only inside the exact disposable fixture below. Normal production
// release keeps no diagnostic rows or per-frame collision history.
TMap<TWeakObjectPtr<AWarfareFrontlineSoldier>,FExitDiagnosticV8445> ExitDiagnosticsV8445;
void IgnoreAnalyticGroundContactV8445(AWarfareFrontlineGameMode* B,FCollisionQueryParams& Query,FExitDiagnosticV8445* D=nullptr)
{
 // Infantry already plants on the authored SurfaceHeight and validates each
 // trench leg. Sweeping its upright capsule against the same sloping ground
 // a second time blocked descending feet at the terrain shell (normal Z>.92).
 // Exclude precisely that component, retaining this actor's real bank/bridge
 // children and every other world structure in the collision query.
 for(TActorIterator<AWarfareFrontlineGround> It(B->GetWorld());It;++It)
  if(const auto* Terrain=Cast<UPrimitiveComponent>(It->GetRootComponent());Terrain&&Terrain->GetFName()==TEXT("Terrain"))
  {Query.AddIgnoredComponent(Terrain);if(D)D->AnalyticGroundComponents.Add(Terrain->GetPathName());}
}
// Only a living fitter displaced into the wreck needs an exit search. An
// already clear ground escort retains his real position and entrance state.
bool EscortGroundClearV8445(AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U,const AFrontlineTank* Wreck,FVector& P,FExitDiagnosticV8445* D=nullptr)
{
 if(!B||!U||!Wreck||P.ContainsNaN()){if(D)D->Reject(TEXT("invalid_input"),P);return false;}
 const float EntryAllowance=U->bFieldEntered?0.f:FrontlinePacingV24::IngressSpeedCm*FrontlinePacingV24::IngressTravelSeconds+350.f;
 if(FMath::Abs(P.X)>=FrontlinePacingV24::CombatHalfLength()+EntryAllowance
  ||FMath::Abs(P.Y)>=FrontlineBattlefield::HalfFrontage-50.f){if(D)D->Reject(TEXT("outside_field"),P);return false;}
 P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);if(P.ContainsNaN()){if(D)D->Reject(TEXT("invalid_ground_height"),P);return false;}
 const float Radius=U->Capsule?U->Capsule->GetScaledCapsuleRadius():25.f;
 // Wrecks no longer participate in operational hull tests, but still have
 // a visible body. Check the standing volume against that actual component.
 for(float Height:{25.f,72.5f,140.f})for(int32 I=0;I<9;++I)
 {
  const float Angle=(I-1)*PI*.25f;
  const FVector Shoulder=I?FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,Height):FVector(0,0,Height);
  if(Wreck->ContainsHullPointV75(P+Shoulder,12.f)){if(D)D->Reject(TEXT("inside_wreck_volume"),P,Wreck);return false;}
 }
 for(const AWarfareFrontlineSoldier* Other:B->Units)
  if(IsValid(Other)&&Other!=U&&Other->IsAlive()&&!Other->IsHidden()
   &&FMath::Abs(Other->Feet().Z-P.Z)<145.f&&FVector::DistSquared2D(Other->Feet(),P)<FMath::Square(80.f)){if(D)D->Reject(TEXT("living_body_at_exit"),P,Other);return false;}
 if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const AFrontlineTank* Other:Fleet->GetVehicles())
  if(IsValid(Other)&&Other!=Wreck&&Other->BlocksInfantryAtV8218(P,Other->GetActorLocation(),Other->GetActorRotation().Yaw,Radius+9.f)){if(D)D->Reject(TEXT("operational_hull_at_exit"),P,Other);return false;}
 FCollisionQueryParams Query(SCENE_QUERY_STAT(FrontlineEscortExitV8445),false);Query.AddIgnoredActor(U);Query.AddIgnoredActor(Wreck);
 for(const AWarfareFrontlineSoldier* Other:B->Units)if(IsValid(Other))Query.AddIgnoredActor(Other);
 IgnoreAnalyticGroundContactV8445(B,Query,D);
 const float HalfHeight=U->Capsule?U->Capsule->GetScaledCapsuleHalfHeight():88.f;
 const bool Blocked=B->GetWorld()->OverlapBlockingTestByChannel(P+FVector(0,0,U->GroundContactOffsetV82+1.f),FQuat::Identity,ECC_Pawn,
  FCollisionShape::MakeCapsule(Radius,HalfHeight),Query);
 if(Blocked&&D)
 {
  TArray<FOverlapResult> Hits;B->GetWorld()->OverlapMultiByChannel(Hits,P+FVector(0,0,U->GroundContactOffsetV82+1.f),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,HalfHeight),Query);
  const FOverlapResult* Hit=Hits.FindByPredicate([](const auto& H){return H.bBlockingHit;});
  D->Reject(TEXT("static_capsule_at_exit"),P,Hit?Hit->GetActor():nullptr,Hit?Hit->GetComponent():nullptr);
 }
 return !Blocked;
}
bool EscortExitV8445(AWarfareFrontlineGameMode* B,const AWarfareFrontlineSoldier* U,const AFrontlineTank* Wreck,FVector& P)
{
 auto* D=ExitDiagnosticsV8445.Find(const_cast<AWarfareFrontlineSoldier*>(U));
 P=U->Feet();if(EscortGroundClearV8445(B,U,Wreck,P,D)){if(D)D->Reject(TEXT("already_clear_ground"),P);return true;}
 const FVector Local=Wreck->GetActorTransform().InverseTransformPosition(U->Feet());
 const float FirstSide=Local.Y<0?-1.f:Local.Y>0?1.f:U->MemberIndex%2?1.f:-1.f;
 for(float Side:{FirstSide,-FirstSide})for(float Shoulder:{90.f,135.f,180.f,225.f,270.f,315.f,365.f,455.f,545.f,635.f,725.f})
  for(float Offset:{0.f,-90.f,90.f,-180.f,180.f,-330.f,330.f})
  {
   FVector Candidate=Wreck->GetActorTransform().TransformPosition(FVector(Local.X+Offset,Side*Shoulder,0));
   if(D)++D->Candidates;
   if(FVector::DistSquared2D(Candidate,U->Feet())>FMath::Square(850.f)){if(D)D->Reject(TEXT("candidate_beyond_local_limit"),Candidate);continue;}
   if((Candidate.X-U->Feet().X)*B->GetTeamDirection(U->Team)<-.01f){if(D)D->Reject(TEXT("candidate_requires_backward_advance"),Candidate);continue;}
   if(!EscortGroundClearV8445(B,U,Wreck,Candidate,D))continue;if(D)++D->GroundAccepted;
   // The disabled wreck is the only ignored obstruction. An exit cannot jump
   // across a standing man, another operational hull or physical trench wall.
   FCollisionQueryParams Query(SCENE_QUERY_STAT(FrontlineEscortExitSweepV8445),false);Query.AddIgnoredActor(U);Query.AddIgnoredActor(Wreck);
   for(const AWarfareFrontlineSoldier* Other:B->Units)if(IsValid(Other))Query.AddIgnoredActor(Other);
   IgnoreAnalyticGroundContactV8445(B,Query,D);
   const float Radius=U->Capsule?U->Capsule->GetScaledCapsuleRadius():25.f;
   const float HalfHeight=U->Capsule?U->Capsule->GetScaledCapsuleHalfHeight():88.f;
   const int32 Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(U->Feet(),Candidate)/40.f));
   FVector From=U->Feet();bool Clear=true;
   for(int32 I=1;I<=Steps&&Clear;++I)
   {
    FVector To=FMath::Lerp(U->Feet(),Candidate,float(I)/Steps);To.Z=FrontlineInfantryV45::SurfaceHeight(B,To);
    if(!FrontlineInfantryV45::AllowsTrenchStep(U,From,To)){if(D)D->Reject(TEXT("trench_rejects_exit_leg"),To);Clear=false;break;}
    FHitResult Hit;
    if(B->GetWorld()->SweepSingleByChannel(Hit,From+FVector(0,0,U->GroundContactOffsetV82+1.f),To+FVector(0,0,U->GroundContactOffsetV82+1.f),
     FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,HalfHeight),Query))
    {if(D)D->Reject(TEXT("static_capsule_blocks_exit_leg"),To,Hit.GetActor(),Hit.GetComponent(),&Hit);Clear=false;break;}
    for(const AWarfareFrontlineSoldier* Other:B->Units)
     if(IsValid(Other)&&Other!=U&&Other->IsAlive()&&!Other->IsHidden()&&FMath::Abs(Other->Feet().Z-To.Z)<145.f
      &&FVector::DistSquared2D(Other->Feet(),To)<FMath::Square(80.f)){if(D)D->Reject(TEXT("living_body_blocks_exit_leg"),To,Other);Clear=false;break;}
    if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const AFrontlineTank* Other:Fleet->GetVehicles())
     if(IsValid(Other)&&Other!=Wreck&&Other->InfantrySweepClearanceV8218(From,To,Radius+9.f)<1.f){if(D)D->Reject(TEXT("operational_hull_blocks_exit_leg"),To,Other);Clear=false;break;}
    From=To;
   }
   if(Clear){P=Candidate;if(D)++D->PathAccepted;return true;}
  }
 return false;
}
}
struct FFrontlineArmourV63Access
{
 static void PersistenceChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
 static void AllocationChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
 static void ForcedRailChecksV842(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
 static TSharedRef<FJsonObject> EscortChecksV8445(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check);
 static void ReleaseOrdersV8445(AWarfareFrontlineGameMode* B,const TSet<int32>& Released)
 {
  for(int32 Id:Released)if(auto* S=B->FindSquad(Id);S&&S->LivingCount()>0)
  {
   // The same passage cursor is consumed by the ordinary next UpdateSquads.
   // No command counter, capture, research or campaign reward is awarded.
   S->Trench=INDEX_NONE;S->Destination=INDEX_NONE;S->bAdvancing=true;S->bHoldPosition=false;
   S->bReinforcement=S->Members.ContainsByPredicate([](const auto& W){const auto* U=W.Get();return U&&U->IsAlive()&&!U->bFieldEntered;});
  }
 }
 static bool Mutate(AWarfareFrontlineGameMode* B,TFunctionRef<void(FFrontlineCampaignProgress&)> Change)
 {
  if(!B||B->Phase==EFrontlinePhase::Fighting||B->bProgressionReadOnly)return false;
  auto* R=B->CampaignRuns.FindByPredicate([&](const FFrontlineCampaignProgress& V){return V.Id==B->ActiveCampaignId;});if(!R)return false;
  const auto Before=*R;Change(*R);R->UpdatedUtc=FDateTime::UtcNow().ToIso8601();
  if(B->SaveProgression())return true;*R=Before;B->LastMessage=B->ProgressionSaveStatus;return false;
 }
 static void Escort(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank)
 {
  if(!B||!IsValid(Tank)||!Tank->IsOperational())return;
  // Recruitment invokes this once; repeated setup must not manufacture a
  // second escort section for the same hull.
  if(EscortAllocatedV8445.Contains(Tank))return;
  if(!B->SpawnSquad(Tank->Team,EFrontlineRole::Pioneer))return;
  EscortAllocatedV8445.Add(Tank);
  auto& S=B->Squads.Last();
  while(S.Members.Num()>2){auto W=S.Members.Pop();if(auto* U=W.Get()){B->Units.Remove(U);U->Destroy();}}
  for(int32 I=0;I<S.Members.Num();++I)if(auto* U=S.Members[I].Get())
  {
   PioneerEscortsV63.Add(U,Tank);
   FVector P=Tank->GetActorTransform().TransformPosition(FVector(-330-I*70,(I?1.f:-1.f)*365.f,0));
   P.Z=FrontlineInfantryV45::SurfaceHeight(B,P)+88;U->SetActorLocation(P);
  }
 }
 static bool ExtraSection(AWarfareFrontlineGameMode* B){return B&&B->SpawnSquad(0,EFrontlineRole::Pioneer);}
};
namespace FrontlineArmourV63
{
FString L(const TCHAR* E,const TCHAR* C){return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?C:E;}
static bool German(const AWarfareFrontlineGameMode* B,int32 T){return B&&B->GetFactionForTeam(T)==EFrontlineFaction::German;}
struct FAuthoredResearchV8218
{
 int32 Id,Branch,Tier,Prerequisite,Cost;
 const TCHAR *UKTitleEN,*UKTitleZH,*UKDescriptionEN,*UKDescriptionZH,*DETitleEN,*DETitleZH,*DEDescriptionEN,*DEDescriptionZH;
};
static const FAuthoredResearchV8218 AuthoredResearchV8218[]={
#define WF_ARMOUR_NODE(...) {__VA_ARGS__},
#include "WarfareFrontlineArmourCatalogueV8218.generated.inl"
#undef WF_ARMOUR_NODE
};
TArray<FResearch> Research(const AWarfareFrontlineGameMode* B)
{
 const bool DE=German(B,0);TArray<FResearch> Result;Result.Reserve(ResearchCountV8218);
 for(const auto& A:AuthoredResearchV8218)
 {
  FResearch N;N.Id=A.Id;N.Prerequisite=A.Prerequisite;N.Cost=A.Cost;N.Branch=A.Branch;N.Tier=A.Tier;
  N.TitleEN=DE?A.DETitleEN:A.UKTitleEN;N.TitleZH=DE?A.DETitleZH:A.UKTitleZH;
  N.Title=L(*N.TitleEN,*N.TitleZH);N.Description=DE?L(A.DEDescriptionEN,A.DEDescriptionZH):L(A.UKDescriptionEN,A.UKDescriptionZH);Result.Add(MoveTemp(N));
 }
 return Result;
}
bool Has(const AWarfareFrontlineGameMode* B,int32 Id,int32 Team)
{
 if(!B||Id<0||Id>=ResearchCountV8218||Team<0||Team>1)return false;
 // TacticalV2 is a separate research contract. Reserved national operation
 // IDs are not late British/German doctrine thresholds, on either side.
 if(WarfareFRRUCampaignV82::Active(B))return false;
 if(Team==1&&FrontlineUltimateArchiveV8215::FullArmory(B,Team))return true;
 if(Team==0){const auto* R=B->GetCurrentCampaign();return R&&(R->ArmourResearchMaskV63&(1u<<Id));}
 // Opposing formations receive their own dated doctrine, independent of player purchases.
 const auto* M=B->GetEncounterCampaignMission();if(!M)return false;
 static const int32 Threshold[]={8,10,11,13,12,14,15,16,18,12,14,15,17,18,17,19};return M->Id>=Threshold[Id];
}
FString ResearchBlock(const AWarfareFrontlineGameMode* B,int32 Id)
{
 if(!B)return L(TEXT("No campaign selected."),TEXT("未选择战役。"));const auto N=Research(B);
 if(!N.IsValidIndex(Id))return L(TEXT("Unknown research."),TEXT("此项研究不存在。"));
 if(Has(B,Id))return L(TEXT("Research completed."),TEXT("研究完成。"));
 if(B->Phase==EFrontlinePhase::Fighting)return L(TEXT("Return to operations to research."),TEXT("回作战室安排研究。"));
 if(N[Id].Prerequisite>=0&&!Has(B,N[Id].Prerequisite))return L(TEXT("Requires: "),TEXT("前置："))+N[N[Id].Prerequisite].Title;
 if(B->GetResearchPoints()<N[Id].Cost)return L(TEXT("Insufficient research points."),TEXT("研究点不足。"));return FString();
}
bool BuyResearch(AWarfareFrontlineGameMode* B,int32 Id)
{if(!ResearchBlock(B,Id).IsEmpty())return false;const int32 Cost=Research(B)[Id].Cost;return FFrontlineArmourV63Access::Mutate(B,[&](FFrontlineCampaignProgress& R){if(!FrontlineDebugV37::UnlimitedResearch(B))R.Research.ResearchPoints-=Cost;R.ArmourResearchMaskV63|=1u<<Id;});}
EFrontlineVehicle PlatformV8218(EFrontlineVehicle T)
{
 switch(T)
 {
 case EFrontlineVehicle::MarkIVFemale:case EFrontlineVehicle::MarkIVPioneer:return EFrontlineVehicle::MarkIVMale;
 case EFrontlineVehicle::BeuteMarkIVFemale:case EFrontlineVehicle::BeutePioneer:return EFrontlineVehicle::BeuteMarkIV;
 case EFrontlineVehicle::A7VGretchen:return EFrontlineVehicle::A7V;
 default:return T;
 }
}
EFrontlineVehicle ConfigurationV8218(const AWarfareFrontlineGameMode* B,EFrontlineVehicle Base,int32 Team)
{
 if(!B)return Base;Base=PlatformV8218(Base);auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!V)return Base;
 EFrontlineVehicle Alternative=Base;
 if(Base==EFrontlineVehicle::MarkIVMale)Alternative=EFrontlineVehicle::MarkIVFemale;
 if(Base==EFrontlineVehicle::BeuteMarkIV)Alternative=EFrontlineVehicle::BeuteMarkIVFemale;
 if(Base==EFrontlineVehicle::A7V)Alternative=EFrontlineVehicle::A7VGretchen;
 if(Alternative==Base||!V->IsUnlocked(Alternative,Team))return Base;
 if(Team==0){const auto* R=B->GetCurrentCampaign();return R&&((R->ArmourConfigurationMaskV836&(1u<<int32(Base)))||(R->ArmourSelectionMaskV63&(1u<<int32(Alternative))))?Alternative:Base;}
 const auto* M=B->GetEncounterCampaignMission();return M&&(M->Id%4)==3?Alternative:Base;
}
bool ForcedAllocationV842(const AWarfareFrontlineGameMode* B,EFrontlineVehicle T,int32 Team)
{
 if(FrontlineUltimateArchiveV8215::FullArmory(B,Team))return false;
 if(!B||T!=EFrontlineVehicle::RailConvoy||Team<0||Team>1)return false;
 const auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 return V&&V->RailAvailableV75(Team);
}
uint32 GetSelectedMask(const AWarfareFrontlineGameMode* B,uint32 Eligible)
{
 const auto* R=B?B->GetCurrentCampaign():nullptr;uint32 Choice=R?R->ArmourSelectionMaskV63&Eligible:0;
 if(!Choice&&(!R||!R->bArmourSelectionExplicitV84413))Choice=Eligible;uint32 Result=0,Platforms=0;int32 Count=0;
 // Mission attachments precede the player's permanent preferences. Do not
 // write the temporary train into the campaign selection or charge research.
 const uint32 Rail=1u<<int32(EFrontlineVehicle::RailConvoy);
 if((Eligible&Rail)&&ForcedAllocationV842(B,EFrontlineVehicle::RailConvoy))
 {Result=Rail;Platforms=Rail;Count=1;Choice&=~Rail;if(!Choice&&(!R||!R->bArmourSelectionExplicitV84413))Choice=Eligible&~Rail;}
 for(int32 I=0;I<int32(EFrontlineVehicle::Count)&&Count<2;++I)if(Choice&(1u<<I))
 {
  const uint32 Platform=1u<<int32(PlatformV8218(EFrontlineVehicle(I)));
  if(Platforms&Platform)continue;Platforms|=Platform;Result|=1u<<I;++Count;
 }
 return Result;
}
bool IsSelected(const AWarfareFrontlineGameMode* B,EFrontlineVehicle T)
{if(!B)return false;auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!V)return false;uint32 E=0;for(auto& D:V->GetRoster())if(V->IsUnlocked(D.Type))E|=1u<<int32(D.Type);return (GetSelectedMask(B,E)&(1u<<int32(T)))!=0;}
bool ToggleSelection(AWarfareFrontlineGameMode* B,EFrontlineVehicle T)
{
 if(!B||uint8(T)>=uint8(EFrontlineVehicle::Count))return false;auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!V||!V->IsUnlocked(T))return false;
 if(ForcedAllocationV842(B,T))return false;
 uint32 E=0;for(const auto& D:V->GetRoster())if(V->IsUnlocked(D.Type))E|=1u<<int32(D.Type);
 uint32 S=GetSelectedMask(B,E),Bit=1u<<int32(T);
 const uint32 Forced=ForcedAllocationV842(B,EFrontlineVehicle::RailConvoy)?(1u<<int32(EFrontlineVehicle::RailConvoy)):0;
 if(S&Bit)S&=~Bit;
 else
 {
  const auto Base=PlatformV8218(T);bool Replacing=false;
  for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)if((S&(1u<<I))&&PlatformV8218(EFrontlineVehicle(I))==Base){S&=~(1u<<I);Replacing=true;}
  if(!Replacing&&FrontlineCampaignV32::BitCount(S)>=2)
  {if(!Forced)return false;S=Forced;}S|=Bit;
 }
 return FFrontlineArmourV63Access::Mutate(B,[&](FFrontlineCampaignProgress& R)
 {
  R.ArmourSelectionMaskV63=S&~Forced;
  R.bArmourSelectionExplicitV84413=true;
  const auto Base=PlatformV8218(T);const uint32 PlatformBit=1u<<int32(Base);
  if(T==EFrontlineVehicle::MarkIVFemale||T==EFrontlineVehicle::BeuteMarkIVFemale||T==EFrontlineVehicle::A7VGretchen)R.ArmourConfigurationMaskV836|=PlatformBit;
  else if(T==EFrontlineVehicle::MarkIVMale||T==EFrontlineVehicle::BeuteMarkIV||T==EFrontlineVehicle::A7V)R.ArmourConfigurationMaskV836&=~PlatformBit;
 });
}
bool ToggleConfigurationV8218(AWarfareFrontlineGameMode* B,EFrontlineVehicle T)
{
 if(!B||B->Phase!=EFrontlinePhase::Briefing)return false;const auto Base=PlatformV8218(T),Current=ConfigurationV8218(B,Base);
 EFrontlineVehicle Alternative=Base;
 if(Base==EFrontlineVehicle::MarkIVMale)Alternative=EFrontlineVehicle::MarkIVFemale;
 else if(Base==EFrontlineVehicle::BeuteMarkIV)Alternative=EFrontlineVehicle::BeuteMarkIVFemale;
 else if(Base==EFrontlineVehicle::A7V)Alternative=EFrontlineVehicle::A7VGretchen;else return false;
 const auto Desired=Current==Base?Alternative:Base;auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 if(!V||!V->IsUnlocked(Desired))return false;
 const auto* R=B->GetCurrentCampaign();if(!R)return false;
 const uint32 BaseBit=1u<<int32(Base),AlternateBit=1u<<int32(Alternative);
 uint32 Eligible=0;for(const auto& D:V->GetRoster())if(V->IsUnlocked(D.Type))Eligible|=1u<<int32(D.Type);
 const uint32 Selected=GetSelectedMask(B,Eligible);const bool Equipped=(Selected&(BaseBit|AlternateBit))!=0;
 return FFrontlineArmourV63Access::Mutate(B,[&](FFrontlineCampaignProgress& C)
 {
  if(Desired==Alternative)C.ArmourConfigurationMaskV836|=BaseBit;else C.ArmourConfigurationMaskV836&=~BaseBit;
  if(Equipped){C.ArmourSelectionMaskV63=(Selected&~(BaseBit|AlternateBit))|(1u<<int32(Desired));C.bArmourSelectionExplicitV84413=true;}
 });
}
const TCHAR* VehicleDescriptionV8218(EFrontlineVehicle Type)
{
 const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 switch(Type)
 {
 case EFrontlineVehicle::RailConvoy:return Chinese?
  TEXT("铁路分遣队的炮塔沿轨道掩护推进，车厢载着步枪班与工兵。铁轨规定了它的来路，也规定了它的退路。\n本次行动临时配属；占用一个装甲栏位，共用装甲整备队列。抵达前沿后卸载两支小队，列车被毁将重挫士气。"):
  TEXT("Turrets cover the advance along the rails; the wagons carry riflemen and pioneers. The same track that brings the train forward is its only way back.\nTemporary allocation for this operation. Uses one armour slot and the shared armour readiness queue. Unloads two sections at the forward railhead; its destruction inflicts a severe morale loss.");
#define WF_ARMOUR_VEHICLE(Key,English,ChineseText) case Key:return Chinese?ChineseText:English;
#include "WarfareFrontlineArmourVehiclesV8218.generated.inl"
#undef WF_ARMOUR_VEHICLE
 default:return TEXT("");
 }
}
float ReadinessScale(const AWarfareFrontlineGameMode* B,EFrontlineVehicle T,int32 Team)
{const float Workshop=!Has(B,0,Team)?1.f:!German(B,Team)?.90f:(T==EFrontlineVehicle::A7V||T==EFrontlineVehicle::A7VGretchen)?.95f:.85f;return Workshop*(Has(B,8,Team)?.90f:1.f);}
float TrackScale(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,1,T)?(German(B,T)?1.18f:1.25f):1.f;}
float GunIntegrityScale(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,2,T)?(German(B,T)?1.35f:1.2f):1.f;}
float CannonCycleScale(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,3,T)?(German(B,T)?.92f:.88f):1.f;}
float HullScaleV8218(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,9,T)?(German(B,T)?1.22f:1.18f):1.f;}
float TravelSpeedScaleV8218(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,10,T)?(German(B,T)?1.10f:1.12f):1.f;}
float MachinegunCycleScaleV8218(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,11,T)?(German(B,T)?.75f:.80f):1.f;}
float ArmourShellDamageScaleV8218(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,12,T)?(German(B,T)?1.30f:1.25f):1.f;}
float InfantryShellRadiusScaleV8218(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,13,T)?(German(B,T)?1.20f:1.25f):1.f;}
float CloseGunAccuracyScaleV8218(const AWarfareFrontlineGameMode* B,float D,int32 T){return Has(B,14,T)&&D<=1200.f?1.30f:1.f;}
float RepairRestoredFractionV8218(const AWarfareFrontlineGameMode* B,int32 T){return Has(B,15,T)?.75f:.50f;}

FString RequisitionTitle(int32 I){return I==0?L(TEXT("ADVANCE PIONEERS"),TEXT("先遣工兵")):I==1?L(TEXT("SMOKE RESERVE"),TEXT("烟幕储备")):L(TEXT("ARMOURED RESERVE"),TEXT("装甲待命"));}
FString RequisitionDescription(int32 I)
{return I==0?L(TEXT("An extra pioneer section arrives at the start without using the infantry readiness queue."),TEXT("开战后，额外一支工兵抵达，不占用步兵整备队列。")):I==1?L(TEXT("After the first smoke call, the reserve becomes ready immediately for one more call."),TEXT("首次呼叫烟幕后，储备烟幕立即就绪，可再呼叫一次。")):L(TEXT("Selected armour is ready at the start. Later dispatches begin a new workshop cycle; each model keeps its own preparation time."),TEXT("选定装甲在开战时即可出动。后续出动会重新开始整备，各型装甲按自身所需时间就绪。"));}
int32 Requisition(const AWarfareFrontlineGameMode* B)
{if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::RequisitionV87(B);const auto* R=B?B->GetCurrentCampaign():nullptr;const auto* M=FrontlineCampaignV32::CurrentOperation(B);if(!R||!M||M->Id<0||M->Id>=20)return -1;const uint32 Bit=1u<<M->Id;return R->PioneerOrdersMaskV63&Bit?0:R->SmokeOrdersMaskV63&Bit?1:R->ArmourOrdersMaskV63&Bit?2:-1;}
bool CanRequisition(const AWarfareFrontlineGameMode* B,int32 Id)
{
 if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::CanRequisitionV87(B,Id);
 const auto* M=B?B->GetSelectedCampaignMission():nullptr;
 const bool Archive=FrontlineUltimateArchiveV8215::FullArmory(B,0);
 if(!M||M->Id<(Archive?0:8)||M->Id>=20||Id<0||Id>2||B->Phase!=EFrontlinePhase::Briefing)return false;
 const int32 Current=Requisition(B);if(Current==Id)return true;
 if(!Archive&&(Current!=-1||(!FrontlineDebugV37::UnlimitedResearch(B)&&B->GetResearchPoints()<3)))return false;
 if(Id==1&&(!B->IsSupportUnlocked(EFrontlineSupport::Smoke)
  ||!FrontlineCampaignV32::SupportBlockReason(B,EFrontlineSupport::Smoke,0).IsEmpty()
  ||!FrontlineCampaignV32::SupportSelected(B,EFrontlineSupport::Smoke,0)))return false;
 if(Id==2){auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld());if((!Archive&&!M->bArmourAllowedV41)||!V||V->GetEquippedRoster().IsEmpty())return false;}
 return true;
}
bool BuyRequisition(AWarfareFrontlineGameMode* B,int32 Id)
{
 if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUCampaignV82::BuyRequisitionV87(B,Id);
 if(!CanRequisition(B,Id))return false;const auto* M=B->GetSelectedCampaignMission();
 const uint32 Bit=1u<<M->Id;const bool Archive=FrontlineUltimateArchiveV8215::FullArmory(B,0),Cancel=Requisition(B)==Id,Free=Archive||FrontlineDebugV37::UnlimitedResearch(B);
 return FFrontlineArmourV63Access::Mutate(B,[&](FFrontlineCampaignProgress& R)
 {
  if(Archive)
  {
   R.PioneerOrdersMaskV63&=~Bit;R.SmokeOrdersMaskV63&=~Bit;R.ArmourOrdersMaskV63&=~Bit;R.RequisitionPaidMaskV836&=~Bit;
   if(!Cancel)(Id==0?R.PioneerOrdersMaskV63:Id==1?R.SmokeOrdersMaskV63:R.ArmourOrdersMaskV63)|=Bit;
   return;
  }
  uint32& Order=Id==0?R.PioneerOrdersMaskV63:Id==1?R.SmokeOrdersMaskV63:R.ArmourOrdersMaskV63;
  if(Cancel){Order&=~Bit;if(R.RequisitionPaidMaskV836&Bit)R.Research.ResearchPoints+=3;R.RequisitionPaidMaskV836&=~Bit;}
  else{Order|=Bit;if(!Free){R.Research.ResearchPoints-=3;R.RequisitionPaidMaskV836|=Bit;}else R.RequisitionPaidMaskV836&=~Bit;}
 });
}
struct FOrders {FGuid Encounter;bool Delivered=false,SmokeUsed=false;float SmokeReady=0;};
static TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FOrders> Orders;
void Tick(AWarfareFrontlineGameMode* B)
{
 if(!B||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE||UGameplayStatics::IsGamePaused(B))return;
 for(auto It=Orders.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 TSet<AFrontlineTank*> LostCarriers;
 for(auto It=PioneerEscortsV63.CreateIterator();It;++It)
 {
  if(!It.Key().IsValid()||!It.Value().IsValid()){It.RemoveCurrent();continue;}
  if(It.Key()->Battle==B&&!It.Value()->IsOperational())LostCarriers.Add(It.Value().Get());
 }
 for(AFrontlineTank* Tank:LostCarriers)ReleasePioneerEscortV8445(B,Tank);
 for(auto It=PioneerEgressV8445.CreateIterator();It;++It)if(!It.Key().IsValid()||!It.Key()->IsAlive()||!It.Value().Wreck.IsValid())It.RemoveCurrent();
 for(auto It=EscortAllocatedV8445.CreateIterator();It;++It)if(!It->IsValid())It.RemoveCurrent();
 const bool National=WarfareFRRUCampaignV82::Active(B);int32 FrozenKind=INDEX_NONE;float FrozenSmoke=0;
 if(National&&!WarfareFRRUCampaignV82::FrozenRequisitionV87(B,FrozenKind,FrozenSmoke))return;
 auto& S=Orders.FindOrAdd(B);if(S.Encounter!=B->GetEncounterId()){S={};S.Encounter=B->GetEncounterId();S.SmokeReady=National?FrozenSmoke:B->SmokeReadyAt[0];}
 const int32 Kind=National?FrozenKind:Requisition(B);
 if(!S.Delivered&&B->Elapsed>.15f)
 {
  if(National)
  {
   // A rejected national spawn owns no section. Keep the entitlement until
   // the ordinary prepared/grounded SpawnSquad transaction really succeeds.
   if(Kind==0)S.Delivered=FFrontlineArmourV63Access::ExtraSection(B);
   else {S.Delivered=true;if(Kind==2)WarfareFRRUArmourV82::ReleaseOpeningReadinessV87(B,0);}
  }
  else {S.Delivered=true;if(Kind==0)FFrontlineArmourV63Access::ExtraSection(B);if(Kind==2)if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))V->ResetReadinessForDebug(0);}
 }
 if(Kind==1&&!S.SmokeUsed&&B->SmokeReadyAt[0]>S.SmokeReady+.1f){S.SmokeUsed=true;B->SmokeReadyAt[0]=B->Elapsed;B->LastMessage=L(TEXT("Reserve smoke is ready."),TEXT("备用烟幕已备妥。"));}
}
void SpawnPioneerEscort(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank){FFrontlineArmourV63Access::Escort(B,Tank);}
int32 ReleasePioneerEscortV8445(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank)
{
 if(!B||!IsValid(Tank)||Tank->IsOperational())return 0;
 TArray<AWarfareFrontlineSoldier*> Living;TSet<int32> Squads;
 for(auto It=PioneerEscortsV63.CreateIterator();It;++It)
  if(It.Value().Get()==Tank)
  {
   if(auto* U=It.Key().Get();U&&U->Battle!=B)continue;
   if(auto* U=It.Key().Get();U&&U->Battle==B&&U->Team==Tank->Team&&U->Role==EFrontlineRole::Pioneer&&U->IsAlive())
   {Living.Add(U);Squads.Add(U->SquadId);}
   It.RemoveCurrent();
  }
 if(Living.IsEmpty())return 0;
 FFrontlineArmourV63Access::ReleaseOrdersV8445(B,Squads);
 int32 EgressOrdered=0,ExitBlocked=0;
 for(auto* U:Living)
 {
  FVector Exit;
  if(EscortExitV8445(B,U,Tank,Exit))
  {
   if(!U->Feet().Equals(Exit,.1f)){PioneerEgressV8445.Add(U,{Tank,Exit});++EgressOrdered;}
  }
  else ++ExitBlocked; // Keep his real ground position; never force an occupied exit.
  // Entry immunity ends only at the same physical entrance line as infantry.
  U->bFieldEntered=U->bFieldEntered||FrontlinePacingV24::HasCrossedEntry(U->Feet().X,B->GetTeamDirection(U->Team));
 }
 FrontlineMobilityV41::InvalidateRoster(B);
 UE_LOG(LogTemp,Display,TEXT("WF_V8445_ESCORT_RELEASE type=%d team=%d serial=%d living=%d egress_orders=%d blocked_exits=%d displacement_cm=0"),
  int32(Tank->Type),Tank->Team,Tank->Serial,Living.Num(),EgressOrdered,ExitBlocked);
 return Living.Num();
}
bool PioneerEgressDestinationV8445(const AWarfareFrontlineSoldier* U,FVector& Destination)
{
 if(!U)return false;
 const auto Key=const_cast<AWarfareFrontlineSoldier*>(U);auto* Exit=PioneerEgressV8445.Find(Key);if(!Exit)return false;
 auto* Wreck=Exit->Wreck.Get();FVector Current=U->Feet();
 if(!U->IsAlive()||!U->Battle||!IsValid(Wreck)||U->IsHidden()
  ||EscortGroundClearV8445(U->Battle,U,Wreck,Current)||FVector::DistSquared2D(U->Feet(),Exit->Destination)<=FMath::Square(10.f))
 {PioneerEgressV8445.Remove(Key);return false;}
 Destination=Exit->Destination;return true;
}
AFrontlineTank* EscortFor(const AWarfareFrontlineSoldier* U)
{
 if(!U||!U->IsActiveCombatant()||U->Role!=EFrontlineRole::Pioneer)return nullptr;
 const auto* Found=PioneerEscortsV63.Find(const_cast<AWarfareFrontlineSoldier*>(U));
 auto* Tank=Found?Found->Get():nullptr;
 return IsValid(Tank)&&Tank->IsOperational()&&Tank->Team==U->Team?Tank:nullptr;
}
bool EscortDestination(const AWarfareFrontlineSoldier* U,FVector& Destination)
{
 auto* Tank=EscortFor(U);if(!Tank||!U->Battle)return false;
 // Flank the hull clear of tracks and sponsons; a turn moves these real slots.
 Destination=Tank->GetActorTransform().TransformPosition(FVector(-150-U->MemberIndex*70,(U->MemberIndex%2?1.f:-1.f)*365.f,0));
 Destination.Z=FrontlineInfantryV45::SurfaceHeight(U->Battle,Destination);return true;
}
bool SquadIsEscorting(const AWarfareFrontlineGameMode* B,int32 Id)
{
 const auto* S=B?B->FindSquad(Id):nullptr;if(!S)return false;
 for(const auto& W:S->Members)if(EscortFor(W.Get()))return true;return false;
}
bool ReviewRequestedV8218(const AWarfareFrontlineGameMode* B)
{
 if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
 FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)||FPaths::IsRelative(Dir)||Dir.Contains(TEXT("..")))return false;
 Dir=FPaths::ConvertRelativePathToFull(Dir);FPaths::NormalizeDirectoryName(Dir);
 const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/Gameplay/V8218/ArchiveReview/Run/"));
 const FString Stamp=Dir.StartsWith(Root,ESearchCase::IgnoreCase)?Dir.Mid(Root.Len()):FString();
 return Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')&&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
}
void RunChecksV8218(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 const auto Nodes=Research(B);TSet<int32> Slots;bool Valid=Nodes.Num()==ResearchCountV8218;
 static const int32 OldIdMeaning[]={-1,-1,-1,2,0,10,-1,6,4};
 for(const auto& N:Nodes)
 {
  Valid&=N.Id>=0&&N.Id<16&&N.Branch>=0&&N.Branch<4&&N.Tier>=0&&N.Tier<4&&!Slots.Contains(N.Branch+N.Tier*4);
  Slots.Add(N.Branch+N.Tier*4);Valid&=!N.TitleEN.IsEmpty()&&!N.TitleZH.IsEmpty()&&!N.Description.IsEmpty();
  if(N.Prerequisite>=0)Valid&=Nodes.IsValidIndex(N.Prerequisite)&&Nodes[N.Prerequisite].Branch==N.Branch&&Nodes[N.Prerequisite].Tier==N.Tier-1;
 }
 Check(TEXT("v8218_armour_sixteen_bilingual_nodes_four_complete_branches"),Valid&&Slots.Num()==16);
 bool OldIds=true;for(int32 I=0;I<9;++I)OldIds&=Nodes.IsValidIndex(I)&&Nodes[I].Prerequisite==OldIdMeaning[I];
 Check(TEXT("v8218_armour_legacy_bit_ids_retain_effects_with_new_branch_prerequisites"),OldIds);
 Check(TEXT("v8218_variant_and_escort_enum_ids_remain_backward_compatible"),int32(EFrontlineVehicle::MarkIVMale)==0&&int32(EFrontlineVehicle::Whippet)==1&&int32(EFrontlineVehicle::A7V)==2&&int32(EFrontlineVehicle::BeuteMarkIV)==3&&int32(EFrontlineVehicle::BeutePioneer)==9&&int32(EFrontlineVehicle::MarkVStar)==10&&int32(EFrontlineVehicle::Austin1918)==11);
 const bool Isolated=ReviewRequestedV8218(B);Check(TEXT("v8218_armour_actual_fixture_requires_exact_volatile_no_save_root"),Isolated);if(!Isolated)return;
 if(auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))Fleet->RunPlatformChecksV8218(Check);else Check(TEXT("v8218_armour_actual_subsystem_present"),false);
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 Check(TEXT("v8218_armour_tree_sixteen_distinct_nodes"),Research(B).Num()==ResearchCountV8218);
 const auto NewNode=Research(B)[8];Check(TEXT("v8215_forward_service_balanced_late_prerequisite"),NewNode.Id==8&&NewNode.Prerequisite==4&&NewNode.Cost==3);
 const uint32 E=(1u<<int32(EFrontlineVehicle::Count))-1;Check(TEXT("v63_armour_selection_never_more_than_two"),FrontlineCampaignV32::BitCount(GetSelectedMask(B,E))<=2);
 auto* V=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;Check(TEXT("v8218_army_catalog_counts_platforms_not_escort_variants"),V&&V->GetRoster().Num()==(German(B,0)?2:5));
 Check(TEXT("v63_equipped_armour_at_most_two"),V&&V->GetEquippedRoster().Num()<=2);
}
void RunAllocationChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 const bool Safe=FrontlineEffectsReviewSafetyV8216::Allowed(B)&&FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"))
  &&!WarfareFRRUCampaignV82::Active(B)&&B->GetCurrentCampaign();
 Check(TEXT("v836_allocation_actual_checks_require_exact_disposable_uk_de_profile"),Safe);
 if(Safe){FFrontlineArmourV63Access::PersistenceChecksV836(B,Check);FFrontlineArmourV63Access::AllocationChecksV836(B,Check);}
}
}
void FFrontlineArmourV63Access::PersistenceChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 // Explicit persistence fixture only. Bind a new GUID file beneath the exact
 // disposable UserDir before temporarily enabling the ordinary serializer.
 FString Dir;const bool ExactRoot=FrontlineEffectsReviewSafetyV8216::Allowed(B)
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV836Review"))
  &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!FPaths::IsRelative(Dir);
 Check(TEXT("v836_rail_persistence_requires_exact_disposable_profile_root"),ExactRoot);if(!ExactRoot)return;
 FPaths::NormalizeDirectoryName(Dir);FPaths::CollapseRelativeDirectories(Dir);
 FString File=FPaths::ConvertRelativePathToFull(FPaths::Combine(Dir,TEXT("Saved/V836"),TEXT("armour-selection-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json")));
 FPaths::NormalizeFilename(File);
 const bool FreshFile=File.StartsWith(Dir+TEXT("/"),ESearchCase::IgnoreCase)&&!IFileManager::Get().FileExists(*File);
 Check(TEXT("v836_rail_persistence_file_is_new_and_inside_disposable_userdir"),FreshFile);if(!FreshFile)return;
 const auto Runs=B->CampaignRuns;const auto Army0=B->ArmyProgress[0],Army1=B->ArmyProgress[1];
 const auto Receipts=B->RewardedEncounterIds;const auto Active=B->ActiveCampaignId,Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId,Encounter=B->EncounterId;
 const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53;
 const auto OriginalFile=B->ProgressionFile,Status=B->ProgressionSaveStatus,Message=B->LastMessage;
 const bool Volatile=B->bProgressionVolatile,ReadOnly=B->bProgressionReadOnly,Advanced=B->bCampaignMissionAdvanced,Rewarded=B->bEncounterRewarded;
 ON_SCOPE_EXIT{B->bProgressionVolatile=Volatile;B->ProgressionFile=OriginalFile;B->bProgressionReadOnly=ReadOnly;
  B->CampaignRuns=Runs;B->ArmyProgress[0]=Army0;B->ArmyProgress[1]=Army1;B->RewardedEncounterIds=Receipts;
  B->ActiveCampaignId=Active;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;B->EncounterId=Encounter;
  B->PlayerFaction=Faction;B->Phase=Phase;B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;
  B->bCampaignMissionAdvanced=Advanced;B->bEncounterRewarded=Rewarded;B->ProgressionSaveStatus=Status;B->LastMessage=Message;};
 const uint32 TrainPair=(1u<<int32(EFrontlineVehicle::RailConvoy))|(1u<<int32(EFrontlineVehicle::A7V));
 FFrontlineCampaignProgress Fixture;Fixture.Id=FGuid::NewGuid();Fixture.Faction=EFrontlineFaction::German;
 Fixture.CreatedUtc=Fixture.UpdatedUtc=FDateTime::UtcNow().ToIso8601();Fixture.ArmourSelectionMaskV63=TrainPair;
 B->CampaignRuns={Fixture};B->ActiveCampaignId=Fixture.Id;B->PlayerFaction=Fixture.Faction;
 B->ArmyProgress[0]={};B->ArmyProgress[1]={};B->RewardedEncounterIds.Empty();
 B->ProgressionFile=File;B->bProgressionReadOnly=false;B->bProgressionVolatile=false;
 FString LegalJson;const bool Written=B->SaveProgression()&&FFileHelper::LoadFileToString(LegalJson,*File);
 Check(TEXT("v836_rail_plus_a7v_ordinary_profile_serializer_writes_disposable_file"),Written);if(!Written)return;
 B->LoadProgression(true);
 const bool Loaded=!B->bProgressionReadOnly&&B->PlayerFaction==EFrontlineFaction::German&&B->ActiveCampaignId==Fixture.Id
  &&B->CampaignRuns.Num()==1&&B->CampaignRuns[0].ArmourSelectionMaskV63==TrainPair;
 Check(TEXT("v836_rail_plus_a7v_actual_profile_reload_preserves_two_selection_bits"),Loaded);if(!Loaded)return;
 TSharedPtr<FJsonObject> Json;const bool Parsed=FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(LegalJson),Json)&&Json.IsValid();
 const TArray<TSharedPtr<FJsonValue>>* JsonRuns=nullptr;
 const bool HasRun=Parsed&&Json->TryGetArrayField(TEXT("campaigns"),JsonRuns)&&JsonRuns&&JsonRuns->Num()==1;
 Check(TEXT("v836_unknown_armour_bit_rejection_uses_actual_profile_file"),HasRun);if(!HasRun)return;
 (*JsonRuns)[0]->AsObject()->SetNumberField(TEXT("armour_selection_v63"),1u<<int32(EFrontlineVehicle::Count));
 FString InvalidJson;FJsonSerializer::Serialize(Json,TJsonWriterFactory<>::Create(&InvalidJson));
 const bool InvalidWritten=FFileHelper::SaveStringToFile(InvalidJson,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 if(InvalidWritten)B->LoadProgression(true);FString Preserved;
 Check(TEXT("v836_unknown_8192_armour_bit_rejected_without_partial_campaign_import_or_file_repair"),InvalidWritten
  &&B->bProgressionReadOnly&&B->CampaignRuns.IsEmpty()&&!B->ActiveCampaignId.IsValid()
  &&FFileHelper::LoadFileToString(Preserved,*File)&&Preserved==InvalidJson);
 const bool Restored=FFileHelper::SaveStringToFile(LegalJson,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 if(Restored)B->LoadProgression(true);
 Check(TEXT("v836_legal_rail_profile_reload_recovers_after_rejected_unknown_bit"),Restored&&!B->bProgressionReadOnly
  &&B->CampaignRuns.Num()==1&&B->CampaignRuns[0].ArmourSelectionMaskV63==TrainPair&&B->ActiveCampaignId==Fixture.Id);
}
void FFrontlineArmourV63Access::AllocationChecksV836(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 using namespace FrontlineArmourV63;
 auto* R=B->CampaignRuns.FindByPredicate([&](const auto& C){return C.Id==B->ActiveCampaignId;});
 auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 Check(TEXT("v836_allocation_actual_campaign_and_vehicle_subsystem_present"),R&&Fleet);if(!R||!Fleet)return;
 const auto Original=*R;const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;
 const auto Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId;const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53;
 const auto Message=B->LastMessage;const bool ReadOnly=B->bProgressionReadOnly;
 ON_SCOPE_EXIT{*R=Original;B->PlayerFaction=Faction;B->Phase=Phase;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;
  B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->LastMessage=Message;B->bProgressionReadOnly=ReadOnly;};
 Check(TEXT("v836_allocation_review_ordinary_research_switch"),!FrontlineDebugV37::UnlimitedResearch(B));
 if(FrontlineDebugV37::UnlimitedResearch(B))return;
 B->Phase=EFrontlinePhase::Briefing;B->bProgressionReadOnly=false;
 for(int32 Nation=0;Nation<2;++Nation)
 {
  *R=Original;R->Faction=EFrontlineFaction(Nation);R->SelectedMission=R->CompletedMissions=19;R->ArmourResearchMaskV63=65535;
  R->ArmourSelectionMaskV63=R->ArmourConfigurationMaskV836=0;B->PlayerFaction=R->Faction;
  B->BoundCampaignId=R->Id;B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=19;
  const bool DE=Nation==1;const FString Prefix=DE?TEXT("v836_de_"):TEXT("v836_uk_");
  const auto Base=DE?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::MarkIVMale;
  const auto Alternate=DE?EFrontlineVehicle::BeuteMarkIVFemale:EFrontlineVehicle::MarkIVFemale;
  const auto Other=DE?EFrontlineVehicle::A7V:EFrontlineVehicle::Whippet;
  const uint32 BaseBit=1u<<int32(Base),AltBit=1u<<int32(Alternate),OtherBit=1u<<int32(Other);
  R->ArmourSelectionMaskV63=OtherBit;
  Check(Prefix+TEXT("unselected_mark_iv_configuration_switches_without_equipping"),ToggleConfigurationV8218(B,Base)
   &&R->ArmourSelectionMaskV63==OtherBit&&ConfigurationV8218(B,Base)==Alternate
   &&Fleet->GetRoster().ContainsByPredicate([&](const auto& D){return D.Type==Alternate;}));
  const bool Selected=ToggleSelection(B,Alternate),Cancelled=ToggleSelection(B,Alternate);
  Check(Prefix+TEXT("deselecting_configuration_retains_preference_and_other_slot"),Selected&&Cancelled
   &&R->ArmourSelectionMaskV63==OtherBit&&ConfigurationV8218(B,Base)==Alternate);
  Check(Prefix+TEXT("reselection_keeps_configuration_without_consuming_extra_slot"),ToggleSelection(B,Alternate)
   &&R->ArmourSelectionMaskV63==(OtherBit|AltBit));
  Check(Prefix+TEXT("selected_configuration_switch_preserves_two_platforms"),ToggleConfigurationV8218(B,Alternate)
   &&R->ArmourSelectionMaskV63==(OtherBit|BaseBit)&&ConfigurationV8218(B,Base)==Base);
  R->Research.ResearchPoints=20;R->PioneerOrdersMaskV63=R->SmokeOrdersMaskV63=R->ArmourOrdersMaskV63=R->RequisitionPaidMaskV836=0;
  const uint32 Bit=1u<<19;
  Check(Prefix+TEXT("actual_requisition_purchase_records_paid_cost_once"),BuyRequisition(B,0)&&R->Research.ResearchPoints==17
   &&R->PioneerOrdersMaskV63==Bit&&R->RequisitionPaidMaskV836==Bit);
  Check(Prefix+TEXT("actual_requisition_cancel_refunds_once_and_releases_card"),BuyRequisition(B,0)&&R->Research.ResearchPoints==20
   &&R->PioneerOrdersMaskV63==0&&R->RequisitionPaidMaskV836==0&&Requisition(B)==INDEX_NONE);
  bool Repeat=true;for(int32 I=0;I<3;++I)Repeat&=BuyRequisition(B,0)&&BuyRequisition(B,0);
  Check(Prefix+TEXT("repeated_purchase_cancel_cannot_mint_research"),Repeat&&R->Research.ResearchPoints==20&&R->RequisitionPaidMaskV836==0);
  R->PioneerOrdersMaskV63=Bit;R->RequisitionPaidMaskV836=0;
  Check(Prefix+TEXT("unpaid_attachment_cancellation_never_refunds"),BuyRequisition(B,0)&&R->Research.ResearchPoints==20);
  R->PioneerOrdersMaskV63=1u<<18;R->RequisitionPaidMaskV836=1u<<18;
  Check(Prefix+TEXT("current_operation_cancel_cannot_refund_previous_operation"),BuyRequisition(B,0)&&BuyRequisition(B,0)
   &&R->Research.ResearchPoints==20&&R->PioneerOrdersMaskV63==(1u<<18)&&R->RequisitionPaidMaskV836==(1u<<18));
  BuyRequisition(B,0);B->Phase=EFrontlinePhase::Fighting;
  Check(Prefix+TEXT("requisition_cannot_be_cancelled_after_battle_starts"),!BuyRequisition(B,0)&&R->Research.ResearchPoints==17
   &&(R->PioneerOrdersMaskV63&Bit));B->Phase=EFrontlinePhase::Briefing;
  B->bProgressionReadOnly=true;
  Check(Prefix+TEXT("read_only_archive_keeps_order_and_paid_balance"),!BuyRequisition(B,0)&&R->Research.ResearchPoints==17
   &&(R->PioneerOrdersMaskV63&Bit)&&(R->RequisitionPaidMaskV836&Bit));B->bProgressionReadOnly=false;
 }
}

void FFrontlineArmourV63Access::ForcedRailChecksV842(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 using namespace FrontlineArmourV63;
 auto* R=B->CampaignRuns.FindByPredicate([&](const auto& C){return C.Id==B->ActiveCampaignId;});
 auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());
 Check(TEXT("v842_rail_fixture_has_actual_campaign"),R&&Fleet);if(!R||!Fleet)return;
 const auto Original=*R;const auto Faction=B->PlayerFaction;const auto Phase=B->Phase;
 const auto Bound=B->BoundCampaignId,Binding=B->BoundCampaignEncounterId;
 const int32 Mission=B->BoundCampaignMission,Attachment=B->BoundAttachmentStageV53;
 const auto Message=B->LastMessage;const bool ReadOnly=B->bProgressionReadOnly;
 ON_SCOPE_EXIT{*R=Original;B->PlayerFaction=Faction;B->Phase=Phase;B->BoundCampaignId=Bound;B->BoundCampaignEncounterId=Binding;
 B->BoundCampaignMission=Mission;B->BoundAttachmentStageV53=Attachment;B->LastMessage=Message;B->bProgressionReadOnly=ReadOnly;};
 const uint32 Rail=1u<<int32(EFrontlineVehicle::RailConvoy),A7V=1u<<int32(EFrontlineVehicle::A7V),Beute=1u<<int32(EFrontlineVehicle::BeuteMarkIV);
 R->Faction=EFrontlineFaction::German;R->SelectedMission=17;R->CompletedMissions=19;R->ArmourResearchMaskV63=65535;
 R->ArmourSelectionMaskV63=A7V|Beute;R->ArmourConfigurationMaskV836=0;B->PlayerFaction=R->Faction;
 B->Phase=EFrontlinePhase::Briefing;B->bProgressionReadOnly=false;
 B->BoundCampaignId=R->Id;B->BoundCampaignEncounterId=B->GetEncounterId();B->BoundCampaignMission=B->BoundAttachmentStageV53=17;
 const int32 Points=R->Research.ResearchPoints;
 auto Equipped=Fleet->GetEquippedRoster(0);
 Check(TEXT("v842_german_rail_mission_forces_train_before_old_two_model_preference"),Equipped.Num()==2
  &&Equipped.ContainsByPredicate([](const auto& D){return D.Type==EFrontlineVehicle::RailConvoy;})&&R->ArmourSelectionMaskV63==(A7V|Beute));
 Check(TEXT("v842_forced_train_cannot_be_deselected_and_costs_no_research"),!ToggleSelection(B,EFrontlineVehicle::RailConvoy)
  &&R->ArmourSelectionMaskV63==(A7V|Beute)&&R->Research.ResearchPoints==Points);
 Check(TEXT("v842_second_rail_slot_can_swap_without_persisting_temporary_train"),ToggleSelection(B,EFrontlineVehicle::BeuteMarkIV)
  &&R->ArmourSelectionMaskV63==Beute&&GetSelectedMask(B,Rail|A7V|Beute)==(Rail|Beute));
 R->SelectedMission=18;B->BoundCampaignMission=B->BoundAttachmentStageV53=18;
 Check(TEXT("v842_next_nonrail_operation_has_no_train_and_keeps_ordinary_preference"),!ForcedAllocationV842(B,EFrontlineVehicle::RailConvoy)
  &&GetSelectedMask(B,A7V|Beute)==Beute&&R->ArmourSelectionMaskV63==Beute);
 R->Faction=EFrontlineFaction::British;R->SelectedMission=17;B->PlayerFaction=R->Faction;B->BoundCampaignMission=B->BoundAttachmentStageV53=17;B->Phase=EFrontlinePhase::Fighting;
 Equipped=Fleet->GetEquippedRoster(1);
 Check(TEXT("v842_opponent_german_rail_doctrine_includes_train_without_player_selection"),Equipped.Num()==2
  &&Equipped.ContainsByPredicate([](const auto& D){return D.Type==EFrontlineVehicle::RailConvoy;})&&!ForcedAllocationV842(B,EFrontlineVehicle::RailConvoy,0)
  &&ForcedAllocationV842(B,EFrontlineVehicle::RailConvoy,1));
}

void FrontlineArmourV63::RunRailAllocationChecksV842(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
 FString UserDir;
 const bool Safe=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV842Review"))
  &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
 Check(TEXT("v842_rail_requires_disposable_no_save_review"),Safe);if(Safe)FFrontlineArmourV63Access::ForcedRailChecksV842(B,Check);
}

TSharedRef<FJsonObject> FFrontlineArmourV63Access::EscortChecksV8445(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> ExternalCheck)
{
 using namespace FrontlineArmourV63;
 auto J=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks,Cases;bool Good=true;double MaximumReleaseDisplacement=0;
 const auto Check=[&](const FString& Name,bool Pass)
 {auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));Good&=Pass;ExternalCheck(Name,Pass);};
 FString Dir;const bool Requested=B&&B->GetWorld()&&B->IsProgressionVolatile()
  &&FParse::Param(FCommandLine::Get(),TEXT("WFV8445Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
  &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)&&!FPaths::IsRelative(Dir)&&!Dir.Contains(TEXT(".."))
  &&!WarfareFRRUCampaignV82::Active(B)&&B->GetCurrentCampaign();
 FPaths::NormalizeDirectoryName(Dir);
 const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V8445/Runtime/"));
 const FString Stamp=Dir.StartsWith(Root,ESearchCase::IgnoreCase)?Dir.Mid(Root.Len()):FString();
 const bool Safe=Requested&&Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
  &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
 Check(TEXT("v8445_escort_requires_exact_volatile_no_save_uk_de_fixture"),Safe);
 if(!Safe){J->SetBoolField(TEXT("success"),false);J->SetArrayField(TEXT("checks"),Checks);return J;}
 const auto Units=B->Units;const auto Squads=B->Squads;const auto Trenches=B->Trenches;const auto Tracers=B->Tracers;const auto Strikes=B->Strikes;const auto Smoke=B->SmokeZones;
 const auto Phase=B->Phase;const int32 Outro=B->BattleOutroWinner,NextId=B->NextSquadId,Captures=B->TrenchCaptures;
 const int32 Advances=B->AdvanceCommands,Shots=B->ShotsFired;const float Elapsed=B->Elapsed,EnemyCounter=B->EnemyCounterattackUntil;
 const int32 EnemyCounterTrench=B->EnemyCounterattackTrench,EnemyWaves=B->EnemyWaveRecruitCount;
 const float EnemyWaveStarted=B->EnemyWaveStartedAt,EnemyRecruit=B->EnemyRecruitAt,EnemyAssault=B->EnemyAssaultAt;
 const bool EnemyCommitted=B->bEnemyRoleCommitted;const auto Message=B->LastMessage;
 const float Morale[]={B->Morale[0],B->Morale[1]},Damage[]={B->DamageDealt[0],B->DamageDealt[1]};
 const int32 Casualties[]={B->Casualties[0],B->Casualties[1]};const auto Record0=B->CombatRecordV65[0],Record1=B->CombatRecordV65[1];
 const float Friendly=B->FriendlyFireDamage,Peak=B->PeakSuppression;
 const auto Escorted=PioneerEscortsV63;const auto Allocated=EscortAllocatedV8445;const auto Egress=PioneerEgressV8445;const auto Diagnostics=ExitDiagnosticsV8445;
 TArray<AWarfareFrontlineSoldier*> Made;TArray<AFrontlineTank*> Tanks;
 struct FHiddenV8445{TWeakObjectPtr<AActor> Actor;bool Hidden,Collision;};TArray<FHiddenV8445> Hidden;
 const auto Hide=[&](AActor* A){if(IsValid(A)){Hidden.Add({A,A->IsHidden(),A->GetActorEnableCollision()});A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}};
 for(AWarfareFrontlineSoldier* U:Units)Hide(U);
 if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:Fleet->GetVehicles())Hide(T);
 ON_SCOPE_EXIT
 {
  for(auto* U:Made)if(IsValid(U))U->Destroy();for(auto* T:Tanks)if(IsValid(T))T->Destroy();
  B->Units=Units;B->Squads=Squads;B->Trenches=Trenches;B->Tracers=Tracers;B->Strikes=Strikes;B->SmokeZones=Smoke;
  B->Phase=Phase;B->BattleOutroWinner=Outro;B->NextSquadId=NextId;B->TrenchCaptures=Captures;B->AdvanceCommands=Advances;B->ShotsFired=Shots;
  B->Elapsed=Elapsed;B->EnemyCounterattackUntil=EnemyCounter;B->EnemyCounterattackTrench=EnemyCounterTrench;B->LastMessage=Message;
  B->EnemyWaveRecruitCount=EnemyWaves;B->EnemyWaveStartedAt=EnemyWaveStarted;B->EnemyRecruitAt=EnemyRecruit;B->EnemyAssaultAt=EnemyAssault;B->bEnemyRoleCommitted=EnemyCommitted;
  B->CombatRecordV65[0]=Record0;B->CombatRecordV65[1]=Record1;B->FriendlyFireDamage=Friendly;B->PeakSuppression=Peak;
  for(int32 T=0;T<2;++T){B->Morale[T]=Morale[T];B->Casualties[T]=Casualties[T];B->DamageDealt[T]=Damage[T];}
  PioneerEscortsV63=Escorted;EscortAllocatedV8445=Allocated;PioneerEgressV8445=Egress;ExitDiagnosticsV8445=Diagnostics;
  for(const auto& H:Hidden)if(auto* A=H.Actor.Get()){A->SetActorHiddenInGame(H.Hidden);A->SetActorEnableCollision(H.Collision);}
  FrontlineMobilityV41::Reset(B);
 };
 B->Units.Empty();B->Squads.Empty();B->Tracers.Empty();B->Strikes.Empty();B->SmokeZones.Empty();B->Phase=EFrontlinePhase::Fighting;B->BattleOutroWinner=INDEX_NONE;
 FrontlineMoraleV36::WithScopedCombatEventsV839(B,[&]{FrontlineAchievementsV76::WithScopedCombatEventsV839(B,[&]
 {
  for(int32 Team:{0,1})for(bool OneCasualty:{false,true})
  {
   for(auto* U:Made)if(IsValid(U))U->Destroy();Made.Empty();B->Units.Empty();B->Squads.Empty();
   FrontlineMobilityV41::Reset(B);B->Elapsed=100.f;
   for(auto& Trench:B->Trenches){Trench.Owner=Team;Trench.bBypass=false;}
   const int32 DirTeam=B->GetTeamDirection(Team);
   FVector P(0,0,0);
   if(B->Trenches.Num()>3)P.X=(B->Trenches[2].X+B->Trenches[3].X)*.5f;
   P.Z=FrontlineInfantryV45::SurfaceHeight(B,P);
   auto* Tank=B->GetWorld()->SpawnActor<AFrontlineTank>(P,FRotator(0,DirTeam>0?0.f:180.f,0));Tanks.Add(Tank);
   const auto Type=B->GetFactionForTeam(Team)==EFrontlineFaction::German?EFrontlineVehicle::BeuteMarkIV:EFrontlineVehicle::MarkIVMale;
   const FString Prefix=FString::Printf(TEXT("v8445_escort_team_%d_casualty_%d_"),Team,OneCasualty?1:0);
   const bool Configured=Tank&&Tank->Configure(B,Type,Team,-844501-Tanks.Num());
   Check(Prefix+TEXT("actual_carrier_configured"),Configured);if(!Configured)continue;
   SpawnPioneerEscort(B,Tank);for(AWarfareFrontlineSoldier* U:B->Units){Made.Add(U);U->SetActorTickEnabled(false);U->ShotClock=U->GrenadeClock=10000.f;U->bFieldEntered=true;}
   auto* S=B->Squads.IsEmpty()?nullptr:&B->Squads.Last();
   const bool Spawned=S&&S->Members.Num()==2&&S->LivingCount()==2;
   Check(Prefix+TEXT("two_existing_ground_fitters"),Spawned);if(!Spawned)continue;
   const int32 Id=S->Id,ResearchBefore=B->GetResearchPoints();
   AWarfareFrontlineSoldier* Enemy=nullptr;
   if(OneCasualty)
   {
    // Keep two existing cases as visible-contact regressions, with ordinary
    // opposing actors and sight acquisition. Only their discharge is delayed;
    // no invulnerability or fabricated Contact pointer protects the fitter.
    FVector EnemyFeet=P+FVector(DirTeam*450.f,DirTeam*300.f,0);EnemyFeet.Z=FrontlineInfantryV45::SurfaceHeight(B,EnemyFeet);
    Enemy=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(EnemyFeet+FVector(0,0,88.f),FRotator(0,DirTeam>0?180.f:0.f,0));
    if(Enemy)
    {
     const int32 EnemyId=B->NextSquadId++;Enemy->Initialize(B,EnemyId,1-Team,EFrontlineRole::Rifle,0);
     Enemy->bFieldEntered=true;Enemy->SetActorTickEnabled(false);Enemy->ShotClock=Enemy->GrenadeClock=10000.f;
     Made.Add(Enemy);B->Units.Add(Enemy);FFrontlineSquad EnemySection;EnemySection.Id=EnemyId;EnemySection.Team=1-Team;
     EnemySection.Members.Add(Enemy);B->Squads.Add(EnemySection);FrontlineMobilityV41::InvalidateRoster(B);
    }
    Check(Prefix+TEXT("visible_contact_opponent_spawned_with_delayed_discharge"),Enemy&&Enemy->IsActiveCombatant()&&!Enemy->IsHidden());
    S=B->FindSquad(Id); // Adding the ordinary opponent may relocate the array.
   }
   const int32 Count=B->Units.Num(),SquadCount=B->Squads.Num();
   const auto RewardedBefore=B->RewardedEncounterIds;
   SpawnPioneerEscort(B,Tank);
   Check(Prefix+TEXT("repeat_spawn_does_not_duplicate_section"),B->Units.Num()==Count&&B->Squads.Num()==SquadCount);
   Check(Prefix+TEXT("operational_carrier_cannot_release"),ReleasePioneerEscortV8445(B,Tank)==0&&SquadIsEscorting(B,Id));
   auto* First=S->Members[0].Get();auto* Second=S->Members[1].Get();
   // Exercise the real displaced-body exit; the second man already stands
   // clear on the flank and must not be needlessly teleported by release.
   First->SetActorLocation(P+FVector(0,0,First->GroundContactOffsetV82));
   ExitDiagnosticsV8445.Add(First,FExitDiagnosticV8445());
   const FVector SecondBefore=Second->Feet();
   if(OneCasualty)Second->ReceiveHit(1.f,0.f,INDEX_NONE,FVector::UpVector,EFrontlineDamageKind::Bullet);
   const int32 LivingBefore=S->LivingCount(),CursorBefore=S->LastPassedTrench,AdvanceBefore=B->AdvanceCommands;
   const FVector FirstBefore=First->Feet(),SecondBeforeRelease=Second->Feet();
   Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,Tank->GetHullIntegrity()+1.f);
   const double ReleaseDisplacement=FMath::Max(FVector::Dist(First->Feet(),FirstBefore),FVector::Dist(Second->Feet(),SecondBeforeRelease));
   MaximumReleaseDisplacement=FMath::Max(MaximumReleaseDisplacement,ReleaseDisplacement);
   FVector Destination;
   const bool Detached=!Tank->IsOperational()&&!EscortFor(First)&&!EscortFor(Second)&&!SquadIsEscorting(B,Id)&&!EscortDestination(First,Destination);
   Check(Prefix+TEXT("actual_fatal_hull_hit_releases_existing_survivors"),Detached&&S->LivingCount()==LivingBefore&&First->IsAlive()&&(OneCasualty?!Second->IsAlive():Second->IsAlive()));
   const bool ObservedContact=Enemy&&B->AcquireTarget(First,true)==Enemy&&B->HasSight(First,Enemy);
   if(OneCasualty)Check(Prefix+TEXT("actual_advancing_acquisition_and_physical_sight_observe_opponent"),ObservedContact);
   FVector SafeExit=FVector::ZeroVector;
   const bool HasExit=PioneerEgressDestinationV8445(First,SafeExit);
   Check(Prefix+TEXT("displaced_survivor_receives_checked_exit_without_teleport"),HasExit
    &&EscortGroundClearV8445(B,First,Tank,SafeExit)&&ReleaseDisplacement<=.01);
   Check(Prefix+TEXT("clear_flank_or_dead_fitter_keeps_position"),Second->Feet().Equals(SecondBefore,.2f));
   const FVector Released=First->Feet();const int32 CasualtyCount=B->Casualties[Team];
   const float ReleaseMorale=B->Morale[Team];
   Check(Prefix+TEXT("release_retains_passage_cursor_and_normal_advance"),S->bAdvancing&&S->Trench==INDEX_NONE&&!S->bHoldPosition
    &&S->LastPassedTrench==CursorBefore&&B->AdvanceCommands==AdvanceBefore);
   Tank->ReceiveModuleHit(EFrontlineTankModule::Hull,10000.f);SpawnPioneerEscort(B,Tank);
   const int32 Again=ReleasePioneerEscortV8445(B,Tank);
   Check(Prefix+TEXT("repeat_destruction_release_and_spawn_have_no_new_units_casualties_or_morale"),Again==0&&B->Units.Num()==Count&&B->Squads.Num()==SquadCount
    &&S->LivingCount()==LivingBefore&&First->Feet().Equals(Released,.01f)&&B->Casualties[Team]==CasualtyCount&&FMath::IsNearlyEqual(B->Morale[Team],ReleaseMorale)
    &&B->GetResearchPoints()==ResearchBefore&&B->RewardedEncounterIds==RewardedBefore);
   B->UpdateSquads();First->ReactionSeconds=First->DepartureClock=0.f;First->Suppression=0.f;
   const FVector BeforeMovement=First->Feet();
   const FVector OrdinaryGoal=B->SoldierDestination(First);
   const bool MineWorkBefore=FrontlineFieldV32::IsMineWorkingV8215(First),AdvancingBefore=S->bAdvancing;
   const int32 TrenchBeforeMovement=S->Trench,SquadDestinationBefore=S->Destination;
   double MaximumStep=0,MaximumRequestedSpeed=0;int32 ContactMovementTicks=0,EngagedEgressTicks=0,MineWorkTicks=0,AtDestinationTicks=0,WantsMovementTicks=0,EngagedTicks=0;
   for(int32 I=0;I<180&&B->Phase==EFrontlinePhase::Fighting;++I)
   {
    const FVector BeforeStep=First->Feet();FVector ActiveExit;
    const bool EgressActive=PioneerEgressDestinationV8445(First,ActiveExit);
    const bool Seen=Enemy&&B->AcquireTarget(First,true)==Enemy&&B->HasSight(First,Enemy);
    B->Elapsed+=1.f/60.f;First->Tick(1.f/60.f);
    const double Travel=FVector::Dist2D(BeforeStep,First->Feet());MaximumStep=FMath::Max(MaximumStep,Travel);
    if(FrontlineFieldV32::IsMineWorkingV8215(First))++MineWorkTicks;if(First->bAtDestinationV43)++AtDestinationTicks;
    if(First->bWantedMovement)++WantsMovementTicks;if(First->bEngaging)++EngagedTicks;
    MaximumRequestedSpeed=FMath::Max(MaximumRequestedSpeed,double(First->RequestedTravelSpeedV43));
    if(EgressActive&&Seen&&First->bEngaging){++EngagedEgressTicks;if(Travel>.01)++ContactMovementTicks;}
   }
   FVector Escaped=First->Feet();
   Check(Prefix+TEXT("production_infantry_tick_continuously_exits_and_follows_normal_orders"),First->IsAlive()&&FVector::Dist2D(First->Feet(),BeforeMovement)>60.f
    &&MaximumStep<=15.f&&EscortGroundClearV8445(B,First,Tank,Escaped)&&!PioneerEgressDestinationV8445(First,Destination));
   if(OneCasualty)Check(Prefix+TEXT("living_fitter_physically_exits_wreck_while_real_contact_remains_engaged"),ObservedContact&&EngagedEgressTicks>0&&ContactMovementTicks>0
    &&First->IsAlive()&&EscortGroundClearV8445(B,First,Tank,Escaped)&&!PioneerEgressDestinationV8445(First,Destination));
   auto C=MakeShared<FJsonObject>();C->SetNumberField(TEXT("team"),Team);C->SetNumberField(TEXT("living_before"),LivingBefore);
   C->SetNumberField(TEXT("living_after"),S->LivingCount());C->SetNumberField(TEXT("survivor_travel_cm"),FVector::Dist2D(First->Feet(),BeforeMovement));
   C->SetNumberField(TEXT("release_displacement_cm"),ReleaseDisplacement);C->SetNumberField(TEXT("maximum_tick_step_cm"),MaximumStep);
   C->SetBoolField(TEXT("visible_contact_variant"),OneCasualty);C->SetBoolField(TEXT("actual_acquisition_and_sight_contact"),ObservedContact);
   C->SetNumberField(TEXT("engaged_egress_ticks"),EngagedEgressTicks);C->SetNumberField(TEXT("contact_movement_ticks"),ContactMovementTicks);
   C->SetStringField(TEXT("initial_feet"),FirstBefore.ToString());C->SetStringField(TEXT("final_feet"),First->Feet().ToString());
   C->SetStringField(TEXT("requested_goal_before_movement"),OrdinaryGoal.ToString());C->SetStringField(TEXT("checked_exit_goal"),SafeExit.ToString());
   C->SetBoolField(TEXT("checked_exit_present"),HasExit);
   C->SetBoolField(TEXT("mine_work_before_movement"),MineWorkBefore);C->SetBoolField(TEXT("squad_advancing_before_movement"),AdvancingBefore);
   C->SetNumberField(TEXT("squad_trench_before_movement"),TrenchBeforeMovement);C->SetNumberField(TEXT("squad_destination_before_movement"),SquadDestinationBefore);
   C->SetNumberField(TEXT("mine_work_ticks"),MineWorkTicks);C->SetNumberField(TEXT("at_destination_ticks"),AtDestinationTicks);
   C->SetNumberField(TEXT("wants_movement_ticks"),WantsMovementTicks);C->SetNumberField(TEXT("engaged_ticks"),EngagedTicks);C->SetNumberField(TEXT("maximum_requested_speed_cm_s"),MaximumRequestedSpeed);
   C->SetNumberField(TEXT("field_half_length_cm"),FrontlinePacingV24::CombatHalfLength());C->SetNumberField(TEXT("field_half_frontage_cm"),FrontlineBattlefield::HalfFrontage);
   C->SetStringField(TEXT("wreck_position"),Tank->GetActorLocation().ToString());C->SetStringField(TEXT("wreck_actor_transform"),Tank->GetActorTransform().ToString());
   if(const auto* Body=Cast<UStaticMeshComponent>(Tank->GetRootComponent())){C->SetStringField(TEXT("wreck_body_world_bounds"),Body->Bounds.GetBox().ToString());C->SetStringField(TEXT("wreck_body_component_transform"),Body->GetComponentTransform().ToString());}
   C->SetNumberField(TEXT("capsule_radius_cm"),First->Capsule?First->Capsule->GetScaledCapsuleRadius():25.f);C->SetNumberField(TEXT("capsule_half_height_cm"),First->Capsule?First->Capsule->GetScaledCapsuleHalfHeight():88.f);
   if(const auto* D=ExitDiagnosticsV8445.Find(First))C->SetObjectField(TEXT("exit_search_diagnostics"),D->Json());
   C->SetBoolField(TEXT("carrier_operational"),Tank->IsOperational());C->SetBoolField(TEXT("casualty_remains_dead"),!OneCasualty||!Second->IsAlive());Cases.Add(MakeShared<FJsonValueObject>(C));
   Tank->Destroy();
  }
  // No escort roster means no evacuees. The destruction hook cannot use its
  // allocation marker, research or vehicle type to invent a new section.
  for(auto* U:Made)if(IsValid(U))U->Destroy();Made.Empty();B->Units.Empty();B->Squads.Empty();
  auto* Empty=B->GetWorld()->SpawnActor<AFrontlineTank>(FVector(0,0,0),FRotator::ZeroRotator);Tanks.Add(Empty);
  const bool EmptyReady=Empty&&Empty->Configure(B,EFrontlineVehicle::MarkIVMale,0,-844590);
  if(EmptyReady)Empty->ReceiveModuleHit(EFrontlineTankModule::Hull,Empty->GetHullIntegrity()+1.f);
  Check(TEXT("v8445_destroyed_carrier_without_fitters_never_creates_pioneers"),EmptyReady&&ReleasePioneerEscortV8445(B,Empty)==0&&B->Units.IsEmpty()&&B->Squads.IsEmpty());
 });});
 J->SetBoolField(TEXT("success"),Good);J->SetArrayField(TEXT("checks"),Checks);J->SetArrayField(TEXT("cases"),Cases);
 J->SetNumberField(TEXT("maximum_release_displacement_cm"),MaximumReleaseDisplacement);
 J->SetStringField(TEXT("scope"),TEXT("Four real UK/DE carrier and ground-fitter fixtures, including one visible-opponent variant per direction: actual acquisition/sight, fatal hull receiver, mixed living/dead roster, once-only release, checked continuous ground exit and production soldier Tick. Opponent discharge is delayed; zero release teleport, no invulnerability, campaign settlement or player file writes."));
 return J;
}

TSharedRef<FJsonObject> FrontlineArmourV63::RunPioneerEscortChecksV8445(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineArmourV63Access::EscortChecksV8445(B,Check);}
