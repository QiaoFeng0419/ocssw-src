# Proj4_FOUND - true if library and headers were found
# Proj4_INCLUDE_DIRS - include directories
# Proj4_LIBRARIES - library directories

find_path(Proj4_INCLUDE_DIR proj_api.h
  HINTS ${PROJ4_DIR}/include
  PATHS $ENV{LIB3_DIR}/include
  )

find_library(Proj4_LIBRARY NAMES proj
  HINTS ${PROJ4_DIR}/lib
  PATHS $ENV{LIB3_DIR}/lib
  )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Proj4 DEFAULT_MSG Proj4_LIBRARY Proj4_INCLUDE_DIR)
if(Proj4_FOUND)
  set(Proj4_LIBRARIES ${Proj4_LIBRARY})
  set(Proj4_INCLUDE_DIRS ${Proj4_INCLUDE_DIR})
endif()

mark_as_advanced(Proj4_INCLUDE_DIR Proj4_LIBRARY)
