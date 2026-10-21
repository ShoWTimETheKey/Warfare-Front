// One source grid for real contact, render triangles and clipped water depth.
#pragma once
namespace WarfareFRRUWorldTerrainV826
{
inline FString Token(const FString& Scene)
{
    if(Scene==TEXT("fr.malmaison.scene.families.v1"))return TEXT("fr_malmaison");
    if(Scene==TEXT("ru.galicia.scene.redesign.v1"))return TEXT("ru_galicia");
    if(Scene==TEXT("fr.chemin_dames.scene.families.v1"))return TEXT("fr_chemin_dames");
    if(Scene==TEXT("fr.champagne_defence.scene.families.v1"))return TEXT("fr_champagne_defence");
    if(Scene==TEXT("fr.sommepy.scene.families.v1"))return TEXT("fr_sommepy");
    if(Scene==TEXT("fr.soissons.scene.families.v1"))return TEXT("fr_soissons");
    if(Scene==TEXT("ru.lutsk.scene.families.v1"))return TEXT("ru_lutsk");
    if(Scene==TEXT("ru.czernowitz.scene.families.v1"))return TEXT("ru_czernowitz");
    if(Scene==TEXT("ru.naroch.scene.families.v1"))return TEXT("ru_naroch");
    if(Scene==TEXT("ru.warsaw_rearguard.scene.families.v1"))return TEXT("ru_warsaw_rearguard");
    if(Scene==TEXT("ru.kovel.scene.families.v1"))return TEXT("ru_kovel");
    if(Scene==TEXT("ru.riga.scene.families.v1"))return TEXT("ru_riga");
    return FString();
}
inline FString Asset(const FString& Scene,const FString& Folder,const FString& Name)
{
    const FString T=Token(Scene);if(T.IsEmpty())return FString();
    const TCHAR* Version=T==TEXT("ru_czernowitz")?TEXT("RailCorrectiveV3/"):
        T==TEXT("fr_malmaison")||T==TEXT("ru_galicia")?TEXT("CorrectiveV1/"):TEXT("BatchV1/");
    return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldTerrainV826/")+FString(Version)+Folder+TEXT("/")+Name+TEXT(".")+Name;
}
inline FString Ground(const FString& Scene)
{const FString T=Token(Scene);return T.IsEmpty()?FString():Asset(Scene,TEXT("Scenes"),TEXT("SM_WF_")+T+TEXT("_ground"));}
inline FString RenderGround(const FString& Scene,int32 Index)
{const FString T=Token(Scene);return T.IsEmpty()||Index<0||Index>=8?FString():Asset(Scene,TEXT("Scenes"),FString::Printf(TEXT("SM_WF_%s_render_%03d_dressing"),*T,Index));}
inline bool HasWater(const FString& Scene)
{const FString T=Token(Scene);return T==TEXT("ru_galicia")||T==TEXT("ru_warsaw_rearguard")||T==TEXT("ru_kovel")||T==TEXT("ru_riga");}
inline FString WaterMesh(const FString& Scene)
{return HasWater(Scene)?Asset(Scene,TEXT("Scenes"),TEXT("SM_WF_")+Token(Scene)+TEXT("_water")):FString();}
inline FString WaterMaterial(const FString& Scene)
{return HasWater(Scene)?Asset(Scene,TEXT("Water"),TEXT("M_Water_")+Token(Scene)+TEXT("_WT_V82")):FString();}
inline FString WaterHeight(const FString& Scene)
{return HasWater(Scene)?Asset(Scene,TEXT("Water"),TEXT("T_Height_")+Token(Scene)+TEXT("_WT_V82")):FString();}
}
