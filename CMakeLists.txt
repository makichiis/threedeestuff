cmake_minimum_required(VERSION 3.1...3.29)

set(PROJECT_NAME "threedeestuff")

project(
  ${PROJECT_NAME}
  VERSION 1.0
  LANGUAGES CXX)

string(TIMESTAMP TIME_OF_BUILD "UTC %Y-%m-%d @ %H:%M" UTC)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

message("Source directory: ${CMAKE_SOURCE_DIR}")
message("Target directory: ${CMAKE_CURRENT_BINARY_DIR}")

# temporary, add debug state check later 
# https://cmake.org/cmake/help/book/mastering-cmake/chapter/Testing%20With%20CMake%20and%20CTest.html
# ^-- for testing via cmake 
if(DEBUG)
  add_compile_definitions(DEBUG)
  set(WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
else()
  add_compile_options(-Werror -O3 -Wall)
  set(WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif(DEBUG)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# add_compile_options(-Wall -Wextra -Wpedantic) # SHUT THE FUCK UP

include_directories(${CMAKE_SOURCE_DIR}/include/)
include_directories(${CMAKE_SOURCE_DIR}/lib/glfw/include/)
include_directories(${CMAKE_SOURCE_DIR}/lib/glad/include/)
include_directories(${CMAKE_SOURCE_DIR}/lib/glm/include/)

add_subdirectory(lib/glfw)
add_subdirectory(lib/glad)

add_custom_target(resources ALL DEPENDS ${CMAKE_SOURCE_DIR}/resources/8k_earth_daymap.jpg)

add_custom_command(
  POST_BUILD 
  TARGET resources 
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/resources
)

add_compile_options(-DFE_VERSION -DFE_VERSION_TIME="${TIME_OF_BUILD}"
  -DFE_VERSION_MAJOR=${VERSION_MAJOR} 
  -DFE_VERSION_MINOR=${VERSION_MINOR} 
  -DFE_VERSION_PATCH=${VERSION_PATCH})

# NOTE: CMakeLists MUST be reconfigured when new files are added. This is not a
# bug: https://github.com/microsoft/vscode-cmake-tools/issues/722
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23 CXX_STANDARD 23)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)
