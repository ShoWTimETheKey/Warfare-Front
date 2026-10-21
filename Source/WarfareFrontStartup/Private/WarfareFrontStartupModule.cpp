#include "Modules/ModuleManager.h"
#include "WarfareFrontStartupV67.h"

class FWarfareFrontStartupModule final : public IModuleInterface
{
    virtual void StartupModule() override { FrontlineStartupV67::AttachSlate(); }
};
IMPLEMENT_MODULE(FWarfareFrontStartupModule,WarfareFrontStartup)
