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
    // Phase 4: Message Processing
    // ========================================================================
    
    /**
     * Detect V2X message frame type from raw COER bytes
     * 
     * First step in decoding: Identifies whether message is BSM, SPaT, PSM, etc.
     * 
     * @param coerBytes Raw COER-encoded message bytes from vehicle network
     * @return Frame type as string (e.g., "BSM", "SPAT", "PSM", "UNKNOWN")
     * @throws RuntimeException if COER parsing fails
     */
    external fun detectFrameType(coerBytes: ByteArray): String
    
    /**
     * Decode complete V2X message from raw COER bytes
     * 
     * Full pipeline:
     *   1. Parse COER wrapper
     *   2. Extract payload, signature, certificate chain
     *   3. Identify message frame type
     *   4. Decode into structured fields
     *   5. Marshal to Java/Kotlin objects
     * 
     * @param coerBytes Raw COER-encoded message from vehicle network or OBU
     * @return DecodedV2XMessage sealed class (BSM, SPaT, PSM, or Unknown)
     *
     * @throws RuntimeException if:
     *         - COER message is malformed
     *         - Message frame type unsupported
     *         - Payload structure invalid
     *
     * Usage:
     * ```kotlin
     * val raw = receivedFromNetwork()  // ByteArray
     * val decoded = V2X.processMessage(raw)
     * 
     * when (decoded) {
     *     is DecodedV2XMessage.BSM -> {
     *         val lat = decoded.message.position.latitude
     *         val lon = decoded.message.position.longitude
     *         updateMapMarker(lat, lon)
     *     }
     *     is DecodedV2XMessage.SPaT -> {
     *         updateSignalIndicator(decoded.message.intersections)
     *     }
     *     else -> Log.w("V2X", "Unhandled message type")
     * }
     * ```
     * 
     * @see DecodedV2XMessage for result type hierarchy
     */
    external fun processMessage(coerBytes: ByteArray): DecodedV2XMessage
    
    /**
     * Process multiple V2X messages in batch
     * 
     * Efficient for high-frequency message streams (10-100 msgs/sec)
     * 
     * @param coerMessages List of raw COER-encoded messages
     * @return List of decoded messages (preserves order)
     */
    external fun processBatch(coerMessages: List<ByteArray>): List<DecodedV2XMessage>

    // ========================================================================
    // Future Methods (Phase 4 & Beyond)
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
