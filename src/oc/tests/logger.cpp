
#include <oc.hpp>
// #include <oc/Log.hpp>

#include <iostream>
#include <memory>
#include <vector>

using namespace std;

int main(){
    //! [oc::Log test]
    // oc::Log log{true};
    oc::Log log{};
    // log.add(std::make_unique<oc::FileStreamLogger>());
    // log.add(std::make_unique<oc::FileLogger>("/tmp/loggertest.txt"));
    // log.add(oc::LogSeverity::min, std::make_unique<oc::FormattedStreamLogger>("%{gmt,}t [%l/%m/%u] (%s)"));

    // log.emergency("test1\n");
    // log.emergency("test2\n");
    // log.debug("test3\n");
    // log.emergency("test4\n");
    // log.emergency("test5\n");
    // log << "test8\n";
    // log << "test6\n";
    // log << 128 << '\n';
    // log.log(-3, "test8\n");
    // log.log(10, "test9\n");
    // log << "test10" << "\n";
    // log << oc::LogSeverity::warn;
    // log << "test11"s + "\n";

    const auto format = "%{gmt,}t [%l/%m/%u] %s";
    log.add(oc::LogSeverity::min, std::make_unique<oc::FormattedStreamLogger>(format));
    log.add(oc::LogSeverity::min, std::make_unique<oc::FormattedFileLogger>("/tmp/logger_test.txt", std::ios_base::out, format));
    log.log(10, "test9");
    log << "test10\n";
    log << "test11" << "test12";
    std::cout << '\n';
    //! [oc::Log test]

    return 0;
}
