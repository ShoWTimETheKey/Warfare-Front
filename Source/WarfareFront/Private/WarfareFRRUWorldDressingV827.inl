// Receiver-owned, actual-camera composition candidate. Existing licensed host
// meshes/materials only; no new collision, floor, route or water authority.
#pragma once
#include "FRRU/Native/TacticalRuntime.h"
#include "FRRU/FRRUVauxHQ27ReleaseV87.generated.inl"
namespace WarfareFRRUWorldDressingV827
{
#include "WarfareFrontlineTimberContactV76.inl"
#if WITH_EDITOR
#include "FRRU/FRRUVauxDressingEventsV87.generated.inl"
#endif
enum class EFamily:uint8 { Ridge, Village, Fields, River, Marsh };
struct FRecipe {const char* Mission;EFamily Family;FVector2D WoodA,WoodB,Approach,Secondary;};
// Authored masses in the ordinary tactical-camera ground band. Outer-map
// dressing alone disappeared behind the HUD or outside the playable view.
inline const FRecipe Recipes[]={
 {"fr.mort_homme",EFamily::Ridge,{-3500,800},{-1700,1300},{500,-900},{5600,-600}},
 {"fr.vaux",EFamily::Village,{-3900,-900},{-2500,-100},{-850,650},{5650,-1000}},
 {"fr.douaumont",EFamily::Ridge,{-3200,-1000},{-1350,-450},{-500,500},{5600,1200}},
 {"fr.louvemont",EFamily::Village,{-3800,750},{-2200,-650},{300,700},{6500,-1500}},
 {"fr.chemin_dames",EFamily::Ridge,{-3300,-1050},{-1600,-500},{-500,700},{5900,1600}},
 {"fr.malmaison",EFamily::Ridge,{-3200,-1050},{-1400,-550},{-500,-300},{5750,1550}},
 {"fr.matz",EFamily::Fields,{-3600,500},{-1900,1250},{-750,500},{5750,-1650}},
 {"fr.champagne_defence",EFamily::Fields,{-3700,-700},{-2500,450},{-650,550},{5800,-1500}},
 {"fr.soissons",EFamily::Village,{-3100,-700},{-1800,400},{-600,700},{6200,-1700}},
 {"fr.sommepy",EFamily::Fields,{-3800,950},{-2100,1100},{-750,-850},{6100,-1200}},
 {"ru.galicia",EFamily::River,{-4200,-800},{-3000,1200},{-3700,1700},{3450,-650}},
 {"ru.przemysl",EFamily::Village,{-3800,-950},{-2250,-350},{-600,600},{5900,-1200}},
 {"ru.gorlice",EFamily::Village,{-3400,850},{-2100,-500},{-600,450},{5700,1300}},
 {"ru.warsaw_rearguard",EFamily::River,{-3900,-900},{-2950,900},{-3450,1700},{3350,-1000}},
 {"ru.naroch",EFamily::Marsh,{-3800,-750},{-2300,650},{-3150,1650},{3800,-1150}},
 {"ru.lutsk",EFamily::Fields,{-3750,700},{-2100,1150},{-700,-650},{5800,-1400}},
 {"ru.czernowitz",EFamily::Village,{-3000,-1500},{-1800,400},{-500,-800},{5900,-1400}},
 {"ru.kovel",EFamily::Marsh,{-3950,-1050},{-2550,700},{-3350,1600},{3600,-1150}},
 {"ru.kerensky",EFamily::Ridge,{-3300,750},{-1700,-550},{-650,650},{5900,-1400}},
 {"ru.riga",EFamily::River,{-3950,-950},{-2800,950},{-3250,1750},{3700,-1000}}
};
inline const TCHAR* MeshPaths[]={
 TEXT("/Game/Warfare/Field/V76/Meshes/SM_SplitStump_A_V76.SM_SplitStump_A_V76"),
 TEXT("/Game/Warfare/Field/V76/Meshes/SM_SplitStump_B_V76.SM_SplitStump_B_V76"),
 TEXT("/Game/Warfare/Field/V76/Meshes/SM_FallenTimber_A_V76.SM_FallenTimber_A_V76"),
 TEXT("/Game/Warfare/Field/V76/Meshes/SM_FallenTimber_B_V76.SM_FallenTimber_B_V76"),
 TEXT("/Game/Warfare/Field/V42/Meshes/SM_RootedApron_A_V42.SM_RootedApron_A_V42"),
 TEXT("/Game/Warfare/Field/V42/Meshes/SM_CollapseScatter_V42.SM_CollapseScatter_V42"),
 TEXT("/Game/Warfare/Field/V40/Meshes/SM_DugoutCollapsed_V40.SM_DugoutCollapsed_V40"),
 TEXT("/Game/Warfare/Field/V40/Meshes/SM_CollapseDebris_V40.SM_CollapseDebris_V40")
};
inline const FRecipe* Recipe(const FString& Scene)
{for(const auto& R:Recipes){const auto* N=wfexp::scene::ForMission(R.Mission);if(N&&Scene==UTF8_TO_TCHAR(N->id))return &R;}return nullptr;}
inline bool Enabled(const FString& Scene)
{
 if(Scene==TEXT("fr.vaux.scene.families.v1")
  &&WarfareFRRUWorldRenderBindingV87::ProductionEnabled(Scene)
  &&WarfareFRRUWorldRenderBindingV87::Find(Scene)==&WarfareFRRUWorldRenderBindingV87::VauxRuins)return true;
#if WITH_EDITOR
 return Recipe(Scene)&&FParse::Param(FCommandLine::Get(),TEXT("WFV827WorldDressing"));
#else
 return false;
#endif
}
inline int32 ComponentCount(const FString& Scene){return Enabled(Scene)?UE_ARRAY_COUNT(MeshPaths):0;}
inline bool SegmentTouches(const FVector2D& A,const FVector2D& Z,const FBox2D& B)
{
 double Lo=0,Hi=1;const FVector2D D=Z-A;
 for(int I=0;I<2;++I){const double Q=I?A.Y:A.X,V=I?D.Y:D.X,Mn=I?B.Min.Y:B.Min.X,Mx=I?B.Max.Y:B.Max.X;
  if(FMath::Abs(V)<1.e-9){if(Q<Mn||Q>Mx)return false;continue;}
  double X=(Mn-Q)/V,Y=(Mx-Q)/V;if(X>Y)Swap(X,Y);Lo=FMath::Max(Lo,X);Hi=FMath::Min(Hi,Y);if(Lo>Hi)return false;}
 return true;
}
inline bool WaterTouches(const wfexp::scene::WaterPatch& P,const FBox2D& Box)
{
 const FBox2D B(Box.Min-FVector2D(120),Box.Max+FVector2D(120));bool Inside=false;
 for(std::size_t I=0,J=P.polygon.size()-1;I<P.polygon.size();J=I++)
 {const auto& A=P.polygon[J];const auto& Z=P.polygon[I];if(SegmentTouches({A.x,A.y},{Z.x,Z.y},B))return true;
  if((A.y>B.Min.Y)!=(Z.y>B.Min.Y)&&B.Min.X<(Z.x-A.x)*(B.Min.Y-A.y)/(Z.y-A.y)+A.x)Inside=!Inside;}
 return Inside;
}
inline bool Clear(const wfexp::scene::SceneDefinition& N,const FBox& Bounds)
{
 const FBox2D B(FVector2D(Bounds.Min.X,Bounds.Min.Y),FVector2D(Bounds.Max.X,Bounds.Max.Y));
 if(B.Min.X<-10500||B.Max.X>10500||B.Min.Y<N.minY+100||B.Max.Y>N.maxY-100)return false;
 // Capsule bays, vehicle approaches and all traversable polyline corners are
 // protected by whole transformed bounds, not the prop's origin or radius.
 const FBox2D RouteBox(B.Min-FVector2D(500),B.Max+FVector2D(500));
 for(const auto& R:N.routes)if(R.traversable)for(std::size_t I=1;I<R.points.size();++I)
  if(SegmentTouches({R.points[I-1].x,R.points[I-1].y},{R.points[I].x,R.points[I].y},RouteBox))return false;
 if(const auto* M=wfexp::tactical::FindMission(N.missionId))for(const auto& Node:M->nodes)
 {const FVector2D P(Node.position.x,Node.position.y),Closest(FMath::Clamp(P.X,B.Min.X,B.Max.X),FMath::Clamp(P.Y,B.Min.Y,B.Max.Y));
  if(FVector2D::DistSquared(P,Closest)<FMath::Square(Node.radius+350.))return false;}
 for(const auto& S:N.solids)
 {const FBox2D Solid({S.minimum.x-100,S.minimum.y-100},{S.maximum.x+100,S.maximum.y+100});if(B.Intersect(Solid))return false;}
 for(const auto& D:N.decks)if(SegmentTouches({D.from.x,D.from.y},{D.to.x,D.to.y},FBox2D(B.Min-FVector2D(D.halfWidth+500),B.Max+FVector2D(D.halfWidth+500))))return false;
 for(const auto& P:N.waterPatches)if(!P.polygon.empty()&&WaterTouches(P,B))return false;
 // This actual train/passenger corridor is host-owned and absent from native
 // routes. Protect the complete receiving yard, not just its rail centre.
 if(std::string_view(N.missionId)=="ru.czernowitz"&&B.Max.X>4750&&B.Max.Y>4400)return false;
 return true;
}
inline bool Settle(const wfexp::scene::SceneDefinition& N,UStaticMesh* Mesh,int32 MeshIndex,const FVector2D& Position,double Yaw,double Scale,FTransform& Out)
{
 double H=0,HX0=0,HX1=0,HY0=0,HY1=0;
 if(!wfexp::scene::HeightAt(N,Position.X,Position.Y,H)||!wfexp::scene::HeightAt(N,Position.X-45,Position.Y,HX0)
  ||!wfexp::scene::HeightAt(N,Position.X+45,Position.Y,HX1)||!wfexp::scene::HeightAt(N,Position.X,Position.Y-45,HY0)
  ||!wfexp::scene::HeightAt(N,Position.X,Position.Y+45,HY1))return false;
 const FVector Normal(-(HX1-HX0)/90.,-(HY1-HY0)/90.,1.);if(Normal.SizeSquared()>1.16)return false;
 const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal.GetSafeNormal())*FQuat(FVector::UpVector,FMath::DegreesToRadians(Yaw));
 const FBox Local=Mesh->GetBoundingBox();FTransform T(Q,FVector(Position.X,Position.Y,H),FVector(Scale));
 // A finite support plane across the complete footprint, including its
 // corners. Reject concave/steep terrain rather than floating a whole patch.
 double TerrainLow=DBL_MAX,TerrainHigh=-DBL_MAX,Low=DBL_MAX,High=-DBL_MAX;
 for(int32 X=0;X<3;++X)for(int32 Y=0;Y<3;++Y)
 {const FVector P=T.TransformPosition(FVector(FMath::Lerp(Local.Min.X,Local.Max.X,X*.5),FMath::Lerp(Local.Min.Y,Local.Max.Y,Y*.5),Local.Min.Z));double Floor=0;
  if(!wfexp::scene::HeightAt(N,P.X,P.Y,Floor))return false;const double Gap=Floor-P.Z;TerrainLow=FMath::Min(TerrainLow,Gap);TerrainHigh=FMath::Max(TerrainHigh,Gap);}
 if(TerrainHigh-TerrainLow>18.)return false;
 if(MeshIndex<4)
 {
  // Actual accepted trunk/body vertices, not projecting branch-minimum bounds.
  // Settling a fallen log by Min.Z would leave its continuous body in the air.
  static const int32 ContactRows[]={1,4,2,5};
  for(const FVector& Contact:TimberContactsV76[ContactRows[MeshIndex]])
  {const FVector P=T.TransformPosition(Contact);double Floor=0;if(!wfexp::scene::HeightAt(N,P.X,P.Y,Floor))return false;
   const double Gap=Floor-P.Z;Low=FMath::Min(Low,Gap);High=FMath::Max(High,Gap);}
 }
 else {Low=TerrainLow;High=TerrainHigh;}
 if(High-Low>18.)return false;T.AddToTranslation(FVector(0,0,High-5.));
 if(!Clear(N,Local.TransformBy(T)))return false;Out=T;return true;
}
#if WITH_EDITOR
inline bool DressingEventsRequestedV87()
{return FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxDressingEvents"));}
inline bool DressingEventsValidateV87(AActor* Owner,const FString& Scene,const wfexp::scene::SceneDefinition& N,UStaticMesh* const* Meshes,FString& Error,FString& ReceiptPath,double& MaximumEmbed)
{
 const auto Fail=[&](const TCHAR* Why){Error=Why;return false;};
 FString User,Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
 FString Expected=FPaths::ConvertRelativePathToFull(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")));FPaths::NormalizeDirectoryName(Expected);
 const UWorld* ReviewWorld=Owner?Owner->GetWorld():nullptr;const auto* Battle=ReviewWorld?Cast<AWarfareFrontlineGameMode>(ReviewWorld->GetAuthGameMode()):nullptr;
 const bool Game=FApp::IsGame(),GameWorld=ReviewWorld&&ReviewWorld->IsGameWorld(),Volatile=Battle&&Battle->IsProgressionVolatile();
 const bool NoSave=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")),Review=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"));
 const bool WorldReview=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview")),NaturalReview=FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82NaturalReview"));
 const bool UserSet=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User),ProjectMatches=Project.Equals(Expected,ESearchCase::IgnoreCase);
 const bool NativeMatches=Scene==TEXT("fr.vaux.scene.families.v1")&&N.revision==2&&N.routes.size()==14&&N.solids.size()==30
  &&N.solids[28].blocksMovement&&N.solids[28].blocksSight
  &&FVector(N.solids[28].minimum.x,N.solids[28].minimum.y,N.solids[28].minimum.z).Equals(FRRUVauxHQ27ReleaseV87::Native28Bounds.Min,.0001)
  &&FVector(N.solids[28].maximum.x,N.solids[28].maximum.y,N.solids[28].maximum.z).Equals(FRRUVauxHQ27ReleaseV87::Native28Bounds.Max,.0001);
 const bool ActiveVaux=Battle&&WarfareFRRUCampaignV82::Active(Battle)&&WarfareFRRUCampaignV82::MissionId(Battle)==TEXT("fr.vaux");
 if(!IsInGameThread()||!Game||!GameWorld||!Volatile||!NoSave||!Review||!WorldReview||!NaturalReview||!UserSet||!ProjectMatches||!NativeMatches||!ActiveVaux)
 {
  Error=FString::Printf(TEXT("Vaux dressing private fixture refused: editorBuild=1 GIsEditor=%d game=%d gameWorld=%d volatile=%d gameThread=%d NoSave=%d review=%d worldReview=%d naturalReview=%d userSet=%d projectMatches=%d nativeMatches=%d activeVaux=%d nativeRev=%llu routes=%llu solids=%llu project={%s} expected={%s} user={%s}"),
   int32(GIsEditor),int32(Game),int32(GameWorld),int32(Volatile),int32(IsInGameThread()),int32(NoSave),int32(Review),int32(WorldReview),int32(NaturalReview),int32(UserSet),int32(ProjectMatches),int32(NativeMatches),int32(ActiveVaux),uint64(N.revision),uint64(N.routes.size()),uint64(N.solids.size()),*Project,*Expected,*User);
  UE_LOG(LogTemp,Error,TEXT("WF_VAUX_DRESSING_EVENTS_V87_GUARD %s"),*Error);return false;
 }
 User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);
 const FString Prefix=Project+TEXT("/Saved/FRRU/V82NaturalReview/vaux-rifle/");
 if(!User.StartsWith(Prefix,ESearchCase::IgnoreCase)||!User.EndsWith(TEXT("/User"))||User.Contains(TEXT("..")))return Fail(TEXT("Vaux dressing events UserDir differs"));
 const FString Run=User.Mid(Prefix.Len(),User.Len()-Prefix.Len()-5);
 if(Run.Len()!=32)return Fail(TEXT("Vaux dressing events require a unique 32 digit run"));
 for(TCHAR C:Run)if(!FChar::IsHexDigit(C))return Fail(TEXT("Vaux dressing events run is not hexadecimal"));
 bool FormalGround=false;TArray<UStaticMeshComponent*> Existing;Owner->GetComponents(Existing);
 for(const auto* C:Existing)if(C)for(int32 I=0;I<C->GetNumMaterials();++I)
  if(const auto* M=C->GetMaterial(I);M&&M->GetMaterial()&&M->GetMaterial()->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxSurfaceParity/CandidateV1/Materials/M_VauxSurfaceParity_V87.M_VauxSurfaceParity_V87"))FormalGround=true;
 if(!FormalGround||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxBrickDust"))||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxMacro"))||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxRubble")))
  return Fail(TEXT("Vaux dressing events require the original actual SurfaceParity underlay"));
 TArray<uint8> Bytes;FString Hash,Text;
 const FString Manifest=Project+TEXT("/ArtSource/FRRU/V87/VauxDressingEvents/CandidateV1R1/manifest.json");
 if(!FFileHelper::LoadFileToArray(Bytes,*Manifest)||!FRRUReadOnlySHA256V87(Bytes,Hash)||Hash!=VauxDressingEventsDataV87::ManifestSHA256||!FFileHelper::LoadFileToString(Text,*Manifest))
  return Fail(TEXT("Vaux dressing events manifest bytes differ"));
 TSharedPtr<FJsonObject> Plan;const auto Reader=TJsonReaderFactory<>::Create(Text);
 if(!FJsonSerializer::Deserialize(Reader,Plan)||!Plan.IsValid())return Fail(TEXT("Vaux dressing events manifest invalid"));
 const TSharedPtr<FJsonObject>* Pins=nullptr;
 if(!Plan->TryGetObjectField(TEXT("projectSourceSHA256"),Pins)||!Pins||!Pins->IsValid())return Fail(TEXT("Vaux dressing events source pins missing"));
 for(const auto& Pin:(*Pins)->Values)
 {const FString Key(*Pin.Key);
  if(Key.Contains(TEXT(".."))||!FPaths::IsRelative(Key)||!FFileHelper::LoadFileToArray(Bytes,*(Project/Key))
   ||!FRRUReadOnlySHA256V87(Bytes,Hash))return Fail(TEXT("Vaux dressing events source hash unavailable"));
  // Original43-placement manifest remains immutable historical provenance.
  // Only this exact approved Native1->Native2 header substitution is allowed.
  const bool ExactNative2Source=Key==TEXT("Source/WarfareFront/Private/FRRU/Native/WorldHeightfieldsV82.generated.inl")
   &&Pin.Value->AsString()==TEXT("b1ec0ec170fec51fa1c29785581ca27da678e321894edc5e6aa5c4383b659385")&&Hash==TEXT("8850437417a3d4b102a4303a763e74a425ff379b70693b28cfed5985ed418461");
  if(Hash!=Pin.Value->AsString()&&!ExactNative2Source)return Fail(TEXT("Vaux dressing events source hash differs"));}
 TArray<FVector> Feet[8];
 for(int32 I=0;I<8;++I)
 {
  if(!Meshes[I]->GetBoundingBox().Equals(VauxDressingEventsDataV87::MeshBounds[I],.05))return Fail(TEXT("Vaux dressing actual source bounds differ"));
  const FMeshDescription* D=Meshes[I]->GetMeshDescription(0);if(!D)return Fail(TEXT("Vaux dressing actual source description missing"));
  const FStaticMeshConstAttributes A(*D);const auto P=A.GetVertexPositions();
  if(I<4){static const int32 Rows[]={1,4,2,5};for(const FVector& V:TimberContactsV76[Rows[I]])Feet[I].Add(V);}
  if(I<2||I>=4)for(const FVertexID V:D->Vertices().GetElementIDs())
  {const FVector Q(P[V]);if(Q.Z<=Meshes[I]->GetBoundingBox().Min.Z+5.)Feet[I].Add(Q);}
  if(Feet[I].Num()<3)return Fail(TEXT("Vaux dressing actual lower bearings missing"));
 }
 TSet<int32> Identities;TArray<FBox> HighBounds;MaximumEmbed=0.;
 for(const auto& Place:VauxDressingEventsDataV87::Places)
 {
  if(Place.Model<0||Place.Model>=8||Place.Pose.ContainsNaN()||Identities.Contains(Place.Ordinal))return Fail(TEXT("Vaux dressing ordinal/pose invalid"));
  Identities.Add(Place.Ordinal);const FBox Local=Meshes[Place.Model]->GetBoundingBox(),World=Local.TransformBy(Place.Pose);
  if(!Clear(N,World))return Fail(TEXT("Vaux dressing actual whole bbox violates native clearance"));
  double Lo=DBL_MAX,Hi=-DBL_MAX;
  for(const FVector& V:Feet[Place.Model])
  {const FVector P=Place.Pose.TransformPosition(V);double Floor;if(!wfexp::scene::HeightAt(N,P.X,P.Y,Floor))return Fail(TEXT("Vaux dressing bearing outside native grid"));
   const double Gap=P.Z-Floor;Lo=FMath::Min(Lo,Gap);Hi=FMath::Max(Hi,Gap);}
  if(Lo< -18.075||Hi> -1.425)return Fail(TEXT("Vaux dressing actual bearing floats or overembeds"));
  MaximumEmbed=FMath::Max(MaximumEmbed,-Lo);double GroundLo=DBL_MAX,GroundHi=-DBL_MAX;
  for(int32 X=0;X<3;++X)for(int32 Y=0;Y<3;++Y)
  {const FVector P=Place.Pose.TransformPosition(FVector(FMath::Lerp(Local.Min.X,Local.Max.X,X*.5),FMath::Lerp(Local.Min.Y,Local.Max.Y,Y*.5),Local.Min.Z));double Floor;
   if(!wfexp::scene::HeightAt(N,P.X,P.Y,Floor))return Fail(TEXT("Vaux dressing full footprint outside native grid"));
   GroundLo=FMath::Min(GroundLo,Floor-P.Z);GroundHi=FMath::Max(GroundHi,Floor-P.Z);}
  if(GroundHi-GroundLo>18.075)return Fail(TEXT("Vaux dressing full footprint slope differs"));
  if(Place.Model!=4&&Place.Model!=5)
  {const FBox2D B(FVector2D(World.Min.X,World.Min.Y),FVector2D(World.Max.X,World.Max.Y));for(const FBox& Old:HighBounds)
    if(B.Intersect(FBox2D(FVector2D(Old.Min.X,Old.Min.Y),FVector2D(Old.Max.X,Old.Max.Y))))return Fail(TEXT("Vaux dressing high bodies overlap"));
   HighBounds.Add(World);}
 }
 if(Identities.Num()!=43)return Fail(TEXT("Vaux dressing must retain all 43 identities"));
 ReceiptPath=User/TEXT("FRRUReview/dressing-events-v87.json");return true;
}
#endif
inline bool ValidateComponent(const FString& Scene,const UStaticMeshComponent* Component)
{
 const auto* C=Cast<UHierarchicalInstancedStaticMeshComponent>(Component);const UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;
 if(!Enabled(Scene)||!C||!Mesh||!C->IsRegistered()||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
  ||C->GetInstanceCount()>128||!C->GetComponentTransform().Equals(FTransform::Identity,.0001))return false;
 bool Known=false;for(const auto* Path:MeshPaths)Known|=Mesh->GetPathName()==Path;if(!Known)return false;
 if(const auto* Body=C->GetBodyInstance();Body&&Body->IsValidBodyInstance())return false;
 for(int32 I=0;I<C->GetNumMaterials();++I)if(C->GetMaterial(I)!=Mesh->GetMaterial(I))return false;
#if WITH_EDITOR
 if(C->ComponentHasTag(TEXT("FRRU_V87VauxDressingEvents")))
 {int32 Index=INDEX_NONE;for(int32 I=0;I<8;++I)if(Mesh->GetPathName()==MeshPaths[I])Index=I;int32 Count=0;
  for(const auto& P:VauxDressingEventsDataV87::Places)if(P.Model==Index)
  {FTransform T;if(!C->GetInstanceTransform(Count++,T,true)||!T.Equals(P.Pose,.0001))return false;}
  if(!DressingEventsRequestedV87()||C->GetInstanceCount()!=Count)return false;}
#endif
 return true;
}
inline bool Stage(AActor* Owner,USceneComponent* Root,const FString& Scene,FString& Error,bool(*MaterialReady)(UMaterialInterface*,FString&))
{
 if(!Enabled(Scene))return true;const auto* R=Recipe(Scene);const auto* N=R?wfexp::scene::ForMission(R->Mission):nullptr;
 if(!Owner||!Root||!R||!N){Error=TEXT("operation dressing has no selected native scene");return false;}
 UStaticMesh* Meshes[8]={};UHierarchicalInstancedStaticMeshComponent* Components[8]={};
 for(int32 I=0;I<8;++I)
 {Meshes[I]=LoadObject<UStaticMesh>(nullptr,MeshPaths[I]);if(!Meshes[I]||!Meshes[I]->GetRenderData()||Meshes[I]->GetStaticMaterials().IsEmpty())
  {Error=TEXT("retained operation dressing asset missing");return false;}
  for(const auto& Slot:Meshes[I]->GetStaticMaterials())
  {if(!Slot.MaterialInterface||!Slot.MaterialInterface->GetMaterial())
   {Error=TEXT("retained operation dressing material missing");return false;}
   if(!MaterialReady||!MaterialReady(Slot.MaterialInterface,Error))return false;}
  auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("OperationDressingV827_%d"),I)),RF_Transient);
  Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetAbsolute(true,true,true);C->SetWorldTransform(FTransform::Identity);
  C->SetStaticMesh(Meshes[I]);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(true);C->SetCullDistances(0,30000);
  C->ComponentTags.Add(TEXT("FRRU_V827OperationDressing"));C->RegisterComponent();Components[I]=C;
 }
#if WITH_EDITOR
 if(DressingEventsRequestedV87())
 {
  FString Receipt;double Embed=0.;if(!DressingEventsValidateV87(Owner,Scene,*N,Meshes,Error,Receipt,Embed))return false;
  for(auto* C:Components)C->ComponentTags.Add(TEXT("FRRU_V87VauxDressingEvents"));
  for(const auto& P:VauxDressingEventsDataV87::Places)if(Components[P.Model]->AddInstance(P.Pose)==INDEX_NONE){Error=TEXT("Vaux dressing instance add failed");return false;}
  for(const auto* C:Components)if(!ValidateComponent(Scene,C)){Error=TEXT("Vaux dressing actual component transforms differ");return false;}
  auto E=MakeShared<FJsonObject>();E->SetBoolField(TEXT("success"),true);E->SetBoolField(TEXT("artAccepted"),false);E->SetStringField(TEXT("manifestSHA256"),VauxDressingEventsDataV87::ManifestSHA256);
  E->SetNumberField(TEXT("actualInstances"),43);E->SetNumberField(TEXT("actualProtectedNativeRoutes"),N->routes.size());E->SetNumberField(TEXT("maximumActualBearingEmbedCm"),Embed);
  E->SetStringField(TEXT("underlay"),TEXT("Original SurfaceParity"));E->SetBoolField(TEXT("nativeCollisionChanged"),false);E->SetBoolField(TEXT("actualWholeBoundsNativeClear"),true);
  FString JSON;const auto Writer=TJsonWriterFactory<>::Create(&JSON);FJsonSerializer::Serialize(E,Writer);
  if(!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Receipt),true)||!FFileHelper::SaveStringToFile(JSON,*Receipt)){Error=TEXT("Vaux dressing actual receipt write failed");return false;}
  UE_LOG(LogTemp,Display,TEXT("WF_VAUX_DRESSING_EVENTS_V87 actualInstances=43 village=12 wood=23 bank=8 maximumActualEmbed=%.4f receipt=%s"),Embed,*Receipt);return true;
 }
#endif
 int32 Added=0;const auto Add=[&](int32 Index,FVector2D P,double Yaw,double Scale)
 {FTransform T;if(Settle(*N,Meshes[Index],Index,P,Yaw,Scale,T)){Components[Index]->AddInstance(T);++Added;}};
 // An asymmetrical, connected broken wood margin with widening shell gaps.
 // The smaller pieces sit under the timber groups; it is not a uniform scatter.
 const FVector2D Axis=R->WoodB-R->WoodA,Side(-Axis.Y,Axis.X);const FVector2D Across=Side.GetSafeNormal();
 const int32 Groups=R->Family==EFamily::Fields?5:9;
 static const double Fractions[]={.01,.13,.21,.34,.43,.57,.65,.81,.94};
 static const double Widths[]={-70,110,-190,230,20,-260,170,-100,140};
 for(int32 I=0;I<Groups;++I)
 {const double T=Fractions[I];const FVector2D P=R->WoodA+Axis*T+Across*Widths[I];const double Angle=FMath::RadiansToDegrees(FMath::Atan2(Axis.Y,Axis.X));
  Add(I%2,P,Angle+I*37,1.08+(I%3)*.12);Add(2+I%2,P+Across*(145+(I%2)*90),Angle-17+I*11,1.12);
  Add(4,P+Across*50,Angle+I*23,1.12);Add(5,P+Across*170+Axis.GetSafeNormal()*90,Angle-I*19,1.05);
  if(I%3==0)Add(7,P-Across*160,Angle+I*31,1.15);
 }
 // Repairs and collapse debris form local masses at the approach shoulder.
 // Fields retain open firing lanes; fortress maps carry the denser remnants.
 const int32 Repairs=R->Family==EFamily::Ridge||R->Family==EFamily::Village?3:1;
 for(int32 I=0;I<Repairs;++I)
 {const FVector2D P=R->Approach+FVector2D(I*330-250,(I%2)*290-100);const double A=-35+I*63;
  Add(6,P,A,1.);Add(7,P+FVector2D(-175,115),A+47,1.13);Add(5,P+FVector2D(165,-95),A-23,1.18);Add(4,P+FVector2D(70,140),A,1.12);}
 // The secondary bank/embankment is a leaning timber/debris band, never a
 // second copied forest. Water, decks and the actual train yard remain clear.
 const int32 BankGroups=R->Family==EFamily::River||R->Family==EFamily::Marsh?6:3;
 for(int32 I=0;I<BankGroups;++I)
 {const FVector2D P=R->Secondary+FVector2D(I*340-620,(I%3-1)*145);const double A=19+I*27;
  Add(2+I%2,P,A,1.08);Add(7,P+FVector2D(75,140),A-48,1.08);if(I%2==0)Add(4,P+FVector2D(-90,-130),A,1.12);}
 UE_LOG(LogTemp,Display,TEXT("WF_OPERATION_DRESSING_V827 scene=%s family=%d acceptedInstances=%d protectedNativeRoutes=%llu"),*Scene,int32(R->Family),Added,uint64(N->routes.size()));
 for(const auto* C:Components)if(!ValidateComponent(Scene,C)){Error=TEXT("operation dressing staging differs");return false;}
 return true;
}
}
