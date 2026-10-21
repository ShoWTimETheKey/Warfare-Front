#include "SceneRuntime.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace wfexp::scene;
namespace {
int checks=0;
std::vector<std::string> failures;
void Check(const std::string& name,bool ok){++checks;if(!ok){failures.push_back(name);std::cerr<<"FAIL "<<name<<'\n';}}
SceneDefinition Fixture(const char* id,const std::int16_t* heights,std::size_t count,int columns=2,int rows=2){
    SceneDefinition s{};s.id=id;s.missionId="triangle.fixture";s.revision=1;s.columns=columns;s.rows=rows;
    s.minX=s.minY=0;s.maxX=s.maxY=100;s.heightsCm=heights;s.heightCount=count;return s;
}
Point Along(const Point& a,const Point& b,double fraction){return {a.x+(b.x-a.x)*fraction,a.y+(b.y-a.y)*fraction,0};}
void CheckPartition(const std::string& name,const SceneDefinition& s,const Point& a,const Point& b){
    const bool whole=CanTraverseSegment(s,a,b);
    Check(name+"_reverse",whole==CanTraverseSegment(s,b,a));
    for(double fraction:{.01,.05,.1,.37,.5,.9,.99}){
        const Point middle=Along(a,b,fraction);
        const bool forward=CanTraverseSegment(s,a,middle)&&CanTraverseSegment(s,middle,b);
        const bool reverse=CanTraverseSegment(s,b,middle)&&CanTraverseSegment(s,middle,a);
        Check(name+"_partition_"+std::to_string(fraction),whole==forward&&whole==reverse);
    }
}
void String(std::ostream& out,std::string_view value){
    out<<'"';for(char c:value){if(c=='"'||c=='\\')out<<'\\';out<<c;}out<<'"';
}
struct RouteReadback {std::string mission,id;bool forward=false,reverse=false;std::vector<std::size_t> rejectedSegments;};
struct RouteCounts {int missions=0,routes=0,forward=0,reverse=0,both=0,asymmetric=0;};
void Counts(std::ostream& out,const RouteCounts& counts){
    out<<"{\"missions\":"<<counts.missions<<",\"routes\":"<<counts.routes<<",\"forwardPassed\":"<<counts.forward
        <<",\"reversePassed\":"<<counts.reverse<<",\"bothPassed\":"<<counts.both<<",\"asymmetric\":"<<counts.asymmetric<<'}';
}
}
int main(int argc,char** argv){
    const std::int16_t flat[]={0,0,0,0},limit[]={0,65,0,65},steep[]={0,66,0,66},fold[]={0,0,10,0};
    auto f=Fixture("flat",flat,4),p=Fixture("limit",limit,4),q=Fixture("steep",steep,4),d=Fixture("fold",fold,4);
    d.maxX=d.maxY=20;
    Check("flat_diagonal",CanTraverseSegment(f,{0,0,0},{100,100,0}));
    Check("exact_065_slope_forward",CanTraverseSegment(p,{0,50,0},{100,50,0}));
    Check("exact_065_slope_reverse",CanTraverseSegment(p,{100,50,0},{0,50,0}));
    Check("066_slope_rejected_both_directions",!CanTraverseSegment(q,{0,50,0},{100,50,0})&&!CanTraverseSegment(q,{100,50,0},{0,50,0}));
    Check("tiny_steep_step_rejected",!CanTraverseSegment(q,{50,50,0},{50.000000001,50,0}));
    Check("zero_length_retains_valid_footing",CanTraverseSegment(q,{50,50,0},{50,50,0}));
    Check("zero_length_outside_rejected",!CanTraverseSegment(f,{-1,50,0},{-1,50,0}));
    Check("sub25cm_diagonal_fold_rejected",!CanTraverseSegment(d,{0,16,0},{16,0,0}));
    CheckPartition("flat",f,{0,0,0},{100,100,0});
    CheckPartition("diagonal_fold",d,{0,16,0},{16,0,0});
    const std::int16_t grid[]={0,0,0,0,0,0,0,0,0};const auto g=Fixture("grid",grid,9,3,3);
    const Point starts[]={{0,0,0},{50,0,0},{0,50,0},{0,100,0},{0,0,0}};
    const Point ends[]={{100,100,0},{50,100,0},{100,50,0},{100,0,0},{100,99.9999999999,0}};
    for(std::size_t i=0;i<5;++i){
        Check("grid_boundary_forward_"+std::to_string(i),CanTraverseSegment(g,starts[i],ends[i]));
        Check("grid_boundary_reverse_"+std::to_string(i),CanTraverseSegment(g,ends[i],starts[i]));
    }
    auto water=f;water.regions.push_back({0,0,100,100,Surface::DeepWater,0});
    Check("deep_water_footing_still_rejected",!CanTraverseSegment(water,{0,50,0},{100,50,0}));
    Check("zero_length_deep_water_still_rejected",!CanTraverseSegment(water,{50,50,0},{50,50,0}));
    auto solid=f;solid.solids.push_back({{49,49,0},{51,51,200},true,true,"fixture_wall"});
    Check("sampled_solid_still_rejected",!CanTraverseSegment(solid,{0,0,0},{100,100,0}));
    Check("nan_slope_rejected",!CanTraverseSegment(f,{0,0,0},{100,100,0},std::numeric_limits<double>::quiet_NaN()));
    Check("negative_slope_rejected",!CanTraverseSegment(f,{0,0,0},{100,100,0},-.1));

    double actualFirstCmSlope=0,actualLegacySampleMaximum=0;
    const auto* chemin=ForMission("fr.chemin_dames");Check("actual_chemin_scene_present",chemin!=nullptr);
    if(chemin){
        const Point from{5308.097,-1575.800,33.386},to{5098,-1606,43};
        const double length=std::hypot(to.x-from.x,to.y-from.y);double startHeight=0,endHeight=0;
        Check("actual_chemin_frozen_start_height",HeightAt(*chemin,from.x,from.y,startHeight)&&std::abs(startHeight-33.386773333331945)<1.e-6);
        Check("actual_chemin_long_leg_rejected",!CanTraverseSegment(*chemin,from,to));
        Check("actual_chemin_reverse_long_leg_rejected",!CanTraverseSegment(*chemin,to,from));
        for(double prefix:{1.e-8,1.,2.,5.,10.,20.,40.,80.}){
            const Point next=Along(from,to,prefix/length);
            Check("actual_chemin_prefix_rejected_"+std::to_string(prefix),!CanTraverseSegment(*chemin,from,next)&&!CanTraverseSegment(*chemin,next,from));
        }
        const Point first=Along(from,to,1./length);
        Check("actual_first_cm_height_available",HeightAt(*chemin,first.x,first.y,endHeight));
        actualFirstCmSlope=std::abs(endHeight-startHeight);
        Check("actual_first_cm_exceeds_unchanged_065",actualFirstCmSlope>.65&&std::abs(actualFirstCmSlope-.732999692584)<1.e-6);
        const int samples=static_cast<int>(std::ceil(length/25.));double prior=startHeight;
        for(int i=1;i<=samples;++i){const Point next=Along(from,to,double(i)/samples);double height=0;
            Check("actual_legacy_sample_height_"+std::to_string(i),HeightAt(*chemin,next.x,next.y,height));
            actualLegacySampleMaximum=std::max(actualLegacySampleMaximum,std::abs(height-prior)/(length/samples));prior=height;}
        Check("actual_original_25cm_sampling_missed_short_slope",actualLegacySampleMaximum<.65);
        Check("actual_legal_tail_retained",CanTraverseSegment(*chemin,Along(from,to,20./length),to));
        CheckPartition("actual_chemin",*chemin,from,to);
    }

    RouteCounts french,russian;std::vector<RouteReadback> frenchRoutes;
    for(const auto& scene:Scenes()){
        const std::string_view mission=scene.missionId;const bool isFrench=mission.rfind("fr.",0)==0,isRussian=mission.rfind("ru.",0)==0;
        if(!isFrench&&!isRussian)continue;auto& counts=isFrench?french:russian;++counts.missions;
        if(isFrench)Check(std::string(mission)+"_has_registered_routes",!scene.routes.empty());
        for(const auto& route:scene.routes){
            ++counts.routes;const bool usable=route.points.size()>=2;
            const bool forward=usable&&CanTraverseRoute(scene,route.id,route.points.front(),route.points.back());
            const bool reverse=usable&&CanTraverseRoute(scene,route.id,route.points.back(),route.points.front());
            counts.forward+=forward;counts.reverse+=reverse;counts.both+=forward&&reverse;counts.asymmetric+=forward!=reverse;
            if(isFrench){
                Check(std::string(mission)+"/"+route.id+"_registered_route_both_directions",forward&&reverse);
                RouteReadback row{std::string(mission),route.id,forward,reverse,{}};
                for(std::size_t i=1;i<route.points.size();++i)
                    if(!CanTraverseSegment(scene,route.points[i-1],route.points[i])||!CanTraverseSegment(scene,route.points[i],route.points[i-1]))row.rejectedSegments.push_back(i-1);
                frenchRoutes.push_back(std::move(row));
            }
        }
    }
    Check("all_ten_french_scenes_inspected",french.missions==10);
    std::ofstream file;std::ostream* output=&std::cout;if(argc>1){file.open(argv[1]);if(!file){std::cerr<<"Cannot open report\n";return 3;}output=&file;}
    auto& out=*output;out<<std::setprecision(17)<<std::boolalpha;
    out<<"{\"revision\":\"v87.native.triangle-traversal.1\",\"checks\":"<<checks<<",\"allPassed\":"<<failures.empty()
        <<",\"scope\":\"Compiled native API fixtures and French registered routes; Russian route counts only. No UE, physical actor, campaign win, art or Shipping acceptance.\",\"ueMovementAccepted\":false,\"shippingAccepted\":false"
        <<",\"actualCheminFirstCmSlope\":"<<actualFirstCmSlope<<",\"actualCheminLegacy25cmMaximum\":"<<actualLegacySampleMaximum<<",\"french\":";Counts(out,french);
    out<<",\"russianCompatibilityCountsOnly\":";Counts(out,russian);out<<",\"frenchRoutes\":[";
    for(std::size_t i=0;i<frenchRoutes.size();++i){if(i)out<<',';const auto& route=frenchRoutes[i];out<<"{\"mission\":";String(out,route.mission);out<<",\"route\":";String(out,route.id);
        out<<",\"forward\":"<<route.forward<<",\"reverse\":"<<route.reverse<<",\"rejectedSegmentStartIndices\":[";
        for(std::size_t j=0;j<route.rejectedSegments.size();++j){if(j)out<<',';out<<route.rejectedSegments[j];}out<<"]}";}
    out<<"],\"failures\":[";for(std::size_t i=0;i<failures.size();++i){if(i)out<<',';String(out,failures[i]);}out<<"]}\n";
    std::cerr<<"Native triangle traversal: "<<checks<<" checks, "<<failures.size()<<" failures; French "<<french.both<<'/'<<french.routes
        <<" routes bidirectional; Russian counts only "<<russian.both<<'/'<<russian.routes<<'\n';return failures.empty()?0:1;
}
