#include "WarfareFRRUTextV82.h"
#include "WarfareFrontlineLocalizationV35.h"
#include "FRRU/FRRUTextV82.generated.h"
FString WarfareFRRUTextV82::English(const FString& StableId,const TCHAR* Field,const FString& Fallback)
{
    for(const auto& Row:Rows)if(StableId==Row.Id&&FCString::Strcmp(Field,Row.Field)==0)return Row.English;
    return Fallback;
}
FString WarfareFRRUTextV82::Resolve(const FString& StableId,const TCHAR* Field,const FString& FallbackEnglish,const FString& FallbackChinese)
{
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    for(const auto& Row:Rows)if(StableId==Row.Id&&FCString::Strcmp(Field,Row.Field)==0)return Chinese?Row.Chinese:Row.English;
    // Missing display copy must not expose the native catalogue's production
    // notes. Stable titles and other callers retain their explicit fallbacks.
    if(FCString::Strcmp(Field,TEXT("description"))==0)return FString();
    return Chinese?FallbackChinese:FallbackEnglish;
}
