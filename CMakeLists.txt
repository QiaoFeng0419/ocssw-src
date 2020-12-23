cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (OCSSW)

# delete the anaconda dirs from the PATH.  That way find_library
# will use system libs instead of anaconda libs.  (ie. zlib)
set(GOOD_PATH "")
string(REPLACE ":" ";" PATH_LIST $ENV{PATH})
foreach(TMP_DIR ${PATH_LIST})
  if(NOT TMP_DIR MATCHES "conda")
    if(GOOD_PATH)
      set(GOOD_PATH "${GOOD_PATH}:${TMP_DIR}")
    else()
      set(GOOD_PATH "${TMP_DIR}")
    endif()
  endif()
endforeach(TMP_DIR)
set(ENV{PATH} ${GOOD_PATH})
unset(TMP_DIR)
unset(GOOD_PATH)

macro(SET_OCSSW_POLICY)
  # set the <package>_ROOT policy
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()
endmacro(SET_OCSSW_POLICY)

# set this to "ON" for SeaDAS export
# need to copy a few VIIRS directories
# $ cp -R viirs_l1/src/l1agen_viirs ocssw/src
# $ cp -R viirs_l1/src/geolocate_viirs ocssw/src
# $ cp -R viirs_l1/src/calibrate_viirs ocssw/src
# $ cp -R viirs_l1/viirs_lib/cmake/Modules/* ocssw/cmake/Modules/viirs
set(SEADAS_EXPORT ON)

set(BUILD_SHARED_LIBS ON)

# where to look for the external libraries
set (CMAKE_PREFIX_PATH $ENV{LIB3_DIR})

# set this to Release or Debug
# note: rerun cmake if you change the environment var OCSSW_DEBUG
if ($ENV{OCSSW_DEBUG})
  set (CMAKE_BUILD_TYPE Debug)
else($ENV{OCSSW_DEBUG})
  set (CMAKE_BUILD_TYPE Release)
endif($ENV{OCSSW_DEBUG})

enable_language (C)
enable_language (CXX)
enable_language (Fortran)

# Finer-grained control...
OPTION(BUILD_HISTORICAL "Build the historical code (e.g. old seawifs binaries no longer needed)" OFF)
OPTION(BUILD_MISC "Build non-essential code" OFF)
OPTION(BUILD_VIIRS_L1 "Build VIIRS L1 code" ON)
OPTION(BUILD_AHMAD_FRASER "Build the AF radiative transfer code" OFF)
OPTION(BUILD_ALL "Build all including  Aquarius, historical, misc, VIIRS L1 and Ahmad-Fraser code" OFF)

if(BUILD_ALL)
   set(BUILD_HISTORICAL ON)
   set(BUILD_MISC ON)
   set(BUILD_VIIRS_L1 ON)
   set(BUILD_AHMAD_FRASER ON)
endif(BUILD_ALL)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/oel_util/cmake/Modules
    ${CMAKE_CURRENT_SOURCE_DIR}/oel_hdf4/cmake/Modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

#include(PrintAllVariables)
#print_all_variables()

if (SEADAS_EXPORT)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/viirs)
else ()
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/viirs_lib/cmake/Modules)
endif ()
  
# if Doxygen exists, create the API documentation 
find_package(Doxygen)
if(DOXYGEN_FOUND AND IS_DIRECTORY ${OCSSW_SOURCE_DIR}/doc)
  configure_file(${OCSSW_SOURCE_DIR}/doc/Doxyfile
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
  )
  add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${OCSSW_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
endif()

# make the build
set (CMAKE_EXE_LINKER_FLAGS "-mcmodel=large")
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_Fortran_FLAGS "")

# Set up C and CXX flags
set (CMAKE_C_FLAGS_RELEASE "-O3 -Wno-unused-result")
if (DEFINED ENV{GCC_TUNE})
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  $ENV{GCC_TUNE}")
endif(DEFINED ENV{GCC_TUNE}) 
set (CMAKE_C_FLAGS_DEBUG "-g3 -gdwarf-2 -Wall")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set (CMAKE_C_FLAGS "-std=c11 -DLINUX -Dlinux -Wno-parentheses")
set (CMAKE_CXX_FLAGS "-std=c++14 -DLINUX -Dlinux -Wno-unused-local-typedefs -Wno-write-strings -Wno-parentheses")

set (CMAKE_EXPORT_COMPILE_COMMANDS on)

if(CMAKE_C_COMPILER_VERSION VERSION_GREATER "6.0")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-overflow")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-overflow")
endif()


# Set up FORTRAN flags
set (CMAKE_Fortran_FLAGS "-DLINUX -Dlinux -ffixed-line-length-none -fconvert=big-endian -fno-automatic -fno-range-check -cpp")
set (CMAKE_Fortran_FLAGS_RELEASE "-O3 -funroll-all-loops")
set (CMAKE_Fortran_FLAGS_DEBUG "-g3 -gdwarf-2 -Wall -Wno-unused-variable -Wno-unused-label -Wno-unused-dummy-argument -Wno-conversion")

# set up OS-specific bits
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
  # standard Linux bits...
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE")
  set (CMAKE_INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../opt/lib")
  set (EOS_ARCH linux)
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  # 64bit bits...
    set (EOS_ARCH linux64)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    #set (CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib/x86_64-linux-gnu")
    #set (CMAKE_SYSTEM_INCLUDE_PATH "/usr/include/x86_64-linux-gnu")
  endif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
# Mac bits
  set (CMAKE_MACOSX_RPATH 1)
  set (CMAKE_INSTALL_RPATH "@executable_path/../lib;@executable_path/../opt/lib")
  set (EOS_ARCH macintel64)
  set (CMAKE_C_FLAGS "-DMACINTOSH ${CMAKE_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "-DMACINTOSH ${CMAKE_CXX_FLAGS}")
  set (CMAKE_Fortran_FLAGS "-DMACINTOSH ${CMAKE_Fortran_FLAGS}")
  set (CMAKE_EXE_LINKER_FLAGS "-Wl,-no_compact_unwind")
else(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
# Miscellaneous - not sure what we are...
  set (CMAKE_INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../opt/lib")
  message ("CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})
  message ("System not recognized, using other")
  set (EOS_ARCH linux)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# where, oh where are your FORTRAN mod files...
set (CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran_modules)

# where we put the binaries
set (CMAKE_INSTALL_PREFIX $ENV{OCSSWROOT})

if (NOT SEADAS_EXPORT)
  include(SvnRevisionFile)

  # add commands to get data directory from svn
  file(STRINGS ".svn_share_branch" OCSSW_SHARE_BRANCH)
  svn_revision_file(
    NAME ocssw_share
    URL svn://$ENV{OCSSW_SVN}/${OCSSW_SHARE_BRANCH}
    DIR share
    FILE .svn_share_revision
    CHECKOUT_CMD share_checkout
    COMMIT_CMD share_commit
    OUTPUT_VARIABLE share_revision
    READ PRINT
    )

  # add commands to get var directory from svn
  file(STRINGS ".svn_var_branch" OCSSW_VAR_BRANCH)
  svn_revision_file(
    NAME ocssw_var
    URL svn://$ENV{OCSSW_SVN}/${OCSSW_VAR_BRANCH}
    DIR var
    FILE .svn_var_revision
    CHECKOUT_CMD var_checkout
    COMMIT_CMD var_commit
    OUTPUT_VARIABLE var_revision
    READ PRINT
    )

  # add commands to get lib3 source files from svn
  file(STRINGS ".svn_lib3_branch" OCSSW_LIB3_BRANCH)
  svn_revision_file(
    NAME ocssw_lib3
    URL svn://$ENV{OCSSW_SVN}/${OCSSW_LIB3_BRANCH}
    DIR opt/src
    FILE .svn_lib3_revision
    CHECKOUT_CMD lib3_checkout
    COMMIT_CMD lib3_commit
    OUTPUT_VARIABLE lib3_revision
    READ PRINT
    )

  add_custom_target(update 
             make lib3_checkout
    COMMAND  make share_checkout
    COMMAND  make var_checkout
    )
  add_custom_target(commit
             make lib3_commit
    COMMAND  make share_commit
    COMMAND  make var_commit
    )
endif()

# set a default for lib3 search paths
set(HDF5_ROOT $ENV{LIB3_DIR})
set(HDF5_FIND_QUIETLY TRUE)

set(NETCDF_DIR $ENV{LIB3_DIR})


# add the submodule repos
add_subdirectory(oel_util)
add_subdirectory(oel_hdf4)

# VIIRS-specific
if (NOT SEADAS_EXPORT)
  add_subdirectory(viirs_lib)
endif ()

add_subdirectory(src)
add_subdirectory(test)

enable_testing()

