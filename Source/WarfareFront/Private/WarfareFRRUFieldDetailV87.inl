// Isolated cosmetic candidate. Ground, sight, formation and vehicle contact
// remain the native scene's authority. Reuse the current accepted prop assets.
#pragma once
#include "WarfareFRRUFieldGuidesV87.generated.inl"
#include "WarfareFRRUGroundcoverV87.generated.inl"
namespace WarfareFRRUFieldDetailV87
{
#include "WarfareFrontlineSandbagContactV84412.inl"
inline bool Enabled(const FString& Scene)
{
#if WITH_EDITOR
 if(!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87FieldDetail"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
 for(const auto& G:WarfareFRRUFieldGuidesV87::Guides)if(Scene==G.Scene)return true;
#endif
 return false;
}
inline constexpr int32 Groups=13;
inline int32 ComponentCount(const FString& Scene){return Enabled(Scene)?Groups:0;}
inline FString MeshPath(int32 I)
{
 if(I<6){const FString Name=FString::Printf(TEXT("SM_Sandbag_%c_V84412"),TEXT('A')+I);
  return TEXT("/Game/Warfare/Field/V84412/Props/Meshes/")+Name+TEXT(".")+Name;}
 if(I>=10&&I<13){const FString Name=FString::Printf(TEXT("SM_WF_FieldVegetation_grass%c"),TEXT('A')+I-10);
  return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldFamilies/Scenes/")+Name+TEXT(".")+Name;}
 static const TCHAR* Paths[]={
  TEXT("/Game/Warfare/Field/V42/Meshes/SM_RootedApron_A_V42.SM_RootedApron_A_V42"),
  TEXT("/Game/Warfare/Field/V42/Meshes/SM_CollapseScatter_V42.SM_CollapseScatter_V42"),
  TEXT("/Game/Warfare/Field/V76/Meshes/SM_FallenTimber_A_V76.SM_FallenTimber_A_V76"),
  TEXT("/Game/Warfare/Field/V76/Meshes/SM_FallenTimber_B_V76.SM_FallenTimber_B_V76")};
 return I<Groups?Paths[I-6]:FString();
}
inline bool Clear(const wfexp::scene::SceneDefinition& N,const FBox& Bounds,bool Sack)
{
 const FBox2D B({Bounds.Min.X,Bounds.Min.Y},{Bounds.Max.X,Bounds.Max.Y});
 if(B.Min.X<N.minX+150||B.Max.X>N.maxX-150||B.Min.Y<N.minY+150||B.Max.Y>N.maxY-150)return false;
 const FBox2D Route(B.Min-FVector2D(Sack?110:300),B.Max+FVector2D(Sack?110:300));
 for(const auto& R:N.routes)if(R.traversable)for(size_t I=1;I<R.points.size();++I)
  if(WarfareFRRUWorldDressingV827::SegmentTouches({R.points[I-1].x,R.points[I-1].y},{R.points[I].x,R.points[I].y},Route))return false;
 if(const auto* M=wfexp::tactical::FindMission(N.missionId))for(const auto& P:M->nodes)
 {
  const FVector2D Q(P.position.x,P.position.y),C(FMath::Clamp(Q.X,B.Min.X,B.Max.X),FMath::Clamp(Q.Y,B.Min.Y,B.Max.Y));
  if(FVector2D::DistSquared(Q,C)<FMath::Square(P.radius+80.))return false;
 }
 for(const auto& S:N.solids)
  if(B.Intersect(FBox2D({S.minimum.x-40,S.minimum.y-40},{S.maximum.x+40,S.maximum.y+40})))return false;
 for(const auto& P:N.waterPatches)if(!P.polygon.empty()&&WarfareFRRUWorldDressingV827::WaterTouches(P,B))return false;
 for(const auto& D:N.decks)if(WarfareFRRUWorldDressingV827::SegmentTouches({D.from.x,D.from.y},{D.to.x,D.to.y},FBox2D(B.Min-FVector2D(D.halfWidth+350),B.Max+FVector2D(D.halfWidth+350))))return false;
 if(std::string_view(N.missionId)=="ru.czernowitz"&&B.Max.X>4750&&B.Max.Y>4400)return false;
 return true;
}
inline bool Validate(const FString& Scene,const UStaticMeshComponent* Component)
{
 const auto* C=Cast<UHierarchicalInstancedStaticMeshComponent>(Component);
 if(!Enabled(Scene)||!C||!C->IsRegistered()||!C->GetStaticMesh()||C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
  ||!C->GetComponentTransform().Equals(FTransform::Identity,.0001))return false;
 for(int32 I=0;I<Groups;++I)if(C->GetStaticMesh()->GetPathName()==MeshPath(I))return C->GetInstanceCount()<=(I<10?600:5000);
 return false;
}
inline bool Stage(AActor* Owner,USceneComponent* Root,const FString& Scene,FString& Error,bool(*Ready)(UMaterialInterface*,FString&))
{
 if(!Enabled(Scene))return true;
 const wfexp::scene::SceneDefinition* N=nullptr;for(const auto& G:wfexp::scene::Scenes())if(Scene==UTF8_TO_TCHAR(G.id)){N=&G;break;}
 if(!Owner||!Root||!N){Error=TEXT("field detail has no staged native surface");return false;}
 UHierarchicalInstancedStaticMeshComponent* Components[Groups]={};UStaticMesh* Meshes[Groups]={};
 for(int32 I=0;I<Groups;++I)
 {
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath(I));if(!Mesh){Error=TEXT("field detail asset missing");return false;}Meshes[I]=Mesh;
  auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner,FName(*FString::Printf(TEXT("FieldDetailV87_%d"),I)),RF_Transient);
  Owner->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetAbsolute(true,true,true);C->SetWorldTransform(FTransform::Identity);
  C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionProfileName(TEXT("NoCollision"));
  C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetReceivesDecals(true);C->SetCastShadow(true);C->SetCullDistances(0,30000);
  for(int32 S=0;S<C->GetNumMaterials();++S)
  {
   auto* Material=C->GetMaterial(S);
   if(I<6&&Material&&Material->GetPathName().StartsWith(TEXT("/Game/Warfare/Field/V84412/Props/Materials/M_Hessian_V84412")))
    Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V84413/Props/Materials/M_Hessian_V84413.M_Hessian_V84413"));
   if(!Material||!Ready||!Ready(Material,Error))return false;
   C->SetMaterial(S,Material);
  }
  C->ComponentTags.Add(TEXT("FRRU_V87FieldDetail"));C->RegisterComponent();Components[I]=C;
 }
 int32 Added=0,Rejected=0;double WorstSackGap=0;
 auto AddSack=[&](int32 Variant,const FVector2D& XY,double Yaw)
 {
  double H,X0,X1,Y0,Y1;if(!wfexp::scene::HeightAt(*N,XY.X,XY.Y,H)
   ||!wfexp::scene::HeightAt(*N,XY.X-20,XY.Y,X0)||!wfexp::scene::HeightAt(*N,XY.X+20,XY.Y,X1)
   ||!wfexp::scene::HeightAt(*N,XY.X,XY.Y-20,Y0)||!wfexp::scene::HeightAt(*N,XY.X,XY.Y+20,Y1)){++Rejected;return;}
  const FVector Normal=FVector(-(X1-X0)/40.,-(Y1-Y0)/40.,1).GetSafeNormal();if(Normal.Z<.92){++Rejected;return;}
  const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal)*FQuat(FVector::UpVector,FMath::DegreesToRadians(Yaw));
  double Low=DBL_MAX,High=-DBL_MAX;
  for(const auto& Foot:SandbagFeetV84412[Variant])
  {const FVector F=Q.RotateVector(Foot);double Z;if(!wfexp::scene::HeightAt(*N,XY.X+F.X,XY.Y+F.Y,Z)){++Rejected;return;}const double Gap=Z-F.Z;Low=FMath::Min(Low,Gap);High=FMath::Max(High,Gap);}
  if(High-Low>7.5){++Rejected;return;}
  const FTransform T(Q,FVector(XY.X,XY.Y,High-1.5));
  if(!Clear(*N,Meshes[Variant]->GetBoundingBox().TransformBy(T),true)){++Rejected;return;}
  Components[Variant]->AddInstance(T);++Added;WorstSackGap=FMath::Max(WorstSackGap,High-Low-1.5);
 };
 int32 GuideIndex=0;
 for(const auto& G:WarfareFRRUFieldGuidesV87::Guides)if(Scene==G.Scene)
 {
  double Distance=0,Next=310+GuideIndex*73;
  for(int32 J=1;J<G.Count;++J)
  {
   const FVector2D A=WarfareFRRUFieldGuidesV87::Points[G.First+J-1],Z=WarfareFRRUFieldGuidesV87::Points[G.First+J];
   const FVector2D Along=(Z-A).GetSafeNormal(),Across(-Along.Y,Along.X);const double Length=(Z-A).Size();
   while(Next<Distance+Length)
   {
    const FVector2D Centre=A+Along*(Next-Distance);const int32 Pattern=int32(Next/700)+GuideIndex;
    for(int32 Side:{-1,1})if((Pattern+Side+GuideIndex)%4!=0)
    {
     const FVector2D Base=Centre+Across*(Side*(G.Floor+G.Shoulder*.84));
     const int32 Count=3+Pattern%3;const double Angle=FMath::RadiansToDegrees(FMath::Atan2(Along.Y,Along.X));
     for(int32 K=0;K<Count;++K)AddSack((K+Pattern+GuideIndex)%6,Base+Along*((K-(Count-1)*.5)*55.)+Across*((K%3-1)*4.),Angle+(K%3-1)*6.);
    }
    Next+=700+(Pattern%3)*83;
   }
   Distance+=Length;
  }
  ++GuideIndex;
 }
 // Broken margins use continuous authored clusters, leaving clear corridors.
 if(const auto* R=WarfareFRRUWorldDressingV827::Recipe(Scene))
 {
  for(int32 Cluster=0;Cluster<22;++Cluster)
  {
   const double T=Cluster/21.;const FVector2D Axis=R->WoodB-R->WoodA;
   const FVector2D Centre=R->WoodA+Axis*T+FVector2D(FMath::Sin(Cluster*2.17)*340,FMath::Cos(Cluster*1.39)*480);
   for(int32 K=0;K<3;++K)
   {
    const int32 I=K==2?8+Cluster%2:6+K;FTransform Transform;
    if(WarfareFRRUWorldDressingV827::Settle(*N,Meshes[I],I>=8?I-6:I-2,Centre+FVector2D(K*84-90,(K%2)*95),Cluster*37+K*51,.9+(Cluster%3)*.11,Transform)
     &&Clear(*N,Meshes[I]->GetBoundingBox().TransformBy(Transform),false)){Components[I]->AddInstance(Transform);++Added;}
   }
  }
 }
 int32 Tufts=0;
 if(Scene==TEXT("ru.galicia.scene.redesign.v1")&&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87GaliciaFacies")))
 for(const auto& P:WarfareFRRUGroundcoverV87::Galicia)
 {
  double H,X0,X1,Y0,Y1;
  if(!wfexp::scene::HeightAt(*N,P.X,P.Y,H)
   ||!wfexp::scene::HeightAt(*N,P.X-12,P.Y,X0)||!wfexp::scene::HeightAt(*N,P.X+12,P.Y,X1)
   ||!wfexp::scene::HeightAt(*N,P.X,P.Y-12,Y0)||!wfexp::scene::HeightAt(*N,P.X,P.Y+12,Y1))continue;
  const FVector Normal=FVector(-(X1-X0)/24.,-(Y1-Y0)/24.,1).GetSafeNormal();if(Normal.Z<.9)continue;
  const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Normal)*FQuat(FVector::UpVector,FMath::DegreesToRadians(P.Yaw));
  const int32 I=10+P.Variant;const FTransform T(Q,FVector(P.X,P.Y,H-.6),FVector(P.Scale));
  if(!Clear(*N,Meshes[I]->GetBoundingBox().TransformBy(T),true))continue;
  Components[I]->AddInstance(T);++Tufts;
 }
 for(const auto* C:Components)if(!Validate(Scene,C)){Error=TEXT("field detail component contract differs");return false;}
 UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_FIELD_TUFTS scene=%s actual=%d nativeGroundAndRoutesUnchanged=1"),*Scene,Tufts);
 UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V87_FIELD_DETAIL scene=%s instances=%d rejectedSacks=%d maxSackGapCm=%.5f noNewCollision=1"),*Scene,Added,Rejected,WorstSackGap);
 return true;
}
}
