#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineAnimation.h"
#include "WarfareFrontlineV11Verification.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "RHI.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "MeshUVChannelInfo.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"
#include "VertexFactory.h"
#include "SceneInterface.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
TSharedRef<FJsonObject> InspectFrontlineUIAssets()
{
    const auto R=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Assets; bool All=true;
    const TCHAR* Textures[]={TEXT("T_Keyart1917"),TEXT("T_Keyart1944"),TEXT("T_Portraits1917")};
    auto Path=[](const TCHAR* Name) { return FString(TEXT("/Game/Warfare/UI/V09/"))+Name+TEXT(".")+Name; };
    for (const TCHAR* Name:Textures)
    {
        UTexture2D* T=LoadObject<UTexture2D>(nullptr,*Path(Name)); const bool Good=T && T->GetSizeX()>0 && T->GetSizeY()>0;
        const auto A=MakeShared<FJsonObject>(); A->SetStringField(TEXT("path"),Path(Name)); A->SetBoolField(TEXT("loaded"),Good);
        A->SetNumberField(TEXT("width"),T?T->GetSizeX():0); A->SetNumberField(TEXT("height"),T?T->GetSizeY():0);
        Assets.Add(MakeShared<FJsonValueObject>(A)); All&=Good;
    }
    // V14 retires the flat emblems. Verify every actual replacement painting.
    const TCHAR* Paintings[]={TEXT("T_Tech_Marksmanship"),TEXT("T_Tech_Reload"),TEXT("T_Tech_Crossfire"),TEXT("T_Tech_Artillery"),TEXT("T_Tech_Grenadier"),TEXT("T_Tech_Supply"),TEXT("T_Support_Artillery"),TEXT("T_Support_Smoke"),TEXT("T_Support_Mortar"),TEXT("T_Portrait_British_Sniper"),TEXT("T_Portrait_British_Officer"),TEXT("T_Portrait_German_Sniper"),TEXT("T_Portrait_German_Officer")};
    for (const TCHAR* Name:Paintings)
    {
        const FString AssetPath=FString::Printf(TEXT("/Game/Warfare/UI/V14/%s.%s"),Name,Name);
        UTexture2D* T=LoadObject<UTexture2D>(nullptr,*AssetPath);
        const bool Good=T && T->GetSizeX()>=512 && T->GetSizeY()>=512;
        const auto A=MakeShared<FJsonObject>(); A->SetStringField(TEXT("path"),AssetPath); A->SetBoolField(TEXT("loaded"),Good);
        A->SetNumberField(TEXT("width"),T?T->GetSizeX():0); A->SetNumberField(TEXT("height"),T?T->GetSizeY():0);
        Assets.Add(MakeShared<FJsonValueObject>(A)); All&=Good;
    }
    for (const TCHAR* Name:{TEXT("F_UI"),TEXT("F_Title")})
    {
        UFont* Font=LoadObject<UFont>(nullptr,*Path(Name)); const bool Good=Font && Font->FontCacheType==EFontCacheType::Runtime;
        const auto A=MakeShared<FJsonObject>(); A->SetStringField(TEXT("path"),Path(Name)); A->SetBoolField(TEXT("loaded_runtime_font"),Good);
        Assets.Add(MakeShared<FJsonValueObject>(A)); All&=Good;
    }
    UTexture2D* CommandSurface=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Warfare/UI/V12/T_CommandSurface.T_CommandSurface"));
    All&=CommandSurface && CommandSurface->GetSizeX()>=2048;
    R->SetBoolField(TEXT("v12_command_surface_loaded_at_source_resolution"),CommandSurface && CommandSurface->GetSizeX()>=2048);
    for (const TCHAR* Name:{TEXT("F_UIFace"),TEXT("F_TitleFace")})
    {
        UFontFace* Face=LoadObject<UFontFace>(nullptr,*Path(Name)); const auto A=MakeShared<FJsonObject>();
        A->SetStringField(TEXT("path"),Path(Name)); A->SetBoolField(TEXT("loaded_font_face"),Face!=nullptr);
        Assets.Add(MakeShared<FJsonValueObject>(A)); All&=Face!=nullptr;
    }
    R->SetArrayField(TEXT("assets"),Assets); R->SetBoolField(TEXT("passed"),All);
    R->SetBoolField(TEXT("cjk_glyph_visual_readability_verified"),false);
    R->SetStringField(TEXT("scope"),TEXT("Runtime load of current key art, portrait atlas, thirteen V14 realistic paintings and fonts. Retired flat icons are no longer a product requirement. Actual Chinese glyph legibility and composition require screenshot review."));
    return R;
}

TSharedRef<FJsonObject> InspectFrontlineGround(UWorld* World)
{
    const TSharedRef<FJsonObject> R=MakeShared<FJsonObject>();
    AWarfareFrontlineGround* Ground=nullptr;
    int32 Count=0;
    for (TActorIterator<AWarfareFrontlineGround> It(World);It;++It) { Ground=*It; ++Count; }
    UStaticMeshComponent* Surface=Ground?Ground->Surface.Get():nullptr;
    UStaticMesh* Mesh=Surface?Surface->GetStaticMesh():nullptr;
    const bool RenderData=Mesh && Mesh->HasValidRenderData();
    const bool Visible=Ground && Surface && Surface->IsRegistered() && Surface->IsVisible()
        && !Surface->bHiddenInGame && !Ground->IsHidden();
    const bool Collision=Surface && Surface->IsQueryCollisionEnabled()
        && Surface->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block;
    const FMeshUVChannelInfo* UV=Mesh && !Mesh->GetStaticMaterials().IsEmpty()
        ? &Mesh->GetStaticMaterials()[0].UVChannelData : nullptr;
    const bool Streaming=UV && UV->IsInitialized() && FMath::IsFinite(UV->LocalUVDensities[0])
        && UV->LocalUVDensities[0]>0;
    R->SetNumberField(TEXT("ground_actor_count"),Count);
    R->SetBoolField(TEXT("render_data_present"),RenderData);
    R->SetBoolField(TEXT("render_visibility_flags"),Visible);
    R->SetBoolField(TEXT("collision_enabled"),Collision);
    R->SetBoolField(TEXT("mesh_allow_cpu_access"),Mesh && Mesh->bAllowCPUAccess);
    R->SetBoolField(TEXT("uv_streaming_metadata_initialized"),Streaming);
    R->SetNumberField(TEXT("lod0_triangles"),RenderData?Mesh->GetNumTriangles(0):0);
    R->SetBoolField(TEXT("pixel_visibility_verified"),false);
    R->SetStringField(TEXT("scope"),TEXT("Actual world collision traces plus render-data flags; inspect screenshots for visible terrain. No physical mouse input is tested."));
    TArray<TSharedPtr<FJsonValue>> Traces;
    const FVector2D Points[]={{-5600,0},{-3300,0},{0,0},{3300,0},{5600,0},{-1600,-220},{1650,220}};
    int32 Passed=0;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FrontlineGroundVerification),true);
    for (TActorIterator<AActor> It(World);It;++It)
        if (*It!=Ground) Params.AddIgnoredActor(*It);
    for (const FVector2D& P:Points)
    {
        const double Z=AWarfareFrontlineGround::HeightAt(P.X,P.Y);
        FHitResult Hit;
        const bool HitGround=World->LineTraceSingleByChannel(Hit,FVector(P.X,P.Y,Z+1500),
            FVector(P.X,P.Y,Z-400),ECC_Visibility,Params) && Hit.GetComponent()==Surface;
        const double Error=HitGround?FMath::Abs(Hit.ImpactPoint.Z-Z):99999.;
        const bool Good=HitGround && Error<15.;
        Passed+=Good?1:0;
        const TSharedRef<FJsonObject> Item=MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("x_cm"),P.X); Item->SetNumberField(TEXT("y_cm"),P.Y);
        Item->SetNumberField(TEXT("expected_z_cm"),Z); Item->SetNumberField(TEXT("height_error_cm"),Error);
        Item->SetBoolField(TEXT("hit_ground_surface"),HitGround); Item->SetBoolField(TEXT("passed"),Good);
        Traces.Add(MakeShared<FJsonValueObject>(Item));
    }
    R->SetArrayField(TEXT("traces"),Traces);
    R->SetBoolField(TEXT("passed"),Count==1 && RenderData && Visible && Collision && Streaming
        && Mesh->bAllowCPUAccess && Passed==UE_ARRAY_COUNT(Points));
    return R;
}

TSharedRef<FJsonObject> InspectFrontlineMaterials(UWorld* World)
{
    struct FSnapshot
    {
        FString Path,CompileErrors,ResolvedName;
        TArray<FString> BasePassShaders;
        FMaterialRenderProxy* Proxy=nullptr;
        bool GTValid=false,GTComplete=false,RTComplete=false,RTMapValid=false,RTFinalized=false,RTCompiled=false,NoFallback=false;
        int32 Platform=-1,Quality=-1,CachedShaders=0,ReadyBasePassVertexShaders=0,ReadyBasePassPixelShaders=0,ReadyNaniteShadingShaders=0;
    };
    TArray<FSnapshot> Snapshots;
    const ERHIFeatureLevel::Type FeatureLevel=World->GetFeatureLevel();
    for (const TCHAR* Name:{TEXT("M_1917_Mud_V12"),TEXT("M_1917_Earthbanks_V12"),TEXT("M_1917_WeatheredTimber_V12"),TEXT("M_1917_Hessian_V12"),TEXT("M_1917_FieldIron_V12"),TEXT("M_1917_DistantFront_V12"),TEXT("M_ImpactVolume_0_V12"),TEXT("M_ImpactVolume_1_V12"),TEXT("M_ImpactVolume_2_V12"),TEXT("M_ImpactFlash_V12")})
    {
        FSnapshot& S=Snapshots.AddDefaulted_GetRef();
        S.Path=FString(TEXT("/Game/Warfare/Field/V12/Materials/"))+Name+TEXT(".")+Name;
        UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,*S.Path);
        S.Proxy=Material?Material->GetRenderProxy():nullptr;
        const FMaterialResource* Resource=Material && Material->GetMaterial() && World->Scene?Material->GetMaterial()->GetMaterialResource(World->Scene->GetShaderPlatform()):nullptr;
        if (Resource)
        {
            S.GTValid=Resource->HasValidGameThreadShaderMap(); S.GTComplete=Resource->IsGameThreadShaderMapComplete();
            S.Platform=int32(Resource->GetShaderPlatform()); S.Quality=int32(Resource->GetQualityLevel());
#if WITH_EDITOR
            S.CompileErrors=FString::Join(Resource->GetCompileErrors(),TEXT("; "));
#endif
        }
    }
    // UE 5.8's base pass tries the required cached shaders on GetMaterialNoFallback;
    // it does not require every unused material permutation to be compiled.
    // Inspect the non-default proxy and its real local-mesh base-pass GPU shaders
    // after the natural match. Whole-map completion remains diagnostic only.
    // No UObject state is modified. Flush keeps the local snapshot lifetime valid.
    ENQUEUE_RENDER_COMMAND(WarfareVerifyFieldMaterials)([&Snapshots,FeatureLevel](FRHICommandListImmediate&)
    {
        for (FSnapshot& S:Snapshots) if (S.Proxy)
        {
            const FMaterial* Direct=S.Proxy->GetMaterialNoFallback(FeatureLevel);
            if (Direct)
            {
                S.RTComplete=Direct->IsRenderingThreadShaderMapComplete();
                const FMaterialShaderMap* Map=Direct->GetRenderingThreadShaderMap();
                S.RTMapValid=Map && Map->IsValidForRendering();
                S.RTFinalized=Map && Map->IsCompilationFinalized(); S.RTCompiled=Map && Map->CompiledSuccessfully();
                const FMaterial& Resolved=S.Proxy->GetIncompleteMaterialWithFallback(FeatureLevel);
                S.NoFallback=&Resolved==Direct && !Direct->IsDefaultMaterial(); S.ResolvedName=Resolved.GetFriendlyName();
                if (Map)
                {
                    TMap<FShaderId,TShaderRef<FShader>> Shaders; Map->GetShaderList(Shaders); S.CachedShaders=Shaders.Num();
                    for (const auto& Entry:Shaders)
                    {
                        const FString VFName=Entry.Key.VFType?Entry.Key.VFType->GetName():TEXT("");
                        const FString TypeName=Entry.Value.GetType()->GetName();
                        // Nanite's material base pass is compute shading. It has
                        // no local-mesh vertex/pixel pair (BasePassRendering.h).
                        if (TypeName.StartsWith(TEXT("TBasePassCS")) && Entry.Value->GetFrequency()==SF_Compute && Entry.Value.GetComputeShader())
                        { ++S.ReadyNaniteShadingShaders; S.BasePassShaders.AddUnique(VFName+TEXT(": ")+TypeName); }
                        const bool LocalMesh=VFName==TEXT("FLocalVertexFactory") || VFName==TEXT("FInstancedStaticMeshVertexFactory");
                        if (!LocalMesh) continue;
                        if (TypeName.StartsWith(TEXT("TBasePassVS")) && Entry.Value->GetFrequency()==SF_Vertex && Entry.Value.GetVertexShader())
                        { ++S.ReadyBasePassVertexShaders; S.BasePassShaders.AddUnique(VFName+TEXT(": ")+TypeName); }
                        if (TypeName.StartsWith(TEXT("TBasePassPS")) && Entry.Value->GetFrequency()==SF_Pixel && Entry.Value.GetPixelShader())
                        { ++S.ReadyBasePassPixelShaders; S.BasePassShaders.AddUnique(VFName+TEXT(": ")+TypeName); }
                    }
                }
            }
        }
    });
    FlushRenderingCommands();
    auto Result=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Materials; bool Passed=Snapshots.Num()==10;
    for (const FSnapshot& S:Snapshots)
    {
        const bool ShadersReady=(S.ReadyBasePassVertexShaders>0 && S.ReadyBasePassPixelShaders>0) || S.ReadyNaniteShadingShaders>0;
        const bool Valid=S.RTMapValid && S.NoFallback && ShadersReady && S.CompileErrors.IsEmpty();
        Passed&=Valid; auto M=MakeShared<FJsonObject>(); M->SetStringField(TEXT("path"),S.Path);
        M->SetBoolField(TEXT("required_base_pass_shaders_ready"),Valid); M->SetBoolField(TEXT("game_thread_map_valid"),S.GTValid);
        M->SetBoolField(TEXT("game_thread_map_complete"),S.GTComplete); M->SetBoolField(TEXT("render_thread_map_complete"),S.RTComplete);
        M->SetBoolField(TEXT("render_map_valid"),S.RTMapValid); M->SetBoolField(TEXT("compilation_finalized"),S.RTFinalized);
        M->SetBoolField(TEXT("compiled_successfully"),S.RTCompiled); M->SetBoolField(TEXT("rendered_without_fallback"),S.NoFallback);
        M->SetStringField(TEXT("resolved_material"),S.ResolvedName); M->SetStringField(TEXT("compile_errors"),S.CompileErrors);
        M->SetNumberField(TEXT("shader_platform"),S.Platform); M->SetNumberField(TEXT("quality_level"),S.Quality);
        M->SetNumberField(TEXT("cached_shader_count"),S.CachedShaders); M->SetNumberField(TEXT("base_pass_vertex_gpu_shaders"),S.ReadyBasePassVertexShaders);
        M->SetNumberField(TEXT("base_pass_pixel_gpu_shaders"),S.ReadyBasePassPixelShaders); M->SetStringField(TEXT("base_pass_shader_names"),FString::Join(S.BasePassShaders,TEXT("; ")));
        M->SetNumberField(TEXT("nanite_base_pass_compute_gpu_shaders"),S.ReadyNaniteShadingShaders);
        Materials.Add(MakeShared<FJsonValueObject>(M));
    }
    Result->SetBoolField(TEXT("passed"),Passed); Result->SetArrayField(TEXT("materials"),Materials);
    Result->SetStringField(TEXT("scope"),TEXT("After the match and two real artillery effects, verify non-default render proxies and cached GPU base-pass shaders: local-mesh vertex/pixel pairs or Nanite compute shading. Unused whole-map permutations remain diagnostic. This verifies shader readiness, not individual GPU dispatch or artistic quality."));
    return Result;
}

struct FFrontlineVerification
{
    double Started=FPlatformTime::Seconds(),StageStarted=0,MatchStarted=0,PauseStarted=0;
    uint64 StartFrame=GFrameCounter;
    int32 Stage=0,RecruitStep=0;
    FString Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V12"));
    FFrontlineV11Verification NaturalActions,FixtureActions;
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Checks,Events;
    TMap<FString,FDateTime> Screenshots;
    TMap<uint32,FVector> PausedPositions;
    TMap<uint32,float> PausedPhases;
    TMap<int32,FVector> HeldCentres,ReleasedCentres;
    float PausedElapsed=0,NextOrderAt=40,HoldStarted=-1,ReleasedAt=0,FixtureImpactAt=0;
    int32 PeakSquads[2]={0,0},PeakTrenchOccupancy=0,PeakEngaging=0;
    bool bAnyFailure=false,bScreenshotPathsCleared=true,bPauseDone=false,bPaused=false;
    bool bCapacityInvariant=true,bWalletInvariant=true,bHeld=false,bReleased=false,bReleaseMoved=false;
    bool bRoleRecruited[3]={false,false,false},bEnemyRoles[AWarfareFrontlineGameMode::RoleCount]={};
    bool bBattleImage=false,bNaturalArtillery=false,bTimedOut=false;
    bool bMotionProfilesFinite=true,bAnimationAdvanced=false,bPlaybackRateVaried=false;
    int64 GripConstraintFrames=0;
    float MaximumGripResidual=0,MaximumGripCorrection=0;
    float MaxDepartureSpread=0,MaxFirstShotSpread=0,MaxGaitSpread=0;
    int32 ShotsWhileMoving=0;
    void Check(const TCHAR* Name,bool Passed,const FString& Detail=FString())
    {
        auto C=MakeShared<FJsonObject>(); C->SetStringField(TEXT("name"),Name);
        C->SetBoolField(TEXT("passed"),Passed); C->SetStringField(TEXT("detail"),Detail);
        Checks.Add(MakeShared<FJsonValueObject>(C)); bAnyFailure|=!Passed;
        UE_LOG(LogTemp,Display,TEXT("WF_FRONTLINE_CHECK %s %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name,*Detail);
    }
    void Event(const TCHAR* Name,float Time,const FString& Detail=FString())
    {
        auto E=MakeShared<FJsonObject>(); E->SetStringField(TEXT("event"),Name);
        E->SetNumberField(TEXT("simulation_seconds"),Time); E->SetStringField(TEXT("detail"),Detail);
        Events.Add(MakeShared<FJsonValueObject>(E));
    }
    void Screenshot(const TCHAR* Name)
    {
        const FString Path=Directory/Name; IFileManager::Get().Delete(*Path,false,false,true);
        bScreenshotPathsCleared&=!IFileManager::Get().FileExists(*Path); Screenshots.Add(Path,FDateTime::UtcNow());
        FScreenshotRequest::RequestScreenshot(Path,true,false);
    }
    bool FinishScreenshots()
    {
        bool All=bScreenshotPathsCleared && Screenshots.Num()>=5;
        TArray<TSharedPtr<FJsonValue>> Files;
        for (const auto& P:Screenshots)
        {
            const int64 Bytes=IFileManager::Get().FileSize(*P.Key);
            const auto Stamp=IFileManager::Get().GetTimeStamp(*P.Key);
            const bool Fresh=Bytes>1000 && Stamp>=P.Value-FTimespan::FromSeconds(2);
            All&=Fresh; auto F=MakeShared<FJsonObject>(); F->SetStringField(TEXT("path"),P.Key);
            F->SetNumberField(TEXT("bytes"),double(Bytes)); F->SetBoolField(TEXT("fresh_for_this_run"),Fresh);
            F->SetBoolField(TEXT("visual_reviewed"),false); Files.Add(MakeShared<FJsonValueObject>(F));
        }
        Report->SetArrayField(TEXT("screenshots"),Files); return All;
    }
};

TSharedRef<FJsonObject> ObserveMatch(AWarfareFrontlineGameMode* B)
{
    auto R=MakeShared<FJsonObject>();
    R->SetStringField(TEXT("phase"),B->Phase==EFrontlinePhase::Victory?TEXT("victory"):B->Phase==EFrontlinePhase::Defeat?TEXT("defeat"):B->Phase==EFrontlinePhase::Fighting?TEXT("fighting"):TEXT("briefing"));
    R->SetStringField(TEXT("end_reason"),B->EndReason); R->SetNumberField(TEXT("simulation_seconds"),B->Elapsed);
    R->SetNumberField(TEXT("shots_fired"),B->ShotsFired); R->SetNumberField(TEXT("friendly_morale"),B->Morale[0]);
    R->SetNumberField(TEXT("enemy_morale"),B->Morale[1]); R->SetNumberField(TEXT("friendly_casualties"),B->Casualties[0]);
    R->SetNumberField(TEXT("enemy_casualties"),B->Casualties[1]); R->SetNumberField(TEXT("friendly_damage"),B->DamageDealt[0]);
    R->SetNumberField(TEXT("enemy_damage"),B->DamageDealt[1]); R->SetNumberField(TEXT("trench_captures"),B->TrenchCaptures);
    R->SetNumberField(TEXT("recruit_commands"),B->RecruitCommands); R->SetNumberField(TEXT("advance_commands"),B->AdvanceCommands);
    R->SetNumberField(TEXT("artillery_commands"),B->ArtilleryCommands); R->SetNumberField(TEXT("grenades_thrown"),B->GrenadesThrown);
    TArray<TSharedPtr<FJsonValue>> Squads,Motion;
    for (const FFrontlineSquad& S:B->Squads)
    {
        auto Q=MakeShared<FJsonObject>(); Q->SetNumberField(TEXT("id"),S.Id); Q->SetNumberField(TEXT("team"),S.Team);
        Q->SetNumberField(TEXT("role_index"),int32(S.Role)); Q->SetNumberField(TEXT("living"),S.LivingCount());
        Q->SetNumberField(TEXT("trench"),S.Trench); Q->SetBoolField(TEXT("advancing"),S.bAdvancing);
        Squads.Add(MakeShared<FJsonValueObject>(Q));
    }
    for (const AWarfareFrontlineSoldier* U:B->Units)
    {
        auto M=MakeShared<FJsonObject>(); M->SetNumberField(TEXT("squad"),U->SquadId); M->SetNumberField(TEXT("member"),U->MemberIndex);
        M->SetNumberField(TEXT("first_movement_seconds"),U->FirstMovementAt); M->SetNumberField(TEXT("first_shot_seconds"),U->FirstShotAt);
        M->SetNumberField(TEXT("gait_scale"),U->GetMovementScale()); M->SetNumberField(TEXT("shots_while_moving"),U->ShotsWhileMoving);
        M->SetNumberField(TEXT("motion_transitions"),U->MotionTransitions); Motion.Add(MakeShared<FJsonValueObject>(M));
    }
    R->SetArrayField(TEXT("squads"),Squads); R->SetArrayField(TEXT("individual_motion"),Motion); return R;
}
}

void AWarfareFrontlineGameMode::BeginVerification()
{
    const auto V=MakeShared<FFrontlineVerification>(); IFileManager::Get().MakeDirectory(*V->Directory,true);
    IFileManager::Get().Delete(*(V->Directory/TEXT("frontline-verification.json")),false,false,true);
    V->Report->SetStringField(TEXT("run_id"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
    V->Report->SetStringField(TEXT("started_utc"),FDateTime::UtcNow().ToIso8601());
    V->Report->SetStringField(TEXT("kind"),TEXT("V12 Armor Games 1917 role, rendering and death revision; natural HUD-command match plus separately labelled fixtures"));
    V->Report->SetBoolField(TEXT("physical_mouse_input_tested"),false);
    V->Report->SetBoolField(TEXT("health_morale_position_or_cooldown_modified_in_single_match"),false);
    V->Report->SetStringField(TEXT("single_match_policy"),TEXT("Normal 1.5x speed; recruit rifle, then wait for Lewis, then assault; hold three squads before releasing trenches from 40 seconds; target enemy with artillery when naturally ready. No resources, health, morale, cooldown, position or terminal state is modified in this match."));
    V->Report->SetStringField(TEXT("limitations"),TEXT("One policy and seed do not establish balance. HUD NotifyHitBoxClick and Controller world-point confirmation do not establish physical mouse hit testing. Screenshots require visual review. Readiness and current squad sizes follow the Armor Games Flash release; this is not a complete campaign reproduction."));
    TWeakObjectPtr<AWarfareFrontlineGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(TEXT("WarfareFrontlineVerification"),.05f,[WeakThis,V](float)
    {
        AWarfareFrontlineGameMode* B=WeakThis.Get(); if (!B) return false;
        auto* PC=Cast<AWarfareFrontlineController>(UGameplayStatics::GetPlayerController(B,0));
        auto* HUD=PC?Cast<AWarfareFrontlineHUD>(PC->GetHUD()):nullptr;
        auto UI=[HUD](const TCHAR* Name) { if (HUD) HUD->NotifyHitBoxClick(FName(Name)); };
        const double Now=FPlatformTime::Seconds(),Wall=Now-V->Started;
        if (V->Stage>=3 && V->Stage<=5) V->NaturalActions.Sample(B);
        if (V->Stage==6 || V->Stage==7 || V->Stage==9) V->FixtureActions.Sample(B);
        if (V->Stage==0 && Wall>.75 && PC && HUD)
        {
            // The scripted HUD driver owns this unattended run. Physical mouse
            // and keyboard validation is a separate interactive run; incidental
            // desktop input must not open a menu and invalidate its match clock.
            PC->DisableInput(PC);
            V->Report->SetBoolField(TEXT("external_player_input_disabled_for_scripted_run"),true);
            auto Assets=InspectFrontlineUIAssets(); V->Report->SetObjectField(TEXT("ui_assets"),Assets);
            V->Check(TEXT("existing_ui_assets_load"),Assets->GetBoolField(TEXT("passed")));
            V->Check(TEXT("character_and_motion_assets_available"),B->AssetFailure.IsEmpty() && B->LoadedSoldier && B->WalkAnimation && B->RunAnimation && B->AimAnimation && B->FireAnimation);
            V->Report->SetStringField(TEXT("character_mesh_path"),GetPathNameSafe(B->LoadedSoldier));
            bool NormalizedRoot=false;
            for (const AWarfareFrontlineSoldier* Unit:B->Units) if (Unit && Unit->Soldier)
            {
                const FVector Scale=Unit->Soldier->GetBoneTransform(0).GetScale3D();
                const float HeadHeight=Unit->Soldier->GetBoneLocation(TEXT("Head")).Z-Unit->Feet().Z;
                V->Report->SetStringField(TEXT("actual_character_root_world_scale"),Scale.ToString());
                V->Report->SetNumberField(TEXT("actual_idle_head_height_cm"),HeadHeight);
                NormalizedRoot=Scale.Equals(FVector::OneVector,.01f) && HeadHeight>140.f && HeadHeight<195.f;
                break;
            }
            V->Check(TEXT("normalized_character_root_scale_and_world_height"),NormalizedRoot);
            auto Ground=InspectFrontlineGround(B->GetWorld()); V->Report->SetObjectField(TEXT("ground"),Ground);
            V->Check(TEXT("ground_render_data_and_real_collision"),Ground->GetBoolField(TEXT("passed")));
            FVector2D L=FVector2D::ZeroVector,R=FVector2D::ZeroVector; int32 W=0,H=0; PC->GetViewportSize(W,H);
            const bool Projected=PC->ProjectWorldLocationToScreen(FVector(-3300,0,0),L) && PC->ProjectWorldLocationToScreen(FVector(3300,0,0),R);
            const auto* C=PC->BattleCamera?PC->BattleCamera->GetCameraComponent():nullptr;
            const double Effective=Projected && R.X>L.X?6600.*W/(R.X-L.X):0;
            auto Projection=MakeShared<FJsonObject>(); Projection->SetNumberField(TEXT("effective_width_cm"),Effective);
            Projection->SetNumberField(TEXT("requested_width_cm"),C?C->OrthoWidth:0); Projection->SetNumberField(TEXT("width_px"),W); Projection->SetNumberField(TEXT("height_px"),H);
            V->Report->SetObjectField(TEXT("camera_projection"),Projection);
            V->Check(TEXT("native_viewport_at_least_1440p"),W>=2560 && H>=1440,FString::Printf(TEXT("%d x %d"),W,H));
            auto Render=MakeShared<FJsonObject>();
            Render->SetStringField(TEXT("adapter"),GRHIAdapterName); Render->SetBoolField(TEXT("rhi_supports_ray_tracing"),GRHISupportsRayTracing);
            for (const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("sg.ResolutionQuality"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("r.AntiAliasingMethod"),TEXT("r.DynamicGlobalIlluminationMethod"),TEXT("r.ReflectionMethod"),TEXT("r.RayTracing"),TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Nanite"),TEXT("sg.ShadowQuality"),TEXT("sg.GlobalIlluminationQuality"),TEXT("sg.TextureQuality"),TEXT("sg.EffectsQuality"),TEXT("r.Streaming.PoolSize")})
                if (const IConsoleVariable* Var=IConsoleManager::Get().FindConsoleVariable(Name)) Render->SetStringField(Name,Var->GetString());
            Render->SetStringField(TEXT("scope"),TEXT("Live RHI capabilities and console variables, not proof of an individual GPU pass or image quality. Pixel and performance review are separate."));
            V->Report->SetObjectField(TEXT("runtime_render_settings"),Render);
            TArray<TSharedPtr<FJsonValue>> RoleMeshes;
            TSet<USkeletalMesh*> DistinctMeshes; bool RoleAssetsValid=B->RoleSoldierMeshes.Num()==6;
            bool RoleMaterialUsageValid=true; TArray<TSharedPtr<FJsonValue>> RoleMaterialSlots;
            for (const auto& Mesh:B->RoleSoldierMeshes)
            {
                auto Item=MakeShared<FJsonObject>(); Item->SetStringField(TEXT("mesh"),GetPathNameSafe(Mesh));
                const bool Valid=Mesh && B->LoadedSoldier && Mesh->GetSkeleton()==B->LoadedSoldier->GetSkeleton();
                Item->SetBoolField(TEXT("matches_centimeter_skeleton"),Valid); RoleAssetsValid&=Valid;
                if (Mesh) for (const FSkeletalMaterial& Slot:Mesh->GetMaterials())
                {
                    UMaterialInterface* Material=Slot.MaterialInterface;
                    const bool SkeletalUsage=Material && Material->GetUsageByFlag(MATUSAGE_SkeletalMesh);
                    const auto M=MakeShared<FJsonObject>(); M->SetStringField(TEXT("mesh"),Mesh->GetPathName());
                    M->SetStringField(TEXT("material"),GetPathNameSafe(Material));
                    M->SetBoolField(TEXT("serialized_skeletal_usage"),SkeletalUsage);
                    RoleMaterialSlots.Add(MakeShared<FJsonValueObject>(M)); RoleMaterialUsageValid&=SkeletalUsage;
                }
                if (Mesh) DistinctMeshes.Add(Mesh.Get()); RoleMeshes.Add(MakeShared<FJsonValueObject>(Item));
            }
            V->Report->SetArrayField(TEXT("role_meshes"),RoleMeshes);
            V->Check(TEXT("six_distinct_role_assets_on_same_centimeter_skeleton"),RoleAssetsValid && DistinctMeshes.Num()==6);
            V->Report->SetArrayField(TEXT("actual_role_material_slots"),RoleMaterialSlots);
            V->Check(TEXT("all_role_materials_support_skeletal_mesh_rendering"),RoleMaterialUsageValid && !RoleMaterialSlots.IsEmpty());
            V->Check(TEXT("camera_projection_matches_width_and_direction"),C && Effective>0 && FMath::Abs(Effective/C->OrthoWidth-1)<.05);
            V->Check(TEXT("initial_menu_and_briefing"),HUD->GetPresentationPage()==0 && B->Phase==EFrontlinePhase::Briefing && B->Elapsed==0);
            bool Ready=true,NoWallet=true;
            for (int32 T=0;T<2;++T)
            {
                for (int32 RIndex=0;RIndex<3;++RIndex) Ready&=B->RecruitCooldown(EFrontlineRole(RIndex),T)==0;
                NoWallet&=B->Supplies[T]==0 && B->CommandPoints[T]==0 && B->Income(T)==0 && B->CommandIncome(T)==0;
            }
            V->Check(TEXT("all_unlocked_infantry_initially_ready"),Ready);
            V->Check(TEXT("no_wallet_or_income"),NoWallet);
            V->Check(TEXT("release_symmetric_rifle6_assault4_lewis3_sections"),B->RoleSize(EFrontlineRole::Rifle,0)==6 && B->RoleSize(EFrontlineRole::Rifle,1)==6 && B->RoleSize(EFrontlineRole::Mills)==4 && B->RoleSize(EFrontlineRole::Lewis)==3);
            const float RifleRate=B->RoleSize(EFrontlineRole::Rifle)/B->RoleRecruitSeconds(EFrontlineRole::Rifle);
            const float GunRate=B->RoleSize(EFrontlineRole::Lewis)/B->RoleRecruitSeconds(EFrontlineRole::Lewis);
            V->Check(TEXT("release_rifle_manpower_and_machinegun_preparation_tradeoff"),FMath::IsFinite(RifleRate)&&GunRate>0&&RifleRate>GunRate&&RifleRate/GunRate<3.f
                &&B->RoleRecruitSeconds(EFrontlineRole::Mills)>0&&B->ArtilleryCooldown()>0);
            V->Check(TEXT("briefing_rejects_battle_orders"),!B->Recruit(EFrontlineRole::Rifle) && !B->AdvanceTrench(0) && !B->RequestArtillery(FVector::ZeroVector));
            V->Screenshot(TEXT("frontline-mainmenu.png")); V->Stage=1; V->StageStarted=Now;
        }
        else if (V->Stage==1 && Now-V->StageStarted>.7 && PC && HUD)
        {
            UI(TEXT("Help")); UI(TEXT("Begin")); UI(TEXT("Rifle"));
            V->Check(TEXT("manual_blocks_underlying_battle_orders"),HUD->GetPresentationPage()==5 && B->Phase==EFrontlinePhase::Briefing && B->RecruitCommands==0);
            UI(TEXT("Back")); UI(TEXT("Dossier1944")); UI(TEXT("Begin"));
            V->Check(TEXT("1944_dossier_does_not_start_1917"),HUD->GetPresentationPage()==2 && B->Phase==EFrontlinePhase::Briefing);
            UI(TEXT("MenuHome")); UI(TEXT("Dossier1917"));
            V->Check(TEXT("1917_dossier_navigation"),HUD->GetPresentationPage()==1);
            V->Screenshot(TEXT("frontline-dossier.png")); V->Stage=2; V->StageStarted=Now;
        }
        else if (V->Stage==2 && Now-V->StageStarted>.7 && PC && HUD)
        {
            UI(TEXT("Begin")); const int32 Count=B->Units.Num(); UI(TEXT("Begin"));
            V->Check(TEXT("begin_is_idempotent"),B->Phase==EFrontlinePhase::Fighting && B->Units.Num()==Count && HUD->GetPresentationPage()==3);
            UI(TEXT("Speed")); V->Check(TEXT("normal_one_point_five_times_speed"),B->BattleSpeed==1.5f && UGameplayStatics::GetGlobalTimeDilation(B)==1.5f);
            const int32 SupportCount=B->SmokeCommands+B->RallyCommands;
            V->Check(TEXT("smoke_initial_preparation_and_disabled_rally"),B->SmokeCooldown()>0 && !B->RequestSmoke(FVector::ZeroVector) && !B->RequestRally(FVector(-3300,0,0)) && B->SmokeCommands+B->RallyCommands==SupportCount && B->SmokeZones.IsEmpty());
            V->Check(TEXT("artillery_initial_preparation_blocks_firing"),!B->RequestArtillery(FVector::ZeroVector) && B->ArtilleryCommands==0);
            V->Screenshot(TEXT("frontline-deployment.png")); V->Stage=3; V->MatchStarted=Now;
        }
        else if (V->Stage==3 && PC && HUD)
        {
            if (V->bPaused)
            {
                if (Now-V->PauseStarted<.7) return true;
                bool Frozen=B->Elapsed==V->PausedElapsed && B->Units.Num()==V->PausedPositions.Num();
                for (const AWarfareFrontlineSoldier* U:B->Units)
                {
                    const auto* P=V->PausedPositions.Find(U->GetUniqueID()); const auto* A=V->PausedPhases.Find(U->GetUniqueID());
                    const auto* Node=U->Soldier?U->Soldier->GetSingleNodeInstance():nullptr;
                    Frozen&=P && A && Node && FVector::Dist(U->GetActorLocation(),*P)<.01 && FMath::Abs(Node->GetCurrentTime()-*A)<.001;
                }
                V->Check(TEXT("pause_freezes_units_motion_and_shared_clock"),Frozen);
                const int32 N=B->RecruitCommands; UI(TEXT("Rifle"));
                V->Check(TEXT("paused_recruit_rejected"),B->RecruitCommands==N && !B->RequestArtillery(FVector::ZeroVector));
                UI(TEXT("Pause")); UI(TEXT("Menu")); const bool Menu=HUD->GetPresentationPage()==4 && UGameplayStatics::IsGamePaused(B); UI(TEXT("Resume"));
                V->Check(TEXT("tactical_menu_resume"),Menu && HUD->GetPresentationPage()==3 && !UGameplayStatics::IsGamePaused(B));
                V->bPaused=false; V->bPauseDone=true;
            }
            for (int32 T=0;T<2;++T)
            {
                V->PeakSquads[T]=FMath::Max(V->PeakSquads[T],B->ActiveSquads(T)); V->bCapacityInvariant&=B->ActiveSquads(T)<=8;
                V->bWalletInvariant&=B->Supplies[T]==0 && B->CommandPoints[T]==0 && B->SuppliesEarned[T]==0 && B->SuppliesSpent[T]==0 && B->CommandEarned[T]==0 && B->CommandSpent[T]==0;
                for (int32 I=0;I<B->Trenches.Num();++I) { const int32 N=B->TrenchSquadCount(I,T); V->PeakTrenchOccupancy=FMath::Max(V->PeakTrenchOccupancy,N); V->bCapacityInvariant&=N<=3; }
            }
            int32 Engaging=0; V->ShotsWhileMoving=0; V->GripConstraintFrames=0;
            for (const AWarfareFrontlineSoldier* U:B->Units)
            {
                Engaging+=U->bEngaging && U->IsAlive()?1:0; V->ShotsWhileMoving+=U->ShotsWhileMoving;
                V->bMotionProfilesFinite&=FMath::IsFinite(U->GetMotionPhase()) && FMath::IsFinite(U->GetMovementScale()) && U->GetMovementScale()>=.92f && U->GetMovementScale()<=1.08f;
                if (auto* Animation=U->Soldier?Cast<UWarfareFrontlineAnimation>(U->Soldier->GetAnimInstance()):nullptr)
                {
                    V->GripConstraintFrames+=Animation->GetGripConstraintFrames();
                    V->MaximumGripResidual=FMath::Max(V->MaximumGripResidual,Animation->GetMaximumGripResidualCm());
                    V->MaximumGripCorrection=FMath::Max(V->MaximumGripCorrection,Animation->GetMaximumGripCorrectionCm());
                }
                if (U->DistanceTravelled>100)
                {
                    V->bAnimationAdvanced|=U->AnimationTimeAdvanced>.1f;
                    const auto* Node=U->Soldier?U->Soldier->GetSingleNodeInstance():nullptr;
                    V->bPlaybackRateVaried|=Node && U->bMoving && FMath::Abs(Node->GetPlayRate()-1.f)>.02f;
                }
            }
            V->PeakEngaging=FMath::Max(V->PeakEngaging,Engaging);
            for (const FFrontlineSquad& S:B->Squads)
            {
                if (S.Team==1) V->bEnemyRoles[int32(S.Role)]=true;
                float FirstMove=MAX_flt,LastMove=-1,FirstShot=MAX_flt,LastShot=-1,MinGait=MAX_flt,MaxGait=0;
                for (const auto& Weak:S.Members) if (Weak.IsValid())
                {
                    const auto* U=Weak.Get();
                    if (U->FirstMovementAt>=0) { FirstMove=FMath::Min(FirstMove,U->FirstMovementAt); LastMove=FMath::Max(LastMove,U->FirstMovementAt); }
                    if (U->FirstShotAt>=0) { FirstShot=FMath::Min(FirstShot,U->FirstShotAt); LastShot=FMath::Max(LastShot,U->FirstShotAt); }
                    MinGait=FMath::Min(MinGait,U->GetMovementScale()); MaxGait=FMath::Max(MaxGait,U->GetMovementScale());
                }
                if (LastMove>=0) V->MaxDepartureSpread=FMath::Max(V->MaxDepartureSpread,LastMove-FirstMove);
                if (LastShot>=0) V->MaxFirstShotSpread=FMath::Max(V->MaxFirstShotSpread,LastShot-FirstShot);
                if (MaxGait>0) V->MaxGaitSpread=FMath::Max(V->MaxGaitSpread,MaxGait-MinGait);
            }
            if (B->Phase==EFrontlinePhase::Fighting)
            {
                const EFrontlineRole Plan[]={EFrontlineRole::Rifle,EFrontlineRole::Lewis,EFrontlineRole::Mills,EFrontlineRole::Rifle};
                const EFrontlineRole RequestedRole=Plan[V->RecruitStep%UE_ARRAY_COUNT(Plan)]; const TCHAR* Boxes[]={TEXT("Rifle"),TEXT("Lewis"),TEXT("Mills")};
                if (B->RecruitBlockReason(RequestedRole).IsEmpty())
                {
                    const int32 Commands=B->RecruitCommands,Living=B->LivingSoldiers(0); UI(Boxes[int32(RequestedRole)]);
                    if (B->RecruitCommands>Commands)
                    {
                        bool Shared=true; for (int32 R=0;R<3;++R) Shared&=FMath::IsNearlyEqual(B->RecruitCooldown(EFrontlineRole(R)),B->RoleRecruitSeconds(EFrontlineRole(R)),.01f);
                        if (!V->bRoleRecruited[int32(RequestedRole)])
                        {
                            V->Check(*FString::Printf(TEXT("role_%d_recruits_actual_squad_and_resets_all_readiness"),int32(RequestedRole)),B->LivingSoldiers(0)-Living==B->RoleSize(RequestedRole) && Shared);
                            V->Check(*FString::Printf(TEXT("role_%d_rejects_immediate_other_role"),int32(RequestedRole)),!B->Recruit(EFrontlineRole((int32(RequestedRole)+1)%3)) && B->RecruitCommands==Commands+1);
                        }
                        V->bRoleRecruited[int32(RequestedRole)]=true; ++V->RecruitStep; V->Event(TEXT("recruited_after_shared_wait"),B->Elapsed,B->RoleName(RequestedRole));
                    }
                }
                if (!V->bPauseDone && B->Elapsed>=5)
                {
                    UI(TEXT("Pause")); V->bPaused=true; V->PauseStarted=Now; V->PausedElapsed=B->Elapsed;
                    for (const AWarfareFrontlineSoldier* U:B->Units) { V->PausedPositions.Add(U->GetUniqueID(),U->GetActorLocation()); const auto* Node=U->Soldier?U->Soldier->GetSingleNodeInstance():nullptr; V->PausedPhases.Add(U->GetUniqueID(),Node?Node->GetCurrentTime():-1.f); }
                    return true;
                }
                if (!V->bHeld && B->TrenchSquadCount(0,0)==3)
                {
                    if (V->HoldStarted<0)
                    {
                        V->HoldStarted=B->Elapsed; for (const FFrontlineSquad& S:B->Squads) if (S.Team==0 && S.Trench==0 && S.LivingCount()>0) V->HeldCentres.Add(S.Id,S.Centre());
                    }
                    else if (B->Elapsed-V->HoldStarted>=1)
                    {
                        bool Still=V->HeldCentres.Num()==3;
                        for (const auto& P:V->HeldCentres) { const auto* S=B->FindSquad(P.Key); Still&=S && S->Trench==0 && !S->bAdvancing && FVector::Dist(S->Centre(),P.Value)<30; }
                        V->bHeld=Still;
                        if (!Still) { V->HoldStarted=-1; V->HeldCentres.Empty(); }
                    }
                }
                if (B->Elapsed>=V->NextOrderAt)
                {
                    for (int32 T=B->Trenches.Num()-1;T>=0;--T) if (B->Trenches[T].Owner==0 && B->TrenchSquadCount(T,0)>0)
                    {
                        if (!V->bReleased) for (const auto& S:B->Squads) if (S.Team==0 && S.Trench==T && S.LivingCount()>0) V->ReleasedCentres.Add(S.Id,S.Centre());
                        const int32 Before=B->AdvanceCommands; UI(*FString::Printf(TEXT("Trench%d"),T)); UI(TEXT("Advance"));
                        if (B->AdvanceCommands>Before) { if (!V->bReleased) { V->bReleased=true; V->ReleasedAt=B->Elapsed; } V->Event(TEXT("trench_released"),B->Elapsed,FString::FromInt(T)); }
                    }
                    V->NextOrderAt=B->Elapsed+7;
                }
                if (V->bReleased && !V->bReleaseMoved && B->Elapsed>V->ReleasedAt+1)
                    for (const auto& P:V->ReleasedCentres) { const auto* S=B->FindSquad(P.Key); V->bReleaseMoved|=S && S->LivingCount()>0 && S->Centre().X>P.Value.X+25; }
                if (B->ArtilleryBlockReason().IsEmpty())
                {
                    const FFrontlineSquad* Target=nullptr;
                    for (const auto& S:B->Squads) if (S.Team==1 && S.LivingCount()>0 && (!Target || S.Centre().X<Target->Centre().X)) Target=&S;
                    if (Target) { const int32 N=B->ArtilleryCommands; UI(TEXT("Artillery")); PC->ConfirmSupportAtWorldPoint(Target->Centre()); if (B->ArtilleryCommands>N) { V->bNaturalArtillery=true; V->Event(TEXT("natural_artillery"),B->Elapsed); } }
                }
                if (!V->bBattleImage && B->ShotsFired>=35 && B->Elapsed>25) { V->Screenshot(TEXT("frontline-battle.png")); V->bBattleImage=true; }
            }
            if (B->Phase==EFrontlinePhase::Victory || B->Phase==EFrontlinePhase::Defeat || B->Elapsed>550 || Now-V->MatchStarted>310)
            {
                V->bTimedOut=B->Phase==EFrontlinePhase::Fighting;
                V->Report->SetObjectField(TEXT("single_match"),ObserveMatch(B)); V->Report->SetBoolField(TEXT("single_match_natural_terminal_outcome"),!V->bTimedOut);
                V->Report->SetNumberField(TEXT("single_match_wall_seconds"),Now-V->MatchStarted);
                V->Report->SetBoolField(TEXT("natural_artillery_order_observed"),V->bNaturalArtillery);
                V->Report->SetNumberField(TEXT("peak_friendly_squads"),V->PeakSquads[0]); V->Report->SetNumberField(TEXT("peak_enemy_squads"),V->PeakSquads[1]);
                V->Report->SetNumberField(TEXT("maximum_within_squad_departure_spread_seconds"),V->MaxDepartureSpread);
                V->Report->SetNumberField(TEXT("maximum_within_squad_first_shot_spread_seconds"),V->MaxFirstShotSpread);
                V->Report->SetNumberField(TEXT("maximum_within_squad_gait_scale_spread"),V->MaxGaitSpread);
                V->Report->SetNumberField(TEXT("grip_constraint_evaluated_frames"),double(V->GripConstraintFrames));
                V->Report->SetNumberField(TEXT("maximum_grip_residual_cm"),V->MaximumGripResidual);
                V->Report->SetNumberField(TEXT("maximum_grip_correction_cm"),V->MaximumGripCorrection);
                V->Check(TEXT("natural_match_reaches_real_victory_or_defeat"),!V->bTimedOut,B->EndReason);
                V->Check(TEXT("capacity_never_exceeds_three_per_trench_or_eight_per_team"),V->bCapacityInvariant);
                V->Check(TEXT("all_three_roles_recruited_through_hud"),V->bRoleRecruited[0] && V->bRoleRecruited[1] && V->bRoleRecruited[2]);
                V->Check(TEXT("enemy_waits_for_machinegun_and_assault"),V->bEnemyRoles[1] && V->bEnemyRoles[2]);
                V->Check(TEXT("three_squads_hold_in_friendly_trench"),V->bHeld);
                V->Check(TEXT("trench_release_moves_troops_forward"),V->bReleased && V->bReleaseMoved);
                V->Check(TEXT("combat_captures_pressure_and_morale_observed"),B->TrenchCaptures>0 && B->ShotsFired>0 && B->DamageDealt[0]>0 && B->DamageDealt[1]>0 && B->PeakSuppression>0 && (B->Morale[0]<100 || B->Morale[1]<100));
                V->Check(TEXT("approaching_troops_stop_to_engage"),V->PeakEngaging>0 && V->ShotsWhileMoving==0);
                V->Check(TEXT("members_depart_and_fire_at_different_times"),V->MaxDepartureSpread>.05f && V->MaxFirstShotSpread>.05f && V->MaxGaitSpread>.01f);
                int32 IndividualShots=0; for (const AWarfareFrontlineSoldier* U:B->Units) IndividualShots+=U->ShotsDischarged;
                V->Check(TEXT("motion_profiles_are_finite_and_actual_animation_advances"),V->bMotionProfilesFinite && V->bAnimationAdvanced && V->bPlaybackRateVaried);
                V->Check(TEXT("individual_discharge_totals_match_battle_shots"),IndividualShots==B->ShotsFired);
                V->Check(TEXT("transition_grip_is_actually_constrained"),V->GripConstraintFrames>0 && V->MaximumGripCorrection>1.f && V->MaximumGripResidual<.1f);
                V->Check(TEXT("wallets_remain_inert_during_natural_match"),V->bWalletInvariant);
                V->Check(TEXT("disabled_rally_never_used_by_either_team"),B->RallyCommands==0);
                V->Check(TEXT("pause_and_resume_observed"),V->bPauseDone);
                if (!V->bBattleImage) { V->Screenshot(TEXT("frontline-battle.png")); V->Stage=4; }
                else { V->Screenshot(TEXT("frontline-end.png")); V->Stage=5; }
                V->StageStarted=Now;
            }
        }
        else if (V->Stage==4 && Now-V->StageStarted>.7)
        { V->Screenshot(TEXT("frontline-end.png")); V->Stage=5; V->StageStarted=Now; }
        else if (V->Stage==5 && Now-V->StageStarted>10.7 && PC && HUD)
        {
            const auto Actions=V->NaturalActions.BuildReport();
            V->Report->SetObjectField(TEXT("v11_natural_weapon_and_death_observation"),Actions);
            V->Check(TEXT("v11_observed_weapon_and_death_invariants"),Actions->GetBoolField(TEXT("observed_invariants_passed")),
                TEXT("Coverage gaps remain separately labelled incomplete; visual review is required."));
            // These arrangements are deliberately outside the untouched natural match above.
            auto Fixture=MakeShared<FJsonObject>(); Fixture->SetBoolField(TEXT("not_part_of_natural_match"),true);
            Fixture->SetStringField(TEXT("setup"),TEXT("Fresh reset. Compare actual successful rifle damage while changing only the two squads' trench-membership state; no target health or position edits. Fresh reset; set infantry readiness to now synchronously to arrange eight squads and test the ninth. Fresh reset again; set only artillery readiness to now to test validation, exact cooldown reset, pause and delayed friendly fire. Reset artillery readiness once more for a second genuine delayed shell at a surviving friendly soldier, covering fatal explosions; observe at least 10.7 seconds after impact. No casualty health/position edits or manufactured terminal outcome."));
            V->Report->SetObjectField(TEXT("controlled_fixture"),Fixture);
            UI(TEXT("Restart")); UI(TEXT("Begin"));
            AWarfareFrontlineSoldier* Shooter=nullptr; AWarfareFrontlineSoldier* Target=nullptr;
            for (AWarfareFrontlineSoldier* U:B->Units) { if (!Shooter && U->Team==0) Shooter=U; if (!Target && U->Team==1) Target=U; }
            float OpenHit=0,CoveredHit=0,EntrenchedHit=0;
            if (Shooter && Target)
            {
                FFrontlineSquad* AttackingSquad=B->FindSquad(Shooter->SquadId); FFrontlineSquad* TargetSquad=B->FindSquad(Target->SquadId);
                auto ActualHit=[B,Shooter,Target]()
                {
                    for (int32 Attempt=0;Attempt<64 && Target->IsAlive();++Attempt)
                    {
                        const float Before=Target->Health; B->Fire(Shooter,Target);
                        if (Target->Health<Before) return Before-Target->Health;
                    }
                    return 0.f;
                };
                if (AttackingSquad && TargetSquad)
                {
                    AttackingSquad->Trench=INDEX_NONE; TargetSquad->Trench=INDEX_NONE; OpenHit=ActualHit();
                    TargetSquad->Trench=2; CoveredHit=ActualHit();
                    AttackingSquad->Trench=0; TargetSquad->Trench=INDEX_NONE; EntrenchedHit=ActualHit();
                }
            }
            Fixture->SetNumberField(TEXT("rifle_open_target_actual_hit"),OpenHit);
            Fixture->SetNumberField(TEXT("rifle_entrenched_target_actual_hit"),CoveredHit);
            Fixture->SetNumberField(TEXT("entrenched_rifle_open_target_actual_hit"),EntrenchedHit);
            V->Check(TEXT("actual_rifle_hits_demonstrate_strong_trench_survival_and_defensive_fire"),OpenHit>0 && CoveredHit>0 && CoveredHit<OpenHit*.35f && EntrenchedHit>OpenHit*1.15f,
                FString::Printf(TEXT("Open %.2f; entrenched target %.2f; entrenched shooter %.2f; only trench-membership state changed in this isolated fixture."),OpenHit,CoveredHit,EntrenchedHit));
            UI(TEXT("Restart")); UI(TEXT("Begin"));
            for (int32 I=0;I<8 && B->ActiveSquads(0)<8;++I) { for (int32 R=0;R<3;++R) B->RecruitReadyAt[0][R]=B->Elapsed; if (!B->Recruit(EFrontlineRole::Rifle)) break; }
            for (int32 R=0;R<3;++R) B->RecruitReadyAt[0][R]=B->Elapsed;
            const int32 Count=B->Units.Num();
            V->Check(TEXT("isolated_ninth_squad_rejected_without_spawning"),B->ActiveSquads(0)==8 && !B->Recruit(EFrontlineRole::Rifle) && B->Units.Num()==Count);
            UI(TEXT("Restart")); UI(TEXT("Begin")); B->ArtilleryReadyAt[0]=B->Elapsed;
            const float Ready=B->ArtilleryReadyAt[0];
            V->Check(TEXT("invalid_artillery_target_preserves_readiness"),!B->RequestArtillery(FVector(99999,0,0)) && B->ArtilleryReadyAt[0]==Ready && B->Strikes.IsEmpty());
            UI(TEXT("Artillery")); const bool Targeting=PC->SupportTargetMode==1; PC->ConfirmSupportAtWorldPoint(FVector(-3300,-260,0));
            bool SharedIntact=true; for (int32 R=0;R<3;++R) SharedIntact&=B->RecruitCooldown(EFrontlineRole(R))==0;
            const bool Ordered=B->ArtilleryCommands==1 && B->Strikes.Num()==1;
            V->Check(TEXT("artillery_confirm_starts_only_its_own_cooldown"),Targeting && Ordered && PC->SupportTargetMode==0 && SharedIntact && FMath::IsNearlyEqual(B->ArtilleryCooldown(),4000.f/35.f,.01f));
            V->Check(TEXT("artillery_repeat_rejected"),!B->RequestArtillery(FVector::ZeroVector) && B->ArtilleryCommands==1);
            V->FixtureImpactAt=Ordered?B->Strikes[0].ImpactAt:0;
            V->Check(TEXT("artillery_has_delayed_impact_without_instant_damage"),Ordered && V->FixtureImpactAt>B->Elapsed+3 && B->FriendlyFireDamage==0);
            UI(TEXT("Pause")); V->PausedElapsed=B->Elapsed; V->Stage=6; V->StageStarted=Now;
        }
        else if (V->Stage==6 && Now-V->StageStarted>.7 && PC && HUD)
        {
            V->Check(TEXT("pause_holds_pending_shell"),B->Elapsed==V->PausedElapsed && B->FriendlyFireDamage==0 && B->Strikes.Num()==1 && !B->Strikes[0].bResolved);
            UI(TEXT("Pause")); V->Stage=7; V->StageStarted=Now;
        }
        else if (V->Stage==7 && B->Elapsed>V->FixtureImpactAt+.4f && PC && HUD)
        {
            V->Check(TEXT("controlled_shell_resolves_after_delay_with_friendly_fire"),B->FriendlyFireDamage>0);
            V->Report->SetNumberField(TEXT("controlled_fixture_friendly_fire_damage"),B->FriendlyFireDamage);
            AWarfareFrontlineSoldier* Survivor=nullptr;
            for (AWarfareFrontlineSoldier* Unit:B->Units) if (Unit->Team==0 && Unit->IsAlive()
                && (!Survivor || Unit->Health<Survivor->Health)) Survivor=Unit;
            B->ArtilleryReadyAt[0]=B->Elapsed;
            UI(TEXT("Artillery"));
            if (Survivor) PC->ConfirmSupportAtWorldPoint(Survivor->Feet());
            const bool SecondOrdered=B->ArtilleryCommands==2;
            V->Check(TEXT("labelled_second_shell_orders_for_fatal_explosion_coverage"),SecondOrdered);
            V->FixtureImpactAt=SecondOrdered?B->Strikes.Last().ImpactAt:B->Elapsed;
            V->Stage=9; V->StageStarted=Now;
        }
        else if (V->Stage==9 && Now-V->StageStarted>14.7 && PC && HUD)
        {
            const auto Materials=InspectFrontlineMaterials(B->GetWorld());
            V->Report->SetObjectField(TEXT("battlefield_material_review"),Materials);
            V->Report->SetArrayField(TEXT("battlefield_materials"),Materials->GetArrayField(TEXT("materials")));
            V->Check(TEXT("all_battlefield_materials_render_without_default_fallback"),Materials->GetBoolField(TEXT("passed")));
            const auto FixtureActions=V->FixtureActions.BuildReport();
            V->Report->SetObjectField(TEXT("v11_labelled_artillery_fixture_observation"),FixtureActions);
            int32 BlastDeaths=0;
            for (const AWarfareFrontlineSoldier* Unit:B->Units)
                BlastDeaths+=Unit->GetDeathState().bStarted && Unit->GetDeathState().Kind!=EFrontlineDamageKind::Bullet?1:0;
            V->Check(TEXT("labelled_artillery_casualties_have_stable_observed_physics"),BlastDeaths>0
                && FixtureActions->GetBoolField(TEXT("observed_invariants_passed")) && V->FixtureActions.AreObservedDeathsSettled(),
                TEXT("Two actual delayed shells; only readiness accelerated in this separate fixture. Missing unrelated weapon coverage remains incomplete."));
            TArray<TWeakObjectPtr<AWarfareFrontlineSoldier>> Old; for (AWarfareFrontlineSoldier* U:B->Units) Old.Add(U);
            UI(TEXT("Pause")); UI(TEXT("Restart")); bool Destroyed=true; for (const auto& U:Old) Destroyed&=!U.IsValid() || U->IsActorBeingDestroyed();
            bool Reset=Destroyed && B->Phase==EFrontlinePhase::Briefing && B->Elapsed==0 && B->Morale[0]==100 && B->Morale[1]==100 && B->Strikes.IsEmpty() && B->ShotsFired==0 && B->RecruitCommands==0 && !UGameplayStatics::IsGamePaused(B) && HUD->GetPresentationPage()==1;
            for (int32 T=0;T<2;++T) { Reset&=B->Supplies[T]==0 && B->CommandPoints[T]==0 && FMath::IsNearlyEqual(B->ArtilleryCooldown(T),4000.f/35.f); for (int32 R=0;R<3;++R) Reset&=B->RecruitCooldown(EFrontlineRole(R),T)==0; }
            V->Check(TEXT("restart_clears_old_world_and_restores_ready_infantry"),Reset);
            UI(TEXT("Begin")); UI(TEXT("Lewis")); V->Stage=8; V->StageStarted=Now;
        }
        if ((V->Stage==8 && Now-V->StageStarted>1) || Wall>335)
        {
            V->Check(TEXT("restart_returns_to_playable_world"),V->Stage==8 && B->Phase==EFrontlinePhase::Fighting && B->Elapsed>.1f && B->RecruitCommands>0);
            V->Check(TEXT("five_fresh_game_screenshots"),V->FinishScreenshots());
            V->Report->SetArrayField(TEXT("checks"),V->Checks); V->Report->SetArrayField(TEXT("command_events"),V->Events);
            V->Report->SetNumberField(TEXT("total_wall_seconds"),Wall); V->Report->SetNumberField(TEXT("game_thread_frames"),double(GFrameCounter-V->StartFrame));
            V->Report->SetBoolField(TEXT("passed"),!V->bAnyFailure && V->Stage==8);
            FString Json; auto Writer=TJsonWriterFactory<>::Create(&Json); FJsonSerializer::Serialize(V->Report,Writer);
            const bool Saved=FFileHelper::SaveStringToFile(Json,*(V->Directory/TEXT("frontline-verification.json")));
            UE_LOG(LogTemp,Display,TEXT("WF_FRONTLINE_VERIFY %s %s"),Saved && !V->bAnyFailure?TEXT("PASS"):TEXT("FAIL"),*(V->Directory/TEXT("frontline-verification.json")));
            FPlatformMisc::RequestExit(false); return false;
        }
        return true;
    });
}
