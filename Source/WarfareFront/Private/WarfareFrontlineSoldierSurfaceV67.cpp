#include "WarfareFrontlineSoldierSurfaceV67.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
bool Enabled(){return !FParse::Param(FCommandLine::Get(),TEXT("WFV67Baseline"));}
UMaterialInterface* LoadSurfaceMaterialV8219(const FString& Path)
{
#if WITH_EDITOR
    // Imports can create a previously absent correction, or replace its object.
    // Editor/commandlet lookups deliberately have no persistent hit/miss cache.
    return LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn);
#else
    // Deployment resolves the same few optional correction paths for every
    // soldier. Keep successful objects alive and remember absent packages.
    static TMap<FString,TStrongObjectPtr<UMaterialInterface>> Materials;
    static TSet<FString> Missing;
    if(const auto* Found=Materials.Find(Path))return Found->Get();
    if(Missing.Contains(Path))return nullptr;
    auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn);
    if(Material)Materials.Add(Path,TStrongObjectPtr<UMaterialInterface>(Material));
    else Missing.Add(Path);
    UE_LOG(LogTemp,Display,TEXT("WF_V8219_SURFACE_LOOKUP found=%d path=%s"),Material!=nullptr,*Path);
    return Material;
#endif
}
}
FString FrontlineSoldierSurfaceV67::SourceMeshName(const FString& Name)
{
    // V68 bodies share the same V34/V41 weapon, casualty and specialist contracts.
    if(!Name.EndsWith(TEXT("_V67"))&&!Name.EndsWith(TEXT("_V68")))return Name;
    if(Name.Contains(TEXT("British_RifleGrenadier_"))||Name.Contains(TEXT("German_AntiTankRifle_")))
        return Name.LeftChop(4)+TEXT("_V41");
    return Name.LeftChop(4)+TEXT("_V34");
}
USkeletalMesh* FrontlineSoldierSurfaceV67::ResolveMesh(USkeletalMesh* Source)
{
    if(!Enabled()||!Source)return Source;
    const FString Original=Source->GetName();
    // Fitted masks are separate, accepted equipment, not tailored bodies.
    if(Original.Contains(TEXT("_Respirator_")))return Source;
    const bool Specialist=(Original==TEXT("SK_British_RifleGrenadier_V41")||Original==TEXT("SK_German_AntiTankRifle_V41"))&&Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V41/"));
    if(!Specialist&&(!Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V34/"))||!Original.EndsWith(TEXT("_V34"))))return Source;
    const bool Casualty=Original.StartsWith(TEXT("SK_Casualty_"));
    FString LivingName=Original.Replace(TEXT("SK_Casualty_"),TEXT("SK_"));LivingName=LivingName.LeftChop(4)+TEXT("_V67");
    const FString CasualtyName=LivingName.Replace(TEXT("SK_"),TEXT("SK_Casualty_"));
    const FString Root=TEXT("/Game/Warfare/Characters/V67/");
    auto* Living=LoadObject<USkeletalMesh>(nullptr,*(Root+TEXT("Meshes/")+LivingName+TEXT(".")+LivingName));
    auto* Dead=LoadObject<USkeletalMesh>(nullptr,*(Root+TEXT("Casualties/")+CasualtyName+TEXT(".")+CasualtyName));
    if(Living&&Dead&&Living->GetSkeleton()==Source->GetSkeleton()&&Dead->GetSkeleton()==Source->GetSkeleton())return Casualty?Dead:Living;
    static TSet<FString> Reported;if(!Reported.Contains(Original))
    {Reported.Add(Original);UE_LOG(LogTemp,Warning,TEXT("WF_V67_WARDROBE_FALLBACK missing_pair_or_skeleton=%s"),*Original);}
    return Source;
}
UMaterialInterface* FrontlineSoldierSurfaceV67::ResolveMaterial(UMaterialInterface* Source)
{
    // V84 preserves the current V81 blood/V75 cloth parent graphs in isolated
    // receivers. No optional derivative changes the mesh, pose or equipment.
    if(Enabled()&&Source&&!FParse::Param(FCommandLine::Get(),TEXT("WFV84MaterialBaseline"))
       &&(Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/"))
          ||Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V67/Materials/"))
          ||Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V75/Materials/"))
          ||Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V81/Materials/"))))
    {
        const FString Name=Source->GetName();
        if(auto* Material=LoadSurfaceMaterialV8219(TEXT("/Game/Warfare/Characters/V84/Materials/")+Name+TEXT(".")+Name))
        {
            static TSet<FString> Reported;
            if(!Reported.Contains(Name)){Reported.Add(Name);UE_LOG(LogTemp,Display,TEXT("WF_V84_UNIT_MATERIAL_BIND source=%s candidate=%s"),*Source->GetPathName(),*Material->GetPathName());}
            return Material;
        }
    }
    if(Enabled()&&Source&&(Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V67/Materials/"))||Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/"))||Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V75/Materials/"))))
    {
        const FString Name=Source->GetName();
        // Only actually bound graphs with missed purple blood have a V81 copy.
        // Each MI retains its parameter overrides and its corrected parent chain.
        if(auto* Blood=LoadSurfaceMaterialV8219(TEXT("/Game/Warfare/Characters/V81/Materials/")+Name+TEXT(".")+Name))
        {
            static TSet<FString> Reported;
            if(!Reported.Contains(Source->GetPathName()))
            {
                Reported.Add(Source->GetPathName());
                UE_LOG(LogTemp,Display,TEXT("WF_V81_BLOOD_BIND source=%s corrected=%s"),*Source->GetPathName(),*Blood->GetPathName());
            }
            return Blood;
        }
        if(auto* Corrected=LoadSurfaceMaterialV8219(TEXT("/Game/Warfare/Characters/V75/Materials/")+Name+TEXT(".")+Name))return Corrected;
    }
    if(!Enabled()||!Source||!Source->GetPathName().StartsWith(TEXT("/Game/Warfare/Characters/V34/Materials/")))return Source;
    const FString Name=Source->GetName();
    if(!Name.Contains(TEXT("Cloth"))&&!Name.Contains(TEXT("Canvas"))&&!Name.Contains(TEXT("Leather"))&&
       !Name.Contains(TEXT("Puttee"))&&!Name.Contains(TEXT("Jackboot"))&&!Name.Contains(TEXT("Helmet")))return Source;
    const FString Path=TEXT("/Game/Warfare/Characters/V67/Materials/")+Name+TEXT(".")+Name;
    if(auto* Material=LoadSurfaceMaterialV8219(Path))return Material;
    return Source;
}
