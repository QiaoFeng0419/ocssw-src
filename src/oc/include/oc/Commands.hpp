
#ifndef OC_COMMANDS
#define OC_COMMANDS

#include "Module.hpp"

namespace oc {
    class HelpCommand : public CommandModule {
        public:
            HelpCommand(const ModuleConfiguration& configuration) : configuration_{configuration} {}
            const std::vector<std::string>& respond_to() const override {
                static std::vector<std::string> commands{"help"};
                return commands;
            }
            const std::string* brief_summary() const override {
                static const std::string brief = "Print help on commands";
                return &brief;
            }
            int call_command(int argc, const char* argv[]) const override;
        private:
            const ModuleConfiguration& configuration_;

    };
    class ListCommandsCommand : public CommandModule {
        public:
            ListCommandsCommand(const ModuleConfiguration& configuration) : configuration_{configuration} {}
            const std::vector<std::string>& respond_to() const override {
                static std::vector<std::string> commands{"list-commands"};
                return commands;
            }
            const std::string* brief_summary() const override {
                static const std::string brief = "List all commands";
                return &brief;
            }

            int call_command(int argc, const char* argv[]) const override;
        private:
            const ModuleConfiguration& configuration_;

    };
    class ListModulesCommand : public CommandModule {
        public:
            ListModulesCommand(const ModuleConfiguration& configuration) : configuration_{configuration} {}
            const std::vector<std::string>& respond_to() const override {
                static std::vector<std::string> commands{"list-modules"};
                return commands;
            }
            const std::string* brief_summary() const override {
                static const std::string brief = "List all modules and what they provide";
                return &brief;
            }

            int call_command(int argc, const char* argv[]) const override;
        private:
            const ModuleConfiguration& configuration_;

    };
} // namespace oc

#endif // OC_COMMANDS

