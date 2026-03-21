#ifndef V2X_CRYPTO_ENGINE_H
#define V2X_CRYPTO_ENGINE_H

#include <string>
#include <vector>
#include <memory>

/**
 * @file v2x_crypto_engine.h
 * @brief V2X Security Cryptographic Engine Interface
 * 
 * Provides high-level cryptographic operations for V2X message verification:
 * - ECDSA signature verification (NIST P-256)
 * - SHA-256 message hashing
 * - X.509 certificate parsing and validation
 * - Certificate chain validation
 * 
 * Uses Botan cryptographic library for FIPS-compliant operations.
 */

namespace sentinel {
namespace v2x {

/**
 * @struct CertificateInfo
 * @brief Parsed X.509 certificate information
 */
struct CertificateInfo {
    std::string subject;              // Certificate subject DN
    std::string issuer;               // Certificate issuer DN
    std::string serial_number;        // Serial number (hex)
    std::string not_before;           // Validity start (ISO 8601)
    std::string not_after;            // Validity end (ISO 8601)
    std::vector<uint8_t> public_key;  // DER-encoded public key
    bool is_ca;                       // Is CA certificate
    std::vector<std::string> key_usage; // Key usage extensions
};

/**
 * @struct SignatureVerificationResult
 * @brief Result of signature verification
 */
struct SignatureVerificationResult {
    bool valid;                       // Signature is valid
    std::string error_message;        // Error details if invalid
    std::string algorithm;            // Used algorithm (e.g., "ECDSA(SHA-256)")
    uint64_t verification_time_ms;    // Time taken to verify
};

/**
 * @class V2XCryptoEngine
 * @brief Main cryptographic engine for V2X security operations
 * 
 * Thread-safe wrapper around Botan cryptographic primitives.
 * Implements IEEE 1609.2 security requirements for V2X communications.
 */
class V2XCryptoEngine {
public:
    /**
     * @brief Constructor - Initializes Botan engine
     */
    V2XCryptoEngine();
    
    /**
     * @brief Destructor - Cleans up resources
     */
    ~V2XCryptoEngine();
    
    /**
     * @brief Initialize with root CA certificate
     * @param root_ca_der DER-encoded root CA certificate
     * @return true if initialization successful, false otherwise
     */
    bool initialize_with_root_ca(const std::vector<uint8_t>& root_ca_der);

    /**
     * @brief Clear the configured trusted root CA
     */
    void clear_trusted_root_ca();
    
    /**
     * @brief Verify ECDSA P-256 signature on message
     * @param message The message that was signed
     * @param signature The ECDSA signature (DER-encoded)
     * @param public_key The signer's EC public key (DER-encoded)
     * @return SignatureVerificationResult with verification outcome
     * 
     * Implements ECDSA(SHA-256) verification per IEEE 1609.2
     */
    SignatureVerificationResult verify_ecdsa_signature(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& signature,
        const std::vector<uint8_t>& public_key
    );
    
    /**
     * @brief Compute SHA-256 hash of data
     * @param data Input data to hash
     * @return 32-byte SHA-256 hash
     */
    std::vector<uint8_t> sha256_hash(const std::vector<uint8_t>& data);
    
    /**
     * @brief Parse X.509 certificate from DER encoding
     * @param cert_der DER-encoded certificate
     * @return Parsed certificate information structure
     * @throw std::runtime_error if certificate is invalid
     */
    CertificateInfo parse_certificate(const std::vector<uint8_t>& cert_der);
    
    /**
     * @brief Validate X.509 certificate chain
     * @param certificate_chain Vector of DER-encoded certificates (leaf to root)
     * @param current_time_unix Unix timestamp for validity checking
     * @return true if entire chain is valid, false if any certificate fails validation
     * 
     * Performs:
     * - Expiration date checking
     * - Signature verification of each cert by issuer
     * - Key usage validation
     * - CA constraint validation
     */
    bool validate_certificate_chain(
        const std::vector<std::vector<uint8_t>>& certificate_chain,
        uint64_t current_time_unix = 0
    );
    
    /**
     * @brief Parse IEEE 1609.2 V2X message
     * @param message_der DER-encoded V2X message
     * @return Vector of extracted data fields (opaque format)
     * @throw std::runtime_error if message parsing fails
     * 
     * Extracts:
     * - Message type
     * - Signing certificate
     * - Signature
     * - Payload
     * - Timestamp
     */
    std::vector<std::vector<uint8_t>> parse_ieee1609_message(
        const std::vector<uint8_t>& message_der
    );
    
    /**
     * @brief Extract sender information from certificate
     * @param certificate_der DER-encoded certificate
     * @return Sender identifier (certificate subject DN)
     */
    std::string extract_sender_info(const std::vector<uint8_t>& certificate_der);
    
    /**
     * @brief Check only the certificate validity window
     * @param cert_der DER-encoded certificate
     * @param current_time_unix Unix timestamp (0 = current time)
     * @return true if certificate is within its not-before/not-after window
     */
    bool is_certificate_time_valid(
        const std::vector<uint8_t>& cert_der,
        uint64_t current_time_unix = 0
    );
    
    /**
     * @brief Cleanup and reset engine state
     */
    void cleanup();
    
    /**
     * @brief Get Botan version string
     * @return Version information
     */
    static std::string get_botan_version();
    
    /**
     * @brief Get SHA-256 digest of byte array (convenience method)
     * @param data Input data
     * @return Hex-encoded SHA-256 hash
     */
    static std::string sha256_hex(const std::vector<uint8_t>& data);

private:
    class Impl; // Forward declaration for pimpl pattern
    std::unique_ptr<Impl> pimpl_;
    
    // Prevent copying
    V2XCryptoEngine(const V2XCryptoEngine&) = delete;
    V2XCryptoEngine& operator=(const V2XCryptoEngine&) = delete;
};

} // namespace v2x
} // namespace sentinel

#endif // V2X_CRYPTO_ENGINE_H
