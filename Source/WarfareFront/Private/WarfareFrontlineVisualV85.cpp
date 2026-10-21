#include "WarfareFrontlineVisualV85.h"
#include "WarfareFrontline.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "WarfareFrontlineParticulateV84412.h"
#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFrontlineReviewPathsV8221.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

struct FFrontlineVisualV85Access
{
    static void Replace(AFrontlineTank* Tank,UMaterialInstanceDynamic* Previous,UMaterialInstanceDynamic* Next)
    {for(auto& Material:Tank->BodyMaterials)if(Material==Previous)Material=Next;}
    static void Place(AFrontlineTank* Tank,const FVector& Position)
    {Tank->SetActorTickEnabled(false);Tank->bEntered=true;Tank->HeadingYawV37=0;Tank->SetActorRotation(FRotator::ZeroRotator);Tank->FitGroundV35(Position,0);}
};

namespace FrontlineVisualV85
{
namespace { bool UseBaseline=false; }
bool Allowed(const AWarfareFrontlineGameMode* B)
{
    if(!B||!B->GetWorld()||!B->IsProgressionVolatile()
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFV85Focused"))
        ||!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave")))return false;
    FString Dir;if(!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir)||Dir.IsEmpty()
        ||FPaths::IsRelative(Dir)||Dir.Contains(TEXT("..")))return false;
    FPaths::NormalizeDirectoryName(Dir);if(!FPaths::CollapseRelativeDirectories(Dir))return false;
    FString Root=FrontlineReviewPathsV8221::HomePath(TEXT("Documents/Codex/2026-09-06/wa/outputs/warfare-frru-validation/Saved/BugAudit/V85/Runtime"));
    FPaths::NormalizeDirectoryName(Root);const FString Prefix=Root+TEXT("/");
    if(!Dir.StartsWith(Prefix,ESearchCase::IgnoreCase))return false;
    const FString Stamp=Dir.Mid(Prefix.Len());
    return Stamp.Len()==19&&Stamp[8]==TEXT('-')&&Stamp[15]==TEXT('-')
        &&Stamp.Left(8).IsNumeric()&&Stamp.Mid(9,6).IsNumeric()&&Stamp.Right(3).IsNumeric();
}
bool SetBaseline(const AWarfareFrontlineGameMode* B,bool Value)
{
    if(!Allowed(B))return false;UseBaseline=Value;return true;
}
bool Baseline(){return UseBaseline;}
float GasOpticalGain(UMaterialInterface* Material)
{
    const auto* Base=Material?Material->GetBaseMaterial():nullptr;
    // #06 exposed the transported rolls only as faint ground mist. This gain
    // raises both view extinction and the existing whole-envelope sun shadow;
    // the original 96^3 density, finite support and producer state stay fixed.
    return Base&&Base->GetPathName()==TEXT("/Game/Warfare/Field/V85/Materials/M_GasVolume_V85.M_GasVolume_V85")?3.f:1.f;
}
bool PlaceFrozenTank(AWarfareFrontlineGameMode* B,AFrontlineTank* Tank,const FVector& Position)
{if(!Allowed(B)||!IsValid(Tank))return false;FFrontlineVisualV85Access::Place(Tank,Position);return true;}
FRebindResult RebindFrozen(AWarfareFrontlineGameMode* B,bool Old)
{
    FRebindResult R;R.Allowed=SetBaseline(B,Old);if(!R.Allowed)return R;bool Ready=true;
    for(TActorIterator<AActor> It(B->GetWorld());It;++It)
    {
        // Refuse changing a live mission or a vehicle while it is simulating.
        if(It->IsActorTickEnabled())continue;
        TArray<UStaticMeshComponent*> Parts;It->GetComponents<UStaticMeshComponent>(Parts);
        for(auto* C:Parts)for(int32 Slot=0;C&&Slot<C->GetNumMaterials();++Slot)
        {
            auto* Previous=Cast<UMaterialInstanceDynamic>(C->GetMaterial(Slot));
            const auto* Base=Previous?Previous->GetBaseMaterial():nullptr;if(!Base)continue;
            const FString Path=Base->GetPathName();
            const bool Gas=Path==TEXT("/Game/Warfare/Field/V85/Materials/M_GasVolume_V85.M_GasVolume_V85")
                ||Path==TEXT("/Game/Warfare/Field/V84413/Materials/M_GasVolume_V84413.M_GasVolume_V84413");
            const bool Armour=Path.StartsWith(TEXT("/Game/Warfare/Vehicles/V85/Materials/M_MarkVStar_"))
                ||Path.StartsWith(TEXT("/Game/Warfare/Vehicles/V84413/Materials/M_MarkVStar_"));
            if(!Gas&&!Armour)continue;
            // German is not an authored V85 key and keeps its previous parent.
            if(Armour&&!Path.Contains(TEXT("_British_"))&&!Path.Contains(TEXT("_Steel_")))continue;
            FString Target=Path;
            Target.ReplaceInline(Old?TEXT("/V85/"):TEXT("/V84413/"),Old?TEXT("/V84413/"):TEXT("/V85/"));
            Target.ReplaceInline(Old?TEXT("_V85"):TEXT("_V84413"),Old?TEXT("_V84413"):TEXT("_V85"));
            auto* Parent=LoadObject<UMaterialInterface>(nullptr,*Target);
            if(!Parent){Ready=false;continue;}
            auto* MID=UMaterialInstanceDynamic::Create(Parent,C);
            // Copy only host state. CopyInterpParameters also copies texture
            // overrides, which could silently replace the candidate density.
            // Sampling/optical kernels and default textures belong to Parent.
            const auto Scalar=[&](const TCHAR* Name)
            {float Value=0;if(Previous->GetScalarParameterValue(FMaterialParameterInfo(Name),Value)&&FMath::IsFinite(Value))MID->SetScalarParameterValue(Name,Value);};
            const auto Vector=[&](const TCHAR* Name)
            {FLinearColor Value;if(Previous->GetVectorParameterValue(FMaterialParameterInfo(Name),Value)&&FMath::IsFinite(Value.R)&&FMath::IsFinite(Value.G)&&FMath::IsFinite(Value.B)&&FMath::IsFinite(Value.A))MID->SetVectorParameterValue(Name,Value);};
            if(Gas)
            {
                for(const TCHAR* Name:{TEXT("Age"),TEXT("Seed")})Scalar(Name);
                // Previous Density already includes its parent's optical gain.
                // Recover the host value before applying the target algorithm,
                // so repeated A/B switching and a fresh EmitGas agree exactly.
                float Density=0.f;
                if(Previous->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Density")),Density)&&FMath::IsFinite(Density))
                    MID->SetScalarParameterValue(TEXT("Density"),FMath::Clamp(Density/GasOpticalGain(Previous)*GasOpticalGain(MID),0.f,4.f));
                for(const TCHAR* Name:{TEXT("VolumeCenter"),TEXT("VolumeRadii"),TEXT("JetRadii"),TEXT("ViewForward"),TEXT("EjectionAxis"),TEXT("DustColor")})Vector(Name);
                MID->SetScalarParameterValue(TEXT("IncidentMode"),2.f);
                FrontlineParticulateV84412::ApplySceneLighting(B->GetWorld(),MID);
            }
            else
            {
                for(const TCHAR* Name:{TEXT("DeploymentSeed"),TEXT("FieldDust"),TEXT("FieldRust"),TEXT("VehicleDamage"),
                    TEXT("WreckStateV8220"),TEXT("WreckHalfLengthV8220"),TEXT("WreckRoofV8220"),TEXT("WreckPatternSeedV8220")})Scalar(Name);
                for(const TCHAR* Name:{TEXT("WreckOriginV8220"),TEXT("WreckForwardV8220"),TEXT("WreckRightV8220"),TEXT("WreckUpV8220")})Vector(Name);
            }
            C->SetMaterial(Slot,MID);
            // Frozen review actors skip the ordinary armour Tick that requests
            // detail mips. Apply the same production request to both A/B sides.
            if(Armour)FrontlineVehicleArtV836::HoldMaterialDetailV837(MID);
            if(auto* Tank=Cast<AFrontlineTank>(*It))FFrontlineVisualV85Access::Replace(Tank,Previous,MID);
            Gas?++R.Gas:++R.Armour;
        }
    }
    R.Ready=Ready&&(R.Gas>0||R.Armour>0);return R;
}
}
