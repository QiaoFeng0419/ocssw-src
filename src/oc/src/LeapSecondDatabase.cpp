
#include "oc/LeapSecondDatabase.hpp"
#include "oc/ShareTree.hpp"
#include "oc/StringUtils.hpp"

#include <cstdint>
#include <iostream>
#include <regex>
#include <string>

#include <boost/date_time/gregorian_calendar.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#include <gsl/gsl>

static std::array<std::string, 2> known_leapsec_files = {
    "../var/viirsn/IETTime.dat",
    "../var/modis/leapsec.dat"
};


static std::array<std::string, 12> months = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

oc::LeapSecondDatabase::LeapSecondDatabase(){
    char *env_configed = std::getenv("LEAPSECOND_DAT");
    if (env_configed != nullptr){
        load_file(std::string{env_configed});
    } else {
        oc::ShareTree share_tree{};
        // std::string viirs  = share_tree.get_path_to_file(boost::filesystem::path{});

        bool loaded = false;
        for (const std::string& p : known_leapsec_files){
            std::string full_path{share_tree.get_path_to_file(p)};
            if (boost::filesystem::exists(full_path)){
                loaded = true;
                load_file(full_path);
                break;
            }
        }
        if (!loaded){
            throw("No suitable leap second file found");
        }
    }
        // const auto sensor_info_path = root / "common" / "SensorInfo.json";
    // ShareTree::ShareTree() : ShareTree(boost::filesystem::path{std::getenv("OCDATAROOT")}){}
}
void oc::LeapSecondDatabase::load_file(const std::string& file){
    path_ = file;
    std::ifstream in{file};
    if (!in){
        throw "Couldn't open leap second file: " + file;
    }
    static const std::regex valid_line{R"/(^\s*(\d+) (.{3})\s*(\d+).*?UTC=\s*([^S\s]+))/"};
    std::smatch matches;
    std::string line;
    while (std::getline(in, line)){
        if (std::regex_search(line, matches, valid_line)){
            unsigned short year = gsl::narrow_cast<unsigned short>(std::stoi(matches[1]));
            std::string month = matches[2];
            unsigned short day = gsl::narrow_cast<unsigned short>(std::stoi(matches[3]));
            float offset = std::stof(matches[4]);

            unsigned short month_i = 0;

            for (size_t i=0;i<months.size();i++){
                if (month == months[i]){
                    month_i = gsl::narrow_cast<unsigned short>(i);
                }
            }
            leap_seconds_.emplace_back(year, month_i + 1, day, offset);
        } else {
            // std::cout << "Can't parse: " << line << "\n";
        }
    }
}
bool oc::LeapSecondDatabase::is_leap_second(const boost::posix_time::ptime& time){
    const auto hours = time.time_of_day().hours();
    const auto minutes = time.time_of_day().minutes();
    const auto seconds = time.time_of_day().seconds();
    boost::posix_time::ptime copy{time.date(), {hours, minutes, seconds}};
    for (const auto& ls : leap_seconds_){
        if (ls.time == copy){
            return true;
        } else if (ls.time > copy){
            break;
        }
    }
    return false;
}

float oc::LeapSecondDatabase::leap_seconds_since(const boost::posix_time::ptime& time){
    float last = 0;
    for (const auto& ls : leap_seconds_){
        if (ls.time > time){
            break;
        }
        last = ls.leap_seconds;
    }
    return last;
}


