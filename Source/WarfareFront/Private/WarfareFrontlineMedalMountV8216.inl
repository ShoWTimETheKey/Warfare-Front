namespace MedalMountV8216
{
#include "WarfareFrontlineMedalMountBoundsV8443.generated.inl"
#include "WarfareFrontlineMedalMountBoundsV84413.generated.inl"
struct FGeometryV8443
{
 FFrontlineImagePlacement Image{},Source{};
 float PivotX=0,PivotY=0;bool Valid=false,Cropped=false;
};
FGeometryV8443 GeometryV8443(const FString& Name,int32 SizeX,int32 SizeY,float X,float Y,float W,float H)
{
 FGeometryV8443 P;
 if(SizeX<=0||SizeY<=0||!FMath::IsFinite(X)||!FMath::IsFinite(Y)||!FMath::IsFinite(W)||!FMath::IsFinite(H)||W<=0||H<=0)return P;
 int32 L=0,T=0,R=SizeX,B=SizeY;
 for(const auto& Cut:SourceBoundsV8443)if(Name==Cut.Name)
 {
  // Unknown or differently imported textures retain the original full-UV
  // contain behaviour. Never apply native-pixel cuts to another source size.
  if(Cut.Width==SizeX&&Cut.Height==SizeY&&Cut.Left>=0&&Cut.Top>=0&&Cut.Right<=SizeX&&Cut.Bottom<=SizeY&&Cut.Left<Cut.Right&&Cut.Top<Cut.Bottom)
  {L=Cut.Left;T=Cut.Top;R=Cut.Right;B=Cut.Bottom;P.Cropped=true;}
  break;
 }
 for(const auto& Cut:SourceBoundsV84413)if(Name==Cut.Name)
 {
  if(Cut.Width==SizeX&&Cut.Height==SizeY&&Cut.Left>=0&&Cut.Top>=0&&Cut.Right<=SizeX&&Cut.Bottom<=SizeY&&Cut.Left<Cut.Right&&Cut.Top<Cut.Bottom)
  {L=Cut.Left;T=Cut.Top;R=Cut.Right;B=Cut.Bottom;P.Cropped=true;}break;
 }
 const float Factor=FMath::Min(W/float(R-L),H/float(B-T)),IW=(R-L)*Factor,IH=(B-T)*Factor;
 if(!FMath::IsFinite(Factor)||Factor<=0||!FMath::IsFinite(IW)||!FMath::IsFinite(IH))return FGeometryV8443();
 P.Image={X+(W-IW)*.5f,Y,IW,IH,L/float(SizeX),T/float(SizeY),(R-L)/float(SizeX),(B-T)/float(SizeY)};
 // Existing bars/leaves are authored relative to the original image canvas.
 // Keep that coordinate system, then rotate both layers about the visible top.
 P.Source={P.Image.X-L*Factor,P.Image.Y-T*Factor,SizeX*Factor,SizeY*Factor,0,0,1,1};
 P.PivotX=P.Image.X+P.Image.W*.5f;P.PivotY=P.Image.Y;
 P.Valid=FMath::IsFinite(P.Image.X)&&FMath::IsFinite(P.Image.Y)&&FMath::IsFinite(P.Source.X)&&FMath::IsFinite(P.Source.Y)
  &&FMath::IsFinite(P.Source.W)&&FMath::IsFinite(P.Source.H)&&FMath::IsFinite(P.PivotX)&&FMath::IsFinite(P.PivotY);
 return P.Valid?P:FGeometryV8443();
}
FGeometryV8443 GeometryV8443(const UTexture2D* T,float X,float Y,float W,float H)
{return T?GeometryV8443(T->GetName(),T->GetSizeX(),T->GetSizeY(),X,Y,W,H):FGeometryV8443();}
void RunGeometryChecksV8443(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Valid=true,Inside=true,UV=true,Canvas=true,Pivot=true,Rotation=true;
 auto Rotate=[](FVector2D Point,const FVector2D& Centre,float Angle)
 {
  const float A=FMath::DegreesToRadians(Angle),C=FMath::Cos(A),S=FMath::Sin(A);const auto D=Point-Centre;
  return Centre+FVector2D(D.X*C-D.Y*S,D.X*S+D.Y*C);
 };
 for(const auto& Cut:SourceBoundsV8443)for(const FVector2D Size:{FVector2D(92,138),FVector2D(180,154)})
 {
  const auto P=GeometryV8443(Cut.Name,Cut.Width,Cut.Height,914,8,Size.X,Size.Y);const auto& I=P.Image;const auto& O=P.Source;
  Valid&=P.Valid&&P.Cropped&&I.W>0&&I.H>0;
  Inside&=I.X>=913.999f&&I.Y==8&&I.X+I.W<=914+Size.X+.001f&&I.H<=Size.Y+.001f;
  UV&=I.U>=0&&I.V>=0&&I.UW>0&&I.VH>0&&I.U+I.UW<=1.00001f&&I.V+I.VH<=1.00001f;
  const FVector2D Centre(P.PivotX,P.PivotY);
  Pivot&=FMath::IsNearlyEqual(P.PivotX,float(914+Size.X*.5),.001f)&&P.PivotY==8;
  for(const FVector2D Fraction:{FVector2D(0,0),FVector2D(.5,0),FVector2D(.32,.15),FVector2D(.62,.40),FVector2D(1,1)})
  {
   const FVector2D A(O.X+O.W*Fraction.X,O.Y+O.H*Fraction.Y);
   const FVector2D B(I.X+(Fraction.X-I.U)/I.UW*I.W,I.Y+(Fraction.Y-I.V)/I.VH*I.H);
   Canvas&=A.Equals(B,.002f);
   for(float Angle:{-11.f,0.f,11.f})Rotation&=Rotate(A,Centre,Angle).Equals(Rotate(B,Centre,Angle),.003f)
    &&Rotate(Centre,Centre,Angle).Equals(Centre,.0001f);
  }
 }
 Check(TEXT("v8443_mount_all_74_native_crops_have_valid_geometry"),Valid);
 Check(TEXT("v8443_mount_and_picker_visible_crops_fit_and_share_top_centre"),Inside&&Pivot);
 Check(TEXT("v8443_mount_cropped_uvs_stay_inside_native_sources"),UV);
 Check(TEXT("v8443_mount_attachments_keep_original_canvas_coordinates"),Canvas);
 Check(TEXT("v8443_mount_body_and_devices_share_rigid_pivot_at_both_swing_limits"),Rotation);
 const auto Fallback=GeometryV8443(TEXT("CommonTextureWithoutMedalBounds"),512,256,914,8,92,138);
 const auto Changed=GeometryV8443(TEXT("T_Medal_de_iron_cross_ii_V8216"),512,256,914,8,92,138);
 Check(TEXT("v8443_mount_unknown_and_changed_size_textures_keep_full_uv_fallback"),Fallback.Valid&&!Fallback.Cropped&&Changed.Valid&&!Changed.Cropped
  &&Fallback.Image.U==0&&Fallback.Image.V==0&&Fallback.Image.UW==1&&Fallback.Image.VH==1&&Fallback.Image.W==92&&Fallback.Image.H==46
  &&Changed.Image.W==Fallback.Image.W&&Changed.Image.H==Fallback.Image.H);
 Check(TEXT("v8443_mount_invalid_sizes_produce_empty_geometry"),!GeometryV8443(TEXT(""),0,256,0,0,92,138).Valid
  &&!GeometryV8443(TEXT(""),512,256,0,0,0,138).Valid&&!GeometryV8443(TEXT(""),512,256,0,0,92,-1).Valid);
 const auto Chest=GeometryV8443(TEXT("T_Medal_de_bavarian_merit_cross_V8216"),1145,1374,914,8,92,138);
 const auto Neck=GeometryV8443(TEXT("T_Medal_de_hohenzollern_V8216_commander"),1242,1266,914,8,92,138);
 Check(TEXT("v8443_mount_chest_and_neck_forms_retain_different_natural_proportions"),Chest.Valid&&Neck.Valid
  &&Neck.Image.W>Chest.Image.W+15&&Chest.Image.H>Neck.Image.H+5);
}
struct FView
{
 bool Picker=false,Loaded=false,OwnsPauseV836=false;int32 Nation=0,ScrollNationV8447=INDEX_NONE;FString Choice[4];
 MedalWallV8215::FScrollV842 CatalogueV8447;
 uint64 ShockSerial=0,LastFrame=MAX_uint64;FGuid Encounter;FVector LastCamera=FVector::ZeroVector;float LastSpeed=0;bool CameraSeen=false;
 FrontlineMedalMotionV8216::FPendulum Motion;
};
inline constexpr float CardHeightV8217=250,NameTopV8217=172,NameHeightV8217=40,GradeTopV8217=218,GradeHeightV8217=20,TextInsetV8217=17;
namespace PickerLayoutV8447
{
 constexpr int32 Columns=4;
 constexpr float X=414,Top=275,Right=1512,Bottom=815,Width=258,Height=250,StrideX=280,StrideY=270;
 float Maximum(int32 Count){return FMath::Max(0.f,((FMath::Max(0,Count)+Columns-1)/Columns-1)*StrideY+Height-(Bottom-Top));}
 FBox2D Cell(int32 Index,float Offset)
 {const FVector2D P(X+(Index%Columns)*StrideX,Top+(Index/Columns)*StrideY-Offset);return FBox2D(P,P+FVector2D(Width,Height));}
 float Reveal(int32 Index,int32 Count)
 {return Index<0?0.f:FMath::Clamp((Index/Columns)*StrideY+Height-(Bottom-Top),0.f,Maximum(Count));}
}
TMap<TWeakObjectPtr<APlayerController>,FView> Views;
FView& View(APlayerController* P){return Views.FindOrAdd(P);}
void Load(FView& S)
{
 if(S.Loaded)return;S.Loaded=true;if(!GConfig)return;
 for(int32 N=0;N<4;++N)GConfig->GetString(TEXT("Warfare.Decorations"),*FString::Printf(TEXT("MountedMedal%d"),N),S.Choice[N],GGameUserSettingsIni);
}
TArray<int32> HangableGrades(int32 I)
{
 switch(I){
 case 3:return {1,3};case 18:return {1,2};case 19:return {1,2,3};case 20:return {1,2,4};case 21:case 22:case 24:return {1,2};
 case 8:return {1,2,3};case 9:case 10:return {1,2,3,4};case 11:return {1,2};case 31:return {2,3};case 32:case 33:return {1,2};
 case 30:case 34:case 35:case 36:case 44:case 49:case 51:return {};case 48:return {1,2,3};case 50:return {1,2};default:return {1};
 }
}
int32 DefaultGrade(int32 I){int32 G=0;for(int32 K:HangableGrades(I))if(K<=FrontlineMedalsV8215::Tier(I))G=K;return G;}
int32 SelectedGrade(const FView& S)
{
 FString Id,Grade;S.Choice[S.Nation].Split(TEXT("|"),&Id,&Grade);
 const int32 I=FrontlineMedalsV8215::IndexForId(Id.IsEmpty()?S.Choice[S.Nation]:Id);
 return Grade.IsNumeric()?FCString::Atoi(*Grade):DefaultGrade(I);
}
int32 Chosen(FView& S,AWarfareFrontlineGameMode* B)
{
 Load(S);if(!B)return INDEX_NONE;S.Nation=FMath::Clamp(int32(WarfareFRRUCampaignV82::Nation(B,0)),0,3);
 FString Id,Grade;S.Choice[S.Nation].Split(TEXT("|"),&Id,&Grade);
 const int32 I=FrontlineMedalsV8215::IndexForId(Id.IsEmpty()?S.Choice[S.Nation]:Id),G=SelectedGrade(S);
 return I>=0&&FrontlineMedalsV8215::Definition(I).Nation==EFRRUNationV81(S.Nation)&&G>0&&G<=FrontlineMedalsV8215::Tier(I)&&HangableGrades(I).Contains(G)?I:INDEX_NONE;
}
TArray<FIntPoint> AwardedV8447(int32 Nation)
{
 TArray<FIntPoint> A;
 for(int32 I:FrontlineMedalsV8215::NationIndices(EFRRUNationV81(Nation)))for(int32 G:HangableGrades(I))
  if(G<=FrontlineMedalsV8215::Tier(I))A.Add(FIntPoint(I,G));
 return A;
}
void RevealChosenV8447(FView& S,AWarfareFrontlineGameMode* B)
{
 const int32 I=Chosen(S,B);const auto A=AwardedV8447(S.Nation);
 S.CatalogueV8447.Reset(PickerLayoutV8447::Reveal(A.Find(FIntPoint(I,SelectedGrade(S))),A.Num()));S.ScrollNationV8447=S.Nation;
}
void CloseV836(APlayerController* P)
{
 auto& S=View(P);S.Picker=false;
 if(S.OwnsPauseV836)
 {S.OwnsPauseV836=false;if(auto* B=FrontlineFor(P);B&&B->Phase==EFrontlinePhase::Fighting)WarfareFRRUWorldV82::ResumeAfterOwnedPause(B);}
}
bool Back(APlayerController* P){auto& S=View(P);if(!S.Picker)return false;CloseV836(P);return true;}
bool Eligible(APlayerController* P,AWarfareFrontlineGameMode* B)
{
 if(!P||!B||B->Phase!=EFrontlinePhase::Fighting||HasEnded(B)||!B->AssetFailure.IsEmpty())return false;
 const auto& S=ScreenState(P);return !S.Menu&&!S.Help&&!S.Settings&&!S.Technology&&!S.AchievementsV76&&!S.Preparing&&!S.Deployment&&!S.UnitIntroduction&&!S.HistoricalDispatch&&!S.CampaignEndingV51;
}
bool Wheel(APlayerController* P,int32 Direction)
{
 if(!P)return false;auto& S=View(P);if(!S.Picker)return false;
 auto* B=FrontlineFor(P);if(!Eligible(P,B)){CloseV836(P);return false;}
 Chosen(S,B);if(S.ScrollNationV8447!=S.Nation||!S.CatalogueV8447.Ready)RevealChosenV8447(S,B);
 // The modal owns every wheel event. Only its catalogue aperture scrolls;
 // the header, fixed footer and dimmed battlefield never zoom the camera.
 int32 W=0,H=0;P->GetViewportSize(W,H);float MX=0,MY=0;const float Scale=FMath::Min(W/1920.f,H/1080.f);
 if(Scale<=0||(!P->GetMousePosition(MX,MY)&&!V28ReviewPointer.IsSet()))return true;
 const FVector2D Mouse=V28ReviewPointer.IsSet()?V28ReviewPointer.GetValue():FVector2D((MX-(W-1920*Scale)*.5f)/Scale,(MY-(H-1080*Scale)*.5f)/Scale);
 if(Mouse.X>=PickerLayoutV8447::X&&Mouse.X<=1530&&Mouse.Y>=PickerLayoutV8447::Top&&Mouse.Y<=PickerLayoutV8447::Bottom)
  S.CatalogueV8447.Target=FMath::Clamp(S.CatalogueV8447.Target+FMath::Clamp(Direction,-1,1)*90.f,0.f,PickerLayoutV8447::Maximum(AwardedV8447(S.Nation).Num()));
 return true;
}
void RunPickerChecksV8447(TFunctionRef<void(const FString&,bool)> Check)
{
 bool Reachable=true,Bounded=true,Distinct=true;
 for(int32 Count=0;Count<=100;++Count)
 {
  const float Maximum=PickerLayoutV8447::Maximum(Count);TSet<FIntPoint> Cells;
  for(int32 K=0;K<Count;++K)
  {
   const float Offset=PickerLayoutV8447::Reveal(K,Count);const auto C=PickerLayoutV8447::Cell(K,Offset);
   Reachable&=C.Min.Y>=PickerLayoutV8447::Top-.001f&&C.Max.Y<=PickerLayoutV8447::Bottom+.001f;
   Bounded&=Offset>=0&&Offset<=Maximum&&C.Min.X>=PickerLayoutV8447::X&&C.Max.X<=PickerLayoutV8447::Right;
   Cells.Add(FIntPoint(K%PickerLayoutV8447::Columns,K/PickerLayoutV8447::Columns));
  }
  Distinct&=Cells.Num()==Count;
 }
 Check(TEXT("v8447_picker_every_entry_and_grade_can_be_revealed_whole"),Reachable&&Distinct);
 Check(TEXT("v8447_picker_empty_short_and_partial_last_rows_have_bounded_offsets"),Bounded&&PickerLayoutV8447::Maximum(0)==0&&PickerLayoutV8447::Maximum(8)==0&&PickerLayoutV8447::Maximum(9)>0);
 MedalWallV8215::FScrollV842 Scroll;Scroll.Reset(1000);Scroll.Last=1;Scroll.Target=9999;Scroll.Advance(1.016,PickerLayoutV8447::Maximum(9));
 Check(TEXT("v8447_picker_collection_refresh_clamps_offset_target_and_velocity"),Scroll.Offset<=PickerLayoutV8447::Maximum(9)&&Scroll.Target==PickerLayoutV8447::Maximum(9)&&Scroll.Velocity==0);
 Scroll.Advance(1.032,PickerLayoutV8447::Maximum(0));
 Check(TEXT("v8447_picker_empty_collection_has_no_scroll_drift"),Scroll.Offset==0&&Scroll.Target==0&&Scroll.Velocity==0);
}
bool Hit(APlayerController* P,AWarfareFrontlineGameMode* B,FName Name)
{
 auto& S=View(P);if(!Eligible(P,B)){CloseV836(P);return false;}const FString N=Name.ToString();
 if(N==TEXT("V8216MedalMount")){Chosen(S,B);if(S.Picker){CloseV836(P);return true;}S.Picker=true;RevealChosenV8447(S,B);S.OwnsPauseV836=!UGameplayStatics::IsGamePaused(P);B->SetPaused(true);MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81(S.Nation)),true);if(auto* C=Cast<AWarfareFrontlineController>(P))C->CancelTargeting();return true;}
 if(N==TEXT("V8216MountClose")){CloseV836(P);return true;}
 if(!S.Picker)return false;
 if(N==TEXT("V8216MountNone")||N.StartsWith(TEXT("V8216MountSelect")))
 {
  Chosen(S,B);const FString D=N.RightChop(FString(TEXT("V8216MountSelect")).Len());FString Id,Grade;D.Split(TEXT("_"),&Id,&Grade);
  const int32 I=(Id.IsEmpty()?D:Id).IsNumeric()?FCString::Atoi(*(Id.IsEmpty()?D:Id)):INDEX_NONE;
  const int32 G=Grade.IsNumeric()?FCString::Atoi(*Grade):DefaultGrade(I);
  if(N!=TEXT("V8216MountNone")&&(I<0||I>=FrontlineMedalsV8215::Count||FrontlineMedalsV8215::Definition(I).Nation!=EFRRUNationV81(S.Nation)||G<=0||G>FrontlineMedalsV8215::Tier(I)||!HangableGrades(I).Contains(G)))return true;
  S.Choice[S.Nation]=I>=0?FString::Printf(TEXT("%s|%d"),FrontlineMedalsV8215::Definition(I).Id,G):TEXT("");CloseV836(P);S.Motion={};
  if(GConfig&&B&&!B->IsProgressionVolatile()&&!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))
  {GConfig->SetString(TEXT("Warfare.Decorations"),*FString::Printf(TEXT("MountedMedal%d"),S.Nation),*S.Choice[S.Nation],GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);}
  return true;
 }
 return true;
}
void Header(const FFrontlineCanvas& UI,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,bool Interaction)
{
 auto& S=View(PC);const int32 I=Chosen(S,B);
 if(S.Encounter!=B->GetEncounterId()){S.Encounter=B->GetEncounterId();S.Motion={};S.CameraSeen=false;S.ShockSerial=MAX_uint64;MedalWallV8215::PreloadVariants(FrontlineMedalsV8215::NationIndices(EFRRUNationV81(S.Nation)),true);}
 if(S.LastFrame!=GFrameCounter)
 {
  S.LastFrame=GFrameCounter;const float DT=FMath::Clamp(float(FApp::GetDeltaTime()),.0001f,.05f);FVector Camera;FRotator Rotation;PC->GetPlayerViewPoint(Camera,Rotation);
  const float Speed=S.CameraSeen?(Camera.X-S.LastCamera.X)/DT:0;const float Acc=S.CameraSeen?(Speed-S.LastSpeed)/DT:0;S.LastCamera=Camera;S.LastSpeed=Speed;S.CameraSeen=true;
  TArray<FrontlineMedalMotionV8216::FShock,TInlineAllocator<16>> Shocks;FrontlineMedalMotionV8216::ReadSince(B->GetWorld(),S.ShockSerial,Shocks);float Impulse=0;
  for(const auto& E:Shocks){const float Distance=FMath::Abs(E.Position.X-Camera.X);const float Sign=E.Position.X>=Camera.X?-1.f:1.f;Impulse+=Sign*E.Strength*16.f*FMath::Exp(-Distance/3300.f);}
  if(I>=0)S.Motion.Step(DT,Acc,Impulse);else S.Motion={};
 }
 // A recessed crossbar and two restrained fasteners fit the existing steel rail.
 UI.Skin(3,933,2,54,13,4,FLinearColor(.72f,.73f,.67f));
 if(I>=0)
 {
  auto* T=MedalWallV8215::Art(I,SelectedGrade(S));const auto P=GeometryV8443(T,914,8,92,138);
  if(FrontlineTextureReady(T)&&P.Valid)
  {
   const auto& A=P.Image;UI.MedalOverlayV8216(T,A.X,A.Y,A.W,A.H,S.Motion.Angle,P.PivotX,P.PivotY,A.U,A.V,A.UW,A.VH);
   if(SelectedGrade(S)==FrontlineMedalsV8215::Tier(I))MedalWallV8215::Attachments(UI,I,P.Source.X,P.Source.Y,P.Source.W,P.Source.H,S.Motion.Angle,P.PivotX,P.PivotY);
  }
 }
 if(Interaction){UI.Hit(915,0,90,I>=0?151.f:74.f,TEXT("V8216MedalMount"),18);if(UI.Hovered(915,0,90,151)){
  UI.CursorPriorityV67=FMath::Max(UI.CursorPriorityV67,18);
  const auto Award=I>=0?FrontlineMedalsV8215::Award(I):FrontlineMedalsV8215::FAward{};
  const uint64 Revision=uint64(HashCombineFast(SelectedGrade(S),HashCombineFast(Award.Repeats,Award.Attachments)));
  UI.TooltipV83(TooltipKeyV83(18,I+1),Revision,[&]{UI.PaperSheet(766,168,388,I>=0?166:128);const FLinearColor Ink(.025f,.029f,.025f);UI.RoleLabel(EFrontlineTextRoleV8210::Headline,WarRoom52::L(TEXT("DECORATIONS"),TEXT("佩章")),788,184,344,32,23,Ink,1);UI.RoleParagraph(EFrontlineTextRoleV8210::Text,WarRoom52::L(TEXT("Choose an awarded decoration to hang here."),TEXT("选择已经获授的勋章，悬挂于此。")),788,231,344,53,20,Ink);if(I>=0)UI.RoleLabel(EFrontlineTextRoleV8210::Annotation,FrontlineMedalsV8215::AwardLabel(I,FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese),788,292,344,27,17,Ink,1);});
 }}
}
void DrawPicker(const FFrontlineCanvas& UI,AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
 auto& S=View(PC);if(!Eligible(PC,B)){CloseV836(PC);return;}if(!S.Picker)return;B->SetPaused(true);const int32 Selected=Chosen(S,B),Grade=SelectedGrade(S);const auto A=AwardedV8447(S.Nation);
 if(S.ScrollNationV8447!=S.Nation||!S.CatalogueV8447.Ready)RevealChosenV8447(S,B);
 S.CatalogueV8447.Advance(FPlatformTime::Seconds(),PickerLayoutV8447::Maximum(A.Num()),UI.MotionStableV83);
 const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
 UI.Rect(0,0,1920,1080,FLinearColor(.008f,.012f,.012f,.65f));UI.Hit(0,0,1920,1080,TEXT("V8216MountClose"),120);UI.PageSurfaceV73(380,146,1160,788,false,false,false);UI.Hit(380,146,1160,788,TEXT("Blocked"),125);
 UI.BilingualLabel(TEXT("DECORATIONS"),TEXT("佩章"),414,179,1088,55,32,Paper,0);UI.ConsoleSeamV79(414,252,1506,252,Border,1);
 MedalWallV8215::MuseumViewportV842(UI,FBox2D(FVector2D(PickerLayoutV8447::X,PickerLayoutV8447::Top),FVector2D(PickerLayoutV8447::Right,PickerLayoutV8447::Bottom)),[&]{
 for(int32 K=0;K<A.Num();++K){const auto Cell=PickerLayoutV8447::Cell(K,S.CatalogueV8447.Offset);if(Cell.Min.Y>=PickerLayoutV8447::Bottom||Cell.Max.Y<=PickerLayoutV8447::Top)continue;const int32 I=A[K].X,G=A[K].Y;const auto& D=FrontlineMedalsV8215::Definition(I);const auto V=FrontlineMedalsV8215::Variants(I);const float X=Cell.Min.X,Y=Cell.Min.Y;
  UI.Skin(2,X,Y,258,250,7,Selected==I&&Grade==G?FLinearColor(.83f,.81f,.70f):FLinearColor(.68f,.70f,.65f));
  auto* T=MedalWallV8215::Art(I,G);const auto Plate=GeometryV8443(T,X+39,Y+12,180,154);
  if(FrontlineTextureReady(T)&&Plate.Valid)
  {
   const auto& ImagePlacement=Plate.Image;UI.Image(T,ImagePlacement.X,ImagePlacement.Y,ImagePlacement.W,ImagePlacement.H,FLinearColor(.91f,.92f,.88f),ImagePlacement.U,ImagePlacement.V,ImagePlacement.UW,ImagePlacement.VH,false,true);
   if(G==FrontlineMedalsV8215::Tier(I))MedalWallV8215::Attachments(UI,I,Plate.Source.X,Plate.Source.Y,Plate.Source.W,Plate.Source.H);
  }
  UI.RoleLabel(EFrontlineTextRoleV8210::Headline,ZH?D.NameZH:D.NameEN,X+TextInsetV8217,Y+NameTopV8217,258-2*TextInsetV8217,NameHeightV8217,ZH?20:19,Paper,1);
  if(V.Num()>1&&V.IsValidIndex(G-1))UI.RoleLabel(EFrontlineTextRoleV8210::Annotation,ZH?V[G-1].NameZH:V[G-1].NameEN,X+TextInsetV8217,Y+GradeTopV8217,258-2*TextInsetV8217,GradeHeightV8217,ZH?16:15,Muted,1);
  UI.Hit(X,Y,258,250,FName(*FString::Printf(TEXT("V8216MountSelect%d_%d"),I,G)),130);
 }
 });
 const float Maximum=PickerLayoutV8447::Maximum(A.Num());
 if(Maximum>0)
 {
  constexpr float TrackTop=PickerLayoutV8447::Top+4,TrackH=PickerLayoutV8447::Bottom-PickerLayoutV8447::Top-8;
  const float ThumbH=FMath::Max(28.f,TrackH*(PickerLayoutV8447::Bottom-PickerLayoutV8447::Top)/(Maximum+PickerLayoutV8447::Bottom-PickerLayoutV8447::Top));
  const float ThumbY=TrackTop+(TrackH-ThumbH)*S.CatalogueV8447.Offset/Maximum;
  UI.Line(1525,TrackTop,1525,TrackTop+TrackH,FLinearColor(.025f,.030f,.027f),4);UI.Line(1525,ThumbY,1525,ThumbY+ThumbH,FLinearColor(.38f,.34f,.24f),4);
 }
 if(A.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Text,WarRoom52::L(TEXT("No decorations have been awarded yet."),TEXT("尚未获授勋章。")),414,454,1088,64,27,Muted,1);
 UI.PageButtonV73(414,854,256,48,WarRoom52::L(TEXT("REMOVE"),TEXT("取下")),TEXT(""),TEXT("V8216MountNone"),true,false,130);
 UI.PageButtonV73(1250,854,256,48,WarRoom52::L(TEXT("RETURN"),TEXT("返回")),TEXT("Esc"),TEXT("V8216MountClose"),true,false,130);
}
}

