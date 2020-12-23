#ifndef OC_SHARETREE
#define OC_SHARETREE

#include <boost/filesystem.hpp>

#include <rapidjson/document.h>

#include <iterator>
#include <string>
#include <map>

namespace oc {
    struct SensorDirectory {
        std::string instrument;
        std::string platform;
        boost::filesystem::path root;

        SensorDirectory(const rapidjson::GenericValue<rapidjson::UTF8<>>& in);
    };
    std::ostream& operator<<(std::ostream& out, const SensorDirectory& in);

    class ShareTree {
        public:
            ShareTree();
            ShareTree(boost::filesystem::path root);

            inline static ShareTree& get_default(){
                static ShareTree _instance{};
                return _instance;
            }

            std::map<const std::string, const SensorDirectory>::const_iterator begin() const;
            std::map<const std::string, const SensorDirectory>::const_iterator end() const;

            const SensorDirectory& operator[] (const std::string& k) const;
            const SensorDirectory& operator[] (std::string&& k) const;

            const SensorDirectory& at(const std::string& k) const;

            std::string get_path_to_file(const std::string& sensor_id, const boost::filesystem::path& rel_path);
            std::string get_path_to_file(const boost::filesystem::path& rel_path);
        private:
            const boost::filesystem::path root_;
            const std::map<const std::string, const SensorDirectory> directories_;
    };
} // namespace oc

#endif // OC_SHARETREE

