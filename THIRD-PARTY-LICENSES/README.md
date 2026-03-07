# Third-Party Software Licenses

This directory contains the licenses for third-party libraries used by sentinel-v-v2x-bridge.

## Included Libraries

### 1. Botan Cryptographic Library
- **File:** BOTAN_LICENSE.txt
- **License:** BSD-2-Clause
- **Copyright:** 1999-2023 The Botan Authors, Jack Lloyd
- **Website:** https://botan.randombit.net/
- **Version Used:** 2.19.1
- **Installation:** `sudo apt-get install libbotan-2-dev` (Ubuntu/Debian)

**What it does:**
- ECDSA signature verification with SHA-256
- X.509 certificate parsing and validation
- Cryptographic primitives (hashing, RNG)

**Why BSD-2-Clause is compatible:**
- ✅ Permissive open-source license
- ✅ Allows commercial use
- ✅ Allows proprietary software to link against it
- ✅ Only requires attribution (this document)

### 2. Google Test Framework
- **License:** BSD-3-Clause
- **Copyright:** Google Inc.
- **Website:** https://github.com/google/googletest
- **Version Used:** 1.11.0
- **Installation:** `sudo apt-get install libgtest-dev` (Ubuntu/Debian)

**What it does:**
- Unit testing framework for crypto engine verification
- 14 tests in native-engine/tests/test_v2x_crypto_engine.cpp

---

## Compliance

This project complies with all third-party license requirements:

✅ **Botan (BSD-2-Clause):**
- Includes license text (this file and BOTAN_LICENSE.txt)
- Attributes copyright holders in documentation
- Does not modify license terms
- Allows linking in our JNI bridge

✅ **Google Test (BSD-3-Clause):**
- Used only for testing (not distributed in binary)
- Linked dynamically via system package

---

## How Dependencies Are Managed

### At Runtime
```
sentinel-v-v2x-bridge (our code)
    ↓ links
libsentinel-engine.so (our crypto engine)
    ↓ links
libbotan-2.so (system package - BSD-2-Clause)
```

### At Build Time
```
CMakeLists.txt
    ↓ finds
libbotan-2-dev (system package)
    ↓ provides
<botan/version.h>, libbotan-2.so
```

### At Test Time
```
crypto_engine_test (our executable)
    ↓ links
libgtest.a (system package - BSD-3-Clause)
    ↓ runs
14 unit tests in test_v2x_crypto_engine.cpp
```

---

## Legal Summary

**License Compatibility:** ✅ FULLY COMPATIBLE
- All third-party libraries have permissive licenses
- Attribution is provided in this directory
- No commercial or proprietary restrictions
- Can be used in closed-source projects per BSD terms

**Recommendation:** Include this directory in your repository and keep it with your source code. When distributing the software (source or binary), include the THIRD-PARTY-LICENSES directory to maintain proper attribution.

---

## Questions?

For questions about specific licenses or usage rights, refer to:
- Botan: https://botan.randombit.net/ (contact Jack Lloyd)
- Google Test: https://github.com/google/googletest

For questions about how these dependencies are used in sentinel-v-v2x-bridge:
- See docs/PHASE-2-COMPLETION-REPORT.md
- See docs/BOTAN-LICENSING-GUIDE.md
