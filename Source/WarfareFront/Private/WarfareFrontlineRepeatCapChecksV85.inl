// Production evaluator and display projections; local maps only, no accounts.
void RunRepeatCapChecksV85(TFunctionRef<void(const FString&,bool)> Check)
{
 FStats Career;Career.Infantry=Career.Armour=Career.Victories=Career.CleanVictories=1000000;Career.NaturalRoute=3;
 FProof P;P.Nation=0;P.Date=19180603;P.Difficulty=2;P.Won=P.Completed=P.Undefeated=true;
 P.SupportArmourV84412=2;P.AircraftArmourV84412=1;P.AircraftInfantryV84412=24;
 auto Same=[](const FAward& A,const FAward& B){return A.Grade==B.Grade&&A.Repeats==B.Repeats&&A.Attachments==B.Attachments
  &&A.Date==B.Date&&A.Campaign==B.Campaign&&A.CampaignDeeds==B.CampaignDeeds;};
 for(int32 I:{0,1,2,12,13,37,38,39})
 {
  const int32 Limit=RepeatLimitV85(I);TMap<FString,FAward> Awards;bool Boundary=true;
  FAward Maximum;
  for(int32 N=0;N<=Limit+2;++N)
  {
   P.Campaign=FGuid::NewGuid();ApplyAwards(Awards,Career,P);const auto A=Awards.FindRef(Definition(I).Id);
   Boundary&=A.Grade==1&&A.Repeats==FMath::Min(N,Limit)&&ValidAward(I,A);
   if(N==Limit)Maximum=A;if(N>Limit)Boundary&=Same(A,Maximum);
  }
  Check(FString::Printf(TEXT("v85_repeat_medal%d_base_to_ceiling_and_extra_deeds_stop_without_mutation"),I),Boundary);
  Check(FString::Printf(TEXT("v85_repeat_medal%d_at_ceiling_has_no_further_progress_target"),I),NextTargetForAwardV85(I,Maximum)==0);
  bool Projection=true;
  for(int32 R=0;R<=255;++R)
  {
   auto Raw=Maximum;Raw.Repeats=uint8(R);Raw.Attachments=R?uint16(EAttachment::Bar):0;const auto Before=Raw;
   const auto Display=PresentedAwardV85(I,Raw);const auto States=InspectionStatesV84413(I,Raw);
   Projection&=Same(Raw,Before)&&Display.Repeats==FMath::Min(R,Limit)&&States.Num()==Limit+1;
   for(int32 K=0;K<States.Num();++K)Projection&=States[K].Form.Grade==1&&States[K].Form.Repeats==K
    &&States[K].Earned==(K<=FMath::Min(R,Limit));
  }
  Check(FString::Printf(TEXT("v85_repeat_medal%d_all_256_raw_counts_project_to_complete_sourced_game_preview"),I),Projection);
  if(I==2)continue;
  bool Legacy=true;
  for(int32 R:{6,9,255})
  {
   auto Raw=Maximum;Raw.Repeats=uint8(R);Raw.Date=TEXT("2026-10-09T00:00:00Z");const auto Before=Raw;
   Awards.Add(Definition(I).Id,Raw);P.Campaign=FGuid::NewGuid();ApplyAwards(Awards,Career,P);
   const auto After=Awards.FindRef(Definition(I).Id);
   Legacy&=ValidAward(I,After)&&Same(Before,After)&&PresentedAwardV85(I,After).Repeats==Limit
    &&NextTargetForAwardV85(I,After)==0&&InspectionLabelV84413(I,After,false)==FString::Printf(TEXT("%d AWARD BAR%s"),Limit,Limit>1?TEXT("S"):TEXT(""));
  }
  Check(FString::Printf(TEXT("v85_repeat_medal%d_legacy_over_cap_bytes_remain_valid_and_unchanged"),I),Legacy);
 }
 Check(TEXT("v85_repeat_cap_does_not_create_french_or_russian_natural_awards"),!Available(EFRRUNationV81::French)&&!Available(EFRRUNationV81::Russian));
}
