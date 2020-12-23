# LibDfutils_FOUND - true if library and headers were found
# LibDfutils_INCLUDE_DIRS - include directories
# LibDfutils_LIBRARIES - library directories

set(LibDfutils_LIBRARY dfutils)
set(LibDfutils_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libdfutils)

set(LibDfutils_LIBRARIES ${LibDfutils_LIBRARY})
set(LibDfutils_INCLUDE_DIRS ${LibDfutils_INCLUDE_DIR})

find_package(LibHdf4utils REQUIRED)
list(APPEND LibDfutils_LIBRARIES ${LibHdf4utils_LIBRARIES})
list(APPEND LibDfutils_INCLUDE_DIRS ${LibHdf4utils_INCLUDE_DIRS})

find_package(LibNetcdfutils REQUIRED)
list(APPEND LibDfutils_LIBRARIES ${LibNetcdfutils_LIBRARIES})
list(APPEND LibDfutils_INCLUDE_DIRS ${LibNetcdfutils_INCLUDE_DIRS})

find_package(LibHdf5utils REQUIRED)
list(APPEND LibDfutils_LIBRARIES ${LibHdf5utils_LIBRARIES})
list(APPEND LibDfutils_INCLUDE_DIRS ${LibHdf5utils_INCLUDE_DIRS})

set(LibDfutils_FOUND TRUE)

mark_as_advanced(FORCE LibDfutils_INCLUDE_DIR LibDfutils_LIBRARY)
