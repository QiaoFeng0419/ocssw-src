
#include "oc/FilterIterator.hpp"
#include "oc/KvStore.hpp"

#include <boost/filesystem.hpp>
#include <gsl/gsl>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <regex>
#include <unordered_map>
#include <string>
#include <vector>


namespace oc {

KvStore::KvStore(const std::string& file){
    load(file);
}
KvStore::KvStore(const std::string& file, read_callback callback) : callback_{callback}{
    load(file);
}

bool KvStore::load(const std::string& file){
    std::ifstream in{file};
    if (!in){
        throw std::invalid_argument(file + " could not be opened");
    }
    current_group_ = "";
    current_file_ = file;
    const bool ret = load(in);
    in.close();
    current_file_ = "";
    return ret;
}
bool KvStore::load(std::istream& in){
    std::string line;
    while (std::getline(in, line)){
        if (load_line(line)){
            return true;
        }
    }
    return false;
}
bool KvStore::load_line(const std::string& line){
    static const std::regex empty_or_comment_re{R"(^\s*$|^\s*#$)"};
    static const std::regex group_re{R"(^\s*\[\s*(\S+)\s*\]\s*$)"};
    static const std::regex command_re{R"(^\s*\[\s*(\S+)\s*(.*?)\s*\]\s*$)"};
    static const std::regex kv_re{R"/(^\s*(\S+)\s*=\s*(?:"(.*)"|'(.*)'|(.*?))\s*$)/"};
    std::smatch matches;

    // std::cout << "Processing line: " << line << "\n";
    if (std::regex_match(line, matches, empty_or_comment_re)){
        return false;
    } else if (std::regex_match(line, matches, group_re)){
        return command_group(matches[1].str());
    } else if (std::regex_match(line, matches, command_re)){
        return command(matches[1].str(), matches[2].str());
    } else if (std::regex_match(line, matches, kv_re)){
        std::string v;
        if (matches[2].matched){
            v = matches[2];
        } else if (matches[3].matched){
            v = matches[3];
        } else if (matches[4].matched){
            v = matches[4];
        }
        if (current_group_.length() > 0){
            kv_[current_group_ + group_separator_ + matches[1].str()] = v;
        } else {
            kv_[matches[1]] = v;
        }
        if (callback_){
            return callback_(std::make_pair(matches[1], v));
        }
    } else {
        // std::cout << "Invalid line: " << line << "\n";
        throw std::runtime_error("Invalid KvFile line: " + line);
    }
    return false;
}

bool KvStore::command(std::string&& cmd, std::string&& args){
    if (cmd == "include_local"){
        return command_include_local(std::move(args));
    } else if (cmd == "include"){
        return command_include(std::move(args));
    } else if (cmd == "group"){
        return command_group(std::move(args));
    } else {
        // std::cout << "Invalid command: " << cmd << "\n";
        throw std::runtime_error("Invalid KvFile command: " + cmd);
    }
    return false;
}
bool KvStore::command_include_local(std::string&& file){
    std::string old_file = current_file_;
    std::string old_group = current_group_;
    const bool ret = load(file);
    current_group_ = old_group;
    current_file_ = old_file;
    return ret;
}
bool KvStore::command_include(std::string&& file){
    std::string old_file = current_file_;
    std::string old_group = current_group_;
    current_group_ = "";
    auto rel_path = boost::filesystem::path{old_file}.parent_path() / file;
    const bool ret = load(rel_path.string());
    current_group_ = old_group;
    current_file_ = old_file;
    return ret;
}
bool KvStore::command_group(std::string&& group){
    if (group == "GLOBAL"){
        current_group_ = "";
    } else {
        current_group_ = group;
        add_group(group);
    }
    return false;
}

KvStore::group_iterator KvStore::group_begin() const {
    return groups_.cbegin();
}
KvStore::group_iterator KvStore::group_end() const {
    return groups_.cend();
}
void KvStore::add_group(const std::string& group){
    if (std::find(groups_.cbegin(), groups_.cend(), group) == groups_.cend()){
        groups_.emplace_back(group);
    }
}

KvStore::iterator KvStore::begin() const {
    return kv_.cbegin();
}
KvStore::iterator KvStore::end() const {
    return kv_.cend();
}
KvStore::group_filtered_iterator KvStore::end(const std::string& group) const {
    return begin(group).end();
}
KvStore::group_filtered_iterator KvStore::begin(const std::string& group) const {
    if (group == "GLOBAL" || group == "global"){
        return oc::FilterIterator<std::unordered_map<std::string, std::string>::const_iterator, std::function<bool(const std::pair<std::string, std::string>&)>> {begin(), end(), [group, this](const std::pair<std::string, std::string>& kv) {
            return std::search(kv.first.begin() + gsl::narrow<long>(group.size()), kv.first.end(), group_separator_.begin(), group_separator_.end()) == kv.first.end();
        }};
    } else {
        std::string group_with_separator = group + group_separator_;
        return oc::FilterIterator<std::unordered_map<std::string, std::string>::const_iterator, std::function<bool(const std::pair<std::string, std::string>&)>> {begin(), end(), [group_with_separator, this](const std::pair<std::string, std::string>& kv) {
            if (kv.first.size() < group_with_separator.size()){
                return false;
            } else {
                auto res = std::mismatch(group_with_separator.begin(), group_with_separator.end(), kv.first.begin());
                if (res.first == group_with_separator.end()){
                    return std::search(kv.first.begin() + gsl::narrow<long>(group_with_separator.size()), kv.first.end(), group_separator_.begin(), group_separator_.end()) == kv.first.end();
                } else {
                    return false;
                }
            }
        }};
    }
}

std::string& KvStore::operator[] (const std::string& k){
    return kv_[k];
}
std::string& KvStore::operator[] (std::string&& k){
    return kv_[k];
}
std::ostream& operator<<(std::ostream& os, const KvStore& kv){
    for (auto it = kv.begin("GLOBAL"); it != kv.end("GLOBAL"); ++it){
        os << (*it)->first << " = " << (*it)->second << "\n";
    }
    for (auto group = kv.group_begin(); group != kv.group_end(); ++group){
        os << "[" << *group << "]" << "\n";
        for (auto it = kv.begin(*group); it != kv.end(*group); ++it){
            os << (*it)->first << " = " << (*it)->second << "\n";
        }
    }
    return os;
}

} // namespace oc

