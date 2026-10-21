#pragma once

class UCanvas;
class UWorld;

// Test-only entry and patches. Both are inert without -WFV25HDRReview.
// DrawTiles must be called through the production HUD's selected UI canvas,
// never directly into a backbuffer or into the calibration overlay.
namespace FrontlineHDRReviewV25
{
    void Begin(UWorld* World);
    void DrawTiles(UCanvas* Canvas);
}
