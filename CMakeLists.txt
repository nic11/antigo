cmake_minimum_required(VERSION 3.20)

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

file(GLOB_RECURSE antigo_sources "src/*")
file(GLOB_RECURSE antigo_headers "include/*")
add_library(antigo STATIC ${antigo_sources})
target_include_directories(antigo PRIVATE "src")
target_include_directories(
  antigo
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_sources(
  antigo
  PRIVATE "${antigo_sources}"
  PUBLIC FILE_SET HEADERS
  BASE_DIRS ${PROJECT_SOURCE_DIR}/include
  FILES "${antigo_headers}"
)

include(CMakePackageConfigHelpers)

install(
  TARGETS antigo
  EXPORT antigo-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  FILE_SET HEADERS
)

export(
  EXPORT antigo-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/antigo-targets.cmake"
  NAMESPACE antigo::
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/antigo-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/antigo-config.cmake"
  INSTALL_DESTINATION share/antigo
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/antigo-config.cmake"
  DESTINATION share/antigo
)

install(
  EXPORT antigo-targets
  FILE antigo-targets.cmake
  NAMESPACE antigo::
  DESTINATION share/antigo
)
