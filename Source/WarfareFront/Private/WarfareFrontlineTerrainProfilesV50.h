#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineCampaignV32.h"

// A shared material palette, not a post-process filter. The photographed scan,
// displaced surface, bank contacts and physical water retain their own response.
struct FFrontlineTerrainPaletteV50
{
    FLinearColor Soil=FLinearColor(.48f,.435f,.385f);
    FLinearColor Horizon=FLinearColor(.012f,.0118f,.0115f);
    float Chalk=.18f,Grass=.0f,Ballast=.0f;
    const TCHAR* Backdrop=TEXT("Chalk");
};
inline FFrontlineTerrainPaletteV50 TerrainPaletteV50(EFrontlineMissionTerrainV32 Style,int32 Operation)
{
    FFrontlineTerrainPaletteV50 P;
    switch(Style)
    {
    case EFrontlineMissionTerrainV32::Ridge:
        P.Soil=FLinearColor(.56f,.525f,.465f);P.Horizon=FLinearColor(.0125f,.012f,.0115f);P.Chalk=.92f;P.Grass=.12f;break;
    case EFrontlineMissionTerrainV32::Marsh:
        P.Soil=FLinearColor(.38f,.40f,.375f);P.Horizon=FLinearColor(.011f,.012f,.012f);P.Chalk=0;P.Grass=.38f;P.Backdrop=TEXT("Marsh");break;
    case EFrontlineMissionTerrainV32::Village:
        P.Soil=FLinearColor(.52f,.425f,.365f);P.Horizon=FLinearColor(.012f,.0107f,.0098f);P.Chalk=.11f;P.Ballast=.75f;P.Backdrop=TEXT("Village");break;
    case EFrontlineMissionTerrainV32::Canal:
        P.Soil=FLinearColor(.415f,.44f,.425f);P.Horizon=FLinearColor(.011f,.0118f,.012f);P.Chalk=.05f;P.Grass=.22f;P.Backdrop=TEXT("Village");break;
    case EFrontlineMissionTerrainV32::RailCutting:
        P.Soil=FLinearColor(.44f,.42f,.39f);P.Chalk=.21f;P.Ballast=1;P.Backdrop=TEXT("Village");break;
    default:P.Grass=.58f;P.Chalk=.24f;break;
    }
    // The winter patrol sector keep the same geology, with pale dry ridges.
    if(Operation==10){P.Soil=FLinearColor(.56f,.55f,.535f);P.Chalk=.65f;P.Grass=.10f;}
    return P;
}
