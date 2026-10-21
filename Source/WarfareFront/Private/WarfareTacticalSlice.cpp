#include "WarfareTacticalSlice.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Canvas.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "MeshUVChannelInfo.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "UnrealClient.h"

namespace
{
constexpr float Cell = 140.f;
constexpr int32 NX = 41, NY = 57;
FVector GridPosition(int32 X, int32 Y) { return FVector((X - NX / 2) * Cell, (Y - NY / 2) * Cell, 0); }
float GroundZ(const FVector& P) { return AWarfareTacticalGround::HeightAt(P.X, P.Y); }
AWarfareTacticalGameMode* BattleFor(const UObject* Object)
{
    return Cast<AWarfareTacticalGameMode>(UGameplayStatics::GetGameMode(Object));
}

TSharedRef<FJsonObject> InspectTacticalGround(AWarfareTacticalGameMode* Battle)
{
    const TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();
    AWarfareTacticalGround* Ground=nullptr;
    int32 GroundCount=0;
    for (TActorIterator<AWarfareTacticalGround> It(Battle->GetWorld());It;++It)
    {
        Ground=*It; ++GroundCount;
    }
    UStaticMeshComponent* Surface=Ground?Ground->Surface.Get():nullptr;
    UStaticMesh* Mesh=Surface?Surface->GetStaticMesh():nullptr;
    const bool RenderData=Mesh && Mesh->HasValidRenderData();
    const int32 Triangles=RenderData?Mesh->GetNumTriangles(0):0;
    const bool VisibilityFlags=Ground && Surface && Surface->IsRegistered() && Surface->IsVisible()
        && !Surface->bHiddenInGame && !Ground->IsHidden();
    const bool QueryCollision=Surface && Surface->IsQueryCollisionEnabled()
        && Surface->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block;
    const FMeshUVChannelInfo* GroundUV=Mesh && !Mesh->GetStaticMaterials().IsEmpty()?&Mesh->GetStaticMaterials()[0].UVChannelData:nullptr;
    const bool StreamingMetadata=GroundUV && GroundUV->IsInitialized() && FMath::IsFinite(GroundUV->LocalUVDensities[0]) && GroundUV->LocalUVDensities[0]>0;
    Result->SetNumberField(TEXT("ground_actor_count"),GroundCount);
    Result->SetStringField(TEXT("surface_component"),GetPathNameSafe(Surface));
    Result->SetBoolField(TEXT("static_mesh_present"),Mesh!=nullptr);
    Result->SetBoolField(TEXT("mesh_allow_cpu_access"),Mesh && Mesh->bAllowCPUAccess);
    Result->SetBoolField(TEXT("valid_lod_render_data"),RenderData);
    Result->SetNumberField(TEXT("lod0_triangle_count"),Triangles);
    Result->SetBoolField(TEXT("visibility_flags_allow_rendering"),VisibilityFlags);
    Result->SetBoolField(TEXT("visibility_query_collision_enabled"),QueryCollision);
    Result->SetNumberField(TEXT("collision_enabled_enum"),Surface?int32(Surface->GetCollisionEnabled()):0);
    Result->SetStringField(TEXT("material"),GetPathNameSafe(Surface?Surface->GetMaterial(0):nullptr));
    Result->SetBoolField(TEXT("uv_streaming_metadata_initialized"),StreamingMetadata);
    Result->SetNumberField(TEXT("uv0_local_world_units_per_uv"),GroundUV?GroundUV->LocalUVDensities[0]:0);
    Result->SetBoolField(TEXT("pixel_visibility_verified"),false);
    Result->SetStringField(TEXT("scope"),TEXT("Component/render-data flags and actual world collision traces only. No pixel visibility or physical mouse click is established; inspect this run's screenshots."));

    FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalGroundVerification),true);
    for (const AWarfareTacticalUnit* Unit:Battle->Units) Params.AddIgnoredActor(Unit);
    const FVector2D Points[]={FVector2D(-400,-650),FVector2D(600,-900),FVector2D(-1400,100),FVector2D(850,500),FVector2D(1900,2200)};
    TArray<TSharedPtr<FJsonValue>> Traces;
    int32 PassedTraces=0;
    for (const FVector2D& Point:Points)
    {
        const double ExpectedZ=AWarfareTacticalGround::HeightAt(Point.X,Point.Y);
        FHitResult Hit;
        const bool BlockingHit=Battle->GetWorld()->LineTraceSingleByChannel(Hit,
            FVector(Point.X,Point.Y,ExpectedZ+2000),FVector(Point.X,Point.Y,ExpectedZ-500),ECC_Visibility,Params);
        const bool CorrectComponent=BlockingHit && Surface && Hit.GetComponent()==Surface;
        const double HeightError=BlockingHit?FMath::Abs(Hit.ImpactPoint.Z-ExpectedZ):-1;
        // Movement uses the analytic height; the 120 cm render grid interpolates it.
        const bool TracePassed=CorrectComponent && HeightError<=15;
        PassedTraces+=TracePassed?1:0;
        const TSharedRef<FJsonObject> Item=MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("x_cm"),Point.X); Item->SetNumberField(TEXT("y_cm"),Point.Y);
        Item->SetNumberField(TEXT("expected_height_cm"),ExpectedZ);
        Item->SetBoolField(TEXT("blocking_hit"),BlockingHit);
        Item->SetStringField(TEXT("hit_component"),GetPathNameSafe(Hit.GetComponent()));
        Item->SetBoolField(TEXT("hit_terrain_surface"),CorrectComponent);
        if (BlockingHit) Item->SetNumberField(TEXT("hit_height_cm"),Hit.ImpactPoint.Z);
        Item->SetNumberField(TEXT("height_error_cm"),HeightError);
        Item->SetBoolField(TEXT("passed"),TracePassed);
        Traces.Add(MakeShared<FJsonValueObject>(Item));
    }
    Result->SetArrayField(TEXT("traces"),Traces);
    Result->SetNumberField(TEXT("trace_count"),UE_ARRAY_COUNT(Points));
    Result->SetNumberField(TEXT("trace_pass_count"),PassedTraces);
    Result->SetNumberField(TEXT("height_tolerance_cm"),15);
    Result->SetBoolField(TEXT("passed"),GroundCount==1 && RenderData && Mesh->bAllowCPUAccess && Triangles>0 && VisibilityFlags && QueryCollision && StreamingMetadata && PassedTraces==UE_ARRAY_COUNT(Points));
    return Result;
}
}

AWarfareTacticalGround::AWarfareTacticalGround()
{
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Terrain"));
    SetRootComponent(Surface);
    Surface->SetCollisionProfileName(TEXT("BlockAll"));
    Sandbags=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Sandbags"));
    Sandbags->SetupAttachment(Surface);
    Sandbags->SetCollisionProfileName(TEXT("NoCollision"));
    Sandbags->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float AWarfareTacticalGround::HeightAt(float X, float Y)
{
    float H = 28 + 42 * FMath::Sin(X / 940.f) * FMath::Cos(Y / 1150.f) + Y * .008f;
    const FVector2D Craters[] = { FVector2D(-1050,-700), FVector2D(900,1200), FVector2D(1450,-2200), FVector2D(-1700,2500) };
    for (const FVector2D& C : Craters)
    {
        const float D = FVector2D::Distance(FVector2D(X,Y), C);
        H -= 62 * FMath::Exp(-FMath::Square(D / 235.f));
        H += 14 * FMath::Exp(-FMath::Square((D - 310) / 90.f));
    }
    H -= 24 * FMath::Exp(-FMath::Square((Y - 650) / 170.f));
    return H;
}

void AWarfareTacticalGround::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildTerrain();
}

void AWarfareTacticalGround::BeginPlay()
{
    Super::BeginPlay();
    // The generated mesh is transient; reconstruct when a saved level starts.
    RebuildTerrain();
}

void AWarfareTacticalGround::RebuildTerrain()
{
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    auto Positions = Attributes.GetVertexPositions();
    auto Normals = Attributes.GetVertexInstanceNormals();
    auto Tangents = Attributes.GetVertexInstanceTangents();
    auto Signs = Attributes.GetVertexInstanceBinormalSigns();
    auto UVs = Attributes.GetVertexInstanceUVs();
    auto Colors = Attributes.GetVertexInstanceColors();
    UVs.SetNumChannels(1);
    const FPolygonGroupID Group = Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Ground");
    constexpr int32 Width = 129, Depth = 181;
    constexpr float WorldUnitsPerUV = 300.f;
    double WeightedUVDensity=0, UVDensityWeight=0;
    TArray<FVertexID> Vertices;
    for (int32 Y=0; Y<Depth; ++Y)
        for (int32 X=0; X<Width; ++X)
        {
            const float PX=(X-Width/2)*120.f, PY=(Y-Depth/2)*120.f;
            const FVertexID V=Description.CreateVertex();
            Positions[V]=FVector3f(PX,PY,HeightAt(PX,PY));
            Vertices.Add(V);
        }
    auto AddTriangle = [&](int32 A, int32 B, int32 C)
    {
        const FVector3f Edge1=Positions[Vertices[B]]-Positions[Vertices[A]];
        const FVector3f Edge2=Positions[Vertices[C]]-Positions[Vertices[A]];
        const double SurfaceArea=FVector3f::CrossProduct(Edge1,Edge2).Size();
        const double UVArea=FMath::Abs(double(Edge1.X)*Edge2.Y-double(Edge1.Y)*Edge2.X)
            /FMath::Square(double(WorldUnitsPerUV));
        if (SurfaceArea>UE_SMALL_NUMBER && UVArea>UE_SMALL_NUMBER)
        {
            // Match UE's world-units-per-UV definition, including this triangle's slope.
            // All generated triangles contribute; there are no unrelated atlas islands.
            const double Weight=FMath::Sqrt(SurfaceArea);
            WeightedUVDensity+=FMath::Sqrt(SurfaceArea/UVArea)*Weight;
            UVDensityWeight+=Weight;
        }
        TArray<FVertexInstanceID> Corners;
        for (int32 Index : { A,B,C })
        {
            const FVertexInstanceID VI=Description.CreateVertexInstance(Vertices[Index]);
            const FVector3f P=Positions[Vertices[Index]];
            const float DX=(HeightAt(P.X+1,P.Y)-HeightAt(P.X-1,P.Y))*.5f;
            const float DY=(HeightAt(P.X,P.Y+1)-HeightAt(P.X,P.Y-1))*.5f;
            Normals[VI]=FVector3f(-DX,-DY,1).GetSafeNormal();
            Tangents[VI]=FVector3f(1,0,DX).GetSafeNormal(); Signs[VI]=1;
            UVs.Set(VI,0,FVector2f(P.X/WorldUnitsPerUV,P.Y/WorldUnitsPerUV));
            const float Shade=.62f+.16f*FMath::Sin(P.X*.0019f)*FMath::Cos(P.Y*.0023f)+.08f*FMath::Cos((P.X+P.Y)*.006f);
            Colors[VI]=FVector4f(Shade,Shade,Shade,1); Corners.Add(VI);
        }
        Description.CreateTriangle(Group,Corners);
    };
    for (int32 Y=0; Y<Depth-1; ++Y)
        for (int32 X=0; X<Width-1; ++X)
        {
            const int32 I=Y*Width+X;
            // Unreal's front-face winding is clockwise when viewed from above.
            AddTriangle(I,I+Width,I+1); AddTriangle(I+1,I+Width,I+Width+1);
        }
    UStaticMesh* Mesh=NewObject<UStaticMesh>(this,NAME_None,RF_Transient);
    // Params.bAllowCpuAccess retains the LOD index buffer, but packaged runtime
    // GetPhysicsTriMeshData also gates access on this separate UStaticMesh flag.
    Mesh->bAllowCPUAccess=true;
    FStaticMaterial GroundSlot(GroundMaterial,TEXT("Ground"));
    // UpdateUVChannelData is editor-only. Initialize before BuildFromMeshDescriptions
    // can expose this transient material slot to streaming in a packaged game.
    GroundSlot.UVChannelData=FMeshUVChannelInfo(0.f);
    GroundSlot.UVChannelData.bOverrideDensities=true;
    GroundSlot.UVChannelData.LocalUVDensities[0]=UVDensityWeight>UE_SMALL_NUMBER
        ? float(WeightedUVDensity/UVDensityWeight) : WorldUnitsPerUV;
    Mesh->GetStaticMaterials().Add(GroundSlot);
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild=true; Params.bBuildSimpleCollision=false;
    Params.bAllowCpuAccess=true; Params.bMarkPackageDirty=false;
    TArray<const FMeshDescription*> Descriptions; Descriptions.Add(&Description);
    if (Mesh->BuildFromMeshDescriptions(Descriptions,Params))
    {
        if (Mesh->GetBodySetup())
        {
            Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
            Mesh->GetBodySetup()->CreatePhysicsMeshes();
        }
        Surface->SetStaticMesh(Mesh);
        if (GroundMaterial) Surface->SetMaterial(0,GroundMaterial);
    }
}

AWarfareTacticalUnit::AWarfareTacticalUnit()
{
    PrimaryActorTick.bCanEverTick=true;
    Capsule=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Selection"));
    Capsule->InitCapsuleSize(27,88); SetRootComponent(Capsule);
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    Capsule->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Soldier=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Soldier"));
    Soldier->SetupAttachment(Capsule); Soldier->SetRelativeLocation(FVector(0,0,-88));
    Soldier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Soldier->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void AWarfareTacticalUnit::Initialize(AWarfareTacticalGameMode* InBattle,int32 InTeam,int32 InSquad)
{
    Battle=InBattle; Team=InTeam; Squad=InSquad;
    Soldier->SetSkeletalMeshAsset(Battle->LoadedSoldier);
    Soldier->SetRelativeRotation(FRotator(0,Battle->MeshYawOffset,0));
    Soldier->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SetMotion(Battle->IdleAnimation); ShotClock=.45f+.22f*Squad; DecisionClock=.6f;
}

void AWarfareTacticalUnit::SetMotion(UAnimationAsset* Animation,bool bLoop)
{
    if (Animation && Playing!=Animation) { Playing=Animation; LastAnimationTime=0; Soldier->PlayAnimation(Animation,bLoop); }
}

bool AWarfareTacticalUnit::MoveTo(const FVector& Destination)
{
    if (!IsAlive() || !Battle) return false;
    TArray<FVector> NewRoute;
    if (!Battle->BuildRoute(Feet(),Destination,NewRoute)) return false;
    Route=MoveTemp(NewRoute); RouteIndex=0; return true;
}

FString AWarfareTacticalUnit::Status() const
{
    if (!IsAlive()) return TEXT("Lost");
    if (Suppression>.62f) return TEXT("Pinned");
    if (IsMoving()) return TEXT("Moving");
    return bFireEnabled ? TEXT("Ready") : TEXT("Hold fire");
}

void AWarfareTacticalUnit::ReceiveShot(float Damage,float Pressure)
{
    if (!IsAlive()) return;
    Suppression=FMath::Clamp(Suppression+Pressure,0.f,1.f); Health=FMath::Max(0.f,Health-Damage);
    if (!IsAlive())
    {
        Route.Empty(); Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Soldier->bPauseAnims=true;
        if (Soldier->GetPhysicsAsset())
        {
            Soldier->SetCollisionProfileName(TEXT("Ragdoll"));
            Soldier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Soldier->SetSimulatePhysics(true);
            Soldier->SetAllBodiesSimulatePhysics(true);
            Soldier->SetAllBodiesPhysicsBlendWeight(1.f);
            Soldier->WakeAllRigidBodies();
            Soldier->AddImpulse(-GetActorForwardVector()*1200+FVector(0,0,250),NAME_None,false);
        }
        else UE_LOG(LogTemp,Warning,TEXT("Tactical casualty has no PhysicsAsset; leaving its actual skeleton still and incapacitated."));
    }
}

void AWarfareTacticalUnit::Tick(float DT)
{
    Super::Tick(DT);
    if (!Battle || !IsAlive() || Battle->bEncounterEnded) return;
    if (UAnimSingleNodeInstance* Instance=Soldier->GetSingleNodeInstance())
    {
        const float Time=Instance->GetCurrentTime();
        float Advanced=Time-LastAnimationTime;
        if (Advanced<0) Advanced+=Instance->GetLength();
        AnimationTimeAdvanced+=FMath::Max(0.f,Advanced); LastAnimationTime=Time;
    }
    Suppression=FMath::Max(0.f,Suppression-DT*(Battle->CoverAt(Feet())>.3f ? .105f : .065f));
    ShotClock-=DT; DecisionClock-=DT; FirePoseClock-=DT;
    if (Team==1 && DecisionClock<=0) { Battle->ThinkEnemy(this); DecisionClock=2.1f; }
    if (IsMoving())
    {
        FVector Delta=Route[RouteIndex]-Feet(); Delta.Z=0;
        const bool Running=Route.Num()-RouteIndex>10 && Suppression<.35f && Battle->RunAnimation;
        const float Speed=Suppression>.65f ? 78.f : (Running?285.f:185.f);
        if (Delta.Size2D()<=FMath::Max(15.f,Speed*DT)) ++RouteIndex;
        else
        {
            FVector Next=Feet()+Delta.GetSafeNormal()*Speed*DT;
            Next.Z=GroundZ(Next)+88; DistanceTravelled+=FVector::Dist2D(GetActorLocation(),Next); SetActorLocation(Next);
            SetActorRotation(FMath::RInterpTo(GetActorRotation(),Delta.Rotation(),DT,7));
        }
        SetMotion(Running?Battle->RunAnimation.Get():(Battle->WalkAnimation ? Battle->WalkAnimation.Get() : Battle->IdleAnimation.Get()));
    }
    else
    {
        AWarfareTacticalUnit* Target=bFireEnabled ? Battle->AcquireTarget(this) : nullptr;
        if (Target)
        {
            FVector Direction=Target->GetActorLocation()-GetActorLocation(); Direction.Z=0;
            SetActorRotation(FMath::RInterpTo(GetActorRotation(),Direction.Rotation(),DT,6));
            if (ShotClock<=0)
            {
                Battle->Fire(this,Target); ShotClock=2.3f+Suppression*1.8f+float(GetUniqueID()%7)*.1f;
                FirePoseClock=.4f;
                if (Battle->FireAnimation) { Playing=nullptr; SetMotion(Battle->FireAnimation,false); }
            }
        }
        if (FirePoseClock<=0) SetMotion(Target && Battle->AimAnimation ? Battle->AimAnimation.Get() : Battle->IdleAnimation.Get());
    }
}

AWarfareTacticalGameMode::AWarfareTacticalGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    PlayerControllerClass=AWarfareTacticalController::StaticClass(); HUDClass=AWarfareTacticalHUD::StaticClass(); DefaultPawnClass=nullptr;
    SoldierAsset=TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Warfare/Characters/V07/Meshes/SK_Soldier_V07.SK_Soldier_V07")));
    const FString Base=TEXT("/Game/Warfare/Characters/V07/Animations/");
    auto Anim=[&](const TCHAR* Name) { return TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(Base+Name+TEXT(".")+Name)); };
    IdleAsset=Anim(TEXT("A_LowReady_V07")); WalkAsset=Anim(TEXT("A_Walk_V07")); RunAsset=Anim(TEXT("A_Run_V07"));
    AimAsset=Anim(TEXT("A_Aim_V07")); FireAsset=Anim(TEXT("A_Fire_V07"));
}

void AWarfareTacticalGameMode::BeginPlay()
{
    Super::BeginPlay();
    LoadedSoldier=SoldierAsset.LoadSynchronous(); IdleAnimation=IdleAsset.LoadSynchronous();
    WalkAnimation=WalkAsset.LoadSynchronous(); RunAnimation=RunAsset.LoadSynchronous();
    AimAnimation=AimAsset.LoadSynchronous(); FireAnimation=FireAsset.LoadSynchronous();
    if (!LoadedSoldier || !IdleAnimation || LoadedSoldier->GetSkeleton()!=IdleAnimation->GetSkeleton())
    {
        AssetFailure=TEXT("Soldier assets unavailable or incompatible. Import V07 mesh and LowReady animation.");
        UE_LOG(LogTemp,Error,TEXT("Tactical slice: %s Required: %s | %s"),*AssetFailure,*SoldierAsset.ToSoftObjectPath().ToString(),*IdleAsset.ToSoftObjectPath().ToString());
    }
    auto Compatible=[&](TObjectPtr<UAnimationAsset>& Animation)
    { if (Animation && (!LoadedSoldier || Animation->GetSkeleton()!=LoadedSoldier->GetSkeleton())) Animation=nullptr; };
    Compatible(WalkAnimation); Compatible(RunAnimation); Compatible(AimAnimation); Compatible(FireAnimation);
    const FString AudioBase=TEXT("/Game/Warfare/Field/V07/Audio/");
    auto Sound=[&](const TCHAR* Name) { return LoadObject<USoundBase>(nullptr,*(AudioBase+Name+TEXT(".")+Name)); };
    RifleSound=Sound(TEXT("S_Rifle_01")); RifleSoundAlternate=Sound(TEXT("S_Rifle_02"));
    SelectSound=Sound(TEXT("S_Orders")); MoveSound=Sound(TEXT("S_Move"));
    if (USoundBase* Wind=Sound(TEXT("S_Wind")))
    {
        if (USoundWave* Wave=Cast<USoundWave>(Wind)) Wave->bLooping=true;
        WindAudio=UGameplayStatics::SpawnSound2D(this,Wind,.10f);
    }
    for (TActorIterator<AActor> It(GetWorld());It;++It)
        if (It->ActorHasTag(TEXT("TacticalCover")))
        {
            const FBox Box=It->GetComponentsBoundingBox(true); CoverBoxes.Add(Box);
            for (int32 Side : {-1,1})
            {
                FVector P=Box.GetCenter(); P.Y+=Side*(Box.GetExtent().Y+125); P.Z=GroundZ(P); CoverPositions.Add(P);
            }
        }
    BattleCamera=GetWorld()->SpawnActor<ACameraActor>();
    BattleCamera->GetCameraComponent()->FieldOfView=43;
    Objective.Z=GroundZ(Objective);
    ResetEncounter();
    if (FParse::Param(FCommandLine::Get(),TEXT("WFVerify"))) BeginVerification();
}

void AWarfareTacticalGameMode::ResetEncounter()
{
    UGameplayStatics::SetGamePaused(this,false);
    for (AWarfareTacticalUnit* Unit : Units) if (IsValid(Unit)) Unit->Destroy();
    Units.Empty(); Tracers.Empty(); HeldSeconds=0; ElapsedSeconds=0; bEncounterEnded=false; bWon=false; Random.Initialize(1917);
    ShotsFired=0; DamageApplied=0; PeakSuppression=0;
    LastOrder=TEXT("Advance one section while the other provides covering fire.");
    if (!AssetFailure.IsEmpty()) return;
    for (int32 Team=0;Team<2;++Team)
        for (int32 I=0;I<7;++I)
        {
            const int32 Squad=I<4 ? 0 : 1;
            FVector P((I%4-1.5f)*165+(Squad==1 ? 950 : -400),Team==0 ? -2250.f-Squad*280 : 2000.f+Squad*220,0);
            P.Z=GroundZ(P)+88;
            FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AWarfareTacticalUnit* Unit=GetWorld()->SpawnActor<AWarfareTacticalUnit>(P,FRotator(0,Team==0?90:-90,0),Spawn);
            if (Unit) { Unit->Initialize(this,Team,Squad); Units.Add(Unit); }
        }
}

void AWarfareTacticalGameMode::BeginVerification()
{
    struct FVerification
    {
        double Start=FPlatformTime::Seconds();
        uint64 StartFrame=GFrameCounter;
        FDateTime StartUtc=FDateTime::UtcNow();
        FDateTime EngagementRequestUtc;
        FDateTime RestartRequestUtc;
        bool CleanScreenshotPaths=false;
        int32 Stage=0;
        double StageStarted=0;
        float PauseSimulationTime=0;
        float ResumeSimulationTime=0;
        TArray<FVector> PausedPositions;
        TArray<float> PausedAnimationTimes;
        TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
        FString Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Gameplay/V07"));
    };
    const TSharedRef<FVerification> Check=MakeShared<FVerification>();
    IFileManager::Get().MakeDirectory(*Check->Directory,true);
    // These two dedicated self-check files belong to this regression path only.
    // Remove their old evidence before requesting images, then verify write timestamps.
    for (const TCHAR* Name : {TEXT("runtime-check.png"),TEXT("runtime-after-restart.png")})
        IFileManager::Get().Delete(*(Check->Directory/Name),false,false,true);
    Check->CleanScreenshotPaths=!IFileManager::Get().FileExists(*(Check->Directory/TEXT("runtime-check.png")))
        && !IFileManager::Get().FileExists(*(Check->Directory/TEXT("runtime-after-restart.png")));
    Check->Report->SetStringField(TEXT("run_id"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Check->Report->SetStringField(TEXT("verification_started_utc"),Check->StartUtc.ToIso8601());
    Check->Report->SetBoolField(TEXT("previous_screenshot_files_cleared"),Check->CleanScreenshotPaths);
    Check->Report->SetStringField(TEXT("kind"),TEXT("in-engine command-path regression; not physical mouse input"));
    Check->Report->SetBoolField(TEXT("physical_mouse_input_tested"),false);
    Check->Report->SetBoolField(TEXT("screenshot_includes_hud"),true);
    Check->Report->SetStringField(TEXT("mesh"),SoldierAsset.ToSoftObjectPath().ToString());
    Check->Report->SetStringField(TEXT("asset_failure"),AssetFailure);
    Check->Report->SetBoolField(TEXT("both_teams_use_shared_scan_based_study_character"),true);
    Check->Report->SetNumberField(TEXT("initial_unit_count"),Units.Num());
    Check->Report->SetNumberField(TEXT("reference_bone_count"),LoadedSoldier?LoadedSoldier->GetRefSkeleton().GetNum():0);
    Check->Report->SetBoolField(TEXT("walk_available"),WalkAnimation!=nullptr);
    Check->Report->SetBoolField(TEXT("run_available"),RunAnimation!=nullptr);
    Check->Report->SetBoolField(TEXT("aim_available"),AimAnimation!=nullptr);
    Check->Report->SetBoolField(TEXT("fire_available"),FireAnimation!=nullptr);
    TWeakObjectPtr<AWarfareTacticalGameMode> WeakThis(this);
    // The core ticker advances while the world is paused. Its weak owner prevents a level-travel dangling callback.
    FTSTicker::GetCoreTicker().AddTicker(TEXT("WarfareTacticalVerification"),0.f,
        [WeakThis,Check](float)
        {
            AWarfareTacticalGameMode* B=WeakThis.Get();
            if (!B) return false;
            AWarfareTacticalController* PC=Cast<AWarfareTacticalController>(UGameplayStatics::GetPlayerController(B,0));
            const double Time=FPlatformTime::Seconds()-Check->Start;
            if (Check->Stage==0 && Time>=.5 && PC)
            {
                // All level actors have completed BeginPlay before the first core-ticker observation.
                const TSharedRef<FJsonObject> GroundCheck=InspectTacticalGround(B);
                Check->Report->SetObjectField(TEXT("ground"),GroundCheck);
                Check->Report->SetBoolField(TEXT("ground_component_and_trace_checks_passed"),GroundCheck->GetBoolField(TEXT("passed")));
                PC->SelectSquad(1); const bool SelectedBravo=PC->SelectedSquad==1;
                PC->SelectSquad(0);
                Check->Report->SetBoolField(TEXT("selection_command_passed"),SelectedBravo && PC->SelectedSquad==0);
                PC->CommandMove(FVector(-400,-650,0));
                int32 Moving=0; for (const AWarfareTacticalUnit* U:B->Units) if (U->Team==0 && U->IsMoving()) ++Moving;
                Check->Report->SetNumberField(TEXT("units_accepting_move"),Moving);
                ++Check->Stage; Check->StageStarted=Time;
            }
            else if (Check->Stage==1 && Time>=4 && Time-Check->StageStarted>=3 && PC)
            {
                PC->TogglePause(); Check->PauseSimulationTime=B->ElapsedSeconds;
                Check->Report->SetBoolField(TEXT("pause_command_passed"),UGameplayStatics::IsGamePaused(B));
                for (const AWarfareTacticalUnit* U:B->Units)
                {
                    Check->PausedPositions.Add(U->GetActorLocation());
                    const UAnimSingleNodeInstance* Animation=U->Soldier->GetSingleNodeInstance();
                    Check->PausedAnimationTimes.Add(Animation?Animation->GetCurrentTime():0.f);
                }
                ++Check->Stage; Check->StageStarted=Time;
            }
            else if (Check->Stage==2 && Time-Check->StageStarted>=.8 && PC)
            {
                double MaxMove=0,MaxAnimationAdvance=0;
                const bool SameUnits=B->Units.Num()==Check->PausedPositions.Num();
                if (SameUnits)
                    for (int32 Index=0;Index<B->Units.Num();++Index)
                    {
                        const AWarfareTacticalUnit* U=B->Units[Index];
                        MaxMove=FMath::Max(MaxMove,FVector::Dist(U->GetActorLocation(),Check->PausedPositions[Index]));
                        const UAnimSingleNodeInstance* Animation=U->Soldier->GetSingleNodeInstance();
                        MaxAnimationAdvance=FMath::Max(MaxAnimationAdvance,double(FMath::Abs((Animation?Animation->GetCurrentTime():0.f)-Check->PausedAnimationTimes[Index])));
                    }
                Check->Report->SetNumberField(TEXT("simulation_advanced_while_paused"),B->ElapsedSeconds-Check->PauseSimulationTime);
                Check->Report->SetNumberField(TEXT("pause_observed_wall_seconds"),Time-Check->StageStarted);
                Check->Report->SetNumberField(TEXT("max_unit_distance_while_paused_cm"),MaxMove);
                Check->Report->SetNumberField(TEXT("max_animation_change_while_paused_seconds"),MaxAnimationAdvance);
                Check->Report->SetBoolField(TEXT("pause_unit_state_frozen"),SameUnits && MaxMove<.01 && MaxAnimationAdvance<.001);
                PC->TogglePause(); Check->Report->SetBoolField(TEXT("resume_command_passed"),!UGameplayStatics::IsGamePaused(B));
                Check->ResumeSimulationTime=B->ElapsedSeconds;
                ++Check->Stage; Check->StageStarted=Time;
            }
            else if (Check->Stage==3 && Time>=13.5 && Time-Check->StageStarted>=8.5)
            {
                TArray<TSharedPtr<FJsonValue>> Observations;
                int32 RealMeshes=0,AdvancedAnimations=0; float FriendlyDistance=0;
                for (const AWarfareTacticalUnit* U:B->Units)
                {
                    const bool Real=U->Soldier && U->Soldier->GetSkeletalMeshAsset()==B->LoadedSoldier && B->LoadedSoldier;
                    RealMeshes+=Real?1:0; AdvancedAnimations+=U->AnimationTimeAdvanced>.1f?1:0;
                    if (U->Team==0) FriendlyDistance+=U->DistanceTravelled;
                    const TSharedRef<FJsonObject> Item=MakeShared<FJsonObject>();
                    Item->SetNumberField(TEXT("team"),U->Team); Item->SetNumberField(TEXT("squad"),U->Squad);
                    Item->SetBoolField(TEXT("expected_skeletal_mesh"),Real);
                    Item->SetNumberField(TEXT("animation_seconds_advanced"),U->AnimationTimeAdvanced);
                    const UAnimSingleNodeInstance* Animation=U->Soldier->GetSingleNodeInstance();
                    Item->SetNumberField(TEXT("animation_current_seconds"),Animation?Animation->GetCurrentTime():0);
                    Item->SetNumberField(TEXT("distance_cm"),U->DistanceTravelled);
                    Item->SetNumberField(TEXT("health"),U->Health); Item->SetNumberField(TEXT("suppression"),U->Suppression);
                    Item->SetStringField(TEXT("state"),U->Status());
                    Observations.Add(MakeShared<FJsonValueObject>(Item));
                }
                Check->Report->SetArrayField(TEXT("units_before_restart"),Observations);
                Check->Report->SetNumberField(TEXT("real_skeletal_mesh_count"),RealMeshes);
                Check->Report->SetNumberField(TEXT("advanced_animation_count"),AdvancedAnimations);
                Check->Report->SetNumberField(TEXT("friendly_distance_total_cm"),FriendlyDistance);
                Check->Report->SetNumberField(TEXT("shots_fired"),B->ShotsFired);
                Check->Report->SetNumberField(TEXT("damage_applied"),B->DamageApplied);
                Check->Report->SetNumberField(TEXT("peak_suppression"),B->PeakSuppression);
                Check->Report->SetNumberField(TEXT("simulation_seconds_before_restart"),B->ElapsedSeconds);
                Check->Report->SetNumberField(TEXT("simulation_seconds_after_resume"),B->ElapsedSeconds-Check->ResumeSimulationTime);
                Check->Report->SetStringField(TEXT("encounter_state_before_restart"),B->bEncounterEnded?(B->bWon?TEXT("won"):TEXT("lost")):TEXT("active"));
                Check->EngagementRequestUtc=FDateTime::UtcNow();
                FScreenshotRequest::RequestScreenshot(Check->Directory/TEXT("runtime-check.png"),true,false);
                ++Check->Stage; Check->StageStarted=Time;
            }
            else if (Check->Stage==4 && Time-Check->StageStarted>=.7 && PC)
            {
                TArray<TWeakObjectPtr<AWarfareTacticalUnit>> OldUnits;
                for (AWarfareTacticalUnit* U:B->Units) OldUnits.Add(U);
                PC->Restart();
                int32 FreshMeshes=0; bool AllPristine=true,AllOldUnitsDestroyed=true;
                for (const TWeakObjectPtr<AWarfareTacticalUnit>& U:OldUnits)
                    if (U.IsValid() && !U->IsActorBeingDestroyed()) AllOldUnitsDestroyed=false;
                for (const AWarfareTacticalUnit* U:B->Units)
                {
                    FreshMeshes+=U->Soldier->GetSkeletalMeshAsset()==B->LoadedSoldier && B->LoadedSoldier?1:0;
                    AllPristine&=U->Health==100.f && U->Suppression==0.f && U->DistanceTravelled==0.f && U->AnimationTimeAdvanced==0.f;
                }
                Check->Report->SetNumberField(TEXT("unit_count_after_restart"),B->Units.Num());
                Check->Report->SetNumberField(TEXT("real_skeletal_mesh_count_after_restart"),FreshMeshes);
                Check->Report->SetBoolField(TEXT("old_units_destroyed_on_restart"),AllOldUnitsDestroyed);
                Check->Report->SetBoolField(TEXT("restart_command_passed"),FreshMeshes==14 && AllPristine && AllOldUnitsDestroyed && B->Units.Num()==14 && !B->bEncounterEnded && B->ElapsedSeconds<.1f && B->AliveCount(0)==7 && B->AliveCount(1)==7);
                PC->SelectSquad(1); PC->CommandMove(FVector(800,-1400,0));
                ++Check->Stage; Check->StageStarted=Time;
            }
            else if (Check->Stage==5 && Time-Check->StageStarted>=.8)
            {
                float Distance=0; int32 Advanced=0;
                for (const AWarfareTacticalUnit* U:B->Units)
                {
                    if (U->Team==0 && U->Squad==1) Distance+=U->DistanceTravelled;
                    Advanced+=U->AnimationTimeAdvanced>.1f?1:0;
                }
                Check->Report->SetNumberField(TEXT("friendly_distance_after_restart_cm"),Distance);
                Check->Report->SetNumberField(TEXT("advanced_animation_count_after_restart"),Advanced);
                Check->Report->SetBoolField(TEXT("restarted_world_advancing"),Distance>10.f && Advanced==14 && B->ElapsedSeconds>.1f);
                Check->RestartRequestUtc=FDateTime::UtcNow();
                FScreenshotRequest::RequestScreenshot(Check->Directory/TEXT("runtime-after-restart.png"),true,false);
                ++Check->Stage; Check->StageStarted=Time;
            }
            if ((Check->Stage==6 && Time-Check->StageStarted>=1) || Time>=60)
            {
                auto Screenshot=[&](const TCHAR* Name,const FDateTime& Requested)
                {
                    const FString Path=Check->Directory/Name;
                    const FDateTime Stamp=IFileManager::Get().GetTimeStamp(*Path);
                    const int64 Bytes=IFileManager::Get().FileSize(*Path);
                    const bool Fresh=Check->CleanScreenshotPaths && Requested.GetTicks()>0 && Bytes>0
                        && Stamp>=Requested-FTimespan::FromSeconds(2) && Stamp<=FDateTime::UtcNow()+FTimespan::FromSeconds(2);
                    const TSharedRef<FJsonObject> Item=MakeShared<FJsonObject>();
                    Item->SetStringField(TEXT("path"),Path); Item->SetStringField(TEXT("requested_utc"),Requested.ToIso8601());
                    Item->SetStringField(TEXT("last_write_utc"),Stamp.ToIso8601()); Item->SetNumberField(TEXT("bytes"),double(Bytes));
                    Item->SetBoolField(TEXT("fresh_for_this_run"),Fresh);
                    Check->Report->SetObjectField(Name,Item); return Fresh;
                };
                const bool EngagementFresh=Screenshot(TEXT("runtime-check.png"),Check->EngagementRequestUtc);
                const bool RestartFresh=Screenshot(TEXT("runtime-after-restart.png"),Check->RestartRequestUtc);
                Check->Report->SetBoolField(TEXT("both_screenshots_fresh"),EngagementFresh && RestartFresh);
                Check->Report->SetNumberField(TEXT("wall_seconds"),Time);
                const double ProcessElapsed=FPlatformTime::Seconds()-GStartTime;
                const uint64 FrameCount=GFrameCounter-Check->StartFrame;
                Check->Report->SetNumberField(TEXT("verification_game_thread_frame_count"),double(FrameCount));
                Check->Report->SetNumberField(TEXT("verification_elapsed_seconds"),Time);
                Check->Report->SetNumberField(TEXT("verification_mean_game_thread_fps"),FrameCount/FMath::Max(.001,Time));
                Check->Report->SetNumberField(TEXT("process_game_thread_frame_count"),double(GFrameCounter));
                Check->Report->SetNumberField(TEXT("process_elapsed_seconds"),ProcessElapsed);
                Check->Report->SetNumberField(TEXT("process_mean_game_thread_fps"),GFrameCounter/FMath::Max(.001,ProcessElapsed));
                Check->Report->SetStringField(TEXT("fps_scope"),TEXT("Engine game-thread frame counters, not presentation frames. Process mean includes engine/map loading and the full verification; verification mean includes warm-up, pause, screenshot I/O, and restart. Neither is steady-state performance."));
                const bool Complete=Check->Stage==6;
                Check->Report->SetBoolField(TEXT("completed_all_stages"),Complete);
                bool Passed=false;
                if (Complete)
                {
                    const FJsonObject& R=Check->Report.Get();
                    Passed=B->AssetFailure.IsEmpty() && R.GetNumberField(TEXT("real_skeletal_mesh_count"))==14
                        && R.GetBoolField(TEXT("ground_component_and_trace_checks_passed"))
                        && R.GetNumberField(TEXT("advanced_animation_count"))==14 && R.GetNumberField(TEXT("friendly_distance_total_cm"))>300
                        && R.GetNumberField(TEXT("units_accepting_move"))==4
                        && R.GetNumberField(TEXT("shots_fired"))>0 && R.GetNumberField(TEXT("damage_applied"))>0
                        && R.GetNumberField(TEXT("peak_suppression"))>0 && R.GetBoolField(TEXT("selection_command_passed"))
                        && R.GetBoolField(TEXT("pause_command_passed")) && R.GetBoolField(TEXT("resume_command_passed"))
                        && R.GetNumberField(TEXT("simulation_advanced_while_paused"))<.08 && R.GetBoolField(TEXT("pause_unit_state_frozen"))
                        && R.GetNumberField(TEXT("simulation_seconds_after_resume"))>1 && R.GetBoolField(TEXT("restart_command_passed"))
                        && R.GetBoolField(TEXT("restarted_world_advancing")) && EngagementFresh && RestartFresh;
                }
                Check->Report->SetBoolField(TEXT("passed"),Passed);
                Check->Report->SetStringField(TEXT("limitations"),TEXT("Automatic stability and command checks do not establish natural animation, real mouse input, combat balance, historical opposing forces, or production readiness."));
                FString Json; const TSharedRef<TJsonWriter<>> Writer=TJsonWriterFactory<>::Create(&Json);
                FJsonSerializer::Serialize(Check->Report,Writer);
                const bool Saved=FFileHelper::SaveStringToFile(Json,*(Check->Directory/TEXT("runtime-check.json")));
                UE_LOG(LogTemp,Display,TEXT("WF_VERIFY %s %s"),Saved?(Passed?TEXT("PASS"):TEXT("FAIL")):TEXT("FAIL_REPORT_WRITE"),*(Check->Directory/TEXT("runtime-check.json")));
                FPlatformMisc::RequestExit(false); return false;
            }
            return true;
        });
}

bool AWarfareTacticalGameMode::IsBlocked(const FVector& P) const
{
    for (const FBox& Box : CoverBoxes)
        if (P.X>Box.Min.X-45 && P.X<Box.Max.X+45 && P.Y>Box.Min.Y-45 && P.Y<Box.Max.Y+45) return true;
    return false;
}

bool AWarfareTacticalGameMode::BuildRoute(const FVector& From,const FVector& To,TArray<FVector>& Out) const
{
    auto IndexFor=[](const FVector& P) { return FMath::Clamp(FMath::RoundToInt(P.Y/Cell)+NY/2,0,NY-1)*NX+FMath::Clamp(FMath::RoundToInt(P.X/Cell)+NX/2,0,NX-1); };
    const int32 Start=IndexFor(From); int32 Goal=IndexFor(To);
    if (IsBlocked(GridPosition(Goal%NX,Goal/NX)))
    {
        float Best=MAX_flt; int32 Found=INDEX_NONE;
        for (int32 I=0;I<NX*NY;++I)
        {
            const FVector P=GridPosition(I%NX,I/NX); const float D=FVector::DistSquared2D(P,To);
            if (D<Best && !IsBlocked(P)) { Best=D; Found=I; }
        }
        if (Found==INDEX_NONE) return false; Goal=Found;
    }
    TArray<float> Cost; Cost.Init(MAX_flt,NX*NY); Cost[Start]=0;
    TArray<int32> Parent; Parent.Init(INDEX_NONE,NX*NY);
    TArray<uint8> Closed; Closed.Init(0,NX*NY);
    TArray<int32> Open; Open.Add(Start);
    while (!Open.IsEmpty())
    {
        int32 BestSlot=0; float BestScore=MAX_flt;
        for (int32 K=0;K<Open.Num();++K)
        {
            const int32 I=Open[K]; const float Score=Cost[I]+FVector::Dist2D(GridPosition(I%NX,I/NX),GridPosition(Goal%NX,Goal/NX));
            if (Score<BestScore) { BestScore=Score; BestSlot=K; }
        }
        const int32 Current=Open[BestSlot]; Open.RemoveAtSwap(BestSlot); if (Closed[Current]) continue;
        if (Current==Goal)
        {
            TArray<int32> Reverse;
            for (int32 I=Goal;I!=Start && I!=INDEX_NONE;I=Parent[I]) Reverse.Add(I);
            Out.Empty();
            for (int32 K=Reverse.Num()-1;K>=0;--K) { FVector P=GridPosition(Reverse[K]%NX,Reverse[K]/NX); P.Z=GroundZ(P); Out.Add(P); }
            return true;
        }
        Closed[Current]=1;
        for (int32 DY=-1;DY<=1;++DY) for (int32 DX=-1;DX<=1;++DX)
        {
            if (!DX && !DY) continue;
            const int32 X=Current%NX+DX,Y=Current/NX+DY;
            if (X<0 || X>=NX || Y<0 || Y>=NY) continue;
            const int32 Next=Y*NX+X;
            if (Closed[Next] || IsBlocked(GridPosition(X,Y))) continue;
            if (DX && DY && (IsBlocked(GridPosition(X-DX,Y)) || IsBlocked(GridPosition(X,Y-DY)))) continue;
            const float Candidate=Cost[Current]+Cell*(DX && DY?1.414214f:1.f);
            if (Candidate<Cost[Next]) { Cost[Next]=Candidate; Parent[Next]=Current; Open.Add(Next); }
        }
    }
    return false;
}

float AWarfareTacticalGameMode::CoverAt(const FVector& P) const
{
    float Best=0;
    for (const FBox& B : CoverBoxes)
    {
        const float DX=FMath::Max(FMath::Max(B.Min.X-P.X,P.X-B.Max.X),0.0);
        const float DY=FMath::Max(FMath::Max(B.Min.Y-P.Y,P.Y-B.Max.Y),0.0);
        const float Distance=FMath::Sqrt(DX*DX+DY*DY);
        Best=FMath::Max(Best,FMath::Clamp(1-Distance/230.f,0.f,.78f));
    }
    return Best;
}

bool AWarfareTacticalGameMode::HasSight(const AWarfareTacticalUnit* From,const AWarfareTacticalUnit* To) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalSight),true);
    for (const AWarfareTacticalUnit* Unit : Units) Params.AddIgnoredActor(Unit);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit,From->Feet()+FVector(0,0,145),To->Feet()+FVector(0,0,125),ECC_Visibility,Params);
}

AWarfareTacticalUnit* AWarfareTacticalGameMode::AcquireTarget(const AWarfareTacticalUnit* Unit) const
{
    AWarfareTacticalUnit* Best=nullptr; float Score=MAX_flt;
    for (AWarfareTacticalUnit* Candidate : Units)
        if (Candidate->Team!=Unit->Team && Candidate->IsAlive())
        {
            const float D=FVector::Dist2D(Unit->GetActorLocation(),Candidate->GetActorLocation());
            if (D<2450 && D<Score && HasSight(Unit,Candidate)) { Score=D; Best=Candidate; }
        }
    return Best;
}

void AWarfareTacticalGameMode::Fire(AWarfareTacticalUnit* Shooter,AWarfareTacticalUnit* Target)
{
    const float Distance=FVector::Dist2D(Shooter->Feet(),Target->Feet());
    const float Accuracy=FMath::Clamp(.66f-Distance/6800.f,.18f,.65f)*(1-.60f*CoverAt(Target->Feet()))*(1-.72f*Shooter->Suppression);
    const FVector Start=Shooter->Feet()+FVector(0,0,133), End=Target->Feet()+FVector(0,0,105);
    Tracers.Add({Start,End,.13f});
    const float BeforeHealth=Target->Health;
    Target->ReceiveShot(Random.FRand()<Accuracy?27.f:0.f,.20f*(1-.45f*CoverAt(Target->Feet())));
    ++ShotsFired; DamageApplied+=BeforeHealth-Target->Health; PeakSuppression=FMath::Max(PeakSuppression,Target->Suppression);
    USoundBase* ShotSound=ShotsFired%2 && RifleSoundAlternate ? RifleSoundAlternate.Get() : RifleSound.Get();
    if (ShotSound) UGameplayStatics::PlaySoundAtLocation(this,ShotSound,Start,.18f,.96f+Random.FRand()*.08f);
    for (AWarfareTacticalUnit* Unit : Units)
        if (Unit!=Target && Unit->Team==Target->Team && Unit->IsAlive() && FVector::Dist2D(Unit->Feet(),Target->Feet())<250)
            Unit->ReceiveShot(0,.035f);
}

void AWarfareTacticalGameMode::ThinkEnemy(AWarfareTacticalUnit* Unit)
{
    if (Unit->IsMoving()) return;
    const float CurrentCover=CoverAt(Unit->Feet());
    AWarfareTacticalUnit* Threat=AcquireTarget(Unit);
    if (Threat && CurrentCover>.4f && Unit->Suppression<.55f) return;
    FVector Best=Unit->Feet(); float BestScore=-MAX_flt;
    for (const FVector& P : CoverPositions)
    {
        const float MoveDistance=FVector::Dist2D(P,Unit->Feet());
        if (MoveDistance>1350 || MoveDistance<130 || IsBlocked(P)) continue;
        bool Occupied=false;
        for (const AWarfareTacticalUnit* Other : Units)
            if (Other!=Unit && Other->IsAlive() && FVector::Dist2D(Other->Feet(),P)<140) { Occupied=true; break; }
        if (Occupied) continue;
        const float ObjectiveDistance=FVector::Dist2D(P,Objective);
        const float Score=CoverAt(P)*1100-MoveDistance*.28f-ObjectiveDistance*.13f+(Unit->Suppression>.55f?P.Y*.15f:0);
        if (Score>BestScore) { BestScore=Score; Best=P; }
    }
    if (BestScore>-MAX_flt && (CurrentCover<.35f || Unit->Suppression>.55f || !Threat)) Unit->MoveTo(Best);
}

void AWarfareTacticalGameMode::OrderSquad(int32 Squad,const FVector& Target)
{
    int32 Count=0,Accepted=0;
    for (AWarfareTacticalUnit* Unit : Units)
        if (Unit->Team==0 && Unit->Squad==Squad && Unit->IsAlive())
        {
            FVector P=Target+FVector((Count%2-.5f)*150,(Count/2)*155,0);
            Accepted+=Unit->MoveTo(P)?1:0; ++Count;
        }
    LastOrder=Accepted>0 ? FString::Printf(TEXT("Section %s advancing."),Squad==0?TEXT("A"):TEXT("B")) : TEXT("No clear route to that position.");
}

void AWarfareTacticalGameMode::ToggleFire(int32 Squad)
{
    bool Enable=true;
    for (const AWarfareTacticalUnit* Unit : Units) if (Unit->Team==0 && Unit->Squad==Squad && Unit->IsAlive()) { Enable=!Unit->bFireEnabled; break; }
    for (AWarfareTacticalUnit* Unit : Units) if (Unit->Team==0 && Unit->Squad==Squad) Unit->bFireEnabled=Enable;
}

void AWarfareTacticalGameMode::PlayOrderSound(bool bMove)
{
    USoundBase* Sound=bMove?MoveSound.Get():SelectSound.Get();
    if (Sound) UGameplayStatics::PlaySound2D(this,Sound,.35f);
}

int32 AWarfareTacticalGameMode::AliveCount(int32 Team,int32 Squad) const
{
    int32 Count=0;
    for (const AWarfareTacticalUnit* Unit : Units) if (Unit->IsAlive() && Unit->Team==Team && (Squad==INDEX_NONE || Unit->Squad==Squad)) ++Count;
    return Count;
}

void AWarfareTacticalGameMode::Tick(float DT)
{
    Super::Tick(DT);
    for (int32 I=Tracers.Num()-1;I>=0;--I) { Tracers[I].Life-=DT; if (Tracers[I].Life<=0) Tracers.RemoveAtSwap(I); }
    if (bEncounterEnded || !AssetFailure.IsEmpty()) return;
    ElapsedSeconds+=DT;
    int32 Friendly=0,Enemy=0;
    for (const AWarfareTacticalUnit* Unit : Units) if (Unit->IsAlive() && FVector::Dist2D(Unit->Feet(),Objective)<570) { if (Unit->Team==0) ++Friendly; else ++Enemy; }
    HeldSeconds=Friendly>0 && Enemy==0 ? FMath::Min(20.f,HeldSeconds+DT) : FMath::Max(0.f,HeldSeconds-DT*.45f);
    if (AliveCount(1)==0 || HeldSeconds>=20) { bEncounterEnded=true; bWon=true; }
    else if (AliveCount(0)==0 || ElapsedSeconds>=240) { bEncounterEnded=true; bWon=false; }
}

AWarfareTacticalController::AWarfareTacticalController()
{
    bShowMouseCursor=true; bEnableClickEvents=true;
}

void AWarfareTacticalController::BeginPlay()
{
    Super::BeginPlay(); FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
}

void AWarfareTacticalController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&AWarfareTacticalController::SelectAtCursor);
    InputComponent->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&AWarfareTacticalController::MoveAtCursor);
    InputComponent->BindKey(EKeys::One,IE_Pressed,this,&AWarfareTacticalController::SelectAlpha);
    InputComponent->BindKey(EKeys::Two,IE_Pressed,this,&AWarfareTacticalController::SelectBravo);
    InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AWarfareTacticalController::ToggleFire);
    InputComponent->BindKey(EKeys::SpaceBar,IE_Pressed,this,&AWarfareTacticalController::TogglePause).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AWarfareTacticalController::Restart).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::MouseScrollUp,IE_Pressed,this,&AWarfareTacticalController::ZoomIn);
    InputComponent->BindKey(EKeys::MouseScrollDown,IE_Pressed,this,&AWarfareTacticalController::ZoomOut);
}

void AWarfareTacticalController::PlayerTick(float DT)
{
    Super::PlayerTick(DT); AWarfareTacticalGameMode* B=BattleFor(this); if (!B || !B->BattleCamera) return;
    SetViewTarget(B->BattleCamera);
    FVector Pan((IsInputKeyDown(EKeys::D)?1.f:0.f)-(IsInputKeyDown(EKeys::A)?1.f:0.f),
                (IsInputKeyDown(EKeys::W)?1.f:0.f)-(IsInputKeyDown(EKeys::S)?1.f:0.f),0);
    CameraFocus+=Pan*DT*1600; CameraFocus.X=FMath::Clamp(CameraFocus.X,-1900.,1900.); CameraFocus.Y=FMath::Clamp(CameraFocus.Y,-2700.,2800.);
    const FVector Location=CameraFocus+FVector(.30,-.82,.48).GetSafeNormal()*CameraDistance;
    B->BattleCamera->SetActorLocation(Location); B->BattleCamera->SetActorRotation((CameraFocus-Location).Rotation());
    OrderMarkerTime=FMath::Max(0.f,OrderMarkerTime-DT);
}

void AWarfareTacticalController::SelectAtCursor()
{
    AWarfareTacticalGameMode* B=BattleFor(this); if (!B) return;
    float X,Y; if (!GetMousePosition(X,Y)) return;
    float Best=42; AWarfareTacticalUnit* Selected=nullptr;
    for (AWarfareTacticalUnit* Unit : B->Units) if (Unit->Team==0 && Unit->IsAlive())
    {
        FVector2D Screen;
        if (ProjectWorldLocationToScreen(Unit->GetActorLocation(),Screen))
        { const float D=FVector2D::Distance(Screen,FVector2D(X,Y)); if (D<Best) { Best=D; Selected=Unit; } }
    }
    if (Selected) SelectSquad(Selected->Squad);
}

void AWarfareTacticalController::MoveAtCursor()
{
    AWarfareTacticalGameMode* B=BattleFor(this); if (!B || B->bEncounterEnded || UGameplayStatics::IsGamePaused(this)) return;
    FHitResult Hit;
    if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility),true,Hit))
    CommandMove(Hit.ImpactPoint);
}
void AWarfareTacticalController::SelectSquad(int32 Squad)
{
    SelectedSquad=FMath::Clamp(Squad,0,1); if (AWarfareTacticalGameMode* B=BattleFor(this)) B->PlayOrderSound(false);
}
void AWarfareTacticalController::CommandMove(const FVector& Destination)
{
    AWarfareTacticalGameMode* B=BattleFor(this); if (!B || B->bEncounterEnded || UGameplayStatics::IsGamePaused(this)) return;
    B->OrderSquad(SelectedSquad,Destination); OrderMarker=Destination; OrderMarkerTime=1.8f; B->PlayOrderSound(true);
}
void AWarfareTacticalController::SelectAlpha() { SelectSquad(0); }
void AWarfareTacticalController::SelectBravo() { SelectSquad(1); }
void AWarfareTacticalController::TogglePause() { UGameplayStatics::SetGamePaused(this,!UGameplayStatics::IsGamePaused(this)); }
void AWarfareTacticalController::Restart() { if (AWarfareTacticalGameMode* B=BattleFor(this)) B->ResetEncounter(); }
void AWarfareTacticalController::ToggleFire() { if (AWarfareTacticalGameMode* B=BattleFor(this)) B->ToggleFire(SelectedSquad); }
void AWarfareTacticalController::ZoomIn() { CameraDistance=FMath::Max(3500.f,CameraDistance-550); }
void AWarfareTacticalController::ZoomOut() { CameraDistance=FMath::Min(9400.f,CameraDistance+550); }

void AWarfareTacticalHUD::WorldRing(const FVector& P,float Radius,FLinearColor Color,float Width)
{
    for (int32 I=0;I<28;++I)
    {
        const float A=I*2*PI/28,B=(I+1)*2*PI/28; FVector2D S,E;
        FVector P1=P+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0),P2=P+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,0);
        P1.Z=GroundZ(P1)+7; P2.Z=GroundZ(P2)+7;
        if (PlayerOwner->ProjectWorldLocationToScreen(P1,S) && PlayerOwner->ProjectWorldLocationToScreen(P2,E)) DrawLine(S.X,S.Y,E.X,E.Y,Color,Width);
    }
}

void AWarfareTacticalHUD::DrawHUD()
{
    Super::DrawHUD(); if (!Canvas || !PlayerOwner) return;
    AWarfareTacticalGameMode* B=BattleFor(this); AWarfareTacticalController* PC=Cast<AWarfareTacticalController>(PlayerOwner); if (!B || !PC) return;
    const FLinearColor Paper(.82f,.83f,.78f),Muted(.52f,.57f,.56f),Accent(.49f,.70f,.69f),Enemy(.70f,.38f,.32f);
    DrawRect(FLinearColor(.025f,.032f,.033f,.9f),24,22,390,92);
    DrawText(TEXT("THE CROSSING  /  FIELD STUDY"),Paper,40,35,nullptr,1.08f);
    DrawText(TEXT("Hold the signal post for 20 seconds"),Muted,40,59,nullptr,.9f);
    DrawText(FString::Printf(TEXT("Control %02d/20s    Enemy %d    Time %02d:%02d"),FMath::FloorToInt(B->HeldSeconds),B->AliveCount(1),int32(B->ElapsedSeconds)/60,int32(B->ElapsedSeconds)%60),Paper,40,83,nullptr,.85f);
    WorldRing(B->Objective,570,FLinearColor(.70f,.70f,.54f,.55f),1);
    for (AWarfareTacticalUnit* Unit : B->Units) if (Unit->IsAlive())
    {
        const bool Selected=Unit->Team==0 && Unit->Squad==PC->SelectedSquad;
        if (Selected) WorldRing(Unit->Feet(),39,Accent,1.8f);
        FVector2D P;
        if (PlayerOwner->ProjectWorldLocationToScreen(Unit->GetActorLocation()+FVector(0,0,105),P))
        {
            DrawRect(Unit->Team==0 ? Accent : Enemy,P.X-3,P.Y-3,6,3);
            if (Unit->Suppression>.35f) DrawRect(FLinearColor(.80f,.65f,.35f),P.X-10,P.Y+3,20*Unit->Suppression,2);
        }
    }
    for (const FWarfareTacticalTracer& T : B->Tracers)
    {
        FVector2D A,C; if (PlayerOwner->ProjectWorldLocationToScreen(T.Start,A) && PlayerOwner->ProjectWorldLocationToScreen(T.End,C)) DrawLine(A.X,A.Y,C.X,C.Y,FLinearColor(.92f,.79f,.52f,T.Life/.13f),1.3f);
    }
    if (PC->OrderMarkerTime>0) WorldRing(PC->OrderMarker,65,Paper);
    const float Y=Canvas->ClipY-133;
    DrawRect(FLinearColor(.025f,.032f,.033f,.9f),24,Y,390,111);
    for (int32 Squad=0;Squad<2;++Squad)
    {
        float Pressure=0; bool Moving=false,Fire=true; int32 N=0;
        for (const AWarfareTacticalUnit* U : B->Units) if (U->Team==0 && U->Squad==Squad && U->IsAlive()) { Pressure+=U->Suppression; Moving|=U->IsMoving(); Fire=U->bFireEnabled; ++N; }
        const TCHAR* State=N==0?TEXT("Lost"):(Pressure/FMath::Max(1,N)>.55f?TEXT("Pinned"):(Moving?TEXT("Moving"):(Fire?TEXT("Ready"):TEXT("Hold fire"))));
        DrawText(FString::Printf(TEXT("%s  SECTION %s   %d/%d   %s"),PC->SelectedSquad==Squad?TEXT(">"):TEXT(" "),Squad==0?TEXT("A"):TEXT("B"),N,Squad==0?4:3,State),PC->SelectedSquad==Squad?Accent:Paper,40,Y+12+Squad*23,nullptr,.95f);
    }
    DrawText(TEXT("LMB select   RMB move   F fire / hold"),Muted,40,Y+62,nullptr,.79f);
    DrawText(TEXT("1/2 sections   Space pause   R restart   WASD / wheel view"),Muted,40,Y+82,nullptr,.72f);
    DrawText(B->LastOrder,Paper,435,Canvas->ClipY-45,nullptr,.83f);
    DrawText(TEXT("DEVELOPMENT BUILD  /  V07"),Muted,Canvas->ClipX-225,24,nullptr,.70f);
    if (!B->AssetFailure.IsEmpty() || B->bEncounterEnded || UGameplayStatics::IsGamePaused(this))
    {
        const FString Title=!B->AssetFailure.IsEmpty()?TEXT("SOLDIER ASSETS REQUIRED"):(B->bEncounterEnded?(B->bWon?TEXT("CROSSING SECURED"):TEXT("SECTION WITHDRAWN")):TEXT("PAUSED"));
        const float X=Canvas->ClipX*.5f-210;
        DrawRect(FLinearColor(.025f,.032f,.033f,.95f),X,Canvas->ClipY*.35f,420,104);
        DrawText(Title,Paper,X+24,Canvas->ClipY*.35f+20,nullptr,1.2f);
        DrawText(!B->AssetFailure.IsEmpty()?TEXT("Import the V07 character and reload this map."):(B->bEncounterEnded?TEXT("R  /  begin another attempt"):TEXT("Space  /  resume")),Muted,X+24,Canvas->ClipY*.35f+62,nullptr,.9f);
    }
}
