cmake_minimum_required(VERSION 3.16.3)

project(acet)

add_executable(${PROJECT_NAME} src/main.cpp src/acet.cpp)

add_library(lodepng LodePNG/lodepng.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC lodepng)

target_include_directories(${PROJECT_NAME} PUBLIC LodePNG)

# For windows
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(BUILD_FOR_WIN TRUE)
endif()
option(STATIC_BUILD "Build a static binary." ${BUILD_FOR_WIN})

if (STATIC_BUILD)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" CONFIG)
    set(BUILD_SHARED_LIBS OFF)
endif()