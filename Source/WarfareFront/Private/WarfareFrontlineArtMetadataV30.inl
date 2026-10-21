// V30 original narrative atlas bindings. Labels identify real regional geography;
// the painted terrain is a narrative reconstruction, not a georeferenced survey.
// Portraits are original painted likeness reconstructions of army commanders.
#pragma once
namespace WFV30Art
{
struct FMapTown { const TCHAR* Name; float X; float Y; };
struct FCommander { const TCHAR* Name; const TCHAR* Army; int32 PortraitSlot; };
struct FMissionArt
{
    const TCHAR* MapKey; const TCHAR* TextureName; const TCHAR* Title;
    FMapTown Towns[5]; int32 TownCount; float FocusX; float FocusY;
    FCommander British; FCommander German;
};
static const FMissionArt Missions[6] =
{
    {TEXT("arras"),TEXT("T_Map_Arras"),TEXT("ARRAS · 9 APRIL 1917"),
     {{TEXT("ARRAS"),0.215f,0.47f},{TEXT("FEUCHY"),0.5f,0.46f},{TEXT("RŒUX"),0.727f,0.306f},{TEXT("MONCHY"),0.788f,0.706f}},4,0.6f,0.52f,
     {TEXT("EDMUND ALLENBY"),TEXT("THIRD ARMY"),0},{TEXT("LUDWIG VON FALKENHAUSEN"),TEXT("SIXTH ARMY"),0}},
    {TEXT("messines"),TEXT("T_Map_Messines"),TEXT("MESSINES · 7 JUNE 1917"),
     {{TEXT("WYTSCHAETE"),0.48f,0.1f},{TEXT("MESSINES"),0.55f,0.532f},{TEXT("PLOEGSTEERT"),0.4f,0.86f}},3,0.45f,0.54f,
     {TEXT("HERBERT PLUMER"),TEXT("SECOND ARMY"),1},{TEXT("FRIEDRICH SIXT VON ARMIN"),TEXT("FOURTH ARMY"),1}},
    {TEXT("ypres"),TEXT("T_Map_Ypres"),TEXT("PILCKEM · 31 JULY 1917"),
     {{TEXT("YPRES"),0.23f,0.71f},{TEXT("BOESINGHE"),0.252f,0.285f},{TEXT("PILCKEM"),0.5f,0.325f},{TEXT("LANGEMARCK"),0.7f,0.15f},{TEXT("PASSCHENDAELE"),0.915f,0.288f}},5,0.48f,0.4f,
     {TEXT("HUBERT GOUGH"),TEXT("FIFTH ARMY"),2},{TEXT("FRIEDRICH SIXT VON ARMIN"),TEXT("FOURTH ARMY"),1}},
    {TEXT("cambrai"),TEXT("T_Map_Cambrai"),TEXT("CAMBRAI · NOVEMBER 1917"),
     {{TEXT("CAMBRAI"),0.805f,0.21f},{TEXT("BOURLON"),0.55f,0.14f},{TEXT("FLESQUIÈRES"),0.43f,0.515f},{TEXT("HAVRINCOURT"),0.235f,0.74f},{TEXT("MARCOING"),0.735f,0.65f}},5,0.53f,0.53f,
     {TEXT("JULIAN BYNG"),TEXT("THIRD ARMY"),3},{TEXT("GEORG VON DER MARWITZ"),TEXT("SECOND ARMY"),2}},
    {TEXT("michael"),TEXT("T_Map_Michael"),TEXT("ST QUENTIN · 21 MARCH 1918"),
     {{TEXT("ST QUENTIN"),0.84f,0.51f},{TEXT("PÉRONNE"),0.215f,0.125f},{TEXT("HAM"),0.395f,0.746f}},3,0.68f,0.49f,
     {TEXT("HUBERT GOUGH"),TEXT("FIFTH ARMY"),2},{TEXT("GEORG VON DER MARWITZ"),TEXT("SECOND ARMY"),2}},
    {TEXT("amiens"),TEXT("T_Map_Amiens"),TEXT("AMIENS · 8 AUGUST 1918"),
     {{TEXT("AMIENS"),0.145f,0.46f},{TEXT("VILLERS-BRETONNEUX"),0.525f,0.635f},{TEXT("CORBIE"),0.442f,0.177f},{TEXT("LE HAMEL"),0.625f,0.32f},{TEXT("CERISY"),0.8f,0.21f}},5,0.65f,0.53f,
     {TEXT("HENRY RAWLINSON"),TEXT("FOURTH ARMY"),4},{TEXT("GEORG VON DER MARWITZ"),TEXT("SECOND ARMY"),2}},
};
inline const FMissionArt& GetMission(int32 MissionIndex)
{
    return Missions[FMath::Clamp(MissionIndex,0,5)];
}
// Emblem atlas has 3 columns and 2 rows; commander atlas UK 3x2 / DE 3x1.
enum EEmblemSlot { BritishArms=0, GermanArms=1, BritishDoctrine=2, GermanDoctrine=3, Locked=4, Dispatch=5 };
}
