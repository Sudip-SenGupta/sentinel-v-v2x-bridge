# README Update: Dependencies & Licensing

**File:** README.md  
**Status:** Pending update before Phase 2 commit

---

## Recommended Addition to README.md

### Location
Add new section after "Getting Started" or "Installation":

### Content to Add

```markdown
## Dependencies

### System Requirements
- **CMake** 3.22+
- **Make** or Ninja
- **C++ Compiler** with C++17 support (GCC 11+, Clang 12+)

### Cryptographic Library
- **Botan 2.19.1** - Cryptographic library for ECDSA signatures and X.509 certificate validation
  - **License:** BSD-2-Clause (permissive, commercial-friendly)
  - **Installation:**
    ```bash
    # Ubuntu/Debian
    sudo apt-get install libbotan-2-dev

    # macOS (Homebrew)
    brew install botan
    ```
  - **Verification:** `botan-config --version`

### Testing Framework
- **Google Test 1.11.0** - C++ unit testing framework
  - **License:** BSD-3-Clause
  - **Installation:**
    ```bash
    # Ubuntu/Debian
    sudo apt-get install libgtest-dev

    # macOS (Homebrew)
    brew install googletest
    ```

### Platform-Specific Requirements

#### Android Development
- **Android NDK** 25+
- **Android SDK** 25+
- Botan cross-compilation support (see Phase 3 documentation)
- **⚠️ Critical:** Ensure `abiFilters` in `app/build.gradle.kts` match your prebuilt Botan architectures (e.g., `arm64-v8a`, `armeabi-v7a`)
  ```kotlin
  android {
      defaultConfig {
          ndk {
              abiFilters.addAll(listOf("arm64-v8a"))  // Must match Botan prebuilt ABI
          }
      }
  }
  ```

#### Linux Development
- Development headers: `linux-headers-generic`

---

## Building the Project

### Linux Development
```bash
cd sentinel-v-v2x-bridge

# Install dependencies
sudo apt-get install libbotan-2-dev libgtest-dev cmake make g++

# Build native-engine
mkdir -p native-engine/build
cd native-engine/build
cmake ..
make -j4

# Run tests
./tests/crypto_engine_test

# Build Android app (if NDK configured)
cd ../../../
export ANDROID_HOME=/path/to/android-sdk
export ANDROID_NDK=/path/to/android-ndk
./gradlew android-app:build
```

### Custom Botan Installation (Advanced)
If system package is unavailable or custom build needed:

```bash
# Set environment variable
export BOTAN_ROOT=/path/to/botan-2.19.1

# Build will use this version
cd native-engine/build
cmake ..
make -j4
```

**For Android NDK:** When building custom Botan for a specific ABI, ensure the `abiFilters` in `app/build.gradle.kts` match:
```bash
# Example: Build Botan for arm64-v8a
export BOTAN_ROOT=/path/to/botan-2.19.1-arm64-v8a

# Then in app/build.gradle.kts:
ndk { abiFilters.addAll(listOf("arm64-v8a")) }

# Do NOT mix: Botan built for arm64 + abiFilters = armeabi-v7a (linker error!)
```

---

## Third-Party Licenses

This project uses open-source libraries under permissive licenses:

| Library | Version | License | Source |
|---------|---------|---------|--------|
| **Botan** | 2.19.1 | BSD-2-Clause | [botan.randombit.net](https://botan.randombit.net/) |
| **Google Test** | 1.11.0 | BSD-3-Clause | [github.com/google/googletest](https://github.com/google/googletest) |

**Compliance:** Full license texts available in [THIRD-PARTY-LICENSES/](THIRD-PARTY-LICENSES/)

### Summary
- ✅ Botan: Commercial use permitted, must include attribution
- ✅ Google Test: Commercial use permitted, must include attribution
- ✅ Our code: Licensed under [YOUR LICENSE - INSERT HERE]

See [docs/BOTAN-LICENSING-GUIDE.md](docs/BOTAN-LICENSING-GUIDE.md) for detailed licensing strategy and compliance procedures.

---

## Integration Architecture (Phase 2)

### V2X Security Stack
The cryptographic engine supports three layers of V2X message security:

1. **Layer 1 - Message Formatting** (IEEE 1609.2 COER Encoding) - *Phase 3*
2. **Layer 2 - Integrity** (ECDSA Signatures) - ✅ Phase 2 Complete
3. **Layer 3 - Identity** (X.509 Certificates) - ✅ Phase 2 Complete

Native-engine provides Layers 2-3 for Android and Linux development.

See [docs/PHASE-2-COMPLETION-REPORT.md](docs/PHASE-2-COMPLETION-REPORT.md) for architecture details.

---

## Security

### Cryptographic Integrity
- All cryptographic operations use Botan 2.19.1 (audited, industry-standard library)
- ECDSA signatures verified with SHA-256 (FIPS 186-4 compliant)
- X.509 certificate validation follows RFC 5280

### Dependency Tracking
- Botan is installed as system package (not bundled)
- Users can audit and control cryptographic library version
- Security updates applied independently of app updates

### Vulnerability Reporting
To report security issues, do not open public issues. Contact [SECURITY CONTACT - INSERT HERE]

---

## Troubleshooting

### "libbotan-2.so: cannot open shared object file"
```bash
# Install missing dependency
sudo apt-get install libbotan-2-dev

# Or verify installation
ldconfig -p | grep botan
```

### CMake cannot find Botan headers
```bash
# Ensure botan-dev package is installed
sudo apt-get install libbotan-2-dev

# For custom installation:
export BOTAN_ROOT=/path/to/botan-install
cd native-engine/build && cmake ..
```

### Tests fail to compile
Ensure Google Test is installed:
```bash
sudo apt-get install libgtest-dev

# Or
brew install googletest
```

### Android NDK ABI Mismatch (Phase 3)
If you see linker errors like `undefined reference to libbotan` during Android build:
```bash
# Problem: Botan built for arm64-v8a, but app building for armeabi-v7a
# Solution: Verify abiFilters in app/build.gradle.kts matches Botan ABI

# Check what ABIs your Botan was built for:
file $BOTAN_ROOT/lib/libbotan-2.so  # Shows architecture

# Fix: Update abiFilters in app/build.gradle.kts:
android {
    defaultConfig {
        ndk {
            abiFilters.addAll(listOf("arm64-v8a"))  # Match your Botan ABI here
        }
    }
}

# Rebuild:
./gradlew clean android-app:build
```

---

## Contributing

When adding new cryptographic operations:
1. Implement in `native-engine/src/v2x_crypto_engine.cpp`
2. Add tests in `native-engine/tests/test_v2x_crypto_engine.cpp`
3. Update JNI wrapper in `android-app/src/main/cpp/SecurityEngine.cpp`
4. Verify compliance with NIST requirements

See [docs/PHASE-2-COMPLETION-REPORT.md](docs/PHASE-2-COMPLETION-REPORT.md) for performance requirements and validation procedures.
```

---

## Implementation Notes

### Where to Add This
1. Open `README.md` in editor
2. Find section after "Getting Started" or "Installation"
3. Add entire **Dependencies** section (starting with `## Dependencies`)
4. Update **Contributing** section to reference crypto operations

### What to Customize
- Replace `[YOUR LICENSE - INSERT HERE]` with actual project license
- Replace `[SECURITY CONTACT - INSERT HERE]` with actual contact info
- Adjust dependency versions if different in your environment
- Add platform-specific instructions if needed

### Verification
After update, verify:
1. All dependency names are correct (libbotan-2-dev, libgtest-dev)
2. All links reference correct files
3. Command examples work on test system
4. No dead links to internal docs

---

## Timeline for Update
- **Recommended:** Include in Phase 2 commit
- **Minimum:** Before end of Phase 2
- **Latest:** Before Phase 3 planning to document full stack

This update ensures:
- ✅ Users understand all dependencies
- ✅ Clear installation instructions
- ✅ Licensing compliance documented
- ✅ Troubleshooting guidance provided
- ✅ Architecture context available
