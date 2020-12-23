
#include "oc.hpp"

#include <iostream>
#include <vector>

using namespace std;

// Can't store C++ functions/etc due to name mangling.
// Every class needs a C allocator and deleter function.
//  (One allocator function per constructor.)
// Use a smart pointer to tie them together for memory safety.
extern "C" {
    int get_42(int);
    int get_42(int a){
        return 42*a;
    }
}
int main(){
    cout << "test\n";
    auto c = get_42;
    cout << c(2) << "\n";
    return 0;
}
