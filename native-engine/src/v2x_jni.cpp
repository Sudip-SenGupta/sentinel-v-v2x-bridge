#include <jni.h>
#include <string>

// For non-Android builds: include full V2X engine
#if !defined(__ANDROID__) || defined(FULL_ENGINE_BUILD)
#include "v2x_message_processor.h"
using namespace sentinel::v2x;
#endif

/**
 * JNI Interface for V2X Message Processing
 * 
 * Minimal JNI binding to verify Android/native integration toolchain.
 * This is a proof-of-concept; full Android interface planned for Phase 4.
 * 
 * Compilation: Handled by CMakeLists.txt when ANDROID=true
 * 
 * Usage from Kotlin:
 *   val version = V2X.getVersion()
 * 
 * Note: This minimal binding validates toolchain only.
 *       Full MessageVerificationResult marshalling deferred to Phase 4
 *       (awaiting ASN.1 decoder for meaningful payload content).
 */

/**
 * Get V2X engine version string
 * 
 * Android: Returns stub version (Phase 4 will expand with actual engine)
 * Non-Android: Maps to V2XMessageProcessor::get_version()
 * 
 * @return Engine version (e.g., "V2X Message Processor v1.0.0")
 */
extern "C" JNIEXPORT jstring JNICALL
Java_com_sentinel_v2x_V2X_getVersion(JNIEnv* env, jclass /* clazz */) {
#ifdef __ANDROID__
    // Android minimal JNI: stub version (Phase 3 Week 2.5 proof-of-concept)
    // No dependencies - no sentinel-engine linking required
    // Phase 4 will replace this with actual sentinel-engine when Botan ARM64 available
    try {
        std::string version = "V2X Message Processor v1.0.0";
        return env->NewStringUTF(version.c_str());
    } catch (...) {
        return env->NewStringUTF("V2X Engine Error");
    }
#else
    // Non-Android: use full sentinel-engine
    try {
        std::string version = sentinel::v2x::V2XMessageProcessor::get_version();
        return env->NewStringUTF(version.c_str());
    } catch (const std::exception& e) {
        return env->NewStringUTF("V2X Engine Error");
    }
#endif
}

/**
 * Future JNI Functions (Phase 4 & beyond)
 * 
 * These will be added after Phase 4 ASN.1 decoder completion:
 * 
 * - processMessage(byte[] rawMessage) → MessageVerificationResult
 * - decodeBasicSafetyMessage(byte[] der) → BsmMessage
 * - decodeProbeDataMessage(byte[] der) → PdmMessage
 * - decodeSignalPhase(byte[] der) → SpaT
 * - validatePositionBounds(double lat, double lon) → boolean
 * 
 * Each will have corresponding Kotlin data classes and error handling.
 */
