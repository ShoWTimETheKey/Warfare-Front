// Actual level surfaces clipped to the authored depressions. The old vertex
// water mask almost vanished after protection masks were multiplied together.
// These small offline meshes leave the walkable strip and ground contacts exact.
namespace
{
FString WaterPackage44(int32 Op,EFrontlineMissionTerrainV32 Style,float Scale,int32 VersionOverride=-1)
{
    const int32 Version=VersionOverride>0?VersionOverride:Op==12?58:Op==7?57:54;
    return FString::Printf(TEXT("/Game/Warfare/Field/V%d/Water/SM_Basins_%02d_T%d_D%d"),Version,Op,int32(Style),FMath::RoundToInt(Scale*1000000.f));
}
FString WaterShorePackageV83(const UStaticMesh* Source)
{
    if(!Source)return FString();
    const FString Package=Source->GetOutermost()->GetName();
    const FString Prefix=TEXT("/Game/Warfare/Field/V");
    if(!Package.StartsWith(Prefix))return FString();
    const int32 End=Package.Find(TEXT("/"),ESearchCase::CaseSensitive,ESearchDir::FromStart,Prefix.Len());
    if(End==INDEX_NONE)return FString();
    const FString Version=Package.Mid(Prefix.Len(),End-Prefix.Len());
    if(!Version.IsNumeric())return FString();
    return FString::Printf(TEXT("/Game/Warfare/Field/V83/Water/%s_S%s"),*Source->GetName(),*Version);
}
}

void AWarfareFrontlineGround::BindWaterV44()
{
    ActiveWaterPlanesV72.Reset();
    if(BasinWaterV44){BasinWaterV44->DestroyComponent();BasinWaterV44=nullptr;}
    // The response belongs to this displayed component, not just the analytic
    // pool query. Clear it even when every replacement mesh is unavailable.
    UFrontlineWaterResponseV51::Bind(this);
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    // RebuildTerrain chose this version from a complete terrain+water set (or
    // a matching runtime source). Never mix old water with a new riverbed.
    const bool bLysV58=Op==12&&GLysV58Height.Load();
    const bool bMeninV59=Op==7&&GMeninV59Height.Load();
    const bool bMeninV63=Op==7&&GMeninV63Water.Load();
    const FString Path=WaterPackage44(Op,Style,FrontlineBattlefield::OperationDepthScale,
        Op==12?(bLysV58?58:57):bMeninV63?63:bMeninV59?59:-1);
    UStaticMesh* Mesh=nullptr;
    auto TryAuthoredWater=[&](int32 Version)
    {
        const FString Candidate=WaterPackage44(Op,Style,FrontlineBattlefield::OperationDepthScale,Version);
        auto* CandidateMesh=LoadObject<UStaticMesh>(nullptr,*(Candidate+TEXT(".")+FPaths::GetBaseFilename(Candidate)));
        if(!CandidateMesh)return;
        auto* Data=Cast<UFrontlineWaterPlanesV72>(CandidateMesh->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass()));
        if(MatchingWaterPlanesV72(Data)){Mesh=CandidateMesh;ActiveWaterPlanesV72.Reset(Data);}
    };
    if(WaterOperationV72(Op)&&!FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline")))
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("WFV75VisualBaseline")))
            TryAuthoredWater(75);
        if(!Mesh)TryAuthoredWater(72);
    }
    if(!Mesh)Mesh=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+FPaths::GetBaseFilename(Path)));
    const bool bV58LysMesh=bLysV58&&Mesh!=nullptr;
    if(!Mesh)return;
    // Keep the shoreline and optical response paired to this exact height
    // source. Historical comparison flags retain their original material.
    UMaterialInterface* ShoreMaterialV83=nullptr;
    const bool WaterBaselineV83=FParse::Param(FCommandLine::Get(),TEXT("WFV83WaterBaseline"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV75VisualBaseline"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV67WaterCandidate"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV65Baseline"))
        ||FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline"));
    const auto* WaterBattleV83=GetWorld()?Cast<AWarfareFrontlineGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
    if(!WaterBaselineV83&&!WarfareFRRUCampaignV82::Active(WaterBattleV83))
    {
        const FString Candidate=WaterShorePackageV83(Mesh);
        auto* ShoreMesh=Candidate.IsEmpty()?nullptr:LoadObject<UStaticMesh>(nullptr,*(Candidate+TEXT(".")+FPaths::GetBaseFilename(Candidate)));
        auto* ShoreMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V83/Materials/MI_BasinWater_V83.MI_BasinWater_V83"));
        if(ShoreMesh&&ShoreMaterial&&ShoreMesh->GetBounds().Origin.Equals(Mesh->GetBounds().Origin,.01f)
            &&ShoreMesh->GetBounds().BoxExtent.Equals(Mesh->GetBounds().BoxExtent,.01f))
        {
            const auto* OldRender=Mesh->GetRenderData();const auto* NewRender=ShoreMesh->GetRenderData();
            auto* Data=Cast<UFrontlineWaterPlanesV72>(ShoreMesh->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass()));
            if(OldRender&&NewRender&&!OldRender->LODResources.IsEmpty()&&!NewRender->LODResources.IsEmpty()
                &&OldRender->LODResources[0].GetNumTriangles()==NewRender->LODResources[0].GetNumTriangles()
                &&(!ActiveWaterPlanesV72.IsValid()||MatchingWaterPlanesV72(Data)))
            {Mesh=ShoreMesh;ShoreMaterialV83=ShoreMaterial;if(Data)ActiveWaterPlanesV72.Reset(Data);}
        }
    }
    BasinWaterV44=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(BasinWaterV44);BasinWaterV44->SetupAttachment(Surface);
    BasinWaterV44->SetMobility(EComponentMobility::Movable);BasinWaterV44->SetStaticMesh(Mesh);
    BasinWaterV44->SetCollisionEnabled(ECollisionEnabled::NoCollision);BasinWaterV44->SetCanEverAffectNavigation(false);
    BasinWaterV44->SetCastShadow(false);BasinWaterV44->SetReceivesDecals(false);BasinWaterV44->RegisterComponent();
    // Preserve the existing last-success-wins precedence without loading every
    // superseded graph. Missing paired V83 assets retain the original fallback.
    UMaterialInterface* Material=ShoreMaterialV83;
    if(!Material&&FParse::Param(FCommandLine::Get(),TEXT("WFV67WaterCandidate")))
        Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V67/Materials/MI_BasinWater_V67.MI_BasinWater_V67"));
    if(!Material&&!FParse::Param(FCommandLine::Get(),TEXT("WFV75VisualBaseline")))
        Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V75/Materials/MI_BasinWater_V75.MI_BasinWater_V75"));
    if(!Material&&!FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline")))
        Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V72/Materials/MI_BasinWater_V72.MI_BasinWater_V72"));
    if(!Material&&!FParse::Param(FCommandLine::Get(),TEXT("WFV65Baseline")))
        Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V65/Materials/MI_BasinWater_V65.MI_BasinWater_V65"));
    if(!Material&&!FParse::Param(FCommandLine::Get(),TEXT("WFV64Baseline")))
        Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V64/Materials/MI_BasinWater_V64.MI_BasinWater_V64"));
    if(!Material)Material=bV58LysMesh?LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Warfare/Field/V58/Materials/MI_BasinWaterLys_V58.MI_BasinWaterLys_V58")):nullptr;
    if(!Material&&(bMeninV63||(bMeninV59&&FParse::Param(FCommandLine::Get(),TEXT("WFV59MeninWater")))))
        Material=LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Warfare/Field/V59/Materials/MI_BasinWaterMenin_V59.MI_BasinWaterMenin_V59"));
    if(!Material&&Op==12)Material=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Warfare/Field/V57/Materials/MI_BasinWaterLys_V57.MI_BasinWaterLys_V57"));
    if(!Material)Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V53/Materials/MI_BasinWater_V53.MI_BasinWater_V53"));
    if(!Material)Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V51/Materials/MI_BasinWater_V51.MI_BasinWater_V51"));
    if(Material)BasinWaterV44->SetMaterial(0,Material);
    UFrontlineWaterResponseV51::Bind(this);
}

FString AWarfareFrontlineGround::BakeWaterProfileV44(int32 OperationIndex)
{
    auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),false);
#if WITH_EDITOR
    const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    if(!Missions.IsValidIndex(OperationIndex))return TerrainJSON43(Report);
    const auto& Mission=Missions[OperationIndex];int32 OldOp;EFrontlineMissionTerrainV32 OldStyle;
    FrontlineFieldV32::TerrainProfileV43(OldOp,OldStyle);
    const float OldScale=FrontlineBattlefield::OperationDepthScale;
    const bool OldLysVersion=GLysV58Height.Load();
    const bool OldMeninV58=GMeninV58Height.Load();
    const bool OldMeninV59=GMeninV59Height.Load();
    const bool OldMeninV63=GMeninV63Water.Load();
    TStrongObjectPtr<UFrontlineWaterPlanesV72> PreviousPlanes(ActiveWaterPlanesV72.Get());
    ON_SCOPE_EXIT{FrontlineFieldV32::TerrainProfileV43(OldOp,OldStyle,true);FrontlineBattlefield::ConfigureOperationDepth(OldScale,OldOp);GLysV58Height.Store(OldLysVersion);GMeninV58Height.Store(OldMeninV58);GMeninV59Height.Store(OldMeninV59);GMeninV63Water.Store(OldMeninV63);ActiveWaterPlanesV72.Reset(PreviousPlanes.Get());};
    int32 Op=OperationIndex;auto Style=Mission.TerrainStyleV32;FrontlineFieldV32::TerrainProfileV43(Op,Style,true);
    const float Scale=FrontlineGameplayV33::OperationDepthScale(Op);FrontlineBattlefield::ConfigureOperationDepth(Scale,Op);
    // Versioned water and terrain bakes must share the same Lys height field,
    // even when the current runtime is showing accepted V57 fallback tiles.
    if(Op==12)GLysV58Height.Store(true);
    const bool bMeninV59Bake=Op==7&&FParse::Param(FCommandLine::Get(),TEXT("WFV59WaterBake"));
    const bool bMeninV63Bake=Op==7&&FParse::Param(FCommandLine::Get(),TEXT("WFV63MeninWaterBake"));
    if(bMeninV59Bake||bMeninV63Bake){GMeninV58Height.Store(true);GMeninV59Height.Store(true);}
    GMeninV63Water.Store(bMeninV63Bake);
    FMeshDescription D;FStaticMeshAttributes A(D);A.Register();A.GetVertexInstanceUVs().SetNumChannels(1);
    const auto Group=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("BasinWater");
    double Area=0,RegionalArea=0;int32 Basins=0,RegionalTriangles=0;
    float MinClearance=MAX_flt,MaximumBoundaryDepth=0,MinimumBankClearance=MAX_flt,MinimumRailClearance=MAX_flt;
    const auto& Field=BasinFieldV47();
    const bool bWaterV72Bake=WaterOperationV72(Op)&&FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBake"));
    ActiveWaterPlanesV72.Reset();
    if(bWaterV72Bake)
    {
        ActiveWaterPlanesV72.Reset(SolveRegionalWaterV72(Op,Style,Field));
        if(!MatchingWaterPlanesV72(ActiveWaterPlanesV72.Get()))return TerrainJSON43(Report);
    }
    TSet<FIntVector> ProcessedCells;TSet<int32> ActiveGroups;
    int32 ShoreVertices=0,ProtectedBankVertices=0;float MaximumWaterDepth=0;
    for(const auto& Basin:Field.Basins)
    {
        if(!Basin.bWetGroup)continue;
        const auto& C=Basin.Shape;const float Level=Basin.Level;
        TArray<int32,TInlineAllocator<12>> Members;
        for(int32 I=0;I<Field.Basins.Num();++I)if(Field.Basins[I].Group==Basin.Group)Members.Add(I);
        auto Protection=[&](const FVector2f& P)
        {
            float V=-MAX_flt;
            // One union contour for all connected lobes, sharing the same
            // cached elevation used by ground G. No overlapping water sheets.
            for(int32 I:Members)V=FMath::Max(V,BasinProtectionV47(Field.Basins[I],P,Field.bRail));
            return V;
        };
        const int32 Before=D.Triangles().Num();constexpr float Step=BasinGridV47;
        const float R=FMath::Max(C.RX,C.RY)*1.12f;
        auto Margin=[&](const FVector2f& P){return FMath::Min(Level-HeightAt(P.X,P.Y),Protection(P));};
        auto Triangle=[&](FVector2f P0,FVector2f P1,FVector2f P2)
        {
            TArray<FVector2f,TInlineAllocator<5>> In{P0,P1,P2},Out;
            // Clip to the true terrain intersection instead of an alpha card.
            for(int32 I=0;I<3;++I)
            {
                const auto P=In[I],Q=In[(I+1)%3];const float H=Margin(P),K=Margin(Q);
                if(H>=0)Out.Add(P);
                if((H>=0)!=(K>=0))
                {
                    auto Lo=P,Hi=Q;bool Positive=H>=0;
                    for(int32 N=0;N<9;++N){const auto Mid=(Lo+Hi)*.5f;if((Margin(Mid)>=0)==Positive)Lo=Mid;else Hi=Mid;}
                    Out.Add((Lo+Hi)*.5f);
                }
            }
            if(Out.Num()<3)return;
            for(int32 I=1;I+1<Out.Num();++I)
            {
                const FVector2f Ps[]={Out[0],Out[I],Out[I+1]};TArray<FVertexInstanceID,TInlineAllocator<3>> Corners;
                const float TriArea=FMath::Abs(FVector2f::CrossProduct(Ps[1]-Ps[0],Ps[2]-Ps[0]))*.5f;
                if(TriArea<.01f)continue;Area+=TriArea;
                for(auto P:Ps)
                {
                    const float Depth=FMath::Max(0.f,Level-HeightAt(P.X,P.Y));
                    MaximumWaterDepth=FMath::Max(MaximumWaterDepth,Depth);ShoreVertices+=Depth<2.f;
                    MinClearance=FMath::Min(MinClearance,FMath::Abs(P.Y));
                    if(Protection(P)<1.f)MaximumBoundaryDepth=FMath::Max(MaximumBoundaryDepth,Depth);
                    if(FMath::Abs(P.Y)<=BankSupportHalfExtentV47)
                    {
                        ++ProtectedBankVertices;
                        for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
                            MinimumBankClearance=FMath::Min(MinimumBankClearance,FMath::Abs(P.X-FrontlineFieldV32::TrenchCentreX(T,P.Y)));
                    }
                    if(Mission.bArmoredTrain)MinimumRailClearance=FMath::Min(MinimumRailClearance,FMath::Abs(P.Y+1130.f));
                    const auto V=D.CreateVertex();A.GetVertexPositions()[V]=FVector3f(P.X,P.Y,Level+1.2f);
                    const auto VI=D.CreateVertexInstance(V);A.GetVertexInstanceNormals()[VI]=FVector3f(0,0,1);
                    A.GetVertexInstanceTangents()[VI]=FVector3f(1,0,0);A.GetVertexInstanceBinormalSigns()[VI]=1;
                    A.GetVertexInstanceUVs().Set(VI,0,FVector2f(P.X/145.f,P.Y/145.f));
                    A.GetVertexInstanceColors()[VI]=FVector4f(FMath::Clamp(Depth/12.f,0.f,1.f),Basin.Group/float(Field.Basins.Num()),0,1);Corners.Add(VI);
                }
                D.CreateTriangle(Group,Corners);
            }
        };
        const int32 X0=FMath::FloorToInt((C.X-R)/Step),X1=FMath::CeilToInt((C.X+R)/Step);
        const int32 Y0=FMath::FloorToInt((C.Y-R)/Step),Y1=FMath::CeilToInt((C.Y+R)/Step);
        for(int32 IY=Y0;IY<Y1;++IY)for(int32 IX=X0;IX<X1;++IX)
        {
            const FIntVector Key(IX,IY,Basin.Group);if(ProcessedCells.Contains(Key))continue;ProcessedCells.Add(Key);
            const float X=IX*Step,Y=IY*Step;
            Triangle({X,Y},{X,Y+Step},{X+Step,Y});Triangle({X+Step,Y},{X,Y+Step},{X+Step,Y+Step});
        }
        if(D.Triangles().Num()>Before){++Basins;ActiveGroups.Add(Basin.Group);}
    }
    if(Style==EFrontlineMissionTerrainV32::Marsh||Style==EFrontlineMissionTerrainV32::Canal||bMeninV63Bake)
    {
        // The photographed floodplain and St Quentin cut need a visible
        // physical wet bed at the wide side camera. Clip a sparse, unshadowed
        // ribbon to the very same analytic shore/depth query used at runtime.
        // V47 basin footprints and trench supports are excluded by that query.
        // The op12 shore changes direction over metre-scale collapsed mud
        // ledges. A 42 cm offline grid resolves those contours without a new
        // draw call or masked-water shader; all older waters keep 58 cm.
        const float Step=bWaterV72Bake?40.f:bMeninV63Bake?30.f:Op==12?42.f:58.f;
        const float XMin=bMeninV63Bake?-3850.f:Style==EFrontlineMissionTerrainV32::Marsh?
            -FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)))-3300.f:Op==16?-3500.f:-2600.f;
        const float XMax=bMeninV63Bake?3850.f:Style==EFrontlineMissionTerrainV32::Marsh?
            FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)))+3300.f:Op==16?6500.f:6200.f;
        // Op16's photographed canal now has actual rear terrain behind the
        // matte. Continue its physical water to the same cut's far end so the
        // transparency transition cannot expose an inexplicably dry bed.
        const float YMin=bMeninV63Bake?-3850.f:Style==EFrontlineMissionTerrainV32::Marsh?(Op==12?-7010.f:-3800.f):Op==16?-7000.f:-5350.f;
        const float YMax=bMeninV63Bake?3850.f:Style==EFrontlineMissionTerrainV32::Marsh?(Op==12?2650.f:1500.f):3000.f;
        auto Sample=[&](FVector2f P){return RegionalWaterV54(P.X,P.Y,Field);};
        auto Triangle=[&](FVector2f P0,FVector2f P1,FVector2f P2)
        {
            TArray<FVector2f,TInlineAllocator<5>> In{P0,P1,P2},Out;
            for(int32 I=0;I<3;++I)
            {
                const auto P=In[I],Q=In[(I+1)%3];
                const float FirstMargin=Sample(P).Margin,SecondMargin=Sample(Q).Margin;
                if(FirstMargin>=0.f)Out.Add(P);
                if((FirstMargin>=0.f)!=(SecondMargin>=0.f))
                {
                    auto Lo=P,Hi=Q;const bool Positive=FirstMargin>=0.f;
                    for(int32 N=0;N<9;++N){const auto Mid=(Lo+Hi)*.5f;if((Sample(Mid).Margin>=0.f)==Positive)Lo=Mid;else Hi=Mid;}
                    // Keep the accepted side: a rejected regional sample has
                    // no valid level, and a midpoint could create a Z spike.
                    Out.Add(Positive?Lo:Hi);
                }
            }
            if(Out.Num()<3)return;
            if(bWaterV72Bake)
            {
                // Disconnected pools must not acquire a sloping triangle at
                // a coarse contour cell. Their dry separator stays exposed.
                const auto* Data=ActiveWaterPlanesV72.Get();int32 Plane=INDEX_NONE;
                for(const auto& P:Out)
                {
                    const int32 ThisPlane=RegionalGroupV72(Data,P.X,P.Y);
                    if(!Data->Levels.IsValidIndex(ThisPlane)|| (Plane!=INDEX_NONE&&Plane!=ThisPlane))return;
                    Plane=ThisPlane;
                }
            }
            for(int32 I=1;I+1<Out.Num();++I)
            {
                const FVector2f Ps[]={Out[0],Out[I],Out[I+1]};
                const float TriArea=FMath::Abs(FVector2f::CrossProduct(Ps[1]-Ps[0],Ps[2]-Ps[0]))*.5f;
                if(TriArea<.01f)continue;
                Area+=TriArea;RegionalArea+=TriArea;++RegionalTriangles;
                TArray<FVertexInstanceID,TInlineAllocator<3>> Corners;
                for(const auto P:Ps)
                {
                    const auto W=Sample(P);
                    MaximumWaterDepth=FMath::Max(MaximumWaterDepth,W.Depth);
                    ShoreVertices+=W.Depth<2.f;
                    MinClearance=FMath::Min(MinClearance,FMath::Abs(P.Y));
                    if(FMath::Abs(P.Y)<=BankSupportHalfExtentV47)
                    {
                        ++ProtectedBankVertices;
                        for(int32 T=0;T<FrontlineBattlefield::TrenchCount;++T)
                            MinimumBankClearance=FMath::Min(MinimumBankClearance,FMath::Abs(P.X-FrontlineFieldV32::TrenchCentreX(T,P.Y)));
                    }
                    const auto V=D.CreateVertex();A.GetVertexPositions()[V]=FVector3f(P.X,P.Y,W.Level+1.2f);
                    const auto VI=D.CreateVertexInstance(V);
                    const bool Flowing=bWaterV72Bake&&FlowingRegionalGroupV72(ActiveWaterPlanesV72.Get(),RegionalGroupV72(ActiveWaterPlanesV72.Get(),P.X,P.Y));
                    const FVector3f Normal=Flowing?RegionalSurfaceNormalV72(P.X,P.Y):FVector3f(0,0,1);
                    A.GetVertexInstanceNormals()[VI]=Normal;
                    A.GetVertexInstanceTangents()[VI]=FVector3f::CrossProduct(FVector3f(0,1,0),Normal).GetSafeNormal();A.GetVertexInstanceBinormalSigns()[VI]=1;
                    A.GetVertexInstanceUVs().Set(VI,0,FVector2f(P.X/145.f,P.Y/145.f));
                    // B distinguishes these broad, silty shallows from old
                    // isolated basins. R retains 0–80 cm depth variation;
                    // 12 cm encoding made every new channel uniformly deep.
                    float Colour=.97f;
                    if(bWaterV72Bake)
                    {
                        const auto* Data=ActiveWaterPlanesV72.Get();const int32 HydraulicGroup=RegionalGroupV72(Data,P.X,P.Y);
                        if(Data->GroupColours.IsValidIndex(HydraulicGroup))
                            Colour=FLinearColor(FColor(0,uint8(Data->GroupColours[HydraulicGroup]),0,255)).G;
                    }
                    A.GetVertexInstanceColors()[VI]=FVector4f(FMath::Clamp(W.Depth/80.f,0.f,1.f),Colour,1,1);
                    Corners.Add(VI);
                }
                D.CreateTriangle(Group,Corners);
            }
        };
        for(float Y=FMath::FloorToFloat(YMin/Step)*Step;Y<YMax;Y+=Step)
            for(float X=FMath::FloorToFloat(XMin/Step)*Step;X<XMax;X+=Step)
            {
                const FVector2f P00(X,Y),P01(X,Y+Step),P10(X+Step,Y),P11(X+Step,Y+Step);
                // Only a tiny fraction of this rectangular grid is water;
                // skip dry cells before paying for edge clipping.
                if(Sample(P00).Margin<0&&Sample(P01).Margin<0&&Sample(P10).Margin<0&&Sample(P11).Margin<0)continue;
                Triangle(P00,P01,P10);Triangle(P10,P01,P11);
            }
    }
    if(D.Triangles().Num()==0)return TerrainJSON43(Report);
    // Keep the cooked mesh's material dependency identical to the V53
    // runtime binding. Vertex B selects regional silty water, while the old
    // V47/V51 pools retain their accepted shader path with B=0.
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V53/Materials/MI_BasinWater_V53.MI_BasinWater_V53"));
    if(!Mat)Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V51/Materials/MI_BasinWater_V51.MI_BasinWater_V51"));
    if(!Mat)return TerrainJSON43(Report);
    const FString Path=WaterPackage44(Op,Style,Scale,bWaterV72Bake?72:bMeninV63Bake?63:bMeninV59Bake?59:-1);auto* Package=CreatePackage(*Path);Package->FullyLoad();
    auto* Mesh=FindObject<UStaticMesh>(Package,*FPaths::GetBaseFilename(Path));
    if(!Mesh)Mesh=NewObject<UStaticMesh>(Package,*FPaths::GetBaseFilename(Path),RF_Public|RF_Standalone);
    if(bWaterV72Bake)
    {
        Mesh->RemoveUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass());
        Mesh->AddAssetUserData(DuplicateObject<UFrontlineWaterPlanesV72>(ActiveWaterPlanesV72.Get(),Mesh,TEXT("HorizontalWaterPlanesV72")));
    }
    Mesh->GetStaticMaterials().Reset();Mesh->GetStaticMaterials().Add(FStaticMaterial(Mat,TEXT("BasinWater")));
    Mesh->SetNumSourceModels(1);auto& BS=Mesh->GetSourceModel(0).BuildSettings;BS.bRecomputeNormals=false;BS.bRecomputeTangents=false;
    if(bWaterV72Bake)Mesh->bAllowCPUAccess=true;
    BS.bUseFullPrecisionUVs=true;BS.bUseHighPrecisionTangentBasis=true;BS.bGenerateLightmapUVs=false;BS.MaxLumenMeshCards=24;
    BS.bGenerateDistanceFieldAsIfTwoSided=true;Mesh->GetNaniteSettings().bEnabled=false;
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=false;Params.bCommitMeshDescription=true;
    Params.bBuildSimpleCollision=false;Params.bMarkPackageDirty=true;TArray<const FMeshDescription*> Descs{&D};
    bool Good=Mesh->BuildFromMeshDescriptions(Descs,Params);FAssetCompilingManager::Get().FinishAllCompilation();
    if(GDistanceFieldAsyncQueue)GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
    if(GCardRepresentationAsyncQueue)GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete();
    FAssetRegistryModule::AssetCreated(Mesh);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
    Good&=FMath::IsFinite(MinClearance)&&ProtectedBankVertices>0&&MinimumBankClearance>=369.9f&&MaximumBoundaryDepth<.25f;
    if(Style==EFrontlineMissionTerrainV32::Marsh)
        Good&=RegionalArea>900000.f&&RegionalTriangles>100;
    if(Style==EFrontlineMissionTerrainV32::Canal)
        // The intact distant canal is on the backdrop; these are deliberately
        // intermittent water pockets in the playable damaged cutting. Keep a
        // substantive presence gate without forcing a grey foreground sheet.
        Good&=RegionalArea>400000.f&&RegionalTriangles>500;
    if(Mission.bArmoredTrain)Good&=MinimumRailClearance>=299.9f;
    // A partial or rejected new bake must never activate a new query beside
    // the accepted mesh. Persist its admission bit only after geometry gates.
    if(bWaterV72Bake)
        if(auto* SavedPlanes=Cast<UFrontlineWaterPlanesV72>(Mesh->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass())))SavedPlanes->bValid=Good;
    Good&=UPackage::SavePackage(Package,Mesh,*FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension()),Args);
    Report->SetBoolField(TEXT("success"),Good);Report->SetStringField(TEXT("asset"),Path);
    Report->SetNumberField(TEXT("area_square_metres"),Area/10000);Report->SetNumberField(TEXT("basins"),Basins);
    Report->SetNumberField(TEXT("regional_water_area_square_metres"),RegionalArea/10000);
    Report->SetNumberField(TEXT("regional_water_triangles"),RegionalTriangles);
    Report->SetNumberField(TEXT("connected_basin_groups"),ActiveGroups.Num());Report->SetNumberField(TEXT("shore_vertex_instances"),ShoreVertices);
    Report->SetNumberField(TEXT("maximum_depth_cm"),MaximumWaterDepth);Report->SetNumberField(TEXT("unique_grid_cells"),ProcessedCells.Num());
    Report->SetObjectField(TEXT("basin_field_v47"),BasinFieldReportV47());
    Report->SetNumberField(TEXT("minimum_frontage_clearance_cm"),MinClearance);Report->SetNumberField(TEXT("triangles"),D.Triangles().Num());
    if(bWaterV72Bake)
    {
        const auto* Data=ActiveWaterPlanesV72.Get();
        Report->SetNumberField(TEXT("regional_horizontal_groups"),Data->Levels.Num());
        Report->SetNumberField(TEXT("legacy_wet_cells"),Data->WetCells);
        Report->SetNumberField(TEXT("retained_wet_cells"),Data->RetainedCells);
        Report->SetNumberField(TEXT("sampled_spill_boundaries"),Data->SpillBoundarySamples);
        Report->SetNumberField(TEXT("physical_bed_rim_crossings"),Data->PhysicalSpillSamples);
        Report->SetNumberField(TEXT("excluded_submerged_mask_boundaries"),Data->ClippedSubmergedBoundaries);
        TArray<TSharedPtr<FJsonValue>> Planes;
        for(int32 I=0;I<Data->Levels.Num();++I)
        {
            auto Plane=MakeShared<FJsonObject>();Plane->SetNumberField(TEXT("group"),I);
            Plane->SetNumberField(TEXT("level_cm"),Data->Levels[I]);Plane->SetNumberField(TEXT("legacy_mean_cm"),Data->LegacyMeanLevels[I]);
            Plane->SetNumberField(TEXT("spill_limit_cm"),Data->SpillLevels[I]);Planes.Add(MakeShared<FJsonValueObject>(Plane));
            Plane->SetStringField(TEXT("surface_kind"),FlowingRegionalGroupV72(Data,I)?TEXT("flowing_reach"):TEXT("standing_pool"));
        }
        Report->SetArrayField(TEXT("regional_horizontal_planes"),Planes);
        int32 FlowingGroups=0;for(int32 Kind:Data->GroupKinds)FlowingGroups+=Kind==1;
        Report->SetNumberField(TEXT("standing_regional_groups"),Data->Levels.Num()-FlowingGroups);
        Report->SetNumberField(TEXT("flowing_regional_groups"),FlowingGroups);
        Report->SetBoolField(TEXT("lys_static_pool_and_flow_composition"),Data->LysComposition==1);
        Report->SetNumberField(TEXT("hydraulic_grid_cm"),Data->CellSize);
        Report->SetBoolField(TEXT("terrain_height_and_collision_unchanged"),true);
        Report->SetBoolField(TEXT("query_labels_embedded_in_water_mesh"),true);
    }
    Report->SetNumberField(TEXT("minimum_bank_clearance_cm"),MinimumBankClearance);Report->SetNumberField(TEXT("maximum_protection_edge_depth_cm"),MaximumBoundaryDepth);
    Report->SetNumberField(TEXT("protected_bank_vertex_instances"),ProtectedBankVertices);
    Report->SetNumberField(TEXT("bank_support_half_extent_cm"),BankSupportHalfExtentV47);
    Report->SetBoolField(TEXT("protected_trench_clearance_gate"),FMath::IsFinite(MinClearance)&&ProtectedBankVertices>0&&MinimumBankClearance>=369.9f&&MaximumBoundaryDepth<.25f);
    Report->SetBoolField(TEXT("regional_presence_gate"),Style==EFrontlineMissionTerrainV32::Marsh?RegionalArea>900000.f&&RegionalTriangles>100:
        Style==EFrontlineMissionTerrainV32::Canal?RegionalArea>400000.f&&RegionalTriangles>500:true);
    if(Mission.bArmoredTrain)Report->SetNumberField(TEXT("minimum_rail_clearance_cm"),MinimumRailClearance);
#endif
    return TerrainJSON43(Report);
}
