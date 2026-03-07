# Sentinel-V V2X Bridge: Directory Structure Explained

## Project Overview
```
sentinel-v-v2x-bridge/
├── Root Config Files (Build & Project Setup)
├── gradle/                    (Gradle Build System)
├── app/                      (Main Android Application)
├── android-app/              (JNI Bridge & Native Code)
├── native-engine/            (Crypto Logic - Future Phase 2)
├── docs/                     (Project Documentation)
├── scripts/                  (Utility Scripts)
└── build/                    (Compiled Artifacts - Auto-generated)
```

---

## 📁 ROOT-LEVEL FILES & DIRECTORIES

### **1. Build & Configuration Files** (Root Level)

#### `build.gradle.kts` ⚙️
- **Purpose:** Root-level Gradle build configuration
- **Why Needed:** Defines common settings for all modules (app, android-app)
- **Contains:**
  - Plugin versions (Android Gradle Plugin, Kotlin)
  - Common dependencies
  - Repository configurations
  - Shared build logic
- **Who Uses It:** Gradle build system applies these settings to all subprojects

#### `settings.gradle.kts` 🗂️
- **Purpose:** Project structure definition
- **Why Needed:** Tells Gradle which modules to include in the build
- **Example Content:** `include(":app", ":android-app")`
- **Impact:** Without this, Gradle won't know about app and android-app modules

#### `gradle.properties` ⚡
- **Purpose:** Gradle runtime configuration
- **Example Settings:**
  - `org.gradle.jvmargs=-Xmx4096m` (Memory allocation)
  - Gradle daemon settings
  - Logging levels
- **Why Needed:** Optimizes build performance

#### `local.properties` 🔧
- **Purpose:** LOCAL machine-specific configuration (DO NOT commit to Git)
- **Contains:**
  ```properties
  sdk.dir=/mnt/c/Users/.../Android/Sdk
  ndk.dir=/home/sudip_dev/Android/Sdk/ndk/android-ndk-r25
  ```
- **Why Needed:** Each developer's machine has different SDK/NDK paths
- **Your Setup:** Points SDK to Windows, NDK to WSL2 (hybrid strategy)

#### `README.md` 📖
- **Purpose:** Project overview and quick start guide
- **Contains:** Setup instructions, build commands, contributing guidelines

#### `LICENSE` 📜
- **Purpose:** Legal licensing information
- **Contains:** Copyright and usage terms

---

### **2. Gradle Wrapper** `gradle/wrapper/`

#### `gradle-wrapper.properties`
- **Purpose:** Specifies exact Gradle version for the project
- **Example:**
  ```properties
  distributionUrl=https\://services.gradle.org/distributions/gradle-8.2-bin.zip
  ```
- **Why Needed:** Ensures all developers use **same** Gradle version (reproducible builds)
- **Your Case:** Uses Gradle 8.2 (stored in `/tmp/gradle-8.2/`)

#### `gradlew` & `gradlew.bat`
- **Purpose:** Gradle wrapper scripts (Linux & Windows)
- **Function:** Downloads correct Gradle version automatically on first run
- **Your Setup:** Doesn't work directly in WSL2, use `/tmp/gradle-8.2/bin/gradle` instead

#### `.gradle/` 📦
- **Auto-generated directory** (by Gradle)
- **Contains:** Downloaded dependencies, build cache
- **Safe to delete:** Will be regenerated on next build
- **Why It Exists:** Caches downloads to speed up builds

---

## 📁 MODULE STRUCTURE

### **3. Main Application Module: `app/`**

```
app/
├── build.gradle.kts              # App module build config
├── proguard-rules.pro            # Code obfuscation rules
├── build/                        # Compiled outputs
│   ├── outputs/apk/             # Generated APK files
│   ├── intermediates/            # Intermediate compiled files
│   └── reports/                 # Test/lint reports
└── src/
    ├── main/                    # Main source code (runs on device)
    │   ├── AndroidManifest.xml  # App metadata & permissions
    │   ├── java/com/example/    # Java/Kotlin source files
    │   └── res/                 # Resources (UI, images, strings)
    ├── androidTest/             # Instrumented tests (runs on device)
    └── test/                    # Unit tests (runs on JVM)
```

#### **`app/build.gradle.kts`**
- **Purpose:** Configuration for main Android application
- **Defines:**
  - App package name: `com.sentinel.v2x`
  - Min/target/compile SDK versions (24/34/34)
  - NDK version (`25.0.8775105`)
  - Dependencies (Androidx, Jetpack Compose, etc.)
- **Key Section:** Gradle plugin = `com.android.application` (creates APK)

#### **`app/src/main/AndroidManifest.xml`** 📋
- **Purpose:** App declaration file (Android requires this)
- **Contains:**
  - App name and icon
  - Activities (screens)
  - Permissions (camera, location, etc.)
  - Services and broadcast receivers
  - Min/target SDK requirements
- **Impact:** Android system reads this to install/run app
- **Your Case:** Declares which permissions the app needs

#### **`app/src/main/res/`** 🎨
- **Purpose:** UI Resources
- **Structure:**
  ```
  res/
  ├── layout/           # XML layout files (.xml)
  ├── drawable/         # images (.png, .jpg, .xml)
  ├── values/           # Strings, colors, styles
  │   ├── strings.xml   # Text translations
  │   ├── colors.xml    # Color definitions
  │   └── styles.xml    # UI styling
  ├── mipmap-*/         # App icons (different resolutions)
  └── menu/             # Menu definitions
  ```
- **Why Needed:** Android separates code from UI/resources
- **Benefit:** Easy localization (strings_es.xml for Spanish, etc.)

#### **`app/src/main/java/` or `kotlin/`** 💻
- **Purpose:** Application source code
- **Your Case:** Package = `com.sentinel.v2x`
- **Contains:**
  - Activities (screens)
  - ViewModels (data management)
  - Fragment layouts
  - UI logic
- **Status:** Currently mostly template/boilerplate files

#### **`app/build/`** 🏗️ (Auto-generated)
- **outputs/apk/debug/**: Final debug APK ready for device
- **intermediates/**: Intermediate compilation artifacts
- **DO NOT commit:** Add `app/build/` to `.gitignore`

---

### **4. JNI Bridge Module: `android-app/`**

```
android-app/
├── build.gradle.kts              # Library module build config
├── proguard-rules.pro            # Code obfuscation rules
├── build/                        # Compiled outputs
│   └── intermediates/
│       └── libsecurity-engine.so # GENERATED native libraries
└── src/
    └── main/
        ├── AndroidManifest.xml   # Library manifest
        ├── kotlin/
        │   └── com/sentinel/v2x/bridge/
        │       └── SecurityEngine.kt    # JNI interface
        ├── cpp/                  # C++ native code
        │   ├── CMakeLists.txt           # CMake build config
        │   ├── SecurityEngine.cpp       # JNI implementation
        │   └── com_sentinel_v2x_bridge_SecurityEngine.h # JNI headers
        └── build.gradle.kts      # (Note: duplicate location)
```

#### **`android-app/build.gradle.kts`** 📚
- **Purpose:** Configuration for JNI bridge library
- **Key Differences from app:**
  - Plugin = `com.android.library` (creates .aar, NOT apk)
  - Enables `externalNativeBuild` (CMake)
  - Configures NDK: `ndkVersion = "25.0.8775105"`
  - ABI filters: `arm64-v8a`, `x86_64`
- **What It Does:** Tells Gradle to compile C++ code via CMake

#### **`android-app/src/main/AndroidManifest.xml`** 📋
- **Purpose:** Library manifest (minimal, no activities)
- **Contains:** Package name and permissions only
- **Different from app:** No UI components

#### **`android-app/src/main/kotlin/com/sentinel/v2x/bridge/SecurityEngine.kt`** 🌉
- **Purpose:** Kotlin JNI interface (THE BRIDGE)
- **Contains:** 6 `external` functions
  ```kotlin
  external fun verifyPacket(messageData, signature, certChain): Boolean
  external fun extractSenderInfo(certificate): String
  ...
  ```
- **How It Works:** 
  1. Kotlin calls `SecurityEngine.verifyPacket()`
  2. JNI framework looks up C++ implementation
  3. Executes `Java_com_sentinel_v2x_bridge_SecurityEngine_verifyPacket()` in C++
  4. Returns result back to Kotlin
- **Loads Native Library:** `System.loadLibrary("security-engine")` at class init

#### **`android-app/src/main/cpp/CMakeLists.txt`** 🛠️
- **Purpose:** CMake build configuration for C++
- **Contains:**
  ```cmake
  add_library(security-engine SHARED SecurityEngine.cpp)
  target_include_directories(security-engine ...)
  target_link_libraries(security-engine liblog.so)
  ```
- **What It Does:**
  1. Defines how to compile SecurityEngine.cpp
  2. Sets include paths for headers
  3. Links against Android system libraries
  4. Handles multi-architecture builds (ARM64, x86_64)
- **Called By:** Gradle's `externalNativeBuild` during build

#### **`android-app/src/main/cpp/SecurityEngine.cpp`** ⚙️
- **Purpose:** C++ JNI implementation (THE ACTUAL BRIDGE CODE)
- **Contains:** 6 JNI functions
  ```cpp
  JNIEXPORT jboolean JNICALL Java_com_sentinel_v2x_bridge_SecurityEngine_verifyPacket(...)
  ```
- **Does:** 
  - Takes Kotlin/Java data (byte arrays, objects)
  - Converts to C++ types (std::vector, strings)
  - Calls underlying security engine
  - Converts results back to Java types
  - Returns to Kotlin
- **310 Lines:** Core JNI marshalling logic

#### **`android-app/src/main/cpp/com_sentinel_v2x_bridge_SecurityEngine.h`** 📄
- **Purpose:** JNI header declarations
- **Auto-generated format:** (but manually created for speed)
- **Contains:** Function signatures for all 6 JNI methods
- **Used By:** SecurityEngine.cpp as reference

#### **`android-app/src/main/cpp/native-lib.cpp`** 
- **Purpose:** Template file (originally generated by Android Studio)
- **Status:** Not used in current implementation
- **Safe to delete:** Kept for reference

#### **`android-app/build/intermediates/...`** 🏗️ (Auto-generated)
- **Contains:** Compiled native libraries
  - `libsecurity-engine.so` (ARM64 - 62KB)
  - `libsecurity-engine.so` (x86_64 - 64KB)
- **These get packaged** into final APK

---

### **5. Future Crypto Engine: `native-engine/`** 🔐

```
native-engine/
├── CMakeLists.txt          # Standalone C++ build config
├── include/                # Header files (EMPTY - for Phase 2)
├── src/                    # Source files (EMPTY - for Phase 2)
└── tests/                  # Unit tests (EMPTY - for Phase 2)
```

#### **Purpose:** Dedicated crypto library (separate from JNI)
- **Future Plan:** Will contain all ECDSA, SHA-256, certificate validation logic
- **Why Separate:** Keeps crypto logic isolated and testable
- **Currently:** Empty, ready for Phase 2 implementation

#### **`native-engine/CMakeLists.txt`**
- **Purpose:** Build configuration for standalone crypto library
- **Will Define:**
  - Sources: SecurityEngine.cpp, V2XProtocolStack.cpp
  - Includes: Botan/OpenSSL headers
  - Outputs: libsentinel_engine.so (standalone crypto library)
- **Integration:** Will be linked from android-app CMake

---

### **6. Documentation: `docs/`** 📚

```
docs/
├── Project-Details.md      # Complete technical specification (UPDATED)
└── Project-Details.html    # HTML version for browser viewing
```

#### **`docs/Project-Details.md`**
- **Purpose:** Complete project documentation
- **Contains:**
  - Architecture overview
  - Security model
  - API specifications
  - Build configuration details
  - Workflow and environment setup
  - Known issues and troubleshooting
- **Status:** Actively maintained (updated after each phase)

---

### **7. Scripts: `scripts/`** 🚀

```
scripts/
(Currently empty - for future automation)
```

#### **Future Use:**
- Build automation scripts
- Testing scripts
- Deployment helpers
- Environment setup scripts

---

## 📊 BUILD HIERARCHY & DATA FLOW

```
User runs build command
    ↓
Gradle reads: settings.gradle.kts (knows about modules)
    ↓
For each module:
    ├─→ app/ module
    │   └─→ build.gradle.kts defines Android app compilation
    │       └─→ Kotlin/Java → javac → .class files
    │           └─→ d8 tool → dex (optimized bytecode)
    │               └─→ aapt tool → .apk (with resources)
    │
    └─→ android-app/ module
        └─→ build.gradle.kts + externalNativeBuild
            ├─→ Calls CMake (via CMakeLists.txt)
            │   └─→ CMake calls clang++ (NDK compiler)
            │       └─→ SecurityEngine.cpp → SecurityEngine.o (object file)
            │           └─→ Linker → libsecurity-engine.so (native library)
            │
            ├─→ Kotlin compilation → .class files
            │   └─→ d8 tool → dex (classes.dex)
            │
            └─→ Package into android-app-debug.aar (library + .so)
                └─→ Referenced by app module
                    └─→ Included in final app-debug.apk

Final APK Contains:
├── Kotlin/Java bytecode (in classes.dex)
├── Resources (layouts, strings, images)
├── Native libraries (lib/arm64-v8a/libsecurity-engine.so)
└── lib/x86_64/libsecurity-engine.so
```

---

## 🔄 Compilation Results (After Build)

### **Build Outputs**

#### **`app/build/outputs/apk/debug/app-debug.apk`**
- Final executable Android application
- Contains: Kotlin code + C++ libraries + resources
- Size: ~30MB
- Can be installed on device/emulator

#### **`android-app/build/intermediates/...`** (Various)
- Intermediate compilation artifacts
- Reused across incremental builds
- Safe to delete (build will regenerate)

#### **Native Libraries Generated**
```
android-app/build/intermediates/library_and_local_jars_jni/debug/jni/
├── arm64-v8a/libsecurity-engine.so      ✅ ARM64 binary (62KB)
└── x86_64/libsecurity-engine.so         ✅ x86_64 binary (64KB)
```

---

## 📋 Directory Summary Table

| Directory | Type | Purpose | Status |
| :--- | :---: | :--- | :---: |
| **app/** | Module | Main Android app (UI, activities) | Active |
| **android-app/** | Module | JNI bridge & native code | Active |
| **native-engine/** | Folder | Future crypto library | Empty (Phase 2) |
| **docs/** | Docs | Technical documentation | Active |
| **gradle/** | Config | Gradle wrapper & settings | Auto-managed |
| **scripts/** | Scripts | Automation scripts | Empty |
| **build/** | Output | Compiled artifacts | Auto-generated |
| **.gradle/** | Cache | Gradle cache & deps | Auto-generated |
| **Root configs** | Config | Build settings (.kts, .properties) | Active |

---

## 🎯 Key Concepts to Remember

### **Android Modules**
- **`app/`** = `:app` module (creates APK - runnable app)
- **`android-app/`** = `:android-app` module (creates AAR - reusable library)
- `app` depends on `android-app` (imports JNI functions)

### **Build Configuration Files (Top Priority)**
1. **`settings.gradle.kts`** - Tells Gradle what modules exist
2. **Root `build.gradle.kts`** - Common settings for all modules
3. **Module `build.gradle.kts`** - Specific settings per module
4. **`local.properties`** - Your machine's paths (SDK, NDK)
5. **`gradle.properties`** - Runtime optimization

### **C++ Build Configuration**
- **`CMakeLists.txt`** (in android-app/src/main/cpp/) - Tells CMake how to compile C++ code
- **Called by:** Gradle's `externalNativeBuild` setting in build.gradle.kts
- **Produces:** libsecurity-engine.so (native library)

### **Two-Language Bridge**
```
Kotlin/Java                          C++
┌──────────────────┐      ┌──────────────────┐
│ SecurityEngine   │      │ SecurityEngine.  │
│ .kt (interface)  │◄────►│ cpp (impl)       │
└──────────────────┘      └──────────────────┘
    JNI Boundary (data marshalling)
```

---

## 🚀 Next Steps (Phase 2)

You'll be working primarily in:
1. **`native-engine/src/`** - Implement crypto logic
2. **`native-engine/include/`** - Crypto headers
3. **`android-app/src/main/cpp/CMakeLists.txt`** - Link crypto libraries
4. **`android-app/build.gradle.kts`** - Update dependencies

The directory structure is now ready for crypto implementation!
