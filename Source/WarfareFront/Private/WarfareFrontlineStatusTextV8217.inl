namespace FrontlineStatusTextV8217
{
FString Shooter(const AWarfareFrontlineGameMode* B,const FFrontlineSquad& Squad,bool PrimaryAlive)
{
    if(!B||Squad.Role!=EFrontlineRole::Marksman||B->GetFactionForTeam(Squad.Team)!=EFrontlineFaction::British)return {};
    return FrontlineArmourV63::L(PrimaryAlive?TEXT("Primary shooter operational"):TEXT("Primary shooter killed · observer remains"),
        PrimaryAlive?TEXT("主射手在位"):TEXT("主射手阵亡 · 仅剩观察手"));
}
FString SupportArrival(const FVector2D& Delay)
{
    if(FrontlineLocalizationV35::GetLanguage()==FrontlineLocalizationV35::ELanguage::Chinese)
        return FString::Printf(TEXT("预计 %.0f–%.0f 秒后抵达"),Delay.X,Delay.Y);
    return FString::Printf(TEXT("Arrival: %.0f–%.0f seconds"),Delay.X,Delay.Y);
}
}
