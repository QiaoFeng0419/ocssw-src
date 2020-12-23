
#include "oc/TimePoint.hpp"

#include <array>
#include <regex>
#include <string>

#include <gsl/gsl>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace oc;

// offsets from TAI, for conversions
static const auto tai_minus_tt = -boost::posix_time::milliseconds(32'184);
static const auto tai_minus_gps = boost::posix_time::seconds(19);
static const auto tai_minus_tai93 = boost::posix_time::seconds(27);

TimePoint::TimePoint(const double seconds_since_1970, const TimePoint::clock_type clock_type){
    ptime_ = boost::posix_time::ptime{{1970,1,1}} + boost::posix_time::milliseconds{gsl::narrow_cast<long>(seconds_since_1970 * 1000)};
    clock_type_ = clock_type;
}
static const boost::posix_time::ptime convert_time(boost::posix_time::ptime ptime, const TimePoint::clock_type from_type, const TimePoint::clock_type to_type) {
    if (to_type == from_type){
        return ptime;
    }
    if (from_type == TimePoint::local || to_type == TimePoint::local){
        throw("Can't convert to or from local time");
    }
    // auto ptime = ptime_;
    if (from_type == TimePoint::utc || to_type == TimePoint::utc){
        // to and from UTC needs leap second correction
        static auto ls_db = LeapSecondDatabase::get_default();
        auto leap_seconds = boost::posix_time::microseconds(gsl::narrow_cast<int>(ls_db.leap_seconds_since(ptime) * 1'000'000));
        if (from_type == TimePoint::utc){
            // if (ls_db.is_leap_second(ptime)){
            //     leap_seconds += boost::posix_time::seconds{1};
            // }
            if (to_type == TimePoint::tai){
                ptime += leap_seconds;
            } else if (to_type == TimePoint::tai93){
                ptime += leap_seconds - tai_minus_tai93;
            } else if (to_type == TimePoint::gps){
                ptime += leap_seconds - tai_minus_gps;
            } else if (to_type == TimePoint::tt){
                ptime += leap_seconds - tai_minus_tt;
            }
        } else {
            if (from_type == TimePoint::tai){
                ptime -=  leap_seconds;
            } else if (from_type == TimePoint::tai93){
                ptime -= leap_seconds - tai_minus_tai93;
            } else if (from_type == TimePoint::gps){
                ptime -= leap_seconds - tai_minus_gps;
            } else if (from_type == TimePoint::tt){
                ptime -= leap_seconds - tai_minus_tt;
            }
            // Did we jump a leap second boundary we have to fix?
            const auto new_leap_seconds = boost::posix_time::microseconds(gsl::narrow_cast<int>(ls_db.leap_seconds_since(ptime) * 1'000'000));
            if (new_leap_seconds != leap_seconds){
                ptime -= new_leap_seconds - leap_seconds;
                if (ls_db.is_leap_second(ptime)){
                    ptime -= boost::posix_time::seconds{1};
                }
            }
        }
    // Every non-UTC clock is parallel to TAI, but offset
    } else if (from_type == TimePoint::tai){
        if (to_type == TimePoint::tai93){
            ptime -= tai_minus_tai93;
        } else if (to_type == TimePoint::gps){
            ptime -= tai_minus_gps;
        } else if (to_type == TimePoint::tt){
            ptime -= tai_minus_tt;
        }
    } else if (from_type == TimePoint::tai93){
        if (to_type == TimePoint::tai){
            ptime += tai_minus_tai93;
        } else if (to_type == TimePoint::gps){
            ptime += tai_minus_tai93 - tai_minus_gps;
        } else if (to_type == TimePoint::tt){
            ptime += tai_minus_tai93 - tai_minus_tt;
        }
    } else if (from_type == TimePoint::gps){
        if (to_type == TimePoint::tai){
            ptime += tai_minus_gps;
        } else if (to_type == TimePoint::tai93){
            ptime += tai_minus_gps - tai_minus_tai93;
        } else if (to_type == TimePoint::tt){
            ptime += tai_minus_gps - tai_minus_tt;
        }
    } else if (from_type == TimePoint::tt){
        if (to_type == TimePoint::tai){
            ptime += tai_minus_tt;
        } else if (to_type == TimePoint::tai93){
            ptime += tai_minus_tt - tai_minus_tai93;
        } else if (to_type == TimePoint::gps){
            ptime += tai_minus_tt - tai_minus_gps;
        }
    }

    return ptime;
}

TimePoint::TimePoint(TimePoint::clock_type clock_type) : ptime_{clock_type == TimePoint::local ? boost::posix_time::microsec_clock::local_time() : convert_time(boost::posix_time::microsec_clock::universal_time(), TimePoint::utc, clock_type)}, clock_type_{clock_type}{}

const TimePoint TimePoint::convert_to(const TimePoint::clock_type clock_type) const {
    return TimePoint{convert_time(ptime_, clock_type_, clock_type), clock_type};
}


static const boost::posix_time::ptime parse_date_string(const std::string& datetime){
    static const std::string& date_regex_str{
        "\\b" // word boundary
        "(\\d{4}|\\d{2})" // \1 = two or four digit year
        "[-/]?"
        "(?:" // day-of-year|cal-day
            "(\\d{3})"   // \2 = day of year
        "|"
            "(\\d{1,2})" // \3 = month
            "[-/]?"
            "(\\d{1,2})" // \4 = day
        ")"
        "(?:" // optional time and zone
            "[T ]?"
            "(\\d{1,2})" // \5 = hour
            ":?"
            "(\\d{1,2})" // \6 = minute
            ":?"
            "(?:" // optional second, with optional fraction
                "(\\d{1,2})?" // \7 = second
                "(\\.\\d*)?"  // \8 = fraction
            ")"
            "(Z|[-+]?:\\d{2}:?\\d{2})?" // \9 = timezone (Z, +/-hh[mm]); ignored
        ")?"
        "\\b" // word boundary
    };
    // To test: https://regex101.com
    // std::cout << date_regex_str << "\n";
    // \b(\d{4}|\d{2})[-/]?(?:(\d{3})|(\d{1,2})[-/]?(\d{1,2}))(?:[T ]?(\d{1,2}):?(\d{1,2}):?(?:(\d{1,2})?(\.\d*)?)(Z|[-+]?:\d{2}:?\d{2})?)?\b
    static const std::regex date_regex{date_regex_str};

    std::smatch matches;
    if (std::regex_search(datetime, matches, date_regex)){
        unsigned short year = gsl::narrow_cast<unsigned short>(std::stoi(matches[1]));
        if (matches[1].length() == 2){
            // This is pretty hand-wavey...
            if (year >= 30){
                year += 1900;
            } else {
                year += 2000;
            }
        }
        boost::gregorian::date d;
        if (matches[2].matched){ // day-of-year
            d = boost::gregorian::date{year, 1, 1};
            d += boost::gregorian::days(std::stoi(matches[2]) - 1);
        } else { // cal-day
            const unsigned short month = gsl::narrow_cast<unsigned short>(std::stoi(matches[3]));
            const unsigned short day = gsl::narrow_cast<unsigned short>(std::stoi(matches[4]));
            d = boost::gregorian::date{year, month, day};
        }
        if (matches[5].matched){ // time
            int second = 0;
            if (matches[7].matched){
                second = std::stoi(matches[7]);
            }
            boost::posix_time::time_duration t{std::stoi(matches[5]), std::stoi(matches[6]), second};
            if (matches[8].matched){ // fraction seconds to integer
                t += boost::posix_time::microseconds(gsl::narrow_cast<long>(std::stod(matches[8]) * 1'000'000));
            }
            return boost::posix_time::ptime{d, t};
        } else {
            return boost::posix_time::ptime{d};
        }
    } else {
        throw("Unable to parse date");
        // std::cout << "Unable to parse " << datetime << "\n";
        // ptime_ = boost::posix_time::not_a_date_time;
    }
}
oc::TimePoint::TimePoint(const std::string& datetime, oc::TimePoint::clock_type clock_type) : ptime_{parse_date_string(datetime)}, clock_type_{clock_type}{ }

std::string TimePoint::to_string(const std::string& format) const {
    std::ostringstream ss;
    ::boost::posix_time::time_facet* p_time_output = new ::boost::posix_time::time_facet;
    std::locale special_locale(std::locale(""), p_time_output);
    ss.imbue(special_locale); // ss is now the owner of the time_facet, so don't delete it!
    size_t clock_tag = format.find("%t");
    if (clock_tag == std::string::npos){  // no %t, so no need to copy format, just pass it along
        (*p_time_output).format(format.c_str());
        ss << ptime_;
        return ss.str();
    } else { // need to replace %t, so copy input
        std::string new_format = format;
        new_format.replace(clock_tag, 2, clock());
        (*p_time_output).format(new_format.c_str());
        ss << ptime_;
        return ss.str();
    }
}
boost::posix_time::time_duration TimePoint::duration_since(const boost::posix_time::ptime& time) const {
    return ptime_ - time;
}


