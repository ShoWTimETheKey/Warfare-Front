#include "WarfareFrontlineNewUnlockV8215.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineGameplayV52.h"
#include "WarfareFrontlinePlayerGuideV43.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFRRUCampaignV82.h"
namespace FrontlineNewUnlockV8215
{
namespace{struct FNotice{FGuid Encounter,Campaign;int32 NextMission=INDEX_NONE;TArray<FCard> Cards;bool Dismissed=false,Released=false;};TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FNotice> Notices;const TArray<FCard> Empty;}
void Capture(AWarfareFrontlineGameMode* B,int32 Won)
{
 if(!B||WarfareFRRUCampaignV82::Active(B)||FrontlineUltimateArchiveV8215::Active(B))return;
 auto& Notice=Notices.FindOrAdd(B);if(Notice.Encounter==B->GetEncounterId())return;
 Notice={};Notice.Encounter=B->GetEncounterId();const auto* Run=B->GetCurrentCampaign();const int32 Next=Run?Run->SelectedMission:Won+1;const auto& Missions=B->GetCampaignMissions();if(Next<=Won||!Missions.IsValidIndex(Next))return;
 if(!Run)return;Notice.Campaign=Run->Id;Notice.NextMission=Next;const auto& M=Missions[Next];
 for(int32 I=0;I<int32(EFrontlineRole::Count);++I)
 {
  const auto Role=EFrontlineRole(I);const int32 Arrival=B->RoleUnlockMission(Role);if(!FrontlineGameplayV52::SelectableRole(Role)||!FrontlineCampaignV41::RoleAllowed(B->GetPlayerFaction(),Role)||Arrival<=Won||Arrival>Next||(M.DisabledRoleMask&(1u<<I)))continue;
  Notice.Cards.Add({0,I,B->GetRecruitName(Role),FrontlinePlayerGuideV43::Unit(B,Role)});
 }
 for(int32 I=0;I<int32(EFrontlineSupport::Count);++I)
 {
  const auto Kind=EFrontlineSupport(I);const int32 Arrival=B->SupportUnlockMission(Kind);if(Kind==EFrontlineSupport::Rally||Arrival<=Won||Arrival>Next||(M.DisabledSupportMask&(1u<<I)))continue;
  Notice.Cards.Add({1,I,B->GetSupportName(Kind),B->GetSupportDescription(Kind)});
 }
 if(M.bArmourAllowedV41)if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const auto& D:V->GetRoster())
  if(V->UnlockMission(D.Type)>Won&&V->UnlockMission(D.Type)<=Next)Notice.Cards.Add({2,int32(D.Type),D.Name,FrontlinePlayerGuideV43::Armour(D.Type)});
}
const TArray<FCard>& Cards(const AWarfareFrontlineGameMode* B)
{const auto* N=B?Notices.Find(const_cast<AWarfareFrontlineGameMode*>(B)):nullptr;const auto* R=B?B->GetCurrentCampaign():nullptr;return N&&R&&N->Campaign==R->Id&&(N->Encounter==B->GetEncounterId()||(N->Released&&R->SelectedMission==N->NextMission))?N->Cards:Empty;}
bool Pending(const AWarfareFrontlineGameMode* B)
{const auto* N=B?Notices.Find(const_cast<AWarfareFrontlineGameMode*>(B)):nullptr;const auto* R=B?B->GetCurrentCampaign():nullptr;return N&&R&&N->Campaign==R->Id&&N->Released&&!N->Dismissed&&!N->Cards.IsEmpty()&&R->SelectedMission==N->NextMission&&B->Phase==EFrontlinePhase::Briefing;}
bool ReleaseAfterReportV8217(AWarfareFrontlineGameMode* B)
{auto* N=B?Notices.Find(B):nullptr;const auto* R=B?B->GetCurrentCampaign():nullptr;if(!N||!R||N->Campaign!=R->Id||N->NextMission!=R->SelectedMission||N->Dismissed||N->Cards.IsEmpty()||B->Phase!=EFrontlinePhase::Briefing)return false;N->Released=true;return true;}
void Dismiss(AWarfareFrontlineGameMode* B){if(auto* N=Notices.Find(B))N->Dismissed=true;}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{if(!B)return;const auto Before=Cards(B);Capture(B,B->GetEncounterCampaignMission()?B->GetEncounterCampaignMission()->Id:0);const auto Once=Cards(B);Capture(B,B->GetEncounterCampaignMission()?B->GetEncounterCampaignMission()->Id:0);Check(TEXT("v8215_next_unlock_snapshot_idempotent"),Cards(B).Num()==Once.Num());Check(TEXT("v8215_next_unlock_never_during_outro"),!B->IsBattleOutroActive()||!Pending(B));}
}
