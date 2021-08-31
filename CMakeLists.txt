cmake_minimum_required(VERSION 3.16.3)

project(acet)

option(STATIC_BUILD "Build a static binary." ON)

if (STATIC_BUILD)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" CONFIG)
    set(BUILD_SHARED_LIBS OFF)
endif()

add_executable(${PROJECT_NAME} src/main.cpp src/acet.cpp)

add_library(lodepng LodePNG/lodepng.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC lodepng)

target_include_directories(${PROJECT_NAME} PUBLIC LodePNG)