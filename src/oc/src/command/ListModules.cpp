
#include "oc/Commands.hpp"
#include "oc/Module.hpp"
#include "oc/ModuleManager.hpp"

namespace oc {
    static const std::string no_summary = "...";

    void print_module(Log& log, const ModuleLoader<>& loader, Module* module);

    void print_module(Log& log, const ModuleLoader<>& loader, Module* module){
        log.info(module->name() + " (" + loader.path().string() + ")\n");
        const auto& cmds = module->command_modules();
        if (cmds.size()){
            log.info("\tCommands provided:\n");
            for (const auto& cmd : cmds){
                const auto& front = cmd->respond_to().front();
                auto* brief = cmd->brief_summary();
                if (!brief){
                    brief = &no_summary;
                }
                if (cmd->respond_to().size() == 1){
                    log.info("\t\t" + front + "    " + *brief + "\n");
                } else {
                    const auto& back = cmd->respond_to().back();
                    for (const auto& c : cmd->respond_to()){
                        if (c == front){
                            log.info("\t\t" + c + " (");
                        } else if (c == back){
                            log.info(c);
                        } else {
                            log.info(c + ", ");
                        }
                    }
                    log.info(")    " + *brief + "\n");
                }
            }
        }
    }
    int ListModulesCommand::call_command(int argc, const char* argv[]) const {
        (void)argc;
        (void)argv;
        Log& log = configuration_.log;
        for (const auto& m : configuration_.module_manager.get_loaders()){
            print_module(log, m, m.get_module().get());
        }
        return 0;
    }

} // namespace oc

