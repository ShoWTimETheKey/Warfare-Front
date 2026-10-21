// Generated from Scripts/FRRU/armour-options-v87.json; do not hand edit.
// Source SHA256 fbc5a6fc91490067a1ed9790437221bed62f00b3dc2b18ad6710ada9cc80fd46
void AppendReceiverArmourOptionsV87(std::vector<OptionDefinition>& rows) {
    rows.push_back({"fr.armour.crew","passive_armour_crew","PassiveArmour","装甲乘员操练","",Faction::French,12,-1,2,{},true,19170416,19181231,0.84999999999999998,1.2});
    rows.push_back({"fr.armour.repair","passive_armour_repair","PassiveArmour","战地维修","",Faction::French,13,-1,2,{"fr.armour.crew"},true,19170416,19181231,0.75,1});
    rows.push_back({"fr.armour.prepare","passive_armour_preparation","PassiveArmour","装甲整备","",Faction::French,14,-1,3,{"fr.armour.crew"},true,19170416,19181231,0.84999999999999998,1});
    rows.push_back({"ru.armour.crew","passive_armour_crew","PassiveArmour","装甲乘员操练","",Faction::Russian,12,-1,2,{},true,19150501,19171231,0.80000000000000004,1.1499999999999999});
    rows.push_back({"ru.armour.repair","passive_armour_repair","PassiveArmour","战地维修","",Faction::Russian,13,-1,2,{"ru.armour.crew"},true,19150501,19171231,0.66666666666666663,1});
    rows.push_back({"ru.armour.prepare","passive_armour_preparation","PassiveArmour","装甲整备","",Faction::Russian,14,-1,3,{"ru.armour.crew"},true,19150501,19171231,0.80000000000000004,1});
}
