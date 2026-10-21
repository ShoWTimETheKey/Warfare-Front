void V32Atlas(const FFrontlineCanvas& UI,UTexture2D* T,int32 Slot,int32 Columns,int32 Rows,float X,float Y,float W,float H,bool Gray=false)
{
    if(!FrontlineTextureReady(T)||Columns<=0||Rows<=0||Slot<0||Slot>=Columns*Rows)return;
    const float IU=1.f/T->GetSizeX(),IV=1.f/T->GetSizeY();
    UI.Image(T,X,Y,W,H,FLinearColor(.96f,.97f,.94f,1),(Slot%Columns)/float(Columns)+IU,(Slot/Columns)/float(Rows)+IV,
        1.f/Columns-2*IU,1.f/Rows-2*IV,Gray,true);
}
TArray<EFrontlineRole> FieldRolesV32(const AWarfareFrontlineGameMode* B)
{
    TArray<EFrontlineRole> R;const auto L=FrontlineCampaignV32::GetLoadout(B);
    if(WarfareFRRUCampaignV82::Active(B))
    {for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(L.SelectedRoles&(1u<<I))R.Add(EFrontlineRole(I));return R;}
    if(!L.bSelectRoles)
    {
        for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(L.EligibleRoles&(1u<<I))R.Add(FrontlineGameplayV52::RecruitVariant(B,EFrontlineRole(I)));
        for(int32 I=0;I<5&&R.Num()<5;++I)R.AddUnique(FrontlineGameplayV52::RecruitVariant(B,EFrontlineRole(I)));
        return R;
    }
    for(int32 I=0;I<int32(EFrontlineRole::Count);++I)if(L.SelectedRoles&(1u<<I))R.Add(FrontlineGameplayV52::RecruitVariant(B,EFrontlineRole(I)));
    return R;
}
TArray<EFrontlineSupport> FieldSupportsV32(const AWarfareFrontlineGameMode* B)
{ return FrontlineCampaignV32::GetEquippedSupports(B); }
void DrawArtRegionV71(const FFrontlineCanvas& UI,const FFrontlineArtRegionV71& Art,float X,float Y,float W,float H,bool Gray=false,bool Contain=true,FLinearColor Tint=FLinearColor::White)
{
    if(!FrontlineTextureReady(Art.Texture))return;
    float U=0,V=0,DU=1,DV=1;
    if(Art.Slot!=INDEX_NONE)
    {
        if(Art.Columns<=0||Art.Rows<=0||Art.Slot<0||Art.Slot>=Art.Columns*Art.Rows)return;
        const float IU=1.f/Art.Texture->GetSizeX(),IV=1.f/Art.Texture->GetSizeY();
        U=(Art.Slot%Art.Columns)/float(Art.Columns)+IU;V=(Art.Slot/Art.Columns)/float(Art.Rows)+IV;
        DU=1.f/Art.Columns-2*IU;DV=1.f/Art.Rows-2*IV;
    }
    UI.Image(Art.Texture.Get(),X,Y,W,H,Tint,U,V,DU,DV,Gray,Contain);
}
int32 RoleVariantV71(const AWarfareFrontlineGameMode* B,EFrontlineRole Role)
{
    if(!B)return INDEX_NONE;
    if(Role==EFrontlineRole::Marksman&&B->HasScopedMarksmen())return 0;
    if(Role==EFrontlineRole::Mills&&B->HasStormtroopers())return 1;
    if(Role==EFrontlineRole::Officer&&B->HasCommissionedOfficers())return 2;
    return INDEX_NONE;
}
enum class EFrontlinePortraitContextV72:uint8 {Compact,Narrative};
FFrontlineArtRegionV71 ResolveRoleArtworkV71(const AWarfareFrontlineGameMode* B,EFrontlineRole Role,EFrontlinePortraitContextV72 Context=EFrontlinePortraitContextV72::Compact)
{
    if(!B||int32(Role)<0||int32(Role)>=int32(EFrontlineRole::Count))return {};
    if(WarfareFRRUCampaignV82::Active(B))
    {
        if(!B->IsRoleUnlocked(Role))return {};
        const auto Nation=WarfareFRRUCampaignV82::Nation(B);const int32 R=int32(Role);
        if(Nation==EFRRUNationV81::French&&(R==1||R==9))
        {static TStrongObjectPtr<UTexture2D> Hotchkiss,Chauchat;auto& Texture=R==1?Hotchkiss:Chauchat;if(!Texture.IsValid())Texture.Reset(FrontlineUIAssetsV8213::Texture(R==1?TEXT("/Game/FRRUIntegration/FormalV82/UI/T_FR_Hotchkiss_Portrait.T_FR_Hotchkiss_Portrait"):TEXT("/Game/FRRUIntegration/FormalV82/UI/T_FR_Chauchat_Portrait.T_FR_Chauchat_Portrait")));return {Texture.Get()};}
        const int32 Slot=R==10?8:R;return {NationalAtlasV82(Nation,TEXT("PORTRAITS")),Slot,3,3};
    }
    const int32 I=int32(Role),F=int32(B->GetPlayerFaction()),Variant=RoleVariantV71(B,Role);
    auto& A=Artwork();
    if(F==1&&Role==EFrontlineRole::Marksman&&A.RolePaintingsV72[F][I].Texture)return A.RolePaintingsV72[F][I];
    if(Variant!=INDEX_NONE)
    {

        if(Variant==2)return {A.OfficerPortraits[F]};
        return {A.UpgradedPortraits[Variant==1?1:0]};
    }
    if(F==1&&I<3&&A.RolePaintingsV72[F][I].Texture)return A.RolePaintingsV72[F][I];
    if(Role==EFrontlineRole::MobileGun||Role==EFrontlineRole::Signals)
    {
        static TStrongObjectPtr<UTexture2D> Attachment;
        if(!Attachment.IsValid())Attachment.Reset(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V51/T_AttachmentPortraits_V51.T_AttachmentPortraits_V51")));
        if(Attachment.IsValid())return {Attachment.Get(),(Role==EFrontlineRole::Signals?2:0)+F,2,2};
        return {Role==EFrontlineRole::MobileGun?A.BasicPortraits[F*3+1].Get():A.OfficerPortraits[F].Get()};
    }
    if(I>=7)return {FrontlineCampaignV41::RolePortrait(Role)};
    if(I>=5)return {A.IllustrationsV32,(I-5)*2+F,6,4};
    return {I<3?A.BasicPortraits[F*3+I].Get():A.SpecialistPortraits[F*2+I-3].Get()};
}
void DrawRolePhotoV32(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,EFrontlineRole Role,float X,float Y,float W,float H,bool Gray=false,EFrontlinePortraitContextV72 Context=EFrontlinePortraitContextV72::Compact)
{
    const auto Art=ResolveRoleArtworkV71(B,Role,Context);
    const FLinearColor Tint=int32(Role)<5?FLinearColor(.95f,.96f,.94f,1):Art.Slot!=INDEX_NONE?FLinearColor(.96f,.97f,.94f,1):FLinearColor::White;
    DrawArtRegionV71(UI,Art,X,Y,W,H,Gray,true,Tint);
}
FString RoleCaptionV32(AWarfareFrontlineGameMode* B,EFrontlineRole Role)
{
    if(WarfareFRRUCampaignV82::Active(B))
    {const bool French=WarfareFRRUCampaignV82::Nation(B)==EFRRUNationV81::French;switch(Role){case EFrontlineRole::Rifle:return French?TEXT("LEBEL"):TEXT("MOSIN");case EFrontlineRole::Lewis:return French?TEXT("HOTCHKISS"):TEXT("MAXIM");case EFrontlineRole::Mills:return TEXT("GRENADIERS");case EFrontlineRole::Marksman:return TEXT("MARKSMEN");case EFrontlineRole::Officer:return TEXT("COMMAND");case EFrontlineRole::Flamethrower:return TEXT("SCHILT");case EFrontlineRole::Pioneer:return TEXT("SAPPERS");case EFrontlineRole::RifleGrenadier:return TEXT("VB");case EFrontlineRole::MobileGun:return TEXT("CHAUCHAT");case EFrontlineRole::Signals:return TEXT("SIGNALS");default:return TEXT("");}}
    const bool G=B->GetPlayerFaction()==EFrontlineFaction::German;
    switch(Role){
    case EFrontlineRole::Rifle:return G?TEXT("GEWEHR 98"):TEXT("SMLE");
    case EFrontlineRole::Lewis:return G?TEXT("MG 08/15"):TEXT("LEWIS");
    case EFrontlineRole::Mills:return B->HasStormtroopers()?TEXT("MP 18"):G?TEXT("GRENADIERS"):TEXT("MILLS");
    case EFrontlineRole::Marksman:return G?TEXT("JÄGER PATROL"):B->HasScopedMarksmen()?TEXT("SNIPERS"):TEXT("MARKSMEN");
    case EFrontlineRole::Officer:return B->HasCommissionedOfficers()?TEXT("OFFICER"):TEXT("NCO");
    case EFrontlineRole::Flamethrower:return G?TEXT("WEX"):TEXT("SCHILT");
    case EFrontlineRole::Pioneer:return G?TEXT("PIONEERS"):TEXT("SAPPERS");
    case EFrontlineRole::RifleGrenadier:return TEXT("NO.23");
    case EFrontlineRole::AntiTankRifle:return TEXT("T-GEWEHR");
    case EFrontlineRole::MobileGun:return G?TEXT("ESCORT MG"):TEXT("LEWIS ASSLT");
    case EFrontlineRole::Signals:return TEXT("OBSERVERS");
    default:return TEXT("");
    }
}
FString ArmourCaptionV70(EFrontlineVehicle Type)
{
    static const TCHAR* Names[]={TEXT("MK IV MALE"),TEXT("WHIPPET"),TEXT("A7V"),TEXT("BEUTE MALE"),TEXT("MK IV FEMALE"),TEXT("BEUTE FEMALE"),TEXT("MARK V"),TEXT("MK IV ESCORT"),TEXT("GRETCHEN"),TEXT("BEUTE ESCORT"),TEXT("MARK V*"),TEXT("AUSTIN 1918"),TEXT("ARMOURED TRAIN")};
    return int32(Type)>=0&&int32(Type)<UE_ARRAY_COUNT(Names)?Names[int32(Type)]:TEXT("");
}
FString RoleTipV32(EFrontlineRole R,const AWarfareFrontlineGameMode* B=nullptr)
{
    if(WarfareFRRUCampaignV82::Active(B))return WarfareFRRUEquipmentV82::RoleDescription(B,R,0);
    const FString Guide=FrontlinePlayerGuideV43::Unit(B,R);if(!Guide.IsEmpty())return Guide;
    switch(R){
    case EFrontlineRole::Lewis:return TEXT("架设机枪形成压制，副射手伴随掩护。");
    case EFrontlineRole::Mills:return TEXT("贴近敌壕，以手榴弹打破顽抗；开阔地带需要掩护。");
    case EFrontlineRole::Marksman:return TEXT("观察并优先打击敌方机枪，保持射界。");
    case EFrontlineRole::Officer:return TEXT("伴随部队，下令重整，缓解压制。");
    case EFrontlineRole::Flamethrower:return TEXT("喷火器需抵近才能发挥作用，火焰可逼退掩体后的守军；让步兵掩护操作手。");
    case EFrontlineRole::Pioneer:return TEXT("工兵沿推进方向开辟铁丝网和爆炸物通道，身后的部队可沿缺口通过。");
    case EFrontlineRole::RifleGrenadier:return TEXT("以枪榴弹压住近处枪位，射手需要稳定支撑与携弹手掩护。");
    case EFrontlineRole::AntiTankRifle:return TEXT("支撑重型反坦克枪逐发射击装甲；装填缓慢，需要步兵掩护侧面。");
    default:return TEXT("用步枪火力掩护前进，夺取并驻守阵地。");
    }
}
struct FSupportArtworkV70
{
    UTexture2D* Texture=nullptr;
    int32 AtlasSlot=INDEX_NONE;
    int32 Columns=6,Rows=4;
};
FSupportArtworkV70 ResolveSupportArtworkV70(EFrontlineFaction Faction,EFrontlineSupport Kind)
{
    const int32 F=Faction==EFrontlineFaction::German?1:0;
    auto& A=Artwork();
    if(int32(Kind)>=0&&int32(Kind)<int32(EFrontlineSupport::Count))
    {
        const auto& New=A.SupportPaintingsV71[F][int32(Kind)];
        if(New.Texture)return {New.Texture.Get(),New.Slot,New.Columns,New.Rows};
    }
    switch(Kind)
    {
    case EFrontlineSupport::Artillery:return {A.SupportPaintings[0].Get()};
    case EFrontlineSupport::Smoke:return {A.SupportPaintings[1].Get()};
    case EFrontlineSupport::Mortar:return {F?A.SupportPaintingsV70[1][2].Get():A.SupportPaintings[2].Get()};
    case EFrontlineSupport::Gas:return {F?A.SupportPaintingsV70[1][3].Get():A.SupportPaintings[3].Get()};
    case EFrontlineSupport::AntiTank:return {A.SupportPaintingsV70[F][4].Get()};
    case EFrontlineSupport::AirStrike:return {F?A.SupportPaintingsV70[1][5].Get():A.SupportPaintings[5].Get()};
    case EFrontlineSupport::CreepingBarrage:return {A.IllustrationsV32.Get(),12+F};
    case EFrontlineSupport::AirBombing:return {A.IllustrationsV32.Get(),22+F};
    case EFrontlineSupport::AirRecon:
    {
        // Preserve the saved enum, but never reuse its retired observer-aircraft art.
        static TStrongObjectPtr<UTexture2D> Shrapnel;
        if(!Shrapnel.IsValid())Shrapnel.Reset(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V64/T_Shrapnel_V64.T_Shrapnel_V64")));
        return {Shrapnel.Get()};
    }
    default:return {};
    }
}
FFrontlineArtRegionV71 ResolveNationalSupportArtworkV82(EFRRUNationV81 Nation,EFrontlineSupport Support)
{
    const TCHAR* Prefix=Nation==EFRRUNationV81::French?TEXT("FR"):Nation==EFRRUNationV81::Russian?TEXT("RU"):nullptr;
    if(!Prefix)return {};
    // The receiver atlas is registered by support meaning, never enum ordinal.
    int32 Slot=INDEX_NONE;
    switch(Support)
    {
    case EFrontlineSupport::Artillery:Slot=0;break;
    case EFrontlineSupport::Smoke:Slot=1;break;
    case EFrontlineSupport::Mortar:Slot=2;break;
    case EFrontlineSupport::Gas:Slot=3;break;
    case EFrontlineSupport::AntiTank:Slot=4;break;
    case EFrontlineSupport::AirStrike:Slot=5;break;
    case EFrontlineSupport::CreepingBarrage:Slot=6;break;
    case EFrontlineSupport::AirRecon:Slot=7;break; // Archive-compatible shrapnel.
    case EFrontlineSupport::AirBombing:Slot=8;break;
    default:return {};
    }
    const FString Name=FString::Printf(TEXT("T_Support_%s_V82"),Prefix);
    const FString Path=TEXT("/Game/FRRUIntegration/FormalV82/UI/")+Name+TEXT(".")+Name;
    static TMap<FString,TStrongObjectPtr<UTexture2D>> Atlases;auto& Texture=Atlases.FindOrAdd(Path);
    if(!Texture.IsValid())Texture.Reset(FrontlineUIAssetsV8213::Texture(Path));
    if(!FrontlineTextureReady(Texture.Get())||Texture->GetSizeX()!=Texture->GetSizeY()||Texture->GetSizeX()%3)return {};
    return {Texture.Get(),Slot,3,3};
}
// Rally is a support order. Its illustration must not depend on whether an
// Officer infantry section is assigned to this particular operation.
FFrontlineArtRegionV71 ResolveNationalOfficerArtworkV87(EFRRUNationV81 Nation)
{
    if(Nation!=EFRRUNationV81::French&&Nation!=EFRRUNationV81::Russian)return {};
    return {NationalAtlasV82(Nation,TEXT("PORTRAITS")),int32(EFrontlineRole::Officer),3,3};
}
void DrawSupportPhotoV32(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,EFrontlineSupport S,float X,float Y,float W,float H,bool Gray=false)
{
    if(!B)return;
    if(WarfareFRRUCampaignV82::Active(B))
    {
        const auto Nation=WarfareFRRUCampaignV82::Nation(B);
        const auto Art=S==EFrontlineSupport::Rally?ResolveNationalOfficerArtworkV87(Nation):ResolveNationalSupportArtworkV82(Nation,S);
        DrawArtRegionV71(UI,Art,X,Y,W,H,Gray,false,FLinearColor(.96f,.97f,.94f,1));return;
    }
    const auto Art=ResolveSupportArtworkV70(B->GetPlayerFaction(),S);
    DrawArtRegionV71(UI,{Art.Texture,Art.AtlasSlot,Art.Columns,Art.Rows},X,Y,W,H,Gray,false,FLinearColor(.96f,.97f,.94f,1));
}
FFrontlineArtRegionV71 ResolveTechnologyArtworkV71(const AWarfareFrontlineGameMode* B,int32 Id)
{
    if(!B||Id<0||Id>=28)return {};
    const int32 F=int32(B->GetPlayerFaction());auto& A=Artwork();
    if(A.TechnologyPaintingsV71[F][Id].Texture)return A.TechnologyPaintingsV71[F][Id];
    if(Id<16)return {A.TechnologyPaintings[F][Id]};
    if(Id==20)
    {
        const auto Art=ResolveSupportArtworkV70(B->GetPlayerFaction(),EFrontlineSupport::AirRecon);
        return {Art.Texture,Art.AtlasSlot,Art.Columns,Art.Rows};
    }
    if(Id==27)return ResolveRoleArtworkV71(B,EFrontlineRole::MobileGun);
    if(Id>=24)return {FrontlineCampaignV41::TechnologyPortrait(Id,B->GetPlayerFaction())};
    return {A.IllustrationsV32,6+(Id-16)*2+F,6,4};
}
FFrontlineArtRegionV71 ResolveArmourTechnologyArtworkV71(EFrontlineFaction Faction,int32 Id)
{
    if(Id<0||Id>=9)return {};
    const int32 F=Faction==EFrontlineFaction::German?1:0;auto& A=Artwork();
    if(A.ArmourTechnologyPaintingsV71[F][Id].Texture)return A.ArmourTechnologyPaintingsV71[F][Id];
    if(Id==8)return {}; // The legacy plate contains exactly eight cells.
    static TStrongObjectPtr<UTexture2D> Legacy[2];auto& Art=Legacy[F];
    if(!Art.IsValid())Art.Reset(FrontlineUIAssetsV8213::Texture(F?TEXT("/Game/Warfare/UI/V63/T_ArmourTech_German.T_ArmourTech_German"):TEXT("/Game/Warfare/UI/V63/T_ArmourTech_British.T_ArmourTech_British")));
    return {Art.Get(),Id,4,2};
}
FString SupportCaptionV32(AWarfareFrontlineGameMode* B,EFrontlineSupport S)
{
    if(B&&WarfareFRRUCampaignV82::Active(B))switch(S)
    {
    case EFrontlineSupport::Artillery:return TEXT("ARTILLERY");
    case EFrontlineSupport::Smoke:return TEXT("SMOKE");
    case EFrontlineSupport::Rally:return TEXT("RALLY");
    case EFrontlineSupport::Mortar:return TEXT("MORTAR");
    case EFrontlineSupport::Gas:return TEXT("GAS");
    case EFrontlineSupport::AntiTank:return TEXT("ANTI-ARMOUR");
    case EFrontlineSupport::AirStrike:return TEXT("STRAFING");
    case EFrontlineSupport::CreepingBarrage:return TEXT("BARRAGE");
    case EFrontlineSupport::AirRecon:return TEXT("SHRAPNEL");
    case EFrontlineSupport::AirBombing:return TEXT("BOMBING");
    default:return TEXT("");
    }
    const bool G=B->GetPlayerFaction()==EFrontlineFaction::German;
    switch(S){
    case EFrontlineSupport::Artillery:return G?TEXT("7.7 cm"):TEXT("18 PDR");
    case EFrontlineSupport::Smoke:return TEXT("SMOKE");
    case EFrontlineSupport::Mortar:return G?TEXT("MINENWERFER"):TEXT("STOKES");
    case EFrontlineSupport::Gas:return TEXT("GAS");
    case EFrontlineSupport::AntiTank:return TEXT("FIELD GUN");
    case EFrontlineSupport::AirStrike:return G?TEXT("CL.II"):TEXT("F.2B");
    case EFrontlineSupport::CreepingBarrage:return TEXT("BARRAGE");
    case EFrontlineSupport::AirRecon:return TEXT("SHRAPNEL");
    case EFrontlineSupport::AirBombing:return TEXT("BOMBING");
    default:return TEXT("");
    }
}
const TCHAR* SupportKeyV32(EFrontlineSupport S)
{return S==EFrontlineSupport::Artillery?TEXT("E"):S==EFrontlineSupport::Smoke?TEXT("C"):S==EFrontlineSupport::Mortar?TEXT("V"):TEXT("");}
void RecruitFieldSlotV32(AWarfareFrontlineController* PC,int32 Slot)
{
    if(DecorationModalV8216(PC))return;
    auto* B=FrontlineFor(PC);if(!B||B->Phase!=EFrontlinePhase::Fighting||FRRUStoryModalV82(B))return;
    const auto& S=ScreenState(PC);if(S.Menu||S.Help||S.Settings||S.Technology||S.Preparing)return;
    const auto R=FieldRolesV32(B);if(R.IsValidIndex(Slot))B->Recruit(R[Slot]);
}
void DrawArchivePhotoV32(const FFrontlineCanvas& UI,int32 Slot,float X,float Y,float W,float H,FLinearColor Tint=FLinearColor::White)
{
    if(Slot>=4)
    {
        auto* Photo=FrontlineCampaignV51::Photo(Slot-4);
        if(FrontlineTextureReady(Photo)){UI.ImageCover(Photo,X,Y,W,H,Tint);return;}
        Slot=2;
    }
    auto* T=Artwork().ArchivePhotosV32.Get();if(!FrontlineTextureReady(T)||W<=0||H<=0)return;
    const float IU=1.f/T->GetSizeX(),IV=1.f/T->GetSizeY();
    // Fit the photograph before cropping the atlas, keeping faces and horizons level.
    const float A=float(T->GetSizeX())/T->GetSizeY(),DestA=W/H;
    float U=(Slot%2)*.5f+IU,V=(Slot/2)*.5f+IV,UW=.5f-2*IU,VH=.5f-2*IV;
    if(DestA>A){const float F=A/DestA;V+=VH*(1-F)*.5f;VH*=F;}else{const float F=DestA/A;U+=UW*(1-F)*.5f;UW*=F;}
    UI.Image(T,X,Y,W,H,Tint,U,V,UW,VH);
}
