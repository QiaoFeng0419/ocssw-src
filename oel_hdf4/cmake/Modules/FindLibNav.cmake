# LibNav_FOUND - true if library and headers were found
# LibNav_INCLUDE_DIRS - include directories
# LibNav_LIBRARIES - library directories

set(LibNav_LIBRARY nav)
set(LibNav_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libnav)

set(LibNav_LIBRARIES ${LibNav_LIBRARY})
set(LibNav_INCLUDE_DIRS ${LibNav_INCLUDE_DIR})

find_package(LibGenutils REQUIRED)
list(APPEND LibNav_LIBRARIES ${LibGenutils_LIBRARIES})
list(APPEND LibNav_INCLUDE_DIRS ${LibGenutils_INCLUDE_DIRS})

find_package(GSL REQUIRED)
list(APPEND LibNav_LIBRARIES ${GSL_LIBRARIES})
list(APPEND LibNav_INCLUDE_DIRS ${GSL_INCLUDE_DIRS})

list(APPEND LibNav_LIBRARIES m)

set(LibNav_FOUND TRUE)

mark_as_advanced(FORCE LibNav_INCLUDE_DIR LibNav_LIBRARY)
