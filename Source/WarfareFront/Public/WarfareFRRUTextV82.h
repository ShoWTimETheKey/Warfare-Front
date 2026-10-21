#pragma once
#include "CoreMinimal.h"
namespace WarfareFRRUTextV82
{
    WARFAREFRONT_API FString Resolve(const FString& StableId,const TCHAR* Field,const FString& FallbackEnglish=FString(),const FString& FallbackChinese=FString());
    WARFAREFRONT_API FString English(const FString& StableId,const TCHAR* Field,const FString& Fallback=FString());
}
