#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

namespace sentinel::v2x {

/**
 * Exception thrown when payload validation fails
 */
class PayloadValidationException : public std::runtime_error {
public:
    explicit PayloadValidationException(const std::string& message);
};

/**
 * Validates DER-encoded payload structure (ASN.1 sanity checks)
 * 
 * This validator performs basic structure checks WITHOUT full ASN.1 decoding:
 * - Verifies SEQUENCE tag (0x30) present
 * - Validates DER length encoding (short and long forms)
 * - Checks payload size consistency
 * 
 * This is a defense-in-depth measure: crypto validation verifies authenticity,
 * payload validation verifies basic structure before interpretation.
 */
class PayloadValidator {
public:
    /**
     * Validate basic DER structure of payload
     * 
     * Performs:
     * 1. Non-empty check
     * 2. SEQUENCE tag (0x30) validation
     * 3. DER length encoding validation (both short and long forms)
     * 4. Payload size vs declared length consistency check
     * 
     * @param payload Raw verified payload bytes
     * @throws PayloadValidationException if validation fails
     */
    static void validate_der_structure(const std::vector<uint8_t>& payload);
    
    /**
     * Get declared payload length from DER header
     * 
     * Parses DER SEQUENCE header to extract declared content length.
     * Only call after successful validate_der_structure().
     * 
     * @param payload DER-encoded bytes (must pass validate_der_structure first)
     * @return Declared length of DER content (excluding header bytes)
     */
    static size_t get_der_declared_length(const std::vector<uint8_t>& payload);
    
private:
    /**
     * Validate and parse DER length field
     * 
     * Handles both:
     * - Short form: 0x00-0x7F (length is the byte value)
     * - Long form: 0x80-0xFF (first byte's lower 7 bits = number of length octets)
     * 
     * @param payload Payload starting with tag byte
     * @return Total header size (tag + length field)
     * @throws PayloadValidationException if encoding invalid
     */
    static size_t parse_der_length_field(const std::vector<uint8_t>& payload);
};

} // namespace sentinel::v2x
