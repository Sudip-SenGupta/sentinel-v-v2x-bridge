#include "v2x_payload_validator.h"
#include <iomanip>
#include <sstream>

namespace sentinel::v2x {

PayloadValidationException::PayloadValidationException(const std::string& message)
    : std::runtime_error(message) {}

void PayloadValidator::validate_der_structure(const std::vector<uint8_t>& payload) {
    // Check 1: Minimum size (tag + length)
    if (payload.empty()) {
        throw PayloadValidationException(
            "Payload validation failed: payload is empty"
        );
    }
    
    if (payload.size() < 2) {
        throw PayloadValidationException(
            "Payload validation failed: payload too short for DER header (need >= 2 bytes, have " +
            std::to_string(payload.size()) + ")"
        );
    }
    
    // Check 2: SEQUENCE tag (0x30)
    if (payload[0] != 0x30) {
        std::ostringstream oss;
        oss << "Payload validation failed: expected SEQUENCE tag 0x30, got 0x"
            << std::setfill('0') << std::setw(2) << std::hex 
            << static_cast<int>(payload[0]);
        throw PayloadValidationException(oss.str());
    }
    
    // Check 3: Validate DER length encoding
    size_t header_size = parse_der_length_field(payload);
    
    // Check 4: Payload size consistency
    if (payload.size() != header_size) {
        throw PayloadValidationException(
            "Payload validation failed: declared length requires " +
            std::to_string(header_size) + " bytes total, but payload has " +
            std::to_string(payload.size())
        );
    }
}

size_t PayloadValidator::get_der_declared_length(const std::vector<uint8_t>& payload) {
    if (payload.size() < 2 || payload[0] != 0x30) {
        throw PayloadValidationException(
            "get_der_declared_length: invalid DER header"
        );
    }
    
    uint8_t len_byte = payload[1];
    
    if (len_byte <= 127) {
        // Short form: length is directly in len_byte
        return static_cast<size_t>(len_byte);
    }
    
    // Long form
    uint8_t num_octets = len_byte & 0x7F;
    if (num_octets == 0 || payload.size() < 2 + num_octets) {
        throw PayloadValidationException(
            "get_der_declared_length: truncated or invalid length field"
        );
    }
    
    uint32_t length = 0;
    for (uint8_t i = 0; i < num_octets; ++i) {
        length = (length << 8) | payload[2 + i];
    }
    
    return static_cast<size_t>(length);
}

size_t PayloadValidator::parse_der_length_field(const std::vector<uint8_t>& payload) {
    uint8_t len_byte = payload[1];
    
    if (len_byte <= 127) {
        // Short form: 0x00-0x7F
        // Total: 1 byte tag + 1 byte length + len_byte content
        return 2 + static_cast<size_t>(len_byte);
    }
    
    // Long form: 0x80-0xFF
    uint8_t num_octets = len_byte & 0x7F;
    
    // Validate num_octets
    if (num_octets == 0) {
        throw PayloadValidationException(
            "Payload validation failed: indefinite length encoding (0x80) not allowed"
        );
    }
    
    if (num_octets > 4) {
        throw PayloadValidationException(
            "Payload validation failed: length field too large (" +
            std::to_string(num_octets) + " octets, max 4)"
        );
    }
    
    // Check enough bytes present for length field
    if (payload.size() < 2 + num_octets) {
        throw PayloadValidationException(
            "Payload validation failed: truncated length field (expected " +
            std::to_string(2 + num_octets) + " bytes for header, have " +
            std::to_string(payload.size()) + ")"
        );
    }
    
    // Decode length from num_octets bytes (big-endian)
    uint32_t length = 0;
    for (uint8_t i = 0; i < num_octets; ++i) {
        length = (length << 8) | payload[2 + i];
    }
    
    // Total: 1 byte tag + 1 byte length_indicator + num_octets + length content
    return 2 + num_octets + static_cast<size_t>(length);
}

} // namespace sentinel::v2x
