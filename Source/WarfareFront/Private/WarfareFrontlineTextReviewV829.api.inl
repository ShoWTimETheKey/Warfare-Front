// Candidate only. Include inside Presentation.cpp's anonymous namespace,
// immediately before WarfareFrontlineOperationPreparationV16.inl.
namespace TextReviewV829
{
static TSharedPtr<FJsonObject> LoadingOverride;
static bool Gate()
{
    FString UserDir;
    return FParse::Param(FCommandLine::Get(),TEXT("WFV829TextReview"))
        &&FParse::Param(FCommandLine::Get(),TEXT("WFRenderNoSave"))
        &&FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)&&!UserDir.IsEmpty();
}
static FString Loading(const TCHAR* Field,const FString& Fallback)
{
    FString Value;
    return Gate()&&LoadingOverride.IsValid()&&LoadingOverride->TryGetStringField(Field,Value)?Value:Fallback;
}
}
