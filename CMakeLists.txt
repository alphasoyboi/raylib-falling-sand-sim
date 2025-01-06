cmake_minimum_required(VERSION 3.15)
project(raylib-sand-sim)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20) # Requires C++20 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add raylib as a dependency
find_package(raylib 4.0 REQUIRED) # Requires at least version 4.0
include_directories(${CMAKE_SOURCE_DIR}/include)
# Define raygui implementation
add_definitions(-DRAYGUI_IMPLEMENTATION)

# Add json-devel as a dependency
find_package(nlohmann_json 3.2.0 REQUIRED)

set(${CMAKE_SOURCE_DIR}/resources)
add_definitions(-DRESOURCES_PATH=\"${CMAKE_SOURCE_DIR}/resources/\")

# Define the source files, add the executable, and link raylib
set(SOURCES
        src/main.cc)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} raylib)
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()