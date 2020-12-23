# LibViirsCmn_FOUND - true if library and headers were found
# LibViirsCmn_INCLUDE_DIRS - include directories
# LibViirsCmn_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libViirsCmn)
    set(LibViirsCmn_LIBRARY ViirsCmn)
    set(LibViirsCmn_INCLUDE_DIR "")
    set(LibViirsCmn_LIBRARIES ${LibViirsCmn_LIBRARY})
    set(LibViirsCmn_INCLUDE_DIRS "")
else()
    set(LibViirsCmn_LIBRARY $ENV{LIB3_DIR}/lib/libViirsCmn.a)
    set(LibViirsCmn_INCLUDE_DIR $ENV{LIB3_DIR}/include/ViirsCmn)
    set(LibViirsCmn_LIBRARIES ${LibViirsCmn_LIBRARY})
    set(LibViirsCmn_INCLUDE_DIRS ${LibViirsCmn_INCLUDE_DIR})

    find_package(LibNovas REQUIRED)
    list(APPEND LibViirsCmn_LIBRARIES ${LibNovas_LIBRARIES})
    list(APPEND LibViirsCmn_INCLUDE_DIRS ${LibNovas_INCLUDE_DIRS})

    find_package(NetCDF REQUIRED COMPONENTS C CXX)
    list(APPEND LibViirsCmn_LIBRARIES ${NETCDF_LIBRARIES})
    list(APPEND LibViirsCmn_INCLUDE_DIRS ${NETCDF_INCLUDE_DIRS})
endif()

if(NOT (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin"))
    include(CheckFunctionExists)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        list(APPEND LibViirsCmn_LIBRARIES rt)
    endif()
endif()

set(LibViirsCmn_FOUND TRUE)

mark_as_advanced(FORCE LibViirsCmn_INCLUDE_DIR LibViirsCmn_LIBRARY)
