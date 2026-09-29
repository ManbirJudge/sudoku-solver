cmake_minimum_required(VERSION 3.22.1)

# ---
set(ABI arm64-v8a)

# --- opencv
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/../../../../third_party/opencv/sdk/native/jni)
set(OpenCV_CONFIG_SUBDIR /abi-${ABI})
find_package(OpenCV 5.0 REQUIRED java)

# ---
project("sudokusolver")

# --- litert
add_library(litert SHARED IMPORTED)
set_target_properties(litert PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../../../third_party/tensorflow-lite-aar/jni/${ABI}/libtensorflowlite_jni.so
)

# --- source files
add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
)

# --- solver
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/../../../../../solver
    ${CMAKE_BINARY_DIR}/solver_build
)

# ---
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../../../../third_party/tensorflow-lite-aar/headers
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    solver
    ${OpenCV_LIBS}
    litert
    android
    log
)

# ---
target_link_options(${CMAKE_PROJECT_NAME} PUBLIC
    -Wl,--rosegment
)