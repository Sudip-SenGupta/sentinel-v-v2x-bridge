/**
 * @file test_vectors.h
 * @brief IEEE 1609.2 COER Test Vectors for V2X Message Parsing
 *
 * This file contains realistic V2X message test vectors in COER format.
 * Each vector includes:
 *   - Hex-encoded raw message
 *   - Expected parse results
 *   - Message structure commentary
 *   - Phase 2 integration expectations
 *
 * Test vectors cover:
 *   1. Unsigned messages (no signature)
 *   2. Signed messages with ECDSA P-256
 *   3. Messages with certificate chains
 *   4. Edge cases (minimal, maximum size)
 *   5. Malformed messages (for error testing)
 *
 * @author Sentinel V2X Bridge
 * @date March 7, 2026
 * @version 1.0.0
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>

namespace sentinel::v2x::test {

// ============================================================================
// Test Vector 1: Minimal Unsigned Message
// ============================================================================
/**
 * @brief UNSIGNED_MINIMAL - Simplest valid V2X message
 *
 * Structure:
 *   [Header Byte]        : 0x03 (protocol version 3, no signature)
 *   [Payload Length]     : 0x000F (15 bytes big-endian)
 *   [Payload]            : Minimal BSM (Basic Safety Message)
 *                          - Timestamp: 1706659200 (Jan 31, 2024)
 *                          - Vehicle Position: Empty (null coordinates)
 *                          - Speed: 25 m/s
 *
 * Total Size           : 18 bytes
 * Expected Parse       : message_type=0x03, is_signed()=false, payload_size=15
 * Phase 2 Usage        : Not applicable (unsigned)
 *
 * Format:
 *   Byte 0:   0x03      = Protocol version 3, message type unsigned
 *   Byte 1-2: 0x000F    = Payload length (big-endian): 15 bytes
 *   Byte 3-17: Payload  = Minimal V2X BSM data
 *
 * @see https://standards.ieee.org/ieee/1609.2/7122/ (IEEE 1609.2-2016 Section 4.2)
 */
constexpr const char* UNSIGNED_MINIMAL_HEX =
    "30"        /* Header: version 3, unsigned */
    "0F"        /* Payload length (varint): 15 bytes */
    "010203040506070809"  /* 9 bytes */
    "0A0B0C0D0E0F";  /* 6 bytes = 15 total */

constexpr uint8_t UNSIGNED_MINIMAL_EXPECTED_MESSAGE_TYPE = 0x30;
constexpr bool UNSIGNED_MINIMAL_EXPECTED_IS_SIGNED = false;
constexpr size_t UNSIGNED_MINIMAL_EXPECTED_PAYLOAD_SIZE = 15;
constexpr const char* UNSIGNED_MINIMAL_DESCRIPTION =
    "Minimal unsigned V2X message for parser validation.\n"
    "  - No cryptographic protection\n"
    "  - Simple payload structure\n"
    "  - Suitable for format/length validation tests\n";

// ============================================================================
// Test Vector 2: Signed Message with Certificate (Typical BSM)
// ============================================================================
/**
 * @brief SIGNED_TYPICAL_BSM - Standard signed V2X message with issuer cert
 *
 * Simulates a real "Cooperative Awareness Message" (CAM) from ETSI standards,
 * adapted to IEEE 1609.2 format. This represents a typical highway vehicle
 * broadcasting its position, speed, and heading every 100ms.
 *
 * Structure:
 *   [Header]              : 0x02 (signed, version 3)
 *   [Payload Length]      : 0x005A (90 bytes)
 *   [Payload]             : Realistic CAM-like BSM
 *                           - Timestamp: 1706659225000 ms (Jan 31 2024 10:00:25)
 *                           - Position: Lat 37.7749, Lon -122.4194 (San Francisco)
 *                           - Speed: 65 km/h (18.06 m/s)
 *                           - Heading: 45 degrees
 *   [Signature Algorithm] : 0x04 (ECDSA P-256)
 *   [Signature]           : 64 bytes (ECDSA r || s)
 *   [Issuer Certificate]  : DER-encoded X.509 v3 (typical ~600 bytes)
 *
 * Total Size            : ~760 bytes
 * Expected Parse        : message_type=0x02, is_signed()=true, signatures present
 * Phase 2 Integration   : Feed to verify_ecdsa_signature() with payload + sig + cert
 *
 * Note: This uses placeholder certificate data. Real certificates would come
 * from PKI (Public Key Infrastructure) operated by transportation authorities.
 *
 * @see IEEE 1609.2-2016 Section 4.4 (Signed Message Structure)
 * @see IEEE 1609.1 Section 5 (CAM/BSM Definition)
 */
constexpr const char* SIGNED_TYPICAL_BSM_HEX =
    "32"        /* Header: version 3, signed */
    "20"        /* Payload length (varint): 32 bytes */
    "0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20"  /* 32 bytes payload */
    "04"        /* Signature algorithm: ECDSA P-256 */
    "40"        /* Signature length (varint): 64 bytes */
    "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2"
    "C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4"
    "20"        /* Issuer cert length (varint): 32 bytes */
    "3020000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D";

constexpr uint8_t SIGNED_TYPICAL_BSM_EXPECTED_MESSAGE_TYPE = 0x32;
constexpr bool SIGNED_TYPICAL_BSM_EXPECTED_IS_SIGNED = true;
constexpr size_t SIGNED_TYPICAL_BSM_EXPECTED_PAYLOAD_SIZE = 32;  /* Simplified to 32 bytes */
constexpr size_t SIGNED_TYPICAL_BSM_EXPECTED_SIGNATURE_SIZE = 64;
constexpr const char* SIGNED_TYPICAL_BSM_DESCRIPTION =
    "Typical signed V2X message (CAM/BSM format - simplified).\n"
    "  - ECDSA P-256 signature\n"
    "  - X.509 v3 issuer certificate\n"
    "  - Simplified 32-byte payload for testing\n"
    "  - Phase 2 integration: verify_ecdsa_signature()\n"
    "  - Total size: ~160 bytes (simplified)\n";

// ============================================================================
// Test Vector 3: Signed Message with Certificate Chain
// ============================================================================
/**
 * @brief SIGNED_WITH_CHAIN - Message with certificate hierarchy
 *
 * Demonstrates full chain-of-trust validation scenario. This message includes
 * multiple certificates representing the certification hierarchy:
 *   - Issuer Cert: Vehicle OEM certificate (e.g., Tesla, BMW)
 *   - Chain[0]: OEM's issuer (e.g., Regional Certificate Authority)
 *   - Chain[1]: Root CA (e.g., National Transportation Authority)
 *
 * This enables Phase 2 to validate the entire chain:
 *   1. Verify message signature with issuer cert's public key
 *   2. Verify issuer cert was signed by Chain[0]
 *   3. Verify Chain[0] was signed by Chain[1] (root)
 *   4. Verify Chain[1] is trusted root
 *
 * Structure:
 *   [Header]              : 0x02 (signed)
 *   [Payload Length]      : 0x0050 (80 bytes)
 *   [Payload]             : V2X message data
 *   [Signature Algorithm] : 0x04 (ECDSA P-256)
 *   [Signature]           : 64 bytes
 *   [Issuer Certificate]  : DER X.509 (~600 bytes)
 *   [Chain Depth]         : 0x02 (2 additional certificates)
 *   [Chain[0]]            : Intermediate CA (~600 bytes)
 *   [Chain[1]]            : Root CA (~600 bytes)
 *
 * Total Size            : ~1900 bytes
 * Expected Parse        : is_signed()=true, chain_depth()=2
 * Phase 2 Integration   : validate_certificate_chain() with all 3 certs
 *
 * Note: Uses placeholder data structure. Real chains would be PEM/DER-encoded
 * X.509 certificates from actual transportation PKI.
 *
 * @see IEEE 1609.2-2016 Section 4.5 (Certificate Chain)
 * @see RFC 5280 (X.509 Certificate Format)
 */
constexpr const char* SIGNED_WITH_CHAIN_HEX =
    "32"        /* Header: version 3, signed */
    "10"        /* Payload length (varint): 16 bytes */
    "0102030405060708090A0B0C0D0E0F10"  /* 16 bytes payload */
    "04"        /* Signature algorithm */
    "40"        /* Signature length (varint): 64 bytes */
    "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2"
    "C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4"
    "10"        /* Issuer cert length (varint): 16 bytes */
    "3020000102030405060708090A0B0C0D"  /* 16 bytes issuer cert */
    "02"        /* Chain depth: 2 certificates */
    "10"        /* Chain[0] length (varint): 16 bytes */
    "300102030405060708090A0B0C0D0E0F"  /* 16 bytes chain[0], starts with 0x30 */
    "10"        /* Chain[1] length (varint): 16 bytes */
    "3010111213141516171819191A1B1C1D";

constexpr uint8_t SIGNED_WITH_CHAIN_EXPECTED_MESSAGE_TYPE = 0x32;
constexpr bool SIGNED_WITH_CHAIN_EXPECTED_IS_SIGNED = true;
constexpr size_t SIGNED_WITH_CHAIN_EXPECTED_PAYLOAD_SIZE = 16;  /* Simplified */
constexpr size_t SIGNED_WITH_CHAIN_EXPECTED_CHAIN_DEPTH = 2;
constexpr const char* SIGNED_WITH_CHAIN_DESCRIPTION =
    "Signed message with certificate chain (3 certificates - simplified).\n"
    "  - Message signature from issuer cert\n"
    "  - Chain[0]: Intermediate Certificate Authority\n"
    "  - Chain[1]: Root Certificate Authority\n"
    "  - Phase 2 integration: validate_certificate_chain()\n"
    "  - Total size: ~200 bytes (simplified)\n";

// ============================================================================
// Test Vector 4: Emergency Alert (Signed, High Priority)
// ============================================================================
/**
 * @brief SIGNED_EMERGENCY_ALERT - Emergency warning message
 *
 * Represents an emergency alert message (e.g., "Hard Braking", "Road Hazard").
 * These messages require signature validation due to potential safety impact.
 * Strict rate limiting and ROI validation would be applied by receivers.
 *
 * Structure similar to SIGNED_TYPICAL_BSM but with:
 *   - Different payload structure (warning flags set)
 *   - Potentially shorter latency requirement
 *   - May have higher certificate trust requirements
 *
 * Message payload flags:
 *   - Bit 0: Hard Braking Alert (1 = yes)
 *   - Bit 1: Pedestrian in Roadway (1 = yes)
 *   - Bit 2: Reduced Visibility (1 = yes)
 *   - Bit 3: Disabled Vehicle (1 = yes)
 *
 * Real-world example: Vehicle detects hard braking, broadcasts to nearby
 * vehicles within 300 meter radius to warn of sudden traffic slowdown.
 *
 * @see SAE J3070 (Cooperative Adaptive Cruise Control)
 * @see DSRC V2X Safety Use Cases
 */
constexpr const char* SIGNED_EMERGENCY_ALERT_HEX =
    "32"        /* Header: version 3, signed */
    "20"        /* Payload length (varint): 32 bytes */
    "0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20"  /* 32 bytes payload */
    "04"        /* Signature algorithm: ECDSA P-256 */
    "40"        /* Signature length (varint): 64 bytes */
    "B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3"
    "D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5"
    "20"        /* Certificate length (varint): 32 bytes */
    "3020000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D";

constexpr uint8_t SIGNED_EMERGENCY_ALERT_EXPECTED_MESSAGE_TYPE = 0x32;
constexpr bool SIGNED_EMERGENCY_ALERT_EXPECTED_IS_SIGNED = true;
constexpr size_t SIGNED_EMERGENCY_ALERT_EXPECTED_PAYLOAD_SIZE = 32;
constexpr const char* SIGNED_EMERGENCY_ALERT_DESCRIPTION =
    "Emergency alert message (e.g., hard braking warning).\n"
    "  - Smaller payload (32 bytes vs 90+ for regular BSM)\n"
    "  - Signature required for safety critical alerts\n"
    "  - Typical latency: <50 ms\n"
    "  - Broadcast range: ~300 meters\n";

// ============================================================================
// Test Vector 5: Maximum Size Message (Fragment of Large Dataset)
// ============================================================================
/**
 * @brief SIGNED_MAXIMUM_SIZE - Larger message with extended payload
 *
 * Tests parser robustness with larger messages. Real ISO 21520 (V2X comms)
 * limits messages to ~2048 bytes due to DSRC bandwidth constraints.
 *
 * This message represents rich telemetry:
 *   - Extended vehicle state (all sensors)
 *   - Weather data (if vehicle has environmental sensors)
 *   - Road condition warnings
 *   - Multiple waypoints or trajectory data
 *
 * Total Size: ~1000 bytes
 * This tests:
 *   - Parser performance with larger data
 *   - Buffer management in COERDecoder
 *   - Realistic DSRC V2X message sizes
 *
 * @see ISO 21520 Section 4 (Message Size Limits)
 */
constexpr const char* SIGNED_MAXIMUM_SIZE_HEX =
    "32"        /* Header: version 3, signed */
    "820110"    /* Payload length (varint): 272 bytes (0x82=2-byte-follows, 0x0110=272) */
    /* Extended payload (272 bytes) */
    "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"
    "202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F"
    "404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F"
    "606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F"
    "808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9F"
    "A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
    "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
    "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"
    "A0A1A2A3A4A5A6A7A8A9AAABACADAEB0"  /* +16 bytes = 272 total */
    "04"        /* Signature algorithm: ECDSA P-256 */
    "40"        /* Signature length (varint): 64 bytes */
    /* ECDSA Signature (64 bytes) */
    "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2"
    "C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4"
    "20"        /* Certificate length (varint): 32 bytes */
    /* Issuer Certificate (32 bytes) */
    "3020000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D";

constexpr uint8_t SIGNED_MAXIMUM_SIZE_EXPECTED_MESSAGE_TYPE = 0x32;
constexpr bool SIGNED_MAXIMUM_SIZE_EXPECTED_IS_SIGNED = true;
constexpr size_t SIGNED_MAXIMUM_SIZE_EXPECTED_PAYLOAD_SIZE = 272;
constexpr const char* SIGNED_MAXIMUM_SIZE_DESCRIPTION =
    "Larger message with extended telemetry data.\n"
    "  - Payload: 256 bytes (near DSRC frame limit)\n"
    "  - Tests parser performance with large messages\n"
    "  - Representative of rich sensor fusion data\n"
    "  - Total size: ~360 bytes\n";

// ============================================================================
// Test Vector 6-10: Malformed Messages (Error Cases)
// ============================================================================

/**
 * @brief MALFORMED_TRUNCATED - Message cut short (buffer overflow risk)
 *
 * Simulate network packet loss: message arrives truncated at receiver.
 * Parser must detect this and throw COERBufferException.
 *
 * Structure:
 *   [Header]         : 0x02 (signed)
 *   [Payload Length] : 0x0100 (256 bytes expected)
 *   [Actual Data]    : Only 32 bytes present (224 bytes missing)
 *
 * Expected Result  : COERBufferException thrown
 * Error Message    : "Buffer underflow: expected 256 bytes, got only 32"
 */
constexpr const char* MALFORMED_TRUNCATED_HEX =
    "32"        /* Header: version 3, signed (per IEEE 1609.2 standard) */
    "8201"      /* Payload length (varint): 256 bytes expected (0x82=2-byte-follows, 0x0100=256) */
    "000102030405060708091011"
    "1213141516171819202122"
    "2324252627282930311111";  /* Only 32 bytes - TRUNCATED */

constexpr const char* MALFORMED_TRUNCATED_DESCRIPTION =
    "Truncated message (packet loss scenario).\n"
    "  - Payload length field uses COER varint encoding\n"
    "  - 0x8201 = 2-byte varint encoding of 256\n"
    "  - Expected Exception: COERBufferException\n"
    "  - Error: Buffer underflow\n";

/**
 * @brief MALFORMED_INVALID_VERSION - Unsupported protocol version
 *
 * Message claims to be protocol version 7 (doesn't exist yet).
 * Parser must reject unknown versions for forward compatibility.
 *
 * Structure:
 *   [Header] : 0x73 (version bits = 0x07 = version 7, invalid)
 *
 * Expected Result  : COERFormatException thrown
 * Error Message    : "Unsupported protocol version: 7"
 */
constexpr const char* MALFORMED_INVALID_VERSION_HEX =
    "72"        /* Header: version 7 (invalid), signed */
    "0F"        /* Payload length (varint): 15 bytes */
    "000102030405060708091011121314";

constexpr const char* MALFORMED_INVALID_VERSION_DESCRIPTION =
    "Invalid unsupported protocol version (7).\n"
    "  - Header version bits: 0x07 (only 0-3 valid)\n"
    "  - Expected Exception: COERFormatException\n"
    "  - Error: Unsupported protocol version\n";

/**
 * @brief MALFORMED_EMPTY_MESSAGE - Zero-length message
 *
 * Attempt to parse completely empty message.
 *
 * Expected Result  : COERBufferException thrown
 * Error Message    : "Message too short: 0 bytes (minimum 3)"
 */
constexpr const char* MALFORMED_EMPTY_MESSAGE_HEX = "";

constexpr const char* MALFORMED_EMPTY_MESSAGE_DESCRIPTION =
    "Empty message (zero bytes).\n"
    "  - Expected minimum: 3 bytes (header + length)\n"
    "  - Expected Exception: COERBufferException\n"
    "  - Error: Message too short\n";

/**
 * @brief MALFORMED_SHORT_HEADER - Only 2 bytes (incomplete header+length)
 *
 * Minimum header is 3 bytes (1 header + 2 length bytes).
 * This message is only 2 bytes.
 *
 * Expected Result  : COERBufferException thrown
 */
constexpr const char* MALFORMED_SHORT_HEADER_HEX =
    "32"        /* Header: version 3, signed */
    "03";  /* Only 1 byte for length, but varint still incomplete */

constexpr const char* MALFORMED_SHORT_HEADER_DESCRIPTION =
    "Incomplete header (needs more bytes for varint).\n"
    "  - Header version: 3, signed\n"
    "  - Payload length field: single byte 0x03 (not a complete varint if multi-byte intended)\n"
    "  - Expected Exception: COERBufferException\n";

/**
 * @brief MALFORMED_ZERO_PAYLOAD_SIGNED - Signed message with empty payload
 *
 * Message claims to be signed but has zero-length payload.
 * Semantically valid (odd but allowed), but important edge case.
 *
 * Should parse successfully but signature verification will fail
 * (no data to verify against signature).
 *
 * Expected Result  : Parses successfully
 * Expected Behavior: is_signed()=true, get_payload() returns empty vector
 * Note: Phase 2 will reject in verify_ecdsa_signature() (empty payload)
 */
constexpr const char* MALFORMED_ZERO_PAYLOAD_SIGNED_HEX =
    "32"        /* Header: version 3, signed */
    "00"        /* Payload length (varint): 0 bytes (unusual but valid) */
    "04"        /* Signature algorithm */
    "40"        /* Signature length (varint): 64 bytes */
    "A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2"
    "C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4E5F6A1B2C3D4"
    "10"        /* Certificate length (varint): 16 bytes */
    "3020" "00010203040506070809" "0A0B0C0D0E0F10111213";

constexpr const char* MALFORMED_ZERO_PAYLOAD_SIGNED_DESCRIPTION =
    "Signed message with zero-length payload (edge case).\n"
    "  - Valid format but unusual semantics\n"
    "  - Expected parse: OK (is_signed()=true)\n"
    "  - Phase 2 action: Will reject (empty payload unverifiable)\n";

// ============================================================================
// Test Vector Collection Container
// ============================================================================

/**
 * @struct TestVector
 * @brief Single test vector with metadata
 */
struct TestVector {
    std::string name;           ///< Identifier (e.g., "UNSIGNED_MINIMAL")
    std::string hex_data;       ///< Hex-encoded message
    std::string description;    ///< Human-readable description
    bool should_parse_ok;       ///< true = expect successful parse
    std::string error_type;     ///< Expected exception class (if should_parse_ok=false)
    std::string error_message;  ///< Expected error substring
};

/**
 * @brief Get all test vectors
 *
 * Returns collection of all test vectors organized by category.
 * Useful for parametrized testing frameworks.
 *
 * @return Vector of TestVector structures
 */
inline std::vector<TestVector> get_all_test_vectors() {
    return {
        // Valid messages
        {
            "UNSIGNED_MINIMAL",
            UNSIGNED_MINIMAL_HEX,
            UNSIGNED_MINIMAL_DESCRIPTION,
            true,
            "",
            ""
        },
        {
            "SIGNED_TYPICAL_BSM",
            SIGNED_TYPICAL_BSM_HEX,
            SIGNED_TYPICAL_BSM_DESCRIPTION,
            true,
            "",
            ""
        },
        {
            "SIGNED_WITH_CHAIN",
            SIGNED_WITH_CHAIN_HEX,
            SIGNED_WITH_CHAIN_DESCRIPTION,
            true,
            "",
            ""
        },
        {
            "SIGNED_EMERGENCY_ALERT",
            SIGNED_EMERGENCY_ALERT_HEX,
            SIGNED_EMERGENCY_ALERT_DESCRIPTION,
            true,
            "",
            ""
        },
        {
            "SIGNED_MAXIMUM_SIZE",
            SIGNED_MAXIMUM_SIZE_HEX,
            SIGNED_MAXIMUM_SIZE_DESCRIPTION,
            true,
            "",
            ""
        },
        
        // Malformed messages
        {
            "MALFORMED_TRUNCATED",
            MALFORMED_TRUNCATED_HEX,
            MALFORMED_TRUNCATED_DESCRIPTION,
            false,
            "COERBufferException",
            "Buffer underflow"
        },
        {
            "MALFORMED_INVALID_VERSION",
            MALFORMED_INVALID_VERSION_HEX,
            MALFORMED_INVALID_VERSION_DESCRIPTION,
            false,
            "COERFormatException",
            "protocol version"
        },
        {
            "MALFORMED_EMPTY_MESSAGE",
            MALFORMED_EMPTY_MESSAGE_HEX,
            MALFORMED_EMPTY_MESSAGE_DESCRIPTION,
            false,
            "COERBufferException",
            "too short"
        },
        {
            "MALFORMED_SHORT_HEADER",
            MALFORMED_SHORT_HEADER_HEX,
            MALFORMED_SHORT_HEADER_DESCRIPTION,
            false,
            "COERBufferException",
            "length"
        },
    };
}

/**
 * @brief Get only valid test vectors
 *
 * Useful for testing successful parsing paths.
 */
inline std::vector<TestVector> get_valid_test_vectors() {
    auto all = get_all_test_vectors();
    std::vector<TestVector> valid;
    std::copy_if(all.begin(), all.end(), std::back_inserter(valid),
        [](const TestVector& v) { return v.should_parse_ok; });
    return valid;
}

/**
 * @brief Get only malformed test vectors
 *
 * Useful for testing error handling paths.
 */
inline std::vector<TestVector> get_malformed_test_vectors() {
    auto all = get_all_test_vectors();
    std::vector<TestVector> malformed;
    std::copy_if(all.begin(), all.end(), std::back_inserter(malformed),
        [](const TestVector& v) { return !v.should_parse_ok; });
    return malformed;
}

/**
 * @brief Helper: Convert hex string to byte vector
 *
 * @param hex_string Hex string (e.g., "0102AABBCCDD")
 *        - Spaces and newlines ignored
 *        - Case-insensitive
 * 
 * @return std::vector<uint8_t> of decoded bytes
 * 
 * @throws std::invalid_argument if hex string contains invalid characters
 */
inline std::vector<uint8_t> hex_to_bytes(const std::string& hex_string) {
    std::vector<uint8_t> bytes;
    std::string cleaned;
    
    // Remove whitespace
    for (char c : hex_string) {
        if (!std::isspace(c)) {
            cleaned += c;
        }
    }
    
    // Validate even length
    if (cleaned.length() % 2 != 0) {
        throw std::invalid_argument("Hex string must have even number of characters");
    }
    
    // Convert pairs of hex digits to bytes
    for (size_t i = 0; i < cleaned.length(); i += 2) {
        std::string byte_str = cleaned.substr(i, 2);
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
            bytes.push_back(byte);
        } catch (const std::exception& e) {
            throw std::invalid_argument(
                std::string("Invalid hex digit at position ") + std::to_string(i) +
                ": '" + byte_str + "'"
            );
        }
    }
    
    return bytes;
}

}  // namespace sentinel::v2x::test
