# LibSGP4_FOUND - true if library and headers were found
# LibSGP4_INCLUDE_DIRS - include directories
# LibSGP4_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libSGP4)
    set(LibSGP4_LIBRARY SGP4)
    set(LibSGP4_INCLUDE_DIR ${VIIRS_LIB_SOURCE_DIR}/libSGP4)
else()
    set(LibNovas_LIBRARY $ENV{LIB3_DIR}/lib/libSGP4.a)
    set(LibNovas_INCLUDE_DIR $ENV{LIB3_DIR}/include/SGP4)
endif()

set(LibSGP4_LIBRARIES ${LibSGP4_LIBRARY} m)
set(LibSGP4_INCLUDE_DIRS ${LibSGP4_INCLUDE_DIR})

set(LibSGP4_FOUND TRUE)

mark_as_advanced(FORCE LibSGP4_INCLUDE_DIR LibSGP4_LIBRARY)
