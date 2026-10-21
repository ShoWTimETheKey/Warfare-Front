void DrawFactionFlag(const FFrontlineCanvas& UI,EFrontlineFaction F,float X,float Y,float W,float H)
{ UI.ImageContain(Artwork().Flags[F==EFrontlineFaction::German?1:0],X,Y,W,H,FLinearColor(.86f,.88f,.89f,1)); }

// Each army's own losses stain its cloth. Morale recovery never cleans the flag.
void DrawCasualtyFlag(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,int32 Team,float X,float Y,float W,float H)
{
    auto& A=Artwork();const bool National=WarfareFRRUCampaignV82::Active(B);
    const int32 F=National?int32(WarfareFRRUCampaignV82::Nation(B,Team)):int32(B->GetFactionForTeam(Team));
    UTexture2D* Cloth=National?NationalTextureV82(WarfareFRRUCampaignV82::Nation(B,Team),TEXT("Flag")):A.Flags[F].Get();
    if(!Cloth)return;
    if(!A.FlagAbsorption){UI.ImageContain(Cloth,X,Y,W,H,FLinearColor(.86f,.88f,.89f,1));return;}
    // A field report draws the same army at several sizes. Publish one
    // immutable material state per frame for every copy of that flag.
    if(A.FlagDrawFrame[Team]!=GFrameCounter)
    {
        A.FlagDrawFrame[Team]=GFrameCounter;
        const float Loss=FMath::Max(0,B->Casualties[Team]);const double Now=FPlatformTime::Seconds();
        // Exponential relaxation is stable for any positive interval. Truncating
        // a long frame would discard time and leave the stain behind its wet pulse.
        const float Delta=float(FMath::Max(0.0,Now-A.FlagUpdatedAt[Team]));
        if(B->Elapsed<A.FlagTime[Team] || Loss<A.FlagTargetLosses[Team] || Loss==0)
        {
            A.FlagLosses[Team]=Loss;A.FlagTargetLosses[Team]=Loss;A.FlagPreviousLosses[Team]=Loss;A.FlagImpactAt[Team]=Now-10;
        }
        // Catch up the old arrival before publishing a newly observed loss.
        // A capture/loading hitch must not count as elapsed time for fresh blood.
        A.FlagLosses[Team]=FMath::Max(A.FlagLosses[Team],FMath::Lerp(A.FlagLosses[Team],A.FlagTargetLosses[Team],1-FMath::Exp(-Delta*5.f)));
        if(Loss>A.FlagTargetLosses[Team])
        {
            A.FlagPreviousLosses[Team]=A.FlagTargetLosses[Team];A.FlagTargetLosses[Team]=Loss;A.FlagImpactAt[Team]=Now;
        }
        // Only the newly arriving liquid is animated. The revealed permanent mask
        // never retracts on another casualty, a menu pause, or morale recovery.
        A.FlagUpdatedAt[Team]=Now;
        A.FlagTime[Team]=B->Elapsed;
    }
    UI.Flag(Cloth,X,Y,W,H,FVector4f(A.FlagLosses[Team],A.FlagTargetLosses[Team],A.FlagPreviousLosses[Team],float(A.FlagUpdatedAt[Team]-A.FlagImpactAt[Team])),B->Elapsed+F*3.7f,19.f+F*137.f+float(GetTypeHash(B->GetEncounterId())&4095)*.017f);
}

#include "WarfareFrontlineTechnologyUIV32.inl"
