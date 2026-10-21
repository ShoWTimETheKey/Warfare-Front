// Included inside FrontlineMedalsV8215. This is a museum projection only;
// FAward persistence and the receipt-bound natural evaluator remain unchanged.
TArray<FInspectionStateV84413> InspectionStatesV84413(int32 I,const FAward& Recorded)
{
 TArray<FInspectionStateV84413> Result;
 if(I<0||I>=Count||I==4)return Result;
 const auto Forms=Variants(I);if(Forms.IsEmpty())return Result;
 auto Add=[&](int32 G,int32 Repeats,uint16 Devices)
 {
  FInspectionStateV84413 S;S.Form.Grade=uint8(G);S.Form.Repeats=uint8(Repeats);S.Form.Attachments=Devices;
  S.Earned=Recorded.Grade>=G&&Recorded.Repeats>=Repeats&&(Devices&~Recorded.Attachments)==0;
  Result.Add(MoveTemp(S));
 };
 if(RepeatBars(I))
 {
  // Exhibition access is independent of qualification. Keep the complete
  // sourced game range available before the first award; preview never
  // changes the saved class, repeats or qualifying deeds.
  const int32 Last=RepeatLimitV85(I);
  for(int32 R=0;R<=Last;++R)Add(1,R,R?uint16(EAttachment::Bar):0);
  return Result;
 }
 if(I==5||I==17)
 {Add(1,0,0);Add(1,0,uint16(EAttachment::OakLeaves));return Result;}
 if(I==6)
 {
  // This save records citation presence, not its authority or count. The
  // four supported forms are always inspectable, including before any award.
  Add(1,0,0);Add(1,0,uint16(EAttachment::Star));Add(1,0,uint16(EAttachment::Palm));
  Add(1,0,uint16(EAttachment::Star)|uint16(EAttachment::Palm));
  return Result;
 }
 for(int32 G=1;G<=Forms.Num();++G)
 {
  // Qualification devices (dated clasps, swords, bows) are not repeat levels.
  // Only the actual class retains the actual optional qualification record.
  Add(G,0,Recorded.Grade==G?Recorded.Attachments:0);
 }
 return Result;
}
FString InspectionLabelV84413(int32 I,const FAward& Recorded,bool ZH)
{
 const auto A=PresentedAwardV85(I,Recorded);
 const auto Forms=Variants(I);if(!Forms.IsValidIndex(int32(A.Grade)-1))return {};
 if(RepeatBars(I))return A.Repeats?
  (ZH?FString::Printf(TEXT("复授 · %d道横杠"),A.Repeats):FString::Printf(TEXT("%d AWARD BAR%s"),A.Repeats,A.Repeats>1?TEXT("S"):TEXT(""))):
  (ZH?TEXT("首次授予"):TEXT("FIRST AWARD"));
 if(I==5)return A.Attachments&uint16(EAttachment::OakLeaves)?(ZH?TEXT("佩橡叶"):TEXT("WITH OAK LEAVES")):(ZH?TEXT("原章"):TEXT("ORIGINAL INSIGNIA"));
 if(I==17)return A.Attachments&uint16(EAttachment::OakLeaves)?(ZH?TEXT("通报嘉奖 · 佩橡叶"):TEXT("MENTION IN DESPATCHES")):(ZH?TEXT("原章"):TEXT("ORIGINAL INSIGNIA"));
 if(I==6)
 {
  const bool Star=A.Attachments&uint16(EAttachment::Star),Palm=A.Attachments&uint16(EAttachment::Palm);
  return Star&&Palm?(ZH?TEXT("星式与棕榈枝嘉奖"):TEXT("STAR AND PALM CITATIONS")):
   Star?(ZH?TEXT("星式嘉奖"):TEXT("STAR CITATION")):Palm?(ZH?TEXT("棕榈枝嘉奖"):TEXT("PALM CITATION")):(ZH?TEXT("章体"):TEXT("CROSS"));
 }
 return ZH?Forms[A.Grade-1].NameZH:Forms[A.Grade-1].NameEN;
}
void RunInspectionChecksV84413(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Stable=true,Bounded=true,OnlyClasses=true,NoLeak=true;
 for(int32 I=0;I<Count;++I)
 {
  const auto Empty=InspectionStatesV84413(I,FAward{});Stable&=I==4?Empty.IsEmpty():!Empty.IsEmpty();
  for(const auto& S:Empty){Bounded&=Empty.Num()<=MaximumRepeatsV85+1&&S.Form.Repeats<=RepeatLimitV85(I);OnlyClasses&=S.Form.Grade>=1&&S.Form.Grade<=Variants(I).Num();NoLeak&=!S.Earned;}
 }
 Check(TEXT("v84413_all_53_stable_indices_keep_52_real_inspectable_categories"),Stable);
 Check(TEXT("v84413_preview_forms_use_real_classes_and_bounded_repeat_counts"),Bounded&&OnlyClasses);
 Check(TEXT("v84413_unawarded_preview_never_becomes_earned"),NoLeak);
 FAward A;A.Grade=1;A.Repeats=3;A.Attachments=uint16(EAttachment::Bar);
 bool Bars=true;for(int32 I:{0,1,12,13,37,38,39})
 {
  const auto States=InspectionStatesV84413(I,A);Bars&=States.Num()==RepeatLimitV85(I)+1;
  for(int32 R=0;R<States.Num();++R)Bars&=States[R].Form.Grade==1&&States[R].Form.Repeats==R&&States[R].Earned==(R<=3);
 }
 Check(TEXT("v84413_all_british_repeat_bars_share_one_preview_route"),Bars);
 const auto VC=InspectionStatesV84413(2,A);Check(TEXT("v84413_victoria_cross_keeps_only_known_single_repeat_bar"),VC.Num()==2&&VC[1].Form.Repeats==1);
 A.Repeats=0;A.Attachments=uint16(EAttachment::OakLeaves);
 const auto PLM=InspectionStatesV84413(5,A),Victory=InspectionStatesV84413(17,A);
 Check(TEXT("v84413_oak_leaf_distinctions_keep_single_real_grade"),PLM.Num()==2&&Victory.Num()==2&&PLM[1].Earned&&Victory[1].Earned&&PLM[1].Form.Grade==1);
 A.Attachments=uint16(EAttachment::Palm);const auto Croix=InspectionStatesV84413(6,A);
 Check(TEXT("v84413_citation_preview_does_not_invent_metals_counts_or_classes"),Croix.Num()==4&&Croix[2].Earned&&!Croix[1].Earned&&!Croix[3].Earned&&Croix[2].Form.Repeats==0&&Croix[2].Form.Grade==1);
 A.Grade=2;A.Attachments=0;const auto Iron=InspectionStatesV84413(3,A);
 Check(TEXT("v84413_real_class_orders_keep_existing_sequence"),Iron.Num()==3&&Iron[0].Earned&&Iron[1].Earned&&!Iron[2].Earned&&Iron[1].Form.Grade==2);
 Check(TEXT("v84413_single_award_medals_do_not_gain_fictional_levels"),InspectionStatesV84413(40,A).Num()==1&&InspectionStatesV84413(45,A).Num()==1&&InspectionStatesV84413(52,A).Num()==1);
 A.Grade=1;A.Repeats=255;A.Attachments=uint16(EAttachment::Bar);const auto Maximum=InspectionStatesV84413(13,A);
 Check(TEXT("v85_legacy_255_record_projects_to_sourced_mm_form_without_rewriting_raw"),Maximum.Num()==RepeatLimitV85(13)+1&&Maximum.Last().Form.Repeats==RepeatLimitV85(13)&&Maximum.Last().Earned&&A.Repeats==255
  &&PresentedAwardV85(13,A).Repeats==RepeatLimitV85(13)&&InspectionLabelV84413(13,A,false)==TEXT("3 AWARD BARS"));
 Check(TEXT("v84413_inspection_does_not_reopen_french_or_russian_natural_awards"),!Available(EFRRUNationV81::French)&&!Available(EFRRUNationV81::Russian));
}
