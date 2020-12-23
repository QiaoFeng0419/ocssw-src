cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

find_package(LibViirsGeo REQUIRED)
if(LibViirsGeo_INCLUDE_DIRS)
  include_directories(${LibViirsGeo_INCLUDE_DIRS})
endif()
set(LIBS ${LibViirsGeo_LIBRARIES})

list(APPEND LIBS piutils)

add_executable(geolocate_viirs
    Geolocate_Viirs_Driver.cpp
    geolocate_viirs_input.cpp
)

target_link_libraries(geolocate_viirs
  ${LIBS}
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,-stack_size -Wl,0x10000000")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 

install (TARGETS geolocate_viirs DESTINATION bin)
