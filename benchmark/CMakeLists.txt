cmake_minimum_required(VERSION 3.22.1)

project(bench)

add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
)

add_subdirectory(
    ${CMAKE_SOURCE_DIR}/../solver
    ${CMAKE_BINARY_DIR}/solver_build
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE solver)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)