#pragma once

#include "CoreMinimal.h"

class AWarfareFrontlineController;
class UTexture2D;
class UTextureRenderTarget2D;

namespace FrontlineMapSceneV61
{
// Prepare the existing UK/DE table behind an opaque menu before Continue.
// No view or world visibility changes occur while dependencies load.
void PreloadContinueV8214(AWarfareFrontlineController* PC);
bool ContinuePreloadSettledV8214(AWarfareFrontlineController* PC);
bool ContinuePreloadReadyV8214(AWarfareFrontlineController* PC);
bool ContinueViewWarmedV8214(AWarfareFrontlineController* PC);
    void PrefetchV75(AWarfareFrontlineController* PC,FVector2D Centre,float Zoom);
// UV is the campaign's western-front Mercator rectangle, north at V=0.
// Project/Unproject use the same letterboxed 1920x1080 space as FFrontlineCanvas.
struct FMapRect
{
    FVector2D Min,Max;
};
FMapRect SectorBounds(int32 Sector);
FVector2D SectorFocus(int32 Sector);
FMapRect OperationBounds(AWarfareFrontlineController* PC,int32 Mission);

// Call every player tick, including Active=false on leaving the war room.
// True means a ready scene owns the main view; skip the battle-camera update.
// CentreUV is the geographic point that stays at logical screen (660,540).
// Zoom 1 shows the table; about 5 a sector; 20 an operation. FlightPhase is
// normalised 0..1 travel progress (zero while stationary), not a phase ID.
bool Update(AWarfareFrontlineController* PC,bool Active,FVector2D CentreUV,
    float Zoom,float FlightPhase,int32 Sector,int32 Mission,float Detail);
FVector2D Project(AWarfareFrontlineController* PC,FVector2D UV);
FVector2D Unproject(AWarfareFrontlineController* PC,FVector2D Screen1920x1080);
bool IsActive(AWarfareFrontlineController* PC);
void SetHoverV64(AWarfareFrontlineController* PC,int32 Sector,int32 Mission,FVector2D Centre,FVector2D Extent);
bool IsSingleDocumentV64(AWarfareFrontlineController* PC);
bool IsAtlasReadyV64(AWarfareFrontlineController* PC);
void SetOperationArtwork(UTexture2D* Texture,int32 Mission);
bool CaptureLoadingView(AWarfareFrontlineController* PC);
UTextureRenderTarget2D* LoadingView(AWarfareFrontlineController* PC);
void ReleaseLoadingView();
void RunV834GeometryChecks(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check);
}
