

#include "oc.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

using namespace oc;

int main(int argc, const char *argv[]) {
    (void)argc;
    (void)argv;

    std::unique_ptr<ModuleManager> module_manager;

    Log& log = Log::get_default();

    char* module_path = getenv("OC_MODULE_DIRECTORY");
    if (module_path != nullptr){
        module_manager = std::make_unique<ModuleManager>(module_path, log);
    } else {
        module_manager = std::make_unique<ModuleManager>(log);
    }
    module_manager->load_modules();

    if (argc > 1){
        const char* command = argv[1];
        for (const auto& module : module_manager->get_modules()){
            const auto& cmds = module->command_modules();
            for (const auto& cmd : cmds){
                if (std::find(cmd->respond_to().cbegin(), cmd->respond_to().cend(), command) != cmd->respond_to().end()){
                    return cmd->call_command(argc + 1, &argv[1]);
                }
            }
        }
        log.error("Command not found: " + std::string(command) + "\n");
    } else {
        log.error("No command found\n");
    }

    return 0;
}
