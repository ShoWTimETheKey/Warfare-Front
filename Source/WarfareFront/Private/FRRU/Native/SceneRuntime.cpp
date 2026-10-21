#include "SceneRuntime.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace wfexp::scene {
namespace {
bool Finite(const Point& p) noexcept { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
bool HeightfieldSegmentSlopeAllowed(const SceneDefinition& s,const Point& a,const Point& b,double length,double slope) noexcept {
    if(length==0)return true;
    const double dx=(s.maxX-s.minX)/(s.columns-1),dy=(s.maxY-s.minY)/(s.rows-1);
    if(!std::isfinite(dx)||!std::isfinite(dy)||dx<=0||dy<=0)return false;
    const double ax=(a.x-s.minX)/(s.maxX-s.minX)*(s.columns-1),ay=(a.y-s.minY)/(s.maxY-s.minY)*(s.rows-1);
    const double bx=(b.x-s.minX)/(s.maxX-s.minX)*(s.columns-1),by=(b.y-s.minY)/(s.maxY-s.minY)*(s.rows-1);
    // HeightAt uses finite 00->11 triangles. Their only boundaries are
    // integer grid X, grid Y and grid X-Y. Merge these three arithmetic
    // crossing streams without allocating or averaging across a short slope.
    struct Crossing {
        double start,delta;int line,direction;
        Crossing(double from,double to) noexcept:start(from),delta(to-from),line(0),direction((to>from)-(to<from)) {
            if(direction>0)line=static_cast<int>(std::floor(from))+1;
            else if(direction<0)line=static_cast<int>(std::ceil(from))-1;
        }
        double Next() const noexcept {return direction?(line-start)/delta:std::numeric_limits<double>::infinity();}
    };
    Crossing crossings[]={{ax,bx},{ay,by},{ax-ay,bx-by}};
    const double ux=(b.x-a.x)/length,uy=(b.y-a.y)/length;double current=0;
    // A segment inside the grid crosses at most twice the combined axis
    // counts. Advance every coincident stream, including rounded zero-width
    // intervals, and retain a finite fail-closed bound for malformed arithmetic.
    const int maximumCuts=2*(s.columns+s.rows)+4;
    for(int cut=0;cut<maximumCuts;++cut){
        const double next=std::min({1.0,crossings[0].Next(),crossings[1].Next(),crossings[2].Next()});
        if(!std::isfinite(next)||next<current)return false;
        if(next>current){
            const double middle=current+(next-current)*.5;
            const double gx=std::clamp(ax+(bx-ax)*middle,0.0,double(s.columns-1));
            const double gy=std::clamp(ay+(by-ay)*middle,0.0,double(s.rows-1));
            const int ix=std::min(s.columns-2,static_cast<int>(gx)),iy=std::min(s.rows-2,static_cast<int>(gy));
            const double tx=gx-ix,ty=gy-iy;
            const auto at=[&](int x,int y){return double(s.heightsCm[y*s.columns+x]);};
            const double h00=at(ix,iy),h10=at(ix+1,iy),h01=at(ix,iy+1),h11=at(ix+1,iy+1);
            const double gradientX=tx>=ty?(h10-h00)/dx:(h11-h01)/dx;
            const double gradientY=tx>=ty?(h11-h10)/dy:(h01-h00)/dy;
            const double directionalSlope=gradientX*ux+gradientY*uy;
            if(!std::isfinite(directionalSlope)||std::abs(directionalSlope)>slope)return false;
        }
        if(next>=1)return true;
        bool advanced=false;for(auto& crossing:crossings)if(crossing.direction&&crossing.Next()<=next){crossing.line+=crossing.direction;advanced=true;}
        if(!advanced)return false;current=next;
    }
    return false;
}
double SegmentDistanceSquared(double x,double y,const Deck& d) noexcept {
    const double dx=d.to.x-d.from.x,dy=d.to.y-d.from.y,den=dx*dx+dy*dy;
    const double t=den>0?std::clamp(((x-d.from.x)*dx+(y-d.from.y)*dy)/den,0.0,1.0):0;
    const double px=x-d.from.x-t*dx,py=y-d.from.y-t*dy;return px*px+py*py;
}
bool Trunk(const Solid& s) noexcept {return s.kind&&std::string_view(s.kind)=="tree_trunk";}
bool InsideSolid(double x,double y,const Solid& s) noexcept {
    if(x<s.minimum.x||x>s.maximum.x||y<s.minimum.y||y>s.maximum.y)return false;
    if(!Trunk(s))return true;
    const double cx=(s.minimum.x+s.maximum.x)/2,cy=(s.minimum.y+s.maximum.y)/2,r=(s.maximum.x-s.minimum.x)/2;
    constexpr double pi=3.14159265358979323846;const double apothem=r*std::cos(pi/20);
    for(int i=0;i<20;++i){const double a=(i+.5)*2*pi/20;if((x-cx)*std::cos(a)+(y-cy)*std::sin(a)>apothem+1.e-7)return false;}
    return true;
}
bool RayHitsSolid(const Point& a,const Point& b,const Solid& s) noexcept {
    double lo=0,hi=1;const double start[3]={a.x,a.y,a.z},end[3]={b.x,b.y,b.z};
    const double mn[3]={s.minimum.x,s.minimum.y,s.minimum.z},mx[3]={s.maximum.x,s.maximum.y,s.maximum.z};
    for(int axis=0;axis<3;++axis){const double delta=end[axis]-start[axis];if(std::abs(delta)<1.e-12){if(start[axis]<mn[axis]||start[axis]>mx[axis])return false;}
        else {double t1=(mn[axis]-start[axis])/delta,t2=(mx[axis]-start[axis])/delta;if(t1>t2)std::swap(t1,t2);lo=std::max(lo,t1);hi=std::min(hi,t2);if(lo>hi)return false;}}
    if(Trunk(s)){
        constexpr double pi=3.14159265358979323846;const double cx=(s.minimum.x+s.maximum.x)/2,cy=(s.minimum.y+s.maximum.y)/2,r=(s.maximum.x-s.minimum.x)/2,apothem=r*std::cos(pi/20);
        for(int i=0;i<20;++i){const double angle=(i+.5)*2*pi/20,nx=std::cos(angle),ny=std::sin(angle),distance=(a.x-cx)*nx+(a.y-cy)*ny,delta=(b.x-a.x)*nx+(b.y-a.y)*ny;
            if(std::abs(delta)<1.e-12){if(distance>apothem)return false;}else{const double t=(apothem-distance)/delta;if(delta>0)hi=std::min(hi,t);else lo=std::max(lo,t);if(lo>hi)return false;}}
    }
    return hi>1.e-6&&lo<1-1.e-6;
}
double CrossXY(const Point& a,const Point& b,const Point& p) noexcept {return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);}
bool InTriangleXY(const Point& a,const Point& b,const Point& c,const Point& p) noexcept {
    constexpr double tolerance=1.e-7;const double u=CrossXY(a,b,p),v=CrossXY(b,c,p),w=CrossXY(c,a,p);
    return (u>=-tolerance&&v>=-tolerance&&w>=-tolerance)||(u<=tolerance&&v<=tolerance&&w<=tolerance);
}
// Maximise z + sqrt(r*r - horizontalDistanceSquared) on a finite triangle.
// The plane's unconstrained tangent and each disk-clipped edge cover every
// maximum of this concave function; no infinite-plane extrapolation is used.
double TriangleSphereSupport(const Point& a,const Point& b,const Point& c,double x,double y,double radius) noexcept {
    const double determinant=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    if(std::abs(determinant)<1.e-12)return -std::numeric_limits<double>::infinity();
    const double sx=((b.z-a.z)*(c.y-a.y)-(c.z-a.z)*(b.y-a.y))/determinant;
    const double sy=((b.x-a.x)*(c.z-a.z)-(c.x-a.x)*(b.z-a.z))/determinant;
    const double norm=std::sqrt(1+sx*sx+sy*sy);Point tangent{x+radius*sx/norm,y+radius*sy/norm,0};
    double best=-std::numeric_limits<double>::infinity();
    if(InTriangleXY(a,b,c,tangent))best=a.z+sx*(x-a.x)+sy*(y-a.y)+radius*norm;
    const Point corners[3]={a,b,c};
    for(int edge=0;edge<3;++edge){const auto& p=corners[edge];const auto& q=corners[(edge+1)%3];
        const double length=std::hypot(q.x-p.x,q.y-p.y);if(length<=1.e-12)continue;
        const double ex=(q.x-p.x)/length,ey=(q.y-p.y)/length,projection=(p.x-x)*ex+(p.y-y)*ey;
        const double perpendicularSquared=std::max(0.0,(p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)-projection*projection);
        const double remaining=radius*radius-perpendicularSquared;if(remaining<0)continue;
        const double disk=std::sqrt(std::max(0.0,remaining));const double lo=std::max(0.0,-projection-disk),hi=std::min(length,-projection+disk);if(lo>hi)continue;
        const double gradient=(q.z-p.z)/length;
        const double along=std::clamp(gradient*disk/std::sqrt(1+gradient*gradient)-projection,lo,hi);
        const double root=std::sqrt(std::max(0.0,remaining-(projection+along)*(projection+along)));
        best=std::max(best,p.z+gradient*along+root);
    }
    return best;
}
#include "Scenes.generated.inl"
#include "MortHeightfieldV82.generated.inl"
#include "WorldHeightfieldsV82.generated.inl"
}
const std::vector<SceneDefinition>& Scenes(){
    // Integration amendment: R2's native quad and R3's preserved water geometry
    // are identical. Sealed generated height/region/solid data stays untouched.
    static const auto receiverScenes=[](){auto scenes=kScenes;
        for(auto& s:scenes){
            if(std::string_view(s.missionId)=="fr.mort_homme")
            {
                s.id="fr.mort_homme.scene.v3";s.revision=3;s.columns=513;s.rows=257;
                s.heightsCm=kMortReceiverHeightsV82;
                s.heightCount=sizeof(kMortReceiverHeightsV82)/sizeof(kMortReceiverHeightsV82[0]);
            }
            const std::string_view id=s.id;
            const bool authored=s.revision==2&&(id=="ru.galicia.scene.v2"||id=="ru.warsaw_rearguard.scene.v2"||id=="ru.kovel.scene.v2"||id=="ru.riga.scene.v2");
            if(!authored)continue;
            s.waterPatches.push_back({"r3.water.main","r3.water.connected",1,{{240,-6000,0},{1920,-6000,0},{1920,6000,0},{240,6000,0}},-22,false});
            s.waterPatches.push_back({"r3.water.outside.north","r3.water.connected",1,{{240,6000,0},{1920,6000,0},{1920,12000,0},{240,12000,0}},-22,true});
            s.waterPatches.push_back({"r3.water.outside.south","r3.water.connected",1,{{240,-12000,0},{1920,-12000,0},{1920,-6000,0},{240,-6000,0}},-22,true});
        }ApplyWorldRedesignV82(scenes);
        for(auto& s:scenes)for(auto& route:s.routes)
        {
            route.traversable=route.points.size()>=2&&route.points.size()<=128;
            for(std::size_t i=1;route.traversable&&i<route.points.size();++i)
                route.traversable=CanTraverseSegment(s,route.points[i-1],route.points[i]);
        }
        return scenes;}();
    return receiverScenes;
}
const SceneDefinition* ForMission(std::string_view id){for(const auto& s:Scenes())if(id==s.missionId)return &s;return nullptr;}
bool Contains(const SceneDefinition& s,const Point& p) noexcept {
    return Finite(p)&&s.columns>=2&&s.columns<=4096&&s.rows>=2&&s.rows<=4096&&s.heightsCm&&
        s.heightCount==static_cast<std::size_t>(s.columns)*static_cast<std::size_t>(s.rows)&&
        std::isfinite(s.minX)&&std::isfinite(s.maxX)&&std::isfinite(s.minY)&&std::isfinite(s.maxY)&&
        s.maxX>s.minX&&s.maxY>s.minY&&s.maxX-s.minX<=1000000&&s.maxY-s.minY<=1000000&&
        p.x>=s.minX&&p.x<=s.maxX&&p.y>=s.minY&&p.y<=s.maxY;
}
bool HeightAt(const SceneDefinition& s,double x,double y,double& out) noexcept {
    if(!Contains(s,{x,y,0})||s.columns<2||s.rows<2||!s.heightsCm||s.maxX<=s.minX||s.maxY<=s.minY)return false;
    const double gx=(x-s.minX)/(s.maxX-s.minX)*(s.columns-1),gy=(y-s.minY)/(s.maxY-s.minY)*(s.rows-1);
    const int ix=std::min(s.columns-2,static_cast<int>(gx)),iy=std::min(s.rows-2,static_cast<int>(gy));const double tx=gx-ix,ty=gy-iy;
    const auto at=[&](int cx,int cy){return static_cast<double>(s.heightsCm[cy*s.columns+cx]);};
    // Same 00->11 diagonal as the exported terrain triangles. A bilinear
    // surface would differ from the actual rendered/colliding triangle floor.
    out=tx>=ty ? (1-tx)*at(ix,iy)+(tx-ty)*at(ix+1,iy)+ty*at(ix+1,iy+1)
               : (1-ty)*at(ix,iy)+(ty-tx)*at(ix,iy+1)+tx*at(ix+1,iy+1);
    // Bridge caps/aprons are already baked into this authoritative grid.
    return std::isfinite(out);
}
bool GroundCapsuleCentre(const SceneDefinition& s,double x,double y,double radius,double halfHeight,double skin,double& out) noexcept {
    if(!Contains(s,{x,y,0})||!std::isfinite(radius)||radius<=0||radius>200||!std::isfinite(halfHeight)||halfHeight<radius||halfHeight>500||!std::isfinite(skin)||skin<0||skin>20)return false;
    const double dx=(s.maxX-s.minX)/(s.columns-1),dy=(s.maxY-s.minY)/(s.rows-1);
    const auto index=[](double coordinate,int maximum){return int(std::clamp(std::floor(coordinate),0.0,double(maximum)));};
    const int firstX=index((x-radius-s.minX)/dx,s.columns-2),lastX=index((x+radius-s.minX)/dx,s.columns-2);
    const int firstY=index((y-radius-s.minY)/dy,s.rows-2),lastY=index((y+radius-s.minY)/dy,s.rows-2);
    if((lastX-firstX+1)*(lastY-firstY+1)>4096)return false;
    double best=-std::numeric_limits<double>::infinity();
    const auto vertex=[&](int ix,int iy){return Point{s.minX+ix*dx,s.minY+iy*dy,double(s.heightsCm[iy*s.columns+ix])};};
    for(int iy=firstY;iy<=lastY;++iy)for(int ix=firstX;ix<=lastX;++ix){const Point a=vertex(ix,iy),b=vertex(ix+1,iy),c=vertex(ix+1,iy+1),d=vertex(ix,iy+1);
        best=std::max(best,TriangleSphereSupport(a,b,c,x,y,radius));best=std::max(best,TriangleSphereSupport(a,c,d,x,y,radius));}
    const double candidate=best+halfHeight-radius+skin;if(!std::isfinite(candidate))return false;out=candidate;return true;
}
bool QueryWaterSurface(const SceneDefinition& s,double x,double y,WaterSurface& out) noexcept {
    if(!Contains(s,{x,y,0}))return false;
    double floor=0;if(!HeightAt(s,x,y,floor))return false;
    for(const auto& patch:s.waterPatches){
        // The retained Mort-Homme revision has one authored 66-point pool.
        // Keep its exact shoreline and identity; the ordinary 64-point budget
        // still rejects unregistered oversized polygons. No fixed-size buffer
        // is used by the winding query below.
        const std::size_t polygonBudget=s.missionId&&s.id&&patch.id&&patch.groupId
            &&std::string_view(s.missionId)=="fr.mort_homme"
            &&std::string_view(s.id)=="fr.mort_homme.scene.redesign.v1"&&s.revision==4
            &&std::string_view(patch.id)=="mort.puddle.6"&&std::string_view(patch.groupId)=="mort.puddle.6"
            &&patch.groupIndex==6?128u:64u;
        if(patch.visualOnly||!patch.id||!patch.groupId||patch.groupIndex==0||patch.polygon.size()<3||patch.polygon.size()>polygonBudget||!std::isfinite(patch.levelCm)||patch.levelCm<=floor+.1)continue;
        bool inside=false,boundary=false,valid=true;
        for(std::size_t i=0,j=patch.polygon.size()-1;i<patch.polygon.size();j=i++){
            const auto& a=patch.polygon[j];const auto& b=patch.polygon[i];if(!Finite(a)||!Finite(b)){valid=false;break;}
            const double cross=(x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);
            const double scale=std::max(1.0,std::hypot(b.x-a.x,b.y-a.y));
            if(std::abs(cross)<=1.e-7*scale&&x>=std::min(a.x,b.x)-1.e-7&&x<=std::max(a.x,b.x)+1.e-7&&y>=std::min(a.y,b.y)-1.e-7&&y<=std::max(a.y,b.y)+1.e-7)boundary=true;
            if((a.y>y)!=(b.y>y)&&x<(b.x-a.x)*(y-a.y)/(b.y-a.y)+a.x)inside=!inside;
        }
        if(!valid||(!inside&&!boundary))continue;
        WaterSurface candidate{s.id,s.revision,patch.id,patch.groupId,patch.groupIndex,patch.levelCm,patch.levelCm-floor};
        if(!std::isfinite(candidate.depthCm))return false;out=candidate;return true;
    }
    return false;
}
Footing FootingAt(const SceneDefinition& s,double x,double y) noexcept {
    Footing f;if(!HeightAt(s,x,y,f.heightCm))return f;f.valid=true;f.surface=Surface::Firm;f.movementScale=1;
    for(const auto& r:s.regions)if(x>=r.minX&&x<=r.maxX&&y>=r.minY&&y<=r.maxY){f.surface=r.surface;f.movementScale=r.movementScale;}
    if(s.waterControlsFooting)
    {
        WaterSurface water;
        if(QueryWaterSurface(s,x,y,water))
        {f.surface=water.depthCm>65?Surface::DeepWater:Surface::ShallowWater;f.movementScale=water.depthCm>65?0:.72;}
    }
    for(const auto& d:s.decks)if(SegmentDistanceSquared(x,y,d)<=d.halfWidth*d.halfWidth){f.surface=Surface::Firm;f.movementScale=1;}
    f.walkable=f.surface!=Surface::DeepWater&&f.surface!=Surface::Invalid&&f.movementScale>0;
    for(const auto& solid:s.solids)if(solid.blocksMovement&&InsideSolid(x,y,solid))f.walkable=false;
    return f;
}
bool LineOfSight(const SceneDefinition& s,const Point& a,const Point& b) noexcept {
    if(!Contains(s,a)||!Contains(s,b))return false;
    for(const auto& solid:s.solids)if(solid.blocksSight&&RayHitsSolid(a,b,solid))return false;
    const double length=std::hypot(b.x-a.x,b.y-a.y);if(!std::isfinite(length)||length>100000)return false;
    const int steps=std::max(1,static_cast<int>(std::ceil(length/40)));
    for(int i=1;i<steps;++i){const double t=static_cast<double>(i)/steps;double floor=0;if(!HeightAt(s,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,floor)||a.z+(b.z-a.z)*t<=floor+2)return false;}
    return true;
}
bool CanTraverseSegment(const SceneDefinition& s,const Point& a,const Point& b,double slope) noexcept {
    if(!Contains(s,a)||!Contains(s,b)||!std::isfinite(slope)||slope<0||slope>2)return false;
    const double length=std::hypot(b.x-a.x,b.y-a.y);if(!std::isfinite(length)||length>100000)return false;
    if(!HeightfieldSegmentSlopeAllowed(s,a,b,length,slope))return false;
    const int steps=std::max(1,static_cast<int>(std::ceil(length/25)));double prior=0;
    for(int i=0;i<=steps;++i){const double t=static_cast<double>(i)/steps;const auto f=FootingAt(s,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t);if(!f.valid||!f.walkable)return false;
        if(i>0&&length>1.e-6&&std::abs(f.heightCm-prior)/(length/steps)>slope)return false;prior=f.heightCm;}
    return true;
}
const RoutePolyline* FindRoute(const SceneDefinition& s,std::string_view id) noexcept {
    for(const auto& route:s.routes)if(route.id&&id==route.id)return &route;return nullptr;
}
bool CanTraverseRoute(const SceneDefinition& s,std::string_view id,const Point& from,const Point& to,double slope) noexcept {
    const auto* route=FindRoute(s,id);if(!route)return CanTraverseSegment(s,from,to,slope);
    if(route->points.size()<2||route->points.size()>128)return false;
    const auto same=[](const Point& a,const Point& b){return std::hypot(a.x-b.x,a.y-b.y)<=.001&&std::abs(a.z-b.z)<=.001;};
    const bool forward=same(from,route->points.front())&&same(to,route->points.back());
    const bool reverse=same(to,route->points.front())&&same(from,route->points.back());
    if(!forward&&!reverse)return false;
    if(slope==.65)return route->traversable;
    for(std::size_t i=1;i<route->points.size();++i)if(!CanTraverseSegment(s,route->points[i-1],route->points[i],slope))return false;
    return true;
}
} // namespace wfexp::scene
