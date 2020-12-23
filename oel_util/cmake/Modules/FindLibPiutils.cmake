# LibPiutils_FOUND - true if library and headers were found
# LibPiutils_INCLUDE_DIRS - include directories
# LibPiutils_LIBRARIES - library directories

set(LibPiutils_LIBRARY piutils)
set(LibPiutils_INCLUDE_DIR ${OEL_UTIL_SOURCE_DIR}/libpiutils)

set(LibPiutils_LIBRARIES ${LibPiutils_LIBRARY})
set(LibPiutils_INCLUDE_DIRS ${LibPiutils_INCLUDE_DIR})

find_package(LibGenutils REQUIRED)
list(APPEND LibPiutils_LIBRARIES ${LibGenutils_LIBRARIES})
list(APPEND LibPiutils_INCLUDE_DIRS ${LibGenutils_INCLUDE_DIRS})

find_package(PugiXML REQUIRED)
list(APPEND LibPiutils_LIBRARIES ${PUGIXML_LIBRARIES})
list(APPEND LibPiutils_INCLUDE_DIRS ${PUGIXML_INCLUDE_DIRS})

set(LibPiutils_FOUND TRUE)

mark_as_advanced(FORCE LibPiutils_INCLUDE_DIR LibPiutils_LIBRARY)

