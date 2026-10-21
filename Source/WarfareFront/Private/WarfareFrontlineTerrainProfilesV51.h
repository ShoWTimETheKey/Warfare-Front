#pragma once
#include "CoreMinimal.h"
#include "WarfareFrontlineCampaignV32.h"

struct FFrontlineTerrainPaletteV51
{
    FLinearColor Soil=FLinearColor(.48f,.435f,.385f);
    // Kept for bank/backdrop callers; V51 soil deliberately never blends to it.
    FLinearColor Horizon=FLinearColor(.012f,.0118f,.0115f);
    float Chalk=.18f,Grass=.0f,Ballast=.0f;
    const TCHAR* Backdrop=TEXT("Chalk");
    const TCHAR* GroundMaterial=TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundMud_V51.MI_GroundMud_V51");
    // Mud remains the structural base. Deposit regions use restrained mud
    // relief; winter's snow surface deliberately excludes the brown scan.
    float ScanReliefScale=1.f;
};
inline FFrontlineTerrainPaletteV51 TerrainPaletteV51(EFrontlineMissionTerrainV32 Style,int32 Operation)
{
    FFrontlineTerrainPaletteV51 P;
    switch(Style)
    {
    case EFrontlineMissionTerrainV32::Ridge:
        P.Soil=FLinearColor(.46f,.47f,.465f);P.Chalk=.04f;P.Grass=.05f;P.ScanReliefScale=.65f;
        P.GroundMaterial=TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundChalk_V51.MI_GroundChalk_V51");break;
    case EFrontlineMissionTerrainV32::Marsh:
        P.Soil=FLinearColor(.38f,.40f,.375f);P.Chalk=0;P.Grass=.19f;P.Backdrop=TEXT("Marsh");break;
    case EFrontlineMissionTerrainV32::Village:
        P.Soil=FLinearColor(.48f,.445f,.405f);P.Chalk=.04f;P.Ballast=.10f;P.Backdrop=TEXT("Village");P.ScanReliefScale=.65f;
        P.GroundMaterial=TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundRubble_V51.MI_GroundRubble_V51");break;
    case EFrontlineMissionTerrainV32::Canal:
        P.Soil=FLinearColor(.415f,.44f,.425f);P.Chalk=.05f;P.Grass=.17f;P.Backdrop=TEXT("Village");break;
    case EFrontlineMissionTerrainV32::RailCutting:
        P.Soil=FLinearColor(.48f,.445f,.405f);P.Chalk=.04f;P.Ballast=.15f;P.Backdrop=TEXT("Village");P.ScanReliefScale=.65f;
        P.GroundMaterial=TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundRubble_V51.MI_GroundRubble_V51");break;
    default:P.Grass=.34f;P.Chalk=.20f;break;
    }
    if(Operation==10)
    {
        P.Soil=FLinearColor(.60f,.62f,.64f);P.Chalk=0;P.Grass=0;P.Ballast=0;P.ScanReliefScale=0;
        P.GroundMaterial=TEXT("/Game/Warfare/Field/V51/Materials/MI_GroundWinter_V51.MI_GroundWinter_V51");
    }
    return P;
}
