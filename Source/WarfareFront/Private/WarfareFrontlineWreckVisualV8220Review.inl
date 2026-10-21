#include "WarfareFrontlineEffectsReviewSafetyV8216.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/StrongObjectPtr.h"
#include "Materials/Material.h"

struct FFrontlineWreckVisualV8220Review
{
 struct FSurface{TWeakObjectPtr<UStaticMeshComponent> Part;int32 Slot=0;TStrongObjectPtr<UMaterialInterface> Burned;};
 static TArray<FSurface>& SurfaceComparison(){static TArray<FSurface> Rows;return Rows;}
 static TArray<TWeakObjectPtr<AFrontlineTank>>& Tanks(){static TArray<TWeakObjectPtr<AFrontlineTank>> Rows;return Rows;}
 static void Clear(){SurfaceComparison().Reset();for(auto& T:Tanks())if(T.IsValid())T->Destroy();Tanks().Reset();}
 static bool LegacySurfaces(bool Legacy)
 {
  if(!Legacy){for(auto& Row:SurfaceComparison())if(Row.Part.IsValid())Row.Part->SetMaterial(Row.Slot,Row.Burned.Get());SurfaceComparison().Reset();return true;}
  bool Good=true;SurfaceComparison().Reset();
  for(auto& Weak:Tanks())if(auto* T=Weak.Get();T&&!T->IsOperational())
  {
   TInlineComponentArray<UStaticMeshComponent*> Parts(T);
   for(auto* Part:Parts)for(int32 Slot=0;Slot<Part->GetNumMaterials();++Slot)
   {
    auto* Burned=Cast<UMaterialInstanceDynamic>(Part->GetMaterial(Slot));if(!Burned)continue;
    const FString Name=Burned->GetMaterial()->GetName();if(!Name.StartsWith(TEXT("M_Wreck_")))continue;
    const FString Key=Name.Mid(8).LeftChop(6);const bool New=Key==TEXT("German")||Key==TEXT("White")||Key==TEXT("Black");
    const FString ParentName=TEXT("M_Vehicle_")+Key+(New?TEXT("_V8218"):TEXT(""));
    auto* Parent=LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Warfare/Vehicles/"))+(New?TEXT("V8218/"):TEXT("V37/"))+ParentName+TEXT(".")+ParentName));
    const bool PriorWreck=FParse::Param(FCommandLine::Get(),TEXT("WFV8221Review"));
    if(PriorWreck){const FString Prior=TEXT("M_Wreck_")+Key+TEXT("_V8220");Parent=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Warfare/Vehicles/V8220/")+Prior+TEXT(".")+Prior));}
    if(!Parent){Good=false;continue;}
    auto* Old=UMaterialInstanceDynamic::Create(Parent,T);if(!Old){Good=false;continue;}Old->CopyInterpParameters(Burned);Old->SetScalarParameterValue(TEXT("VehicleDamage"),PriorWreck?0.f:1.f);
    SurfaceComparison().Add(FSurface{Part,Slot,TStrongObjectPtr<UMaterialInterface>(Burned)});Part->SetMaterial(Slot,Old);
   }
  }
  return Good&&!SurfaceComparison().IsEmpty();
 }
 static void Camera(AWarfareFrontlineController* PC,float Width=6600.f)
 {
  PC->CameraFocus=FVector(0,-180,0);PC->CameraWidth=PC->TargetCameraWidth=Width;PC->CameraPanVelocity=PC->CameraVerticalVelocity=0;
  const FVector Focus=PC->CameraFocus+FVector(0,0,650),Location=PC->CameraFocus+FVector(0,14000,4650);
  PC->BattleCamera->SetActorLocation(Location);PC->BattleCamera->SetActorRotation((Focus-Location).Rotation());
  PC->BattleCamera->GetCameraComponent()->OrthoWidth=PC->CameraWidth;PC->SetViewTarget(PC->BattleCamera);
  // The controller is frozen for these fixtures; keep its ordinary background
  // registration in step with the exact captured tactical camera.
  if(PC->DistantMatte.IsValid())FrontlineTerrainV42::PlaceBackdropV43(PC->DistantMatte.Get(),PC->CameraFocus,Width,16.f/9.f);
 }
 static void Setup(AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC,const FString& Dir,TFunctionRef<void(const FString&,bool)> Check)
 {
  if(!FrontlineEffectsReviewSafetyV8216::Allowed(B))return;
  Clear();Check(TEXT("v8220_wreck_materials_prewarmed"),FFrontlineArmourWreckV8220Access::Warm());
  TArray<TSharedPtr<FJsonValue>> Records;
  for(int32 I=0;I<4;++I)
  {
   const bool German=I>=2,Dead=(I%2)==1;const FVector P(-1680+I*1120,280,AWarfareFrontlineGround::HeightAt(-1680+I*1120,280));
   auto* T=B->GetWorld()->SpawnActor<AFrontlineTank>(P,FRotator::ZeroRotator);
   if(!T||!T->Configure(B,German?EFrontlineVehicle::A7V:EFrontlineVehicle::MarkIVMale,German?1:0,822000+I))
   {Check(TEXT("v8220_actual_wreck_fixture_assets"),false);if(T)T->Destroy();continue;}
   Tanks().Add(T);T->FitGroundV35(P,0);T->bEntered=true;
   const UStaticMesh* LiveBody=T->Body?T->Body->GetStaticMesh():nullptr;
   const FString LiveBodyPath=LiveBody?LiveBody->GetPathName():TEXT("");
   const int32 LiveBodySlots=LiveBody?LiveBody->GetStaticMaterials().Num():0;
   const int32 Instances=T->TrackLinks->GetInstanceCount();
   if(Dead)
   {
    T->ReceiveModuleHit(EFrontlineTankModule::Hull,100000);const auto Record=FFrontlineArmourWreckV8220Access::Describe(T);
    Check(FString::Printf(TEXT("v8221_%s_wreck_has_derived_torn_geometry"),German?TEXT("german"):TEXT("british")),Record->GetBoolField(TEXT("body_geometry_replaced"))&&Record->GetStringField(TEXT("body_mesh"))!=LiveBodyPath);
    Check(FString::Printf(TEXT("v8221_%s_wreck_keeps_original_material_slot_count"),German?TEXT("german"):TEXT("british")),T->Body->GetStaticMesh()->GetStaticMaterials().Num()==LiveBodySlots);
    Check(FString::Printf(TEXT("v8220_%s_wreck_persistent_surface_and_collision"),German?TEXT("german"):TEXT("british")),
     !T->IsOperational()&&!T->GetActorEnableCollision()&&Record->GetBoolField(TEXT("persistent_wreck_parameters"))&&Record->GetBoolField(TEXT("surface_coverage_complete"))&&Record->GetBoolField(TEXT("body_surface_complete"))&&Record->GetBoolField(TEXT("legacy_damage_zero"))&&Record->GetBoolField(TEXT("removed_from_live_material_updates"))&&Record->GetBoolField(TEXT("physical_components_collision_disabled"))&&T->TrackLinks->GetInstanceCount()==Instances);
    Check(FString::Printf(TEXT("v8220_%s_thrown_chain_is_actual_grounded_instances"),German?TEXT("german"):TEXT("british")),Record->GetBoolField(TEXT("thrown_chain_has_ground_contact"))&&Record->GetBoolField(TEXT("track_cache_matches_actual_instances")));
    const auto Before=T->TrackTransformsV33;T->Simulate(.5f);bool SameChain=Before.Num()==T->TrackTransformsV33.Num();
    for(int32 K=0;SameChain&&K<Before.Num();++K)SameChain&=Before[K].Equals(T->TrackTransformsV33[K],.001f);
    Check(FString::Printf(TEXT("v8220_%s_wreck_does_not_restore_chain_or_paint"),German?TEXT("german"):TEXT("british")),
     SameChain&&FFrontlineArmourWreckV8220Access::Describe(T)->GetBoolField(TEXT("persistent_wreck_parameters")));
    Records.Add(MakeShared<FJsonValueObject>(Record));
   }
   else Check(TEXT("v8220_live_armour_retains_accepted_material"),T->Body&&T->Body->GetMaterial(0)&&!T->Body->GetMaterial(0)->GetMaterial()->GetName().StartsWith(TEXT("M_Wreck_")));
   T->Battle=nullptr;T->SetActorTickEnabled(false);
  }
  auto Report=MakeShared<FJsonObject>();Report->SetArrayField(TEXT("wrecks"),Records);FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
  FFileHelper::SaveStringToFile(Json,*(Dir/(B->GetPlayerFaction()==EFrontlineFaction::German?TEXT("german-wreck-bindings.json"):TEXT("british-wreck-bindings.json"))));Camera(PC);
 }
 static void Cooled()
 {
  for(auto& Weak:Tanks())if(auto* T=Weak.Get();T&&!T->IsOperational()){T->WreckAge=130.f;T->UpdateEffects(.016f);}
 }
};
