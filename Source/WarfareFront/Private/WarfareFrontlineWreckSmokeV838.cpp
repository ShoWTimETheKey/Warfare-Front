#include "WarfareFrontlineWreckSmokeV838.h"
#include "WarfareFrontlineVehiclesV28.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "WarfareFrontlineSmokeVolumeV35.h"
#include "Materials/MaterialInterface.h"

namespace FrontlineWreckSmokeV838
{
UMaterialInterface* Material()
{
    if(auto* Soot=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V838/Materials/M_WreckSoot_V838.M_WreckSoot_V838")))return Soot;
    // Existing packaged material remains a recovery fallback. Dedicated
    // runtime/import checks require the actual derived wreck material.
    return FrontlineSmokeVolumeV35::MaterialV40();
}
FFrontlineWreckSmokeProfileV838 Profile(EFrontlineVehicle Type,const UStaticMeshComponent* Hull)
{
    FFrontlineWreckSmokeProfileV838 P;float ReferenceLength=784.f;
    const auto Add=[&](const TCHAR* Name,const FVector& At,float Width,float Flame,float Smoke,float Scale,bool Primary)
    {FFrontlineWreckSmokeSiteV838 S;S.Name=Name;S.LocalOrigin=At;S.WidthCm=Width;S.FlameWeight=Flame;
     S.SmokeWeight=Smoke;S.FlameScale=Scale;S.Primary=Primary;P.Sites.Add(S);};
    switch(Type)
    {
    case EFrontlineVehicle::Austin1918:
        P.Name=TEXT("austin_bonnet_and_cabin");ReferenceLength=470.4f;
        Add(TEXT("bonnet_engine"),FVector(151,-12,128),126,.78f,.84f,.68f,true);
        Add(TEXT("rear_fighting_compartment"),FVector(-100,18,204),104,.38f,.44f,.56f,false);break;
    case EFrontlineVehicle::Whippet:
        P.Name=TEXT("whippet_forward_engine_and_rear_compartment");ReferenceLength=615.f;
        Add(TEXT("forward_engine_panel"),FVector(127,-16,172),160,.88f,1.f,.86f,true);
        Add(TEXT("rear_fighting_compartment"),FVector(-161,18,238),130,.54f,.60f,.72f,false);break;
    case EFrontlineVehicle::A7V:case EFrontlineVehicle::A7VGretchen:
        P.Name=TEXT("a7v_twin_engine_roof");ReferenceLength=732.f;
        Add(TEXT("rear_engine_roof"),FVector(-125,-26,252),205,1.f,1.f,1.f,true);
        Add(TEXT("forward_engine_roof"),FVector(73,27,255),176,.64f,.70f,.88f,false);break;
    case EFrontlineVehicle::MarkVMale:
        P.Name=TEXT("mark_v_raised_engine_and_body");
        Add(TEXT("raised_engine_hatch"),FVector(-122,-23,290),216,1.f,1.f,1.04f,true);
        Add(TEXT("forward_body_seam"),FVector(27,32,225),163,.54f,.60f,.83f,false);break;
    case EFrontlineVehicle::MarkVStar:
        P.Name=TEXT("mark_v_star_extended_engine_and_body");ReferenceLength=966.5f;
        Add(TEXT("extended_rear_engine_hatch"),FVector(-252,-23,292),235,1.06f,1.08f,1.08f,true);
        Add(TEXT("forward_body_seam"),FVector(27,32,228),182,.58f,.64f,.86f,false);break;
    case EFrontlineVehicle::RailConvoy:
        // Unequal authored cars, not a convoy-length plume. The locomotive's
        // soot comes from fire at the cab/boiler seam, never its steam chimney.
        P.Name=TEXT("six_separate_rail_fire_sites");ReferenceLength=5200.f;
        Add(TEXT("front_gun_car"),FVector(2210,15,295),194,.78f,.80f,.95f,true);
        Add(TEXT("front_troop_car"),FVector(1320,-22,252),152,.46f,.52f,.78f,false);
        Add(TEXT("locomotive_cab_boiler_seam"),FVector(310,-30,305),218,.70f,.92f,1.f,true);
        Add(TEXT("coal_tender"),FVector(-580,15,175),178,.48f,.67f,.82f,false);
        Add(TEXT("rear_troop_car"),FVector(-1400,40,240),148,.46f,.50f,.78f,false);
        Add(TEXT("rear_gun_car"),FVector(-2260,-12,295),184,.64f,.72f,.91f,true);break;
    default:
        // Male/female/captured/pioneer variants use the same physical chassis.
        P.Name=TEXT("mark_iv_central_engine_and_body");
        Add(TEXT("central_engine_roof_seam"),FVector(-122,-23,224),205,1.f,1.f,1.f,true);
        Add(TEXT("forward_folded_roof_panel"),FVector(27,32,224),157,.55f,.59f,.82f,false);break;
    }
    if(Hull&&Hull->GetStaticMesh())
    {
        const FBox Bounds=Hull->GetStaticMesh()->GetBoundingBox();const float Length=float(Bounds.GetSize().X);
        if(Bounds.IsValid&&!Bounds.Min.ContainsNaN()&&!Bounds.Max.ContainsNaN()&&Length>100)
        {
            // Use the current imported model, with bounded scaling so a small
            // replacement detail or a coupler cannot inflate the smoke column.
            P.FootprintScale=FMath::Clamp(FMath::Sqrt(Length/ReferenceLength),.80f,1.18f);
            for(auto& S:P.Sites)
            {
                S.WidthCm*=P.FootprintScale;
                S.LocalOrigin.X=FMath::Clamp(S.LocalOrigin.X,float(Bounds.Min.X+12),float(Bounds.Max.X-12));
                S.LocalOrigin.Y=FMath::Clamp(S.LocalOrigin.Y,float(Bounds.Min.Y+8),float(Bounds.Max.Y-8));
                S.LocalOrigin.Z=FMath::Clamp(S.LocalOrigin.Z,float(Bounds.Min.Z+12),float(Bounds.Max.Z+6));
            }
        }
    }
    return P;
}
FVector Wind(float GameTime)
{
    return FVector(18.f+7.f*FMath::Sin(GameTime*.41f),-9.f+5.f*FMath::Sin(GameTime*.29f),0);
}
FFrontlineWreckSmokeLayerV838 Layer(const FVector& Source,float WidthCm,float Strength,float Heat,
    float SourcePhase,float FlowAge,float FeedAge,float GameTime,int32 Index)
{
    FFrontlineWreckSmokeLayerV838 L;
    if(Source.ContainsNaN()||!FMath::IsFinite(WidthCm)||!FMath::IsFinite(Strength)||!FMath::IsFinite(Heat)
        ||!FMath::IsFinite(SourcePhase)||!FMath::IsFinite(FlowAge)||!FMath::IsFinite(FeedAge)
        ||!FMath::IsFinite(GameTime)||Index<0||Index>=LayersPerSource)return L;
    const float Hot=FMath::Clamp(Heat/1.45f,0.f,1.f);
    const float Width=FMath::Clamp(WidthCm,70.f,280.f)*FMath::Lerp(.72f,1.f,Hot);
    const float Rise=FMath::Lerp(.64f,1.f,Hot);
    const float Life=1.f-FMath::SmoothStep(.15f,TailSeconds,FMath::Max(0.f,FeedAge));
    const float Filled=FMath::SmoothStep(Index*.27f,Index*.27f+.62f,FMath::Max(0.f,FlowAge));
    const float Roll=.91f+.06f*FMath::Sin(FlowAge*1.36f+SourcePhase-Index*.72f)
        +.03f*FMath::Sin(FlowAge*2.1f+SourcePhase*.63f-Index*1.13f);
    const float Across[]={1.05f,1.60f,2.10f},Deep[]={.94f,1.43f,1.88f};
    const float Tall[]={1.35f,2.05f,2.55f},Height[]={.52f,1.48f,3.04f};
    const float Optical[]={.82f,.60f,.39f};
    L.Dimensions=FVector(Width*Across[Index],Width*Deep[Index],Width*Tall[Index]*Rise);
    const float Transit=(.6f+Index*1.5f)*FMath::Lerp(1.35f,1.f,Hot);
    const float Curl=SourcePhase+FlowAge*.72f-Index*.8f;
    const FVector Bend=FVector(FMath::Sin(Curl),FMath::Cos(Curl*.83f+1.7f),0)*Width*(.025f+.025f*Index);
    // Wind is coherent across sources and builds with height; buoyancy is
    // stronger than the lateral drift, keeping the column above its fire.
    L.Centre=Source+Wind(GameTime)*Transit+Bend+FVector(0,0,Width*Height[Index]*Rise);
    L.Density=FMath::Clamp(Strength,0.f,1.55f)*Optical[Index]*Life*Filled*Roll;
    const float Grey=FMath::Lerp(.026f,.047f,Index*.5f);
    L.Colour=FLinearColor(Grey,Grey,Grey*.97f,1);
    // The dedicated shader uses its pause-aware game clock for convection.
    // Source age remains available for actual lifecycle readback; no burst
    // frame age owns the primary finite soot body.
    L.FieldAge=FMath::Clamp(FlowAge,0.f,120.f);
    return L;
}
}
