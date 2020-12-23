# LibNetcdfutils_FOUND - true if library and headers were found
# LibNetcdfutils_INCLUDE_DIRS - include directories
# LibNetcdfutils_LIBRARIES - library directories

set(LibNetcdfutils_LIBRARY netcdfutils)
set(LibNetcdfutils_INCLUDE_DIR ${OEL_UTIL_SOURCE_DIR}/libnetcdfutils)

set(LibNetcdfutils_LIBRARIES ${LibNetcdfutils_LIBRARY})
set(LibNetcdfutils_INCLUDE_DIRS ${LibNetcdfutils_INCLUDE_DIR})

find_package(LibPiutils REQUIRED)
list(APPEND LibNetcdfutils_LIBRARIES ${LibPiutils_LIBRARIES})
list(APPEND LibNetcdfutils_INCLUDE_DIRS ${LibPiutils_INCLUDE_DIRS})

find_package(LibTimeutils REQUIRED)
list(APPEND LibNetcdfutils_LIBRARIES ${LibTimeutils_LIBRARIES})
list(APPEND LibNetcdfutils_INCLUDE_DIRS ${LibTimeutils_INCLUDE_DIRS})

find_package(NetCDF REQUIRED COMPONENTS C CXX)
list(APPEND LibNetcdfutils_LIBRARIES ${NETCDF_LIBRARIES})
list(APPEND LibNetcdfutils_INCLUDE_DIRS ${NETCDF_INCLUDE_DIRS})

set(LibNetcdfutils_FOUND TRUE)

mark_as_advanced(FORCE LibNetcdfutils_INCLUDE_DIR LibNetcdfutils_LIBRARY)

