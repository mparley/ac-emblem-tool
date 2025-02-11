add_subdirectory(LodePNG)
add_executable(acet-cli main.cpp)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" CONFIG)
set(BUILD_SHARED_LIBS OFF)

target_link_libraries(acet-cli PUBLIC lodepng)
target_link_libraries(acet-cli PUBLIC acet)
