// Included after the V75 mesh/hash helpers in Ground.cpp. Only offline copies
// are edited; collision, height queries and hydraulic labels remain unchanged.
FString AWarfareFrontlineGround::CloneWaterEdgeCoverageV83(int32 Op)
{
    auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),false);Report->SetNumberField(TEXT("operation"),Op);
#if WITH_EDITOR
    const auto& Missions=AWarfareFrontlineGameMode::GetCampaignMissionsForFaction(EFrontlineFaction::British);
    if(!Missions.IsValidIndex(Op)){Report->SetStringField(TEXT("error"),TEXT("Only an existing British/German legacy operation may be copied."));return TerrainJSON43(Report);}
    const auto Style=Missions[Op].TerrainStyleV32;const float Scale=FrontlineGameplayV33::OperationDepthScale(Op);
    TArray<UStaticMesh*> Terrain;int32 TerrainVersion=0;
    if(!LoadTerrainTiles43(Op,Style,Scale,Terrain,&TerrainVersion))
    {Report->SetStringField(TEXT("error"),TEXT("The complete accepted terrain set is missing."));return TerrainJSON43(Report);}
    const int32 WaterVersion=Op==12?(TerrainVersion==58?58:57):Op==7?TerrainVersion==63?63:TerrainVersion==59?59:57:54;
    UStaticMesh* Source=nullptr;
    if(WaterOperationV72(Op))for(const int32 Version:{75,72})
    {
        const FString Path=WaterPackage44(Op,Style,Scale,Version);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+FPaths::GetBaseFilename(Path)));
        const auto* Data=Mesh?Cast<UFrontlineWaterPlanesV72>(Mesh->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass())):nullptr;
        if(Data&&Data->bValid&&Data->Schema==72&&Data->Operation==Op&&Data->Style==int32(Style)
            &&Data->DepthKey==FMath::RoundToInt(Scale*1000000.f)&&Data->GeometryVersion==(Op==12&&TerrainVersion==58?1:0))
        {Source=Mesh;break;}
    }
    if(!Source)
    {
        const FString Path=WaterPackage44(Op,Style,Scale,WaterVersion);
        Source=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+FPaths::GetBaseFilename(Path)));
    }
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Warfare/Field/V83/Materials/MI_BasinWater_V83.MI_BasinWater_V83"));
    const FMeshDescription* Original=Source?Source->GetMeshDescription(0):nullptr;
    if(!Material||!Original||Source->GetNumSourceModels()!=1)
    {Report->SetStringField(TEXT("error"),TEXT("The paired accepted water mesh or candidate material is missing."));return TerrainJSON43(Report);}
    FAssetCompilingManager::Get().FinishAllCompilation();
    const FString PackageName=WaterShorePackageV83(Source),Asset=FPaths::GetBaseFilename(PackageName);
    const auto* Data=Cast<UFrontlineWaterPlanesV72>(Source->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass()));
    const FString PlaneBefore=Data?WaterPlaneDigestV75(Data):TEXT("no_regional_metadata");
    const auto Before=WaterDescriptionDigestsV75(*Original);
    FString RenderBefore,RGBBefore;
    if(!WaterRenderDigestsV75(Source,RenderBefore,RGBBefore))
    {Report->SetStringField(TEXT("error"),TEXT("Actual source render buffers are unavailable."));return TerrainJSON43(Report);}
    FMeshDescription D=*Original;FStaticMeshAttributes A(D);const auto P=A.GetVertexPositions();auto Colours=A.GetVertexInstanceColors();
    TArray<FWaterWeldVertexV75> Welded;TMap<FIntVector,TArray<int32>> WeldCells;TMap<FVertexInstanceID,int32> Canonical;
    TMap<uint64,FWaterEdgeV75> Edges;TArray<FWaterTriangleV75> Triangles;TMap<FIntPoint,TArray<int32>> TriangleCells;
    int32 Legacy=0,Regional=0,Degenerate=0;
    auto Weld=[&](FVertexInstanceID VI)
    {
        if(const auto* Existing=Canonical.Find(VI))return *Existing;
        const FVector3f Position=P[D.GetVertexInstanceVertex(VI)];const uint32 Group=WaterGroupBitsV75(Colours[VI].Y);
        const auto Cell=WaterWeldCellV75(Position);int32 Found=INDEX_NONE;
        for(int32 Z=-1;Z<=1&&Found==INDEX_NONE;++Z)for(int32 Y=-1;Y<=1&&Found==INDEX_NONE;++Y)for(int32 X=-1;X<=1&&Found==INDEX_NONE;++X)
            if(const auto* Bucket=WeldCells.Find(Cell+FIntVector(X,Y,Z)))for(int32 I:*Bucket)
                if(Welded[I].Group==Group&&(Welded[I].Position-Position).SizeSquared()<=.04f){Found=I;break;}
        if(Found==INDEX_NONE){Found=Welded.Add({Position,Group});WeldCells.FindOrAdd(Cell).Add(Found);}
        Canonical.Add(VI,Found);return Found;
    };
    for(const auto Triangle:D.Triangles().GetElementIDs())
    {
        const auto Corners=D.GetTriangleVertexInstances(Triangle);int32 B=0;
        for(const auto VI:Corners)B+=Colours[VI].Z>.5f;
        if(B==3){++Regional;continue;}
        if(B||Colours[Corners[0]].Y!=Colours[Corners[1]].Y||Colours[Corners[1]].Y!=Colours[Corners[2]].Y)
        {Report->SetStringField(TEXT("error"),TEXT("Mixed legacy branches or groups in a source triangle."));return TerrainJSON43(Report);}
        ++Legacy;FWaterTriangleV75 T;T.Group=WaterGroupBitsV75(Colours[Corners[0]].Y);int32 W[3];
        for(int32 I=0;I<3;++I){const FVector3f V=P[D.GetVertexInstanceVertex(Corners[I])];T.P[I]=FVector2D(V.X,V.Y);W[I]=Weld(Corners[I]);}
        const int32 TI=Triangles.Add(T);
        const auto Lo=WaterBucketV75(FVector2D(FMath::Min3(T.P[0].X,T.P[1].X,T.P[2].X),FMath::Min3(T.P[0].Y,T.P[1].Y,T.P[2].Y)),100);
        const auto Hi=WaterBucketV75(FVector2D(FMath::Max3(T.P[0].X,T.P[1].X,T.P[2].X),FMath::Max3(T.P[0].Y,T.P[1].Y,T.P[2].Y)),100);
        for(int32 Y=Lo.Y;Y<=Hi.Y;++Y)for(int32 X=Lo.X;X<=Hi.X;++X)TriangleCells.FindOrAdd(FIntPoint(X,Y)).Add(TI);
        if(W[0]==W[1]||W[1]==W[2]||W[2]==W[0]){++Degenerate;continue;}
        for(int32 I=0;I<3;++I)
        {
            const int32 J=(I+1)%3;const uint64 Key=(uint64(FMath::Min(W[I],W[J]))<<32)|uint32(FMath::Max(W[I],W[J]));auto& E=Edges.FindOrAdd(Key);
            if(!E.Uses){E.A=T.P[I];E.B=T.P[J];E.Group=T.Group;E.DepthA=Colours[Corners[I]].X*12.f;E.DepthB=Colours[Corners[J]].X*12.f;}++E.Uses;
        }
    }
    auto Covered=[&](const FVector2D& V,uint32 Group)
    {
        if(const auto* Bucket=TriangleCells.Find(WaterBucketV75(V,100)))for(int32 I:*Bucket)
            if(Triangles[I].Group==Group&&WaterTriangleContainsV75(Triangles[I],V))return true;
        return false;
    };
    constexpr double Width=45.;TArray<FWaterSegmentV75> Segments;TMap<FIntPoint,TArray<int32>> Cells;
    int32 Open=0,FalseInternal=0,NonManifold=0;float MaxEdgeDepth=0;
    for(const auto& Pair:Edges)
    {
        const auto& E=Pair.Value;if(E.Uses==2)continue;if(E.Uses>2){++NonManifold;continue;}++Open;
        const FVector2D Delta=E.B-E.A;const double Length=Delta.Size();if(Length<.001)continue;
        const FVector2D Mid=(E.A+E.B)*.5,Across(-Delta.Y/Length*.3,Delta.X/Length*.3);
        if(Covered(Mid+Across,E.Group)&&Covered(Mid-Across,E.Group)){++FalseInternal;continue;}
        MaxEdgeDepth=FMath::Max(MaxEdgeDepth,FMath::Max(E.DepthA,E.DepthB));
        const int32 SI=Segments.Add({E.A,E.B,E.Group});
        const auto Lo=WaterBucketV75(FVector2D(FMath::Min(E.A.X,E.B.X)-Width,FMath::Min(E.A.Y,E.B.Y)-Width),Width);
        const auto Hi=WaterBucketV75(FVector2D(FMath::Max(E.A.X,E.B.X)+Width,FMath::Max(E.A.Y,E.B.Y)+Width),Width);
        for(int32 Y=Lo.Y;Y<=Hi.Y;++Y)for(int32 X=Lo.X;X<=Hi.X;++X)Cells.FindOrAdd(FIntPoint(X,Y)).Add(SI);
    }
    int32 Changed=0,RegionalInstances=0;float MinAlpha=1.f;bool RegionalEqual=true;
    for(const auto VI:D.VertexInstances().GetElementIDs())
    {
        auto C=Colours[VI];if(C.Z>.5f){++RegionalInstances;continue;}
        const FVector3f V=P[D.GetVertexInstanceVertex(VI)];const FVector2D XY(V.X,V.Y);const uint32 Group=WaterGroupBitsV75(C.Y);double DistanceSquared=Width*Width;
        if(const auto* Bucket=Cells.Find(WaterBucketV75(XY,Width)))for(int32 I:*Bucket)
        {
            const auto& S=Segments[I];if(S.Group!=Group)continue;const FVector2D Delta=S.B-S.A;
            const double T=FMath::Clamp(FVector2D::DotProduct(XY-S.A,Delta)/FMath::Max(Delta.SizeSquared(),1.e-9),0.,1.);
            DistanceSquared=FMath::Min(DistanceSquared,(XY-(S.A+Delta*T)).SizeSquared());
        }
        const float T=float(FMath::Sqrt(DistanceSquared)/Width),Alpha=T*T*(3.f-2.f*T);
        Changed+=C.W!=Alpha;MinAlpha=FMath::Min(MinAlpha,Alpha);C.W=Alpha;Colours[VI]=C;
    }
    const auto Edited=WaterDescriptionDigestsV75(D);FStaticMeshConstAttributes OldA(*Original);
    for(const auto VI:D.VertexInstances().GetElementIDs())if(Colours[VI].Z>.5f)RegionalEqual&=Colours[VI]==OldA.GetVertexInstanceColors()[VI];
    if(PackageName.IsEmpty()||!Legacy||!Changed||Segments.IsEmpty()||Edited.Geometry!=Before.Geometry||Edited.RGB!=Before.RGB||Edited.Basis!=Before.Basis||!RegionalEqual)
    {Report->SetStringField(TEXT("error"),TEXT("Legacy perimeter or exact geometry/RGB/basis/regional preservation failed."));return TerrainJSON43(Report);}
    auto* Package=CreatePackage(*PackageName);Package->FullyLoad();UStaticMesh* Mesh=FindObject<UStaticMesh>(Package,*Asset);const bool NewAsset=!Mesh;
    if(!Mesh)Mesh=NewObject<UStaticMesh>(Package,*Asset,RF_Public|RF_Standalone);
    Mesh->SetNumSourceModels(1);Mesh->GetSourceModel(0).BuildSettings=Source->GetSourceModel(0).BuildSettings;
    Mesh->GetNaniteSettings()=Source->GetNaniteSettings();Mesh->GetStaticMaterials()=Source->GetStaticMaterials();Mesh->bAllowCPUAccess=true;
    for(auto& Slot:Mesh->GetStaticMaterials())Slot.MaterialInterface=Material;
    Mesh->RemoveUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass());if(Data)Mesh->AddAssetUserData(DuplicateObject<UFrontlineWaterPlanesV72>(Data,Mesh,TEXT("HorizontalWaterPlanesV72")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;Params.bFastBuild=false;Params.bCommitMeshDescription=true;Params.bBuildSimpleCollision=false;Params.bMarkPackageDirty=true;
    TArray<const FMeshDescription*> Descriptions{&D};bool Good=Mesh->BuildFromMeshDescriptions(Descriptions,Params);FAssetCompilingManager::Get().FinishAllCompilation();
    if(GDistanceFieldAsyncQueue)GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();if(GCardRepresentationAsyncQueue)GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete();
    const auto* AfterD=Mesh->GetMeshDescription(0);FString RenderAfter,RGBAfter;Good&=AfterD&&WaterRenderDigestsV75(Mesh,RenderAfter,RGBAfter);
    const auto After=AfterD?WaterDescriptionDigestsV75(*AfterD):FWaterMeshDigestsV75();
    const auto* AfterData=Cast<UFrontlineWaterPlanesV72>(Mesh->GetAssetUserDataOfClass(UFrontlineWaterPlanesV72::StaticClass()));
    const FString PlaneAfter=AfterData?WaterPlaneDigestV75(AfterData):TEXT("no_regional_metadata");
    Good&=After.Geometry==Before.Geometry&&After.RGB==Before.RGB&&After.Basis==Before.Basis&&RenderBefore==RenderAfter&&RGBBefore==RGBAfter&&PlaneBefore==PlaneAfter&&RegionalEqual;
    const auto SourceAfter=WaterDescriptionDigestsV75(*Source->GetMeshDescription(0));Good&=SourceAfter.Geometry==Before.Geometry&&SourceAfter.RGB==Before.RGB&&SourceAfter.Basis==Before.Basis;
    Report->SetStringField(TEXT("source_asset"),Source->GetPathName());Report->SetStringField(TEXT("clone_asset"),Mesh->GetPathName());Report->SetNumberField(TEXT("terrain_version"),TerrainVersion);
    auto SetDigests=[&](const FString& Name,const FString& Old,const FString& New)
    {Report->SetStringField(TEXT("source_")+Name+TEXT("_digest"),Old);Report->SetStringField(TEXT("clone_")+Name+TEXT("_digest"),New);};
    SetDigests(TEXT("geometry"),Before.Geometry,After.Geometry);SetDigests(TEXT("rgb"),Before.RGB,After.RGB);
    SetDigests(TEXT("basis"),Before.Basis,After.Basis);SetDigests(TEXT("render_geometry"),RenderBefore,RenderAfter);
    SetDigests(TEXT("render_rgb"),RGBBefore,RGBAfter);SetDigests(TEXT("userdata"),PlaneBefore,PlaneAfter);
    Report->SetNumberField(TEXT("source_triangles"),Original->Triangles().Num());Report->SetNumberField(TEXT("legacy_triangles"),Legacy);Report->SetNumberField(TEXT("regional_triangles"),Regional);
    Report->SetNumberField(TEXT("legacy_perimeter_segments"),Segments.Num());Report->SetNumberField(TEXT("changed_legacy_alpha_instances"),Changed);Report->SetNumberField(TEXT("regional_instances_unchanged"),RegionalInstances);
    Report->SetNumberField(TEXT("false_internal_edges_excluded"),FalseInternal);Report->SetNumberField(TEXT("open_edges"),Open);Report->SetNumberField(TEXT("nonmanifold_edges"),NonManifold);Report->SetNumberField(TEXT("weld_degenerate_triangles"),Degenerate);
    Report->SetNumberField(TEXT("maximum_perimeter_encoded_depth_cm"),MaxEdgeDepth);Report->SetNumberField(TEXT("minimum_alpha"),MinAlpha);Report->SetNumberField(TEXT("coverage_world_width_cm"),Width);
    Report->SetBoolField(TEXT("exact_preservation"),Good);Report->SetBoolField(TEXT("regional_alpha_unchanged"),RegionalEqual);Report->SetBoolField(TEXT("cpu_access"),Mesh->bAllowCPUAccess);
    bool Saved=false;if(Good){if(NewAsset)FAssetRegistryModule::AssetCreated(Mesh);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_None;Saved=UPackage::SavePackage(Package,Mesh,*FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension()),Args);}
    Report->SetBoolField(TEXT("saved"),Saved);Report->SetBoolField(TEXT("success"),Good&&Saved);if(!Good)Report->SetStringField(TEXT("error"),TEXT("Built clone failed exact source contract; not saved."));
#else
    Report->SetStringField(TEXT("error"),TEXT("Water shoreline copying is editor-only."));
#endif
    return TerrainJSON43(Report);
}
