// Exact receiver-owned render parts; terrain grid and physical ground stay paired.
#pragma once
#include "WarfareFRRUWorldTerrainV826.inl"
namespace WarfareFRRUWorldRenderV826
{
struct FEntry {const TCHAR* Scene;const TCHAR* Token;int32 Ground,Solids,Dressing;};
inline const FEntry Entries[]={
    {TEXT("fr.malmaison.scene.families.v1"),TEXT("fr_malmaison"),8,10,11},
    {TEXT("ru.galicia.scene.redesign.v1"),TEXT("ru_galicia"),8,0,37},
    {TEXT("fr.chemin_dames.scene.families.v1"),TEXT("fr_chemin_dames"),8,0,0},
    {TEXT("fr.champagne_defence.scene.families.v1"),TEXT("fr_champagne_defence"),8,0,0},
    {TEXT("fr.sommepy.scene.families.v1"),TEXT("fr_sommepy"),8,0,0},
    {TEXT("fr.soissons.scene.families.v1"),TEXT("fr_soissons"),8,0,0},
    {TEXT("ru.lutsk.scene.families.v1"),TEXT("ru_lutsk"),8,0,0},
    {TEXT("ru.czernowitz.scene.families.v1"),TEXT("ru_czernowitz"),8,0,0},
    {TEXT("ru.naroch.scene.families.v1"),TEXT("ru_naroch"),8,0,0},
    {TEXT("ru.warsaw_rearguard.scene.families.v1"),TEXT("ru_warsaw_rearguard"),8,0,0},
    {TEXT("ru.kovel.scene.families.v1"),TEXT("ru_kovel"),8,0,0},
    {TEXT("ru.riga.scene.families.v1"),TEXT("ru_riga"),8,0,0},
};
inline const FEntry* Find(const FString& Scene)
{for(const auto& Entry:Entries)if(Scene==Entry.Scene)return &Entry;return nullptr;}
inline int32 Count(const FEntry* Entry,int32 Section)
{return Entry?(Section==0?Entry->Ground:Section==1?Entry->Solids:Entry->Dressing):0;}
inline FString Path(const FEntry* Entry,int32 Section,int32 Index)
{
    if(!Entry||Index<0||Index>=Count(Entry,Section))return FString();
    // The visible deck and its exact soil cut share the original walk height.
    // Imported collision, native ground and water paths remain unchanged.
    if(FString(Entry->Scene)==TEXT("ru.galicia.scene.redesign.v1"))
    {
        const TCHAR* Token=nullptr;
        if(Section==0&&Index==2)Token=TEXT("ground_002");
        else if(Section==0&&Index==4)Token=TEXT("ground_004");
        else if(Section==2&&Index==13)Token=TEXT("bridge_west");
        else if(Section==2&&Index==16)Token=TEXT("bridge_east");
        if(Token)
        {
            const FString Name=FString::Printf(TEXT("SM_WF_ru_galicia_%s_v827_dressing"),Token);
            return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/GaliciaBridgeV827/CandidateV1/Scenes/%s.%s"),*Name,*Name);
        }
    }
    if(Section==0)
    {
        const FString Terrain=WarfareFRRUWorldTerrainV826::RenderGround(Entry->Scene,Index);
        if(!Terrain.IsEmpty())return Terrain;
    }
    const TCHAR* Sections[]={TEXT("ground"),TEXT("solids"),TEXT("dressing")};
    const FString Name=FString::Printf(TEXT("SM_WF_%s_%s_%03d_v826_dressing"),Entry->Token,Sections[Section],Index);
    return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/WorldRenderV826/CorrectiveV1/Scenes/%s.%s"),*Name,*Name);
}
inline bool Registered(const FString& Asset,const FEntry* Entry,int32 Section)
{for(int32 I=0;I<Count(Entry,Section);++I)if(Asset==Path(Entry,Section,I))return true;return false;}
inline bool Registered(const FString& Asset)
{for(const auto& Entry:Entries)for(int32 S=0;S<3;++S)if(Registered(Asset,&Entry,S))return true;return false;}
}
