/**
 * @file v2x_coer_decoder.cpp
 * @brief IEEE 1609.2 COER Decoder Implementation
 *
 * Implements parsing of IEEE 1609.2-2016 Canonical Octet Encoding Rules (COER)
 * messages for V2X (Vehicle-to-Everything) communication.
 *
 * This implementation provides:
 *   - Low-level parsing primitives (byte reading, TLV parsing)
 *   - Message structure validation
 *   - Component extraction (signature, certificates, payload)
 *   - Comprehensive error handling
 *   - Debug logging capabilities
 *
 * Phase 3 Week 1 Delivery: Parsing Primitives Foundation
 * Phase 3 Week 2: Full Parser Implementation
 *
 * @author Sentinel V2X Bridge
 * @date March 7, 2026
 * @version 1.0.0
 */

#include "v2x_coer_decoder.h"
#include <cstring>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace sentinel::v2x {

// ============================================================================
// Debug Logging Configuration
// ============================================================================

namespace {
    bool g_debug_logging_enabled = false;

    /**
     * @brief Internal debug logging macro
     */
    #define COER_LOG_DEBUG(msg) \
        do { \
            if (g_debug_logging_enabled) { \
                std::cerr << "[COER DEBUG] " << msg << std::endl; \
            } \
        } while (0)

    /**
     * @brief Internal error logging macro
     */
    #define COER_LOG_ERROR(msg) \
        std::cerr << "[COER ERROR] " << msg << std::endl
}

// ============================================================================
// Parsing Primitives Helper Functions (Binary Reading)
// ============================================================================

namespace helpers {

/**
 * @brief Read a single byte at current position
 *
 * @param data Message buffer
 * @param pos Current position (will be incremented)
 * @return Byte value at current position
 * @throws COERBufferException if position exceeds buffer size
 */
uint8_t read_byte(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos >= data.size()) {
        throw COERBufferException(
            "Buffer underflow: attempted to read at position " +
            std::to_string(pos) + " (buffer size: " + std::to_string(data.size()) + ")"
        );
    }
    return data[pos++];
}

/**
 * @brief Read a 16-bit big-endian unsigned integer
 *
 * @param data Message buffer
 * @param pos Current position (will be incremented by 2)
 * @return 16-bit value (interpreted as big-endian)
 * @throws COERBufferException if not enough bytes available
 */
uint16_t read_uint16_be(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 2 > data.size()) {
        throw COERBufferException(
            "Buffer underflow: need 2 bytes at position " +
            std::to_string(pos) + " (only " + std::to_string(data.size() - pos) + " available)"
        );
    }
    uint16_t value = (static_cast<uint16_t>(data[pos]) << 8) |
                     static_cast<uint16_t>(data[pos + 1]);
    pos += 2;
    return value;
}

/**
 * @brief Read a 32-bit big-endian unsigned integer
 *
 * @param data Message buffer
 * @param pos Current position (will be incremented by 4)
 * @return 32-bit value (interpreted as big-endian)
 * @throws COERBufferException if not enough bytes available
 */
uint32_t read_uint32_be(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 4 > data.size()) {
        throw COERBufferException(
            "Buffer underflow: need 4 bytes at position " +
            std::to_string(pos) + " (only " + std::to_string(data.size() - pos) + " available)"
        );
    }
    uint32_t value = (static_cast<uint32_t>(data[pos]) << 24) |
                     (static_cast<uint32_t>(data[pos + 1]) << 16) |
                     (static_cast<uint32_t>(data[pos + 2]) << 8) |
                     static_cast<uint32_t>(data[pos + 3]);
    pos += 4;
    return value;
}

/**
 * @brief Read variable-length encoded integer (COER specific)
 *
 * COER uses variable-length encoding for lengths to save space:
 *   - 0x00-0x7F: Single byte (value as-is)
 *   - 0x80-0xFF: Multi-byte (high bit set, followed by length byte, then value)
 *
 * Example:
 *   Length 100: Encoded as 0x64 (single byte)
 *   Length 256: Encoded as 0x81 0x01 0x00 (0x81 = "2 bytes follow", then 0x01 0x00)
 *
 * @param data Message buffer
 * @param pos Current position (will be incremented)
 * @return Decoded length value
 * @throws COERBufferException if format is invalid or buffer too short
 */
uint32_t read_varint(const std::vector<uint8_t>& data, size_t& pos) {
    uint8_t first = read_byte(data, pos);

    // Short form (0-127): Single byte encoding
    if (first <= 0x7F) {
        return first;
    }

    // Long form: 0x80 + length_of_length_field
    uint8_t length_of_length = first & 0x7F;  // Extract lower 7 bits

    if (length_of_length == 0) {
        throw COERFormatException("Indefinite length encoding not supported");
    }

    if (length_of_length > 4) {
        throw COERFormatException(
            "Variable-length integer too large (length_of_length=" +
            std::to_string(length_of_length) + ")"
        );
    }

    uint32_t value = 0;
    for (uint8_t i = 0; i < length_of_length; ++i) {
        value = (value << 8) | read_byte(data, pos);
    }

    return value;
}

/**
 * @brief Read a byte sequence of specified length
 *
 * @param data Message buffer
 * @param pos Current position (will be incremented by length)
 * @param length Number of bytes to read
 * @return Vector containing the read bytes
 * @throws COERBufferException if not enough bytes available
 */
std::vector<uint8_t> read_bytes(const std::vector<uint8_t>& data, size_t& pos, size_t length) {
    if (pos + length > data.size()) {
        throw COERBufferException(
            "Buffer underflow: need " + std::to_string(length) + " bytes at position " +
            std::to_string(pos) + " (only " + std::to_string(data.size() - pos) + " available)"
        );
    }
    std::vector<uint8_t> result(data.begin() + pos, data.begin() + pos + length);
    pos += length;
    return result;
}

/**
 * @brief Parse a Type-Length-Value (TLV) structure
 *
 * COER messages are often structured as TLV sequences:
 *   - Type: Tag byte identifying the field (e.g., 0x04 = ECDSA P-256)
 *   - Length: Number of bytes in value (variable-length encoded)
 *   - Value: The actual data
 *
 * Example:
 *   0x04 (type) 0x40 (length=64) [64 bytes signature]
 *
 * @param data Message buffer
 * @param pos Current position (will be incremented)
 * @param out_tag Returns the tag byte
 * @param out_value Returns the value bytes
 *
 * @throws COERBufferException if format is invalid or buffer truncated
 * @throws COERFormatException if tag is invalid
 */
void read_tlv(const std::vector<uint8_t>& data, size_t& pos,
              uint8_t& out_tag, std::vector<uint8_t>& out_value) {
    // Read tag
    out_tag = read_byte(data, pos);

    // Read length (variable-length encoded)
    uint32_t length = read_varint(data, pos);

    if (length > 65536) {  // Sanity check: max reasonable TLV value
        throw COERFormatException(
            "TLV value too large (" + std::to_string(length) + " bytes)"
        );
    }

    // Read value
    out_value = read_bytes(data, pos, static_cast<size_t>(length));
}

/**
 * @brief Skip to a specific position in the buffer
 *
 * Useful for skipping optional fields or unknown extensions.
 *
 * @param data Message buffer
 * @param pos Current position (will be set to target)
 * @param target Target position
 * @throws COERBufferException if target exceeds buffer size
 */
void skip_to(const std::vector<uint8_t>& data, size_t& pos, size_t target) {
    if (target > data.size()) {
        throw COERBufferException(
            "Skip target exceeds buffer: " + std::to_string(target) +
            " > " + std::to_string(data.size())
        );
    }
    pos = target;
}

/**
 * @brief Get remaining bytes from current position
 *
 * @param data Message buffer
 * @param pos Current position
 * @return Number of bytes remaining
 */
size_t remaining_bytes(const std::vector<uint8_t>& data, size_t pos) {
    return (pos < data.size()) ? (data.size() - pos) : 0;
}

}  // namespace helpers

// ============================================================================
// Public API: Exception Classes
// ============================================================================

// Exception implementations are implicitly defined by their constructors
// in the header file. No additional implementation needed here.

// ============================================================================
// Public API: COERMessage Methods
// ============================================================================

bool COERMessage::SignatureContainer::has_chain() const {
    return !cert_chain.empty();
}

size_t COERMessage::SignatureContainer::chain_depth() const {
    return cert_chain.size();
}

bool COERMessage::is_signed() const {
    return (message_type & 0x02) != 0;
}

bool COERMessage::is_encrypted() const {
    return (message_type & 0x04) != 0;
}

size_t COERMessage::total_size() const {
    size_t size = 3;  // Header (1 byte) + length (2 bytes)
    size += payload.size();
    if (is_signed()) {
        size += 1;  // Signature algorithm byte
        size += signature_container.signature.size();
        size += 2;  // Certificate length field
        size += signature_container.issuer_cert.size();
        if (signature_container.has_chain()) {
            size += 1;  // Chain depth indicator
            for (const auto& cert : signature_container.cert_chain) {
                size += 2;  // Certificate length field
                size += cert.size();
            }
        }
    }
    return size;
}

// ============================================================================
// Public API: COERDecoder - Core Parsing
// ============================================================================

COERMessage COERDecoder::parse(const std::vector<uint8_t>& raw_message) {
    COER_LOG_DEBUG("Starting COER message parsing (size: " << raw_message.size() << " bytes)");

    if (raw_message.size() < 3) {
        throw COERBufferException(
            "Message too short: " + std::to_string(raw_message.size()) +
            " bytes (minimum 3 bytes for header + length)"
        );
    }

    size_t pos = 0;
    COERMessage msg;

    try {
        // ====== Step 1: Parse Header Byte ======
        uint8_t header = helpers::read_byte(raw_message, pos);
        msg.message_type = header;
        msg.protocol_version = (header >> 4) & 0x0F;  // Upper 4 bits

        COER_LOG_DEBUG("Header: 0x" << std::hex << static_cast<int>(header) << std::dec);
        COER_LOG_DEBUG("Protocol Version: " << static_cast<int>(msg.protocol_version));
        COER_LOG_DEBUG("Is Signed: " << msg.is_signed());
        COER_LOG_DEBUG("Is Encrypted: " << msg.is_encrypted());

        // Validate protocol version (currently supporting 1-3)
        if (msg.protocol_version > 3) {
            throw COERFormatException(
                "Unsupported protocol version: " + std::to_string(msg.protocol_version) +
                " (only 1-3 supported)"
            );
        }

        // ====== Step 2: Parse Payload Length (COER variable-length encoded) ======
        uint32_t payload_length = helpers::read_varint(raw_message, pos);
        COER_LOG_DEBUG("Payload Length: " << payload_length << " bytes");

        // Sanity check: payload length shouldn't exceed remaining buffer
        if (pos + payload_length > raw_message.size()) {
            size_t available = (raw_message.size() > pos) ? (raw_message.size() - pos) : 0;
            throw COERBufferException(
                "Buffer underflow: expected " + std::to_string(payload_length) +
                " bytes of payload, but only " + std::to_string(available) + " available"
            );
        }

        // ====== Step 3: Extract Payload ======
        msg.payload = helpers::read_bytes(raw_message, pos, static_cast<size_t>(payload_length));
        COER_LOG_DEBUG("Extracted payload: " << payload_length << " bytes");

        // ====== Step 4: Parse Signature Container (if signed) ======
        if (msg.is_signed()) {
            COER_LOG_DEBUG("Message is signed, parsing signature container");

            // Signature algorithm
            msg.signature_container.signature_algorithm = helpers::read_byte(raw_message, pos);
            COER_LOG_DEBUG("Signature Algorithm: 0x" << std::hex
                          << static_cast<int>(msg.signature_container.signature_algorithm)
                          << std::dec);

            // ECDSA signature (typically 64 bytes for P-256, COER variable-length encoded)
            uint32_t sig_length = helpers::read_varint(raw_message, pos);
            msg.signature_container.signature = helpers::read_bytes(raw_message, pos, static_cast<size_t>(sig_length));
            COER_LOG_DEBUG("Signature: " << sig_length << " bytes");

            // Issuer certificate (COER variable-length encoded length)
            uint32_t cert_length = helpers::read_varint(raw_message, pos);
            msg.signature_container.issuer_cert = helpers::read_bytes(raw_message, pos, static_cast<size_t>(cert_length));
            COER_LOG_DEBUG("Issuer Certificate: " << cert_length << " bytes");

            // Certificate chain (optional, COER variable-length encoded lengths)
            if (helpers::remaining_bytes(raw_message, pos) > 0) {
                uint8_t chain_depth = helpers::read_byte(raw_message, pos);
                COER_LOG_DEBUG("Certificate Chain Depth: " << static_cast<int>(chain_depth));

                for (int i = 0; i < chain_depth; ++i) {
                    uint32_t chain_cert_length = helpers::read_varint(raw_message, pos);
                    auto chain_cert = helpers::read_bytes(raw_message, pos, static_cast<size_t>(chain_cert_length));
                    msg.signature_container.cert_chain.push_back(chain_cert);
                    COER_LOG_DEBUG("Chain Certificate[" << i << "]: " << chain_cert_length << " bytes");
                }
            }
        }

        COER_LOG_DEBUG("Parse successful: total bytes consumed: " << pos << "/" << raw_message.size());
        return msg;

    } catch (const COERDecodeException&) {
        // Re-throw COER exceptions as-is
        throw;
    } catch (const std::exception& e) {
        // Catch any other exceptions and wrap them
        throw COERDecodeException(std::string("Parse error: ") + e.what());
    }
}

// ============================================================================
// Public API: COERDecoder - Validation
// ============================================================================

bool COERDecoder::validate_structure(const COERMessage& message) {
    COER_LOG_DEBUG("Validating message structure");

    // Check protocol version range
    if (message.protocol_version > 3) {
        COER_LOG_DEBUG("Invalid protocol version: " << static_cast<int>(message.protocol_version));
        return false;
    }

    // Check payload is not unreasonably large
    if (message.payload.size() > 65536) {
        COER_LOG_DEBUG("Payload too large: " << message.payload.size());
        return false;
    }

    // Check signature structure if signed
    if (message.is_signed()) {
        // Signature should be present and reasonable size
        if (message.signature_container.signature.empty() ||
            message.signature_container.signature.size() > 256) {
            COER_LOG_DEBUG("Invalid signature size: "
                          << message.signature_container.signature.size());
            return false;
        }

        // Issuer certificate must be present
        if (message.signature_container.issuer_cert.empty()) {
            COER_LOG_DEBUG("Missing issuer certificate");
            return false;
        }

        // Check certificate chain depth (prevent DoS with huge chains)
        if (message.signature_container.chain_depth() > 20) {
            COER_LOG_DEBUG("Certificate chain too deep: "
                          << message.signature_container.chain_depth());
            return false;
        }

        // Check each certificate in chain
        for (size_t i = 0; i < message.signature_container.cert_chain.size(); ++i) {
            if (message.signature_container.cert_chain[i].empty()) {
                COER_LOG_DEBUG("Empty certificate in chain at index " << i);
                return false;
            }
            if (message.signature_container.cert_chain[i].size() > 4096) {
                COER_LOG_DEBUG("Certificate too large in chain at index " << i);
                return false;
            }
        }
    }

    COER_LOG_DEBUG("Message structure validation passed");
    return true;
}

// ============================================================================
// Public API: COERDecoder - Component Extraction
// ============================================================================

std::vector<uint8_t> COERDecoder::extract_signature(const COERMessage& message) {
    if (!message.is_signed()) {
        throw COERFormatException("Cannot extract signature from unsigned message");
    }
    if (message.signature_container.signature.empty()) {
        throw COERFormatException("Message signature is empty");
    }
    return message.signature_container.signature;
}

std::vector<uint8_t> COERDecoder::extract_issuer_certificate(const COERMessage& message) {
    if (!message.is_signed()) {
        throw COERFormatException("Cannot extract issuer certificate from unsigned message");
    }
    if (message.signature_container.issuer_cert.empty()) {
        throw COERFormatException("Issuer certificate is missing");
    }
    return message.signature_container.issuer_cert;
}

std::vector<std::vector<uint8_t>> COERDecoder::extract_certificate_chain(const COERMessage& message) {
    if (!message.is_signed()) {
        throw COERFormatException("Cannot extract certificate chain from unsigned message");
    }
    return message.signature_container.cert_chain;
}

const std::vector<uint8_t>& COERDecoder::get_payload(const COERMessage& message) {
    return message.payload;
}

// ============================================================================
// Public API: COERDecoder - Utilities
// ============================================================================

std::string COERDecoder::message_type_to_string(uint8_t message_type) {
    uint8_t version = (message_type >> 4) & 0x0F;  // High nibble: version
    bool is_signed = (message_type & 0x02) != 0;   // Low nibble: bit 1 = signed
    bool is_encrypted = (message_type & 0x04) != 0; // Low nibble: bit 2 = encrypted

    std::ostringstream oss;
    oss << "Version " << static_cast<int>(version);

    if (is_signed && is_encrypted) {
        oss << ", Signed+Encrypted";
    } else if (is_signed) {
        oss << ", Signed";
    } else if (is_encrypted) {
        oss << ", Encrypted";
    } else {
        oss << ", Unsigned";
    }

    return oss.str();
}

std::string COERDecoder::signature_algorithm_to_string(uint8_t algorithm) {
    switch (algorithm) {
        case 0x04:
            return "ECDSA P-256";
        case 0x05:
            return "ECDSA P-384";
        default:
            std::ostringstream oss;
            oss << "Unknown (0x" << std::hex << static_cast<int>(algorithm) << ")";
            return oss.str();
    }
}

std::string COERDecoder::get_version() {
    return "1.0.0";
}

// ============================================================================
// Public API: COERDecoder - Logging Control
// ============================================================================

void COERDecoder::set_debug_logging(bool enabled) {
    g_debug_logging_enabled = enabled;
    if (enabled) {
        COER_LOG_DEBUG("Debug logging enabled");
    }
}

void COERDecoder::log_message_structure(const COERMessage& message) {
    std::ostringstream oss;
    oss << "\n"
        << "=== COER Message Structure ===\n"
        << "  Message Type: " << message_type_to_string(message.message_type) << "\n"
        << "  Protocol Version: " << static_cast<int>(message.protocol_version) << "\n"
        << "  Is Signed: " << (message.is_signed() ? "yes" : "no") << "\n"
        << "  Is Encrypted: " << (message.is_encrypted() ? "yes" : "no") << "\n"
        << "  Payload Size: " << message.payload.size() << " bytes\n"
        << "  Total Size: " << message.total_size() << " bytes\n";

    if (message.is_signed()) {
        oss << "  Signature Algorithm: "
            << signature_algorithm_to_string(message.signature_container.signature_algorithm) << "\n"
            << "  Signature Size: " << message.signature_container.signature.size() << " bytes\n"
            << "  Issuer Cert Size: " << message.signature_container.issuer_cert.size() << " bytes\n"
            << "  Chain Depth: " << message.signature_container.chain_depth() << "\n";
        for (size_t i = 0; i < message.signature_container.cert_chain.size(); ++i) {
            oss << "    Chain[" << i << "]: " << message.signature_container.cert_chain[i].size() << " bytes\n";
        }
    }
    oss << "==============================\n";

    std::cout << oss.str();
}

}  // namespace sentinel::v2x
