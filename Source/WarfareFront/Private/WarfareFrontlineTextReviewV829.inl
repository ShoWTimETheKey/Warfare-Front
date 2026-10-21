// Candidate only: include after the existing review inls at the end of
// Presentation.cpp. Declare Draw before DrawHUD as described in INTEGRATION.txt.
// Does not create/select/start any campaign, prepare an encounter or save progress.
namespace
{
namespace TextReviewV829
{
struct FRun
{
    bool Initialized=false,Done=false,Good=true,Requested=false;
    int32 Index=0,First=0,Limit=0,Frames=0,LegacyKeys=0,LegacyCalls=0;
    double PageStarted=0,Started=0;
    FString Dir,ManifestPath;
    TSharedPtr<FJsonObject> Manifest;
    TArray<TSharedPtr<FJsonValue>> Pages,Reports,LegacyMismatches;
    void Save(const FString& Name,const TSharedRef<FJsonObject>& Object)
    {
        FString Text;FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));
        Good&=FFileHelper::SaveStringToFile(Text,*(Dir/Name));
    }
    void Finish(const FString& Error=FString())
    {
        if(Done)return;Done=true;Good&=Error.IsEmpty();LoadingOverride.Reset();
        const bool Coverage=FrontlineLocalizationV35::WriteCoverageReport();Good&=Coverage;
        auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),Good);
        J->SetStringField(TEXT("error"),Error);J->SetStringField(TEXT("manifest"),ManifestPath);
        J->SetStringField(TEXT("scope"),TEXT("Exact production Canvas Paragraph and fonts; unchanged source rectangle dimensions. Translated four-card sample sheets and original full LoadingPage. No campaign execution. Actual V81 UI and manual screenshot inspection remain separate."));
        J->SetArrayField(TEXT("pages"),Reports);J->SetNumberField(TEXT("first_page"),First);
        J->SetNumberField(TEXT("legacy_exact_keys"),LegacyKeys);J->SetNumberField(TEXT("legacy_resolver_calls"),LegacyCalls);
        J->SetArrayField(TEXT("legacy_resolver_mismatches"),LegacyMismatches);J->SetBoolField(TEXT("runtime_coverage_success"),Coverage);
        FString CoverageText;TSharedPtr<FJsonObject> CoverageJSON;
        if(FFileHelper::LoadFileToString(CoverageText,*(FPaths::ProjectSavedDir()/TEXT("Gameplay/V35/localization-runtime.json")))
            &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CoverageText),CoverageJSON)&&CoverageJSON.IsValid())
        {J->SetObjectField(TEXT("localization_runtime"),CoverageJSON.ToSharedRef());J->SetNumberField(TEXT("format_signature_errors"),CoverageJSON->GetNumberField(TEXT("format_signature_errors")));J->SetNumberField(TEXT("missing_messages"),CoverageJSON->GetArrayField(TEXT("unmapped_visible_messages")).Num());}
        J->SetNumberField(TEXT("requested_pages"),Limit);J->SetNumberField(TEXT("completed_pages"),Reports.Num());
        J->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        if(Manifest.IsValid())J->SetObjectField(TEXT("source_sha256"),Manifest->GetObjectField(TEXT("sourceSHA256")));
        Save(TEXT("review.json"),J);UE_LOG(LogTemp,Display,TEXT("WF_V829_TEXT_REVIEW_COMPLETE success=%d pages=%d error=%s"),Good,Reports.Num(),*Error);
        FPlatformMisc::RequestExit(false);
    }
    bool Init(AWarfareFrontlineGameMode* B)
    {
        Initialized=true;Started=FPlatformTime::Seconds();Dir=FPaths::ProjectSavedDir()/TEXT("V829TextReview");
        FParse::Value(FCommandLine::Get(),TEXT("WFV829TextOutput="),Dir);IFileManager::Get().MakeDirectory(*Dir,true);IFileManager::Get().MakeDirectory(*(Dir/TEXT("captures")),true);
        FString Text;
        if(!B||!B->IsProgressionVolatile()){Finish(TEXT("Disposable progression required"));return false;}
        if(!FParse::Value(FCommandLine::Get(),TEXT("WFV829TextManifest="),ManifestPath)||!FFileHelper::LoadFileToString(Text,*ManifestPath)
            ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Manifest)||!Manifest.IsValid())
            {Finish(TEXT("Cannot load review manifest"));return false;}
        const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
        if(!Manifest->TryGetArrayField(TEXT("pages"),Array)||!Array||Array->IsEmpty()){Finish(TEXT("Review pages missing"));return false;}
        Pages=*Array;FParse::Value(FCommandLine::Get(),TEXT("WFV829TextFirst="),First);
        const auto& OldKeys=Manifest->GetArrayField(TEXT("legacyKeys"));LegacyKeys=OldKeys.Num();Good&=LegacyKeys==4517;
        for(const FString Lang:{FString(TEXT("en")),FString(TEXT("zh"))})
        {
            FrontlineLocalizationV35::SetLanguage(Lang==TEXT("zh")?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English);
            for(const auto& Value:OldKeys)
            {
                const auto Row=Value->AsObject();const FString Key=Row->GetStringField(TEXT("key")),Expected=Row->GetStringField(Lang);
                const FString Actual=FrontlineLocalizationV35::Text(Key);++LegacyCalls;
                if(Actual!=Expected){auto Bad=MakeShared<FJsonObject>();Bad->SetStringField(TEXT("key"),Key);Bad->SetStringField(TEXT("language"),Lang);Bad->SetStringField(TEXT("expected"),Expected);Bad->SetStringField(TEXT("actual"),Actual);LegacyMismatches.Add(MakeShared<FJsonValueObject>(Bad));Good=false;}
            }
        }
        First=FMath::Clamp(First,0,Pages.Num()-1);Limit=Pages.Num()-First;
        FParse::Value(FCommandLine::Get(),TEXT("WFV829TextCount="),Limit);Limit=FMath::Clamp(Limit,1,Pages.Num()-First);Index=First;
        return true;
    }
    void Begin(const FFrontlineCanvas& UI)
    {
        V36TypographyAudit.Empty();V27ImageAudit.Empty();V825FontDraws.Empty();V825FontEdges.Reset();
        V36TypographyAuditActive=V27ImageAuditActive=true;
        V825FontAuditActive=FParse::Param(FCommandLine::Get(),TEXT("WFV825FontLayout"));
        if(V825FontAuditActive)UI.AuditFontEdgesV825();
    }
    TSharedRef<FJsonObject> Record(const FFrontlineCanvas& UI,const TSharedPtr<FJsonObject>& Sample,
        const FString& Language,float X,float Y,float W,float H,float RequestedSize)
    {
        auto J=MakeShared<FJsonObject>();const FString Expected=Sample->GetObjectField(TEXT("expected"))->GetStringField(Language);
        const FString Raw=Sample->GetObjectField(TEXT("raw"))->GetStringField(Language);const FString Actual=UI.DisplayText(Raw);
        J->SetStringField(TEXT("sample_id"),Sample->GetStringField(TEXT("sampleId")));
        J->SetArrayField(TEXT("stable_ids"),Sample->GetArrayField(TEXT("ids")));
        J->SetStringField(TEXT("language"),Language);J->SetStringField(TEXT("expected"),Expected);J->SetStringField(TEXT("resolved"),Actual);
        J->SetStringField(TEXT("text_sha256"),Sample->GetObjectField(TEXT("textSHA256"))->GetStringField(Language));
        J->SetStringField(TEXT("profile"),Sample->GetStringField(TEXT("profile")));
        J->SetNumberField(TEXT("requested_font_size"),RequestedSize);J->SetNumberField(TEXT("x"),X);J->SetNumberField(TEXT("y"),Y);
        J->SetNumberField(TEXT("width"),W);J->SetNumberField(TEXT("height"),H);
        J->SetNumberField(TEXT("crop_x"),UI.OX+X*UI.Scale);J->SetNumberField(TEXT("crop_y"),UI.OY+Y*UI.Scale);
        J->SetNumberField(TEXT("crop_width"),W*UI.Scale);J->SetNumberField(TEXT("crop_height"),H*UI.Scale);
        TSharedPtr<FJsonObject> Found;
        for(const auto& E:V36TypographyAudit)
            if(E.Value->GetStringField(TEXT("text"))==Actual
                &&FMath::IsNearlyEqual(float(E.Value->GetNumberField(TEXT("x"))),X,.01f)
                &&FMath::IsNearlyEqual(float(E.Value->GetNumberField(TEXT("y"))),Y,.01f)
                &&FMath::IsNearlyEqual(float(E.Value->GetNumberField(TEXT("width"))),W,.01f)
                &&FMath::IsNearlyEqual(float(E.Value->GetNumberField(TEXT("height"))),H,.01f)){Found=E.Value;break;}
        const float Minimum=FMath::Min(RequestedSize,FFrontlineCanvas::ContainsCJKV45(Actual)?15.f:13.f);
        const bool AtMinimum=Found.IsValid()&&RequestedSize>Minimum&&Found->GetNumberField(TEXT("font_size"))<=Minimum+.01;
        const bool Pass=Found.IsValid()&&Actual==Expected&&Found->GetBoolField(TEXT("fits"))&&!Found->GetBoolField(TEXT("truncated"))&&!AtMinimum;
        J->SetBoolField(TEXT("at_minimum_font"),AtMinimum);J->SetBoolField(TEXT("success"),Pass);
        if(Found.IsValid())J->SetObjectField(TEXT("production_typography"),Found.ToSharedRef());return J;
    }
    bool Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FFrontlineScreenState& State)
    {
        if(!Gate())return false;if(!Initialized&&!Init(B))return true;if(Done)return true;
        if(!B->IsProgressionVolatile()){Finish(TEXT("Progression isolation lost"));return true;}
        if(UI.Surface->ClipX!=3840||UI.Surface->ClipY!=2160){Finish(TEXT("Exact 3840x2160 viewport required"));return true;}
        const double Now=FPlatformTime::Seconds();
        const auto Page=Pages[Index]->AsObject();const FString Language=Page->GetStringField(TEXT("language"));
        const auto Wanted=Language==TEXT("zh")?FrontlineLocalizationV35::ELanguage::Chinese:FrontlineLocalizationV35::ELanguage::English;
        if(FrontlineLocalizationV35::GetLanguage()!=Wanted)FrontlineLocalizationV35::SetLanguage(Wanted);
        if(Frames==0){PageStarted=Now;Requested=false;}
        Begin(UI);const FString Profile=Page->GetStringField(TEXT("profile"));const auto P=Manifest->GetObjectField(TEXT("profiles"))->GetObjectField(Profile);
        const float W=P->GetNumberField(TEXT("width")),H=P->GetNumberField(TEXT("height")),Size=P->GetNumberField(TEXT("size"));
        const auto& Samples=Page->GetArrayField(TEXT("samples"));TArray<TSharedPtr<FJsonValue>> SampleReports;
        const FLinearColor Ink(.035f,.038f,.035f);
        FString ManualPage;
        if(Page->TryGetStringField(TEXT("manualPage"),ManualPage))
        {
            TGuardValue<bool> Help(State.Help,true);
            TGuardValue<bool> Credits(State.Credits,ManualPage==TEXT("credits"));
            TGuardValue<bool> Terms(State.ManualTermsV829,ManualPage==TEXT("terms"));
            DrawFieldManual(UI);
        }
        else if(Profile==TEXT("loading"))
        {
            const auto S=Samples[0]->AsObject();auto Override=MakeShared<FJsonObject>();
            Override->SetStringField(TEXT("narrative"),S->GetObjectField(TEXT("raw"))->GetStringField(Language));
            Override->SetStringField(TEXT("title"),S->GetObjectField(TEXT("heading"))->GetStringField(Language));
            Override->SetStringField(TEXT("date"),S->GetObjectField(TEXT("date"))->GetStringField(Language));
            Override->SetStringField(TEXT("nation"),S->GetObjectField(TEXT("nation"))->GetStringField(Language));
            TGuardValue<TSharedPtr<FJsonObject>> ScopedOverride(LoadingOverride,Override);
            FrontlineLoadingPageV82::Draw(UI,B,State);
            SampleReports.Add(MakeShared<FJsonValueObject>(Record(UI,S,Language,1277,201,W,H,Size)));
        }
        else
        {
            UI.Rect(0,0,1920,1080,FLinearColor(.018f,.022f,.024f));
            // Test markers bypass DisplayText so they do not contaminate the
            // production localization coverage report.
            UI.RuntimeTextV62(Page->GetStringField(TEXT("pageId")),60,24,18,Paper,false);
            for(int32 I=0;I<Samples.Num();++I)
            {
                const auto S=Samples[I]->AsObject();const float X=140.f+(I%2)*940.f,Y=104.f+(I/2)*474.f;
                UI.PaperSheet(X-24,Y-64,W+48,H+88);
                UI.RuntimeTextV62(S->GetStringField(TEXT("sampleId")),X,Y-46,17,Ink,false);
                UI.Paragraph(S->GetObjectField(TEXT("raw"))->GetStringField(Language),X,Y,W,H,Size,Ink,P->GetBoolField(TEXT("plain")));
                SampleReports.Add(MakeShared<FJsonValueObject>(Record(UI,S,Language,X,Y,W,H,Size)));
            }
        }
        ++Frames;
        // Keep the same draw on the frame after the screenshot request. Requests
        // are asynchronous; do not advance while a capture remains outstanding.
        if(Frames>=4&&Now-PageStarted>=(Index==First?3.0:.45)&&!Requested)
        {
            const FString Capture=Dir/TEXT("captures")/(Page->GetStringField(TEXT("pageId"))+TEXT(".png"));
            FScreenshotRequest::RequestScreenshot(Capture,true,false);Requested=true;
        }
        else if(Requested&&Frames>=6&&Now-PageStarted>=.65&&!FScreenshotRequest::IsScreenshotRequested())
        {
            auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("page_id"),Page->GetStringField(TEXT("pageId")));
            R->SetStringField(TEXT("capture"),TEXT("captures/")+Page->GetStringField(TEXT("pageId"))+TEXT(".png"));
            R->SetArrayField(TEXT("samples"),SampleReports);R->SetNumberField(TEXT("viewport_width"),UI.Surface->ClipX);R->SetNumberField(TEXT("viewport_height"),UI.Surface->ClipY);
            bool PageGood=true;for(const auto& S:SampleReports)PageGood&=S->AsObject()->GetBoolField(TEXT("success"));
            TArray<TSharedPtr<FJsonValue>> Texts,Draws;for(const auto& E:V36TypographyAudit)
            {
                Texts.Add(MakeShared<FJsonValueObject>(E.Value));PageGood&=E.Value->GetBoolField(TEXT("fits"))&&!E.Value->GetBoolField(TEXT("truncated"));
                bool Check=true;if(E.Value->TryGetBoolField(TEXT("painted_rows_match_authored"),Check))PageGood&=Check;
                if(E.Value->TryGetBoolField(TEXT("painted_rows_drawn_verbatim"),Check))PageGood&=Check;
            }
            float WidthError=0,HeightError=0;
            for(const auto& E:V825FontDraws){Draws.Add(MakeShared<FJsonValueObject>(E.Value));WidthError=FMath::Max(WidthError,float(E.Value->GetNumberField(TEXT("width_error"))));HeightError=FMath::Max(HeightError,float(E.Value->GetNumberField(TEXT("height_error"))));}
            R->SetArrayField(TEXT("typography"),Texts);R->SetArrayField(TEXT("native_draws"),Draws);
            const bool Native=V825FontEdges.IsValid()&&V825FontEdges->GetBoolField(TEXT("success"))&&Draws.Num()>0&&WidthError<.05f&&HeightError<.05f;
            R->SetNumberField(TEXT("maximum_native_width_error"),WidthError);R->SetNumberField(TEXT("maximum_native_height_error"),HeightError);
            R->SetBoolField(TEXT("native_font_check"),Native);PageGood&=Native;
            if(V825FontEdges.IsValid())R->SetObjectField(TEXT("font_edges"),V825FontEdges.ToSharedRef());
            R->SetBoolField(TEXT("success"),PageGood);Good&=PageGood;Reports.Add(MakeShared<FJsonValueObject>(R));
            Save(Page->GetStringField(TEXT("pageId"))+TEXT(".json"),R);
            UE_LOG(LogTemp,Display,TEXT("WF_V829_TEXT_PAGE page=%s samples=%d success=%d"),*Page->GetStringField(TEXT("pageId")),SampleReports.Num(),PageGood);
            Frames=0;++Index;if(Index>=First+Limit)Finish();
        }
        else if(Now-PageStarted>20){Finish(TEXT("Screenshot completion timed out"));}
        V36TypographyAuditActive=V27ImageAuditActive=V825FontAuditActive=false;return true;
    }
};
static bool Draw(const FFrontlineCanvas& UI,AWarfareFrontlineGameMode* B,FFrontlineScreenState& State)
{static FRun Run;return Run.Draw(UI,B,State);}
}
}
