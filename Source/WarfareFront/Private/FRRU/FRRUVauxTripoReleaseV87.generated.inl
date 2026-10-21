// SPDX-License-Identifier: MIT
// Source-only interface replacement. Original Tripo model geometry, contact
// samples, texture identities and placement records are not distributed.
// The loader is disabled by SourceDataAvailable; placeholders are never staged.
#pragma once
namespace FRRUVauxTripoReleaseV87
{
inline constexpr bool SourceDataAvailable=false;
struct FTexture {const TCHAR* Asset=TEXT("");int32 Width=0,Height=0;bool SRGB=false;uint8 Compression=0;};
struct FModel
{
    const TCHAR* Key=TEXT("");const TCHAR* Mesh=TEXT("");const TCHAR* Material=TEXT("");
    const TCHAR* Slot=TEXT("");const TCHAR* SourceFBXSHA256=TEXT("");
    FBox LocalBounds=FBox(ForceInit);
    int32 SourceVertices=0,SourceTriangles=0,NaniteTriangles=0,FallbackTriangles=0;
    const FTexture* Textures=nullptr;int32 TextureCount=0;
    const FVector* Contacts=nullptr;int32 ContactCount=0;
};
struct FSite
{
    int32 Solid=INDEX_NONE,Model=INDEX_NONE,OldPart=INDEX_NONE;
    FTransform Pose=FTransform::Identity;
    FBox NativeBounds=FBox(ForceInit),PrevalidatedSourceWorldBounds=FBox(ForceInit);
    double ExpectedMinimumGap=0,ExpectedMaximumGap=0;const TCHAR* FitSHA256=TEXT("");
};
// Preserve compile-time interface extents only, with no original model data.
static const FModel Models[3]={};
static const FSite Sites[8]={};
}
