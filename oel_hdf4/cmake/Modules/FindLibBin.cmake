# LibBin_FOUND - true if library and headers were found
# LibBin_INCLUDE_DIRS - include directories
# LibBin_LIBRARIES - library directories
#
# LibBin64_INCLUDE_DIRS - include directories
# LibBin64_LIBRARIES - library directories

set(LibBin_LIBRARY bin)
set(LibBin64_LIBRARY bin64)
set(LibBin_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libbin)
set(LibBin64_INCLUDE_DIR ${LibBin_INCLUDE_DIR})

set(LibBin_LIBRARIES ${LibBin_LIBRARY})
set(LibBin64_LIBRARIES ${LibBin64_LIBRARY})
set(LibBin_INCLUDE_DIRS ${LibBin_INCLUDE_DIR})

find_package(LibDfutils REQUIRED)
list(APPEND LibBin_LIBRARIES ${LibDfutils_LIBRARIES})
list(APPEND LibBin64_LIBRARIES ${LibDfutils_LIBRARIES})
list(APPEND LibBin_INCLUDE_DIRS ${LibDfutils_INCLUDE_DIRS})

set(LibBin64_INCLUDE_DIRS ${LibBin_INCLUDE_DIRS})

set(LibBin_FOUND TRUE)

mark_as_advanced(FORCE
  LibBin_INCLUDE_DIR
  LibBin64_INCLUDE_DIR
  LibBin_LIBRARY
  LibBin64_LIBRARY)
