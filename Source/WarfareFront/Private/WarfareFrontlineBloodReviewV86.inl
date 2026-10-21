// Focused production allocator/lifecycle checks. Synthetic source records
// test slot ownership; they do not claim a physical-ragdoll acceptance run.
namespace
{
bool BloodReviewAllowedV86(const AWarfareFrontlineGameMode* B)
{
    FString User;return B&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV86Focused"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
}
}

void AWarfareFrontlineWoundFX::RunBloodChecksV86(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    Check(TEXT("v86_blood_disposable_state_guard"),BloodReviewAllowedV86(B));if(!BloodReviewAllowedV86(B))return;
    auto* FX=Get(B->GetWorld());Check(TEXT("v86_blood_bounded_64_pools_48_runoff_segments"),FX&&FX->bReady&&FX->Marks.Num()==64&&FX->RunoffsV828.Num()==48);
    if(!FX||!FX->bReady)return;FX->ClearPresentation();
    auto* Mat=FX->Marks[0]->GetDecalMaterial();Check(TEXT("v86_blood_accepted_liquid_colour_and_material_retained"),Mat&&Mat->GetBaseMaterial()&&Mat->GetBaseMaterial()->GetName()==TEXT("M_BloodDeposit_V84411"));
    const auto Height=[B](float X,float Y){return FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0));};
    FVector Actual=FVector::ZeroVector;bool Found=false;
    const int32 Home=B->GetHomeTrenchIndex(0),Next=FMath::Clamp(Home+B->GetTeamDirection(0),0,B->Trenches.Num()-1);
    const float Mid=B->Trenches.IsValidIndex(Home)&&B->Trenches.IsValidIndex(Next)?(B->Trenches[Home].X+B->Trenches[Next].X)*.5f:0.f;
    for(int32 X=-6;X<=6&&!Found;++X)for(int32 Y=-6;Y<=6&&!Found;++Y)
    {
        FVector P(Mid+X*55.f,Y*75.f,0);P.Z=Height(P.X,P.Y);float Level=0,Depth=0;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth))continue;
        FVector Step=P,Q;int32 Count=0;bool Dry=true;
        for(int32 I=0;I<8;++I){if(!BloodStepV828(Step,Q,Height))break;Step=Q;++Count;Dry&=!AWarfareFrontlineGround::QueryWaterSurfaceV51(Step.X,Step.Y,Level,Depth);}
        if(Count>=3&&Dry){Actual=P;Found=true;}
    }
    Check(TEXT("v86_blood_actual_ground_has_a_continuously_downhill_dry_path"),Found);if(!Found)return;
    TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Owners;TArray<uint32> Identities;
    for(int32 I=0;I<24;++I)
    {
        auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>();if(!U)continue;
        U->SetActorTickEnabled(false);U->Health=0;Owners.Add(U);
        const int32 Slot=FX->AddMark(Actual+FVector(0,I*110.f,0),FVector::ForwardVector,86,1,I,true,true,.4f);
        if(Slot==INDEX_NONE)continue;
        auto& S=FX->CorpseSeepsV53.AddDefaulted_GetRef();S.Unit=U;S.bGroundMarked=true;S.GroundMarkV84411=Slot;
        S.GroundMarkIdentityV84411=FX->MarkState[Slot].IdentityV84411;S.GroundDepositV84411=Actual+FVector(0,I*110.f,0);Identities.Add(S.GroundMarkIdentityV84411);
    }
    Check(TEXT("v86_blood_24_simultaneous_corpse_sources_get_distinct_receiving_pools"),Owners.Num()==24&&Identities.Num()==24);
    for(int32 I=0;I<20;++I)FX->AddMark(Actual+FVector(350,I*70.f,0),FVector::ForwardVector,44,1,I+99,true,true,.2f);
    FX->TickMarksV86(.5f);bool Pinned=true,Monotone=true;int32 Reclaimed=0;TArray<float> LastSpread;
    for(const auto& S:FX->CorpseSeepsV53)LastSpread.Add(FX->MarkState[S.GroundMarkV84411].SpreadV84411);
    for(int32 I=0;I<240;++I)
    {
        const int32 New=FX->AddMark(Actual+FVector(650,I*9.f,0),FVector::ForwardVector,40,1,I+299,true,true,.2f);Reclaimed+=New!=INDEX_NONE;
        FX->TickMarksV86(.05f);
        for(int32 J=0;J<FX->CorpseSeepsV53.Num();++J)
        {
            const auto& S=FX->CorpseSeepsV53[J];const auto& M=FX->MarkState[S.GroundMarkV84411];
            Pinned&=M.IdentityV84411==Identities[J]&&M.RetirementAgeV86<0&&FX->Marks[S.GroundMarkV84411]->IsVisible();
            Monotone&=M.SpreadV84411>=LastSpread[J];LastSpread[J]=M.SpreadV84411;
        }
    }
    Check(TEXT("v86_blood_terminal_pressure_never_restarts_or_steals_24_active_sources"),Pinned&&Monotone&&Reclaimed>20);
    FX->ClearPresentation();const int32 Pool=FX->AddMark(Actual,FVector::ForwardVector,76,1,17,true,true,.4f);
    const float Used=FX->AddRunoffV828(Actual,17,.32f);Check(TEXT("v86_blood_whole_contact_route_reserved_before_mass_debit"),Pool!=INDEX_NONE&&Used>.319f);
    bool Causal=true;for(int32 T=0;T<240;++T)
    {
        FX->TickRunoffsV828(.05f);FX->TickMarksV86(.05f);
        for(const auto& F:FX->RunoffsV828)if(F.ChainV84411&&F.NextSlotV84411!=INDEX_NONE)
            if(FX->RunoffsV828[F.NextSlotV84411].bStartedV84411&&!F.bDelivered)Causal=false;
    }
    float Budget=0;bool Delivered=true;for(const auto& F:FX->RunoffsV828)if(F.ChainV84411){Budget+=F.FilmMassV84411;Delivered&=F.bDelivered;}
    for(int32 I=0;I<FX->MarkState.Num();++I)if(I!=Pool&&FX->MarkState[I].Age<110)Budget+=FX->MarkState[I].MassV84411;
    Check(TEXT("v86_blood_chain_delivers_once_causally_and_conserves_its_finite_mass"),Causal&&Delivered&&FMath::IsNearlyEqual(Budget,.32f,.00001f));
    FX->ClearPresentation();for(int32 I=0;I<48;++I)
    {
        auto& F=FX->RunoffsV828[I];F.Age=0;F.ChainV84411=I+1;F.Start=Actual;F.End=Actual+FVector(12,0,0);F.Mass=.2f;F.bDelivered=false;
    }
    bool Waiting=false;const float Refused=FX->AddRunoffV828(Actual,77,.32f,&Waiting);bool Intact=true;
    for(int32 I=0;I<48;++I)Intact&=FX->RunoffsV828[I].ChainV84411==uint32(I+1)&&FX->RunoffsV828[I].RetirementAgeV86<0;
    Check(TEXT("v86_blood_pool_exhaustion_cannot_overwrite_or_partially_reserve_an_active_chain"),Refused==0&&Waiting&&Intact);
    Waiting=true;FX->AddRunoffV828(Actual,77,-1.f,&Waiting);
    Check(TEXT("v86_blood_invalid_transport_does_not_schedule_capacity_retries"),!Waiting);
    for(int32 I=0;I<48;++I){auto& F=FX->RunoffsV828[I];F.ChainV84411=1;F.bDelivered=F.bStartedV84411=true;F.Age=5;}
    Check(TEXT("v86_blood_finished_chain_waits_for_gradual_retirement_before_reuse"),FX->AddRunoffV828(Actual,77,.32f)==0);
    FX->TickRunoffsV828(.16f);float Half=0;FX->RunoffMaterialsV828[0]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Half);
    Check(TEXT("v86_blood_finished_chain_film_fades_as_one_without_a_single_frame_cut"),FX->RunoffDecalsV828[0]->IsVisible()&&Half>.2f&&Half<.8f);
    FX->TickRunoffsV828(.17f);Check(TEXT("v86_blood_completed_retirement_permits_a_new_whole_chain"),FX->AddRunoffV828(Actual,77,.32f)>.319f);
    FX->ClearPresentation();for(int32 I=0;I<44;++I)FX->AddMark(Actual+FVector(0,I*12.f,0),FVector::ForwardVector,46,1,I,true,true,.1f);
    auto& Terminal=FX->RunoffsV828[0];Terminal.Start=Actual;Terminal.End=Actual+FVector(12,0,0);Terminal.Age=1;Terminal.Duration=.1f;
    Terminal.bStartedV84411=Terminal.bTerminal=true;Terminal.ChainV84411=1;Terminal.Mass=.25f;Terminal.FilmMassV84411=.03f;
    FX->TickRunoffsV828(.01f);Check(TEXT("v86_blood_terminal_keeps_mass_pending_while_a_receiving_pool_retires"),!Terminal.bDelivered&&Terminal.Mass==.25f);
    FX->TickMarksV86(.16f);float Density=0;FX->MarkMaterials[20]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Density);
    Check(TEXT("v86_blood_old_pool_is_faded_before_budget_reclamation"),FX->Marks[20]->IsVisible()&&Density>.2f&&Density<.8f);
    FX->TickMarksV86(.17f);FX->TickRunoffsV828(.01f);const uint32 Once=FX->BloodMarkIdentityV84411;
    for(int32 I=0;I<10;++I)FX->TickRunoffsV828(.05f);
    Check(TEXT("v86_blood_terminal_retries_once_then_cannot_replay_its_deposit"),Terminal.bDelivered&&FX->BloodMarkIdentityV84411==Once);
    FX->ClearPresentation();
    if(auto* U=Owners.IsEmpty()?nullptr:Owners[0].Get())
    {
        FX->FatalHit(U,FVector::ForwardVector,false);const int32 Hits=FX->HitsShown,Sources=FX->CorpseSeepsV53.Num();
        FX->FatalHit(U,FVector::ForwardVector,false);Check(TEXT("v86_blood_repeated_fatal_notification_cannot_restart_a_registered_corpse"),Hits==1&&FX->HitsShown==Hits&&Sources==1&&FX->CorpseSeepsV53.Num()==Sources);
        FX->CorpseSeepsV53.Reset();FX->FatalHit(U,FVector::ForwardVector,false);
        Check(TEXT("v86_blood_a_drained_corpse_never_reopens_a_new_reservoir"),FX->HitsShown==Hits&&FX->CorpseSeepsV53.IsEmpty());
    }
    FX->ClearPresentation();for(const auto& U:Owners)if(auto* Actor=U.Get())Actor->Destroy();
    bool Clear=true;for(const auto& M:FX->MarkState)Clear&=M.IdentityV84411==0&&M.RetirementAgeV86<0;
    for(const auto& F:FX->RunoffsV828)Clear&=F.ChainV84411==0&&F.RetirementAgeV86<0;
    Check(TEXT("v86_blood_encounter_clear_removes_all_source_chain_and_retirement_state"),Clear&&FX->RegisteredCorpseSeepsV86.IsEmpty());
}

bool AWarfareFrontlineWoundFX::ReviewBloodCaptureV86(FVector& Ground)
{
    auto* B=GetWorld()->GetAuthGameMode<AWarfareFrontlineGameMode>();if(!BloodReviewAllowedV86(B)||!bReady)return false;
    const auto Height=[B](float X,float Y){return FrontlineInfantryV45::SurfaceHeight(B,FVector(X,Y,0));};
    FVector Best=Ground;float Score=-MAX_flt;
    for(int32 X=-6;X<=6;++X)for(int32 Y=-6;Y<=6;++Y)
    {
        FVector P=Ground+FVector(X*28.f,Y*28.f,0),Q;P.Z=Height(P.X,P.Y);float Level=0,Depth=0;
        if(AWarfareFrontlineGround::QueryWaterSurfaceV51(P.X,P.Y,Level,Depth)||!BloodStepV828(P,Q,Height))continue;
        const float Value=P.Z-Q.Z-.004f*FVector::Dist2D(P,Ground);if(Value>Score){Score=Value;Best=P;}
    }
    if(Score==-MAX_flt)return false;ClearPresentation();Ground=Best;
    const int32 Mark=AddMark(Best,FVector::ForwardVector,76,1,17,true,true,.40f);
    return Mark!=INDEX_NONE&&AddRunoffV828(Best,17,.32f)>.31f;
}
