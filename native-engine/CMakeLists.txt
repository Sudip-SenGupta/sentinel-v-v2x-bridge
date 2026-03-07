cmake_minimum_required(VERSION 3.22.1)

# Project name and metadata
project("SentinelNativeEngine")

# Set C++ standard (showcasing your Modern C++ skills)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Define your Security Engine Library
add_library(sentinel_engine SHARED
        src/SecurityEngine.cpp
        src/V2XProtocolStack.cpp
)

# 2. Include Directories
# This points to your headers and simulated PKCS#11/Botan wrappers
target_include_directories(sentinel_engine PRIVATE 
        include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/botan/include
)

# 3. Link Android Logging (Essential for JNI debugging)
find_library(log-lib log)
target_link_libraries(sentinel_engine ${log-lib})

# 4. (Advanced) Link your prebuilt Botan/Security libs
# As a Senior Lead, you know how to handle prebuilt vendor blobs:
# target_link_libraries(sentinel_engine ${CMAKE_CURRENT_SOURCE_DIR}/libs/libbotan.so)