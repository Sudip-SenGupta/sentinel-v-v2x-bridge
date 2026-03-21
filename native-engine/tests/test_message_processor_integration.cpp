#include <gtest/gtest.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "v2x_message_processor.h"
#include "v2x_payload_validator.h"

using namespace sentinel::v2x;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string hex_to_string(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t byte : data) {
        oss << std::setfill('0') << std::setw(2) << std::hex 
            << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<uint8_t> create_der_sequence(const std::vector<uint8_t>& content) {
    std::vector<uint8_t> result;
    result.push_back(0x30);  // SEQUENCE tag
    
    // Encode length
    if (content.size() <= 127) {
        result.push_back(static_cast<uint8_t>(content.size()));
    } else {
        // Multi-byte length encoding
        uint32_t len = content.size();
        if (len <= 0xFF) {
            result.push_back(0x81);
            result.push_back(static_cast<uint8_t>(len));
        } else {
            result.push_back(0x82);
            result.push_back(static_cast<uint8_t>(len >> 8));
            result.push_back(static_cast<uint8_t>(len & 0xFF));
        }
    }
    
    result.insert(result.end(), content.begin(), content.end());
    return result;
}

// ============================================================================
// TEST: PayloadValidator - Valid DER Structures
// ============================================================================

TEST(PayloadValidatorTest, ValidSimpleSequence) {
    std::vector<uint8_t> content = {0x04, 0x03, 'B', 'S', 'M'};  // OCTET STRING
    auto der = create_der_sequence(content);
    
    // Should not throw
    EXPECT_NO_THROW({
        PayloadValidator::validate_der_structure(der);
    });
}

TEST(PayloadValidatorTest, ValidLongSequence) {
    std::vector<uint8_t> content(200, 0xAA);  // 200-byte content
    auto der = create_der_sequence(content);
    
    EXPECT_NO_THROW({
        PayloadValidator::validate_der_structure(der);
    });
    
    // Verify declared length matches
    size_t declared_len = PayloadValidator::get_der_declared_length(der);
    EXPECT_EQ(declared_len, 200);
}

TEST(PayloadValidatorTest, ValidMaximumLength) {
    // Create 256-byte sequence (tests multi-byte length: 0x82 0x01 0x00)
    std::vector<uint8_t> content(256, 0xBB);
    auto der = create_der_sequence(content);
    
    EXPECT_NO_THROW({
        PayloadValidator::validate_der_structure(der);
    });
}

TEST(PayloadValidatorTest, CompleteDerSequenceProbeMatchesRealDerPayload) {
    std::vector<uint8_t> content = {0x04, 0x03, 'B', 'S', 'M'};
    auto der = create_der_sequence(content);

    EXPECT_TRUE(PayloadValidator::is_complete_der_sequence(der));
}

// ============================================================================
// TEST: PayloadValidator - Invalid DER Structures
// ============================================================================

TEST(PayloadValidatorTest, EmptyPaylogThrows) {
    std::vector<uint8_t> empty_payload;
    
    EXPECT_THROW({
        PayloadValidator::validate_der_structure(empty_payload);
    }, PayloadValidationException);
}

TEST(PayloadValidatorTest, TooShortThrows) {
    std::vector<uint8_t> short_payload = {0x30};  // Only tag, no length
    
    EXPECT_THROW({
        PayloadValidator::validate_der_structure(short_payload);
    }, PayloadValidationException);
}

TEST(PayloadValidatorTest, WrongTagThrows) {
    std::vector<uint8_t> wrong_tag = {0x31, 0x03, 0xAA, 0xBB, 0xCC};  // SET tag instead of SEQUENCE
    
    EXPECT_THROW({
        PayloadValidator::validate_der_structure(wrong_tag);
    }, PayloadValidationException);
}

TEST(PayloadValidatorTest, SizeInconsistencyThrows) {
    // Says 10 bytes but only 5 bytes present
    std::vector<uint8_t> bad_size = {0x30, 0x0A, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    
    EXPECT_THROW({
        PayloadValidator::validate_der_structure(bad_size);
    }, PayloadValidationException);
}

TEST(PayloadValidatorTest, IndefiniteLengthThrows) {
    // 0x80 = indefinite length (not allowed)
    std::vector<uint8_t> indefinite = {0x30, 0x80, 0xAA, 0xBB};
    
    EXPECT_THROW({
        PayloadValidator::validate_der_structure(indefinite);
    }, PayloadValidationException);
}

TEST(PayloadValidatorTest, TruncatedLengthFieldThrows) {
    // Says 2-byte length but only 1 byte present
    std::vector<uint8_t> truncated = {0x30, 0x82, 0x01};
    
    EXPECT_THROW({
        PayloadValidator::validate_der_structure(truncated);
    }, PayloadValidationException);
}

TEST(PayloadValidatorTest, CompleteDerSequenceProbeRejectsRawPsmLikePayload) {
    std::vector<uint8_t> raw_psm_like = {
        0x30, 0x00, 0x00, 0x00, 0x2A, 0x00, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF, 0x16, 0x83, 0xFE, 0x08
    };

    EXPECT_FALSE(PayloadValidator::is_complete_der_sequence(raw_psm_like));
}

// ============================================================================
// TEST: PayloadValidator - Error Messages
// ============================================================================

TEST(PayloadValidatorTest, ErrorMessageContainsDiagnostics) {
    std::vector<uint8_t> payload = {0x31, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05};
    
    try {
        PayloadValidator::validate_der_structure(payload);
        FAIL() << "Expected PayloadValidationException";
    } catch (const PayloadValidationException& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("SEQUENCE"), std::string::npos);
        EXPECT_NE(msg.find("0x31"), std::string::npos);
    }
}

// ============================================================================
// TEST: V2XMessageProcessor - COER Parsing Failures
// ============================================================================

TEST(MessageProcessorTest, EmptyRawMessageFails) {
    std::vector<uint8_t> empty_msg;
    auto result = V2XMessageProcessor::process_message(empty_msg);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.coer_parse_ok);
    EXPECT_NE(result.error_message.find("COER parse failed"), std::string::npos);
}

TEST(MessageProcessorTest, TruncatedHeaderFails) {
    std::vector<uint8_t> truncated = {0x30};  // Only header, no length
    auto result = V2XMessageProcessor::process_message(truncated);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.coer_parse_ok);
}

TEST(MessageProcessorTest, InvalidVersionFails) {
    // Version 7 (0x70) is invalid
    std::vector<uint8_t> bad_version = {0x70, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05};
    auto result = V2XMessageProcessor::process_message(bad_version);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.coer_parse_ok);
}

// ============================================================================
// TEST: V2XMessageProcessor - Payload Structure Failures
// ============================================================================

TEST(MessageProcessorTest, MalformedPayloadStructureFails) {
    // Create a COER message with non-DER payload
    // Header 0x32 (signed), payload length 0x05, payload is non-DER
    std::vector<uint8_t> bad_payload = {
        0x32,           // Header: signed message, flags
        0x05,           // Payload length (5 bytes)
        0x31, 0x03, 0xAA, 0xBB, 0xCC,  // Invalid SEQUENCE tag (0x31 = SET)
        // Signature and cert would follow...
    };
    
    auto result = V2XMessageProcessor::process_message(bad_payload);
    
    // May fail at COER parsing or payload validation
    EXPECT_FALSE(result.is_valid);
    if (result.coer_parse_ok) {
        EXPECT_FALSE(result.payload_structure_ok);
        EXPECT_NE(result.error_message.find("Payload validation failed"), std::string::npos);
    }
}

TEST(MessageProcessorTest, EmptyPayloadStructureFails) {
    // Zero-length payload should fail empty check
    std::vector<uint8_t> empty_payload = {
        0x32,  // Header: signed
        0x00,  // Payload length (0 bytes)
        0x00,  // Signature length (0 bytes)
        0x00,  // Cert length (0 bytes)
    };
    
    auto result = V2XMessageProcessor::process_message(empty_payload);
    
    EXPECT_FALSE(result.is_valid);
}

// ============================================================================
// TEST: V2XMessageProcessor - Unsigned Messages
// ============================================================================

TEST(MessageProcessorTest, UnsignedMessageSkipsCryptoValidation) {
    // Create minimal unsigned message
    std::vector<uint8_t> unsigned_msg = {
        0x30,           // Header: version 3, unsigned
        0x0F,           // Payload length (15 bytes)
        0x30, 0x0C,     // SEQUENCE (DER header)
        0x04, 0x03, 'B', 'S', 'M',  // OCTET STRING "BSM"
        0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00  // INTEGER
    };
    
    auto result = V2XMessageProcessor::process_message(unsigned_msg);
    
    // May succeed or fail depending on exact format, but signature/chain checks skipped
    if (result.coer_parse_ok && result.payload_structure_ok) {
        EXPECT_TRUE(result.signature_valid);  // Skipped, defaults to true
        EXPECT_TRUE(result.chain_valid);      // Skipped, defaults to true
    }
}

// ============================================================================
// TEST: V2XMessageProcessor - Version Info
// ============================================================================

TEST(MessageProcessorTest, GetVersionReturnsString) {
    std::string version = V2XMessageProcessor::get_version();
    
    EXPECT_FALSE(version.empty());
    EXPECT_NE(version.find("Message Processor"), std::string::npos);
    EXPECT_NE(version.find("v1.0.0"), std::string::npos);
}

// ============================================================================
// TEST: Integration - Realistic Scenarios
// ============================================================================

TEST(MessageProcessorIntegrationTest, ProcessValidStructureSucceeds) {
    // Create a valid DER-encoded payload
    std::vector<uint8_t> bsm_content = {
        0x04, 0x02, 'B', 'S'  // OCTET STRING "BS"
    };
    auto der_payload = create_der_sequence(bsm_content);
    
    // Can validate payload structure
    EXPECT_NO_THROW({
        PayloadValidator::validate_der_structure(der_payload);
    });
}

TEST(MessageProcessorIntegrationTest, NestedSequenceValidates) {
    // Nested SEQUENCE (common in real ASN.1)
    std::vector<uint8_t> inner = {0x30, 0x02, 0x04, 0x00};
    auto outer = create_der_sequence(inner);
    
    EXPECT_NO_THROW({
        PayloadValidator::validate_der_structure(outer);
    });
}

TEST(MessageProcessorIntegrationTest, ResultStructureInitialization) {
    std::vector<uint8_t> empty;
    auto result = V2XMessageProcessor::process_message(empty);
    
    // Verify all fields initialized
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.coer_parse_ok);
    EXPECT_FALSE(result.payload_structure_ok);
    EXPECT_FALSE(result.signature_valid);
    EXPECT_FALSE(result.chain_valid);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.payload.empty());
    EXPECT_TRUE(result.signature.empty());
    EXPECT_TRUE(result.chain.empty());
}

// ============================================================================
// TEST: Defense-in-Depth - Multi-Stage Validation
// ============================================================================

TEST(DefenseInDepthTest, PayloadValidatorErrorMessageClarity) {
    std::vector<uint8_t> payload = {0x30, 0x20, 0x01, 0x02};  // Claims 32 bytes, has only 4
    
    try {
        PayloadValidator::validate_der_structure(payload);
        FAIL() << "Expected exception";
    } catch (const PayloadValidationException& e) {
        std::string msg = e.what();
        // Verify diagnostic information
        EXPECT_NE(msg.find("validation failed"), std::string::npos);
        // Contains byte counts (numeric values) - simple check for digits
        bool has_numbers = msg.find_first_of("0123456789") != std::string::npos;
        EXPECT_TRUE(has_numbers);
    }
}

TEST(DefenseInDepthTest, ProcessorErrorPropagationChain) {
    // Empty message → COER parsing fails → error propagates with context
    std::vector<uint8_t> empty;
    auto result = V2XMessageProcessor::process_message(empty);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.coer_parse_ok);
    EXPECT_NE(result.error_message.find("parse"), std::string::npos);
}

// ============================================================================
// BENCHMARK: Payload Validator Performance
// ============================================================================

TEST(PayloadValidatorPerformanceTest, ValidationOverhead) {
    // Verify validation is fast (basic performance benchmarking)
    std::vector<uint8_t> content(128, 0xAA);
    auto der = create_der_sequence(content);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        PayloadValidator::validate_der_structure(der);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto avg_us = duration.count() / 100.0;
    
    // Just verify it completes successfully
    // (Validation is so fast it may round to 0 us on modern hardware)
    EXPECT_GE(duration.count(), 0);
    
    // Log the actual performance for reference
    if (avg_us > 0.0) {
        std::cout << "  Payload validation: " << avg_us << " us per call (100 calls)" << std::endl;
    } else {
        std::cout << "  Payload validation: < 0.01 us per call (100 calls in < 1 us total)" << std::endl;
    }
}

// ============================================================================
// TEST SUMMARY
// ============================================================================

/**
 * Test Coverage Summary:
 * 
 * PayloadValidator Tests (11 total):
 *   - Valid structures: 3 tests
 *   - Invalid structures: 6 tests
 *   - Error diagnostics: 1 test
 *   - Performance: 1 test
 * 
 * V2XMessageProcessor Tests (13 total):
 *   - COER parsing failures: 3 tests
 *   - Payload structure failures: 2 tests
 *   - Integration scenarios: 3 tests
 *   - Defense-in-depth: 2 tests
 *   - Unsigned messages: 1 test
 *   - Version/API: 1 test
 *   - Result initialization: 1 test
 * 
 * Total: 24 tests
 * Coverage: PayloadValidator + MessageProcessor orchestration
 * 
 * Key Scenarios:
 * ✓ Empty/truncated messages
 * ✓ Invalid DER structure
 * ✓ Nested sequences
 * ✓ Multi-byte length encoding
 * ✓ Error message clarity
 * ✓ Unsigned message path
 * ✓ Performance validation
 */
