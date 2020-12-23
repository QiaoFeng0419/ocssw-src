
#include "oc/Commands.hpp"
#include "oc/Module.hpp"
#include "oc/ModuleManager.hpp"

namespace oc {
    int HelpCommand::call_command(int argc, const char* argv[]) const {
        (void)argc;
        (void)argv;
        auto& log = configuration_.log;
        log.info("Super helpful messages to follow.\n");
        return 0;
    }
} // namespace oc

