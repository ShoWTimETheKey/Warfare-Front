// One authored identity at every title-bearing UI surface. The transparent
// source keeps its own aspect ratio; title type never inherits a local font.
void DrawBrandTitleV72(const FFrontlineCanvas& UI,float X,float Y,float W,float H,float Opacity=1.f)
{
    if(FrontlineTextureReady(Artwork().BrandWordmarkV72))
    {
        auto* Source=Artwork().BrandWordmarkV72.Get();
        const float Fit=FMath::Min(W/Source->GetSizeX(),H/Source->GetSizeY());
        const float Width=Source->GetSizeX()*Fit,Height=Source->GetSizeY()*Fit;
        UI.ImageContain(Source,X,Y+(H-Height)*.5f,Width,Height,FLinearColor(1,1,1,Opacity));
    }
    else UI.PrimaryLabel(TEXT("WARFARE FRONT"),X,Y,W,H,FMath::Min(H*.46f,W*.078f),FLinearColor(.81f,.80f,.73f,Opacity));
}

int32 ResearchCardKeyV72(const AWarfareFrontlineGameMode* B,int32 Id,bool Armour)
{return (int32(B->GetPlayerFaction())*128)+(Armour?64:0)+Id;}
float ResearchColourV72(const AWarfareFrontlineGameMode* B,int32 Id,bool Owned,bool Armour=false)
{
    auto& S=ScreenState(B->GetWorld()->GetFirstPlayerController());
    const FGuid Campaign=B->GetCurrentCampaign()?B->GetCurrentCampaign()->Id:FGuid();
    if(S.ResearchCampaignV72!=Campaign){S.ResearchCampaignV72=Campaign;S.ResearchRevealsV72.Empty();}
    if(!Owned)return 0.f;
    if(const double* Started=S.ResearchRevealsV72.Find(ResearchCardKeyV72(B,Id,Armour)))
        return FMath::SmoothStep(0.f,.65f,float(FPlatformTime::Seconds()-*Started));
    return 1.f;
}
void DrawResearchPhotoRegionsV82(const FFrontlineCanvas& UI,const FFrontlineArtRegionV71& Art,
    const FFrontlineArtRegionV71& Archive,int32 CardKey,float Reveal,float X,float Y,float W,float H)
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
    if(Art.ContentRectV87.Width()>0&&Art.ContentRectV87.Height()>0)
    {
        const auto& R=Art.ContentRectV87;
        if(R.Min.X<0||R.Min.Y<0||R.Max.X>Art.Texture->GetSizeX()||R.Max.Y>Art.Texture->GetSizeY())return;
        U=R.Min.X/float(Art.Texture->GetSizeX());V=R.Min.Y/float(Art.Texture->GetSizeY());
        DU=R.Width()/float(Art.Texture->GetSizeX());DV=R.Height()/float(Art.Texture->GetSizeY());
    }
    float AU=0,AV=0,ADU=1,ADV=1;
    if(FrontlineTextureReady(Archive.Texture)&&Archive.Slot!=INDEX_NONE)
    {
        const float IU=1.f/Archive.Texture->GetSizeX(),IV=1.f/Archive.Texture->GetSizeY();
        AU=(Archive.Slot%Archive.Columns)/float(Archive.Columns)+IU;
        AV=(Archive.Slot/Archive.Columns)/float(Archive.Rows)+IV;
        ADU=1.f/Archive.Columns-2*IU;ADV=1.f/Archive.Rows-2*IV;
    }
    if(FrontlineTextureReady(Archive.Texture)&&Archive.ContentRectV87.Width()>0&&Archive.ContentRectV87.Height()>0)
    {
        const auto& R=Archive.ContentRectV87;
        if(R.Min.X<0||R.Min.Y<0||R.Max.X>Archive.Texture->GetSizeX()||R.Max.Y>Archive.Texture->GetSizeY())return;
        AU=R.Min.X/float(Archive.Texture->GetSizeX());AV=R.Min.Y/float(Archive.Texture->GetSizeY());
        ADU=R.Width()/float(Archive.Texture->GetSizeX());ADV=R.Height()/float(Archive.Texture->GetSizeY());
    }
    UI.ArchivePhotoV72(Art.Texture.Get(),Archive.Texture.Get(),CardKey,X,Y,W,H,U,V,DU,DV,AU,AV,ADU,ADV,Reveal);
    if(Reveal>0.f&&Reveal<1.f)
    {
        // An exposure pulse catches the paper edge as the colour develops.
        // It is a single purchase acknowledgement, never an idle animation.
        const float Pulse=FMath::Sin(Reveal*PI)*.34f;
        UI.Line(X,Y+H,X+W,Y+H,FLinearColor(.70f,.62f,.43f,Pulse),1.4f);
    }
}
void DrawResearchPhotoV72(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFrontlineArtRegionV71& Art,
    int32 Id,bool Owned,bool Armour,float X,float Y,float W,float H)
{
    if(!FrontlineTextureReady(Art.Texture))return;
    const int32 Nation=int32(B->GetPlayerFaction()),ArchiveId=Id+(Armour?28:0);
    if(Nation<0||Nation>=2||ArchiveId<0||ArchiveId>=UE_ARRAY_COUNT(Artwork().ArchivePaintingsV74[0]))return;
    const auto& Archive=Artwork().ArchivePaintingsV74[Nation][ArchiveId];
    DrawResearchPhotoRegionsV82(UI,Art,Archive,ResearchCardKeyV72(B,Id,Armour),ResearchColourV72(B,Id,Owned,Armour),X,Y,W,H);
}
void SynchroniseNationalResearchV82(AWarfareFrontlineGameMode* B,FFrontlineScreenState& State)
{
    const FGuid Campaign=WarfareFRRUCampaignV82::CampaignId(B);
    if(State.NationalResearchCampaignV82!=Campaign)
    {State.NationalResearchCampaignV82=Campaign;State.NationalResearchRevealsV82.Empty();State.FRRUResearchPageV87=0;State.TechnologyFocus=INDEX_NONE;}
}
bool PurchaseNationalResearchV82(AWarfareFrontlineGameMode* B,const FString& StableId)
{
    const bool WasOwned=WarfareFRRUCampaignV82::OwnsOption(B,StableId);
    const bool Replay=WarfareFRRUCampaignV82::ArchiveReplayActiveV87(B);
    const bool Purchased=Replay?WarfareFRRUCampaignV82::SetArchiveOptionEnabledV87(B,StableId,!WasOwned):WarfareFRRUCampaignV82::PurchaseOption(B,StableId);
    if(Purchased&&Replay&&WasOwned)
    {
        auto& State=ScreenState(B->GetWorld()->GetFirstPlayerController());
        State.NationalResearchRevealsV82.Remove(StableId);SynchroniseNationalResearchV82(B,State);
    }
    if(Purchased&&!WasOwned)
    {
        auto& State=ScreenState(B->GetWorld()->GetFirstPlayerController());
        SynchroniseNationalResearchV82(B,State);
        State.NationalResearchRevealsV82.Add(StableId,FPlatformTime::Seconds());
    }
    return Purchased;
}
float NationalResearchColourV82(AWarfareFrontlineGameMode* B,const FFRRUTacticalOptionV82& Option)
{
    auto& State=ScreenState(B->GetWorld()->GetFirstPlayerController());
    SynchroniseNationalResearchV82(B,State);
    if(!Option.Owned)return 0.f;
    if(const double* Started=State.NationalResearchRevealsV82.Find(Option.Id))
        return FMath::SmoothStep(0.f,.65f,float(FPlatformTime::Seconds()-*Started));
    return 1.f;
}
void DrawNationalResearchPhotoV82(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,const FFRRUTacticalOptionV82& Option,
    UTexture2D* Texture,int32 ColourCell,int32 ArchiveCell,int32 Columns,int32 Rows,float X,float Y,float W,float H)
{
    FFrontlineArtRegionV71 Colour;Colour.Texture=Texture;Colour.Slot=ColourCell;Colour.Columns=Columns;Colour.Rows=Rows;
    auto Archive=Colour;Archive.Slot=ArchiveCell;
    if(Texture&&Texture->GetName()==TEXT("T_RU_RESEARCH")&&Texture->GetSizeX()==1536&&Texture->GetSizeY()==1024&&Columns==6&&Rows==4)
    {
        // Actual frozen atlas separators occur at 236/237, 499/500 and
        // 745/746, not at equal quarter heights. Sample paired square content
        // inside those separators; do not stretch a neighbouring white gutter.
        static const int32 Top[]={0,238,501,747},Bottom[]={236,499,745,1024};
        auto Content=[](int32 Slot)
        {
            const int32 Row=Slot/6,Column=Slot%6;if(Row<0||Row>=4||Column<0||Column>=6)return FIntRect(0,0,0,0);
            const int32 W=250,H=Bottom[Row]-Top[Row]-6,Side=FMath::Min(W,H);
            const int32 X0=Column*256+3+(W-Side)/2,Y0=Top[Row]+3+(H-Side)/2;
            return FIntRect(X0,Y0,X0+Side,Y0+Side);
        };
        Colour.ContentRectV87=Content(ColourCell);Archive.ContentRectV87=Content(ArchiveCell);
    }
    // The shared material cache has a separate national key range. Purchase
    // timing uses campaign identity and the complete stable option ID.
    const int32 CardKey=1024+int32(WarfareFRRUCampaignV82::Nation(B))*128+Option.Index;
    DrawResearchPhotoRegionsV82(UI,Colour,Archive,CardKey,NationalResearchColourV82(B,Option),X,Y,W,H);
}
