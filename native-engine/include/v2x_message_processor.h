#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "v2x_crypto_engine.h"
#include "v2x_structures.hpp"

namespace sentinel::v2x {

/**
 * Result of complete end-to-end V2X message verification
 * 
 * Contains:
 * - Overall validation result (is_valid)
 * - Step-by-step status (which stages passed/failed)
 * - Extracted components (payload, signature, chain)
 * - Diagnostic error message (if failed)
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

    /// Decoded frame type after successful verification and frame decoding
    MessageFrameType frame_type = MessageFrameType::UNKNOWN;

    /// Verified and decoded message, populated only when processing succeeds
    std::optional<DecodedV2XMessage> decoded_message;
};

/**
 * Complete end-to-end V2X message verification pipeline
 * 
 * Orchestrates:
 * 1. COER format parsing (Week 1)
 * 2. Payload DER structure validation (Week 2)
 * 3. Cryptographic signature verification (Phase 2)
 * 4. Certificate chain validation (Phase 2)
 */
class V2XMessageProcessor {
public:
    V2XMessageProcessor() = delete;

    /**
     * Process and verify a V2X message
     *
     * Each stage returns gracefully on failure (no exceptions to caller).
     * Check result.is_valid or inspect stage-specific fields for diagnostics.
     *
     * @param raw_message Raw V2X message in COER format
     * @return Verification result with decoded output on success
     */
    static MessageVerificationResult process_message(
        const std::vector<uint8_t>& raw_message
    );

    /**
     * Get processor version
     * @return Version string
     */
    static std::string get_version();

#if defined(SENTINEL_V2X_TESTING)
    struct ChainValidationResult {
        bool valid;
        std::string error_message;
    };

    using SignatureVerifierHook = std::function<SignatureVerificationResult(
        const std::vector<uint8_t>&,
        const std::vector<uint8_t>&,
        const std::vector<uint8_t>&
    )>;

    using ChainValidatorHook = std::function<ChainValidationResult(
        const std::vector<std::vector<uint8_t>>&,
        uint64_t
    )>;

    // Test-only seam for off-target crypto-boundary coverage.
    static void set_test_crypto_hooks(
        SignatureVerifierHook signature_verifier,
        ChainValidatorHook chain_validator
    );
    static void clear_test_crypto_hooks();
#endif
};

} // namespace sentinel::v2x
