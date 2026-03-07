cmake_minimum_required(VERSION 3.22.1)

# Android JNI Build for Sentinel V2X Security Engine
project(SecurityEngine CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# PHASE 2: Native Engine Integration
# ============================================================================

# Add native-engine as subdirectory (includes V2XCryptoEngine and Botan)
set(NATIVE_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../native-engine)
if(EXISTS ${NATIVE_ENGINE_DIR}/CMakeLists.txt)
    add_subdirectory(${NATIVE_ENGINE_DIR} native-engine)
else()
    message(WARNING "native-engine CMakeLists.txt not found at ${NATIVE_ENGINE_DIR}")
endif()

# Find Botan library (required for V2XCryptoEngine)
# Supports BOTAN_ROOT environment variable for custom/cross-compilation builds (Phase 3 Android)
find_library(BOTAN_LIB botan-2 
    HINTS ${BOTAN_ROOT} $ENV{BOTAN_ROOT}
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)

find_path(BOTAN_INCLUDE botan/version.h 
    HINTS ${BOTAN_ROOT} $ENV{BOTAN_ROOT}
    PATHS /usr/include/botan-2 /usr/local/include/botan-2)

if(BOTAN_LIB AND BOTAN_INCLUDE)
    message(STATUS "Botan library found: ${BOTAN_LIB}")
    message(STATUS "Botan headers found: ${BOTAN_INCLUDE}")
else()
    message(WARNING "Botan library not found - crypto operations may be limited")
endif()

# ============================================================================
# Add the JNI Security Engine Bridge Library
# ============================================================================
add_library(security-engine SHARED
    SecurityEngine.cpp
)

# Include directories
target_include_directories(security-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${NATIVE_ENGINE_DIR}/include
    ${BOTAN_INCLUDE}
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# Link native-engine library (V2XCryptoEngine)
if(TARGET sentinel-engine)
    target_link_libraries(security-engine PRIVATE sentinel-engine)
    message(STATUS "Linked sentinel-engine target")
endif()

# Link Botan cryptographic library
if(BOTAN_LIB)
    target_link_libraries(security-engine PRIVATE ${BOTAN_LIB})
endif()

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

# Define Android flag for cross-platform code
target_compile_definitions(security-engine PRIVATE __ANDROID__=1)
