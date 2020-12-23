# LibTimeutils_FOUND - true if library and headers were found
# LibTimeutils_INCLUDE_DIRS - include directories
# LibTimeutils_LIBRARIES - library directories

set(LibTimeutils_LIBRARY timeutils)
set(LibTimeutils_INCLUDE_DIR ${OEL_UTIL_SOURCE_DIR}/libtimeutils)

set(LibTimeutils_LIBRARIES ${LibTimeutils_LIBRARY})
set(LibTimeutils_INCLUDE_DIRS ${LibTimeutils_INCLUDE_DIR})

list(APPEND LibTimeutils_LIBRARIES m)

set(LibTimeutils_FOUND TRUE)

mark_as_advanced(FORCE LibTimeutils_INCLUDE_DIR LibTimeutils_LIBRARY)
