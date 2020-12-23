
// #include <check.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <ratio>

#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

#include <oc.hpp>

using namespace std;

const std::string sf = "%Y-%m-%dT%H:%M:%S.%f";

void leap_conversions(){
    const oc::TimePoint replay_tai{{2015, 7, 1}, {0, 0, 35}, oc::TimePoint::clock_type::tai};
    const oc::TimePoint ambiguous_conversion{{2015, 7, 1}, {0, 0, 34}, oc::TimePoint::clock_type::tai};

    const oc::TimePoint replay_tai_half{{2015, 7, 1}, {0, 0, 35}, 500, oc::TimePoint::clock_type::tai};
    const oc::TimePoint ambiguous_conversion_half{{2015, 7, 1}, {0, 0, 34}, 500, oc::TimePoint::clock_type::tai};

    std::vector<oc::TimePoint> utc_clocks{
        {{2015, 6, 30}, {23, 59, 59}, oc::TimePoint::clock_type::utc},
        {{2015, 6, 30}, {23, 59, 59}, 500, oc::TimePoint::clock_type::utc},
        {{2015, 6, 30}, {23, 59, 59}, oc::TimePoint::clock_type::utc}, // replayed second
        {{2015, 6, 30}, {23, 59, 59}, 500, oc::TimePoint::clock_type::utc}, // replayed second
        {{2015, 7,  1}, {0, 0, 0}, oc::TimePoint::clock_type::utc},
        {{2015, 7,  1}, {0, 0, 0}, 500, oc::TimePoint::clock_type::utc},
        {{2015, 7,  1}, {0, 0, 1}, oc::TimePoint::clock_type::utc},
        {{2015, 7,  1}, {0, 0, 1}, 500, oc::TimePoint::clock_type::utc},
    };
    std::vector<oc::TimePoint> tai_clocks{
        {{2015, 7, 1}, {0, 0, 34}, oc::TimePoint::clock_type::tai},
        {{2015, 7, 1}, {0, 0, 34}, 500, oc::TimePoint::clock_type::tai},
        replay_tai,
        replay_tai_half,
        {{2015, 7, 1}, {0, 0, 36}, oc::TimePoint::clock_type::tai},
        {{2015, 7, 1}, {0, 0, 36}, 500, oc::TimePoint::clock_type::tai},
        {{2015, 7, 1}, {0, 0, 37}, oc::TimePoint::clock_type::tai},
        {{2015, 7, 1}, {0, 0, 37}, 500, oc::TimePoint::clock_type::tai},
    };

    std::cout << "UTC                        => UTC-to-TAI                 == TAI                        => TAI-to-UTC\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------\n";
    for (auto utc = utc_clocks.begin(), tai = tai_clocks.begin(); utc != utc_clocks.end(); ++utc, ++tai){
        auto to_tai = utc->convert_to(oc::TimePoint::tai);
        auto to_utc = tai->convert_to(oc::TimePoint::utc);
        std::cout << utc->to_string(sf) << " => " << to_tai.to_string(sf) << " == " << tai->to_string(sf) << " => " << to_utc.to_string(sf) << "\n";
        if (*tai == replay_tai){
            assert(to_tai == ambiguous_conversion);
        } else if (*tai == replay_tai_half){
            assert(to_tai == ambiguous_conversion_half);
        } else {
            assert(to_tai == *tai);
        }
        assert(to_utc == *utc);
    }
    std::cout << "\n\n";
}

void simple_conversions(){
    std::vector<oc::TimePoint> clocks{ // all of these clocks are the same instant in time
        {{2019, 1, 1}, {3, 23, 19}, oc::TimePoint::clock_type::utc},
        {{2019, 1, 1}, {3, 23, 56}, oc::TimePoint::clock_type::tai},
        {{2019, 1, 1}, {3, 23, 29}, oc::TimePoint::clock_type::tai93},
        {{2019, 1, 1}, {3, 23, 37}, oc::TimePoint::clock_type::gps},
        {{2019, 1, 1}, {3, 24, 28}, 184, oc::TimePoint::clock_type::tt}
    };

    for (auto c1 = clocks.begin(); c1 != clocks.end(); ++c1){
        for (auto c2 = clocks.begin(); c2 != clocks.end(); ++c2){
            if (c1->get_clock_type() != c2->get_clock_type()){
                auto cc1 = c1->convert_to(c2->get_clock_type());
                std::cout << cc1.to_string(sf) << " == " << c2->to_string(sf) << " (" << c1->clock() << " => " << c2->clock() << ")\n";
                assert(cc1 == *c2);
            }
        }
    }
    std::cout << "\n\n";
}
void string_conversion(){
    const oc::TimePoint should_be_millis{{2019, 1, 2}, {3, 4, 5}, 60};
    const oc::TimePoint should_be{{2019, 1, 2}, {3, 4, 5}};
    const oc::TimePoint should_be_date_only{{2019, 1, 2}, {0, 0, 0}};

    assert(oc::TimePoint{"2019-01-02T03:04:05.06Z"} == should_be_millis);

    assert(oc::TimePoint{"2019-01-02T03:04:05Z"} == should_be);
    assert(oc::TimePoint{"2019-01-02 03:04:05Z"} == should_be);
    assert(oc::TimePoint{"20190102T030405"} == should_be);
    assert(oc::TimePoint{"20190102T030405-0700"} == should_be);
    assert(oc::TimePoint{"20190102030405"} == should_be);
    assert(oc::TimePoint{"2019-002 03:04:05Z"} == should_be);
    assert(oc::TimePoint{"2019002 03:04:05Z"} == should_be);
    assert(oc::TimePoint{"2019002030405"} == should_be);

    assert(oc::TimePoint{"2019-01-02"} == should_be_date_only);
    assert(oc::TimePoint{"20190102"} == should_be_date_only);
    assert(oc::TimePoint{"2019-002"} == should_be_date_only);
}
void seconds_since(){
    oc::TimePoint t{"2019-01-01T00:00:00", oc::TimePoint::clock_type::utc};

    // std::cout << t << '\n';
    // std::cout << std::fixed;
    // std::cout << "seconds since 1970: " << t.seconds_since_1970() << '\n';
    // std::cout << "seconds since 1993: " << t.seconds_since_1993() << '\n';
    // std::cout << "julian date: " << t.julian_date() << '\n';
    // std::cout << "truncated julian date: " << t.truncated_julian_date() << '\n';
    // std::cout << "modified julian date: " << t.modified_julian_date() << '\n';
    // std::cout << "reduced julian date: " << t.reduced_julian_date() << '\n';

    assert(t.seconds_since_1970() == 1546300800);
    assert(t.seconds_since_1993() == 820454400);
    assert(t.julian_date() == 2458484.5);
    assert(t.truncated_julian_date() == 18484.);
    assert(t.modified_julian_date() == 58484.);
    assert(t.reduced_julian_date() == 58484.5);

    oc::TimePoint from_unix(t.seconds_since_1970());
    assert(from_unix == t);
}

int main() {
    string_conversion();
    simple_conversions();
    leap_conversions();
    seconds_since();

    return 0;
}

