// Private sidecar codec. Older game packages only rewrite achievements-v2.json;
// this ledger therefore remains independent of their account schema.
FString ArmourTallyPathV8220()
{const FString Main=AccountPath();return Main.IsEmpty()?FString():FPaths::Combine(FPaths::GetPath(Main),TEXT("armour-kill-tally-v8220.json"));}
bool DecodeArmourTallyV8220(const FString& Text,FArmourTallyLedgerV8220& Out)
{
    TSharedPtr<FJsonObject> J;double Schema=0;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J.IsValid()
        ||!J->TryGetNumberField(TEXT("schema"),Schema)||(Schema!=1&&Schema!=2&&Schema!=3))return false;
    const TSharedPtr<FJsonObject>* Receipts=nullptr;
    if(!J->TryGetObjectField(TEXT("receipts"),Receipts)||!Receipts||!Receipts->IsValid()
        ||(*Receipts)->Values.Num()>MaximumArmourKillReceiptsV8220)return false;
    Out=FArmourTallyLedgerV8220();static const TCHAR* Keys[]={TEXT("uk"),TEXT("de"),TEXT("fr"),TEXT("ru")};
    if(Schema>=2)
    {
        double Generation=0;if(!J->TryGetNumberField(TEXT("reset_generation_v83"),Generation)||!FMath::IsFinite(Generation)
            ||Generation<0||Generation>4294967295.||Generation!=FMath::FloorToDouble(Generation))return false;
        Out.ResetGenerationV83=uint32(Generation);
    }
    for(int32 N=0;N<(Schema>=3?4:2);++N)
    {
        double Total=0;if(!J->TryGetNumberField(Keys[N],Total)||!FMath::IsFinite(Total)||Total<0
            ||Total>MaximumArmourKillReceiptsV8220||Total!=FMath::FloorToDouble(Total))return false;
        Out.Totals[N]=int32(Total);
    }
    int32 Seen[4]={};
    for(const auto& Entry:(*Receipts)->Values)
    {
        FGuid Id;double Nation=0;
        if(!FGuid::Parse(FString(Entry.Key.ToView()),Id)||!Id.IsValid()||Out.Receipts.Contains(Id)
            ||!Entry.Value.IsValid()||!Entry.Value->TryGetNumber(Nation)||(!FMath::IsFinite(Nation)||Nation<0||Nation>(Schema>=3?3:1)||FMath::FloorToDouble(Nation)!=Nation))return false;
        Out.Receipts.Add(Id,uint8(Nation));++Seen[int32(Nation)];
    }
    for(int32 N=0;N<4;++N)if(Seen[N]!=Out.Totals[N])return false;return true;
}
bool ReadArmourTallyFileV8220(const FString& Path,FArmourTallyLedgerV8220& Out)
{
    const int64 Size=IFileManager::Get().FileSize(*Path);FString Text;
    return Size>0&&Size<=2097152&&FFileHelper::LoadFileToString(Text,*Path)&&DecodeArmourTallyV8220(Text,Out);
}
bool ReadArmourTallyV8220(FArmourTallyLedgerV8220& Out)
{
    const FString Path=ArmourTallyPathV8220();Out=FArmourTallyLedgerV8220();if(Path.IsEmpty())return false;
    // A supported backup does not authorize overwriting a future ledger.
    for(const FString& Candidate:{Path,Path+TEXT(".bak")})
    {
        FString Text;TSharedPtr<FJsonObject> J;double Schema=0;
        if(IFileManager::Get().FileSize(*Candidate)<=2097152&&FFileHelper::LoadFileToString(Text,*Candidate)
            &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)&&J.IsValid()
            &&J->TryGetNumberField(TEXT("schema"),Schema)&&Schema!=1&&Schema!=2&&Schema!=3)return false;
    }
    auto CurrentGeneration=[&]()
    {
        // The account reset is the logical commit point. A crash before its
        // independent tally rewrite must not reveal the old deeds on relaunch.
        if(Out.ResetGenerationV83<Account.ResetGenerationV83){Out=FArmourTallyLedgerV8220();Out.ResetGenerationV83=Account.ResetGenerationV83;}
        return Out.ResetGenerationV83==Account.ResetGenerationV83;
    };
    if(ReadArmourTallyFileV8220(Path,Out))return CurrentGeneration();
    if(ReadArmourTallyFileV8220(Path+TEXT(".bak"),Out)){Out.Recovered=true;return CurrentGeneration();}
    Out.ResetGenerationV83=Account.ResetGenerationV83;
    return !IFileManager::Get().FileExists(*Path)&&!IFileManager::Get().FileExists(*(Path+TEXT(".bak")));
}
void LoadArmourTallyV8220()
{
    Load();if(ArmourTallyLedgerV8220.Loaded)return;
    FArmourTallyLedgerV8220 Read;const bool Valid=ReadArmourTallyV8220(Read);
    Read.Loaded=true;Read.Invalid=!Valid;ArmourTallyLedgerV8220=MoveTemp(Read);
}
bool SaveArmourTallyLockedV8220(bool& PrimaryCommitted)
{
    PrimaryCommitted=false;
    if(ReviewFailSaveV81&&IsolatedAwardAccountV8217()&&IsolatedAwardBattle.IsValid())return false;
    if(ArmourTallyLedgerV8220.Invalid)return false;
    const FString Path=ArmourTallyPathV8220();if(Path.IsEmpty())return false;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    auto J=MakeShared<FJsonObject>(),Receipts=MakeShared<FJsonObject>();J->SetNumberField(TEXT("schema"),3);J->SetNumberField(TEXT("reset_generation_v83"),ArmourTallyLedgerV8220.ResetGenerationV83);
    J->SetNumberField(TEXT("uk"),ArmourTallyLedgerV8220.Totals[0]);J->SetNumberField(TEXT("de"),ArmourTallyLedgerV8220.Totals[1]);J->SetNumberField(TEXT("fr"),ArmourTallyLedgerV8220.Totals[2]);J->SetNumberField(TEXT("ru"),ArmourTallyLedgerV8220.Totals[3]);
    TArray<FGuid> Ids;ArmourTallyLedgerV8220.Receipts.GetKeys(Ids);
    Ids.Sort([](const FGuid& A,const FGuid& B){return A.ToString(EGuidFormats::Digits)<B.ToString(EGuidFormats::Digits);});
    for(const auto& Id:Ids)Receipts->SetNumberField(Id.ToString(EGuidFormats::Digits),ArmourTallyLedgerV8220.Receipts[Id]);J->SetObjectField(TEXT("receipts"),Receipts);
    FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
    const FString Temp=Path+TEXT(".tmp-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if(!FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return false;
    FArmourTallyLedgerV8220 Verified;if(!ReadArmourTallyFileV8220(Temp,Verified)){IFileManager::Get().Delete(*Temp);return false;}
    if(IFileManager::Get().FileExists(*Path)&&!ArmourTallyLedgerV8220.Recovered)
    {
        const FString BackupTemp=Path+TEXT(".bak.tmp");
        if(IFileManager::Get().Copy(*BackupTemp,*Path,true,true)!=COPY_OK||!Replace(Path+TEXT(".bak"),BackupTemp))
        {IFileManager::Get().Delete(*Temp);IFileManager::Get().Delete(*BackupTemp);return false;}
    }
    if(!Replace(Path,Temp)){IFileManager::Get().Delete(*Temp);return false;}PrimaryCommitted=true;
    if((ReviewFailMirrorV81||ReviewFailTallyMirrorV83)&&IsolatedAwardAccountV8217()&&IsolatedAwardBattle.IsValid())return false;
    const FString Mirror=Path+TEXT(".bak.next");
    if(IFileManager::Get().Copy(*Mirror,*Path,true,true)!=COPY_OK||!Replace(Path+TEXT(".bak"),Mirror))
    {IFileManager::Get().Delete(*Mirror);return false;}
    ArmourTallyLedgerV8220.Recovered=false;return true;
}
void PersistArmourTallyV8220()
{
    if(PendingArmourTallyV8220.IsEmpty())return;
    Load();ArmourTallyRetryV8220=FPlatformTime::Seconds()+5.;FAccountLock Lock;
    if(!Lock.Valid()||Account.Corrupt||Account.Unsupported||!MergeDisk())return;
    FArmourTallyLedgerV8220 Disk;if(!ReadArmourTallyV8220(Disk)){ArmourTallyLedgerV8220.Invalid=true;return;}
    Disk.Loaded=true;ArmourTallyLedgerV8220=MoveTemp(Disk);
    for(auto It=PendingArmourTallyV8220.CreateIterator();It;++It)
        if(Account.Tainted.Contains(It.Value().Campaign)||RetryTaintedV81.Contains(It.Value().Campaign))It.RemoveCurrent();
    if(PendingArmourTallyV8220.IsEmpty())return;
    const auto Before=ArmourTallyLedgerV8220;TMap<FGuid,FrontlineArmourKillTallyV8220::FNotification> NewNotices;
    for(auto It=PendingArmourTallyV8220.CreateIterator();It;++It)
    {
        const int32 Nation=int32(It.Value().Notice.Nation);
        if(ArmourTallyLedgerV8220.Receipts.Contains(It.Key()))continue;
        // Capacity is reserved at admission. A second instance can consume the
        // remaining space meanwhile; discard only that excess record, never
        // roll back other valid records into an endless retry batch.
        if(Nation<0||Nation>3||It.Value().Notice.Receipt!=It.Key()||!It.Value().Notice.Encounter.IsValid()
            ||ArmourTallyLedgerV8220.Receipts.Num()>=MaximumArmourKillReceiptsV8220){It.RemoveCurrent();continue;}
        auto Notice=It.Value().Notice;Notice.Total=++ArmourTallyLedgerV8220.Totals[Nation];
        ArmourTallyLedgerV8220.Receipts.Add(It.Key(),uint8(Nation));NewNotices.Add(It.Key(),Notice);
    }
    bool PrimaryCommitted=false;
    if(SaveArmourTallyLockedV8220(PrimaryCommitted))
    {
        TArray<FrontlineArmourKillTallyV8220::FNotification> Published;
        for(const auto& Entry:PendingArmourTallyV8220)
            if(const auto* Notice=NewNotices.Find(Entry.Key))Published.Add(*Notice);
            else if(Entry.Value.PrimaryCommitted)Published.Add(Entry.Value.Notice);
        Published.Sort([](const auto& A,const auto& B){return A.Nation==B.Nation?A.Total<B.Total:uint8(A.Nation)<uint8(B.Nation);});
        ArmourKillNotificationsV8220.Append(Published);PendingArmourTallyV8220.Empty();
    }
    else
    {
        if(PrimaryCommitted)for(auto& Entry:PendingArmourTallyV8220)if(const auto* Notice=NewNotices.Find(Entry.Key))
            {Entry.Value.PrimaryCommitted=true;Entry.Value.Notice=*Notice;}
        ArmourTallyLedgerV8220=Before;
    }
}

void RunNationalArmourTallyCodecChecksV87(TFunctionRef<void(const FString&,bool)> Check)
{
 const FString A=FGuid::NewGuid().ToString(EGuidFormats::Digits),B=FGuid::NewGuid().ToString(EGuidFormats::Digits);
 FArmourTallyLedgerV8220 Read;
 const FString Old=TEXT("{\"schema\":2,\"reset_generation_v83\":0,\"uk\":1,\"de\":0,\"receipts\":{\"")+A+TEXT("\":0}}");
 Check(TEXT("v87_two_nation_tally_keeps_old_deeds_without_inventing_national_deeds"),DecodeArmourTallyV8220(Old,Read)&&Read.Totals[0]==1&&Read.Totals[2]==0&&Read.Totals[3]==0);
 const FString Four=TEXT("{\"schema\":3,\"reset_generation_v83\":0,\"uk\":0,\"de\":0,\"fr\":1,\"ru\":1,\"receipts\":{\"")+A+TEXT("\":2,\"")+B+TEXT("\":3}}");
 Check(TEXT("v87_four_nation_tally_preserves_separate_receipt_counts"),DecodeArmourTallyV8220(Four,Read)&&Read.Totals[0]==0&&Read.Totals[1]==0&&Read.Totals[2]==1&&Read.Totals[3]==1);
 FString Wrong=Four;Wrong.ReplaceInline(TEXT("\"fr\":1"),TEXT("\"fr\":2"));
 Check(TEXT("v87_national_tally_rejects_counter_without_receipt"),!DecodeArmourTallyV8220(Wrong,Read));
 Wrong=Four;Wrong.ReplaceInline(TEXT("\":3}"),TEXT("\":4}"));
 Check(TEXT("v87_national_tally_rejects_nonplayable_nation"),!DecodeArmourTallyV8220(Wrong,Read));
}
