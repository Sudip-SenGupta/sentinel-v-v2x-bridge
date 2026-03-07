#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace sentinel::v2x {

/**
 * Result of complete end-to-end V2X message verification
 * 
 * Contains:
 * - Overall validation result (is_valid)
 * - Step-by-step status (which stages passed/failed)
 * - Extracted components (payload, signature, chain)
 * - Diagnostic error message (if failed)
 * 
 * Usage:
 * ```cpp
 * auto result = V2XMessageProcessor::process_message(raw_msg);
 * if (result.is_valid) {
 *     // Use result.payload for further processing
 * } else {
 *     // Log result.error_message
 * }
 * ```
 */
struct MessageVerificationResult {
    /// Overall validation result (true only if ALL stages passed)
    bool is_valid = false;
    
    /// COER parsing succeeded
    bool coer_parse_ok = false;
    
    /// Payload DER structure valid (only checked if signed)
    bool payload_structure_ok = false;
    
    /// Cryptographic signature verification passed (only checked if signed)
    bool signature_valid = false;
    
    /// Certificate chain validation passed (only checked if signed)
    bool chain_valid = false;
    
    /// Diagnostic error message (empty if is_valid=true)
    std::string error_message;
    
    /// Verified payload bytes
    std::vector<uint8_t> payload;
    
    /// Extracted signature (if found)
    std::vector<uint8_t> signature;
    
    /// Certificate chain (if found)
    std::vector<std::vector<uint8_t>> chain;
};

/**
 * Complete end-to-end V2X message verification pipeline
 * 
 * Orchestrates:
 * 1. COER format parsing (Week 1)
 * 2. Payload DER structure validation (Week 2)
 * 3. Cryptographic signature verification (Phase 2)
 * 4. Certificate chain validation (Phase 2)
 * 
 * Defense-in-depth strategy:
 * - COER parsing: Formats are well-formed
 * - Payload validation: Payload has valid structure
 * - Signature verification: Message is authentic
 * - Chain validation: Issuer certificate is trusted
 */
class V2XMessageProcessor {
public:
    /**
     * Process and verify a V2X message
     * 
     * Complete pipeline:
     * 
     * Raw Message
     *     ↓
     * [COER Parse]        → COERDecoder::parse()
     *     ↓
     * [Payload Validate]  → PayloadValidator::validate_der_structure()
     *     ↓
     * [Verify Signature]  → V2XCryptoEngine::verify_ecdsa_signature()
     *     ↓
     * [Validate Chain]    → V2XCryptoEngine::validate_certificate_chain()
     *     ↓
     * [MessageVerificationResult]
     * 
     * Each stage returns gracefully on failure (no exceptions to caller).
     * Check result.is_valid or inspect stage-specific fields for diagnostics.
     * 
     * @param raw_message Raw V2X message in COER format
     * @return Verification result with all details
     */
    static MessageVerificationResult process_message(
        const std::vector<uint8_t>& raw_message
    );
    
    /**
     * Get processor version
     * @return Version string
     */
    static std::string get_version();
};

} // namespace sentinel::v2x
