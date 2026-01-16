cmake_minimum_required(VERSION 3.22.1)

set(ABI arm64-v8a)

set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/opencv/sdk/native/jni)
set(OpenCV_CONFIG_SUBDIR /abi-${ABI})  # TODO: automate

find_package(OpenCV 4.12 REQUIRED java)

project("sudokusolver")

add_library(litert SHARED IMPORTED)
set_target_properties(litert PROPERTIES
    IMPORTED_LOCATION  ${CMAKE_SOURCE_DIR}/../jniLibs/litert/${ABI}/libtensorflowlite_jni.so
)

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    solver.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_options(sudokusolver PUBLIC
    -Wl,--rosegment  # TODO: what is this?
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    log
    ${OpenCV_LIBS}
    litert
)