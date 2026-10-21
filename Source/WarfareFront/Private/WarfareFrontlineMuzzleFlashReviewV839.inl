struct FFrontlineMuzzleFlashV839Review
{
 static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
 {
  auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;FString User;
  const bool Safe=B&&B->IsProgressionVolatile()&&!WarfareFRRUCampaignV82::Active(B)
    &&FParse::Param(FCommandLine::Get(),TEXT("WFV839Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
    &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)&&!FPaths::IsRelative(User);
  Check(TEXT("v839_flash_disposable_receiver_scope"),Safe);if(!Safe)return Report;
  const EFrontlineVehicle Types[]={EFrontlineVehicle::MarkIVMale,EFrontlineVehicle::Whippet,EFrontlineVehicle::A7V,EFrontlineVehicle::BeuteMarkIV,EFrontlineVehicle::MarkIVFemale,EFrontlineVehicle::BeuteMarkIVFemale,EFrontlineVehicle::MarkVMale,EFrontlineVehicle::A7VGretchen,EFrontlineVehicle::MarkVStar,EFrontlineVehicle::Austin1918};
  for(auto Type:Types)
  {
   auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>();
   if(!V){Check(TEXT("v839_flash_actual_actor"),false);continue;}
   const int32 Team=(B->GetFactionForTeam(0)==EFrontlineFaction::German)==GermanVehicle(Type)?0:1;
   const bool Ready=V->Configure(B,Type,Team,9300+int32(Type));bool Impulse=Ready,Off=Ready;int32 Mounts=0;
   if(Ready)for(int32 I=0;I<V->Flashes.Num();++I)
   {
    if(!V->FlashAge.IsValidIndex(I)||!V->Flashes[I]){Impulse=Off=false;continue;}
    ++Mounts;V->FlashAge[I]=0;V->PresentMuzzleFlashV839(I);
    Impulse&=V->Flashes[I]->IsVisible();
    const float Lifetime=I>=V->Guns.Num()?.055f:.030f;
    V->FlashAge[I]=Lifetime-.001f;V->PresentMuzzleFlashV839(I);
    Impulse&=V->Flashes[I]->IsVisible();
    float Density=0;Impulse&=V->FlashMaterials.IsValidIndex(I)&&V->FlashMaterials[I]
      &&V->FlashMaterials[I]->GetScalarParameterValue(TEXT("Density"),Density)&&Density==1.f;
    V->FlashAge[I]=Lifetime+.001f;V->PresentMuzzleFlashV839(I);Off&=!V->Flashes[I]->IsVisible();
   }
   Check(TEXT("v839_muzzle_hard_impulse_model_")+FString::FromInt(int32(Type)),Ready&&Mounts>0&&Impulse&&Off);
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("model"),int32(Type));J->SetNumberField(TEXT("mounts"),Mounts);
   J->SetBoolField(TEXT("full_strength_until_cut"),Impulse);J->SetBoolField(TEXT("off_after_cut"),Off);Rows.Add(MakeShared<FJsonValueObject>(J));V->Destroy();
  }
  Report->SetArrayField(TEXT("models"),Rows);Report->SetNumberField(TEXT("mg_lifetime_seconds"),.030);
  Report->SetNumberField(TEXT("cannon_lifetime_seconds"),.055);Report->SetNumberField(TEXT("unchanged_mg_burst_interval_seconds"),.11);
  Report->SetStringField(TEXT("scope"),TEXT("Actual imported receiver component and material impulse checks. Rendered temporal persistence requires sequence review; this does not establish FG behaviour."));return Report;
 }
};
TSharedRef<FJsonObject> RunFrontlineMuzzleFlashV839Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineMuzzleFlashV839Review::Run(B,Check);}
