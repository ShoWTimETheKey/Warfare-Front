// V30 textured emblems and contextual strength, at the actual game camera scale.
#include "WarfareFrontlineStatusTextV8217.inl"
void DrawEmblemV30(const FFrontlineCanvas& UI,int32 Slot,float X,float Y,float W,float H)
{
    if(Slot==4)
    {
        if(UI.ConsoleToolV79(7,X,Y,W,H))return;
        FrontlineLockV39::Draw(UI.Surface,nullptr,UI.Screen(X,Y),FVector2D(W*UI.Scale,H*UI.Scale));
        return;
    }
    UTexture2D* T=Artwork().EmblemsV30.Get();if(!FrontlineTextureReady(T))return;
    const float InsetU=2.f/T->GetSizeX(),InsetV=2.f/T->GetSizeY();
    UI.Image(T,X,Y,W,H,FLinearColor::White,(Slot%3)/3.f+InsetU,(Slot/3)/2.f+InsetV,1.f/3-2*InsetU,.5f-2*InsetV,false,true);
}
UTexture2D* CampaignMapTextureV41(AWarfareFrontlineGameMode* B,const FFrontlineCampaignMission* M)
{
    if(!M)return nullptr;
    if(WarfareFRRUCampaignV82::Active(B))
    {
        const FString Name=TEXT("T_Map_")+FString(M->MapKey).Replace(TEXT("."),TEXT("_"))+TEXT("_V82");
        const FString Path=TEXT("/Game/FRRUIntegration/FormalV82/UI/Operations/")+Name+TEXT(".")+Name;
        static TMap<FString,TStrongObjectPtr<UTexture2D>> NationalMaps;
        auto& Map=NationalMaps.FindOrAdd(Path);
        if(!Map.IsValid())Map.Reset(FrontlineUIAssetsV8213::Texture(Path));
        return Map.Get();
    }
    if(M->Id==16){static TStrongObjectPtr<UTexture2D> Survey;
        if(!Survey.IsValid())Survey.Reset(FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V62/T_RiquevalSurvey.T_RiquevalSurvey")));
        if(Survey.IsValid())return Survey.Get();}
    if(auto* New=FrontlineCampaignV41::NewMap(*M))return New;
    const int32 I=M->LegacyArtV41;
    if(I>=6&&I<10)return Artwork().LateMapsV32[I-6].Get();
    if(I==3&&M->Id==8&&B->GetPlayerFaction()==EFrontlineFaction::German)return Artwork().CambraiCounterV31.Get();
    return I>=0&&I<6?Artwork().MapsV30[I].Get():nullptr;
}
bool DrawPaintedMapV30(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,float X,float Y,float W,float H)
{
    auto* T=CampaignMapTextureV41(B,B->GetSelectedCampaignMission());if(!FrontlineTextureReady(T))return false;
    UI.ImageContain(T,X,Y,W,H);return true;
}
struct FCommanderBiographyV41 {FString Name,Role,Text;};
FCommanderBiographyV41 CommanderBiographyV41(const FFrontlineCampaignMission& M,int32 F)
{
    const int32 I=M.CommanderPortraitV41[F];FCommanderBiographyV41 R;
    if(F==1&&I==10)
    {
        R.Name=TEXT("费迪南德·冯·夸斯特");R.Role=TEXT("德军第六集团军");
        R.Text=TEXT("1850年出生的步兵将领，先后指挥第九军与近卫军，1917年秋接掌第六集团军。利斯河攻势中，他的部队向河沟与道路间的薄弱地段推进；眼前的快速突入仍需要后续步兵与补给接上。");return R;
    }
    if(I>=6){const auto& Old=WFV32Commanders::GetForMission(F==1,I);R.Name=Old.Name;R.Text=Old.Text;}
    else{const auto& Old=WFV31Commanders::GetForMission(F==1,I);R.Name=Old.Name;R.Text=Old.Text;}
    static const TCHAR* UK[]={TEXT("英军第三集团军"),TEXT("英军第二集团军"),TEXT("英军第五集团军"),TEXT("英军第三集团军"),TEXT("英军第五集团军"),TEXT("英军第四集团军"),TEXT("英军第一集团军"),TEXT("英军第四集团军"),TEXT("英军第四集团军"),TEXT("英军第三集团军")};
    static const TCHAR* DE[]={TEXT("德军第六集团军"),TEXT("德军第四集团军"),TEXT("德军第四集团军"),TEXT("德军第二集团军"),TEXT("德军第二集团军"),TEXT("德军第二集团军"),TEXT("德军第十七集团军"),TEXT("德军第二集团军"),TEXT("德军第二集团军"),TEXT("德军第十七集团军")};
    R.Role=F?DE[FMath::Clamp(I,0,9)]:UK[FMath::Clamp(I,0,9)];
    if(F==1&&M.Id==2)R.Role=TEXT("德军第六集团军");
    return R;
}
void DrawCommandersV30(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,float X,float Y)
{
    const auto* M=B->GetSelectedCampaignMission();if(!M)return;
    int32 Hover=INDEX_NONE;
    for(int32 F=0;F<2;++F)
    {
        const float CX=X+F*535;const auto Bio=CommanderBiographyV41(*M,F);const int32 I=M->CommanderPortraitV41[F];
        if(F==1&&I==10)UI.ImageContain(FrontlineCampaignV41::QuastPortrait(),CX,Y+3,90,90);
        else
        {
            const auto Binding=FrontlinePresentationV37::Commander(F==1,I);
            UTexture2D* Atlas=Binding.Atlas==2?Artwork().CommandersV37.Get():Artwork().CommandersV30[Binding.Atlas].Get();
            if(FrontlineTextureReady(Atlas)){const float IU=2.f/Atlas->GetSizeX(),IV=2.f/Atlas->GetSizeY();
                UI.Image(Atlas,CX,Y+3,90,90,FLinearColor(.92f,.93f,.91f,1),(Binding.Slot%Binding.Columns)/float(Binding.Columns)+IU,(Binding.Slot/Binding.Columns)/float(Binding.Rows)+IV,1.f/Binding.Columns-2*IU,1.f/Binding.Rows-2*IV,false,true);}
        }
        UI.Label(F?TEXT("德军指挥"):TEXT("协约国指挥"),CX+108,Y+2,407,24,18,FLinearColor(.04f,.05f,.04f));
        UI.Label(Bio.Name,CX+108,Y+31,407,25,17,FLinearColor(.04f,.05f,.04f));
        UI.Label(Bio.Role,CX+108,Y+61,407,23,14,FLinearColor(.10f,.11f,.09f));
        if(UI.Hovered(CX,Y,490,96))Hover=F;
    }
    if(Hover!=INDEX_NONE)
    {
        const auto Bio=CommanderBiographyV41(*M,Hover);const float TX=X+Hover*535,TY=Y-300;
        UI.Metal(TX,TY,475,280,true);UI.Label(Bio.Name,TX+20,TY+13,435,33,22,Paper);
        UI.Label(Bio.Role,TX+20,TY+52,435,27,15,Muted);UI.Paragraph(Bio.Text,TX+20,TY+95,435,166,17,Paper,true);
    }
}
struct FUnitStrengthV39
{
    int32 Remaining=0,Original=0;
    float Fraction=0;
};
FUnitStrengthV39 SquadStrengthV39(const FFrontlineSquad& Squad)
{
    // Recruitment appends each successfully spawned man once. Dead or expired
    // weak references stay in this roster, preserving the original complement.
    FUnitStrengthV39 Strength;
    Strength.Original=Squad.Members.Num();
    Strength.Remaining=Squad.LivingCount();
    Strength.Fraction=Strength.Original>0?float(Strength.Remaining)/Strength.Original:0.f;
    return Strength;
}
struct FStatusReadbackV39
{
    bool Visible=false,Tank=false;
    int32 Team=INDEX_NONE,SquadId=INDEX_NONE;
    float Current=0,Maximum=0,Fraction=0;
};
FStatusReadbackV39 V39LastStatus;
void DrawSquadStrengthV30(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,AWarfareFrontlineController* PC)
{
    V39LastStatus={};
    auto* Stars=FrontlineUIAssetsV8213::Texture(TEXT("/Game/Warfare/UI/V52/T_CommandStars_V52.T_CommandStars_V52"));
    if(Stars)for(const auto& Squad:B->Squads)if(Squad.Team==0)
    {
        const AWarfareFrontlineSoldier* AnchorUnit=nullptr;const AWarfareFrontlineSoldier* Receiving=nullptr;bool Gold=false;
        for(const auto& W:Squad.Members)if(const auto* U=W.Get();U&&U->IsActiveCombatant()&&!U->IsHidden())
        {
            if(!AnchorUnit)AnchorUnit=U;
            if(FrontlineGameplayV52::IsCommandLeader(U)){AnchorUnit=U;Gold=true;break;}
            if(!Receiving&&FrontlineGameplayV52::CommandLeader(U))Receiving=U;
        }
        if(!AnchorUnit||(!Gold&&!Receiving))continue;
        FVector2D At;if(!PC->ProjectWorldLocationToScreen(AnchorUnit->Feet()+FVector(0,0,230),At))continue;
        At=UI.Local(At);if(At.X<18||At.X>1902||At.Y<85||At.Y>806)continue;
        UI.Image(Stars,At.X-14,At.Y-14,28,28,FLinearColor::White,Gold?0.f:.5f,0,.5f,1,false,true);
        if(UI.Hovered(At.X-17,At.Y-17,34,34))
        {
            const auto* Commander=Gold?AnchorUnit:FrontlineGameplayV52::CommandLeader(Receiving);
            const bool Officer=Commander&&Commander->IsCommissionedOfficer();
            const float X=FMath::Clamp(float(At.X)-158.f,15.f,1589.f),Y=FMath::Clamp(float(At.Y)+22.f,100.f,678.f);
            UI.TooltipV83(TooltipKeyV83(16,AnchorUnit->GetUniqueID()),Officer?1:0,[&]{
            UI.PaperSheet(X,Y,316,126);UI.Hit(X,Y,316,126,TEXT("Blocked"),110);
            UI.Paragraph(Officer?TEXT("金星：指挥者；银星：接受命令。\n附近友军命中率提高50%，不叠加。军官阵亡、受重压或离开范围即失效。"):TEXT("金星：指挥者；银星：接受命令。\n附近友军命中率提高25%，不叠加。军士阵亡、受重压或离开范围即失效。"),X+13,Y+12,290,102,16,FLinearColor(.03f,.035f,.03f),true);
            },{X,Y});
        }
    }
    FVector2D Mouse;if(!UI.PointerPosition(Mouse)||Mouse.Y<75||Mouse.Y>830||PC->SupportTargetMode)return;
    const FFrontlineSquad* Selected=nullptr;AFrontlineTank* Tank=nullptr;FVector2D Anchor=FVector2D::ZeroVector;double Best=38.*38.;
    // Team zero is the player's army for either campaign and either map edge.
    for(const auto& S:B->Squads)if(S.Team==0)for(const auto& Ref:S.Members)
    {
        const auto* U=Ref.Get();if(!U||!U->IsActiveCombatant()||U->IsHidden())continue;
        FVector2D Point;if(!PC->ProjectWorldLocationToScreen(U->Feet()+FVector(0,0,88),Point))continue;
        Point=UI.Local(Point);
        FVector2D FeetScreen,HeadScreen;
        double D=FVector2D::DistSquared(Point,Mouse);
        if(PC->ProjectWorldLocationToScreen(U->Feet()+FVector(0,0,12),FeetScreen)
            &&PC->ProjectWorldLocationToScreen(U->Feet()+FVector(0,0,168),HeadScreen))
        {
            FeetScreen=UI.Local(FeetScreen);HeadScreen=UI.Local(HeadScreen);
            const double Top=FMath::Min(FeetScreen.Y,HeadScreen.Y)-8.,Bottom=FMath::Max(FeetScreen.Y,HeadScreen.Y)+8.;
            if(Mouse.Y>=Top&&Mouse.Y<=Bottom&&FMath::Abs(Mouse.X-Point.X)<=20.)
                D=FMath::Square(Mouse.X-Point.X)+FMath::Square(FMath::Clamp(Mouse.Y,Top,Bottom)-Mouse.Y);
        }
        if(D<Best){Best=D;Selected=&S;Anchor=Point;}
    }
    if(auto* V=UFrontlineVehicleSubsystem::Get(B->GetWorld()))for(AFrontlineTank* T:V->GetVehicles())
    {
        if(!IsValid(T)||T->Team!=0||T->IsHidden()||!T->IsOperational()||!T->HasEnteredField())continue;
        FVector2D Point;if(!PC->ProjectWorldLocationToScreen(T->AimPoint(),Point))continue;
        Point=UI.Local(Point);double D=FVector2D::DistSquared(Point,Mouse);
        // Use the projected collision hull, not a small aim-point circle. Fire,
        // smoke and decorations have no collision and cannot enlarge this box.
        const FBox Hull=T->GetComponentsBoundingBox(false);
        FVector2D Minimum(DBL_MAX,DBL_MAX),Maximum(-DBL_MAX,-DBL_MAX);int32 Projected=0;
        if(Hull.IsValid)for(int32 Corner=0;Corner<8;++Corner)
        {
            const FVector World(Corner&1?Hull.Max.X:Hull.Min.X,Corner&2?Hull.Max.Y:Hull.Min.Y,Corner&4?Hull.Max.Z:Hull.Min.Z);
            FVector2D Screen;if(!PC->ProjectWorldLocationToScreen(World,Screen))continue;
            Screen=UI.Local(Screen);Minimum.X=FMath::Min(Minimum.X,Screen.X);Minimum.Y=FMath::Min(Minimum.Y,Screen.Y);
            Maximum.X=FMath::Max(Maximum.X,Screen.X);Maximum.Y=FMath::Max(Maximum.Y,Screen.Y);++Projected;
        }
        if(Projected>=4&&Mouse.X>=Minimum.X-12.&&Mouse.X<=Maximum.X+12.&&Mouse.Y>=Minimum.Y-12.&&Mouse.Y<=Maximum.Y+12.)
        {
            const FVector2D Centre=(Minimum+Maximum)*.5,Extent=(Maximum-Minimum)*.5+FVector2D(12.,12.);
            const FVector2D Normalised((Mouse.X-Centre.X)/FMath::Max(1.,Extent.X),(Mouse.Y-Centre.Y)/FMath::Max(1.,Extent.Y));
            D=FMath::Min(36.*36.,Normalised.SizeSquared()*16.*16.);
        }
        if(D<Best){Best=D;Tank=T;Selected=nullptr;Anchor=Point;}
    }
    if(!Selected&&!Tank)return;
    const float X=FMath::Clamp(float(Anchor.X)+(Anchor.X>1490?-360.f:48.f),10.f,1590.f);float Y=FMath::Clamp(float(Anchor.Y)-105,78.f,700.f);

    uint32 Revision=0;
    if(Tank)
    {
        Revision=HashCombineFast(GetTypeHash(FMath::RoundToInt(Tank->GetHullIntegrity())),uint32(Tank->IsImmobilized()));
        for(int32 I=0;I<Tank->GetMachinegunCount();++I)Revision=HashCombineFast(Revision,Tank->GetMachinegunIntegrity(I)>0?1:0);
        for(int32 I=0;I<2;++I)Revision=HashCombineFast(Revision,Tank->GetTrackIntegrity(I)>0?1:0);
        Revision=HashCombineFast(Revision,uint32(WarfareFRRUArmourV82::Passengers(Tank)));
    }
    else
    {
        Revision=HashCombineFast(uint32(SquadStrengthV39(*Selected).Remaining),uint32(Selected->Trench+1));
        Revision=HashCombineFast(Revision,uint32(Selected->bAdvancing));
        for(const auto& W:Selected->Members)if(auto* U=W.Get();U&&U->IsActiveCombatant())
            Revision=HashCombineFast(Revision,uint32(U->MemberIndex+1)*8+uint32(U->IsMachineGunner())*4+uint32(U->bRecoveredMachineGun)*2+uint32(U->GunRecoveryReadyAt>B->Elapsed));
    }
    const auto& CurrentKills=Tank?Tank->KillsV65:Selected->KillsV65;
    Revision=HashCombineFast(Revision,HashCombineFast(uint32(CurrentKills.Infantry),uint32(CurrentKills.Armour)));
    // Readback remains scalar and current even when the drawing snapshot is
    // reused. Delay/fade presentation must not masquerade as a live hover.
    const uint64 MotionKey=TooltipKeyV83(Tank?18:17,Tank?Tank->GetUniqueID():uint32(Selected->Id));
    const auto* MotionState=UI.TooltipStateV83;
    if(UI.MotionStableV83||(MotionState&&MotionState->Pending==MotionKey&&MotionState->Active.Ready))
    {
        V39LastStatus.Visible=true;V39LastStatus.Team=0;V39LastStatus.Tank=Tank!=nullptr;
        if(Tank){V39LastStatus.Current=Tank->GetHullIntegrity();V39LastStatus.Maximum=Tank->GetMaximumHullIntegrity();}
        else{const auto Strength=SquadStrengthV39(*Selected);V39LastStatus.SquadId=Selected->Id;V39LastStatus.Current=Strength.Remaining;V39LastStatus.Maximum=Strength.Original;}
        V39LastStatus.Fraction=V39LastStatus.Maximum>0?V39LastStatus.Current/V39LastStatus.Maximum:0;
    }
    const FVector2D MotionAnchor(X,Y);
    UI.TooltipV83(TooltipKeyV83(Tank?18:17,Tank?Tank->GetUniqueID():uint32(Selected->Id)),Revision,[&]{
    const FLinearColor Color(.73f,.83f,.62f);
    V39LastStatus.Visible=true;V39LastStatus.Team=0;V39LastStatus.Tank=Tank!=nullptr;
    float Fraction=0;FString Title,Status;TArray<FString> Details;
    auto L=[](const TCHAR* E,const TCHAR* C){return FrontlineArmourV63::L(E,C);};
    if(Tank)
    {
        const auto* National=WarfareFRRUArmourV82::Definition(Tank);
        const bool Rail=Tank->IsRailConvoyV32();const float MaxHull=Tank->GetMaximumHullIntegrity();
        const bool Wheeled=(National&&National->Wheeled)||Tank->IsWheeledV8218();
        Fraction=FMath::Clamp(Tank->GetHullIntegrity()/FMath::Max(1.f,MaxHull),0.f,1.f);
        Title=National?National->Name:Rail?L(TEXT("Armoured train"),TEXT("装甲列车")):UFrontlineVehicleSubsystem::Definition(Tank->Type).Name;
        const bool DisabledDrive=Tank->IsImmobilized()||(Rail&&!National&&Tank->RailEngineIntegrityV837()<=0);
        const FString Mobility=DisabledDrive?((Rail||Wheeled)?L(TEXT("Drive disabled"),TEXT("行走机构损坏")):L(TEXT("Tracks disabled"),TEXT("履带损坏"))):L(TEXT("Mobile"),TEXT("可机动"));
        Status=FString::Printf(TEXT("%s %.0f / %.0f  ·  %s"),*L(TEXT("Armour"),TEXT("装甲")),Tank->GetHullIntegrity(),MaxHull,*Mobility);
        V39LastStatus.Current=Tank->GetHullIntegrity();V39LastStatus.Maximum=MaxHull;
        const int32 GunCount=Tank->GetMachinegunCount();int32 Lost=0;for(int32 I=0;I<GunCount;++I)Lost+=Tank->GetMachinegunIntegrity(I)<=0;
        if(GunCount)Details.Add(L(TEXT("Machine guns: "),TEXT("机枪："))+FString::Printf(TEXT("%d / %d"),GunCount-Lost,GunCount)+L(TEXT(" operational"),TEXT(" 可用")));
        if(Lost>0)Details.Add(L(TEXT("Disabled gun positions cannot fire."),TEXT("损坏枪位无法开火。")));
        if(Rail&&!National)
        {
            const int32 Cars=int32(Tank->RailCannonOperationalV837(0))+int32(Tank->RailCannonOperationalV837(1));
            Details.Add(L(TEXT("Gun cars: "),TEXT("炮车："))+FString::Printf(TEXT("%d / 2"),Cars));
            if(Tank->RailEngineIntegrityV837()<=0)Details.Add(L(TEXT("Locomotive disabled"),TEXT("机车已损坏")));
        }
        if(Tank->GetTrackIntegrity(0)<=0)Details.Add(Wheeled?L(TEXT("Left drive disabled"),TEXT("左侧行走机构损坏")):L(TEXT("Left track disabled"),TEXT("左履带损坏")));
        if(Tank->GetTrackIntegrity(1)<=0)Details.Add(Wheeled?L(TEXT("Right drive disabled"),TEXT("右侧行走机构损坏")):L(TEXT("Right track disabled"),TEXT("右履带损坏")));
        if(National&&National->Rail)Details.Add(L(TEXT("Troops aboard: "),TEXT("车内士兵："))+FString::FromInt(WarfareFRRUArmourV82::Passengers(Tank)));
    }
    else
    {
        const auto Strength=SquadStrengthV39(*Selected);
        Fraction=Strength.Fraction;Title=B->GetRecruitName(Selected->Role,Selected->Team);
        Status=FString::Printf(TEXT("%d / %d 人"),Strength.Remaining,Strength.Original);
        V39LastStatus.SquadId=Selected->Id;V39LastStatus.Current=Strength.Remaining;V39LastStatus.Maximum=Strength.Original;
        bool Primary=false,Gunner=false,Recovered=false,Waiting=false;int32 Ammo=0;
        for(const auto& W:Selected->Members)if(auto* U=W.Get();U&&U->IsActiveCombatant())
        {Primary|=U->MemberIndex==0;Gunner|=U->IsMachineGunner();Recovered|=U->bRecoveredMachineGun;Waiting|=U->GunRecoveryReadyAt>B->Elapsed;Ammo+=U->RoundsRemaining;}
        if(Selected->Role==EFrontlineRole::Lewis||Selected->Role==EFrontlineRole::MobileGun)
          Details.Add(Gunner?(Recovered?L(TEXT("An assistant has taken over the gun."),TEXT("副射手已接替机枪。")):L(TEXT("Machine-gun crew operational"),TEXT("机枪射手在位"))):Waiting?L(TEXT("Gunner lost · the crew is recovering the gun"),TEXT("主射手阵亡 · 同伴正在接枪")):L(TEXT("Machine-gun operator lost"),TEXT("机枪射手阵亡")));
        else if(Selected->Role==EFrontlineRole::RifleGrenadier||Selected->Role==EFrontlineRole::AntiTankRifle||Selected->Role==EFrontlineRole::Flamethrower)
          Details.Add(Primary?L(TEXT("Specialist weapon operational"),TEXT("专职射手在位")):L(TEXT("Specialist killed · rifles remain"),TEXT("专职射手阵亡 · 仅剩步枪手")));
        else if(Selected->Role==EFrontlineRole::Officer)Details.Add(Primary?L(TEXT("Command section present"),TEXT("指挥者在位")):L(TEXT("Section leader killed"),TEXT("指挥者阵亡")));
        else if(const FString Shooter=FrontlineStatusTextV8217::Shooter(B,*Selected,Primary);!Shooter.IsEmpty())Details.Add(Shooter);
        Details.Add(Selected->Trench>=0?L(TEXT("In trench cover"),TEXT("壕内掩蔽")):Selected->bAdvancing?L(TEXT("Advancing"),TEXT("向前推进")):L(TEXT("Holding position"),TEXT("停步守备")));
    }
    const auto& Kills=Tank?Tank->KillsV65:Selected->KillsV65;
    Details.Add(L(TEXT("Infantry killed: "),TEXT("击毙步兵："))+FString::FromInt(Kills.Infantry));
    Details.Add(L(TEXT("Armour destroyed: "),TEXT("摧毁装甲："))+FString::FromInt(Kills.Armour));
    V39LastStatus.Fraction=Fraction;
    const float H=108+Details.Num()*29;Y=FMath::Min(Y,830.f-H);UI.Metal(X,Y,320,H,true);UI.Hit(X,Y,320,H,TEXT("Blocked"),110);
    UI.Label(Title,X+13,Y+9,294,30,18,Paper);UI.Label(Status,X+13,Y+43,294,25,16,Muted);
    for(int32 I=0;I<Details.Num();++I)UI.Label(Details[I],X+13,Y+75+I*29,294,25,15,Paper);
    UI.Skin(2,X+13,Y+H-22,294,12,3);if(Fraction>.005f)UI.Skin(7,X+15,Y+H-20,290*FMath::Clamp(Fraction,0.f,1.f),8,2,Color);
    },MotionAnchor,830);
}
