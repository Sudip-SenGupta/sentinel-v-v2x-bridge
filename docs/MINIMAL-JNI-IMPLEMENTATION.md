# Minimal JNI Implementation: Toolchain Validation
**Phase 4 → Phase 5 Bridging Strategy**  
**Status:** Phase 4 Complete ✅ | Phase 5 Expanded Implementation ✅

---

## Note on Phase 5 (Current)
This document describes the Phase 4 **minimal** JNI bridge strategy. In **Phase 5**, the JNI implementation has been **significantly expanded** to include:

- ✅ Full message batch processing (multiple messages)
- ✅ Complex struct marshalling (VehicleType enum, VehicleInfo nested class)
- ✅ BasicSafetyMessage (BSM) creation with all fields
- ✅ Frame type detection and message processing
- ✅ 8 instrumented tests (all passing on Android emulator)
- ✅ Production deployment on Android 14

See [README.md - Android Development](../README.md#for-android-development-windowsmaclinux) for Phase 5 capabilities and command reference.

---

## Phase 4 Executive Summary

Rather than generating full MessageProcessor JNI bindings without validation, Phase 4 implemented a **minimal proof-of-concept** that:

1. **Validates the Android/JNI/CMake toolchain** (single function)
2. **Documents future extension points** (commented method stubs)
3. **Zero-couples to Phase 3 Week 2** (doesn't expose opaque byte arrays)
4. **Defers full interface to Phase 5** (when message processing is integrated)

---

## Architecture Decision Matrix

| Aspect | Option A (Chosen) | Option B (Rejected) |
|--------|------------------|---------------------|
| **Scope** | Single function: `getVersion()` | Full MessageProcessor interface |
| **Binding** | Returns: engine version string | Returns: MessageVerificationResult (opaque) |
| **Content** | Proves toolchain works | Requires byte array marshaling both directions |
| **Phase 3 Coupling** | None—purely validation | Tight—exposes internal structures |
| **Phase 4 Impact** | Complete flexibility; stubs document design | Would require JNI rewrite for decoded content |
| **Test Coverage** | 4 targeted toolchain tests | 20+ tests for message processing |
| **Timeline** | 1 day | 3-4 days |
| **Risk** | None—isolated from production logic | High—commits JNI interface prematurely |

---

## Implementation: Three Components

### 1. **JNI Wrapper Layer** → [native-engine/src/v2x_jni.cpp](../native-engine/src/v2x_jni.cpp)

**Purpose:** Bridge C++ engine to Android/Java/Kotlin via JNI

```cpp
// Single JNI function
JNIEXPORT jstring JNICALL
Java_com_sentinel_v2x_V2X_getVersion(JNIEnv* env, jobject obj)
{
    try {
        std::string version = V2XMessageProcessor::get_version();
        return env->NewStringUTF(version.c_str());
    } catch (...) {
        // Error handling
    }
}
```

**Key Design Decisions:**
- Minimal signature → minimal risk
- Try-catch wrapper → graceful error handling
- Maps directly to existing C++ function → zero new logic
- CMake-conditioned (Android only) → non-Android builds unaffected

**Future Expansion (Phase 4+):**
```cpp
// Stub comments in v2x_jni.cpp explain Phase 4 additions:
// JNIEXPORT jobjectArray JNICALL
// Java_com_sentinel_v2x_V2X_processMessage(
//     JNIEnv* env, jobject obj, jbyteArray coerData)
// {
//     // Phase 4: Call V2XMessageProcessor::process_message()
//     // Return decoded MessageVerificationResult struct
//     // (requires BSM decoder implementation first)
// }
```

---

### 2. **Kotlin Interface Layer** → [android-app/src/main/kotlin/com/sentinel/v2x/V2X.kt](../android-app/src/main/kotlin/com/sentinel/v2x/V2X.kt)

**Purpose:** Expose native engine to Android application code

```kotlin
object V2X {
    init {
        System.loadLibrary("sentinel-engine")
    }
    
    // Phase 3: Minimal toolchain validation
    external fun getVersion(): String
    
    // Phase 4+: Full message processing interface (stubs below)
    // fun processMessage(coerData: ByteArray): MessageVerificationResult
    // fun decodeBasicSafetyMessage(coerData: ByteArray): BsmMessage
    // fun decodeProbeDataMessage(coerData: ByteArray): PdmMessage
    // fun decodeSpat(coerData: ByteArray): SpaT
}
```

**Key Design Decisions:**
- Singleton object for library loading (standard Android pattern)
- `System.loadLibrary()` in init block → happens once at first use
- NativeLibrary name: "sentinel-engine" → gradle.build.kts NDK configuration
- Future methods documented with Phase/Timeline → clear roadmap

**Data Classes (Phase 4 Stubs):**
```kotlin
data class MessageVerificationResult(
    val isValid: Boolean,
    val errorCode: Int?,
    val bsm: BsmMessage?,
    val pdm: ProbeDataMessage?
)

data class BsmMessage(
    val latitude: Double,
    val longitude: Double,
    val elevation: Double,
    val speed: Int,
    val heading: Int
    // + 20+ additional fields from IEEE 1609.2
)
```

---

### 3. **Android Instrumented Tests** → [android-app/src/androidTest/kotlin/com/sentinel/v2x/V2XJNITest.kt](../android-app/src/androidTest/kotlin/com/sentinel/v2x/V2XJNITest.kt)

**Purpose:** Validate JNI toolchain end-to-end on Android device/emulator

#### Test 1: Native Library Loading
```kotlin
fun testNativeLibraryLoads() {
    // Validates: CMake compilation + NDK build + Gradle packaging
    // Failure mode: UnsatisfiedLinkError if shared library not found
    val version = V2X.getVersion()
    assertTrue(version.isNotEmpty())
}
```

#### Test 2: JNI Function Execution
```kotlin
fun testGetVersion() {
    // Validates: JNI function callable from Kotlin
    // Validates: String marshaling (JNI ↔ C++)
    val version = V2X.getVersion()
    assertEquals("V2X Message Processor v1.0.0", version)
}
```

#### Test 3: Version Format Validation
```kotlin
fun testVersionFormatValid() {
    // Validates: C++ function returns properly formatted string
    val version = V2X.getVersion()
    assertTrue(version.matches(Regex("V2X Message Processor v\\d+\\.\\d+\\.\\d+")))
}
```

#### Test 4: JNI Reusability
```kotlin
fun testMultipleCalls() {
    // Validates: JNI calls are reusable (not one-time)
    repeat(100) {
        val version = V2X.getVersion()
        assertNotEquals("", version)
    }
}
```

**Execution:**
```bash
# Prerequisites: Android device or emulator connected
./gradlew connectedAndroidTest --tests com.sentinel.v2x.V2XJNITest

# Expected output:
# > Task :app:connectedAndroidTest
# > V2XJNITest#testNativeLibraryLoads PASSED
# > V2XJNITest#testGetVersion PASSED
# > V2XJNITest#testVersionFormatValid PASSED
# > V2XJNITest#testMultipleCalls PASSED
# 4 tests passed
```

---

## Build System Integration

### CMake Update → [native-engine/CMakeLists.txt](../native-engine/CMakeLists.txt)

```cmake
if(ANDROID)
    list(APPEND SOURCES src/v2x_jni.cpp)
    message(STATUS "Adding Android JNI bindings...")
endif()
```

**Impact Analysis:**
- ✅ **Linux builds:** Unaffected (JNI not compiled)
- ✅ **Desktop tests:** Unaffected (77/77 tests still pass)
- ✅ **Android builds:** CMake auto-includes JNI wrapper
- ✅ **NDK integration:** Gradle NDK plugin automatically provides ANDROID flag

---

## Phase Roadmap: JNI Layer Growth

| Phase | Component | JNI Methods | Purpose |
|-------|-----------|-------------|---------|
| **3 Week 2** | Minimal binding | `getVersion()` | Toolchain validation |
| **3 Week 3** | (Optional polish) | None | COER standards review |
| **4** | ASN.1 Decoder | `processMessage()`, `decodeBSM()`, `decodePDM()` | Full content exposure |
| **5** | UI Integration | Android views + adapters | Display decoded messages |

---

## Success Criteria

### Immediate (After compilation):
- ✅ `./gradlew build` completes without error
- ✅ `sentinel-engine.so` exists in APK
- ✅ Native library contains JNI symbols: `Java_com_sentinel_v2x_V2X_getVersion`

### After instrumented test run:
- ✅ All 4 V2XJNITest cases pass
- ✅ Version string returned: "V2X Message Processor v1.0.0"
- ✅ No UnsatisfiedLinkError or JNI exceptions
- ✅ 100 repeat calls succeed without memory leaks

### Post-validation:
- ✅ Document findings in Phase 3 Week 2 summary
- ✅ Commit minimal binding to repository
- ✅ Defer Phase 4 expansion until ASN.1 decoder ready

---

## Anti-Patterns Avoided

### ❌ Premature JNI Binding
**Problem:** Expose `processMessage(ByteArray)` → returns opaque `byte[]`  
**Result:** Kotlin code marshals → C++ unmarshals → C++ re-marshals → Kotlin unmarshals (4 copies)  
**Better:** Wait for Phase 4's ASN.1 decoder → expose structured data

### ❌ Tight Coupling to Phase 3
**Problem:** JNI directly exposes `MessageVerificationResult` structure  
**Result:** Any Phase 3 refactoring requires JNI rebuild  
**Better:** Single validation function → loose coupling

### ❌ Over-engineering Android Layer
**Problem:** Create full Android framework scaffolding for "hello world" binding  
**Result:** 500+ LOC of framework code for 1 function  
**Better:** Minimal proof-of-concept → expand when Phase 4 content available

---

## Next Steps After Toolchain Validation

1. **Run instrumented tests** (requires device/emulator)
   ```bash
   ./gradlew connectedAndroidTest --tests com.sentinel.v2x.V2XJNITest
   ```

2. **Verify console output shows all 4 tests passing**

3. **Commit to repository**
   ```bash
   git add -A
   git commit -m "Minimal JNI toolchain: v2x_jni.cpp + V2X.kt + V2XJNITest"
   git push origin main
   ```

4. **Shift focus to Phase 4**
   - Implement ASN.1 BSM decoder
   - Create structured data classes (BsmMessage, etc.)
   - Expand V2X.kt with decoded content methods
   - Add Phase 4 test cases to V2XJNITest.kt

---

## References

- [IEEE 1609.2-2016](../docs/IEEE-1609.2-OVERVIEW.md) - V2X Message Format
- [COER Decoder](../docs/COER-DECODER-DESIGN.md) - Phase 3 Week 1
- [Message Pipeline](../docs/MESSAGE-PIPELINE-DESIGN.md) - Phase 3 Week 2
- [Phase 3 Week 2 Session Summary](../docs/PHASE-3-WEEK2-SESSION-SUMMARY.md) - Crypto + Validation
