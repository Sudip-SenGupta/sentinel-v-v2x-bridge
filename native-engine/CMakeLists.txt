cmake_minimum_required(VERSION 3.22)

project(sentinel-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android JNI with Phase 6A: Botan integration for on-device crypto
if(ANDROID)
    add_library(v2x-jni SHARED
        src/v2x_jni.cpp
        src/v2x_coer_decoder.cpp
        src/v2x_message_frame.cpp
        src/v2x_payload_validator.cpp
        src/v2x_message_processor.cpp
        src/v2x_jni_message_processor.cpp
        src/v2x_crypto_engine.cpp
        src/v2x_jni_crypto.cpp
    )

    target_include_directories(v2x-jni
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # Phase 6A: Link pre-built Botan for Android
    set(BOTAN_ANDROID_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../botan-android/${ANDROID_ABI}")

    if(EXISTS "${BOTAN_ANDROID_ROOT}/lib/libbotan-2.a")
        message(STATUS "Found pre-built Botan at: ${BOTAN_ANDROID_ROOT}")
        target_include_directories(v2x-jni PRIVATE "${BOTAN_ANDROID_ROOT}/include")
        target_link_libraries(v2x-jni PRIVATE "${BOTAN_ANDROID_ROOT}/lib/libbotan-2.a")
    else()
        message(WARNING "Botan not found at ${BOTAN_ANDROID_ROOT}")
        message(WARNING "Build with: scripts/build-botan-android.sh")
    endif()

    target_link_libraries(v2x-jni
        PRIVATE
            android
            log
    )

    target_compile_options(v2x-jni PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fPIC
        -fvisibility=hidden
    )

    target_compile_definitions(v2x-jni PRIVATE __ANDROID__=1)

    message(STATUS "=== Sentinel V2X Android JNI Build (Phase 6A) ===")
    message(STATUS "ABI: ${ANDROID_ABI}")
    message(STATUS "Target: v2x-jni (COER decoder + message processor + crypto engine)")
    message(STATUS "Botan: ${BOTAN_ANDROID_ROOT}")
    return()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fPIC")

# ============================================================================
# Botan Library Detection
# ============================================================================

find_package(Botan QUIET)

if(NOT Botan_FOUND)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(BOTAN botan-2)
    endif()

    if(NOT BOTAN_FOUND)
        set(BOTAN_INCLUDE_DIRS "/usr/include/botan-2" "/usr/local/include/botan-2")
        set(BOTAN_LIBRARIES "botan-2")

        find_library(BOTAN_LIB_PATH
            NAMES botan-2 botan
            HINTS ${BOTAN_ROOT} $ENV{BOTAN_ROOT}
            PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
        )

        if(BOTAN_LIB_PATH)
            message(STATUS "Botan library found: ${BOTAN_LIB_PATH}")
        else()
            message(FATAL_ERROR "Botan library not found. Install with: sudo apt-get install libbotan-2-dev")
        endif()
    endif()
endif()

find_file(BOTAN_HEADER "botan/version.h"
    HINTS ${BOTAN_ROOT} $ENV{BOTAN_ROOT}
    PATHS ${BOTAN_INCLUDE_DIRS}
)
if(NOT BOTAN_HEADER)
    message(FATAL_ERROR "Botan headers not found in ${BOTAN_INCLUDE_DIRS}")
endif()

# ============================================================================
# Source Files
# ============================================================================

set(SOURCES
    src/v2x_crypto_engine.cpp
    src/v2x_coer_decoder.cpp
    src/v2x_payload_validator.cpp
    src/v2x_message_processor.cpp
    src/v2x_message_frame.cpp
)

set(HEADERS
    include/v2x_crypto_engine.h
    include/v2x_coer_decoder.h
    include/v2x_payload_validator.h
    include/v2x_message_processor.h
    include/v2x_structures.hpp
)

# ============================================================================
# Sentinel Engine Library
# ============================================================================

add_library(sentinel-engine SHARED ${SOURCES})

target_include_directories(sentinel-engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BOTAN_INCLUDE_DIRS)
    target_include_directories(sentinel-engine
        PRIVATE
            ${BOTAN_INCLUDE_DIRS}
    )
endif()

if(BOTAN_LIB_PATH)
    target_link_libraries(sentinel-engine
        PUBLIC
            ${BOTAN_LIB_PATH}
    )
elseif(BOTAN_LIBRARIES)
    target_link_libraries(sentinel-engine
        PUBLIC
            ${BOTAN_LIBRARIES}
    )
else()
    message(FATAL_ERROR "Botan library required for non-Android builds")
endif()

target_compile_options(sentinel-engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
    -fvisibility=hidden
)

# ============================================================================
# Optional: Unit Tests
# ============================================================================

enable_testing()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS sentinel-engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS}
    DESTINATION include/sentinel/v2x
)

message(STATUS "")
message(STATUS "=== Sentinel V2X Crypto Engine ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Botan Include: ${BOTAN_INCLUDE_DIRS}")
message(STATUS "")
