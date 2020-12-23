
#include <oc.hpp>

#include <check.h>

#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;

Suite *stub_suite(void);

START_TEST(test1) {
    //! [FilterIterator example]
    vector<int> numbers{1, 3, 10, 7, 4, 20, 30};
    auto find_greater_than_9 = [](int n){return n > 9;};
    oc::FilterIterator<std::vector<int>::iterator, std::function<bool(int)>> iter{numbers.begin(), numbers.end(), find_greater_than_9};
    ck_assert_int_eq(**iter, 10);
    ++iter;
    ck_assert_int_eq(**iter, 20);
    ++iter;
    ck_assert_int_eq(**iter, 30);
    ++iter;
    ck_assert(iter == iter.end());
    //! [FilterIterator example]
}
END_TEST

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

