add_subdirectory(lodepng)
add_executable(acet-cli main.cpp)
target_link_libraries(acet-cli PRIVATE lodepng)
target_link_libraries(acet-cli PRIVATE acet)