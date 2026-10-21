#include "WarfareFrontlineCharacterV29.h"
#include "WarfareFrontlineCharacterV34.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
TAutoConsoleVariable<int32> GCharactersV29(TEXT("wf.Characters.V29"),1,TEXT("Use refined infantry meshes when available. New spawns only."));
}
USkeletalMesh* FrontlineCharacterV29::Load(const FString& OriginalPath)
{
    if(auto* Reconstructed=FrontlineCharacterV34::TryLoad(OriginalPath))return Reconstructed;
    if(GCharactersV29.GetValueOnGameThread()!=0 && !FParse::Param(FCommandLine::Get(),TEXT("WFV29Baseline")))
    {
        FString Name=OriginalPath;int32 Dot=INDEX_NONE;
        if(Name.FindLastChar('.',Dot))Name=Name.Mid(Dot+1);
        else {int32 Slash=INDEX_NONE;if(Name.FindLastChar('/',Slash))Name=Name.Mid(Slash+1);}
        for(const TCHAR* Version:{TEXT("_V16"),TEXT("_V23"),TEXT("_V24")})
            if(Name.EndsWith(Version)){Name.LeftChopInline(4);Name+=TEXT("_V29");break;}
        if(Name.EndsWith(TEXT("_V29")))
        {
            const FString Folder=Name.StartsWith(TEXT("SK_Casualty_"))?TEXT("Casualties"):TEXT("Meshes");
            const FString Path=TEXT("/Game/Warfare/Characters/V29/")+Folder+TEXT("/")+Name+TEXT(".")+Name;
            if(auto* Refined=LoadObject<USkeletalMesh>(nullptr,*Path))return Refined;
            UE_LOG(LogTemp,Warning,TEXT("WF_V29_MESH_FALLBACK %s"),*Name);
        }
    }
    return LoadObject<USkeletalMesh>(nullptr,*OriginalPath);
}
