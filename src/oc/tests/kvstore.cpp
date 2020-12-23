
#include <oc.hpp>

#include <check.h>

#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;

Suite *stub_suite(void);

START_TEST(test1) {
    oc::KvStore par{"kv_store/test1.par"};
    ck_assert(par["key1"] == "value1");
    ck_assert(par["quoted_key1"] == "quoted value1");
    ck_assert(par["quoted_key2"] == "quoted value2");
    ck_assert(par["replaced_key1"] == "new and improved");
} END_TEST

START_TEST(test2) {
    //! [oc::KvStore example1]
    oc::KvStore par{"kv_store/test2.par"};
    ck_assert(par["key1"] == "value1");
    ck_assert(par["quoted_key1"] == "quoted value1");
    ck_assert(par["quoted_key2"] == "quoted value2");
    ck_assert(par["replaced_key1"] == "new and improved");
    ck_assert(par["additional_key"] == "additional_value1");
    ck_assert(par["g1.group1_key1"] == "g1v1");
    ck_assert(par["g2.group2_key1"] == "g2v1");
    //! [oc::KvStore example1]
} END_TEST

START_TEST(test3) {
    //! [oc::KvStore example2]
    int count = 0;
    oc::KvStore par{"kv_store/test1.par", [&count](std::pair<std::string, std::string> kv){
        std::cout << kv.first << " = " << kv.second << std::endl;
        count++;
        return false;
    }};
    ck_assert_int_eq(count, 5);
    //! [oc::KvStore example2]
} END_TEST

Suite *stub_suite(void) {
    Suite *s = suite_create("Stub");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test1);
    tcase_add_test(tc_core, test2);
    tcase_add_test(tc_core, test3);
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

