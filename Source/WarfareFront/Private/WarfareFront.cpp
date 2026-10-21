#include "Modules/ModuleManager.h"
#include "WarfareFrontStartupV67.h"

class FWarfareFrontModule final : public FDefaultGameModuleImpl
{
    void StartupModule() override { FrontlineStartupV67::Begin(); }
    void ShutdownModule() override { FrontlineStartupV67::Shutdown(); }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FWarfareFrontModule, WarfareFront, "WarfareFront");
