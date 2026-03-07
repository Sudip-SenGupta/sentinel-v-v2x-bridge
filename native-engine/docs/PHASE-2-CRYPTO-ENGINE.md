# Phase 2: V2X Cryptographic Engine - Botan Integration

This phase implements the cryptographic security layer for V2X message validation using Botan library.

## Overview

The V2X Crypto Engine provides:
- **ECDSA Signature Verification** (NIST P-256, SHA-256)
- **X.509 Certificate Parsing** (DER format)
- **Certificate Chain Validation** (expiration, signature verification)
- **SHA-256 Message Hashing**
- **IEEE 1609.2 Message Parsing** (preliminary)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ Android App (Kotlin JNI Interface)                          │
│ SecurityEngine.kt ↔ SecurityEngine.cpp                      │
└────────────────────────┬────────────────────────────────────┘
                         │ JNI Calls
                         ↓
        ┌────────────────────────────────────┐
        │ native-engine/src/                 │
        │ V2XCryptoEngine.cpp (Main Engine)  │
        │ ├─ ECDSA Signature Verification    │
        │ ├─ SHA-256 Hashing                 │
        │ ├─ X.509 Certificate Parsing       │
        │ └─ Certificate Chain Validation    │
        └────────────┬───────────────────────┘
                     │ Links Against
                     ↓
        ┌────────────────────────────────────┐
        │ Botan Cryptographic Library        │
        │ ├─ Elliptic Curve Operations       │
        │ ├─ Hash Functions (SHA-256)        │
        │ ├─ X.509 Certificate Handling      │
        │ ├─ ASN.1 DER Encoding/Decoding    │
        │ └─ Random Number Generation        │
        └────────────────────────────────────┘
```

## Project Structure

```
native-engine/
├── CMakeLists.txt              ← Botan configuration for crypto library
├── include/
│   └── v2x_crypto_engine.h     ← Main crypto engine interface (165 LOC)
├── src/
│   └── v2x_crypto_engine.cpp   ← Full implementation with Botan (320 LOC)
└── tests/
    └── (TBD)                   ← Unit test suite
```

## Files in Phase 2

### `include/v2x_crypto_engine.h` (165 lines)

**Public Interface:**
- `struct CertificateInfo` - Parsed certificate details
- `struct SignatureVerificationResult` - Verification results
- `class V2XCryptoEngine` - Main crypto engine with 10 public methods

**Key Methods:**
1. `initialize_with_root_ca()` - Load trusted root CA
2. `verify_ecdsa_signature()` - Verify P-256 ECDSA signatures
3. `sha256_hash()` - Compute SHA-256 hashes
4. `parse_certificate()` - Parse X.509 certificates
5. `validate_certificate_chain()` - Full chain validation
6. `parse_ieee1609_message()` - IEEE 1609.2 message parsing
7. `extract_sender_info()` - Extract certificate subject
8. `is_certificate_valid()` - Check expiration status
9. `cleanup()` - Release resources
10. `get_botan_version()` - Version information

### `src/v2x_crypto_engine.cpp` (320 lines)

**Implementation:**
- Pimpl pattern for clean Botan isolation
- Complete ECDSA(SHA-256) signature verification
- X.509 certificate parsing and validation
- SHA-256 hash computation
- Certificate expiration checking
- Android logging integration
- Exception handling and error reporting

**Botan Modules Used:**
- `botan/pubkey.h` - Public key cryptography
- `botan/ecdsa.h` - ECDSA signatures
- `botan/x509cert.h` - X.509 certificates
- `botan/hash.h` - Hash functions
- `botan/der_*.h` - DER encoding/decoding
- `botan/auto_rng.h` - Random number generation

## Building with Botan

### Prerequisites

1. **Install Botan Development Headers:**
```bash
# Ubuntu/Debian
sudo apt-get install libbotan-2-dev

# Or build from source (recommended for Android)
git clone https://github.com/randombit/botan.git
cd botan
```

2. **For Android NDK Integration:**
```bash
# Build Botan for arm64 (used by NDK 25)
python3 configure.py \
  --cc=clang \
  --os=linux \
  --cpu=arm64 \
  --prefix=$HOME/Android/Sdk/botan/arm64 \
  --enable-modules=ecdsa,ecdh,sha2_32,sha2_64,x509,asn1,pubkey,auto_rng

make -j4 && make install
```

3. **CMake FindBotan Module:**
```bash
# Place in /usr/share/cmake-3.22/Modules/ or set CMake path
# Or use environment variable:
export CMAKE_PREFIX_PATH=/path/to/botan/installation
```

### CMakeLists.txt Configuration

The `native-engine/CMakeLists.txt` includes:
- Botan library detection via `find_package(Botan REQUIRED)`
- Support for prebuilt Android binaries
- Multi-architecture builds (arm64-v8a, x86_64)
- Android NDK integration
- Compiler optimization flags

### Build Commands

```bash
# Build native-engine (standalone)
cd /home/sudip_dev/sentinel-v-v2x-bridge/native-engine
mkdir build
cd build
cmake ..
make -j4

# Build everything (including JNI)
cd /home/sudip_dev/sentinel-v-v2x-bridge
export ANDROID_HOME="/mnt/c/Users/SenGuptaSudip/AppData/Local/Android/Sdk"
/tmp/gradle-8.2/bin/gradle build
```

## API Documentation

### Signature Verification

```cpp
// Load public key from certificate
std::vector<uint8_t> public_key = ...;

// Verify ECDSA(SHA-256) signature
auto result = engine.verify_ecdsa_signature(message, signature, public_key);

if (result.valid) {
    std::cout << "Signature is valid!" << std::endl;
    std::cout << "Algorithm: " << result.algorithm << std::endl;
    std::cout << "Time: " << result.verification_time_ms << "ms" << std::endl;
} else {
    std::cerr << "Invalid: " << result.error_message << std::endl;
}
```

### Certificate Chain Validation

```cpp
// Load certificate chain (leaf → intermediate → root)
std::vector<std::vector<uint8_t>> chain = { leaf_cert, intermediate_cert };

// Validate entire chain
bool valid = engine.validate_certificate_chain(chain);

if (valid) {
    std::cout << "Certificate chain is valid and trusted" << std::endl;
}
```

### SHA-256 Hashing

```cpp
// Hash a message
std::vector<uint8_t> hash = engine.sha256_hash(message);

// Or get hex encoding directly
std::string hex_hash = V2XCryptoEngine::sha256_hex(message);
std::cout << "SHA-256: " << hex_hash << std::endl;
```

## Integration with JNI

The SecurityEngine.cpp JNI wrapper calls these methods:

```kotlin
// Kotlin (app module)
class SecurityEngine {
    external fun verifyPacket(messageData: ByteArray, signature: ByteArray, 
                             certChain: Array<ByteArray>): Boolean
    external fun validateCertificateChain(certs: Array<ByteArray>): Boolean
    external fun extractSenderInfo(certificate: ByteArray): String
    // ...
}
```

```cpp
// C++ JNI Bridge
JNIEXPORT jboolean JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_verifyPacket(...) {
    V2XCryptoEngine engine;
    return engine.verify_ecdsa_signature(...).valid;
}
```

## Testing Strategy

### Unit Tests (native-engine/tests/)
- ECDSA signature verification with known test vectors
- X.509 certificate parsing and validation
- SHA-256 hash computation verification
- Certificate chain validation tests

### Integration Tests (app module)
- End-to-end V2X message validation
- Multiple certificate chain depths
- Expired certificate handling
- Malformed message handling

## Known Limitations & TODOs

1. **IEEE 1609.2 Full Parsing** - Currently stubbed, needs ASN.1 schema
2. **Revocation Lists** - CRL support not yet implemented
3. **OCSP Stapling** - Not yet supported
4. **Multiple Signature Algorithms** - Currently ECDSA only
5. **Hardware Security Module** - HSM support not implemented

## Performance Characteristics

Based on Botan benchmarks:
- ECDSA P-256 Signature Verification: ~2-5ms
- SHA-256 Hash (1KB message): <1ms
- X.509 Certificate Parsing: ~1-2ms
- Full Chain Validation (3 certs): ~5-10ms

## Security Considerations

1. **Constant-Time Operations** - Botan uses constant-time implementations for crypto
2. **RNG Quality** - AutoSeeded_RNG uses system entropy (/dev/urandom)
3. **Memory Clearing** - No sensitive data retained after operations
4. **Exception Safety** - All methods provide strong exception guarantees

## References

- [IEEE 1609.2-2016](https://standards.ieee.org/standard/1609_2-2016.html) - V2X Security Standard
- [Botan Documentation](https://botan.randombit.net/) - Cryptographic Library
- [NIST P-256](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf) - Digital Signature Algorithm
- [RFC 5280](https://tools.ietf.org/html/rfc5280) - X.509 Certificates

---

**Status**: Phase 2 (Crypto Engine) - In Progress  
**Target Completion**: 2-3 days  
**Next Phase**: Phase 3 (IEEE 1609.2 Message Parsing)
