#pragma once
// Compile-independent adapter contract. Include from a host game-thread module.
// No UObject/tick/timer/damage/ammunition/spawn authority is introduced here.
#include "TacticalRuntime.h"
#include <cmath>
namespace wfexp::tactical {
class AtomicHostBatch {
public:
    virtual ~AtomicHostBatch()=default;
    // Validate every intent against ordinary recruitment/trench/weapon/support
    // safety and the captured actor context. Must not mutate anything.
    virtual Result Preflight(const Snapshot&,const std::vector<Intent>&)=0;
    // Synchronous same-frame, all-or-none commit. Implementations must reserve
    // all destination slots/support budget before mutating any existing actor.
    // Neither function may pump the world, dispatch async work, or increment
    // Elapsed. Return success only after EVERY intent has been accepted.
    virtual Result Commit(const Snapshot&,const std::vector<Intent>&)=0;
};
inline Result Execute(State& state,const Snapshot& captured,const Command& command,AtomicHostBatch& host){
    const auto prepared=Prepare(state,captured,command);
    if(!prepared.View().allowed)return {false,prepared.View().reason};
    if(prepared.View().duplicate)return Acknowledge(state,captured,prepared,{});
    const auto before=StateSignature(state);
    auto result=host.Preflight(captured,prepared.View().intents);
    if(!result)return result;
    if(StateSignature(state)!=before)return {false,"Host preflight reentered or mutated tactical context"};
    result=host.Commit(captured,prepared.View().intents);
    if(!result)return result;
    // Host guarantees it did not reenter this reducer or replace captured input.
    std::vector<std::string> accepted;for(const auto& intent:prepared.View().intents)accepted.push_back(intent.id);
    return Acknowledge(state,captured,prepared,accepted);
}
inline Result CanonicalPosition(const scene::Point& world,int playerDirection,scene::Point& output){
    if((playerDirection!=1&&playerDirection!=-1)||!std::isfinite(world.x)||!std::isfinite(world.y)||!std::isfinite(world.z))return {false,"Invalid travel direction or world position"};
    output={world.x*playerDirection,world.y,world.z};return {true,{}};
}
} // namespace wfexp::tactical
