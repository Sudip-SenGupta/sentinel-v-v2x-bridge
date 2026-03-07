# Sentinel-V V2X Security Bridge

Professional-grade V2X (Vehicle-to-Everything) security bridge for Android automotive platforms, implementing IEEE 1609.2 cryptographic message validation.

> 📊 **Diagram Formats**: This README includes ASCII diagrams (works everywhere). For interactive Mermaid diagrams, see [docs/ARCHITECTURE-DIAGRAMS.md](docs/ARCHITECTURE-DIAGRAMS.md)

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  🚗 AUTOMOTIVE ANDROID UI                                       │
│     Kotlin/Jetpack Compose                                      │
│     (User Interface & Activities)                               │
└────────────────────┬────────────────────────────────────────────┘
                     │ Native Method Calls
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│  🌉 JNI BRIDGE                                                  │
│     Java ↔ C++ Data Marshalling                                 │
│     (SecurityEngine.kt/SecurityEngine.cpp)                      │
└────────────────────┬────────────────────────────────────────────┘
                     │ Byte Array Conversion
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│  ⚙️ SENTINEL-V ENGINE                                           │
│     C++17 Security Core                                         │
│     (ECDSA, SHA-256, X.509 Validation)                          │
└────────────────────┬────────────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
   YES  ▼                    NO   ▼
┌───────────────────┐     ┌──────────────────┐
│ ✅ VALID MESSAGE  │     │ 🚨 THREAT ALERT  │
│  V2X Dispatcher   │     │  Alert Service   │
└────────┬──────────┘     └────────┬─────────┘
         │                         │
         └─────────────┬───────────┘
                       ▼
        ┌──────────────────────────┐
        │  📡 V2V / V2I            │
        │  Communication Handler   │
        └──────────────────────────┘
```

## 📋 Project Structure

```
sentinel-v-v2x-bridge/
│
├── 📦 app/                              Main Android Application
│   ├── src/main/
│   │   ├── AndroidManifest.xml
│   │   ├── kotlin/                      Activities & UI (Kotlin)
│   │   └── res/                         Layout, Strings, Drawables
│   └── build.gradle.kts
│
├── 🌉 android-app/                      JNI Bridge Module
│   ├── src/main/
│   │   ├── kotlin/com/sentinel/v2x/bridge/
│   │   │   └── SecurityEngine.kt        ← JNI Interface (6 methods)
│   │   ├── cpp/
│   │   │   ├── SecurityEngine.cpp       ← C++ Implementation (310 LOC)
│   │   │   ├── CMakeLists.txt          ← CMake Config
│   │   │   └── native-lib.h
│   │   └── AndroidManifest.xml
│   └── build.gradle.kts                externalNativeBuild enabled
│
├── 🔐 native-engine/                    Crypto Engine (Phase 2)
│   ├── include/                         Headers (TBD)
│   ├── src/                             Implementation (TBD)
│   ├── tests/                           Unit Tests (TBD)
│   └── CMakeLists.txt
│
├── 📚 docs/
│   ├── Project-Details.md               Technical Specification
│   └── Project-Details.html
│
├── ⚙️ Gradle Configuration
│   ├── build.gradle.kts                 Root build config
│   ├── settings.gradle.kts              Module declarations
│   ├── gradle.properties                Gradle properties
│   └── local.properties                 SDK/NDK paths
│
└── 📄 Documentation Files
    ├── README.md                        (This file)
    ├── DIRECTORY-STRUCTURE-GUIDE.md     Detailed structure
    ├── PHASE-1-SESSION-SUMMARY.md       Implementation notes
    └── LICENSE
```

## 🚀 Quick Start

### Prerequisites
- **Android SDK 34+**: `/mnt/c/Users/.../Android/Sdk` (Windows)
- **NDK 25**: `/home/sudip_dev/Android/Sdk/ndk/android-ndk-r25` (WSL2)
- **Gradle 8.2**: Auto-downloaded or `/tmp/gradle-8.2/`
- **CMake 3.22+**: Included with NDK

### Build

```bash
# Full build (all modules)
cd /home/sudip_dev/sentinel-v-v2x-bridge
export ANDROID_HOME="/mnt/c/Users/SenGuptaSudip/AppData/Local/Android/Sdk"
/tmp/gradle-8.2/bin/gradle build

# Native library only (faster)
/tmp/gradle-8.2/bin/gradle android-app:assembleDebug

# Clean build
/tmp/gradle-8.2/bin/gradle clean build
```

### Installation

```bash
# Install debug APK to emulator/device
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## 📊 Build Output

| Artifact | Location | Size | Purpose |
|----------|----------|------|---------|
| **libsecurity-engine.so (ARM64)** | `android-app/build/.../arm64-v8a/` | 62 KB | ARM64 native library |
| **libsecurity-engine.so (x86_64)** | `android-app/build/.../x86_64/` | 64 KB | x86_64 native library |
| **app-debug.apk** | `app/build/outputs/apk/debug/` | ~30 MB | Complete Android app |
| **android-app-debug.aar** | `android-app/build/outputs/aar/` | - | JNI library package |

## 📈 Development Phases

```
┌─────────────────────────────────┐
│ ✅ PHASE 1: JNI BRIDGE (DONE)   │  Completed ✓
│ • Kotlin JNI Interface          │  Commit: b4cd545
│ • C++ JNI Implementation        │  47 files, +2,500 LOC
│ • CMake Build Config            │
│ • Gradle Integration            │
│ • Native Compilation (2 ABIs)   │
└──────────────┬──────────────────┘
               │ 2-3 days
               ▼
       ┌──────────────────────────────────┐
       │ 🔄 PHASE 2: CRYPTO ENGINE        │  Pending
       │ • ECDSA Signature Verification   │
       │ • SHA-256 Hashing                │
       │ • X.509 Certificate Parsing      │
       │ • Botan/OpenSSL Integration      │
       └────────────┬─────────────────────┘
                    │ 3-4 days
                    ▼
            ┌──────────────────────────────┐
            │ 🎯 PHASE 3: MESSAGE PARSER   │  Pending
            │ • IEEE 1609.2 Decoding       │
            │ • Header Extraction          │
            │ • Payload Parsing            │
            └────────────┬─────────────────┘
                         │ 2-3 days
                         ▼
                  ┌──────────────────────────┐
                  │ 🔒 PHASE 4: CERT CHAIN   │  Pending
                  │ • Expiration Checking    │
                  │ • Chain Validation       │
                  │ • Revocation List        │
                  └──────────────────────────┘
```

### Phase 1: JNI Bridge ✅ (COMPLETE)
- Kotlin JNI interface (6 external methods)
- C++ JNI implementation (310 LOC)
- CMake build configuration
- Gradle integration
- Native library compilation (both architectures)
- **Commit**: `b4cd545` — 47 files, +2,500 insertions

### Phase 2: Crypto Engine (PENDING)
- ECDSA signature verification
- SHA-256 hashing
- X.509 certificate parsing
- Botan/OpenSSL integration

### Phase 3: V2X Message Parser (PENDING)
- IEEE 1609.2 message decoding
- Header extraction
- Payload parsing

### Phase 4: Certificate Chain Validation (PENDING)
- Certificate expiration checking
- Chain validation
- Revocation list support

## 🔧 Key Technologies

| Component | Technology | Purpose |
|-----------|-----------|---------|
| **Application UI** | Kotlin, Jetpack Compose | Android user interface |
| **JNI Bridge** | C, JNI | Java ↔ Native communication |
| **Security Engine** | C++17, std library | Cryptographic operations |
| **Build System** | Gradle 8.2, CMake 3.22 | Project compilation |
| **Android NDK** | v25 (Linux) | C++ compilation toolchain |
| **Crypto Library** | Botan (planned) | ECDSA & X.509 primitives |

## 📚 Documentation

- **[Architecture Diagrams](docs/ARCHITECTURE-DIAGRAMS.md)** — Mermaid diagrams (GitHub/VS Code rendering)
- **[Project Details](docs/Project-Details.md)** — Complete technical specification
- **[Directory Structure Guide](DIRECTORY-STRUCTURE-GUIDE.md)** — Folder organization explained
- **[Phase 1 Summary](PHASE-1-SESSION-SUMMARY.md)** — Implementation details & decisions

## 🛠️ Development Environment

**WSL2 Strategy (Hybrid Setup)**
- **Windows Host**: Hosts SDK for GPU-accelerated emulator
- **WSL2 Linux**: Hosts NDK for native compilation
- **Rationale**: Windows NDK 29 lacks Linux prebuilts; Linux tools must run in WSL2

**Key Paths**
```bash
# Windows (for emulator)
SDK: /mnt/c/Users/SenGuptaSudip/AppData/Local/Android/Sdk

# WSL2 Linux (for compilation)
NDK: /home/sudip_dev/Android/Sdk/ndk/android-ndk-r25
Project: /home/sudip_dev/sentinel-v-v2x-bridge
```

## 🔗 Building & Testing

### Incremental Build (Fast)
```bash
# Only recompiles changed files (~5 seconds)
/tmp/gradle-8.2/bin/gradle android-app:assembleDebug
```

### Full Build (Slow)
```bash
# Complete rebuild of all modules
/tmp/gradle-8.2/bin/gradle build --rerun-tasks
```

### Verify Native Libraries
```bash
file android-app/build/intermediates/library_and_local_jars_jni/debug/jni/arm64-v8a/libsecurity-engine.so
# Output: ELF 64-bit LSB shared object, ARM aarch64, ...
```

## 📋 System Requirements

- **OS**: Linux (WSL2) or macOS for development
- **RAM**: 8GB minimum, 16GB recommended
- **Disk**: 20GB for SDK/NDK/gradle cache
- **Java**: JDK 11 or higher
- **Android SDK**: SDK 24+ (Android 7.0)

## 🐛 Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| `clang++: not found` | Wrong NDK prebuilts | Use NDK 25 with Linux prebuilts in WSL2 |
| `CMake not found` | Missing dependencies | CMake is bundled in NDK, ensure NDK configured |
| `JNI header mismatch` | Gradle/NDK version mismatch | Verify `ndkVersion` in build.gradle.kts matches actual NDK |
| `APK size too large` | Debug symbols included | Use `gradle assembleRelease` for optimized build |

## 📞 Support

- **Architecture Questions**: See [DIRECTORY-STRUCTURE-GUIDE.md](DIRECTORY-STRUCTURE-GUIDE.md)
- **Build Issues**: Check [gradle-build.log](gradle-build.log)
- **Implementation Details**: See [PHASE-1-SESSION-SUMMARY.md](PHASE-1-SESSION-SUMMARY.md)

## 📄 License

See [LICENSE](LICENSE) file.

---

**Last Updated**: March 7, 2026  
**Current Phase**: 1/4 (JNI Bridge - Complete)  
**Next Phase**: Crypto Engine Implementation
