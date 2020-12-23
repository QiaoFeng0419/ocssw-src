#ifndef OC_COREMODULE
#define OC_COREMODULE

#include "oc/Module.hpp"

#include "oc/Commands.hpp"

#include <iostream>
#include <string>

namespace oc {
    class CoreModule : public Module {
    public:
        CoreModule(const ModuleConfiguration& configuration);
        ~CoreModule() override = default;

        const std::string& name() const override {return name_;}
        const std::vector<std::unique_ptr<CommandModule>>& command_modules() const override {return commands_;}
    private:
        const ModuleConfiguration& configuration_;
        std::string name_{"Core"};
        std::vector<std::unique_ptr<CommandModule>> commands_{};
    };
}

#endif // OC_COREMODULE
