// Disposable art staging only; never an achievement-feasibility result.
struct FFrontlineArmourArtV838Review
{
static AFrontlineTank* Prepare(AWarfareFrontlineGameMode* B,int32 Index)
{
 FString User;
 if(!B||!B->IsProgressionVolatile()||Index<0||Index>10||(!FParse::Param(FCommandLine::Get(),TEXT("WFV838Review"))&&!FParse::Param(FCommandLine::Get(),TEXT("WFV839Review")))
    ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||User.IsEmpty()||FPaths::IsRelative(User))return nullptr;
 static const EFrontlineVehicle Types[]={EFrontlineVehicle::Whippet,EFrontlineVehicle::Austin1918,
    EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::MarkIVFemale,EFrontlineVehicle::MarkVMale,EFrontlineVehicle::MarkVStar,
    EFrontlineVehicle::A7V,EFrontlineVehicle::A7VGretchen,EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::BeuteMarkIVFemale,EFrontlineVehicle::RailConvoy};
 B->AbandonBattleToOperations();
 if(!B->StartNewCampaign(Index>=6?EFrontlineFaction::German:EFrontlineFaction::British,EFrontlineDifficulty::Veteran))return nullptr;
 auto* P=const_cast<FFrontlineCampaignProgress*>(B->GetCurrentCampaign());if(!P)return nullptr;
 int32 ReviewOperation=17;if(FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review")))FParse::Value(FCommandLine::Get(),TEXT("WFV8311Operation="),ReviewOperation);
 ReviewOperation=FMath::Clamp(ReviewOperation,0,19);
 P->CompletedMissions=19;P->SelectedMission=ReviewOperation;P->WonOperationMaskV52=(1u<<19)-1;
 P->DispatchResolvedMaskV52=(1u<<20)-1;P->Research.TechnologyMask=(1u<<int32(EFrontlineTechnology::Count))-1;P->ArmourResearchMaskV63=65535;
 B->ResetEncounter();B->BeginBattle();if(B->Phase!=EFrontlinePhase::Fighting||!B->AssetFailure.IsEmpty())return nullptr;
 B->SetActorTickEnabled(false);B->Elapsed=120;
 auto* F=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!F)return nullptr;F->LastEncounter=B->GetEncounterId();F->NextEnemyAt=MAX_flt;
 auto* Live=F->Spawn(Types[Index],0,true);auto* Wreck=Index==10?Live:F->Spawn(Types[Index],0,true);
 if(!Live||!Wreck||!Live->AreAssetsReady()||!Wreck->AreAssetsReady())return nullptr;
 const bool V8311=FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"));const int32 Home=B->GetHomeTrenchIndex(0),Dir=B->GetTeamDirection(0),Next=Home+Dir;
 float LiveX=-1250.f,WreckX=1050.f;
 if(V8311&&B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next))
 {LiveX=(B->Trenches[Home].X+B->Trenches[Next].X)*.5f-Dir*650.f;const int32 Further=Next+Dir;WreckX=B->Trenches.IsValidIndex(Further)?(B->Trenches[Next].X+B->Trenches[Further].X)*.5f:B->Trenches[Next].X+Dir*850.f;}
 for(AFrontlineTank* V:{Live,Wreck})
 {V->SetActorTickEnabled(false);V->HeadingYawV37=FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))&&B->GetTeamDirection(0)<0?180.f:0.f;V->SetActorRotation(FRotator(0,V->HeadingYawV37,0));V->bEntered=true;V->PreferredLaneV37=V->DesiredLaneV37=-650.f;
  V->FitGroundV35(Index==10?FVector(0,-1130,28):FVector(V==Wreck?WreckX:LiveX,V->DesiredLaneV37,0),0);}
 if(Index!=10&&!V8311)for(int32 I=0;I<40;++I){B->Elapsed+=.05f;Live->Simulate(.05f);}
 Wreck->ReceiveModuleHit(EFrontlineTankModule::Hull,Wreck->GetMaximumHullIntegrity()+1);
 if(Index==10&&(!Wreck->Body->GetStaticMesh()||Wreck->Body->GetStaticMesh()->GetName()!=TEXT("SM_RailConvoy_Wreck_V837")))return nullptr;
 // Wreck actors must continue their ordinary visual tick, including cooling.
 // This disposable still-life uses the normal outro cosmetics branch: the
 // fleet keeps feeding/cooling real wreck effects while live hulls stay put.
 B->BattleOutroWinner=0;
 Wreck->SetActorTickEnabled(true);if(Index!=10)Live->SetActorTickEnabled(true);return Live;
}
};
static AFrontlineTank* PrepareArmourArtV838(AWarfareFrontlineGameMode* B,int32 Index)
{return FFrontlineArmourArtV838Review::Prepare(B,Index);}
