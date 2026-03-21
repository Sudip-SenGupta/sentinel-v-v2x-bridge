package com.sentinel.v2x

import java.io.ByteArrayOutputStream

/**
 * COERBinaryMessageBuilder: Generate parser-compatible COER messages
 *
 * Binary format (matching v2x_coer_decoder.cpp):
 * - 1 byte header (version nibble + flags)
 * - varint payload length (ASN.1 encoding: short-form 0-127, long-form 128+)
 * - payload bytes
 * - if signed:
 *   - 1 byte signature algorithm (0x04 = ECDSA P-256)
 *   - varint signature length
 *   - signature bytes
 *   - varint issuer certificate length
 *   - issuer certificate bytes
 *   - optional 1 byte chain depth
 *   - repeated (varint cert_length + cert bytes)
 */
class COERBinaryMessageBuilder {

    /**
     * Encode integer as COER varint (ASN.1 style)
     *
     * Short-form: 0-127 in single byte
     * Long-form: 0x80 + length_of_length byte(s) + big-endian value bytes
     */
    fun encodeVarint(value: Int): ByteArray {
        require(value >= 0) { "Varint must be non-negative, got $value" }

        // Short form: single byte for 0-127
        if (value <= 0x7F) {
            return byteArrayOf((value and 0xFF).toByte())
        }

        // Long form: determine how many bytes needed for big-endian representation
        val valueBytes = mutableListOf<Byte>()
        var temp = value
        while (temp > 0) {
            valueBytes.add(0, (temp and 0xFF).toByte())
            temp = temp shr 8
        }

        // First byte: 0x80 | length_of_length
        val result = ByteArrayOutputStream()
        result.write(0x80 or valueBytes.size)
        result.write(valueBytes.toByteArray())

        return result.toByteArray()
    }

    /**
     * Build header byte from version and flags
     *
     * Bit layout:
     * - Bits 7-4: version (0x1 = version 1)
     * - Bits 3-2: reserved
     * - Bit 1: is_signed (0 = unsigned, 1 = signed)
     * - Bit 0: reserved
     */
    fun buildHeaderByte(version: Int = 1, isSigned: Boolean = false): Byte {
        require(version in 1..15) { "Version must be 1-15, got $version" }

        var header = (version and 0x0F) shl 4  // Upper nibble
        if (isSigned) {
            header = header or 0x02  // Set signed flag (bit 1)
        }

        return (header and 0xFF).toByte()
    }

    /**
     * Build unsigned COER message
     *
     * Format: [header] [varint_length] [payload]
     */
    fun buildUnsignedMessage(payload: ByteArray): ByteArray {
        val result = ByteArrayOutputStream()

        // Header byte (version 1, unsigned)
        result.write(buildHeaderByte(version = 1, isSigned = false).toInt())

        // Payload length
        result.write(encodeVarint(payload.size))

        // Payload
        result.write(payload)

        return result.toByteArray()
    }

    /**
     * Build signed COER message with signature container
     *
     * Format: [header] [varint_length] [payload] [sig_algo] [varint_sig_len] [signature]
     *         [varint_issuer_cert_len] [issuer_cert] [chain_depth] [cert1] [cert2] ...
     */
    fun buildSignedMessage(
        payload: ByteArray,
        signature: ByteArray,
        issuerCert: ByteArray,
        chainCerts: List<ByteArray> = emptyList(),
        signatureAlgorithm: Byte = 0x04  // ECDSA P-256
    ): ByteArray {
        val result = ByteArrayOutputStream()

        // Header byte (version 1, signed)
        result.write(buildHeaderByte(version = 1, isSigned = true).toInt())

        // Payload length
        result.write(encodeVarint(payload.size))

        // Payload
        result.write(payload)

        // Signature container
        // Algorithm byte
        result.write(signatureAlgorithm.toInt())

        // Signature length and data
        result.write(encodeVarint(signature.size))
        result.write(signature)

        // Issuer certificate length and data
        result.write(encodeVarint(issuerCert.size))
        result.write(issuerCert)

        // Certificate chain (if present)
        if (chainCerts.isNotEmpty()) {
            result.write(chainCerts.size)
            chainCerts.forEach { cert ->
                result.write(encodeVarint(cert.size))
                result.write(cert)
            }
        }

        return result.toByteArray()
    }

    /**
     * Build minimal valid BSM payload with frame type 0x01
     *
     * Format (simplified):
     * - 1 byte frame type (0x10 for BSM, where upper nibble = 0x01)
     * - 4 bytes timestamp (milliseconds, big-endian)
     * - 6 bytes sender ID
     * - ... (rest of BSM fields)
     */
    fun buildBSMPayload(
        timestamp: Long = System.currentTimeMillis(),
        senderID: ByteArray? = null
    ): ByteArray {
        val payload = ByteArrayOutputStream()

        // Frame type byte: 0x01 in upper nibble, lower nibble varies
        val frameTypeByte = 0x10  // BSM indicator
        payload.write(frameTypeByte)

        // Timestamp (4 bytes, big-endian)
        payload.write(((timestamp shr 24) and 0xFF).toInt())
        payload.write(((timestamp shr 16) and 0xFF).toInt())
        payload.write(((timestamp shr 8) and 0xFF).toInt())
        payload.write((timestamp and 0xFF).toInt())

        // Sender ID (6 bytes) - default: zeros
        val sender = senderID ?: ByteArray(6)
        require(sender.size == 6) { "Sender ID must be 6 bytes, got ${sender.size}" }
        payload.write(sender)

        // Minimal BSM padding (can extend with lat/lon/speed/heading as needed)
        // For now, just fill with placeholder data
        payload.write(0x00)  // Sequence number
        payload.write(ByteArray(10))  // Position/speed placeholder

        return payload.toByteArray()
    }

    /**
     * Build minimal valid SPaT payload with frame type 0x02
     */
    fun buildSPaTPayload(): ByteArray {
        val payload = ByteArrayOutputStream()

        // Frame type byte: 0x02 in upper nibble
        payload.write(0x20)

        // SPaT intersectionID (4 bytes)
        payload.write(ByteArray(4))

        // Status/timing placeholder
        payload.write(ByteArray(8))

        return payload.toByteArray()
    }

    /**
     * Build minimal valid PSM payload with frame type 0x03
     */
    fun buildPSMPayload(): ByteArray {
        val payload = ByteArrayOutputStream()

        // Frame type byte: 0x03 in upper nibble
        payload.write(0x30)

        // PSM data placeholder
        payload.write(ByteArray(20))

        return payload.toByteArray()
    }

    /**
     * Build complete test fixture: unsigned BSM message
     */
    fun buildTestBSM(): ByteArray {
        val payload = buildBSMPayload()
        return buildUnsignedMessage(payload)
    }

    /**
     * Build complete test fixture: unsigned SPaT message
     */
    fun buildTestSPaT(): ByteArray {
        val payload = buildSPaTPayload()
        return buildUnsignedMessage(payload)
    }

    /**
     * Build complete test fixture: unsigned PSM message
     */
    fun buildTestPSM(): ByteArray {
        val payload = buildPSMPayload()
        return buildUnsignedMessage(payload)
    }
}
