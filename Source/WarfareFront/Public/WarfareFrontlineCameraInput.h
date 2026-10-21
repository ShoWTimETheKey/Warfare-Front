#pragma once

#include "CoreMinimal.h"

namespace FrontlineCameraInput
{
    struct FInput
    {
        FVector2D Mouse=FVector2D::ZeroVector;
        FVector2D Viewport=FVector2D::ZeroVector;
        float FieldTop=0,FieldBottom=0;
        bool bActive=false,bPointerValid=false,bOverUI=false,bLeftKey=false,bRightKey=false;
    };

    inline bool AllowsMotion(const FInput& In)
    {
        if(!In.bActive)return false;
        if(In.bLeftKey||In.bRightKey)return true;
        if(!In.bPointerValid||In.bOverUI||In.Viewport.X<=0||In.Viewport.Y<=0
            ||In.Mouse.X<0||In.Mouse.X>=In.Viewport.X||In.Mouse.Y<In.FieldTop
            ||In.Mouse.Y>=In.FieldBottom||In.Mouse.Y>=In.Viewport.Y)return false;
        return true;
    }

    inline float Axis(const FInput& In)
    {
        if(!AllowsMotion(In))return 0.f;
        // A deliberate keyboard command wins over a cursor parked at an edge.
        if(In.bLeftKey||In.bRightKey)return float(In.bRightKey)-float(In.bLeftKey);
        const float Band=In.Viewport.X*.04f;
        float Depth=0,Direction=0;
        if(In.Mouse.X<Band){Depth=1.f-In.Mouse.X/Band;Direction=-1.f;}
        else if(In.Mouse.X>In.Viewport.X-Band){Depth=1.f-(In.Viewport.X-In.Mouse.X)/Band;Direction=1.f;}
        const float T=FMath::Clamp(Depth,0.f,1.f);
        return Direction*T*T*(3.f-2.f*T);
    }

    inline void Step(float Axis,float Width,float DT,bool bActive,double Limit,double& X,float& Velocity)
    {
        // Zoom can change the legal extent even while input is blocked by UI.
        if(!bActive){Velocity=0;X=FMath::Clamp(X,-Limit,Limit);return;}
        DT=FMath::Clamp(DT,0.f,.05f);
        const float Target=Axis*Width*.55f;
        // Reversal has no residual movement in the direction just cancelled.
        if(Target*Velocity<0)Velocity=0;
        const float Rate=FMath::IsNearlyZero(Axis)?28.f:15.f;
        const float Decay=FMath::Exp(-Rate*DT);
        X+=Target*DT+(Velocity-Target)*(1.f-Decay)/Rate;
        Velocity=Target+(Velocity-Target)*Decay;
        if(FMath::IsNearlyZero(Axis)&&FMath::Abs(Velocity)<.5f)Velocity=0;
        X=FMath::Clamp(X,-Limit,Limit);
        if((X>=Limit&&Velocity>0)||(X<=-Limit&&Velocity<0))Velocity=0;
    }
}
