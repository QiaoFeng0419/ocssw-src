
#include "oc/Log.hpp"
#include "oc/log/Stream.hpp"

#include <iostream>
#include <vector>

namespace oc {

FileStreamLogger::~FileStreamLogger(){
    stream_.flush();
}

void FileStreamLogger::log(int severity, const std::string& s){
    (void)severity;
    stream_ << s;
}

} // namespace oc

