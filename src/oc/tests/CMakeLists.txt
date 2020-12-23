
set(CHECK_MIN_VERSION 0.9.11)
find_package(Check ${CHECK_MIN_VERSION})
find_package(Valgrind)

include_directories("../include")
include_directories("../../libtimeutils")

# These shouldn't be needed later
include(CData OPTIONAL RESULT_VARIABLE CDATA_FOUND)
set(CDATA_DEFAULT_URL "https://oceandata.sci.gsfc.nasa.gov/ocssw_test/")
set(CDATA_DEFAULT_STRIP_PARTS 1)
set(CDATA_DEFAULT_BASE "$ENV{OCSSWROOT}/testdata")

if (CHECK_FOUND AND NOT CHECK_MIN_VERSION VERSION_GREATER CHECK_VERSION)

    # include(GetSubdirs)

    if (CDATA_FOUND)
        cdata(READ data_files "${CMAKE_CURRENT_LIST_DIR}/CDataList.txt"
         GENERATE "${CMAKE_CURRENT_BINARY_DIR}/oc-data"
         VERBOSE
         TEST oc-data
         DIR "$ENV{OCSSWROOT}/testdata/oc"
         SAVE_DIR CDATA_DEST_DIR)
    endif (CDATA_FOUND)

    # message(STATUS "Check ${CHECK_VERSION} found, building test suite")
    include_directories(SYSTEM ${CHECK_INCLUDE_DIRS})
    link_directories(${CHECK_LIBRARY_DIRS})

    if (VALGRIND_FOUND)
        add_definitions(-DHAVE_VALGRIND_H)
        set(VALGRIND_OPTIONS "--tool=memcheck --error-exitcode=1 --leak-check=full --show-leak-kinds=all --show-reachable=yes --trace-children=yes")
        set(memcheck_command "${VALGRIND_BIN} ${VALGRIND_OPTIONS}")
        separate_arguments(memcheck_command)
    endif (VALGRIND_FOUND)

    function(add_memcheck_test name)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp")
            add_executable(${TEST_PREFIX}${name}-test ${name}.cpp ${TEST_SOURCES})
        elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cc")
            add_executable(${TEST_PREFIX}${name}-test ${name}.cc ${TEST_SOURCES})
        elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.c")
            add_executable(${TEST_PREFIX}${name}-test ${name}.c ${TEST_SOURCES})
        else ()
            message(FATAL_ERROR "No viable file found at ${CMAKE_CURRENT_SOURCE_DIR}/${name}.{c,cc,cpp}")
        endif ()
        add_test(NAME ${TEST_PREFIX}${name}-test COMMAND ${TEST_PREFIX}${name}-test ${ARGN} WORKING_DIRECTORY ${CDATA_DEST_DIR})
        if (VALGRIND_FOUND)
            add_test(NAME ${TEST_PREFIX}${name}-test-memcheck COMMAND ${memcheck_command} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_PREFIX}${name}-test ${ARGN} WORKING_DIRECTORY ${CDATA_DEST_DIR})
        endif (VALGRIND_FOUND)
    endfunction(add_memcheck_test)

    function(set_memcheck_test_properties name)
        set_tests_properties(${TEST_PREFIX}${name}-test ${ARGN})
        if (VALGRIND_FOUND)
            set_tests_properties(${TEST_PREFIX}${name}-test-memcheck ${ARGN})
        endif (VALGRIND_FOUND)
    endfunction(set_memcheck_test_properties)

    function(memcheck_target_link_libraries name)
        target_link_libraries(${TEST_PREFIX}${name}-test ${ARGN})
    endfunction(memcheck_target_link_libraries)


    enable_testing()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

    set(TESTS filter_iterator kvstore datfile string_utils stopwatch time_point leap_second_database)
    # Tests that aren't ready:
    #   share_tree - doesn't actually assert anything
    foreach(t ${TESTS})
        add_memcheck_test(${t})
        memcheck_target_link_libraries(${t} ${CHECK_LIBRARIES} oc)
    endforeach(t)

    if (LIBOC_DEV)
        # These are just files I'm using for testing and are not actually tests
        # I should add a cmake directive to build these
        set(DEV_TESTS boost_datetime boost_units class_pointer_testing logger share_tree file_reader product_testing)
        foreach(t ${DEV_TESTS})
            add_executable(${TEST_PREFIX}${t}-devtest ${t}.cpp)
            target_link_libraries(${t}-devtest ${CHECK_LIBRARIES} oc)
        endforeach(t)

        find_package(NetCDF COMPONENTS CXX)
        if (NETCDF_FOUND)
            add_executable(${TEST_PREFIX}netcdf_testing-devtest netcdf_testing.cpp)
            target_link_libraries(netcdf_testing-devtest ${CHECK_LIBRARIES} oc ${NETCDF_LIBRARIES})
        endif (NETCDF_FOUND)

    endif (LIBOC_DEV)

else (CHECK_FOUND AND NOT CHECK_MIN_VERSION VERSION_GREATER CHECK_VERSION)
    message(STATUS "tests/ directory requires Check>=${CHECK_MIN_VERSION}, no tests will be built")
endif (CHECK_FOUND AND NOT CHECK_MIN_VERSION VERSION_GREATER CHECK_VERSION)

# add_test(<name> <command> [<arg>...])
# add_test(NAME <name> COMMAND <command> [<arg>...] [CONFIGURATIONS <config>...] [WORKING_DIRECTORY <dir>])
# target_link_libraries(<target> [item1 [item2 [...]]] [[debug|optimized|general] <item>] ...)
