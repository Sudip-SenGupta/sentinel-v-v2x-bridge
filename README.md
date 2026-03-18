# Sentinel-V V2X Security Bridge

Android and native V2X message-processing bridge with JNI integration, parser-compatible COER fixture coverage, and native cryptographic validation.

## Repository Layout

- `app/` - Android application module
- `android-app/` - Android library module with the active JNI-facing API
- `native-engine/` - native C++ parser, message processor, and crypto engine
- `docs/` - engineering documentation
- `THIRD-PARTY-LICENSES/` - third-party license attribution

## Current Verified Baseline

Current validated branch baseline:
- branch: `feature/v2x-parser-and-crypto-hardening`
- Android instrumentation suite: `67` passing tests
- end-to-end signed-message verification through `V2X.processMessage(...)`
- fail-closed malformed-input and trust-policy coverage

## Prerequisites

### Android
- Android SDK matching current Gradle configuration
- Android NDK `25.0.8775105`
- Java 17+
- Gradle wrapper from the repository

### Native / Linux / WSL
- CMake 3.22+
- GCC or Clang with C++17 support
- Make or Ninja
- Botan available to the native build environment

## Quick Start

### Android instrumentation tests
```bash
bash gradlew :android-app:connectedDebugAndroidTest
```

### Android test APK build only
```bash
bash gradlew :android-app:assembleDebugAndroidTest
```

### Native engine build
```bash
mkdir -p native-engine/build
cd native-engine/build
cmake ..
make
```

## Canonical Documentation

How to read the canonical docs:
- Start with [Architecture](docs/architecture.md) for system structure and runtime boundaries.
- Read [High-Level Design](docs/high-level-design.md) for design intent, trust strategy, and tradeoffs.
- Read [Low-Level Design](docs/low-level-design.md) for binary layout, JNI flow, and native verification details.
- Read [Test Plan](docs/test-plan.md) for fixture categories, execution commands, and coverage expectations.
- Read [Limitations and Deviations](docs/limitations-and-deviations.md) for standards gaps, deferred PKI work, and production risks.
- Read [Implementation Status](docs/implementation-status.md) for the current validated baseline and remaining engineering work.

- [Architecture](docs/architecture.md)
- [High-Level Design](docs/high-level-design.md)
- [Low-Level Design](docs/low-level-design.md)
- [Test Plan](docs/test-plan.md)
- [Limitations and Deviations](docs/limitations-and-deviations.md)
- [Implementation Status](docs/implementation-status.md)

## Dependencies And Attribution

Important third-party components include:
- Botan for native cryptography and PKIX validation
- BouncyCastle in Android instrumentation tests for deterministic certificate generation

Dependency strategy notes:
- Botan is treated as an external dependency rather than vendored project source
- third-party attribution is maintained under `THIRD-PARTY-LICENSES/`
- this keeps repository ownership and licensing boundaries clearer

See [THIRD-PARTY-LICENSES/README.md](THIRD-PARTY-LICENSES/README.md) for license attribution.

## Notes

This repository currently prioritizes:
- parser-compatible validation of the active implementation
- JNI/native correctness and hardening
- deterministic Android instrumentation coverage

It does not yet claim:
- full standards-faithful payload semantics
- revocation-aware PKI behavior
- production-complete semantic validation of BSM, SPaT, and PSM contents

![Visitors](https://api.visitorbadge.io/api/visitors?path=https%3A%2F%2Fgithub.com%2FSudip-SenGupta%2Fsentinel-v-v2x-bridge&label=TOTAL%20VISITS&countColor=%23263159)

