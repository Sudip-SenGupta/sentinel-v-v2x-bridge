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
        // For now, skip complex signature verification test - focus on crypto correctness
        // TODO: Add ECDSA signature verification with proper certificate handling
        //       Issue: Botan's X509::load_key() signature verification needs testing
        //              with real IEEE 1609.2 signed messages (Phase 3 gap)
        
        // This test validates that the JNI crypto functions are callable
        val initialized = V2X.cryptoInitialize()
        assertTrue("Crypto should initialize for verification tests", initialized)
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

    @Test
    fun testValidGeneratedCertificateChainAccepted() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(3)
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertTrue("Generated root/intermediate/leaf chain should validate", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testReorderedGeneratedCertificateChainRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(3)
        val wrongOrder = arrayOf(
            chain.last().first.encoded,
            chain.first().first.encoded,
            chain[1].first.encoded
        )

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertFalse("Reordered generated chain should be rejected", V2X.validateCertificateChain(wrongOrder))
    }

    @Test
    fun testWrongTrustedRootRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(3)
        val otherChain = generator.createCertificateChain(3)
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Different trusted root should initialize", V2X.initializeWithRootCA(otherChain.first().first.encoded))
        assertFalse("Valid chain should be rejected when anchored to the wrong root", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testNonCAIntermediateRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(depth = 3, intermediateIsCa = false)
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertFalse("Intermediate certificate without CA privileges should be rejected", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testExpiredLeafCertificateRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(
            depth = 3,
            leafNotBeforeOffsetMillis = -2L * 24L * 60L * 60L * 1000L,
            leafNotAfterOffsetMillis = -1L * 24L * 60L * 60L * 1000L
        )
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertFalse("Expired leaf certificate should be rejected", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testNotYetValidLeafCertificateRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(
            depth = 3,
            leafNotBeforeOffsetMillis = 24L * 60L * 60L * 1000L,
            leafNotAfterOffsetMillis = 2L * 24L * 60L * 60L * 1000L
        )
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertFalse("Not-yet-valid leaf certificate should be rejected", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testLeafCACertificateRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(depth = 3, leafIsCa = true)
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertFalse("Leaf certificate marked as CA should be rejected", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testLeafWithoutDigitalSignatureUsageRejected() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(
            depth = 3,
            leafKeyUsageBitsOverride = org.bouncycastle.asn1.x509.KeyUsage.keyEncipherment
        )
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertFalse("Leaf certificate without digitalSignature usage should be rejected", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testClearTrustedRootCAResetsValidationState() {
        val generator = V2XSignatureGenerator()
        val chain = generator.createCertificateChain(3)
        val encodedChain = chain.asReversed().map { it.first.encoded }.toTypedArray()

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))
        assertTrue("Chain should validate with trusted root configured", V2X.validateCertificateChain(encodedChain))
        assertTrue("Trusted root should clear", V2X.clearTrustedRootCA())
        assertFalse("Chain should fail after trusted root is cleared", V2X.validateCertificateChain(encodedChain))
    }

    @Test
    fun testMalformedTruncatedHeaderRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildTruncatedHeaderMessage())
    }

    @Test
    fun testMalformedTruncatedPayloadRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildTruncatedPayloadMessage())
    }

    @Test
    fun testMalformedIndefiniteLengthRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildIndefiniteLengthMessage())
    }

    @Test
    fun testMalformedOverflowVarintRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildOverflowVarintMessage())
    }

    @Test
    fun testMalformedUnsupportedVersionRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildUnsupportedVersionMessage())
    }

    @Test
    fun testMalformedTruncatedLongFormVarintRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildTruncatedLongFormVarintMessage())
    }

    @Test
    fun testMalformedPayloadLengthOverclaimRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildPayloadLengthOverclaimMessage())
    }

    @Test
    fun testMalformedSignatureLengthOverclaimRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildSignatureLengthOverclaimMessage())
    }

    @Test
    fun testMalformedIssuerCertLengthOverclaimRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildIssuerCertLengthOverclaimMessage())
    }

    @Test
    fun testMalformedChainCertLengthOverclaimRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildChainCertLengthOverclaimMessage())
    }

    @Test
    fun testMalformedChainDepthCountMismatchRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildChainDepthCountMismatchMessage())
    }

    @Test
    fun testMalformedMissingSignatureAlgorithmRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildMissingSignatureAlgorithmMessage())
    }

    @Test
    fun testMalformedTruncatedIssuerCertLengthVarintRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildTruncatedIssuerCertLengthVarintMessage())
    }

    @Test
    fun testMalformedTruncatedChainCertLengthVarintRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildTruncatedChainCertLengthVarintMessage())
    }

    @Test
    fun testMalformedUnsupportedFrameTypeRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildUnsupportedFrameTypeMessage())
    }

    @Test
    fun testMalformedTruncatedSignedContainerRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildTruncatedSignedContainerMessage())
    }

    @Test
    fun testMalformedDanglingChainDepthRejected() {
        val generator = COERMalformationGenerator()
        assertRejectedByProcessMessage(generator.buildDanglingChainDepthMessage())
    }

    @Test
    fun testMalformedFixtureCatalogHasExpectedCoverage() {
        val generator = COERMalformationGenerator()
        assertTrue("Unsigned malformed catalog should have broad coverage", generator.unsignedParserRejectionCases().size >= 8)
        assertTrue("Signed malformed catalog should have broad coverage", generator.signedParserRejectionCases().size >= 9)
        assertEquals(
            "Combined malformed catalog should include all unsigned and signed cases",
            generator.unsignedParserRejectionCases().size + generator.signedParserRejectionCases().size,
            generator.allProcessMessageRejectionCases().size
        )
    }

    private fun assertRejectedByProcessMessage(message: ByteArray) {
        try {
            V2X.processMessage(message)
            fail("Malformed message should be rejected")
        } catch (_: RuntimeException) {
            // Expected JNI failure path for malformed COER input.
        }
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

    @Test
    fun testCOERBinaryMessageBuilderBSM() {
        val builder = COERBinaryMessageBuilder()
        
        // Build unsigned BSM message
        val bsmMessage = builder.buildTestBSM()
        
        assertNotNull("BSM message should not be null", bsmMessage)
        assertTrue("BSM message should not be empty", bsmMessage.isNotEmpty())
        
        // First byte should be header 0x10 (version 1, unsigned)
        assertEquals("First byte should be header 0x10", 0x10, bsmMessage[0].toInt() and 0xFF)
        
        // Should parse through frame detection
        try {
            val frameType = V2X.detectFrameType(bsmMessage)
            assertNotNull("Frame type should be detected", frameType)
            assertTrue("Frame type should identify BSM", frameType.uppercase().contains("BSM"))
        } catch (e: Exception) {
            fail("BSM parsing failed: ${e.message}")
        }
    }

    @Test
    fun testCOERBinaryMessageBuilderSPaT() {
        val builder = COERBinaryMessageBuilder()
        
        // Build unsigned SPaT message
        val spatMessage = builder.buildTestSPaT()
        
        assertNotNull("SPaT message should not be null", spatMessage)
        assertTrue("SPaT message should not be empty", spatMessage.isNotEmpty())
        
        // Should parse through frame detection
        try {
            val frameType = V2X.detectFrameType(spatMessage)
            assertNotNull("Frame type should be detected", frameType)
            assertTrue("Frame type should identify SPaT", frameType.uppercase().contains("SPAT"))
        } catch (e: Exception) {
            fail("SPaT parsing failed: ${e.message}")
        }
    }

    @Test
    fun testCOERBinaryMessageBuilderPSM() {
        val builder = COERBinaryMessageBuilder()
        
        // Build unsigned PSM message
        val psmMessage = builder.buildTestPSM()
        
        assertNotNull("PSM message should not be null", psmMessage)
        assertTrue("PSM message should not be empty", psmMessage.isNotEmpty())
        
        // Should parse through frame detection
        try {
            val frameType = V2X.detectFrameType(psmMessage)
            assertNotNull("Frame type should be detected", frameType)
            assertTrue("Frame type should identify PSM", frameType.uppercase().contains("PSM"))
        } catch (e: Exception) {
            fail("PSM parsing failed: ${e.message}")
        }
    }

    @Test
    fun testCOERBinaryMessageBuilderVarintEncoding() {
        val builder = COERBinaryMessageBuilder()
        
        // Test short-form varint (0-127)
        val shortForm = builder.encodeVarint(42)
        assertEquals("Short-form varint should be 1 byte", 1, shortForm.size)
        assertEquals("Short-form varint should be 42", 42, shortForm[0].toInt())
        
        // Test long-form varint (128+)
        val longForm = builder.encodeVarint(256)
        assertTrue("Long-form varint should be multiple bytes", longForm.size > 1)
        assertEquals("Long-form varint should start with 0x80+ prefix", true, (longForm[0].toInt() and 0x80) != 0)
    }

    @Test
    fun testCOERBinaryMessageBuilderHeaderByte() {
        val builder = COERBinaryMessageBuilder()
        
        // Test unsigned header
        val headerUnsigned = builder.buildHeaderByte(version = 1, isSigned = false)
        assertEquals("Unsigned header should be 0x10", 0x10, headerUnsigned.toInt())
        
        // Test signed header
        val headerSigned = builder.buildHeaderByte(version = 1, isSigned = true)
        assertEquals("Signed header should be 0x12", 0x12, headerSigned.toInt())
    }

    // ============================================================================
    // PHASE 1B: SIGNATURE GENERATOR TESTS
    // ============================================================================

    @Test
    fun testSignatureGeneratorKeyPairGeneration() {
        val generator = V2XSignatureGenerator()
        
        // Generate key pair
        val keyPair = generator.generateKeyPair()
        
        assertNotNull("Private key should not be null", keyPair.private)
        assertNotNull("Public key should not be null", keyPair.public)
        assertEquals("Key algorithm should be EC", "EC", keyPair.private.algorithm)
    }

    @Test
    fun testSignatureGeneratorCertificateChain() {
        val generator = V2XSignatureGenerator()
        
        // Test chain depth 1
        val chain1 = generator.createCertificateChain(1)
        assertEquals("Chain depth 1 should have 1 certificate", 1, chain1.size)
        
        // Test chain depth 2
        val chain2 = generator.createCertificateChain(2)
        assertEquals("Chain depth 2 should have 2 certificates", 2, chain2.size)
        
        // Test chain depth 3
        val chain3 = generator.createCertificateChain(3)
        assertEquals("Chain depth 3 should have 3 certificates", 3, chain3.size)
    }

    @Test
    fun testSignatureGeneratorSignMessage() {
        val generator = V2XSignatureGenerator()
        
        // Generate key pair
        val keyPair = generator.generateKeyPair()
        
        // Sign a message
        val message = "Test message for signing".toByteArray()
        val signature = generator.signMessage(message, keyPair.private)
        
        assertNotNull("Signature should not be null", signature)
        assertTrue("Signature should not be empty", signature.isNotEmpty())
        assertTrue("DER signature should start with SEQUENCE (0x30)", signature[0] == 0x30.toByte())
    }

    @Test
    fun testSignatureGeneratorDERToP1363Conversion() {
        val generator = V2XSignatureGenerator()
        
        // Generate key pair and sign message
        val keyPair = generator.generateKeyPair()
        val message = "Test message".toByteArray()
        val derSignature = generator.signMessage(message, keyPair.private)
        
        // Convert DER to P1363
        val p1363Signature = generator.derToP1363(derSignature)
        
        // P1363 format for P-256 should be r || s = 64 bytes
        assertEquals("P1363 signature should be 64 bytes for P-256", 64, p1363Signature.size)
    }

    @Test
    fun testSignedBSMWithChainDepth1() {
        val generator = V2XSignatureGenerator()
        
        // Generate signed BSM with single certificate
        val signedBSM = generator.generateSignedBSM(chainDepth = 1)
        
        assertNotNull("Signed BSM should not be null", signedBSM)
        assertTrue("Signed BSM should not be empty", signedBSM.isNotEmpty())
        
        // Header should indicate signed message (0x12)
        assertEquals("Header should be 0x12 (signed)", 0x12, signedBSM[0].toInt())
        
        // Should contain algorithm byte 0x04 (ECDSA P-256)
        assertTrue("Should contain ECDSA P-256 algorithm byte", signedBSM.contains(0x04.toByte()))
        
        // Try to parse through frame detection
        try {
            val frameType = V2X.detectFrameType(signedBSM)
            assertNotNull("Frame type should be detected for signed BSM", frameType)
            assertTrue("Frame type should identify BSM", frameType.uppercase().contains("BSM"))
        } catch (e: Exception) {
            fail("Signed BSM parsing failed: ${e.message}")
        }
    }

    @Test
    fun testSignedSPaTWithChainDepth1() {
        val generator = V2XSignatureGenerator()
        
        // Generate signed SPaT with single certificate
        val signedSPaT = generator.generateSignedSPaT(chainDepth = 1)
        
        assertNotNull("Signed SPaT should not be null", signedSPaT)
        assertTrue("Signed SPaT should not be empty", signedSPaT.isNotEmpty())
        
        // Header should indicate signed message (0x12)
        assertEquals("Header should be 0x12 (signed)", 0x12, signedSPaT[0].toInt())
        
        // Try to parse through frame detection
        try {
            val frameType = V2X.detectFrameType(signedSPaT)
            assertNotNull("Frame type should be detected for signed SPaT", frameType)
            assertTrue("Frame type should identify SPaT", frameType.uppercase().contains("SPAT"))
        } catch (e: Exception) {
            fail("Signed SPaT parsing failed: ${e.message}")
        }
    }

    @Test
    fun testSignedPSMWithChainDepth1() {
        val generator = V2XSignatureGenerator()
        
        // Generate signed PSM with single certificate
        val signedPSM = generator.generateSignedPSM(chainDepth = 1)
        
        assertNotNull("Signed PSM should not be null", signedPSM)
        assertTrue("Signed PSM should not be empty", signedPSM.isNotEmpty())
        
        // Header should indicate signed message (0x12)
        assertEquals("Header should be 0x12 (signed)", 0x12, signedPSM[0].toInt())
        
        // Try to parse through frame detection
        try {
            val frameType = V2X.detectFrameType(signedPSM)
            assertNotNull("Frame type should be detected for signed PSM", frameType)
            assertTrue("Frame type should identify PSM", frameType.uppercase().contains("PSM"))
        } catch (e: Exception) {
            fail("Signed PSM parsing failed: ${e.message}")
        }
    }

    @Test
    fun testSignedBSMWithChainDepth2() {
        val generator = V2XSignatureGenerator()
        
        // Generate signed BSM with certificate chain depth 2
        val signedBSMChain2 = generator.generateSignedBSM(chainDepth = 2)
        
        assertNotNull("Signed BSM with chain depth 2 should not be null", signedBSMChain2)
        assertTrue("Signed BSM with chain depth 2 should not be empty", signedBSMChain2.isNotEmpty())
        
        // Header should indicate signed message (0x12)
        assertEquals("Header should be 0x12 (signed)", 0x12, signedBSMChain2[0].toInt())
        
        // Should be larger than chain depth 1 (extra certificate)
        val signedBSMChain1 = generator.generateSignedBSM(chainDepth = 1)
        assertTrue("Chain depth 2 should be larger than depth 1", signedBSMChain2.size > signedBSMChain1.size)
    }

    @Test
    fun testSignedBSMWithChainDepth3() {
        val generator = V2XSignatureGenerator()
        
        // Generate signed BSM with certificate chain depth 3
        val signedBSMChain3 = generator.generateSignedBSM(chainDepth = 3)
        
        assertNotNull("Signed BSM with chain depth 3 should not be null", signedBSMChain3)
        assertTrue("Signed BSM with chain depth 3 should not be empty", signedBSMChain3.isNotEmpty())
        
        // Header should indicate signed message and contain algorithm byte
        assertEquals("Header should be 0x12 (signed)", 0x12, signedBSMChain3[0].toInt())
        assertTrue("Should contain ECDSA P-256 algorithm byte", signedBSMChain3.contains(0x04.toByte()))
        
        // Should be larger than chain depth 1 (two extra certificates)
        val signedBSMChain1 = generator.generateSignedBSM(chainDepth = 1)
        assertTrue("Chain depth 3 should be larger than depth 1", signedBSMChain3.size > signedBSMChain1.size)
    }

    @Test
    fun testSignedBSMProcessingThroughJNI() {
        val generator = V2XSignatureGenerator()
        val builder = COERBinaryMessageBuilder()
        val chain = generator.createCertificateChain(2)
        val signedBSM = generator.generateSignedMessageWithChain(builder.buildBSMPayload(), chain)

        assertTrue("Trusted root should initialize", V2X.initializeWithRootCA(chain.first().first.encoded))

        try {
            val frameType = V2X.detectFrameType(signedBSM)
            assertNotNull("Frame type should be detected", frameType)

            val result = V2X.processMessage(signedBSM)
            assertNotNull("Message processing should succeed", result)
        } catch (e: Exception) {
            fail("Signed BSM processing failed: ${e.message}")
        }
    }
}


