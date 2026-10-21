// Optional texture quality must never be mistaken for a failed level load.
namespace FrontlinePresentationV37
{
inline bool PreparationReady(double SceneSeconds,int32 RenderedFrames,bool AllRequestedMipsResident)
{
    return RenderedFrames>=60 && SceneSeconds>=3.5 && (AllRequestedMipsResident||SceneSeconds>=8.0);
}
inline bool UsePhotoTransition(int32 From,int32 To,double SinceLastPhoto)
{
    // Preparation already owns its full-screen artwork. Tool panels, introductions
    // and deployment selections remain responsive without replaying a photograph.
    return SinceLastPhoto>=2.0 && ((From==3&&To==6) || To==18
        || (To==1 && (From==0 || From==2 || From==6 || From==11 || From==12 || From==18)));
}
// Count only visible draws. Loading a photo or a window losing focus must not
// consume its entire fade before the player has seen a frame.
struct FPhotoClock
{
    double LastDrawAt=-1.;
    float VisibleSeconds=0.f;
    bool Advance(double Now,bool Ready)
    {
        if(!Ready){LastDrawAt=-1.;return false;}
        if(LastDrawAt>=0.)VisibleSeconds+=float(FMath::Clamp(Now-LastDrawAt,0.,.10));
        LastDrawAt=Now;
        return VisibleSeconds<.55f;
    }
};
struct FCommanderPortrait { int32 Atlas,Slot,Columns,Rows;const TCHAR* Identity; };
inline FCommanderPortrait Commander(bool German,int32 Mission)
{
    static const TCHAR* BritishNames[]={TEXT("Allenby"),TEXT("Plumer"),TEXT("Gough"),TEXT("Byng"),TEXT("Rawlinson")};
    static const TCHAR* GermanNames[]={TEXT("Falkenhausen"),TEXT("Sixt von Armin"),TEXT("Marwitz")};
    if(Mission<6)
    {
        const auto& Art=WFV30Art::GetMission(Mission);
        const int32 Slot=German?Art.German.PortraitSlot:Art.British.PortraitSlot;
        return {German?1:0,Slot,3,German?1:2,German?GermanNames[Slot]:BritishNames[Slot]};
    }
    if(!German)
    {
        if(Mission==6)return {2,0,2,2,TEXT("Henry Horne")};
        if(Mission==9)return {0,3,3,2,TEXT("Byng")};
        return {0,4,3,2,TEXT("Rawlinson")};
    }
    if(Mission==6)return {2,1,2,2,TEXT("Otto von Below")};
    if(Mission==9)return {2,3,2,2,TEXT("Bruno von Mudra")};
    return {2,2,2,2,TEXT("Adolph von Carlowitz")};
}
}
