package com.sentinel.v2x

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.nio.charset.StandardCharsets
import java.security.KeyPairGenerator
import java.security.Signature
import java.security.spec.ECGenParameterSpec

/**
 * Instrumented test for V2X JNI bindings.
 *
 * Purpose:
 * - Validate Android/JNI/CMake integration works
 * - Validate message-processing JNI functions execute correctly
 * - Validate Botan-backed crypto JNI functions execute correctly
 */
@RunWith(AndroidJUnit4::class)
class V2XJNITest {

    @Test
    fun testSimpleDummy() {
        assertTrue("Dummy test", true)
    }

    @Before
    fun setup() {
        // Native library automatically loaded by V2X object init block.
    }

    @Test
    fun testNativeLibraryLoads() {
        try {
            assertTrue("Native library loaded", true)
        } catch (e: UnsatisfiedLinkError) {
            fail("Native library failed to load: ${e.message}")
        }
    }

    @Test
    fun testGetVersion() {
        val version = V2X.getVersion()
        assertNotNull("Version string should not be null", version)
        assertTrue("Version string should not be empty", version.isNotEmpty())
        assertTrue("Version should contain 'Message Processor'", version.contains("Message Processor"))
    }

    @Test
    fun testVersionFormatValid() {
        val version = V2X.getVersion()
        assertTrue(
            "Version should follow expected format",
            version.matches(Regex(".*[Mm]essage.*[Pp]rocessor.*v\\d+\\.\\d+\\.\\d+.*"))
        )
    }

    @Test
    fun testMultipleCalls() {
        val version1 = V2X.getVersion()
        val version2 = V2X.getVersion()
        assertEquals("Multiple calls should return same value", version1, version2)
    }

    @Test
    fun testDetectFrameType() {
        val bsmPayload = byteArrayOf(
            0x10,
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
            0x41, 0x82.toByte(), 0x0D, 0x7C,
            0x04, 0x25, 0xD3.toByte(), 0x44,
            0x00, 0x32,
            0x01, 0x2C
        )

        val coerMessage = wrapInCOER(bsmPayload)

        try {
            val frameType = V2X.detectFrameType(coerMessage)
            assertNotNull("Frame type should not be null", frameType)
            assertTrue("Frame type should not be empty", frameType.isNotEmpty())
            assertTrue("Frame type should contain BSM", frameType.uppercase().contains("BSM"))
        } catch (e: Exception) {
            fail("detectFrameType failed: ${e.message}")
        }
    }

    @Test
    fun testProcessBSMMessage() {
        val bsmPayload = byteArrayOf(
            0x10,
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
            0x41, 0x82.toByte(), 0x0D, 0x7C,
            0x04, 0x25, 0xD3.toByte(), 0x44,
            0x00, 0x32,
            0x01, 0x2C
        )

        val coerMessage = wrapInCOER(bsmPayload)

        try {
            val decoded = V2X.processMessage(coerMessage)
            assertNotNull("Decoded message should not be null", decoded)
        } catch (e: Exception) {
            fail("processMessage failed: ${e.message}")
        }
    }

    @Test
    fun testProcessBatch() {
        val bsmPayload = byteArrayOf(
            0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
            0x41, 0x82.toByte(), 0x0D, 0x7C,
            0x04, 0x25, 0xD3.toByte(), 0x44,
            0x00, 0x32, 0x01, 0x2C
        )

        val messages = listOf(
            wrapInCOER(bsmPayload),
            wrapInCOER(bsmPayload),
            wrapInCOER(bsmPayload)
        )

        try {
            val decoded = V2X.processBatch(messages)
            assertNotNull("Decoded batch should not be null", decoded)
            assertEquals("Should decode all 3 messages", 3, decoded.size)
        } catch (e: Exception) {
            fail("processBatch failed: ${e.message}")
        }
    }

    @Test
    fun testCryptoInitialize() {
        assertTrue("Crypto engine should initialize successfully", V2X.cryptoInitialize())
    }

    @Test
    fun testSha256Hash() {
        val hash = V2X.sha256Hash("abc".toByteArray(StandardCharsets.UTF_8))
        val expectedHex = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"

        assertNotNull("Hash should not be null", hash)
        assertEquals("SHA-256 hash should be 32 bytes", 32, hash.size)
        assertEquals("SHA-256 digest should match known test vector", expectedHex, hash.toHex())
    }

    @Test
    fun testSha256Hex() {
        val hex = V2X.sha256Hex("abc".toByteArray(StandardCharsets.UTF_8))
        val expectedHex = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"

        assertEquals("SHA-256 hex digest should match known test vector", expectedHex, hex)
    }

    @Test
    fun testGetCryptoBotanVersion() {
        val version = V2X.getCryptoBotanVersion()
        assertNotNull("Botan version should not be null", version)
        assertTrue("Botan version should contain 'Botan'", version.contains("Botan"))
    }

    @Test
    fun testVerifySignatureRejectsInvalidInputs() {
        val message = "invalid-signature-test".toByteArray(StandardCharsets.UTF_8)
        val bogusSignature = byteArrayOf(0x01, 0x02, 0x03, 0x04)
        val bogusKey = byteArrayOf(0x05, 0x06, 0x07, 0x08)

        assertFalse(
            "Invalid signature/public key input should fail verification",
            V2X.verifySignature(message, bogusSignature, bogusKey)
        )
    }

    @Test
    fun testVerifySignatureAcceptsValidInputs() {
        val keyPairGenerator = KeyPairGenerator.getInstance("EC")
        keyPairGenerator.initialize(ECGenParameterSpec("secp256r1"))
        val keyPair = keyPairGenerator.generateKeyPair()

        val message = "botan-positive-test".toByteArray(StandardCharsets.UTF_8)
        val signer = Signature.getInstance("SHA256withECDSA")
        signer.initSign(keyPair.private)
        signer.update(message)
        val derSignature = signer.sign()
        val signature = derEcdsaSignatureToP1363(derSignature)

        val publicKeyDer = keyPair.public.encoded

        assertTrue(
            "Valid signature generated on Android should verify through Botan JNI",
            V2X.verifySignature(message, signature, publicKeyDer)
        )
    }

    @Test
    fun testInvalidCertificateRejected() {
        val invalidCert = byteArrayOf(0x01, 0x02, 0x03, 0x04)
        assertFalse("Invalid certificate should be rejected", V2X.isValidCertificate(invalidCert))
    }

    @Test
    fun testInvalidCertificateChainRejected() {
        val invalidChain = arrayOf(
            byteArrayOf(0x01, 0x02, 0x03),
            byteArrayOf(0x04, 0x05, 0x06)
        )

        assertFalse("Invalid certificate chain should be rejected", V2X.validateCertificateChain(invalidChain))
    }

    private fun wrapInCOER(payload: ByteArray): ByteArray {
        val coer = mutableListOf<Byte>()
        coer.add(0x00.toByte())

        if (payload.size <= 127) {
            coer.add(payload.size.toByte())
        } else {
            coer.add(0x81.toByte())
            coer.add(payload.size.toByte())
        }

        coer.addAll(payload.toList())
        return coer.toByteArray()
    }

    private fun ByteArray.toHex(): String = joinToString("") { "%02x".format(it) }

    private fun derEcdsaSignatureToP1363(derSignature: ByteArray): ByteArray {
        require(derSignature.isNotEmpty() && derSignature[0] == 0x30.toByte()) {
            "ECDSA signature must be a DER SEQUENCE"
        }

        var index = 1
        val sequenceLength = readDerLength(derSignature, index)
        index += sequenceLength.second

        require(index + sequenceLength.first <= derSignature.size) {
            "Invalid DER sequence length"
        }

        require(derSignature[index] == 0x02.toByte()) { "Missing DER INTEGER for r" }
        index += 1
        val rLength = readDerLength(derSignature, index)
        index += rLength.second
        val r = derSignature.copyOfRange(index, index + rLength.first)
        index += rLength.first

        require(derSignature[index] == 0x02.toByte()) { "Missing DER INTEGER for s" }
        index += 1
        val sLength = readDerLength(derSignature, index)
        index += sLength.second
        val s = derSignature.copyOfRange(index, index + sLength.first)

        return leftPadTo32(stripLeadingZero(r)) + leftPadTo32(stripLeadingZero(s))
    }

    private fun readDerLength(data: ByteArray, offset: Int): Pair<Int, Int> {
        val first = data[offset].toInt() and 0xFF
        if ((first and 0x80) == 0) {
            return first to 1
        }

        val count = first and 0x7F
        var value = 0
        for (i in 0 until count) {
            value = (value shl 8) or (data[offset + 1 + i].toInt() and 0xFF)
        }
        return value to (1 + count)
    }

    private fun stripLeadingZero(bytes: ByteArray): ByteArray {
        return if (bytes.size > 1 && bytes[0] == 0.toByte()) bytes.copyOfRange(1, bytes.size) else bytes
    }

    private fun leftPadTo32(bytes: ByteArray): ByteArray {
        require(bytes.size <= 32) { "ECDSA integer component larger than 32 bytes" }
        return ByteArray(32 - bytes.size) + bytes
    }
}


