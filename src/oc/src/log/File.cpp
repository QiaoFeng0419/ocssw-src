
#include "oc/Log.hpp"
#include "oc/log/File.hpp"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace oc {

FileLogger::~FileLogger(){
    handle_.close();
}

void FileLogger::log(int severity, const std::string& s){
    (void)severity;
    handle_ << s;
}

} // namespace oc

