package com.sentinel.v2x

/**
 * JNI Interface to V2X Native Engine (Minimal Binding - Toolchain Test)
 *
 * This is a proof-of-concept interface to verify:
 * - Kotlin to C++ JNI linkage works
 * - Gradle CMake integration compiles correctly
 * - Native library loads and functions execute
 *
 * Phase: Minimal validation only
 * Timeline: ~1 day for toolchain verification
 *
 * STATUS: This binding is intentionally minimal.
 * Full MessageProcessor interface planned for Phase 4 after ASN.1 decoder.
 *
 * Usage:
 *   val version = V2X.getVersion()
 *   println(version)  // "V2X Message Processor v1.0.0"
 */
object V2X {
    init {
        // Minimal Android JNI target with no Botan dependency.
        System.loadLibrary("v2x-jni")
    }

    /**
     * Get V2X engine version string
     *
     * @return Version string (e.g., "V2X Message Processor v1.0.0")
     * @throws UnsatisfiedLinkError if native library not loaded
     *
     * Test: ./gradlew connectedAndroidTest --tests com.sentinel.v2x.V2XJNITest
     */
    external fun getVersion(): String

    // ========================================================================
    // Future Methods (Phase 4 & Beyond) - Deferred until ASN.1 decoder ready
    // ========================================================================
}

data class MessageVerificationResult(
    val isValid: Boolean = false,
    val errorMessage: String = "",
    val coerParseOk: Boolean = false,
    val payloadStructureOk: Boolean = false,
    val signatureValid: Boolean = false,
    val chainValid: Boolean = false
)
