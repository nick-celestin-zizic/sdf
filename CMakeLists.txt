cmake_minimum_required(VERSION 3.0)

project(sdf)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/src")

add_subdirectory("${CMAKE_SOURCE_DIR}/dep")

add_executable(game ${PROJECT_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/include/IMGUI)
target_link_libraries(game IMGUI imm32 gdi32 SDL2main SDL2 ${PROJECT_SOURCE_DIR}/dep/glew32.lib opengl32)



