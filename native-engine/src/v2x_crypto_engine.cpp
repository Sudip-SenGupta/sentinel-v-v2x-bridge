#include "v2x_crypto_engine.h"

#include <botan/botan.h>
#include <botan/pubkey.h>
#include <botan/ecdsa.h>
#include <botan/hash.h>
#include <botan/x509cert.h>
#include <botan/x509_ca.h>
#include <botan/der_enc.h>
#include <botan/asn1_time.h>
#include <botan/certstor.h>
#include <botan/x509path.h>
#include <botan/auto_rng.h>
#include <botan/pem.h>

#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <mutex>

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

namespace {

std::vector<uint8_t> encode_der_length(size_t length) {
    if (length <= 0x7F) {
        return {static_cast<uint8_t>(length)};
    }

    std::vector<uint8_t> bytes;
    while (length > 0) {
        bytes.insert(bytes.begin(), static_cast<uint8_t>(length & 0xFF));
        length >>= 8;
    }

    std::vector<uint8_t> encoded;
    encoded.push_back(static_cast<uint8_t>(0x80 | bytes.size()));
    encoded.insert(encoded.end(), bytes.begin(), bytes.end());
    return encoded;
}

std::vector<uint8_t> to_der_integer_component(const uint8_t* begin, size_t length) {
    while (length > 1 && *begin == 0x00) {
        ++begin;
        --length;
    }

    std::vector<uint8_t> component(begin, begin + length);
    if (!component.empty() && (component.front() & 0x80) != 0) {
        component.insert(component.begin(), 0x00);
    }
    return component;
}

std::vector<uint8_t> ieee1363_p256_to_der(const std::vector<uint8_t>& signature) {
    if (signature.size() != 64) {
        return {};
    }

    auto r = to_der_integer_component(signature.data(), 32);
    auto s = to_der_integer_component(signature.data() + 32, 32);

    std::vector<uint8_t> body;
    body.push_back(0x02);
    auto r_len = encode_der_length(r.size());
    body.insert(body.end(), r_len.begin(), r_len.end());
    body.insert(body.end(), r.begin(), r.end());

    body.push_back(0x02);
    auto s_len = encode_der_length(s.size());
    body.insert(body.end(), s_len.begin(), s_len.end());
    body.insert(body.end(), s.begin(), s.end());

    std::vector<uint8_t> der;
    der.push_back(0x30);
    auto seq_len = encode_der_length(body.size());
    der.insert(der.end(), seq_len.begin(), seq_len.end());
    der.insert(der.end(), body.begin(), body.end());
    return der;
}

std::mutex g_trusted_root_mutex;
std::vector<uint8_t> g_trusted_root_der;

std::chrono::system_clock::time_point resolve_validation_time(uint64_t current_time_unix) {
    if(current_time_unix == 0) {
        return std::chrono::system_clock::now();
    }

    return std::chrono::system_clock::from_time_t(static_cast<std::time_t>(current_time_unix));
}

}  // namespace

namespace sentinel {
namespace v2x {

/**
 * @brief Private implementation class (pimpl pattern)
 * Botan 2.19 wrapper for cryptographic operations
 */
class V2XCryptoEngine::Impl {
public:
    Impl() : rng_(std::make_unique<Botan::AutoSeeded_RNG>()) {
        {
            std::lock_guard<std::mutex> lock(g_trusted_root_mutex);
            if(!g_trusted_root_der.empty()) {
                Botan::DataSource_Memory ds(g_trusted_root_der.data(), g_trusted_root_der.size());
                root_ca_ = std::make_unique<Botan::X509_Certificate>(ds);
            }
        }
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
        
        Botan::DataSource_Memory ds(root_ca_der.data(), root_ca_der.size());
        auto root_ca = std::make_unique<Botan::X509_Certificate>(ds);
        
        if (!root_ca->is_CA_cert() && !root_ca->is_self_signed()) {
            LOGE("Provided certificate is neither a CA certificate nor a self-signed trust anchor");
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(g_trusted_root_mutex);
            g_trusted_root_der = root_ca_der;
        }
        pimpl_->root_ca_ = std::move(root_ca);
        
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
        LOGD("Verifying ECDSA signature (msg: %zu, sig: %zu bytes, sig[0]=0x%02x, key: %zu bytes)",
             message.size(), signature.size(), signature.size() > 0 ? signature[0] : 0, public_key.size());
        
        // Load the public key from DER
        try {
            Botan::DataSource_Memory key_ds(public_key.data(), public_key.size());
            auto pk = Botan::X509::load_key(key_ds);
            LOGD("Public key loaded successfully from X.509 DER");
            
            // Create ECDSA verifier with SHA-256
            // Botan uses EMSA1 (PSS with SHA-256) for ECDSA signature verification
            Botan::PK_Verifier verifier(*pk, "EMSA1(SHA-256)");
            
            // Try DER signature first (most common)
            LOGD("Attempting DER signature verification (sig size: %zu)", signature.size());
            result.valid = verifier.verify_message(message.data(), message.size(),
                                                signature.data(), signature.size());
            
            // If DER failed but signature is 64 bytes (P1363 format), try conversion
            if (!result.valid && signature.size() == 64) {
                LOGD("DER verification failed with 64-byte signature. Trying P1363 conversion...");
                auto der_signature = ieee1363_p256_to_der(signature);
                if (!der_signature.empty()) {
                    LOGD("Converted P1363 to DER (%zu bytes), retrying verification", der_signature.size());
                    result.valid = verifier.verify_message(message.data(), message.size(),
                                                        der_signature.data(), der_signature.size());
                    if (result.valid) {
                        LOGD("P1363 conversion successful!");
                    }
                }
            }
            
            result.algorithm = "EMSA1(SHA-256)";
            
            if (result.valid) {
                LOGI("ECDSA signature verification successful");
            } else {
                LOGE("ECDSA signature verification failed (all attempts)");
                result.error_message = "Signature verification failed";
            }
        } catch (const std::exception& key_error) {
            LOGE("Failed to load/process public key: %s", key_error.what());
            result.error_message = std::string("Key loading failed: ") + key_error.what();
        }
        
    } catch (const std::exception& e) {
        LOGE("ECDSA verification exception: %s", e.what());
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
    uint64_t current_time_unix) {
    
    try {
        if (certificate_chain.empty()) {
            LOGE("Empty certificate chain");
            return false;
        }
        
        LOGI("Validating certificate chain (%zu certificates)", certificate_chain.size());

        std::vector<Botan::X509_Certificate> parsed_chain;
        parsed_chain.reserve(certificate_chain.size());

        for (size_t i = 0; i < certificate_chain.size(); ++i) {
            try {
                Botan::DataSource_Memory ds(certificate_chain[i].data(), certificate_chain[i].size());
                parsed_chain.emplace_back(ds);
                LOGD("Certificate %zu parsed successfully", i);
            } catch (const std::exception& e) {
                LOGE("Certificate %zu parsing failed: %s", i, e.what());
                return false;
            }
        }

        if (!pimpl_->root_ca_) {
            LOGE("Certificate chain validation failed: no trusted root CA configured");
            return false;
        }

        for (size_t i = 0; i + 1 < parsed_chain.size(); ++i) {
            if (!(parsed_chain[i].issuer_dn() == parsed_chain[i + 1].subject_dn())) {
                LOGE("Certificate chain validation failed: certificate %zu issuer does not match certificate %zu subject", i, i + 1);
                return false;
            }
            if (!parsed_chain[i + 1].is_CA_cert()) {
                LOGE("Certificate chain validation failed: certificate %zu is not a CA", i + 1);
                return false;
            }
        }

        if (!(parsed_chain.back() == *pimpl_->root_ca_)) {
            LOGE("Certificate chain validation failed: final certificate does not match configured trust anchor");
            return false;
        }

        Botan::Certificate_Store_In_Memory trust_store;
        trust_store.add_certificate(*pimpl_->root_ca_);
        LOGD("Using initialized root CA as trust anchor");

        std::vector<Botan::X509_Certificate> end_certs = parsed_chain;

        const auto validation_time = resolve_validation_time(current_time_unix);
        const Botan::Path_Validation_Restrictions restrictions(false, 110);
        const auto validation_result = Botan::x509_path_validate(
            end_certs,
            restrictions,
            trust_store,
            "",
            Botan::Usage_Type::UNSPECIFIED,
            validation_time
        );

        if (!validation_result.successful_validation()) {
            LOGE("Certificate chain validation failed: %s", validation_result.result_string().c_str());
            const auto warnings = validation_result.warnings_string();
            if (!warnings.empty()) {
                LOGE("Certificate chain warnings: %s", warnings.c_str());
            }
            return false;
        }

        if (!validation_result.no_warnings()) {
            LOGD("Certificate chain validated with warnings: %s", validation_result.warnings_string().c_str());
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

bool V2XCryptoEngine::is_certificate_time_valid(
    const std::vector<uint8_t>& cert_der,
    uint64_t current_time_unix) {
    
    try {
        Botan::DataSource_Memory ds(cert_der.data(), cert_der.size());
        Botan::X509_Certificate cert(ds);
        const auto validation_time = resolve_validation_time(current_time_unix);

        const bool valid = cert.not_before().to_std_timepoint() <= validation_time &&
                           validation_time <= cert.not_after().to_std_timepoint();

        LOGD("Certificate validity check performed: %s", valid ? "valid" : "expired or not yet valid");
        return valid;
        
    } catch (const std::exception& e) {
        LOGE("Certificate validity check error: %s", e.what());
        return false;
    }
}

void V2XCryptoEngine::clear_trusted_root_ca() {
    LOGD("Clearing trusted root CA");
    pimpl_->root_ca_.reset();
    std::lock_guard<std::mutex> lock(g_trusted_root_mutex);
    g_trusted_root_der.clear();
}

void V2XCryptoEngine::cleanup() {
    LOGD("Cleaning up V2XCryptoEngine");
    clear_trusted_root_ca();
}

std::string V2XCryptoEngine::get_botan_version() {
    return std::string(Botan::version_cstr());
}

} // namespace v2x
} // namespace sentinel
