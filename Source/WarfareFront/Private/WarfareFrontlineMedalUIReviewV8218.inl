namespace MedalUIReviewV8218
{
void Checks(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,AWarfareFrontlineHUD* HUD,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;const bool Safe=B&&PC&&HUD&&B->IsProgressionVolatile()&&FrontlineUltimateArchiveV8215::ReviewUserDir(Dir)&&FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review"));
 Check(TEXT("v8218_medal_ui_exact_disposable_scope"),Safe);if(!Safe)return;
 Check(TEXT("v8218_locked_medal_uses_dark_alpha_material_namespace"),Artwork().MedalLockedMaterialV8217&&Artwork().MedalLockedMaterialV8217->GetPathName()==TEXT("/Game/Warfare/UI/V8218/M_MedalLocked_V8218.M_MedalLocked_V8218"));
 for(int32 N=0;N<4;++N)
 {
  const auto Items=FrontlineMedalsV8215::NationIndices(EFRRUNationV81(N));bool Bounded=Items.Num()==13;
  for(int32 Slot=0;Slot<9;++Slot){const auto Cut=MedalWallV8215::RibbonTextileRectV8218(N,Slot);Bounded&=Cut.Min.X>=0&&Cut.Min.Y>=0&&Cut.Max.X<=724&&Cut.Max.Y<=241&&Cut.Width()>600&&Cut.Height()>130;}
  Check(FString::Printf(TEXT("v8218_nation%d_actual_textile_cuts_remove_gutters"),N),Bounded);
 }
 bool Touching=true,Fits=true;
 for(int32 Count=1;Count<=9;++Count)for(bool Centre:{false,true})
 {
  const auto Wall=MedalWallV8215::RibbonBarLayoutV8218(Count,126,516,949,60,Centre),Room=MedalWallV8215::RibbonBarLayoutV8218(Count,1451,82,400,42,Centre);
  for(const auto* Row:{&Wall,&Room})for(int32 K=0;K<Row->Num();++K)
  {const auto& Cell=(*Row)[K];Fits&=Cell.GetSize().X>0&&Cell.GetSize().Y>0;if(K>0&&FMath::IsNearlyEqual(float((*Row)[K-1].Min.Y),float(Cell.Min.Y),.001f))Touching&=FMath::IsNearlyEqual(float((*Row)[K-1].Max.X),float(Cell.Min.X),.001f);}
  Fits&=Wall.Num()==Count&&Room.Num()==Count&&Wall.Last().Max.X<=1075.001&&Room.Last().Max.X<=1851.001;
 }
 Check(TEXT("v8218_wall_and_warroom_ribbon_rows_touch_without_horizontal_gaps"),Touching&&Fits);
 FrontlineRibbonRackV8222::RunChecks(Check);
 const auto MenuBounds=TrenchControlBounds(FVector2D(700,460));const FVector2D Centre(MenuBounds.X+MenuBounds.W*.5f,MenuBounds.Y+MenuBounds.H*.5f),Neighbour(MenuBounds.X+MenuBounds.W+8,MenuBounds.Y+MenuBounds.H*.5f);
 Check(TEXT("v8218_trench_panel_claim_is_actual_rect_not_retention_corridor"),FieldHoverV8218::Contains(Centre,MenuBounds.X,MenuBounds.Y,MenuBounds.W,MenuBounds.H)&&!FieldHoverV8218::Contains(Neighbour,MenuBounds.X,MenuBounds.Y,MenuBounds.W,MenuBounds.H));
}
// Called once with the actual production Canvas during the isolated battle
// fixture. Projection, real field-site state and normal hit geometry are used.
void WithCanvas(FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,float BattlefieldBottom,TFunctionRef<void(const FString&,bool)> Check)
{
 FString Dir;if(!B||!PC||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV8218Review"))||!FrontlineUltimateArchiveV8215::ReviewUserDir(Dir))return;
 const auto Mouse=UI.Mouse;const int32 Target=PC->SupportTargetMode;ON_SCOPE_EXIT{UI.Mouse=Mouse;PC->SupportTargetMode=Target;};PC->SupportTargetMode=0;
 int32 Tested=0;bool Detected=true;
 for(const auto& Site:FrontlineFieldSitesV59::Views(B))
 {
  FVector2D P;if(!FieldHoverV8218::SitePoint(UI,PC,Site,BattlefieldBottom,P))continue;UI.Mouse=P;Detected&=FieldHoverV8218::HasFieldTarget(UI,B,PC,BattlefieldBottom);++Tested;
 }
 Check(TEXT("v8218_real_projected_facility_centres_are_hover_targets"),Tested>0&&Detected);
 const FVector2D Anchor(700,460);const auto MenuBounds=TrenchControlBounds(Anchor);UI.Mouse=FVector2D(MenuBounds.X+40,MenuBounds.Y+35);const bool MenuHit=FieldHoverV8218::PanelOwnsPointer(UI,Anchor);
 UI.Mouse=FVector2D(MenuBounds.X+MenuBounds.W+8,MenuBounds.Y+35);const bool OutsideHit=FieldHoverV8218::PanelOwnsPointer(UI,Anchor);
 Check(TEXT("v8218_actual_canvas_menu_claim_does_not_cover_neighbouring_facility"),MenuHit&&!OutsideHit);
 PC->SupportTargetMode=1;Check(TEXT("v8218_support_targeting_suppresses_passive_facility_hover"),!FieldHoverV8218::HasFieldTarget(UI,B,PC,BattlefieldBottom));
}
}
