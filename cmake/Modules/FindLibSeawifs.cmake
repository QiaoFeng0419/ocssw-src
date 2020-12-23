# LibSeawifs_FOUND - true if library and headers were found
# LibSeawifs_INCLUDE_DIRS - include directories
# LibSeawifs_LIBRARIES - librarys

set(LibSeawifs_LIBRARY seawifs)
set(LibSeawifs_INCLUDE_DIR ${OCSSW_SOURCE_DIR}/src/libseawifs)

set(LibSeawifs_LIBRARIES ${LibSeawifs_LIBRARY})
set(LibSeawifs_INCLUDE_DIRS ${LibSeawifs_INCLUDE_DIR})

find_package(LibNav REQUIRED)
list(APPEND LibSeawifs_LIBRARIES ${LibNav_LIBRARIES})
list(APPEND LibSeawifs_INCLUDE_DIRS ${LibNav_INCLUDE_DIRS})

find_package(LibHdf4utils REQUIRED)
list(APPEND LibSeawifs_LIBRARIES ${LibHdf4utils_LIBRARIES})
list(APPEND LibSeawifs_INCLUDE_DIRS ${LibHdf4utils_INCLUDE_DIRS})

find_package(LibTimeutils REQUIRED)
list(APPEND LibSeawifs_LIBRARIES ${LibTimeutils_LIBRARIES})
list(APPEND LibSeawifs_INCLUDE_DIRS ${LibTimeutils_INCLUDE_DIRS})

set(LibSeawifs_FOUND TRUE)

mark_as_advanced(FORCE LibSeawifs_INCLUDE_DIR LibSeawifs_LIBRARY)

