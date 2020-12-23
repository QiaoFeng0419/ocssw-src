# LibBin++_FOUND - true if library and headers were found
# LibBin++_INCLUDE_DIRS - include directories
# LibBin++_LIBRARIES - library directories
#
# LibBin64++_LIBRARIES - library directories
# LibBin64++_INCLUDE_DIRS - include directories

set(LibBin++_LIBRARY bin++)
set(LibBin64++_LIBRARY bin64++)
set(LibBin++_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libbin++)
set(LibBin64++_INCLUDE_DIR ${LibBin++_INCLUDE_DIR})

set(LibBin++_LIBRARIES ${LibBin++_LIBRARY})
set(LibBin64++_LIBRARIES ${LibBin64++_LIBRARY})
set(LibBin++_INCLUDE_DIRS ${LibBin++_INCLUDE_DIR})

find_package(NetCDF REQUIRED COMPONENTS CXX)
list(APPEND LibBin++_LIBRARIES ${NetCDF_LIBRARIES})
list(APPEND LibBin64++_LIBRARIES ${NetCDF_LIBRARIES})
list(APPEND LibBin++_INCLUDE_DIRS ${NetCDF_INCLUDE_DIRS})

find_package(LibBin REQUIRED)
list(APPEND LibBin++_LIBRARIES ${LibBin_LIBRARIES})
list(APPEND LibBin64++_LIBRARIES ${LibBin64_LIBRARIES})
list(APPEND LibBin++_INCLUDE_DIRS ${LibBin_INCLUDE_DIRS})

set(LibBin64++_INCLUDE_DIRS ${LibBin++_INCLUDE_DIRS})

set(LibBin++_FOUND TRUE)

mark_as_advanced(FORCE
  LibBin++_INCLUDE_DIR
  LibBin64++_INCLUDE_DIR
  LibBin++_LIBRARY
  LibBin64++_LIBRARY)
