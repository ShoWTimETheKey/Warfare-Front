#pragma once
#include "CoreMinimal.h"
class AWarfareFrontlineGround;
class FJsonObject;
struct FFrontlineGroundTriangleV8449
{
    FVector Points[3];
    int32 SourceTriangle=INDEX_NONE;
};

/** Read-only contact with the actual retained terrain LOD. No physics cooking
 * or analytic height fallback; prepare once after the terrain has been bound. */
namespace FrontlineGroundContactV8449
{
    WARFAREFRONT_API bool Prepare(AWarfareFrontlineGround* Ground);
    WARFAREFRONT_API bool Sample(AWarfareFrontlineGround* Ground,float WorldX,float WorldY,
        FVector& Point,FVector& Normal,FLinearColor* LinearPhysicalColor=nullptr);
    /** Copies only the bounded local patch's real indexed triangles. */
    WARFAREFRONT_API bool TrianglesInBox(AWarfareFrontlineGround* Ground,const FVector2D& Minimum,
        const FVector2D& Maximum,TArray<FFrontlineGroundTriangleV8449>& Triangles);
    WARFAREFRONT_API TSharedRef<FJsonObject> Describe(AWarfareFrontlineGround* Ground);
}
