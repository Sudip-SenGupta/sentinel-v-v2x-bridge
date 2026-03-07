cmake_minimum_required(VERSION 3.22)

project(sentinel-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fPIC")

# ============================================================================
# Botan Library Detection
# ============================================================================

# Try to find Botan via CMake module first
find_package(Botan QUIET)

if(NOT Botan_FOUND)
    # Manual configuration for system-installed Botan
    # Botan 2.19.1 installed via: sudo apt-get install libbotan-2-dev
    
    # Try pkg-config if available
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(BOTAN botan-2)
    endif()
    
    # Manual fallback for common Linux system locations
    # Note: Supports BOTAN_ROOT environment variable for custom installations
    #       and cross-compilation (e.g., Android NDK)
    if(NOT BOTAN_FOUND)
        set(BOTAN_INCLUDE_DIRS "/usr/include/botan-2" "/usr/local/include/botan-2")
        set(BOTAN_LIBRARIES "botan-2")
        
        # Find the library
        # HINTS: Allows user to set BOTAN_ROOT environment variable for custom builds
        # PATHS: Standard system locations (Linux development)
        find_library(BOTAN_LIB_PATH 
            NAMES botan-2 botan
            HINTS ${BOTAN_ROOT} $ENV{BOTAN_ROOT}
            PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
        )
        
        if(BOTAN_LIB_PATH)
            message(STATUS "✓ Botan library found: ${BOTAN_LIB_PATH}")
        else()
            message(FATAL_ERROR "✗ Botan library not found. Install with: sudo apt-get install libbotan-2-dev")
        endif()
    endif()
endif()

# Verify Botan headers exist
# Supports custom BOTAN_ROOT for cross-compilation (Phase 3 Android)
find_file(BOTAN_HEADER "botan/version.h" 
    HINTS ${BOTAN_ROOT} $ENV{BOTAN_ROOT}
    PATHS ${BOTAN_INCLUDE_DIRS}
)
if(NOT BOTAN_HEADER)
    message(FATAL_ERROR "✗ Botan headers not found in ${BOTAN_INCLUDE_DIRS}")
else()
    message(STATUS "✓ Botan headers found: ${BOTAN_HEADER}")
endif()

message(STATUS "Botan Include Dirs: ${BOTAN_INCLUDE_DIRS}")
message(STATUS "Botan Libraries: ${BOTAN_LIBRARIES}")

# ============================================================================
# Source Files
# ============================================================================

set(SOURCES
    src/v2x_crypto_engine.cpp
)

set(HEADERS
    include/v2x_crypto_engine.h
)

# ============================================================================
# Sentinel Engine Library
# ============================================================================

add_library(sentinel-engine SHARED ${SOURCES})

target_include_directories(sentinel-engine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${BOTAN_INCLUDE_DIRS}
)

# Link Botan library
if(BOTAN_LIB_PATH)
    target_link_libraries(sentinel-engine 
        PUBLIC
            ${BOTAN_LIB_PATH}
    )
else()
    # Fallback: link by name (system will find it)
    target_link_libraries(sentinel-engine 
        PUBLIC
            ${BOTAN_LIBRARIES}
    )
endif()

# Link Android libraries only when building for Android (NDK detection)
if(ANDROID)
    target_link_libraries(sentinel-engine PRIVATE android log)
endif()

# Set compiler flags for Android/Linux
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

# Test executable (if tests/ directory exists)
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

# ============================================================================
# Build Information
# ============================================================================

message(STATUS "")
message(STATUS "=== Sentinel V2X Crypto Engine ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Botan Include: ${BOTAN_INCLUDE_DIRS}")
message(STATUS "")