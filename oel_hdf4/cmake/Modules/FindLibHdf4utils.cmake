# LibHdf4utils_FOUND - true if library and headers were found
# LibHdf4utils_INCLUDE_DIRS - include directories
# LibHdf4utils_LIBRARIES - library directories

set(LibHdf4utils_LIBRARY hdf4utils)
set(LibHdf4utils_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libhdf4utils)

set(LibHdf4utils_LIBRARIES ${LibHdf4utils_LIBRARY})
set(LibHdf4utils_INCLUDE_DIRS ${LibHdf4utils_INCLUDE_DIR})

find_package(LibPiutils REQUIRED)
list(APPEND LibHdf4utils_LIBRARIES ${LibPiutils_LIBRARIES})
list(APPEND LibHdf4utils_INCLUDE_DIRS ${LibPiutils_INCLUDE_DIRS})

find_package(HDF4 REQUIRED)
list(APPEND LibHdf4utils_LIBRARIES ${HDF4_LIBRARIES})
list(APPEND LibHdf4utils_INCLUDE_DIRS ${HDF4_INCLUDE_DIRS})

set(LibHdf4utils_FOUND TRUE)

mark_as_advanced(FORCE LibHdf4utils_INCLUDE_DIR LibHdf4utils_LIBRARY)

