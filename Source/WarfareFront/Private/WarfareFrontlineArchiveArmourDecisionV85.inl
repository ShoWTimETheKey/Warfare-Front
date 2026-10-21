namespace
{
// Reserve a purpose before waiting for its own preparation. A cheaper model
// becoming ready must not restart every heavier model's shared queue forever.
int32 ArchiveArmourReserveV85(const TArray<FFrontlineVehicleEntry>& R,int32 Deployments,bool SeenArmour,
    TFunctionRef<bool(EFrontlineVehicle)> Eligible)
{
    TArray<int32> Cannon,Support;
    for(int32 I=0;I<R.Num();++I)if(Eligible(R[I].Type))
    {
        if(R[I].Type==EFrontlineVehicle::RailConvoy){if(Deployments==0)return I;continue;}
        (R[I].CannonCount>0?Cannon:Support).Add(I);
    }
    const int32 Cycle=FMath::Max(0,Deployments)%3;
    if(!Cannon.IsEmpty()&&(SeenArmour||Support.IsEmpty()||Cycle!=2))
        return Cannon[(FMath::Max(0,Deployments)/3*2+(Cycle==1?1:0))%Cannon.Num()];
    if(!Support.IsEmpty())return Support[(FMath::Max(0,Deployments)/3)%Support.Num()];
    if(!Cannon.IsEmpty())return Cannon[0];
    for(int32 I=0;I<R.Num();++I)if(Eligible(R[I].Type))return I;
    return INDEX_NONE;
}
}
