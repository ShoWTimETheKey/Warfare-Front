// Bounded FT contact solve shared by Editor/cooked runtime; no acceptance is granted.
// Native centimetres, actor scale one. The Model's existing Y reflection is
// already undone when producing these actual in-place mechanical-pose markers.
// A mathematical candidate is NOT permission to move. Final checked faces,
// chassis and whole-hull sweep are an explicit, mandatory separate result.
namespace FRRUVehicleGroundDraftV82
{
constexpr int32 MaxMarkers=4096, MaxSeeds=16, MaxTriples=560;
constexpr int32 MaxNewtonCandidates=8, MaxIterations=8, MaxFullValidations=8;
constexpr double MaxSlope=.65;
struct FMarker
{
    FVector NativeActorLocalCm=FVector::ZeroVector;
    int32 StableIndex=INDEX_NONE, Strip=INDEX_NONE, Element=INDEX_NONE;
    // A compiled recipe/bind-pose property: the rounded lower sole/grouser
    // band supplies seeds only. Non-seed surfaces still reject penetration.
    bool LowerSeedBand=false;
    int32 GearSide=0;
};
struct FNinePoint { FVector2D WorldXY;double NativeCm=0; };
struct FPlane
{
    double A=0,B=0,C=0,RawReliefCm=0,ResidualRangeCm=0;
};
struct FStats
{
    int32 FloorQueries=0,FloorCacheHits=0,Triples=0,NewtonCandidates=0;
    int32 Iterations=0,FullValidations=0,ColdNewtonCandidates=0;
    int32 ConvergedCandidates=0,MarkerRejectedCandidates=0,ActivePivots=0,ReferenceRejectedCandidates=0;
    int32 ActiveBatchAttempts=0,ActiveBatchTriples=0,ActiveBatchNewtonCandidates=0,ActiveBatchMarkerRejections=0;
    int32 WorstRejectedMarker=INDEX_NONE;
    int32 WorstRejectedElement=INDEX_NONE;
    FIntVector BestRejectedSupport=FIntVector(INDEX_NONE,INDEX_NONE,INDEX_NONE);
    FVector BestRejectedParams=FVector::ZeroVector,WorstRejectedLocal=FVector::ZeroVector,WorstRejectedWorld=FVector::ZeroVector;
    bool BudgetExhausted=false,ColdSlotReserved=false;
    double SolveMs=0,MinimumMarkerGapCm=0,MaximumMarkerGapCm=0,BestRejectedGapCm=0;
    FString FirstBlock;
};
struct FCandidate
{
    FTransform Pose;
    FVector Params=FVector::ZeroVector; // world slopes A/B and actor root Z
    int32 Support[3]={INDEX_NONE,INDEX_NONE,INDEX_NONE};
    FVector WorldSupport[3];
    double GapCm[3]={0,0,0},TriangleAreaCm2=0;
    FVector KinematicReferenceWeights=FVector::ZeroVector;
    bool MathematicalCandidateOnly=true;
    FStats Stats;
};
struct FWarmSupport
{
    // Caller must bind these to encounter, scene id/revision, stable vehicle,
    // serial and exact marker binding identity. Invalid binding gives nullptr.
    int32 Indices[3]={INDEX_NONE,INDEX_NONE,INDEX_NONE};
    double A=0,B=0,RootAboveCenterFloorCm=0;
};

bool Solve3(const FVector& Row0,const FVector& Row1,const FVector& Row2,const FVector& Values,FVector& Out)
{
    const double Det=FVector::DotProduct(Row0,FVector::CrossProduct(Row1,Row2));
    if(!FMath::IsFinite(Det)||FMath::Abs(Det)<1.e-9)return false;
    const FVector Result=(FVector::CrossProduct(Row1,Row2)*Values.X
        +FVector::CrossProduct(Row2,Row0)*Values.Y+FVector::CrossProduct(Row0,Row1)*Values.Z)/Det;
    if(Result.ContainsNaN())return false;Out=Result;return true;
}
FQuat Orientation(double A,double B,double HeadingDegrees)
{
    const double Yaw=FMath::DegreesToRadians(HeadingDegrees);
    const FVector Forward(FMath::Cos(Yaw),FMath::Sin(Yaw),A*FMath::Cos(Yaw)+B*FMath::Sin(Yaw));
    return FRotationMatrix::MakeFromXZ(Forward.GetSafeNormal(),FVector(-A,-B,1).GetSafeNormal()).ToQuat();
}
bool FitNine(const TArray<FNinePoint>& Points,const FVector2D& RootXY,FPlane& Out)
{
    if(Points.Num()!=9)return false;
    double XX=0,XY=0,YY=0,SX=0,SY=0,SZ=0,XZ=0,YZ=0;
    double Low=DBL_MAX,High=-DBL_MAX;
    for(const auto& P:Points)
    {
        if(P.WorldXY.ContainsNaN()||!FMath::IsFinite(P.NativeCm))return false;
        const double X=P.WorldXY.X-RootXY.X,Y=P.WorldXY.Y-RootXY.Y;
        XX+=X*X;XY+=X*Y;YY+=Y*Y;SX+=X;SY+=Y;SZ+=P.NativeCm;XZ+=X*P.NativeCm;YZ+=Y*P.NativeCm;
        Low=FMath::Min(Low,P.NativeCm);High=FMath::Max(High,P.NativeCm);
    }
    FVector Fit;if(!Solve3({XX,XY,SX},{XY,YY,SY},{SX,SY,9},{XZ,YZ,SZ},Fit))return false;
    FPlane Candidate;Candidate.A=Fit.X;Candidate.B=Fit.Y;Candidate.C=Fit.Z;Candidate.RawReliefCm=High-Low;
    double MinResidual=DBL_MAX,MaxResidual=-DBL_MAX;
    for(const auto& P:Points)
    {
        const double Residual=P.NativeCm-(Fit.X*(P.WorldXY.X-RootXY.X)+Fit.Y*(P.WorldXY.Y-RootXY.Y)+Fit.Z);
        MinResidual=FMath::Min(MinResidual,Residual);MaxResidual=FMath::Max(MaxResidual,Residual);
    }
    Candidate.ResidualRangeCm=MaxResidual-MinResidual;Out=Candidate;return true;
}

// Floor must read the SAME validated scene directly (height/deck, not legacy
// cache). Do not run FootingAt's solid loop for every mathematical candidate.
// Context, native center/gear leg .65 and footing are independent mandatory
// caller gates. Re-query final pose XY because tilt changes footprint XY.
template<typename FloorQuery>
bool Find(const TArray<FMarker>& Markers,const TArray<FNinePoint>& Nine,
    const FVector2D& RootXY,double Heading,double ExistingReliefLimit,double ContactResolutionCm,
    const FWarmSupport* Warm,FloorQuery&& Floor,FCandidate& Out,FStats& Failure,bool ReserveColdLast=false)
{
    const double Begin=FPlatformTime::Seconds();FStats Stats;
    auto FinishFailure=[&](const TCHAR* Why)
    {Stats.FirstBlock=Why;Stats.SolveMs=(FPlatformTime::Seconds()-Begin)*1000.;Failure=Stats;return false;};
    if(Markers.Num()<3||Markers.Num()>MaxMarkers||RootXY.ContainsNaN()||!FMath::IsFinite(Heading)
        ||!FMath::IsFinite(ContactResolutionCm)||ContactResolutionCm<=0||ContactResolutionCm>.1)return FinishFailure(TEXT("ground_context_or_markers"));
    FPlane Plane;if(!FitNine(Nine,RootXY,Plane))return FinishFailure(TEXT("native_floor_or_plane"));
    // Values remain tracked=125/wheeled=50, without raising either. Rail keeps
    // its existing dedicated rail/contact path and must never call this helper.
    if(Plane.ResidualRangeCm>ExistingReliefLimit)return FinishFailure(TEXT("plane_residual"));
    TMap<FVector2D,double> FloorCache;
    auto Height=[&](const FVector2D& XY,double& Z)
    {
        if(const double* Cached=FloorCache.Find(XY)){++Stats.FloorCacheHits;Z=*Cached;return true;}
        ++Stats.FloorQueries;if(!Floor(XY,Z)||!FMath::IsFinite(Z))return false;
        FloorCache.Add(XY,Z);return true;
    };
    double CenterFloor=0;if(!Height(RootXY,CenterFloor))return FinishFailure(TEXT("native_floor"));
    const FQuat Yaw=FRotator(0,Heading,0).Quaternion();
    TArray<FVector> Original;TArray<double> Needed;
    Original.Reserve(Markers.Num());Needed.Reserve(Markers.Num());
    for(const auto& M:Markers)
    {
        if(M.NativeActorLocalCm.ContainsNaN())return FinishFailure(TEXT("marker_not_finite"));
        const FVector P=Yaw.RotateVector(M.NativeActorLocalCm);double Z=0;
        if(!Height({RootXY.X+P.X,RootXY.Y+P.Y},Z))return FinishFailure(TEXT("native_floor"));
        Original.Add(P);Needed.Add(Z-P.Z);
    }
    TMap<int32,TArray<int32>> Strips;
    for(int32 I=0;I<Markers.Num();++I)Strips.FindOrAdd(Markers[I].Strip).Add(I);
    if(Strips.Num()>4)return FinishFailure(TEXT("profile_strip_contract"));
    TArray<int32> Seeds;
    for(const auto& Strip:Strips)
    {
        int32 Rear=INDEX_NONE,Front=INDEX_NONE,Highest=INDEX_NONE,Worst=INDEX_NONE;
        double HighestZ=-DBL_MAX,WorstResidual=-DBL_MAX;
        for(int32 I:Strip.Value)
        {
            if(Markers[I].LowerSeedBand)
            {
                if(Rear==INDEX_NONE||Markers[I].NativeActorLocalCm.X<Markers[Rear].NativeActorLocalCm.X)Rear=I;
                if(Front==INDEX_NONE||Markers[I].NativeActorLocalCm.X>Markers[Front].NativeActorLocalCm.X)Front=I;
            }
            if(Needed[I]>HighestZ){HighestZ=Needed[I];Highest=I;}
            const double Residual=Needed[I]-(Plane.A*Original[I].X+Plane.B*Original[I].Y);
            if(Residual>WorstResidual){WorstResidual=Residual;Worst=I;}
        }
        for(int32 I:{Rear,Front,Highest,Worst})if(I!=INDEX_NONE)Seeds.AddUnique(I);
    }
    Seeds.Sort();if(Seeds.Num()>MaxSeeds)return FinishFailure(TEXT("seed_limit"));
    TArray<FIntVector> Triples;
    if(Warm)Triples.Add({Warm->Indices[0],Warm->Indices[1],Warm->Indices[2]});
    for(int32 A=0;A<Seeds.Num();++A)for(int32 B=A+1;B<Seeds.Num();++B)for(int32 C=B+1;C<Seeds.Num();++C)
        Triples.Add({Seeds[A],Seeds[B],Seeds[C]});
    bool Found=false;FCandidate Best;
    auto Evaluate=[&](const FVector& Params,int32 Index,double& Gap,FVector* World=nullptr)
    {
        const FVector P=Orientation(Params.X,Params.Y,Heading).RotateVector(Markers[Index].NativeActorLocalCm)
            +FVector(RootXY.X,RootXY.Y,Params.Z);double Z=0;
        if(!Height({P.X,P.Y},Z))return false;Gap=P.Z-Z;if(World)*World=P;return true;
    };
    struct FTrial { FIntVector Indices;FVector Params;double Score=0;bool IsWarm=false,IsActiveBatch=false,IsCold=false; };
    TArray<FTrial> Trials;Trials.Reserve(Triples.Num());
    TArray<FTrial>* TrialDestination=&Trials;
    auto Canonical=[](FIntVector Indices)
    {if(Indices.X>Indices.Y)Swap(Indices.X,Indices.Y);if(Indices.Y>Indices.Z)Swap(Indices.Y,Indices.Z);if(Indices.X>Indices.Y)Swap(Indices.X,Indices.Y);return Indices;};
    TSet<FIntVector> Visited;
    auto Admissible=[&](const FIntVector& Indices)
    {
        const int32 I[3]={Indices.X,Indices.Y,Indices.Z};
        if(!Markers.IsValidIndex(I[0])||!Markers.IsValidIndex(I[1])||!Markers.IsValidIndex(I[2])||I[0]==I[1]||I[1]==I[2]||I[0]==I[2])return false;
        if(Markers[I[0]].Strip==Markers[I[1]].Strip&&Markers[I[1]].Strip==Markers[I[2]].Strip)return false;
        if(Markers[I[0]].GearSide==Markers[I[1]].GearSide&&Markers[I[1]].GearSide==Markers[I[2]].GearSide)return false;
        if(Markers[I[0]].Element==Markers[I[1]].Element&&Markers[I[1]].Element==Markers[I[2]].Element)return false;
        // Conservative kinematic actor-root reference, NOT an authored mass
        // centre or force-equilibrium proof. Exclude tripods whose projected
        // support domain cannot carry even this unchanged reference point.
        FVector Weights;
        return Solve3({Original[I[0]].X,Original[I[1]].X,Original[I[2]].X},
            {Original[I[0]].Y,Original[I[1]].Y,Original[I[2]].Y},{1,1,1},{0,0,1},Weights)
            &&Weights.GetMin()>=-1.e-6;
    };
    auto RankTrial=[&](int32 T)
    {
        if(Stats.Triples>=MaxTriples+(Warm?1:0)){Stats.BudgetExhausted=true;return;}
        ++Stats.Triples;const FIntVector Indices=Triples[T];
        const int32 I[3]={Indices.X,Indices.Y,Indices.Z};
        if(!Admissible(Indices))return;
        FVector Params;
        if(Warm&&T==0)Params={Warm->A,Warm->B,CenterFloor+Warm->RootAboveCenterFloorCm};
        else
        {
            if(!Solve3({Original[I[0]].X,Original[I[0]].Y,1},{Original[I[1]].X,Original[I[1]].Y,1},
                {Original[I[2]].X,Original[I[2]].Y,1},{Needed[I[0]],Needed[I[1]],Needed[I[2]]},Params))return;
            const double SeedSlope=FVector2D(Params.X,Params.Y).Size();
            if(SeedSlope>MaxSlope){Params.X*=MaxSlope/SeedSlope;Params.Y*=MaxSlope/SeedSlope;}
        }
        // The yaw-only linear solution is a seed, not a penetration proof.
        // Rank it at its REAL tilted XY before spending the Newton budget.
        // A native grid kink can move under a shoe when the actor tilts.
        double Low=DBL_MAX,SupportError=0;bool Ranked=true;
        for(int32 J:Seeds){double Gap=0;if(!Evaluate(Params,J,Gap)){Ranked=false;break;}Low=FMath::Min(Low,Gap);}
        for(int32 J:I){double Gap=0;if(!Evaluate(Params,J,Gap)){Ranked=false;break;}SupportError=FMath::Max(SupportError,FMath::Abs(Gap));}
        if(Ranked)TrialDestination->Add({Indices,Params,FMath::Max(0.,-Low)+SupportError,Warm&&T==0,false,!(Warm&&T==0)});
    };
    bool ColdRanked=false;
    auto RankCold=[&]()
    {
        // Append cold trials without reordering already evaluated active pivots.
        TArray<FTrial> ColdTrials;TrialDestination=&ColdTrials;
        for(int32 T=Warm?1:0;T<Triples.Num();++T)RankTrial(T);
        TrialDestination=&Trials;ColdTrials.StableSort([](const FTrial& A,const FTrial& B){return A.Score<B.Score;});
        Trials.Append(MoveTemp(ColdTrials));
        ColdRanked=true;
    };
    bool ActiveBatchAttempted=false;
    auto QueueActivePivots=[&](const FIntVector& Supports,const FVector& Params,int32 Worst,const TArray<int32>& StripWorst,int32 NextTrial)
    {
        // The most negative REAL tilted marker becomes an active contact.
        // Predict each one-support replacement with the same native floor and
        // rank all markers at that predicted pose. Root Z follows these three
        // contact equations; there is no arbitrary clearance lift.
        TArray<FTrial> Pivots;
        for(int32 Replace=0;Replace<3;++Replace)
        {
            FIntVector Indices=Supports;Indices[Replace]=Worst;Indices=Canonical(Indices);
            if(!Admissible(Indices)||Visited.Contains(Indices))continue;
            const int32 I[3]={Indices.X,Indices.Y,Indices.Z};FVector Errors,Columns[3];bool Valid=true;
            for(int32 J=0;J<3;++J)if(!Evaluate(Params,I[J],Errors[J])){Valid=false;break;}
            const double Step[3]={1.e-5,1.e-5,.001};
            for(int32 Axis=0;Axis<3&&Valid;++Axis)
            {
                FVector PerturbedParams=Params;PerturbedParams[Axis]+=Step[Axis];
                for(int32 J=0;J<3;++J){double Gap=0;if(!Evaluate(PerturbedParams,I[J],Gap)){Valid=false;break;}Columns[Axis][J]=(Gap-Errors[J])/Step[Axis];}
            }
            FVector Delta;if(!Valid||!Solve3({Columns[0].X,Columns[1].X,Columns[2].X},
                {Columns[0].Y,Columns[1].Y,Columns[2].Y},{Columns[0].Z,Columns[1].Z,Columns[2].Z},-Errors,Delta))continue;
            if(FVector2D(Delta.X,Delta.Y).Size()>.5||FMath::Abs(Delta.Z)>125)continue;
            const FVector Seed=Params+Delta;if(Seed.ContainsNaN()||FVector2D(Seed.X,Seed.Y).Size()>MaxSlope)continue;
            double Low=DBL_MAX,Error=0;
            for(int32 J=0;J<Markers.Num();++J){double Gap=0;if(!Evaluate(Seed,J,Gap)){Valid=false;break;}Low=FMath::Min(Low,Gap);}
            for(int32 J:I){double Gap=0;if(!Evaluate(Seed,J,Gap)){Valid=false;break;}Error=FMath::Max(Error,FMath::Abs(Gap));}
            if(Valid)Pivots.Add({Indices,Seed,FMath::Max(0.,-Low)+Error,false});
        }
        if(!ActiveBatchAttempted)
        {
            // A single exchange can leave the root outside the support domain
            // or below another strip even when two exchanges yield a valid
            // tripod. The existence of more single exchanges is not progress:
            // do not let that queue starve the multi-contact topology. Use the actual
            // deepest point of each strip at THIS rejected pose, not another
            // phase or an artificial root-height correction. At most seven
            // points give 35 triples, once per Find, under the unchanged eight
            // Newton/full-validation budget.
            ActiveBatchAttempted=true;++Stats.ActiveBatchAttempts;
            TArray<int32> Active;Active.AddUnique(Supports.X);Active.AddUnique(Supports.Y);Active.AddUnique(Supports.Z);
            for(int32 Index:StripWorst)if(Index!=INDEX_NONE)Active.AddUnique(Index);
            Active.Sort();
            if(Active.Num()<=7)
            for(int32 A=0;A<Active.Num();++A)for(int32 B=A+1;B<Active.Num();++B)for(int32 C=B+1;C<Active.Num();++C)
            {
                ++Stats.ActiveBatchTriples;const FIntVector Indices(Active[A],Active[B],Active[C]);
                if(!Admissible(Indices)||Visited.Contains(Indices)
                    ||Pivots.ContainsByPredicate([&](const FTrial& Trial){return Trial.Indices==Indices;}))continue;
                const int32 I[3]={Indices.X,Indices.Y,Indices.Z};FVector Seed;
                // Seed these three contact equations directly. A tangent step
                // from the old, deeply penetrating tripod can be ill-conditioned.
                // Newton still solves the actual tilted XY before any acceptance.
                if(!Solve3({Original[I[0]].X,Original[I[0]].Y,1},{Original[I[1]].X,Original[I[1]].Y,1},
                    {Original[I[2]].X,Original[I[2]].Y,1},{Needed[I[0]],Needed[I[1]],Needed[I[2]]},Seed))continue;
                const double SeedSlope=FVector2D(Seed.X,Seed.Y).Size();
                if(SeedSlope>MaxSlope){Seed.X*=MaxSlope/SeedSlope;Seed.Y*=MaxSlope/SeedSlope;}
                double Low=DBL_MAX,Error=0;bool Valid=true;
                for(int32 J=0;J<Markers.Num();++J){double Gap=0;if(!Evaluate(Seed,J,Gap)){Valid=false;break;}Low=FMath::Min(Low,Gap);}
                for(int32 J:I){double Gap=0;if(!Evaluate(Seed,J,Gap)){Valid=false;break;}Error=FMath::Max(Error,FMath::Abs(Gap));}
                int32 Shared=0;for(int32 Index:I)if(Index==Supports.X||Index==Supports.Y||Index==Supports.Z)++Shared;
                if(Valid)Pivots.Add({Indices,Seed,FMath::Max(0.,-Low)+Error,false,Shared<=1});
            }
        }
        Pivots.StableSort([](const FTrial& A,const FTrial& B){return A.Score<B.Score;});
        // Spend one of the unchanged remaining Newton slots on the best actual
        // multi-exchange seed. Merely generating it would still allow a newly
        // queued single-exchange chain to consume all eight slots first.
        const int32 BatchFirst=Pivots.IndexOfByPredicate([](const FTrial& Trial){return Trial.IsActiveBatch;});
        if(BatchFirst>0){const FTrial BatchTrial=Pivots[BatchFirst];Pivots.RemoveAt(BatchFirst);Pivots.Insert(BatchTrial,0);}
        for(int32 J=Pivots.Num()-1;J>=0;--J){Trials.Insert(Pivots[J],NextTrial);++Stats.ActivePivots;}
    };
    if(Warm)RankTrial(0);else RankCold();
    bool HaveRejectedGap=false;
    for(int32 T=0;T<Trials.Num()||!ColdRanked;++T)
    {
        // A warm tripod is re-solved and validated first. Do not pay the cold
        // tilted-XY ranking cost on every already supported movement substep.
        if(T>=Trials.Num()){RankCold();if(T>=Trials.Num())break;}
        // A warm/pivot chain cannot consume the final unchanged Newton slot
        // without trying one cold topology. Keep its first seven attempts;
        // rank cold only now, then use the best unvisited actual cold seed.
        if(ReserveColdLast&&Stats.NewtonCandidates==MaxNewtonCandidates-1&&Stats.ColdNewtonCandidates==0)
        {
            if(!ColdRanked)RankCold();int32 Cold=INDEX_NONE;
            for(int32 C=0;C<Trials.Num();++C)if(Trials[C].IsCold&&!Visited.Contains(Canonical(Trials[C].Indices))
                &&(Cold==INDEX_NONE||Trials[C].Score<Trials[Cold].Score))Cold=C;
            if(Cold!=INDEX_NONE)
            {const FTrial LastCold=Trials[Cold];Trials.RemoveAt(Cold);Trials.Insert(LastCold,T);Stats.ColdSlotReserved=true;}
        }
        const FIntVector TrialIndices=Trials[T].Indices;
        if(Visited.Contains(Canonical(TrialIndices)))continue;
        if(Stats.NewtonCandidates>=MaxNewtonCandidates||Stats.FullValidations>=MaxFullValidations)
        {Stats.BudgetExhausted=true;break;}
        Visited.Add(Canonical(TrialIndices));const int32 I[3]={TrialIndices.X,TrialIndices.Y,TrialIndices.Z};FVector Params=Trials[T].Params;
        ++Stats.NewtonCandidates;if(Trials[T].IsCold)++Stats.ColdNewtonCandidates;if(Trials[T].IsActiveBatch)++Stats.ActiveBatchNewtonCandidates;bool Converged=false;
        for(int32 Iteration=0;Iteration<MaxIterations;++Iteration)
        {
            ++Stats.Iterations;FVector Errors;bool Valid=true;
            for(int32 J=0;J<3;++J)if(!Evaluate(Params,I[J],Errors[J])){Valid=false;break;}
            if(!Valid)break;
            if(Errors.GetAbsMax()<1.e-6){Converged=true;break;}
            FVector Columns[3];const double Step[3]={1.e-5,1.e-5,.001};
            for(int32 Axis=0;Axis<3&&Valid;++Axis)
            {
                FVector PerturbedParams=Params;PerturbedParams[Axis]+=Step[Axis];
                for(int32 J=0;J<3;++J){double E=0;if(!Evaluate(PerturbedParams,I[J],E)){Valid=false;break;}Columns[Axis][J]=(E-Errors[J])/Step[Axis];}
            }
            FVector Delta;if(!Valid||!Solve3({Columns[0].X,Columns[1].X,Columns[2].X},
                {Columns[0].Y,Columns[1].Y,Columns[2].Y},{Columns[0].Z,Columns[1].Z,Columns[2].Z},-Errors,Delta))break;
            if(FVector2D(Delta.X,Delta.Y).Size()>.5||FMath::Abs(Delta.Z)>125)break;
            Params+=Delta;if(Params.ContainsNaN()||FVector2D(Params.X,Params.Y).Size()>MaxSlope)break;
        }
        if(!Converged)continue;
        ++Stats.ConvergedCandidates;++Stats.FullValidations;double MinGap=DBL_MAX,MaxGap=-DBL_MAX;bool Valid=true;int32 Worst=INDEX_NONE;
        TArray<double> CandidateGaps;CandidateGaps.SetNumUninitialized(Markers.Num());
        for(int32 J=0;J<Markers.Num();++J)
        {
            double Gap=0;if(!Evaluate(Params,J,Gap))return FinishFailure(TEXT("native_floor_at_tilted_marker"));
            CandidateGaps[J]=Gap;
            if(Gap< -ContactResolutionCm)Valid=false;
            if(Gap<MinGap){MinGap=Gap;Worst=J;}MaxGap=FMath::Max(MaxGap,Gap);
        }
        if(!Valid)
        {
            ++Stats.MarkerRejectedCandidates;
            if(Trials[T].IsActiveBatch)++Stats.ActiveBatchMarkerRejections;
            if(!HaveRejectedGap||MinGap>Stats.MinimumMarkerGapCm)
            {
                Stats.MinimumMarkerGapCm=MinGap;Stats.MaximumMarkerGapCm=MaxGap;Stats.WorstRejectedMarker=Worst;HaveRejectedGap=true;
                Stats.BestRejectedSupport=TrialIndices;Stats.BestRejectedParams=Params;Stats.BestRejectedGapCm=MinGap;
                Stats.WorstRejectedElement=Markers[Worst].Element;Stats.WorstRejectedLocal=Markers[Worst].NativeActorLocalCm;
                double Gap=0;Evaluate(Params,Worst,Gap,&Stats.WorstRejectedWorld);
            }
            if(Worst!=INDEX_NONE&&Stats.NewtonCandidates<MaxNewtonCandidates&&Stats.FullValidations<MaxFullValidations)
            {
                TArray<int32> StripWorst;StripWorst.Reserve(Strips.Num());
                for(const auto& Strip:Strips)
                {
                    int32 Deepest=INDEX_NONE;
                    for(int32 J:Strip.Value)if(Deepest==INDEX_NONE||CandidateGaps[J]<CandidateGaps[Deepest])Deepest=J;
                    StripWorst.Add(Deepest);
                }
                QueueActivePivots(TrialIndices,Params,Worst,StripWorst,T+1);
            }
            continue;
        }
        FCandidate Candidate;Candidate.Params=Params;Candidate.Pose=FTransform(Orientation(Params.X,Params.Y,Heading),{RootXY.X,RootXY.Y,Params.Z});
        for(int32 J=0;J<3;++J){Candidate.Support[J]=I[J];if(!Evaluate(Params,I[J],Candidate.GapCm[J],&Candidate.WorldSupport[J]))Valid=false;}
        if(!Valid)continue;
        Candidate.TriangleAreaCm2=FVector::CrossProduct(Candidate.WorldSupport[1]-Candidate.WorldSupport[0],Candidate.WorldSupport[2]-Candidate.WorldSupport[0]).Size()*.5;
        if(Candidate.TriangleAreaCm2<1.e-3)continue;
        if(!Solve3({Candidate.WorldSupport[0].X-RootXY.X,Candidate.WorldSupport[1].X-RootXY.X,Candidate.WorldSupport[2].X-RootXY.X},
            {Candidate.WorldSupport[0].Y-RootXY.Y,Candidate.WorldSupport[1].Y-RootXY.Y,Candidate.WorldSupport[2].Y-RootXY.Y},
            {1,1,1},{0,0,1},Candidate.KinematicReferenceWeights)||Candidate.KinematicReferenceWeights.GetMin()< -1.e-6)
        {++Stats.ReferenceRejectedCandidates;continue;}
        Candidate.Stats.MinimumMarkerGapCm=MinGap;Candidate.Stats.MaximumMarkerGapCm=MaxGap;
        if(!Found||Candidate.Params.Z<Best.Params.Z){Best=Candidate;Found=true;}
        // A re-solved, fully checked warm tripod avoids repeating the cold
        // combinatorial search every frame. It is never a cached world plane.
        if(Found)break;
    }
    // These are search results. Only the consumer's actual contact faces and
    // physics queries can report a physical collision; budget is not collision.
    if(!Found)return FinishFailure(Stats.BudgetExhausted?TEXT("support_search_budget"):
        Trials.IsEmpty()?TEXT("support_no_admissible_tripod"):
        Stats.ConvergedCandidates==0?TEXT("support_search_not_converged"):
        Stats.ReferenceRejectedCandidates>0&&Stats.MarkerRejectedCandidates==0?TEXT("kinematic_reference_outside_support"):
        TEXT("support_candidates_below_ground"));
    Stats.MinimumMarkerGapCm=Best.Stats.MinimumMarkerGapCm;Stats.MaximumMarkerGapCm=Best.Stats.MaximumMarkerGapCm;
    Stats.SolveMs=(FPlatformTime::Seconds()-Begin)*1000.;Best.Stats=Stats;Out=Best;return true;
}

// Conservative continuous sweep envelope. The FT consumer supplies the actual
// complete registered model bounds, replacing the legacy thin proxy. Ground
// is ignored ONLY after the finite sole/body contact surface passes.
// For a rigidly interpolated
// pose, each corner lies within 2*R*sin(angle/4) of the midpoint-rotation box.
// Includes the local query-box centre in R so its rotation is covered too.
// Caller performs both endpoint overlaps and this full sweep, plus finite
// sole/conservative-body contact checks. Rejection leaves actor/phase unchanged.
void HullSweepEnvelope(const FTransform& From,const FTransform& To,const FVector& LocalCentre,
    const FVector& ExistingHalfExtents,FVector& StartCentre,FVector& EndCentre,FQuat& MidRotation,FVector& HalfExtents)
{
    StartCentre=From.TransformPosition(LocalCentre);EndCentre=To.TransformPosition(LocalCentre);
    MidRotation=FQuat::Slerp(From.GetRotation(),To.GetRotation(),.5).GetNormalized();
    const double Angle=From.GetRotation().AngularDistance(To.GetRotation());
    const double Radius=LocalCentre.Size()+ExistingHalfExtents.Size();
    const double Inflation=2*Radius*FMath::Sin(Angle*.25);
    HalfExtents=ExistingHalfExtents+FVector(Inflation);
}
} // FRRUVehicleGroundDraftV82
