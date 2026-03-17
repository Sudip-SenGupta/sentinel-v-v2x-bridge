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

## � Complete Message Processing Pipeline (Phase 1-6A)

This diagram shows the entire V2X message lifecycle from raw packet input through cryptographic verification, representing all phases completed to date:

```mermaid
graph TD
    A["📡 V2X Message Input<br/>(Vehicle, OBU, RSU)"] --> B["📦 Raw Packet Data<br/>(Binary COER Format)"]
    
    B --> C["🔍 Phase 2-3: COER Decoding<br/>native-engine/v2x_coer_decoder.cpp"]
    C --> D["✅ Decoded Message Frame<br/>(VehicleInfo, Location, Speed, etc)"]
    
    D --> E["🔄 Phase 4: Message Processing<br/>v2x_message_frame.cpp"]
    E --> F["📋 Message Batch Storage<br/>(In-Memory Buffer)"]
    
    F --> G["🧪 Phase 5: Validation<br/>validateMessage()"]
    G --> H{Message Valid?}
    H -->|❌ No| I["⚠️ Discard"]
    H -->|✅ Yes| J["✔️ Validation Passed"]
    
    J --> K["🔐 Phase 6A: Cryptographic Verification<br/>(Botan Integrated)"]
    
    K --> K1["SHA-256 Hash<br/>Message Digest"]
    K --> K2["ECDSA Verify<br/>Signature Check"]
    K --> K3["X.509 Validate<br/>Certificate Chain"]
    
    K1 --> L{Crypto Valid?}
    K2 --> L
    K3 --> L
    
    L -->|❌ No| M["🚫 Authentication Failed"]
    L -->|✅ Yes| N["🎯 Authenticated Message<br/>(On-Device, No Server Required)"]
    
    N --> O["📤 Output Options"]
    O --> O1["Application Logic"]
    O --> O2["Analytics/Logging"]
    O --> O3["Vehicle Systems Integration"]
    
    I -.->|Discarded| P["❌ Invalid Messages"]
    M -.->|Rejected| P
    
    style A fill:#e1f5ff
    style C fill:#fff3e0
    style E fill:#f3e5f5
    style G fill:#e8f5e9
    style K fill:#fce4ec
    style K1 fill:#ffe0b2
    style K2 fill:#ffe0b2
    style K3 fill:#ffe0b2
    style N fill:#c8e6c9
    style I fill:#ffcdd2
    style M fill:#ffcdd2
    style P fill:#ef5350
```

### Pipeline Stages Explanation

| Stage | Phase | Component | Purpose |
|-------|-------|-----------|---------|
| **1. Input** | N/A | V2X Source | Raw V2X messages from vehicles, infrastructure |
| **2. Decoding** | 2-3 | COER Decoder | Converts binary COER format to structured data |
| **3. Processing** | 4 | Message Frame | Organizes decoded data, manages batch buffering |
| **4. Validation** | 5 | V2X Validator | Checks message format and content integrity |
| **5. Cryptography** | 6A | Botan Engine | On-device signature & certificate verification |
| **6. Output** | 6A | Dispatcher | Routes authenticated messages to application |

### Key Achievement: Phase 6A On-Device Cryptography
- ✅ **Botan 2.19.1** cryptographic library integrated
- ✅ **7 JNI Functions** for SHA-256, ECDSA, RSA, X.509 validation
- ✅ **No Server Dependency** - All verification happens on device
- ✅ **Low Latency** - Crypto operations complete in <500ms
- ✅ **18 MB APK** - Both arm64-v8a and x86_64 architectures

---

## �📋 Project Structure

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
    
    AndroidApp --> AndroidJNI["V2X.kt<br/>Unified JNI Interface"]
    AndroidApp --> AndroidCPP["native-engine JNI bridge<br/>message + crypto sources"]
    AndroidApp --> AndroidCMake["CMakeLists.txt<br/>CMake Configuration"]
    AndroidApp --> AndroidLib["libv2x-jni.so<br/>Native Libraries arm64-v8a x86_64"]
    
    NativeEngine --> EngineInc["include/<br/>COER + crypto headers"]
    NativeEngine --> EngineSrc["src/<br/>COER decoder, JNI bridge, crypto engine"]
    NativeEngine --> EngineTests["tests/<br/>Native decoder + integration tests"]
    
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
  - Initial Android JNI bridge prototype
  - Legacy `SecurityEngine` interface introduced in this phase
  - CMake build configuration
  - Gradle integration with externalNativeBuild
  - Native compilation for arm64-v8a and x86_64
  - Historical build artifacts for the early JNI prototype

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
    K["Kotlin Code<br/>V2X.kt"]
    JNI["JNI Layer<br/>Data Marshalling"]
    C["C++ Code<br/>native-engine/src/*"]
    ENGINE["Native V2X Engine<br/>COER + Crypto Operations"]
    
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

### Active JNI Functions

| Function | Kotlin Signature | C++ Implementation |
|----------|------------------|-------------------|
| `getVersion()` | `() -> String` | `Java_com_sentinel_v2x_V2X_getVersion` |
| `detectFrameType()` | `(ByteArray) -> String` | `Java_com_sentinel_v2x_V2X_detectFrameType` |
| `processMessage()` | `(ByteArray) -> DecodedV2XMessage` | `Java_com_sentinel_v2x_V2X_processMessage` |
| `processBatch()` | `(List<ByteArray>) -> List<DecodedV2XMessage>` | `Java_com_sentinel_v2x_V2X_processBatch` |
| `cryptoInitialize()` | `() -> Boolean` | `Java_com_sentinel_v2x_V2X_cryptoInitialize` |
| `sha256Hash()` | `(ByteArray) -> ByteArray` | `Java_com_sentinel_v2x_V2X_sha256Hash` |
| `sha256Hex()` | `(ByteArray) -> String` | `Java_com_sentinel_v2x_V2X_sha256Hex` |
| `verifySignature()` | `(ByteArray, ByteArray, ByteArray) -> Boolean` | `Java_com_sentinel_v2x_V2X_verifySignature` |
| `isValidCertificate()` | `(ByteArray) -> Boolean` | `Java_com_sentinel_v2x_V2X_isValidCertificate` |
| `validateCertificateChain()` | `(Array<ByteArray>) -> Boolean` | `Java_com_sentinel_v2x_V2X_validateCertificateChain` |
| `getCryptoBotanVersion()` | `() -> String` | `Java_com_sentinel_v2x_V2X_getCryptoBotanVersion` |

`parseIEEE1609Message()` belonged to the older `SecurityEngine` JNI surface and is no longer part of the active Android implementation.

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
    
    H["libv2x-jni.so<br/>Compiled Native Library"]
    
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
6. Compiles the active `v2x-jni` native sources for multiple ABIs
7. Links against Android system libraries
8. Produces `libv2x-jni.so` binaries
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

## �️ Technical Architecture Layers (Complete Stack)

This diagram illustrates the complete layered architecture from Kotlin application through to Botan cryptographic library:

```mermaid
graph LR
    subgraph "📱 Kotlin/Java Layer"
        K1["V2X.kt<br/>(11 Functions)"]
        K2["V2XMessage.kt<br/>(Data Models)"]
        K3["Message Processor<br/>Interface"]
    end
    
    subgraph "🌉 JNI Bridge"
        J1["Message JNI<br/>(4 functions)"]
        J2["Crypto JNI<br/>(7 functions)"]
    end
    
    subgraph "C++ Native Layer"
        N1["COER Decoder<br/>v2x_coer_decoder.cpp"]
        N2["Message Frame<br/>v2x_message_frame.cpp"]
        N3["Message Processor<br/>v2x_jni_message_processor.cpp"]
        N4["Crypto Engine<br/>v2x_jni_crypto.cpp"]
        N5["Crypto Wrapper<br/>v2x_crypto_engine.cpp"]
    end
    
    subgraph "🔐 Botan Cryptography"
        B1["SHA-256"]
        B2["ECDSA P-256"]
        B3["RSA 2048"]
        B4["X.509 Validation"]
    end
    
    subgraph "📦 NDK Integration"
        NDK["Android NDK<br/>arm64-v8a + x86_64<br/>API Level 21+"]
    end
    
    K1 --> J1
    K1 --> J2
    K2 --> K1
    K3 --> K1
    
    J1 --> N1
    J1 --> N2
    J1 --> N3
    J2 --> N4
    
    N3 --> N1
    N3 --> N2
    N4 --> N5
    
    N5 --> B1
    N5 --> B2
    N5 --> B3
    N5 --> B4
    
    N1 -.-> NDK
    N2 -.-> NDK
    N3 -.-> NDK
    N4 -.-> NDK
    N5 -.-> NDK
    
    style K1 fill:#81c784
    style K2 fill:#81c784
    style K3 fill:#81c784
    style J1 fill:#64b5f6
    style J2 fill:#64b5f6
    style N1 fill:#ffb74d
    style N2 fill:#ffb74d
    style N3 fill:#ffb74d
    style N4 fill:#f48fb1
    style N5 fill:#f48fb1
    style B1 fill:#ce93d8
    style B2 fill:#ce93d8
    style B3 fill:#ce93d8
    style B4 fill:#ce93d8
    style NDK fill:#90caf9
```

### Layer Breakdown

#### 📱 Kotlin/Java Layer (Application Interface)
- **V2X.kt**: Unified interface exposing 11 external native functions
  - 4 Message processing functions (`getVersion`, `detectFrameType`, `processMessage`, `processBatch`)
  - 7 Cryptographic functions (`cryptoInitialize`, `sha256Hash`, `sha256Hex`, `verifySignature`, `isValidCertificate`, `validateCertificateChain`, `getCryptoBotanVersion`)
- **V2XMessage.kt**: Data classes for decoded V2X messages
- **Message Processor Interface**: Callbacks and handlers for processed messages

#### 🌉 JNI Bridge (Java↔C++ Interoperability)
- **Message JNI** (4 functions): Bridges message operations
  - `getVersion()` - Native engine version query
  - `detectFrameType()` - COER frame identification
  - `processMessage()` - COER binary to Kotlin objects
  - `processBatch()` - Batch message decoding
  
- **Crypto JNI** (7 functions): Bridges cryptographic operations
  - `sha256Hash()` - Message digest computation
  - `cryptoInitialize()` - One-time crypto engine setup
  - `sha256Hex()` - Message digest as hex string
  - `verifySignature()` - ECDSA signature validation
  - `isValidCertificate()` - Single X.509 certificate validation
  - `validateCertificateChain()` - Certificate chain validation
  - `getCryptoBotanVersion()` - Botan version query

#### C++ Native Layer (Core Implementation)
- **COER Decoder** (`v2x_coer_decoder.cpp`): Parses binary COER format
- **Message Frame** (`v2x_message_frame.cpp`): Manages message structure
- **Message Processor** (`v2x_jni_message_processor.cpp`): Orchestrates message flow
- **Crypto Engine** (`v2x_jni_crypto.cpp`): JNI wrapper for Botan operations
- **Crypto Wrapper** (`v2x_crypto_engine.cpp`): Botan API abstraction layer

#### 🔐 Botan Cryptography (Core Security Library)
- **SHA-256**: Secure hash algorithm for message digests
- **ECDSA P-256**: Elliptic curve digital signatures (IEEE 1609.2)
- **RSA 2048**: RSA digital signatures (key management)
- **X.509 Validation**: Certificate chain and trust validation

#### 📦 Android NDK Integration
- **Compiled for multiple architectures**: arm64-v8a + x86_64
- **API Level 21+**: Supports wide range of Android devices
- **Static Botan linking**: 6.2MB arm64-v8a, 5.8MB x86_64

### Data Flow Through Layers

```
App Request (Kotlin)
    ↓
JNI Bridge (Data conversion)
    ↓
C++ Message/Crypto Engine
    ↓
Botan Library (Cryptographic operations)
    ↓
Result ← Convert to Java ← JNI Bridge ← Response to App
```

### Key Statistics

| Layer | Files | LOC | Components |
|-------|-------|-----|------------|
| **Kotlin/Java** | 3 | ~400 | 11 external functions |
| **JNI Bridge** | 2 | ~800 | Auto-generated headers |
| **C++ Native** | 5 | ~3,200 | COER + Message + Crypto |
| **Botan** | 83MB (build) | Millions | Complete crypto library |
| **Total APK** | N/A | ~4,000 (custom) | 18 MB final |

---

## �🔐 Security Flow Diagram

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
    
    ARM64_OBJ["v2x-jni objects"]
    X86_64_OBJ["v2x-jni objects"]
    
    ARM64_SO["libv2x-jni.so<br/>ARM64<br/>ELF 64-bit"]
    X86_64_SO["libv2x-jni.so<br/>x86_64<br/>ELF 64-bit"]
    
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

## ⚡ Performance & Lifecycle Best Practices

### Critical: API Lifecycle Management

**⚠️ IMPORTANT:** The Botan cryptographic engine must be initialized exactly **once** during application startup, not on every message:

```kotlin
// ✅ CORRECT: Initialize in Application.onCreate()
class SentinelApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        V2X.cryptoInitialize()  // One-time initialization
    }
}

// ❌ WRONG: Re-initialize on every call (massive overhead)
fun processMessage(msg: ByteArray) {
    V2X.cryptoInitialize()  // ❌ DO NOT DO THIS!
    // ...
}
```

**Impact:**
- One-time init: ~150 ms (negligible)
- Re-initialization per 50 BSMs: Would waste 7.5 seconds CPU per 10 seconds
- **Result:** Single init call saves 999% of unnecessary CPU ✅

**See:** [PHASE-6A-COMPLETION-REPORT.md - API Lifecycle Management](../docs/PHASE-6A-COMPLETION-REPORT.md#️-api-lifecycle-management---best-practices) for full details.

### Performance: Certificate Chain Caching

**Scenario:** 50 vehicles sending 1 BSM each per second (common highway density)
- **Without caching:** 50 X.509 certificate validations/sec × 89ms = **4.45 seconds CPU needed (impossible!)**
- **With caching:** 15 unique CA chains × 89ms = 1.3 seconds total, cached hits = 0.5ms each = **~90% CPU savings** ✅

**Quick Implementation:**
```kotlin
// Implement LRU cache for certificate validation with 60s TTL
val certCache = CertificateValidationCache(maxSize = 100, ttlMillis = 60_000)

// Every message
if (!certCache.getOrValidate(message.certChain)) {
    // Certificate validation failed
    return INVALID
}

// Result: 178x faster for repeated certificate chains!
```

**See:** [PHASE-6A-COMPLETION-REPORT.md - Performance Optimization](../docs/PHASE-6A-COMPLETION-REPORT.md#-performance-optimization-guidelines) for implementation details and benchmarks.

### Real-World Throughput Comparison

| Scenario | Without Optimization | With Optimization | Delta |
|----------|--------------------|--------------------|-------|
| **50 BSMs/sec (same car)** | ❌ CPU throttle | ✅ 90% efficient | 900% improvement |
| **250 mixed BSMs/sec** | ❌ Dropped messages | ✅ All processed | 178x faster |
| **High-density highway** | ❌ Impossible | ✅ ~15% CPU | Production-ready |

**Critical Resources:**
- 📖 [Lifecycle Management Guide](../docs/PHASE-6A-COMPLETION-REPORT.md#️-api-lifecycle-management---best-practices)
- 🚀 [Performance Guide](../docs/PHASE-6A-COMPLETION-REPORT.md#-performance-optimization-guidelines)
- 📊 [Real-World Benchmarks](../docs/PHASE-6A-COMPLETION-REPORT.md#-real-world-performance-projections)

---

**Phase**: 1-6A Complete (Message Processing + Cryptography)

