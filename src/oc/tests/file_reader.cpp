
#include <oc.hpp>

#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2){
        cout << "Usage: " << argv[0] << " <filename>\n";
        return 0;
    }
    oc::Stopwatch stopwatch{};
    oc::FileReaderHints hints{argv[1]};
    cout << "File name: " << hints.filename() << "\n";
    cout << "File size: " << hints.size() << "\n";
    cout << "First bytes:";
    for (auto i = hints.first_bytes().begin(); i != hints.first_bytes().end() && *i != EOF; i++){
        cout << " " << *i;
    }
    cout << "\n";

    cout << "File format: " << hints.format() << "\n";
    cout << "Attributes:\n";
    for (auto i = hints.attributes().cbegin(); i != hints.attributes().cend(); i++){
        cout << "\t" << i->first << " = " << i->second << "\n";
    }
    cout << "\n";

    cout << "Checking magic numbers:\n";
    cout << "#! : " << hints.magic_number("#!") << "\n";
    cout << "0x0E 0x03 0x13 0x01 : " << hints.magic_number({0x0E, 0x03, 0x13, 0x01}) << "\n";
    cout << "\n";
    cout << "Time elapsed: " << stopwatch << "s\n";

    return 0;
}

