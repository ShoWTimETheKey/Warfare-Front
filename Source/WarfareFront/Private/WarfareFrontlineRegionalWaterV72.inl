// Included beside the legacy regional query: no terrain height or collision changes.
TStrongObjectPtr<UFrontlineWaterPlanesV72> ActiveWaterPlanesV72;
bool WaterOperationV72(int32 Op){return Op==5||Op==6||Op==12||Op==16||Op==19;}
int32 WaterGeometryV72(int32 Op){return Op==12&&GLysV58Height.Load()?1:0;}
bool MatchingWaterPlanesV72(const UFrontlineWaterPlanesV72* Data)
{
    int32 Op=0;EFrontlineMissionTerrainV32 Style;FrontlineFieldV32::TerrainProfileV43(Op,Style);
    return Data&&Data->bValid&&Data->Schema==72&&WaterOperationV72(Op)&&Data->Operation==Op
        &&Data->Style==int32(Style)&&Data->DepthKey==FMath::RoundToInt(FrontlineBattlefield::OperationDepthScale*1000000.f)
        &&Data->GeometryVersion==WaterGeometryV72(Op)&&Data->CellSize>0
        &&Data->Columns>0&&Data->Rows>0&&Data->CellGroups.Num()==Data->Columns*Data->Rows
        &&Data->Levels.Num()>0&&Data->GroupColours.Num()==Data->Levels.Num()
        &&Data->LegacyMeanLevels.Num()==Data->Levels.Num()&&Data->SpillLevels.Num()==Data->Levels.Num()
        &&(Data->GroupKinds.IsEmpty()||Data->GroupKinds.Num()==Data->Levels.Num())
        &&(Op!=12||Data->LysComposition==1);
}
bool FlowingRegionalGroupV72(const UFrontlineWaterPlanesV72* Data,int32 Group)
{return Data&&Data->Operation==12&&Data->GroupKinds.IsValidIndex(Group)&&Data->GroupKinds[Group]==1;}
float RegionalReferenceLevelV72(float X,float Y)
{
    // Same accepted Lys height formula, independent of a wet/dry mask. It
    // can be differentiated at a clipped shore without sampling default Z=0.
    const float Broad=25.f+26.f*FMath::Sin(X/1550.f)*FMath::Cos(Y/1250.f)
        +Ease((-Y-1900.f)/3100.f)*(80.f+42.f*FMath::Sin(X/1900.f)+28.f*FMath::Cos(X/780.f));
    return Broad+FrontlineFieldV32::TerrainLift(X,Y,Broad)-60.f;
}
FVector3f RegionalSurfaceNormalV72(float X,float Y)
{
    constexpr float Step=8.f;
    const float DX=(RegionalReferenceLevelV72(X+Step,Y)-RegionalReferenceLevelV72(X-Step,Y))/(2*Step);
    const float DY=(RegionalReferenceLevelV72(X,Y+Step)-RegionalReferenceLevelV72(X,Y-Step))/(2*Step);
    return FVector3f(-DX,-DY,1.f).GetSafeNormal();
}
int32 RegionalGroupV72(const UFrontlineWaterPlanesV72* Data,float X,float Y)
{
    if(!MatchingWaterPlanesV72(Data))return INDEX_NONE;
    const int32 IX=FMath::FloorToInt((X-Data->Origin.X)/Data->CellSize);
    const int32 IY=FMath::FloorToInt((Y-Data->Origin.Y)/Data->CellSize);
    if(IX<0||IY<0||IX>=Data->Columns||IY>=Data->Rows)return INDEX_NONE;
    return Data->CellGroups[IY*Data->Columns+IX];
}
FRegionalWaterV54 RegionalWaterV54(float X,float Y,const FBasinFieldV47& Field,float GroundSlopeForShore=-1.f)
{
    auto R=LegacyRegionalWaterV54(X,Y,Field,GroundSlopeForShore);
    if(FParse::Param(FCommandLine::Get(),TEXT("WFV72WaterBaseline")))return R;
    const auto* Data=ActiveWaterPlanesV72.Get();
    if(!MatchingWaterPlanesV72(Data))return R;
    const int32 Group=RegionalGroupV72(Data,X,Y);
    if(!Data->Levels.IsValidIndex(Group))return FRegionalWaterV54();
    // Flowing Lys reaches retain their actual longitudinal fall. Their baked
    // basis follows this surface; their standing tributary pools do not slope.
    if(FlowingRegionalGroupV72(Data,Group))return R;
    // Never add a new lake outside the accepted wet footprint. Reclip the
    // existing footprint to a shared horizontal surface, above its actual bed.
    R.Level=Data->Levels[Group];
    R.Depth=R.Level+1.2f-AWarfareFrontlineGround::HeightAt(X,Y);
    const float DepthMargin=R.Depth-.35f;
    R.Margin=FMath::Min(R.Margin,DepthMargin);
    if(GroundSlopeForShore>=0.f)
        R.SignedShoreCm=FMath::Min(R.SignedShoreCm,DepthMargin/FMath::Max(.06f,GroundSlopeForShore));
    return R;
}

#if WITH_EDITOR
UFrontlineWaterPlanesV72* SolveRegionalWaterV72(int32 Op,EFrontlineMissionTerrainV32 Style,const FBasinFieldV47& Field)
{
    auto* Data=NewObject<UFrontlineWaterPlanesV72>(GetTransientPackage());
    Data->Operation=Op;Data->Style=int32(Style);Data->GeometryVersion=WaterGeometryV72(Op);
    Data->DepthKey=FMath::RoundToInt(FrontlineBattlefield::OperationDepthScale*1000000.f);
    const float Home=FMath::Max(1900.f,FMath::Abs(FrontlineBattlefield::TrenchX(FrontlineBattlefield::TrenchCount-1)));
    const float XMin=Style==EFrontlineMissionTerrainV32::Marsh?-Home-3300.f:Op==16?-3500.f:-2600.f;
    const float XMax=Style==EFrontlineMissionTerrainV32::Marsh?Home+3300.f:Op==16?6500.f:6200.f;
    const float YMin=Style==EFrontlineMissionTerrainV32::Marsh?(Op==12?-7010.f:-3800.f):Op==16?-7000.f:-5350.f;
    const float YMax=Style==EFrontlineMissionTerrainV32::Marsh?(Op==12?2650.f:1500.f):Op==16?2400.f:3000.f;
    const float Step=Data->CellSize;
    Data->Origin=FVector2D(FMath::FloorToFloat(XMin/Step)*Step,FMath::FloorToFloat(YMin/Step)*Step);
    Data->Columns=FMath::CeilToInt((XMax-Data->Origin.X)/Step)+1;
    Data->Rows=FMath::CeilToInt((YMax-Data->Origin.Y)/Step)+1;
    const int32 Count=Data->Columns*Data->Rows,VColumns=Data->Columns+1;
    Data->CellGroups.Init(INDEX_NONE,Count);
    TArray<uint8> VertexWet;VertexWet.Init(0,VColumns*(Data->Rows+1));
    for(int32 Y=0;Y<=Data->Rows;++Y)for(int32 X=0;X<=Data->Columns;++X)
        VertexWet[Y*VColumns+X]=LegacyRegionalWaterV54(Data->Origin.X+X*Step,Data->Origin.Y+Y*Step,Field).Margin>=0.f;
    TArray<uint8> Candidate;Candidate.Init(0,Count);
    TArray<float> ReferenceLevel;ReferenceLevel.Init(0.f,Count);
    auto Centre=[&](int32 Cell){return FVector2D(Data->Origin.X+(Cell%Data->Columns+.5f)*Step,
        Data->Origin.Y+(Cell/Data->Columns+.5f)*Step);};
    for(int32 Y=0;Y<Data->Rows;++Y)for(int32 X=0;X<Data->Columns;++X)
    {
        const int32 I=Y*Data->Columns+X,V=Y*VColumns+X;
        const auto P=Centre(I);auto Wet=LegacyRegionalWaterV54(P.X,P.Y,Field);
        Candidate[I]=Wet.Margin>=0||VertexWet[V]||VertexWet[V+1]||VertexWet[V+VColumns]||VertexWet[V+VColumns+1];
        if(!Candidate[I])continue;
        if(Wet.Margin<0)
        {
            const FVector2D Corners[]={{Data->Origin.X+X*Step,Data->Origin.Y+Y*Step},
                {Data->Origin.X+(X+1)*Step,Data->Origin.Y+Y*Step},
                {Data->Origin.X+X*Step,Data->Origin.Y+(Y+1)*Step},
                {Data->Origin.X+(X+1)*Step,Data->Origin.Y+(Y+1)*Step}};
            for(const auto& Q:Corners){const auto W=LegacyRegionalWaterV54(Q.X,Q.Y,Field);if(W.Margin>=0){Wet=W;break;}}
        }
        ReferenceLevel[I]=Wet.Level;++Data->WetCells;
    }
    const FIntPoint Directions[]={{-1,0},{1,0},{0,-1},{0,1}};
    TArray<int32> Queue;
    for(int32 Seed=0;Seed<Count;++Seed)
    {
        if(!Candidate[Seed]||Data->CellGroups[Seed]!=INDEX_NONE)continue;
        const int32 Group=Data->Levels.Num();Queue.Reset();Queue.Add(Seed);Data->CellGroups[Seed]=Group;
        double LevelSum=0;float Spill=MAX_flt,ReferenceMin=MAX_flt,ReferenceMax=-MAX_flt;
        float YMinGroup=MAX_flt,YMaxGroup=-MAX_flt;int32 SiltPocketCells=0;
        for(int32 Head=0;Head<Queue.Num();++Head)
        {
            const int32 Cell=Queue[Head],X=Cell%Data->Columns,Y=Cell/Data->Columns;const auto P=Centre(Cell);
            LevelSum+=ReferenceLevel[Cell];
            ReferenceMin=FMath::Min(ReferenceMin,ReferenceLevel[Cell]);ReferenceMax=FMath::Max(ReferenceMax,ReferenceLevel[Cell]);
            YMinGroup=FMath::Min(YMinGroup,float(P.Y));YMaxGroup=FMath::Max(YMaxGroup,float(P.Y));
            if(Op==12)
            {
                // Direct placement follows the six existing staggered silt
                // lenses. Only isolated shallow pockets beside these real
                // deposits are candidates; the long river thread is flowing.
                static const FVector2f Deposits[]={{-5250.f,155.f},{-3730.f,-105.f},{-2210.f,185.f},
                    {-760.f,-190.f},{180.f,165.f},{1150.f,-70.f}};
                const auto Wash=LysWashV57(P.X,P.Y,Home);bool Near=false;
                for(const auto& Site:Deposits)Near|=FMath::Abs(P.Y-Site.X)<520.f&&FMath::Abs(Wash.Distance-Site.Y)<350.f;
                SiltPocketCells+=Near;
            }
            for(const auto& Direction:Directions)
            {
                const int32 NX=X+Direction.X,NY=Y+Direction.Y;
                if(NX<0||NY<0||NX>=Data->Columns||NY>=Data->Rows)continue;
                const int32 Other=NY*Data->Columns+NX;const auto Q=Centre(Other),Edge=(P+Q)*.5;
                const int32 SharedX=Direction.X>0?X+1:X,SharedY=Direction.Y>0?Y+1:Y;
                const bool SharedCorner=Direction.X!=0
                    ?VertexWet[Y*VColumns+SharedX]||VertexWet[(Y+1)*VColumns+SharedX]
                    :VertexWet[SharedY*VColumns+X]||VertexWet[SharedY*VColumns+X+1];
                const bool Across=SharedCorner||LegacyRegionalWaterV54(Edge.X,Edge.Y,Field).Margin>=0.f;
                if(Candidate[Other]&&Across)
                {
                    if(Data->CellGroups[Other]==INDEX_NONE){Data->CellGroups[Other]=Group;Queue.Add(Other);}
                    continue;
                }
                // A clipped neighbouring impact pool or a protected trench
                // support is not an open spill outlet of this drainage patch.
                bool Protected=false;
                if(const auto* Near=Field.ShoreCells.Find(FIntPoint(FMath::FloorToInt(Q.X/800.f),FMath::FloorToInt(Q.Y/800.f))))
                    for(int32 I:*Near)Protected|=Field.Basins[I].bWetGroup&&BasinProtectionV47(Field.Basins[I],FVector2f(Q),Field.bRail)>-85.f;
                if(Protected)continue;
                const FVector2D Outward(Direction.X*Step,Direction.Y*Step);
                const float Bank=FMath::Max3(AWarfareFrontlineGround::HeightAt(Edge.X,Edge.Y),
                    AWarfareFrontlineGround::HeightAt(Q.X,Q.Y),AWarfareFrontlineGround::HeightAt(Q.X+Outward.X,Q.Y+Outward.Y));
                ++Data->SpillBoundarySamples;
                // Legacy Margin combines bed depth with artist-defined silt,
                // channel extent and trench masks. A dry mask over submerged
                // soil is not a retaining bank: using its bed as an outlet
                // drained the whole upstream component in the first bake.
                // Admit only an actual bed/rim crossing of the local accepted
                // visible surface. Keep the original 1.5 cm bank clearance.
                const float LocalVisibleSurface=ReferenceLevel[Cell]+1.2f;
                if(Bank<LocalVisibleSurface){++Data->ClippedSubmergedBoundaries;continue;}
                ++Data->PhysicalSpillSamples;
                Spill=FMath::Min(Spill,Bank-1.5f);
            }
        }
        const float Mean=float(LevelSum/Queue.Num());
        // Constant Z constrained by the lowest neighbouring retained bank.
        // A patch without a physical retaining lip is allowed to drain.
        const float StandingLevel=FMath::Min(Mean,Spill);int32 StandingCells=0;
        for(int32 Cell:Queue){const auto P=Centre(Cell);StandingCells+=StandingLevel+1.2f>AWarfareFrontlineGround::HeightAt(P.X,P.Y)+.35f;}
        // The Lys shallow river cannot be compressed into a common lake.
        // Admit a directly placed static pocket only when a real silt lens
        // bounds a short, low-gradient reach and its retaining level preserves
        // at least 80% of the sampled pocket. Otherwise retain actual flow.
        const bool Flowing=Op==12&&!(SiltPocketCells*2>=Queue.Num()&&YMaxGroup-YMinGroup<=800.f
            &&ReferenceMax-ReferenceMin<=8.f&&StandingCells>=Queue.Num()*.8f);
        const float Level=Flowing?Mean:StandingLevel;
        Data->Levels.Add(Level);Data->LegacyMeanLevels.Add(Mean);Data->SpillLevels.Add(Spill);
        Data->GroupKinds.Add(Flowing?1:0);Data->RetainedCells+=Flowing?Queue.Num():StandingCells;
    }
    Data->LysComposition=Op==12?1:0;
    bool Used[256]={false};Used[0]=Used[255]=true;
    for(const auto& B:Field.Basins)if(B.bWetGroup)
        Used[FLinearColor(0.f,B.Group/float(Field.Basins.Num()),0.f,1.f).ToFColor(true).G]=true;
    for(int32 Group=0;Group<Data->Levels.Num();++Group)
    {
        int32 Colour=INDEX_NONE;
        for(int32 C=254;C>0;--C)if(!Used[C]){Colour=C;Used[C]=true;break;}
        Data->GroupColours.Add(Colour);
    }
    Data->bValid=WaterOperationV72(Op)&&Data->Levels.Num()>0&&Data->RetainedCells>0
        &&!Data->GroupColours.Contains(INDEX_NONE);
    return Data;
}
#endif
