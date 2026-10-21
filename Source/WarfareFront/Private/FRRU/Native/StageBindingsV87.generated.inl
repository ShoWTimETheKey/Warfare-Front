// Generated from Scripts/FRRU/stage-bindings-v87.json; do not edit.
// Source SHA256 1ef9109eb1adf84a2d6fa6670675668813658d052bbc99dcde8b32947cf0d790
void ApplyReceiverStageBindingsV87(std::vector<MissionDefinition>& missions)
{
    for(auto& mission:missions)if(mission.id=="ru.galicia")
        for(auto& stage:mission.stages)if(stage.id=="ru.galicia.stage.2")stage.followClearedPassageV87=true;
    for(auto& mission:missions)if(mission.id=="fr.louvemont")
        for(auto& stage:mission.stages)if(stage.id=="fr.louvemont.stage.2")stage.followClearedPassageV87=true;
    for(auto& mission:missions)if(mission.id=="ru.kovel")
        for(auto& stage:mission.stages)if(stage.id=="ru.kovel.stage.2")stage.followClearedPassageV87=true;
}
