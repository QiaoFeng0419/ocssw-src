
#include "oc/CoreModule.hpp"
#include "oc/Module.hpp"

#include <memory>

namespace oc {

CoreModule::CoreModule(const ModuleConfiguration& configuration) : configuration_{configuration} {
    commands_.push_back(std::make_unique<ListCommandsCommand>(configuration_));
    commands_.push_back(std::make_unique<ListModulesCommand>(configuration_));
    commands_.push_back(std::make_unique<HelpCommand>(configuration_));
}

} // namespace oc

#ifdef WIN32
#define DECLSPEC __declspec \(dllexport\)
#else
#define DECLSPEC
#endif

extern "C" {
    DECLSPEC oc::CoreModule* oc_module_allocator(const oc::ModuleConfiguration& configuration);
    DECLSPEC void oc_module_deleter(oc::CoreModule *module);

    DECLSPEC oc::CoreModule* oc_module_allocator(const oc::ModuleConfiguration& configuration) {
        return new oc::CoreModule(configuration);
    }
    DECLSPEC void oc_module_deleter(oc::CoreModule *module) {
        delete module;
    }
}

