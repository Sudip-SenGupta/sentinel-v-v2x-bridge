#!/bin/bash
#
# setup-wsl-environment.sh
# Automated setup script for sentinel-v-v2x-bridge development on Linux/WSL

set -e

ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
ANDROID_NDK_VERSION="25.0.8775105"
ANDROID_NDK_DIR="${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}"
JAVA_MAJOR_VERSION="17"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SETUP_NATIVE=true
SETUP_ANDROID=true

print_header() {
    echo -e "\n${BLUE}=== $1 ===${NC}\n"
}

print_success() {
    echo -e "${GREEN}[OK] $1${NC}"
}

print_error() {
    echo -e "${RED}[ERR] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[WARN] $1${NC}"
}

print_info() {
    echo -e "${BLUE}[INFO] $1${NC}"
}

show_help() {
    cat <<EOF
Usage:
  bash scripts/setup-wsl-environment.sh [--android-only] [--native-only] [--help]

Options:
  --android-only    Skip native development tools
  --native-only     Skip Android SDK/NDK/Java setup
  --help            Show this help message
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --android-only)
                SETUP_NATIVE=false
                shift
                ;;
            --native-only)
                SETUP_ANDROID=false
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    else
        echo "unknown"
    fi
}

check_command() {
    command -v "$1" >/dev/null 2>&1
}

install_native_tools() {
    print_header "Setting up Native Development Tools"

    local distro
    distro=$(detect_distro)

    case "$distro" in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y build-essential cmake python3 make pkg-config openjdk-17-jdk
            ;;
        fedora|rhel|centos)
            sudo dnf install -y gcc gcc-c++ make cmake python3 pkgconfig java-17-openjdk-devel
            ;;
        arch)
            sudo pacman -S --noconfirm base-devel cmake python pkgconf jdk17-openjdk
            ;;
        *)
            print_warning "Unknown distribution: $distro"
            print_warning "Please install CMake 3.22+, GCC/Clang, Python 3.8+, Make, and JDK 17 manually"
            ;;
    esac

    for cmd in cmake gcc python3 make java; do
        if check_command "$cmd"; then
            print_success "$cmd found"
        else
            print_error "$cmd not found"
        fi
    done
}

install_android_tools() {
    print_header "Setting up Android SDK and NDK"

    mkdir -p "$ANDROID_SDK_ROOT"

    if [ ! -x "$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager" ]; then
        print_info "Installing Android command-line tools"
        local temp_dir
        temp_dir=$(mktemp -d)
        cd "$temp_dir"
        if check_command wget; then
            wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O cmdline-tools.zip
        else
            curl -sSL https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -o cmdline-tools.zip
        fi
        unzip -q cmdline-tools.zip
        mkdir -p "$ANDROID_SDK_ROOT/cmdline-tools/latest"
        cp -r cmdline-tools/* "$ANDROID_SDK_ROOT/cmdline-tools/latest/"
        cd - >/dev/null
        rm -rf "$temp_dir"
    fi

    export ANDROID_SDK_ROOT
    export PATH="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH"

    yes | sdkmanager --licenses >/dev/null || true
    sdkmanager "platform-tools" "build-tools;34.0.0" "platforms;android-34" "platforms;android-24" "ndk;$ANDROID_NDK_VERSION"

    if [ -d "$ANDROID_NDK_DIR" ]; then
        print_success "NDK installed at $ANDROID_NDK_DIR"
    else
        print_error "NDK not found at $ANDROID_NDK_DIR"
        exit 1
    fi
}

setup_environment() {
    print_header "Configuring Environment Variables"

    local shell_rc="$HOME/.bashrc"
    [ -f "$HOME/.zshrc" ] && shell_rc="$HOME/.zshrc"

    if ! grep -q "sentinel-v-v2x-bridge" "$shell_rc" 2>/dev/null; then
        cat >> "$shell_rc" <<EOF

# Android SDK/NDK configuration for sentinel-v-v2x-bridge
export ANDROID_SDK_ROOT="$ANDROID_SDK_ROOT"
export ANDROID_NDK_ROOT="$ANDROID_NDK_DIR"
export PATH="\$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:\$ANDROID_SDK_ROOT/platform-tools:\$PATH"
EOF
        print_success "Environment variables added to $shell_rc"
    else
        print_info "Environment variables already present in $shell_rc"
    fi
}

setup_local_properties() {
    print_header "Creating local.properties"

    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local project_root
    project_root="$(dirname "$script_dir")"

    cat > "$project_root/local.properties" <<EOF
# Auto-generated by setup-wsl-environment.sh
sdk.dir=$ANDROID_SDK_ROOT
ndk.dir=$ANDROID_NDK_DIR
EOF

    print_success "local.properties written to $project_root/local.properties"
}

verify_native_build() {
    print_header "Verifying Native Build"

    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local project_root
    project_root="$(dirname "$script_dir")"

    mkdir -p "$project_root/native-engine/build"
    cd "$project_root/native-engine/build"
    cmake ..
    make

    if [ -f "$project_root/native-engine/build/tests/crypto_engine_test" ]; then
        print_success "Native test target built successfully"
    else
        print_error "Native test target not found"
        exit 1
    fi
}

verify_gradle() {
    print_header "Verifying Gradle Configuration"

    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local project_root
    project_root="$(dirname "$script_dir")"

    cd "$project_root"
    ./gradlew -v
    print_success "Gradle wrapper is available"
}

main() {
    print_header "Sentinel-V V2X Bridge - WSL Environment Setup"
    parse_args "$@"

    if [ "$SETUP_NATIVE" = true ]; then
        install_native_tools
    fi

    if [ "$SETUP_ANDROID" = true ]; then
        install_android_tools
        setup_environment
        setup_local_properties
        verify_gradle
    fi

    if [ "$SETUP_NATIVE" = true ]; then
        verify_native_build
    fi

    print_header "Setup Complete"
    print_success "Environment is ready"
    print_info "Next steps:"
    echo "  1. source ~/.bashrc"
    echo "  2. ./gradlew :app:build"
    echo "  3. ./gradlew :android-app:connectedDebugAndroidTest -Pandroid.testInstrumentationRunnerArguments.class=com.sentinel.v2x.V2XJNITest"
}

main "$@"
