#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;
class FJsonObject;
void StartFrontlineV41Review(AWarfareFrontlineGameMode* Battle);
// Narrow access to the actual production overlay and camera state. Implemented
// beside that private state; guarded to an isolated V41 review process.
namespace FrontlineReviewUIV41
{
 void Clear(AWarfareFrontlineController* PC);
 void Dossier(AWarfareFrontlineController* PC);
 void Deployment(AWarfareFrontlineController* PC);
 void HonoursV82(AWarfareFrontlineController* PC,int32 Focus,bool Banner=false);
  void FactionV83(AWarfareFrontlineController* PC,int32 Army,bool Animate);
  TSharedRef<FJsonObject> FactionFramesV83();
 void HonoursBadgeChecksV82(AWarfareFrontlineController* PC,TFunctionRef<void(const FString&,bool)> Check);
 void Technology(AWarfareFrontlineController* PC,int32 Page,int32 Focus);
 void Pointer(FVector2D Position);
 void Camera(AWarfareFrontlineController* PC,float X,float Y,float Width,float Height,bool Cut=true);
 void BeginAudit();
 TSharedRef<FJsonObject> EndAudit();
}
