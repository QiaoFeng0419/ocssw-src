# LibL2_FOUND - true if library and headers were found
# LibL2_INCLUDE_DIRS - include directories
# LibL2_LIBRARIES - librarys

set(LibL2_LIBRARY l2)
set(LibL2_INCLUDE_DIR ${OCSSW_SOURCE_DIR}/src/libl2)

set(LibL2_LIBRARIES ${LibL2_LIBRARY})
set(LibL2_INCLUDE_DIRS ${LibL2_INCLUDE_DIR})

find_package(LibSeawifs REQUIRED)
list(APPEND LibL2_LIBRARIES ${LibSeawifs_LIBRARIES})
list(APPEND LibL2_INCLUDE_DIRS ${LibSeawifs_INCLUDE_DIRS})

find_package(LibDfutils REQUIRED)
list(APPEND LibL2_LIBRARIES ${LibDfutils_LIBRARIES})
list(APPEND LibL2_INCLUDE_DIRS ${LibDfutils_INCLUDE_DIRS})

set(LibL2_FOUND TRUE)

mark_as_advanced(FORCE LibL2_INCLUDE_DIR LibL2_LIBRARY)

