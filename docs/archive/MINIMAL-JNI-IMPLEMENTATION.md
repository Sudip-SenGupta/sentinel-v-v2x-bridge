# Minimal JNI Implementation

This document describes the current minimal Android JNI validation path.

## Scope

The current Android JNI path is intentionally minimal. Its purpose is to verify:

- Android can load the native library
- Kotlin can call into JNI successfully
- basic string marshaling works end to end

It is not the full Botan-backed Android crypto integration path.

## Active Components

### 1. Native JNI wrapper

File:
- `native-engine/src/v2x_jni.cpp`

Purpose:
- exposes a small JNI surface for Android validation
- currently centered on `getVersion()` and other minimal validation methods

### 2. Kotlin interface

File:
- `android-app/src/main/kotlin/com/sentinel/v2x/V2X.kt`

Purpose:
- loads the active Android JNI library: `v2x-jni`
- exposes the Android-facing API used by app code and tests

### 3. Android instrumented tests

Files:
- `android-app/src/androidTest/kotlin/com/sentinel/v2x/V2XJNITest.kt`
- `app/src/androidTest/kotlin/com/sentinel/v2x/V2XJNITest.kt` if present in the application module

Purpose:
- validate JNI load and invocation on emulator/device

## Current Library Name

The active Android JNI library is:

- `v2x-jni`

Older references to `security-engine`, `libsecurity-engine.so`, or `SecurityEngine.kt` are stale and should not be used for the current Android path.

## Validated Command

```bash
./gradlew :android-app:connectedDebugAndroidTest -Pandroid.testInstrumentationRunnerArguments.class=com.sentinel.v2x.V2XJNITest
```

## What This Document Does Not Claim

This document does not claim that:

- full Botan-backed Android packaging is stable
- all native crypto functionality is available in the Android APK
- the older `SecurityEngine` JNI path is still active

## Current Interpretation

Use this document as a note on the minimal Android JNI validation path only.

For broader project setup and workflow guidance, see:

- `README.md`
- `DIRECTORY-STRUCTURE-GUIDE.md`
- `docs/DOCUMENTATION-INDEX.md`
