
#include "oc/DatFile.hpp"
#include "oc/StringUtils.hpp"

#include <boost/filesystem.hpp>

#include <gsl/gsl>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <regex>
#include <unordered_map>
#include <string>
#include <tuple>
#include <vector>


namespace oc {

DatFile::DatFile(const std::string& file){
    load(file);
}
DatFile::DatFile(const std::string& file, Callback callback) : callback_{callback}{
    load(file);
}

bool DatFile::load(const std::string& file){
    std::ifstream in{file};
    if (!in){
        throw "Couldn't open DatFile: " + file;
    }
    bool ret = load(in);
    in.close();
    return ret;
}
bool DatFile::load(std::istream& in){
    std::string line;
    while (std::getline(in, line)){
        if (load_line(line)){
            return true;
        }
    }
    return false;
}
static bool is_comment(const std::string& line){
    for (auto i=line.cbegin();i!=line.cend();i++){
        if (!std::isspace(*i)){
            return *i == '#';
        }
    }
    return false;
}
bool DatFile::load_line(const std::string& line){
    // static const std::regex empty_or_comment_re{R"(^\s*$|^\s*#)"}; // for some reason, this doesn't catch comments
    // in fact, doing a regex of "#" doesn't even catch comments
    // EDIT: I think this has to do with regex_match vs regex_search (_match requires entire string to match?)
    //  Might keep it the same anyway, I think is_comment would probably be faster than regex
    static const std::regex empty_line_re{R"(^\s*$)"};
    static const std::regex kv_re{R"/(^\s*(\S+?)(\(\d+\))?\s*=\s*(?:"(.*)"|'(.*)'|(.*?))\s*$)/"};
    std::smatch matches;

    // std::cout << "Processing line: " << line << "\n";
    if (is_comment(line)){
        return false;
    } else if (std::regex_match(line, matches, empty_line_re)){
        return false;
    } else if (std::regex_match(line, matches, kv_re)){
        std::string v;
        if (matches[3].matched){
            v = matches[3];
        } else if (matches[4].matched){
            v = matches[4];
        } else if (matches[5].matched){
            v = matches[5];
        }
        double vf = std::stod(v);
        unsigned index = 0;
        if (matches[2].matched){
            index = gsl::narrow_cast<unsigned>(std::stoi(StringUtils::strip_enclosure(matches[2], '(', ')')));
        }
        if (index == 0){
            std::vector<double> vec{vf};
            kv_[matches[1]] = vec;
        } else if (index == 1){
            // std::vector<double> vec{NAN, vf};
            std::vector<double> vec{std::numeric_limits<double>::quiet_NaN(), vf};
            kv_[matches[1]] = vec;
        } else {
            kv_[matches[1]].emplace_back(vf);
        }
        if (callback_){
            callback_(std::make_tuple(matches[1], index, vf));
        }
    } else {
        // std::cout << "Invalid line: " << line << "\n";
        throw "Invalid DatFile line: " + line;
        return true;
    }
    return false;
}

std::unordered_map<std::string, std::vector<double>>::const_iterator DatFile::begin() const {
    return kv_.cbegin();
}
std::unordered_map<std::string, std::vector<double>>::const_iterator DatFile::end() const {
    return kv_.cend();
}

std::vector<double>& DatFile::operator[] (const std::string& k){
    return kv_[k];
}
std::vector<double>& DatFile::operator[] (std::string&& k){
    return kv_[k];
}
std::ostream& operator<<(std::ostream& os, const DatFile& kv){
    for (auto var = kv.begin(); var != kv.end(); ++var){
        auto& vec = var->second;
        int i = 1;
        auto val = vec.cbegin();
        if (std::isnan(*val)){
            ++val;
            for (; val != vec.cend(); ++val, ++i){
                os << var->first << "(" << i << ") = " << *val << "\n";
            }
        } else {
            os << var->first << " = " << *val << "\n";
        }
    }
    return os;
}

} // namespace oc

