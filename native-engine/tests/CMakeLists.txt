cmake_minimum_required(VERSION 3.22.1)

# ============================================================================
# Find Google Test Framework (system package)
# ============================================================================
find_package(GTest REQUIRED)

if(NOT GTest_FOUND)
    message(FATAL_ERROR "Google Test not found. Install with: sudo apt-get install libgtest-dev")
endif()

# Find Botan library
find_library(BOTAN_LIB botan-2 PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib REQUIRED)
find_path(BOTAN_INCLUDE botan/version.h PATHS /usr/include/botan-2 /usr/local/include/botan-2 REQUIRED)

message(STATUS "✓ Google Test found: ${GTEST_LIBRARIES}")
message(STATUS "✓ Botan library found: ${BOTAN_LIB}")

# ============================================================================
# Build test executable
# ============================================================================
add_executable(crypto_engine_test
    test_v2x_crypto_engine.cpp
    test_coer_decoder_vectors.cpp
    test_message_processor_integration.cpp
    test_v2x_message_frame_integration.cpp
    ../src/v2x_crypto_engine.cpp
    ../src/v2x_coer_decoder.cpp
    ../src/v2x_payload_validator.cpp
    ../src/v2x_message_processor.cpp
    ../src/v2x_message_frame.cpp
)

# Include directories
target_include_directories(crypto_engine_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${BOTAN_INCLUDE}
    ${GTEST_INCLUDE_DIRS}
)

# Link against Botan and GTest directly (not through sentinel-engine target)
target_link_libraries(crypto_engine_test PRIVATE
    ${BOTAN_LIB}
    ${GTEST_LIBRARIES}
)

# Compile options
target_compile_options(crypto_engine_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g  # Debug symbols for better test output
    -fPIC
)

# Add test to CTest
add_test(NAME CryptoEngineTests COMMAND crypto_engine_test)

message(STATUS "✓ Test executable configured: crypto_engine_test")
