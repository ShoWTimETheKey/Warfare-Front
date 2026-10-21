#include "WarfareFrontlineMedalRibbonV8446.h"

namespace FrontlineMedalRibbonV8446
{
namespace
{
using FAward=FrontlineMedalsV8215::FAward;
using EAttachment=FrontlineMedalsV8215::EAttachment;

int32 SelectedClassGrade(int32 I,int32 G)
{
    // Preserve the catalogue's known textile when a class changes its full
    // insignia's pin, neck or sash form. That form alone does not establish
    // the absence of a military ribbon bar. Only these two documented
    // exceptions omit the selected class's corresponding ribbon.
    if((I==3||I==42||I==43)&&G==2)return 0; // First-Class breast pins have no separate ribbon.
    if(I==31&&G==1)return 0; // St Anne IV is a weapon badge.
    return G;
}

int32 EarnedRackGrade(int32 I,int32 G)
{
    if(I==3||I==18||I==21||I==22||I==11||I==32||I==33||I==42||I==43)return 1;
    if(I==20)return FMath::Min(G,2);
    if(I==31)return G>=2?2:0;
    if(I==34||I==35||I==36)return 0;
    return G;
}

bool Has(const FAward& A,EAttachment Kind)
{return (A.Attachments&uint16(Kind))!=0;}

void Add(FPresentation& R,EDevice Kind,EMetal Metal=EMetal::Unspecified,int32 Count=1)
{if(Count>0)R.Devices.Add({Kind,Metal,Count});}

bool Device(const FPresentation& R,EDevice Kind,EMetal Metal,int32 Count=1)
{for(const auto& D:R.Devices)if(D.Kind==Kind&&D.Metal==Metal&&D.Count==Count)return true;return false;}
}

FPresentation Presentation(int32 I,int32 G,EContext Context,const FAward& Recorded)
{
    FPresentation R;
    if(I<0||I>=FrontlineMedalsV8215::Count||G<1
        ||G>FrontlineMedalsV8215::Variants(I).Num()
        ||!FrontlineMedalsV8215::Definition(I).Ribbon)return R;
    R.Grade=Context==EContext::SelectedClass?SelectedClassGrade(I,G):EarnedRackGrade(I,G);
    if(!R.HasRibbon())return R;

    // Fourth/Second are plain, Third/First have a bow. Class numbers in the
    // catalogue ascend from IV to I, so this is deliberately non-monotonic.
    if((I==9||I==10)&&(R.Grade==2||R.Grade==4))Add(R,EDevice::Bow);

    // The 1916 military insignia already distinguish the upper grades.
    // The 5 July 1918 decree adopted the same distinctions for civilian
    // dress; current Code R70 also describes military demi-barrettes.
    // All five grades use the same red textile, without fabricated colours.
    if(I==8&&R.Grade>=2)
    {
        if(R.Grade>=3)Add(R,EDevice::RosetteBacking,
            R.Grade==3?EMetal::Silver:R.Grade==4?EMetal::SilverAndGold:EMetal::Gold);
        Add(R,EDevice::Rosette,EMetal::RedTextile);
    }

    // A preview of another class must not borrow the account's current
    // repeat bars, citations or optional attributes. Rack projection keeps
    // the real record while displaying its retained breast class.
    const bool RecordedApplies=Recorded.Grade>0&&int32(Recorded.Grade)==G;
    if(I==2)Add(R,EDevice::MiniatureCross,EMetal::Bronze,
        1+(RecordedApplies?FMath::Min(1,int32(Recorded.Repeats)):0));
    if(!RecordedApplies)return R;

    if(I==0||I==1||I==12||I==13||I==37||I==38||I==39)
        Add(R,EDevice::Rosette,I==12?EMetal::Gold:EMetal::Silver,FrontlineMedalsV8215::PresentedRepeatsV85(I,Recorded.Repeats));
    if(I==14&&(Has(Recorded,EAttachment::Clasp)||Has(Recorded,EAttachment::Rosette)))
        Add(R,EDevice::Rosette,EMetal::Silver); // dated clasp on full medal, rosette on ribbon alone
    if(I==17&&Has(Recorded,EAttachment::OakLeaves))Add(R,EDevice::OakLeaves,EMetal::Bronze);
    if(I==6)
    {
        // The existing receipt stores presence, not citation authority or
        // count. Do not invent a bronze/silver/vermeil ladder from Grade.
        if(Has(Recorded,EAttachment::Star))Add(R,EDevice::Star,EMetal::Unspecified);
        if(Has(Recorded,EAttachment::Palm))Add(R,EDevice::Palm,EMetal::Bronze);
    }
    // Optional swords/crowns and Russian-order military bows remain in the
    // existing full insignia artwork. There is no verified separate mini
    // asset for these textiles; an orange/black St George bow is not reused
    // on a different order, nor is a fabricated generic emblem substituted.
    if((I==26||I==27)&&Has(Recorded,EAttachment::Clasp))Add(R,EDevice::Clasp,EMetal::Silver);
    return R;
}

void RunChecks(TFunctionRef<void(const FString&,bool)> Check)
{
    const auto Selected=EContext::SelectedClass,Rack=EContext::EarnedRack;
    bool GradeSelection=true,NoRecordLeak=true;
    for(int32 I=0;I<FrontlineMedalsV8215::Count;++I)
        for(int32 G=1;G<=FrontlineMedalsV8215::Variants(I).Num();++G)
    {
        const auto P=Presentation(I,G,Selected),R=Presentation(I,G,Rack);
        const bool HasTextile=FrontlineMedalsV8215::Definition(I).Ribbon;
        GradeSelection&=P.Grade==(HasTextile?SelectedClassGrade(I,G):0)
            &&R.Grade==(HasTextile?EarnedRackGrade(I,G):0);
        FAward Foreign;Foreign.Grade=uint8(G==1?2:1);Foreign.Repeats=3;Foreign.Attachments=uint16(EAttachment::Star)|uint16(EAttachment::Palm)|uint16(EAttachment::Swords);
        const auto Preview=Presentation(I,G,Selected,Foreign);
        NoRecordLeak&=Preview.Grade==P.Grade&&Preview.Devices.Num()==P.Devices.Num();
    }
    // Index 4 is the legacy duplicate and has no variants; no presentation.
    Check(TEXT("v8446_all_catalogue_classes_project_without_account_reads"),GradeSelection);
    Check(TEXT("v8446_other_preview_class_does_not_borrow_recorded_devices"),NoRecordLeak);
    bool George=true;
    for(int32 I:{9,10})for(int32 G=1;G<=4;++G)
    {const auto P=Presentation(I,G,Selected);George&=P.HasRibbon()&&P.Devices.Num()==(G==2||G==4?1:0);}
    Check(TEXT("v8446_george_medal_and_cross_bows_only_third_and_first"),George);
    Check(TEXT("v8446_legion_all_five_real_grade_devices"),
        Presentation(8,1,Selected).Devices.IsEmpty()
        &&Device(Presentation(8,2,Selected),EDevice::Rosette,EMetal::RedTextile)
        &&Device(Presentation(8,3,Selected),EDevice::RosetteBacking,EMetal::Silver)
        &&Device(Presentation(8,4,Selected),EDevice::RosetteBacking,EMetal::SilverAndGold)
        &&Device(Presentation(8,5,Selected),EDevice::RosetteBacking,EMetal::Gold));
    Check(TEXT("v8446_only_verified_selected_class_ribbon_exceptions_are_hidden"),
        !Presentation(3,2,Selected).HasRibbon()&&!Presentation(31,1,Selected).HasRibbon()
        &&Presentation(3,3,Selected).Grade==3&&Presentation(18,3,Selected).Grade==3
        &&Presentation(20,3,Selected).Grade==3&&Presentation(31,2,Selected).Grade==2
        &&Presentation(31,3,Selected).Grade==3);
    Check(TEXT("v8446_earned_rack_retains_real_lower_breast_classes"),
        Presentation(3,3,Rack).Grade==1&&Presentation(18,3,Rack).Grade==1
        &&Presentation(20,6,Rack).Grade==2&&Presentation(31,4,Rack).Grade==2);
    FAward A;A.Grade=1;A.Repeats=2;A.Attachments=uint16(EAttachment::Bar);
    Check(TEXT("v8446_actual_repeat_bars_become_ribbon_rosettes"),
        Device(Presentation(1,1,Selected,A),EDevice::Rosette,EMetal::Silver,2)
        &&Device(Presentation(12,1,Selected,A),EDevice::Rosette,EMetal::Gold,2)
        &&Presentation(1,1,Selected).Devices.IsEmpty());
    bool RepeatCeiling=true;
    for(int32 I:{0,1,12,13,37,38,39})for(int32 N=0;N<=255;++N)
    {
        A.Repeats=uint8(N);const auto P=Presentation(I,1,Selected,A),R=Presentation(I,1,Rack,A);
        const int32 Count=FrontlineMedalsV8215::PresentedRepeatsV85(I,N);
        const auto Metal=I==12?EMetal::Gold:EMetal::Silver;
        RepeatCeiling&=(Count?Device(P,EDevice::Rosette,Metal,Count)&&Device(R,EDevice::Rosette,Metal,Count):P.Devices.IsEmpty()&&R.Devices.IsEmpty())&&A.Repeats==N;
    }
    Check(TEXT("v85_ribbon_all_legacy_repeat_counts_project_to_each_sourced_game_boundary"),RepeatCeiling);
    A.Repeats=1;
    Check(TEXT("v8446_victoria_ribbon_has_one_cross_per_real_award"),
        Device(Presentation(2,1,Selected),EDevice::MiniatureCross,EMetal::Bronze,1)
        &&Device(Presentation(2,1,Selected,A),EDevice::MiniatureCross,EMetal::Bronze,2));
    A.Repeats=0;A.Attachments=uint16(EAttachment::Clasp);
    Check(TEXT("v8446_1914_star_clasp_entitlement_uses_silver_rosette"),
        Device(Presentation(14,1,Selected,A),EDevice::Rosette,EMetal::Silver)
        &&Presentation(14,1,Selected).Devices.IsEmpty());
    A.Attachments=uint16(EAttachment::Star)|uint16(EAttachment::Palm);
    Check(TEXT("v8446_croix_de_guerre_citations_are_record_devices_not_grades"),
        Device(Presentation(6,1,Selected,A),EDevice::Star,EMetal::Unspecified)
        &&Device(Presentation(6,1,Selected,A),EDevice::Palm,EMetal::Bronze)
        &&Presentation(6,1,Selected).Devices.IsEmpty()&&!Presentation(6,2,Selected,A).HasRibbon());
    Check(TEXT("v8446_invalid_index_and_class_return_empty"),
        !Presentation(-1,1,Selected).HasRibbon()&&!Presentation(FrontlineMedalsV8215::Count,1,Selected).HasRibbon()
        &&!Presentation(8,0,Selected).HasRibbon()&&!Presentation(8,6,Selected).HasRibbon());
    Check(TEXT("v8446_known_order_textiles_do_not_fabricate_grade_devices"),
        Presentation(18,3,Selected).Devices.IsEmpty()&&Presentation(20,6,Selected).Devices.IsEmpty()
        &&Presentation(34,1,Selected).HasRibbon()==FrontlineMedalsV8215::Definition(34).Ribbon
        &&Presentation(35,1,Selected).HasRibbon()==FrontlineMedalsV8215::Definition(35).Ribbon
        &&Presentation(36,1,Selected).HasRibbon()==FrontlineMedalsV8215::Definition(36).Ribbon);
}
}
