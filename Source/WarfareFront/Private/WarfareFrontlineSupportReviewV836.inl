struct FFrontlineSupportV836Review
{
static TSharedRef<FJsonObject> Run(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    auto R=MakeShared<FJsonObject>();bool Good=true;
    const auto C=[&](const TCHAR* Name,bool Passed){Good&=Passed;Check(Name,Passed);};
    if(!B||!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))
    {C(TEXT("v836_support_disposable_guard"),false);R->SetBoolField(TEXT("success"),false);return R;}
    const auto Saved=B->Strikes;const auto SavedZones=B->SmokeZones;
    ON_SCOPE_EXIT{for(auto& S:B->Strikes)if(S.Projectile.IsValid())S.Projectile->Destroy();B->Strikes=Saved;B->SmokeZones=SavedZones;};
    B->Strikes.Reset();
    auto* Gun=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
    const FVector Ordered(0,650,AWarfareFrontlineGround::HeightAt(0,650)+105.f);bool Configured=Gun&&Gun->Configure(B,EFrontlineSupport::AntiTank,Ordered,0);
    C(TEXT("v836_fixed_area_gun_configures_without_armour"),Configured);
    if(Configured)
    {
        // A short clear battery line uses the ordinary area sampler and trace.
        // Reciprocal armoured engagement is checked by the separate fixture.
        Gun->Arrival=0;Gun->NextAction=0;Gun->Target=Ordered;Gun->GunOriginV8215=Ordered+FVector(-600,0,500);
        for(int32 I=0;I<6;++I)Gun->FireAntitank(float(I)*3.4f);
        C(TEXT("v836_area_gun_fires_all_six_without_an_armoured_target"),Gun->RoundsFired==6&&B->Strikes.Num()==6);
        bool Power=true,Source=true;float MaxX=0,MaxY=0;
        for(const auto& S:B->Strikes)
        {Power&=FMath::IsNearlyEqual(S.ArmorPower,260.f);Source&=S.bDirectFire&&S.Team==0;MaxX=FMath::Max(MaxX,float(FMath::Abs(S.Target.X-Ordered.X)));MaxY=FMath::Max(MaxY,float(FMath::Abs(S.Target.Y-Ordered.Y)));}
        C(TEXT("v836_area_gun_retains_ordered_scatter_not_a_tank_snap"),MaxX<=151&&MaxY<=101);
        C(TEXT("v836_area_gun_all_shells_use_reduced_power_and_real_source"),Power&&Source);
        R->SetNumberField(TEXT("actual_rounds"),Gun->RoundsFired);R->SetNumberField(TEXT("maximum_ordered_scatter_x_cm"),MaxX);R->SetNumberField(TEXT("maximum_ordered_scatter_y_cm"),MaxY);
        Gun->Destroy();
    }
    auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
    const FVector P(0,650,AWarfareFrontlineGround::HeightAt(0,650));
    if(Gas&&Gas->Configure(B,EFrontlineSupport::Gas,P,0))
    {
        Gas->EmitGas(Gas->Arrival+9.f);bool Round=true,Clearer=true;int32 Visible=0;
        for(int32 I=0;I<Gas->Visuals.Num();++I)
        {
            const FVector S=Gas->Visuals[I]->GetComponentScale();float D=0;
            Gas->Materials[I]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),D);
            Round&=S.Z/S.X>=.669&&S.Z/S.X<=.75;Clearer&=D<=.60f&&D>0;Visible+=Gas->Visuals[I]->IsVisible();
        }
        C(TEXT("v836_gas_all_six_actual_volumes_have_rounder_vertical_envelopes"),Round&&Visible==6);
        C(TEXT("v836_gas_actual_material_density_is_reduced"),Clearer);
        R->SetNumberField(TEXT("actual_gas_volumes"),Visible);Gas->Destroy();
    }
    else C(TEXT("v836_gas_configures_actual_density_material"),false);
    R->SetBoolField(TEXT("success"),Good);return R;
}
static bool GasVisual(AWarfareFrontlineGameMode* B,const FVector& P)
{
    if(!B||(!FrontlineVisualV85::Allowed(B)&&(!FrontlineEffectsReviewSafetyV8216::Allowed(B)||!FParse::Param(FCommandLine::Get(),TEXT("WFV836Review")))))return false;
    auto* Gas=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
    if(!Gas||!Gas->Configure(B,EFrontlineSupport::Gas,P,0))return false;
    Gas->EmitGas(Gas->Arrival+7.5f);Gas->SetActorTickEnabled(false);return true;
}
};
TSharedRef<FJsonObject> RunFrontlineSupportV836Review(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{return FFrontlineSupportV836Review::Run(B,Check);}
bool PrepareFrontlineGasVisualV836(AWarfareFrontlineGameMode* B,const FVector& P)
{return FFrontlineSupportV836Review::GasVisual(B,P);}
