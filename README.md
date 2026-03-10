# Sentinel-V V2X Security Bridge

V2X security bridge for Android platforms, with a Kotlin/Android layer, an Android JNI library module, and a native C++ engine.

## Current State

The repository currently has three important parts:

- `app/`: Android application module
- `android-app/`: Android library module that owns the JNI-facing Android integration
- `native-engine/`: C++ engine with crypto, decoding, validation, and native tests

This is not currently a fully symmetric cross-platform build.

- Linux/WSL is the reliable environment for full native engine development and testing.
- Android currently uses a minimal JNI surface for integration validation.
- Full Botan-backed native crypto is implemented in `native-engine/`, but that is not the current stable Android packaging path.

## Prerequisites

### For Linux/WSL Native Development

| Requirement | Minimum Version | Notes |
|---|---|---|
| CMake | 3.22+ | Required by current `native-engine` CMake files |
| GCC/Clang | GCC 9+ or Clang 10+ | C++17 support required |
| Python | 3.8+ | For Botan configure script |
| Make | GNU Make 4.2+ | For native builds |

### For Android Development

| Requirement | Version | Notes |
|---|---|---|
| Android SDK | API 24+ | Target/compile SDK 34 in current Gradle files |
| Android NDK | 25.0.8775105 | Matches the pinned project NDK version |
| Gradle | 8.2 | Bundled via wrapper |
| Java | 17+ | Required for AGP 8.2 |
| Android Studio | Latest | Optional but recommended |

### Machine-Specific Configuration

Create `local.properties` in the project root.

Recommended Linux/WSL-first setup:

```properties
sdk.dir=/home/your-user/Android/Sdk
ndk.dir=/home/your-user/Android/Sdk/ndk/25.0.8775105
```

If you are deliberately using Windows-hosted Android tooling for Gradle tasks, adjust the paths to your Windows installation instead.

Adjust paths to match your actual Android SDK/NDK installation location.

## Automated Environment Setup

To automate Linux/WSL setup, use the provided setup script:

```bash
# Full setup (native + Android tools in WSL)
bash scripts/setup-wsl-environment.sh

# Native tools only
bash scripts/setup-wsl-environment.sh --native-only

# Android tools only
bash scripts/setup-wsl-environment.sh --android-only
```

The script is intended for Linux/WSL environment bootstrapping. It will:

- Install/verify CMake, GCC, Python, Make
- Download and install Android SDK/NDK in WSL
- Set up environment variables
- Create `local.properties` automatically
- Build and verify native tests
- Test Gradle configuration

After running, update your shell configuration:

```bash
source ~/.bashrc  # or ~/.zshrc if using zsh
```

For manual setup, see [Machine-Specific Configuration](#machine-specific-configuration).

## Quick Start

### For Native Engine Development (Linux/WSL)

```bash
# Clone and enter directory
git clone <repo-url>
cd sentinel-v-v2x-bridge

# Build native engine with CMake
mkdir -p native-engine/build
cd native-engine/build
cmake ..
make

# Run native tests
./tests/crypto_engine_test
```

### For Android Development (Windows/Mac/Linux)

```bash
# Ensure local.properties is configured (see Prerequisites)

# Build Android app
./gradlew :app:build

# Build Android library
./gradlew :android-app:build

# Run instrumented tests on emulator (x86_64 validated)
# Exact validated class filter:
./gradlew :android-app:connectedDebugAndroidTest -Pandroid.testInstrumentationRunnerArguments.class=com.sentinel.v2x.V2XJNITest
```

### For Full Environment Setup

1. Set up Android SDK/NDK paths in `local.properties`
2. For native work: use Linux/WSL terminal
3. For Android work: use Android Studio or command-line Gradle
4. For validation: run native tests on Linux, then Android tests on emulator/device

## What Changed From The Original Plan

The original approach assumed:

- Android Studio on Windows 11
- SDK on Windows
- NDK/toolchain work in WSL/Linux
- a smooth hybrid build flow from Windows UI tooling into Linux native tooling

That plan diverged in practice.

The main issues were:

- Windows/WSL path handling became fragile
- Gradle and Java execution differed by host environment
- native packaging accidentally pulled Linux-built shared libraries into Android
- documentation drifted away from the real module/build state

As a result, the project has effectively shifted to:

- Linux/WSL for full native engine work
- Android for JNI integration validation
- a minimal Android JNI library target, `v2x-jni`, for current device/emulator tests

## Module Layout

```text
sentinel-v-v2x-bridge/
+-- app/                 Android application module
+-- android-app/         Android library / JNI integration module
+-- native-engine/       Native C++ engine and native tests
+-- docs/                Supporting documentation
+-- gradle/              Gradle wrapper files
+-- build.gradle.kts     Root Gradle config
+-- settings.gradle.kts  Includes :app and :android-app
+-- gradle.properties    Shared Gradle settings
+-- local.properties     Local SDK/NDK paths (machine-specific)
```

## Active Build Model

Current intended dependency chain:

```text
app -> android-app -> native-engine
```

In practice:

- `app` builds the Android app
- `android-app` builds the Android library and JNI-facing Android integration
- `native-engine` contains the real native engine implementation

For Android instrumented testing today, the active native target is:

- `v2x-jni`

This target is intentionally minimal and avoids pulling full Botan dependencies into the Android test APK.

## Native Engine Status

`native-engine/` is no longer a placeholder.

It already contains:

- C++ headers in `native-engine/include/`
- C++ implementation in `native-engine/src/`
- native tests in `native-engine/tests/`
- CMake-based native build logic in `native-engine/CMakeLists.txt`

Native engine work is currently best treated as Linux/WSL-first.

## Android Status

Android support currently means:

- Gradle modules `:app` and `:android-app`
- Android instrumented tests for JNI integration
- minimal JNI validation via `V2X.kt` and `v2x-jni`

Android support does not currently mean:

- stable full Botan-backed crypto packaging into the Android APK/test APK
- a fully frictionless Windows Android Studio + WSL backend workflow

## Recommended Working Model

### Linux / WSL

Use Linux/WSL for:

- native engine development
- native CMake builds
- native test execution
- Botan-backed crypto development

### Android

Use Android for:

- app/module integration
- instrumented JNI smoke tests
- validating that the Android boundary is wired correctly

## Current Status Matrix

| Feature | Linux/WSL | Android Emulator (x86_64) | Android Device (arm64-v8a) | Notes |
|---|---|---|---|---|
| Native Engine Build | Ready | N/A | N/A | CMake full build works reliably |
| Native Tests | Ready | N/A | N/A | Can run locally in Linux/WSL |
| Android App Compile | Ready | Host op | Host op | Gradle builds reliably (runs on host) |
| Android Library Compile | Ready | Host op | Host op | JNI integration module builds (runs on host) |
| JNI Integration Tests | N/A | Ready | Experimental | x86_64 emulator validated; arm64-v8a device support incomplete |
| Full Botan Packaging | Ready | Not supported | Not supported | Not stable in APK yet |
| Hybrid Build (Win+WSL) | Not supported | N/A | N/A | Path handling fragile, not recommended |

**Legend:** Ready = validated and supported | Experimental = limited/partial support | Not supported = not a supported path | N/A = not applicable | Host op = runs on host, not on emulator/device

## Build Notes

### Gradle project

The active Gradle modules are:

- `:app`
- `:android-app`

### Android JNI library name

The active Android JNI library currently loaded by Kotlin is:

- `v2x-jni` (the shared library `libv2x-jni.so` generated by the Android build)

Older references to `security-engine` or `libsecurity-engine.so` in historical docs should be treated as stale.

## Known Documentation Drift

The older documentation may still incorrectly state that:

- `native-engine/` is future work or empty
- `security-engine` is the active Android JNI library
- Botan-backed Android packaging is already stable
- the Windows-host + WSL-backend setup is a settled, reliable default

Those assumptions are no longer accurate.

## Troubleshooting Common Issues

### Windows/WSL Path Handling

**Problem:** Gradle fails with path resolution errors when using WSL tools from Windows.

**Solution:** Use Linux/WSL terminal for all native engine work. Use Windows-based Android Studio or `gradlew.bat` only for Android Gradle tasks, not native CMake.

### Native Tests Don't Run on Android

**Problem:** CMake-built native tests fail when copied to Android device.

**Solution:** Native tests are intentionally Linux/WSL-only. The minimal JNI wrapper (`v2x-jni`) provides Android validation only. For comprehensive testing, build and run natively on Linux/WSL.

### NDK Compilation Issues

**Problem:** `android-app` fails to build with NDK errors about missing includes.

**Solution:**
1. Verify `local.properties` has the correct pinned NDK path (`25.0.8775105`)
2. Run `./gradlew clean` to clear build cache
3. Check `native-engine/CMakeLists.txt` for Android-specific configuration

### Botan Library Not Found in APK

**Problem:** App crashes with `libbotan-2.so not found` at runtime.

**Solution:** Full Botan packaging into the Android APK is not currently stable. Current Android builds use only the minimal JNI layer. For production Botan integration, this requires additional work (see `docs/PHASE-3-COER-LIBRARY-STRATEGY.md`).

## Documentation & Resources

For detailed technical information, see:

- [ARCHITECTURE-DIAGRAMS.md](docs/ARCHITECTURE-DIAGRAMS.md) - System architecture overview
- [PHASE-2-COMPLETION-REPORT.md](docs/PHASE-2-COMPLETION-REPORT.md) - Project status and completed work
- [BOTAN-LICENSING-GUIDE.md](docs/BOTAN-LICENSING-GUIDE.md) - Botan integration and licensing
- [MINIMAL-JNI-IMPLEMENTATION.md](docs/MINIMAL-JNI-IMPLEMENTATION.md) - JNI layer technical details
- [PHASE-3-COER-LIBRARY-STRATEGY.md](docs/PHASE-3-COER-LIBRARY-STRATEGY.md) - Future roadmap and COER library plans
- [native-engine/docs/](native-engine/docs/) - Native engine design and test vectors

## Practical Summary

The product goal has not changed much:

- Android app
- JNI bridge
- native V2X engine

The execution model has changed significantly:

- less confidence in the original hybrid Windows/WSL workflow
- more Linux-first native development
- thinner Android JNI validation path
- more need to keep documentation aligned with actual build behavior
