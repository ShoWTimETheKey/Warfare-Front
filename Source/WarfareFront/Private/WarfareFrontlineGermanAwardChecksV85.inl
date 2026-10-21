// Pure production-evaluator checks. No game actors, account access or player writes.
void RunGermanAwardChecksV85(TFunctionRef<void(const FString&,bool)> Check)
{
 FStats Career;Career.Infantry=1000000;Career.Armour=1000000;Career.Victories=1000000;Career.CleanVictories=1000000;
 FProof P;P.Nation=1;P.Date=19171120;P.Difficulty=1;P.Won=true;P.Campaign=FGuid::NewGuid();
 auto AwardFor=[&](int32 I,const FProof& Proof,const FStats& Stats=FStats{})
 {TMap<FString,FAward> Map;ApplyAwards(Map,Stats,Proof);return Map.FindRef(Definition(I).Id);};
 bool NoCounterSubstitute=true;
 for(int32 I:{18,19,20,21,22,23,41,42,43,44})NoCounterSubstitute&=AwardFor(I,P,Career).Grade==0;
 Check(TEXT("v85_german_large_career_totals_cannot_forge_current_battle_deeds"),NoCounterSubstitute);
 FProof D=P;D.Infantry=60;
 Check(TEXT("v85_hohenzollern_sixty_current_battle_kills_qualify"),AwardFor(18,D).Grade==1);
 D.Infantry=59;Check(TEXT("v85_hohenzollern_below_sixty_does_not_qualify"),AwardFor(18,D,Career).Grade==0);
 D.Infantry=120;D.FriendlyFire=1;Check(TEXT("v85_hohenzollern_friendly_fire_blocks_even_high_kills"),AwardFor(18,D).Grade==0);
 D=P;D.Infantry=24;D.Obstacles=3;
 Check(TEXT("v85_bavarian_cross_actual_clearance_assault_qualifies"),AwardFor(19,D).Grade==1);
 D.Obstacles=2;Check(TEXT("v85_bavarian_cross_career_cannot_replace_clearance"),AwardFor(19,D,Career).Grade==0);
 D=P;D.Armour=2;D.ArmourLosses=1;FStats ArmourCareer;ArmourCareer.Armour=12;
 Check(TEXT("v85_bavarian_order_real_armour_deed_allows_armour_losses"),AwardFor(20,D,ArmourCareer).Grade==1);
 D.Armour=1;Check(TEXT("v85_bavarian_order_career_cannot_replace_second_actual_kill"),AwardFor(20,D,Career).Grade==0);
 D=P;D.Completed=D.Undefeated=true;D.Difficulty=2;TMap<FString,FAward> Joseph;bool Campaigns=true;
 for(int32 G=1;G<=3;++G)
 {D.Campaign=FGuid::NewGuid();ApplyAwards(Joseph,FStats{},D);auto A=Joseph.FindRef(Definition(21).Id);Campaigns&=A.Grade==G&&A.CampaignDeeds.Num()==G&&ValidAward(21,A);ApplyAwards(Joseph,FStats{},D);Campaigns&=Joseph.FindRef(Definition(21).Id).Grade==G;}
 Check(TEXT("v85_max_joseph_three_distinct_undefeated_campaigns_no_clean_win_grind"),Campaigns);
 D.Replay=true;Check(TEXT("v85_max_joseph_replay_cannot_create_campaign_award"),AwardFor(21,D,Career).Grade==0);
 D=P;D.Infantry=40;D.EnteredInfantryV842=100;D.ReturnedInfantryV842=75;TMap<FString,FAward> Henry;bool Preservation=true;
 for(int32 G=1;G<=4;++G)
 {D.ReturnedInfantryV842=70+G*5;ApplyAwards(Henry,FStats{},D);const auto A=Henry.FindRef(Definition(22).Id);Preservation&=A.Grade==G&&ValidAward(22,A);if(G<4){--D.ReturnedInfantryV842;ApplyAwards(Henry,Career,D);Preservation&=Henry.FindRef(Definition(22).Id).Grade==G;}}
 Check(TEXT("v85_st_henry_four_classes_allow_losses_with_actual_roster_percentages"),Preservation);
 D=P;D.Infantry=40;D.EnteredInfantryV842=0;D.ReturnedInfantryV842=24;
 Check(TEXT("v85_st_henry_missing_entered_roster_cannot_forge_survival"),AwardFor(22,D,Career).Grade==0);
 D=P;D.Armour=2;D.MinimumMorale=.249f;
 Check(TEXT("v85_hanseatic_recovery_is_current_battle_armour_deed"),AwardFor(23,D).Grade==1);
 D.MinimumMorale=.25f;Check(TEXT("v85_hanseatic_morale_boundary_is_strict"),AwardFor(23,D,Career).Grade==0);
 D=P;D.Infantry=48;D.MinimumMorale=.299f;
 Check(TEXT("v85_baden_low_morale_infantry_deed_qualifies_without_career_total"),AwardFor(41,D).Grade==1);
 D.MinimumMorale=.30f;Check(TEXT("v85_baden_morale_boundary_is_strict"),AwardFor(41,D,Career).Grade==0);
 D=P;D.Infantry=60;D.Obstacles=5;TMap<FString,FAward> Crosses;ApplyAwards(Crosses,FStats{},D);ApplyAwards(Crosses,FStats{},D);
 Check(TEXT("v85_mecklenburg_two_real_assault_classes_no_victory_count_grind"),Crosses.FindRef(Definition(42).Id).Grade==2);
 D=P;D.Armour=100;Check(TEXT("v85_oldenburg_total_armour_cannot_replace_support_source"),AwardFor(43,D,Career).Grade==0);
 D.SupportArmourV84412=2;Crosses.Empty();ApplyAwards(Crosses,FStats{},D);ApplyAwards(Crosses,FStats{},D);
 Check(TEXT("v85_oldenburg_two_real_support_classes_no_clean_armour_grind"),Crosses.FindRef(Definition(43).Id).Grade==2);
 D=P;D.Difficulty=2;D.Infantry=80;D.Date=19170513;
 Check(TEXT("v85_hessian_actual_deed_is_closed_before_institution"),AwardFor(44,D,Career).Grade==0);
 D.Date=19170514;Check(TEXT("v85_hessian_actual_survivor_deed_opens_at_institution"),AwardFor(44,D).Grade==1);
 const FString Date=TEXT("2026-10-01T00:00:00Z");TMap<FString,FAward> Legacy;
 for(int32 I:NationIndices(EFRRUNationV81::German))Legacy.Add(Definition(I).Id,CompleteCheatAwardV8218(I,FGuid::NewGuid(),Date));
 const auto Before=Legacy;ApplyAwards(Legacy,Career,P);bool Retained=true;
 for(const auto& E:Before){const auto A=Legacy.FindRef(E.Key);Retained&=A.Grade==E.Value.Grade&&A.Repeats==E.Value.Repeats&&A.Attachments==E.Value.Attachments&&A.Date==E.Value.Date&&A.Campaign==E.Value.Campaign&&A.CampaignDeeds==E.Value.CampaignDeeds;}
 Check(TEXT("v85_all_thirteen_existing_german_classes_dates_and_deeds_remain_exact"),Retained);
 P.Won=false;bool Defeat=true;for(int32 I:NationIndices(EFRRUNationV81::German))Defeat&=AwardFor(I,P,Career).Grade==0;
 Check(TEXT("v85_german_revised_conditions_never_award_on_defeat"),Defeat);
 bool NoFakeProgress=true;for(int32 I:{18,19,21,22,23,41,42,43,44})NoFakeProgress&=Definition(I).Metric==6&&TargetForStage(I,0)==0&&StatValue(Definition(I),Career)==0;
 Check(TEXT("v85_german_current_battle_conditions_do_not_display_career_progress"),NoFakeProgress);
}
