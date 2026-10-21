#include "WarfareFRRUArmourV82.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "WarfareFRRUCampaignV82.h"
#include "WarfareFRRUWorldV82.h"
#include "FRRU/FRRUVehicleNavV87.h"
#include "FRRU/WarfareFRRUGermanHostAcceptanceV82.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineAudio.h"
#include "WarfareFrontlineImpact.h"
#include "WarfareFrontlineWeaponProfile.h"
#include "WarfareFrontlineGameplayV43.h"
#include "WarfareFrontlinePacingV24.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFRRUPresentationV82.h"
#include "FRRUTrainGeometryV82.generated.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "Native/Expansion.h"
#include "Native/SceneRuntime.h"
#include "Native/TacticalRuntime.h"
#include "Native/UE/ExpansionArchiveUE.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "Algo/Sort.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include <algorithm>
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "WarfareFRRUContentV82.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimCompressionTypes.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Math/RotationMatrix.h"
#include "HAL/PlatformTime.h"
#include <cfloat>
#if WITH_EDITOR
#include "FRRU/FRRUVehicleNavCandidateV87.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "WarfareFRRUCampaignInternalV82.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "FRRU/FRRUParityReviewSafetyV87.h"
#endif

namespace WarfareFRRUArmourV82
{
namespace
{
// Deployment acceptance is independent of import success and sealed source flags.
constexpr bool FormalFiveVehicleAcceptance=true;
struct FMount {FVector Origin;float BaseYaw=0,Arc=0;FName YawBone,PitchBone;FVector LocalMuzzle=FVector::ZeroVector;FTransform RestYaw;FName MuzzleBone=NAME_None;};
struct FMechanicalOverlayV87 {FName Bone,ParentBone;int32 SkeletonIndex=INDEX_NONE;FTransform RestLocal;int32 WeaponIndex=0;bool Cannon=true;};
struct FWeaponContact
{
    AWarfareFrontlineSoldier* Infantry=nullptr;AFrontlineTank* Armour=nullptr;
    FVector Point=FVector::ZeroVector;double DistanceCm=DBL_MAX;
    AActor* Actor()const{return Armour?static_cast<AActor*>(Armour):static_cast<AActor*>(Infantry);}
};
struct FMountReadback
{
    TWeakObjectPtr<AActor> Target;FVector Point=FVector::ZeroVector,Muzzle=FVector::ZeroVector;
    double DistanceCm=-1,YawDelta=0,Pitch=0;int32 Mount=INDEX_NONE,TargetSquad=INDEX_NONE,TargetMember=INDEX_NONE,TargetSerial=INDEX_NONE;
    bool Cannon=false,Enabled=false,Selected=false,WithinRange=false,WithinSector=false,WithinPitch=false;
    bool FinalLOSChecked=false,FinalLOSClear=false,YawAligned=false,Fired=false;
};
struct FPassenger
{
    TWeakObjectPtr<AWarfareFrontlineSoldier> Actor;
    FFRRUTrainPassengerStateV82 State;
    FVector From=FVector::ZeroVector,To=FVector::ZeroVector;
    float PhaseSeconds=0,WalkDistance=0;int32 AisleLeg=0;
    ECollisionEnabled::Type FieldCollision=ECollisionEnabled::NoCollision;
};
struct FVehicle
{
    FString StableId;TWeakObjectPtr<AWarfareFrontlineGameMode> Battle;
    TWeakObjectPtr<UPoseableMeshComponent> Model;TWeakObjectPtr<USkeletalMeshComponent> Driver;
    TWeakObjectPtr<UBoxComponent> Hull;
    TArray<FMount> Machineguns,Cannons;
    TArray<float> MachinegunYaw,CannonYaw;
    FName RootBone;FTransform RootRest;
    float Phase=0,Heading=0,Wait=0,RailHold=0,AvoidHeading=0,AvoidUntil=0,AvoidRetryAt=0;bool CrewUnloaded=false,RailReturning=false;
    FGuid Encounter;TArray<FPassenger> Passengers;
    TArray<TWeakObjectPtr<UBoxComponent>> CarriageBoxes;
    TWeakObjectPtr<UStaticMeshComponent> Doors[2];TWeakObjectPtr<UBoxComponent> DoorBoxes[2];
    float DoorOpen[2]={0,0},LastPassengerHostAt=0;bool CrewLossHandled=false,DismountRegistered=false;
    FFRRUArmourEffectiveSpecV87 Spec;
    TWeakObjectPtr<AWarfareFrontlineSoldier> RepairWorker;
    float RepairProgress=0,RepairStartedAt=0,RepairLastHostAt=-1;int32 RepairSide=INDEX_NONE;
    TWeakObjectPtr<UAnimSequence> RecoilClip,ServiceClip;
    TArray<FMechanicalOverlayV87> RecoilBones,HatchBones;
    TArray<float> CannonRecoilAge,GunRecoilAge;
    bool WreckPoseApplied=false;int32 AppearanceMaterials=0,RecoilFrames=0;float MaximumRecoilCm=0,LastRecoilCm=0,MaximumMechanismAngle=0;
#if WITH_EDITOR
    float EngagementHostAt=-1,LastTravelCm=0;bool HaltForWeapon=false,HullAcquiringArmour=false;
    FMountReadback StopContact;TArray<FMountReadback> WeaponReadbacks;
#endif
};
struct FFleet
{
    FGuid Encounter;float ReadyAt[2]={0,0},Cycle[2]={1,1},EnemyAt=35;
    TMap<FString,float> ModelReadyAt[2],ModelCycle[2];
    bool OpeningReleasedV87[2]={false,false};
};
TMap<TWeakObjectPtr<AFrontlineTank>,FVehicle> Vehicles;
TMap<TWeakObjectPtr<UFrontlineVehicleSubsystem>,FFleet> Fleets;
struct FPreparedVehicleAssetsV82
{
    TStrongObjectPtr<USkeletalMesh> Mesh;
    TStrongObjectPtr<UAnimSequence> Drive;
    TStrongObjectPtr<UAnimSequence> Recoil,Service;
};
// Keep only requested catalogue assets alive for this encounter. The HUD may
// inspect an unused selected vehicle after the shared queue becomes ready;
// weak-only references otherwise cause a synchronous reload after each GC.
// Readiness, rig and shader checks still run against the actual live objects.
TMap<FString,FPreparedVehicleAssetsV82> PreparedVehicleAssetsV82;
TMap<FString,TStrongObjectPtr<UMaterialInterface>> AppearanceMaterialsV87;
UMaterialInterface* AppearanceMaterialV87(UMaterialInterface* Source)
{
    if(!Source)return nullptr;const FString Name=TEXT("M_NV_")+Source->GetName()+TEXT("_V87");
    if(const auto* Cached=AppearanceMaterialsV87.Find(Name))return Cached->Get();
    const FString URI=TEXT("/Game/FRRUIntegration/FormalV82/Materials/NationalArmourV87/")+Name+TEXT(".")+Name;
    auto* Material=LoadObject<UMaterialInterface>(nullptr,*URI,nullptr,LOAD_NoWarn);if(!Material)return nullptr;
    float Value=0;for(const TCHAR* Parameter:{TEXT("FieldDust"),TEXT("FieldRust"),TEXT("VehicleDamage"),TEXT("VehicleSeed")})
        if(!Material->GetScalarParameterValue(FMaterialParameterInfo(Parameter),Value))return nullptr;
    AppearanceMaterialsV87.Add(Name,TStrongObjectPtr<UMaterialInterface>(Material));return Material;
}
FFRRUArmourEffectiveSpecV87 DispatchSpecV87(const AWarfareFrontlineGameMode* B,int32 Side)
{
    FFRRUArmourEffectiveSpecV87 Out;if(!B||Side<0||Side>1||!WarfareFRRUCampaignV82::Active(B))return Out;
    const auto Nation=WarfareFRRUCampaignV82::Nation(B,Side);if(Nation!=EFRRUNationV81::French&&Nation!=EFRRUNationV81::Russian)return Out;
    const FString Prefix=Nation==EFRRUNationV81::French?TEXT("fr.armour."):TEXT("ru.armour.");const int32 Date=WarfareFRRUCampaignV82::CalendarDate(B);
    const auto Option=[&](const TCHAR* Suffix)->const wfexp::tactical::OptionDefinition*
    {
        const FString Id=Prefix+Suffix;const auto* O=wfexp::tactical::FindOption(wfexpue::Narrow(Id));
        if(!O||Date<O->fromDate||Date>O->toDate||!FMath::IsFinite(O->primaryEffect)||!FMath::IsFinite(O->secondaryEffect)||O->primaryEffect<=0||O->secondaryEffect<=0)return nullptr;
        // Ordinary enemies never inherit the player's national purchases.
        if(!WarfareFRRUCampaignV82::ArchiveFullArmoryV87(B,Side)&&(Side!=0||!WarfareFRRUCampaignV82::OwnsOption(B,Id)))return nullptr;
        return O;
    };
    if(const auto* O=Option(TEXT("crew")))
    {
        Out.Crew=true;if(Nation==EFRRUNationV81::French){Out.CannonCycleScale=float(O->primaryEffect);Out.CannonAimRateScale=float(O->secondaryEffect);}
        else{Out.MachinegunRecoveryScale=float(O->primaryEffect);Out.MachinegunAimRateScale=float(O->secondaryEffect);}
    }
    if(const auto* O=Option(TEXT("repair"))){Out.Repair=true;Out.RepairSeconds=12.f*float(O->primaryEffect);}
    if(const auto* O=Option(TEXT("prepare"))){Out.Prepare=true;Out.PreparationScale=float(O->primaryEffect);}
    return Out;
}
FFleet& FleetForEncounterV87(const AWarfareFrontlineGameMode* B,UFrontlineVehicleSubsystem* Fleet)
{
    auto& State=Fleets.FindOrAdd(Fleet);if(State.Encounter==B->GetEncounterId())return State;
    State=FFleet();State.Encounter=B->GetEncounterId();
    // Match the host's shorter opening preparation, scaled by each actual
    // model. ReadyAt is encounter-relative, never "first HUD query + delay".
    // Freeze national research once here; later dispatch freezes its own cycle.
    const float Reference=UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::MarkIVMale).Cooldown;
    const int32 Date=WarfareFRRUCampaignV82::CalendarDate(B);
    for(int32 Side=0;Side<2;++Side)
    {
        const auto Nation=WarfareFRRUCampaignV82::Nation(B,Side);const float PreparationScale=DispatchSpecV87(B,Side).PreparationScale;
        float Earliest=MAX_flt;
        // Registration does not grant deployment. Eligibility still checks the
        // mission, selection and real railway; late railway staging must not
        // accidentally obtain the fallback "already ready" time.
        for(const auto& Model:Definitions())if(Model.Nation==Nation&&Date>=Model.FirstDate&&Date<=Model.LastDate)
        {
            const float Duration=FrontlineGameplayV43::PreparationSeconds(50.f*(Model.Cooldown/Reference)*PreparationScale*FrontlinePacingV24::TempoIntervalScale,Side,B->GetCampaignDifficulty());
            State.ModelCycle[Side].Add(Model.Id,Duration);State.ModelReadyAt[Side].Add(Model.Id,Duration);Earliest=FMath::Min(Earliest,Duration);
        }
        if(Earliest<MAX_flt){State.Cycle[Side]=Earliest;State.ReadyAt[Side]=Earliest;}
    }
    return State;
}
void ApplyMechanicalOverlayV87(AFrontlineTank* V,FVehicle& R,bool Wreck,float DT)
{
    auto* Pose=R.Model.Get();if(!Pose)return;bool Changed=false;R.LastRecoilCm=0;
    const auto Apply=[&](const FMechanicalOverlayV87& O,UAnimSequence* Clip,double Time)
    {
        if(!Clip||O.SkeletonIndex==INDEX_NONE||!Clip->IsCompressedDataValid())return;
        FAnimExtractContext Context(FMath::Clamp(Time,0.,double(Clip->GetPlayLength())),false);
#if WITH_EDITOR
        Context.bEnforceCompressedDataSampling=true;
#endif
        FTransform Local;Clip->GetBoneTransform(Local,FSkeletonPoseBoneIndex(O.SkeletonIndex),Context,false);if(Local.ContainsNaN())return;
        const FTransform Parent=O.ParentBone.IsNone()?FTransform::Identity:Pose->GetBoneTransformByName(O.ParentBone,EBoneSpaces::ComponentSpace);
        const FVector Delta=Local.GetTranslation()-O.RestLocal.GetTranslation();FQuat Rotation=Local.GetRotation()*O.RestLocal.GetRotation().Inverse();Rotation.Normalize();
        FTransform Current=Pose->GetBoneTransformByName(O.Bone,EBoneSpaces::ComponentSpace);Current.AddToTranslation(Parent.TransformVectorNoScale(Delta));
        Current.SetRotation(Parent.GetRotation()*Rotation*Parent.GetRotation().Inverse()*Current.GetRotation());
        if(Current.ContainsNaN())return;Pose->SetBoneTransformByName(O.Bone,Current,EBoneSpaces::ComponentSpace);Changed=true;
        if(!Wreck)
        {
            R.LastRecoilCm=FMath::Max(R.LastRecoilCm,float(Delta.Size()));R.MaximumRecoilCm=FMath::Max(R.MaximumRecoilCm,R.LastRecoilCm);
            const float Angle=FMath::RadiansToDegrees(2.f*FMath::Acos(FMath::Clamp(float(FMath::Abs(Rotation.W)),0.f,1.f)));
            R.MaximumMechanismAngle=FMath::Max(R.MaximumMechanismAngle,Angle);
        }
    };
    if(Wreck)
    {
        if(R.WreckPoseApplied)return;
        if(auto* Clip=R.ServiceClip.Get())for(const auto& O:R.HatchBones)Apply(O,Clip,Clip->GetPlayLength()*.5);
        R.WreckPoseApplied=Changed;
    }
    else
    {
        if(auto* Clip=R.RecoilClip.Get())for(const auto& O:R.RecoilBones)
        {
            const auto& Ages=O.Cannon?R.CannonRecoilAge:R.GunRecoilAge;if(!Ages.IsValidIndex(O.WeaponIndex))continue;
            // Reuse the authored recoil/feed curve as a short impulse. Do not
            // replace, restart or seek the independent Drive144cm animation.
            const double Time=Clip->GetPlayLength()*.23+Ages[O.WeaponIndex]*3.;if(Time<Clip->GetPlayLength())Apply(O,Clip,Time);
        }
        for(float& Age:R.CannonRecoilAge)Age+=DT;for(float& Age:R.GunRecoilAge)Age+=DT;
        if(Changed)++R.RecoilFrames;
    }
    if(Changed)Pose->RefreshBoneTransforms();
}
struct FRail {FGuid Encounter;float Y=-2500;TWeakObjectPtr<AActor> Owner;TWeakObjectPtr<UInstancedStaticMeshComponent> Track;};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FRail> Railways;
float RailHeight(const AWarfareFrontlineGameMode* B,float X)
{
    const auto* R=Railways.Find(const_cast<AWarfareFrontlineGameMode*>(B));if(!R)return 0;
    double Height=0;const auto* G=wfexp::scene::ForMission(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(B)));
    if(G)wfexp::scene::HeightAt(*G,FMath::Clamp(double(X),G->minX,G->maxX),R->Y,Height);
    return float(Height)+17.2f;
}
FString URI(const FFRRUArmourDefinitionV82& D,const TCHAR* Kind,const TCHAR* Clip=nullptr)
{
    if(D.Rail&&!Clip)return TEXT("/Game/FRRUIntegration/FormalV82/Train/SK_AH_TroopTrain41_V82.SK_AH_TroopTrain41_V82");
    const FString Name=Clip?TEXT("A_R2_")+D.Token+TEXT("_AN_V2_")+D.Token+TEXT("_")+Clip:FString(Kind)+TEXT("_R2_")+D.Token;
    return TEXT("/Game/FRRUExpansionR2/FormalV82/Vehicles/")+D.Token+TEXT("/")+Name+TEXT(".")+Name;
}
UAnimSequence* TrainClip(const TCHAR* Semantic)
{
    static TMap<FString,TStrongObjectPtr<UAnimSequence>> Loaded;const FString Key(Semantic);
    if(const auto* Found=Loaded.Find(Key))return Found->Get();
    const FString Name=TEXT("A_AH_Train_")+Key+TEXT("67_V82");
    auto* A=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/FRRUIntegration/FormalV82/Motions/Train67Correction1/")+Name+TEXT(".")+Name));
    if(!A||A->GetPlayLength()<=0||!A->GetSkeleton()||!A->IsCompressedDataValid())return nullptr;
    Loaded.Add(Key,TStrongObjectPtr<UAnimSequence>(A));return A;
}
float CarCentre(int Car){return Car==0?-680.f:680.f;}
FVector SeatFeet(int Car,int Seat){const float Positions[]={-180.f,-65.f,190.f};return {CarCentre(Car)+Positions[FMath::Clamp(Seat,0,2)],38,93};}
FVector ExitFeet(int Car){return {CarCentre(Car)+85.f,-270.f,-19.f};}
bool Review()
{
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV13ManualReview")))
    {FString PersistenceUser;return WarfareFRRUCampaignV82::TryPersistenceReviewUserV82(PersistenceUser);}
#endif
    FString User;FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());FPaths::NormalizeDirectoryName(Project);
    if(!Project.Equals(FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation")),ESearchCase::IgnoreCase)
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82Review"))||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||FPaths::IsRelative(User))return false;
    User=FPaths::ConvertRelativePathToFull(User);FPaths::NormalizeDirectoryName(User);return User.StartsWith(Project+TEXT("/Saved/FRRU/"),ESearchCase::IgnoreCase)&&!User.Contains(TEXT(".."));
}
// The receiver's canonical FBX imports retain the mechanical bone labels;
// the V2 object prefix is not part of their actual reference skeleton.
FName Bone(const FFRRUArmourDefinitionV82&,const TCHAR* Name){return FName(Name);}
FTransform ReferenceFrame(const FReferenceSkeleton& Ref,int32 BoneIndex)
{
    FTransform T=Ref.GetRefBonePose()[BoneIndex];for(int32 Parent=Ref.GetParentIndex(BoneIndex);Parent!=INDEX_NONE;Parent=Ref.GetParentIndex(Parent))T=T*Ref.GetRefBonePose()[Parent];return T;
}
FString Message(const TCHAR* EN,const TCHAR* ZH)
{return FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?ZH:EN;}
const wfexp::scene::SceneDefinition* Scene(const AWarfareFrontlineGameMode* B)
{return B?wfexp::scene::ForMission(wfexpue::Narrow(WarfareFRRUCampaignV82::MissionId(B))):nullptr;}
FVehicle* Runtime(const AFrontlineTank* V){return Vehicles.Find(TWeakObjectPtr<AFrontlineTank>(const_cast<AFrontlineTank*>(V)));}
FVector NearHull(const AFrontlineTank* V,const FVector& P)
{
    const auto* D=Definition(V);if(!D)return P;const FVector L=V->GetActorTransform().InverseTransformPosition(P);
    const FVector N(FMath::Clamp(L.X,D->Minimum.X,D->Maximum.X),FMath::Clamp(L.Y,D->Minimum.Y,D->Maximum.Y),FMath::Clamp(L.Z,D->Minimum.Z,D->Maximum.Z));
    return V->GetActorTransform().TransformPosition(N);
}
bool Clear(const AFrontlineTank* V,const AActor* Target,const FVector& From,const FVector& To,const FCollisionQueryParams* Prepared=nullptr)
{
    const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;
    if(!B||B->DirectFireMultiplier(From,To)<.9f||!WarfareFRRUWorldV82::HasSight(B,From,To))return false;
    if(Prepared)return !V->GetWorld()->LineTraceTestByChannel(From,To,ECC_Visibility,*Prepared);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV82VehicleShot),true);Q.AddIgnoredActor(V);Q.AddIgnoredActor(Target);
    for(const auto& U:B->Units)if(U)Q.AddIgnoredActor(U);
    return !V->GetWorld()->LineTraceTestByChannel(From,To,ECC_Visibility,Q);
}
FRotator MountBearing(const AFrontlineTank* V,const FMount& Mount,const FVector& Point)
{return (V->GetActorTransform().InverseTransformPosition(Point)-Mount.Origin).Rotation();}
FWeaponContact SelectWeaponContact(AFrontlineTank* V,const FVehicle& R,
    int32 MountIndex,bool Cannon,const FCollisionQueryParams& InfantrySight,FMountReadback* Readback=nullptr)
{
    FWeaponContact Selected;const auto* B=R.Battle.Get();const auto& MountArray=Cannon?R.Cannons:R.Machineguns;
    if(Readback){*Readback={};Readback->Mount=MountIndex;Readback->Cannon=Cannon;}
    if(!B||!MountArray.IsValidIndex(MountIndex)||(!Cannon&&V->GetMachinegunIntegrity(MountIndex)<=0))return Selected;
    if(Readback)Readback->Enabled=true;
    const auto& Mount=MountArray[MountIndex];const FVector From=Muzzle(V,MountIndex,Cannon);
    const auto Consider=[&](AWarfareFrontlineSoldier* Infantry,AFrontlineTank* Armour,const FVector& Point)
    {
        AActor* Target=Armour?static_cast<AActor*>(Armour):static_cast<AActor*>(Infantry);
        const double Distance=FVector::Dist2D(V->GetActorLocation(),Point);
        if(Point.ContainsNaN()||!FMath::IsFinite(Distance)||Distance>=Selected.DistanceCm)return;
        const FRotator Bearing=MountBearing(V,Mount,Point);const double YawDelta=FMath::FindDeltaAngleDegrees(Mount.BaseYaw,float(Bearing.Yaw));
        const float Range=WeaponRangeV87(V,Cannon);
        const bool InRange=Distance<Range&&FVector::Dist2D(From,Point)<Range,InSector=FMath::Abs(YawDelta)<=Mount.Arc,InPitch=Bearing.Pitch>=-10&&Bearing.Pitch<=20;
        const bool CheckLOS=InRange&&InSector&&InPitch;
        const bool LOS=CheckLOS&&Clear(V,Target,From,Point,Infantry?&InfantrySight:nullptr);
        const bool Eligible=CheckLOS&&LOS;
        // Mirror the mature per-mount search: an ineligible nearest enemy
        // cannot monopolise this weapon or make the entire vehicle halt.
        const bool Choose=Eligible&&Distance<Selected.DistanceCm;
        if(Readback&&(Choose||(!Readback->Selected&&(Readback->DistanceCm<0||Distance<Readback->DistanceCm))))
        {
            Readback->Target=Target;Readback->Point=Point;Readback->Muzzle=From;Readback->DistanceCm=Distance;
            Readback->YawDelta=YawDelta;Readback->Pitch=Bearing.Pitch;Readback->WithinRange=InRange;
            Readback->WithinSector=InSector;Readback->WithinPitch=InPitch;Readback->FinalLOSChecked=CheckLOS;
            Readback->FinalLOSClear=LOS;Readback->Selected=Choose;
            Readback->TargetSquad=Infantry?Infantry->SquadId:INDEX_NONE;Readback->TargetMember=Infantry?Infantry->MemberIndex:INDEX_NONE;
            Readback->TargetSerial=Armour?Armour->Serial:INDEX_NONE;
        }
        if(Choose){Selected.Infantry=Infantry;Selected.Armour=Armour;Selected.Point=Point;Selected.DistanceCm=Distance;}
    };
    if(Cannon)if(const auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld()))
        for(AFrontlineTank* Other:Fleet->GetVehicles())if(Other&&Other!=V&&Other->Team!=V->Team&&Other->IsOperational()&&Other->HasEnteredField())
            Consider(nullptr,Other,Other->AimPoint());
    if(!Selected.Armour)for(AWarfareFrontlineSoldier* Unit:B->Units)
        if(Unit&&Unit->Team!=V->Team&&Unit->IsActiveCombatant())
        {
            // Use the same real torso/head alternatives as the mature mounted
            // gun. A parapet may hide the torso while leaving the head exposed.
            const FVector Points[]={Unit->Soldier?Unit->Soldier->GetBoneLocation(TEXT("spine_03")):Unit->Feet()+FVector(0,0,100),
                Unit->Soldier?Unit->Soldier->GetBoneLocation(TEXT("Head"))+FVector(0,0,5):Unit->Feet()+FVector(0,0,145)};
            for(const FVector& Point:Points){Consider(Unit,nullptr,Point);if(Selected.Infantry==Unit)break;}
        }
    return Selected;
}
void Mounts(FVehicle& R,const FFRRUArmourDefinitionV82& D)
{
    auto Cannon=[&](FVector P,const TCHAR* Yaw,const TCHAR* Pitch,float Base,float Arc){R.Cannons.Add({P,Base,Arc,Bone(D,Yaw),Bone(D,Pitch)});};
    auto Machinegun=[&](FVector P,float Base,float Arc,const TCHAR* Yaw=nullptr,const TCHAR* Pitch=nullptr){R.Machineguns.Add({P,Base,Arc,Yaw?Bone(D,Yaw):NAME_None,Pitch?Bone(D,Pitch):NAME_None});};
    if(D.Id==TEXT("fr.schneider")){Cannon({272,103,149},TEXT("short_75_right_yaw"),TEXT("short_75_right_pitch"),0,18);for(int S:{-1,1})Machinegun({-58,S*164.,148},S*90,45);}
    else if(D.Id==TEXT("fr.saint_chamond")){Cannon({522,0,149},TEXT("frontal_75_yaw"),TEXT("frontal_75_pitch"),0,12);for(int S:{-1,1})for(float X:{-210.f,174.f})Machinegun({X,S*186.,146},S*90,45);}
    else if(D.Id==TEXT("fr.ft"))Cannon({143,0,178},TEXT("turret_yaw"),TEXT("SA18_pitch"),0,180);
    else if(D.Id==TEXT("ru.austin_series1"))for(int S:{-1,1})
    {const FString Y=FString::Printf(TEXT("turret_%d_yaw"),S),P=FString::Printf(TEXT("Maxim_%d_pitch"),S);Machinegun({24,S*48.,216},S*35,145,*Y,*P);}
    else if(D.Rail)for(int S:{-1,1})for(int Car:{0,2})for(int I=0;I<3;++I)
        Machinegun({(Car==0?-849.:511.)+I*169.,S*143.,214},S*90,48);
    R.MachinegunYaw.SetNumZeroed(R.Machineguns.Num());R.CannonYaw.SetNumZeroed(R.Cannons.Num());
    for(auto* List:{&R.Cannons,&R.Machineguns})for(auto& Mount:*List)if(Mount.PitchBone!=NAME_None)
        Mount.MuzzleBone=FName(*Mount.PitchBone.ToString().Replace(TEXT("_pitch"),TEXT("_muzzle")));
}
bool Occupy(AFrontlineTank* V,const FVector& P,float Yaw)
{
    const auto* D=Definition(V);const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;const auto* G=Scene(B);if(!D||!B||!G||P.ContainsNaN())return false;
    const FTransform T(FRotator(0,Yaw,0),P);
    const float CX=float((D->Minimum.X+D->Maximum.X)*.5),CY=float((D->Minimum.Y+D->Maximum.Y)*.5);
    const float HX=float((D->Maximum.X-D->Minimum.X)*.5),HY=float((D->Maximum.Y-D->Minimum.Y)*.5);
    float Low=MAX_flt,High=-MAX_flt;
    for(float X:{float(D->Minimum.X),CX,float(D->Maximum.X)})for(float Y:{float(D->Minimum.Y),CY,float(D->Maximum.Y)})
    {
        const FVector F=T.TransformPosition({X,Y,0});float Z=0;
        // Offstage entry is a bounded apron, not a teleport or second height map.
        if(D->Rail)Z=RailHeight(B,float(F.X));
        else if(!WarfareFRRUWorldV82::TryHeight(B,float(F.X),float(F.Y),Z))return false;
        if(!D->Rail&&FMath::Abs(F.X)<=12000&&FMath::Abs(F.Y)<=6000)
        {
            const auto Contact=wfexp::scene::FootingAt(*G,F.X,F.Y);if(!Contact.valid||!Contact.walkable)return false;
        }
        Low=FMath::Min(Low,Z);High=FMath::Max(High,Z);
    }
    if(High-Low>(D->Wheeled?50.f:D->Rail?18.f:125.f))return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV82VehicleOccupancy),false);Q.AddIgnoredActor(V);
    for(const auto& U:B->Units)if(U)Q.AddIgnoredActor(U);
    return !V->GetWorld()->OverlapBlockingTestByChannel(T.TransformPosition({CX,CY,High-P.Z+100}),FRotator(0,Yaw,0).Quaternion(),ECC_WorldStatic,
        FCollisionShape::MakeBox(FVector(HX+12,HY+12,80)),Q);
}
#include "FTGroundSupportV82.inl"
#include "FTGroundPilotV82.inl"
UBoxComponent* TrainBox(AFrontlineTank* V,const TCHAR* Name,const FVector& Centre,const FVector& Size)
{
    auto* Box=NewObject<UBoxComponent>(V,FName(Name));V->AddInstanceComponent(Box);Box->SetupAttachment(V->GetRootComponent());
    Box->SetRelativeLocation(Centre);Box->SetBoxExtent(Size*.5);Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();return Box;
}
bool ConfigureCarriages(AFrontlineTank* V,FVehicle& R)
{
    // The imported visual FBX has source Y reflected. Undo that once on the
    // visible components; all authored seat/door/collision coordinates remain
    // in source centimetres, including the -102 degree opening direction.
    if(!R.Model.IsValid()||!R.Hull.IsValid())return false;
    R.Model->SetRelativeScale3D(FVector(1,-1,1));R.Hull->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Ignore);
    for(const auto& Row:FRRUTrainGeometryV82::Boxes)R.CarriageBoxes.Add(TrainBox(V,Row.Id,Row.Centre,Row.Size));
    for(int Index=0;Index<2;++Index)
    {
        const int Car=Index?2:0;const float Centre=CarCentre(Car);
        R.CarriageBoxes.Add(TrainBox(V,*FString::Printf(TEXT("TroopCar%dDeck"),Car),FVector(Centre,0,85),FVector(606,227,16)));
        const FString Name=FString::Printf(TEXT("SM_AH_TroopDoor%d_V82"),Car);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/FRRUIntegration/FormalV82/Train/")+Name+TEXT(".")+Name));
        if(!Mesh||!Mesh->GetRenderData())return false;
        auto* Door=NewObject<UStaticMeshComponent>(V,FName(*FString::Printf(TEXT("TroopCar%dDoor"),Car)));V->AddInstanceComponent(Door);
        Door->SetupAttachment(V->GetRootComponent());Door->SetStaticMesh(Mesh);Door->SetRelativeLocation(FVector(Centre+43,-114,93));
        Door->SetRelativeScale3D(FVector(1,-1,1));Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);Door->SetCastShadow(true);Door->RegisterComponent();R.Doors[Index]=Door;
        R.DoorBoxes[Index]=TrainBox(V,*FString::Printf(TEXT("TroopCar%dDoorBody"),Car),FVector(Centre+85,-114,188),FVector(84,3,190));
    }
    return R.CarriageBoxes.Num()==UE_ARRAY_COUNT(FRRUTrainGeometryV82::Boxes)+2;
}
void DoorPose(FVehicle& R,int Index,float Open)
{
    R.DoorOpen[Index]=FMath::Clamp(Open,0.f,1.f);const int Car=Index?2:0;
    const FRotator Rotation(0,-102.f*R.DoorOpen[Index],0);const FVector Hinge(CarCentre(Car)+43,-114,93);
    if(auto* Door=R.Doors[Index].Get()){Door->SetRelativeLocation(Hinge);Door->SetRelativeRotation(Rotation);}
    if(auto* Box=R.DoorBoxes[Index].Get()){Box->SetRelativeLocation(Hinge+Rotation.RotateVector(FVector(42,0,95)));Box->SetRelativeRotation(Rotation);}
}
bool PassengerContact(const AFrontlineTank* V,const FVector& LocalFeet,FVector& WorldFeet,FVector& Centre)
{
    const auto* R=Runtime(V);if(!R||!R->Battle.IsValid()||LocalFeet.ContainsNaN())return false;
    WorldFeet=V->GetActorTransform().TransformPosition(LocalFeet);Centre=WorldFeet+FVector(0,0,90);
    FVector GroundFeet,GroundCentre;
    if(WarfareFRRUWorldV82::GroundContact(R->Battle.Get(),WorldFeet,GroundFeet,GroundCentre))Centre.Z=FMath::Max(Centre.Z,GroundCentre.Z);
    else
    {
        // The rear carriage may still be outside the infantry ingress apron.
        // Its real deck supports a seated actor; no ground, objective or route
        // is created outside the native scene. Walking begins only at the
        // actual in-scene stop, where the normal ground query must succeed.
        const auto* G=Scene(R->Battle.Get());bool Deck=false;
        for(int Car:{0,2})Deck|=FMath::Abs(LocalFeet.X-CarCentre(Car))<=303&&FMath::Abs(LocalFeet.Y)<=113.5&&FMath::Abs(LocalFeet.Z-93)<.01;
        if(!G||!Deck||(WorldFeet.X>=G->minX&&WorldFeet.X<=G->maxX))return false;
    }
    // Exact lower-sphere support on the actual horizontal deck and three
    // treads. Wall/roof boxes are never treated as walking surfaces. The
    // standing capsule remains 25x85 with the same 5 cm skin.
    for(int Car:{0,2})
    {
        const FVector C(CarCentre(Car),0,93);const FVector2D Half(303,113.5);
        auto Support=[&](const FVector& Top,const FVector2D& Extent)
        {
            const double DX=FMath::Max(0.,FMath::Abs(LocalFeet.X-Top.X)-Extent.X),DY=FMath::Max(0.,FMath::Abs(LocalFeet.Y-Top.Y)-Extent.Y);
            const double Distance2=DX*DX+DY*DY;if(Distance2>625.)return;
            const FVector Surface=V->GetActorTransform().TransformPosition(Top);
            Centre.Z=FMath::Max(Centre.Z,Surface.Z+FMath::Sqrt(FMath::Max(0.,625.-Distance2))+65.);
        };
        Support(C,Half);
        for(int Step=0;Step<3;++Step)Support(FVector(CarCentre(Car)+85,-137.-34.*Step,65.-28.*Step),FVector2D(42,19));
    }
    return !WorldFeet.ContainsNaN()&&!Centre.ContainsNaN();
}
bool PlacePassenger(AFrontlineTank* V,FPassenger& P,const FVector& LocalFeet,bool Sweep)
{
    auto* U=P.Actor.Get();const auto* R=Runtime(V);auto* B=R?R->Battle.Get():nullptr;
    if(!U||!B||!U->Capsule||!U->Soldier)return false;FVector Feet,Centre;
    if(!PassengerContact(V,LocalFeet,Feet,Centre))return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV82CarriageContact),true);for(const auto& Other:B->Units)if(Other)Q.AddIgnoredActor(Other);
    if(Sweep)
    {
        const FVector OldLocal=V->GetActorTransform().InverseTransformPosition(U->Feet());const double Distance=FVector::Dist(OldLocal,LocalFeet);
        const int Steps=FMath::Max(1,FMath::CeilToInt(Distance/2.));FVector Previous=U->GetActorLocation();
        for(int I=1;I<=Steps;++I)
        {
            const FVector Sample=FMath::Lerp(OldLocal,LocalFeet,double(I)/Steps);FVector SampleFeet,SampleCentre;
            if(!PassengerContact(V,Sample,SampleFeet,SampleCentre)||V->GetWorld()->SweepTestByChannel(Previous,SampleCentre,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),Q))return false;
            for(const auto& Other:B->Units)if(Other&&Other!=U&&Other->IsAlive()&&(Other->bFieldEntered||Embarked(Other)))
            {
                const double Initial=FVector::Dist2D(U->Feet(),Other->Feet()),Gap=FVector::Dist2D(SampleFeet,Other->Feet());
                if(Gap<56.5&&Gap<Initial-.01)return false;
            }
            Previous=SampleCentre;
        }
    }
    else if(V->GetWorld()->OverlapBlockingTestByChannel(Centre,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),Q))return false;
    const double Offset=Centre.Z-Feet.Z;const float BeforeOffset=U->GroundContactOffsetV82;
    const FVector Old=U->Feet();U->GroundContactOffsetV82=float(Offset);U->Soldier->SetRelativeLocation(FVector(0,0,-Offset));
    if(!U->SetActorLocation(Centre)){U->GroundContactOffsetV82=BeforeOffset;U->Soldier->SetRelativeLocation(FVector(0,0,-BeforeOffset));return false;}
    if(Sweep)U->DistanceTravelled+=float(FVector::Dist2D(Old,Feet));P.State.LocalFeet=LocalFeet;P.State.WorldFeet=Feet;return true;
}
void PassengerStage(FPassenger& P,EFRRUTrainPassengerStageV82 Stage,float HostElapsed)
{
    P.State.Stage=Stage;P.State.StageStartedAt=HostElapsed;P.State.Progress=0;P.PhaseSeconds=0;P.From=P.State.LocalFeet;P.To=P.From;
}
bool PassengerPose(FPassenger& P,const TCHAR* Semantic,float Phase,FString& Error)
{
    auto* A=TrainClip(Semantic);if(!A){Error=TEXT("The corrected train passenger action is missing.");return false;}
    return WarfareFRRUPresentationV82::SeekTrainPassenger(P.Actor.Get(),A,FMath::Clamp(Phase,0.f,1.f),Error);
}
bool WalkPassenger(AFrontlineTank* V,FPassenger& P,const FVector& Goal,float Seconds,FString& Error)
{
    const FVector Delta=Goal-P.State.LocalFeet;const double Remaining=Delta.Size2D();const double Travel=FMath::Min(Remaining,105.*Seconds);
    P.State.Facing=V->GetActorTransform().TransformVectorNoScale(Delta.GetSafeNormal2D());
    if(Remaining<.1)return true;
    const FVector Next=P.State.LocalFeet+Delta*(Travel/FMath::Max(.001,Remaining));
    if(!PlacePassenger(V,P,Next,true))return false;
    P.WalkDistance+=float(Travel);P.State.Progress=FMath::Fmod(P.WalkDistance/180.f,1.f);
    PassengerPose(P,TEXT("CarryWalk"),P.State.Progress,Error);return Remaining-Travel<.1;
}
bool UpdatePassengers(AFrontlineTank* V,float Seconds)
{
    auto* R=Runtime(V);auto* B=R?R->Battle.Get():nullptr;if(!R||!B)return false;
    const bool Parked=!R->RailReturning&&FMath::Abs(V->GetActorLocation().X-7150)<.1f&&V->GetHullIntegrity()>0;
    bool AllCleared=!R->Passengers.IsEmpty();FString Error;
    for(int Index=0;Index<2;++Index)
    {
        const int Car=Index?2:0;FPassenger* Active=nullptr;bool CarCleared=true;
        for(auto& P:R->Passengers)if(P.State.Car==Car&&P.Actor.IsValid()&&P.Actor->IsAlive()&&P.State.Stage!=EFRRUTrainPassengerStageV82::Field&&P.State.Stage!=EFRRUTrainPassengerStageV82::Lost)
        {CarCleared=false;if(!Active||P.State.Seat<Active->State.Seat)Active=&P;}
        AllCleared&=CarCleared;DoorPose(*R,Index,FMath::FInterpConstantTo(R->DoorOpen[Index],Parked&&!CarCleared?1.f:0.f,Seconds,.9f));
        for(auto& P:R->Passengers)if(P.State.Car==Car&&P.Actor.IsValid()&&P.Actor->IsAlive())
        {
            if(P.State.Stage==EFRRUTrainPassengerStageV82::Field||P.State.Stage==EFRRUTrainPassengerStageV82::Lost)continue;
            if(P.State.Stage==EFRRUTrainPassengerStageV82::Seated)
            {
                P.State.Facing=V->GetActorTransform().TransformVectorNoScale(FVector(0,-1,0));
                if(!PlacePassenger(V,P,SeatFeet(Car,P.State.Seat),false)){Error=TEXT("The actual seated capsule is obstructed.");break;}
                PassengerPose(P,TEXT("Seated"),float(FMath::Fmod(B->Elapsed+P.State.MemberIndex*.07,1.6)/1.6),Error);
                if(Parked&&Active==&P&&R->DoorOpen[Index]>=.999f)PassengerStage(P,EFRRUTrainPassengerStageV82::Standing,B->Elapsed);
                continue;
            }
            if(!Parked||Active!=&P)continue;
            if(P.State.Stage==EFRRUTrainPassengerStageV82::Standing)
            {
                P.PhaseSeconds=FMath::Min(1.6f,P.PhaseSeconds+Seconds);P.State.Progress=P.PhaseSeconds/1.6f;
                PassengerPose(P,TEXT("Stand"),P.State.Progress,Error);
                if(P.State.Progress>=1){PassengerStage(P,EFRRUTrainPassengerStageV82::Aisle,B->Elapsed);P.AisleLeg=0;}
            }
            else if(P.State.Stage==EFRRUTrainPassengerStageV82::Aisle)
            {
                FVector Goal=P.AisleLeg==0?FVector(SeatFeet(Car,P.State.Seat).X,-35,93):FVector(CarCentre(Car)+85,-35,93);
                if(WalkPassenger(V,P,Goal,Seconds,Error))
                {if(P.AisleLeg++==1)PassengerStage(P,EFRRUTrainPassengerStageV82::Queue,B->Elapsed);}
            }
            else if(P.State.Stage==EFRRUTrainPassengerStageV82::Queue)
            {
                if(WalkPassenger(V,P,FVector(CarCentre(Car)+85,0,93),Seconds,Error))PassengerStage(P,EFRRUTrainPassengerStageV82::Sill,B->Elapsed);
            }
            else if(P.State.Stage==EFRRUTrainPassengerStageV82::Sill)
            {
                if(WalkPassenger(V,P,FVector(CarCentre(Car)+85,-111,93),Seconds,Error))PassengerStage(P,EFRRUTrainPassengerStageV82::Steps,B->Elapsed);
            }
            else if(P.State.Stage==EFRRUTrainPassengerStageV82::Steps)
            {
                const float Next=FMath::Min(5.f,P.PhaseSeconds+Seconds),Scaled=Next;const int Leg=FMath::Min(4,FMath::FloorToInt(Scaled));
                const FVector Markers[]={FVector(0,0,0),FVector(0,-26,-28),FVector(0,-60,-56),FVector(0,-94,-84),FVector(0,-159,-112),FVector(0,-159,-112)};
                float T=FMath::Clamp(Scaled-Leg,0.f,1.f);T=T*T*(3.f-2.f*T);const FVector Feet=P.From+FMath::Lerp(Markers[Leg],Markers[Leg+1],T);
                P.State.Facing=V->GetActorTransform().TransformVectorNoScale(FVector(0,-1,0));
                if(PlacePassenger(V,P,Feet,true)){P.PhaseSeconds=Next;P.State.Progress=Next/5.f;PassengerPose(P,TEXT("Stairs"),P.State.Progress,Error);}
                if(P.PhaseSeconds>=5)PassengerStage(P,EFRRUTrainPassengerStageV82::GroundClear,B->Elapsed);
            }
            else if(P.State.Stage==EFRRUTrainPassengerStageV82::GroundClear)
            {
                // The reauthored final foot is 46 cm beyond the last tread.
                // Only a real clear capsule and a real reserved field route
                // release this SAME actor into ordinary combat.
                if(!PlacePassenger(V,P,ExitFeet(Car),true))continue;
                if(!R->DismountRegistered)R->DismountRegistered=WarfareFRRUWorldV82::Disembarked(B,P.State.SquadId,P.Actor.Get());
                if(!R->DismountRegistered)continue;
                auto* U=P.Actor.Get();U->Capsule->SetCollisionEnabled(P.FieldCollision);U->bFieldEntered=true;
                PassengerStage(P,EFRRUTrainPassengerStageV82::Field,B->Elapsed);WarfareFRRUPresentationV82::EndTrainPassengerPose(U);
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_TRAIN_DISMOUNT serial=%d squad=%d member=%d feet=%s"),V->Serial,U->SquadId,U->MemberIndex,*U->Feet().ToString());
            }
            if(!Error.IsEmpty())break;
        }
        if(!Error.IsEmpty())break;
    }
    if(!Error.IsEmpty())UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_TRAIN_PASSENGER serial=%d %s"),V->Serial,*Error);
    R->CrewUnloaded=AllCleared;return AllCleared&&R->DoorOpen[0]<.001f&&R->DoorOpen[1]<.001f;
}
}
void ClearRailway(AWarfareFrontlineGameMode* B)
{
    if(!B)return;if(const auto* R=Railways.Find(B))if(auto* Owner=R->Owner.Get())Owner->Destroy();Railways.Remove(B);
}
bool StageRailway(AWarfareFrontlineGameMode* B,FString& Error)
{
    ClearRailway(B);if(!B||WarfareFRRUCampaignV82::MissionId(B)!=TEXT("ru.czernowitz")){Error.Empty();return true;}
    const auto* G=Scene(B);if(!G||WarfareFRRUCampaignV82::Nation(B,1)!=EFRRUNationV81::AustroHungarian||WarfareFRRUCampaignV82::CalendarDate(B)<19150101)return false;
    // A fictional local railway attachment, not a historical train-unit claim.
    FRail Draft;Draft.Encounter=B->GetEncounterId();
    // This authored family contains a gently graded railway corridor. Other
    // scene revisions retain their existing alignment and contact checks.
    const bool FamilyRail=wfexpue::Wide(G->id)==TEXT("ru.czernowitz.scene.families.v1")&&G->revision==1;
    if(FamilyRail)Draft.Y=5375;
    const float RailHalfWidth=FamilyRail?171.504f:170.f;
    float Low=MAX_flt,High=-MAX_flt;
    for(float X=6000;X<=G->maxX;X+=25)for(float DY:{-RailHalfWidth,0.f,RailHalfWidth})
    {
        const auto Foot=wfexp::scene::FootingAt(*G,X,Draft.Y+DY);if(!Foot.valid||!Foot.walkable){Error=TEXT("The railway corridor crosses blocked ground.");return false;}
        Low=FMath::Min(Low,float(Foot.heightCm));High=FMath::Max(High,float(Foot.heightCm));
    }
    if(High-Low>18){Error=TEXT("The railway foundation needs an engineered alignment.");return false;}
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Warfare/Field/V40/Meshes/SM_Rail_A_V40.SM_Rail_A_V40"));
    if(!Mesh||!Mesh->GetRenderData()||!Mesh->GetBodySetup()){Error=TEXT("The running rail geometry is unavailable.");return false;}
    auto* Owner=B->GetWorld()->SpawnActor<AActor>();if(!Owner)return false;Owner->Tags.Add(TEXT("FRRU_V82Scene"));Owner->Tags.Add(TEXT("FRRU_V82RunningRailway"));
    auto* Track=NewObject<UInstancedStaticMeshComponent>(Owner,TEXT("BukovinaRunningRailway"));Owner->AddInstanceComponent(Track);Owner->SetRootComponent(Track);Track->SetStaticMesh(Mesh);
    Track->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Track->SetCollisionResponseToAllChannels(ECR_Ignore);Track->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Track->SetCastShadow(true);Track->SetCanEverAffectNavigation(false);Track->RegisterComponent();
    for(float X=6250;X<(FamilyRail?15000.f:14000.f);X+=500)
    {
        double A=0,Z=0;wfexp::scene::HeightAt(*G,FMath::Clamp(double(X-250),G->minX,G->maxX),Draft.Y,A);wfexp::scene::HeightAt(*G,FMath::Clamp(double(X+250),G->minX,G->maxX),Draft.Y,Z);
        Track->AddInstance(FTransform(FRotator(FMath::RadiansToDegrees(FMath::Atan2(Z-A,500.)),0,0),FVector(X,Draft.Y,(A+Z)*.5)));
    }
    Draft.Owner=Owner;Draft.Track=Track;Railways.Add(B,MoveTemp(Draft));Error.Empty();return true;
}
bool WalkSurface(const AWarfareFrontlineGameMode* B,float X,float Y,float NativeHeight,float& Height)
{
    const auto* R=Railways.Find(const_cast<AWarfareFrontlineGameMode*>(B));if(!R||!R->Track.IsValid()||X<6000||X>12000||FMath::Abs(Y-R->Y)>175)return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FRRUV82RailwayFeet),true);FHitResult Hit;
    if(!R->Track->LineTraceComponent(Hit,FVector(X,Y,NativeHeight+40),FVector(X,Y,NativeHeight-25),Q))return false;
    if(Hit.ImpactPoint.Z<NativeHeight||Hit.ImpactPoint.Z>NativeHeight+24)return false;
    Height=float(Hit.ImpactPoint.Z);return true;
}
const TArray<FFRRUArmourDefinitionV82>& Definitions()
{
    // Balance profiles are receiver design, not invented specifications in the source catalogue.
    static const TArray<FFRRUArmourDefinitionV82> D={
        {TEXT("fr.schneider"),TEXT("fr_schneider"),TEXT("Schneider CA1"),TEXT("A short-track assault tank. Its offset 75 mm gun breaks field positions; flank machineguns protect the approach."),EFRRUNationV81::French,19170416,19181231,105,155,330,6300,430,128,{-286.5,-164,0},{318.5,164,238}},
        {TEXT("fr.saint_chamond"),TEXT("fr_saint_chamond"),TEXT("Saint-Chamond"),TEXT("A long hull carrying a frontal 75 mm gun. Strong fire support on firm ground; narrow cuts and steep crater lips restrict its movement."),EFRRUNationV81::French,19170505,19181231,128,175,360,7300,485,150,{-398.5,-186,0},{522.15,186,283.5}},
        {TEXT("fr.ft"),TEXT("fr_ft"),TEXT("Renault FT · 37 mm"),TEXT("A compact rotating turret carries the SA18 gun. It follows infantry through tighter ground; light armour cannot endure a sustained gun duel."),EFRRUNationV81::French,19180531,19181231,78,200,230,5600,300,83,{-304,-87.4042,0},{195,87.4042,212}},
        {TEXT("ru.austin_series1"),TEXT("ru_austin_series1"),TEXT("Austin · First Series"),TEXT("Two Maxim turrets cover separate sectors. Fast on firm routes and effective against infantry; wheels and thin armour remain vulnerable to field guns."),EFRRUNationV81::Russian,19150501,19171231,78,300,190,5200,0,0,{-212.5,-123.5,0},{239,123.5,249},true},
        {TEXT("at-hu.armoured_train"),TEXT("at_hu_armoured_train"),TEXT("Armoured Railway Detachment"),TEXT("A three-car armoured detachment follows the railway. Side ports cover its dismounted infantry; losing the train shakes the whole position."),EFRRUNationV81::AustroHungarian,19150101,19181231,170,240,660,5700,0,0,{-1019,-143,-3},{1019,143,353},false,true}};
    return D;
}
const FFRRUArmourDefinitionV82* Definition(const FString& StableId){for(const auto& D:Definitions())if(D.Id==StableId)return &D;return nullptr;}
const FFRRUArmourDefinitionV82* Definition(const AFrontlineTank* V){const auto* R=Runtime(V);return R?Definition(R->StableId):nullptr;}
FString Id(const AFrontlineTank* V){const auto* R=Runtime(V);return R?R->StableId:FString();}
bool National(const AFrontlineTank* V){return Runtime(V)!=nullptr;}
float WeaponRangeV87(const AFrontlineTank* V,bool Cannon)
{
    const auto* D=Definition(V);const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;if(!D||!B)return 0;
    // Preserve each national cannon's authored range. Mounted machineguns use
    // the current shared 51 m cap and 0.9 range factor before weather, rather
    // than borrowing a 75 mm gun's reach or the Type::Count fallback chassis.
    return (Cannon?D->Range:FMath::Min(5100.f,D->Range)*.9f)*B->EncounterVisibilityScale();
}
static bool AssignedToOperationV827(const FFRRUArmourDefinitionV82& D,const FString& Mission)
{
    if(D.Rail)return Mission==TEXT("ru.czernowitz");
    if(D.Wheeled)return Mission==TEXT("ru.warsaw_rearguard")||Mission==TEXT("ru.riga");
    if(D.Id==TEXT("fr.schneider"))return Mission==TEXT("fr.chemin_dames")||Mission==TEXT("fr.malmaison")||Mission==TEXT("fr.matz")||Mission==TEXT("fr.soissons")||Mission==TEXT("fr.sommepy");
    if(D.Id==TEXT("fr.saint_chamond"))return Mission==TEXT("fr.malmaison")||Mission==TEXT("fr.matz")||Mission==TEXT("fr.soissons")||Mission==TEXT("fr.sommepy");
    if(D.Id==TEXT("fr.ft"))return Mission==TEXT("fr.matz")||Mission==TEXT("fr.soissons")||Mission==TEXT("fr.sommepy");
    return false;
}
int32 FirstMissionForArmour(const AWarfareFrontlineGameMode* B,const FString& StableId)
{
    const auto* D=Definition(StableId);
    if(!B||!D||!WarfareFRRUCampaignV82::Active(B)||WarfareFRRUCampaignV82::Nation(B)!=D->Nation)return MAX_int32;
    int32 First=MAX_int32;
    for(const auto& M:wfexp::Missions())
        if(static_cast<uint8>(M.faction)==static_cast<uint8>(D->Nation)&&M.date>=D->FirstDate&&M.date<=D->LastDate
            &&AssignedToOperationV827(*D,wfexpue::Wide(M.id)))First=FMath::Min(First,M.order);
    return First;
}
bool Eligible(const AWarfareFrontlineGameMode* B,const FString& StableId,int32 Side)
{
    const auto* D=Definition(StableId);if(!B||!D||Side<0||Side>1||!WarfareFRRUCampaignV82::Active(B)
        ||WarfareFRRUCampaignV82::Nation(B,Side)!=D->Nation)return false;
    const int32 Date=WarfareFRRUCampaignV82::CalendarDate(B);if(Date<D->FirstDate||Date>D->LastDate)return false;
    if(!WarfareFRRUCampaignV82::ArchiveFullArmoryV87(B,Side)&&!AssignedToOperationV827(*D,WarfareFRRUCampaignV82::MissionId(B)))return false;
    const auto* G=Scene(B);if(D->Rail)
    {const auto* Rail=Railways.Find(const_cast<AWarfareFrontlineGameMode*>(B));return WarfareFRRUCampaignV82::MissionId(B)==TEXT("ru.czernowitz")&&Rail&&Rail->Owner.IsValid()&&Rail->Track.IsValid()&&Rail->Encounter==B->GetEncounterId();}
    // Scene-owned road qualification prevents a wheeled car appearing on an impassable marsh.
    if(D->Wheeled)return G!=nullptr;
    return true;
}
bool AssetsReady(const FString& StableId,FString& Error)
{
    const auto* D=Definition(StableId);if(!D){Error=TEXT("Unknown vehicle");return false;}
    auto& Assets=PreparedVehicleAssetsV82.FindOrAdd(StableId);
    if(!Assets.Mesh.IsValid())Assets.Mesh.Reset(LoadObject<USkeletalMesh>(nullptr,*URI(*D,TEXT("SK"))));
    if(!Assets.Drive.IsValid())Assets.Drive.Reset(LoadObject<UAnimSequence>(nullptr,*URI(*D,TEXT(""),TEXT("Drive144cm"))));
    if(!D->Rail&&!Assets.Recoil.IsValid())Assets.Recoil.Reset(LoadObject<UAnimSequence>(nullptr,*URI(*D,TEXT(""),TEXT("Recoil"))));
    if(!Assets.Service.IsValid())Assets.Service.Reset(LoadObject<UAnimSequence>(nullptr,*URI(*D,TEXT(""),TEXT("ServiceHatches"))));
    auto* M=Assets.Mesh.Get();auto* A=Assets.Drive.Get();
#if WITH_EDITOR
    // A shared mechanical Skeleton can invalidate an earlier Editor DDC key.
    // Finish the current-platform compressed result; never evaluate raw data
    // as a substitute. Cook supplies platform compression for Shipping.
    if(A&&!A->IsCompressedDataValid()){A->CacheDerivedDataForCurrentPlatform();A->WaitOnExistingCompression(true);}
#endif
    if(!M||!M->GetResourceForRendering()||!M->GetSkeleton()||!A||A->GetSkeleton()!=M->GetSkeleton()||A->GetPlayLength()<=0||!A->IsCompressedDataValid())
    {
        UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_V82_MECHANICAL_ASSET id=%s mesh=%s render=%d skeleton=%s drive=%s driveSkeleton=%s length=%.3f compressed=%d"),
            *StableId,*GetPathNameSafe(M),int(M&&M->GetResourceForRendering()),*GetPathNameSafe(M?M->GetSkeleton():nullptr),
            *GetPathNameSafe(A),*GetPathNameSafe(A?A->GetSkeleton():nullptr),A?A->GetPlayLength():0.f,int(A&&A->IsCompressedDataValid()));
        Error=Message(TEXT("The vehicle has not completed its workshop checks."),TEXT("这辆战车尚未完成工场检修。"));return false;
    }
    const auto& Ref=M->GetRefSkeleton();const int32 Root=Ref.FindBoneIndex(Bone(*D,TEXT("root")));if(Root!=0){Error=TEXT("Vehicle root frame does not match");return false;}
    FVehicle Probe;Mounts(Probe,*D);for(const auto& Mount:Probe.Cannons)if(Ref.FindBoneIndex(Mount.YawBone)==INDEX_NONE||Ref.FindBoneIndex(Mount.PitchBone)==INDEX_NONE||Ref.FindBoneIndex(Mount.MuzzleBone)==INDEX_NONE){Error=TEXT("Cannon rig is incomplete");return false;}
    for(const auto& Mount:Probe.Machineguns)if(Mount.YawBone!=NAME_None&&(Ref.FindBoneIndex(Mount.YawBone)==INDEX_NONE||Ref.FindBoneIndex(Mount.PitchBone)==INDEX_NONE||Ref.FindBoneIndex(Mount.MuzzleBone)==INDEX_NONE)){Error=TEXT("Machinegun rig is incomplete");return false;}
    if(M->GetMaterials().IsEmpty()){Error=TEXT("Vehicle surfaces are absent");return false;}
    for(auto* Clip:{Assets.Recoil.Get(),Assets.Service.Get()})if(Clip)
    {
#if WITH_EDITOR
        if(!Clip->IsCompressedDataValid()){Clip->CacheDerivedDataForCurrentPlatform();Clip->WaitOnExistingCompression(true);}
#endif
        if(Clip->GetSkeleton()!=M->GetSkeleton()||!Clip->IsCompressedDataValid()){Error=TEXT("Mechanical overlay compression/skeleton mismatch");return false;}
    }
    if((!D->Rail&&!Assets.Recoil.IsValid())||!Assets.Service.IsValid()){Error=TEXT("Mechanical recoil/service overlay absent");return false;}
    for(const auto& Slot:M->GetMaterials())if(!Slot.MaterialInterface||!Slot.MaterialInterface->GetMaterialResource(GMaxRHIShaderPlatform)||!Slot.MaterialInterface->GetMaterialResource(GMaxRHIShaderPlatform)->GetGameThreadShaderMap())
    {Error=TEXT("Vehicle surface shaders are not ready");return false;}
    for(const auto& Slot:M->GetMaterials())if(auto* Appearance=AppearanceMaterialV87(Slot.MaterialInterface))
        if(!Appearance->GetMaterialResource(GMaxRHIShaderPlatform)||!Appearance->GetMaterialResource(GMaxRHIShaderPlatform)->GetGameThreadShaderMap())
        {Error=TEXT("National vehicle weathering shaders are not ready");return false;}
    Error.Empty();return true;
}
bool Accepted(const AWarfareFrontlineGameMode* B,const FString& StableId){return B&&Definition(StableId)&&(FormalFiveVehicleAcceptance||Review());}
FString BlockReason(const AWarfareFrontlineGameMode* B,const FString& StableId,int32 Side)
{
    if(!Eligible(B,StableId,Side))return Message(TEXT("This vehicle is not assigned to this operation."),TEXT("本次行动未配属这辆战车。"));
    if(Side==0&&!WarfareFRRUCampaignV82::ArmourSelection(B).Contains(StableId))return Message(TEXT("This vehicle is not in the selected armour column."),TEXT("\u672c\u6b21\u88c5\u7532\u7eb5\u961f\u672a\u9009\u7528\u8fd9\u8f86\u6218\u8f66\u3002"));
    if(!Accepted(B,StableId))return Message(TEXT("The vehicle is awaiting its field trial."),TEXT("这辆战车正在等待前线试车。"));
    if(B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return Message(TEXT("Armour moves on the battle order."),TEXT("战车将在开战命令下达后出发。"));
    if(UGameplayStatics::IsGamePaused(B))return Message(TEXT("Battle paused."),TEXT("战斗已暂停。"));
    auto* Fleet=UFrontlineVehicleSubsystem::Get(B->GetWorld());const auto* F=Fleet?&FleetForEncounterV87(B,Fleet):nullptr;
    if(F)
    {
        const float* Ready=F->ModelReadyAt[Side].Find(StableId);
        if(B->Elapsed<(Ready?*Ready:F->ReadyAt[Side]))return Message(TEXT("The armour column is preparing."),TEXT("装甲纵队正在整备。"));
    }
    if(const auto* D=Definition(StableId);D&&D->Rail&&Fleet)
        for(const AFrontlineTank* Existing:Fleet->GetVehicles())
            if(IsValid(Existing)&&Existing->Team==Side&&Existing->IsOperational()&&Id(Existing)==StableId)
                return Message(TEXT("The railway detachment is already in action."),TEXT("铁路分遣队已在行动。"));
    // A closed reserve queue must not repeatedly reload an unused model.
    // Actual dispatch still requires the complete asset/rig/shader checks.
    FString Error;if(!AssetsReady(StableId,Error))
    {
        UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_V82_ARMOUR_PREPARATION id=%s reason=%s"),*StableId,*Error);
        return Message(TEXT("This vehicle cannot be prepared. Return to the war room and retry."),TEXT("这辆战车暂时无法整备。请返回作战室后重试。"));
    }
    return FString();
}
TArray<FString> Roster(const AWarfareFrontlineGameMode* B,int32 Side){TArray<FString> R;for(const auto& D:Definitions())if(Eligible(B,D.Id,Side))R.Add(D.Id);return R;}
void PreparationMeshes(const AWarfareFrontlineGameMode* B,TArray<USkeletalMesh*>& OutMeshes)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return;
    for(int32 Side=0;Side<2;++Side)for(const FString& Id:Roster(B,Side))
        if(const auto* D=Definition(Id))
            if(auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*URI(*D,TEXT("SK"))))OutMeshes.AddUnique(Mesh);
}
float CooldownFraction(const AWarfareFrontlineGameMode* B,int32 Side,const FString& StableId)
{
    if(!B||Side<0||Side>1||!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting)return 0;
    auto* S=UFrontlineVehicleSubsystem::Get(B->GetWorld());const auto* F=S?&FleetForEncounterV87(B,S):nullptr;
    if(!F)return 0;
    const float* Ready=StableId.IsEmpty()?nullptr:F->ModelReadyAt[Side].Find(StableId);
    const float* Cycle=StableId.IsEmpty()?nullptr:F->ModelCycle[Side].Find(StableId);
    return FMath::Clamp(((Ready?*Ready:F->ReadyAt[Side])-B->Elapsed)/FMath::Max(1.f,Cycle?*Cycle:F->Cycle[Side]),0.f,1.f);
}
bool Entered(const AFrontlineTank* V){const auto* D=Definition(V);const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;return D&&B&&V->GetActorLocation().X*B->GetTeamDirection(V->Team)+HalfLength(V)>=-12000;}
bool PassengerState(const AWarfareFrontlineSoldier* U,FFRRUTrainPassengerStateV82& Out)
{
    if(!U)return false;
    for(const auto& Entry:Vehicles)
    {
        const auto* V=Entry.Key.Get();const auto& R=Entry.Value;const auto* B=R.Battle.Get();
        if(!V||!B||B!=U->Battle.Get()||R.Encounter!=B->GetEncounterId())continue;
        for(const auto& Passenger:R.Passengers)if(Passenger.Actor.Get()==U)
        {
            FFRRUTrainPassengerStateV82 Candidate=Passenger.State;Candidate.Alive=U->IsAlive();
            Candidate.CarrierExitFeet=V->GetActorTransform().TransformPosition(ExitFeet(Candidate.Car));
            const auto* D=Definition(V);if(!D||!D->Rail)return false;
            Candidate.CarrierClearanceCorner=V->GetActorTransform().TransformPosition(FVector(D->Maximum.X+100.,-270.,-19.));
            const bool GroundSurvivor=Candidate.Stage==EFRRUTrainPassengerStageV82::GroundClear&&V->GetHullIntegrity()<=0;
            Candidate.Embarked=Candidate.Alive&&!GroundSurvivor&&Candidate.Stage!=EFRRUTrainPassengerStageV82::Field&&Candidate.Stage!=EFRRUTrainPassengerStageV82::Lost;
            Out=Candidate;return true;
        }
    }
    return false;
}
bool Embarked(const AWarfareFrontlineSoldier* U){FFRRUTrainPassengerStateV82 State;return PassengerState(U,State)&&State.Embarked;}
bool DestroyedCarrierPassenger(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->IsAlive()||!U->Battle)return false;
    for(const auto& Entry:Vehicles)
    {
        const auto* V=Entry.Key.Get();const auto& R=Entry.Value;
        if(!V||V->GetHullIntegrity()>0||R.Battle.Get()!=U->Battle.Get()||R.Encounter!=U->Battle->GetEncounterId())continue;
        for(const auto& P:R.Passengers)if(P.Actor.Get()==U)
            return P.State.Stage!=EFRRUTrainPassengerStageV82::Field&&P.State.Stage!=EFRRUTrainPassengerStageV82::GroundClear&&P.State.Stage!=EFRRUTrainPassengerStageV82::Lost;
    }
    return false;
}
void CrewLost(AFrontlineTank* V,const FFrontlineKillSourceV65& Source)
{
    auto* R=Runtime(V);auto* B=R?R->Battle.Get():nullptr;
    if(!V||!R||!B||R->Encounter!=B->GetEncounterId()||V->GetHullIntegrity()>0||R->CrewLossHandled)return;
    R->CrewLossHandled=true;
    for(auto& P:R->Passengers)if(auto* U=P.Actor.Get();U&&U->IsAlive())
    {
        if(P.State.Stage==EFRRUTrainPassengerStageV82::Field||P.State.Stage==EFRRUTrainPassengerStageV82::Lost)continue;
        if(P.State.Stage==EFRRUTrainPassengerStageV82::GroundClear)
        {
            // This actor is already beyond the last tread. A destroyed carrier
            // cannot retroactively kill a passenger who reached the ground.
            if(!R->DismountRegistered)R->DismountRegistered=WarfareFRRUWorldV82::Disembarked(B,U->SquadId,U);
            U->Capsule->SetCollisionEnabled(P.FieldCollision);U->bFieldEntered=true;
            // A surviving actor is physically on the ground, but Field/route
            // publication must still wait for a real receiving bay. It may
            // defend this location while the wreck retries that transaction.
            if(R->DismountRegistered)PassengerStage(P,EFRRUTrainPassengerStageV82::Field,B->Elapsed);
            WarfareFRRUPresentationV82::EndTrainPassengerPose(U);continue;
        }
        U->ReceiveHit(1.f,.9f,Source.Team,U->Feet()-V->GetActorLocation(),EFrontlineDamageKind::Artillery,.8f,Source);
        if(!U->IsAlive())PassengerStage(P,EFRRUTrainPassengerStageV82::Lost,B->Elapsed);
    }
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_TRAIN_CREW_LOST serial=%d remaining=%d"),V->Serial,Passengers(V));
}
int32 Passengers(const AFrontlineTank* V)
{
    const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;if(!R||!B||R->Encounter!=B->GetEncounterId())return 0;
    int32 Count=0;for(const auto& Passenger:R->Passengers)if(Embarked(Passenger.Actor.Get()))++Count;return Count;
}
float HalfLength(const AFrontlineTank* V){const auto* D=Definition(V);return D?float(FMath::Max(FMath::Abs(D->Minimum.X),FMath::Abs(D->Maximum.X))):0;}
FVector Muzzle(const AFrontlineTank* V,int32 I,bool Cannon)
{
    const auto* R=Runtime(V);if(!R)return V?V->GetActorLocation():FVector::ZeroVector;const auto& MountsArray=Cannon?R->Cannons:R->Machineguns;
    if(!MountsArray.IsValidIndex(I))return V->GetActorLocation();const auto& Mount=MountsArray[I];
    if(Mount.MuzzleBone!=NAME_None&&R->Model.IsValid())return R->Model->GetComponentTransform().TransformPosition(R->Model->GetBoneLocationByName(Mount.MuzzleBone,EBoneSpaces::ComponentSpace));
    return V->GetActorTransform().TransformPosition(Mount.Origin);
}
bool OccupiesNode(const AFrontlineTank* V,const FVector& Centre,float Radius)
{
    if(!V||!V->IsOperational()||!V->HasEnteredField()||Centre.ContainsNaN()||!FMath::IsFinite(Radius)||Radius<0)return false;
    const auto* D=Definition(V);if(!D)return V->OccupiesNationalNodeV82(Centre,Radius);
    const FVector P=V->GetActorTransform().InverseTransformPosition(Centre);
    const FVector Nearest(FMath::Clamp(P.X,D->Minimum.X,D->Maximum.X),FMath::Clamp(P.Y,D->Minimum.Y,D->Maximum.Y),P.Z);
    return FVector::DistSquared2D(P,Nearest)<=FMath::Square(double(Radius));
}
bool Contains(const AFrontlineTank* V,const FVector& P,float Margin)
{const auto* D=Definition(V);return D&&!P.ContainsNaN()&&FMath::IsFinite(Margin)&&FBox(D->Minimum,D->Maximum).ExpandBy(FMath::Clamp(Margin,0.f,25.f)).IsInsideOrOn(V->GetActorTransform().InverseTransformPosition(P));}
FVector GrenadeAim(const AFrontlineTank* V,const FVector& Shooter){FVector P=NearHull(V,Shooter);float Z=0;const auto* R=Runtime(V);if(R&&WarfareFRRUWorldV82::TryHeight(R->Battle.Get(),float(P.X),float(P.Y),Z))P.Z=Z+12;return P;}
FFRRUArmourEffectiveSpecV87 EffectiveSpecV87(const AFrontlineTank* V)
{const auto* R=Runtime(V);return R?R->Spec:FFRRUArmourEffectiveSpecV87();}
static int32 NativeRepairSideV87(const AFrontlineTank* V)
{
    const auto* D=Definition(V);if(!V||!D||D->Rail||!V->IsOperational()||!V->HasEnteredField())return INDEX_NONE;
    for(int32 Side=0;Side<2;++Side)if(V->CanRepairTrackV60(Side))return Side;return INDEX_NONE;
}
static bool NativeRepairThreatV87(const AFrontlineTank* V)
{
    const auto* R=Runtime(V);const auto* B=R?R->Battle.Get():nullptr;if(!B)return true;
    for(const AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team!=V->Team&&U->IsActiveCombatant()&&U->bFieldEntered&&FVector::DistSquared2D(U->Feet(),V->GetActorLocation())<FMath::Square(1100.))return true;
    if(const auto* F=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(const AFrontlineTank* Enemy:F->GetVehicles())if(IsValid(Enemy)&&Enemy->Team!=V->Team&&Enemy->IsOperational()&&Enemy->HasEnteredField()&&FVector::DistSquared2D(Enemy->GetActorLocation(),V->GetActorLocation())<FMath::Square(1100.))return true;
    return false;
}
static bool NativeRepairWorkerV87(const AWarfareFrontlineSoldier* U)
{
    if(!U||!U->Battle||!U->IsActiveCombatant()||!U->bFieldEntered||U->Role!=EFrontlineRole::Pioneer||U->bEngaging||U->Suppression>=.35f||U->WeaponPhase!=EFrontlineWeaponPhase::Ready||Embarked(U))return false;
    const auto* B=U->Battle.Get();const auto* Group=B->FindSquad(U->SquadId);
    if(!Group||Group->bAdvancing||Group->bHoldPosition||!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting||B->BattleOutroWinner!=INDEX_NONE)return false;
    const auto View=WarfareFRRUWorldV82::View(B);if(!View.Finished&&View.RequiredRole==int32(EFrontlineRole::Pioneer))return false;
    for(const auto& Weak:Group->Members)if(const auto* Other=Weak.Get();Other&&Other->IsAlive()&&Other->MemberIndex<U->MemberIndex)return false;
    return true;
}
static bool NativeRepairPointV87(const AFrontlineTank* V,int32 Side,const AWarfareFrontlineSoldier* U,FVector& Out)
{
    const auto* D=Definition(V);const auto* R=Runtime(V);if(!D||!R||!U)return false;
    const double Width=FMath::Max(FMath::Abs(D->Minimum.Y),FMath::Abs(D->Maximum.Y));
    Out=V->GetActorLocation()+V->GetActorRightVector()*((Side==0?-1.:1.)*(Width+90.))+V->GetActorForwardVector()*((U->MemberIndex%3-1)*64.);
    float Floor=0;if(!WarfareFRRUWorldV82::TryHeight(R->Battle.Get(),Out.X,Out.Y,Floor))return false;Out.Z=Floor;return true;
}
bool RepairDestinationV87(const AWarfareFrontlineSoldier* U,FVector& Out)
{
    if(!NativeRepairWorkerV87(U))return false;const auto* B=U->Battle.Get();double Best=FMath::Square(1000.);bool Found=false;
    for(const auto& Pair:Vehicles)if(const auto* V=Pair.Key.Get();V&&V->Team==U->Team&&Pair.Value.Battle.Get()==B&&Pair.Value.Encounter==B->GetEncounterId())
    {
        const int32 Side=NativeRepairSideV87(V);if(Side==INDEX_NONE||NativeRepairThreatV87(V))continue;FVector Point;
        if(!NativeRepairPointV87(V,Side,U,Point))continue;const double Distance=FVector::DistSquared2D(U->Feet(),Point);
        if(Distance<Best){Best=Distance;Out=Point;Found=true;}
    }
    return Found;
}
bool RepairWorkStateV87(const AWarfareFrontlineSoldier* U,FFRRUWorkStateV82& Out)
{
    if(!NativeRepairWorkerV87(U)||U->bMoving)return false;const auto* B=U->Battle.Get();
    for(const auto& Pair:Vehicles)if(const auto* V=Pair.Key.Get();V&&Pair.Value.RepairWorker.Get()==U&&Pair.Value.Battle.Get()==B&&Pair.Value.Encounter==B->GetEncounterId())
    {
        const auto& R=Pair.Value;if(NativeRepairSideV87(V)!=R.RepairSide||NativeRepairThreatV87(V))return false;FVector Point;if(!NativeRepairPointV87(V,R.RepairSide,U,Point)||FVector::DistSquared2D(U->Feet(),Point)>FMath::Square(70.))return false;
        Out=FFRRUWorkStateV82();Out.Encounter=R.Encounter;Out.StageId=TEXT("vehicle.repair");Out.NodeId=FString::Printf(TEXT("vehicle.repair.%d.%d"),V->Serial,R.RepairSide);Out.SquadId=U->SquadId;Out.MemberIndex=U->MemberIndex;
        Out.Semantic=EFRRUWorkSemanticV82::ClearPassage;Out.Target=V->GetActorLocation()+FVector(0,0,45);Out.Facing=(V->GetActorLocation()-U->Feet()).GetSafeNormal2D();
        Out.HostElapsed=B->Elapsed;Out.StartedAt=R.RepairStartedAt;Out.EndsAt=R.RepairStartedAt+R.Spec.RepairSeconds;Out.ProgressSeconds=R.RepairProgress;Out.RequiredSeconds=R.Spec.RepairSeconds;
        Out.NormalizedPhase=FMath::Clamp(R.RepairProgress/FMath::Max(.1f,R.Spec.RepairSeconds),0.f,1.f);Out.Timed=true;Out.Active=true;Out.Paused=UGameplayStatics::IsGamePaused(B);return true;
    }
    return false;
}
static void TickNativeRepairV87(AFrontlineTank* V,FVehicle& R)
{
    auto* B=R.Battle.Get();const int32 Side=NativeRepairSideV87(V);AWarfareFrontlineSoldier* Worker=nullptr;
    if(B&&Side!=INDEX_NONE&&B->BattleOutroWinner==INDEX_NONE&&!NativeRepairThreatV87(V))
        for(AWarfareFrontlineSoldier* U:B->Units)if(U&&U->Team==V->Team&&NativeRepairWorkerV87(U)&&!U->bMoving)
        {
            FVector Point;if(!NativeRepairPointV87(V,Side,U,Point)||FVector::DistSquared2D(U->Feet(),Point)>FMath::Square(70.))continue;
            FFRRUWorkStateV82 Priority;if(WarfareFRRUWorldV82::WorkState(U,Priority)&&Priority.Active&&!Priority.NodeId.StartsWith(TEXT("vehicle.repair.")))continue;
            if(!Worker||U->SquadId*16+U->MemberIndex<Worker->SquadId*16+Worker->MemberIndex)Worker=U;
        }
    if(!B)return;
    if(!Worker){R.RepairWorker.Reset();R.RepairProgress=0;R.RepairSide=INDEX_NONE;R.RepairLastHostAt=B->Elapsed;return;}
    if(R.RepairWorker.Get()!=Worker||R.RepairSide!=Side)
    {R.RepairWorker=Worker;R.RepairProgress=0;R.RepairSide=Side;R.RepairStartedAt=R.RepairLastHostAt=B->Elapsed;return;}
    const float Seconds=FMath::Max(0.f,B->Elapsed-R.RepairLastHostAt);R.RepairLastHostAt=B->Elapsed;R.RepairProgress+=Seconds;
    if(R.RepairProgress<R.Spec.RepairSeconds||!V->RepairTrackV60(Side))return;
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_ARMOUR_FIELD_REPAIR id=%s serial=%d side=%d worker=%d/%d realWorkSeconds=%.3f required=%.3f"),*R.StableId,V->Serial,Side,Worker->SquadId,Worker->MemberIndex,R.RepairProgress,R.Spec.RepairSeconds);
    if(V->Team==0)B->LastMessage=Message(TEXT("The running gear is repaired. Keep the crew covered."),TEXT("行走机构已修复。继续掩护车组。"));
    R.RepairWorker.Reset();R.RepairProgress=0;R.RepairSide=INDEX_NONE;
}
}

// Narrow access for the additional stable-ID actor consumer. Old enum values,
// serials, two-side arrays and the old roster/save format remain unchanged.
struct FFRRUArmourAccessV82
{
    static void BindHostEncounter(UFrontlineVehicleSubsystem* F,AWarfareFrontlineGameMode* B)
    {
        if(!F||!B||F->LastEncounter==B->GetEncounterId())return;
        // The host performs this same transition in its Tick/Recruit. A
        // national dispatch can legally precede that first subsystem tick;
        // bind before publishing an actor or taking a national fleet reference
        // so the later host transition cannot erase the new vehicle/cooldown.
        const int32 PreviousVehicles=F->Vehicles.Num();F->ClearEncounter();F->LastEncounter=B->GetEncounterId();
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_ARMOUR_HOST_EPOCH_SYNC encounter=%s hostElapsed=%.3f clearedPriorVehicles=%d"),
            *B->GetEncounterId().ToString(),B->Elapsed,PreviousVehicles);
    }
    static bool AssemblePassengers(AFrontlineTank* V,AWarfareFrontlineGameMode* B,int32 Side)
    {
        using namespace WarfareFRRUArmourV82;auto* R=Runtime(V);FString Error;
        if(!R||!WarfareFRRUPresentationV82::TrainPassengerAssetsReady(B,Side,Error)
            ||!WarfareFRRUPresentationV82::PrepareRoleAssets(B,EFrontlineRole::Rifle,Side,Error))return false;
        const FString PreviousFailure=B->AssetFailure;const int32 SquadId=B->NextSquadId++;
        FFrontlineSquad Squad;Squad.Id=SquadId;Squad.Team=Side;Squad.Role=EFrontlineRole::Rifle;
        B->Squads.Add(Squad);R->LastPassengerHostAt=B->Elapsed;R->Passengers.Reserve(6);
        auto Reject=[&]()
        {
            for(auto& P:R->Passengers)if(auto* U=P.Actor.Get()){B->Units.Remove(U);U->Destroy();}
            R->Passengers.Empty();B->Squads.RemoveAll([&](const FFrontlineSquad& S){return S.Id==SquadId;});
            if(B->NextSquadId==SquadId+1)B->NextSquadId=SquadId;B->AssetFailure=PreviousFailure;
            UE_LOG(LogTemp,Error,TEXT("WF_FRRU_TRAIN_ASSEMBLY_REJECT serial=%d %s"),V->Serial,*Error);return false;
        };
        for(int32 I=0;I<6;++I)
        {
            const int Car=I<3?0:2,Seat=I%3;FPassenger P;
            P.State.CarrierSerial=V->Serial;P.State.SquadId=SquadId;P.State.MemberIndex=I;P.State.Car=Car;P.State.Seat=Seat;
            P.State.LocalFeet=SeatFeet(Car,Seat);P.State.StageStartedAt=B->Elapsed;
            P.State.Facing=V->GetActorTransform().TransformVectorNoScale(FVector(0,-1,0));
            FVector Feet,Centre;if(!PassengerContact(V,P.State.LocalFeet,Feet,Centre)){Error=TEXT("The seat has no physical support.");return Reject();}
            FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            auto* U=B->GetWorld()->SpawnActor<AWarfareFrontlineSoldier>(Centre,P.State.Facing.Rotation(),Params);
            if(!U){Error=TEXT("The actual passenger actor could not be created.");return Reject();}
            P.Actor=U;P.State.WorldFeet=Feet;R->Passengers.Add(MoveTemp(P));auto& Created=R->Passengers.Last();
            U->bFieldEntered=false;U->Initialize(B,SquadId,Side,EFrontlineRole::Rifle,I);
            if(!U->Soldier||!U->Soldier->GetSkeletalMeshAsset()||!WarfareFRRUPresentationV82::HasBoundWeapon(U))
            {Error=B->AssetFailure.IsEmpty()?TEXT("The passenger's actual national equipment is not bound."):B->AssetFailure;return Reject();}
            FrontlineFieldV32::InitializeUnit(U);Created.FieldCollision=U->Capsule->GetCollisionEnabled();U->Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            B->Units.AddUnique(U);B->FindSquad(SquadId)->Members.Add(U);
            if(!PlacePassenger(V,Created,Created.State.LocalFeet,false))
            {Error=TEXT("The actual seated capsule intersects the carriage.");return Reject();}
            if(!PassengerPose(Created,TEXT("Seated"),0,Error))return Reject();
        }
        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_TRAIN_ASSEMBLED serial=%d squad=%d actors=%d"),V->Serial,SquadId,R->Passengers.Num());return true;
    }
    static bool Configure(AFrontlineTank* V,AWarfareFrontlineGameMode* B,const FString& Id,int32 Side,int32 Serial)
    {
        using namespace WarfareFRRUArmourV82;const auto* D=Definition(Id);FString Error;if(!D||!AssetsReady(Id,Error))return false;
        V->Battle=B;V->Type=EFrontlineVehicle::Count;V->Team=Side;V->Serial=Serial;V->Random.Initialize(821918+Serial*113+Side*991);
        V->HullIntegrity=V->MaximumHullIntegrity=D->Hull;V->TrackIntegrity[0]=V->TrackIntegrity[1]=D->Wheeled?100:230;
        V->Body->SetStaticMesh(nullptr);V->Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->TrackLinks->SetVisibility(false);
        FVehicle R;R.StableId=Id;R.Battle=B;R.Encounter=B->GetEncounterId();R.Spec=DispatchSpecV87(B,Side);R.Heading=B->GetTeamDirection(Side)>0?0:180;Mounts(R,*D);
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*URI(*D,TEXT("SK")));
        auto* Driver=NewObject<USkeletalMeshComponent>(V,TEXT("NationalMechanicalDriver"));V->AddInstanceComponent(Driver);Driver->SetupAttachment(V->Body);Driver->SetSkeletalMeshAsset(Mesh);
        Driver->SetCollisionEnabled(ECollisionEnabled::NoCollision);Driver->SetVisibility(false);Driver->SetComponentTickEnabled(false);Driver->RegisterComponent();
        Driver->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;Driver->PlayAnimation(LoadObject<UAnimSequence>(nullptr,*URI(*D,TEXT(""),TEXT("Drive144cm"))),true);Driver->SetPosition(0,false);Driver->TickAnimation(0,false);Driver->RefreshBoneTransforms();
        auto* Pose=NewObject<UPoseableMeshComponent>(V,TEXT("NationalArmouredModel"));V->AddInstanceComponent(Pose);Pose->SetupAttachment(V->Body);Pose->SetSkinnedAssetAndUpdate(Mesh);Pose->SetRelativeScale3D(FVector(1,-1,1));Pose->SetCollisionEnabled(ECollisionEnabled::NoCollision);Pose->SetCastShadow(true);Pose->RegisterComponent();Pose->CopyPoseFromSkeletalComponent(Driver);
        R.RootBone=Bone(*D,TEXT("root"));R.RootRest=Mesh->GetRefSkeleton().GetRefBonePose()[0];Pose->SetBoneTransformByName(R.RootBone,R.RootRest,EBoneSpaces::ComponentSpace);
        const auto& Prepared=PreparedVehicleAssetsV82.FindChecked(Id);R.RecoilClip=Prepared.Recoil.Get();R.ServiceClip=Prepared.Service.Get();
        R.CannonRecoilAge.Init(1000.f,R.Cannons.Num());R.GunRecoilAge.Init(1000.f,R.Machineguns.Num());
        const auto& Ref=Mesh->GetRefSkeleton();const auto& SkeletonRef=Mesh->GetSkeleton()->GetReferenceSkeleton();
        const auto Overlay=[&](const FName Name,int32 Weapon,bool Cannon,TArray<FMechanicalOverlayV87>& List)
        {
            const int32 Index=Ref.FindBoneIndex(Name),SkeletonIndex=SkeletonRef.FindBoneIndex(Name);if(Index==INDEX_NONE||SkeletonIndex==INDEX_NONE)return;
            FMechanicalOverlayV87 O;O.Bone=Name;O.SkeletonIndex=SkeletonIndex;O.RestLocal=Ref.GetRefBonePose()[Index];O.WeaponIndex=Weapon;O.Cannon=Cannon;
            const int32 Parent=Ref.GetParentIndex(Index);O.ParentBone=Parent==INDEX_NONE?NAME_None:Ref.GetBoneName(Parent);List.Add(O);
        };
        for(int32 I=0;I<R.Cannons.Num();++I)Overlay(FName(*R.Cannons[I].PitchBone.ToString().Replace(TEXT("_pitch"),TEXT("_recoil"))),I,true,R.RecoilBones);
        if(D->Wheeled)for(int32 I=0;I<R.Machineguns.Num();++I)
            Overlay(FName(*R.Machineguns[I].PitchBone.ToString().Replace(TEXT("_pitch"),TEXT("_feed_lever"))),I,false,R.RecoilBones);
        for(int32 I=0;I<Ref.GetNum();++I)
        {
            const FString Name=Ref.GetBoneName(I).ToString();
            if(Name.Contains(TEXT("hinge"))&&(Name.Contains(TEXT("roof"))||Name.Contains(TEXT("rear"))||Name.Contains(TEXT("engine"))))Overlay(Ref.GetBoneName(I),0,false,R.HatchBones);
        }
        const float Seed=FMath::Frac(float(GetTypeHash(Id)%10000)*.0001f+Serial*.6180339f);V->BodyMaterials.Empty();
        for(int32 Slot=0;Slot<Pose->GetNumMaterials();++Slot)if(auto* Original=Pose->GetMaterial(Slot))if(auto* Parent=AppearanceMaterialV87(Original))
        {
            auto* MID=UMaterialInstanceDynamic::Create(Parent,V);if(!MID)continue;
            MID->SetScalarParameterValue(TEXT("FieldDust"),(D->Wheeled?.14f:.18f)+Seed*.05f);MID->SetScalarParameterValue(TEXT("FieldRust"),.08f+Seed*.035f);
            MID->SetScalarParameterValue(TEXT("VehicleDamage"),0);MID->SetScalarParameterValue(TEXT("VehicleSeed"),Seed);
            FLinearColor Tint(1,1,1,1);
            if(Original->GetPathName()==TEXT("/Game/FRRUExpansionR2/FormalV82/Materials/Vehicle/M_R2_Vehicle_paint_UV0_T1.M_R2_Vehicle_paint_UV0_T1"))
            {
                // Modulate the retained painted/worn albedo; steel, brass and
                // recess slots keep their original colours and PBR response.
                if(D->Id==TEXT("fr.schneider"))Tint=FLinearColor(.98f,.96f,.89f);
                else if(D->Id==TEXT("fr.saint_chamond"))Tint=FLinearColor(.92f,.96f,.90f);
                else if(D->Id==TEXT("fr.ft"))Tint=FLinearColor(.90f,.95f,.86f);
                else if(D->Id==TEXT("ru.austin_series1"))Tint=FLinearColor(.86f,.90f,.82f);
            }
            else if(Original->GetPathName()==TEXT("/Game/FRRUIntegration/FormalV82/Train/Materials/M_AHTrain_V82_paint.M_AHTrain_V82_paint"))Tint=FLinearColor(.86f,.90f,.88f);
            MID->SetVectorParameterValue(TEXT("VehiclePaintTint"),Tint);Pose->SetMaterial(Slot,MID);V->BodyMaterials.Add(MID);++R.AppearanceMaterials;
        }
        if(R.AppearanceMaterials!=Pose->GetNumMaterials())UE_LOG(LogTemp,Warning,TEXT("WF_FRRU_ARMOUR_APPEARANCE_NOT_READY id=%s actualDerivedSlots=%d expected=%d"),*Id,R.AppearanceMaterials,Pose->GetNumMaterials());
        auto* Hull=NewObject<UBoxComponent>(V,TEXT("NationalHullQuery"));V->AddInstanceComponent(Hull);Hull->SetupAttachment(V->Body);Hull->SetBoxExtent((D->Maximum-D->Minimum)*.5);Hull->SetRelativeLocation((D->Minimum+D->Maximum)*.5);
        Hull->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Hull->SetCollisionResponseToAllChannels(ECR_Ignore);Hull->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Hull->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);Hull->SetCanEverAffectNavigation(false);Hull->RegisterComponent();
        R.Model=Pose;R.Driver=Driver;R.Hull=Hull;
        if(D->Rail){if(!ConfigureCarriages(V,R))return false;V->bRailConvoyV32=true;V->bRailUnloadedV75=false;}
        for(auto* MountList:{&R.Cannons,&R.Machineguns})for(auto& Mount:*MountList)if(Mount.PitchBone!=NAME_None)
        {
            Mount.LocalMuzzle=ReferenceFrame(Ref,Ref.FindBoneIndex(Mount.PitchBone)).InverseTransformPosition(FVector(Mount.Origin.X,-Mount.Origin.Y,Mount.Origin.Z));
            Mount.RestYaw=ReferenceFrame(Ref,Ref.FindBoneIndex(Mount.YawBone));
        }
        for(const auto& M:R.Machineguns){auto* C=NewObject<UStaticMeshComponent>(V);V->AddInstanceComponent(C);C->SetupAttachment(V->Body);C->SetRelativeLocation(M.Origin);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();V->Guns.Add(C);V->GunMounts.Add(M.Origin);V->GunBaseYaw.Add(M.BaseYaw);V->GunIntegrity.Add(65);V->GunClock.Add(V->Random.FRandRange(1.f,3.f));V->BurstRemaining.Add(0);}
        for(const auto& M:R.Cannons){auto* C=NewObject<UStaticMeshComponent>(V);V->AddInstanceComponent(C);C->SetupAttachment(V->Body);C->SetRelativeLocation(M.Origin);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();V->Cannons.Add(C);V->CannonMounts.Add(M.Origin);V->CannonBaseYaw.Add(M.BaseYaw);}
        auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Characters/V13/Materials/M_MuzzleFlash_V13.M_MuzzleFlash_V13"));if(!Plane||!Material)return false;
        for(int32 I=0;I<V->Guns.Num()+V->Cannons.Num();++I){auto* F=NewObject<UStaticMeshComponent>(V);V->AddInstanceComponent(F);F->SetupAttachment(V->Body);F->SetStaticMesh(Plane);F->SetCollisionEnabled(ECollisionEnabled::NoCollision);F->SetCastShadow(false);F->SetVisibility(false);auto* MID=UMaterialInstanceDynamic::Create(Material,V);F->SetMaterial(0,MID);F->RegisterComponent();V->Flashes.Add(F);V->FlashMaterials.Add(MID);V->FlashAge.Add(100);}
        Vehicles.Add(V,MoveTemp(R));return true;
    }
    static AFrontlineTank* Spawn(UFrontlineVehicleSubsystem* F,AWarfareFrontlineGameMode* B,const FString& Id,int32 Side)
    {
        using namespace WarfareFRRUArmourV82;const auto* D=Definition(Id);if(!D)return nullptr;const int32 Dir=B->GetTeamDirection(Side);
        const int32 Lane=(F->DeploymentCountV37[Side]%7)-3;float Y=Lane*680.f;if(D->Rail)if(const auto* Rail=Railways.Find(B))Y=Rail->Y;
        float X=-Dir*(12000.f+D->Speed*2.8f);X-=Dir*float(FMath::Max(FMath::Abs(D->Minimum.X),FMath::Abs(D->Maximum.X)));
        float Z=0;if(D->Rail)Z=RailHeight(B,X);else if(!WarfareFRRUWorldV82::TryHeight(B,X,Y,Z))return nullptr;
        auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>(FVector(X,Y,Z),FRotator(0,Dir<0?180:0,0));if(!V)return nullptr;
        const int32 PreviousSerial=F->NextSerial;
        if(!Configure(V,B,Id,Side,++F->NextSerial)){F->NextSerial=PreviousSerial;V->Destroy();return nullptr;}
        if(!Occupy(V,V->GetActorLocation(),V->GetActorRotation().Yaw)||(D->Rail&&!AssemblePassengers(V,B,Side)))
        {F->NextSerial=PreviousSerial;Vehicles.Remove(V);V->Destroy();return nullptr;}
        F->Vehicles.Add(V);++F->DeploymentCountV37[Side];return V;
    }
    static void Simulate(AFrontlineTank* V,float DT)
    {
        using namespace WarfareFRRUArmourV82;auto* R=Runtime(V);const auto* D=Definition(V);if(!R||!D||!R->Battle.IsValid())return;auto* B=R->Battle.Get();
        TickNativeRepairV87(V,*R);
        const bool Outro=B->BattleOutroWinner!=INDEX_NONE;
        if(Outro){for(auto& S:V->Shells)if(S.Visual.IsValid())S.Visual->DestroyComponent();V->Shells.Empty();}
        else V->UpdateShells(DT);
        // Retain the host's no-post-surrender damage rule while allowing
        // already visible fire, smoke, flashes and wreck audio to finish.
        V->UpdateEffects(DT);
        if(!V->IsOperational())
        {
            ApplyMechanicalOverlayV87(V,*R,true,DT);
            if(R->CrewLossHandled&&B->Phase==EFrontlinePhase::Fighting)
                for(auto& P:R->Passengers)if(auto* U=P.Actor.Get();U&&U->IsAlive()&&P.State.Stage==EFRRUTrainPassengerStageV82::GroundClear)
                {
                    if(!R->DismountRegistered)R->DismountRegistered=WarfareFRRUWorldV82::Disembarked(B,U->SquadId,U);
                    if(R->DismountRegistered)PassengerStage(P,EFRRUTrainPassengerStageV82::Field,B->Elapsed);
                }
            V->WreckAge+=DT;
            if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->UpdateTankAudio(V->Serial+V->Team*1000,V->GetActorLocation(),0,false,V->WreckAge<70,true);
            // Wrecks belong to the encounter, including a surviving passenger
            // waiting for a clear receiving bay. Cleanup owns their lifetime.
            return;
        }
        if(Outro)
        {if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->UpdateTankAudio(V->Serial+V->Team*1000,V->GetActorLocation(),0,!V->IsImmobilized(),V->HullIntegrity<D->Hull*.45f,true);return;}
        V->bEntered|=Entered(V);const FVector Start=V->GetActorLocation();const int32 Dir=B->GetTeamDirection(V->Team);const auto* F=UFrontlineVehicleSubsystem::Get(B->GetWorld());
        AFrontlineTank* Armour=nullptr;float ArmourDistance=WeaponRangeV87(V,true);
        if(F&&!R->Cannons.IsEmpty())for(AFrontlineTank* Other:F->GetVehicles())if(Other&&Other!=V&&Other->Team!=V->Team&&Other->IsOperational()&&Other->HasEnteredField())
        {const float Distance=FVector::Dist2D(Start,Other->AimPoint());if(Distance<ArmourDistance&&Clear(V,Other,Start+FVector(0,0,150),Other->AimPoint())){ArmourDistance=Distance;Armour=Other;}}
        const bool HullAcquiringArmour=Armour&&!D->Rail&&D->Id!=TEXT("fr.ft");
        if(HullAcquiringArmour)
        {const float Want=float((Armour->GetActorLocation()-Start).Rotation().Yaw);const float Turned=FMath::FixedTurn(R->Heading,Want,DT*25);
            if(FTGroundPilotV82::Enabled(V)&&FTGroundPilotV82::InCompleteGrid(V,Start,Turned))
            {
                FTransform Candidate;
                if(FTGroundPilotV82::Plan(V,Start,Turned,0,Candidate)&&V->SetActorTransform(Candidate,false,nullptr,ETeleportType::None))
                {R->Heading=Turned;FTGroundPilotV82::CommitWarm(V);}
            }
            else if(Occupy(V,Start,Turned)){R->Heading=Turned;V->SetActorRotation(FRotator(0,Turned,0));}}
        // All infantry candidates share the original immutable ignore list.
        // Selection and the final shot retain both native sight and Visibility.
        FCollisionQueryParams InfantrySight(SCENE_QUERY_STAT(FRRUV82VehicleShot),true);InfantrySight.AddIgnoredActor(V);
        for(const auto& Unit:B->Units)if(Unit)InfantrySight.AddIgnoredActor(Unit);
        bool HaltForWeapon=false;
#if WITH_EDITOR
        R->EngagementHostAt=B->Elapsed;R->HaltForWeapon=false;R->HullAcquiringArmour=HullAcquiringArmour;R->StopContact={};R->WeaponReadbacks.Reset();
#endif
        if(V->bEntered&&!D->Rail)for(int32 Kind=0;Kind<2&&!HaltForWeapon;++Kind)
        {
            const bool Cannon=Kind==0;const auto& MountArray=Cannon?R->Cannons:R->Machineguns;
            for(int32 MountIndex=0;MountIndex<MountArray.Num();++MountIndex)
            {
                FMountReadback* StopReadback=nullptr;
#if WITH_EDITOR
                FMountReadback CandidateStop;StopReadback=&CandidateStop;
#endif
                const auto Contact=SelectWeaponContact(V,*R,MountIndex,Cannon,InfantrySight,StopReadback);
                // Keep the existing close-infantry pacing and armour priority,
                // but only an actual available mount may justify a combat halt.
                // Reload and finite yaw slew remain execution waits, not new
                // permission to select an unseen or out-of-sector enemy.
                if(Contact.Actor()&&(Contact.Armour||Contact.DistanceCm<D->Range*.6))
                {
                    HaltForWeapon=true;
#if WITH_EDITOR
                    R->StopContact=CandidateStop;R->HaltForWeapon=true;
#endif
                    break;
                }
            }
        }
        float Travel=0;
        const auto NavGuide=D->Id==TEXT("fr.ft")?FRRUVehicleNavV87::FollowHeading(B,V):FRRUVehicleNavV87::FGuidance{};
        const bool NavCandidateStop=NavGuide.Active&&(NavGuide.Pending||NavGuide.StopAtActualGoal||!NavGuide.HasHeading);
        if(!V->IsImmobilized()&&!HaltForWeapon&&!NavCandidateStop)
        {
            float MoveHeading=R->Heading;
            if(!D->Rail)
            {
                // Fixed-gun acquisition has just committed a supported hull
                // turn above. The route controller must not immediately turn
                // it back faster than the gun can acquire its narrow sector.
                float RouteYaw=HullAcquiringArmour?R->Heading:B->Elapsed<R->AvoidUntil?R->AvoidHeading:Dir>0?0.f:180.f;
                if(NavGuide.Active&&NavGuide.HasHeading&&B->Elapsed>=R->AvoidUntil)RouteYaw=NavGuide.Heading;
                const float Turned=FMath::FixedTurn(R->Heading,RouteYaw,DT*32);bool KeepSupportedPose=false;
                KeepSupportedPose=FTGroundPilotV82::Enabled(V);
                if(KeepSupportedPose)MoveHeading=Turned;
                if(!KeepSupportedPose&&Occupy(V,Start,Turned)){R->Heading=Turned;V->SetActorRotation(FRotator(0,Turned,0));}}
            if(!FTGroundPilotV82::Enabled(V))MoveHeading=R->Heading;
            const FVector Heading=D->Rail?FVector(R->RailReturning?1:Dir,0,0):FRotator(0,MoveHeading,0).Vector();
            FVector Goal=Start+Heading*D->Speed*DT;
            if(D->Rail&&!R->RailReturning&&Goal.X<7150){Goal.X=7150;R->RailHold+=DT;}
            if(D->Rail&&R->RailReturning&&Goal.X>13050){V->Destroy();return;}
            float Z=0;if(D->Rail)Z=RailHeight(B,float(Goal.X));
            if(D->Rail||WarfareFRRUWorldV82::TryHeight(B,float(Goal.X),float(Goal.Y),Z)){Goal.Z=Z;
                const bool NativePath=D->Rail||FMath::Abs(Start.X)>12000||FMath::Abs(Goal.X)>12000||WarfareFRRUWorldV82::CanTraverse(B,Start,Goal);
                bool CandidateAccepted=false,SupportedGround=false;FTransform CandidatePose;
                SupportedGround=FTGroundPilotV82::Enabled(V)
                    &&FTGroundPilotV82::InCompleteGrid(V,Start,MoveHeading)
                    &&FTGroundPilotV82::InCompleteGrid(V,Goal,MoveHeading);
                if(SupportedGround)
                {
                    if(NativePath)CandidateAccepted=FTGroundPilotV82::Plan(V,Goal,MoveHeading,float(FVector::Dist2D(Start,Goal)),CandidatePose);
                    else FTGroundPilotV82::RecordRejected(V,Goal,TEXT("native_route"));
                }
                if(!SupportedGround)CandidateAccepted=NativePath&&Occupy(V,Goal,MoveHeading);
                if(CandidateAccepted)
                {
                    const FTransform CommittedPose=SupportedGround?CandidatePose:FTransform(FRotator(0,MoveHeading,0),Goal);
                    const bool Committed=V->SetActorTransform(CommittedPose,false,nullptr,ETeleportType::None);
                    if(Committed)
                    {
                        if(SupportedGround)FTGroundPilotV82::CommitWarm(V);
                        R->Heading=MoveHeading;
                        Travel=float(FVector::Dist2D(Start,Goal));V->DistanceTravelled+=Travel;R->Wait=0;
                    }
                    else
                    {
                        if(SupportedGround)FTGroundPilotV82::RecordRejected(V,Goal,TEXT("actor_transform_commit_refused"));
                        R->Wait+=DT;
                    }
                }
                else R->Wait+=DT;
            }
            // A supported tank cannot simply force its long hull into a crater.
            // After a real blocked interval, turn gradually toward the less
            // broken side. This is a proposal only: native routes, every actual
            // face, support rays and the complete hull still decide admission.
            if(!D->Rail&&FTGroundPilotV82::Enabled(V)&&Travel==0&&R->Wait>=.3f&&B->Elapsed>=R->AvoidRetryAt)
            {
                R->AvoidRetryAt=B->Elapsed+.5f;float RouteYaw=Dir>0?0.f:180.f;
                if(NavGuide.Active&&NavGuide.HasHeading)RouteYaw=NavGuide.Heading;
                auto SideScore=[&](float Yaw)
                {
                    const FVector Forward=FRotator(0,Yaw,0).Vector(),Across(-Forward.Y,Forward.X,0);
                    const FVector Ahead=Start+Forward*650.f;double Low=DBL_MAX,High=-DBL_MAX;
                    if(!WarfareFRRUWorldV82::CanTraverse(B,Start,Ahead))return DBL_MAX;
                    for(const FVector2D Offset:{FVector2D(0,0),FVector2D(-200,-140),FVector2D(-200,140),FVector2D(200,-140),FVector2D(200,140)})
                    {
                        const FVector P=Ahead+Forward*Offset.X+Across*Offset.Y;float Height=0;
                        if(!WarfareFRRUWorldV82::TryHeight(B,float(P.X),float(P.Y),Height))return DBL_MAX;
                        Low=FMath::Min(Low,double(Height));High=FMath::Max(High,double(Height));
                    }
                    return High-Low;
                };
                const float Left=RouteYaw-35.f,Right=RouteYaw+35.f;
                const double LeftScore=SideScore(Left),RightScore=SideScore(Right);
                const float Preferred=LeftScore<RightScore?Left:RightScore<LeftScore?Right:V->Serial%2?Left:Right;
                const bool Held=B->Elapsed<R->AvoidUntil;TArray<float,TInlineAllocator<2>> Wants;
                if(Held)Wants.Add(R->AvoidHeading);else{Wants.Add(Preferred);Wants.Add(Preferred==Left?Right:Left);}
                bool TurnAccepted=false;
                for(const float Want:Wants)
                {
                    if((Want==Left?LeftScore:RightScore)==DBL_MAX)continue;
                    const float Turned=FMath::FixedTurn(R->Heading,Want,DT*32);FTransform TurnPose;
                    // Revalidating the identical pose is not an avoidance turn.
                    // In particular, it must not renew a blocked heading's hold.
                    if(FMath::Abs(FMath::FindDeltaAngleDegrees(R->Heading,Turned))<=KINDA_SMALL_NUMBER)continue;
                    if(!FTGroundPilotV82::InCompleteGrid(V,Start,Turned)
                        ||!FTGroundPilotV82::Plan(V,Start,Turned,0,TurnPose))continue;
                    if(!V->SetActorTransform(TurnPose,false,nullptr,ETeleportType::None))continue;
                    FTGroundPilotV82::CommitWarm(V);R->Heading=Turned;R->AvoidHeading=Want;if(!Held)R->AvoidUntil=B->Elapsed+10.f;
                    // Turning uses the existing host tick. It grants no forward
                    // distance, speed, track phase, weapon or objective progress.
                    TurnAccepted=true;break;
                }
                if(!TurnAccepted&&D->Id==TEXT("fr.ft"))
                {
                    FTransform EscapePose;float EscapeHeading=0,EscapeTravel=0,EscapeAvoidHeading=0;
                    if(FTGroundPilotV82::TryContinuousEscape(V,DT,EscapePose,EscapeHeading,EscapeTravel,EscapeAvoidHeading)
                        &&V->SetActorTransform(EscapePose,false,nullptr,ETeleportType::None))
                    {
                        FTGroundPilotV82::CommitWarm(V);R->Heading=EscapeHeading;R->AvoidHeading=EscapeAvoidHeading;
                        R->AvoidUntil=B->Elapsed+10.f;R->Wait=0;Travel=EscapeTravel;V->DistanceTravelled+=Travel;
                        UE_LOG(LogTemp,Display,TEXT("WF_FRRU_FT_CONTINUOUS_ESCAPE_COMMITTED serial=%d host=%.3f actual=(%s) travel=%.6f yaw=%.6f"),
                            V->Serial,B->Elapsed,*V->GetActorLocation().ToString(),Travel,R->Heading);
                    }
                }
            }
        }
#if WITH_EDITOR
        R->LastTravelCm=Travel;
#endif
        if(D->Id==TEXT("fr.ft"))
        {
#if WITH_EDITOR
            if(FRRUVehicleNavCandidateV87::Requested())FRRUVehicleNavCandidateV87::ObserveMovement(B,V,Start,Travel,HaltForWeapon);
            else
#endif
                FRRUVehicleNavV87::ObserveMovement(B,V,Start,Travel,HaltForWeapon);
        }
        if(auto* Driver=R->Driver.Get())if(auto* Pose=R->Model.Get())
        {
            R->Phase=FMath::Fmod(R->Phase+(D->Rail&&R->RailReturning?-Travel:Travel)/60.f+2.4f,2.4f);Driver->SetPosition(R->Phase,false);Driver->TickAnimation(0,false);Driver->RefreshBoneTransforms();Pose->CopyPoseFromSkeletalComponent(Driver);Pose->SetBoneTransformByName(R->RootBone,R->RootRest,EBoneSpaces::ComponentSpace);
            if(D->Rail)for(int32 Car=0;Car<3;++Car)for(int32 Wheel=0;Wheel<(Car==1?3:2);++Wheel)for(int32 Side:{-1,1})
            {
                const FName Name=Bone(*D,*FString::Printf(TEXT("car_%d_wheel_%d_%d"),Car,Wheel,Side));
                FTransform WheelPose=Pose->GetBoneTransformByName(Name,EBoneSpaces::ComponentSpace);FVector P=WheelPose.GetTranslation();P.Y=-Side*71.75;
                const FVector NativeP(P.X,-P.Y,P.Z);const FVector W=V->GetActorTransform().TransformPosition(NativeP);const FVector Floor=V->GetActorTransform().InverseTransformPosition(FVector(W.X,W.Y,RailHeight(B,float(W.X))));
                P.Z=Floor.Z+44.;WheelPose.SetTranslation(P);Pose->SetBoneTransformByName(Name,WheelPose,EBoneSpaces::ComponentSpace);
            }
        }
        if(D->Rail)
        {
            // Passenger phases use the authoritative host elapsed delta. Pause
            // freezes the same clock; rejected physical steps spend no phase.
            const float Seconds=FMath::Max(0.f,B->Elapsed-R->LastPassengerHostAt);R->LastPassengerHostAt=B->Elapsed;
            const bool ClearToDepart=UpdatePassengers(V,Seconds);V->bRailUnloadedV75=R->CrewUnloaded;
            if(R->RailHold>=55&&ClearToDepart)R->RailReturning=true;
        }
        if(!V->bEntered)return;
        auto Aim=[&](const FMount& Mount,const FVector& Target,float& Yaw,float RateScale)
        {
            const FRotator Wanted=MountBearing(V,Mount,Target);
            const float Want=float(Wanted.Yaw),Delta=FMath::FindDeltaAngleDegrees(Mount.BaseYaw,Want);if(FMath::Abs(Delta)>Mount.Arc||Wanted.Pitch< -10||Wanted.Pitch>20)return false;
            Yaw=FMath::FixedTurn(Yaw,Want,DT*65*RateScale);
            if(auto* Pose=R->Model.Get())if(Mount.YawBone!=NAME_None)
            {
                FTransform YawPose=Mount.RestYaw;YawPose.SetRotation(FRotator(0,-Yaw,0).Quaternion()*Mount.RestYaw.GetRotation());Pose->SetBoneTransformByName(Mount.YawBone,YawPose,EBoneSpaces::ComponentSpace);Pose->RefreshBoneTransforms();
                FTransform PitchPose=Pose->GetBoneTransformByName(Mount.PitchBone,EBoneSpaces::ComponentSpace);FRotator PitchRotation=PitchPose.Rotator();PitchRotation.Pitch=Wanted.Pitch;PitchPose.SetRotation(PitchRotation.Quaternion());Pose->SetBoneTransformByName(Mount.PitchBone,PitchPose,EBoneSpaces::ComponentSpace);Pose->RefreshBoneTransforms();
            }
            return FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,Want))<8;
        };
        for(int32 I=0;I<R->Cannons.Num();++I)
        {
            V->CannonClock[I]=FMath::Max(0.f,V->CannonClock[I]-DT);FMountReadback* Readback=nullptr;
#if WITH_EDITOR
            FMountReadback CannonReadback;Readback=&CannonReadback;
#endif
            // Re-select from the committed pose, not the pre-movement sight.
            // The same actual torso point is selected, aimed and fired at.
            const auto Contact=SelectWeaponContact(V,*R,I,true,InfantrySight,Readback);
            if(Contact.Actor())
            {
                const bool Aligned=Aim(R->Cannons[I],Contact.Point,R->CannonYaw[I],R->Spec.CannonAimRateScale);
                const FVector From=Muzzle(V,I,true);const bool FinalLOS=Clear(V,Contact.Actor(),From,Contact.Point,Contact.Infantry?&InfantrySight:nullptr);
#if WITH_EDITOR
                CannonReadback.Muzzle=From;CannonReadback.YawAligned=Aligned;CannonReadback.FinalLOSChecked=true;CannonReadback.FinalLOSClear=FinalLOS;
#endif
                if(V->CannonClock[I]<=0&&Aligned&&FinalLOS)
                {
                    const int32 BeforeShots=V->CannonShots;
                    V->FireCannon(I,Contact.Point,Contact.Armour);V->CannonClock[I]=(D->Id==TEXT("fr.ft")?7.f:9.f)*R->Spec.CannonCycleScale;
                    if(V->CannonShots>BeforeShots&&R->CannonRecoilAge.IsValidIndex(I))R->CannonRecoilAge[I]=0;
#if WITH_EDITOR
                    CannonReadback.Fired=V->CannonShots>BeforeShots;
#endif
                }
            }
#if WITH_EDITOR
            if(R->WeaponReadbacks.Num()<13)R->WeaponReadbacks.Add(CannonReadback);
#endif
        }
        for(int32 I=0;I<R->Machineguns.Num();++I)
        {
            V->GunClock[I]=FMath::Max(0.f,V->GunClock[I]-DT);FMountReadback* Readback=nullptr;
#if WITH_EDITOR
            FMountReadback GunReadback;Readback=&GunReadback;
#endif
            const auto Contact=SelectWeaponContact(V,*R,I,false,InfantrySight,Readback);
            if(Contact.Infantry)
            {
                const bool Aligned=Aim(R->Machineguns[I],Contact.Point,R->MachinegunYaw[I],R->Spec.MachinegunAimRateScale);
                const FVector From=Muzzle(V,I,false);const bool FinalLOS=Clear(V,Contact.Infantry,From,Contact.Point,&InfantrySight);
#if WITH_EDITOR
                GunReadback.Muzzle=From;GunReadback.YawAligned=Aligned;GunReadback.FinalLOSChecked=true;GunReadback.FinalLOSClear=FinalLOS;
#endif
                if(V->GunClock[I]<=0&&Aligned&&FinalLOS)
                {
                    if(V->BurstRemaining[I]<=0)V->BurstRemaining[I]=V->Random.RandRange(7,11);
                    const int32 BeforeShots=V->MachinegunShots;
                    V->FireMachinegun(I,Contact.Infantry);--V->BurstRemaining[I];V->GunClock[I]=V->BurstRemaining[I]>0?.11f:V->Random.FRandRange(1.3f,2.2f)*R->Spec.MachinegunRecoveryScale;
                    if(V->MachinegunShots>BeforeShots&&R->GunRecoilAge.IsValidIndex(I))R->GunRecoilAge[I]=0;
#if WITH_EDITOR
                    GunReadback.Fired=V->MachinegunShots>BeforeShots;
#endif
                }
            }
#if WITH_EDITOR
            if(R->WeaponReadbacks.Num()<13)R->WeaponReadbacks.Add(GunReadback);
#endif
        }
        ApplyMechanicalOverlayV87(V,*R,false,DT);
        if(auto* Audio=B->GetWorld()->GetSubsystem<UFrontlineAudioSubsystem>())Audio->UpdateTankAudio(V->Serial+V->Team*1000,V->GetActorLocation(),FMath::Clamp(Travel/(DT*D->Speed),0.f,1.f),!V->IsImmobilized(),V->HullIntegrity<D->Hull*.45f,true);
    }
    static void Reset(UFrontlineVehicleSubsystem* F)
    {using namespace WarfareFRRUArmourV82;Fleets.Remove(F);for(auto I=Vehicles.CreateIterator();I;++I)if(!I.Key().IsValid()||I.Value().Battle.Get()==F->Battle())I.RemoveCurrent();}
    static void Enemy(UFrontlineVehicleSubsystem* F)
    {
        using namespace WarfareFRRUArmourV82;auto* B=F->Battle();if(!B||B->BattleOutroWinner!=INDEX_NONE)return;
        BindHostEncounter(F,B);
        auto& State=FleetForEncounterV87(B,F);
        if(B->Elapsed<State.EnemyAt)return;
        State.EnemyAt=B->Elapsed+5.f;const auto Choices=Roster(B,1);
        // Use the full legally dated roster. A fixed first element must not
        // starve a later attachment whenever the shared workshop becomes free.
        const int32 First=Choices.IsEmpty()?0:F->DeploymentCountV37[1]%Choices.Num();
        for(int32 Index=0;Index<Choices.Num();++Index)
            if(Recruit(B,Choices[(First+Index)%Choices.Num()],1))
            {
                float Earliest=MAX_flt;for(const auto& Pair:State.ModelReadyAt[1])Earliest=FMath::Min(Earliest,Pair.Value);
                State.EnemyAt=FMath::Max(B->Elapsed+.25f,Earliest);break;
            }
    }
#if WITH_EDITOR
#include "WarfareFRRUArmourGameplayReviewV87.inl"
#endif
};

namespace WarfareFRRUArmourV82
{
bool ReleaseOpeningReadinessV87(AWarfareFrontlineGameMode* B,int32 Side)
{
    if(!B||Side<0||Side>1||!WarfareFRRUCampaignV82::Active(B)||B->Phase!=EFrontlinePhase::Fighting
        ||B->BattleOutroWinner!=INDEX_NONE||Roster(B,Side).IsEmpty())return false;
    auto* F=UFrontlineVehicleSubsystem::Get(B->GetWorld());if(!F)return false;
    FFRRUArmourAccessV82::BindHostEncounter(F,B);
    auto& State=FleetForEncounterV87(B,F);
    // Host deployment count includes national actors and the dated German
    // host reserve. No call after an actual dispatch can erase its cooldown.
    if(State.OpeningReleasedV87[Side]||F->GetDeploymentCountV76(Side)!=0)return false;
    State.OpeningReleasedV87[Side]=true;State.ReadyAt[Side]=B->Elapsed;State.Cycle[Side]=0;
    for(auto& Pair:State.ModelReadyAt[Side])Pair.Value=B->Elapsed;
    for(auto& Pair:State.ModelCycle[Side])Pair.Value=0;
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_ARMOUR_OPENING_RESERVE encounter=%s side=%d hostElapsed=%.3f models=%d"),
        *B->GetEncounterId().ToString(),Side,B->Elapsed,State.ModelReadyAt[Side].Num());
    return true;
}
bool Recruit(AWarfareFrontlineGameMode* B,const FString& StableId,int32 Side)
{
    auto* F=B?UFrontlineVehicleSubsystem::Get(B->GetWorld()):nullptr;if(!F)return false;
    FFRRUArmourAccessV82::BindHostEncounter(F,B);
    const auto Reason=BlockReason(B,StableId,Side);if(!Reason.IsEmpty()){if(Side==0)B->LastMessage=Reason;return false;}
    const auto* D=Definition(StableId);if(!D)return false;auto* Dispatched=FFRRUArmourAccessV82::Spawn(F,B,StableId,Side);if(!Dispatched)return false;
    const float PreparationScale=Runtime(Dispatched)->Spec.PreparationScale;
    auto& S=FleetForEncounterV87(B,F);
    // Freeze preparation at dispatch, as the British/German workshop does.
    // Distinct model durations still share this one dispatch timestamp.
    S.ModelReadyAt[Side].Empty();S.ModelCycle[Side].Empty();
    for(const FString& Candidate:Roster(B,Side))if(const auto* Model=Definition(Candidate))
    {
        const float Duration=FrontlineGameplayV43::PreparationSeconds(Model->Cooldown*PreparationScale,Side,B->GetCampaignDifficulty());
        S.ModelCycle[Side].Add(Candidate,Duration);S.ModelReadyAt[Side].Add(Candidate,B->Elapsed+Duration);
    }
    S.Cycle[Side]=FrontlineGameplayV43::PreparationSeconds(D->Cooldown*PreparationScale,Side,B->GetCampaignDifficulty());S.ReadyAt[Side]=B->Elapsed+S.Cycle[Side];return true;
}
void Reset(UFrontlineVehicleSubsystem* F)
{
    FTGroundPilotV82::States.Empty();
    PreparedVehicleAssetsV82.Empty();
    if(F)FFRRUArmourAccessV82::Reset(F);
}
void TickEnemy(UFrontlineVehicleSubsystem* F){if(F)FFRRUArmourAccessV82::Enemy(F);}
void Simulate(AFrontlineTank* V,float DT){if(V&&FMath::IsFinite(DT)&&DT>0)FFRRUArmourAccessV82::Simulate(V,DT);}
#if WITH_EDITOR
TSharedRef<FJsonObject> GroundPilotDiagnosis(const AFrontlineTank* Vehicle)
{
    auto Out=FTGroundPilotV82::Diagnosis(Vehicle);const auto* R=Runtime(Vehicle);
    if(!Vehicle||!R)return Out;
    auto Appearance=MakeShared<FJsonObject>();Appearance->SetBoolField(TEXT("readOnly"),true);
    Appearance->SetStringField(TEXT("visiblePose"),GetPathNameSafe(R->Model.Get()));Appearance->SetNumberField(TEXT("derivedSlotsAtDispatch"),R->AppearanceMaterials);
    Appearance->SetNumberField(TEXT("recoilOverlayBones"),R->RecoilBones.Num());Appearance->SetNumberField(TEXT("wreckHatchBones"),R->HatchBones.Num());
    Appearance->SetStringField(TEXT("actualRecoilClip"),GetPathNameSafe(R->RecoilClip.Get()));Appearance->SetStringField(TEXT("actualServiceClip"),GetPathNameSafe(R->ServiceClip.Get()));
    Appearance->SetNumberField(TEXT("recoilFrames"),R->RecoilFrames);Appearance->SetNumberField(TEXT("lastRecoilCm"),R->LastRecoilCm);Appearance->SetNumberField(TEXT("maximumRecoilCm"),R->MaximumRecoilCm);Appearance->SetNumberField(TEXT("maximumMechanismDegrees"),R->MaximumMechanismAngle);Appearance->SetBoolField(TEXT("actualWreckHatchesApplied"),R->WreckPoseApplied);
    TArray<TSharedPtr<FJsonValue>> Materials;
    if(auto* Pose=R->Model.Get())for(int32 Slot=0;Slot<Pose->GetNumMaterials();++Slot)
    {
        auto Row=MakeShared<FJsonObject>();auto* Material=Pose->GetMaterial(Slot);Row->SetNumberField(TEXT("slot"),Slot);Row->SetStringField(TEXT("actualMaterial"),GetPathNameSafe(Material));
        Row->SetBoolField(TEXT("isDynamicInstance"),Material&&Material->IsA<UMaterialInstanceDynamic>());
        for(const TCHAR* Parameter:{TEXT("FieldDust"),TEXT("FieldRust"),TEXT("VehicleDamage"),TEXT("VehicleSeed")})
        {float Value=0;const bool Found=Material&&Material->GetScalarParameterValue(FMaterialParameterInfo(Parameter),Value);Row->SetBoolField(FString(Parameter)+TEXT("Connected"),Found);if(Found)Row->SetNumberField(Parameter,Value);}
        Materials.Add(MakeShared<FJsonValueObject>(Row));
    }
    Appearance->SetArrayField(TEXT("actualVisibleMaterials"),Materials);Out->SetObjectField(TEXT("actualNationalAppearanceV87"),Appearance);
    if(R->EngagementHostAt<0)return Out;
    auto Engagement=MakeShared<FJsonObject>();Engagement->SetBoolField(TEXT("readOnly"),true);
    Engagement->SetNumberField(TEXT("hostElapsed"),R->EngagementHostAt);
    const auto* Battle=R->Battle.Get();const float SampleAge=Battle?FMath::Max(0.f,Battle->Elapsed-R->EngagementHostAt):0.f;
    Engagement->SetNumberField(TEXT("sampleAgeHostSeconds"),SampleAge);
    Engagement->SetBoolField(TEXT("sampleIsCurrent"),Battle&&SampleAge<=.001f&&Vehicle->IsOperational()&&Battle->BattleOutroWinner==INDEX_NONE);
    Engagement->SetBoolField(TEXT("movementStoppedForWeapon"),R->HaltForWeapon);
    Engagement->SetBoolField(TEXT("hullAcquiringObservedArmour"),R->HullAcquiringArmour);
    Engagement->SetBoolField(TEXT("runningGearImmobilized"),Vehicle->IsImmobilized());
    Engagement->SetNumberField(TEXT("actualTravelCm"),R->LastTravelCm);
    auto Spec=MakeShared<FJsonObject>();Spec->SetBoolField(TEXT("nativeCrewOwnedAtDispatch"),R->Spec.Crew);Spec->SetBoolField(TEXT("nativeRepairOwnedAtDispatch"),R->Spec.Repair);Spec->SetBoolField(TEXT("nativePrepareOwnedAtDispatch"),R->Spec.Prepare);
    Spec->SetNumberField(TEXT("cannonCycleScale"),R->Spec.CannonCycleScale);Spec->SetNumberField(TEXT("cannonAimRateScale"),R->Spec.CannonAimRateScale);Spec->SetNumberField(TEXT("machinegunRecoveryScale"),R->Spec.MachinegunRecoveryScale);Spec->SetNumberField(TEXT("machinegunAimRateScale"),R->Spec.MachinegunAimRateScale);Spec->SetNumberField(TEXT("repairSeconds"),R->Spec.RepairSeconds);Spec->SetNumberField(TEXT("preparationScale"),R->Spec.PreparationScale);Spec->SetBoolField(TEXT("classicModifiersApplied"),false);
    Engagement->SetObjectField(TEXT("nativeEffectiveSpecV87"),Spec);
    auto Repair=MakeShared<FJsonObject>();Repair->SetNumberField(TEXT("side"),R->RepairSide);Repair->SetNumberField(TEXT("progressSeconds"),R->RepairProgress);Repair->SetNumberField(TEXT("requiredSeconds"),R->Spec.RepairSeconds);Repair->SetNumberField(TEXT("lastActualHostAt"),R->RepairLastHostAt);Repair->SetStringField(TEXT("actualWorker"),GetPathNameSafe(R->RepairWorker.Get()));Engagement->SetObjectField(TEXT("actualEngineerRepairV87"),Repair);
    const auto MountRow=[&](const FMountReadback& Item)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetBoolField(TEXT("cannon"),Item.Cannon);Row->SetNumberField(TEXT("mount"),Item.Mount);
        Row->SetBoolField(TEXT("enabled"),Item.Enabled);Row->SetBoolField(TEXT("selected"),Item.Selected);
        Row->SetStringField(TEXT("targetActor"),GetPathNameSafe(Item.Target.Get()));Row->SetNumberField(TEXT("targetSquad"),Item.TargetSquad);
        Row->SetNumberField(TEXT("targetMember"),Item.TargetMember);Row->SetNumberField(TEXT("targetSerial"),Item.TargetSerial);
        Row->SetStringField(TEXT("aimPoint"),Item.Point.ToString());Row->SetStringField(TEXT("actualMuzzle"),Item.Muzzle.ToString());
        Row->SetNumberField(TEXT("distanceCm"),Item.DistanceCm);Row->SetNumberField(TEXT("yawDeltaDegrees"),Item.YawDelta);Row->SetNumberField(TEXT("pitchDegrees"),Item.Pitch);
        Row->SetBoolField(TEXT("withinRange"),Item.WithinRange);Row->SetBoolField(TEXT("withinSector"),Item.WithinSector);Row->SetBoolField(TEXT("withinPitch"),Item.WithinPitch);
        Row->SetBoolField(TEXT("finalLOSChecked"),Item.FinalLOSChecked);Row->SetBoolField(TEXT("finalLOSClear"),Item.FinalLOSClear);
        Row->SetBoolField(TEXT("yawAligned"),Item.YawAligned);Row->SetBoolField(TEXT("fired"),Item.Fired);
        return Row;
    };
    if(R->HaltForWeapon)Engagement->SetObjectField(TEXT("stopContact"),MountRow(R->StopContact));
    TArray<TSharedPtr<FJsonValue>> MountReceipts;
    for(int32 Index=0;Index<R->WeaponReadbacks.Num()&&Index<13;++Index)MountReceipts.Add(MakeShared<FJsonValueObject>(MountRow(R->WeaponReadbacks[Index])));
    Engagement->SetArrayField(TEXT("mounts"),MountReceipts);Out->SetObjectField(TEXT("actualEngagement"),Engagement);return Out;
}
#endif
TSharedRef<FJsonObject> RunAssetChecks(AWarfareFrontlineGameMode* B)
{
    auto Out=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;auto Check=[&](const FString& Name,bool Good){auto C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Good);Checks.Add(MakeShared<FJsonValueObject>(C));};
    if(!B||!Review()){Check(TEXT("exact_isolated_world_review_required"),false);Out->SetArrayField(TEXT("checks"),Checks);Out->SetBoolField(TEXT("success"),false);return Out;}
    for(const auto& D:Definitions())
    {
        FString Error;const bool Ready=AssetsReady(D.Id,Error);Check(D.Id+TEXT(".actual_assets_rig_surfaces"),Ready);
        auto* V=B->GetWorld()->SpawnActor<AFrontlineTank>(FVector(0,0,0),FRotator::ZeroRotator);
        const bool Configured=V&&FFRRUArmourAccessV82::Configure(V,B,D.Id,0,9000+Checks.Num());Check(D.Id+TEXT(".distinct_stable_model"),Configured&&V->Type==EFrontlineVehicle::Count&&Id(V)==D.Id);
        if(Configured)
        {
            auto* R=Runtime(V);Check(D.Id+TEXT(".hull_query_and_root_once"),R&&R->Hull.IsValid()&&R->Model.IsValid()&&R->RootRest.GetTranslation().Equals(FVector::ZeroVector,.01));
            Check(D.Id+TEXT(".historical_mounts"),R&&(D.Rail?R->Cannons.IsEmpty()&&R->Machineguns.Num()==12:D.Wheeled?R->Cannons.IsEmpty()&&R->Machineguns.Num()==2:D.Id==TEXT("fr.ft")?R->Cannons.Num()==1&&R->Machineguns.IsEmpty():R->Cannons.Num()==1));
            const FVector Before=V->GetActorLocation();if(auto* Driver=R->Driver.Get())if(auto* Pose=R->Model.Get()){Driver->SetPosition(2.f,false);Driver->TickAnimation(0,false);Driver->RefreshBoneTransforms();Pose->CopyPoseFromSkeletalComponent(Driver);Pose->SetBoneTransformByName(R->RootBone,R->RootRest,EBoneSpaces::ComponentSpace);Check(D.Id+TEXT(".drive_pose_does_not_translate_actor"),V->GetActorLocation()==Before&&Pose->GetBoneLocationByName(R->RootBone,EBoneSpaces::ComponentSpace).Equals(R->RootRest.GetTranslation(),.01));}
        }
        if(V){Vehicles.Remove(V);V->Destroy();}
    }
    int32 Passed=0;for(const auto& C:Checks)Passed+=C->AsObject()->GetBoolField(TEXT("passed"))?1:0;Out->SetArrayField(TEXT("checks"),Checks);Out->SetNumberField(TEXT("passed"),Passed);Out->SetNumberField(TEXT("total"),Checks.Num());Out->SetBoolField(TEXT("success"),Passed==Checks.Num());Out->SetBoolField(TEXT("artAccepted"),false);Out->SetBoolField(TEXT("deploymentAccepted"),false);return Out;
}
}

#if WITH_EDITOR
namespace WarfareFRRUArmourV82
{
namespace
{
FAutoConsoleCommand ArmourReview(TEXT("WF.FRRU.ReviewArmourV82"),TEXT("Exact isolated real five-vehicle rig/root candidate checks."),FConsoleCommandDelegate::CreateLambda([]()
{
    if(!Review()){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_ARMOUR_REVIEW_REFUSED"));return;}
    static bool Started=false;if(Started)return;Started=true;const double Begin=FPlatformTime::Seconds();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Begin](float)
    {
        if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* W=Context.World();W&&W->WorldType==EWorldType::Game)
            if(auto* B=W->GetAuthGameMode<AWarfareFrontlineGameMode>();B&&B->HasActorBegunPlay()&&B->IsProgressionVolatile())
            {
                auto Report=RunAssetChecks(B);Report->SetStringField(TEXT("revision"),TEXT("v82.frru.armour.candidate.1"));
                Report->SetBoolField(TEXT("combatAccepted"),false);Report->SetBoolField(TEXT("shippingAccepted"),false);
                FString User,Json;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
                const FString Path=FPaths::Combine(User,TEXT("FRRUReview/armour-assets.json"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
                const bool Written=FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json))&&FFileHelper::SaveStringToFile(Json,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
                UE_LOG(LogTemp,Display,TEXT("WF_FRRU_ARMOUR_REVIEW success=%d receipt=%s"),Report->GetBoolField(TEXT("success")),*Path);
                FPlatformMisc::RequestExitWithStatus(false,Written&&Report->GetBoolField(TEXT("success"))?0:3);return false;
            }
        if(FPlatformTime::Seconds()-Begin>45){FPlatformMisc::RequestExitWithStatus(false,4);return false;}return true;
    }));
}));
FAutoConsoleCommand ArmourGameplayReviewV87(TEXT("WF.FRRU.ReviewArmourGameplayV87"),TEXT("Isolated real national armour acquisition, readiness, wreck and train checks."),FConsoleCommandDelegate::CreateLambda([]()
{
    if(!Review()||!FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87ArmourGameplayReview")))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V87_ARMOUR_GAMEPLAY_REFUSED"));return;}
    static bool Started=false;if(Started)return;Started=true;
    auto Probe=MakeShared<FFRRUArmourAccessV82::FGameplayReviewV87>();
    FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Probe->User);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float){return Probe->Tick();}));
}));
}
}
#endif
