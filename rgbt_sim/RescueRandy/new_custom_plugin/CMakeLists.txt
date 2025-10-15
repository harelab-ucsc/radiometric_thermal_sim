
cmake_minimum_required(VERSION 3.10)
project(radiometric_thermal_sensor)

# Find required Gazebo packages
find_package(gz-cmake3 REQUIRED)
find_package(gz-sim9 REQUIRED)
find_package(gz-common6 REQUIRED)
find_package(gz-msgs10 REQUIRED)
find_package(gz-transport13 REQUIRED)

# Create the plugin library
add_library(RadiometricThermalSensor SHARED
  src/RadiometricThermalSensor.cc
)

# Link Gazebo libraries
target_link_libraries(RadiometricThermalSensor
  gz-sim9::gz-sim9
  gz-common6::gz-common6
  gz-msgs10::gz-msgs10
  gz-transport13::gz-transport13
)

# Include directories
target_include_directories(RadiometricThermalSensor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Set C++ standard
set_target_properties(RadiometricThermalSensor PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

# Install the plugin library
install(TARGETS RadiometricThermalSensor
  LIBRARY DESTINATION lib
)

# Find and set plugin path for local testing
set(PLUGIN_PATH "${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Plugin will be built at: ${PLUGIN_PATH}")