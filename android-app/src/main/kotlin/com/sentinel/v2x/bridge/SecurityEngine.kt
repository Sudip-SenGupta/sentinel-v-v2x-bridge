package com.sentinel.v2x.bridge

/**
 * SecurityEngine: JNI Bridge to the native C++17 V2X security engine.
 * Handles IEEE 1609.2 message verification, certificate validation,
 * and ECDSA signature verification.
 */
object SecurityEngine {
    init {
        // Load the native security engine library
        System.loadLibrary("security-engine")
    }

    /**
     * Verify a V2X message signature.
     *
     * @param messageData The raw V2X message bytes
     * @param signatureBytes The ECDSA signature bytes
     * @param certificateChain Array of X.509 certificates in DER format
     * @return true if signature is valid and certificate chain is trusted, false otherwise
     */
    external fun verifyPacket(
        messageData: ByteArray,
        signatureBytes: ByteArray,
        certificateChain: Array<ByteArray>
    ): Boolean

    /**
     * Extract sender information from a certificate.
     *
     * @param certificate X.509 certificate in DER format
     * @return Sender identifier or empty string if extraction fails
     */
    external fun extractSenderInfo(certificate: ByteArray): String

    /**
     * Initialize the crypto engine with the root CA certificate.
     *
     * @param rootCAPath Path to the root CA certificate file
     * @return 0 on success, error code on failure
     */
    external fun initializeWithRootCA(rootCAPath: String): Int

    /**
     * Validate a certificate chain for authenticity and validity.
     *
     * @param chain Array of X.509 certificates in DER format (leaf to root)
     * @return true if chain is valid and trusted, false otherwise
     */
    external fun validateCertificateChain(chain: Array<ByteArray>): Boolean

    /**
     * Parse and validate an IEEE 1609.2 message.
     *
     * @param message Raw message bytes in IEEE 1609.2 format
     * @return Hash of parsed message content or null if parsing fails
     */
    external fun parseIEEE1609Message(message: ByteArray): ByteArray?

    /**
     * Cleanup resources (called before app shutdown).
     *
     * @return 0 on success, error code on failure
     */
    external fun cleanup(): Int
}
