/**
 * @file v2x_jni_crypto.cpp
 * @brief JNI Bindings for V2X Crypto Engine
 *
 * Phase 6A: ECDSA signature verification, SHA-256 hashing, and certificate validation
 * Wraps v2x_crypto_engine.cpp functions for Kotlin/Android access
 *
 * @author Sentinel V2X Bridge
 * @date March 12, 2026
 */

#include <jni.h>
#include <android/log.h>
#include <memory>
#include "v2x_crypto_engine.h"

#define TAG "V2X-Crypto-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

using namespace sentinel::v2x;

/**
 * Global crypto engine instance (singleton)
 * Initialized once, used for all JNI calls
 */
static std::unique_ptr<V2XCryptoEngine> g_crypto_engine = nullptr;

// ============================================================================
// JNI: Crypto Engine Initialization
// ============================================================================

/**
 * Java: V2X.cryptoInitialize()
 * Returns: true if Botan initialized successfully
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_sentinel_v2x_V2X_cryptoInitialize(JNIEnv* env, jclass clazz) {
    try {
        LOGI("Initializing V2X Crypto Engine");
        if (!g_crypto_engine) {
            g_crypto_engine = std::make_unique<V2XCryptoEngine>();
        }
        LOGI("Crypto engine initialized: %s", g_crypto_engine->get_botan_version().c_str());
        return JNI_TRUE;
    } catch (const std::exception& e) {
        LOGE("Crypto initialization failed: %s", e.what());
        return JNI_FALSE;
    }
}

// ============================================================================
// JNI: SHA-256 Hashing
// ============================================================================

/**
 * Java: V2X.sha256Hash(data: ByteArray): ByteArray
 * Computes SHA-256 hash of input data
 */
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_sentinel_v2x_V2X_sha256Hash(JNIEnv* env, jclass clazz, jbyteArray data) {
    try {
        if (!g_crypto_engine) {
            Java_com_sentinel_v2x_V2X_cryptoInitialize(env, clazz);
        }

        // Convert Java byte array to C++
        jsize len = env->GetArrayLength(data);
        jbyte* bytes = env->GetByteArrayElements(data, nullptr);
        
        std::vector<uint8_t> input(bytes, bytes + len);
        env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);

        // Hash
        auto hash = g_crypto_engine->sha256_hash(input);

        // Convert result to Java byte array
        jbyteArray result = env->NewByteArray(hash.size());
        env->SetByteArrayRegion(result, 0, hash.size(), (jbyte*)hash.data());

        LOGD("SHA-256 hash computed for %d bytes", len);
        return result;
    } catch (const std::exception& e) {
        LOGE("SHA-256 error: %s", e.what());
        return nullptr;
    }
}

/**
 * Java: V2X.sha256Hex(data: ByteArray): String
 * Returns SHA-256 as hex string (e.g., "a665a45920422f9d417e4867efdc4fb8a04a1f3fff...")
 */
extern "C" JNIEXPORT jstring JNICALL
Java_com_sentinel_v2x_V2X_sha256Hex(JNIEnv* env, jclass clazz, jbyteArray data) {
    try {
        if (!g_crypto_engine) {
            Java_com_sentinel_v2x_V2X_cryptoInitialize(env, clazz);
        }

        jsize len = env->GetArrayLength(data);
        jbyte* bytes = env->GetByteArrayElements(data, nullptr);
        
        std::vector<uint8_t> input(bytes, bytes + len);
        env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);

        // Get hex string
        auto hex = g_crypto_engine->sha256_hex(input);

        LOGD("SHA-256 hex computed: %.16s...", hex.c_str());
        return env->NewStringUTF(hex.c_str());
    } catch (const std::exception& e) {
        LOGE("SHA-256 hex error: %s", e.what());
        return nullptr;
    }
}

// ============================================================================
// JNI: ECDSA Signature Verification
// ============================================================================

/**
 * Java: V2X.verifySignature(
 *   message: ByteArray,
 *   signature: ByteArray,
 *   publicKey: ByteArray
 * ): Boolean
 * 
 * Verifies ECDSA(SHA-256) signature
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_sentinel_v2x_V2X_verifySignature(
    JNIEnv* env, jclass clazz,
    jbyteArray message,
    jbyteArray signature,
    jbyteArray publicKey) {
    try {
        if (!g_crypto_engine) {
            Java_com_sentinel_v2x_V2X_cryptoInitialize(env, clazz);
        }

        // Convert message
        jsize msg_len = env->GetArrayLength(message);
        jbyte* msg_bytes = env->GetByteArrayElements(message, nullptr);
        std::vector<uint8_t> msg_data(msg_bytes, msg_bytes + msg_len);
        env->ReleaseByteArrayElements(message, msg_bytes, JNI_ABORT);

        // Convert signature
        jsize sig_len = env->GetArrayLength(signature);
        jbyte* sig_bytes = env->GetByteArrayElements(signature, nullptr);
        std::vector<uint8_t> sig_data(sig_bytes, sig_bytes + sig_len);
        env->ReleaseByteArrayElements(signature, sig_bytes, JNI_ABORT);

        // Convert public key
        jsize key_len = env->GetArrayLength(publicKey);
        jbyte* key_bytes = env->GetByteArrayElements(publicKey, nullptr);
        std::vector<uint8_t> key_data(key_bytes, key_bytes + key_len);
        env->ReleaseByteArrayElements(publicKey, key_bytes, JNI_ABORT);

        // Verify
        auto result = g_crypto_engine->verify_ecdsa_signature(msg_data, sig_data, key_data);

        LOGI("ECDSA signature verification: %s (time: %ldms)",
             result.valid ? "VALID" : "INVALID", result.verification_time_ms);

        return result.valid ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("Signature verification error: %s", e.what());
        return JNI_FALSE;
    }
}

// ============================================================================
// JNI: Certificate Operations
// ============================================================================

/**
 * Java: V2X.isValidCertificate(certDER: ByteArray): Boolean
 * Basic certificate validity check
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_sentinel_v2x_V2X_isValidCertificate(JNIEnv* env, jclass clazz, jbyteArray certDER) {
    try {
        if (!g_crypto_engine) {
            Java_com_sentinel_v2x_V2X_cryptoInitialize(env, clazz);
        }

        jsize len = env->GetArrayLength(certDER);
        jbyte* bytes = env->GetByteArrayElements(certDER, nullptr);
        std::vector<uint8_t> cert_data(bytes, bytes + len);
        env->ReleaseByteArrayElements(certDER, bytes, JNI_ABORT);

        bool valid = g_crypto_engine->is_certificate_valid(cert_data, 0);

        LOGD("Certificate validity check: %s", valid ? "VALID" : "INVALID");
        return valid ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("Certificate validity check error: %s", e.what());
        return JNI_FALSE;
    }
}

/**
 * Java: V2X.validateCertificateChain(certificates: Array<ByteArray>): Boolean
 * Validates a chain of certificates
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_sentinel_v2x_V2X_validateCertificateChain(
    JNIEnv* env, jclass clazz,
    jobjectArray certificateChain) {
    try {
        if (!g_crypto_engine) {
            Java_com_sentinel_v2x_V2X_cryptoInitialize(env, clazz);
        }

        // Convert certificate array
        jsize chain_len = env->GetArrayLength(certificateChain);
        std::vector<std::vector<uint8_t>> chain;

        for (jsize i = 0; i < chain_len; i++) {
            jbyteArray cert_jarray = (jbyteArray)env->GetObjectArrayElement(certificateChain, i);
            jsize cert_len = env->GetArrayLength(cert_jarray);
            jbyte* cert_bytes = env->GetByteArrayElements(cert_jarray, nullptr);
            
            std::vector<uint8_t> cert_data(cert_bytes, cert_bytes + cert_len);
            chain.push_back(cert_data);
            
            env->ReleaseByteArrayElements(cert_jarray, cert_bytes, JNI_ABORT);
            env->DeleteLocalRef(cert_jarray);
        }

        // Validate chain
        bool valid = g_crypto_engine->validate_certificate_chain(chain, 0);

        LOGI("Certificate chain validation (%d certs): %s", chain_len, valid ? "VALID" : "INVALID");
        return valid ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("Certificate chain validation error: %s", e.what());
        return JNI_FALSE;
    }
}

// ============================================================================
// JNI: Version Info
// ============================================================================

/**
 * Java: V2X.getCryptoBotanVersion(): String
 * Returns Botan version string
 */
extern "C" JNIEXPORT jstring JNICALL
Java_com_sentinel_v2x_V2X_getCryptoBotanVersion(JNIEnv* env, jclass clazz) {
    try {
        if (!g_crypto_engine) {
            Java_com_sentinel_v2x_V2X_cryptoInitialize(env, clazz);
        }

        auto version = g_crypto_engine->get_botan_version();
        LOGD("Botan version: %s", version.c_str());
        return env->NewStringUTF(version.c_str());
    } catch (const std::exception& e) {
        LOGE("Version query error: %s", e.what());
        return env->NewStringUTF("Unknown");
    }
}
