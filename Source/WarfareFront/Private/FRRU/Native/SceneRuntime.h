#pragma once
// Shared centimetre scene geometry; no actor, timer, physics world or file I/O.
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <vector>
namespace wfexp::scene {
struct Point { double x=0,y=0,z=0; };
enum class Surface : std::uint8_t { Firm, Mud, ShallowWater, DeepWater, Forest, Rubble, Snow, Invalid=255 };
struct Region { double minX,minY,maxX,maxY; Surface surface; double movementScale; };
struct Solid { Point minimum,maximum; bool blocksMovement,blocksSight; const char* kind; };
struct Deck { Point from,to; double halfWidth,topZ; const char* edgeId; };
// Receiver water records match authored geometry, rather than a terrain region.
// Outside continuations are visible scenery only and cannot create wet footing.
struct WaterPatch {
    const char* id; const char* groupId; std::uint8_t groupIndex;
    std::vector<Point> polygon; double levelCm; bool visualOnly;
};
struct WaterSurface {
    const char* sceneId=nullptr; std::uint32_t revision=0;
    const char* patchId=nullptr; const char* groupId=nullptr; std::uint8_t groupIndex=0;
    double levelCm=0,depthCm=0;
};
struct RoutePolyline { const char* id; const char* from; const char* to; std::vector<Point> points; bool traversable=false; };
struct SceneDefinition {
    const char* id; const char* missionId; std::uint32_t revision;
    int columns,rows; double minX,minY,maxX,maxY;
    const std::int16_t* heightsCm; std::size_t heightCount;
    std::vector<Region> regions; std::vector<Solid> solids; std::vector<Deck> decks;
    std::vector<WaterPatch> waterPatches{};
    std::vector<RoutePolyline> routes{};
    bool waterControlsFooting=false;
};
struct Footing { bool valid=false,walkable=false; double heightCm=0,movementScale=0; Surface surface=Surface::Invalid; };
const std::vector<SceneDefinition>& Scenes();
const SceneDefinition* ForMission(std::string_view missionId);
bool Contains(const SceneDefinition&,const Point&) noexcept;
bool HeightAt(const SceneDefinition&,double x,double y,double& height) noexcept;
// Exact lower-sphere contact against the same finite 00->11 ground triangles.
// The caller supplies its unchanged capsule dimensions and physical skin.
bool GroundCapsuleCentre(const SceneDefinition&,double x,double y,double radiusCm,double halfHeightCm,double skinCm,double& centreZ) noexcept;
bool QueryWaterSurface(const SceneDefinition&,double x,double y,WaterSurface& out) noexcept;
Footing FootingAt(const SceneDefinition&,double x,double y) noexcept;
bool LineOfSight(const SceneDefinition&,const Point& eye,const Point& target) noexcept;
bool CanTraverseSegment(const SceneDefinition&,const Point& from,const Point& to,double maximumSlope=.65) noexcept;
const RoutePolyline* FindRoute(const SceneDefinition&,std::string_view edgeId) noexcept;
bool CanTraverseRoute(const SceneDefinition&,std::string_view edgeId,const Point& from,const Point& to,double maximumSlope=.65) noexcept;
} // namespace wfexp::scene
