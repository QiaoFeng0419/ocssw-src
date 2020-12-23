
#include <oc.hpp>

#include <check.h>

#include <iostream>
#include <string>

using namespace std;

Suite *stub_suite(void);

START_TEST(replace_all) {
    //! [oc::StringUtils example1]
    std::string test1{"test1 test2 test3"};
    oc::StringUtils::replace_all(test1, "tes", "set");
    ck_assert(test1 == "sett1 sett2 sett3");
    oc::StringUtils::replace_all(test1, "set", nullptr);
    ck_assert(test1 == "sett1 sett2 sett3");

    // To do these tests properly, it'd need to know the expected output, which it doesn't.
    // All these do is verify that it changes and that it's perfectly reversible.
    std::string test2{"$OCSSWROOT/fake_dir"};
    oc::StringUtils::replace_oc_roots(test2);
    ck_assert(test2 != "$OCSSWROOT/fake_dir");
    oc::StringUtils::insert_oc_roots(test2);
    ck_assert(test2 == "$OCSSWROOT/fake_dir");
    //! [oc::StringUtils example1]
} END_TEST

START_TEST(string_to_arrays) {
    //! [oc::StringUtils example2]
    std::string test1{"test1 test2 test3"};
    auto s1 = oc::StringUtils::stov<std::string>(test1, " "s);
    ck_assert(s1[0] == "test1");
    ck_assert(s1[1] == "test2");
    ck_assert(s1[2] == "test3");

    std::string test2{"4,5,6"};
    auto i1 = oc::StringUtils::stov<int>(test2);
    ck_assert(i1[0] == 4);
    ck_assert(i1[1] == 5);
    ck_assert(i1[2] == 6);

    std::string test3{"4.5 ,5.5, 6.5"};
    auto f1 = oc::StringUtils::stov<float>(test3);
    ck_assert(f1[0] == 4.5);
    ck_assert(f1[1] == 5.5);
    ck_assert(f1[2] == 6.5);

    std::string test4{""};
    auto f2 = oc::StringUtils::stov<float>(test4);
    ck_assert(f2.size() == 0);
    //! [oc::StringUtils example2]
} END_TEST

START_TEST(strip_enclosure) {
    //! [oc::StringUtils example3]
    std::string test1{"[test1 test2 test3]"};
    oc::StringUtils::strip_brackets(test1);
    ck_assert(test1 == "test1 test2 test3");

    std::string test2{"\"test1 test2 test3\""};
    oc::StringUtils::strip_quotes(test2);
    ck_assert(test2 == "test1 test2 test3");
    ck_assert(oc::StringUtils::strip_quotes(test2) == "test1 test2 test3");
    ck_assert(oc::StringUtils::strip_quotes("\"qwer\"") == "qwer");
    //! [oc::StringUtils example3]
} END_TEST

Suite *stub_suite(void) {
    Suite *s = suite_create("Stub");

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, replace_all);
    tcase_add_test(tc_core, string_to_arrays);
    tcase_add_test(tc_core, strip_enclosure);
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

