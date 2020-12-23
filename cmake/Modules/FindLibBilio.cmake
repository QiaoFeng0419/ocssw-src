# LibBilio_FOUND - true if library and headers were found
# LibBilio_INCLUDE_DIRS - include directories
# LibBilio_LIBRARIES - librarys

set(LibBilio_LIBRARY bilio)
set(LibBilio_INCLUDE_DIR ${OCSSW_SOURCE_DIR}/src/libbilio)

set(LibBilio_LIBRARIES ${LibBilio_LIBRARY})
set(LibBilio_INCLUDE_DIRS ${LibBilio_INCLUDE_DIR})

find_package(LibNav REQUIRED)
list(APPEND LibBilio_LIBRARIES ${LibNav_LIBRARIES})
list(APPEND LibBilio_INCLUDE_DIRS ${LibNav_INCLUDE_DIRS})

find_package(Proj4 REQUIRED)
list(APPEND LibBilio_LIBRARIES ${Proj4_LIBRARIES})
list(APPEND LibBilio_INCLUDE_DIRS ${Proj4_INCLUDE_DIRS})

set(LibBilio_FOUND TRUE)

mark_as_advanced(FORCE LibBilio_INCLUDE_DIR LibBilio_LIBRARY)

