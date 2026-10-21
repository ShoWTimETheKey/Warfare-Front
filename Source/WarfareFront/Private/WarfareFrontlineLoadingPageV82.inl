// Four-army loading artwork bindings; reviewed imports are owned by the integrator.
namespace FrontlineLoadingPageV82
{
struct FCaptionV844
{
    double Started=-1.;bool Entering=false;
    void Reset(double Value){Started=Value;Entering=false;}
    bool Update(bool Resident,bool SceneRendered){Entering|=Resident&&SceneRendered;return Entering;}
};
TMap<TWeakObjectPtr<AWarfareFrontlineGameMode>,FCaptionV844> CaptionsV844;
void ResetCaptionV844(AWarfareFrontlineGameMode* B,double Started)
{
    for(auto It=CaptionsV844.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    if(B)CaptionsV844.FindOrAdd(B).Reset(Started);
}
bool EnteringCaptionV844(AWarfareFrontlineGameMode* B,const FFrontlineScreenState& S,bool Resident,bool SceneRendered)
{
    if(!B)return false;auto& State=CaptionsV844.FindOrAdd(B);
    if(State.Started!=S.PreparationStarted)State.Reset(S.PreparationStarted);
    return State.Update(Resident,SceneRendered);
}
void RunCaptionChecksV844(TFunctionRef<void(const FString&,bool)> Check)
{
    FCaptionV844 S;S.Reset(1);
    Check(TEXT("v844_loading_caption_starts_loading"),!S.Update(false,false));
    Check(TEXT("v844_loading_caption_needs_scene_and_resources"),!S.Update(true,false)&&!S.Update(false,true));
    Check(TEXT("v844_loading_caption_advances_when_both_ready"),S.Update(true,true));
    Check(TEXT("v844_loading_caption_does_not_reverse_after_new_mip_request"),S.Update(false,true));
    S.Reset(2);Check(TEXT("v844_loading_caption_new_operation_resets_without_old_ready_state"),!S.Update(false,false));
}
FString ArmyNameV8217(EFRRUNationV81 Nation)
{
    switch(Nation)
    {
    case EFRRUNationV81::French:return WarRoom52::L(TEXT("FRENCH ARMY"),TEXT("法国陆军"));
    case EFRRUNationV81::Russian:return WarRoom52::L(TEXT("IMPERIAL RUSSIAN ARMY"),TEXT("俄罗斯帝国陆军"));
    case EFRRUNationV81::German:return WarRoom52::L(TEXT("IMPERIAL GERMAN ARMY"),TEXT("德意志帝国陆军"));
    default:return WarRoom52::L(TEXT("BRITISH EXPEDITIONARY FORCE"),TEXT("英国远征军"));
    }
}
#include "FRRU/LoadingNarrativesV82.generated.inl"
struct FAssets { const TCHAR* Mission; const TCHAR* Photo; const TCHAR* OperationMap; };
static const FAssets Assets[] = {
    {TEXT("uk.arras"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_arras_V82.T_Photo_uk_arras_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_arras_V839.T_Operation_arras_V839")},
    {TEXT("uk.roeux1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_roeux1917_V82.T_Photo_uk_roeux1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_arras_V839.T_Operation_arras_V839")},
    {TEXT("uk.bullecourt1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_bullecourt1917_V82.T_Photo_uk_bullecourt1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_bullecourt_V839.T_Operation_bullecourt_V839")},
    {TEXT("uk.messines"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_messines_V82.T_Photo_uk_messines_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_messines_V839.T_Operation_messines_V839")},
    {TEXT("uk.messines_counter1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_messines_counter1917_V82.T_Photo_uk_messines_counter1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_messines_V839.T_Operation_messines_V839")},
    {TEXT("uk.ypres"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_ypres_V82.T_Photo_uk_ypres_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_ypres_V839.T_Operation_ypres_V839")},
    {TEXT("uk.langemarck1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_langemarck1917_V82.T_Photo_uk_langemarck1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_ypres_V839.T_Operation_ypres_V839")},
    {TEXT("uk.menin1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_menin1917_V82.T_Photo_uk_menin1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_ypres_V839.T_Operation_ypres_V839")},
    {TEXT("uk.cambrai"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_cambrai_V82.T_Photo_uk_cambrai_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_cambrai_V839.T_Operation_cambrai_V839")},
    {TEXT("uk.bourlon1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_bourlon1917_V82.T_Photo_uk_bourlon1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_cambrai_V839.T_Operation_cambrai_V839")},
    {TEXT("uk.winter_raid1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_winter_raid1918_V82.T_Photo_uk_winter_raid1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_cambrai_V839.T_Operation_cambrai_V839")},
    {TEXT("uk.michael"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_michael_V82.T_Photo_uk_michael_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_michael_V839.T_Operation_michael_V839")},
    {TEXT("uk.lys1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_lys1918_V82.T_Photo_uk_lys1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_lys_V839.T_Operation_lys_V839")},
    {TEXT("uk.villers1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_villers1918_V82.T_Photo_uk_villers1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_villers_V839.T_Operation_villers_V839")},
    {TEXT("uk.amiens"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_amiens_V82.T_Photo_uk_amiens_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_amiens_V839.T_Operation_amiens_V839")},
    {TEXT("uk.scarpe1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_scarpe1918_V82.T_Photo_uk_scarpe1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_scarpe_V839.T_Operation_scarpe_V839")},
    {TEXT("uk.hindenburg"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_hindenburg_V82.T_Photo_uk_hindenburg_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_hindenburg_V839.T_Operation_hindenburg_V839")},
    {TEXT("uk.selle"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_selle_V82.T_Photo_uk_selle_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_selle_V839.T_Operation_selle_V839")},
    {TEXT("uk.valenciennes1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_valenciennes1918_V82.T_Photo_uk_valenciennes1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_valenciennes_V839.T_Operation_valenciennes_V839")},
    {TEXT("uk.sambre"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_uk_sambre_V82.T_Photo_uk_sambre_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_sambre_V839.T_Operation_sambre_V839")},
    {TEXT("de.arras"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_arras_V82.T_Photo_de_arras_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_arras_V839.T_Operation_arras_V839")},
    {TEXT("de.roeux1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_roeux1917_V82.T_Photo_de_roeux1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_arras_V839.T_Operation_arras_V839")},
    {TEXT("de.bullecourt1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_bullecourt1917_V82.T_Photo_de_bullecourt1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_bullecourt_V839.T_Operation_bullecourt_V839")},
    {TEXT("de.messines"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_messines_V82.T_Photo_de_messines_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_messines_V839.T_Operation_messines_V839")},
    {TEXT("de.messines_counter1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_messines_counter1917_V82.T_Photo_de_messines_counter1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_messines_V839.T_Operation_messines_V839")},
    {TEXT("de.ypres"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_ypres_V82.T_Photo_de_ypres_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_ypres_V839.T_Operation_ypres_V839")},
    {TEXT("de.langemarck1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_langemarck1917_V82.T_Photo_de_langemarck1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_ypres_V839.T_Operation_ypres_V839")},
    {TEXT("de.menin1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_menin1917_V82.T_Photo_de_menin1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_ypres_V839.T_Operation_ypres_V839")},
    {TEXT("de.cambrai"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_cambrai_V82.T_Photo_de_cambrai_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_cambrai_V839.T_Operation_cambrai_V839")},
    {TEXT("de.bourlon1917"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_bourlon1917_V82.T_Photo_de_bourlon1917_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_cambrai_V839.T_Operation_cambrai_V839")},
    {TEXT("de.winter_raid1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_winter_raid1918_V82.T_Photo_de_winter_raid1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_cambrai_V839.T_Operation_cambrai_V839")},
    {TEXT("de.michael"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_michael_V82.T_Photo_de_michael_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_michael_V839.T_Operation_michael_V839")},
    {TEXT("de.lys1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_lys1918_V82.T_Photo_de_lys1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_lys_V839.T_Operation_lys_V839")},
    {TEXT("de.villers1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_villers1918_V82.T_Photo_de_villers1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_villers_V839.T_Operation_villers_V839")},
    {TEXT("de.amiens"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_amiens_V82.T_Photo_de_amiens_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_amiens_V839.T_Operation_amiens_V839")},
    {TEXT("de.scarpe1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_scarpe1918_V82.T_Photo_de_scarpe1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_scarpe_V839.T_Operation_scarpe_V839")},
    {TEXT("de.hindenburg"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_hindenburg_V82.T_Photo_de_hindenburg_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_hindenburg_V839.T_Operation_hindenburg_V839")},
    {TEXT("de.selle"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_selle_V82.T_Photo_de_selle_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_selle_V839.T_Operation_selle_V839")},
    {TEXT("de.valenciennes1918"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_valenciennes1918_V82.T_Photo_de_valenciennes1918_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_valenciennes_V839.T_Operation_valenciennes_V839")},
    {TEXT("de.sambre"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_de_sambre_V82.T_Photo_de_sambre_V82"),TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_sambre_V839.T_Operation_sambre_V839")},
    {TEXT("fr.mort_homme"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_mort_homme_V82.T_Photo_fr_mort_homme_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_mort_homme_V82.T_Map_fr_mort_homme_V82")},
    {TEXT("fr.vaux"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_vaux_V82.T_Photo_fr_vaux_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_vaux_V82.T_Map_fr_vaux_V82")},
    {TEXT("fr.douaumont"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_douaumont_V82.T_Photo_fr_douaumont_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_douaumont_V82.T_Map_fr_douaumont_V82")},
    {TEXT("fr.louvemont"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_louvemont_V82.T_Photo_fr_louvemont_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_louvemont_V82.T_Map_fr_louvemont_V82")},
    {TEXT("fr.chemin_dames"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_chemin_dames_V82.T_Photo_fr_chemin_dames_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_chemin_dames_V82.T_Map_fr_chemin_dames_V82")},
    {TEXT("fr.malmaison"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_malmaison_V82.T_Photo_fr_malmaison_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_malmaison_V82.T_Map_fr_malmaison_V82")},
    {TEXT("fr.matz"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_matz_V82.T_Photo_fr_matz_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_matz_V82.T_Map_fr_matz_V82")},
    {TEXT("fr.champagne_defence"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_champagne_defence_V82.T_Photo_fr_champagne_defence_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_champagne_defence_V82.T_Map_fr_champagne_defence_V82")},
    {TEXT("fr.soissons"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_soissons_V82.T_Photo_fr_soissons_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_soissons_V82.T_Map_fr_soissons_V82")},
    {TEXT("fr.sommepy"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_fr_sommepy_V82.T_Photo_fr_sommepy_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_fr_sommepy_V82.T_Map_fr_sommepy_V82")},
    {TEXT("ru.galicia"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_galicia_V82.T_Photo_ru_galicia_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_galicia_V82.T_Map_ru_galicia_V82")},
    {TEXT("ru.przemysl"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_przemysl_V82.T_Photo_ru_przemysl_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_przemysl_V82.T_Map_ru_przemysl_V82")},
    {TEXT("ru.gorlice"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_gorlice_V82.T_Photo_ru_gorlice_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_gorlice_V82.T_Map_ru_gorlice_V82")},
    {TEXT("ru.warsaw_rearguard"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_warsaw_rearguard_V82.T_Photo_ru_warsaw_rearguard_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_warsaw_rearguard_V82.T_Map_ru_warsaw_rearguard_V82")},
    {TEXT("ru.naroch"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_naroch_V82.T_Photo_ru_naroch_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_naroch_V82.T_Map_ru_naroch_V82")},
    {TEXT("ru.lutsk"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_lutsk_V82.T_Photo_ru_lutsk_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_lutsk_V82.T_Map_ru_lutsk_V82")},
    {TEXT("ru.czernowitz"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_czernowitz_V82.T_Photo_ru_czernowitz_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_czernowitz_V82.T_Map_ru_czernowitz_V82")},
    {TEXT("ru.kovel"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_kovel_V82.T_Photo_ru_kovel_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_kovel_V82.T_Map_ru_kovel_V82")},
    {TEXT("ru.kerensky"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_kerensky_V82.T_Photo_ru_kerensky_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_kerensky_V82.T_Map_ru_kerensky_V82")},
    {TEXT("ru.riga"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Loading/T_Photo_ru_riga_V82.T_Photo_ru_riga_V82"),TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/T_Map_ru_riga_V82.T_Map_ru_riga_V82")},
};
FString MissionKey(const AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M)
{
    if(!B||!M)return FString();
    if(WarfareFRRUCampaignV82::Active(B))return M->MapKey;
    return FString(B->GetPlayerFaction()==EFrontlineFaction::German?TEXT("de."):TEXT("uk."))+M->MapKey;
}
const FAssets* Find(const FString& Key)
{for(const auto& A:Assets)if(Key==A.Mission)return &A;return nullptr;}
UTexture2D* Texture(const TCHAR* Path)
{
    if(!Path||!*Path)return nullptr;
    if(!FrontlineUIAssetsV8213::BaselineMode())return FrontlineUIAssetsV8213::Texture(Path);
    // An absent candidate must not synchronously reload on every loading frame.
    static TMap<FString,TStrongObjectPtr<UTexture2D>> Loaded;
    if(auto* Existing=Loaded.Find(Path))return Existing->Get();
    if(auto* Image=FrontlineUIAssetsV8213::Texture(Path))return Loaded.Add(Path,TStrongObjectPtr<UTexture2D>(Image)).Get();
    return nullptr;
}
void PreloadAvailable()
{
    if(FrontlineUIAssetsV8213::BaselineMode())return;
    static bool Requested=false;if(Requested)return;Requested=true;
    TArray<FString> Paths;
    for(const auto& A:Assets)
        if(FCString::Strncmp(A.Mission,TEXT("uk."),3)==0||FCString::Strncmp(A.Mission,TEXT("de."),3)==0)
            for(const TCHAR* Path:{A.Photo,A.OperationMap})if(Path&&*Path)Paths.AddUnique(Path);
    FrontlineUIAssetsV8213::PreloadTextures(Paths);
}
UTexture2D* FallbackPhoto(const AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M)
{
    const int32 Slot=FrontlineCampaignV51::PhotoForOperation(M?M->Id:INDEX_NONE);
    if(auto* T=FrontlineCampaignV51::Photo(Slot>=0?Slot:0);FrontlineTextureReady(T))return T;
    for(int32 I=0;I<3;++I)if(auto* T=FrontlineCampaignV51::Photo(I);FrontlineTextureReady(T))return T;
    for(auto& T:Artwork().ArchivePhotosV35)if(FrontlineTextureReady(T.Get()))return T.Get();
    if(auto* T=MenuCoverV63(B);FrontlineTextureReady(T))return T;
    for(auto& T:Artwork().Keyart)if(FrontlineTextureReady(T.Get()))return T.Get();
    return nullptr;
}
void FallbackReceipt(const FString& Key,const TCHAR* Kind,const TCHAR* Expected,const UTexture2D* Actual)
{
    static TSet<FString> Recorded;
    const FString Token=Key+TEXT(".")+Kind+TEXT(".")+(Actual?Actual->GetPathName():TEXT("missing"));
    if(Recorded.Contains(Token))return;Recorded.Add(Token);
    UE_LOG(LogTemp,Warning,TEXT("WF_LOADING_V82_FALLBACK mission=%s kind=%s expected=%s actual=%s"),
        *Key,Kind,Expected&&*Expected?Expected:TEXT("unbound"),Actual?*Actual->GetPathName():TEXT("missing"));
}
UTexture2D* OperationMapV8447(const AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M)
{
    if(!B||!M)return nullptr;
    const FString Key=MissionKey(B,M);const auto* A=Find(Key);
    // Loading and the filed return resolve the same action, including when
    // SelectedMission has already advanced. Never substitute another edition
    // of its map while the canonical texture is streaming.
    auto* Map=A?Texture(A->OperationMap):nullptr;
    if(!FrontlineTextureReady(Map))
    {
        static TSet<FString> Recorded;
        if(!Recorded.Contains(Key)){Recorded.Add(Key);UE_LOG(LogTemp,Warning,TEXT("WF_OPERATION_MAP_V8447_NOT_READY mission=%s expected=%s"),*Key,A?A->OperationMap:TEXT("unbound"));}
    }
    return Map;
}
// The national loading page cannot paint an empty canonical map while its
// asynchronous object or editor resource is pending. Poll only this action.
bool SelectedArtworkReadyV87(AWarfareFrontlineGameMode* B,FFrontlineScreenState& S)
{
    if(!B||!WarfareFRRUCampaignV82::Active(B))return true;
    const auto* M=B->GetSelectedCampaignMission();const FString Key=MissionKey(B,M);const auto* A=Find(Key);
    bool Ready=A&&A->Photo&&*A->Photo&&A->OperationMap&&*A->OperationMap;
    if(Ready)for(const TCHAR* Path:{A->Photo,A->OperationMap})
    {
        auto* T=Texture(Path);const auto* R=T?T->GetResource():nullptr;
        const bool ResourceReady=FrontlineTextureReady(T)&&R&&R->IsInitialized()&&R->TextureRHI.IsValid();
        if(ResourceReady)S.PreparationTextures.AddUnique(T);
        Ready&=ResourceReady;
    }
    const bool Failed=!A||(A&&(!A->Photo||!*A->Photo||!A->OperationMap||!*A->OperationMap))
        ||(!Ready&&FrontlineUIAssetsV8213::PreloadSettled()&&!FrontlineUIAssetsV8213::PreloadReady());
    if(Failed&&S.PreparationError.IsEmpty())
    {
        S.PreparationError=WarRoom52::L(TEXT("Interface artwork could not be loaded."),TEXT("界面原画未能载入。"));
        UE_LOG(LogTemp,Error,TEXT("WF_LOADING_V87_ART_FAILURE mission=%s photo=%s map=%s"),*Key,A&&A->Photo?A->Photo:TEXT("unbound"),A&&A->OperationMap?A->OperationMap:TEXT("unbound"));
    }
    return Ready;
}
void DrawOperationMapV8447(const FFrontlineCanvas& UI,const AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M,float X,float Y,float W,float H,FLinearColor Tint=FLinearColor::White)
{
    auto* Map=OperationMapV8447(B,M);
    if(WarfareFRRUCampaignV82::Active(B))UI.ImageContain(Map,X,Y,W,H,Tint);
    else
    {
        // Preserve the accepted loading aperture even in a smaller return
        // frame; both sheets therefore show the exact same source crop.
        constexpr float Aspect=1080.f/428.f;
        const float MapW=FMath::Min(W,H*Aspect),MapH=MapW/Aspect;
        UI.ImageCover(Map,X+(W-MapW)*.5f,Y+(H-MapH)*.5f,MapW,MapH,Tint);
    }
}
void RunMapChecksV8447(TFunctionRef<void(const FString&,bool)> Check)
{
    bool Complete=true,Canonical=true,Pair=true;TSet<FString> Keys;
    for(EFrontlineFaction F:{EFrontlineFaction::British,EFrontlineFaction::German})
    {
        const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(F);Complete&=Missions.Num()==20;
        for(const auto& M:Missions)
        {
            const FString Key=FString(F==EFrontlineFaction::German?TEXT("de."):TEXT("uk."))+M.MapKey;const auto* A=Find(Key);Keys.Add(Key);
            Complete&=A&&A->OperationMap&&*A->OperationMap;
            if(!A)continue;Canonical&=FString(A->OperationMap).StartsWith(TEXT("/Game/Warfare/UI/V839/Maps/T_Operation_"));
            const auto* Twin=Find(FString(F==EFrontlineFaction::German?TEXT("uk."):TEXT("de."))+M.MapKey);
            Pair&=Twin&&FString(Twin->OperationMap)==A->OperationMap;
        }
    }
    Check(TEXT("v8447_loading_and_return_all_40_uk_de_actions_have_canonical_bindings"),Complete&&Keys.Num()==40);
    Check(TEXT("v8447_action_maps_keep_v839_revision_for_all_uk_de_actions"),Canonical);
    Check(TEXT("v8447_same_action_uk_de_returns_share_authoritative_map"),Pair);
    Check(TEXT("v8447_unknown_operation_has_no_random_map_substitution"),!Find(TEXT("uk.missing_operation"))&&!OperationMapV8447(nullptr,nullptr));
}
void Prewarm(AWarfareFrontlineGameMode* B,FFrontlineScreenState& S)
{
    FrontlineBlastCompositionV8221::Prewarm();
    if(B&&!WarfareFRRUCampaignV82::Active(B)&&!FFrontlineArmourWreckV8220Access::Warm())
        UE_LOG(LogTemp,Error,TEXT("WF_V8220_WRECK_PREWARM_INCOMPLETE"));
    const auto* M=B?B->GetSelectedCampaignMission():nullptr;const auto* A=Find(MissionKey(B,M));
    // Request the selected pair together; do not preload every private national map.
    if(A)FrontlineUIAssetsV8213::PreloadTextures({A->Photo,A->OperationMap});
    if(A)for(const TCHAR* Path:{A->Photo,A->OperationMap})if(auto* T=Texture(Path))S.PreparationTextures.AddUnique(T);
    SelectedArtworkReadyV87(B,S);
    if(auto* T=FallbackPhoto(B,M))S.PreparationTextures.AddUnique(T);
}
FString Narrative(AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M)
{
    // All four armies use the authored loading narrative when one is available.
    const FString Key=MissionKey(B,M);
    for(const auto& Row:LoadingNarrativesV82)if(Key==Row.Key)return WarRoom52::L(Row.En,Row.Zh);
    if(!M)return WarRoom52::L(TEXT("The company waits for its orders. Beyond the assembly area, the front is still in motion."),TEXT("部队在集合点等待命令，前沿的枪声还没有停歇。"));
    // Receiver fields already contain the bilingual authoring overrides.
    // ObjectiveText is a rule summary and does not belong on this narrative sheet.
    const FString Context=M->HistoricalContext,Brief=M->Briefing;
    return Context.IsEmpty()?Brief:Context;
}
void Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FFrontlineScreenState& S)
{
    const auto* M=B->GetSelectedCampaignMission();const FString Key=MissionKey(B,M);const auto* A=Find(Key);
    UTexture2D* Photo=A?Texture(A->Photo):nullptr;
    if(!FrontlineTextureReady(Photo)){Photo=FallbackPhoto(B,M);FallbackReceipt(Key,TEXT("photo"),A?A->Photo:nullptr,Photo);}
    // Fill the viewport with a complete photograph, including non-16:9 margins.
    const float X=-UI.OX/UI.Scale,Y=-UI.OY/UI.Scale,W=UI.Surface->ClipX/UI.Scale,H=UI.Surface->ClipY/UI.Scale;
    UI.ImageCover(Photo,X,Y,W,H);
    UI.Hit(0,0,1920,1080,TEXT("Blocked"),60);
    UI.Gradient(X,Y,W,390,FLinearColor(.018f,.021f,.022f,.42f),FLinearColor(.018f,.021f,.022f,0),true);
    const bool Chinese=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese;
    if(Chinese)UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("战地简报"),86,88,1044,38,20,Paper);
    UI.BilingualTitle(TextReviewV829::Loading(TEXT("title"),M?M->Title:B->GetFactionName()),86,141,1044,116,Chinese?34.f:49.f,Paper);
    UI.RoleLabel(EFrontlineTextRoleV8210::Typed,FrontlineLocalizationV35::English(TextReviewV829::Loading(TEXT("date"),M?M->DateLabel:TEXT(""))),88,274,1020,32,20,Paper);

    constexpr float MX=82,MY=422,MW=1080,MH=428;
    UI.PaperSheet(MX-16,MY-48,MW+32,MH+70);
    const FLinearColor Ink(.035f,.038f,.035f),Quiet(.16f,.17f,.15f);
    if(Chinese)UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("作战地域"),MX+9,MY-35,MW-18,26,17,Ink);
    // V839 UK/DE sheets share this panel's wide aspect. Cover crops only
    // excess edges and keeps circles and roads undistorted inside the frame.
    DrawOperationMapV8447(UI,B,M,MX,MY,MW,MH);

    UI.PaperSheet(1244,96,608,894);
    if(Chinese)UI.RoleLabel(EFrontlineTextRoleV8210::Label,TEXT("前线形势"),1277,123,542,33,20.5f,Ink);
    else UI.BilingualLabel(TEXT("FIELD NOTES"),TEXT("前线形势"),1277,123,542,33,19,Ink);
    UI.Line(1277,171,1819,171,Quiet*.55f,2);
    UI.Line(1277,176,1819,176,Quiet*.4f);
    const FString Prose=UI.DisplayText(TextReviewV829::Loading(TEXT("narrative"),Narrative(B,M)));
    TArray<FString> Blocks;Prose.ParseIntoArray(Blocks,TEXT("\n\n"),false);
    if(Blocks.Num()==4)
    {
        // The authored diary heading, two narrative paragraphs and historian's
        // note are separate records. Never infer a prose boundary from punctuation.
        UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Blocks[0],1277,201,542,64,20,Ink);
        const float SecondY=UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Blocks[1],1277,278,542,348,20,Ink)+16;
        UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Blocks[2],1277,SecondY,542,626-SecondY,20,Ink);
        UI.Line(1277,647,1819,647,Quiet*.35f);
        UI.RoleParagraph(EFrontlineTextRoleV8210::Annotation,Blocks[3],1277,665,542,91,20,WarRoom52::Red);
    }
    else UI.RoleParagraph(EFrontlineTextRoleV8210::Text,Prose,1277,201,542,554,20,Ink);
    const FString Army=ArmyNameV8217(WarfareFRRUCampaignV82::Active(B)?WarfareFRRUCampaignV82::Nation(B):EFRRUNationV81(uint8(B->GetPlayerFaction())));
    UI.RoleLabel(EFrontlineTextRoleV8210::Plate,Army,1277,774,542,35,Chinese?20.f:25.f,Ink);
    UI.Line(1277,831,1819,831,Quiet*.35f);
    const bool Failed=!S.PreparationError.IsEmpty();
    if(!Failed)
    {
        // Once the scene is ready, later mip requests cannot reverse its caption.
        bool Resident=true;
        for(const auto& Weak:S.PreparationTextures)
        {
            if(auto* T=Weak.Get()){const auto R=T->GetStreamableResourceState();Resident&=R.NumResidentLODs>=R.NumRequestedLODs;}
            else Resident=false;
        }
        const bool SceneRendered=B->Phase==EFrontlinePhase::Fighting&&S.PreparationSceneFrames>0;
        const bool Entering=EnteringCaptionV844(B,S,Resident,SceneRendered);
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,WarRoom52::L(
            Entering?TEXT("Entering the battlefield"):TEXT("Loading the battlefield"),
            Entering?TEXT("即将进入战场"):TEXT("正在载入战场")),1277,850,542,40,21,Ink);
    }
    else
    {
        const auto* Run=B->GetCurrentCampaign();
        const bool Unconfirmed=Run&&(!S.AcceptedLoadoutCampaign.IsValid()||S.AcceptedLoadoutCampaign!=Run->Id||S.AcceptedLoadoutMission!=Run->SelectedMission);
        UI.RoleLabel(EFrontlineTextRoleV8210::Text,WarRoom52::L(
            Unconfirmed?TEXT("Deployment not confirmed"):TEXT("The operation could not begin."),
            Unconfirmed?TEXT("出战序列尚未确认"):TEXT("行动未能开始。")),1277,850,542,40,20,WarRoom52::Red);
        UI.RoleParagraph(EFrontlineTextRoleV8210::Text,WarRoom52::L(
            Unconfirmed?TEXT("Return to the operations room and confirm deployment."):TEXT("Return to the operations room and try again."),
            Unconfirmed?TEXT("请返回作战室确认出战序列。"):TEXT("请返回作战室重试。")),1277,902,542,40,17,Quiet);
        UI.PageButtonV73(1277,945,542,34,WarRoom52::L(TEXT("RETURN TO OPERATIONS"),TEXT("返回作战室")),TEXT("Esc"),TEXT("PreparationBack"),true,false,70);
    }
}
}
