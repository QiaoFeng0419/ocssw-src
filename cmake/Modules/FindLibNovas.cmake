# LibNovas_FOUND - true if library and headers were found
# LibNovas_INCLUDE_DIRS - include directories
# LibNovas_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libnovas)
    set(LibNovas_LIBRARY novas)
    set(LibNovas_INCLUDE_DIR "")
else()
    set(LibNovas_LIBRARY $ENV{LIB3_DIR}/lib/libnovas.a)
    set(LibNovas_INCLUDE_DIR $ENV{LIB3_DIR}/include/novas)
endif()

set(LibNovas_FOUND TRUE)
set(LibNovas_LIBRARIES ${LibNovas_LIBRARY} m)
set(LibNovas_INCLUDE_DIRS ${LibNovas_INCLUDE_DIR})

mark_as_advanced(FORCE LibNovas_INCLUDE_DIR LibNovas_LIBRARY)
