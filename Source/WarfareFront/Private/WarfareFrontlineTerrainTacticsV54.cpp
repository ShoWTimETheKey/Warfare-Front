#include "WarfareFrontlineTerrainTacticsV54.h"
#include "WarfareFrontlineGround.h"
#include "WarfareFrontlineBattlefield.h"
#include "WarfareFrontlineFieldV32.h"
#include "WarfareFrontlineGameplayV33.h"
#include "WarfareFrontlineProgression.h"
#include "WarfareFRRUWorldV82.h"
#include "Misc/ScopeExit.h"

namespace
{
// Marsh water and the canal bottom are slow crossings. Broken masonry, drift
// and the raised rail ballast cost less time; no surface stops a whole squad.
float InfantryScale(const FFrontlineTerrainFeaturesV54& F)
{
    const float Drag=FMath::Max(FMath::Max3(.27f*F.MarshChannel,.21f*F.CanalCut,.13f*F.Ruins),
        FMath::Max3(.15f*F.SnowDrift,.075f*F.RailShoulder,.27f*F.RailWash));
    const float Field=FMath::Max3(.12f*F.FieldDitch,.07f*F.FieldBoundary,.11f*F.TankChurn);
    return FMath::Clamp(1.f-FMath::Max3(Drag,Field,FMath::Max(.055f*F.RidgeCrest,.17f*F.WoodlandGully)),.72f,1.f);
}
float ArmourScale(const FFrontlineTerrainFeaturesV54& F)
{
    // Tanks bear disproportionately on saturated ground and cannot skim a
    // cutting or collapsed wall at their road speed. The train has a fixed rail
    // route and is excluded by its caller.
    const float Drag=FMath::Max(FMath::Max3(.43f*F.MarshChannel,.34f*F.CanalCut,.20f*F.Ruins),
        FMath::Max3(.14f*F.SnowDrift,.16f*F.RailShoulder,.40f*F.RailWash));
    const float Field=FMath::Max3(.22f*F.FieldDitch,.15f*F.FieldBoundary,.24f*F.TankChurn);
    return FMath::Clamp(1.f-FMath::Max3(Drag,Field,FMath::Max(.10f*F.RidgeCrest,.29f*F.WoodlandGully)),.55f,1.f);
}
float HitScale(const FFrontlineTerrainFeaturesV54& From,const FFrontlineTerrainFeaturesV54& To,
    const FVector& Shooter,const FVector& Target,bool bMoving)
{
    // A canal bottom, a cellar floor and a shallow field drain sit below broken
    // lips. Raised rubble, field boundaries and rail shoulders stay exposed.
    // This is used only after sight/range checks and never grants trench orders.
    const float Shelter=FMath::Max(FMath::Max3(.19f*To.RuinCellar,.14f*To.CanalCut,.08f*To.FieldDitch),
        FMath::Max(.10f*To.RailWash,.12f*To.WoodlandGully));
    const float Posture=bMoving?.32f:1.f;
    const float HeightAdvantage=FMath::Clamp((Shooter.Z-Target.Z-35.f)/160.f,0.f,1.f)*From.RidgeCrest;
    return FMath::Clamp((1.f-Shelter*Posture)*(1.f+.075f*HeightAdvantage),.78f,1.08f);
}
}

float FrontlineTerrainTacticsV54::InfantryTravelScale(const FVector& Position)
{
    if(WarfareFRRUWorldV82::PhysicalSceneActive())return WarfareFRRUWorldV82::GlobalTravelScale(Position);
    if(Position.ContainsNaN())return 1.f;
    if(FMath::Abs(Position.X)>FrontlineBattlefield::GroundHalfLength||FMath::Abs(Position.Y)>FrontlineBattlefield::HalfFrontage)return 1.f;
    return InfantryScale(AWarfareFrontlineGround::QueryTerrainFeaturesV54(Position.X,Position.Y));
}
float FrontlineTerrainTacticsV54::VehicleTravelScale(const FVector& Position)
{
    if(Position.ContainsNaN())return 1.f;
    if(FMath::Abs(Position.X)>FrontlineBattlefield::GroundHalfLength||FMath::Abs(Position.Y)>FrontlineBattlefield::HalfFrontage)return 1.f;
    return ArmourScale(AWarfareFrontlineGround::QueryTerrainFeaturesV54(Position.X,Position.Y));
}
float FrontlineTerrainTacticsV54::DirectFireHitScale(const FVector& Shooter,const FVector& Target,bool bTargetMoving)
{
    if(WarfareFRRUWorldV82::PhysicalSceneActive())return 1.f;
    if(Shooter.ContainsNaN()||Target.ContainsNaN())return 1.f;
    if(FMath::Abs(Shooter.X)>FrontlineBattlefield::GroundHalfLength||FMath::Abs(Target.X)>FrontlineBattlefield::GroundHalfLength
        ||FMath::Abs(Shooter.Y)>FrontlineBattlefield::HalfFrontage||FMath::Abs(Target.Y)>FrontlineBattlefield::HalfFrontage)return 1.f;
    return HitScale(AWarfareFrontlineGround::QueryTerrainFeaturesV54(Shooter.X,Shooter.Y),
        AWarfareFrontlineGround::QueryTerrainFeaturesV54(Target.X,Target.Y),Shooter,Target,bTargetMoving);
}
void FrontlineTerrainTacticsV54::RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    FFrontlineTerrainFeaturesV54 Flat,Marsh,Ridge,Gully,Ruins,Canal,Rail,Wash,Snow,Farm;
    Marsh.MarshChannel=1.f;Ridge.RidgeCrest=1.f;Ruins.Ruins=1.f;
    Gully.WoodlandGully=1.f;
    Canal.CanalCut=1.f;Rail.RailShoulder=1.f;Wash.RailWash=1.f;Snow.SnowDrift=1.f;
    Ruins.RuinCellar=1.f;
    Farm.FieldDitch=1.f;Farm.FieldBoundary=1.f;Farm.TankChurn=1.f;
    Check(TEXT("v54_flat_land_retains_baseline_pace_and_fire"),InfantryScale(Flat)==1.f&&ArmourScale(Flat)==1.f
        &&HitScale(Flat,Flat,FVector(0,0,100),FVector(1200,0,100),false)==1.f);
    Check(TEXT("v54_marsh_and_canal_crossings_slow_both_armies_locally"),
        InfantryScale(Marsh)<InfantryScale(Flat)&&InfantryScale(Canal)<InfantryScale(Flat)
        &&ArmourScale(Marsh)<InfantryScale(Marsh)&&ArmourScale(Canal)<InfantryScale(Canal));
    Check(TEXT("v54_flooded_marsh_and_canal_cost_armour_more_than_dry_ruts"),
        ArmourScale(Marsh)<ArmourScale(Canal)&&ArmourScale(Canal)<ArmourScale(Farm)
        &&ArmourScale(Marsh)>.55f&&InfantryScale(Marsh)>.7f);
    Check(TEXT("v54_rail_rubble_and_drift_are_distinct_bounded_footing"),
        InfantryScale(Rail)>InfantryScale(Ruins)&&InfantryScale(Ruins)>InfantryScale(Snow)
        &&ArmourScale(Rail)<InfantryScale(Rail)&&InfantryScale(Snow)>.8f);
    Check(TEXT("v57_menin_gully_has_shelter_at_a_cost_while_the_road_keeps_pace"),
        InfantryScale(Gully)<InfantryScale(Ridge)&&ArmourScale(Gully)<InfantryScale(Gully)
        &&HitScale(Flat,Gully,FVector(0,0,120),FVector(1000,0,120),false)<1.f
        &&HitScale(Flat,Gully,FVector(0,0,120),FVector(1000,0,120),true)
            >HitScale(Flat,Gully,FVector(0,0,120),FVector(1000,0,120),false));
    Check(TEXT("v56_selle_wash_has_local_footing_and_cover_but_no_rail_roadblock"),
        InfantryScale(Wash)<InfantryScale(Rail)&&ArmourScale(Wash)<InfantryScale(Wash)
        &&InfantryScale(Wash)>.72f&&ArmourScale(Wash)>.55f
        &&HitScale(Flat,Wash,FVector(0,0,100),FVector(1000,0,100),false)
          <HitScale(Flat,Rail,FVector(0,0,100),FVector(1000,0,100),false));
    Check(TEXT("v54_shallow_farmland_relief_changes_crossing_without_becoming_a_wall"),
        InfantryScale(Farm)<1.f&&InfantryScale(Farm)>.8f&&ArmourScale(Farm)<InfantryScale(Farm)
        &&HitScale(Flat,Farm,FVector(0,0,100),FVector(1000,0,100),false)
          <HitScale(Flat,Farm,FVector(0,0,100),FVector(1000,0,100),true));
    const FVector Low(1000,0,110),High(0,0,310);
    Check(TEXT("v54_visible_ridge_gives_modest_elevation_not_global_accuracy"),
        HitScale(Ridge,Flat,High,Low,false)>1.f&&HitScale(Ridge,Flat,Low,High,false)==1.f
        &&HitScale(Flat,Flat,High,Low,false)==1.f);
    Check(TEXT("v54_low_cellar_and_canal_shelter_still_target_but_rail_shoulder_is_exposed"),
        HitScale(Flat,Canal,Low,Low,false)<HitScale(Flat,Canal,Low,Low,true)
        &&HitScale(Flat,Ruins,Low,Low,false)<HitScale(Flat,Ruins,Low,Low,true)
        &&HitScale(Flat,Canal,Low,Low,false)<1.f&&HitScale(Flat,Rail,Low,Low,false)==1.f);
    bool Bounded=true;
    for(int32 I=0;I<=10;++I)
    {
        FFrontlineTerrainFeaturesV54 Mixed;const float T=I*.1f;
        Mixed.MarshChannel=T;Mixed.CanalCut=1.f-T;Mixed.Ruins=T;Mixed.RailShoulder=1.f-T;Mixed.RailWash=T;
        Mixed.RidgeCrest=T;Mixed.WoodlandGully=1.f-T;Mixed.SnowDrift=1.f-T;Mixed.RuinCellar=T;
        Mixed.FieldDitch=T;Mixed.FieldBoundary=1.f-T;Mixed.TankChurn=T;
        const float Foot=InfantryScale(Mixed),Armour=ArmourScale(Mixed);
        const float Hit=HitScale(Mixed,Mixed,FVector(0,0,300),FVector(1000,0,100),false);
        Bounded&=FMath::IsFinite(Foot)&&Foot>=.72f&&Foot<=1.f&&FMath::IsFinite(Armour)&&Armour>=.55f&&Armour<=1.f
            &&FMath::IsFinite(Hit)&&Hit>=.78f&&Hit<=1.08f;
    }
    Check(TEXT("v54_all_mixed_landforms_remain_finite_and_bounded"),Bounded);
    bool Aligned=true;int32 AuthoredSamples=0,SlowedSamples=0;
    for(int32 IX=-6;IX<=6;++IX)for(int32 IY=-3;IY<=3;++IY)
    {
        const float X=FrontlineBattlefield::GoalHalfLength*IX/7.f,Y=1050.f*IY/3.f;
        const auto F=AWarfareFrontlineGround::QueryTerrainFeaturesV54(X,Y);
        const FVector P(X,Y,AWarfareFrontlineGround::HeightAt(X,Y));
        const float Foot=InfantryTravelScale(P),Tank=VehicleTravelScale(P);
        const float Mask=FMath::Max3(F.RidgeCrest,F.MarshChannel,F.CanalCut);
        const float Other=FMath::Max3(F.RailShoulder,F.RailWash,FMath::Max(F.Ruins,F.RuinCellar));
        const float FarmMask=FMath::Max3(F.FieldDitch,F.FieldBoundary,F.TankChurn);
        if(FMath::Max3(Mask,Other,FMath::Max3(F.SnowDrift,FarmMask,F.WoodlandGully))>.12f)
        {++AuthoredSamples;if(Foot<.99f||Tank<.99f)++SlowedSamples;}
        Aligned&=FMath::IsFinite(F.ReliefCm)&&FMath::IsNearlyEqual(Foot,InfantryScale(F),.00001f)
            &&FMath::IsNearlyEqual(Tank,ArmourScale(F),.00001f)
            &&FMath::IsNearlyEqual(DirectFireHitScale(P,P,false),HitScale(F,F,P,P,false),.00001f);
    }
    Check(TEXT("v54_actual_field_masks_feed_traversal_and_fire_consistently"),Aligned);
    Check(TEXT("v54_authored_landforms_change_actual_crossing_pace"),AuthoredSamples==0||SlowedSamples>0);
    bool WorksUnchanged=true;
    for(int32 I=0;I<FrontlineBattlefield::TrenchCount;++I)
    {
        const float X=FrontlineFieldV32::TrenchCentreX(I,0.f);
        const FVector P(X,0.f,AWarfareFrontlineGround::HeightAt(X,0.f));
        WorksUnchanged&=FMath::IsNearlyEqual(InfantryTravelScale(P),1.f,.0001f)
            &&FMath::IsNearlyEqual(VehicleTravelScale(P),1.f,.0001f)
            &&FMath::IsNearlyEqual(DirectFireHitScale(P,P,false),1.f,.0001f);
    }
    Check(TEXT("v54_occupied_trench_earthworks_keep_existing_pace_and_cover"),WorksUnchanged);
    {
        int32 PreviousOp=0;EFrontlineMissionTerrainV32 PreviousStyle;
        FrontlineFieldV32::TerrainProfileV43(PreviousOp,PreviousStyle);
        const float PreviousDepth=FrontlineBattlefield::OperationDepthScale;
        const int32 PreviousOperation=FrontlineBattlefield::OperationIndex;
        ON_SCOPE_EXIT
        {
            FrontlineFieldV32::TerrainProfileV43(PreviousOp,PreviousStyle,true);
            FrontlineBattlefield::ConfigureOperationDepth(PreviousDepth,PreviousOperation);
        };
        FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(17),17);
        int32 SelleOp=17;auto SelleStyle=EFrontlineMissionTerrainV32::RailCutting;
        FrontlineFieldV32::TerrainProfileV43(SelleOp,SelleStyle,true);
        const float Home=FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
        const auto Axis=[&](float Y){return -.18f*Home+.22f*Y+145.f*FMath::Sin(Y/770.f+1.7f);};
        const FVector WashPoint(Axis(0.f),0.f,0.f),DryPoint(Axis(410.f),410.f,0.f);
        const auto WetFeature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(WashPoint.X,WashPoint.Y);
        const auto DryFeature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(DryPoint.X,DryPoint.Y);
        Check(TEXT("v56_selle_squads_and_armour_can_choose_a_drier_crossing"),
            WetFeature.RailWash>.65f&&DryFeature.RailWash<.15f
            &&InfantryTravelScale(WashPoint)+.12f<InfantryTravelScale(DryPoint)
            &&VehicleTravelScale(WashPoint)+.20f<VehicleTravelScale(DryPoint));
        FrontlineBattlefield::ConfigureOperationDepth(FrontlineGameplayV33::OperationDepthScale(13),13);
        int32 VillersOp=13;auto VillersStyle=EFrontlineMissionTerrainV32::Village;
        FrontlineFieldV32::TerrainProfileV43(VillersOp,VillersStyle,true);
        const float VillersHome=FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1));
        const auto RoadY=[](float X){return 90.f+.085f*X+125.f*FMath::Sin(X/1590.f+.4f);};
        const float RoadX=.40f*VillersHome;
        const FVector RoadPoint(RoadX,RoadY(RoadX),0.f);
        const FVector RubblePoint(RoadX,RoadPoint.Y-930.f,0.f);
        const float CellarX=-.50f*VillersHome;
        const FVector CellarPoint(CellarX,RoadY(CellarX)+1280.f,0.f);
        const auto RoadFeature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(RoadPoint.X,RoadPoint.Y);
        const auto RubbleFeature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(RubblePoint.X,RubblePoint.Y);
        const auto CellarFeature=AWarfareFrontlineGround::QueryTerrainFeaturesV54(CellarPoint.X,CellarPoint.Y);
        Check(TEXT("v57_villers_road_favours_armour_while_ruins_shelter_infantry"),
            RoadFeature.VillageRoad>.70f&&RubbleFeature.Ruins>.35f&&CellarFeature.RuinCellar>.70f
            &&InfantryTravelScale(RoadPoint)>InfantryTravelScale(RubblePoint)+.035f
            &&VehicleTravelScale(RoadPoint)>VehicleTravelScale(RubblePoint)+.055f
            &&DirectFireHitScale(RoadPoint,CellarPoint,false)
                <DirectFireHitScale(CellarPoint,RoadPoint,false)-.09f);
    }
    UE_LOG(LogTemp,Display,TEXT("WF_V54_TERRAIN_TACTICS sampled_feature_cells=%d slowed_cells=%d"),AuthoredSamples,SlowedSamples);
}
