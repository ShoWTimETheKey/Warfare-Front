namespace MedalUIReviewV8217
{
void Checks(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,AWarfareFrontlineHUD* HUD,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&PC&&HUD&&B->IsProgressionVolatile()&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir);
 Check(TEXT("v8217_decoration_ui_disposable_boundary"),Safe);if(!Safe)return;
 const auto Before=MedalWallV8215::View(PC);ON_SCOPE_EXIT{MedalWallV8215::View(PC)=Before;};
 Check(TEXT("v8217_picker_text_has_twelve_pixel_bottom_padding"),MedalMountV8216::CardHeightV8217-MedalMountV8216::GradeTopV8217-MedalMountV8216::GradeHeightV8217>=12);
 Check(TEXT("v8217_picker_text_insets_and_six_pixel_line_gap"),MedalMountV8216::TextInsetV8217>=17&&MedalMountV8216::GradeTopV8217-MedalMountV8216::NameTopV8217-MedalMountV8216::NameHeightV8217>=6);
 Check(TEXT("v8217_locked_medal_alpha_material_loaded"),Artwork().MedalLockedMaterialV8217!=nullptr);
 bool Textures=true,Atlases=true;static const TCHAR* Code[]={TEXT("uk"),TEXT("de"),TEXT("fr"),TEXT("ru")};
 for(int32 N=0;N<4;++N)
 {
  const auto Items=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(N));Check(FString::Printf(TEXT("v84412_nation_%d_thirteen_distinct_categories"),N),Items.Num()==13);
  const FString Stem=FString(TEXT("T_RibbonAtlas_"))+Code[N]+TEXT("_V8217");auto* Atlas=FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8217/")+Stem+TEXT(".")+Stem);Atlases&=FrontlineTextureReady(Atlas)&&Atlas->GetSizeX()==3*Atlas->GetSizeY();
  for(int32 Slot=0;Slot<9;++Slot){const float U=float(Slot%3)/3,V=float(Slot/3)/3;Check(FString::Printf(TEXT("v8217_ribbon_%d_%d_exact_uv_in_bounds"),N,Slot),U>=0&&V>=0&&U+1.f/3<=1.0001f&&V+1.f/3<=1.0001f);}
  MedalWallV8215::Hit(PC,FName(*FString::Printf(TEXT("V8215MedalNation%d"),N)));MedalWallV8215::View(PC).Wall=true;
  const auto Catalogue=MedalWallV8215::Items(EFRRUNationV81(N));
  MedalWallV8215::Wheel(PC,20);Check(FString::Printf(TEXT("v8217_nation_%d_lower_last_row_reachable"),N),MedalWallV8215::View(PC).Row==FMath::Max(0,(Catalogue.Num()+2)/3-2));MedalWallV8215::Wheel(PC,-20);
  const auto Forms=MedalWallV8215::Exhibits(EFRRUNationV81(N));int32 Expected=0;for(int32 I:Items)Expected+=FMath::Min(FrontlineMedalsV8215::Tier(I),FrontlineMedalsV8215::Variants(I).Num());
  const auto Deed=MedalWallV8215::Deed(MedalWallV8215::DeedBaseV8222+N);if(Deed.Available&&Deed.Distinctions>0)++Expected;
  Check(FString::Printf(TEXT("v8217_nation_%d_all_earned_classes_exhibited"),N),Forms.Num()==Expected&&!Forms.Contains(FIntPoint(4,1)));
  for(const auto F:Forms){Textures&=FrontlineTextureReady(MedalWallV8215::Art(F.X,F.Y));MedalWallV8215::Hit(PC,FName(*FString::Printf(TEXT("V8217MedalExhibit%d_%d"),F.X,F.Y)));const auto& S=MedalWallV8215::View(PC);Check(FString::Printf(TEXT("v8217_exhibit_%d_%d_opens_exact_real_class"),F.X,F.Y),S.Inspect&&S.Focus==F.X&&S.PreviewGrade==F.Y);MedalWallV8215::Back(PC);}
  MedalWallV8215::Hit(PC,TEXT("V8217MedalExhibit4_1"));Check(FString::Printf(TEXT("v8217_nation_%d_legacy_virtual_class_not_selectable"),N),!MedalWallV8215::View(PC).Inspect);
  if(N>=2)Check(FString::Printf(TEXT("v8217_nation_%d_no_paused_campaign_awards"),N),Forms.IsEmpty()&&!FrontlineMedalsV8215::Available(EFRRUNationV81(N)));
 }
 Check(TEXT("v8217_earned_exhibit_variants_ready"),Textures);Check(TEXT("v8217_all_four_ribbon_atlases_ready_exact_three_to_one"),Atlases);
 auto* Wall=FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V8217/T_MedalDisplayWall_V8217.T_MedalDisplayWall_V8217"));Check(TEXT("v8217_display_wall_ready_exact_three_to_one"),FrontlineTextureReady(Wall)&&Wall->GetSizeX()==3*Wall->GetSizeY());
}
}
