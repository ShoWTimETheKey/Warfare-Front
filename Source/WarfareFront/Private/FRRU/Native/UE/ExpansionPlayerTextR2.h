#pragma once
#include "CoreMinimal.h"
#include "../PlayerTextR2.generated.h"

// Pure presentation lookup. Stable IDs do not purchase or activate a doctrine.
// Return false for unknown IDs; caller must not substitute an r1 stat upgrade.
inline bool WFExpansionTechnologyTextR2(const FString& Id, FString& Title,
    FString& Description, FString& Tradeoff)
{
    const FTCHARToUTF8 Encoded(*Id);
    const auto* Row = WarfareExpansion::FindPlayerTechnologyTextR2(
        std::string_view(Encoded.Get(), static_cast<size_t>(Encoded.Length())));
    if (!Row) return false;
    Title = UTF8_TO_TCHAR(Row->title);
    Description = UTF8_TO_TCHAR(Row->description);
    Tradeoff = UTF8_TO_TCHAR(Row->tradeoff);
    return true;
}
