
#include <oc.hpp>

#include <check.h>

#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;

Suite *stub_suite(void);

START_TEST(test1) {
    try {
        oc::ShareTree tree{};
        std::for_each(tree.begin(), tree.end(), [](const auto s){std::cout << s.first << ": " << s.second << '\n';});
        std::cout << tree.get_path_to_file("MODISA", "fake/file") << '\n';
    } catch (const std::logic_error&){
        std::cout << "Failed to open ShareTree\n";
    }
    ck_assert(1 == 1);
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

