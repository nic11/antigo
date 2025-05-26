cmake_minimum_required(VERSION 3.5)

set(package_name antigo)

project(
  antigo
  VERSION 0.0.1
  DESCRIPTION "Simple yet powerful trace logging and debugging library inspired by Go's context"
  HOMEPAGE_URL "https://github.com/nic11/antigo"
  LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "using real antigo")

file(GLOB_RECURSE antigo_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
add_library(antigo STATIC ${antigo_sources})
target_include_directories(antigo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(antigo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
