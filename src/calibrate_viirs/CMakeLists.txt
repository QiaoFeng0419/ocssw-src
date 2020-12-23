cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

find_package(LibViirsCal REQUIRED)
if(LibViirsCal_INCLUDE_DIRS)
  include_directories(${LibViirsCal_INCLUDE_DIRS})
endif()
set(LIBS ${LibViirsCal_LIBRARIES})

list(APPEND LIBS piutils)

add_executable(calibrate_viirs
    Calibrate_Viirs.cpp
    calibrate_viirs_input.cpp
)

target_link_libraries(calibrate_viirs
  ${LIBS}
)

install (TARGETS calibrate_viirs DESTINATION bin)
