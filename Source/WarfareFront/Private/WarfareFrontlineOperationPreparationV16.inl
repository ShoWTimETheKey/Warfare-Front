#include "WarfareFrontlineLoadingPageV82.inl"
namespace NavigationV61 { void SilenceClickForLoadingV82(); }

// Actual resident-resource preparation for the already loaded L_Frontline.
// The existing battlefield keeps rendering beneath the loading artwork.
// A minimum observation window lets its temporal lighting/upscaler histories warm; no battle simulation is advanced.
void ClearPreparationWarmersV43(FFrontlineScreenState& S,bool ReleaseView=true)
{
    if(ReleaseView)FrontlineMapSceneV61::ReleaseLoadingView();
    for(const auto& W:S.PreparationWarmersV43)if(auto* C=W.Get())C->DestroyComponent();
    S.PreparationWarmersV43.Reset();S.PreparationTextures.Reset();
}
void BeginOperationPreparation(AWarfareFrontlineController* PC,AWarfareFrontlineGameMode* B)
{
    auto& S=ScreenState(PC);if (S.Preparing) return;
    NavigationV61::SilenceClickForLoadingV82();
    S.UnitIntroduction=S.Deployment=S.HistoricalDispatch=false;
    S.Preparing=true;S.PreparationPainted=false;S.PreparationError.Empty();
    ClearPreparationWarmersV43(S,false);
    if(!FrontlineMapSceneV61::LoadingView(PC))FrontlineMapSceneV61::CaptureLoadingView(PC);
    S.PreparationTextures.Reset();S.PreparationStarted=FPlatformTime::Seconds();
    FrontlineLoadingPageV82::ResetCaptionV844(B,S.PreparationStarted);
    S.PreparationLastDrawAt=-1.;S.PreparationVisualSeconds=0.f;
    S.PreparationFirstSceneAt=0;S.PreparationSceneFrames=0;S.PreparationDrawFrame=MAX_uint64;
    FrontlineLoadingPageV82::Prewarm(B,S);
    if(WarfareFRRUCampaignV82::Active(B))
    {FString Error;if(!WarfareFRRUWorldV82::PrepareScene(B,Error)){S.PreparationError=Error.IsEmpty()?TEXT("National scene preparation was rejected without a diagnostic."):Error;UE_LOG(LogTemp,Error,TEXT("WF_PREPARATION_SCENE_FAILURE %s"),*S.PreparationError);return;}}
    FrontlineSupportArtV36::PrewarmVisuals();
    PC->ResetCamera();
    const bool National=WarfareFRRUCampaignV82::Active(B);
    TArray<USkeletalMesh*> PreparedSkeletal;
    TArray<UStaticMesh*> PreparedStatic;
    if(National)
    {
        for(int32 Side=0;Side<2;++Side)for(int32 Role=0;Role<int32(EFrontlineRole::Count);++Role)
            if(WarfareFRRUEquipmentV82::RoleAvailable(B,EFrontlineRole(Role),Side))
            {
                FString Error;
                if(!WarfareFRRUPresentationV82::PrepareRoleAssets(B,EFrontlineRole(Role),Side,Error))
                {S.PreparationError=Error.IsEmpty()?TEXT("National role preparation was rejected without a diagnostic."):Error;UE_LOG(LogTemp,Error,TEXT("WF_PREPARATION_ROLE_FAILURE side=%d role=%d %s"),Side,Role,*S.PreparationError);return;}
                TArray<USkeletalMesh*> RoleSkeletal;TArray<UStaticMesh*> RoleStatic;
                if(!WarfareFRRUPresentationV82::PreparedMeshes(B,EFrontlineRole(Role),Side,RoleSkeletal,RoleStatic,Error))
                {S.PreparationError=Error;UE_LOG(LogTemp,Error,TEXT("WF_PREPARATION_CACHED_MESH_FAILURE side=%d role=%d %s"),Side,Role,*Error);return;}
                for(auto* Mesh:RoleSkeletal)PreparedSkeletal.AddUnique(Mesh);
                for(auto* Mesh:RoleStatic)PreparedStatic.AddUnique(Mesh);
            }
    }
    if(National)for(int32 Side=0;Side<2;++Side)
        if(WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::AirBombing,Side))
            if(auto* Aircraft=FrontlineSupportArtV36::AircraftFor(B,Side))PreparedStatic.AddUnique(Aircraft);
    if(National)WarfareFRRUArmourV82::PreparationMeshes(B,PreparedSkeletal);
    if(!National)for(const auto& Mesh:B->V16SoldierMeshes.Num()?B->V16SoldierMeshes:B->RoleSoldierMeshes)if(Mesh)PreparedSkeletal.AddUnique(Mesh.Get());
    const auto& Meshes=PreparedSkeletal;
    for (const auto& Mesh:Meshes) if (Mesh)
    {
        // An invisible, non-ticking component collects the real skeletal vertex
        // factory PSOs before first recruitment, without spawning gameplay units.
        auto* Warm=NewObject<USkeletalMeshComponent>(PC);PC->AddInstanceComponent(Warm);
        Warm->SetSkeletalMeshAsset(Mesh);Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if(National)for(int32 Slot=0;Slot<Mesh->GetMaterials().Num();++Slot)
            if(auto* SourceMaterial=Mesh->GetMaterials()[Slot].MaterialInterface.Get())
            {
                auto* Material=WarfareFRRUPresentationV82::ResolveMaterial(SourceMaterial);
                if(!Material)
                {
                    UE_LOG(LogTemp,Error,TEXT("WF_PREPARATION_NATIONAL_MATERIAL_FAILURE mesh=%s slot=%d material=%s"),*Mesh->GetPathName(),Slot,*SourceMaterial->GetPathName());
                    Warm->DestroyComponent();ClearPreparationWarmersV43(S,false);
                    S.PreparationError=TEXT("行动准备未能完成，请返回作战室重试。");return;
                }
                Warm->SetMaterial(Slot,Material);
            }
        for(int32 Slot=0;!National&&Slot<Mesh->GetMaterials().Num();++Slot)
            Warm->SetMaterial(Slot,FrontlineSoldierSurfaceV67::ResolveMaterial(FrontlineSoldierSurfaceV62::Resolve(Mesh->GetMaterials()[Slot].MaterialInterface.Get())));
        Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
        for (int32 Slot=0;Slot<Warm->GetNumMaterials();++Slot) if (auto* Material=Warm->GetMaterial(Slot))
        {
            TArray<UTexture*> Textures;Material->GetUsedTextures(Textures);
            for (UTexture* Texture:Textures) if (auto* T=Cast<UTexture2D>(Texture))
            {S.PreparationTextures.AddUnique(T);}
        }
    }
    for(auto* Mesh:PreparedStatic)if(Mesh)
    {
        auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
        Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
        Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
        for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
        {
            TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
            for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
        }
    }
    // Collect vehicle PSOs before removing the loading artwork. Texture requests
    // remain constrained by the selected tier and projected screen coverage.
    for(int32 Type=0;!National&&Type<int32(EFrontlineVehicle::Count);++Type)
    {
        const auto Kind=EFrontlineVehicle(Type);
        // The catalogue key already maps both pioneer escorts to the same
        // female body used by Configure; no distinct pioneer hull is loaded.
        const TCHAR* Key=UFrontlineVehicleSubsystem::Definition(Kind).AssetKey;
        auto* Mesh=FrontlineVehicleArtV836::ResolveMesh(FString(TEXT("SM_"))+Key);
        if(!Mesh)
        {
            if(Kind==EFrontlineVehicle::RailConvoy)
                Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Vehicles/V75/SM_RailConvoy_V75.SM_RailConvoy_V75"));
            else
            {
                const bool NewHull=Type==2||Type==3||Type==5||Type==8||Type==9||Type>=10;
                const TCHAR* Folder=NewHull?TEXT("V8218"):Type==6?TEXT("Production/V63"):(Type==4||Type==7)?TEXT("Production/Female"):TEXT("V33");
                const FString Name=FString(TEXT("SM_"))+Key+(NewHull?TEXT("_V8218"):TEXT(""));
                Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Vehicles/%s/%s.%s"),Folder,*Name,*Name));
            }
        }
        if(Mesh)
        {
            auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
            Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
            const auto& Slots=Mesh->GetStaticMaterials();
            for(int32 SlotIndex=0;SlotIndex<Slots.Num();++SlotIndex)
            {
                FString MaterialKey=Slots[SlotIndex].MaterialSlotName.ToString();
                if(Kind==EFrontlineVehicle::Whippet&&MaterialKey==TEXT("White"))MaterialKey=TEXT("British");
                if(Kind==EFrontlineVehicle::RailConvoy&&MaterialKey==TEXT("German"))MaterialKey=TEXT("RailPaint");
                if(Kind==EFrontlineVehicle::RailConvoy&&MaterialKey==TEXT("Steel"))MaterialKey=TEXT("RailSteel");
                UMaterialInterface* Surface=Mesh->GetName()==FString(TEXT("SM_"))+Key+TEXT("_V838")?FrontlineVehicleArtV836::ResolveModelMaterialV838(Key,MaterialKey):nullptr;
                if(!Surface&&!MaterialKey.IsEmpty())Surface=FrontlineVehicleArtV836::ResolveMaterial(MaterialKey);
                if(Surface)Warm->SetMaterial(SlotIndex,Surface);
            }
            Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
            for(int32 SlotIndex=0;SlotIndex<Warm->GetNumMaterials();++SlotIndex)if(auto* Surface=Warm->GetMaterial(SlotIndex))
            {
                FrontlineVehicleArtV836::HoldMaterialDetailV837(Surface);
                TArray<UTexture*> Textures;Surface->GetUsedTextures(Textures);
                for(UTexture* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))
                {S.PreparationTextures.AddUnique(T);}
            }
        }
        if(auto* Wreck=FrontlineVehicleArtV836::ResolveMesh(FString(TEXT("SM_"))+Key+TEXT("_Wreck")))
        {
            auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Wreck);
            Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
            const auto& Slots=Wreck->GetStaticMaterials();
            for(int32 SlotIndex=0;SlotIndex<Slots.Num();++SlotIndex)
            {
                FString MaterialKey=Slots[SlotIndex].MaterialSlotName.ToString();
                if(Kind==EFrontlineVehicle::Whippet&&MaterialKey==TEXT("White"))MaterialKey=TEXT("British");
                if(Kind==EFrontlineVehicle::RailConvoy&&MaterialKey==TEXT("German"))MaterialKey=TEXT("RailPaint");
                if(Kind==EFrontlineVehicle::RailConvoy&&MaterialKey==TEXT("Steel"))MaterialKey=TEXT("RailSteel");
                if(!MaterialKey.IsEmpty())if(auto* Surface=FrontlineVehicleArtV836::ResolveWreckMaterialV838(Key,MaterialKey))Warm->SetMaterial(SlotIndex,Surface);
            }
            Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
            for(int32 SlotIndex=0;SlotIndex<Warm->GetNumMaterials();++SlotIndex)if(auto* Surface=Warm->GetMaterial(SlotIndex))
            {FrontlineVehicleArtV836::HoldMaterialDetailV837(Surface);TArray<UTexture*> Textures;Surface->GetUsedTextures(Textures);
             for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);}
        }
    }
    // Austin's independently articulated wheels, turrets and water jackets use
    // distinct vertex factories/material slots; warm those before recruitment.
    for(const TCHAR* Part:{TEXT("SM_AustinWheel"),TEXT("SM_AustinTurret"),TEXT("SM_AustinVickers")})if(!National)
        if(auto* Mesh=FrontlineVehicleArtV836::ResolveMesh(Part)?FrontlineVehicleArtV836::ResolveMesh(Part):LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Vehicles/V8218/%s_V8218.%s_V8218"),Part,Part)))
        {
            auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
            Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
            Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
            for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
            {
                FrontlineVehicleArtV836::HoldMaterialDetailV837(Slot.MaterialInterface);
                TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
                for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
            }
        }
    for(const TCHAR* Part:{TEXT("SM_RailBogieWheel"),TEXT("SM_RailDoor"),TEXT("SM_RailGunCupola"),TEXT("SM_RailCannon")})if(!National)
        if(auto* Mesh=FrontlineVehicleArtV836::ResolveMesh(Part))
        {
            auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
            Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
            Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
            for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
            {
                TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
                for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
            }
        }
    if(!National)for(const TCHAR* Name:{TEXT("SM_Sandbag_A_V84412"),TEXT("SM_Sandbag_B_V84412"),
        TEXT("SM_Sandbag_C_V84412"),TEXT("SM_Sandbag_D_V84412"),TEXT("SM_Sandbag_E_V84412"),TEXT("SM_Sandbag_F_V84412"),
        TEXT("SM_Dugout_V84412"),TEXT("SM_DugoutCollapsed_V84412")})
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V84412/Props/Meshes/%s.%s"),Name,Name)))
        {
            auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
            Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
            Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
            for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
            {
                TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
                for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
            }
        }
    if(!National)for(const TCHAR* Name:{TEXT("SM_Dugout_V84413"),TEXT("SM_DugoutCollapsed_V84413")})
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V84413/Props/Meshes/%s.%s"),Name,Name)))
        {
            auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
            Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
            Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
            for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
            {
                TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
                for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
            }
        }
    for(bool German:{false,true})if(!National)if(auto* Mesh=FrontlineSupportArtV36::Aircraft(German))
    {
        auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Mesh);
        Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Warm->SetCanEverAffectNavigation(false);
        Warm->RegisterComponent();Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
        for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
        {
            TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
            for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
        }
    }
    // Warm the real bounded fire batches while the loading art still covers
    // the scene; no casualty or fuel burst needs to pay first-use costs.
    if(auto* Fire=AFrontlineFuelFireV33::Get(B->GetWorld()))
    {
        TInlineComponentArray<UMeshComponent*> FireMeshes(Fire);
        for(auto* Mesh:FireMeshes)if(Mesh)
        {
            Mesh->PrecachePSOs();
            for(int32 Slot=0;Slot<Mesh->GetNumMaterials();++Slot)if(auto* Material=Mesh->GetMaterial(Slot))
            {
                TArray<UTexture*> Textures;Material->GetUsedTextures(Textures);
                for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
            }
        }
    }
    // Warm the same volume and penetration vertex factories that a casualty
    // will use. These invisible components never advance combat or emit FX.
    if(auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")))
        for(const TCHAR* Path:{TEXT("/Game/Warfare/Field/V8215/Materials/M_BlastVolume_V8215.M_BlastVolume_V8215"),
            TEXT("/Game/Warfare/Field/V8215/Materials/M_GasVolume_V8215.M_GasVolume_V8215"),
            TEXT("/Game/Warfare/Field/V8215/Materials/M_PenetrationShard_V8215.M_PenetrationShard_V8215"),
            TEXT("/Game/Warfare/Field/V831/Materials/M_BlastCore_V831.M_BlastCore_V831"),
            TEXT("/Game/Warfare/Field/V831/Materials/M_IncidentAerosol_V831.M_IncidentAerosol_V831"),
            TEXT("/Game/Warfare/Field/V831/Materials/M_MineralDustVeil_V831.M_MineralDustVeil_V831"),
            TEXT("/Game/Warfare/Field/V831/Materials/M_BlastVolumeEnvelope_V831.M_BlastVolumeEnvelope_V831"),
            TEXT("/Game/Warfare/Field/V84412/Materials/M_MineralDustVolume_V84412.M_MineralDustVolume_V84412"),
            TEXT("/Game/Warfare/Field/V84412/Materials/M_IncidentAerosolVolume_V84412.M_IncidentAerosolVolume_V84412"),
            TEXT("/Game/Warfare/Field/V84412/Materials/M_GasVolume_V84412.M_GasVolume_V84412"),
            TEXT("/Game/Warfare/Field/V84413/Materials/M_ScorchResidue_V84413.M_ScorchResidue_V84413"),
            TEXT("/Game/Warfare/Field/V84413/Materials/M_MineralDustVolume_V84413.M_MineralDustVolume_V84413"),
            TEXT("/Game/Warfare/Field/V84413/Materials/M_IncidentAerosolVolume_V84413.M_IncidentAerosolVolume_V84413"),
            TEXT("/Game/Warfare/Field/V84413/Materials/M_GasVolume_V84413.M_GasVolume_V84413"),
            TEXT("/Game/Warfare/Field/V84413/Props/Materials/M_Hessian_V84413.M_Hessian_V84413")})
            if(!National||!FString(Path).Contains(TEXT("/V84413/")))if(auto* Material=LoadObject<UMaterialInterface>(nullptr,Path))
            {
                auto* Warm=NewObject<UStaticMeshComponent>(PC);PC->AddInstanceComponent(Warm);Warm->SetStaticMesh(Cube);
                Warm->SetMaterial(0,Material);Warm->SetVisibility(false);Warm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Warm->SetCanEverAffectNavigation(false);Warm->SetComponentTickEnabled(false);Warm->RegisterComponent();
                Warm->PrecachePSOs();S.PreparationWarmersV43.Add(Warm);
                TArray<UTexture*> Textures;Material->GetUsedTextures(Textures);
                for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))S.PreparationTextures.AddUnique(T);
            }
    UE_LOG(LogTemp,Display,TEXT("WF_V28_PREPARE requested_textures=%d"),S.PreparationTextures.Num());
}

int32 ResidentOperationTextures(const FFrontlineScreenState& S)
{
    int32 Count=0;for (const auto& Texture:S.PreparationTextures)
        if (Texture.IsValid())
        {const auto R=Texture->GetStreamableResourceState();if(R.NumResidentLODs>=R.NumRequestedLODs)++Count;}
    return Count;
}

void TickOperationPreparation(AWarfareFrontlineController* PC)
{
    auto& S=ScreenState(PC);if(!S.Preparing)return;
    auto* B=FrontlineFor(PC);if(!B)return;
    if(!FrontlineLoadingPageV82::SelectedArtworkReadyV87(B,S))return;
    if(!S.PreparationPainted||!S.PreparationError.IsEmpty())return;
    if (!B->AssetFailure.IsEmpty()) {UE_LOG(LogTemp,Error,TEXT("WF_PREPARATION_ASSET_FAILURE %s"),*B->AssetFailure);S.PreparationError=TEXT("行动准备未能完成，请返回作战室重试。");return;}
    // Build this operation's actual terrain and camera before warming RR.
    // World pause holds units, support clocks and AI while rendering continues.
    if(B->Phase==EFrontlinePhase::Briefing)
    {
        B->BeginBattle();
        if(B->Phase!=EFrontlinePhase::Fighting){S.PreparationError=TEXT("行动准备未能完成，请返回作战室重试。");return;}
        B->SetPaused(true);PC->ResetCamera();
        S.PreparationFirstSceneAt=0;S.PreparationSceneFrames=0;S.PreparationDrawFrame=MAX_uint64;
        return;
    }
    const double SceneSeconds=S.PreparationFirstSceneAt>0?FPlatformTime::Seconds()-S.PreparationFirstSceneAt:0.;
    const bool Resident=ResidentOperationTextures(S)==S.PreparationTextures.Num();
    // A streaming budget or unrelated background request is not a failed scene.
    // Mandatory assets have already been checked by BeginBattle. Rendered frames
    // gate release, so a background/minimized window cannot exhaust a deadline.
    const int32 PendingPSOs=PipelineStateCache::GetNumActivePipelinePrecompileTasks();
    int32 OwnedPSOs=0;
    for(const auto& Weak:S.PreparationWarmersV43)
        if(auto* Component=Weak.Get();Component&&Component->CheckPSOPrecachingAndBoostPriority(EPSOPrecachePriority::High))++OwnedPSOs;
    if (FrontlinePresentationV37::PreparationReady(SceneSeconds,S.PreparationSceneFrames,Resident) && (PendingPSOs==0||SceneSeconds>=30.))
    {
        ClearPreparationWarmersV43(S);
        S.Preparing=false;
        UE_LOG(LogTemp,Display,TEXT("WF_V28_PREPARE_READY textures=%d seconds=%.3f scene_seconds=%.3f scene_frames=%d simulation_elapsed=%.3f owned_pso_pending=%d global_pso_pending=%d bounded_timeout=%d"),S.PreparationTextures.Num(),FPlatformTime::Seconds()-S.PreparationStarted,FPlatformTime::Seconds()-S.PreparationFirstSceneAt,S.PreparationSceneFrames,B->Elapsed,OwnedPSOs,PendingPSOs,PendingPSOs!=0);
        WarfareFRRUWorldV82::ResumeAfterOwnedPause(B);return;
    }
    // Deliberately no wall-clock failure for optional mip residency.
}

void DrawOperationPreparation(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FFrontlineScreenState& S)
{
    if(B&&B->Phase==EFrontlinePhase::Briefing&&WarfareFRRUCampaignV82::Active(B)
        &&!FrontlineLoadingPageV82::SelectedArtworkReadyV87(B,S))
    {
        // Keep the complete operations page visible until the canonical pair can
        // actually draw. This scoped view change never exposes its input: the
        // confirmed preparation state is restored before input is dispatched.
        {TGuardValue<bool> PreparingView(S.Preparing,false);TGuardValue<int32> DossierView(S.Dossier,1917);FRRUWarRoomV82::Draw(UI,B);}
        UI.Hit(0,0,1920,1080,TEXT("Blocked"),100);
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,S.PreparationError.IsEmpty()
            ?WarRoom52::L(TEXT("Loading the battlefield · Esc: return to operations"),TEXT("正在载入战场 · Esc：返回作战室")):S.PreparationError,
            1298,945,550,40,17,WarRoom52::Ink);
        if(!S.PreparationError.IsEmpty())UI.PageButtonV73(1562,994,308,54,WarRoom52::L(TEXT("RETURN TO OPERATIONS"),TEXT("返回作战室")),TEXT("Esc"),TEXT("PreparationBack"),true,false,110);
        S.PreparationPainted=false;return;
    }
    FrontlineLoadingPageV82::Draw(UI,B,S);
    S.PreparationPainted=true;
    const auto* Viewport=B->GetWorld()->GetGameViewport();
    if(Viewport && !Viewport->bDisableWorldRendering && S.PreparationDrawFrame!=GFrameCounter)
    {
        S.PreparationDrawFrame=GFrameCounter;++S.PreparationSceneFrames;
        if(S.PreparationFirstSceneAt<=0)S.PreparationFirstSceneAt=FPlatformTime::Seconds();
    }
}

