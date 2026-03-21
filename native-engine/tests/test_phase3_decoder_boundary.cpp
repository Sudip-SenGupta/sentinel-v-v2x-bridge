#include <gtest/gtest.h>

#include <iostream>
#include <vector>
#include <string>

#include "v2x_coer_decoder.h"
#include "v2x_frame_decoder.h"
#include "v2x_message_processor.h"

using namespace sentinel::v2x;

namespace {

std::vector<uint8_t> wrapInCOERContainer(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> coer;
    coer.push_back(0x00);

    const size_t len = payload.size();
    if(len <= 127) {
        coer.push_back(static_cast<uint8_t>(len));
    } else if(len <= 255) {
        coer.push_back(0x81);
        coer.push_back(static_cast<uint8_t>(len));
    } else {
        coer.push_back(0x82);
        coer.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        coer.push_back(static_cast<uint8_t>(len & 0xFF));
    }

    coer.insert(coer.end(), payload.begin(), payload.end());
    return coer;
}

std::vector<uint8_t> createMinimalBSMPayload() {
    std::vector<uint8_t> payload;
    payload.push_back(0x10);

    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x2A);

    payload.push_back(0x01);
    payload.push_back(0x23);
    payload.push_back(0x45);
    payload.push_back(0x67);
    payload.push_back(0x89);
    payload.push_back(0xAB);

    payload.push_back(0x42);

    payload.push_back(0x16);
    payload.push_back(0x83);
    payload.push_back(0xFE);
    payload.push_back(0x08);

    payload.push_back(0xB7);
    payload.push_back(0x08);
    payload.push_back(0x48);
    payload.push_back(0x30);

    payload.push_back(0x02);
    payload.push_back(0x71);

    payload.push_back(0x23);
    payload.push_back(0x28);

    payload.push_back(0x00);
    payload.push_back(0x00);

    return payload;
}

}  // namespace

TEST(Phase3DecoderBoundaryTest, EnvelopeParsingCanSucceedForUnknownFrameType) {
    const std::vector<uint8_t> payload = {0xF0, 0x01, 0x02};
    const auto raw_message = wrapInCOERContainer(payload);

    const COERMessage parsed = COERDecoder::parse(raw_message);

    EXPECT_TRUE(COERDecoder::validate_structure(parsed));
    const auto& parsed_payload = COERDecoder::get_payload(parsed);
    EXPECT_EQ(parsed_payload, payload);
    EXPECT_EQ(V2XFrameDecoder::detect_frame_type(parsed_payload), MessageFrameType::UNKNOWN);
}

TEST(Phase3DecoderBoundaryTest, FrameDetectionFailureRemainsAFrameLayerConcern) {
    const std::vector<uint8_t> payload = {0x10, 0x00};
    const auto raw_message = wrapInCOERContainer(payload);

    const COERMessage parsed = COERDecoder::parse(raw_message);

    EXPECT_TRUE(COERDecoder::validate_structure(parsed));
    const auto& parsed_payload = COERDecoder::get_payload(parsed);
    EXPECT_THROW(V2XFrameDecoder::detect_frame_type(parsed_payload), V2XFrameBufferException);
}

TEST(Phase3DecoderBoundaryTest, ProcessorReturnsDecodedOutputForUnsignedBSM) {
    const auto raw_message = wrapInCOERContainer(createMinimalBSMPayload());

    const MessageVerificationResult result = V2XMessageProcessor::process_message(raw_message);

    ASSERT_TRUE(result.is_valid);
    EXPECT_TRUE(result.coer_parse_ok);
    EXPECT_TRUE(result.payload_structure_ok);
    EXPECT_TRUE(result.signature_valid);
    EXPECT_TRUE(result.chain_valid);
    EXPECT_EQ(result.frame_type, MessageFrameType::BSM);
    ASSERT_TRUE(result.decoded_message.has_value());
    EXPECT_EQ(result.decoded_message->frame_type, MessageFrameType::BSM);
    EXPECT_FALSE(result.decoded_message->is_verified);
    EXPECT_EQ(result.decoded_message->issuer_name, "unsigned");
    EXPECT_EQ(result.decoded_message->get_bsm().sequence_num, 0x42);
}

TEST(Phase3DecoderBoundaryTest, ProcessorFailsClosedWhenFrameDecodeThrows) {
    /**
     * @test Frame Decode Failure -> Processor Fail-Closed
     *
     * Verifies the boundary between decoder and frame layers:
     * 1. Envelope parses successfully
     * 2. Structure validation passes
     * 3. Frame decoder detects UNKNOWN frame type
     * 4. Frame decode throws V2XFrameDecodeException
     * 5. Processor catches exception and returns fail-closed with:
     *    - is_valid = false
     *    - coer_parse_ok = true
     *    - payload_structure_ok = true
     *    - error message contains "Frame decode failed"
     *
     * This ensures that frame layer failures don't leak as codec errors
     * or leave partial/invalid state.
     */
    const std::vector<uint8_t> payload = {0xF0, 0x01, 0x02};
    const auto raw_message = wrapInCOERContainer(payload);

    // Step 1: Verify envelope parses
    const COERMessage parsed = COERDecoder::parse(raw_message);
    EXPECT_TRUE(COERDecoder::validate_structure(parsed));
    const auto& parsed_payload = COERDecoder::get_payload(parsed);
    EXPECT_EQ(parsed_payload, payload);

    // Step 2: Verify frame detection returns UNKNOWN
    EXPECT_EQ(V2XFrameDecoder::detect_frame_type(parsed_payload), MessageFrameType::UNKNOWN);

    // Step 3: Verify frame decode throws V2XFrameDecodeException
    EXPECT_THROW(
        V2XFrameDecoder::decode(parsed_payload, MessageFrameType::UNKNOWN),
        V2XFrameDecodeException
    );

    // Step 4: Verify processor returns fail-closed
    const MessageVerificationResult result = V2XMessageProcessor::process_message(raw_message);

    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.coer_parse_ok);
    EXPECT_TRUE(result.payload_structure_ok);
    EXPECT_TRUE(result.error_message.find("Frame decode failed") != std::string::npos)
        << "Error message should contain 'Frame decode failed', got: " << result.error_message;
    EXPECT_FALSE(result.decoded_message.has_value());

    std::cout << "\n[BOUNDARY TEST: Frame Decode Failure]\n"
              << "  COER Parse: OK\n"
              << "  Structure Validation: OK\n"
              << "  Frame Detection: UNKNOWN\n"
              << "  Frame Decode: THREW (expected)\n"
              << "  Processor Result: FAIL-CLOSED\n"
              << "  Error: " << result.error_message << "\n";
}
