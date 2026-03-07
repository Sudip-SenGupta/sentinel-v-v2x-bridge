/**
 * @file v2x_coer_decoder.h
 * @brief IEEE 1609.2 COER (Canonical Octet Encoding Rules) Message Decoder
 * 
 * This header defines the public API for parsing and extracting components from
 * IEEE 1609.2-2016 V2X messages encoded in COER format.
 * 
 * Phase 3 Delivery: Custom lightweight decoder for automotive V2X use cases.
 * 
 * Message Flow:
 *   Raw COER Message → parse() → COERMessage
 *                                    ├→ extract_signature() → Phase 2 verify_ecdsa_signature()
 *                                    ├→ extract_issuer_certificate() → Phase 2 validate_certificate_chain()
 *                                    └→ extract_certificate_chain() → Phase 2 chain validation
 * 
 * @author Sentinel V2X Bridge
 * @date March 7, 2026
 * @version 1.0.0
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

namespace sentinel::v2x {

// ============================================================================
// Forward Declarations
// ============================================================================

class COERDecoder;

// ============================================================================
// Constants
// ============================================================================

/** IEEE 1609.2 Protocol Version (2016) */
constexpr uint8_t COER_PROTOCOL_VERSION = 3;

/** Message Type Bits (IEEE 1609.2-2016 §4.2.1) */
enum class MessageType : uint8_t {
    UNSECURED              = 0x00,  /** No signature or encryption */
    SIGNED                 = 0x02,  /** Integrity protection (ECDSA signature) */
    ENCRYPTED_AND_SIGNED   = 0x06,  /** Confidentiality + Integrity */
    /* Additional types may be defined by IEEE 1609.2 extensions */
};

/** Signature Algorithm Identifiers (IEEE 1609.2) */
enum class SignatureAlgorithm : uint8_t {
    ECDSA_P256 = 0x04,    /** ECDSA with P-256 curve (most common in V2X) */
    ECDSA_P384 = 0x05,    /** ECDSA with P-384 curve */
    /* Additional algorithms reserved for future use */
};

/** Certificate Type Indicators */
enum class CertificateType : uint8_t {
    X509_V3                = 0x00,  /** Standard X.509 v3 (DER encoded) */
    IMPLICIT_CERT          = 0x01,  /** Implicit certificate (ECDSA specific) */
};

// ============================================================================
// Exception Classes
// ============================================================================

/**
 * @class COERDecodeException
 * @brief Exception thrown when COER message parsing fails
 */
class COERDecodeException : public std::runtime_error {
public:
    explicit COERDecodeException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @class COERBufferException
 * @brief Exception thrown for buffer overflow or underflow conditions
 */
class COERBufferException : public COERDecodeException {
public:
    explicit COERBufferException(const std::string& message)
        : COERDecodeException("Buffer error: " + message) {}
};

/**
 * @class COERFormatException
 * @brief Exception thrown for invalid COER message format
 */
class COERFormatException : public COERDecodeException {
public:
    explicit COERFormatException(const std::string& message)
        : COERDecodeException("Format error: " + message) {}
};

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @struct COERMessage
 * @brief Parsed representation of an IEEE 1609.2 COER message
 * 
 * Contains the extracted components of a V2X message including metadata,
 * payload data, and optional signature container with certificate chain.
 */
struct COERMessage {
    /**
     * @struct SignatureContainer
     * @brief Cryptographic signature and certificate information
     * 
     * Contains the ECDSA signature, issuer certificate (X.509), and optional
     * certificate chain for chain-of-trust validation.
     */
    struct SignatureContainer {
        /**
         * Signature algorithm identifier
         * @see SignatureAlgorithm enum
         */
        uint8_t signature_algorithm = 0;
        
        /**
         * ECDSA Signature bytes in raw format
         * 
         * For ECDSA P-256:
         *   - First 32 bytes: r component (big-endian)
         *   - Next 32 bytes: s component (big-endian)
         *   - Total: 64 bytes
         * 
         * This format matches IEEE 1609.2 COER encoding.
         */
        std::vector<uint8_t> signature;
        
        /**
         * Issuer Certificate in X.509 DER format
         * 
         * Compatible with Phase 2 V2XCryptoEngine:
         *   - Must be parseable by Botan X509::load_key()
         *   - Contains public key for signature verification
         *   - Must be valid at time of message verification
         */
        std::vector<uint8_t> issuer_cert;
        
        /**
         * Certificate chain for chain-of-trust validation
         * 
         * Each element is an X.509 certificate in DER format:
         *   - cert[0]: Issuer's issuer (one level up chain)
         *   - cert[1]: Issuer's issuer's issuer (two levels up)
         *   - ... up to root CA
         * 
         * Optional: May be empty if issuer_cert is directly issued by root CA.
         * 
         * @see COERDecoder::validate_structure() for chain depth limits
         */
        std::vector<std::vector<uint8_t>> cert_chain;
        
        /**
         * Check if certificate chain is present
         * 
         * @return true if chain contains at least one certificate
         */
        bool has_chain() const;
        
        /**
         * Get depth of certificate chain
         * 
         * @return Number of certificates in chain (0 if empty)
         */
        size_t chain_depth() const;
    };
    
    /**
     * Message type (enumerated in bits)
     * 
     * Encodes:
     *   - Bit 0: Protocol version (lower 4 bits: 0-15, typically 3 for 1609.2-2016)
     *   - Bit 1: Signature present (1 = signed)
     *   - Bit 2: Encryption present (1 = encrypted)
     * 
     * @see MessageType enum for common values
     */
    uint8_t message_type = 0;
    
    /**
     * IEEE 1609.2 Protocol Version
     * 
     * Current version: 3 (IEEE 1609.2-2016)
     * Typical range: 1-3 (forward compatibility recommended)
     */
    uint8_t protocol_version = 0;
    
    /**
     * Message payload (actual V2X data)
     * 
     * This is the data that was signed by the issuer. For SIGNED messages,
     * this exact payload must be used with extract_signature() and Phase 2's
     * verify_ecdsa_signature() for successful verification.
     * 
     * Contains:
     *   - Timestamp
     *   - Vehicle position (latitude, longitude, elevation)
     *   - Heading, speed, acceleration
     *   - Warnings (hard braking, hazard, etc.)
     *   - Other V2X specific telemetry
     */
    std::vector<uint8_t> payload;
    
    /**
     * Signature container (only present if is_signed() returns true)
     * 
     * For signed messages, contains the ECDSA signature, issuer certificate,
     * and optional certificate chain needed for authentication.
     */
    SignatureContainer signature_container;
    
    // ========================================================================
    // Query Methods
    // ========================================================================
    
    /**
     * Check if message is cryptographically signed
     * 
     * Examines the message_type to determine if a signature is present.
     * 
     * @return true if message contains ECDSA signature
     * @note If false, signature_container is not populated
     */
    bool is_signed() const;
    
    /**
     * Check if message is encrypted
     * 
     * Examines the message_type to determine if encryption is present.
     * 
     * @return true if message is encrypted (implies confidentiality protection)
     * @note Currently not fully supported; Phase 3 focus is on signed messages
     */
    bool is_encrypted() const;
    
    /**
     * Get total message size (including overhead)
     * 
     * Useful for logging and statistics.
     * 
     * @return Sum of header + payload + signature container sizes (approximate)
     */
    size_t total_size() const;
};

// ============================================================================
// Main Decoder Class
// ============================================================================

/**
 * @class COERDecoder
 * @brief IEEE 1609.2 COER Message Parser and Component Extractor
 * 
 * Provides static methods for:
 *   1. Parsing raw COER bytes into structured COERMessage
 *   2. Validating message structure (bounds, lengths, magic numbers)
 *   3. Extracting cryptographic components for Phase 2 verification
 * 
 * This is a lightweight, focused parser optimized for V2X automotive use cases.
 * It does NOT perform cryptographic verification (that's Phase 2's responsibility).
 * 
 * Thread-Safe: All methods are stateless and thread-safe (no global state).
 * 
 * @example
 * @code
 *   // Step 1: Parse raw V2X message
 *   std::vector<uint8_t> raw_message = receive_from_vehicle_network();
 *   COERMessage msg = COERDecoder::parse(raw_message);
 *   
 *   // Step 2: Validate structure (bounds, lengths)
 *   if (!COERDecoder::validate_structure(msg)) {
 *       LOGE("Invalid message structure");
 *       return false;
 *   }
 *   
 *   // Step 3: Extract components for Phase 2 verification
 *   auto signature = COERDecoder::extract_signature(msg);
 *   auto issuer_cert = COERDecoder::extract_issuer_certificate(msg);
 *   auto payload = COERDecoder::get_payload(msg);
 *   
 *   // Step 4: Feed to Phase 2 crypto engine
 *   V2XCryptoEngine engine;
 *   auto result = engine.verify_ecdsa_signature(payload, signature, issuer_cert);
 *   
 *   if (result.valid) {
 *       LOGI("Message authenticated!");
 *   }
 * @endcode
 */
class COERDecoder {
public:
    
    // ========================================================================
    // Core Parsing Methods
    // ========================================================================
    
    /**
     * Parse raw COER message bytes into structured message object
     * 
     * This is the main entry point. It performs:
     *   1. Header validation (version, type)
     *   2. Payload extraction (length-prefixed)
     *   3. Signature container parsing (if message is signed)
     * 
     * @param raw_message Raw bytes from vehicle network
     *        Expected format:
     *        [1 byte header]
     *        [2 bytes payload length (big-endian)]
     *        [N bytes payload]
     *        [M bytes signature container (if signed)]
     * 
     * @return Parsed COERMessage object with all components extracted
     * 
     * @throws COERFormatException if message format is invalid
     *         - Unsupported protocol version
     *         - Invalid message type
     *         - Reserved bits set (future compatibility)
     * 
     * @throws COERBufferException if message is too short or truncated
     *         - Less than 3 bytes
     *         - Payload length field exceeds actual message size
     *         - Signature container truncated
     * 
     * @complexity Time: O(n) where n = message size (single pass through data)
     * @complexity Space: O(n) for payload and signature container copies
     * 
     * @note This method does NOT verify cryptographic signatures. Use Phase 2's
     *       verify_ecdsa_signature() after extracting components.
     * 
     * @see validate_structure() - Recommended to call after parse() for extra validation
     */
    static COERMessage parse(const std::vector<uint8_t>& raw_message);
    
    // ========================================================================
    // Validation Methods
    // ========================================================================
    
    /**
     * Validate COER message structure (non-cryptographic checks)
     * 
     * Performs structural validation including:
     *   - Length field consistency checks
     *   - Signature container format validation
     *   - Certificate chain depth limits (DoS prevention)
     *   - Buffer boundary checks
     * 
     * This is a defense-in-depth validation. Most issues are caught during parse(),
     * but this provides additional safety checks and can be called independently.
     * 
     * @param message Parsed COERMessage object (from parse())
     * 
     * @return true if structure is valid and safe to use
     * @return false if structural anomalies detected (malicious or corrupted input)
     * 
     * @example
     * @code
     *   COERMessage msg = COERDecoder::parse(raw_data);
     *   if (!COERDecoder::validate_structure(msg)) {
     *       LOGE("Structure validation failed - rejecting message");
     *       return false;
     *   }
     * @endcode
     * 
     * @note Does NOT perform cryptographic validation. Safe to call on untrusted input.
     * @complexity Time: O(n) where n = total message size
     */
    static bool validate_structure(const COERMessage& message);
    
    // ========================================================================
    // Component Extraction Methods (for Phase 2 Integration)
    // ========================================================================
    
    /**
     * Extract ECDSA signature from parsed message
     * 
     * Returns the raw ECDSA signature bytes in the format they were encoded
     * in the COER message. This is ready to pass directly to Phase 2's
     * verify_ecdsa_signature() method.
     * 
     * For ECDSA P-256:
     *   - Returned bytes: r (32 bytes) || s (32 bytes) = 64 bytes total
     *   - Both components in big-endian format
     *   - Range: 0 < r,s < n (where n = order of P-256 curve)
     * 
     * @param message Parsed COERMessage (from parse())
     * 
     * @return Raw signature bytes in ECDSA format (r || s)
     * @return Can be passed directly to V2XCryptoEngine::verify_ecdsa_signature()
     * 
     * @throws COERFormatException if message is not signed
     *         - message.is_signed() returns false
     *         - signature_container.signature is empty
     * 
     * @throws COERFormatException if signature algorithm is not ECDSA P-256
     *         - Other algorithms not currently supported
     * 
     * @example
     * @code
     *   COERMessage msg = COERDecoder::parse(raw_data);
     *   auto sig = COERDecoder::extract_signature(msg);
     *   
     *   auto result = crypto_engine.verify_ecdsa_signature(
     *       payload,  // From get_payload()
     *       sig,      // Extracted signature
     *       issuer_cert  // From extract_issuer_certificate()
     *   );
     * @endcode
     * 
     * @complexity Time: O(1) - simple data reference
     * @complexity Space: O(k) where k = signature size (~64 bytes)
     */
    static std::vector<uint8_t> extract_signature(const COERMessage& message);
    
    /**
     * Extract issuer certificate from parsed message
     * 
     * Returns the X.509 certificate of the entity that signed the message,
     * in DER format. This certificate must be validated against a root CA
     * (typically through a certificate chain).
     * 
     * The certificate contains:
     *   - Public key (for signature verification)
     *   - Subject Name (issuer identifier)
     *   - Issuer Name (CA that signed this cert)
     *   - Serial number
     *   - Validity period (not-before, not-after)
     *   - Extensions (CRL distribution, key usage, etc.)
     * 
     * @param message Parsed COERMessage (from parse())
     * 
     * @return X.509 certificate in DER format
     * @return Format compatible with Botan's X509::load_key()
     * @return Can be passed directly to V2XCryptoEngine methods
     * 
     * @throws COERFormatException if message is not signed
     *         - message.is_signed() returns false
     * 
     * @throws COERFormatException if issuer certificate is missing or empty
     *         - Corrupted or incomplete COER message
     * 
     * @example
     * @code
     *   COERMessage msg = COERDecoder::parse(raw_data);
     *   auto issuer_cert = COERDecoder::extract_issuer_certificate(msg);
     *   
     *   auto result = crypto_engine.validate_certificate_chain(
     *       issuer_cert,  // Extracted issuer
     *       chain         // From extract_certificate_chain()
     *   );
     * @endcode
     * 
     * @complexity Time: O(1) - simple data reference
     * @complexity Space: O(k) where k = certificate size (typically 300-1500 bytes)
     */
    static std::vector<uint8_t> extract_issuer_certificate(const COERMessage& message);
    
    /**
     * Extract certificate chain from parsed message
     * 
     * Returns the chain of certificates from the issuer's issuer up to
     * (but typically not including) the root CA. These certificates form
     * the "chain of trust" needed to validate the issuer certificate.
     * 
     * Chain structure:
     *   - cert[0]: Issuer's issuer (immediate parent in hierarchy)
     *   - cert[1]: Issuer's issuer's issuer (grandparent)
     *   - ... etc ...
     *   - cert[n-1]: Typically a root CA or intermediate CA
     * 
     * Each certificate is in X.509 DER format, compatible with Botan.
     * 
     * @param message Parsed COERMessage (from parse())
     * 
     * @return Vector of X.509 certificates in DER format
     * @return Empty vector if no chain present (issuer may be direct child of root)
     * @return Can be passed to V2XCryptoEngine::validate_certificate_chain()
     * 
     * @throws COERFormatException if message is not signed
     *         - message.is_signed() returns false
     * 
     * @note This method does NOT fail if chain is empty. Empty chain is valid
     *       if issuer_cert is directly issued by root CA.
     * 
     * @example
     * @code
     *   COERMessage msg = COERDecoder::parse(raw_data);
     *   auto chain = COERDecoder::extract_certificate_chain(msg);
     *   
     *   if (chain.empty()) {
     *       LOGI("No cert chain - issuer is direct child of root");
     *   } else {
     *       LOGI("Chain contains %zu intermediate certificates", chain.size());
     *   }
     * @endcode
     * 
     * @complexity Time: O(n) where n = number of certificates in chain
     * @complexity Space: O(m) where m = total size of all chain certificates
     */
    static std::vector<std::vector<uint8_t>> extract_certificate_chain(const COERMessage& message);
    
    /**
     * Extract message payload (the data that was signed)
     * 
     * Returns the actual V2X message payload that was protected by the signature.
     * For signature verification, this exact payload must be used with
     * extract_signature() in Phase 2's verify_ecdsa_signature().
     * 
     * Payload contains:
     *   - Timestamp
     *   - Vehicle position (GPS/RTK coordinates)
     *   - Heading, speed, acceleration
     *   - Safety warnings (hard braking, hazards, etc.)
     *   - Other V2X telemetry
     * 
     * @param message Parsed COERMessage (from parse())
     * 
     * @return Raw payload bytes (exact data that was signed)
     * @return Must be used unchanged with verify_ecdsa_signature()
     * 
     * @note For unsigned messages, payload is still returned
     *       (just not cryptographically protected)
     * 
     * @example
     * @code
     *   COERMessage msg = COERDecoder::parse(raw_data);
     *   auto payload = COERDecoder::get_payload(msg);
     *   auto signature = COERDecoder::extract_signature(msg);
     *   
     *   // This is the correct pairing for signature verification
     *   bool valid = crypto_engine.verify_ecdsa_signature(
     *       payload,     // Exact bytes signed
     *       signature,   // Extracted signature
     *       issuer_cert  // Issuer's public key
     *   ).valid;
     * @endcode
     * 
     * @complexity Time: O(1) - simple data reference
     * @complexity Space: O(1) - no allocation, reference only
     */
    static const std::vector<uint8_t>& get_payload(const COERMessage& message);
    
    // ========================================================================
    // Utility and Information Methods
    // ========================================================================
    
    /**
     * Get descriptive string for message type
     * 
     * @param message_type Raw message type byte
     * @return Human-readable description (e.g., "Signed", "Encrypted+Signed")
     */
    static std::string message_type_to_string(uint8_t message_type);
    
    /**
     * Get descriptive string for signature algorithm
     * 
     * @param algorithm Signature algorithm byte
     * @return Human-readable description (e.g., "ECDSA P-256")
     */
    static std::string signature_algorithm_to_string(uint8_t algorithm);
    
    /**
     * Get version string for compatibility checking
     * 
     * @return COER decoder implementation version (e.g., "1.0.0")
     */
    static std::string get_version();
    
    // ========================================================================
    // Logging and Debug
    // ========================================================================
    
    /**
     * Enable or disable debug logging
     * 
     * When enabled, logs detailed parsing steps for troubleshooting.
     * Has minimal performance impact when disabled.
     * 
     * @param enabled true to enable debug logs
     */
    static void set_debug_logging(bool enabled);
    
    /**
     * Log parsed message structure for debugging
     * 
     * Outputs: message type, version, payload size, signature presence, etc.
     * 
     * @param message Parsed COERMessage to log
     */
    static void log_message_structure(const COERMessage& message);
};

} // namespace sentinel::v2x
