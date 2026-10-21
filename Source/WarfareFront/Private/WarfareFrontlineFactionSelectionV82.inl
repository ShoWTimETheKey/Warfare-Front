// One shared dawn, four clipped foregrounds, and one cancellable public start.
namespace FactionSelectionV82
{
constexpr double HoldSeconds=.700,CommitSeconds=1.950;
float SceneToneV844=1.f;
double LastCommitVisualAtV844=-1.e30;
bool SuppressPageVeilV844(double Now){return Now-LastCommitVisualAtV844>=0.&&Now-LastCommitVisualAtV844<.25;}
uint64 LastDrawFrameForInputReview=MAX_uint64,LastCommitPaintedFrameForInputReview=MAX_uint64;
int32 LastCommitPaintedArmyForInputReview=INDEX_NONE;
namespace InputReview {void Start(UWorld* World);}
namespace LayoutV831
{
    uint64 Frame=MAX_uint64;bool Success=false;FString Language;
    float CanvasDPI=0.f,OutputScale=0.f;
    float DescriptionMaxHeight=0.f,DescriptionMaxWidth=0.f;int32 DescriptionMaxRows=0;
    TArray<TSharedPtr<FJsonValue>> Checks;
}
void RunLayoutChecksV831(const FFrontlineCanvas& UI,TFunctionRef<void(const FString&,bool)> Check);
struct FPreloadedImages
{
    TStrongObjectPtr<UTexture2D> Images[9];
    TStrongObjectPtr<UMaterialInterface> NeutralMaterial;
    TStrongObjectPtr<UMaterialInstanceDynamic> NeutralImages[9];
    TSharedPtr<FStreamableHandle> Handle;
    bool Requested=false,Completed=false;FString Failure;
};
FPreloadedImages& Images(){static FPreloadedImages Cache;return Cache;}
void Preload()
{
    auto& Cache=Images();if(Cache.Requested)return;Cache.Requested=true;
    const TCHAR* Names[]={TEXT("T_SharedDawnV82"),TEXT("T_BritishIsolatedDawnV82"),TEXT("T_FrenchIsolatedDawnV82"),TEXT("T_GermanIsolatedDawnV82"),TEXT("T_RussianIsolatedDawnV82"),
        TEXT("T_BritishHorizonV82"),TEXT("T_FrenchHorizonV82"),TEXT("T_GermanHorizonV82"),TEXT("T_RussianHorizonV82")};
    TArray<FSoftObjectPath> Paths;
    for(const auto* Name:Names)Paths.Emplace(FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/UI/FactionSelection/%s.%s"),Name,Name));
    Paths.Emplace(TEXT("/Game/FRRUIntegration/FormalV82/UI/FactionSelection/M_FactionNeutralV83.M_FactionNeutralV83"));
    Cache.Handle=UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,[Paths]()
    {
        auto& Loaded=Images();
        for(int32 I=0;I<9;++I)
        {
            Loaded.Images[I].Reset(Cast<UTexture2D>(Paths[I].ResolveObject()));
            if(!Loaded.Images[I].IsValid()){Loaded.Failure=TEXT("Army selection artwork could not be loaded.");UE_LOG(LogTemp,Error,TEXT("WF_FACTION_ART_MISSING %s"),*Paths[I].ToString());}
        }
        Loaded.NeutralMaterial.Reset(Cast<UMaterialInterface>(Paths[9].ResolveObject()));
        if(!Loaded.NeutralMaterial.IsValid())
        {Loaded.Failure=TEXT("Army selection artwork could not be loaded.");UE_LOG(LogTemp,Error,TEXT("WF_FACTION_NEUTRAL_MATERIAL_MISSING %s"),*Paths[9].ToString());}
        else for(int32 I=0;I<9;++I)if(Loaded.Images[I].IsValid())
        {
            auto* Material=UMaterialInstanceDynamic::Create(Loaded.NeutralMaterial.Get(),GetTransientPackage());
            Loaded.NeutralImages[I].Reset(Material);
            if(Material)Material->SetTextureParameterValue(TEXT("Photo"),Loaded.Images[I].Get());
            else Loaded.Failure=TEXT("Army selection artwork could not be loaded.");
        }
        Loaded.Completed=true;
    });
    if(!Cache.Handle.IsValid()){Cache.Completed=true;Cache.Failure=TEXT("Army selection artwork could not be loaded.");}
}
bool PreloadSettled()
{
    // Completed-with-failure is an explicit error state, not readiness. It
    // opens the existing asset notice rather than hiding the window forever.
    const auto& Cache=Images();if(!Cache.Completed)return false;
    for(const auto& Image:Cache.Images)if(Image.IsValid()&&!FrontlineTextureReady(Image.Get()))return false;
    return true;
}
bool WarmType(const FFrontlineCanvas& UI)
{
    static float OutputScale=-1,DPIScale=-1;static uint64 Frame=MAX_uint64;static TWeakObjectPtr<UFont> Face;
    if(!PreloadSettled())return false;
    if(OutputScale!=UI.Scale||DPIScale!=UI.Surface->Canvas->GetDPIScale()||Face.Get()!=UI.Font)
    {
        OutputScale=UI.Scale;DPIScale=UI.Surface->Canvas->GetDPIScale();Face=UI.Font;Frame=GFrameCounter;
        const TCHAR* Names[]={TEXT("BRITISH EXPEDITIONARY FORCE"),TEXT("ARMÉE FRANÇAISE"),TEXT("DEUTSCHES HEER"),TEXT("РУССКАЯ АРМИЯ")};
        // Populate the output-sized glyph atlas behind the startup artwork.
        // Off-canvas text cannot appear over the main menu.
        TGuardValue<bool> NativeScope(UI.bEnglishPrimaryV45,true);
        TGuardValue<bool> VisibleAudit(V36TypographyAuditActive,false);
        for(int32 I=0;I<4;++I)for(float Size:{31.f,33.f})
        {UI.ArmyNameV8210(Names[I],-4096,-4096,328,86,Size,FLinearColor::White,1,I==2);UI.ArmyNameV8210(Names[I],-4096,-4096,340,86,Size,FLinearColor::White,0,I==2);}
    }
    return GFrameCounter>Frame+1;
}
namespace FrameReviewV83
{
bool Active=false,Moving=false;int32 Army=INDEX_NONE;double Last=0;
TArray<double> Wall,GPU;
void Sample()
{
    const double Now=FPlatformTime::Seconds();if(Last>0)Wall.Add((Now-Last)*1000.);Last=Now;
    const double G=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());if(G>0)GPU.Add(G);
}
TSharedRef<FJsonObject> Summary()
{
    auto R=MakeShared<FJsonObject>();
    for(const auto& Pair:{TPair<FString,TArray<double>>(TEXT("frame_interval_ms"),Wall),TPair<FString,TArray<double>>(TEXT("gpu_ms"),GPU)})
    {
        auto V=Pair.Value;V.Sort();double Total=0;for(double X:V)Total+=X;
        auto D=MakeShared<FJsonObject>();D->SetNumberField(TEXT("samples"),V.Num());D->SetNumberField(TEXT("mean"),V.Num()?Total/V.Num():0);
        D->SetNumberField(TEXT("p95"),V.Num()?V[FMath::Min(V.Num()-1,FMath::CeilToInt(V.Num()*.95)-1)]:0);
        D->SetNumberField(TEXT("max"),V.Num()?V.Last():0);R->SetObjectField(Pair.Key,D);
    }
    R->SetStringField(TEXT("scope"),TEXT("Every controller frame in the isolated rendered selector fixture, including entry and width animation; no outlier filtering, FG off. Not OS-input or sustained battle performance."));
    Active=false;return R;
}
}
struct FRect
{
    float X=0,Y=0,W=0,H=0;
    bool Contains(const FVector2D& P)const{return P.X>=X&&P.X<X+W&&P.Y>=Y&&P.Y<Y+H;}
};
struct FLayout
{
    FRect Panels[4],Back{48,970,248,58},Confirm{1400,970,472,58},Difficulty[3];
};
constexpr float FooterDescriptionYV831=1032.f,FooterDescriptionWV831=1506.f,FooterDescriptionHV831=34.f;
FLayout Layout(const FFrontlineFactionSelectionV82& S)
{
    FLayout L;float X=0;
    for(int32 I=0;I<4;++I){L.Panels[I]={X,112,S.Widths[I],802};X+=S.Widths[I];}
    for(int32 I=0;I<3;++I)L.Difficulty[I]={366+I*328.f,970,312,58};
    return L;
}
bool Pointer(AWarfareFrontlineController* PC,FVector2D& Out)
{
    int32 W=0,H=0;float X=0,Y=0;PC->GetViewportSize(W,H);
    const float Scale=FMath::Min(W/1920.f,H/1080.f);
    if(Scale<=0||!PC->GetMousePosition(X,Y))return false;
    Out=FVector2D((X-(W-1920.f*Scale)*.5f)/Scale,(Y-(H-1080.f*Scale)*.5f)/Scale);
    return Out.X>=0&&Out.X<=1920&&Out.Y>=0&&Out.Y<=1080;
}
bool Focused(AWarfareFrontlineController* PC)
{
    auto* Client=PC&&PC->GetWorld()?PC->GetWorld()->GetGameViewport():nullptr;
    auto* Viewport=Client?Client->Viewport:nullptr;
    return Viewport&&Viewport->HasFocus()&&Viewport->IsForegroundWindow()&&FApp::HasFocus();
}
bool Visible(AWarfareFrontlineController* PC,const AWarfareFrontlineGameMode* B)
{
    if(!PC||!B||B->Phase!=EFrontlinePhase::Briefing||FrontlineDebugV37::IsOpen(PC))return false;
    const auto& S=ScreenState(PC);
    const auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
    return S.Dossier==1918&&!S.Menu&&!S.Help&&!S.Credits&&!S.Settings&&!S.Technology
        &&!S.AchievementsV76&&!S.Preparing&&!S.HistoricalDispatch&&!S.Deployment&&!S.UnitIntroduction
        &&!S.ExitConfirmV61&&(B->AssetFailure.IsEmpty()||S.bAssetNoticeDismissed)
        &&HUD&&HUD->GetPresentationPage()==11;
}
uint64 LastWorldCheckFrameV826=MAX_uint64;
bool LastWorldCoveredV826=false,LastWorldPreloadV826=false;
struct FCoveredWorldV826
{
    TWeakObjectPtr<UGameViewportClient> Client;
    TWeakObjectPtr<UWorld> ScopeWorldV834;
    TWeakObjectPtr<UGameViewportClient> NGXViewportV834;
    bool Previous=false,Owned=false;
    IConsoleVariable* NGXPoolV834=nullptr;
    int32 NGXPreviousV834=3;EConsoleVariableFlags NGXPriorityV834=ECVF_SetByCode;
    uint64 NGXReleaseFrameV834=MAX_uint64;
    uint64 NGXSubmittedWorldFrameV834=MAX_uint64,NGXFenceBeginFrameV834=MAX_uint64;
    TUniquePtr<FRenderCommandFence> NGXResumeFenceV834;
    bool NGXOwnedV834=false,NGXWritingV834=false,NGXExternallySetV834=false,ShutDownV834=false;
    FDelegateHandle NGXChangedV834,PreExitV834,WorldCleanupV834;
    uint32 NGXRetainsV834=0,NGXRestoresV834=0,NGXExternalYieldsV834=0;
    uint32 NGXSubmittedViewsV834=0,NGXFenceReadyV834=0,NGXFencesStartedV834=0,NGXFencesCompletedV834=0,NGXFencesCancelledV834=0;
    FCoveredWorldV826()
    {
        // Restore while Core, UObject and the plugin CVar are still alive,
        // rather than accessing them from process static destruction.
        PreExitV834=FCoreDelegates::OnEnginePreExit.AddLambda([this]()
        {
            Restore();ReleaseNGXV834();ScopeWorldV834.Reset();ShutDownV834=true;
            FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupV834);WorldCleanupV834.Reset();
        });
        WorldCleanupV834=FWorldDelegates::OnWorldCleanup.AddLambda([this](UWorld* World,bool,bool)
        {
            if(ScopeWorldV834.HasSameIndexAndSerialNumber(TWeakObjectPtr<UWorld>(World)))
            {Restore();ReleaseNGXV834();ScopeWorldV834.Reset();NGXExternallySetV834=false;}
        });
    }
    IConsoleVariable* RegisteredNGXV834()const
    {return IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.FramesUntilFeatureDestruction"));}
    void DetachNGXV834()
    {
        if(NGXChangedV834.IsValid())if(auto* Current=RegisteredNGXV834();Current&&Current==NGXPoolV834)
            Current->OnChangedDelegate().Remove(NGXChangedV834);
        NGXChangedV834.Reset();NGXPoolV834=nullptr;
    }
    void CancelResumeFenceV834()
    {
        // FRenderCommandFence owns only a completion-task handle. Dropping it
        // never waits; the queued command owns its own task event, not this.
        if(NGXResumeFenceV834&&FrontlineTextLayoutV834::ReviewEnabled())++NGXFencesCancelledV834;
        NGXResumeFenceV834.Reset();NGXReleaseFrameV834=MAX_uint64;
        NGXSubmittedWorldFrameV834=MAX_uint64;NGXFenceBeginFrameV834=MAX_uint64;
    }
    void ReleaseNGXV834()
    {
        // Same-value writes also transfer ownership. Match the registered object
        // and priority before restoring, including across plugin unregister.
        auto* Current=RegisteredNGXV834();
        if(NGXOwnedV834&&!NGXExternallySetV834&&Current&&Current==NGXPoolV834
            &&Current->GetInt()==MAX_int32&&EConsoleVariableFlags(Current->GetFlags()&ECVF_SetByMask)==NGXPriorityV834)
        {
            TGuardValue<bool> Internal(NGXWritingV834,true);Current->Set(NGXPreviousV834,NGXPriorityV834);
            if(FrontlineTextLayoutV834::ReviewEnabled()&&Current->GetInt()==NGXPreviousV834)++NGXRestoresV834;
        }
        NGXOwnedV834=false;CancelResumeFenceV834();NGXViewportV834.Reset();
        DetachNGXV834();
    }
    void NotifySubmittedWorldV834(AWarfareFrontlineController* PC,const UCanvas* HUDCanvas)
    {
        if(ShutDownV834||!NGXOwnedV834||NGXExternallySetV834||NGXReleaseFrameV834==MAX_uint64
            ||NGXSubmittedWorldFrameV834!=MAX_uint64||!PC||!HUDCanvas||!HUDCanvas->SceneView)return;
        auto* World=PC->GetWorld();auto* Viewport=World?World->GetGameViewport():nullptr;
        const auto* View=HUDCanvas->SceneView;const auto* Family=View->Family;
        // These mirror GameViewportClient's main-view submission guards.
        // A Canvas alone does not establish that a 3D view was submitted.
        if(ScopeWorldV834.Get()!=World||NGXViewportV834.Get()!=Viewport||!Viewport||!Viewport->Viewport
            ||Viewport->bDisableWorldRendering||View->bIsSceneCapture||!Family||!Family->bIsMainViewFamily
            ||Family->Scene!=World->Scene||Family->RenderTarget!=Viewport->Viewport
            ||!FSlateApplication::IsInitialized()
            ||!FSlateApplication::Get().GetPlatformApplication()->IsAllowedToRender())return;
        NGXSubmittedWorldFrameV834=GFrameCounter;
        if(FrontlineTextLayoutV834::ReviewEnabled())++NGXSubmittedViewsV834;
    }
    void RetainNGXV834(bool Covered,UGameViewportClient* Viewport)
    {
        static const bool Baseline=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
            &&FParse::Param(FCommandLine::Get(),TEXT("WFNGXMenuBaselineV834"));
        if(Baseline||ShutDownV834)return;
        if(Covered)
        {
            CancelResumeFenceV834();
            if(!NGXOwnedV834&&!NGXExternallySetV834)
            {
                NGXPoolV834=RegisteredNGXV834();
                if(NGXPoolV834)
                {
                    NGXPreviousV834=NGXPoolV834->GetInt();
                    NGXPriorityV834=EConsoleVariableFlags(NGXPoolV834->GetFlags()&ECVF_SetByMask);
                    if(NGXPreviousV834!=MAX_int32)
                    {
                        // IConsoleVariable's change delegates are dispatched on
                        // the game thread, even for a same-value Set (Core API).
                        NGXChangedV834=NGXPoolV834->OnChangedDelegate().AddLambda([this](IConsoleVariable*)
                        {
                            if(!NGXWritingV834)
                            {
                                NGXExternallySetV834=true;NGXOwnedV834=false;
                                if(FrontlineTextLayoutV834::ReviewEnabled())++NGXExternalYieldsV834;
                            }
                        });
                        TGuardValue<bool> Internal(NGXWritingV834,true);NGXPoolV834->Set(MAX_int32,NGXPriorityV834);
                        NGXOwnedV834=NGXPoolV834->GetInt()==MAX_int32;
                        if(NGXOwnedV834)
                        {NGXViewportV834=Viewport;if(FrontlineTextLayoutV834::ReviewEnabled())++NGXRetainsV834;}
                        if(!NGXOwnedV834)DetachNGXV834();
                    }
                    else NGXPoolV834=nullptr; // Already retained by an external policy.
                }
            }
        }
        else if(NGXOwnedV834||NGXChangedV834.IsValid())
        {
            if(!NGXOwnedV834||NGXExternallySetV834)
            {ReleaseNGXV834();NGXExternallySetV834=false;return;}
            if(RegisteredNGXV834()!=NGXPoolV834)
            {ReleaseNGXV834();NGXExternallySetV834=false;return;}
            if(!Viewport||NGXViewportV834.Get()!=Viewport||Viewport->bDisableWorldRendering)return;
            // The initial three game frames are only an enqueue boundary.
            // A confirmed main-view submission plus an RHI submission fence
            // establishes that the resumed view's queued work has retired.
            if(NGXReleaseFrameV834==MAX_uint64)NGXReleaseFrameV834=GFrameCounter+3;
            if(GFrameCounter<NGXReleaseFrameV834||NGXSubmittedWorldFrameV834==MAX_uint64)return;
            if(!NGXResumeFenceV834)
            {
                NGXResumeFenceV834=MakeUnique<FRenderCommandFence>();NGXFenceBeginFrameV834=GFrameCounter;
                NGXResumeFenceV834->BeginFence(FRenderCommandFence::ESyncDepth::RHIThread);
                if(FrontlineTextLayoutV834::ReviewEnabled()){++NGXFenceReadyV834;++NGXFencesStartedV834;}
            }
            else if(GFrameCounter>NGXFenceBeginFrameV834&&NGXResumeFenceV834->IsFenceComplete())
            {
                if(FrontlineTextLayoutV834::ReviewEnabled())++NGXFencesCompletedV834;
                NGXResumeFenceV834.Reset();ReleaseNGXV834();NGXExternallySetV834=false;
            }
        }
    }
    TSharedPtr<FJsonObject> NGXReviewV834()const
    {
        auto J=MakeShared<FJsonObject>();auto* Current=RegisteredNGXV834();
        J->SetBoolField(TEXT("cvar_available"),Current!=nullptr);
        J->SetNumberField(TEXT("current_value"),Current?Current->GetInt():0);
        J->SetNumberField(TEXT("retain_count"),NGXRetainsV834);J->SetNumberField(TEXT("restore_count"),NGXRestoresV834);
        J->SetNumberField(TEXT("external_override_yields"),NGXExternalYieldsV834);
        J->SetBoolField(TEXT("owned"),NGXOwnedV834);J->SetBoolField(TEXT("externally_set"),NGXExternallySetV834);
        J->SetNumberField(TEXT("captured_previous_value"),NGXPreviousV834);
        J->SetNumberField(TEXT("resumed_views_submitted"),NGXSubmittedViewsV834);
        J->SetNumberField(TEXT("rhi_fence_ready_count"),NGXFenceReadyV834);
        J->SetNumberField(TEXT("rhi_fences_started"),NGXFencesStartedV834);
        J->SetNumberField(TEXT("rhi_fences_completed"),NGXFencesCompletedV834);
        J->SetNumberField(TEXT("rhi_fences_cancelled"),NGXFencesCancelledV834);
        J->SetBoolField(TEXT("rhi_fence_pending"),NGXResumeFenceV834.IsValid());
        return J;
    }
    void Restore()
    {
        if(Owned)if(auto* Viewport=Client.Get())Viewport->bDisableWorldRendering=Previous;
        Client.Reset();Owned=false;
    }
    void Update(AWarfareFrontlineController* PC,const AWarfareFrontlineGameMode* B)
    {
        if(ShutDownV834)return;
        auto* World=PC?PC->GetWorld():nullptr;
        auto* Viewport=World?World->GetGameViewport():nullptr;
        if((ScopeWorldV834.Get()!=World||((NGXOwnedV834||NGXChangedV834.IsValid())&&NGXViewportV834.Get()!=Viewport))
            &&(Owned||NGXOwnedV834||NGXChangedV834.IsValid()))
        {Restore();ReleaseNGXV834();NGXExternallySetV834=false;}
        ScopeWorldV834=World;
        // Draw() always paints an opaque full-screen base, including while
        // pictures stream or an asset notice is visible. Readiness is not a
        // reason to render the hidden battlefield during the first hover.
        const auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        const int32 Page=HUD?HUD->GetPresentationPage():INDEX_NONE;
        // These production pages all start with PageSurfaceV73's alpha-one
        // viewport rectangle. Maps, reports, pause overlays and battle loading
        // keep their view family; loading needs real temporal-lighting warmup.
        const bool Auxiliary=!FrontlineUIAssetsV8213::BaselineMode()
            &&(Page==5||Page==7||Page==8||Page==12||Page==17||Page==19);
        bool HomeOpaque=false;
        if(!FrontlineUIAssetsV8213::BaselineMode()&&Page==0&&B&&B->Phase==EFrontlinePhase::Briefing
            &&!WarfareFRRUCampaignV82::Active(B))
        {
            int32 W=0,H=0;PC->GetViewportSize(W,H);
            // DrawMainMenu covers only the centred 1920x1080 panel. Other
            // aspects retain the world in the exposed viewport margins.
            if(W>0&&H>0&&int64(W)*9==int64(H)*16)
            {
                auto* Cover=MenuCoverV63(B);auto& Art=Artwork();
                // These exact V72 UK/DE packages were exporter-verified opaque.
                // National and historical fallback covers retain their prior route.
                const bool KnownOpaque=Cover&&(Cover==Art.MenuCoversV72[0].Texture.Get()||Cover==Art.MenuCoversV72[1].Texture.Get());
                if(KnownOpaque&&FrontlineTextureReady(Cover))
                {const auto* Resource=Cover->GetResource();HomeOpaque=Resource->IsInitialized()&&Resource->TextureRHI.IsValid();}
            }
        }
        // A prepared UK/DE table uses its existing main view behind this
        // verified alpha-one cover. Retaining the view avoids a cold renderer
        // and an environment restore/hide cycle on the Continue click.
        if(HomeOpaque&&ContinueHomeCoveredV8214(PC,B)&&FrontlineMapSceneV61::IsActive(PC))HomeOpaque=false;
        const auto* Render=World?UFrontlineRenderSettingsSubsystem::Get(World):nullptr;
        // The SDK receives Off during real view families. Keep that bounded
        // drain behind the opaque page, then restore normal covered-page culling.
        const bool Draining=Render&&Render->IsFrameGenerationDrainPendingV844();
        const bool Covered=Viewport&&!Draining&&(Visible(PC,B)||Auxiliary||HomeOpaque);
        RetainNGXV834(Covered,Viewport);
        LastWorldCheckFrameV826=GFrameCounter;LastWorldCoveredV826=Covered;LastWorldPreloadV826=PreloadSettled();
        if(!Covered||Client.Get()!=Viewport)Restore();
        if(Covered&&!Owned)
        {
            Client=Viewport;Previous=Viewport->bDisableWorldRendering;Owned=true;
            // The selector draws an opaque full-screen Canvas. UE continues
            // drawing HUDs when this flag skips the covered 3D view family.
            Viewport->bDisableWorldRendering=true;
        }
    }
    ~FCoveredWorldV826()
    {
        // Hot-unload before engine exit still has live delegates; the normal
        // process-static path has already restored and touches no engine state.
        if(!ShutDownV834)
        {
            Restore();ReleaseNGXV834();FCoreDelegates::OnEnginePreExit.Remove(PreExitV834);
            FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupV834);
        }
    }
};
FCoveredWorldV826& CoveredWorldV826()
{static FCoveredWorldV826 State;return State;}
bool OwnsCoveredWorldV826(AWarfareFrontlineController* PC)
{
    const auto& State=CoveredWorldV826();
    auto* Viewport=PC&&PC->GetWorld()?PC->GetWorld()->GetGameViewport():nullptr;
    return Viewport&&State.Owned&&State.Client.Get()==Viewport
        &&Viewport->bDisableWorldRendering&&LastWorldCheckFrameV826==GFrameCounter;
}
int32 PanelAt(const FLayout& L,const FVector2D& P)
{for(int32 I=0;I<4;++I)if(L.Panels[I].Contains(P))return I;return INDEX_NONE;}
int32 ExpandedPanel(const FFrontlineFactionSelectionV82& S)
{return S.Selected;}
wfexp::Faction NativeFaction(int32 I)
{return I==1?wfexp::Faction::French:I==3?wfexp::Faction::Russian:wfexp::Faction::Invalid;}
bool Available(int32 I)
{
    if(I==0||I==2)return !AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(I==2?EFrontlineFaction::German:EFrontlineFaction::British).IsEmpty();
    if(I!=1&&I!=3)return false;
    if(!WarfareFRRUCampaignV82::CampaignsAvailable(I==1?EFRRUNationV81::French:EFRRUNationV81::Russian))return false;
    // Native records alone do not promise a playable host. Reuse its actual
    // gate, including the existing isolated candidate-review boundary.
    if(!WarfareFRRUWorldV82::HostHooksConnected())return false;
    int32 Count=0;
    for(const auto& M:wfexp::tactical::Missions())if(M.player==NativeFaction(I))
    {if(M.id.empty()||M.sceneId.empty()||M.sceneRevision==0||M.nodes.empty()||M.stages.empty())return false;++Count;}
    return Count>0;
}
bool HoverSelectV826(FFrontlineFactionSelectionV82& S,int32 Candidate,bool PointerMoved)
{
    S.Hover=Available(Candidate)?Candidate:INDEX_NONE;
    // The last hovered army remains selected below the four panes. Moving to
    // difficulty or confirmation therefore never collapses the composition.
    // A parked pointer must not override keyboard selection; real movement
    // returns ownership. Confirmation locks its army until released/cancelled.
    if(S.CommitAt>=0||S.HoldSource||S.Hover==INDEX_NONE
        ||(!S.PointerOwnsSelectionV826&&!PointerMoved))return false;
    const bool Changed=S.Selected!=S.Hover;
    S.PointerOwnsSelectionV826=true;S.Selected=S.Hover;S.Notice.Empty();
    return Changed;
}
void Cancel(AWarfareFrontlineController* PC)
{
    if(!PC)return;auto& S=ScreenState(PC).FactionSelectionV82;
    S.HoldAt=S.CommitAt=-1;S.HoldSource=0;S.CommitPainted=false;S.PaintedFrame=MAX_uint64;S.FreshInput=false;
}
struct FFocusCancellation
{
    TWeakObjectPtr<AWarfareFrontlineController> Owner;
    FDelegateHandle Handle;
    void Watch(AWarfareFrontlineController* PC)
    {
        Owner=PC;
        if(!Handle.IsValid()&&FSlateApplication::IsInitialized())
            Handle=FSlateApplication::Get().OnApplicationActivationStateChanged().AddLambda([this](bool Active)
            {if(!Active)if(auto* Controller=Owner.Get())Cancel(Controller);});
    }
    ~FFocusCancellation()
    {if(Handle.IsValid()&&FSlateApplication::IsInitialized())FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(Handle);}
};
void Select(AWarfareFrontlineController* PC,int32 I,bool FromPointer=false)
{
    auto& S=ScreenState(PC).FactionSelectionV82;if(S.CommitAt>=0)return;
    Cancel(PC);
    if(!Available(I)){S.Notice=WarRoom52::L(TEXT("This army is not yet available."),TEXT("该阵营暂未开放。"));return;}
    S.PointerOwnsSelectionV826=FromPointer;
    if(S.Selected!=I){S.Selected=I;S.Notice.Empty();}
}
bool Back(AWarfareFrontlineController* PC,const AWarfareFrontlineGameMode* B)
{
    if(!Visible(PC,B))return false;auto& State=ScreenState(PC);auto& S=State.FactionSelectionV82;
    const bool Selected=S.Selected!=INDEX_NONE||S.CommitAt>=0;Cancel(PC);S.Hover=INDEX_NONE;S.Notice.Empty();
    if(Selected)
    {
        S.Selected=INDEX_NONE;S.PointerOwnsSelectionV826=false;
        FVector2D P;S.PointerSeenV826=Pointer(PC,P);if(S.PointerSeenV826)S.LastPointerV826=P;
        return true;
    }
    S=FFrontlineFactionSelectionV82();State.Dossier=0;return true;
}
void UpdateLayout(AWarfareFrontlineController* PC,double Now)
{
    auto& S=ScreenState(PC).FactionSelectionV82;if(S.LayoutFrame==GFrameCounter)return;
    const float DT=S.LastTick<0?0.f:float(FMath::Clamp(Now-S.LastTick,0.,.1));S.LastTick=Now;S.LayoutFrame=GFrameCounter;
    if(S.CommitAt>=0&&S.Selected>=0)
    {
        const float T=FMath::Clamp(float((Now-S.CommitAt)/.9),0.f,1.f),Ease=T*T*(3.f-2.f*T);
        for(int32 I=0;I<4;++I)S.Widths[I]=FMath::Lerp(S.CommitWidths[I],I==S.Selected?1920.f:0.f,Ease);
    }
    else
    {
        const int32 Expanded=ExpandedPanel(S);
        const float Blend=1.f-FMath::Exp(-DT/ .11f);
        for(int32 I=0;I<4;++I)S.Widths[I]=FMath::Lerp(S.Widths[I],Expanded==INDEX_NONE?480.f:I==Expanded?768.f:384.f,Blend);
    }
    // Rounding never opens a gap in the shared canvas or its hit regions.
    float Sum=0;for(float W:S.Widths)Sum+=W;
    S.Widths[S.Selected>=0?S.Selected:3]+=1920.f-Sum;
}
void BeginHold(AWarfareFrontlineController* PC,int32 Source,double Now)
{
    auto& S=ScreenState(PC).FactionSelectionV82;
    if(!S.FreshInput||S.HoldSource||S.CommitAt>=0||!Available(S.Selected))return;
    S.HoldSource=Source;S.HoldAt=Now;S.FreshInput=false;S.Notice.Empty();
}
void Tick(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    if(PC)Preload(); // Starts behind the startup artwork, never in a draw call.
    if(B&&!Images().Failure.IsEmpty()&&B->AssetFailure.IsEmpty())B->AssetFailure=WarRoom52::L(*Images().Failure,TEXT("无法载入阵营选择画面。"));
    static bool PersistenceAttempted=false;
    static const bool PersistenceRequested=WarfareFRRUCampaignV82::PersistenceReviewRequestedV82(true);
    if(!PersistenceAttempted&&PC&&PersistenceRequested)
    {PersistenceAttempted=true;InputReview::Start(PC->GetWorld());}
    CoveredWorldV826().Update(PC,B);
    if(!PC)return;auto& State=ScreenState(PC);auto& S=State.FactionSelectionV82;
    // OS focus changes cancel even when gameplay ticks are suspended.
    static FFocusCancellation Focus;Focus.Watch(PC);
    const bool Enter=PC->IsInputKeyDown(EKeys::Enter),Mouse=PC->IsInputKeyDown(EKeys::LeftMouseButton);
    if(FrameReviewV83::Active&&B&&B->IsProgressionVolatile()
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV83FactionOnly"))&&Visible(PC,B))
    {
        FrameReviewV83::Sample();S.Selected=FrameReviewV83::Army;
        if(FrameReviewV83::Moving)UpdateLayout(PC,FPlatformTime::Seconds());
        return;
    }
    if(!Visible(PC,B))
    {
        if(auto* Audio=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->SetMenuMusicNationV828(INDEX_NONE);
        if(S.Visible)S=FFrontlineFactionSelectionV82();
        S.EnterDown=Enter;S.MouseDown=Mouse;return;
    }
    if(!S.Visible)
    {
        S=FFrontlineFactionSelectionV82();S.Visible=true;S.EnterDown=Enter;S.MouseDown=Mouse;
        // An Enter/click which opened this page cannot confirm on entry.
        S.FreshInput=!Enter&&!Mouse;S.LastTick=FPlatformTime::Seconds();return;
    }
    const double Now=FPlatformTime::Seconds();
    if(!Focused(PC))
    {Cancel(PC);S.Hover=INDEX_NONE;S.EnterDown=Enter;S.MouseDown=Mouse;S.LastTick=Now;return;}
    FVector2D P;const bool HasPointer=Pointer(PC,P);const auto L=Layout(S);
    const bool PointerMoved=HasPointer&&(!S.PointerSeenV826||!P.Equals(S.LastPointerV826,.01f));
    if(HasPointer)S.LastPointerV826=P;S.PointerSeenV826=HasPointer;
    HoverSelectV826(S,HasPointer?PanelAt(L,P):INDEX_NONE,PointerMoved);
    if(auto* Audio=PC->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())
    {const int32 Nations[]={0,2,1,3};Audio->SetMenuMusicNationV828(S.Selected>=0&&S.Selected<4?Nations[S.Selected]:INDEX_NONE);}
    if(!Enter&&!Mouse)S.FreshInput=true;
    if(S.CommitAt>=0)
    {
        UpdateLayout(PC,Now);
        if(Now-S.CommitAt>=CommitSeconds&&S.CommitPainted&&S.PaintedFrame<GFrameCounter)
        {
            const int32 Selected=S.Selected;const auto Difficulty=State.PendingDifficulty;
            Cancel(PC);NavigationV61::FRecord Record(PC);
            const bool Started=Available(Selected)&&(Selected==1||Selected==3
                ?WarfareFRRUCampaignV82::Start(B,Selected==1?EFRRUNationV81::French:EFRRUNationV81::Russian,Difficulty)
                :B->StartNewCampaign(Selected==2?EFrontlineFaction::German:EFrontlineFaction::British,Difficulty));
            if(Started){auto& Next=ScreenState(PC);Next.Dossier=1917;Next.Menu=false;Next.FactionSelectionV82=FFrontlineFactionSelectionV82();PC->ResetCamera();}
            else
            {
                S.Notice=Selected==1||Selected==3?WarfareFRRUCampaignV82::Status(B):B->LastMessage;
                if(S.Notice.IsEmpty())S.Notice=WarRoom52::L(TEXT("The campaign could not be opened."),TEXT("暂时无法打开战役。"));
            }
        }
        S.EnterDown=Enter;S.MouseDown=Mouse;return;
    }
    const FKey NumberKeys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};bool Changed=false;
    for(int32 I=0;I<4;++I)if(PC->WasInputKeyJustPressed(NumberKeys[I])){Select(PC,I);Changed=true;break;}
    if(!Changed&&(PC->WasInputKeyJustPressed(EKeys::Right)||PC->WasInputKeyJustPressed(EKeys::Left)))
    {
        const int32 Direction=PC->WasInputKeyJustPressed(EKeys::Right)?1:-1;
        int32 I=S.Selected==INDEX_NONE?(Direction>0?-1:4):S.Selected;
        for(int32 N=0;N<4;++N){I=(I+Direction+4)%4;if(Available(I)){Select(PC,I);break;}}Changed=true;
    }
    if(S.HoldSource)
    {
        const bool Holding=S.HoldSource==1?Enter:Mouse&&HasPointer&&L.Confirm.Contains(P);
        if(!Holding||!Available(S.Selected))Cancel(PC);
        else if(Now-S.HoldAt>=HoldSeconds)
        {
            S.HoldSource=0;S.HoldAt=-1;S.CommitAt=Now;S.CommitPainted=false;S.PaintedFrame=MAX_uint64;
            for(int32 I=0;I<4;++I)S.CommitWidths[I]=S.Widths[I];
        }
    }
    else if(!Changed)
    {
        if(Enter&&!S.EnterDown)BeginHold(PC,1,Now);
        else if(Mouse&&!S.MouseDown&&HasPointer&&L.Confirm.Contains(P))BeginHold(PC,2,Now);
    }
    UpdateLayout(PC,Now);S.EnterDown=Enter;S.MouseDown=Mouse;
}
bool Click(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B,FName Command)
{
    const bool Choice=Command==TEXT("FactionBritish")||Command==TEXT("FactionFrenchV82")
        ||Command==TEXT("FactionGerman")||Command==TEXT("FactionRussianV82");
    const bool Confirm=Command==TEXT("FactionConfirmV82"),BackButton=Command==TEXT("FactionBackV82");
    const FString Name=Command.ToString();const bool Difficulty=Name==TEXT("Difficulty0")||Name==TEXT("Difficulty1")||Name==TEXT("Difficulty2");
    if(!Choice&&!Confirm&&!BackButton&&!Difficulty)return false;
    if(!Visible(PC,B)||!Focused(PC)||!NavigationV61::AuthenticHUDInputV75)return true;
    auto& S=ScreenState(PC).FactionSelectionV82;FVector2D P;
    // NotifyHitBoxClick is the real press event. The controller key-state
    // snapshot may still describe the previous frame when HUD dispatch runs.
    // Hold duration remains governed by Tick's actual button state/release.
    if(!Pointer(PC,P))return true;
    const auto L=Layout(S);
    if(BackButton&&L.Back.Contains(P)){PC->ToggleMenu();return true;}
    if(S.CommitAt>=0)return true;
    if(Choice)
    {
        const int32 Actual=PanelAt(L,P);if(Actual!=INDEX_NONE&&Actual!=S.Selected)
        {Select(PC,Actual,true);NavigationV61::Click(PC,Command);}return true;
    }
    if(Confirm&&L.Confirm.Contains(P))
    {BeginHold(PC,2,FPlatformTime::Seconds());return true;}
    if(Difficulty)for(int32 I=0;I<3;++I)if(L.Difficulty[I].Contains(P))
    {Cancel(PC);ScreenState(PC).PendingDifficulty=EFrontlineDifficulty(I);return true;}
    return true;
}
UTexture2D* Texture(int32 I)
{
    if(I<0||I>=5)return nullptr;
    auto* Image=Images().Images[I].Get();
    // Do not retain dimensions from an asynchronous default/placeholder frame.
    return FrontlineTextureReady(Image)&&Image->GetSizeX()>1&&Image->GetSizeY()>1?Image:nullptr;
}
void DrawImageV83(const FFrontlineCanvas& UI,UTexture2D* Image,float X,float Y,float W,float H,FLinearColor Tint,bool Colour,
    float U=0,float V=0,float UW=1,float VH=1)
{
    if(!FrontlineTextureReady(Image)||W<=0||H<=0||Tint.A<=0)return;
    // Tint the source image itself; no rectangular overlay enters the compositor.
    Tint.R*=SceneToneV844;Tint.G*=SceneToneV844;Tint.B*=SceneToneV844;
    if(Colour){UI.Image(Image,X,Y,W,H,Tint,U,V,UW,VH);return;}
    UMaterialInstanceDynamic* Material=nullptr;const auto& Cache=Images();
    for(int32 I=0;I<9;++I)if(Cache.Images[I].Get()==Image){Material=Cache.NeutralImages[I].Get();break;}
    // Every instance and texture parameter is immutable after async preload.
    // Material batches therefore retain their own Photo and vertex opacity.
    if(!Material)return;
    const auto P=PlaceFrontlineImage(Image,X,Y,W,H,U,V,UW,VH,false);RecordV27ImagePlacement(Image,P,false);
    UI.MaterialTile(Material,P.X,P.Y,P.W,P.H,Image->GetResource(),P.U,P.V,P.UW,P.VH,Tint);
}
void CompactNativeV83(const FFrontlineCanvas& UI,int32 Army,const TCHAR* Name,float X,float Y,float W,float H,FLinearColor Ink)
{
    if(!PreloadSettled())return;
    UI.ArmyNameV8210(Name,X,Y,W,H,31,Ink,1,Army==2);
}
void Foreground(const FFrontlineCanvas& UI,UTexture2D* Image,const FRect& PaneRegion,float Centre,FLinearColor Tint,bool Colour)
{
    if(!Image||PaneRegion.W<.1f)return;
    const float H=625,W=H*float(Image->GetSizeX())/float(Image->GetSizeY()),X=Centre-W*.5f,Y=475;
    const float Left=FMath::Max(X,PaneRegion.X),Right=FMath::Min(X+W,PaneRegion.X+PaneRegion.W);
    const float Top=FMath::Max(Y,112.f),Bottom=FMath::Min(Y+H,1080.f);
    if(Right>Left&&Bottom>Top)DrawImageV83(UI,Image,Left,Top,Right-Left,Bottom-Top,Tint,Colour,(Left-X)/W,(Top-Y)/H,(Right-Left)/W,(Bottom-Top)/H);
}
UTexture2D* HorizonTexture(int32 I)
{
    if(I<0||I>=4)return nullptr;
    auto* Image=Images().Images[I+5].Get();return FrontlineTextureReady(Image)&&Image->GetSizeX()>1&&Image->GetSizeY()>1?Image:nullptr;
}
void Midground(const FFrontlineCanvas& UI,int32 Army,const FRect& Pane,float Bright,bool Colour)
{
    auto* Image=HorizonTexture(Army);if(!Image||Pane.W<1.f)return;
    const float W=FMath::Max(768.f,Pane.W+128.f),H=W*float(Image->GetSizeY())/float(Image->GetSizeX());
    const float Pivots[]={.61f,.52f,.62f,.58f},X=Pane.X+(Pane.W-W)*.5f,Y=659.f-H*Pivots[Army];
    const float Left=FMath::Max(X,Pane.X),Right=FMath::Min(X+W,Pane.X+Pane.W);if(Right<=Left)return;
    // Transparent terrain shares the existing sun and horizon. Atmospheric
    // fading belongs to the display layer, not another painted sky or frame.
    for(int32 S=0;S<16;++S)
    {
        const float V=(S+.5f)/16.f,FadeIn=FMath::SmoothStep(.18f,.38f,V),FadeOut=1.f-FMath::SmoothStep(.80f,1.f,V);
        DrawImageV83(UI,Image,Left,Y+H*S/16.f,Right-Left,H/16.f,FLinearColor(.70f*Bright,.70f*Bright,.70f*Bright,.78f*FadeIn*FadeOut),Colour,
            (Left-X)/W,float(S)/16.f,(Right-Left)/W,1.f/16.f);
    }
}
float CommitBlackV83(double Age)
{return FMath::SmoothStep(.68f,1.65f,float(Age));}
FString RouteCopy(int32 I)
{
    if(I==0||I==2)
    {
        const int32 Count=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(I==2?EFrontlineFaction::German:EFrontlineFaction::British).Num();
        const FString EN=FString::Printf(TEXT("%d OPERATIONS · 1917–1918"),Count),ZH=FString::Printf(TEXT("%d 场行动 · 1917–1918"),Count);
        return WarRoom52::L(*EN,*ZH);
    }
    int32 Count=0,First=MAX_int32,Last=0;
    for(const auto& M:wfexp::tactical::Missions())if(M.player==NativeFaction(I)){++Count;First=FMath::Min(First,M.date);Last=FMath::Max(Last,M.date);}
    if(!Count)return WarRoom52::L(TEXT("NOT YET AVAILABLE"),TEXT("尚未开放"));
    const FString EN=FString::Printf(TEXT("%d OPERATIONS · %d–%d"),Count,First/10000,Last/10000),ZH=FString::Printf(TEXT("%d 场行动 · %d–%d"),Count,First/10000,Last/10000);
    return WarRoom52::L(*EN,*ZH);
}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B)
{
    auto* PC=Cast<AWarfareFrontlineController>(UI.HUD.GetOwningPlayerController());if(!PC||!B)return;
    // Warm the fixed native-name compositions after a live output/DPI change.
    WarmType(UI);
    auto& State=ScreenState(PC);auto& S=State.FactionSelectionV82;const auto L=Layout(S);
    const double Now=FPlatformTime::Seconds(),CommitAge=S.CommitAt>=0?Now-S.CommitAt:0.;
    const float SceneTone=S.CommitAt>=0?1.f-CommitBlackV83(CommitAge):1.f;
    TGuardValue<float> ToneScope(SceneToneV844,SceneTone);
    if(S.CommitAt>=0)LastCommitVisualAtV844=Now;
    const float TextAlpha=S.CommitAt>=0?FMath::Clamp(1.f-float(CommitAge/.4),0.f,1.f):1.f;
    const int32 Expanded=ExpandedPanel(S);
    const float FullX=-UI.OX/UI.Scale,FullY=-UI.OY/UI.Scale;
    UI.Rect(FullX,FullY,UI.Surface->ClipX/UI.Scale,UI.Surface->ClipY/UI.Scale,
        S.CommitAt>=0?FLinearColor(0,0,0,1):FLinearColor(.025f,.034f,.045f,1));
    if(auto* Dawn=Texture(0))
    {
        DrawImageV83(UI,Dawn,0,0,1920,1080,FLinearColor::White,false);
        if(S.Selected>=0&&S.Selected<4)
        {const auto& P=L.Panels[S.Selected];DrawImageV83(UI,Dawn,P.X,0,P.W,1080,FLinearColor::White,true,P.X/1920.f,0,P.W/1920.f,1);}
    }
    else UI.Gradient(0,0,1920,1080,FLinearColor(.05f*SceneTone,.065f*SceneTone,.09f*SceneTone,1),FLinearColor(.14f*SceneTone,.12f*SceneTone,.085f*SceneTone,1),true);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),40);
    const TCHAR* Native[]={TEXT("BRITISH EXPEDITIONARY FORCE"),TEXT("ARMÉE FRANÇAISE"),TEXT("DEUTSCHES HEER"),TEXT("РУССКАЯ АРМИЯ")};
    const TCHAR* English[]={TEXT("BRITISH CAMPAIGN"),TEXT("FRENCH CAMPAIGN"),TEXT("GERMAN CAMPAIGN"),TEXT("RUSSIAN CAMPAIGN")};
    const TCHAR* Chinese[]={TEXT("英国远征军"),TEXT("法兰西陆军"),TEXT("德意志帝国陆军"),TEXT("俄罗斯帝国陆军")};
    const TCHAR* Honours[]={TEXT("ARRAS · MESSINES · YPRES · CAMBRAI"),TEXT("VERDUN · CHEMIN DES DAMES · CHAMPAGNE"),TEXT("FLANDERN · MICHAEL · AMIENS"),TEXT("EAST PRUSSIA · GALICIA · BUKOVINA · RIGA")};
    const TCHAR* TraitsEnglish[][3]={{TEXT("Creeping barrage and the 18-pounder"),TEXT("Mark IV tanks break the wire"),TEXT("Lewis guns move with the section")},{TEXT("The quick-firing 75"),TEXT("VB rifle grenades and the Chauchat"),TEXT("Hold the line at Verdun")},{TEXT("MG 08/15 anchors the trench"),TEXT("Stormtroopers find the gaps"),TEXT("Tankgewehr against armour")},{TEXT("Mosin–Nagant and the fixed bayonet"),TEXT("Maxim M1910 heavy machine gun"),TEXT("Infantry and artillery reserves")}};
    const TCHAR* TraitsChinese[][3]={{TEXT("徐进弹幕与 18 磅炮"),TEXT("Mark IV 坦克破障"),TEXT("Lewis 机枪随班推进")},{TEXT("75 毫米速射野战炮"),TEXT("VB 枪榴弹与 Chauchat"),TEXT("坚守凡尔登防线")},{TEXT("MG 08/15 扼守战壕"),TEXT("风暴突击队渗透缺口"),TEXT("Tankgewehr 反坦克步枪")},{TEXT("莫辛–纳甘与常装刺刀"),TEXT("马克沁 M1910 重机枪"),TEXT("步兵与炮兵预备队")}};
    const FName Commands[]={TEXT("FactionBritish"),TEXT("FactionFrenchV82"),TEXT("FactionGerman"),TEXT("FactionRussianV82")};
    for(int32 I=0;I<4;++I)
    {
        const auto& P=L.Panels[I];if(P.W<1.f)continue;const bool On=I==Expanded,Ready=Available(I);
        const float Open=FMath::Clamp((P.W-480.f)/288.f,0.f,1.f);
        const bool PaneHover=UI.Hovered(P.X,P.Y,P.W,P.H),OtherHover=S.Selected!=I&&PaneHover;
        const float Bright=Ready?(Expanded==INDEX_NONE?(PaneHover?1.04f:1.f):On?1.f:OtherHover?.82f:.68f):.42f;
        Midground(UI,I,P,Bright,On);
        Foreground(UI,Texture(I+1),P,P.X+P.W*FMath::Lerp(.5f,.26f,Open),FLinearColor(Bright*.82f,Bright*.82f,Bright*.82f,1),On);
        const float CurtainAlpha=!Ready?.38f:Expanded==INDEX_NONE||On?0.f:OtherHover?.14f:.24f;
        if(CurtainAlpha>0&&S.CommitAt<0)UI.Rect(P.X,P.Y,P.W,P.H,FLinearColor(0,0,0,CurtainAlpha));
        UI.Gradient(P.X,692,P.W,246,FLinearColor(.012f*SceneTone,.013f*SceneTone,.012f*SceneTone,0),FLinearColor(.012f*SceneTone,.013f*SceneTone,.012f*SceneTone,(On?.76f:.65f)*TextAlpha),true);
        if(I>0&&S.CommitAt<0)
        {
            UI.Rect(P.X-3,112,6,802,FLinearColor(.008f,.009f,.010f,.82f));
            UI.Line(P.X-2,112,P.X-2,914,FLinearColor(.24f,.23f,.20f,.38f));
            UI.Line(P.X+2,112,P.X+2,914,FLinearColor(.055f,.057f,.052f,.80f));
        }
        if(On&&Open>.01f)UI.Gradient(P.X+P.W*.40f,596,P.W*.60f,318,FLinearColor(.008f*SceneTone,.010f*SceneTone,.010f*SceneTone,0),FLinearColor(.008f*SceneTone,.010f*SceneTone,.010f*SceneTone,.48f*Open*TextAlpha),true);
        if(TextAlpha>0&&P.W>=200.f)
        {
            // Panes move continuously, but type uses two stable compositions.
            // Reflowing through every fractional width caused glyph-cache
            // churn and repeated line breaking during the opening animation.
            const bool ZH=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
            const float DetailAlpha=On?FMath::SmoothStep(.70f,.94f,Open):0.f;
            const float CompactAlpha=(1.f-FMath::SmoothStep(.20f,.45f,Open))*FMath::SmoothStep(356.f,380.f,P.W);
            if(DetailAlpha>0)
            {
                constexpr float W=340,Top=542;const float X=P.X+P.W-28-W,A=TextAlpha*DetailAlpha;
                const FLinearColor Ink(.88f,.85f,.77f,A);
                UI.ArmyNameV8210(Native[I],X,Top+34,W,86,33,Ink,1,I==2);
                UI.RoleLabel(EFrontlineTextRoleV8210::Headline,WarRoom52::L(English[I],Chinese[I]),X,Top+122,W,44,ZH?23:28,FLinearColor(.73f,.71f,.64f,A),1);
                UI.Line(X,Top+177,X+W,Top+177,FLinearColor(.62f,.48f,.27f,.36f*A));
                UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Honours[I],X,Top+188,W,48,16,FLinearColor(.69f,.55f,.32f,A));
                for(int32 T=0;T<3;++T)UI.RoleLabel(EFrontlineTextRoleV8210::Objective,WarRoom52::L(TraitsEnglish[I][T],TraitsChinese[I][T]),X,Top+241+T*32,W,32,20,Ink);
                UI.RoleLabel(EFrontlineTextRoleV8210::Label,RouteCopy(I),X,Top+340,W,30,ZH?18:21,FLinearColor(.60f,.61f,.55f,A));
            }
            if(CompactAlpha>0)
            {
                constexpr float W=328,Top=718;const float X=P.X+(P.W-W)*.5f,A=TextAlpha*CompactAlpha;
                const FLinearColor Ink(.88f,.85f,.77f,A*FMath::Lerp(.57f,1.f,Bright));
                CompactNativeV83(UI,I,Native[I],X,Top+34,W,86,Ink);
                UI.RoleLabel(EFrontlineTextRoleV8210::Headline,Ready?WarRoom52::L(English[I],Chinese[I]):WarRoom52::L(TEXT("NOT YET AVAILABLE"),TEXT("暂未开放")),X,Top+128,W,46,ZH?23:28,FLinearColor(.70f,.69f,.61f,A),1);
            }
        }
        if(S.CommitAt<0)UI.Hit(P.X,P.Y,P.W,P.H,Commands[I],50);
    }
    UI.Gradient(0,0,1920,150,FLinearColor(0,0,0,.50f*TextAlpha),FLinearColor(0,0,0,0),true);
    UI.Gradient(0,918,1920,162,FLinearColor(.008f*SceneTone,.009f*SceneTone,.008f*SceneTone,0),FLinearColor(.008f*SceneTone,.009f*SceneTone,.008f*SceneTone,.96f*TextAlpha),true);
    if(TextAlpha>0)
    {
        // The dawn stays unframed. Only the command tray belongs to the
        // existing physical console family, always the British finish here.
        // Its native tiled rails retain their texel density at 4K.
        FFrontlineCanvas Tray=UI;Tray.ConsoleNationIndexV82=0;Tray.bGermanFinish=false;
        Tray.HoverShortcut.Empty();
        if(!Tray.ConsoleSkinV79(1,28,936,1864,144,12,FLinearColor(1,1,1,TextAlpha)))
            UI.Rect(28,936,1864,144,FLinearColor(.022f,.031f,.027f,TextAlpha));
        UI.RoleLabel(EFrontlineTextRoleV8210::Headline,WarRoom52::L(TEXT("NEW CAMPAIGN · CHOOSE YOUR ARMY"),TEXT("新战役 · 选择你的军队")),48,36,1436,60,FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?29:39,FLinearColor(.88f,.85f,.77f,TextAlpha));
        UI.RoleLabel(EFrontlineTextRoleV8210::Typed,TEXT("1917–1918"),1656,38,216,56,30,FLinearColor(.68f,.54f,.31f,TextAlpha),2);
        const auto Button=[&](const FRect& R,const FString& Label,const FString& Key,FName Command,bool Enabled,bool Selected=false,float Fill=0.f,bool DifficultyCaption=false)
        {
            const bool Over=Enabled&&R.Contains(UI.Mouse);
            const bool Pressed=Over&&PC->IsInputKeyDown(EKeys::LeftMouseButton);
            const float Press=Pressed?1.f:0.f;
            Tray.PageShadowV74(R.X,R.Y,R.W,R.H,Pressed?1.f:3.f);
            const float Light=!Enabled?.43f:Selected?1.f:Over?.94f:.78f;
            Tray.ConsoleSliceV80(2,R.X,R.Y+Press,R.W,R.H-Press,11.f,Tray.ConsoleTintV79()*FLinearColor(Light,Light,Light,TextAlpha));
            Tray.ConsoleSelectionV79(R.X,R.Y+Press,R.W,R.H-Press,Selected,Over);
            if(Selected||Fill>0)
            {
                const float End=Selected?1.f:Fill;
                UI.Line(R.X+24,R.Y+R.H-10,R.X+24+(R.W-48)*End,R.Y+R.H-10,FLinearColor(.65f,.51f,.30f,.66f*TextAlpha));
                UI.Line(R.X+24,R.Y+R.H-9,R.X+24+(R.W-48)*End,R.Y+R.H-9,FLinearColor(.12f,.10f,.06f,.75f*TextAlpha));
            }
            if(DifficultyCaption)UI.RoleLabel(EFrontlineTextRoleV8210::Label,WarRoom52::L(TEXT("DIFFICULTY"),TEXT("难度")),R.X+20,R.Y+5,R.W-40,17,12,FLinearColor(.64f,.63f,.56f,TextAlpha),1);
            UI.RoleLabel(EFrontlineTextRoleV8210::Plate,Label,R.X+20,R.Y+Press+(DifficultyCaption?22:0),R.W-40,DifficultyCaption?22:R.H-Press,UI.ContainsCJKV45(Label)?21:DifficultyCaption?21:24,FLinearColor(.88f,.85f,.77f,TextAlpha*(Enabled?1.f:.43f)),1);
            UI.Shortcut(R.X,R.Y,R.W,R.H,Key,60);
            if(S.CommitAt<0&&Enabled)UI.Hit(R.X,R.Y,R.W,R.H,Command,60);
        };
        Button(L.Back,WarRoom52::L(TEXT("BACK"),TEXT("返回")),TEXT("Esc"),TEXT("FactionBackV82"),true);
        for(int32 I=0;I<3;++I)Button(L.Difficulty[I],FrontlineDifficultyV30::Label(EFrontlineDifficulty(I)),TEXT(""),FName(*FString::Printf(TEXT("Difficulty%d"),I)),true,State.PendingDifficulty==EFrontlineDifficulty(I),0.f,true);
        UI.RoleParagraph(EFrontlineTextRoleV8210::Text,FrontlineDifficultyV30::Description(State.PendingDifficulty),366,FooterDescriptionYV831,FooterDescriptionWV831,FooterDescriptionHV831,FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?20:19,FLinearColor(.63f,.64f,.58f,TextAlpha));
        const float Fill=S.HoldAt>=0?FMath::Clamp(float((Now-S.HoldAt)/HoldSeconds),0.f,1.f):0.f;
        Button(L.Confirm,Available(S.Selected)?WarRoom52::L(TEXT("HOLD · TAKE COMMAND"),TEXT("按住 · 接受任命")):WarRoom52::L(TEXT("CHOOSE AN ARMY FIRST"),TEXT("先选择军队")),TEXT("Enter"),TEXT("FactionConfirmV82"),Available(S.Selected),false,Fill);
        if(!S.Notice.IsEmpty())UI.RoleLabel(EFrontlineTextRoleV8210::Text,S.Notice,48,938,1824,28,18,NoticeWarningV75,1);
    }
    if(S.CommitAt>=0)
    {
        // The images, atmosphere and pane shadows reach the same pure-black
        // base. The commit path contains no translucent covering rectangle.
        if(CommitAge>=1.9&&S.Selected>=0&&S.Widths[S.Selected]>=1919.f)
        {S.CommitPainted=true;S.PaintedFrame=GFrameCounter;
            LastCommitPaintedFrameForInputReview=GFrameCounter;LastCommitPaintedArmyForInputReview=S.Selected;
        }
    }
    LastDrawFrameForInputReview=GFrameCounter;
    // Capture the measurements against the Canvas that actually painted this
    // page. Review mode is disposable and ordinary play allocates no records.
    static const bool LayoutReview=FParse::Param(FCommandLine::Get(),TEXT("WFV831Review"));
    if(LayoutReview)
    {
        LayoutV831::Frame=GFrameCounter;LayoutV831::Success=true;LayoutV831::Checks.Empty();
        LayoutV831::DescriptionMaxHeight=LayoutV831::DescriptionMaxWidth=0.f;LayoutV831::DescriptionMaxRows=0;
        LayoutV831::CanvasDPI=UI.Surface->Canvas->GetDPIScale();LayoutV831::OutputScale=UI.Scale;
        LayoutV831::Language=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?TEXT("zh-CN"):TEXT("en");
        RunLayoutChecksV831(UI,[](const FString& Name,bool Pass)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);
            LayoutV831::Checks.Add(MakeShared<FJsonValueObject>(Row));LayoutV831::Success&=Pass;
        });
    }
}

void RunLayoutChecksV831(const FFrontlineCanvas& UI,TFunctionRef<void(const FString&,bool)> Check)
{
    const auto L=Layout(FFrontlineFactionSelectionV82());
    bool KeysFit=true,ClearFooter=true,CopyFits=true;
    for(int32 I=0;I<3;++I)
    {
        const auto& R=L.Difficulty[I];
        KeysFit&=UI.RoleWidth(EFrontlineTextRoleV8210::Plate,FrontlineDifficultyV30::Label(EFrontlineDifficulty(I)),24)<=R.W-40;
        ClearFooter&=R.Y>=948&&R.Y+R.H<=1030;
        const FString Copy=UI.RoleValueV8210(EFrontlineTextRoleV8210::Text,FrontlineDifficultyV30::Description(EFrontlineDifficulty(I)));
        const float Em=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?20.f:19.f;
        const auto Rows=UI.RoleRowsV8210(EFrontlineTextRoleV8210::Text,Copy,FooterDescriptionWV831,Em);
        const auto Size=UI.RoleSizeRawV8210(EFrontlineTextRoleV8210::Text,Copy,Em);
        CopyFits&=Rows.Num()==1&&Size.Y<=FooterDescriptionHV831;
        LayoutV831::DescriptionMaxHeight=FMath::Max(LayoutV831::DescriptionMaxHeight,float(Size.Y));
        LayoutV831::DescriptionMaxWidth=FMath::Max(LayoutV831::DescriptionMaxWidth,float(Size.X));
        LayoutV831::DescriptionMaxRows=FMath::Max(LayoutV831::DescriptionMaxRows,Rows.Num());
    }
    Check(TEXT("v831_selector_live_shaped_difficulty_names_fit_padded_keys"),KeysFit);
    Check(TEXT("v831_selector_commands_clear_description_and_footer_lip"),ClearFooter&&FooterDescriptionYV831>=1028+4&&FooterDescriptionYV831+FooterDescriptionHV831<=1080-12);
    Check(TEXT("v831_selector_live_shaped_descriptions_fit_single_padded_row"),CopyFits);
    bool CaseReady=true;for(int32 P:{0,2,4})CaseReady&=FrontlineTextureReady(Artwork().ConsoleV79[0][P].Get());
    Check(TEXT("v831_selector_actual_british_case_key_enamel_ready"),CaseReady);
    bool RackFits=true;for(int32 Count=1;Count<=12;++Count)
    {
        const auto Cells=FrontlineRibbonRackV8222::Layout(Count,1654,153,184,40,false);
        RackFits&=Cells.Num()==Count;for(const auto& Cell:Cells)
            RackFits&=Cell.Min.X>=1654&&Cell.Max.X<=1838.001&&Cell.Min.Y>=153&&Cell.Max.Y<=193.001;
    }
    Check(TEXT("v831_warroom_multiline_ribbons_clear_state_title_and_paper_binding"),RackFits&&1332+306<1654&&153+40<197&&1838<1840);
}

// Bounded engine-input regression. This never represents OS/manual input proof.
namespace InputReview
{
bool Box(FString& ReviewUser)
{
    if(WarfareFRRUCampaignV82::PersistenceReviewRequestedV82(true))
        return WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(ReviewUser);
#if WITH_EDITOR
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!FApp::IsGame()||!FParse::Param(FCommandLine::Get(),TEXT("game"))||FApp::IsUnattended()
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFAuthenticInputReviewV82"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        ||!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        ||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),ReviewUser)||FPaths::IsRelative(ReviewUser)||ReviewUser.Contains(TEXT("..")))return false;
    FPaths::NormalizeDirectoryName(ReviewUser);
    return ReviewUser.StartsWith(Project+TEXT("/Saved/FRRU/UIReview/"),ESearchCase::IgnoreCase);
#else
    return false;
#endif
}
struct FRun
{
    TWeakObjectPtr<UWorld> ReviewWorld;
    TWeakObjectPtr<AWarfareFrontlineController> ReviewPC;
    FString Directory,User;
    TArray<TSharedPtr<FJsonValue>> Cases,Captures,Checks;
    TArray<FKey> HeldKeys,ObservedKeys;
    TSharedPtr<FJsonObject> ArmyCase;
    TSharedPtr<FJsonObject> CampaignProof;
    int32 Army=0,Step=0,LegacyBefore=0,NationalBefore=0;
    EFrontlineDifficulty ExpectedDifficulty=EFrontlineDifficulty::Rookie;
    FGuid IdentityBefore;
    FFRRUCampaignSummaryV82 ExpectedRecord;
    int32 ExpectedLegacy=0,ExpectedNational=0;
    double Began=0,StepAt=0,FullHoldAt=-1,CommitObservedAt=-1,IdleAt=0;
    uint64 StepFrame=0;
    float MouseObservedX=0,MouseObservedY=0;
    bool Passed=true,CasePassed=true,Driving=false,Finished=false,SawPaint=false,TransitionShot=false,EarlyFlareChecked=false,MouseObserved=false,Interrupted=false;
    bool Persistence=false,ColdContinue=false,EarnedSource=false;
    bool EarnedLoadingPainted=false;
    int32 EarnedIntroPages=0;
    FString EarnedCombatRecord;
    TSharedPtr<FJsonObject> EarnedProof,SceneProof;
    FString EarnedRoot()const
    {return FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/FRRU/Persistence/62ca7c605b7c4357b98fe4033751d265"));}
    bool EarnedFile(const FString& Path,const TCHAR* SHA,int64 Limit=40*1024*1024)const
    {
        const int64 Size=IFileManager::Get().FileSize(*Path);if(Size<=0||Size>Limit)return false;
#if PLATFORM_WINDOWS
        FString Part=Path;const FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation"));
        while(Part.Len()>=Root.Len())
        {
            const DWORD Attributes=GetFileAttributesW(*Part);
            if(Attributes==INVALID_FILE_ATTRIBUTES||(Attributes&FILE_ATTRIBUTE_REPARSE_POINT)!=0)return false;
            if(Part.Equals(Root,ESearchCase::IgnoreCase))break;Part=FPaths::GetPath(Part);
        }
#endif
        TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path)||Bytes.Num()!=Size)return false;
        uint8 Digest[FSHA1::DigestSize];FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Digest);
        return BytesToHex(Digest,FSHA1::DigestSize).Equals(SHA,ESearchCase::IgnoreCase);
    }
    bool EarnedFiles(bool Copies)const
    {
        // SHA256 approval remains in the launch manifest; these independent
        // immutable SHA1 pins also reject changed source/copy bytes in-process.
        const TCHAR* Source[]={TEXT("User/Saved/FRRU/Profiles/frru-tactical-v2.json"),TEXT("User/Saved/FRRU/Profiles/frru-tactical-v2.json.bak"),
            TEXT("User/Saved/FRRU/Account/achievements-v2.json"),TEXT("User/Saved/FRRU/Account/achievements-v2.json.bak"),
            TEXT("User/FRRUReview/persistence-start.json"),TEXT("owned-process-exit-start.json")};
        const TCHAR* Copy[]={TEXT("Saved/FRRU/Profiles/frru-tactical-v2.json"),TEXT("Saved/FRRU/Profiles/frru-tactical-v2.json.bak"),
            TEXT("Saved/FRRU/Account/achievements-v2.json"),TEXT("Saved/FRRU/Account/achievements-v2.json.bak"),
            TEXT("EarnedSourceV81/persistence-start.json"),TEXT("EarnedSourceV81/owned-process-exit-start.json")};
        const TCHAR* SHA[]={TEXT("0f92d0042f44acda4e6d75b3b4431f05b4bf24d7"),TEXT("0f92d0042f44acda4e6d75b3b4431f05b4bf24d7"),
            TEXT("49858b77c7cad7b57652ddbc19b664bff4a654f2"),TEXT("49858b77c7cad7b57652ddbc19b664bff4a654f2"),
            TEXT("f565ac54b619ea8eed2b1fa44770d6377d197a5f"),TEXT("6bcd380f04e4f7add19fcda93ecf02c517d75bfb")};
        int64 Total=0;
        for(int32 I=0;I<6;++I)
        {
            const FString Original=EarnedRoot()/Source[I];Total+=IFileManager::Get().FileSize(*Original);
            if(Total>64*1024*1024||!EarnedFile(Original,SHA[I])||(Copies&&!EarnedFile(User/Copy[I],SHA[I])))return false;
        }
        return true;
    }
    bool ReadEarnedProof()
    {
        if(!EarnedSource||WITH_EDITOR||Army!=1||!EarnedFiles(true)
            ||IFileManager::Get().FileExists(*(User/TEXT("FactionSelectionInputV82/new/result.json"))))return false;
        FString Text;TSharedPtr<FJsonObject> Root;
        if(!FFileHelper::LoadFileToString(Text,*(User/TEXT("EarnedSourceV81/persistence-start.json")))
            ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid())return false;
        auto Flag=[&](const TCHAR* Name,bool Expected){bool Value=!Expected;return Root->TryGetBoolField(Name,Value)&&Value==Expected;};
        FString Revision,Id;double Nation=0,Difficulty=0;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if(!Root->TryGetStringField(TEXT("revision"),Revision)||Revision!=TEXT("v82.frru.play-persistence.2")
            ||!Flag(TEXT("allPassed"),false)||!Flag(TEXT("normalPublicStart"),true)||!Flag(TEXT("normalGameplayVictory"),true)
            ||!Flag(TEXT("actualSubmissionPersisted"),true)||!Flag(TEXT("actualOutcomeCommitWitnessedBeforeReturnV87"),true)
            ||!Flag(TEXT("normalLiveReportReturnCompletedV87"),false)||!Flag(TEXT("progressionFixtureSynthetic"),false)
            ||!Flag(TEXT("simulationClockArtificiallyAdvanced"),false)||!Flag(TEXT("actorsTeleported"),false)
            ||!wfexpue::Integer(*Root,TEXT("requestedNation"),2,2,Nation)||!wfexpue::Integer(*Root,TEXT("requestedDifficulty"),0,0,Difficulty)
            ||!Root->TryGetStringField(TEXT("actualCampaignGuid"),Id)||Id!=TEXT("94B2FA344043843F59B1D797B25571A3")
            ||!Root->TryGetArrayField(TEXT("checks"),Rows)||!Rows||Rows->Num()>4096)return false;
        int32 Failures=0;
        for(const auto& Value:*Rows)
        {const auto Row=Value.IsValid()?Value->AsObject():nullptr;bool Good=false;FString Name;
            if(!Row.IsValid()||!Row->TryGetBoolField(TEXT("passed"),Good)||!Row->TryGetStringField(TEXT("name"),Name))return false;
            if(!Good){if(Name!=TEXT("ordinary_public_report_return_to_operations"))return false;++Failures;}}
        if(Failures!=1||!FGuid::ParseExact(Id,EGuidFormats::Digits,ExpectedRecord.Id))return false;
        for(const TCHAR* Name:{TEXT("actualSavedRecordBeforeReportReturnV87"),TEXT("actualSavedRecord")})
        {
            const TSharedPtr<FJsonObject>* Record=nullptr;FString Guid,Mission;bool Available=false;double N=0;
            if(!Root->TryGetObjectField(Name,Record)||!Record||!Record->IsValid()
                ||!(*Record)->TryGetBoolField(TEXT("available"),Available)||!Available
                ||!(*Record)->TryGetStringField(TEXT("guid"),Guid)||Guid!=Id
                ||!(*Record)->TryGetStringField(TEXT("mission"),Mission)||Mission!=TEXT("fr.vaux"))return false;
            for(const auto& Pair:{TPair<const TCHAR*,int32>(TEXT("nation"),2),{TEXT("difficulty"),0},{TEXT("victories"),1},{TEXT("defeats"),0},{TEXT("completed"),1},{TEXT("researchPoints"),4}})
                if(!wfexpue::Integer(**Record,Pair.Key,Pair.Value,Pair.Value,N))return false;
        }
        ExpectedRecord.PlayerNation=EFRRUNationV81::French;ExpectedRecord.Difficulty=ExpectedDifficulty=EFrontlineDifficulty::Rookie;
        ExpectedRecord.MissionId=TEXT("fr.vaux");ExpectedLegacy=0;ExpectedNational=1;
        EarnedProof=MakeShared<FJsonObject>();EarnedProof->SetStringField(TEXT("sourceRun"),TEXT("62ca7c605b7c4357b98fe4033751d265"));
        EarnedProof->SetBoolField(TEXT("originalReturnFailureRetained"),true);EarnedProof->SetBoolField(TEXT("provesPriorLiveReturn"),false);
        EarnedProof->SetBoolField(TEXT("syntheticQualification"),false);
        return EarnedDisk(nullptr,false,true);
    }
    bool EarnedDisk(const AWarfareFrontlineGameMode* B,bool NewEncounter,bool Remember=false)
    {
        const FString Accounts=User/TEXT("Saved/FRRU/Account/achievements-v2.json");
        if(!EarnedFile(Accounts,TEXT("49858b77c7cad7b57652ddbc19b664bff4a654f2"),256*1024)
            ||!EarnedFile(Accounts+TEXT(".bak"),TEXT("49858b77c7cad7b57652ddbc19b664bff4a654f2"),256*1024))return false;
        for(int32 Backup=0;Backup<2;++Backup)
        {
            const FString Path=RepositoryPath()+(Backup?TEXT(".bak"):TEXT(""));const int64 Size=IFileManager::Get().FileSize(*Path);
            FString Text;TSharedPtr<FJsonObject> Root;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;double N=0;FString Active;
            if(Size<=0||Size>8*1024*1024||!FFileHelper::LoadFileToString(Text,*Path)
                ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid()
                ||!wfexpue::Integer(*Root,TEXT("hostSchema"),2,2,N)||!Root->TryGetStringField(TEXT("activeId"),Active)
                ||Active!=ExpectedRecord.Id.ToString(EGuidFormats::Digits)||!Root->TryGetArrayField(TEXT("campaigns"),Rows)||!Rows||Rows->Num()!=1)return false;
            const auto Row=(*Rows)[0]->AsObject();FString Id,Error;const TSharedPtr<FJsonObject>* Tactical=nullptr,*Combat=nullptr;wfexp::tactical::Campaign Native;
            if(!Row.IsValid()||!Row->TryGetStringField(TEXT("id"),Id)||Id!=Active||!wfexpue::Integer(*Row,TEXT("difficulty"),0,0,N)
                ||!Row->TryGetObjectField(TEXT("tactical"),Tactical)||!Tactical||!Tactical->IsValid()||!wfexpue::ReadTacticalCampaign(**Tactical,Native,Error)
                ||!Row->TryGetObjectField(TEXT("combat_record_v65"),Combat)||!Combat||!Combat->IsValid())return false;
            const auto& P=Native.progression;
            if(Native.faction!=wfexp::Faction::French||P.selectedMissionId!="fr.vaux"||P.victories!=1||P.defeats!=0||P.researchPoints!=4
                ||P.wonMissionIds!=std::vector<std::string>{"fr.mort_homme"}
                ||P.resolvedEncounterIds!=std::vector<std::string>{"67ed33d2-4eb9-8eb9-b0e6-0dabb87ced98"}
                ||!P.defeatedEncounterIds.empty()||!Native.purchasedOptions.empty()||P.technologyMask!=0
                ||!P.boughtTechnologyIds.empty()||!P.resolvedDispatchIds.empty()||!P.choiceIds.empty()||!P.consumedLoanDispatchIds.empty()
                ||!P.pendingLoanRoleId.empty()||!P.pendingLoanDispatchId.empty()||!P.pendingLoanMissionId.empty())return false;
            const FString Encounter=B?B->GetEncounterId().ToString(EGuidFormats::DigitsWithHyphens).ToLower():FString();
            if(NewEncounter&&Backup==0)
            {if(!B||!B->GetEncounterId().IsValid()||Encounter==TEXT("67ed33d2-4eb9-8eb9-b0e6-0dabb87ced98")
                ||wfexpue::Wide(P.activeEncounterId).ToLower()!=Encounter||P.activeEncounterMissionId!="fr.vaux")return false;}
            else if(!P.activeEncounterId.empty()&&(!NewEncounter||wfexpue::Wide(P.activeEncounterId).ToLower()!=Encounter||P.activeEncounterMissionId!="fr.vaux"))return false;
            else if(P.activeEncounterId.empty()&&!P.activeEncounterMissionId.empty())return false;
            FString Record;FJsonSerializer::Serialize(Combat->ToSharedRef(),TJsonWriterFactory<>::Create(&Record));
            if(Remember&&Backup==0)EarnedCombatRecord=Record;
            if(EarnedCombatRecord.IsEmpty()||Record!=EarnedCombatRecord)return false;
        }
        return true;
    }
    bool ReadEarnedScene(const AWarfareFrontlineGameMode* B)
    {
        SceneProof=WarfareFRRUContentV82::DescribeSceneRendering(B);FString Scene;bool Read=false;
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if(!SceneProof.IsValid()||!SceneProof->TryGetBoolField(TEXT("renderingStateRead"),Read)||!Read
            ||!SceneProof->TryGetStringField(TEXT("sceneId"),Scene)||Scene!=TEXT("fr.vaux.scene.families.v1")
            ||!SceneProof->TryGetArrayField(TEXT("components"),Rows)||!Rows||Rows->Num()>512)return false;
        int32 Buildings=0,Roots=0,Dressing=0,DressingComponents=0;TSet<FString> Names;
        for(const auto& Value:*Rows)
        {
            const auto Row=Value.IsValid()?Value->AsObject():nullptr;FString Name,MeshPath;bool Visible=false;
            if(!Row.IsValid()||!Row->TryGetStringField(TEXT("component"),Name)||!Row->TryGetStringField(TEXT("mesh"),MeshPath)
                ||!Row->TryGetBoolField(TEXT("visible"),Visible)||Names.Contains(Name))return false;Names.Add(Name);
            const bool Building=Name.StartsWith(TEXT("VauxTripoReleaseSolid")),Root=Name.StartsWith(TEXT("VauxRootsV87_")),Dress=Name.StartsWith(TEXT("OperationDressingV827_"));
            if(!Building&&!Root&&!Dress)continue;if(!Visible)return false;
            if(Building){if(!MeshPath.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxTripo/")))return false;++Buildings;}
            if(Root||Dress)
            {
                UInstancedStaticMeshComponent* Actual=nullptr;
                for(TActorIterator<AActor> It(B->GetWorld());It;++It)
                {TArray<UInstancedStaticMeshComponent*> Components;It->GetComponents(Components);
                    for(auto* C:Components)if(C&&C->GetName()==Name&&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==MeshPath){if(Actual)return false;Actual=C;}}
                if(!Actual||!Actual->IsRegistered()||!Actual->IsVisible()||Actual->bHiddenInGame||Actual->GetInstanceCount()<0)return false;
                Row->SetNumberField(TEXT("actualInstanceCount"),Actual->GetInstanceCount());
                if(Root){if(!MeshPath.StartsWith(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxComposition/ReleaseV1/"))
                    ||!Actual->GetStaticMesh()->bAllowCPUAccess||!WarfareFRRUVauxCompositionV87::ValidateComponent(Scene,Actual))return false;
                    Row->SetBoolField(TEXT("actualCPUAccess"),true);++Roots;}
                if(Dress){++DressingComponents;Dressing+=Actual->GetInstanceCount();}
            }
        }
        SceneProof->SetNumberField(TEXT("actualTripoBuildings"),Buildings);SceneProof->SetNumberField(TEXT("actualCPURootMeshes"),Roots);
        SceneProof->SetNumberField(TEXT("actualOperationDressingInstances"),Dressing);
        return Buildings==8&&Roots==3&&DressingComponents==8&&Dressing==43;
    }
    bool HumanInput()
    {
        auto* PC=ReviewPC.Get();if(!PC)return false;
#if PLATFORM_WINDOWS
        // Engine simulated input never changes hardware key state. In the
        // writable boxed mode this also catches the same Enter/number keys.
        if(Persistence)for(int32 KeyCode=1;KeyCode<255;++KeyCode)
            if((GetAsyncKeyState(KeyCode)&0x8000)!=0)return true;
#endif
        float X=0,Y=0;
        if(PC->GetMousePosition(X,Y))
        {
            if(MouseObserved&&(FMath::Abs(X-MouseObservedX)>.25f||FMath::Abs(Y-MouseObservedY)>.25f))return true;
            MouseObserved=true;MouseObservedX=X;MouseObservedY=Y;
        }
        if(ObservedKeys.IsEmpty())EKeys::GetAllKeys(ObservedKeys);
        for(const auto& ReviewKey:ObservedKeys)
            if(ReviewKey.IsDigital()&&ReviewKey!=EKeys::AnyKey&&ReviewKey!=EKeys::Enter&&ReviewKey!=EKeys::One
                &&ReviewKey!=EKeys::Two&&ReviewKey!=EKeys::Three&&ReviewKey!=EKeys::Four
                &&(PC->IsInputKeyDown(ReviewKey)||PC->WasInputKeyJustPressed(ReviewKey)||PC->WasInputKeyJustReleased(ReviewKey)))return true;
        return false;
    }
    int32 LegacyCount(const AWarfareFrontlineGameMode* B)const{return B->GetCampaignRuns().Num();}
    int32 NationalCount(const AWarfareFrontlineGameMode* B)const{return WarfareFRRUCampaignV82::Archives(B).Num();}
    FGuid Identity(const AWarfareFrontlineGameMode* B)const
    {return WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::CampaignId(B):B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();}
    bool Unchanged(const AWarfareFrontlineGameMode* B)const
    {return LegacyCount(B)==LegacyBefore&&NationalCount(B)==NationalBefore&&Identity(B)==IdentityBefore;}
    FString RepositoryPath()const{return User/TEXT("Saved/FRRU/Profiles/frru-tactical-v2.json");}
    bool DiskRecord(const FFRRUCampaignSummaryV82& Record,int32 Count,int32* Revision=nullptr)const
    {
        const FString Path=RepositoryPath();const int64 Bytes=IFileManager::Get().FileSize(*Path);
        if(Bytes<=0||Bytes>8*1024*1024)return false;
        FString Text;TSharedPtr<FJsonObject> Root;
        if(!FFileHelper::LoadFileToString(Text,*Path)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid())return false;
        double Schema=0,Version=0;FString Profile,Recent;
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if(!wfexpue::Integer(*Root,TEXT("hostSchema"),2,2,Schema)||!wfexpue::Integer(*Root,TEXT("revision"),1,MAX_int32,Version)
            ||!Root->TryGetStringField(TEXT("profile"),Profile)||Profile!=TEXT("tactical_v2")
            ||!Root->TryGetStringField(TEXT("activeId"),Recent)||Recent!=Record.Id.ToString(EGuidFormats::Digits)
            ||!Root->TryGetArrayField(TEXT("campaigns"),Rows)||!Rows||Rows->Num()!=Count)return false;
        int32 Matches=0;TSet<FGuid> Ids;
        for(const auto& Value:*Rows)
        {
            const auto Row=Value.IsValid()?Value->AsObject():nullptr;FString IdText;FGuid Id;double Difficulty=0;
            const TSharedPtr<FJsonObject>* Tactical=nullptr;wfexp::tactical::Campaign Native;FString Error;
            if(!Row.IsValid()||!Row->TryGetStringField(TEXT("id"),IdText)||!FGuid::ParseExact(IdText,EGuidFormats::Digits,Id)||!Id.IsValid()||Ids.Contains(Id)
                ||!wfexpue::Integer(*Row,TEXT("difficulty"),0,2,Difficulty)||!Row->TryGetObjectField(TEXT("tactical"),Tactical)
                ||!Tactical||!Tactical->IsValid()||!wfexpue::ReadTacticalCampaign(**Tactical,Native,Error))return false;
            Ids.Add(Id);
            if(Id==Record.Id)
            {
                if(int32(Difficulty)!=int32(Record.Difficulty)||Native.faction!=NativeFaction(Army)
                    ||wfexpue::Wide(Native.progression.selectedMissionId)!=Record.MissionId)return false;
                ++Matches;
            }
        }
        if(Revision)*Revision=int32(Version);return Matches==1;
    }
    bool ReadNewProof()
    {
        const FString Path=User/TEXT("FactionSelectionInputV82/new/result.json");const int64 Bytes=IFileManager::Get().FileSize(*Path);
        if(Bytes<=0||Bytes>256*1024)return false;
        FString Text;TSharedPtr<FJsonObject> Root;bool Good=false,WasInterrupted=true,LeftOpen=true,Shipping=false;FString Mode,Project;
        if(!FFileHelper::LoadFileToString(Text,*Path)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid()
            ||!Root->TryGetBoolField(TEXT("allPassed"),Good)||!Good||!Root->TryGetBoolField(TEXT("interrupted"),WasInterrupted)||WasInterrupted
            ||!Root->TryGetBoolField(TEXT("windowLeftOpen"),LeftOpen)||LeftOpen||!Root->TryGetStringField(TEXT("persistenceMode"),Mode)||Mode!=TEXT("new")
            ||!Root->TryGetBoolField(TEXT("shippingInjection"),Shipping)||Shipping!=!WITH_EDITOR
            ||!Root->TryGetStringField(TEXT("project"),Project)||Project!=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()))return false;
        const TSharedPtr<FJsonObject>* Proof=nullptr;FString Id,Nation,Mission,Repository;double Difficulty=0,Legacy=0,National=0;
        if(!Root->TryGetObjectField(TEXT("campaign"),Proof)||!Proof||!Proof->IsValid()
            ||!(*Proof)->TryGetStringField(TEXT("id"),Id)||!FGuid::ParseExact(Id,EGuidFormats::Digits,ExpectedRecord.Id)||!ExpectedRecord.Id.IsValid()
            ||!(*Proof)->TryGetStringField(TEXT("nation"),Nation)||Nation!=(Army==1?TEXT("fr"):TEXT("ru"))
            ||!(*Proof)->TryGetStringField(TEXT("missionId"),Mission)||Mission.IsEmpty()
            ||!(*Proof)->TryGetStringField(TEXT("repository"),Repository)||Repository!=RepositoryPath()
            ||!wfexpue::Integer(**Proof,TEXT("difficulty"),0,2,Difficulty)
            ||!wfexpue::Integer(**Proof,TEXT("legacyCount"),0,0,Legacy)||!wfexpue::Integer(**Proof,TEXT("nationalCount"),1,1,National))return false;
        ExpectedRecord.PlayerNation=Army==1?EFRRUNationV81::French:EFRRUNationV81::Russian;
        ExpectedRecord.Difficulty=EFrontlineDifficulty(int32(Difficulty));ExpectedRecord.MissionId=Mission;
        ExpectedDifficulty=ExpectedRecord.Difficulty;ExpectedLegacy=int32(Legacy);ExpectedNational=int32(National);
        return DiskRecord(ExpectedRecord,ExpectedNational);
    }
    bool SameContinuedRecord(const AWarfareFrontlineGameMode* B)const
    {
        FFRRUCampaignSummaryV82 Record;
        return WarfareFRRUCampaignV82::CurrentRecord(B,Record)&&Record.Id==ExpectedRecord.Id&&Record.PlayerNation==ExpectedRecord.PlayerNation
            &&Record.Difficulty==ExpectedRecord.Difficulty&&Record.MissionId==ExpectedRecord.MissionId
            &&WarfareFRRUCampaignV82::CampaignId(B)==Record.Id&&WarfareFRRUCampaignV82::Nation(B)==Record.PlayerNation
            &&WarfareFRRUCampaignV82::Difficulty(B)==Record.Difficulty&&WarfareFRRUCampaignV82::MissionId(B)==Record.MissionId
            &&WarfareFRRUCampaignV82::SelectedMission(B)&&LegacyCount(B)==ExpectedLegacy&&NationalCount(B)==ExpectedNational
            &&(!EarnedSource||(Record.Victories==1&&Record.Defeats==0&&Record.Completed==1&&Record.ResearchPoints==4));
    }
    bool RecordDiskProof(const AWarfareFrontlineGameMode* B)
    {
        FFRRUCampaignSummaryV82 Record;int32 Revision=0;
        if(!WarfareFRRUCampaignV82::CurrentRecord(B,Record)||!DiskRecord(Record,NationalCount(B),&Revision))return false;
        CampaignProof=MakeShared<FJsonObject>();CampaignProof->SetStringField(TEXT("id"),Record.Id.ToString(EGuidFormats::Digits));
        CampaignProof->SetStringField(TEXT("nation"),Army==1?TEXT("fr"):TEXT("ru"));CampaignProof->SetNumberField(TEXT("difficulty"),int32(Record.Difficulty));
        CampaignProof->SetStringField(TEXT("missionId"),Record.MissionId);CampaignProof->SetStringField(TEXT("repository"),RepositoryPath());
        CampaignProof->SetNumberField(TEXT("repositoryRevision"),Revision);CampaignProof->SetNumberField(TEXT("legacyCount"),LegacyCount(B));
        CampaignProof->SetNumberField(TEXT("nationalCount"),NationalCount(B));return true;
    }
    void Key(const FKey& ReviewKey,EInputEvent Event)
    {
        if(Event==IE_Pressed&&HumanInput()){Interrupted=true;Check(TEXT("interrupted_by_user_activity"),false);Finish();return;}
        if(auto* PC=ReviewPC.Get())
        {
            const FInputDeviceId Device=IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(PC->GetPlatformUserId());
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(ReviewKey,Event,Event==IE_Released?0.f:1.f,-1,Device));
        }
        if(Event==IE_Pressed)HeldKeys.AddUnique(ReviewKey);else HeldKeys.Remove(ReviewKey);
    }
    void Release()
    {const auto Keys=HeldKeys;for(const auto& ReviewKey:Keys)Key(ReviewKey,IE_Released);}
    void Check(const TCHAR* Name,bool Result)
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Result);
        Checks.Add(MakeShared<FJsonValueObject>(J));Passed&=Result;CasePassed&=Result;
    }
    void Advance(int32 Next)
    {if(!Finished){Step=Next;StepAt=FPlatformTime::Seconds();StepFrame=GFrameCounter;}}
    bool Delay(double Seconds=.12)const
    {return FPlatformTime::Seconds()-StepAt>=Seconds&&GFrameCounter>StepFrame+1;}
    bool Warm()const
    {
        if(!Images().NeutralMaterial.IsValid())return false;
        for(const auto& Material:Images().NeutralImages)if(!Material.IsValid())return false;
        if(LastDrawFrameForInputReview==MAX_uint64||GFrameCounter<LastDrawFrameForInputReview||GFrameCounter-LastDrawFrameForInputReview>2)return false;
        for(int32 I=0;I<5;++I)if(!Texture(I))return false;
        for(int32 I=0;I<4;++I)if(!HorizonTexture(I))return false;
        UFont* Fonts[]={Artwork().Sans.Get(),Artwork().ChineseMediumV45.Get()};
        for(UFont* Face:Fonts){if(!Face)return false;for(UTexture2D* Page:Face->Textures)if(!FrontlineTextureReady(Page))return false;}
        return Artwork().ControlRuntimeV62!=nullptr;
    }
    bool Shot(const TCHAR* Suffix)
    {
        if(FScreenshotRequest::IsScreenshotRequested())return false;
        if(EarnedSource)
        {auto* PC=ReviewPC.Get();int32 W=0,H=0;if(!PC)return false;PC->GetViewportSize(W,H);if(W!=3840||H!=2160)return false;}
        else if(!Warm())return false;
        const FString Path=Directory/TEXT("captures")/FString::Printf(TEXT("%d-%s.png"),Army+1,Suffix);
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("path"),Path);J->SetStringField(TEXT("kind"),TEXT("engine_injected_game_frame"));
        J->SetNumberField(TEXT("requestedFrame"),double(GFrameCounter));Captures.Add(MakeShared<FJsonValueObject>(J));
        FScreenshotRequest::RequestScreenshot(Path,true,false);return true;
    }
    void StoreCase()
    {
        if(!ArmyCase)return;ArmyCase->SetArrayField(TEXT("checks"),Checks);ArmyCase->SetBoolField(TEXT("passed"),CasePassed);
        Cases.Add(MakeShared<FJsonValueObject>(ArmyCase));ArmyCase.Reset();Checks.Empty();
    }
    void Finish()
    {
        if(Finished)return;
        if(EarnedSource&&Driving&&!Interrupted)
        {
            auto* PC=ReviewPC.Get();const auto* B=PC?FrontlineFor(PC):nullptr;
            Check(TEXT("approved_earned_source_files_remain_byte_identical"),EarnedFiles(false));
            Check(TEXT("earned_victory_rp_combat_record_and_account_not_rewarded_again"),SameContinuedRecord(B)&&EarnedDisk(B,Step>=105));
        }
        if(Persistence&&Driving&&!Interrupted)
        {
            auto* PC=ReviewPC.Get();
            if(!PC||!Focused(PC)||HumanInput()){Interrupted=true;Check(TEXT("uninterrupted_owned_review_at_finish"),false);}
        }
        Release();if(auto* PC=ReviewPC.Get())Cancel(PC);StoreCase();
        for(const auto& V:Captures)
        {auto J=V->AsObject();const bool Exists=IFileManager::Get().FileSize(*J->GetStringField(TEXT("path")))>0;J->SetBoolField(TEXT("exists"),Exists);Passed&=Exists;}
        auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("allPassed"),Passed);
        Report->SetStringField(TEXT("inputEvidence"),TEXT("UE PlayerController CreateSimulated input; not authentic OS/manual hold proof"));
        Report->SetStringField(TEXT("focusEvidence"),TEXT("actual focused viewport required; logical Cancel tested separately; no Windows focus-loss proof"));
        const bool CleanExit=Persistence&&Driving&&Passed&&!Interrupted&&CampaignProof.IsValid();
        Report->SetBoolField(TEXT("shippingInjection"),Persistence&&!WITH_EDITOR);Report->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-Began);
        Report->SetBoolField(TEXT("interrupted"),Interrupted);Report->SetBoolField(TEXT("windowLeftOpen"),!CleanExit);
        if(Persistence)
        {
            Report->SetStringField(TEXT("persistenceMode"),EarnedSource?TEXT("earned-vaux"):ColdContinue?TEXT("continue"):TEXT("new"));
            Report->SetStringField(TEXT("project"),FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
            Report->SetStringField(TEXT("scope"),TEXT("One public native campaign Start or cold Continue, actual boxed repository bytes and engine input; legacy progression remains volatile. No OS/manual hold, campaign completion or battle acceptance claim."));
            if(CampaignProof)Report->SetObjectField(TEXT("campaign"),CampaignProof);
            if(EarnedSource)
            {
                Report->SetStringField(TEXT("scope"),TEXT("Exact genuinely earned Editor Mort outcome retained with its original return failure; fresh isolated Shipping public Continue/deployment/introduction/loading to Vaux and one cooked production scene. No Shipping victory, prior live-return or full-campaign acceptance."));
                if(EarnedProof)Report->SetObjectField(TEXT("earnedSource"),EarnedProof);
                if(SceneProof)Report->SetObjectField(TEXT("sceneRenderingAtCaptureCompletion"),SceneProof);
                Report->SetNumberField(TEXT("actualIntroductionPages"),EarnedIntroPages);
                Report->SetBoolField(TEXT("actualLoadingPainted"),EarnedLoadingPainted);
            }
        }
        Report->SetArrayField(TEXT("cases"),Cases);Report->SetArrayField(TEXT("captures"),Captures);
        FString Text;const auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Report,Writer);
        const FString Path=Directory/TEXT("result.json");const bool Saved=FFileHelper::SaveStringToFile(Text,*Path);
        bool Exit=CleanExit&&Saved;
        if(Exit&&(!Focused(ReviewPC.Get())||HumanInput()))
        {
            Interrupted=true;Passed=false;Exit=false;Report->SetBoolField(TEXT("allPassed"),false);
            Report->SetBoolField(TEXT("interrupted"),true);Report->SetBoolField(TEXT("windowLeftOpen"),true);
            Text.Reset();FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
            if(!FFileHelper::SaveStringToFile(Text,*Path))IFileManager::Get().Delete(*Path);
        }
        UE_LOG(LogTemp,Display,TEXT("WF_FACTION_INPUT_REVIEW allPassed=%d saved=%d report=%s evidence=engine_injected"),Passed,Saved,*Path);Finished=true;
        if(Exit)FPlatformMisc::RequestExit(false);
    }
    bool LockedGeometry(const FFrontlineFactionSelectionV82& S)const
    {
        if(S.Selected!=Army||FMath::Abs(S.Widths[Army]-768.f)>3.f)return false;
        for(int32 I=0;I<4;++I)if(I!=Army&&FMath::Abs(S.Widths[I]-384.f)>3.f)return false;
        const auto Before=Layout(S);float Sum=0;for(float Width:S.Widths)Sum+=Width;
        if(FMath::Abs(Sum-1920.f)>.05f)return false;
        // Holding confirmation and a stationary pointer after keyboard input
        // both retain ownership. Only copies receive synthetic pointer values.
        for(int32 I=0;I<4;++I)
        {
            auto Copy=S;Copy.HoldSource=2;HoverSelectV826(Copy,I,true);
            if(ExpandedPanel(Copy)!=Army)return false;const auto After=Layout(Copy);
            for(int32 N=0;N<4;++N)if(Before.Panels[N].X!=After.Panels[N].X||Before.Panels[N].W!=After.Panels[N].W)return false;
            Copy=S;Copy.PointerOwnsSelectionV826=false;HoverSelectV826(Copy,I,false);
            if(ExpandedPanel(Copy)!=Army)return false;
        }
        auto Footer=S;HoverSelectV826(Footer,INDEX_NONE,true);
        if(ExpandedPanel(Footer)!=Army)return false;
        return true;
    }
    bool CorrectRecord(const AWarfareFrontlineGameMode* B)const
    {
        if(Army==1||Army==3)
        {
            FFRRUCampaignSummaryV82 Record;
            return WarfareFRRUCampaignV82::Active(B)&&WarfareFRRUCampaignV82::CurrentRecord(B,Record)
                &&Record.PlayerNation==(Army==1?EFRRUNationV81::French:EFRRUNationV81::Russian)&&Record.Difficulty==ExpectedDifficulty
                &&Record.Id.IsValid()&&Record.Id!=IdentityBefore&&LegacyCount(B)==LegacyBefore&&NationalCount(B)==NationalBefore+1;
        }
        const auto* Record=B->GetCurrentCampaign();
        return !WarfareFRRUCampaignV82::Active(B)&&Record&&Record->Faction==(Army==2?EFrontlineFaction::German:EFrontlineFaction::British)
            &&Record->Difficulty==ExpectedDifficulty&&Record->Id.IsValid()&&Record->Id!=IdentityBefore
            &&LegacyCount(B)==LegacyBefore+1&&NationalCount(B)==NationalBefore;
    }
    bool Tick(float)
    {
        if(Finished)return false;const double Now=FPlatformTime::Seconds();
        auto* World=ReviewWorld.Get();auto* PC=World?Cast<AWarfareFrontlineController>(World->GetFirstPlayerController()):nullptr;
        auto* B=PC?FrontlineFor(PC):nullptr;
        if(Now-Began>150.){Check(TEXT("bounded_deadline"),false);Finish();return false;}
        if(!PC||!B||!PC->PlayerInput)return true;ReviewPC=PC;
        if(!Focused(PC))
        {if(Driving){Interrupted=true;Check(TEXT("focus_retained_during_injection"),false);Finish();return false;}MouseObserved=false;IdleAt=Now;return true;}
        if(HumanInput())
        {
            if(!Driving){MouseObserved=false;IdleAt=Now;return true;}
            Interrupted=true;Check(TEXT("interrupted_by_user_activity"),false);Finish();return false;
        }
        if(B->Phase!=EFrontlinePhase::Briefing&&!(EarnedSource&&Step>=104&&B->Phase==EFrontlinePhase::Fighting))
        {Check(EarnedSource?TEXT("briefing_or_earned_loading_battle_only"):TEXT("briefing_only"),false);Finish();return false;}
        if(ArmyCase&&!CasePassed){Finish();return false;}
        auto& State=ScreenState(PC);auto& S=State.FactionSelectionV82;
        const FKey NumberKeys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
        if(Step==0)
        {
            if(!Driving&&(!MouseObserved||Now-IdleAt<.35))return true;
            if(State.Dossier!=0||State.Menu||State.Help||State.Settings||State.Credits||State.Technology||State.AchievementsV76||State.ExitConfirmV61)
            {Check(TEXT("starts_at_normal_main_menu"),false);Finish();return false;}
            if(PC->IsInputKeyDown(EKeys::Enter)||PC->IsInputKeyDown(EKeys::LeftMouseButton))return true;
            Driving=true;CasePassed=true;Checks.Empty();ArmyCase=MakeShared<FJsonObject>();
            const TCHAR* Names[]={TEXT("british"),TEXT("french"),TEXT("german"),TEXT("russian")};ArmyCase->SetStringField(TEXT("army"),Names[Army]);
            Check(TEXT("actual_host_and_campaign_available"),Available(Army));if(!CasePassed){Finish();return false;}
            LegacyBefore=LegacyCount(B);NationalBefore=NationalCount(B);IdentityBefore=Identity(B);ExpectedDifficulty=State.PendingDifficulty;
            ArmyCase->SetNumberField(TEXT("difficulty"),int32(ExpectedDifficulty));ArmyCase->SetNumberField(TEXT("legacyBefore"),LegacyBefore);ArmyCase->SetNumberField(TEXT("nationalBefore"),NationalBefore);
            if(Persistence)
            {
                auto* MenuHUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());
                Check(TEXT("public_main_menu_is_visible"),MenuHUD&&MenuHUD->GetPresentationPage()==0);
                Check(TEXT("legacy_progression_is_volatile"),B->IsProgressionVolatile());
                if(ColdContinue||EarnedSource)
                {
                    Check(EarnedSource?TEXT("approved_real_earned_source_proof_and_initial_bytes_match"):TEXT("first_cold_new_proof_and_disk_record_match"),EarnedSource?ReadEarnedProof():ReadNewProof());
                    if(EarnedSource)Check(TEXT("shipping_french_release_open_russian_release_closed"),!WITH_EDITOR&&WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::French)&&!WarfareFRRUCampaignV82::CampaignsAvailable(EFRRUNationV81::Russian));
                    Check(TEXT("cold_load_is_not_already_active"),!WarfareFRRUCampaignV82::Active(B)&&!IdentityBefore.IsValid());
                    Check(TEXT("cold_load_has_exact_saved_counts"),LegacyBefore==ExpectedLegacy&&NationalBefore==ExpectedNational);
                    auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());Check(TEXT("public_continue_hud_available"),HUD&&HUD->GetPresentationPage()==0);
                    if(!CasePassed){Finish();return false;}
                    HUD->NotifyHitBoxClick(TEXT("ContinueCampaign"));Advance(100);return true;
                }
                Check(TEXT("new_box_has_no_campaign_or_repository"),LegacyBefore==0&&NationalBefore==0&&!IdentityBefore.IsValid()
                    &&!IFileManager::Get().FileExists(*RepositoryPath())&&!IFileManager::Get().FileExists(*(RepositoryPath()+TEXT(".bak"))));
                if(!CasePassed){Finish();return false;}
            }
            Key(EKeys::Enter,IE_Pressed);Advance(1);return true;
        }
        if(Step==100&&Delay(.4))
        {
            Check(TEXT("public_cold_continue_restored_same_record"),State.Dossier==1917&&SameContinuedRecord(B));
            Check(TEXT("continued_record_matches_actual_disk_bytes"),RecordDiskProof(B));
            if(EarnedSource)Check(TEXT("public_continue_preserves_real_first_victory_and_account"),EarnedDisk(B,false));
            Advance(EarnedSource?101:17);return true;
        }
        if(EarnedSource&&Step==101&&Delay(.6))
        {
            Check(TEXT("earned_continue_reaches_public_operations_room"),State.Dossier==1917&&!State.Deployment&&!State.UnitIntroduction&&!State.Preparing&&SameContinuedRecord(B));
            if(!CasePassed){Finish();return false;}PC->BeginBattle();
            Check(TEXT("earned_public_begin_opens_deployment"),State.Deployment&&B->Phase==EFrontlinePhase::Briefing);Advance(102);return true;
        }
        if(EarnedSource&&Step==102&&Delay(.6))
        {
            auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());const auto Cards=UnitIntroductionCards(B);
            Check(TEXT("earned_vaux_has_legal_actual_roster_and_new_assignment_cards"),HUD&&State.Deployment&&!Cards.IsEmpty()&&Cards.Num()<=16
                &&WarfareFRRUCampaignV82::DeploymentBlockReason(B).IsEmpty()&&SameContinuedRecord(B));
            if(!CasePassed){Finish();return false;}HUD->NotifyHitBoxClick(TEXT("V32DeploymentConfirm"));
            Check(TEXT("earned_public_deployment_enters_actual_introduction"),State.UnitIntroduction&&State.UnitIntroductionPage==0&&!State.Preparing);Advance(103);return true;
        }
        if(EarnedSource&&Step==103&&Delay(.5))
        {
            auto* HUD=Cast<AWarfareFrontlineHUD>(PC->GetHUD());const auto Cards=UnitIntroductionCards(B);const int32 Page=State.UnitIntroductionPage;
            Check(TEXT("earned_actual_introduction_page_exists"),HUD&&State.UnitIntroduction&&Cards.IsValidIndex(Page)&&Page==EarnedIntroPages&&Page<16);
            if(!CasePassed){Finish();return false;}const bool Last=Page+1==Cards.Num();
            HUD->NotifyHitBoxClick(TEXT("UnitIntroductionBegin"));++EarnedIntroPages;
            Check(TEXT("earned_public_introduction_advances_without_bypass"),Last?(!State.UnitIntroduction&&State.Preparing
                &&State.AcceptedNationalIntroCampaignV827==ExpectedRecord.Id&&State.AcceptedNationalIntroMissionV827==TEXT("fr.vaux")):
                (State.UnitIntroduction&&State.UnitIntroductionPage==Page+1&&!State.Preparing));
            Advance(Last?104:103);return true;
        }
        if(EarnedSource&&Step==104)
        {
            EarnedLoadingPainted|=State.Preparing&&State.PreparationPainted;
            if(State.Preparing||B->Phase!=EFrontlinePhase::Fighting)return true;
            Check(TEXT("earned_public_loading_was_painted_and_started_real_vaux"),EarnedLoadingPainted&&!State.UnitIntroduction&&!State.Deployment
                &&WarfareFRRUWorldV82::Ready(B)&&WarfareFRRUWorldV82::SceneId(B)==TEXT("fr.vaux.scene.families.v1")&&SameContinuedRecord(B));
            Check(TEXT("earned_new_encounter_keeps_old_victory_rp_and_account"),EarnedDisk(B,true));
            if(!CasePassed){Finish();return false;}PC->TogglePause();
            Check(TEXT("earned_public_pause_preserves_scene_for_capture"),UGameplayStatics::IsGamePaused(B));Advance(105);return true;
        }
        if(EarnedSource&&Step==105&&Delay(1.))
        {
            Check(TEXT("earned_cooked_vaux_has_eight_tripo_three_cpu_roots_and_43_dressing"),ReadEarnedScene(B));
            if(!CasePassed){Finish();return false;}
            if(!Shot(TEXT("earned-vaux-production-scene")))return true;Advance(106);return true;
        }
        if(EarnedSource&&Step==106&&Delay(.5)&&!FScreenshotRequest::IsScreenshotRequested())
        {
            Check(TEXT("earned_capture_completed_with_same_cooked_scene"),ReadEarnedScene(B)&&UGameplayStatics::IsGamePaused(B)&&WarfareFRRUWorldV82::Ready(B));
            Check(TEXT("earned_final_repository_proof"),RecordDiskProof(B));Finish();return false;
        }
        if(Step==1)
        {
            if(!Visible(PC,B)||!S.Visible||!Delay(.85)||!Warm())return true;
            Check(TEXT("v826_opaque_selector_skips_only_covered_world"),PC->GetWorld()->GetGameViewport()->bDisableWorldRendering);
            ArmyCase->SetNumberField(TEXT("coveredWorldLastTickFrameV826"),LastWorldCheckFrameV826==MAX_uint64?-1:double(LastWorldCheckFrameV826));
            ArmyCase->SetNumberField(TEXT("coveredWorldCheckFrameV826"),GFrameCounter);
            ArmyCase->SetBoolField(TEXT("coveredWorldAtControllerTickV826"),LastWorldCoveredV826);
            ArmyCase->SetBoolField(TEXT("coveredWorldPreloadAtControllerTickV826"),LastWorldPreloadV826);
            ArmyCase->SetBoolField(TEXT("coveredWorldPreloadAtInputCheckV826"),PreloadSettled());
            ArmyCase->SetStringField(TEXT("coveredWorldImageErrorV826"),Images().Failure);
            Check(TEXT("held_entry_enter_cannot_confirm"),S.HoldSource==0&&S.CommitAt<0&&Unchanged(B));
            bool HoverPreviews=true,FooterRetains=true,KeyboardOwns=true,HoldRetains=true;
            for(int32 I=0;I<4;++I)
            {
                FFrontlineFactionSelectionV82 Copy;HoverSelectV826(Copy,I,false);
                HoverPreviews&=Copy.Selected==I&&ExpandedPanel(Copy)==I&&Copy.HoldSource==0&&Copy.CommitAt<0;
                HoverSelectV826(Copy,INDEX_NONE,true);FooterRetains&=Copy.Selected==I&&ExpandedPanel(Copy)==I;
                const int32 Other=(I+1)%4;Copy.PointerOwnsSelectionV826=false;HoverSelectV826(Copy,Other,false);
                KeyboardOwns&=Copy.Selected==I;HoverSelectV826(Copy,Other,true);KeyboardOwns&=Copy.Selected==Other;
                Copy.HoldSource=2;HoverSelectV826(Copy,I,true);HoldRetains&=Copy.Selected==Other;
            }
            Check(TEXT("v826_hover_without_click_expands_and_colours_each_available_army"),HoverPreviews);
            Check(TEXT("v826_pointer_travel_to_footer_retains_last_hovered_army"),FooterRetains);
            Check(TEXT("v826_keyboard_ownership_yields_only_to_pointer_movement"),KeyboardOwns);
            Check(TEXT("v826_active_confirmation_rejects_hover_retargeting"),HoldRetains);
            {FFrontlineFactionSelectionV82 Copy;HoverSelectV826(Copy,4,true);Check(TEXT("v826_unavailable_hover_cannot_enable_confirmation"),Copy.Selected==INDEX_NONE&&Copy.Hover==INDEX_NONE);}
            Key(EKeys::Enter,IE_Released);Advance(2);return true;
        }
        if(Step==2&&Delay()){Key(NumberKeys[Army],IE_Pressed);Advance(3);return true;}
        if(Step==3&&Delay())
        {
            Check(TEXT("fresh_number_selects_expected_army"),S.Selected==Army&&S.HoldSource==0&&Unchanged(B));
            Key(NumberKeys[Army],IE_Released);Advance(4);return true;
        }
        if(Step==4&&Delay(.65))
        {
            Check(TEXT("v826_confirm_and_keyboard_ownership_preserve_layout"),LockedGeometry(S));
            if(!Shot(TEXT("selected")))return true;Advance(5);return true;
        }
        if(Step==5&&Delay(.4)&&!FScreenshotRequest::IsScreenshotRequested())
        {Key(EKeys::Enter,IE_Pressed);Advance(6);return true;}
        if(Step==6)
        {
            if(S.HoldAt>=0&&Now-S.HoldAt>=.30)
            {ArmyCase->SetNumberField(TEXT("quickHoldMilliseconds"),(Now-S.HoldAt)*1000.);Check(TEXT("quick_hold_is_below_700ms"),Now-S.HoldAt<HoldSeconds&&S.CommitAt<0);Key(EKeys::Enter,IE_Released);Advance(7);}
            else if(S.CommitAt>=0){Check(TEXT("quick_hold_is_below_700ms"),false);Finish();return false;}return true;
        }
        if(Step==7&&Delay())
        {
            Check(TEXT("quick_release_creates_no_archive"),S.HoldSource==0&&S.CommitAt<0&&Unchanged(B));
            PC->ToggleMenu();Check(TEXT("escape_unlocks_first"),State.Dossier==1918&&S.Selected==INDEX_NONE);
            PC->ToggleMenu();Check(TEXT("escape_returns_to_main_menu_second"),State.Dossier==0&&Unchanged(B));
            Key(EKeys::Enter,IE_Pressed);Advance(8);return true;
        }
        if(Step==8&&Visible(PC,B)&&S.Visible&&Delay())
        {Key(EKeys::Enter,IE_Released);Advance(9);return true;}
        if(Step==9&&Delay()){Key(NumberKeys[Army],IE_Pressed);Advance(10);return true;}
        if(Step==10&&Delay()){Key(NumberKeys[Army],IE_Released);Advance(11);return true;}
        if(Step==11&&Delay(.3)){Key(EKeys::Enter,IE_Pressed);Advance(12);return true;}
        if(Step==12&&S.HoldAt>=0&&Delay())
        {
            Cancel(PC);Check(TEXT("logical_focus_cancel_resets_confirmation"),S.HoldSource==0&&S.HoldAt<0&&S.CommitAt<0&&Unchanged(B));
            Key(EKeys::Enter,IE_Released);Advance(13);return true;
        }
        if(Step==13&&Delay())
        {FullHoldAt=-1;SawPaint=false;TransitionShot=false;EarlyFlareChecked=false;Key(EKeys::Enter,IE_Pressed);Advance(14);return true;}
        if(Step==14)
        {
            if(S.HoldAt>=0)FullHoldAt=S.HoldAt;
            if(S.CommitAt>=0)
            {
                CommitObservedAt=S.CommitAt;ArmyCase->SetNumberField(TEXT("fullHoldMilliseconds"),(S.CommitAt-FullHoldAt)*1000.);
                Check(TEXT("700ms_hold_enters_transition_without_archive"),FullHoldAt>=0&&S.CommitAt-FullHoldAt>=HoldSeconds&&Unchanged(B));
                Advance(15);
            }
            return true;
        }
        if(Step==15)
        {
            const double Age=Now-CommitObservedAt;
            if(S.CommitAt>=0)
            {
                SawPaint|=S.CommitPainted&&S.PaintedFrame<GFrameCounter&&S.Widths[Army]>=1919.f;
                if(!EarlyFlareChecked&&Age>=1.2&&Age<1.9)
                {Check(TEXT("flare_before_commit_creates_no_archive"),Unchanged(B));EarlyFlareChecked=true;}
                if(!TransitionShot&&Age>=.9&&Age<1.9)TransitionShot=Shot(TEXT("flare"));
            }
            else if(State.Dossier==1917)
            {
                Check(TEXT("start_waited_for_drawn_full_panel_and_flare"),Age>=CommitSeconds&&(SawPaint||(LastCommitPaintedArmyForInputReview==Army&&LastCommitPaintedFrameForInputReview<GFrameCounter&&LastCommitPaintedFrameForInputReview>StepFrame))&&TransitionShot&&EarlyFlareChecked);
                Check(TEXT("public_start_created_exactly_one_correct_archive"),CorrectRecord(B));
                if(Persistence)Check(TEXT("created_record_matches_actual_disk_bytes"),RecordDiskProof(B));
                ArmyCase->SetStringField(TEXT("createdCampaign"),Identity(B).ToString(EGuidFormats::Digits));Advance(16);return true;
            }
            if(Age>10.){Check(TEXT("public_start_completed"),false);Finish();return false;}return true;
        }
        if(Step==16&&Delay(.4))
        {
            Check(TEXT("v826_world_rendering_restored_after_public_campaign_start"),!PC->GetWorld()->GetGameViewport()->bDisableWorldRendering);
            Check(TEXT("held_confirmation_never_repeats_start"),PC->IsInputKeyDown(EKeys::Enter)&&CorrectRecord(B));
            Key(EKeys::Enter,IE_Released);
            if(Persistence){Advance(17);return true;}
            StoreCase();PC->ToggleMenu();++Army;
            if(Army==4){Advance(17);return true;}Advance(0);return true;
        }
        if(Step==17&&Delay(.5)&&!FScreenshotRequest::IsScreenshotRequested()){Finish();return false;}
        return true;
    }
};
TWeakPtr<FRun> Running;
void Start(UWorld* World)
{
    FString User;if(!Box(User)||!World||(World->WorldType!=EWorldType::Game&&World->WorldType!=EWorldType::PIE)||Running.IsValid())
    {UE_LOG(LogTemp,Warning,TEXT("WF_FACTION_INPUT_REVIEW rejected: requires exact boxed visible editor or matching packaged Shipping location and no active run"));return;}
    auto Run=MakeShared<FRun>();Run->ReviewWorld=World;Run->Began=FPlatformTime::Seconds();Run->StepAt=Run->IdleAt=Run->Began;
    FString Nation;Run->Persistence=FParse::Value(FCommandLine::Get(),TEXT("WFV81FRRUPersistence="),Nation);Run->User=User;
    if(Run->Persistence)
    {
        Run->Army=Nation==TEXT("fr")?1:3;Run->ColdContinue=FParse::Param(FCommandLine::Get(),TEXT("WFV81FRRUContinue"));
        FString Earned;Run->EarnedSource=FParse::Value(FCommandLine::Get(),TEXT("WFV81FRRUEarnedSource="),Earned);
        Run->Directory=User/TEXT("FactionSelectionInputV82")/(Run->EarnedSource?TEXT("earned-vaux"):Run->ColdContinue?TEXT("continue"):TEXT("new"));
        if(IFileManager::Get().FileExists(*(Run->Directory/TEXT("result.json"))))
        {UE_LOG(LogTemp,Error,TEXT("WF_FACTION_INPUT_REVIEW rejected: existing boxed result is retained"));return;}
    }
    else Run->Directory=User/TEXT("FactionSelectionInputV82")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
    IFileManager::Get().MakeDirectory(*(Run->Directory/TEXT("captures")),true);Running=Run;
    UE_LOG(LogTemp,Display,TEXT("WF_FACTION_INPUT_REVIEW begin evidence=engine_injected directory=%s"),*Run->Directory);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float DT){return Run->Tick(DT);}),0.f);
}
FAutoConsoleCommandWithWorld Command(TEXT("WF.UI.ReviewFactionInputV82"),TEXT("Exact boxed engine-input regression; optional packaged single native Start/cold Continue; no OS/manual input claim."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World){Start(World);}));
}
}

// Presentation-local geometry/policy checks; pixel appearance is checked in
// the same focused live draws. No font, resolution or player setting is changed.
void RunInterfaceChecksV831(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto L=FactionSelectionV82::Layout(FFrontlineFactionSelectionV82());
    bool TrayFits=true,SeparateRows=true;
    TArray<FactionSelectionV82::FRect> Keys{L.Back,L.Confirm};
    for(const auto& R:L.Difficulty)Keys.Add(R);
    for(const auto& R:Keys)
    {
        TrayFits&=R.X>=40&&R.X+R.W<=1880&&R.Y>=948&&R.Y+R.H<=1068;
        SeparateRows&=R.Y+R.H<=1030;
    }
    Check(TEXT("v831_selector_all_hit_rects_inside_british_tray_inner_binding"),TrayFits);
    Check(TEXT("v831_selector_description_row_clear_of_keys_and_bottom_binding"),SeparateRows&&FactionSelectionV82::FooterDescriptionYV831>=1028+4
        &&FactionSelectionV82::FooterDescriptionYV831+FactionSelectionV82::FooterDescriptionHV831<=1068);
    bool RackFits=true;for(int32 Count=1;Count<=12;++Count)
        for(const auto& Cell:FrontlineRibbonRackV8222::Layout(Count,52,79,240,42,false))
            RackFits&=Cell.Min.X>=52&&Cell.Max.X<=292.001&&Cell.Min.Y>=79&&Cell.Max.Y<=121.001;
    Check(TEXT("v832_warroom_rack_below_headquarters_heading_clears_subtitle_and_map"),RackFits&&24+49<79&&52+240<318&&79+42<132);
    const auto Selected=SettingsFaceV831(true,true,false),Hover=SettingsFaceV831(true,false,true),Idle=SettingsFaceV831(true,false,false),Disabled=SettingsFaceV831(false,false,false);
    const auto Luma=[](const FLinearColor& C){return .2126f*C.R+.7152f*C.G+.0722f*C.B;};
    Check(TEXT("v831_settings_selection_paint_response_distinct_from_hover_idle_disabled"),
        Luma(Selected)>Luma(Idle)*1.7f&&Luma(Selected)>Luma(Hover)*1.15f&&Luma(Hover)>Luma(Idle)*1.35f&&Luma(Idle)>Luma(Disabled)*1.3f);
}
