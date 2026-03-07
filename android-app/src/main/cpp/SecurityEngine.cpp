#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include "com_sentinel_v2x_bridge_SecurityEngine.h"

// ============================================================================
// PHASE 2: Include V2X Cryptographic Engine
// ============================================================================
#include "v2x_crypto_engine.h"

// Logging macro
#define LOG_TAG "SecurityEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Global V2X Crypto Engine Instance
// ============================================================================
static std::unique_ptr<sentinel::v2x::V2XCryptoEngine> g_crypto_engine = nullptr;

/**
 * JNI Wrapper: Initialize security engine with root CA
 */
JNIEXPORT jint JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_initializeWithRootCA(
    JNIEnv* env,
    jobject /*obj*/,
    jstring rootCAPath)
{
    try {
        // Create crypto engine if not already created
        if (!g_crypto_engine) {
            g_crypto_engine = std::make_unique<sentinel::v2x::V2XCryptoEngine>();
            LOGI("V2XCryptoEngine created");
        }
        
        // Get root CA path from Java
        const char* caPath = env->GetStringUTFChars(rootCAPath, nullptr);
        LOGI("Initializing with root CA: %s", caPath);
        
        // TODO: Load root CA certificate from file path
        // For now, implementation expects certificate bytes passed separately
        // This is a preliminary integration point
        
        env->ReleaseStringUTFChars(rootCAPath, caPath);
        return 0; // Success
        
    } catch (const std::exception& e) {
        LOGE("initializeWithRootCA exception: %s", e.what());
        return -1;
    }
}

/**
 * JNI Wrapper: Verify V2X packet signature
 */
JNIEXPORT jboolean JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_verifyPacket(
    JNIEnv* env, 
    jobject /*obj*/,
    jbyteArray messageData,
    jbyteArray signatureBytes,
    jobjectArray certificateChain)
{
    try {
        if (!g_crypto_engine) {
            LOGE("Crypto engine not initialized");
            return JNI_FALSE;
        }
        
        // Get message data
        jsize messageLen = env->GetArrayLength(messageData);
        jbyte* messagePtr = env->GetByteArrayElements(messageData, nullptr);
        std::vector<uint8_t> message(messagePtr, messagePtr + messageLen);
        env->ReleaseByteArrayElements(messageData, messagePtr, JNI_ABORT);
        
        // Get signature data
        jsize signatureLen = env->GetArrayLength(signatureBytes);
        jbyte* signaturePtr = env->GetByteArrayElements(signatureBytes, nullptr);
        std::vector<uint8_t> signature(signaturePtr, signaturePtr + signatureLen);
        env->ReleaseByteArrayElements(signatureBytes, signaturePtr, JNI_ABORT);
        
        // Get certificate chain (first certificate for verification)
        jsize chainLen = env->GetArrayLength(certificateChain);
        if (chainLen == 0) {
            LOGE("No certificates provided in chain");
            return JNI_FALSE;
        }
        
        jbyteArray firstCertArray = (jbyteArray)env->GetObjectArrayElement(certificateChain, 0);
        jsize certLen = env->GetArrayLength(firstCertArray);
        jbyte* certPtr = env->GetByteArrayElements(firstCertArray, nullptr);
        std::vector<uint8_t> senderCert(certPtr, certPtr + certLen);
        env->ReleaseByteArrayElements(firstCertArray, certPtr, JNI_ABORT);
        env->DeleteLocalRef(firstCertArray);
        
        // Call V2XCryptoEngine to verify signature
        LOGI("Verifying packet: msg_len=%zu, sig_len=%zu, cert_len=%zu", 
             message.size(), signature.size(), senderCert.size());
        
        // Parse sender certificate to get public key
        auto cert_info = g_crypto_engine->parse_certificate(senderCert);
        if (cert_info.subject.empty()) {
            LOGE("Failed to parse sender certificate");
            return JNI_FALSE;
        }
        
        // Verify ECDSA signature
        auto result = g_crypto_engine->verify_ecdsa_signature(
            message, 
            signature, 
            senderCert  // Public key embedded in certificate
        );
        
        if (!result.valid) {
            LOGI("Signature verification failed: %s", result.error_message.c_str());
            return JNI_FALSE;
        }
        
        LOGI("Signature verified successfully");
        return JNI_TRUE;
        
    } catch (const std::exception& e) {
        LOGE("verifyPacket exception: %s", e.what());
        return JNI_FALSE;
    }
}

/**
 * JNI Wrapper: Validate certificate chain
 */
JNIEXPORT jboolean JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_validateCertificateChain(
    JNIEnv* env,
    jobject /*obj*/,
    jobjectArray certificateChain)
{
    try {
        if (!g_crypto_engine) {
            LOGE("Crypto engine not initialized");
            return JNI_FALSE;
        }
        
        jsize chainLen = env->GetArrayLength(certificateChain);
        std::vector<std::vector<uint8_t>> chain(chainLen);
        
        LOGI("Validating certificate chain with %zu certificates", chainLen);
        
        // Extract all certificates from Java array
        for (jsize i = 0; i < chainLen; ++i) {
            jbyteArray certArray = (jbyteArray)env->GetObjectArrayElement(certificateChain, i);
            jsize certLen = env->GetArrayLength(certArray);
            jbyte* certPtr = env->GetByteArrayElements(certArray, nullptr);
            chain[i] = std::vector<uint8_t>(certPtr, certPtr + certLen);
            env->ReleaseByteArrayElements(certArray, certPtr, JNI_ABORT);
            env->DeleteLocalRef(certArray);
            
            LOGI("  Certificate[%zu]: %zu bytes", i, chain[i].size());
        }
        
        // Validate chain using V2XCryptoEngine
        bool result = g_crypto_engine->validate_certificate_chain(chain);
        
        if (result) {
            LOGI("Certificate chain validated successfully");
        } else {
            LOGE("Certificate chain validation failed");
        }
        
        return result ? JNI_TRUE : JNI_FALSE;
        
    } catch (const std::exception& e) {
        LOGE("validateCertificateChain exception: %s", e.what());
        return JNI_FALSE;
    }
}

/**
 * JNI Wrapper: Extract sender information from certificate
 */
JNIEXPORT jstring JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_extractSenderInfo(
    JNIEnv* env,
    jobject /*obj*/,
    jbyteArray certificate)
{
    try {
        if (!g_crypto_engine) {
            LOGE("Crypto engine not initialized");
            return env->NewStringUTF("");
        }
        
        jsize certLen = env->GetArrayLength(certificate);
        jbyte* certPtr = env->GetByteArrayElements(certificate, nullptr);
        std::vector<uint8_t> cert(certPtr, certPtr + certLen);
        env->ReleaseByteArrayElements(certificate, certPtr, JNI_ABORT);
        
        // Parse certificate to extract sender info
        auto cert_info = g_crypto_engine->parse_certificate(cert);
        
        LOGI("Extracted sender: %s", cert_info.subject.c_str());
        return env->NewStringUTF(cert_info.subject.c_str());
        
    } catch (const std::exception& e) {
        LOGE("extractSenderInfo exception: %s", e.what());
        return env->NewStringUTF("");
    }
}

/**
 * JNI Wrapper: Parse IEEE 1609.2 message
 */
JNIEXPORT jbyteArray JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_parseIEEE1609Message(
    JNIEnv* env,
    jobject /*obj*/,
    jbyteArray message)
{
    try {
        if (!g_crypto_engine) {
            LOGE("Crypto engine not initialized");
            return nullptr;
        }
        
        jsize messageLen = env->GetArrayLength(message);
        jbyte* messagePtr = env->GetByteArrayElements(message, nullptr);
        std::vector<uint8_t> messageData(messagePtr, messagePtr + messageLen);
        env->ReleaseByteArrayElements(message, messagePtr, JNI_ABORT);
        
        LOGI("Parsing IEEE 1609.2 message: %zu bytes", messageData.size());
        
        // TODO: Implement IEEE 1609.2 message parsing
        // Extract payload, headers, and metadata from structured message format
        // For now, return SHA-256 hash of message for integrity verification
        
        auto hash = g_crypto_engine->sha256_hash(messageData);
        
        if (hash.empty()) {
            LOGE("Failed to compute message hash");
            return nullptr;
        }
        
        jbyteArray returnArray = env->NewByteArray(hash.size());
        env->SetByteArrayRegion(returnArray, 0, hash.size(), (const jbyte*)hash.data());
        return returnArray;
        
    } catch (const std::exception& e) {
        LOGE("parseIEEE1609Message exception: %s", e.what());
        return nullptr;
    }
}

/**
 * JNI Wrapper: Cleanup and release resources
 */
JNIEXPORT jint JNICALL 
Java_com_sentinel_v2x_bridge_SecurityEngine_cleanup(
    JNIEnv* /*env*/,
    jobject /*obj*/)
{
    try {
        if (g_crypto_engine) {
            // Cleanup V2XCryptoEngine
            g_crypto_engine->cleanup();
            g_crypto_engine.reset();
            LOGI("V2XCryptoEngine cleaned up");
        }
        return 0;
        
    } catch (const std::exception& e) {
        LOGE("cleanup exception: %s", e.what());
        return -1;
    }
}
