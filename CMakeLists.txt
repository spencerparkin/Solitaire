# CMakeLists.txt for the Solitaire project.

cmake_minimum_required(VERSION 3.27)

project(Solitaire)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(DirectXTK12)

set(SOLITAIRE_SOURCES
    Source/Main.cpp
    Source/Main.h
    Source/Application.cpp
    Source/Application.h
    Source/SolitaireGame.cpp
    Source/SolitaireGame.h
    Source/SpiderSolitaireGame.cpp
    Source/SpiderSolitaireGame.h
)

add_executable(Solitaire WIN32 ${SOLITAIRE_SOURCES})

target_compile_definitions(Solitaire PRIVATE
    _USE_MATH_DEFINES
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(Solitaire PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    DirectXTK12
)

target_include_directories(Solitaire PRIVATE
    "Source"
    "DirectXHeader"
)