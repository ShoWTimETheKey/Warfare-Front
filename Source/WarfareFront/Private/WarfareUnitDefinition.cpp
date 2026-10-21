#include "WarfareUnitDefinition.h"

FPrimaryAssetId UWarfareUnitDefinition::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(FPrimaryAssetType(FName(TEXT("WarfareUnit"))), GetFName());
}
