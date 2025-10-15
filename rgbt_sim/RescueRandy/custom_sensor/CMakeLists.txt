cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)
project(RadiometricThermalSensor)

find_package(gz-plugin3 REQUIRED COMPONENTS register)
find_package(gz-sim9 REQUIRED)
find_package(gz-common6 REQUIRED)
find_package(gz-math8 REQUIRED)
find_package(gz-transport14 REQUIRED)
find_package(gz-rendering9 REQUIRED)
find_package(gz-sensors9 REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/RadiometricThermalSensor.cc
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${gz-sim9_INCLUDE_DIRS}
    ${gz-common6_INCLUDE_DIRS}
    ${gz-sensors9_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    gz-plugin3::register
    gz-sim9::gz-sim9
    gz-common6::gz-common6
    gz-math8::gz-math8
    gz-transport14::gz-transport14
    gz-rendering9::gz-rendering9
    gz-sensors9::gz-sensors9
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
