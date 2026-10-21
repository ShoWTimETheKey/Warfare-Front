#include "WarfareFrontlineSupportArtV36.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUEquipmentV82.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "WarfareFrontlineFXV47.h"
#include "WarfareFrontlineSupportV28.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AFrontlineOrdnanceV36::AFrontlineOrdnanceV36()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Forged ammunition"));SetRootComponent(Body);
    Wake=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Short aerodynamic wake"));Wake->SetupAttachment(Body);
    for(auto* C:{Body.Get(),Wake.Get()}){C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetReceivesDecals(false);C->SetVisibility(false);}
}
bool AFrontlineOrdnanceV36::Configure(AWarfareFrontlineGameMode* B,const FVector& From,const FVector& To,float Launched,float Impact,bool Direct,bool AirBullet,bool Terminal,float TerminalSpeed)
{
    if(!B||From.ContainsNaN()||To.ContainsNaN()||!FMath::IsFinite(Launched)||!FMath::IsFinite(Impact)||!FMath::IsFinite(TerminalSpeed)||Impact<=Launched)return false;
    auto* Shape=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V36/Meshes/SM_Shell_V36.SM_Shell_V36"));
    auto* Quad=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V36/Materials/M_BallisticWake_V36.M_BallisticWake_V36"));
    if(!Shape||!Quad||!Material)return false;
    int32 Active=0;for(TActorIterator<AFrontlineOrdnanceV36> It(B->GetWorld());It;++It)if(*It!=this&&!It->IsActorBeingDestroyed())++Active;
    if(Active>=32)return false; // Visual-only cap never removes or changes the committed strike.
    Battle=B;Encounter=B->GetEncounterId();Origin=From;Target=To;LaunchAt=Launched;ImpactAt=Impact;bDirect=Direct;bBullet=AirBullet;
    bTerminalV43=Terminal&&!Direct&&!AirBullet;TerminalSpeedV43=FMath::Clamp(TerminalSpeed,7500.f,18000.f);
    Body->SetStaticMesh(Shape);Body->SetRelativeScale3D(FVector::OneVector);Wake->SetStaticMesh(Quad);
    WakeMaterial=UMaterialInstanceDynamic::Create(Material,this);Wake->SetMaterial(0,WakeMaterial);
    WakeMaterial->SetVectorParameterValue(TEXT("Tint"),bBullet?FLinearColor(4.8f,1.75f,.35f):FLinearColor(.19f,.19f,.18f));
    const float Clock=bCommittedSupportV8217?B->CommittedSupportTimeV8217():B->Elapsed;
    SetLifeSpan(FMath::Max(.2f,Impact-Clock+.25f));UpdateVisual(Clock);return true;
}
FVector AFrontlineOrdnanceV36::PositionAt(const FVector& From,const FVector& To,float Duration,float T,bool Direct)
{
    Duration=FMath::Max(.001f,Duration);T=FMath::Clamp(T,0.f,Duration);
    return Direct?FMath::Lerp(From,To,T/Duration):From+((To-From)/Duration+FVector(0,0,490.f*Duration))*T-FVector(0,0,490.f*T*T);
}
FVector AFrontlineOrdnanceV36::TerminalPositionV43(const FVector& From,const FVector& To,float SecondsToImpact,float Speed)
{
    const float Remaining=FMath::Max(0.f,SecondsToImpact);
    FVector Direction=(To-From).GetSafeNormal();
    // Indirect shells enter from above; an invalid near-horizontal request must
    // not turn a committed impact into an ascending visible projectile.
    Direction.Z=FMath::Min(-.55,Direction.Z);Direction.Normalize();
    const FVector ImpactVelocity=Direction*FMath::Clamp(Speed,7500.f,18000.f);
    // Back-integrate gravity from the immutable impact endpoint and clock.
    return To-ImpactVelocity*Remaining-FVector(0,0,490.f*Remaining*Remaining);
}
void AFrontlineOrdnanceV36::UpdateVisual(float Time)
{
    if(!WakeMaterial)return;
    const float VisibleLead=bTerminalV43?FMath::Clamp(3800.f/TerminalSpeedV43,.26f,.46f):ImpactAt-LaunchAt;
    const float VisibleStart=FMath::Max(LaunchAt,ImpactAt-VisibleLead);
    const bool Visible=Time>=VisibleStart&&Time<ImpactAt;Body->SetVisibility(Visible&&!bBullet);Wake->SetVisibility(Visible);
    if(!Visible)return;
    const float Duration=ImpactAt-LaunchAt,T=Time-LaunchAt;
    const float Remaining=ImpactAt-Time;
    const FVector P=bTerminalV43?TerminalPositionV43(Origin,Target,Remaining,TerminalSpeedV43):PositionAt(Origin,Target,Duration,T,bDirect);
    const FVector V=bTerminalV43?(TerminalPositionV43(Origin,Target,FMath::Max(0.f,Remaining-.001f),TerminalSpeedV43)-P)/FMath::Min(.001f,Remaining)
        :(Target-Origin)/Duration+(bDirect?FVector::ZeroVector:FVector(0,0,490.f*Duration-980.f*T));
    const FVector Forward=V.GetSafeNormal();SetActorLocation(P);SetActorRotation(Forward.Rotation());
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    FVector Eye=Camera?(Camera->GetCameraLocation()-P).GetSafeNormal():FVector(0,-1,.3f);
    if(FMath::Abs(FVector::DotProduct(Forward,Eye))>.98f)Eye=FVector(0,0,1);
    const float Length=bBullet?FMath::Clamp(float(V.Size())*(bHeavyBulletV8216?.013f:.011f),75.f,bHeavyBulletV8216?240.f:165.f):FMath::Clamp(float(V.Size())*.013f,95.f,240.f);
    Wake->SetWorldLocation(P-Forward*Length*.54f);Wake->SetWorldRotation(FRotationMatrix::MakeFromXZ(Forward,Eye).Rotator());
    Wake->SetWorldScale3D(FVector(Length/100.f,bBullet?(bHeavyBulletV8216?.055f:.026f):.065f,1));
    // A brief separated tracer dash; shell wake is low-density cold air, never an exhaust flame.
    WakeMaterial->SetScalarParameterValue(TEXT("Density"),bBullet?.82f:.30f*FMath::Clamp((Time-VisibleStart)/.028f,0.f,1.f));
    WakeMaterial->SetScalarParameterValue(TEXT("Age"),T);
}
void AFrontlineOrdnanceV36::Tick(float DT)
{
    Super::Tick(DT);auto* B=Battle.Get();
    if(!B||B->GetEncounterId()!=Encounter||(B->Phase!=EFrontlinePhase::Fighting&&(!bCommittedSupportV8217||!B->CanResolveCommittedSupportV8217()))){Destroy();return;}
    const float Clock=bCommittedSupportV8217?B->CommittedSupportTimeV8217():B->Elapsed;
    if(Clock>=ImpactAt)
    {
        if(bBullet&&bGroundImpact)if(auto* Puff=GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Target,FRotator::ZeroRotator))Puff->ConfigureStrafeStrike((Target-Origin).GetSafeNormal());
        Destroy();return;
    }
    UpdateVisual(Clock);
}
bool AFrontlineOrdnanceV36::IsVisualReady()const{return Body&&Body->GetStaticMesh()&&WakeMaterial&&Wake&&Wake->GetStaticMesh();}
namespace FrontlineSupportArtV36
{
namespace
{
void RequestResident(UMaterialInterface* Material)
{
    // Small shared effects can be prepared cheaply. Aircraft paint and other
    // large maps obey the view-dependent pool instead of evicting terrain mips
    // every time a support aircraft is requested.
    if(!Material)return;
    TArray<UTexture*> Textures;Material->GetUsedTextures(Textures);
    for(UTexture* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))
        if(FMath::Max(T->GetSizeX(),T->GetSizeY())<=1024)
            T->SetForceMipLevelsToBeResident(5.f);
}
}
UStaticMesh* Aircraft(bool German)
{
    const TCHAR* Key=German?TEXT("Halberstadt"):TEXT("Bristol");
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V40/Meshes/SM_%s_V40.SM_%s_V40"),Key,Key));
    if(!Mesh)Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V36/Meshes/SM_%s_V36.SM_%s_V36"),Key,Key));
    // The visible wing is one cell of a shared atlas. A 64px resident mip
    // reduces its paint and linen to 16px even at 4K. Request the four small
    // atlases before the offscreen approach, then release the residency lease.
    // This is bounded to an aircraft appearance, never a per-frame global pin.
    if(Mesh)
    {
        TSet<UTexture2D*> AirframeTextures;
        for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
        {
            TArray<UTexture*> Textures;Slot.MaterialInterface->GetUsedTextures(Textures);
            for(auto* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))AirframeTextures.Add(T);
        }
        for(auto* T:AirframeTextures)T->SetForceMipLevelsToBeResident(10.f);
    }
    return Mesh;
}
UStaticMesh* AircraftFor(const AWarfareFrontlineGameMode* Battle,int32 Side)
{
    if(!Battle||Side<0||Side>1)return nullptr;
    if(!WarfareFRRUCampaignV82::Active(Battle))return Aircraft(Battle->GetFactionForTeam(Side)==EFrontlineFaction::German);
    const auto Nation=WarfareFRRUCampaignV82::Nation(Battle,Side);
    const int32 Date=WarfareFRRUCampaignV82::CalendarDate(Battle);
    if(Nation==EFRRUNationV81::German)
    {
        if(!WarfareFRRUEquipmentV82::SupportAircraftYearAllowedV87(Nation,EFrontlineSupport::AirBombing,Date))return nullptr;
        return Aircraft(true);
    }
    const TCHAR* Key=nullptr;
    if(Nation==EFRRUNationV81::French&&Date>=19160101&&Date<=19181231)
        Key=Date<19161101?TEXT("VoisinLAS"):TEXT("VoisinVIII");
    if(Nation==EFRRUNationV81::Russian&&Date>=19160201&&Date<=19171231)Key=TEXT("MurometsG");
    if(!Key)return nullptr;
    const FString Name=FString::Printf(TEXT("SM_%s_V82"),Key);
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Aircraft/%s.%s"),*Name,*Name));
    if(Mesh)for(const auto& Slot:Mesh->GetStaticMaterials())RequestResident(Slot.MaterialInterface);
    return Mesh;
}
void PrewarmVisuals()
{
    FrontlineFXV47::Prewarm();
    Aircraft(false);Aircraft(true);
    RequestResident(FrontlineSmokeVolumeV35::MaterialV40());
    for(const TCHAR* Kind:{TEXT("Ejecta"),TEXT("Roll")})
    {
        auto* Material=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V47/Materials/M_Blast%s_V47.M_Blast%s_V47"),Kind,Kind));
        if(!Material)Material=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V38/Materials/M_Blast%s_V38.M_Blast%s_V38"),Kind,Kind));
        if(!Material)Material=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Warfare/Field/V31/Materials/M_Blast%s_V31.M_Blast%s_V31"),Kind,Kind));
        RequestResident(Material);
    }
    RequestResident(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V31/Materials/M_BlastFlash_V31.M_BlastFlash_V31")));
}
bool PrepareAircraftReview(bool Report)
{
    bool Ready=true;TSet<UTexture2D*> Seen;
    for(bool German:{false,true})
    {
        auto* Mesh=Aircraft(German);Ready&=Mesh!=nullptr;if(!Mesh)continue;
        for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface)
        {
            TArray<UTexture*> Used;Slot.MaterialInterface->GetUsedTextures(Used);
            for(auto* Texture:Used)if(auto* T=Cast<UTexture2D>(Texture))if(!Seen.Contains(T))
            {
                Seen.Add(T);Ready&=T->GetResource()!=nullptr&&T->GetNumResidentMips()==T->GetNumMips();
                if(Report)UE_LOG(LogTemp,Display,TEXT("WF_V67_AIRCRAFT_RESIDENCY texture=%s resident=%d total=%d size=%dx%d"),*T->GetName(),T->GetNumResidentMips(),T->GetNumMips(),T->GetSizeX(),T->GetSizeY());
            }
        }
    }
    return Ready&&!Seen.IsEmpty();
}
AFrontlineOrdnanceV36* SpawnShell(AWarfareFrontlineGameMode* B,const FFrontlineStrike& S)
{
    if(!B)return nullptr;auto* A=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();
    if(A)A->SetCommittedSupportV8217(S.SourceV65.Support>=0&&S.SourceV65.Support<int32(EFrontlineSupport::Count));
    if(A&&!A->Configure(B,S.Origin,S.Target,S.LaunchedAt,S.ImpactAt,S.bDirectFire,false,!S.bAirDropped,S.bMortar?8500.f:14500.f)){A->Destroy();return nullptr;}return A;
}
void StrafeRound(AWarfareFrontlineGameMode* B,const FVector& Muzzle,const FVector& Ground,float Duration,bool GroundImpact,bool CommittedSupport)
{
    if(!B)return;auto* A=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();
    if(A){A->SetCommittedSupportV8217(CommittedSupport);A->SetGroundImpactEnabled(GroundImpact);const float Clock=CommittedSupport?B->CommittedSupportTimeV8217():B->Elapsed;if(!A->Configure(B,Muzzle,Ground,Clock,Clock+Duration,false,true))A->Destroy();}
}
void StrafeRoundAtV8220(AWarfareFrontlineGameMode* B,const FVector& Muzzle,const FVector& Ground,float LaunchAt,float ImpactAt)
{
    // A coarse game step can reach an impact before its optional tracer is
    // allocated. Never replay that tracer or move the real impact into the future.
    if(!B||B->CommittedSupportTimeV8217()>=ImpactAt)return;
    if(auto* A=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>())
    {
        A->SetCommittedSupportV8217();A->SetGroundImpactEnabled(false);
        if(!A->Configure(B,Muzzle,Ground,LaunchAt,ImpactAt,false,true))A->Destroy();
    }
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{
    using WarfareFRRUEquipmentV82::SupportAircraftYearAllowedV87;
    Check(TEXT("FRRU German CL.II negative-year filter excludes both 1916 aircraft tasks"),
        !SupportAircraftYearAllowedV87(EFRRUNationV81::German,EFrontlineSupport::AirStrike,19160601)
        &&!SupportAircraftYearAllowedV87(EFRRUNationV81::German,EFrontlineSupport::AirBombing,19161231));
    Check(TEXT("FRRU negative-year filter preserves other nations and artillery identities"),
        SupportAircraftYearAllowedV87(EFRRUNationV81::French,EFrontlineSupport::AirBombing,19160601)
        &&SupportAircraftYearAllowedV87(EFRRUNationV81::Russian,EFrontlineSupport::AirBombing,19160601)
        &&SupportAircraftYearAllowedV87(EFRRUNationV81::German,EFrontlineSupport::AirRecon,19160601)
        &&SupportAircraftYearAllowedV87(EFRRUNationV81::German,EFrontlineSupport::CreepingBarrage,19160601));
    Check(TEXT("FRRU negative-year rule adds no new 1917 or 1918 exclusion or service-day claim"),
        SupportAircraftYearAllowedV87(EFRRUNationV81::German,EFrontlineSupport::AirBombing,19170416)
        &&SupportAircraftYearAllowedV87(EFRRUNationV81::German,EFrontlineSupport::AirStrike,19180611));
    bool Shapes=true,Sections=true;
    for(bool German:{false,true}){auto* M=Aircraft(German);Shapes&=M&&M->GetBounds().BoxExtent.Y>500;Sections&=M&&M->GetStaticMaterials().Num()<=2;}
    Check(TEXT("V36 aircraft assets preserve historical span and two-section budget"),Shapes&&Sections);
    if(B&&!WarfareFRRUCampaignV82::Active(B))
        Check(TEXT("legacy UK DE actual aircraft resolver remains unchanged"),
            AircraftFor(B,0)==Aircraft(B->GetFactionForTeam(0)==EFrontlineFaction::German)
            &&AircraftFor(B,1)==Aircraft(B->GetFactionForTeam(1)==EFrontlineFaction::German));
    FVector A(-1400,190,2300),Z(500,-100,45);bool Arc=true;
    for(bool Direct:{false,true})Arc&=AFrontlineOrdnanceV36::PositionAt(A,Z,2.1f,0,Direct).Equals(A,.01)&&AFrontlineOrdnanceV36::PositionAt(A,Z,2.1f,2.1f,Direct).Equals(Z,.01);
    Check(TEXT("V36 shell and tracer endpoints match committed strike"),Arc);
    if(!B){Check(TEXT("V36 live shell material and arrival profile"),false);return;}
    if(WarfareFRRUCampaignV82::Active(B)&&WarfareFRRUCampaignV82::Nation(B,1)==EFRRUNationV81::German
        &&WarfareFRRUCampaignV82::CalendarDate(B)/10000<1917)
    {
        const float BeforeTime=B->Elapsed;const int32 BeforeStrikes=B->Strikes.Num();
        auto* Mission=B->GetWorld()->SpawnActor<AFrontlineSupportMissionV28>();
        const bool DirectClosed=Mission&&!Mission->Configure(B,EFrontlineSupport::AirStrike,FVector::ZeroVector,1);
        if(Mission)Mission->Destroy();
        Check(TEXT("FRRU actual early German availability aircraft and direct strafe consumer reject before combat commit"),
            !WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::AirStrike,1)
            &&!WarfareFRRUEquipmentV82::SupportAvailable(B,EFrontlineSupport::AirBombing,1)
            &&AircraftFor(B,1)==nullptr&&DirectClosed&&B->Elapsed==BeforeTime&&B->Strikes.Num()==BeforeStrikes);
    }
    auto* Shell=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();bool Ready=Shell&&Shell->Configure(B,A,Z,B->Elapsed-.4f,B->Elapsed+1.7f,false)&&Shell->IsVisualReady();if(Shell)Shell->Destroy();
    auto* Puff=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Z,FRotator::ZeroRotator);if(Puff)Puff->ConfigureStrafeStrike(FVector(1,0,-1));Ready&=Puff&&Puff->IsVisualReady()&&!Puff->IsFlashVisible()&&Puff->GetBlastProfileIndex()==3;if(Puff)Puff->Destroy();
    Check(TEXT("V36 live shell material and strafe dust never create artillery flash"),Ready);
}
void SetupVisualFixture(AWarfareFrontlineGameMode* B)
{
    if(!B)return;
    // Record actual residency for this focused capture; requesting mips alone
    // is not evidence that their GPU resources were ready in the screenshot.
    for(const TCHAR* Path:{TEXT("/Game/Warfare/Field/V36/Materials/M_Airframe_V36.M_Airframe_V36"),
        TEXT("/Game/Warfare/Field/V38/Materials/M_BlastEjecta_V38.M_BlastEjecta_V38"),
        TEXT("/Game/Warfare/Field/V38/Materials/M_BlastRoll_V38.M_BlastRoll_V38")})
        if(auto* Material=LoadObject<UMaterialInterface>(nullptr,Path))
        {
            TArray<UTexture*> Textures;Material->GetUsedTextures(Textures);
            for(UTexture* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))
                UE_LOG(LogTemp,Display,TEXT("WF_V36_ART_RESIDENCY texture=%s resident=%d total=%d size=%dx%d"),*T->GetName(),T->GetNumResidentMips(),T->GetNumMips(),T->GetSizeX(),T->GetSizeY());
        }
    for(int32 I=0;I<2;++I)
    {
        auto* A=B->GetWorld()->SpawnActor<AActor>();auto* M=NewObject<UStaticMeshComponent>(A);A->SetRootComponent(M);A->AddInstanceComponent(M);M->SetStaticMesh(Aircraft(I==1));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->RegisterComponent();
        A->SetActorLocation(FVector(I?1400:-1400,I?250:0,1150));A->SetActorRotation(FRotator(-4,I?201:21,I?-3:3));A->SetLifeSpan(10);
    }
    for(int32 I=0;I<3;++I)
    {
        FVector Target(-650+I*550,-160,0);Target.Z=AWarfareFrontlineGround::HeightAt(Target.X,Target.Y);
        if(auto* Impact=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Target,FRotator::ZeroRotator))Impact->ConfigureBlast(false,I==0);
    }
    // Static captured material at flight midpoint; production movement is checked independently.
    FFrontlineStrike S;S.Origin=FVector(-700,-200,1900);S.Target=FVector(1000,-200,0);S.LaunchedAt=B->Elapsed-1.3;S.ImpactAt=B->Elapsed+1.3;
    if(auto* Shell=SpawnShell(B,S)){Shell->SetActorTickEnabled(false);Shell->UpdateVisual(B->Elapsed+.85);Shell->SetLifeSpan(10);}
    for(int32 I=0;I<5;++I){const FVector P(-650+I*190,-250,0);if(auto* Puff=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(FVector(P.X,P.Y,AWarfareFrontlineGround::HeightAt(P.X,P.Y)),FRotator::ZeroRotator))Puff->ConfigureStrafeStrike(FVector(1,0,-1));}
}
}

namespace FrontlineSupportArtV43
{
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const FString&,bool)> Check)
{
    bool Finite=true,Descending=true,Endpoint=true,Speed=true;
    const FVector End(700,140,30),From=End+FVector(-650,0,1450);
    for(float Rate:{8500.f,14500.f})
    {
        Endpoint&=AFrontlineOrdnanceV36::TerminalPositionV43(From,End,0,Rate).Equals(End,.001);
        FVector Previous=AFrontlineOrdnanceV36::TerminalPositionV43(From,End,.46f,Rate);
        for(int32 I=1;I<=46;++I)
        {
            const FVector P=AFrontlineOrdnanceV36::TerminalPositionV43(From,End,.46f-I*.01f,Rate);
            Finite&=!P.ContainsNaN();Descending&=P.Z<Previous.Z;
            Speed&=FVector::Distance(P,Previous)/.01f>7500.f;Previous=P;
        }
    }
    Check(TEXT("v43_terminal_shells_finite_descending_and_impact_exact"),Finite&&Descending&&Endpoint);
    Check(TEXT("v43_terminal_rounds_have_brief_fast_descent"),Speed);
    const FVector Shot(2000,300,5);
    Check(TEXT("v43_direct_fire_endpoint_preserved"),AFrontlineOrdnanceV36::PositionAt(From,Shot,.42f,.42f,true).Equals(Shot,.001));
    bool Ready=false;
    if(B)if(auto* A=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>())
    {
        Ready=A->Configure(B,From,End,B->Elapsed-2,B->Elapsed+.15f,false)&&A->IsVisualReady();
        A->UpdateVisual(B->Elapsed);Ready&=A->GetActorLocation().Equals(AFrontlineOrdnanceV36::TerminalPositionV43(From,End,.15f,14500.f),.5f);
        A->Destroy();
    }
    Check(TEXT("v43_actual_terminal_actor_uses_committed_clock"),Ready);
}
bool SetupTerminalFixture(AWarfareFrontlineGameMode* B,float SecondsBeforeImpact)
{
    if(!B||!FParse::Param(FCommandLine::Get(),TEXT("WFV43Review")))return false;
    const FName Tag(TEXT("WF_V43_TERMINAL_REVIEW"));
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->Tags.Contains(Tag))It->Destroy();
    bool Ready=true;
    for(int32 I=0;I<2;++I)
    {
        FVector End(I?650:-650,70,0);End.Z=AWarfareFrontlineGround::HeightAt(End.X,End.Y);
        auto* A=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();if(!A){Ready=false;continue;}
        A->Tags.Add(Tag);const float Left=FMath::Max(.001f,SecondsBeforeImpact);
        Ready&=A->Configure(B,End+FVector(I?-350:-650,0,1450),End,B->Elapsed-2,B->Elapsed+Left,false,false,true,I?8500.f:14500.f);
        A->UpdateVisual(B->Elapsed);A->SetActorTickEnabled(false);A->SetLifeSpan(12.f);
    }
    return Ready;
}
}

namespace FrontlineSupportArtV8220
{
FrontlineSupportArtV40::FFlight BomberFlight(const FVector& Target,int32 Direction,float Age)
{
    const int32 Dir=Direction>=0?1:-1;
    const float Fall=FMath::Sqrt(2.f*BomberHeight/980.f);
    const float Run=(Age-1.925f)*BomberSpeed-BomberSpeed*Fall;
    const float Exit=FMath::Max(0.f,Age-2.75f);
    FrontlineSupportArtV40::FFlight F;
    F.Position=Target+FVector(Dir*Run,-170.f+Dir*Exit*Exit*35.f,BomberHeight+Exit*Exit*48.f);
    F.Velocity=FVector(Dir*BomberSpeed,Dir*Exit*70.f,Exit*96.f);
    F.Rotation=F.Velocity.Rotation();F.Rotation.Roll=Dir*(4.f+FMath::Sin(Age*.6f)*7.f);
    return F;
}
}
namespace FrontlineSupportArtV40
{
FFlight AttackFlight(const FVector& Target,int32 Direction,float T,int32 PassIndex)
{
    const FVector Along=FVector(Direction>=0?1:-1,(Direction>=0?1:-1)*.13,0).GetSafeNormal();
    const FVector Side(-Along.Y,Along.X,0);
    const float Speed=2300.f,Ahead=3500.f,DiveEnd=2.75f;
    const float DiveT=FMath::Min(T,DiveEnd),Height=1350.f*FMath::Exp(-(DiveT-1.925f)*Speed/Ahead);
    float Z=Height,VZ=-Speed/Ahead*Height;
    // Side reverses with the return heading, giving two different ground lanes.
    float Offset=320.f,SideSpeed=0,Bank=0;
    if(T<1.35f)
    {
        const float U=FMath::Max(0.f,1.35f-T);const float Sign=PassIndex?-1.f:1.f;
        // Enter in a shallow bank, then lower the nose continuously. Extending
        // the attack exponential backwards would make the distant approach
        // almost vertical. Position and velocity still meet exactly at1.35s.
        const float EntryHeight=1350.f*FMath::Exp(.575f*Speed/Ahead);
        const float BlendRate=Speed/Ahead*EntryHeight-120.f,Decay=FMath::Exp(-U/1.4f);
        Z=EntryHeight+BlendRate*1.4f*(1-Decay)+120.f*U;VZ=-BlendRate*Decay-120.f;
        Offset+=Sign*150.f*U*U;SideSpeed=-Sign*300.f*U;Bank=Sign*FMath::Clamp(U*13.f,0.f,24.f);
    }
    else if(T>DiveEnd)
    {
        const float U=T-DiveEnd,Sign=PassIndex?1.f:-1.f;
        Z+=VZ*U+420.f*U*U;VZ+=840.f*U;
        Offset+=Sign*115.f*U*U;SideSpeed=Sign*230.f*U;Bank=-Sign*FMath::Clamp(U*16.f,0.f,27.f);
    }
    FFlight F;F.Position=Target+Along*((T-1.925f)*Speed-Ahead)+Side*Offset;
    F.Position.Z=Target.Z+Z;F.Velocity=Along*Speed+Side*SideSpeed+FVector(0,0,VZ);
    F.Rotation=F.Velocity.Rotation();F.Rotation.Roll=Bank;
    return F;
}
FFlight TransitFlight(const FVector& Target,int32 Direction,float Age)
{
    const float Along=Direction*((Age-2.86f)*1250.f);
    const float Curve=FMath::Max(Age-4.f,0.f),Bank=Direction*(4.f+FMath::Sin(Age*.6f)*7.f);
    FFlight F;F.Position=FVector(Target.X+Along,Target.Y-170+Along*.28f+Direction*Curve*Curve*35.f,Target.Z+1550+55*FMath::Sin(Age*.7f)+Curve*Curve*48);
    F.Velocity=FVector(Direction*1250,Direction*(350+Curve*70),38.5*FMath::Cos(Age*.7f)+Curve*96);
    F.Rotation=F.Velocity.Rotation();F.Rotation.Roll=Bank;return F;
}
bool ForwardGroundShot(const FVector& Muzzle,const FVector& Forward,FVector& OutPoint,float& OutDuration)
{
    if(Muzzle.ContainsNaN()||Forward.ContainsNaN()||Forward.Z>=-.05)return false;
    const FVector Velocity=Forward.GetSafeNormal()*76000.f;
    auto At=[&](float T){return Muzzle+Velocity*T-FVector(0,0,490.f*T*T);};
    float Before=0;FVector P=Muzzle;
    for(int32 I=1;I<=64;++I)
    {
        const float T=I*(.20f/64.f);P=At(T);
        if(P.Z<=AWarfareFrontlineGround::HeightAt(P.X,P.Y))
        {
            float Lo=Before,Hi=T;
            for(int32 J=0;J<10;++J){const float Mid=(Lo+Hi)*.5f;const FVector M=At(Mid);if(M.Z>AWarfareFrontlineGround::HeightAt(M.X,M.Y))Lo=Mid;else Hi=Mid;}
            OutDuration=(Lo+Hi)*.5f;OutPoint=At(OutDuration);OutPoint.Z=AWarfareFrontlineGround::HeightAt(OutPoint.X,OutPoint.Y);
            return OutDuration>.001f&&FVector::DotProduct(OutPoint-Muzzle,Forward)>0;
        }
        Before=T;
    }
    return false;
}
void RunChecks(AWarfareFrontlineGameMode* B,TFunctionRef<void(const TCHAR*,bool)> Check)
{
    FString UserDir;const bool Safe=B&&B->IsProgressionVolatile()&&FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))&&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
    Check(TEXT("V40 air review requires disposable profile"),Safe);if(!Safe)return;
    bool Forward=true,Contact=true,Finite=true,Curved=true,Bounded=true;
    for(int32 D:{-1,1})for(int32 Pass=0;Pass<2;++Pass)
    {
        const FVector Target(0,0,AWarfareFrontlineGround::HeightAt(0,0));
        for(int32 I=0;I<=23;++I)
        {
            const auto F=AttackFlight(Target,D,1.35f+I*.05f,Pass);const FVector M=FTransform(F.Rotation,F.Position).TransformPosition(FVector(287,-31,49));
            FVector Hit;float Duration=0;const bool Found=ForwardGroundShot(M,F.Rotation.Vector(),Hit,Duration);
            Forward&=Found&&FVector::DotProduct((Hit-M).GetSafeNormal(),F.Rotation.Vector())>.999f&&FMath::Abs(Hit.X-M.X)>1500&&Hit.Z<M.Z;
            Contact&=Found&&FMath::Abs(Hit.Z-AWarfareFrontlineGround::HeightAt(Hit.X,Hit.Y))<.1f;
            Bounded&=Found&&Duration>.02f&&Duration<.12f&&FMath::Abs(Hit.X)<1800&&FMath::Abs(Hit.Y)<550;
            Finite&=!F.Position.ContainsNaN()&&!F.Velocity.ContainsNaN()&&!F.Rotation.ContainsNaN();
            if(Found)Contact&=AFrontlineOrdnanceV36::PositionAt(M,Hit,Duration,Duration,false).Equals(Hit,.01f);
        }
        const auto Approach=AttackFlight(Target,D,.5f,Pass),Dive=AttackFlight(Target,D,1.9f,Pass),Exit=AttackFlight(Target,D,4.2f,Pass);
        Curved&=Approach.Rotation.Roll!=Dive.Rotation.Roll&&Dive.Rotation.Pitch< -10&&Exit.Rotation.Pitch>10;
    }
    Check(TEXT("V40 strafing exits fixed nose forward and down"),Forward);
    Check(TEXT("V40 committed ballistic tracer terminates on actual terrain"),Contact&&Bounded);
    Check(TEXT("V40 both armies approach dive and bank away with finite poses"),Finite&&Curved);
    RunFrontlineSupportChecksV40(B,Check);
}
bool SetupFlightFixture(AWarfareFrontlineGameMode* B,float PassAge)
{
    FString UserDir;if(!B||!B->IsProgressionVolatile()||!FParse::Param(FCommandLine::Get(),TEXT("WFV40Review"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||UserDir.IsEmpty())return false;
    const FName Tag(TEXT("V40AirPhaseFixture"));for(TActorIterator<AActor> It(B->GetWorld());It;++It)if(It->ActorHasTag(Tag))It->Destroy();
    bool Ready=true;
    for(int32 I=0;I<2;++I)
    {
        // Independent inspection lanes prevent two opposing frozen aircraft
        // from overlapping and hiding the bank silhouettes at the pullout.
        const float X=I?1250.f:-1250.f,Y=I?1150.f:-1150.f;
        const FVector Target(X,Y,AWarfareFrontlineGround::HeightAt(X,Y));
        const auto F=AttackFlight(Target,I?-1:1,PassAge,I);
        auto* A=B->GetWorld()->SpawnActor<AActor>();A->Tags.Add(Tag);auto* M=NewObject<UStaticMeshComponent>(A);A->SetRootComponent(M);A->AddInstanceComponent(M);
        M->SetStaticMesh(FrontlineSupportArtV36::Aircraft(I==1));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->RegisterComponent();A->SetActorTransform(FTransform(F.Rotation,F.Position));A->SetLifeSpan(15.f);Ready&=M->GetStaticMesh()!=nullptr;
        if(PassAge>=1.35f&&PassAge<=2.5f)
        {
            const FVector Muzzle=A->GetActorTransform().TransformPosition(FVector(287,-31,49));FVector Hit;float Duration;
            if(ForwardGroundShot(Muzzle,F.Rotation.Vector(),Hit,Duration))
            {
                auto* Round=B->GetWorld()->SpawnActor<AFrontlineOrdnanceV36>();Round->Tags.Add(Tag);
                Ready&=Round->Configure(B,Muzzle,Hit,B->Elapsed,B->Elapsed+Duration,false,true);Round->SetActorTickEnabled(false);Round->UpdateVisual(B->Elapsed+Duration*.65f);Round->SetLifeSpan(15.f);
                if(auto* Dust=B->GetWorld()->SpawnActor<AWarfareFrontlineImpact>(Hit,FRotator::ZeroRotator)){Dust->Tags.Add(Tag);Dust->ConfigureStrafeStrike(Hit-Muzzle);Dust->SetActorTickEnabled(false);Dust->Tick(0);Dust->Tick(.1f);Dust->SetLifeSpan(15.f);}
            }
            else Ready=false;
        }
    }
    return Ready;
}
}
