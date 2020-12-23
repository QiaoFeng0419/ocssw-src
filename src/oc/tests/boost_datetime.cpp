
// #include <check.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <ratio>
#include <timeutils.h>

#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/date_facet.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_facet.hpp>

#include <oc.hpp>

using namespace std;

using namespace boost::posix_time;
// using namespace boost::date_time;
using namespace boost::gregorian;
using namespace boost::local_time; 

void input_testing();
void output_testing();
void leap_testing();

std::string formatted_datetime(::boost::posix_time::ptime pt) {
    std::string s;
    std::ostringstream datetime_ss;
    // ::boost::posix_time::time_facet* p_time_output = new ::boost::posix_time::time_facet;
    ::boost::posix_time::time_facet* p_time_output = new ::boost::posix_time::time_facet;
    std::locale special_locale(std::locale(""), p_time_output);
    // special_locale takes ownership of the p_time_output facet
    datetime_ss.imbue(special_locale);
    // (*p_time_output).format("%x %X"); // date time
    // (*p_time_output).format("%Y-%m-%d %H:%M:%S.%f%q"); // date time
    (*p_time_output).format("%Y-%m-%dT%H:%M:%S.000Z"); // date time
    datetime_ss << pt;
    s = datetime_ss.str().c_str();
    // don't explicitly delete p_time_output
    return s;
}

// TT(TAI) = TAI + 32.184 s
void simple_conversions(){
    oc::TimePoint original { date { 2019, 1, 1 }, time_duration { 3, 23, 19 } };
    oc::TimePoint orig_to_tai = original;
    oc::TimePoint orig_to_tai93 = original;
    oc::TimePoint orig_to_gps = original;
    oc::TimePoint orig_to_tt = original;
    orig_to_tai.convert_to(oc::TimePoint::clock_type::tai);
    orig_to_tai93.convert_to(oc::TimePoint::clock_type::tai93);
    orig_to_gps.convert_to(oc::TimePoint::clock_type::gps);
    orig_to_tt.convert_to(oc::TimePoint::clock_type::tt);
    cout << "original    utc: " << original << " (should be 03:23:19)\n";
    cout << "original to tai: " << orig_to_tai << " (should be 03:23:56)\n";
    cout << "original to t93: " << orig_to_tai93 << " (should be 03:23:29)\n";
    cout << "original to gps: " << orig_to_gps << " (should be 03:23:37)\n";
    cout << "original to tt:  " << orig_to_tt << " (should be 03:24:28.184)\n\n";

    oc::TimePoint tai_to_utc = orig_to_tai;
    oc::TimePoint tai_to_tai93 = orig_to_tai;
    oc::TimePoint tai_to_gps = orig_to_tai;
    oc::TimePoint tai_to_tt = orig_to_tai;
    tai_to_utc.convert_to(oc::TimePoint::clock_type::utc);
    tai_to_tai93.convert_to(oc::TimePoint::clock_type::tai93);
    tai_to_gps.convert_to(oc::TimePoint::clock_type::gps);
    tai_to_tt.convert_to(oc::TimePoint::clock_type::tt);
    cout << "tai to utc: " << tai_to_utc << " (should be 03:23:19)\n";
    cout << "tai to t93: " << tai_to_tai93 << " (should be 03:23:29)\n";
    cout << "tai to gps: " << tai_to_gps << " (should be 03:23:37)\n";
    cout << "tai to tt:  " << tai_to_tt << " (should be 03:24:28.184)\n\n";

    oc::TimePoint gps_to_utc = orig_to_gps;
    oc::TimePoint gps_to_tai = orig_to_gps;
    oc::TimePoint gps_to_tai93 = orig_to_gps;
    oc::TimePoint gps_to_tt = orig_to_gps;
    gps_to_utc.convert_to(oc::TimePoint::clock_type::utc);
    gps_to_tai.convert_to(oc::TimePoint::clock_type::tai);
    gps_to_tai93.convert_to(oc::TimePoint::clock_type::tai93);
    gps_to_tt.convert_to(oc::TimePoint::clock_type::tt);
    cout << "gps to utc: " << gps_to_utc << " (should be 03:23:19)\n";
    cout << "gps to tai: " << gps_to_tai << " (should be 03:23:56)\n";
    cout << "gps to t93: " << gps_to_tai93 << " (should be 03:23:29)\n";
    cout << "gps to tt:  " << gps_to_tt << " (should be 03:24:28.184)\n\n";

    cout << gps_to_utc.to_string() << "\n";
    cout << gps_to_utc.to_string("%Y-%m-%dT%H:%M:%S%F (%t)") << "\n";
    cout << "\n\n\n";
}

void leap_conversions(){
    oc::TimePoint original { date { 2008, 12, 31 }, time_duration { 23, 59, 55 } };
    cout << "original    utc: " << original << " (should be 23:59:55)\n";
    oc::TimePoint orig_to_tai = original;
    orig_to_tai.convert_to(oc::TimePoint::clock_type::tai);
    cout << "original to tai: " << orig_to_tai << " (should be 00:00:28)\n";
    oc::TimePoint orig_to_gps = original;
    orig_to_gps.convert_to(oc::TimePoint::clock_type::gps);
    cout << "original to gps: " << orig_to_gps << " (should be 00:00:09)\n\n\n";

    oc::TimePoint tai_to_utc = orig_to_tai;
    tai_to_utc.convert_to(oc::TimePoint::clock_type::utc);
    cout << "tai to utc: " << tai_to_utc << " (should be 23:59:55)\n";
    oc::TimePoint tai_to_gps = orig_to_tai;
    tai_to_gps.convert_to(oc::TimePoint::clock_type::gps);
    cout << "tai to gps: " << tai_to_gps << " (should be 00:00:09)\n\n\n";

    oc::TimePoint gps_to_utc = orig_to_gps;
    gps_to_utc.convert_to(oc::TimePoint::clock_type::utc);
    cout << "gps to utc: " << gps_to_utc << " (should be 23:59:55)\n";
    oc::TimePoint gps_to_tai = orig_to_gps;
    gps_to_tai.convert_to(oc::TimePoint::clock_type::tai);
    cout << "gps to tai: " << gps_to_tai << " (should be 00:00:28)\n\n\n";

    oc::TimePoint pre_leap_utc { date { 2008, 12, 31 }, time_duration { 23, 59, 59 } };
    oc::TimePoint post_leap_utc { date { 2009, 1, 1 }, time_duration { 0, 0, 0 } };
    cout << "23:59:59, UTC: " << pre_leap_utc << "\n";
    cout << "00:00:00, UTC: " << post_leap_utc << "\n";
    oc::TimePoint pre_leap_tai = pre_leap_utc;
    pre_leap_tai.convert_to(oc::TimePoint::clock_type::tai);
    cout << "pre leap, in TAI: " << pre_leap_tai << "\n";
    oc::TimePoint post_leap_tai = post_leap_utc;
    post_leap_tai.convert_to(oc::TimePoint::clock_type::tai);
    cout << "post leap, in TAI: " << post_leap_tai << "\n\n\n";

    oc::TimePoint actual_leap { date { 2008, 12, 31 }, time_duration { 23, 59, 60 } };
    cout << "23:59:60, time_duration: " << time_duration{23,59,60} << "\n";
    cout << "23:59:60, UTC: " << actual_leap << "\n";
    oc::TimePoint actual_leap_tai1 { date { 2009, 1, 1 }, time_duration { 0, 0, 32 }, oc::TimePoint::clock_type::tai };
    cout << "pre leap, in TAI: " << actual_leap_tai1 << "\n";
    oc::TimePoint actual_leap_utc1 = actual_leap_tai1;
    actual_leap_utc1.convert_to(oc::TimePoint::clock_type::utc);
    cout << "pre leap conv, in UTC: " << actual_leap_utc1 << "\n";
    oc::TimePoint actual_leap_tai2 { date { 2009, 1, 1 }, time_duration { 0, 0, 34 }, oc::TimePoint::clock_type::tai };
    cout << "post leap, in TAI: " << actual_leap_tai2 << "\n";
    oc::TimePoint actual_leap_utc2 = actual_leap_tai2;
    actual_leap_utc2.convert_to(oc::TimePoint::clock_type::utc);
    cout << "post leap conv, in UTC: " << actual_leap_utc2 << "\n";
    cout << "Two seconds happened between the UTC times...\n\n\n";

}
void parse_testing(){
    cout << "Jan 2 03:04:05[.06]\n";
    cout << oc::TimePoint{"2019-01-02T03:04:05.06Z"} << "\n";
    cout << oc::TimePoint{"2019-01-02T03:04:05Z"} << "\n";
    cout << oc::TimePoint{"2019-01-02 03:04:05Z"} << "\n";
    cout << oc::TimePoint{"20190102T030405"} << "\n";
    cout << oc::TimePoint{"20190102T030405-0700"} << "\n";
    cout << oc::TimePoint{"20190102030405"} << "\n";
    cout << oc::TimePoint{"2019-002 03:04:05Z"} << "\n";
    cout << oc::TimePoint{"2019002 03:04:05Z"} << "\n";
    cout << oc::TimePoint{"2019002030405"} << "\n";
    cout << "Jan 2\n";
    cout << oc::TimePoint{"2019-01-02"} << "\n";
    cout << oc::TimePoint{"20190102"} << "\n";
    cout << oc::TimePoint{"2019-002"} << "\n";
}
void default_times(){
    cout << oc::TimePoint{} << "\n";
    cout << oc::TimePoint{oc::TimePoint::clock_type::local} << "\n";
    cout << oc::TimePoint{oc::TimePoint::clock_type::utc} << "\n";
    cout << oc::TimePoint{oc::TimePoint::clock_type::tai} << "\n";
    cout << oc::TimePoint{oc::TimePoint::clock_type::gps} << "\n";
}
void seconds_since(){
    auto t = oc::TimePoint{"2019-01-01T00:00:00", oc::TimePoint::clock_type::utc};
    cout << t << '\n';
    cout << std::fixed;
    cout << "seconds since 1970: " << t.seconds_since_1970() << '\n';
    cout << "seconds since 1993: " << t.seconds_since_1993() << '\n';
    cout << "julian date: " << t.julian_date() << '\n';
    cout << "truncated julian date: " << t.truncated_julian_date() << '\n';
    cout << "modified julian date: " << t.modified_julian_date() << '\n';
    cout << "reduced julian date: " << t.reduced_julian_date() << '\n';
}
int main() {
    // parse_testing();
    simple_conversions();
    // leap_conversions();
    // default_times();
    // seconds_since();

    return 0;
}
int testing_with_don(){
    ptime p1993{ date { 1993, 1, 1 }, time_duration { 0, 0, 0 } };
    ptime p1970{ date { 1970, 1, 1 }, time_duration { 0, 0, 0 } };
    cout << "1993 - 1970 = " << (p1993 - p1970).total_seconds() << "\n";

    // auto tai2015 = oc::TimePoint{"2015-07-01", oc::TimePoint::clock_type::tai};
    // auto utc1993 = oc::TimePoint{"1993-01-01", oc::TimePoint::clock_type::utc};
    // auto tai1993 = oc::TimePoint{"1993-01-01", oc::TimePoint::clock_type::tai93};
    // auto tai1993_fake = utc1993;
    // tai1993_fake.convert_to(oc::TimePoint::clock_type::tai);
    // cout << utc1993 << " UTC = " <<  tai1993_fake << " TAI" << "\n";
    // cout << utc1993 << " UTC = " <<  tai1993 << " TAI93" << "\n";
    // cout << (tai2015.native() - tai1993_fake.native()).total_seconds() << "\n";
    // cout << (tai2015.native() - tai1993.native()).total_seconds() << "\n";

    auto utc = oc::TimePoint{"2015-06-30 23:59:55", oc::TimePoint::clock_type::utc}.native();
    for (int i=0;i<10;i++){
        auto tai93 = oc::TimePoint{utc}.convert_to(oc::TimePoint::clock_type::tai93);
        auto tai93n = tai93.native();
        auto back_to_utc = tai93;
        back_to_utc = back_to_utc.convert_to(oc::TimePoint::clock_type::utc);
        cout << utc << " - unix=" << (utc - p1970).total_seconds() << ", TAI93=" << (tai93n - p1993).total_seconds() << " (" << tai93n << ") => " << back_to_utc.to_string() << "\n";
        utc = utc + boost::posix_time::time_duration{0,0,1};
    }

    auto tai93 = oc::TimePoint{"2015-06-30 23:59:59", oc::TimePoint::clock_type::tai93}.native();
    for (int i=0;i<15;i++){
        auto utc = oc::TimePoint{tai93, oc::TimePoint::clock_type::tai93}.convert_to(oc::TimePoint::clock_type::utc);
        cout << utc.to_string() << " - unix=" << (utc.native() - p1970).total_seconds() << ", TAI93=" << (tai93 - p1993).total_seconds() << " (" << tai93 << ")\n";
        tai93 = tai93 + boost::posix_time::time_duration{0,0,1};
    }


    // cout << d.total_seconds() << "\n";
    //
    // auto u = t.convert_to(oc::TimePoint::clock_type::utc);
    // cout << u->to_string() << "\n";
    // auto d2 = u->native() - p1970;
    // cout << d2.total_seconds() + 35 << "\n";

    




    // cout << pt1.get_utc() << endl;
    // cout << pt1.get_tai() << endl;
    // input_testing();
    // output_testing();
    // leap_testing();

    // ptime t1(second_clock::local_time());
    // ptime t2(second_clock::universal_time());
    // std::cout << formatted_datetime(t1) << std::endl;
    // std::cout << formatted_datetime(t2) << std::endl;
    //
    // date d(2000,Jan,20);
    // ptime start(d);
    // ptime end = start + hours(1);
    // time_iterator titr(start,minutes(15)); //increment by 15 minutes
    // //produces 00:00:00, 00:15:00, 00:30:00, 00:45:00
    // while (titr < end) {
    //     std::cout << to_simple_string(*titr) << std::endl;
    //     ++titr;
    // }


    // ptime start { date { 1990, 12, 31 }, time_duration { 23, 59, 50 } };
    // ptime end   { date { 1991, 1,  1 }, time_duration {  0, 0, 10 } };
    // time_iterator titr(start,seconds(1));
    // while (titr <= end) {
    //     std::cout << formatted_datetime(*titr) << std::endl;
    //     ++titr;
    // }
    return 0;
}
void leap_testing(){
    tz_database tz_db;
    try {
      tz_db.load_from_file("date_time_zonespec.csv");
    }catch(data_not_accessible dna) {
      std::cerr << "Error with time zone data file: " << dna.what() << std::endl;
      exit(EXIT_FAILURE);
    }catch(bad_field_count bfc) {
      std::cerr << "Error with time zone data file: " << bfc.what() << std::endl;
      exit(EXIT_FAILURE);
    }

    time_zone_ptr nyc_tz = tz_db.time_zone_from_region("America/New_York");
    // date in_date(1990,12,31);
    // time_duration td(23,59,59);
    date in_date(1991,1,1);
    time_duration td(0,0,1);
    // construct with local time value
    // create not-a-date-time if invalid (eg: in dst transition)
    local_date_time nyc_time(in_date, td, nyc_tz, local_date_time::NOT_DATE_TIME_ON_ERROR);

    std::cout << nyc_time << std::endl;

    ptime time_t_epoch(date(1970,1,1)); 
    std::cout << time_t_epoch << std::endl;

    // first convert nyc_time to utc via the utc_time() 
    // call and subtract the ptime.
    time_duration diff = nyc_time.utc_time() - time_t_epoch;
    time_duration diff2 = nyc_time.local_time() - time_t_epoch;

    //Expected 1096906472
    std::cout << "Seconds diff:  " << diff.total_seconds() << std::endl;
    std::cout << "Seconds diff2: " << diff2.total_seconds() << std::endl;
    std::cout << "diff diff: " << diff2.total_seconds() - diff.total_seconds() << std::endl;
}

// void other_lib_example(){
// {
//     using namespace std::chrono;
//     using namespace date;
//
//     auto nyc_time = make_zoned("America/New_York",
//                                local_days(2004_y/oct/4) + 12h + 14min + 32s);
//     std::cout << nyc_time << '\n';
//     auto sys_epoch = sys_days{};
//     std::cout << sys_epoch << '\n';
//
//     // first convert nyc_time to utc via the get_sys_time()
//     // call and subtract the sys_epoch.
//     auto sys_time = nyc_time.get_sys_time();
//
//     // Expected 1096906472
//     std::cout << "Seconds diff: " << sys_time - sys_epoch << '\n';
//
//     // Take leap seconds into account
//     auto utc_time = to_utc_time(sys_time);
//     auto utc_epoc = to_utc_time(sys_epoch);
//
//     // Expected 1096906494, an extra 22s
//     std::cout << "Seconds diff: " << utc_time - utc_epoc << '\n';
// }

void input_testing(){
    // const std::string time = "16:43 December 12, 2012";
    // boost::posix_time::time_input_facet* facet = new boost::posix_time::time_input_facet("%H:%M %B %d, %Y");
    const std::string time = "2009-03-08T02:30:00.500-0500";
    boost::posix_time::time_input_facet* facet = new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S%F%q");
    std::stringstream ss;
    ss.imbue(std::locale(std::locale(), facet));
    ss << time;
    boost::posix_time::ptime pt;
    ss >> pt;
    std::cout << formatted_datetime(pt) << std::endl;
}
void output_testing(){
    std::stringstream ss { "2009-03-08T02:30:00Z" };
    cout << ss.str() << endl;

    ptime t = microsec_clock::universal_time();
    std::cout << to_iso_extended_string(t) << "Z\n";

    ptime pt1 { date { 2009, 3, 8 }, time_duration { 02, 30, 0 } };
    ptime pt2 { date { 2009, 3, 8 }, time_duration { 03, 00, 0, 500 } };
    time_duration td = pt2 - pt1;
    cout << td.hours() << '\n';
    cout << td.minutes() << '\n';
    cout << td.seconds() << '\n';
    std::cout << to_iso_extended_string(pt1) << "\n";
    // std::cout << to_iso_extended_string(pt2) << "\n";
    std::cout << formatted_datetime(pt2) << "\n";
    auto pt3 = boost::posix_time::from_iso_string("20090308T023000.500");
    std::cout << to_iso_extended_string(pt3) << "\n";
    // auto pt4 = boost::posix_time::time_from_string("2009-03-08 04:30:00.500");
    // std::cout << to_iso_extended_string(pt4) << "\n";
    // std::cout << pt3.time_of_day().num_fractional_digits(3);
    std::cout << formatted_datetime(pt3) << "\n";
    std::cout << formatted_datetime(pt3) << "\n";

    cout << "Local times and zones:\n";
    std::cout << microsec_clock::local_time() << endl;
    std::cout << microsec_clock::local_time().zone_as_posix_string() << endl;
    std::cout << microsec_clock::local_time().zone_abbrev() << endl;
    std::cout << microsec_clock::local_time().zone_name() << endl;
}
