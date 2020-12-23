
#include <oc.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

int main(){
    // These tests are finicky.  If the system running them is too slow/busy, the class could work but fail these tests.

    //! [Stopwatch example]
    oc::Stopwatch sw;
    assert(sw.ms() == 0);

    // do some slow operations...
    std::this_thread::sleep_for(std::chrono::milliseconds(1250));

    assert(sw.ms() >= 1250 && sw.ms() <= 1260);
    assert(sw.pretty(2) == "1.25");
    assert(sw.now<std::chrono::seconds>().count() == 1);
    sw.reset();
    assert(sw.ms() == 0);
    //! [Stopwatch example]

    //! [Stopwatch reconstructing stopwatches]
    sw.reset(std::chrono::steady_clock::time_point{std::chrono::seconds{72}});
    assert(sw.pretty(0) == "1:12");

    sw.reset(std::chrono::seconds{7272});
    assert(sw.pretty(0) == "2:01:12");
    //! [Stopwatch reconstructing stopwatches]

    // Tests for pretty printing and zero padding
    sw.reset(8888);
    assert(sw.pretty(1) == "2:28:08.0");

    sw.reset(7202);
    assert(sw.pretty(0) == "2:00:02");

    return 0;
}

