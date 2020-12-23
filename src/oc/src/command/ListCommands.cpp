
#include "oc/Commands.hpp"
#include "oc/Module.hpp"
#include "oc/ModuleManager.hpp"

#include <libintl.h>
#include <locale.h>

constexpr auto &_ = ::gettext;

namespace oc {
    static const std::string no_summary = "...";

    int ListCommandsCommand::call_command(int argc, const char* argv[]) const {
        (void)argc;
        (void)argv;
        Log& log = configuration_.log;
        // log.info("Available commands:\n");
        log.info(_("Available commands:\n"));
        for (const auto& m : configuration_.module_manager.get_loaders()){
            const auto& cmds = m.get_module()->command_modules();
            if (cmds.size()){
                for (const auto& cmd : cmds){
                    const auto& front = cmd->respond_to().front();
                    auto* brief = cmd->brief_summary();
                    if (!brief){
                        brief = &no_summary;
                    }
                    if (cmd->respond_to().size() == 1){
                        log.info("\t" + front + "    " + *brief + "\n");
                    } else {
                        const auto& back = cmd->respond_to().back();
                        for (const auto& c : cmd->respond_to()){
                            if (c == front){
                                log.info("\t" + c + " (");
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
        return 0;
    }

} // namespace oc

