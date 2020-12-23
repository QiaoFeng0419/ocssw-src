# LibGenutils_FOUND - true if library and headers were found
# LibGenutils_INCLUDE_DIRS - include directories
# LibGenutils_LIBRARIES - library directories

set(LibGenutils_LIBRARY genutils)
set(LibGenutils_INCLUDE_DIR ${OEL_UTIL_SOURCE_DIR}/libgenutils)

set(LibGenutils_LIBRARIES ${LibGenutils_LIBRARY})
set(LibGenutils_INCLUDE_DIRS ${LibGenutils_INCLUDE_DIR})

set(LibGenutils_FOUND TRUE)

mark_as_advanced(FORCE LibGenutils_INCLUDE_DIR LibGenutils_LIBRARY)
