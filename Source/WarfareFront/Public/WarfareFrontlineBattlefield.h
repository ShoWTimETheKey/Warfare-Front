#pragma once

/** Shared centimetre contract for terrain, battle, camera and support targeting. */
namespace FrontlineBattlefield
{
    inline constexpr int AuthoringTrenchCount=5;
    inline constexpr int MaxTrenchCount=5;
    inline int TrenchCount=5;
    inline constexpr int TrenchCapacity=3;
    inline int CentreTrench=2;
    // Saved level components were authored at this pitch. Preserve that basis
    // when fitting their cached original transforms to an operation's trenches.
    inline constexpr float AuthoringTrenchSpacing=3680.f;
    constexpr float AuthoringTrenchX(int Index) { return (Index-2)*AuthoringTrenchSpacing; }
    // The game owns one active field. Configure only while clearing an encounter,
    // before rebuilding terrain; movement, support and the camera share it.
    inline float TrenchSpacing=AuthoringTrenchSpacing;
    inline float OperationDepthScale=1.f;
    inline float SpawnHalfLength=10488.f;
    inline float GoalHalfLength=10856.f;
    inline float SupportHalfLength=11956.f;
    inline constexpr float GroundHalfLength=15000.f;
    inline float HalfFrontage=1300.f;
    inline float CameraPanHalfLength=9936.f;
    inline int OperationIndex=-1;
    inline float TrenchPositions[MaxTrenchCount]={-7360.f,-3680.f,0.f,3680.f,7360.f};
    // Inner positions as fractions of the home-line distance from the origin.
    // The two-line operations have no inner position. Four-line operations use
    // both entries; all others use the first. These are local battlefield
    // layouts, shared by both armies, rather than changes to objective order.
    inline constexpr float OperationInnerFractions[20][2]={
        {-.25f,0.f}, // Arras: chalk ridge and short surviving approach
        {0.f,0.f},   // Roeux: two outposts by the factory
        { .24f,0.f}, // Bullecourt: Hindenburg wire ahead of the centre
        {-.29f,0.f}, // Messines: broken reverse slope
        {0.f,0.f},   // Messines counterattack: two connected positions
        { .32f,0.f}, // Ypres: forward dry footing across the marsh
        {0.f,0.f},   // Langemarck: two concrete outposts
        {-.17f,0.f}, // Menin Road: limited advance below the ridge
        {-.46f,.16f},// Cambrai: forward gap and staggered depth defence
        { .27f,0.f}, // Bourlon: wooded exit and reserve position
        {0.f,0.f},   // Winter raid: two close patrol lines
        {-.23f,0.f}, // Michael: broken rearward link through the fog
        {0.f,0.f},   // Lys: two positions across low ground
        { .18f,0.f}, // Villers-Bretonneux: village approach
        {-.16f,0.f}, // Amiens: broad field with a withdrawn support line
        { .21f,0.f}, // Scarpe: old chalk works along the field boundary
        {-.16f,.48f},// St Quentin Canal: staggered surface defences
        { .29f,0.f}, // Selle: rail cutting and narrowed approach
        {0.f,0.f},   // Valenciennes: two positions outside the town
        {-.27f,0.f} // Sambre: canal-side rear guard
    };
    inline int OperationTrenchCount(int Index)
    {
        constexpr int Counts[20]={3,2,3,3,2,3,2,3,4,3,2,3,2,3,3,3,4,3,2,3};
        return Index>=0&&Index<20?Counts[Index]:3;
    }
    inline float TrenchX(int Index) { return Index>=0&&Index<TrenchCount?TrenchPositions[Index]:(Index<0?-GoalHalfLength:GoalHalfLength); }
    inline void ConfigureOperationDepth(float Scale,int MissionIndex=-1)
    {
        HalfFrontage=1300.f;
        // Defensive bound without a Core dependency in this geometry header.
        // V45's shortest valid route is .415701 of the authored field.
        const float S=Scale>=.40f && Scale<=1.f?Scale:1.f;
        OperationDepthScale=S;
        OperationIndex=MissionIndex;
        TrenchCount=MissionIndex>=0?OperationTrenchCount(MissionIndex):(S<.60f?3:5);
        CentreTrench=TrenchCount/2;
        TrenchSpacing=AuthoringTrenchSpacing*S*(4.f/float(TrenchCount-1));
        const float Home=2.f*AuthoringTrenchSpacing*S;
        for(int I=0;I<TrenchCount;++I)TrenchPositions[I]=-Home+float(I)*TrenchSpacing;
        // Preserve the two symmetric homes and the authored bank mapping. The
        // inner works vary with each operation while remaining ordered and at
        // least 1800 cm apart even at the minimum supported depth scale.
        if(MissionIndex>=0&&MissionIndex<20&&TrenchCount==3)
            TrenchPositions[1]=Home*OperationInnerFractions[MissionIndex][0];
        if(MissionIndex>=0&&MissionIndex<20&&TrenchCount==4)
        {
            constexpr float MinPitch=1800.f;
            const float DesiredLeft=Home*OperationInnerFractions[MissionIndex][0];
            const float LeftMin=-Home+MinPitch,LeftMax=Home-2.f*MinPitch;
            TrenchPositions[1]=DesiredLeft<LeftMin?LeftMin:DesiredLeft>LeftMax?LeftMax:DesiredLeft;
            const float DesiredRight=Home*OperationInnerFractions[MissionIndex][1];
            const float RightMin=TrenchPositions[1]+MinPitch,RightMax=Home-MinPitch;
            TrenchPositions[2]=DesiredRight<RightMin?RightMin:DesiredRight>RightMax?RightMax:DesiredRight;
        }
        SpawnHalfLength=10488.f*S;
        // Line count must not silently lengthen the reinforcement route or goal.
        const float ReferencePitch=AuthoringTrenchSpacing*S*(S<.60f?2.f:1.f);
        GoalHalfLength=10856.f*S+.20f*ReferencePitch;
        SupportHalfLength=GoalHalfLength+1100.f;
        CameraPanHalfLength=GoalHalfLength-920.f*S;
    }
    inline void ConfigurePhysicalScene(float HalfLength,float HalfWidth)
    {
        HalfFrontage=HalfWidth;SpawnHalfLength=HalfLength;GoalHalfLength=HalfLength-1100.f;
        SupportHalfLength=HalfLength;CameraPanHalfLength=HalfLength-4600.f;
        OperationDepthScale=1.f;OperationIndex=-1;TrenchCount=0;CentreTrench=-1;
    }
    inline int HomeTrench(int Team) { return Team==0?0:TrenchCount-1; }
    inline int GoalIndex(int Team) { return Team==0?TrenchCount:-2; }
    inline int AuthoringFromActive(int Index)
    {
        if(Index<0||Index>=TrenchCount)return -1;
        if(TrenchCount==2)return Index*4;
        if(TrenchCount==3)return Index*2;
        if(TrenchCount==4)return Index<2?Index:Index+1;
        return Index;
    }
    inline int ActiveFromAuthoring(int Index)
    {
        for(int I=0;I<TrenchCount;++I)if(AuthoringFromActive(I)==Index)return I;
        return -1;
    }
}
