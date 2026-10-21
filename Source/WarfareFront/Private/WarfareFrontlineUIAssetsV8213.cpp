#include "WarfareFrontlineUIAssetsV8213.h"
#include "WarfareFrontlineArmourValourV8221.h"
#include "WarfareFrontlineAchievementsV76.h"
#include "WarfareFrontlineMedalsV8215.h"
#include "WarfareFrontlineCampaignV41.h"
#include "WarfareFrontlineCampaignV51.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProfilingDebugging/ResourceSize.h"
#include "RenderCommandFence.h"
#include "TextureResource.h"
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"

namespace FrontlineUIAssetsV8213
{
namespace
{
struct FResourceCheck
{
    TSharedPtr<FRenderCommandFence> Fence;
    const FTextureResource* Resource=nullptr;
};
struct FCache
{
    TMap<FString,TStrongObjectPtr<UTexture2D>> Textures;
    TMap<FString,TWeakObjectPtr<UTexture2D>> BaselineTextures;
    TSet<FString> Pending,Failed;
    TMap<FString,FString> FailureReasons;
    TMap<FString,FResourceCheck> ResourceChecks;
    TArray<TSharedPtr<FStreamableHandle>> Handles;
    bool CoreRequested=false;
    uint64 AsyncBatches=0,AlreadyLoaded=0,CacheHits=0,SyncRequests=0;
    uint64 TerminalResourceFailures=0;
    double RequestedAt=0,CompletedAt=0,SyncLoadMs=0;
};
FCache& Cache(){static FCache Value;return Value;}
FString ObjectPath(const FString& Path)
{
    if(Path.IsEmpty())return {};
    int32 Slash=INDEX_NONE,Dot=INDEX_NONE;
    Path.FindLastChar(TEXT('/'),Slash);Path.FindLastChar(TEXT('.'),Dot);
    return Dot>Slash?Path:Path+TEXT(".")+Path.Mid(Slash+1);
}
bool ResourceReady(const UTexture2D* Texture)
{
    if(!IsValid(Texture)||Texture->GetSizeX()<=0||Texture->GetSizeY()<=0)return false;
#if WITH_EDITOR
    if(Texture->IsDefaultTexture())return false;
#endif
    const auto* Resource=Texture->GetResource();
    return Resource&&Resource->IsInitialized()&&Resource->TextureRHI.IsValid();
}
bool MissingFailureReview()
{
    static const bool Enabled=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV67StartupReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFUIAssetFailureV8213"));
    return Enabled;
}
bool ResourceFailureReview()
{
    static const bool Enabled=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFV67StartupReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFUIRHIAssetFailureV8213"));
    return Enabled;
}
bool SyntheticResourceFailure(const FString& Path)
{
    return ResourceFailureReview()&&Path==TEXT("/Game/Warfare/UI/V52/T_CommandStars_V52.T_CommandStars_V52");
}
void RecordFailure(const FString& Path,const TCHAR* Reason=TEXT("missing_object"))
{
    auto& C=Cache();
    if(C.Failed.Contains(Path))return;
    C.Failed.Add(Path);
    C.FailureReasons.Add(Path,Reason);
    UE_LOG(LogTemp,Warning,TEXT("WF_UI_ASSET_V8213_MISSING %s reason=%s"),*Path,Reason);
}
void QueueResourceChecks(const TArray<FString>& Keys)
{
    if(Keys.IsEmpty())return;
    auto& C=Cache();auto Fence=MakeShared<FRenderCommandFence>();
    // UTexture::PostLoad has already queued UpdateResource/BeginInitResource.
    // One batch fence covers each object's preceding initialization, including
    // render command pipes and RHI submission, without waiting on the CPU.
    Fence->BeginFence(FRenderCommandFence::ESyncDepth::RHIThread);
    for(const auto& Key:Keys)if(const auto* Texture=C.Textures.Find(Key))
        C.ResourceChecks.Add(Key,FResourceCheck{Fence,Texture->Get()->GetResource()});
}
}

bool BaselineMode()
{
    // The comparison route cannot be enabled for an ordinary player profile.
    static const bool Enabled=FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfV8213"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFUIPerfBaselineV8213"));
    return Enabled;
}
void PreloadTextures(const TArray<FString>& Paths)
{
    check(IsInGameThread());if(BaselineMode())return;
    auto& C=Cache();TArray<FSoftObjectPath> Targets;TArray<FString> ExistingKeys;
    for(const auto& Path:Paths)
    {
        const FString Key=ObjectPath(Path);
        if(Key.IsEmpty()||C.Textures.Contains(Key)||C.Pending.Contains(Key)||C.Failed.Contains(Key))continue;
        const FSoftObjectPath Target(Key);
        if(auto* Texture=Cast<UTexture2D>(Target.ResolveObject());Texture&&!Texture->HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad|RF_NeedPostLoadSubobjects))
        {C.Textures.Add(Key,TStrongObjectPtr<UTexture2D>(Texture));ExistingKeys.Add(Key);++C.AlreadyLoaded;continue;}
        C.Pending.Add(Key);Targets.Add(Target);
    }
    QueueResourceChecks(ExistingKeys);
    if(Targets.IsEmpty())return;
    if(C.RequestedAt<=0)C.RequestedAt=FPlatformTime::Seconds();
    ++C.AsyncBatches;
    auto Handle=UAssetManager::GetStreamableManager().RequestAsyncLoad(Targets,
        FStreamableDelegateWithHandle::CreateLambda([Targets](TSharedPtr<FStreamableHandle> Completed)
        {
            check(IsInGameThread());auto& Loaded=Cache();TArray<FString> LoadedKeys;
            for(const auto& Target:Targets)
            {
                const FString Key=Target.ToString();
                if(auto* Texture=Cast<UTexture2D>(Target.ResolveObject()))
                {Loaded.Textures.Add(Key,TStrongObjectPtr<UTexture2D>(Texture));LoadedKeys.Add(Key);}
                else RecordFailure(Key);
                Loaded.Pending.Remove(Key);
            }
            QueueResourceChecks(LoadedKeys);
            // Pin every successful UObject before releasing the handle's GC
            // ownership. Pending is never confused with a failed load.
            if(Completed.IsValid())
            {Completed->ReleaseHandle();Loaded.Handles.Remove(Completed);}
            if(Loaded.Pending.IsEmpty())Loaded.CompletedAt=FPlatformTime::Seconds();
        }),FStreamableManager::DefaultAsyncLoadPriority,false,false,TEXT("WarfareFrontCurrentUIV8213"));
    if(Handle.IsValid())C.Handles.Add(Handle);
    else for(const auto& Target:Targets)
    {const FString Key=Target.ToString();C.Pending.Remove(Key);RecordFailure(Key);}
}
void PreloadCore()
{
    check(IsInGameThread());auto& C=Cache();if(C.CoreRequested)return;C.CoreRequested=true;
    if(BaselineMode())return;
    TArray<FString> Paths;
    for(int32 I=0;I<FrontlineAchievementsV76::Count;++I)Paths.Add(FrontlineAchievementsV76::BadgeObjectPathV82(I));
    // Warm the current honours with the rest of the menu. First inspection must
    // not become the point at which a full-resolution award starts loading.
    for(int32 I=37;I<FrontlineMedalsV8215::Count;++I)
    {
        const auto Variants=FrontlineMedalsV8215::Variants(I);
        for(int32 Grade=1;Grade<=Variants.Num();++Grade)
            Paths.Add(FrontlineMedalsV8215::VariantArtworkPath(I,Grade));
        if(FrontlineMedalsV8215::Definition(I).Ribbon)
        {
            const FString Stem=FString::Printf(TEXT("T_Ribbon_%d_V84414"),I);
            Paths.Add(TEXT("/Game/Warfare/UI/V84414/Honours/")+Stem+TEXT(".")+Stem);
        }
    }
    FrontlineCampaignV41::AppendUIPreloadPathsV8213(Paths);
    FrontlineCampaignV51::AppendUIPreloadPathsV8213(Paths);
    // These remain visible in the current UK/DE roster, support and map routes.
    // No directory scan, retired artwork, or national campaign image is loaded.
    for(const TCHAR* Path:{
        TEXT("/Game/Warfare/UI/V62/T_RiquevalSurvey.T_RiquevalSurvey"),
        TEXT("/Game/Warfare/UI/V51/T_AttachmentPortraits_V51.T_AttachmentPortraits_V51"),
        TEXT("/Game/Warfare/UI/V64/T_Shrapnel_V64.T_Shrapnel_V64"),
        TEXT("/Game/Warfare/UI/V63/T_ArmourTech_British.T_ArmourTech_British"),
        TEXT("/Game/Warfare/UI/V63/T_ArmourTech_German.T_ArmourTech_German"),
        TEXT("/Game/Warfare/UI/V8218/T_ArmourTechUK_V8218.T_ArmourTechUK_V8218"),
        TEXT("/Game/Warfare/UI/V8218/T_ArmourTechDE_V8218.T_ArmourTechDE_V8218"),
        TEXT("/Game/Warfare/UI/V8218/T_ArmourTechUKArchive_V8218.T_ArmourTechUKArchive_V8218"),
        TEXT("/Game/Warfare/UI/V8218/T_ArmourTechDEArchive_V8218.T_ArmourTechDEArchive_V8218"),
        TEXT("/Game/Warfare/UI/V8218/T_Armour_MarkVStar_V8218.T_Armour_MarkVStar_V8218"),
        TEXT("/Game/Warfare/UI/V8218/T_Armour_Austin1918_V8218.T_Armour_Austin1918_V8218"),
        TEXT("/Game/Warfare/Vehicles/V28/T_Portrait_MarkIVMale.T_Portrait_MarkIVMale"),
        TEXT("/Game/Warfare/Vehicles/V28/T_Portrait_Whippet.T_Portrait_Whippet"),
        TEXT("/Game/Warfare/Vehicles/V28/T_Portrait_A7V.T_Portrait_A7V"),
        TEXT("/Game/Warfare/Vehicles/V28/T_Portrait_BeuteMarkIV.T_Portrait_BeuteMarkIV"),
        TEXT("/Game/Warfare/Vehicles/Production/Female/T_Portrait_MarkIVFemale.T_Portrait_MarkIVFemale"),
        TEXT("/Game/Warfare/Vehicles/Production/Female/T_Portrait_BeuteMarkIVFemale.T_Portrait_BeuteMarkIVFemale"),
        TEXT("/Game/Warfare/UI/V63/T_Armour_MarkVMale.T_Armour_MarkVMale"),
        TEXT("/Game/Warfare/UI/V63/T_Armour_MarkIVPioneer.T_Armour_MarkIVPioneer"),
        TEXT("/Game/Warfare/UI/V63/T_Armour_A7VGretchen.T_Armour_A7VGretchen"),
        TEXT("/Game/Warfare/UI/V63/T_Armour_BeutePioneer.T_Armour_BeutePioneer"),
        TEXT("/Game/Warfare/UI/V52/T_CommandStars_V52.T_CommandStars_V52"),
        TEXT("/Game/Warfare/UI/V8215/T_GermanTechA_V8215.T_GermanTechA_V8215"),
        TEXT("/Game/Warfare/UI/V8215/T_GermanTechAArchive_V8215.T_GermanTechAArchive_V8215"),
        TEXT("/Game/Warfare/UI/V8215/T_MobileGunTech_V8215.T_MobileGunTech_V8215"),
        TEXT("/Game/Warfare/UI/V8215/T_ForwardServiceTeams_V8215.T_ForwardServiceTeams_V8215"),
        TEXT("/Game/Warfare/UI/V8215/T_MedalAtlasV8215.T_MedalAtlasV8215"),
        TEXT("/Game/Warfare/UI/V8220/T_ArmourKillTallyV8220.T_ArmourKillTallyV8220"),
        TEXT("/Game/Warfare/UI/V8217/T_MedalDisplayWall_V8217.T_MedalDisplayWall_V8217"),
        TEXT("/Game/Warfare/UI/V8217/T_RibbonAtlas_uk_V8217.T_RibbonAtlas_uk_V8217"),
        TEXT("/Game/Warfare/UI/V8217/T_RibbonAtlas_de_V8217.T_RibbonAtlas_de_V8217"),
        TEXT("/Game/Warfare/UI/V8217/T_RibbonAtlas_fr_V8217.T_RibbonAtlas_fr_V8217"),
        TEXT("/Game/Warfare/UI/V8217/T_RibbonAtlas_ru_V8217.T_RibbonAtlas_ru_V8217")})Paths.Add(Path);
    // The battlefield mount requests these even when no decoration is hung.
    // Warm them with the core instead of starting I/O on the first battle draw.
    for(const TCHAR* Name:{TEXT("SilverBar"),TEXT("GoldBar"),TEXT("BronzeBar"),TEXT("OakLeaves"),TEXT("BronzeDespatchOakLeaf"),TEXT("GoldRosette")})
        Paths.AddUnique(FString::Printf(TEXT("/Game/Warfare/UI/V8216/T_MedalAttachment_%s_V8216.T_MedalAttachment_%s_V8216"),Name,Name));
    for(const TCHAR* Name:{TEXT("SilverCrownBar"),TEXT("SilverEagleBar")})
        Paths.AddUnique(FString::Printf(TEXT("/Game/Warfare/UI/V85/Medals/T_MedalAttachment_%s_V85.T_MedalAttachment_%s_V85"),Name,Name));
    // Disposable failure acceptance only. No real package is changed or removed.
    if(MissingFailureReview())Paths.Add(TEXT("/Game/Warfare/UI/ReviewOnly/T_Missing_UIAsset_V8213.T_Missing_UIAsset_V8213"));
    for(const auto& Path:FrontlineArmourValourV8221::AssetPaths())Paths.AddUnique(Path);
    Paths.AddUnique(TEXT("/Game/FRRUIntegration/FormalV82/UI/T_ArmourResearch_FRRU_V87.T_ArmourResearch_FRRU_V87"));
    for(const TCHAR* Path:{TEXT("/Game/Warfare/UI/V837/Achievements/T_hearts_homeward_V837.T_hearts_homeward_V837"),
        TEXT("/Game/Warfare/UI/V837/Achievements/T_all_quiet_at_last_V837.T_all_quiet_at_last_V837"),
        TEXT("/Game/Warfare/UI/V837/T_Armour_RailConvoy_V837.T_Armour_RailConvoy_V837")})Paths.AddUnique(Path);
    PreloadTextures(Paths);
}
UTexture2D* Texture(const FString& Path)
{
    check(IsInGameThread());if(Path.IsEmpty())return nullptr;
    auto& C=Cache();
    if(BaselineMode())
    {
        ++C.SyncRequests;const double Begin=FPlatformTime::Seconds();
        auto* Loaded=LoadObject<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn);
        if(Loaded)C.BaselineTextures.Add(ObjectPath(Path),TWeakObjectPtr<UTexture2D>(Loaded));
        C.SyncLoadMs+=(FPlatformTime::Seconds()-Begin)*1000.;return Loaded;
    }
    const FString Key=ObjectPath(Path);
    if(C.Failed.Contains(Key))return nullptr;
    if(const auto* Existing=C.Textures.Find(Key)){++C.CacheHits;return Existing->Get();}
    if(C.Pending.Contains(Key))return nullptr;
    PreloadTextures({Key});
    if(const auto* Existing=C.Textures.Find(Key))return Existing->Get();
    return nullptr;
}
bool PreloadSettled()
{
    check(IsInGameThread());if(BaselineMode())return true;
    auto& C=Cache();if(!C.CoreRequested||!C.Pending.IsEmpty())return false;
    bool Settled=true;TArray<FString> Refence;
    for(const auto& Pair:C.Textures)
    {
        if(C.Failed.Contains(Pair.Key))continue;
        auto* Texture=Pair.Value.Get();
        if(ResourceReady(Texture)&&!SyntheticResourceFailure(Pair.Key))
        {C.ResourceChecks.Remove(Pair.Key);continue;}
        const auto* Check=C.ResourceChecks.Find(Pair.Key);
        if(!Check||Check->Resource!=(Texture?Texture->GetResource():nullptr))
        {Refence.Add(Pair.Key);Settled=false;continue;}
        if(!Check->Fence->IsFenceComplete()){Settled=false;continue;}
        // Initialization may finish between the first readiness read and this
        // fence poll. Re-read after the completion synchronization before failing.
        if(ResourceReady(Texture)&&!SyntheticResourceFailure(Pair.Key))
        {C.ResourceChecks.Remove(Pair.Key);continue;}
#if WITH_EDITOR
        // A compiler placeholder can outlive the load/fence and later replace
        // its resource. It is still pending, never a terminal negative cache.
        if(Texture&&Texture->IsDefaultTexture()){Settled=false;continue;}
#endif
        RecordFailure(Pair.Key,SyntheticResourceFailure(Pair.Key)?TEXT("review_resource_failure"):TEXT("resource_initialization_failed"));
        ++C.TerminalResourceFailures;C.ResourceChecks.Remove(Pair.Key);
    }
    QueueResourceChecks(Refence);return Settled;
}
bool PreloadReady(){return PreloadSettled()&&Cache().Failed.IsEmpty();}
TSharedRef<FJsonObject> Summary()
{
    check(IsInGameThread());const bool Settled=PreloadSettled();const auto& C=Cache();auto Out=MakeShared<FJsonObject>();
    uint64 CPU=0,ObjectCPU=0,UnknownCPU=0,GPU=0,AllMips=0;int32 Ready=0;TArray<TSharedPtr<FJsonValue>> Textures,Failures;
    TArray<FString> Keys;if(BaselineMode())C.BaselineTextures.GetKeys(Keys);else C.Textures.GetKeys(Keys);Keys.Sort();
    for(const auto& Key:Keys)
    {
        auto* T=BaselineMode()?C.BaselineTextures.FindChecked(Key).Get():C.Textures.FindChecked(Key).Get();if(!T)continue;
        ObjectCPU+=T->GetClass()->GetStructureSize();
        FResourceSizeEx Size(EResourceSizeMode::Exclusive);T->GetResourceSizeEx(Size);
        const uint64 Resident=T->CalcTextureMemorySizeEnum(TMC_ResidentMips),Full=T->CalcTextureMemorySizeEnum(TMC_AllMips);
        CPU+=Size.GetDedicatedSystemMemoryBytes();UnknownCPU+=Size.GetUnknownMemoryBytes();GPU+=Resident;AllMips+=Full;
        const bool RHIReady=ResourceReady(T);Ready+=RHIReady;
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("object"),Key);
        Row->SetNumberField(TEXT("width"),T->GetSizeX());Row->SetNumberField(TEXT("height"),T->GetSizeY());
        Row->SetNumberField(TEXT("resident_mips"),T->GetNumResidentMips());Row->SetNumberField(TEXT("resident_gpu_bytes"),double(Resident));
        Row->SetNumberField(TEXT("all_mips_gpu_bytes"),double(Full));Row->SetBoolField(TEXT("rhi_ready"),RHIReady);
        Textures.Add(MakeShared<FJsonValueObject>(Row));
    }
    TArray<FString> Missing=C.Failed.Array();Missing.Sort();for(const auto& Key:Missing)Failures.Add(MakeShared<FJsonValueString>(Key));
    Out->SetBoolField(TEXT("baseline_mode"),BaselineMode());Out->SetBoolField(TEXT("core_requested"),C.CoreRequested);
    Out->SetBoolField(TEXT("settled"),Settled);Out->SetBoolField(TEXT("ready"),Settled&&C.Failed.IsEmpty());
    Out->SetBoolField(TEXT("review_missing_injected"),!BaselineMode()&&C.CoreRequested&&MissingFailureReview());
    Out->SetBoolField(TEXT("review_resource_failure_requested"),!BaselineMode()&&ResourceFailureReview());
    Out->SetBoolField(TEXT("review_resource_failure_injected"),C.FailureReasons.FindRef(TEXT("/Game/Warfare/UI/V52/T_CommandStars_V52.T_CommandStars_V52"))==TEXT("review_resource_failure"));
    Out->SetNumberField(TEXT("loaded_textures"),Textures.Num());Out->SetNumberField(TEXT("strong_cached_textures"),C.Textures.Num());Out->SetNumberField(TEXT("rhi_ready_textures"),Ready);
    Out->SetNumberField(TEXT("pending_textures"),C.Pending.Num());Out->SetNumberField(TEXT("failed_textures"),C.Failed.Num());
    TArray<FString> PendingPaths=C.Pending.Array();PendingPaths.Sort();
    TArray<TSharedPtr<FJsonValue>> PendingValues;for(const auto& Path:PendingPaths)PendingValues.Add(MakeShared<FJsonValueString>(Path));
    Out->SetArrayField(TEXT("pending_paths"),PendingValues);
    Out->SetNumberField(TEXT("pending_resource_checks"),C.ResourceChecks.Num());Out->SetNumberField(TEXT("terminal_resource_failures"),double(C.TerminalResourceFailures));
    auto Reasons=MakeShared<FJsonObject>();for(const auto& Pair:C.FailureReasons)Reasons->SetStringField(Pair.Key,Pair.Value);
    Out->SetObjectField(TEXT("failure_reasons"),Reasons);
    Out->SetNumberField(TEXT("async_batches"),double(C.AsyncBatches));Out->SetNumberField(TEXT("already_loaded_textures"),double(C.AlreadyLoaded));
    Out->SetNumberField(TEXT("cache_hits"),double(C.CacheHits));Out->SetNumberField(TEXT("synchronous_requests"),double(C.SyncRequests));
    Out->SetNumberField(TEXT("synchronous_load_ms"),C.SyncLoadMs);
    Out->SetNumberField(TEXT("async_completion_ms"),C.CompletedAt>C.RequestedAt?(C.CompletedAt-C.RequestedAt)*1000.:0.);
    Out->SetNumberField(TEXT("estimated_cpu_resource_bytes"),double(CPU));Out->SetNumberField(TEXT("cpu_object_structure_bytes"),double(ObjectCPU));Out->SetNumberField(TEXT("unclassified_resource_bytes"),double(UnknownCPU));
    Out->SetNumberField(TEXT("resident_gpu_bytes"),double(GPU));Out->SetNumberField(TEXT("all_mips_gpu_bytes"),double(AllMips));
    Out->SetStringField(TEXT("memory_scope"),TEXT("Engine exclusive texture resource accounting, fixed UObject structure sizes and format-aware resident mip bytes; excludes the remaining process heap, async I/O buffers, driver allocation overhead and unrelated UI artwork. Baseline observes weak references without extending the original lifetime."));
    Out->SetArrayField(TEXT("textures"),Textures);Out->SetArrayField(TEXT("failures"),Failures);return Out;
}
}
