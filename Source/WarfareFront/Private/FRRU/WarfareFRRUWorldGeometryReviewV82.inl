#if WITH_EDITOR
namespace
{
TArray<TSharedPtr<FJsonValue>> GeometryVectorJsonV82(const FVector& Value)
{return {MakeShared<FJsonValueNumber>(Value.X),MakeShared<FJsonValueNumber>(Value.Y),MakeShared<FJsonValueNumber>(Value.Z)};}
TSharedRef<FJsonObject> GeometryBlockingHitV82(const FHitResult& Hit)
{
    auto Row=MakeShared<FJsonObject>();const auto* Actor=Hit.GetActor();const auto* Component=Hit.GetComponent();
    Row->SetStringField(TEXT("actor"),Actor?Actor->GetPathName():FString());Row->SetStringField(TEXT("component"),Component?Component->GetPathName():FString());
    Row->SetBoolField(TEXT("blockingHit"),Hit.bBlockingHit);Row->SetBoolField(TEXT("initialPenetration"),Hit.bStartPenetrating);
    Row->SetNumberField(TEXT("time"),Hit.Time);Row->SetNumberField(TEXT("penetrationDepth"),Hit.PenetrationDepth);Row->SetNumberField(TEXT("faceIndex"),Hit.FaceIndex);
    Row->SetArrayField(TEXT("location"),GeometryVectorJsonV82(Hit.Location));Row->SetArrayField(TEXT("impactPoint"),GeometryVectorJsonV82(Hit.ImpactPoint));
    Row->SetArrayField(TEXT("normal"),GeometryVectorJsonV82(Hit.Normal));Row->SetArrayField(TEXT("impactNormal"),GeometryVectorJsonV82(Hit.ImpactNormal));
    if(Component)
    {Row->SetArrayField(TEXT("boundsOrigin"),GeometryVectorJsonV82(Component->Bounds.Origin));Row->SetArrayField(TEXT("boundsExtent"),GeometryVectorJsonV82(Component->Bounds.BoxExtent));
        if(const auto* Static=Cast<UStaticMeshComponent>(Component))Row->SetStringField(TEXT("mesh"),Static->GetStaticMesh()?Static->GetStaticMesh()->GetPathName():FString());}
    return Row;
}
bool RunWorldGeometryReviewV82(AWarfareFrontlineGameMode* B)
{
    FString User;
    if(!CandidateReview()||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User)||!B||!B->HasActorBegunPlay()
        ||!B->IsProgressionVolatile()||B->Phase!=EFrontlinePhase::Briefing)
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_GEOMETRY_REVIEW_REFUSED: exact disposable BeginPlay context required"));return false;}
    FPaths::NormalizeDirectoryName(User);
    const FString Repository=FPaths::Combine(User,TEXT("FRRUReview/campaigns-tactical-v2.json"));
    if(IFileManager::Get().FileExists(*Repository)||IFileManager::Get().FileExists(*(Repository+TEXT(".bak"))))
    {UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_GEOMETRY_REVIEW_REFUSED: disposable national repository must be fresh"));return false;}
    const bool PreviousTick=B->IsActorTickEnabled();B->SetActorTickEnabled(false);
    TArray<TSharedPtr<FJsonValue>> Results,Scenes;int32 Passed=0;
    auto Check=[&](const FString& Name,bool Pass,const FString& Detail=FString())
    {auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("name"),Name);Item->SetBoolField(TEXT("passed"),Pass);Item->SetStringField(TEXT("detail"),Detail);Results.Add(MakeShared<FJsonValueObject>(Item));if(Pass)++Passed;else UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_GEOMETRY_CHECK_FAILED %s %s"),*Name,*Detail);};
    const auto Legacy=B->GetCampaignRuns();
    Check(TEXT("legacy_side_enums_remain_0_1"),uint8(EFrontlineFaction::British)==0&&uint8(EFrontlineFaction::German)==1);
    Check(TEXT("formal_release_uses_connected_host_hooks"),ActualHostHooksConnected);
    int32 MissionCount=0;
    for(const auto& Mission:wfexp::Missions())if(wfexp::IsSelectable(Mission.faction))
    {
        ++MissionCount;const FString Id=wfexpue::Wide(Mission.id);FString Error;
        const bool Fixture=WarfareFRRUCampaignInternalV82::PrepareReviewMission(B,Id,Error);
        Check(Id+TEXT(".synthetic_progression_actual_repository_fixture"),Fixture,Error);if(!Fixture)continue;
        const auto* G=Geometry(B);const auto* M=Definition(B);const bool Staged=G&&M&&PrepareScene(B,Error);
        Check(Id+TEXT(".actual_four_mesh_stage"),Staged,Error);if(!Staged)continue;
        auto Scene=MakeShared<FJsonObject>();Scene->SetStringField(TEXT("missionId"),Id);Scene->SetStringField(TEXT("sceneId"),wfexpue::Wide(G->id));
        Scene->SetBoolField(TEXT("progressionFixtureIsSynthetic"),true);Scene->SetNumberField(TEXT("nativeSolidCount"),G->solids.size());
        TArray<TSharedPtr<FJsonValue>> WaterRows;
        for(const FVector2D& At:{FVector2D(1080,0),FVector2D(1080,-3480),FVector2D(1080,3780),FVector2D(239,0),FVector2D(1921,0),FVector2D(1080,6001)})
        {
            wfexp::scene::WaterSurface Native;const bool NativeWet=wfexp::scene::QueryWaterSurface(*G,At.X,At.Y,Native);
            float Level=99,Depth=99,Group=99;const bool HostWet=TryWater(B,At.X,At.Y,Level,Depth,&Group);
            float GroundLevel=99,GroundDepth=99,GroundGroup=99;const bool GroundWet=AWarfareFrontlineGround::QueryWaterSurfaceV51(At.X,At.Y,GroundLevel,GroundDepth,&GroundGroup);
            const bool Same=HostWet==NativeWet&&GroundWet==NativeWet&&(!NativeWet||(FMath::Abs(Level-Native.levelCm)<.01&&FMath::Abs(Depth-Native.depthCm)<.01&&FMath::Abs(Group-1.f/255)<.000001
                &&FMath::IsNearlyEqual(Level,GroundLevel,.01f)&&FMath::IsNearlyEqual(Depth,GroundDepth,.01f)&&FMath::IsNearlyEqual(Group,GroundGroup,.000001f)));
            auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("x"),At.X);Row->SetNumberField(TEXT("y"),At.Y);Row->SetBoolField(TEXT("nativeWet"),NativeWet);Row->SetBoolField(TEXT("hostWet"),HostWet);Row->SetBoolField(TEXT("groundWet"),GroundWet);
            if(NativeWet){Row->SetNumberField(TEXT("levelCm"),Native.levelCm);Row->SetNumberField(TEXT("depthCm"),Native.depthCm);Row->SetStringField(TEXT("patchId"),wfexpue::Wide(Native.patchId));}
            WaterRows.Add(MakeShared<FJsonValueObject>(Row));
            Check(Id+FString::Printf(TEXT(".typed_water_%.0f_%.0f"),At.X,At.Y),Same,TEXT("Authored native patch only; bridge floor/dry/outside never use legacy basin data."));
        }
        Scene->SetArrayField(TEXT("typedWaterQueries"),WaterRows);Scene->SetBoolField(TEXT("waterMaterialResponseAccepted"),false);
        TArray<FVector2D> Heights;TArray<FFRRUSceneSolidRayV82> Rays;
        for(const auto& Post:M->nodes)
        {
            TArray<FVector> Places;bool Spacing=true,Fits=true;
            TArray<TSharedPtr<FJsonValue>> FailedCells;
            for(int Side=0;Side<2;++Side)for(int Cell=0;Cell<Post.capacity;++Cell)
            {
                const bool FitsCell=CellFits(B,Post,Side,Cell);Fits&=FitsCell;
                for(int Member=0;Member<6;++Member)
                {
                    const FVector Place=FormationPoint(Post,Side,Cell,Member);for(const auto& Other:Places)Spacing&=FVector::Dist2D(Place,Other)>=56.;Places.Add(Place);Heights.Add(FVector2D(Place));
                    if(!FitsCell)
                    {
                        const auto Footing=wfexp::scene::FootingAt(*G,Place.X,Place.Y);
                        const bool RadiusValid=FVector::Dist2D(Place,Vector(Post.position))<=Post.radius;
                        const bool NativeLeg=wfexp::scene::CanTraverseSegment(*G,Post.position,Point(Place));
                        FCollisionQueryParams CellQuery(SCENE_QUERY_STAT(FRRUV82FormationCellDiagnostic),true);for(const auto& Unit:B->Units)if(Unit)CellQuery.AddIgnoredActor(Unit);
                        FVector ContactFeet,ContactCentre;const bool ContactValid=GroundContact(B,Place,ContactFeet,ContactCentre);
                        TArray<FOverlapResult> Overlaps;const bool Blocked=ContactValid&&B->GetWorld()->OverlapMultiByChannel(Overlaps,ContactCentre,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),CellQuery);
                        if(!Footing.valid||!Footing.walkable||!RadiusValid||!NativeLeg||!ContactValid||Blocked)
                        {
                            auto Failure=MakeShared<FJsonObject>();Failure->SetStringField(TEXT("nodeId"),wfexpue::Wide(Post.id));Failure->SetNumberField(TEXT("side"),Side);Failure->SetNumberField(TEXT("cell"),Cell);Failure->SetNumberField(TEXT("member"),Member);
                            Failure->SetArrayField(TEXT("feet"),GeometryVectorJsonV82(FVector(Place.X,Place.Y,Footing.heightCm)));Failure->SetBoolField(TEXT("nativeValid"),Footing.valid);Failure->SetBoolField(TEXT("nativeWalkable"),Footing.walkable);
                            Failure->SetNumberField(TEXT("nativeSurface"),int32(Footing.surface));Failure->SetBoolField(TEXT("withinRadius"),RadiusValid);Failure->SetBoolField(TEXT("nativeCentreLeg"),NativeLeg);Failure->SetBoolField(TEXT("actualBlocked"),Blocked);
                            Failure->SetBoolField(TEXT("contactValid"),ContactValid);if(ContactValid)Failure->SetArrayField(TEXT("actualContactCentre"),GeometryVectorJsonV82(ContactCentre));
                            TArray<TSharedPtr<FJsonValue>> Bodies;for(const auto& Overlap:Overlaps)if(Overlap.bBlockingHit)
                            {auto Body=MakeShared<FJsonObject>();Body->SetStringField(TEXT("actor"),Overlap.GetActor()?Overlap.GetActor()->GetPathName():FString());Body->SetStringField(TEXT("component"),Overlap.GetComponent()?Overlap.GetComponent()->GetPathName():FString());Bodies.Add(MakeShared<FJsonValueObject>(Body));}
                            Failure->SetArrayField(TEXT("blockingBodies"),Bodies);FailedCells.Add(MakeShared<FJsonValueObject>(Failure));
                        }
                    }
                }
            }
            if(!FailedCells.IsEmpty())Scene->SetArrayField(TEXT("failedCells.")+wfexpue::Wide(Post.id),FailedCells);
            Check(Id+TEXT(".post.")+wfexpue::Wide(Post.id)+TEXT(".physical_bay_spacing"),Spacing,TEXT("Every side/squad/member destination is distinct and at least 56 cm apart."));
            Check(Id+TEXT(".post.")+wfexpue::Wide(Post.id)+TEXT(".terrain_and_capsule_fit"),Fits,TEXT("Each 2x3 formation remains within its registered radius and clear real geometry."));
            FFormationCells CandidateCells;bool Exclusive=true,PhantomsRejected=true;
            for(int Side=0;Side<2;++Side)
            {
                // Symbolic owners exercise the same capacity allocator against
                // explicit real six-rank terrain/capsule witnesses. They are
                // never passed off as existing production sections.
                const auto Claim=[&](int32 Owner){return AllocateFormationBayV87(Post,Side,Owner,CandidateCells,
                    [&](int32 Cell){return CellFits(B,Post,Side,Cell,INDEX_NONE);});};
                for(int Slot=0;Slot<Post.capacity;++Slot)Exclusive&=Claim(100+Side*10+Slot);
                const auto Before=CandidateCells;Exclusive&=!Claim(999+Side)&&CandidateCells==Before;
                FFormationCells RuntimeCells;RuntimeCells[Post.id];const auto Empty=RuntimeCells;const int32 Phantom=100000+Side;
                PhantomsRejected&=!B->FindSquad(Phantom)&&!ReserveCell(B,Post,Side,Phantom,RuntimeCells)&&RuntimeCells==Empty;
            }
            Check(Id+TEXT(".post.")+wfexpue::Wide(Post.id)+TEXT(".bounded_exclusive_reservations"),Exclusive&&PhantomsRejected,
                TEXT("Explicit six-man geometric bays use the real allocator; each side has separate capacity, overflow is unchanged, and normal runtime rejects nonexistent sections."));
        }
        for(const auto& Deck:G->decks)
        {
            const FVector2D A(Deck.from.x,Deck.from.y),Z(Deck.to.x,Deck.to.y),Normal=FVector2D(-(Z-A).Y,(Z-A).X).GetSafeNormal();
            for(float T:{0.f,.25f,.5f,.75f,1.f})for(float Width:{-.8f,0.f,.8f})Heights.Add(A+(Z-A)*T+Normal*(Deck.halfWidth*Width));
        }
        double Top=0;for(const auto& Solid:G->solids)Top=FMath::Max(Top,Solid.maximum.z);
        int32 SolidOrdinal=0;
        for(const auto& Solid:G->solids)
        {
            const FVector Centre=Vector({(Solid.minimum.x+Solid.maximum.x)*.5,(Solid.minimum.y+Solid.maximum.y)*.5,(Solid.minimum.z+Solid.maximum.z)*.5});
            const double Extent=(Solid.maximum.x-Solid.minimum.x)*.65+25;
            Rays.Add({FString::Printf(TEXT("solid_%d_through"),SolidOrdinal),Centre-FVector(Extent,0,0),Centre+FVector(Extent,0,0),true});
            Rays.Add({FString::Printf(TEXT("solid_%d_above"),SolidOrdinal),FVector(Centre.X-Extent,Centre.Y,Top+150),FVector(Centre.X+Extent,Centre.Y,Top+150),false});++SolidOrdinal;
        }
        TArray<TSharedPtr<FJsonValue>> Routes;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(FRRUV82RouteCapsuleReview),true);Query.AddIgnoredActor(B);
        for(const auto& Link:M->edges)
        {
            const auto* A=Node(*M,Link.from);const auto* Z=Node(*M,Link.to);
            const auto* Polyline=wfexp::scene::FindRoute(*G,Link.id);
            const bool Chord=A&&Z&&wfexp::scene::CanTraverseSegment(*G,A->position,Z->position);
            const bool Native=A&&Z&&(Polyline?wfexp::scene::CanTraverseRoute(*G,Link.id,A->position,Z->position):Chord);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("edgeId"),wfexpue::Wide(Link.id));Row->SetBoolField(TEXT("nativeWalkable"),Native);
            Row->SetBoolField(TEXT("directChordWalkable"),Chord);Row->SetBoolField(TEXT("registeredPolyline"),Polyline!=nullptr);
            bool Capsule=Native;int32 BlockingSegments=0;
            if(A&&Z)
            {
                TArray<FVector> Waypoints,Contacts;
                if(Polyline)for(const auto& Point:Polyline->points)Waypoints.Add(Vector(Point));
                else {Waypoints.Add(Vector(A->position));Waypoints.Add(Vector(Z->position));}
                bool ContactValid=Waypoints.Num()>=2;
                for(int32 Leg=1;Leg<Waypoints.Num();++Leg)
                {
                    TArray<FVector> LegContacts;
                    if(!GroundContactPath(B,Waypoints[Leg-1],Waypoints[Leg],LegContacts)){ContactValid=false;break;}
                    if(!Contacts.IsEmpty()&&!LegContacts.IsEmpty()&&Contacts.Last().Equals(LegContacts[0],.001))LegContacts.RemoveAt(0);
                    Contacts.Append(LegContacts);
                }
                Row->SetNumberField(TEXT("registeredLegs"),FMath::Max(0,Waypoints.Num()-1));
                Row->SetBoolField(TEXT("contactPathValid"),ContactValid);Row->SetNumberField(TEXT("contactPoints"),Contacts.Num());if(!ContactValid)Capsule=false;
                for(int32 I=0;I<Contacts.Num();++I)
                {
                    const FVector PointOnRoute=Contacts[I];
                    // Every contact leg is physically swept. Height readback
                    // retains its prior camera-independent sampling density.
                    if(I%8==0||I==Contacts.Num()-1)Heights.Add(FVector2D(PointOnRoute));
                    if(I>0)
                    {
                        FHitResult Hit;if(B->GetWorld()->SweepSingleByChannel(Hit,Contacts[I-1],PointOnRoute,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeCapsule(25,85),Query))
                        {
                            Capsule=false;++BlockingSegments;
                            if(BlockingSegments==1)
                            {auto Failure=GeometryBlockingHitV82(Hit);Failure->SetNumberField(TEXT("segment"),I);Failure->SetArrayField(TEXT("from"),GeometryVectorJsonV82(Contacts[I-1]));Failure->SetArrayField(TEXT("to"),GeometryVectorJsonV82(PointOnRoute));Row->SetObjectField(TEXT("firstBlockingHit"),Failure);}
                        }
                    }
                }
            }
            Row->SetNumberField(TEXT("blockingSegments"),BlockingSegments);Row->SetBoolField(TEXT("actualCapsuleClear"),Capsule);Routes.Add(MakeShared<FJsonValueObject>(Row));
            Check(Id+TEXT(".route.")+wfexpue::Wide(Link.id),Native&&Capsule,TEXT("All registered physical route legs, real 25x85 cm capsule; wire is a separate logical gate."));
        }
        Scene->SetArrayField(TEXT("routeProbes"),Routes);
        TSharedPtr<FJsonObject> Physical;const bool Probed=WarfareFRRUContentV82::ProbeScene(B,Heights,Rays,Physical,Error);
        if(Physical)Scene->SetObjectField(TEXT("actualComponentProbes"),Physical);Scene->SetStringField(TEXT("probeError"),Error);
        Check(Id+TEXT(".actual_ground_solids_decks_and_route_height"),Probed,Error);
        Check(Id+TEXT(".full_120m_frontage_and_240m_depth_retained"),FMath::IsNearlyEqual(FrontlineBattlefield::HalfFrontage,6000.f,.1f)&&FMath::IsNearlyEqual(FrontlineBattlefield::SupportHalfLength,12000.f,.1f)&&B->Trenches.IsEmpty());
        Scenes.Add(MakeShared<FJsonValueObject>(Scene));
    }
    Check(TEXT("all_twenty_registered_national_scenes_examined"),MissionCount==20&&Scenes.Num()==20);
    bool SameLegacy=B->GetCampaignRuns().Num()==Legacy.Num();
    for(int32 I=0;SameLegacy&&I<Legacy.Num();++I)
    {const auto& Before=Legacy[I];const auto& After=B->GetCampaignRuns()[I];SameLegacy=Before.Id==After.Id&&Before.Faction==After.Faction&&Before.Difficulty==After.Difficulty&&Before.SelectedMission==After.SelectedMission&&Before.Research.TechnologyMask==After.Research.TechnologyMask;}
    Check(TEXT("original_uk_de_campaign_descriptors_not_replaced"),SameLegacy);
    Reset(B);B->SetActorTickEnabled(PreviousTick);
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("revision"),TEXT("v82.frru.geometry.3"));Report->SetBoolField(TEXT("allPassed"),Passed==Results.Num());Report->SetNumberField(TEXT("passed"),Passed);Report->SetNumberField(TEXT("checks"),Results.Num());
    Report->SetArrayField(TEXT("results"),Results);Report->SetArrayField(TEXT("scenes"),Scenes);
    Report->SetBoolField(TEXT("progressionIsSynthetic"),true);Report->SetBoolField(TEXT("combatAccepted"),false);Report->SetBoolField(TEXT("artAccepted"),false);Report->SetBoolField(TEXT("dynamicWireAccepted"),false);Report->SetBoolField(TEXT("shippingAccepted"),false);
    const FString Receipt=FPaths::Combine(User,TEXT("FRRUReview/world-geometry.json"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Receipt),true);
    FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(Report,Writer);
    const bool Written=FFileHelper::SaveStringToFile(Json,*Receipt,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("WF_FRRU_V82_GEOMETRY_REVIEW %d/%d receipt=%s"),Passed,Results.Num(),*Receipt);return Written&&Passed==Results.Num();
}
FAutoConsoleCommand WorldGeometryReviewCommandV82(TEXT("WF.FRRU.ReviewWorldGeometryV82"),TEXT("Disposable twenty-scene actual geometry/physics review."),FConsoleCommandDelegate::CreateLambda([]()
{
    if(!CandidateReview()){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_GEOMETRY_REVIEW_REFUSED: isolation flags/path invalid"));return;}
    static bool Requested=false;if(Requested)return;Requested=true;const double Started=FPlatformTime::Seconds();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Started](float)
    {
        if(GEngine)for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World();World&&World->WorldType==EWorldType::Game)
            if(auto* B=World->GetAuthGameMode<AWarfareFrontlineGameMode>();B&&B->HasActorBegunPlay())
            {const bool Passed=RunWorldGeometryReviewV82(B);FPlatformMisc::RequestExitWithStatus(false,Passed?0:3);return false;}
        if(FPlatformTime::Seconds()-Started>45){UE_LOG(LogTemp,Error,TEXT("WF_FRRU_V82_GEOMETRY_REVIEW_FAILED: no actual BeginPlay within 45 seconds"));FPlatformMisc::RequestExitWithStatus(false,4);return false;}
        return true;
    }));
}));
}
#endif
