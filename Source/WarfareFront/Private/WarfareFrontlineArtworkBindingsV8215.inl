// New paired plates. Historical baseline packages remain untouched.
auto NewPlateV8215=[&](const TCHAR* Name,int32 Slot)->FFrontlineArtRegionV71
{
    const FString Path=FString::Printf(TEXT("/Game/Warfare/UI/V8215/%s.%s"),Name,Name);
    // Bind once during the artwork constructor. An asynchronous cache miss here
    // would otherwise remain a null plate for the lifetime of the UI subsystem.
    return {LoadObject<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn),Slot,2,2};
};
TechnologyPaintingsV71[1][1]=NewPlateV8215(TEXT("T_GermanTechA_V8215"),1);
ArchivePaintingsV74[1][1]=NewPlateV8215(TEXT("T_GermanTechAArchive_V8215"),1);
for(int32 N=0;N<2;++N)
{
    TechnologyPaintingsV71[N][27]=NewPlateV8215(TEXT("T_MobileGunTech_V8215"),N);
    ArchivePaintingsV74[N][27]=NewPlateV8215(TEXT("T_MobileGunTech_V8215"),N+2);
    ArmourTechnologyPaintingsV71[N][8]=NewPlateV8215(TEXT("T_ForwardServiceTeams_V8215"),N);
    ArchivePaintingsV74[N][36]=NewPlateV8215(TEXT("T_ForwardServiceTeams_V8215"),N+2);
}
