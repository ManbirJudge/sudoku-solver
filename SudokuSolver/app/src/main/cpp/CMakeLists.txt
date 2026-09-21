cmake_minimum_required(VERSION 3.22.1)

set(ABI arm64-v8a)

set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/../../../../third_party/opencv/sdk/native/jni)
set(OpenCV_CONFIG_SUBDIR /abi-${ABI})
find_package(OpenCV 5.0 REQUIRED java)

project("sudokusolver")

# set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/download_cache)
# set(TFLITE_HOST_TOOLS_DIR "/usr/bin" CACHE PATH "Directory containing host-native TensorFlow Lite tools")
# add_subdirectory(
#     ${CMAKE_SOURCE_DIR}/../../../../third_party/tensorflow/tensorflow/lite
#     ${CMAKE_BINARY_DIR}/litert_build
# )

add_library(litert SHARED IMPORTED)
set_target_properties(litert PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../../../third_party/tensorflow-lite-aar/jni/${ABI}/libtensorflowlite_jni.so
)

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    solver.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../../../../third_party/tensorflow-lite-aar/headers
    # ${TENSORFLOW_SOURCE_DIR}
)

target_link_options(${CMAKE_PROJECT_NAME} PUBLIC
    -Wl,--rosegment
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    log
    ${OpenCV_LIBS}
    litert
    # tensorflowlite_c
)