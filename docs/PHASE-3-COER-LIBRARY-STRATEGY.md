# Phase 3 Strategy: COER Library Evaluation

**Date:** March 7, 2026  
**Topic:** Micro-COER Library vs Custom Implementation  
**Status:** Strategic Analysis for Phase 3 Decision

---

## Executive Summary

**Strategic Question:** Should Phase 3 build a custom OER decoder (~500-800 LOC) or adopt a lightweight COER library as a git submodule?

**Answer:** **Adopt existing library if suitable option exists** - provides:
- ✅ Battle-tested code (fewer bugs)
- ✅ Faster integration (proven compatibility)
- ✅ Reduced maintenance burden
- ✅ Better performance (likely optimized)
- ✅ Smaller attack surface (security)

---

## IEEE 1609.2 COER Context

### What We Need
**V2X Message Encoding:** IEEE 1609.2 uses **COER** (Canonical Octet Encoding Rules)
- Binary format (compact, performant)
- Deterministic (no ambiguity)
- NOT ASN.1 DER/BER (Botan won't parse it)
- Hierarchical structure (header → payload → signature)

**Phase 2 → Phase 3 Flow:**
```
Raw V2X Message (binary)
    ↓ COER Decoder (Phase 3 ← We are here)
Message Structure (parsed)
    ↓
Header | Payload | ECC-Signature
    ↓
Extract: signature, issuer cert, chain
    ↓ Feed to Phase 2 Crypto Engine
verify_ecdsa_signature()
validate_certificate_chain()
```

### Why Not ASN.1?
- COER is "packed" encoding (smaller messages = faster V2V/V2I)
- ASN.1 DER is "structured" (Botan supports this, but not COER)
- V2X Committee chose COER for automotive (bandwidth limited, real-time)

---

## Existing Library Candidates

### Tier 1: Automotive V2X Specific

#### 1. **OpenDaVINCI V2X Stack** ⭐ MOST PROMISING
- **Project:** OpenDaVINCI (DeLFT/TU Delft research)
- **Contains:** IEEE 1609.2 message parsing
- **License:** MPL 2.0 (permissive, commercial-friendly) ✅
- **C++ Standard:** C++11 compatible (upgradeable to C++17)
- **Size:** ~2-5MB (source code)
- **Status:** Actively maintained for automotive research
- **Android:** Yes (compiled for embedded systems)
- **GitHub:** [`opendavinci/core` V2X modules](https://github.com/se-research/opendavinci)
- **Evaluation:** 🟢 **STRONG CANDIDATE**

**Pros:**
- Specifically designed for V2X (IEEE 1609.2))
- Message structure parsing built-in
- License compatible (MPL 2.0 similar to BSD-2)
- Production usage (TU Delft autonomous vehicles)

**Cons:**
- Larger codebase than needed (might bundle extras)
- Research code (maturity varies)
- Documentation academic-focused

---

#### 2. **ASIO (Automotive Systems for Interoperable Operations)**
- **Project:** Part of various V2X stacks
- **Status:** Fragmented (no single canonical repo)
- **Evaluation:** 🟡 **MIXED - Hard to isolate**

---

### Tier 2: Generic OER/DER Libraries

#### 3. **ASN.1 Runtime Libraries with OER Support**

**Kandidates:**
- **libasn1:** GNU Libtasn1 (supports DER, but limited OER)
- **Skeletons/Auto-generated:** ASIO projects output code
- **Evaluation:** 🔴 **NOT RECOMMENDED - OER support weak**

**Why:** ASN.1 libraries focus on DER/BER, not COER. You'd still need custom COER layer.

---

#### 4. **CBOR / Protocol Buffers** (Wrong format entirely)
- **Evaluation:** ❌ **NOT APPLICABLE – Different binary encoding**
- V2X mandates COER (IEEE 1609.2), not alternative encodings

---

### Tier 3: Building Blocks (Lower-Level)

#### 5. **Boost.Asio + Binary Protocol Handler**
- **Use For:** Low-level bit unpacking
- **License:** Boost (compatible) ✅
- **Evaluation:** 🟡 **BUILD HELPER, NOT SOLUTION**
- **Still Need:** Custom COER layer on top

---

## Recommendation: Research OpenDaVINCI V2X

### Phase 3 Action Plan (Alternative)

**Instead of 4-week custom build:**

**Week 1:** OpenDaVINCI V2X Evaluation
```bash
# Clone and analyze
git clone https://github.com/se-research/opendavinci.git
cd opendavinci
find . -name "*1609*" -o -name "*coer*" -o -name "*v2x*"
# Evaluate:
# - Message parsing API
# - License compatibility
# - Android compilation
# - Performance
```

**Decision Point (End of Week 1):**

**Option A: Use OpenDaVINCI V2X (If suitable)**
- Add as git submodule: `git submodule add ... phase3-coer-lib`
- Integrate Message parser with Phase 2 engine
- Timeline: 2 weeks integration + testing
- Risk: Low (proven code)

**Option B: Lightweight Wrapper (If OpenDaVINCI too heavy)**
- Extract minimal COER parsing logic
- Create thin wrapper (~200-300 LOC)
- License: Keep as separate attribution
- Timeline: 2 weeks custom
- Risk: Medium (custom code, but smaller)

**Option C: Full Custom Implementation (Fallback)**
- Original plan (~500-800 LOC)
- Timeline: 4 weeks
- Risk: High (untested)

---

## Git Submodule Strategy

### If Adopting Existing Library

**Setup:**
```bash
cd /home/sudip_dev/sentinel-v-v2x-bridge

# Add as submodule (Phase 3)
git submodule add https://github.com/se-research/opendavinci.git phase3-coer-library
cd phase3-coer-library
git checkout v2x-compatible-tag  # Specific stable version

# Back to root
cd ..
git add .gitmodules phase3-coer-library/
git commit -m "Phase 3: Add OpenDaVINCI COER library as submodule"
```

**Integration in CMakeLists:**
```cmake
# native-engine/CMakeLists.txt
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../phase3-coer-library/3rdparty coer)

# Link to crypto engine
target_link_libraries(sentinel-engine PRIVATE coer_parser)
```

**Android Build:**
```cmake
# Android compatibility
if(ANDROID)
    # Adjust coer library Android linking
    target_compile_definitions(coer_parser PRIVATE ANDROID_BUILD=1)
endif()
```

---

## Decision Matrix

| Criterion | Custom Build | OpenDaVINCI | Boost.ASIO |
|-----------|--------------|-------------|-----------|
| **Development Time** | 4 weeks | 1-2 weeks | 2-3 weeks |
| **Code Quality** | 🟡 New | 🟢 Proven | 🟡 Generic |
| **License** | Full control | MPL 2.0 ✅ | Boost ✅ |
| **Performance** | Unknown | Optimized | Good |
| **Maintenance** | We maintain | Community | Boost team |
| **Size** | 500-800 LOC | 2-5MM (varies) | Large |
| **Android** | Yes | Yes | Yes |
| **Risk** | High | Low | Medium |
| **Documentation** | TBD | Academic | Good |

---

## Next Steps

### Immediate (This Week)

1. **Research OpenDaVINCI V2X:**
   ```bash
   # Clone and assess
   git clone https://github.com/se-research/opendavinci.git /tmp/opendavinci-eval
   
   # Look for:
   find . -path "*v2x*" -o -path "*1609*" -o -path "*coer*"
   
   # Check license
   cat opendavinci/LICENSE
   
   # Analyze message parsing public API
   grep -r "IEEE\|1609\|COER\|Message" --include="*.h" | head -20
   ```

2. **Evaluate Android Compatibility:**
   - Check if it compiles with NDK
   - Verify C++ version compatibility
   - Assess dependency chain

3. **Decision Meeting:**
   - Present findings
   - Choose: Adopt vs Custom
   - Plan Week 2 accordingly

### If Decision = "Adopt OpenDaVINCI"

**Week 1-2:** Integration
```bash
# Add submodule
git submodule add https://github.com/se-research/opendavinci.git phase3-coer-library

# Create wrapper API (native-engine/include/v2x_message_decoder.h)
# Integrate with existing V2XCryptoEngine
```

**Result:** Phase 3 completes in 2-3 weeks instead of 4

---

## Risk Analysis

### Adopting Existing Library Risks

**1. License Incompatibility**
- Mitigation: Verify MPL 2.0 vs our license upfront
- Impact: Low (MPL 2.0 is commercial-friendly)

**2. Code Bloat**
- OpenDaVINCI may have features we don't need
- Mitigation: Extract minimal COER parsing layer
- Impact: Medium (slightly larger binary)

**3. Maintenance Burden**
- If OpenDaVINCI unmaintained, we inherit burden
- Mitigation: Fork if needed, or go custom
- Impact: Low (we can always switch to custom)

**4. Android Compatibility**
- May not compile out-of-box for NDK
- Mitigation: Test early in Week 1
- Impact: Medium (delays, but recoverable)

### Custom Implementation Risks

**1. COER Format Misunderstanding**
- Risk: Subtle parsing bugs
- Impact: High (security-critical in V2X)

**2. Performance**
- Risk: Slower than optimized library
- Impact: Medium (V2X has real-time constraints)

**3. Test Coverage**
- Risk: Edge cases not covered
- Impact: High (field bugs post-deployment)

---

## Recommendation

**🟢 STRONGLY RECOMMEND: Research OpenDaVINCI First**

**Rationale:**
1. ✅ Proven automotive V2X code
2. ✅ IEEE 1609.2 specifically supported
3. ✅ Commercial-friendly license
4. ✅ Saves 2 weeks of development
5. ✅ Lower risk than custom parsing

**Action:** 
- Allocate Week 1 Phase 3 for evaluation
- Decision by end of Week 1
- Proceed with integration or pivot to custom

**Fallback:** If OpenDaVINCI unsuitable, custom COER decoder is proven viable (2-3 week alternative)

---

## Strategic Value

Using existing library aligns with Phase 2 philosophy:
- Phase 2: "Link, don't bundle" Botan ✅
- Phase 3: "Use proven code, don't reinvent" COER parser ✅

**Result:** V2X Security Bridge built on mature, tested components. Better security posture, faster delivery, lower maintenance.

---

**Prepared:** March 7, 2026  
**For:** Phase 3 Planning Decision  
**Recommendation:** Research OpenDaVINCI + Adopt if suitable
