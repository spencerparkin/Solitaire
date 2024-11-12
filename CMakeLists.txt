# CMakeLists.txt for the Spider Solitaire project.

cmake_minimum_required(VERSION 3.27)

project(SpiderSolitaire)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SPIDER_SOURCES
    Source/Main.cpp
    Source/Main.h
)

#source_group("Sources" TREE ${CMAKE_CURRENT_SOURE_DIR} FILES ${SPIDER_SOURCES})

add_executable(SpiderSolitaire WIN32 ${SPIDER_SOURCES})

target_compile_definitions(SpiderSolitaire PRIVATE
    _USE_MATH_DEFINES
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(SpiderSolitaire PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

target_include_directories(SpiderSolitaire PRIVATE
    "Source"
)