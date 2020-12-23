
#include "oc/Log.hpp"
#include "oc/log/FormattedStream.hpp"

#include <gsl/gsl>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace oc {

const std::array<const std::string, 8> severities_lower{ "emergency", "alert", "critical", "error", "warning", "notice", "info", "debug" };
const std::array<const std::string, 8> severities_upper{ "EMERGENCY", "ALERT", "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG" };
const std::array<const std::string, 8> severities_title{ "Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Info", "Debug" };

//! \cond
class StaticStringCommand : public FormatCommand {
    public:
        StaticStringCommand(const std::string& s) : string_{s} {}
        StaticStringCommand(const std::string::const_iterator& first, const std::string::const_iterator& last) : string_{first, last} {}
        ~StaticStringCommand(){}
        void log(std::ostream& stream, const int level, const std::string& s) const override {
            (void)level;
            (void)s;
            stream << string_;
        }
    private:
        const std::string string_;
};

class NamedFunctionCommand : public FormatCommand {
    using named_function = std::function<std::string(const std::string& name, std::ostream& stream, const int level, const std::string& str)>;
    public:
        NamedFunctionCommand(const std::string& s, named_function f) : string_{s}, function_{f} {}
        ~NamedFunctionCommand(){}
        void log(std::ostream& stream, const int level, const std::string& s) const override {
            function_(string_, stream, level, s);
        }
    private:
        const std::string string_;
        named_function function_;
};
class IndexedFunctionCommand : public FormatCommand {
    using indexed_function = std::function<std::string(const size_t index, std::ostream& stream, const int level, const std::string& str)>;
    public:
        IndexedFunctionCommand(const size_t i, indexed_function f) : index_{i}, function_{f} {}
        ~IndexedFunctionCommand(){}
        void log(std::ostream& stream, const int level, const std::string& s) const override {
            function_(index_, stream, level, s);
        }
    private:
        const size_t index_;
        indexed_function function_;
};
class UserStringCommand : public FormatCommand {
    public:
        ~UserStringCommand(){}
        void log(std::ostream& stream, const int level, const std::string& s) const override {
            (void)level;
            stream << s;
        }
        bool is_user_string() override { return true; }
};
class SeverityNumberCommand : public FormatCommand {
    public:
        ~SeverityNumberCommand(){}
        void log(std::ostream& stream, const int level, const std::string& s) const override {
            (void)s;
            stream << level;
        }
};
class SeverityStringCommand : public FormatCommand {
    public:
        SeverityStringCommand(const std::array<const std::string, 8>& severities) : severities_{severities}{}
        ~SeverityStringCommand(){}
        void log(std::ostream& stream, const int level, const std::string& s) const override {
            (void)s;
            if (level < 0){
                stream << *severities_.cbegin() << '+' << 0 - level;
            } else if (level >= gsl::narrow_cast<int>(severities_.size())){
                stream << *severities_.crbegin() << '+' << level - severities_.size() + 1;
            } else {
                stream << severities_[level];
            }
        }
    private:
        const std::array<const std::string, 8>& severities_;
};
class TimeStringCommand : public FormatCommand {
    public:
        TimeStringCommand() {}
        TimeStringCommand(const std::string& format) : format_{format} {}
        TimeStringCommand(bool in_local_time) : in_local_time_{in_local_time} {}
        TimeStringCommand(const std::string& format, bool in_local_time) : format_{format}, in_local_time_{in_local_time} {}
        ~TimeStringCommand(){}

        void log(std::ostream& stream, const int level, const std::string& s) const override {
            (void)level;
            (void)s;
            auto t = std::time(nullptr);
            if (in_local_time_){
                stream << std::put_time(std::localtime(&t), format_.c_str());
            } else {
                stream << std::put_time(std::gmtime(&t), format_.c_str());
            }
        }
    private:
        const std::string format_{"%FT%T%z"};
        bool in_local_time_{false};
};
//! \endcond

FormattedStreamLogger::FormattedStreamLogger(const std::string& format, map_of_functions named_functions, vector_of_functions numbered_functions, std::ostream& stream) : stream_{stream}, named_functions_{named_functions}, numbered_functions_{numbered_functions} {
    parse_format_string(format);
}
FormattedStreamLogger::~FormattedStreamLogger(){
    stream_.flush();
}

void FormattedStreamLogger::log(int level, const std::string& s){
    if (last_character_was_newline_){
        for (const auto& command : commands_){
            if (command->is_user_string()){
                break;
            }
            command->log(stream_, level, s);
        }
    }
    auto newline = s.find_first_of('\n');
    if (newline == std::string::npos){
        stream_ << s;
        last_character_was_newline_ = false;
    } else {
        // Implement this via string_view when we switch to C++17 to give more data to stream_ at a time.
        auto last = s.cend() - 1;
        for (auto c = s.cbegin(); c != s.cend(); c++){
            if (*c == '\n'){
                for (auto command = after_user_string_; command != commands_.cend(); command++){
                    (*command)->log(stream_, level, s);
                }
                stream_ << '\n';
                if (!user_string_is_last_){
                    bool found_user_string = false;
                    for (const auto& command : commands_){
                        if (command->is_user_string()){
                            break;
                        }
                        if (found_user_string){
                            command->log(stream_, level, s);
                        }
                    }
                }
                if (c != last){
                    for (const auto& command : commands_){
                        if (command->is_user_string()){
                            break;
                        }
                        command->log(stream_, level, s);
                    }
                } else {
                    last_character_was_newline_ = true;
                }
            } else {
                stream_ << *c;
            }
        }
    }
    if (last_format_char_is_newline_){
        for (auto command = after_user_string_; command != commands_.cend(); command++){
            (*command)->log(stream_, level, s);
        }
        last_character_was_newline_ = true;
    }
    // for (const auto& command : commands_){
    //     command->log(stream_, level, s);
    // }
}

void FormattedStreamLogger::parse_format_string(const std::string& format) {
    unsigned max_command_count = 0;

    auto pt = format.cbegin();
    for (; pt != format.cend(); ++pt){
        if (*pt == '%') {
            ++pt;
            if (*pt == '\0') {
                throw("failed to compile log format: unfinished escape sequence at end of string");
            } else if (*pt != '%'){
                ++max_command_count;
            }
        }
    }
    commands_.reserve(max_command_count);
    pt = format.cbegin();

    if (format.back() == '\n'){
        last_format_char_is_newline_ = true;
    }

    bool last_char_was_command = true;

    auto first_char = pt;
    while (pt != format.cend()){
        if (*pt == '%') {
            if (!last_char_was_command) {
                commands_.emplace_back(std::make_unique<StaticStringCommand>(first_char, pt));

                last_char_was_command = true;

                if (*(pt + 1) == '%') {
                    // *(pt + 1) = '\0';
                    pt += 2;
                    continue;
                }
            }

            ++pt;

            if (*pt == '\0') {
                throw("failed to compile log format: unfinished escape sequence at end of string");
//          } else if (*pt == '%') {
//              /* skip */
            } else if (*pt == '{' || *pt == '[') {
                char start_char = *pt;
                auto quote_end = std::find(pt, format.cend(), start_char == '{' ? '}' : ']');
                if (quote_end == format.cend()) {
                    // throw("failed to compile log format: unterminated named pointer name starting at: "s + *pt);
                    throw;
                }
                std::string quote = std::string{pt + 1, quote_end};
                quote_end++;
                switch (*quote_end) {
                case 'f': {
                    if (start_char == '{'){
                        commands_.emplace_back(std::make_unique<NamedFunctionCommand>(quote, named_functions_[quote]));
                    } else {
                        size_t index = std::stoull(quote);
                        commands_.emplace_back(std::make_unique<IndexedFunctionCommand>(index, numbered_functions_[index]));
                    }
                } break;
                case 't': {
                    auto comma = quote.find_first_of(',');
                    if (comma == std::string::npos){
                        comma = quote.size();
                    }

                    auto in_local_time = false;
                    std::string format = "%FT%T%z";

                    if (quote.compare(0, comma, "local") == 0){
                        in_local_time = true;
                        comma++;
                    } else if (quote.compare(0, comma, "gmt") == 0 || quote.compare(0, comma, "utc") == 0){
                        in_local_time = false;
                        comma++;
                    } else {
                        comma = 0;
                    }
                    if (comma < quote.size()){
                        format = std::string{quote, comma};
                    }

                    commands_.emplace_back(std::make_unique<TimeStringCommand>(format, in_local_time));
                } break;
                default:
                    throw("failed to compile log format: named pointer name is not followed by one of: f, t");
                }
                pt = quote_end + 1;
            } else {
                switch (*pt++) {
                case 's': {
                    user_string_is_last_ = false;
                    commands_.emplace_back(std::make_unique<UserStringCommand>());
                } break;
                case 'l':
                    commands_.emplace_back(std::make_unique<SeverityStringCommand>(severities_lower));
                    break;
                case 'u':
                    commands_.emplace_back(std::make_unique<SeverityStringCommand>(severities_upper));
                    break;
                case 'm':
                    commands_.emplace_back(std::make_unique<SeverityStringCommand>(severities_title));
                    break;
                case 'n':
                    commands_.emplace_back(std::make_unique<SeverityNumberCommand>());
                    break;
                case 't':
                    commands_.emplace_back(std::make_unique<TimeStringCommand>());
                    break;
                default:
                    // throw("failed to compile log format: unknown escape sequence: %"s + pt[-1]);
                    throw;
                }
                continue;
            }
        } else if (last_char_was_command) {
            first_char = pt++;
            last_char_was_command = false;
        } else {
            ++pt;
        }
    }
    if (!last_char_was_command) {
        commands_.emplace_back(std::make_unique<StaticStringCommand>(first_char, pt));
    }
    if (user_string_is_last_) {
        // commands_.emplace_back(std::make_unique<UserStringCommand>());
        after_user_string_ = commands_.cend();
    } else {
        if ((*commands_.rbegin())->is_user_string()){
            commands_.pop_back();
            user_string_is_last_ = true;
            after_user_string_ = commands_.cend();
        }
        for (auto c = commands_.cbegin(); c != commands_.cend(); c++){
            if ((*c)->is_user_string()){
                after_user_string_ = c+1;
                break;
            }
        }
    }

    // return ret;
}

} // namespace oc

