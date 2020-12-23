#ifndef OC_VARIABLE
#define OC_VARIABLE

#include <string>
#include <unordered_map>

namespace oc {

class Variable {
    public:
        Variable() = default;
        virtual ~Variable();

        std::unordered_map<std::string, std::string>::const_iterator attributes_begin() const {return attributes_.begin();}
        std::unordered_map<std::string, std::string>::const_iterator attributes_end() const {return attributes_.end();}

        virtual int number_of_variables(){return 1;}
        // virtual Variable get_(){return 1;}

    private:
        std::unordered_map<std::string, std::string> attributes_;
};

} // namespace oc

#endif // OC_VARIABLE
