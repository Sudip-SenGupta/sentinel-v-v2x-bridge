#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "test_vectors.h"
#include "v2x_message_processor.h"

using namespace sentinel::v2x;
using namespace sentinel::v2x::test;

namespace {

std::vector<uint8_t> hex_to_bytes_local(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        const std::string byte_string = hex.substr(i, 2);
        bytes.push_back(static_cast<uint8_t>(std::strtoul(byte_string.c_str(), nullptr, 16)));
    }
    return bytes;
}

class CryptoHookGuard {
public:
    CryptoHookGuard(
        V2XMessageProcessor::SignatureVerifierHook signature_hook,
        V2XMessageProcessor::ChainValidatorHook chain_hook) {
        V2XMessageProcessor::set_test_crypto_hooks(std::move(signature_hook), std::move(chain_hook));
    }

    ~CryptoHookGuard() {
        V2XMessageProcessor::clear_test_crypto_hooks();
    }
};

}  // namespace

TEST(Phase4CryptoBoundaryTest, InvalidSignatureFailsClosedBeforeChainValidation) {
    const std::vector<uint8_t> raw_message = hex_to_bytes_local(SIGNED_TYPICAL_BSM_HEX);

    bool signature_hook_called = false;
    bool chain_hook_called = false;

    CryptoHookGuard hook_guard(
        [&](const std::vector<uint8_t>& payload,
            const std::vector<uint8_t>& signature,
            const std::vector<uint8_t>& issuer_cert) {
            signature_hook_called = true;
            EXPECT_FALSE(payload.empty());
            EXPECT_FALSE(signature.empty());
            EXPECT_FALSE(issuer_cert.empty());
            return SignatureVerificationResult{false, "forced invalid signature", "test-hook", 0};
        },
        [&](const std::vector<std::vector<uint8_t>>&, uint64_t) {
            chain_hook_called = true;
            return true;
        }
    );

    const MessageVerificationResult result = V2XMessageProcessor::process_message(raw_message);

    EXPECT_TRUE(signature_hook_called);
    EXPECT_FALSE(chain_hook_called);
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.coer_parse_ok);
    EXPECT_TRUE(result.payload_structure_ok);
    EXPECT_FALSE(result.signature_valid);
    EXPECT_FALSE(result.chain_valid);
    EXPECT_FALSE(result.payload.empty());
    EXPECT_FALSE(result.signature.empty());
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("Signature verification failed"), std::string::npos);
    EXPECT_FALSE(result.decoded_message.has_value());
}
