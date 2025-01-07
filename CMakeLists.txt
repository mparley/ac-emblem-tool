cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(acet LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(wxBUILD_SHARED OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(acet)
add_subdirectory(acet-gui)
add_subdirectory(acet-cli)