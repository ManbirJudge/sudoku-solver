cmake_minimum_required(VERSION 3.16)

project(solver LANGUAGES CXX)

add_library(solver STATIC
    src/solver.cpp
)

target_include_directories(solver
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(solver PUBLIC cxx_std_17)