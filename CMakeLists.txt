cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(acet LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(acet)
add_subdirectory(acet-gui)
add_subdirectory(acet-cli)
