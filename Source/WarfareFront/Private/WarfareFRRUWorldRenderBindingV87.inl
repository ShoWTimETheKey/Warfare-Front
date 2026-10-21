// Accepted Vaux split presentation; native ground, collision and LOS stay.
// The existing Editor NoSave VauxParts comparison still selects its old split.
#pragma once
#include "WarfareFRRUDressingBasisBindingV87.inl"
namespace WarfareFRRUWorldRenderBindingV87
{
using FEntry=WarfareFRRUWorldRenderV826::FEntry;
inline const FEntry Vaux={TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),0,0,18};
inline const FEntry VauxRuins={TEXT("fr.vaux.scene.families.v1"),TEXT("fr_vaux"),0,12,18};
inline bool CandidateEnabled()
{
#if WITH_EDITOR
    return FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxParts"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV82WorldReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"));
#else
    return false;
#endif
}
inline bool RuinsEnabled()
{return !CandidateEnabled()||FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxRuins"));}
inline bool ProductionEnabled(const FString& Scene)
{return Scene==Vaux.Scene&&!CandidateEnabled();}
// Only the reviewed single HQ27 release changes the formal part0 path.
inline bool HQ27ProductionEnabled(const FString& Scene)
{
    if(!ProductionEnabled(Scene))return false;
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WFFRRUV87VauxHQ27")))return false;
#endif
    return true;
}
inline const FEntry* Find(const FString& Scene)
{return Scene==Vaux.Scene?(RuinsEnabled()?&VauxRuins:&Vaux):WarfareFRRUWorldRenderV826::Find(Scene);}
inline int32 Count(const FEntry* E,int32 S)
{return WarfareFRRUWorldRenderV826::Count(E,S);}
inline FString Path(const FEntry* E,int32 S,int32 I)
{
    if(E!=&Vaux&&E!=&VauxRuins)
    {
        const FString Original=WarfareFRRUWorldRenderV826::Path(E,S,I);
        return S==2?WarfareFRRUDressingBasisBindingV87::Resolve(Original):Original;
    }
    if(E==&VauxRuins&&S==1&&I==0&&HQ27ProductionEnabled(Vaux.Scene))return TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRedoubtRemainder/CandidateR3Without28/Scenes/SM_WF_fr_vaux_redoubt27_28_remainder_current_R2_V87_dressing.SM_WF_fr_vaux_redoubt27_28_remainder_current_R2_V87_dressing");
    if(E==&VauxRuins&&S==1&&I>=0&&I<VauxRuins.Solids)
    {
        const FString Name=FString::Printf(TEXT("SM_WF_fr_vaux_ruins_%03d_V87_dressing"),I);
        return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRuins/CandidateV2/Scenes/%s.%s"),*Name,*Name);
    }
    if(S!=2||I<0||I>=Vaux.Dressing)return FString();
    const FString Name=FString::Printf(TEXT("SM_VauxDressing_P%03d_V87"),I);
    return FString::Printf(TEXT("/Game/FRRUIntegration/FormalV82/Terrain/V87/VauxRender/SpatialPartsV2/%s.%s"),*Name,*Name);
}
inline bool Registered(const FString& Asset,const FEntry* E,int32 S)
{for(int32 I=0;I<WarfareFRRUWorldRenderBindingV87::Count(E,S);++I)if(Asset==WarfareFRRUWorldRenderBindingV87::Path(E,S,I))return true;return false;}
inline bool Registered(const FString& Asset)
{
    for(const auto& E:WarfareFRRUWorldRenderV826::Entries)for(int32 S=0;S<3;++S)
        if(WarfareFRRUWorldRenderBindingV87::Registered(Asset,&E,S))return true;
    for(int32 S=0;S<3;++S)
        if(WarfareFRRUWorldRenderBindingV87::Registered(Asset,WarfareFRRUWorldRenderBindingV87::Find(Vaux.Scene),S))return true;
    return false;
}
}
