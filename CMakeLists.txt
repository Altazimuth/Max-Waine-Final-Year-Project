cmake_minimum_required(VERSION 3.11)
project(raytracer LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(OpenGL REQUIRED)
find_package(GLEW   REQUIRED)
find_package(SDL2   REQUIRED)

add_executable(raytracer
    source/main.cpp
    source/shader_fragment.h
    source/shader_vertex.h
)

target_link_libraries(raytracer PRIVATE
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
)

target_include_directories(raytracer SYSTEM PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

set_target_properties(raytracer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)