
#ifndef OC_MODULE
#define OC_MODULE

#include "Log.hpp"

#include <boost/filesystem.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace oc {
    static const std::string module_allocator_function = "oc_module_allocator";
    static const std::string module_deleter_function = "oc_module_deleter";

    class ModuleManager;
    struct ModuleConfiguration {
        ModuleConfiguration(ModuleManager& module_manager_, Log& log_) : module_manager{module_manager_}, log{log_} {}
        ModuleManager& module_manager;
        Log& log;
    };

    class CommandModule {
    public:
        virtual ~CommandModule() = default;
        virtual const std::vector<std::string>& respond_to() const = 0;
        virtual int call_command(int argc, const char* argv[]) const = 0;
        virtual const std::string* brief_summary() const {return nullptr;}
        virtual const std::string* group() const {return nullptr;}
    };

    class Module {
    public:
        virtual ~Module() = default;

        virtual const std::string& name() const  = 0;
        virtual const std::vector<std::unique_ptr<CommandModule>>& command_modules() const {return no_commands_;}
    private:
        std::vector<std::unique_ptr<CommandModule>> no_commands_{};
    };
}

// Nothing below here is for user consumption.

namespace oc {
    // An Interface class isn't strictly necessary, but it keeps the API spec above the platform specific code.
    template <class T=Module>
    class ModuleLoaderInterface {
    public:
        static bool is_module(const boost::filesystem::path& path);

        virtual ~ModuleLoaderInterface() = default;
        virtual void load_module() = 0;
        virtual std::shared_ptr<T> get_module() const = 0;
        virtual void unload_module() = 0;
        virtual bool is_loaded() const = 0;
        virtual const boost::filesystem::path& path() const = 0;
    };
    template <class T=Module> class ModuleLoader;
}


// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************
//
// #    # ###### #####  ######    #####  ######    #####  #####    ##    ####   ####  #    #  ####
// #    # #      #    # #         #    # #         #    # #    #  #  #  #    # #    # ##   # #
// ###### #####  #    # #####     #####  #####     #    # #    # #    # #      #    # # #  #  ####
// #    # #      #####  #         #    # #         #    # #####  ###### #  ### #    # #  # #      #
// #    # #      #   #  #         #    # #         #    # #   #  #    # #    # #    # #   ## #    #
// #    # ###### #    # ######    #####  ######    #####  #    # #    #  ####   ####  #    #  ####
// (Platform-specific implementations of ModuleLoaders.)


// also works for OSX, do I need to update this ifdef?
#if __linux__ || __APPLE__ || __unix__

#include <dlfcn.h>

namespace oc {
    template <class T>
    class ModuleLoader : public ModuleLoaderInterface<T> {
    public:
        static bool is_module(const boost::filesystem::path& path){
            return path.extension() == ".so"; // should actually check lib*.so
        }

        ModuleLoader(const boost::filesystem::path& path, const ModuleConfiguration& configuration) : path_(path), configuration_{configuration} {}

        ~ModuleLoader(){
            unload_module();
        }

        void load_module() {
            if (!handle_){
                if (!(handle_ = dlopen(path_.c_str(), RTLD_NOW | RTLD_LAZY))) {
                    std::cerr << dlerror() << '\n';
                } else {
                    using allocator_class = T* (*)(const ModuleConfiguration&);
                    using deleter_class = void (*)(T*);

                    auto allocator_function = reinterpret_cast<allocator_class>(dlsym(handle_, module_allocator_function.c_str()));
                    if (!allocator_function) {
                        unload_module();
                        std::cerr << "Can't find allocator symbol in " << path_ << '\n';
                        return;
                    }

                    auto deleter_function = reinterpret_cast<deleter_class>(dlsym(handle_, module_deleter_function.c_str()));
                    if (!deleter_function) {
                        unload_module();
                        std::cerr << "Can't find deleter symbol in " << path_ << '\n';
                        return;
                    }

                    module_ = std::shared_ptr<T>(allocator_function(configuration_), [deleter_function](T* p) noexcept { deleter_function(p); });
                }
            }
        }

        std::shared_ptr<T> get_module() const override {
            return module_;
        }

        void unload_module() override {
            module_.reset();
            if (handle_){
                if (dlclose(handle_) != 0) {
                    handle_ = nullptr;
                    std::cerr << dlerror() << '\n';
                }
            }
        }

        const boost::filesystem::path& path() const override {
            return path_;
        }
        bool is_loaded() const {
            return module_.get() != nullptr;
        }
    private:
        boost::filesystem::path path_;
        const ModuleConfiguration& configuration_;
        void* handle_{nullptr};
        std::shared_ptr<T> module_;
    };
}
#endif // __linux__

// #if 0
#ifdef WIN32

#include <windows.h>

namespace oc {
    template <class T>
    class ModuleLoader : public ModuleLoaderInterface<T> {
    public:
        static bool is_module(const boost::filesystem::path& path){
            return path.extension() == ".dll";
        }

        ModuleLoader(const boost::filesystem::path& path, const ModuleConfiguration& configuration) : path_(path), configuration_{configuration} {}

        ~ModuleLoader(){
            unload_module();
        }

        void load_module() override {
            if (!handle_){
                if (!(handle_ = LoadLibrary(path_.string().c_str()))) {
                    std::cerr << "Can't open and load " << path_ << '\n';
                } else {
                    using allocator_class = T* (*)(const ModuleConfiguration&);
                    using deleter_class = void (*)(T*);

                    auto allocator_function = reinterpret_cast<allocater_class>(GetProcAddress(handle_, module_allocator_function.c_str()));
                    if (!allocator_function) {
                        std::cerr << "Can't find allocator symbol in " << path_ << '\n';
                        unload_library();
                        return;
                    }

                    auto deleter_function = reinterpret_cast<deleter_class>(GetProcAddress(handle_, module_deleter_function.c_str()));
                    if (!deleter_function) {
                        std::cerr << "Can't find deleter symbol in " << path_ << '\n';
                        unload_library();
                        return;
                    }

                    module_ = std::shared_ptr<T>(allocator_function(configuration_), [deleter_function](T* p) noexcept { deleter_function(p); });
                }
            }
        }

        std::shared_ptr<T> get_module() const override {
            return module_;
        }

        void unload_module() override {
            module_.reset();
            if (handle_){
                if (FreeLibrary(handle_) == 0) {
                    std::cerr << "Can't close " << path_ << '\n';
                }
                handle_ = nullptr;
            }
        }

        const boost::filesystem::path& path() const override {
            return path_;
        }
        bool is_loaded() const {
            return module_.get() != nullptr;
        }
    private:
        boost::filesystem::path path_;
        const ModuleConfiguration& configuration_;
        HMODULE handle_{nullptr};
        std::shared_ptr<T> module_;
    };
}
#endif // WIN32

#endif // OC_MODULE
