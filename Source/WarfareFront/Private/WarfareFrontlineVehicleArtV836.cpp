#include "WarfareFrontlineVehicleArtV836.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "WarfareFrontlineRenderSettings.h"
#include "WarfareFrontlineVisualV85.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
namespace FrontlineVehicleArtV836
{
FString EffectiveHullMaterialKeyV8310(const FString& Model,const FString& Key)
{
    // These are the inherited empty pair of white geometry strokes, not
    // authored serials or the German recognition cross. The actual crew
    // identity remains on its independent, weathered stencil receiver.
    if(Key==TEXT("White")&&(Model==TEXT("MarkIVMale")||Model==TEXT("MarkIVFemale")
        ||Model==TEXT("MarkVMale")||Model==TEXT("MarkVStar")||Model==TEXT("Whippet")))return TEXT("British");
    // Austin's White slot is its lamp glass; German paint and train crosses
    // are separate receivers and must keep their existing pigments.
    return Key;
}

// Root's disposable focused review calls the real public definitions and the
// same material resolver used by live/wreck actors. No simulation or save edit.
void FrontlineRunArmourBalanceChecksV84412(TFunctionRef<void(const FString&,bool)> Check)
{
    const float Speeds[]={130.f,215.6f,145.f,120.f,138.f,125.f,165.f,138.f,145.f,125.f,145.f,308.f,152.f};
    const float Hulls[]={700.f,357.5f,810.f,630.f,680.f,630.f,760.f,680.f,760.f,630.f,840.f,268.125f,2900.f};
    const float Cooldowns[]={104.f,72.f,116.f,108.f,82.f,86.f,132.f,66.f,94.f,70.f,142.f,54.f,145.f};
    static_assert(UE_ARRAY_COUNT(Speeds)==int32(EFrontlineVehicle::Count),"Every playable vehicle has a base-speed witness");
    for(int32 I=0;I<int32(EFrontlineVehicle::Count);++I)
    {
        const auto& D=UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle(I));
        const FString Prefix=FString::Printf(TEXT("v84412_vehicle_%d_"),I);
        Check(Prefix+TEXT("latest_base_speed_scaled_once_80_percent"),FMath::IsNearlyEqual(D.SpeedCm,Speeds[I],.0001f));
        Check(Prefix+TEXT("latest_base_hull_scaled_once_125_percent"),FMath::IsNearlyEqual(D.Hull,Hulls[I],.0001f));
        Check(Prefix+TEXT("readiness_retains_model_specific_value"),FMath::IsNearlyEqual(D.Cooldown,Cooldowns[I],.0001f));
    }
    const TCHAR* Models[]={TEXT("MarkVMale"),TEXT("MarkVStar"),TEXT("A7V"),TEXT("A7VGretchen"),TEXT("BeuteMarkIV"),TEXT("BeuteMarkIVFemale")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Models);++I)
    {
        const FString Key=I<2?TEXT("British"):TEXT("German");
        auto* Live=FrontlineVehicleArtV836::ResolveModelMaterialV838(Models[I],Key);
        auto* Wreck=FrontlineVehicleArtV836::ResolveWreckMaterialV838(Models[I],Key);
        const TCHAR* Namespace=I==1&&!FrontlineVisualV85::Baseline()?TEXT("/Game/Warfare/Vehicles/V85/Materials/"):TEXT("/Game/Warfare/Vehicles/V84413/Materials/");
        Check(FString(TEXT("v84412_"))+Models[I]+TEXT("_actual_live_coating_derivative_bound"),Live&&Live->GetPathName().StartsWith(Namespace));
        Check(FString(TEXT("v84412_"))+Models[I]+TEXT("_matching_wreck_coating_retains_own_derivative"),Wreck&&Wreck->GetPathName().StartsWith(Namespace));
    }
    Check(TEXT("v84412_pioneer_aliases_keep_base_platform_hull_and_speed"),
        UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::MarkIVPioneer).Hull==UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::MarkIVFemale).Hull
        &&UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::BeutePioneer).SpeedCm==UFrontlineVehicleSubsystem::Definition(EFrontlineVehicle::BeuteMarkIVFemale).SpeedCm);
}
UStaticMesh* ResolveMesh(const FString& Name)
{
    static const TSet<FString> V838={TEXT("SM_MarkIVMale"),TEXT("SM_MarkIVFemale"),TEXT("SM_MarkVMale"),TEXT("SM_MarkVStar"),
        TEXT("SM_Whippet"),TEXT("SM_Austin1918"),TEXT("SM_A7V"),TEXT("SM_A7VGretchen"),TEXT("SM_BeuteMarkIV"),TEXT("SM_BeuteMarkIVFemale"),
        TEXT("SM_AustinWheel"),TEXT("SM_AustinTurret")};
    if(V838.Contains(Name))
    {
        const FString Asset=TEXT("/Game/Warfare/Vehicles/V838/")+Name+TEXT("_V838");
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Asset+TEXT(".")+Name+TEXT("_V838"))))return Mesh;
    }
    if(Name.EndsWith(TEXT("_Wreck"))&&V838.Contains(Name.LeftChop(6)))
    {
        const FString Asset=TEXT("/Game/Warfare/Vehicles/V838/WreckMeshes/")+Name+TEXT("_V838");
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Asset+TEXT(".")+Name+TEXT("_V838"))))return Mesh;
        const FString Old=TEXT("/Game/Warfare/Vehicles/V836/WreckMeshes/")+Name+TEXT("_V836");
        return LoadObject<UStaticMesh>(nullptr,*(Old+TEXT(".")+Name+TEXT("_V836")));
    }
    static const TSet<FString> V837={TEXT("SM_RailConvoy"),TEXT("SM_RailBogieWheel"),TEXT("SM_RailDoor"),TEXT("SM_RailGunCupola"),TEXT("SM_RailCannon"),
        TEXT("SM_MarkIVMale"),TEXT("SM_MarkIVFemale"),TEXT("SM_MarkVMale"),TEXT("SM_Whippet"),TEXT("SM_A7V"),TEXT("SM_A7VGretchen"),
        TEXT("SM_BeuteMarkIV"),TEXT("SM_BeuteMarkIVFemale"),TEXT("SM_MarkVStar"),TEXT("SM_Austin1918"),TEXT("SM_RailConvoy_Wreck")};
    if(V837.Contains(Name))
    {
        const FString Asset=TEXT("/Game/Warfare/Vehicles/V837/")+Name+TEXT("_V837");
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Asset+TEXT(".")+Name+TEXT("_V837"))))return Mesh;
    }
    static const TSet<FString> Candidates={TEXT("SM_MarkIVMale"),TEXT("SM_MarkIVFemale"),TEXT("SM_MarkVMale"),TEXT("SM_Whippet"),
        TEXT("SM_A7V"),TEXT("SM_A7VGretchen"),TEXT("SM_BeuteMarkIV"),TEXT("SM_BeuteMarkIVFemale"),TEXT("SM_MarkVStar"),TEXT("SM_Austin1918"),
        TEXT("SM_AustinWheel"),TEXT("SM_AustinTurret"),TEXT("SM_TrackShoe"),TEXT("SM_RailConvoy"),TEXT("SM_RailWheel"),TEXT("SM_RailCouplingRod")};
    if(!Candidates.Contains(Name))return nullptr;
    const FString Asset=TEXT("/Game/Warfare/Vehicles/V836/")+Name+TEXT("_V836");
    return LoadObject<UStaticMesh>(nullptr,*(Asset+TEXT(".")+Name+TEXT("_V836")));
}
UMaterialInterface* ResolveMaterial(const FString& Key)
{
    const FString Name=Key.StartsWith(TEXT("M_Vehicle_"))?Key:TEXT("M_Vehicle_")+Key;
    // Independent recognition pigments on the retained rail paint geometry.
    // Model-specific lamps/glass are resolved before this generic fallback.
    if(Name==TEXT("M_Vehicle_Black")||Name==TEXT("M_Vehicle_White"))
    {
        const FString Target=TEXT("/Game/Warfare/Vehicles/V84413/Materials/")+Name+TEXT("_V84413");
        if(auto* Paint=LoadObject<UMaterialInterface>(nullptr,*(Target+TEXT(".")+Name+TEXT("_V84413")),nullptr,LOAD_NoWarn|LOAD_Quiet))return Paint;
    }
    if(Name==TEXT("M_Vehicle_Stencil")&&!FParse::Param(FCommandLine::Get(),TEXT("WFV842StencilBaseline")))
        if(auto* Stable=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Vehicles/V842/Materials/M_Vehicle_Stencil_V842.M_Vehicle_Stencil_V842"),nullptr,LOAD_NoWarn|LOAD_Quiet))return Stable;
    const FString NewTarget=TEXT("/Game/Warfare/Vehicles/V837/Materials/")+Name;
    if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*(NewTarget+TEXT(".")+Name)))return Material;
    const FString Target=TEXT("/Game/Warfare/Vehicles/V836/Materials/")+Name;
    return LoadObject<UMaterialInterface>(nullptr,*(Target+TEXT(".")+Name));
}
UMaterialInterface* ResolveModelMaterialV838(const FString& Model,const FString& Key)
{
    if(Model.IsEmpty()||Key.IsEmpty())return nullptr;
    const FString Effective=EffectiveHullMaterialKeyV8310(Model,Key);
    if(Model==TEXT("MarkVStar")&&!FrontlineVisualV85::Baseline()&&(Effective==TEXT("British")||Effective==TEXT("Steel")))
    {
        const FString Name=TEXT("M_MarkVStar_")+Effective+TEXT("_V85");
        const FString Path=TEXT("/Game/Warfare/Vehicles/V85/Materials/")+Name+TEXT(".")+Name;
        if(auto* Paint=LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet))return Paint;
    }
    static const TSet<FString> Finished={TEXT("MarkVMale"),TEXT("MarkVStar"),TEXT("A7V"),TEXT("A7VGretchen"),TEXT("BeuteMarkIV"),TEXT("BeuteMarkIVFemale")};
    if(Finished.Contains(Model)&&(Effective==TEXT("British")||Effective==TEXT("German")||Effective==TEXT("Steel")))
    {
        const FString Name=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_V84413");
        const FString Path=TEXT("/Game/Warfare/Vehicles/V84413/Materials/")+Name+TEXT(".")+Name;
        if(auto* Paint=LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet))return Paint;
    }
    if(!FParse::Param(FCommandLine::Get(),TEXT("WFV84MaterialBaseline"))
        &&(Model==TEXT("MarkIVMale")||Model==TEXT("A7V"))
        &&(Effective==TEXT("British")||Effective==TEXT("German")||Effective==TEXT("Steel")))
    {
        const FString CandidateName=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_V84");
        const FString CandidatePath=TEXT("/Game/Warfare/Vehicles/V84/Materials/")+CandidateName+TEXT(".")+CandidateName;
        if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*CandidatePath,nullptr,LOAD_NoWarn|LOAD_Quiet))return Material;
    }
    if(Model!=TEXT("AustinWheel")&&Model!=TEXT("AustinTurret")
        &&(Effective==TEXT("British")||Effective==TEXT("German")||Effective==TEXT("Steel")))
    {
        const FString NewName=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_V8310");
        const FString NewPath=TEXT("/Game/Warfare/Vehicles/V8310/Materials/")+NewName+TEXT(".")+NewName;
        if(auto* Corrected=LoadObject<UMaterialInterface>(nullptr,*NewPath))return Corrected;
    }
    const FString Name=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_V838");
    const FString Path=TEXT("/Game/Warfare/Vehicles/V838/Materials/")+Name+TEXT(".")+Name;
    return LoadObject<UMaterialInterface>(nullptr,*Path);
}
UMaterialInterface* ResolveWreckMaterialV838(const FString& Model,const FString& Key)
{
    if(Model.IsEmpty()||Key.IsEmpty())return nullptr;
    const FString Effective=EffectiveHullMaterialKeyV8310(Model,Key);
    if(Model==TEXT("MarkVStar")&&!FrontlineVisualV85::Baseline()&&(Effective==TEXT("British")||Effective==TEXT("Steel")))
    {
        const FString Name=TEXT("M_MarkVStar_")+Effective+TEXT("_Wreck_V85");
        const FString Path=TEXT("/Game/Warfare/Vehicles/V85/Materials/")+Name+TEXT(".")+Name;
        if(auto* Paint=LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet))return Paint;
    }
    static const TSet<FString> Finished={TEXT("MarkVMale"),TEXT("MarkVStar"),TEXT("A7V"),TEXT("A7VGretchen"),TEXT("BeuteMarkIV"),TEXT("BeuteMarkIVFemale")};
    if(Finished.Contains(Model)&&(Effective==TEXT("British")||Effective==TEXT("German")||Effective==TEXT("Steel")))
    {
        const FString Name=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_Wreck_V84413");
        const FString Path=TEXT("/Game/Warfare/Vehicles/V84413/Materials/")+Name+TEXT(".")+Name;
        if(auto* Paint=LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet))return Paint;
    }
    if(Model!=TEXT("AustinWheel")&&Model!=TEXT("AustinTurret")
        &&(Effective==TEXT("British")||Effective==TEXT("German")||Effective==TEXT("Steel")))
    {
        const FString NewName=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_Wreck_V8310");
        const FString NewPath=TEXT("/Game/Warfare/Vehicles/V8310/Materials/")+NewName+TEXT(".")+NewName;
        if(auto* Corrected=LoadObject<UMaterialInterface>(nullptr,*NewPath))return Corrected;
    }
    const FString Name=TEXT("M_")+Model+TEXT("_")+Effective+TEXT("_Wreck_V838");
    const FString Path=TEXT("/Game/Warfare/Vehicles/V838/Materials/")+Name+TEXT(".")+Name;
    return LoadObject<UMaterialInterface>(nullptr,*Path);
}
void HoldMaterialDetailV837(UMaterialInterface* Material)
{
    if(!Material||UFrontlineRenderSettingsSubsystem::EffectsBudgetV43()<.95f)return;
    static TMap<TWeakObjectPtr<UMaterialInterface>,double> Next;
    const double Now=FPlatformTime::Seconds();const TWeakObjectPtr<UMaterialInterface> Key(Material);
    if(const double* At=Next.Find(Key);At&&Now<*At)return;
    Next.Add(Key,Now+10.);
    TArray<UTexture*> Textures;Material->GetUsedTextures(Textures);
    for(UTexture* Texture:Textures)if(auto* T=Cast<UTexture2D>(Texture))
        if(T->GetSizeX()>=1024&&T->GetNumMips()>1)T->SetForceMipLevelsToBeResident(30.f);
    if(Next.Num()>256)for(auto It=Next.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
}
