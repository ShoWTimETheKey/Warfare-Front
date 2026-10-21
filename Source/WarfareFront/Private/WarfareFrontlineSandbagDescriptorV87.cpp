// SPDX-License-Identifier: MIT
// Source-only replacement for an asset-dependent adapter. Optional Tripo
// sandbags remain unavailable; TryLoad preserves the existing six-variant
// fallback contract. This does not recreate the omitted artwork or certify it.
#include "WarfareFrontlineSandbagDescriptorV87.h"
#include "WarfareFrontlineSandbagContactV87.inl"

bool FrontlineSandbagDescriptorV87::TryLoad(TArray<UStaticMesh*>& Meshes,UMaterialInterface*& Material)
{
    static_assert(!WarfareOpenSourceDataV87::SandbagGeometryAvailable,"No private contact data is distributed");
    Meshes.Reset();Material=nullptr;return false;
}

int32 FrontlineSandbagDescriptorV87::Variant(int32 OriginalVariant)
{return FMath::Clamp(OriginalVariant,0,5);}

const FVector* FrontlineSandbagDescriptorV87::Feet(int32 VariantIndex)
{(void)VariantIndex;return nullptr;}

bool FrontlineSandbagDescriptorV87::TopHeight(int32 VariantIndex,const FTransform& Transform,float X,float Y,float& Height)
{(void)VariantIndex;(void)Transform;(void)X;(void)Y;Height=0.f;return false;}

bool FrontlineSandbagDescriptorV87::IsMaterial(UMaterialInterface* Material)
{(void)Material;return false;}
