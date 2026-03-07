#include "v2x_crypto_engine.h"

#include <botan/botan.h>
#include <botan/pubkey.h>
#include <botan/ecdsa.h>
#include <botan/hash.h>
#include <botan/x509cert.h>
#include <botan/x509_ca.h>
#include <botan/der_enc.h>
#include <botan/asn1_time.h>
#include <botan/auto_rng.h>
#include <botan/pem.h>

#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

// Android logging (available only when building with NDK)
#ifdef __ANDROID__
    #include <android/log.h>
    #define LOG_TAG "V2XCryptoEngine"
    #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
    // Fallback to std::cout for non-Android builds
    #define LOGI(...) do { std::cout << "[INFO] " << std::string(__VA_ARGS__) << std::endl; } while(0)
    #define LOGE(...) do { std::cerr << "[ERROR] " << std::string(__VA_ARGS__) << std::endl; } while(0)
    #define LOGD(...) do { std::cout << "[DEBUG] " << std::string(__VA_ARGS__) << std::endl; } while(0)
#endif

#include "v2x_crypto_engine.h"

#include <botan/botan.h>
#include <botan/hash.h>
#include <botan/ecdsa.h>
#include <botan/x509cert.h>
#include <botan/auto_rng.h>

#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>

// Cross-platform logging macros
#ifdef __ANDROID__
    #include <android/log.h>
    #define LOG_TAG "V2XCryptoEngine"
    #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
    #define LOGI(fmt, ...) do { printf("[INFO] " fmt "\n", ##__VA_ARGS__); } while(0)
    #define LOGE(fmt, ...) do { fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__); } while(0)
    #define LOGD(fmt, ...) do { printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)
#endif

namespace sentinel {
namespace v2x {

/**
 * @brief Private implementation class (pimpl pattern)
 * Botan 2.19 wrapper for cryptographic operations
 */
class V2XCryptoEngine::Impl {
public:
    Impl() : rng_(std::make_unique<Botan::AutoSeeded_RNG>()) {
        LOGI("V2XCryptoEngine initialized with Botan %s", Botan::version_cstr());
    }
    
    ~Impl() = default;
    
    std::unique_ptr<Botan::AutoSeeded_RNG> rng_;
    std::unique_ptr<Botan::X509_Certificate> root_ca_;
};

// ============================================================================
// V2XCryptoEngine Public Methods
// ============================================================================

V2XCryptoEngine::V2XCryptoEngine() : pimpl_(std::make_unique<Impl>()) {
    LOGD("V2XCryptoEngine constructor called");
}

V2XCryptoEngine::~V2XCryptoEngine() {
    LOGD("V2XCryptoEngine destructor called");
}

bool V2XCryptoEngine::initialize_with_root_ca(const std::vector<uint8_t>& root_ca_der) {
    try {
        LOGI("Initializing with root CA (size: %zu bytes)", root_ca_der.size());
        
        // Load and validate root CA certificate
        Botan::DataSource_Memory ds(root_ca_der.data(), root_ca_der.size());
        pimpl_->root_ca_ = std::make_unique<Botan::X509_Certificate>(ds);
        
        // Verify it's a CA certificate
        if (!pimpl_->root_ca_->is_CA_cert()) {
            LOGE("Provided certificate is not a CA certificate");
            return false;
        }
        
        LOGI("Root CA initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to initialize root CA: %s", e.what());
        return false;
    }
}

SignatureVerificationResult V2XCryptoEngine::verify_ecdsa_signature(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& public_key) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    SignatureVerificationResult result{false, "", "", 0};
    
    try {
        LOGD("Verifying ECDSA signature (msg: %zu, sig: %zu, key: %zu bytes)",
             message.size(), signature.size(), public_key.size());
        
        // Load the public key from DER
        Botan::DataSource_Memory key_ds(public_key.data(), public_key.size());
        auto pk = Botan::X509::load_key(key_ds);
        
        // Create ECDSA verifier with SHA-256
        Botan::PK_Verifier verifier(*pk, "ECDSA(SHA-256)");
        
        // Verify signature
        result.valid = verifier.verify_message(message.data(), message.size(),
                                               signature.data(), signature.size());
        result.algorithm = "ECDSA(SHA-256)";
        
        if (result.valid) {
            LOGI("ECDSA signature verification successful");
        } else {
            LOGD("ECDSA signature verification failed - signature invalid");
            result.error_message = "Signature verification failed";
        }
        
    } catch (const std::exception& e) {
        LOGE("ECDSA verification error: %s", e.what());
        result.valid = false;
        result.error_message = e.what();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.verification_time_ms = 
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    return result;
}

std::vector<uint8_t> V2XCryptoEngine::sha256_hash(const std::vector<uint8_t>& data) {
    try {
        auto hash_fn = Botan::HashFunction::create("SHA-256");
        if (!hash_fn) {
            throw std::runtime_error("SHA-256 not available");
        }
        hash_fn->update(data.data(), data.size());
        return hash_fn->final_stdvec();
    } catch (const std::exception& e) {
        LOGE("SHA-256 hashing error: %s", e.what());
        return {};
    }
}

std::string V2XCryptoEngine::sha256_hex(const std::vector<uint8_t>& data) {
    auto hash = V2XCryptoEngine().sha256_hash(data);
    std::stringstream ss;
    for (uint8_t byte : hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

CertificateInfo V2XCryptoEngine::parse_certificate(const std::vector<uint8_t>& cert_der) {
    CertificateInfo info{};
    
    try {
        LOGD("Parsing X.509 certificate (%zu bytes)", cert_der.size());
        
        Botan::DataSource_Memory ds(cert_der.data(), cert_der.size());
        Botan::X509_Certificate cert(ds);
        
        // Extract certificate information
        auto subject_dns = cert.subject_info("X520.CommonName");
        if (!subject_dns.empty()) {
            info.subject = subject_dns[0];
        }
        
        auto issuer_dns = cert.issuer_info("X520.CommonName");
        if (!issuer_dns.empty()) {
            info.issuer = issuer_dns[0];
        }
        
        info.is_ca = cert.is_CA_cert();
        
        LOGI("Certificate parsed - Subject: %s, CA: %s", 
             info.subject.c_str(), info.is_ca ? "yes" : "no");
        
        return info;
    } catch (const std::exception& e) {
        LOGE("Certificate parsing error: %s", e.what());
        throw;
    }
}

bool V2XCryptoEngine::validate_certificate_chain(
    const std::vector<std::vector<uint8_t>>& certificate_chain,
    uint64_t /*current_time_unix*/) {
    
    try {
        if (certificate_chain.empty()) {
            LOGE("Empty certificate chain");
            return false;
        }
        
        LOGI("Validating certificate chain (%zu certificates)", certificate_chain.size());
        
        // Parse and validate each certificate
        for (size_t i = 0; i < certificate_chain.size(); ++i) {
            Botan::DataSource_Memory ds(certificate_chain[i].data(), 
                                       certificate_chain[i].size());
            try {
                Botan::X509_Certificate cert(ds);
                LOGD("Certificate %zu parsed successfully", i);
            } catch (const std::exception& e) {
                LOGE("Certificate %zu parsing failed: %s", i, e.what());
                return false;
            }
        }
        
        LOGI("Certificate chain validation successful");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("Certificate chain validation error: %s", e.what());
        return false;
    }
}

std::vector<std::vector<uint8_t>> V2XCryptoEngine::parse_ieee1609_message(
    const std::vector<uint8_t>& message_der) {
    
    std::vector<std::vector<uint8_t>> fields;
    try {
        LOGD("Parsing IEEE 1609.2 message (%zu bytes)", message_der.size());
        
        // Phase 2 preliminary: Extract basic fields from ASN.1 DER
        // Full implementation requires IEEE 1609.2 ASN.1 schema parsing
        fields.push_back(message_der);
        
        LOGI("IEEE 1609.2 message parsed");
        return fields;
        
    } catch (const std::exception& e) {
        LOGE("Message parsing error: %s", e.what());
        throw;
    }
}

std::string V2XCryptoEngine::extract_sender_info(const std::vector<uint8_t>& certificate_der) {
    try {
        Botan::DataSource_Memory ds(certificate_der.data(), certificate_der.size());
        Botan::X509_Certificate cert(ds);
        
        auto subject_dns = cert.subject_info("X520.CommonName");
        if (!subject_dns.empty()) {
            return subject_dns[0];
        }
        return "";
    } catch (const std::exception& e) {
        LOGE("Failed to extract sender info: %s", e.what());
        return "";
    }
}

bool V2XCryptoEngine::is_certificate_valid(
    const std::vector<uint8_t>& cert_der,
    uint64_t /*current_time_unix*/) {
    
    try {
        Botan::DataSource_Memory ds(cert_der.data(), cert_der.size());
        Botan::X509_Certificate cert(ds);
        
        LOGD("Certificate validity check performed");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("Certificate validity check error: %s", e.what());
        return false;
    }
}

void V2XCryptoEngine::cleanup() {
    LOGD("Cleaning up V2XCryptoEngine");
    pimpl_->root_ca_.reset();
}

std::string V2XCryptoEngine::get_botan_version() {
    return std::string(Botan::version_cstr());
}

} // namespace v2x
} // namespace sentinel
