// Included in FrontlineMedalsV8215. No Load/Save/account access: exercise the
// actual award evaluator against distinct local battle proofs and award maps.
void RunAwardChecksV84414(TFunctionRef<void(const FString&,bool)> Check)
{
 RunGermanAwardChecksV85(Check);
 RunRepeatCapChecksV85(Check);
 FStats EmptyCareer;FProof P;P.Nation=0;P.Date=19180603;P.Difficulty=1;
 P.Won=true;P.Campaign=FGuid::NewGuid();P.SupportArmourV84412=2;P.AircraftArmourV84412=1;
 auto For=[&](int32 I,const FProof& Proof,const FStats& Career=FStats{})
 {TMap<FString,FAward> A;ApplyAwards(A,Career,Proof);return A.FindRef(Definition(I).Id);};
 const auto Service=For(37,P),Flying=For(39,P);
 Check(TEXT("v84414_service_cross_first_award_is_one_actual_battle_deed"),Service.Grade==1&&Service.Repeats==0&&Service.Attachments==0&&ValidAward(37,Service));
 Check(TEXT("v84414_flying_medal_first_award_is_one_actual_aircraft_deed"),Flying.Grade==1&&Flying.Repeats==0&&Flying.Attachments==0&&ValidAward(39,Flying));
 TMap<FString,FAward> A;ApplyAwards(A,EmptyCareer,P);
 const auto FirstService=A.FindRef(Definition(37).Id),FirstFlying=A.FindRef(Definition(39).Id);
 bool Repeated=true;
 for(int32 N=1;N<=5;++N)
 {
  P.Campaign=FGuid::NewGuid();ApplyAwards(A,EmptyCareer,P);
  const auto S=A.FindRef(Definition(37).Id),F=A.FindRef(Definition(39).Id);
  Repeated&=S.Grade==1&&F.Grade==1&&S.Repeats==FMath::Min(N,RepeatLimitV85(37))&&F.Repeats==FMath::Min(N,RepeatLimitV85(39))
   &&S.Attachments==uint16(EAttachment::Bar)&&F.Attachments==uint16(EAttachment::Bar)
   &&ValidAward(37,S)&&ValidAward(39,F)&&S.Date==FirstService.Date&&F.Date==FirstFlying.Date;
 }
 Check(TEXT("v84414_each_further_qualifying_battle_adds_one_real_bar_without_career_grind"),Repeated);
 const auto SavedService=A.FindRef(Definition(37).Id),SavedFlying=A.FindRef(Definition(39).Id);
 const auto Retained=[&]()
 {
  const auto S=A.FindRef(Definition(37).Id),F=A.FindRef(Definition(39).Id);
  return S.Grade==SavedService.Grade&&S.Repeats==SavedService.Repeats&&S.Attachments==SavedService.Attachments
   &&S.Date==SavedService.Date&&S.Campaign==SavedService.Campaign
   &&F.Grade==SavedFlying.Grade&&F.Repeats==SavedFlying.Repeats&&F.Attachments==SavedFlying.Attachments
   &&F.Date==SavedFlying.Date&&F.Campaign==SavedFlying.Campaign;
 };
 FProof Fail=P;Fail.Won=false;ApplyAwards(A,EmptyCareer,Fail);
 Check(TEXT("v84414_failed_battle_neither_awards_nor_revokes_existing_bars"),For(37,Fail).Grade==0&&For(39,Fail).Grade==0&&Retained());
 Fail=P;Fail.Difficulty=0;ApplyAwards(A,EmptyCareer,Fail);
 Check(TEXT("v84414_recruit_difficulty_does_not_add_bar_and_keeps_old_awards"),For(37,Fail).Grade==0&&For(39,Fail).Grade==0&&Retained());
 Fail=P;Fail.SupportArmourV84412=1;Fail.AircraftArmourV84412=0;FStats Huge;Huge.Armour=1000000;
 ApplyAwards(A,Huge,Fail);
 Check(TEXT("v84414_large_career_counts_cannot_replace_this_battles_support_and_aircraft_kills"),For(37,Fail,Huge).Grade==0&&For(39,Fail,Huge).Grade==0&&Retained());
 Fail=P;Fail.Date=19180602;
 Check(TEXT("v84414_flying_medal_is_closed_before_3_june_1918"),For(39,Fail).Grade==0&&For(37,Fail).Grade==1);
 Check(TEXT("v84414_flying_medal_is_open_from_3_june_1918"),For(39,P).Grade==1);
 Fail=P;Fail.Date=19181112;
 Check(TEXT("v84414_both_deeds_remain_inside_the_great_war_date_window"),For(37,Fail).Grade==0&&For(39,Fail).Grade==0);
 Fail=P;Fail.Nation=1;
 Check(TEXT("v84414_german_support_deeds_do_not_grant_british_decorations"),For(37,Fail).Grade==0&&For(39,Fail).Grade==0);
 for(int32 N:{2,3})
 {Fail=P;Fail.Nation=N;Check(FString::Printf(TEXT("v84414_paused_nation%d_cannot_naturally_award_british_decorations"),N),For(37,Fail).Grade==0&&For(39,Fail).Grade==0);}
 for(int32 I:{37,39})
 {
  FAward Legacy=I==37?FirstService:FirstFlying;Legacy.Repeats=9;Legacy.Attachments=uint16(EAttachment::Bar);
  Legacy.Date=TEXT("2026-10-09T00:00:00Z");TMap<FString,FAward> Old;Old.Add(Definition(I).Id,Legacy);
  ApplyAwards(Old,EmptyCareer,P);const auto Now=Old.FindRef(Definition(I).Id);
  Check(FString::Printf(TEXT("v85_medal%d_existing_nine_bars_stay_valid_raw_and_present_with_sourced_boundary"),I),Now.Grade==1&&Now.Repeats==9&&PresentedAwardV85(I,Now).Repeats==RepeatLimitV85(I)&&Now.Date==Legacy.Date&&Now.Campaign==Legacy.Campaign&&ValidAward(I,Now));
  Legacy.Repeats=255;Old.Add(Definition(I).Id,Legacy);ApplyAwards(Old,EmptyCareer,P);
  Check(FString::Printf(TEXT("v84414_medal%d_maximum_repeat_does_not_wrap"),I),Old.FindRef(Definition(I).Id).Repeats==255&&ValidAward(I,Old.FindRef(Definition(I).Id)));
  Check(FString::Printf(TEXT("v84414_medal%d_progress_does_not_invent_a_cumulative_target"),I),TargetForStage(I,0)==0&&TargetForStage(I,10)==0&&Definition(I).Threshold[0]==0&&Definition(I).Threshold[1]==0&&Definition(I).Threshold[2]==0);
 }
}
