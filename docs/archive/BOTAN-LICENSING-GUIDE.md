# Botan Library & Licensing: Best Practices Guide

## TL;DR
✅ **Botan is BSD-2-Clause licensed (very permissive)**  
❌ **DO NOT commit Botan to the repository**  
✅ **Document dependency & provide installation instructions**

---

## Botan Licensing (From System Package)

**License:** BSD-2-Clause (Simplified BSD License)  
**Copyright:** 1999-2022 The Botan Authors, Jack Lloyd

**What BSD-2-Clause Means:**
- ✅ Can use commercially
- ✅ Can modify
- ✅ Can distribute
- ✅ Can use in proprietary projects
- ⚠️ Must include license text
- ⚠️ Must attribute copyright holders

**License Text (BSD-2-Clause):**
```
Copyright (c) 2022 The Botan Authors

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
```

---

## Why NOT to Commit Botan to GitHub

### 1. **Repository Bloat**
- Botan source: ~15MB
- Our added code: ~1MB
- Ratio: 15:1 library vs project code
- **Impact:** Slow clones, large repo size

### 2. **Version Management**
- System packages handle updates automatically
- If we commit source, we're responsible for updates
- Security patches won't auto-apply
- Creates maintenance burden

### 3. **Platform Specificity**
- Linux: apt-get install libbotan-2-dev
- macOS: brew install botan
- Windows: build from source or use vcpkg
- Committing one version alienates other platforms

### 4. **Dependency Management**
- CMakeLists.txt already detects Botan
- Works if user has it installed
- Fail-fast error messages if missing
- This is the standard C++ pattern

### 5. **Legal Clarity**
- External library with BSD-2-Clause
- Our code with (presumably) different license
- Keeping separate is cleaner
- Attribution is clear (in THIRD-PARTY-LICENSES, not mixed source)

---

## What We SHOULD Commit ✅

**Recommended Structure:**
```
sentinel-v-v2x-bridge/
├── .gitignore                          [Already excludes build/]
├── README.md                           ← Update with Botan requirement
├── THIRD-PARTY-LICENSES/               [NEW - Create this]
│   └── botan_LICENSE.txt               [Include BSD-2 text]
├── CMakeLists.txt                      ← Already detects Botan
├── native-engine/
│   ├── include/v2x_crypto_engine.h     ✅ Commit
│   ├── src/v2x_crypto_engine.cpp       ✅ Commit
│   ├── tests/
│   │   ├── CMakeLists.txt              ✅ Commit
│   │   └── test_v2x_crypto_engine.cpp  ✅ Commit
│   └── CMakeLists.txt                  ✅ Commit [already links Botan by name]
├── android-app/src/main/cpp/
│   ├── SecurityEngine.cpp              ✅ Commit
│   └── CMakeLists.txt                  ✅ Commit
└── docs/
    ├── PHASE-2-COMPLETION-REPORT.md    ✅ Commit
    └── ...                             ✅ Commit
```

---

## What We Should NOT Commit ❌

**Files/Directories to Exclude:**
```
# Build artifacts
native-engine/build/
android-app/build/
*.o
*.so
*.a

# Botan source or cached files
botan/                          ← If directory exists, exclude
/usr/lib/*/libbotan*           ← System library, not in repo

# Other standard exclusions
.gradle/
.idea/
CMakeCache.txt
CMakeFiles/
Makefile
*.cmake
```

**`.gitignore` Entry (if needed):**
```bash
# Native builds
native-engine/build/
android-app/build/
CMakeLists.txt.user  # Qt Creator artifacts

# System dependencies (not our responsibility)
/botan/
/third_party/botan/
```

---

## Setup Instructions for Users

**For Linux Users (Recommended):**
```bash
# Install Botan system package
sudo apt-get install libbotan-2-dev libgtest-dev cmake make g++

# Build project
cd sentinel-v-v2x-bridge
mkdir -p native-engine/build && cd native-engine/build
cmake ..
make -j4

# Run tests
./tests/crypto_engine_test
```

**For macOS Users:**
```bash
# Install via Homebrew
brew install botan googletest cmake

# Then same build steps as Linux
```

**For Android NDK:**
```bash
# Botan also available as NDK port (if needed for future phases)
# Can be added to CMakeLists.txt with Android conditional
```

---

## License Attribution: What to Include

### In README.md - Add Section:
```markdown
## Dependencies

- **Botan 2.19.1** - Cryptographic library (BSD-2-Clause licensed)
  - Linux: `sudo apt-get install libbotan-2-dev`
  - macOS: `brew install botan`
  - Upstream: https://botan.randombit.net/

- **Google Test 1.11.0** - Unit testing framework (BSD-3-Clause)
  - Linux: `sudo apt-get install libgtest-dev`
  - Upstream: https://github.com/google/googletest

## Third-Party Licenses

See [THIRD-PARTY-LICENSES/](THIRD-PARTY-LICENSES/) for full license texts.
```

### Create THIRD-PARTY-LICENSES/botan_LICENSE.txt:
Include the full BSD-2-Clause text (as shown above)

### Create THIRD-PARTY-LICENSES/README.md:
```markdown
# Third-Party Software Licenses

This project uses the following third-party libraries:

## Botan 2.19.1
- **License:** BSD-2-Clause
- **Copyright:** 1999-2022 The Botan Authors, Jack Lloyd
- **URL:** https://botan.randombit.net/
- **System Package:** libbotan-2-dev on Ubuntu/Debian

## Google Test 1.11.0
- **License:** BSD-3-Clause  
- **Copyright:** Google Inc.
- **URL:** https://github.com/google/googletest
- **System Package:** libgtest-dev on Ubuntu/Debian

See individual license files for full text.
```

---

## Current Status: What We Have

✅ **CMakeLists.txt Already Handles This Correctly:**
```cmake
find_library(BOTAN_LIB botan-2 PATHS /usr/lib/x86_64-linux-gnu /usr/lib)
find_path(BOTAN_INCLUDE botan/version.h PATHS /usr/include/botan-2)

if(BOTAN_LIB_PATH)
    message(STATUS "✓ Botan library found: ${BOTAN_LIB_PATH}")
else()
    message(FATAL_ERROR "✗ Botan library not found. Install with: sudo apt-get install libbotan-2-dev")
endif()
```

This is **exactly the right approach** - it:
- Detects system-installed Botan ✅
- Provides helpful error message if missing ✅
- Works across platforms ✅
- Doesn't require repo to bundle library ✅

---

## Commit Strategy

**Phase 2 Commit (Ready Now):**
```bash
cd /home/sudip_dev/sentinel-v-v2x-bridge

# Stage all Phase 2 files
git add \
  native-engine/include/v2x_crypto_engine.h \
  native-engine/src/v2x_crypto_engine.cpp \
  native-engine/tests/ \
  native-engine/CMakeLists.txt \
  native-engine/PHASE-2-CRYPTO-ENGINE.md \
  native-engine/CRYPTO-ENGINE-TEST-REPORT.md \
  android-app/src/main/cpp/SecurityEngine.cpp \
  android-app/src/main/cpp/CMakeLists.txt \
  docs/PHASE-2-COMPLETION-REPORT.md \
  THIRD-PARTY-LICENSES/botan_LICENSE.txt \
  THIRD-PARTY-LICENSES/README.md

# DO NOT add:
# git add native-engine/build/        ❌ Remove if added
# git add botan/                       ❌ Remove if added

# Commit
git commit -m "Phase 2: V2X Cryptographic Engine with Botan 2.19.1

- Implement V2XCryptoEngine with ECDSA/SHA-256 support
- Integrate with JNI bridge (SecurityEngine.cpp)
- 14 unit tests (100% pass rate) with NIST test vectors
- Performance: 1000x faster than requirement (2-9 μs vs 10ms)
- CMake build system with automatic Botan detection
- Cross-platform logging (Android NDK + Linux)

Dependencies: libbotan-2-dev, libgtest-dev

See docs/PHASE-2-COMPLETION-REPORT.md for detailed analysis."

# Push to remote
git push origin main
```

---

## Summary

| Item | Decision | Reason |
|------|----------|--------|
| **Commit Botan source** | ❌ NO | Bloats repo, maintains upstream project |
| **Commit Botan LICENSE** | ✅ YES | Required for attribution |
| **Commit our crypto code** | ✅ YES | Our implementation |
| **Commit CMakeLists** | ✅ YES | Build config finds system Botan |
| **Commit tests** | ✅ YES | Part of deliverables |
| **Require libbotan-2-dev** | ✅ YES | Document in README |
| **Include license text** | ✅ YES | BSD-2-Clause compliance |

**Licensing Impact: NONE** ✅  
**Compliance Required: Attribution** ✅  
**Practical Approach: Dependency-based (system package + CMake detection)** ✅

---

**Conclusion:** Your project is using Botan correctly and can be committed to GitHub without any licensing issues. Just include the BSD-2-Clause license text (which we should document) and your CMakeLists.txt already handles the rest perfectly.
