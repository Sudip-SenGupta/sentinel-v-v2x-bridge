# Sentinel-V Architecture Diagrams

This document contains visual diagrams in Mermaid format for the project architecture. For universal markdown compatibility, see the ASCII diagrams in [README.md](README.md).

> **Note**: These diagrams render best in:
> - GitHub (web interface)
> - VS Code with Markdown Preview Enhanced extension
> - Online Mermaid viewers (mermaid.live)

---

## 🏗️ Architecture Overview

```mermaid
graph TD
    A["🚗 Automotive Android UI<br/>Kotlin/Jetpack Compose"] 
    B["🌉 JNI Bridge<br/>Java ↔ C++"]
    C["⚙️ Sentinel-V Engine<br/>C++17 Security Core"]
    D["🔐 Security Manager"]
    E["✅ Valid Messages<br/>V2X Dispatcher"]
    F["🚨 Invalid Messages<br/>Threat Alert Service"]
    G["📡 V2V / V2I<br/>Communication"]
    
    A -->|Native Method Calls| B
    B -->|Data Marshalling| C
    C --> D
    D -->|Signature Valid| E
    D -->|Invalid| F
    E --> G
    F --> G
    
    style A fill:#e1f5ff,stroke:#01579b,stroke-width:2px
    style B fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style C fill:#f3e5f5,stroke:#4a148c,stroke-width:2px
    style D fill:#ffe0b2,stroke:#e65100,stroke-width:2px
    style E fill:#c8e6c9,stroke:#1b5e20,stroke-width:2px
    style F fill:#ffcdd2,stroke:#b71c1c,stroke-width:2px
    style G fill:#b2dfdb,stroke:#004d40,stroke-width:2px
```

**Flow Description:**
1. **UI Layer** (Kotlin) initiates security operations
2. **JNI Bridge** marshals data between Java and C++
3. **Security Engine** performs cryptographic validation
4. **Validation Result** routes to appropriate handler
5. **Communication Layer** sends/receives V2X messages

---

## 📋 Project Structure

```mermaid
graph TD
    Root["sentinel-v-v2x-bridge<br/>(Root Project)"]
    
    Root --> App["📦 app/<br/>Main Android Application"]
    Root --> AndroidApp["🌉 android-app/<br/>JNI Bridge Module"]
    Root --> NativeEngine["🔐 native-engine/<br/>Crypto Engine Phase 2"]
    Root --> Docs["📚 docs/<br/>Documentation"]
    Root --> Config["⚙️ Build Configuration"]
    
    App --> AppMani["AndroidManifest.xml"]
    App --> AppKotlin["kotlin/<br/>Activities & Services"]
    App --> AppRes["res/<br/>Layout, Strings, Drawables"]
    
    AndroidApp --> AndroidJNI["SecurityEngine.kt<br/>JNI Interface 6 methods"]
    AndroidApp --> AndroidCPP["SecurityEngine.cpp<br/>C++ Implementation 310 LOC"]
    AndroidApp --> AndroidCMake["CMakeLists.txt<br/>CMake Configuration"]
    AndroidApp --> AndroidLib["libsecurity-engine.so<br/>Native Libraries arm64-v8a x86_64"]
    
    NativeEngine --> EngineInc["include/<br/>Crypto Headers TBD"]
    NativeEngine --> EngineSrc["src/<br/>Implementation TBD"]
    NativeEngine --> EngineTests["tests/<br/>Unit Tests TBD"]
    
    Docs --> DocDetail["Project-Details.md<br/>Technical Specification"]
    Docs --> DocDir["DIRECTORY-STRUCTURE-GUIDE.md<br/>Folder Organization"]
    Docs --> DocPhase["PHASE-1-SESSION-SUMMARY.md<br/>Implementation Details"]
    
    Config --> ConfigRoot["build.gradle.kts<br/>Root Configuration"]
    Config --> ConfigSettings["settings.gradle.kts<br/>Module Declarations"]
    Config --> ConfigProps["gradle.properties<br/>Gradle Properties"]
    Config --> ConfigLocal["local.properties<br/>SDK NDK Paths"]
    
    style Root fill:#e3f2fd,stroke:#1976d2,stroke-width:3px
    style App fill:#e1f5ff,stroke:#0277bd,stroke-width:2px
    style AndroidApp fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px
    style NativeEngine fill:#ffe0b2,stroke:#e65100,stroke-width:2px
    style Docs fill:#fce4ec,stroke:#c2185b,stroke-width:2px
    style Config fill:#f1f8e9,stroke:#558b2f,stroke-width:2px
```

**Module Purpose:**
- **app**: Runnable Android application (creates APK)
- **android-app**: JNI library module (creates AAR)
- **native-engine**: Standalone crypto library (Phase 2)
- **docs**: Project documentation
- **config**: Gradle build configuration

---

## 📈 Development Phases Timeline

```mermaid
graph LR
    P1["✅ Phase 1<br/>JNI Bridge<br/>COMPLETE"] 
    P2["🔄 Phase 2<br/>Crypto Engine<br/>PENDING"]
    P3["🎯 Phase 3<br/>Message Parser<br/>PENDING"]
    P4["🔒 Phase 4<br/>Certificate Chain<br/>PENDING"]
    
    P1 -->|2-3 days| P2
    P2 -->|3-4 days| P3
    P3 -->|2-3 days| P4
    
    style P1 fill:#4caf50,stroke:#2e7d32,stroke-width:3px,color:#fff
    style P2 fill:#ff9800,stroke:#e65100,stroke-width:2px,color:#fff
    style P3 fill:#2196f3,stroke:#1565c0,stroke-width:2px,color:#fff
    style P4 fill:#9c27b0,stroke:#6a1b9a,stroke-width:2px,color:#fff
```

### Phase Timeline Details

**Phase 1: JNI Bridge** ✅
- Duration: 2-3 days
- Status: Complete (Commit: b4cd545)
- Deliverables:
  - Kotlin SecurityEngine.kt (6 external methods)
  - C++ SecurityEngine.cpp (310 LOC JNI implementation)
  - CMake build configuration
  - Gradle integration with externalNativeBuild
  - Native compilation for arm64-v8a and x86_64
  - Build artifacts: libsecurity-engine.so (62KB + 64KB)

**Phase 2: Crypto Engine** ⏳
- Duration: 3-4 days (estimated)
- Dependencies: Phase 1 complete
- Key Tasks:
  - Implement ECDSA signature verification
  - Implement SHA-256 hashing
  - Implement X.509 certificate parsing
  - Integrate Botan or OpenSSL library
  - Update CMakeLists.txt for crypto linkage

**Phase 3: V2X Message Parser** ⏳
- Duration: 2-3 days (estimated)
- Dependencies: Phase 2 complete
- Key Tasks:
  - IEEE 1609.2 message decoding
  - Header extraction and validation
  - Payload parsing and interpretation

**Phase 4: Certificate Chain Validation** ⏳
- Duration: 2-3 days (estimated)
- Dependencies: Phases 2 & 3 complete
- Key Tasks:
  - Certificate expiration checking
  - Full chain validation
  - Revocation list support

---

## 🔄 JNI Bridge Data Flow

```mermaid
graph LR
    K["Kotlin Code<br/>SecurityEngine.kt"]
    JNI["JNI Layer<br/>Data Marshalling"]
    C["C++ Code<br/>SecurityEngine.cpp"]
    ENGINE["Native Security Engine<br/>Crypto Operations"]
    
    K -->|Java byte arrays<br/>String objects| JNI
    JNI -->|std::vector<br/>std::string| C
    C -->|Call functions| ENGINE
    ENGINE -->|Return results| C
    C -->|Convert to Java<br/>native types| JNI
    JNI -->|byte arrays<br/>boolean| K
    
    style K fill:#e1f5ff,stroke:#01579b,stroke-width:2px
    style JNI fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style C fill:#f3e5f5,stroke:#4a148c,stroke-width:2px
    style ENGINE fill:#ffe0b2,stroke:#e65100,stroke-width:2px
```

### JNI Functions (6 total)

| Function | Kotlin Signature | C++ Implementation |
|----------|------------------|-------------------|
| `verifyPacket()` | `(ByteArray, ByteArray, Array<ByteArray>) → Boolean` | `Java_com_sentinel_v2x_bridge_SecurityEngine_verifyPacket` |
| `extractSenderInfo()` | `(ByteArray) → String` | `Java_com_sentinel_v2x_bridge_SecurityEngine_extractSenderInfo` |
| `initializeWithRootCA()` | `(ByteArray) → Boolean` | `Java_com_sentinel_v2x_bridge_SecurityEngine_initializeWithRootCA` |
| `validateCertificateChain()` | `(Array<ByteArray>) → Boolean` | `Java_com_sentinel_v2x_bridge_SecurityEngine_validateCertificateChain` |
| `parseIEEE1609Message()` | `(ByteArray) → Array<String>` | `Java_com_sentinel_v2x_bridge_SecurityEngine_parseIEEE1609Message` |
| `cleanup()` | `() → Unit` | `Java_com_sentinel_v2x_bridge_SecurityEngine_cleanup` |

---

## 🔨 Build System Architecture

```mermaid
graph TD
    A["settings.gradle.kts<br/>Declares Modules"]
    B["root build.gradle.kts<br/>Common Settings"]
    
    C["app/build.gradle.kts<br/>Android Application<br/>Plugin: com.android.application"]
    D["android-app/build.gradle.kts<br/>JNI Library<br/>Plugin: com.android.library"]
    
    E["CMakeLists.txt<br/>C++ Build Config"]
    F["Gradle externalNativeBuild<br/>Calls CMake"]
    
    G["NDK Compiler<br/>clang++ v14.0.6<br/>Targets: arm64-v8a, x86_64"]
    
    H["libsecurity-engine.so<br/>Compiled Native Library"]
    
    I["app-debug.apk<br/>Final Android Package<br/>Contains JNI + Java Code"]
    
    A --> B
    B --> C
    B --> D
    D --> E
    F --> E
    D --> F
    F --> G
    G --> H
    H --> I
    C --> I
    
    style A fill:#f1f8e9,stroke:#558b2f,stroke-width:2px
    style B fill:#f1f8e9,stroke:#558b2f,stroke-width:2px
    style C fill:#e1f5ff,stroke:#0277bd,stroke-width:2px
    style D fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px
    style E fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style F fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style G fill:#fce4ec,stroke:#c2185b,stroke-width:2px
    style H fill:#c8e6c9,stroke:#1b5e20,stroke-width:2px
    style I fill:#b3e5fc,stroke:#0277bd,stroke-width:3px
```

**Build Pipeline:**
1. Gradle reads module structure
2. Applies common build settings
3. Configures JNI library module with CMake
4. Invokes CMake during build
5. CMake calls NDK C++ compiler
6. Compiles SecurityEngine.cpp for multiple ABIs
7. Links against Android system libraries
8. Produces libsecurity-engine.so binaries
9. Packages into final APK

---

## 🌍 Deployment Architecture

```mermaid
graph TD
    DEV["👨‍💻 Development Environment<br/>WSL2 Linux"]
    BUILD["🏗️ Build System<br/>Gradle + CMake + NDK"]
    APK["📦 APK Package<br/>app-debug.apk"]
    DEVICE["📱 Android Device<br/>or Emulator"]
    
    DEV -->|gradle build| BUILD
    BUILD -->|Compile + Link| APK
    APK -->|adb install| DEVICE
    
    EMULATOR["🖥️ GPU Emulator<br/>Windows Host<br/>SDK on Windows"]
    DEVICE -->|Connect via<br/>adb over network| EMULATOR
    
    style DEV fill:#e1f5ff,stroke:#0277bd,stroke-width:2px
    style BUILD fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style APK fill:#c8e6c9,stroke:#1b5e20,stroke-width:2px
    style DEVICE fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px
    style EMULATOR fill:#fce4ec,stroke:#c2185b,stroke-width:2px
```

**Hybrid Setup Rationale:**
- **WSL2 Linux**: Hosts NDK (Linux prebuilts required for native compilation)
- **Windows Host**: Hosts SDK (GPU acceleration for Android Emulator)
- **Result**: Optimal performance for both compilation and testing

---

## 🔐 Security Flow Diagram

```mermaid
graph TD
    MSG["📩 V2X Message Received"]
    EXTRACT["Extract Certificate<br/>& Signature"]
    VERIFY["🔐 Verify ECDSA<br/>Signature"]
    CHAIN["🔗 Validate Certificate<br/>Chain"]
    PARSE["📄 Parse Message<br/>Contents"]
    CHECK["✅ Security Check<br/>Passed?"]
    
    VALID["✅ Message Valid<br/>Route to Handler"]
    INVALID["🚨 Message Invalid<br/>Alert & Log"]
    
    MSG --> EXTRACT
    EXTRACT --> VERIFY
    VERIFY --> CHAIN
    CHAIN --> PARSE
    PARSE --> CHECK
    
    CHECK -->|YES| VALID
    CHECK -->|NO| INVALID
    
    VALID --> ROUTE["📡 Route to V2V/V2I<br/>Handler"]
    INVALID --> LOG["📝 Log Threat<br/>Send Alert"]
    
    style MSG fill:#e1f5ff,stroke:#0277bd,stroke-width:2px
    style EXTRACT fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style VERIFY fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px
    style CHAIN fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px
    style PARSE fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style CHECK fill:#ffe0b2,stroke:#e65100,stroke-width:2px
    style VALID fill:#c8e6c9,stroke:#1b5e20,stroke-width:2px
    style INVALID fill:#ffcdd2,stroke:#b71c1c,stroke-width:2px
    style ROUTE fill:#b2dfdb,stroke:#004d40,stroke-width:2px
    style LOG fill:#ffcdd2,stroke:#b71c1c,stroke-width:2px
```

---

## 📊 Multi-Architecture Compilation

```mermaid
graph TD
    CMAKE["CMakeLists.txt"]
    
    ARM64["🔨 arm64-v8a<br/>LLVM Compiler"]
    X86_64["🔨 x86_64<br/>LLVM Compiler"]
    
    ARM64_OBJ["SecurityEngine.o"]
    X86_64_OBJ["SecurityEngine.o"]
    
    ARM64_SO["libsecurity-engine.so<br/>ARM64 62KB<br/>ELF 64-bit"]
    X86_64_SO["libsecurity-engine.so<br/>x86_64 64KB<br/>ELF 64-bit"]
    
    APK["app-debug.apk<br/>Contains both .so files<br/>in lib/ directories"]
    
    CMAKE --> ARM64
    CMAKE --> X86_64
    
    ARM64 --> ARM64_OBJ
    X86_64 --> X86_64_OBJ
    
    ARM64_OBJ --> ARM64_SO
    X86_64_OBJ --> X86_64_SO
    
    ARM64_SO --> APK
    X86_64_SO --> APK
    
    style CMAKE fill:#fff3e0,stroke:#e65100,stroke-width:2px
    style ARM64 fill:#fce4ec,stroke:#c2185b,stroke-width:2px
    style X86_64 fill:#fce4ec,stroke:#c2185b,stroke-width:2px
    style ARM64_SO fill:#c8e6c9,stroke:#1b5e20,stroke-width:2px
    style X86_64_SO fill:#c8e6c9,stroke:#1b5e20,stroke-width:2px
    style APK fill:#b3e5fc,stroke:#0277bd,stroke-width:3px
```

**Compilation Details:**
- **CMake Generator**: Ninja (via NDK)
- **Compiler**: Android LLVM 14.0.6 (clang++)
- **C++ Standard**: C++17 with std:: library
- **Android STL**: c++_shared
- **Optimization**: Default (debug symbols included)
- **Compiler Flags**: -Wall -Wextra -Wpedantic -fPIC

---

## 📞 View Options

| Format | Location | Best For | Rendering |
|--------|----------|----------|-----------|
| **ASCII Diagrams** | [README.md](../README.md) | Universal compatibility | Any markdown viewer |
| **Mermaid Diagrams** | This file (docs/ARCHITECTURE-DIAGRAMS.md) | Rich visualization | GitHub, VS Code with extension |
| **Detailed Structure** | [DIRECTORY-STRUCTURE-GUIDE.md](../DIRECTORY-STRUCTURE-GUIDE.md) | Text-based documentation | Any text editor |

**Recommended Viewers for This File:**
- GitHub Web Interface (native Mermaid support)
- VS Code + Markdown Preview Enhanced extension
- Online: [mermaid.live](https://mermaid.live)
- GitLab, Gitea, Gitpod (Mermaid support)

---

**Last Updated**: March 7, 2026  
**Phase**: 1/4 Complete
