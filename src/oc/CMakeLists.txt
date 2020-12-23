cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

option(STANDALONE "Build oc as a stand-alone library")

set(LIB3_DIR $ENV{LIB3_DIR})

if (STANDALONE)
    project(oc)

    set(CMAKE_EXPORT_COMPILE_COMMANDS on)

    include_directories(BEFORE SYSTEM ${LIB3_DIR}/include)
    link_directories(${LIB3_DIR}/lib)

    list(APPEND CMAKE_MODULE_PATH
        ${LIB3_DIR}/cmake
        $ENV{OCSSWROOT}/oel_util/cmake/Modules
        $ENV{OCSSWROOT}/oel_hdf4/cmake/Modules
        $ENV{OCSSWROOT}/cmake/Modules)
    set(CMAKE_PREFIX_PATH ${LIB3_DIR}) # semi-colon separated list
else (STANDALONE)
    set_ocssw_policy()
endif (STANDALONE)

# "shouldn't" technically modify C/CXX_FLAGS
set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wpedantic")

# set(Boost_DEBUG 1)
find_package(Boost REQUIRED COMPONENTS filesystem date_time)

find_package(HDF5 COMPONENTS C HL)
find_package(NetCDF COMPONENTS CXX)
find_package(HDF4)
find_package(Intl)

set(OC_MODULE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/oc")

configure_file(include/oc/Configuration.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/oc/Configuration.hpp)

add_library(oc SHARED
    src/KvStore.cpp
    src/DatFile.cpp
    src/StringUtils.cpp
    src/ShareTree.cpp
    src/TimePoint.cpp
    src/LeapSecondDatabase.cpp

    src/FileReader.cpp
    src/Algorithm.cpp

    src/ModuleManager.cpp

    src/Log.cpp
    src/log/Stream.cpp
    src/log/File.cpp
    src/log/FormattedStream.cpp
    src/log/FormattedFile.cpp
)

add_library(oc-core SHARED
    src/CoreModule.cpp

    src/command/Help.cpp
    src/command/ListCommands.cpp
    src/command/ListModules.cpp
)

add_executable(oc-cmd src/MasterCommand.cpp)

target_link_libraries(oc-core oc)

target_include_directories(oc-cmd PUBLIC "include")
target_include_directories(oc-cmd SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(oc-cmd oc)


# As far as developers are aware, there's no difference between the framework library and the core module
foreach(library oc oc-core)
    target_link_libraries(${library} ${Boost_LIBRARIES})
    target_include_directories(${library} PUBLIC "include")
    
    target_include_directories(${library} SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} "${LIB3_DIR}/include")

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${library} dl)
    endif()

    if (HDF4_FOUND)
        target_link_libraries(${library} ${HDF4_LIBRARIES})
        target_include_directories(${library} SYSTEM PUBLIC ${HDF4_INCLUDE_DIRS})
    endif(HDF4_FOUND)

    if (HDF5_FOUND)
        target_link_libraries(${library} ${HDF5_CXX_LIBRARIES})
        target_include_directories(${library} SYSTEM PUBLIC ${HDF5_INCLUDE_DIRS})
    endif(HDF5_FOUND)

    if (NETCDF_FOUND)
        target_link_libraries(${library} ${NETCDF_LIBRARIES})
        target_include_directories(${library} SYSTEM PUBLIC ${NETCDF_INCLUDE_DIRS})
    endif(NETCDF_FOUND)

    if (Intl_FOUND)
        target_link_libraries(${library} ${Intl_LIBRARIES})
        target_include_directories(${library} SYSTEM PUBLIC ${Intl_INCLUDE_DIRS})
    endif(Intl_FOUND)
endforeach(library)

set_target_properties(oc
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    # RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
set_target_properties(oc-core
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/oc"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/oc"
    # RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    OUTPUT_NAME core
)
set_target_properties(oc-cmd
    PROPERTIES
    # ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/oc"
    # LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/oc"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    OUTPUT_NAME oc
)

# Using this to install header files destroys directory structure
# file(GLOB_RECURSE OC_HEADER_FILES "include/*.hpp" "include/*.h")
# set_target_properties(oc PROPERTIES PUBLIC_HEADER "${OC_HEADER_FILES}")

install(TARGETS oc oc-cmd
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(TARGETS oc-core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib/oc
    ARCHIVE DESTINATION lib/oc)


# install(TARGETS oc      DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
# install(TARGETS oc-core DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/oc" RENAME core)
# install(PROGRAMS oc-cmd DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" RENAME oc)

# install(TARGETS oc PUBLIC_HEADER DESTINATION "include") # unused (set_target_properties is commented out)
# install(TARGETS oc-core PUBLIC_HEADER DESTINATION "include" RENAME core) # unused (set_target_properties is commented out)
# install(PROGRAMS oc-cmd DESTINATION "bin" RENAME oc)

install(DIRECTORY "include/oc"
    DESTINATION "include" FILES_MATCHING PATTERN "*.hpp")
install(FILES "include/oc.hpp"
    DESTINATION "include")

add_subdirectory("tests")

enable_testing()

