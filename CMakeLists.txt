cmake_minimum_required(VERSION 3.11)
project(mwcoursework LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(OpenGL REQUIRED)
find_package(GLEW   REQUIRED)
find_package(SDL2   REQUIRED)
find_package(DevIL  REQUIRED)

add_executable(mwcoursework
    source/main.cpp
    source/shader_fragment.h
    source/shader_vertex.h
)

target_link_libraries(mwcoursework PRIVATE
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${IL_LIBRARIES}
)

target_include_directories(mwcoursework SYSTEM PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${IL_INCLUDE_DIR}
)

set_target_properties(mwcoursework PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)