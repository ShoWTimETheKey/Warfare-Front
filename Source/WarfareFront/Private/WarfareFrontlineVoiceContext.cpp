#include "WarfareFrontlineVoiceContext.h"
#include "WarfareFrontline.h"
#include "Engine/World.h"

void WarnFrontlineSoldiers(UWorld* World,EFrontlineVoiceEvent Event,const FVector& Threat,float Radius,int32 WitnessTeam)
{
    auto* Battle=World?Cast<AWarfareFrontlineGameMode>(World->GetAuthGameMode()):nullptr;
    auto* Audio=World?World->GetSubsystem<UFrontlineAudioSubsystem>():nullptr;
    if(!Battle || !Audio || Battle->Phase!=EFrontlinePhase::Fighting || Threat.ContainsNaN())return;
    AWarfareFrontlineSoldier* Witnesses[2]={nullptr,nullptr};
    float Distances[2]={Radius*Radius,Radius*Radius};
    for(AWarfareFrontlineSoldier* Soldier:Battle->Units)
    {
        if(!Soldier || !Soldier->IsActiveCombatant() || Soldier->Team<0 || Soldier->Team>1)continue;
        if(WitnessTeam!=INDEX_NONE && Soldier->Team!=WitnessTeam)continue;
        const float Distance=FVector::DistSquared2D(Soldier->Feet(),Threat);
        if(Distance<Distances[Soldier->Team]){Distances[Soldier->Team]=Distance;Witnesses[Soldier->Team]=Soldier;}
    }
    // The most exposed witness gets first chance; the shared speech budget then
    // prevents two national versions from talking across the same incoming shell.
    if(Distances[1]<Distances[0])Swap(Witnesses[0],Witnesses[1]);
    for(AWarfareFrontlineSoldier* Soldier:Witnesses)
        if(Soldier && Audio->PlayVoice(Event,Soldier->GetFaction()==EFrontlineFaction::German,Soldier->Feet(),Soldier->SquadId))break;
}
