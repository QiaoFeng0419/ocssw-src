
#include "oc/ShareTree.hpp"
#include "oc/StringUtils.hpp"

#include <boost/filesystem.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <map>

namespace oc {
    using namespace std::string_literals;

    static std::map<const std::string, const SensorDirectory> fill_directories(boost::filesystem::path& root){
        const auto sensor_info_path = root / "common" / "SensorInfo.json";
        std::ifstream sensor_info_h{sensor_info_path.native()};

        rapidjson::Document sensor_info_doc;
        rapidjson::IStreamWrapper sensor_info_i{sensor_info_h};
        sensor_info_doc.ParseStream(sensor_info_i);

        std::map<const std::string, const SensorDirectory> directories{};
        for (rapidjson::Value::ConstMemberIterator itr = sensor_info_doc.MemberBegin(); itr != sensor_info_doc.MemberEnd(); ++itr) {
            directories.emplace(itr->name.GetString(), itr->value);
        }
        return directories;
    }

    ShareTree::ShareTree() : ShareTree(boost::filesystem::path{std::getenv("OCDATAROOT")}){}
    ShareTree::ShareTree(boost::filesystem::path root) : root_{root}, directories_{fill_directories(root)}{}


    SensorDirectory::SensorDirectory(const rapidjson::GenericValue<rapidjson::UTF8<>>& in) : instrument{in["instrument"].GetString()}, platform{in["platform"].GetString()}, root{StringUtils::replace_oc_roots("$OCDATAROOT/"s + in["directory"].GetString())}  {}

    std::ostream& operator<<(std::ostream& out, const SensorDirectory& in) {
        return out << "SensorDirectory{instrument=\"" << in.instrument << "\", platform=\"" << in.platform << "\", root=\"" << in.root << "\"}";
    }

    std::map<const std::string, const SensorDirectory>::const_iterator ShareTree::begin() const {
        return directories_.cbegin();
    }
    std::map<const std::string, const SensorDirectory>::const_iterator ShareTree::end() const {
        return directories_.cend();
    }

    const SensorDirectory& ShareTree::operator[] (const std::string& k) const {
        return directories_.at(k);
    }
    const SensorDirectory& ShareTree::operator[] (std::string&& k) const {
        return directories_.at(k);
    }
    const SensorDirectory& ShareTree::at(const std::string& k) const {
        return directories_.at(k);
    }

    std::string ShareTree::get_path_to_file(const boost::filesystem::path& rel_path){
        return (root_ / rel_path).native();
    }
    std::string ShareTree::get_path_to_file(const std::string& sensor_id, const boost::filesystem::path& rel_path){
        return (directories_.at(sensor_id).root / rel_path).native();
    }

} // namespace oc

