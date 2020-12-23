
#include <oc.hpp>

#include <check.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;

Suite *stub_suite(void);

START_TEST(test1) {
    //! [oc::LeapSecond example]
    oc::LeapSecondDatabase dat{};
    ck_assert_float_eq(dat.leap_seconds_since({boost::gregorian::date{1970, 1, 1}, boost::posix_time::time_duration{0, 0, 0}}), 4.21317);
    ck_assert_float_eq(dat.leap_seconds_since({boost::gregorian::date{1972, 1, 1}, boost::posix_time::time_duration{0, 0, 0}}), 10);

    ck_assert(dat.is_leap_second({boost::gregorian::date{1972, 1, 1}, boost::posix_time::time_duration{0, 0, 0}}) == true);
    ck_assert(dat.is_leap_second({boost::gregorian::date{1972, 1, 1}, boost::posix_time::time_duration{0, 0, 1}}) == false);
    ck_assert(dat.is_leap_second({boost::gregorian::date{1972, 1, 1}, boost::posix_time::time_duration{0, 0, 0, 1}}) == true); // fractional second is part of leap second
    //! [oc::LeapSecond example]
} END_TEST

Suite *stub_suite(void) {
    Suite *s = suite_create("Stub");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test1);
    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed;

    Suite *s = stub_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

