
#include <oc.hpp>

#include <check.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace std;

Suite *stub_suite(void);

START_TEST(test1) {
    //! [DatFile example]
    oc::DatFile dat{"datfile.dat"};
    cout << dat << '\n';
    ck_assert(dat["unindexed_variable"][0] == 42);
    ck_assert(dat["indexed"][1] == 4);
    ck_assert(dat["indexed"][2] == 16);
    ck_assert(dat["indexed"][3] == 64);
    ck_assert(dat["indexed"][4] == 256);
    auto& indexed = dat["indexed"];
    ck_assert(indexed[1] == 4);
    ck_assert(indexed[2] == 16);
    ck_assert(indexed[3] == 64);
    ck_assert(indexed[4] == 256);
    //! [DatFile example]
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

