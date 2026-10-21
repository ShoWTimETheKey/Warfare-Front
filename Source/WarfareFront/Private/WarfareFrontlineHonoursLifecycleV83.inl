// Included inside FrontlineAchievementsV76. These administrative operations
// share the existing account mutex and never alter campaign gameplay values.
namespace
{
bool ReviewFailTallyResetV83=false;
struct FHonoursFileSnapshotV83
{
    struct FFile{FString Path;TArray<uint8> Bytes;bool Existed=false;};
    TArray<FFile> Files;
    bool Capture(bool IncludeTally)
    {
        const FString Main=AccountPath();if(Main.IsEmpty())return false;
        TArray<FString> Paths={Main,Main+TEXT(".bak")};
        if(IncludeTally){const FString Tally=ArmourTallyPathV8220();if(Tally.IsEmpty())return false;Paths.Add(Tally);Paths.Add(Tally+TEXT(".bak"));}
        for(const auto& Path:Paths)
        {
            FFile File;File.Path=Path;File.Existed=IFileManager::Get().FileExists(*Path);
            if(File.Existed&&(IFileManager::Get().FileSize(*Path)>2097152||!FFileHelper::LoadFileToArray(File.Bytes,*Path)))return false;
            Files.Add(MoveTemp(File));
        }
        return true;
    }
    bool Restore()const
    {
        // The account primary is the reset commit point. Undo dependent tally
        // files and all mirrors before restoring that primary last.
        for(int32 I=Files.Num()-1;I>=0;--I)
        {
            const auto& File=Files[I];
            if(!File.Existed){if(IFileManager::Get().FileExists(*File.Path)&&!IFileManager::Get().Delete(*File.Path))return false;continue;}
            const FString Temp=File.Path+TEXT(".rollback-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
            const bool Written=FFileHelper::SaveArrayToFile(File.Bytes,*Temp);
            const bool Restored=Written&&Replace(File.Path,Temp);IFileManager::Get().Delete(*Temp);
            if(!Restored)return false;
        }
        return true;
    }
    bool Matches()const
    {
        for(const auto& File:Files)
        {
            if(IFileManager::Get().FileExists(*File.Path)!=File.Existed)return false;
            if(File.Existed){TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File.Path)||Bytes!=File.Bytes)return false;}
        }
        return true;
    }
};
bool WritableHonoursV83(AWarfareFrontlineGameMode* B)
{
    return B&&!B->IsProgressionReadOnly()&&(!B->IsProgressionVolatile()||IsolatedAwardsAllowed(B)
        ||(FrontlineUltimateArchiveV8215::Active(B)&&!FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))));
}
void DropAssistedPendingV83()
{
    for(auto It=SettlementsV81.CreateIterator();It;++It)
        if(Account.Tainted.Contains(It.Value().CampaignV8215.IsValid()?It.Value().CampaignV8215:It.Key()))It.RemoveCurrent();
    for(auto It=PendingArmourTallyV8220.CreateIterator();It;++It)
        if(Account.Tainted.Contains(It.Value().Campaign)||RetryTaintedV81.Contains(It.Value().Campaign))It.RemoveCurrent();
}
void LifecycleResponseV83(FString& Response,const TCHAR* EN,const TCHAR* ZH)
{Response=FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese?ZH:EN;}
bool RestoreHonoursV83(const FHonoursFileSnapshotV83& Files,const FAccount& Before,FString& Response)
{
    const bool Restored=Files.Restore();Account=Before;
    if(!Restored)
    {
        Account.Corrupt=true;Account.Notice=TEXT("Honour records could not be restored. Further writes have been stopped.");
        LifecycleResponseV83(Response,TEXT("Record rollback failed. Further honour writes are blocked."),TEXT("记录恢复失败，已停止继续写入战功。"));
    }
    return Restored;
}
}
bool CommitArchiveDeletionV83(AWarfareFrontlineGameMode* B,TFunctionRef<bool()> CommitDeletion,FString& Response)
{
    LifecycleResponseV83(Response,TEXT("Archive deletion could not be saved. Existing records have been retained."),TEXT("战役档案删除未能保存，原记录已保留。"));
    if(!B||B->IsProgressionReadOnly())return false;
    // Unwritable previews may exercise their in-memory archive controls, but
    // cannot revoke anything in the real global account.
    if(B->IsProgressionVolatile()&&!IsolatedAwardsAllowed(B))return CommitDeletion();
    Load();FAccountLock Lock;
    if(Account.Corrupt||Account.Unsupported||!Lock.Valid()||!MergeDisk())
    {LifecycleResponseV83(Response,TEXT("Honour records are unavailable. No archives were deleted."),TEXT("战功记录暂不可用，未删除战役档案。"));return false;}
    if(Account.CheatHonoursV8218==0&&Account.CheatMedalsV8218.IsEmpty()&&Account.CheatMedalAwardsV8222.IsEmpty())
    {const bool Deleted=CommitDeletion();if(Deleted)DropAssistedPendingV83();return Deleted;}
    FHonoursFileSnapshotV83 Files;if(!Files.Capture(false))return false;
    const FAccount Before=Account;
    Account.CheatHonoursV8218=0;Account.CheatMedalsV8218.Empty();Account.CheatMedalAwardsV8222.Empty();
    bool PrimaryCommitted=false;
    if(!SaveLocked(&PrimaryCommitted))
    {
        LifecycleResponseV83(Response,TEXT("Cheat grants could not be revoked. No archives were deleted."),TEXT("作弊战功未能撤销，未删除战役档案。"));
        RestoreHonoursV83(Files,Before,Response);return false;
    }
    if(!CommitDeletion())
    {
        LifecycleResponseV83(Response,TEXT("Archive deletion failed. Honour records have been retained."),TEXT("战役档案删除失败，战功记录已保留。"));
        RestoreHonoursV83(Files,Before,Response);return false;
    }
    // Only explicit cheat overlays are removed. Genuine classes, dates, route
    // proofs, accumulated kills and pre-assistance deeds remain untouched.
    DropAssistedPendingV83();return true;
}
bool ResetAllHonoursV83(AWarfareFrontlineGameMode* B,FString& Response)
{
    LifecycleResponseV83(Response,TEXT("Honour reset was not saved. Existing records have been retained."),TEXT("战功重置未保存，原记录已保留。"));
    if(!WritableHonoursV83(B))
    {LifecycleResponseV83(Response,TEXT("Honour records are read-only; nothing was reset."),TEXT("战功记录只读，未重置。"));return false;}
    Load();FAccountLock Lock;
    if(Account.Corrupt||Account.Unsupported||!Lock.Valid()||!MergeDisk())
    {LifecycleResponseV83(Response,TEXT("Honour records are unavailable; nothing was reset."),TEXT("战功记录暂不可用，未重置。"));return false;}
    FArmourTallyLedgerV8220 DiskTally;
    if(!ReadArmourTallyV8220(DiskTally)||Account.ResetGenerationV83==MAX_uint32)return false;
    DiskTally.Loaded=true;ArmourTallyLedgerV8220=MoveTemp(DiskTally);
    FHonoursFileSnapshotV83 Files;if(!Files.Capture(true))return false;
    const FAccount Before=Account;const auto BeforeTally=ArmourTallyLedgerV8220;
    Account=FAccount();Account.Loaded=true;Account.Recovered=Before.Recovered;Account.ResetGenerationV83=Before.ResetGenerationV83+1;
    Account.Tainted=Before.Tainted;for(const auto& Id:RetryTaintedV81)Account.Tainted.Add(Id);
    ArmourTallyLedgerV8220=FArmourTallyLedgerV8220();ArmourTallyLedgerV8220.Loaded=true;ArmourTallyLedgerV8220.Recovered=BeforeTally.Recovered;
    ArmourTallyLedgerV8220.ResetGenerationV83=Account.ResetGenerationV83;
    bool AccountPrimary=false,TallyPrimary=false;
    const bool FaultTally=ReviewFailTallyResetV83&&IsolatedAwardAccountV8217()&&IsolatedAwardBattle.Get()==B;
    if(!SaveLocked(&AccountPrimary)||FaultTally||!SaveArmourTallyLockedV8220(TallyPrimary))
    {
        LifecycleResponseV83(Response,TEXT("Honour reset was not saved. Existing records have been retained."),TEXT("战功重置未保存，原记录已保留。"));
        RestoreHonoursV83(Files,Before,Response);ArmourTallyLedgerV8220=BeforeTally;return false;
    }
    // Cancel pre-reset frozen proofs and banners, including this battle's
    // outro deltas. Existing assisted campaigns keep their ineligibility.
    InvalidatePreResetHonoursV83();RetryTaintedV81.Empty();
    LifecycleResponseV83(Response,TEXT("All achievements, decorations and accumulated honour progress have been reset."),TEXT("全部成就、勋章与累计战功进度已重置。"));return true;
}
