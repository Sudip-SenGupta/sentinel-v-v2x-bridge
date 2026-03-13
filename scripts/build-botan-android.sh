#!/bin/bash
# Build Botan for Android NDK
# Phase 6A: Cross-compile Botan for arm64-v8a and x86_64

set -e

# Configuration
NDK_ROOT="/home/sudip_dev/Android/Sdk/ndk/25.0.8775105"
BOTAN_SRC="$HOME/sentinel-v-v2x-bridge/botan"
WORK_ROOT="$HOME/sentinel-v-v2x-bridge/botan-android-work"
INSTALL_ROOT="$HOME/sentinel-v-v2x-bridge/botan-android"

# Create directories
mkdir -p "$WORK_ROOT" "$INSTALL_ROOT"

echo "=== Botan Android Cross-Compilation ===" 
echo "NDK: $NDK_ROOT"
echo "Source: $BOTAN_SRC"
echo "Work: $WORK_ROOT"
echo "Install: $INSTALL_ROOT"

# Function to build for a specific ABI
build_for_abi() {
    local ABI=$1
    local CPU=$2
    local API_LEVEL=21
    
    echo ""
    echo "=== Building for $ABI (cpu: $CPU) ==="
    
    local WORK_DIR="$WORK_ROOT/$ABI"
    local INSTALL_DIR="$INSTALL_ROOT/$ABI"
    
    # Copy Botan source to work directory for this ABI
    rm -rf "$WORK_DIR"
    cp -r "$BOTAN_SRC" "$WORK_DIR"
    
    cd "$WORK_DIR"
    
    # Set up NDK environment
    export CC="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/clang"
    export CXX="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++"
    export AR="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar"
    export RANLIB="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib"
    
    case $ABI in
        arm64-v8a)
            export CFLAGS="-target aarch64-linux-android$API_LEVEL -fPIC"
            export CXXFLAGS="-target aarch64-linux-android$API_LEVEL -fPIC"
            export LDFLAGS="-target aarch64-linux-android$API_LEVEL"
            ;;
        x86_64)
            export CFLAGS="-target x86_64-linux-android$API_LEVEL -fPIC"
            export CXXFLAGS="-target x86_64-linux-android$API_LEVEL -fPIC"
            export LDFLAGS="-target x86_64-linux-android$API_LEVEL"
            ;;
    esac
    
    # Configure Botan for Android (in-tree build)
    python3 configure.py \
        --cc=clang \
        --os=android \
        --cpu=$CPU \
        --prefix="$INSTALL_DIR" \
        --disable-shared-library \
        --enable-static-library \
        --without-sphinx \
        --without-documentation \
        --disable-cc-tests
    
    # Build - allow CLI to fail, we only need the library
    make -j4 || true
    
    # Find and copy the static library
    mkdir -p "$INSTALL_DIR/lib" "$INSTALL_DIR/include"
    find . -name "libbotan-2.a" -exec cp {} "$INSTALL_DIR/lib/" \;
    cp -rL build/include/botan "$INSTALL_DIR/include/" 2>/dev/null || true
    
    ls -lah "$INSTALL_DIR/lib/"
    echo "✓ Built for $ABI"
}

# Build for both ABIs
build_for_abi "arm64-v8a" "arm64"
build_for_abi "x86_64" "x86_64"

echo ""
echo "=== Build Complete ==="
ls -R "$INSTALL_ROOT"
