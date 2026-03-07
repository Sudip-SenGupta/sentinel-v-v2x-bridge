cmake_minimum_required(VERSION 3.22.1)

# Android JNI Build for Sentinel V2X Security Engine
project(SecurityEngine CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the JNI Security Engine Bridge Library
add_library(security-engine SHARED
    SecurityEngine.cpp
)

# Include directories
target_include_directories(security-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# Link Android logging library
find_library(log-lib log)
target_link_libraries(security-engine PRIVATE ${log-lib})

# Link Android native window library
find_library(android-lib android)
target_link_libraries(security-engine PRIVATE ${android-lib})

# Set compiler flags
target_compile_options(security-engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
)

# Optimization flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(security-engine PRIVATE
        -O3
        -flto
    )
endif()

# TODO: Link external crypto libraries when native-engine is integrated
# target_link_libraries(security-engine PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../../../native-engine/build/libsentinel_engine.so
# )
