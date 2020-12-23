
#include "oc/Configuration.hpp"
#include "oc/Log.hpp"
#include "oc/ModuleManager.hpp"

#include <boost/filesystem.hpp>

#include <iostream>
#include <memory>
#include <string>

// silence editor that doesn't know CMake defined this
#ifndef OC_MODULE_DIRECTORY
#define OC_MODULE_DIRECTORY ""
#endif

namespace oc {

const boost::filesystem::path ModuleManager::default_module_directory = OC_MODULE_DIRECTORY;

ModuleManager::ModuleManager(const boost::filesystem::path& module_directory, Log& log) : module_directory_{module_directory}, log_{log}, configuration_{*this, log_} {}
ModuleManager::ModuleManager(Log& log) : ModuleManager(default_module_directory, log) {}

ModuleManager::~ModuleManager(){
    modules_.clear();
    module_loaders_.clear();
}

void ModuleManager::load_modules(){
    log_ << oc::LogSeverity::warning << "Opening module directory " << module_directory_ << "\n";
    size_t module_count = 0;
    for(const auto &p : boost::filesystem::directory_iterator(module_directory_)){
        (void)p;
        ++module_count;
    }
    module_loaders_.reserve(module_count);
    modules_.reserve(module_count);

    for(const auto& p : boost::filesystem::directory_iterator(module_directory_)){
        if (ModuleLoader<>::is_module(p.path())){
            module_loaders_.emplace_back(p.path(), configuration_);
            module_loaders_.back().load_module();
            modules_.emplace_back(module_loaders_.back().get_module());
        }
    }
}
const std::vector<std::shared_ptr<Module>>& ModuleManager::get_modules() const {
    return modules_;
}
const std::vector<ModuleLoader<>>& ModuleManager::get_loaders() const {
    return module_loaders_;
}

} // namespace oc
