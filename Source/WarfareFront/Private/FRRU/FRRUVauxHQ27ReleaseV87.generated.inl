// SPDX-License-Identifier: MIT
// Source-only interface replacement. No original Tripo mesh, contact cloud,
// texture identity, receipt hash or source contour is included. Its loader is
// disabled by SourceDataAvailable; neutral sentinels are not scene artwork.
#pragma once
namespace FRRUVauxHQ27ReleaseV87
{
inline constexpr bool SourceDataAvailable=false;
static constexpr const TCHAR* RemainderPath=TEXT("");
static constexpr const TCHAR* MeshPath=TEXT("");
static constexpr const TCHAR* MaterialPath=TEXT("");
static constexpr const TCHAR* GroundPath=TEXT("");
static constexpr const TCHAR* SolidsPath=TEXT("");
static constexpr const TCHAR* Slot=TEXT("");
static const FBox LocalBounds(ForceInit),NativeBounds(ForceInit),Native28Bounds(ForceInit),Base28LocalBounds(ForceInit);
static const FTransform Pose=FTransform::Identity,Pose28=FTransform::Identity;
static constexpr double MinimumGap=0.,MaximumGap=0.,Base28Top=0.;
struct FTexture {const TCHAR* Asset;bool SRGB;uint8 Compression;};
static const FTexture Textures[]={{TEXT(""),false,0}};
static const FVector Contacts[]={FVector::ZeroVector};
static constexpr const TCHAR* Base28Path=TEXT("");
static constexpr const TCHAR* Base28Materials[]={TEXT(""),TEXT("")};
static constexpr const TCHAR* Base28Slots[]={TEXT(""),TEXT("")};
static const FVector Base28Footprint[]={FVector::ZeroVector};
}
