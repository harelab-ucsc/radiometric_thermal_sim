
cmake_minimum_required(VERSION 3.10)
project(radiometric_depth_sensor)

# Find required Gazebo packages
find_package(gz-cmake3 REQUIRED)
find_package(gz-sim9 REQUIRED)
find_package(gz-common6 REQUIRED)
find_package(gz-msgs11 REQUIRED)
find_package(gz-transport14 REQUIRED)

# Create the plugin library
add_library(RadiometricDepthSensor SHARED
  src/RadiometricDepthSensor.cc
)

# Link Gazebo libraries
target_link_libraries(RadiometricDepthSensor
  gz-sim9::gz-sim9
  gz-common6::gz-common6
  gz-msgs11::gz-msgs11
  gz-transport14::gz-transport14
)

# Include directories
target_include_directories(RadiometricDepthSensor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Set C++ standard
set_target_properties(RadiometricDepthSensor PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

# Install the plugin library
install(TARGETS RadiometricDepthSensor
  LIBRARY DESTINATION lib
)

# Find and set plugin path for local testing
set(PLUGIN_PATH "${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Plugin will be built at: ${PLUGIN_PATH}")
