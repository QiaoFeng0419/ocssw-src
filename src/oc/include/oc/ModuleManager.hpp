
#ifndef OC_MODULEMANAGER
#define OC_MODULEMANAGER

#include "Module.hpp"

#include <boost/filesystem.hpp>

#include <memory>

namespace oc {
    class ModuleManager {
    public:
        ModuleManager(Log& log);
        ModuleManager(const boost::filesystem::path& module_directory, Log& log);
        ~ModuleManager();

        void load_modules();
        const std::vector<std::shared_ptr<Module>>& get_modules() const;
        const std::vector<ModuleLoader<>>& get_loaders() const;

        static const boost::filesystem::path default_module_directory;
    private:
        boost::filesystem::path module_directory_{default_module_directory};
        std::vector<std::shared_ptr<Module>> modules_{};
        std::vector<ModuleLoader<>> module_loaders_{};
        bool is_loaded_{false};
        Log& log_;
        ModuleConfiguration configuration_;
    };
}

#endif // OC_MODULEMANAGER
