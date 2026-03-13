# Sentinel-V V2X Bridge: Current Directory Structure

This file describes the current repository structure as it exists now, not every historical intermediate state.

## High-Level Summary

```text
sentinel-v-v2x-bridge/
+-- app/                 Android application module
+-- android-app/         Android library module for Android/JNI integration
+-- native-engine/       Native C++ engine and native tests
+-- docs/                Project documentation
+-- scripts/             Utility scripts
+-- gradle/              Gradle wrapper/config
+-- build/               Generated build output
```

## Important Clarification

The repository originally aimed for a clean hybrid workflow:

- Android Studio on Windows 11
- SDK on Windows
- NDK and native compilation in WSL/Linux

That model did not stay clean in practice. The current repository should be understood as:

- Linux/WSL-first for full native engine development
- Android-focused for JNI integration validation
- partially migrated from older JNI and packaging assumptions

## Root Configuration Files

### `settings.gradle.kts`

Defines the active Gradle modules:

- `:app`
- `:android-app`

### `build.gradle.kts`

Root-level Gradle plugin configuration.

### `gradle.properties`

Shared Gradle runtime settings.

### `local.properties`

Machine-specific SDK/NDK paths.

This file is local configuration and should not be treated as portable project configuration.

## Module Structure

## 1. `app/`

Android application module.

Purpose:

- app packaging
- Android manifest/resources
- app-level Android tests
- dependency on `:android-app`

Current role:

- this is the runnable Android application module
- it should not be the place where full native engine complexity is owned directly

## 2. `android-app/`

Android library module.

Purpose:

- Android-side JNI integration
- Android instrumented JNI tests
- native bridge loading from Kotlin

Current role:

- this is the Android module that owns the JNI-facing integration layer
- it is the module used for current JNI/device validation

Important note:

- the active Android Kotlin API is `android-app/src/main/kotlin/com/sentinel/v2x/V2X.kt`
- the active minimal Android JNI target is currently `v2x-jni`
- older `SecurityEngine`-based JNI references should be treated as legacy/stale

## 3. `native-engine/`

Native C++ engine.

This is not future placeholder work anymore.

It currently contains:

- `include/`: C++ headers
- `src/`: C++ implementation
- `tests/`: native test suite
- `CMakeLists.txt`: native build configuration

Current role:

- primary home of crypto, decoding, validation, and message-processing logic
- best supported in Linux/WSL native builds

## Current Dependency Model

Current intended dependency chain:

```text
app -> android-app -> native-engine
```

Meaning:

- `app` depends on the Android library
- `android-app` is the Android integration layer
- `native-engine` contains the substantive native implementation

## What Is Stable vs Less Stable

### More Stable

- native engine source structure
- Linux/WSL native development model
- app + android-app module split
- minimal Android JNI smoke/integration path

### Less Stable

- full Botan-backed Android packaging
- old Windows-host plus WSL-backend assumptions
- older JNI naming/path references in historical docs
- any doc that still describes `native-engine` as empty/future-only

## Historical Drift To Be Aware Of

The repository has carried several generations of assumptions at once:

1. original JNI bridge naming around `SecurityEngine`
2. planned standalone native engine layering
3. full native crypto implementation in `native-engine`
4. later minimal Android JNI validation through `V2X.kt` and `v2x-jni`

That means some files and docs may reflect older intermediate states rather than the current intended state.

## How To Read The Repo Today

If you are trying to understand the project quickly, use this mental model:

1. `app/` is the Android app shell
2. `android-app/` is the Android JNI integration module
3. `native-engine/` is the real native engine
4. Linux/WSL is the primary trustworthy environment for full native engine work
5. Android currently validates the JNI boundary more than the full native crypto stack

## Documentation Caveat

If another markdown file conflicts with this one, prefer the actual build files and source tree over the older documentation.
