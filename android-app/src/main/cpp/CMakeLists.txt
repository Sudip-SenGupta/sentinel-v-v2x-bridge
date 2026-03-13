cmake_minimum_required(VERSION 3.22.1)

project(V2XJNI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep this module as a thin entry point. The real Android JNI target is built
# from native-engine/CMakeLists.txt.
set(NATIVE_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../native-engine)
if(EXISTS ${NATIVE_ENGINE_DIR}/CMakeLists.txt)
    add_subdirectory(${NATIVE_ENGINE_DIR} native-engine)
else()
    message(FATAL_ERROR "native-engine CMakeLists.txt not found at ${NATIVE_ENGINE_DIR}")
endif()

if(TARGET v2x-jni)
    message(STATUS "Using v2x-jni target from native-engine")
else()
    message(FATAL_ERROR "v2x-jni target was not created by native-engine/CMakeLists.txt")
endif()
