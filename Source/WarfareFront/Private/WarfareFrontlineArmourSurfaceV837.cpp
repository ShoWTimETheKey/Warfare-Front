#include "WarfareFrontlineArmourSurfaceV837.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineParticulateV84412.h"
#include "WarfareFrontlineProgression.h"
#include "WarfareFrontlineWeather.h"
#include "WarfareFRRUCampaignV82.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "WarfareFrontlineUltimateArchiveV8215.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"

AFrontlineArmourSurfaceV837::AFrontlineArmourSurfaceV837()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PostPhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArmourSurfaceRootV837")));
    Spall=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PlateSplintersV837"));Spall->SetupAttachment(RootComponent);
    Spall->SetCollisionEnabled(ECollisionEnabled::NoCollision);Spall->SetCanEverAffectNavigation(false);
    Spall->SetCastShadow(true);Spall->SetReceivesDecals(false);Spall->NumCustomDataFloats=2;
}
AFrontlineArmourSurfaceV837* AFrontlineArmourSurfaceV837::Get(UWorld* W)
{
    if(!W)return nullptr;for(TActorIterator<AFrontlineArmourSurfaceV837> It(W);It;++It)if(IsValid(*It))return *It;
    return W->SpawnActor<AFrontlineArmourSurfaceV837>();
}
void AFrontlineArmourSurfaceV837::BeginPlay()
{
    Super::BeginPlay();
    UMaterialInterface* TrackBase=nullptr;
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV8311TracksBaseline")))
        TrackBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V8311/Materials/M_RunningGearImprint_V8311.M_RunningGearImprint_V8311"));
    if(!TrackBase)TrackBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V838/Materials/M_RunningGearImprint_V838.M_RunningGearImprint_V838"));
    auto* ScarBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V837/Materials/M_PlateStrike_V837.M_PlateStrike_V837"));
    for(int32 I=0;I<TrackBudget+ScarBudget;++I)
    {
        auto* C=NewObject<UDecalComponent>(this,*FString::Printf(TEXT("ArmourContactDecalV837_%d"),I));
        C->SetupAttachment(RootComponent);C->SetVisibility(false);C->SetFadeScreenSize(I<TrackBudget?.00025f:.002f);C->SortOrder=2;
        C->RegisterComponent();
        if(I<TrackBudget){auto* M=TrackBase?UMaterialInstanceDynamic::Create(TrackBase,this):nullptr;C->SetDecalMaterial(M);Tracks.Add(C);TrackMaterialsV838.Add(M);}
        else{auto* M=ScarBase?UMaterialInstanceDynamic::Create(ScarBase,this):nullptr;C->SetDecalMaterial(M);Scars.Add(C);ScarMaterials.Add(M);}
    }
    auto* SpallBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V839/Materials/M_PlateSplinter_V839.M_PlateSplinter_V839"));
    if(!SpallBase)SpallBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V837/Materials/M_PlateSplinter_V837.M_PlateSplinter_V837"));
    Spall->SetMaterial(0,SpallBase);
    // Bind the usage-ready override before a registered ISM creates its mesh
    // proxy; the protected mesh still retains its historical default material.
    Spall->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V837/Meshes/SM_PlateSplinter_V837.SM_PlateSplinter_V837")));
    for(int32 I=0;I<SpallBudget;++I)Spall->AddInstance(FTransform(FQuat::Identity,FVector(0,0,-10000),FVector::ZeroVector),true);
    auto* Cube=FrontlineParticulateV84412::Cube();auto* Soot=FrontlineParticulateV84412::Material(GetWorld());
    if(!Soot)Soot=FrontlineSmokeVolumeV35::MaterialV40();
    if(Cube&&Soot)for(int32 I=0;I<DustBudget;++I)
    {
        auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("BogieDustV837_%d"),I));
        C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);C->SetReceivesDecals(false);C->SetCanEverAffectNavigation(false);C->SetVisibleInRayTracing(false);
        // The last two pooled slots are locomotive exhaust, not wheel dust.
        // Retain their accepted smoke graph and producer update path.
        auto* Surface=I>=DustBudget-2?FrontlineSmokeVolumeV35::MaterialV40():Soot;
        auto* M=UMaterialInstanceDynamic::Create(Surface,this);M->SetScalarParameterValue(TEXT("VolumeProfile"),0);
        M->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.075f,.065f,.053f));M->SetScalarParameterValue(TEXT("Density"),0);
        C->SetMaterial(0,M);C->SetVisibility(false);C->RegisterComponent();Dust.Add(C);DustMaterials.Add(M);
    }
    if(!IsReady())UE_LOG(LogTemp,Error,TEXT("WF_V837_ARMOUR_SURFACE_ASSETS_MISSING"));
}
bool AFrontlineArmourSurfaceV837::IsReady()const
{
    return Tracks.Num()==TrackBudget&&Scars.Num()==ScarBudget&&Dust.Num()==DustBudget&&Spall&&Spall->GetStaticMesh()
        &&Spall->GetMaterial(0)&&Tracks[0]->GetDecalMaterial()&&Scars[0]->GetDecalMaterial();
}
void AFrontlineArmourSurfaceV837::Clear()
{
    Last.Reset();for(const auto& C:Tracks)C->SetVisibility(false);
    for(int32 I=0;I<Scars.Num();++I){Scars[I]->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);Scars[I]->SetVisibility(false);ScarState[I].Age=100;}
    for(int32 I=0;I<Dust.Num();++I){Clouds[I].Age=10;Dust[I]->SetVisibility(false);}
    for(int32 I=0;I<SpallBudget;++I){Fragments[I].Age=10;Fragments[I].Visible=false;Spall->UpdateInstanceTransform(I,FTransform(FQuat::Identity,FVector(0,0,-10000),FVector::ZeroVector),true,false,true);}
    Spall->MarkRenderStateDirty();NextTrack=NextScar=NextSpall=NextDust=0;
    TrackPairs=RejectedGround=DustEmissions=0;SoftGroundImprintsV8311=HardGroundImprintsV8311=SnowImprintsV8311=0;
}
void AFrontlineArmourSurfaceV837::EmitContact(const FVector& P,const FVector& Normal,float Strength,bool Explosive)
{
    if(!IsReady()||P.ContainsNaN()||Normal.ContainsNaN()||!FMath::IsFinite(Strength)||Strength<=0)return;
    const FVector N=Normal.GetSafeNormal(.001f,FVector::UpVector);const FVector A=FVector::CrossProduct(N,FVector::UpVector).GetSafeNormal(.001f,FVector::RightVector);
    const FVector B=FVector::CrossProduct(N,A).GetSafeNormal();const int32 Count=Explosive?12:7;++Contacts;
    for(int32 I=0;I<Count;++I)
    {
        auto& F=Fragments[NextSpall++%SpallBudget];F.Position=P+N*3.f;
        const float Angle=Random.FRandRange(0,2*PI),Spread=Random.FRandRange(.16f,.85f);
        F.Velocity=(N+A*FMath::Cos(Angle)*Spread+B*FMath::Sin(Angle)*Spread).GetSafeNormal()
            *Random.FRandRange(Explosive?240.f:330.f,Explosive?620.f:790.f);
        F.Age=0;F.Life=Random.FRandRange(.18f,.46f);F.Heat=Explosive?.48f:.78f;
        F.Scale=FVector(Random.FRandRange(.45f,1.35f),Random.FRandRange(.55f,1.20f),Random.FRandRange(.6f,1.f));
        F.Rotation=Random.VRand().Rotation().Quaternion();F.Visible=true;
    }
    // Scars follow the actual damaged plate. Their small projection depth cannot
    // stain the opposite side of a hull or leave a floating ground rectangle.
    if(auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld()))for(AFrontlineTank* T:Fleet->GetVehicles())
        if(IsValid(T)&&T->Body&&T->Body->GetStaticMesh()&&T->ContainsHullPointV75(P+N*-3.f))
        {
            const int32 Slot=NextScar++%ScarBudget;auto* C=Scars[Slot].Get();C->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            C->AttachToComponent(T->Body,FAttachmentTransformRules::KeepWorldTransform);T->Body->SetReceivesDecals(true);
            C->SetWorldLocation(P+N*3);C->SetWorldRotation(FRotationMatrix::MakeFromX(-N).Rotator());
            C->DecalSize=FVector(9,Explosive?15.f:8.f,Explosive?18.f:10.f);C->SetVisibility(true);
            ScarState[Slot].Age=0;ScarMaterials[Slot]->SetScalarParameterValue(TEXT("ImpactAge"),0);
            ScarMaterials[Slot]->SetScalarParameterValue(TEXT("Seed"),Random.FRandRange(0,12));++AcceptedScars;break;
        }
}
void AFrontlineArmourSurfaceV837::EmitDust(const FVector& At,const FVector& Velocity,float Strength,float RadiusScale,float HeightScale)
{
    if(Dust.Num()!=DustBudget)return;auto& D=Clouds[NextDust++%(DustBudget-2)];
    D.Position=At;D.Velocity=Velocity.GetClampedToMaxSize(100.f);D.Age=0;D.Seed=Random.FRandRange(0,20);
    D.Strength=FMath::Clamp(Strength,.015f,.15f);D.RadiusScale=FMath::Clamp(RadiusScale,.6f,1.5f);D.HeightScale=FMath::Clamp(HeightScale,.6f,1.5f);++DustEmissions;
}
void AFrontlineArmourSurfaceV837::StampTracks(AFrontlineTank* T,const FVector& Before,const FVector& After,float Travel)
{
    const FVector Forward=(After-Before).GetSafeNormal2D();if(Forward.IsNearlyZero())return;
    const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward);
    const bool Wheel=T->IsWheeledV8218();float Width=Wheel?17.f:25.f,Offset=Wheel?86.f:109.f;
    const bool Fast=T->Type==EFrontlineVehicle::Whippet;
    const bool Box=T->Type==EFrontlineVehicle::A7V||T->Type==EFrontlineVehicle::A7VGretchen;
    const bool Long=T->Type==EFrontlineVehicle::MarkVStar;
    const float Pressure=Wheel?.72f:Fast?.81f:Box?1.f:Long?1.f:.94f;
    if(!Wheel&&T->TrackLinks&&T->TrackLinks->GetStaticMesh()&&!T->TrackTransformsV33.IsEmpty())
    {
        // Read the actual live track centre and shoe width; different hulls
        // must not leave two generic ruts underneath the wrong running gear.
        Offset=FMath::Abs(float(T->TrackTransformsV33[0].GetLocation().Y));
        Width=float(T->TrackLinks->GetStaticMesh()->GetBounds().BoxExtent.Y)*float(T->TrackTransformsV33[0].GetScale3D().Y);
        Width=FMath::Clamp(Width,12.f,42.f);
    }
    else if(Wheel&&!T->RoadWheelsV8218.IsEmpty()&&T->RoadWheelsV8218[0])
    {
        const auto* W=T->RoadWheelsV8218[0].Get();
        Offset=FMath::Abs(float(W->GetRelativeLocation().Y));
        if(W->GetStaticMesh())Width=FMath::Clamp(float(W->GetStaticMesh()->GetBounds().BoxExtent.Y)
            *float(W->GetRelativeScale3D().Y),9.f,25.f);
    }
    // Adjacent patches overlap only their zero-opacity feathered margins.
    // Imprints stay behind the two actual running-gear contact paths.
    // Only the feathered four-centimetre margins overlap; opaque tread bodies
    // do not stack into a row of darker rectangles.
    const float Length=FMath::Clamp(Travel*.50f+4.f,36.f,88.f);
    const float Period=Wheel?2.f*PI*48.f:T->PathLength;
    const int32 Shoes=Wheel?22:T->TrackLinks?T->TrackLinks->GetInstanceCount()/2:0;
    const float Pitch=Period>1.f&&Shoes>0?Period/Shoes:Wheel?14.f:19.f;
    const auto* Record=Last.Find(TWeakObjectPtr<AFrontlineTank>(T));
    const float PhaseDirection=FVector::DotProduct(Forward,T->GetActorForwardVector())>=0.f?1.f:-1.f;
    bool Made=false;
    for(float Side:{-1.f,1.f})
    {
        const FVector P=(Before+After)*.5f+Right*(Side*Offset);
        bool Wet=false,Finite=true;float Level=0,Depth=0,Heights[3][3]={};
        int32 LI=0;
        for(float L:{-1.f,0.f,1.f})for(float R:{-1.f,0.f,1.f})
        {
            const FVector Q=P+Forward*(Length*L)+Right*(Width*R);
            if(AWarfareFrontlineGround::QueryWaterSurfaceV51(Q.X,Q.Y,Level,Depth)&&Depth>3.f)Wet=true;
            Heights[LI/3][LI%3]=AWarfareFrontlineGround::HeightAt(Q.X,Q.Y);
            Finite&=FMath::IsFinite(Heights[LI/3][LI%3]);++LI;
        }
        const float Z=Heights[1][1],Along=(Heights[2][1]-Heights[0][1])/(2.f*Length);
        const float Across=(Heights[1][2]-Heights[1][0])/(2.f*Width);
        float Residual=0;
        for(int32 L=0;L<3;++L)for(int32 R=0;R<3;++R)
            Residual=FMath::Max(Residual,FMath::Abs(Heights[L][R]-(Z+Along*Length*(L-1)+Across*Width*(R-1))));
        if(Wet||!Finite||FMath::Abs(Along*Length)>34.f||FMath::Abs(Across*Width)>19.f||Residual>12.f)
        {++RejectedGround;continue;}
        const auto Features=AWarfareFrontlineGround::QueryTerrainFeaturesV54(P.X,P.Y);
        const bool Snow=Features.Operation==10;
        const bool Stone=Features.Style==EFrontlineMissionTerrainV32::Ridge;
        const float Hardness=FMath::Clamp(FMath::Max(Features.RailShoulder*.88f,Features.Ruins*.90f),0.f,.94f);
        const float Softness=Snow?1.f:Stone?.55f:FMath::Lerp(1.f,.15f,Hardness);
        const bool Damp=Features.Style==EFrontlineMissionTerrainV32::Marsh||Features.Style==EFrontlineMissionTerrainV32::Canal
            ||SurfaceWetnessV8311>.68f;
        // Compacted wet soil darkens as pore air is displaced; dry tracks keep
        // their soil hue. Chalk and snow retain their own restrained values.
        // Coverage, pressure, tread phase and physical projection are unchanged.
        const FLinearColor Colour=Snow?FLinearColor(.16f,.18f,.188f):Stone?FLinearColor(.0735f,.0735f,.0693f)
            :Damp?FLinearColor(.02035f,.01925f,.0165f):FLinearColor(.031f,.02666f,.02108f);
        const FVector SurfaceNormal=(FVector::UpVector-Forward*Along-Right*Across).GetSafeNormal();
        const int32 Slot=NextTrack++%TrackBudget;auto* C=Tracks[Slot].Get();
        // Set the size before the transform update queues the decal proxy;
        // do not destroy/recreate a proxy for every reused stamp.
        C->DecalSize=FVector(22,Width,Length);C->SetWorldLocation(FVector(P.X,P.Y,Z+8));
        C->SetWorldRotation(FRotationMatrix::MakeFromXZ(-SurfaceNormal,Forward).Rotator());
        C->SetVisibility(true);Made=true;
        if(auto* M=TrackMaterialsV838[Slot].Get())
        {
            M->SetScalarParameterValue(TEXT("TreadKind"),Wheel?1.f:0.f);
            M->SetScalarParameterValue(TEXT("TreadCount"),2.f*Length/Pitch*PhaseDirection);
            const int32 GearSide=FVector::DotProduct(Right*Side,T->GetActorRightVector())<0.f?0:1;
            const float EndPhase=T->TrackSideOffsetV8311[GearSide];
            const float BeginPhase=Record?Record->SidePhase[GearSide]:EndPhase-Travel;
            // Unwrap the actual running-gear travel across its closed loop.
            // A source patch is centred halfway between the two contact poses.
            const float Delta=Period>1.f?FMath::FindDeltaAngleRadians(BeginPhase/Period*(2.f*PI),EndPhase/Period*(2.f*PI))*Period/(2.f*PI):Travel;
            M->SetScalarParameterValue(TEXT("TreadPhase"),(EndPhase-Delta*.5f-Length*PhaseDirection)/Pitch*(2.f*PI));
            M->SetScalarParameterValue(TEXT("GroundPressure"),Pressure);
            M->SetScalarParameterValue(TEXT("GroundSoftness"),Softness);
            M->SetScalarParameterValue(TEXT("CompactedRoughness"),Snow?.89f:Stone?.87f:
                FMath::Lerp(.76f,.52f,FMath::Clamp(SurfaceWetnessV8311,.0f,1.f)));
            M->SetScalarParameterValue(TEXT("ImprintSeed"),float(T->Serial)*.17f);
            M->SetVectorParameterValue(TEXT("CompactedColour"),Colour);
            SnowImprintsV8311+=Snow;HardGroundImprintsV8311+=!Snow&&Softness<.6f;
            SoftGroundImprintsV8311+=!Snow&&Softness>=.6f;
        }
        const FVector SideAfter=After+Right*(Side*Offset);
        if(!(AWarfareFrontlineGround::QueryWaterSurfaceV51(SideAfter.X,SideAfter.Y,Level,Depth)&&Depth>3.f))
            EmitDust(FVector(SideAfter.X,SideAfter.Y,AWarfareFrontlineGround::HeightAt(SideAfter.X,SideAfter.Y)+18),
                -Forward*(Wheel?82.f:Fast?72.f:48.f),Wheel?.020f:Fast?.029f:Box?.046f:Long?.051f:.042f,
                Wheel?.72f:Fast?.90f:Long?1.22f:1.06f,Wheel?.82f:Fast?.98f:1.16f);
    }
    TrackPairs+=Made;
}
void AFrontlineArmourSurfaceV837::TickVehicles(float DT)
{
    auto* B=Cast<AWarfareFrontlineGameMode>(UGameplayStatics::GetGameMode(GetWorld()));if(!B)return;
    const FGuid Current=B->GetEncounterId();if(Encounter!=Current){Clear();Encounter=Current;}
    if(B->Phase!=EFrontlinePhase::Fighting||WarfareFRRUCampaignV82::Active(B))return;
    if(auto* Weather=AWarfareFrontlineWeather::Get(GetWorld()))SurfaceWetnessV8311=Weather->GetWetness();
    if(auto* Fleet=UFrontlineVehicleSubsystem::Get(GetWorld()))for(AFrontlineTank* T:Fleet->GetVehicles())
        if(IsValid(T))
        {
            if(T->Body)for(int32 I=0;I<T->Body->GetNumMaterials();++I)FrontlineVehicleArtV836::HoldMaterialDetailV837(T->Body->GetMaterial(I));
            if(!T->IsOperational()||!T->HasEnteredField())continue;
            if(T->IsRailConvoyV32()&&T->RailEngineIntegrityV837()>0)
            {
                const int32 Slot=DustBudget-2+FMath::Clamp(T->Team,0,1);auto& D=Clouds[Slot];
                D.Position=T->GetActorTransform().TransformPosition(FVector(535,0,407));D.Velocity=FVector(26,-12,0);
                D.Age=.10f;D.Seed=float(T->Serial)*1.73f;D.Strength=.12f;
                DustMaterials[Slot]->SetVectorParameterValue(TEXT("DustColor"),FLinearColor(.027f,.029f,.028f));
            }
            auto& Record=Last.FindOrAdd(TWeakObjectPtr<AFrontlineTank>(T));const FVector P=T->GetActorLocation();
            if(!Record.Valid){Record.Position=P;Record.SidePhase[0]=T->TrackSideOffsetV8311[0];Record.SidePhase[1]=T->TrackSideOffsetV8311[1];Record.Valid=true;continue;}
            const float D=FVector::Dist2D(P,Record.Position);if(D>400.f){Record.Position=P;Record.Distance=0;Record.SidePhase[0]=T->TrackSideOffsetV8311[0];Record.SidePhase[1]=T->TrackSideOffsetV8311[1];continue;}
            if(D<64.f)continue;
            if(!T->IsRailConvoyV32())StampTracks(T,Record.Position,P,D);
            Record.Position=P;Record.SidePhase[0]=T->TrackSideOffsetV8311[0];Record.SidePhase[1]=T->TrackSideOffsetV8311[1];
        }
    for(auto It=Last.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
void AFrontlineArmourSurfaceV837::Tick(float DT)
{
    Super::Tick(DT);if(!IsReady()||!FMath::IsFinite(DT)||DT<=0)return;
    TrackClock+=DT;if(TrackClock>=.10f){TickVehicles(TrackClock);TrackClock=0;}
    bool Dirty=false;
    for(int32 I=0;I<SpallBudget;++I)
    {
        auto& F=Fragments[I];F.Age+=DT;if(F.Age>=F.Life)
        {if(F.Visible){F.Visible=false;Spall->UpdateInstanceTransform(I,FTransform(FQuat::Identity,FVector(0,0,-10000),FVector::ZeroVector),true,false,false);Dirty=true;}continue;}
        const int32 Steps=FMath::Max(1,FMath::CeilToInt(DT/.025f));const float Step=DT/Steps;
        for(int32 J=0;J<Steps;++J){F.Velocity.Z-=980.f*Step;F.Velocity*=FMath::Exp(-Step*.9f);F.Position+=F.Velocity*Step;}
        F.Rotation=FQuat(FVector(.3f,.7f,.2f).GetSafeNormal(),DT*8.f)*F.Rotation;
        Spall->UpdateInstanceTransform(I,FTransform(F.Rotation,F.Position,F.Scale),true,false,false);
        Spall->SetCustomDataValue(I,0,F.Age,false);Spall->SetCustomDataValue(I,1,F.Heat,false);Dirty=true;
    }
    if(Dirty)Spall->MarkRenderStateDirty();
    for(int32 I=0;I<Scars.Num();++I)
    {
        const float Before=ScarState[I].Age;ScarState[I].Age+=DT;const float Age=ScarState[I].Age;
        if(Age>=150){if(Before<150)Scars[I]->SetVisibility(false);continue;}
        if(Age>=120&&FMath::FloorToInt(Age*4)!=FMath::FloorToInt(Before*4)&&ScarMaterials[I])
            ScarMaterials[I]->SetScalarParameterValue(TEXT("ImpactAge"),Age);
    }
    FVector Camera;FRotator View;if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetPlayerViewPoint(Camera,View);
    for(int32 I=0;I<Dust.Num();++I)
    {
        auto& D=Clouds[I];D.Age+=DT;if(D.Age>=1.35f){Dust[I]->SetVisibility(false);continue;}
        const float A=D.Age,T=A/1.35f;const bool Steam=I>=DustBudget-2;
        const FVector Size=Steam?FVector(210,190,350):FVector((120+85*T)*D.RadiusScale,(110+75*T)*D.RadiusScale,(55+85*T)*D.HeightScale);
        const FVector Centre=D.Position+D.Velocity*(Steam?3.f+A:A)+FVector(0,0,Size.Z*.36f);
        auto* M=DustMaterials[I].Get();M->SetScalarParameterValue(TEXT("Seed"),D.Seed);
        const float Density=D.Strength*(1-FMath::SmoothStep(.30f,1.f,T))*FMath::Min(1.f,A/.05f);
        if(Steam)FrontlineSmokeVolumeV35::Update(Dust[I],M,Centre,Size,View.Vector(),Density,GetWorld()->TimeSeconds*.30f+D.Seed);
        else FrontlineParticulateV84412::Update(Dust[I],M,Centre,Size,View.Vector(),Density,A);
        Dust[I]->SetVisibility(true);
    }
}
TSharedRef<FJsonObject> AFrontlineArmourSurfaceV837::Describe()const
{
    auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("assets_ready"),IsReady());J->SetNumberField(TEXT("track_pairs"),TrackPairs);
    J->SetNumberField(TEXT("accepted_plate_scars"),AcceptedScars);J->SetNumberField(TEXT("contacts"),Contacts);
    J->SetNumberField(TEXT("rejected_water_or_steep_ground"),RejectedGround);J->SetNumberField(TEXT("dust_emissions"),DustEmissions);
    int32 Active=0;for(const auto& F:Fragments)Active+=F.Age<F.Life;J->SetNumberField(TEXT("active_spall"),Active);
    J->SetNumberField(TEXT("track_capacity"),TrackBudget);J->SetNumberField(TEXT("spall_capacity"),SpallBudget);
    J->SetNumberField(TEXT("soft_ground_imprints"),SoftGroundImprintsV8311);
    J->SetNumberField(TEXT("hard_ground_imprints"),HardGroundImprintsV8311);
    J->SetNumberField(TEXT("snow_imprints"),SnowImprintsV8311);
    TArray<TSharedPtr<FJsonValue>> TrackRows;int32 VisibleTracks=0;
    for(int32 I=0;I<Tracks.Num();++I)if(Tracks[I]&&Tracks[I]->IsVisible())
    {
        ++VisibleTracks;if(TrackRows.Num()>=8)continue;
        auto Q=MakeShared<FJsonObject>();const auto* C=Tracks[I].Get();auto* M=TrackMaterialsV838[I].Get();
        Q->SetStringField(TEXT("actual_material"),M&&M->GetBaseMaterial()?M->GetBaseMaterial()->GetPathName():FString());
        Q->SetNumberField(TEXT("height_above_ground_cm"),C->GetComponentLocation().Z
            -AWarfareFrontlineGround::HeightAt(C->GetComponentLocation().X,C->GetComponentLocation().Y));
        Q->SetNumberField(TEXT("projected_half_width_cm"),C->DecalSize.Y);Q->SetNumberField(TEXT("projected_half_length_cm"),C->DecalSize.Z);
        if(M){Q->SetNumberField(TEXT("ground_softness"),M->K2_GetScalarParameterValue(TEXT("GroundSoftness")));
            Q->SetNumberField(TEXT("compacted_roughness"),M->K2_GetScalarParameterValue(TEXT("CompactedRoughness")));
            Q->SetNumberField(TEXT("tread_kind"),M->K2_GetScalarParameterValue(TEXT("TreadKind")));
            Q->SetNumberField(TEXT("tread_phase_radians"),M->K2_GetScalarParameterValue(TEXT("TreadPhase")));
            Q->SetNumberField(TEXT("tread_count"),M->K2_GetScalarParameterValue(TEXT("TreadCount")));}
        TrackRows.Add(MakeShared<FJsonValueObject>(Q));
    }
    J->SetNumberField(TEXT("visible_imprints"),VisibleTracks);J->SetArrayField(TEXT("actual_imprints"),TrackRows);
    auto* SpallMat=Spall?Spall->GetMaterial(0):nullptr;
    const auto* SpallResource=SpallMat?SpallMat->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;
    J->SetStringField(TEXT("spall_actual_material"),GetPathNameSafe(SpallMat));
    J->SetStringField(TEXT("spall_actual_mesh"),Spall?GetPathNameSafe(Spall->GetStaticMesh()):FString());
    J->SetBoolField(TEXT("spall_instanced_usage"),SpallMat&&SpallMat->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
    J->SetBoolField(TEXT("spall_shader_map_valid"),SpallResource&&SpallResource->HasValidGameThreadShaderMap());
    J->SetBoolField(TEXT("spall_shader_map_complete"),SpallResource&&SpallResource->IsGameThreadShaderMapComplete());
    J->SetBoolField(TEXT("spall_uses_v839_derived_material"),SpallMat&&SpallMat->GetBaseMaterial()
        &&SpallMat->GetBaseMaterial()->GetName()==TEXT("M_PlateSplinter_V839"));
    TArray<TSharedPtr<FJsonValue>> Vehicles;
    if(auto* F=UFrontlineVehicleSubsystem::Get(GetWorld()))for(AFrontlineTank* T:F->GetVehicles())if(T&&T->Body&&T->Body->GetStaticMesh())
    {
        auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("body_mesh"),T->Body->GetStaticMesh()->GetPathName());
        TArray<TSharedPtr<FJsonValue>> Mats;
        for(int32 I=0;I<T->Body->GetNumMaterials();++I)if(auto* M=T->Body->GetMaterial(I))
        {
            auto Q=MakeShared<FJsonObject>();Q->SetStringField(TEXT("material"),M->GetPathName());
            Q->SetStringField(TEXT("base_material"),M->GetBaseMaterial()->GetPathName());
            TArray<UTexture*> Textures;M->GetUsedTextures(Textures);TArray<TSharedPtr<FJsonValue>> Images;
            for(UTexture* Texture:Textures)if(auto* Tex=Cast<UTexture2D>(Texture))
            {
                auto X=MakeShared<FJsonObject>();X->SetStringField(TEXT("texture"),Tex->GetPathName());
                X->SetNumberField(TEXT("resident_mips"),Tex->GetNumResidentMips());X->SetNumberField(TEXT("source_size"),Tex->GetSizeX());X->SetNumberField(TEXT("total_mips"),Tex->GetNumMips());X->SetNumberField(TEXT("requested_mips"),Tex->GetStreamableResourceState().NumRequestedLODs);
                Images.Add(MakeShared<FJsonValueObject>(X));
            }
            Q->SetArrayField(TEXT("textures"),Images);Mats.Add(MakeShared<FJsonValueObject>(Q));
        }
        V->SetArrayField(TEXT("materials"),Mats);Vehicles.Add(MakeShared<FJsonValueObject>(V));
    }
    J->SetArrayField(TEXT("vehicles"),Vehicles);return J;
}
void AFrontlineArmourSurfaceV837::RunTracksChecksV8311(TFunctionRef<void(const FString&,bool)> Check)const
{
    Check(TEXT("v8311_imprint_persistent_pool_ready"),IsReady()&&Tracks.Num()==768&&TrackMaterialsV838.Num()==Tracks.Num());
    bool Bound=true,Contact=true;
    for(int32 I=0;I<Tracks.Num();++I)
    {
        auto* C=Tracks[I].Get();auto* M=TrackMaterialsV838[I].Get();
        Bound&=C&&M&&C->GetDecalMaterial()==M&&M->GetBaseMaterial()
            &&M->GetBaseMaterial()->GetFName()==(FParse::Param(FCommandLine::Get(),TEXT("WFV8311TracksBaseline"))
                ?FName(TEXT("M_RunningGearImprint_V838")):FName(TEXT("M_RunningGearImprint_V8311")));
        if(C&&C->IsVisible())Contact&=FMath::Abs(C->GetComponentLocation().Z
            -AWarfareFrontlineGround::HeightAt(C->GetComponentLocation().X,C->GetComponentLocation().Y)-8.f)<.2f
            &&C->DecalSize.Y>=9.f&&C->DecalSize.Y<=42.f&&C->DecalSize.Z>=36.f&&C->DecalSize.Z<=88.f
            &&M&&FMath::IsFinite(M->K2_GetScalarParameterValue(TEXT("TreadPhase")));
    }
    Check(TEXT("v8311_imprint_actual_material_bound"),Bound);
    Check(TEXT("v8311_imprint_actual_terrain_projection_and_running_gear_width"),Contact);
}

namespace
{
struct FImprintDiagnosticSlotV8311
{int32 Slot=0;bool Visible=false;FLinearColor Colour=FLinearColor::Black;float Pressure=0,Softness=0,Roughness=0;};
TMap<TWeakObjectPtr<AFrontlineArmourSurfaceV837>,TArray<FImprintDiagnosticSlotV8311>> ImprintDiagnosticV8311;
}
TSharedRef<FJsonObject> AFrontlineArmourSurfaceV837::SetImprintDiagnosticV8311(int32 Mode)
{
    auto J=MakeShared<FJsonObject>();FString User;
    auto* B=Cast<AWarfareFrontlineGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    const bool Allowed=B&&B->IsProgressionVolatile()&&Mode>=0&&Mode<=2
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV8311Review"))&&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FrontlineUltimateArchiveV8215::ReviewUserDir(User);
    J->SetBoolField(TEXT("allowed"),Allowed);if(!Allowed)return J;
    J->SetNumberField(TEXT("mode"),Mode);J->SetBoolField(TEXT("actor_hidden"),IsHidden());
    const auto CVar=[&](const TCHAR* Name){auto* V=IConsoleManager::Get().FindConsoleVariable(Name);J->SetNumberField(Name,V?V->GetInt():-1);};
    for(const TCHAR* Name:{TEXT("r.DBuffer"),TEXT("r.ForwardShading"),TEXT("r.EarlyZPass"),TEXT("r.VelocityOutputPass")})CVar(Name);
    if(GEngine&&GEngine->GameViewport)
    {J->SetBoolField(TEXT("show_decals"),GEngine->GameViewport->EngineShowFlags.Decals);J->SetBoolField(TEXT("path_tracing"),GEngine->GameViewport->EngineShowFlags.PathTracing);}
    auto* Saved=ImprintDiagnosticV8311.Find(this);
    if(Mode!=0&&!Saved)
    {
        auto& State=ImprintDiagnosticV8311.FindOrAdd(this);
        for(int32 I=0;I<Tracks.Num();++I)if(auto* C=Tracks[I].Get();C&&C->GetVisibleFlag())if(auto* M=TrackMaterialsV838[I].Get())
        {
            FImprintDiagnosticSlotV8311 Row;Row.Slot=I;Row.Visible=C->GetVisibleFlag();
            const bool Read=M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("CompactedColour")),Row.Colour)
                &&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("GroundPressure")),Row.Pressure)
                &&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("GroundSoftness")),Row.Softness)
                &&M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("CompactedRoughness")),Row.Roughness);
            if(Read)State.Add(Row);
        }
        Saved=&State;
    }
    int32 Changed=0;
    if(Saved)for(const auto& Row:*Saved)if(Tracks.IsValidIndex(Row.Slot))
    {
        auto* C=Tracks[Row.Slot].Get();auto* M=TrackMaterialsV838[Row.Slot].Get();if(!C||!M)continue;
        if(Mode==1)
        {M->SetVectorParameterValue(TEXT("CompactedColour"),FLinearColor(.015f,.85f,.60f));M->SetScalarParameterValue(TEXT("GroundPressure"),16.f);
            M->SetScalarParameterValue(TEXT("GroundSoftness"),1.f);M->SetScalarParameterValue(TEXT("CompactedRoughness"),1.f);C->SetVisibility(Row.Visible);}
        else
        {M->SetVectorParameterValue(TEXT("CompactedColour"),Row.Colour);M->SetScalarParameterValue(TEXT("GroundPressure"),Row.Pressure);
            M->SetScalarParameterValue(TEXT("GroundSoftness"),Row.Softness);M->SetScalarParameterValue(TEXT("CompactedRoughness"),Row.Roughness);
            C->SetVisibility(Mode==2?false:Row.Visible);}
        ++Changed;
    }
    if(Mode==0)ImprintDiagnosticV8311.Remove(this);
    J->SetNumberField(TEXT("changed_existing_stamps"),Changed);
    TArray<TSharedPtr<FJsonValue>> Receivers,Decals,Shaders;TSet<UMaterialInterface*> Materials;
    for(TActorIterator<AWarfareFrontlineGround> It(GetWorld());It;++It)
    {
        TInlineComponentArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components)if(C&&C->GetStaticMesh())
        {
            auto Row=MakeShared<FJsonObject>();auto* M=C->GetMaterial(0);auto* Base=M?M->GetBaseMaterial():nullptr;
            Row->SetStringField(TEXT("component"),C->GetPathName());Row->SetStringField(TEXT("mesh"),C->GetStaticMesh()->GetPathName());
            Row->SetBoolField(TEXT("component_visible"),C->GetVisibleFlag());Row->SetBoolField(TEXT("component_hidden"),C->bHiddenInGame);
            Row->SetBoolField(TEXT("actor_hidden"),It->IsHidden());Row->SetBoolField(TEXT("receives_decals"),C->bReceivesDecals);
            Row->SetBoolField(TEXT("force_disable_nanite"),C->IsForceDisableNanite());Row->SetStringField(TEXT("bounds"),C->Bounds.GetBox().ToString());
            Row->SetStringField(TEXT("material"),GetPathNameSafe(M));Row->SetStringField(TEXT("base_material"),GetPathNameSafe(Base));
            Row->SetNumberField(TEXT("decal_response"),Base?int32(Base->MaterialDecalResponse):INDEX_NONE);
            Receivers.Add(MakeShared<FJsonValueObject>(Row));if(M)Materials.Add(M);
        }
    }
    for(int32 I=0;I<Tracks.Num();++I)if(auto* C=Tracks[I].Get();C&&C->GetVisibleFlag())
    {
        auto Row=MakeShared<FJsonObject>();auto* M=C->GetDecalMaterial();
        Row->SetNumberField(TEXT("slot"),I);Row->SetStringField(TEXT("material"),GetPathNameSafe(M));
        Row->SetStringField(TEXT("position"),C->GetComponentLocation().ToString());Row->SetStringField(TEXT("rotation"),C->GetComponentRotation().ToString());
        Row->SetStringField(TEXT("size"),C->DecalSize.ToString());Row->SetBoolField(TEXT("component_hidden"),C->bHiddenInGame);
        Row->SetBoolField(TEXT("render_state_created"),C->IsRenderStateCreated());
        const FVector P=C->GetComponentLocation();Row->SetNumberField(TEXT("height_above_analytic_ground_cm"),P.Z-AWarfareFrontlineGround::HeightAt(P.X,P.Y));
        Decals.Add(MakeShared<FJsonValueObject>(Row));if(M)Materials.Add(M);
    }
    for(auto* M:Materials)
    {
        struct FResult{bool Same=false,Complete=false,Valid=false;FString Name;};FResult Result;
        const auto* Assigned=M->GetRenderProxy();auto* Out=&Result;const auto Feature=GetWorld()->GetFeatureLevel();
        if(Assigned)
        {ENQUEUE_RENDER_COMMAND(WFImprintProxyV8311)([Assigned,Feature,Out](FRHICommandListImmediate&){const FMaterialRenderProxy* Effective=Assigned;
            const FMaterial& Actual=Assigned->GetMaterialWithFallback(Feature,Effective);Out->Same=Effective==Assigned;Out->Name=Actual.GetFriendlyName();
            Out->Complete=Actual.IsRenderingThreadShaderMapComplete();const auto* Map=Actual.GetRenderingThreadShaderMap();Out->Valid=Map&&Map->IsValidForRendering();});FlushRenderingCommands();}
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("assigned_material"),M->GetPathName());Row->SetStringField(TEXT("actual_shader"),Result.Name);
        Row->SetBoolField(TEXT("assigned_proxy_used"),Result.Same);Row->SetBoolField(TEXT("shader_complete"),Result.Complete);Row->SetBoolField(TEXT("shader_valid"),Result.Valid);
        Shaders.Add(MakeShared<FJsonValueObject>(Row));
    }
    J->SetArrayField(TEXT("receivers"),Receivers);J->SetArrayField(TEXT("visible_decals"),Decals);J->SetArrayField(TEXT("render_shaders"),Shaders);
    J->SetStringField(TEXT("shader_lookup_limitation"),TEXT("Generic GetMaterialWithFallback requires the entire shader map, unlike draw-pass-specific ODSC readiness. Its default result does not establish the decal draw pass used a fallback; use the colour/off/restored images for actual projection evidence."));
    J->SetStringField(TEXT("scope"),TEXT("Strict disposable current-stamp high-contrast/off/restored renderer probe. No production pressure, size, terrain, asset, persistence or quality change."));return J;
}
