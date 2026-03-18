/**
 * @file test_v2x_message_frame_integration.cpp
 * @brief End-to-End Integration Tests: COER → Decoded Message
 * 
 * Validates the complete message processing pipeline:
 *   1. Raw COER bytes → COERMessage
 *   2. COERMessage → MessageFrameType detection  
 *   3. MessageFrameType → DecodedV2XMessage (structured data)
 * 
 * Tests with realistic V2X test vectors for BSM, SPaT, PSM.
 * 
 * Phase 4 Delivery: End-to-end message processing validation
 * 
 * @author Sentinel V2X Bridge
 * @date March 11, 2026
 */

#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>
#include "v2x_coer_decoder.h"
#include "v2x_frame_decoder.h"
#include "v2x_structures.hpp"

using namespace sentinel::v2x;

// ============================================================================
// Test Helpers: Create Realistic COER BSM Test Vectors
// ============================================================================

namespace {

/**
 * Helper: Create a minimal valid COER BSM (Basic Safety Message)
 * Returns raw COER bytes that can be parsed and decoded
 * 
 * Structure:
 *   [1 byte] Frame type (0x1X where X = message version)
 *   [4 bytes] Timestamp (milliseconds, big-endian)
 *   [6 bytes] Sender MAC address
 *   [1 byte] Sequence number
 *   [4 bytes] Latitude (WGS84, big-endian signed)
 *   [4 bytes] Longitude (WGS84, big-endian signed)
 *   [2 bytes] Speed (16-bit unsigned, 0.02 m/s per unit)
 *   [2 bytes] Heading (16-bit unsigned, 0.01 degree per unit)
 *   [2 bytes] Acceleration (16-bit signed, 0.01 m/s² per unit)
 */
std::vector<uint8_t> createMinimalBSMVector(
    uint64_t timestamp_ms = 1710086400000ULL,
    double latitude = 37.7749,
    double longitude = -122.4194,
    float speed = 12.5f,      // m/s
    float heading = 90.0f,    // degrees
    float acceleration = 0.0f // m/s²
) {
    std::vector<uint8_t> bsm;
    
    // Frame type byte (BSM = 0x01, flags in lower bits)
    bsm.push_back(0x10);  // 0x1 = BSM, 0x0 = basic flags
    
    // Timestamp (4 bytes, big-endian)
    bsm.push_back((timestamp_ms >> 24) & 0xFF);
    bsm.push_back((timestamp_ms >> 16) & 0xFF);
    bsm.push_back((timestamp_ms >> 8) & 0xFF);
    bsm.push_back(timestamp_ms & 0xFF);
    
    // Sender ID (6 bytes - MAC address)
    bsm.push_back(0x01);
    bsm.push_back(0x23);
    bsm.push_back(0x45);
    bsm.push_back(0x67);
    bsm.push_back(0x89);
    bsm.push_back(0xAB);
    
    // Sequence number
    bsm.push_back(0x42);
    
    // Latitude (4 bytes, big-endian signed int, units of 1/10,000,000 degree)
    int32_t lat_encoded = static_cast<int32_t>(latitude * 10000000.0);
    bsm.push_back((lat_encoded >> 24) & 0xFF);
    bsm.push_back((lat_encoded >> 16) & 0xFF);
    bsm.push_back((lat_encoded >> 8) & 0xFF);
    bsm.push_back(lat_encoded & 0xFF);
    
    // Longitude (4 bytes, big-endian signed int)
    int32_t lon_encoded = static_cast<int32_t>(longitude * 10000000.0);
    bsm.push_back((lon_encoded >> 24) & 0xFF);
    bsm.push_back((lon_encoded >> 16) & 0xFF);
    bsm.push_back((lon_encoded >> 8) & 0xFF);
    bsm.push_back(lon_encoded & 0xFF);
    
    // Speed (2 bytes, unsigned, 0.02 m/s per unit)
    uint16_t speed_raw = static_cast<uint16_t>(speed / 0.02f);
    bsm.push_back((speed_raw >> 8) & 0xFF);
    bsm.push_back(speed_raw & 0xFF);
    
    // Heading (2 bytes, unsigned, 0.01 degree per unit)
    uint16_t heading_raw = static_cast<uint16_t>(heading / 0.01f);
    bsm.push_back((heading_raw >> 8) & 0xFF);
    bsm.push_back(heading_raw & 0xFF);
    
    // Acceleration (2 bytes, signed, 0.01 m/s² per unit)
    int16_t accel_raw = static_cast<int16_t>(acceleration / 0.01f);
    bsm.push_back((accel_raw >> 8) & 0xFF);
    bsm.push_back(accel_raw & 0xFF);
    
    return bsm;
}

/**
 * Helper: Create a minimal valid COER SPaT (Signal Phase and Timing)
 */
std::vector<uint8_t> createMinimalSPaTVector(
    uint64_t timestamp_ms = 1710086400000ULL,
    uint8_t intersection_count = 1
) {
    std::vector<uint8_t> spat;
    
    // Frame type (SPaT = 0x02)
    spat.push_back(0x20);
    
    // Timestamp (4 bytes, big-endian)
    spat.push_back((timestamp_ms >> 24) & 0xFF);
    spat.push_back((timestamp_ms >> 16) & 0xFF);
    spat.push_back((timestamp_ms >> 8) & 0xFF);
    spat.push_back(timestamp_ms & 0xFF);
    
    // Intersection count
    spat.push_back(intersection_count);
    
    // Single intersection state
    // Intersection ID (4 bytes)
    spat.push_back(0x00);
    spat.push_back(0x00);
    spat.push_back(0x00);
    spat.push_back(0x01);
    
    // Phase info (4 bytes): current_phase, time_in_phase, next_phase, time_to_next
    spat.push_back(0x02);  // current_phase = 2
    spat.push_back(30);    // time_in_phase_sec = 30
    spat.push_back(0x03);  // next_phase = 3
    spat.push_back(45);    // time_to_next_phase_sec = 45
    
    return spat;
}

/**
 * Helper: Create a minimal valid COER PSM (Personal Safety Message)
 */
std::vector<uint8_t> createMinimalPSMVector(
    uint64_t timestamp_ms = 1710086400000ULL,
    double latitude = 37.7749,
    double longitude = -122.4194
) {
    std::vector<uint8_t> psm;
    
    // Frame type (PSM = 0x03)
    psm.push_back(0x30);
    
    // Timestamp (4 bytes, big-endian)
    psm.push_back((timestamp_ms >> 24) & 0xFF);
    psm.push_back((timestamp_ms >> 16) & 0xFF);
    psm.push_back((timestamp_ms >> 8) & 0xFF);
    psm.push_back(timestamp_ms & 0xFF);
    
    // User type (1 byte: 0=pedestrian, 1=cyclist)
    psm.push_back(0x00);  // pedestrian
    
    // Sender ID (6 bytes - MAC address)
    psm.push_back(0xAA);
    psm.push_back(0xBB);
    psm.push_back(0xCC);
    psm.push_back(0xDD);
    psm.push_back(0xEE);
    psm.push_back(0xFF);
    
    // Latitude (4 bytes)
    int32_t lat_encoded = static_cast<int32_t>(latitude * 10000000.0);
    psm.push_back((lat_encoded >> 24) & 0xFF);
    psm.push_back((lat_encoded >> 16) & 0xFF);
    psm.push_back((lat_encoded >> 8) & 0xFF);
    psm.push_back(lat_encoded & 0xFF);
    
    // Longitude (4 bytes)
    int32_t lon_encoded = static_cast<int32_t>(longitude * 10000000.0);
    psm.push_back((lon_encoded >> 24) & 0xFF);
    psm.push_back((lon_encoded >> 16) & 0xFF);
    psm.push_back((lon_encoded >> 8) & 0xFF);
    psm.push_back(lon_encoded & 0xFF);
    
    // Heading and speed (4 bytes)
    uint16_t heading_raw = 180;  // degrees * 100
    uint16_t speed_raw = 500;    // m/s * 50
    psm.push_back((heading_raw >> 8) & 0xFF);
    psm.push_back(heading_raw & 0xFF);
    psm.push_back((speed_raw >> 8) & 0xFF);
    psm.push_back(speed_raw & 0xFF);
    
    return psm;
}

/**
 * Helper: Wrap payload in COER container with proper length encoding
 * COER Length Encoding:
 *   - 0-127: Single byte (value as-is)
 *   - 128+: 0x81 + 1 byte, 0x82 + 2 bytes, 0x83 + 3 bytes, etc.
 */
std::vector<uint8_t> wrapInCOERContainer(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> coer;
    
    // COER Header (message type + flags)
    coer.push_back(0x00);  // Version 3, UNSIGNED message (no signature to simplify test)
    
    // Encode length in COER variable-length format
    size_t len = payload.size();
    if (len <= 127) {
        coer.push_back(static_cast<uint8_t>(len));
    } else if (len <= 255) {
        coer.push_back(0x81);  // 1 byte follows
        coer.push_back(static_cast<uint8_t>(len));
    } else {
        coer.push_back(0x82);  // 2 bytes follow
        coer.push_back((len >> 8) & 0xFF);
        coer.push_back(len & 0xFF);
    }
    
    // Append payload
    coer.insert(coer.end(), payload.begin(), payload.end());
    
    return coer;
}

}  // namespace

// ============================================================================
// Test Suite: End-to-End Message Pipeline
// ============================================================================

class V2XMessageIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug logging for this test suite
        COERDecoder::set_debug_logging(true);
    }
    
    void TearDown() override {
        COERDecoder::set_debug_logging(false);
    }
};

// ============================================================================
// Test: BSM Pipeline (Full End-to-End)
// ============================================================================

TEST_F(V2XMessageIntegrationTest, BSMPipeline_MinimalMessage) {
    // Create minimal BSM test vector
    auto bsm_payload = createMinimalBSMVector(
        1710086400000ULL,   // timestamp
        37.7749,            // latitude (San Francisco)
        -122.4194,          // longitude
        12.5f,              // speed (m/s)
        90.0f,              // heading (East)
        0.0f                // acceleration
    );
    
    std::cout << "BSM payload size: " << bsm_payload.size() << " bytes" << std::endl;
    
    // Wrap in COER container
    auto coer_message = wrapInCOERContainer(bsm_payload);
    std::cout << "COER message size: " << coer_message.size() << " bytes" << std::endl;
    
    // Step 1: Parse COER container
    ASSERT_NO_THROW({
        COERMessage raw_msg = COERDecoder::parse(coer_message);
        EXPECT_FALSE(raw_msg.payload.empty());
        std::cout << "✓ COER parsing successful" << std::endl;
    });
    
    COERMessage raw_msg = COERDecoder::parse(coer_message);
    
    // Step 2: Detect frame type
    MessageFrameType frame_type = V2XFrameDecoder::detect_frame_type(raw_msg.payload);
    EXPECT_EQ(frame_type, MessageFrameType::BSM);
    std::cout << "✓ Frame type detected: " << V2XFrameDecoder::frame_type_to_string(frame_type) << std::endl;
    
    // Step 3: Decode frame
    ASSERT_NO_THROW({
        DecodedV2XMessage decoded = V2XFrameDecoder::decode(raw_msg.payload, frame_type);
        EXPECT_EQ(decoded.frame_type, MessageFrameType::BSM);
        std::cout << "✓ Frame decoded successfully" << std::endl;
    });
    
    DecodedV2XMessage decoded = V2XFrameDecoder::decode(raw_msg.payload, frame_type);
    
    // Step 4: Verify decoded fields
    const auto& bsm = std::get<BasicSafetyMessage>(decoded.payload);
    
    EXPECT_EQ(bsm.sender_id, "01:23:45:67:89:AB");
    std::cout << "✓ Sender ID: " << bsm.sender_id << std::endl;
    
    EXPECT_EQ(bsm.sequence_num, 0x42);
    std::cout << "✓ Sequence number: " << (int)bsm.sequence_num << std::endl;
    
    // Position (allow small tolerance for encoding/decoding)
    EXPECT_NEAR(bsm.position.latitude, 37.7749, 0.00001);
    EXPECT_NEAR(bsm.position.longitude, -122.4194, 0.00001);
    std::cout << "✓ Position: (" << std::fixed << std::setprecision(6) 
              << bsm.position.latitude << ", " << bsm.position.longitude << ")" << std::endl;
    
    // Motion
    EXPECT_NEAR(bsm.motion.speed, 12.5f, 0.1f);
    EXPECT_NEAR(bsm.motion.heading, 90.0f, 0.1f);
    EXPECT_NEAR(bsm.motion.acceleration, 0.0f, 0.1f);
    std::cout << "✓ Motion: speed=" << bsm.motion.speed << " m/s, "
              << "heading=" << bsm.motion.heading << "°, "
              << "accel=" << bsm.motion.acceleration << " m/s²" << std::endl;
    
    std::cout << "\n✅ BSM Pipeline Test PASSED\n" << std::endl;
}

// ============================================================================
// Test: SPaT Pipeline
// ============================================================================

TEST_F(V2XMessageIntegrationTest, SPaTpipeline_IntersectionState) {
    auto spat_payload = createMinimalSPaTVector();
    auto coer_message = wrapInCOERContainer(spat_payload);
    
    // Parse → Detect → Decode
    COERMessage raw_msg = COERDecoder::parse(coer_message);
    MessageFrameType frame_type = V2XFrameDecoder::detect_frame_type(raw_msg.payload);
    
    EXPECT_EQ(frame_type, MessageFrameType::SPAT);
    std::cout << "✓ Frame type detected: SPAT" << std::endl;
    
    DecodedV2XMessage decoded = V2XFrameDecoder::decode(raw_msg.payload, frame_type);
    
    const auto& spat = std::get<SignalPhaseAndTiming>(decoded.payload);
    EXPECT_FALSE(spat.intersections.empty());
    
    const auto& intersection = spat.intersections[0];
    EXPECT_EQ(intersection.intersection_id, 1);
    EXPECT_EQ(intersection.current_phase, 2);
    EXPECT_EQ(intersection.time_in_phase_sec, 30);
    EXPECT_EQ(intersection.next_phase, 3);
    EXPECT_EQ(intersection.time_to_next_phase_sec, 45);
    
    std::cout << "✓ Intersection ID: " << intersection.intersection_id << std::endl;
    std::cout << "✓ Current phase: " << (int)intersection.current_phase 
              << " (" << (int)intersection.time_in_phase_sec << " sec remaining)" << std::endl;
    std::cout << "✓ Next phase: " << (int)intersection.next_phase 
              << " (in " << (int)intersection.time_to_next_phase_sec << " sec)" << std::endl;
    
    std::cout << "\n✅ SPaT Pipeline Test PASSED\n" << std::endl;
}

// ============================================================================
// Test: PSM Pipeline
// ============================================================================

TEST_F(V2XMessageIntegrationTest, PSMPipeline_PedestrianAlert) {
    auto psm_payload = createMinimalPSMVector();
    auto coer_message = wrapInCOERContainer(psm_payload);
    
    COERMessage raw_msg = COERDecoder::parse(coer_message);
    MessageFrameType frame_type = V2XFrameDecoder::detect_frame_type(raw_msg.payload);
    
    EXPECT_EQ(frame_type, MessageFrameType::PSM);
    std::cout << "✓ Frame type detected: PSM" << std::endl;
    
    DecodedV2XMessage decoded = V2XFrameDecoder::decode(raw_msg.payload, frame_type);
    
    const auto& psm = std::get<PersonalSafetyMessage>(decoded.payload);
    EXPECT_EQ(psm.sender_id, "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(psm.user_type, 0);  // pedestrian
    
    EXPECT_NEAR(psm.position.latitude, 37.7749, 0.00001);
    EXPECT_NEAR(psm.position.longitude, -122.4194, 0.00001);
    
    std::cout << "✓ Sender: " << psm.sender_id << " (pedestrian)" << std::endl;
    std::cout << "✓ Position: (" << psm.position.latitude << ", " << psm.position.longitude << ")" << std::endl;
    std::cout << "✓ Heading: " << psm.heading << "°, Speed: " << psm.speed << " m/s" << std::endl;
    
    std::cout << "\n✅ PSM Pipeline Test PASSED\n" << std::endl;
}

// ============================================================================
// Test: Error Cases
// ============================================================================

TEST_F(V2XMessageIntegrationTest, InvalidFrameType_ReturnsUnknown) {
    // Create message with invalid frame type byte
    std::vector<uint8_t> invalid_payload;
    invalid_payload.push_back(0xFF);  // Invalid frame type
    invalid_payload.insert(invalid_payload.end(), 100, 0x00);  // Padding
    
    auto coer_message = wrapInCOERContainer(invalid_payload);
    
    COERMessage raw_msg = COERDecoder::parse(coer_message);
    MessageFrameType frame_type = V2XFrameDecoder::detect_frame_type(raw_msg.payload);
    
    EXPECT_EQ(frame_type, MessageFrameType::UNKNOWN);
    std::cout << "✓ Invalid frame type correctly detected as UNKNOWN" << std::endl;
}

TEST_F(V2XMessageIntegrationTest, TruncatedMessage_ThrowsException) {
    std::vector<uint8_t> truncated = {0x10, 0x20, 0x30};  // Too short
    
    EXPECT_THROW({
        COERDecoder::parse(truncated);
    }, COERBufferException);
    
    std::cout << "✓ Truncated COER message correctly throws exception" << std::endl;
}

// ============================================================================
// Test: Performance Benchmark
// ============================================================================

TEST_F(V2XMessageIntegrationTest, BENCHMARK_ProcessingPipeline) {
    auto bsm_payload = createMinimalBSMVector();
    auto coer_message = wrapInCOERContainer(bsm_payload);
    
    // Warm-up
    for (int i = 0; i < 10; ++i) {
        COERMessage msg = COERDecoder::parse(coer_message);
        MessageFrameType type = V2XFrameDecoder::detect_frame_type(msg.payload);
        DecodedV2XMessage decoded = V2XFrameDecoder::decode(msg.payload, type);
    }
    
    // Benchmark: Full pipeline
    auto start = std::chrono::high_resolution_clock::now();
    int iterations = 10000;
    
    for (int i = 0; i < iterations; ++i) {
        COERMessage msg = COERDecoder::parse(coer_message);
        MessageFrameType type = V2XFrameDecoder::detect_frame_type(msg.payload);
        DecodedV2XMessage decoded = V2XFrameDecoder::decode(msg.payload, type);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_us = duration.count() / (double)iterations;
    double throughput = 1000000.0 / avg_us;  // Messages per second
    
    std::cout << "\nPerformance: " << std::fixed << std::setprecision(3)
              << avg_us << " µs/message, " 
              << throughput << " msg/sec ("
              << (int)throughput / 10 << " Hz @ 10x real-time)" << std::endl;
    
    // Should be fast enough for 10 Hz real-time (100ms per message minimum)
    EXPECT_LT(avg_us, 100000.0);  // < 100ms is plenty fast
}

// ============================================================================
// Test Summary and Instructions
// ============================================================================

class V2XMessageIntegrationTestSummary : public ::testing::Test {
};

TEST_F(V2XMessageIntegrationTestSummary, DISABLED_PrintTestVectorDocumentation) {
    // This is a documentation test (disabled by default)
    // Run with: --gtest_filter="*PrintTestVectorDocumentation"
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "V2X Message Processing Pipeline - Test Vector Documentation" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    std::cout << "\n1. BSM (Basic Safety Message) Vector:\n";
    auto bsm = createMinimalBSMVector();
    std::cout << "   - Size: " << bsm.size() << " bytes\n";
    std::cout << "   - Hex: ";
    for (auto b : bsm) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    std::cout << std::dec << "\n";
    
    std::cout << "\n2. SPaT (Signal Phase and Timing) Vector:\n";
    auto spat = createMinimalSPaTVector();
    std::cout << "   - Size: " << spat.size() << " bytes\n";
    std::cout << "   - Hex: ";
    for (auto b : spat) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    std::cout << std::dec << "\n";
    
    std::cout << "\n3. PSM (Personal Safety Message) Vector:\n";
    auto psm = createMinimalPSMVector();
    std::cout << "   - Size: " << psm.size() << " bytes\n";
    std::cout << "   - Hex: ";
    for (auto b : psm) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    std::cout << std::dec << "\n";
    
    std::cout << "\n" << std::string(80, '=') << "\n" << std::endl;
}
