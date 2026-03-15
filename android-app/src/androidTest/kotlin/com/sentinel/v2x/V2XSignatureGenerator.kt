package com.sentinel.v2x

import java.math.BigInteger
import java.security.KeyPair
import java.security.KeyPairGenerator
import java.security.PrivateKey
import java.security.SecureRandom
import java.security.Security
import java.security.Signature
import java.security.cert.X509Certificate
import java.security.spec.ECGenParameterSpec
import java.util.Date
import javax.security.auth.x500.X500Principal
import org.bouncycastle.asn1.x509.BasicConstraints
import org.bouncycastle.asn1.x509.Extension
import org.bouncycastle.asn1.x509.KeyUsage
import org.bouncycastle.cert.jcajce.JcaX509CertificateConverter
import org.bouncycastle.cert.jcajce.JcaX509ExtensionUtils
import org.bouncycastle.cert.jcajce.JcaX509v3CertificateBuilder
import org.bouncycastle.jce.provider.BouncyCastleProvider
import org.bouncycastle.operator.jcajce.JcaContentSignerBuilder

/**
 * V2XSignatureGenerator: Generate parser-compatible ECDSA signatures and certificate chains
 *
 * Produces signatures and certificates compatible with:
 * - COER binary format (algorithm byte 0x04 = ECDSA P-256)
 * - Botan crypto verification (v2x_crypto_engine.cpp)
 * - parser-compatible signature container format
 *
 * Signature container format (from v2x_coer_decoder.cpp):
 * - 1 byte algorithm (0x04 = ECDSA P-256, 0x05 = ECDSA P-384)
 * - varint signature length
 * - signature bytes (raw DER from Java, or P1363 format for Botan)
 * - varint issuer cert length
 * - issuer certificate bytes (X.509 DER)
 * - optional 1 byte chain depth
 * - repeated (varint cert_length + cert bytes) for chain
 */
class V2XSignatureGenerator {

    companion object {
        private const val KEY_ALGORITHM = "EC"
        private const val CURVE = "secp256r1"
        private const val SIGNATURE_ALGORITHM = "SHA256withECDSA"
        private const val SIGNATURE_ALGORITHM_BYTE: Byte = 0x04
        private const val VALIDITY_DAYS = 365
    }

    private val random = SecureRandom()
    private val bcProvider = BouncyCastleProvider()

    init {
        if (Security.getProvider(bcProvider.name) == null) {
            Security.addProvider(bcProvider)
        }
    }

    /**
     * Generate P-256 ECDSA key pair
     */
    fun generateKeyPair(): KeyPair {
        val keyGen = KeyPairGenerator.getInstance(KEY_ALGORITHM)
        val spec = ECGenParameterSpec(CURVE)
        keyGen.initialize(spec, random)
        return keyGen.generateKeyPair()
    }

    /**
     * Create self-signed X.509 certificate.
     */
    fun createSelfSignedCertificate(
        subject: String,
        keyPair: KeyPair,
        validity: Int = VALIDITY_DAYS
    ): X509Certificate {
        return buildCertificate(
            subject = X500Principal(subject),
            subjectKeyPair = keyPair,
            issuerCert = null,
            issuerPrivateKey = keyPair.private,
            validity = validity,
            isCa = false
        )
    }

    private fun buildCertificate(
        subject: X500Principal,
        subjectKeyPair: KeyPair,
        issuerCert: X509Certificate?,
        issuerPrivateKey: PrivateKey,
        validity: Int,
        isCa: Boolean
    ): X509Certificate {
        val issuer = issuerCert?.subjectX500Principal ?: subject
        val now = System.currentTimeMillis()
        val notBefore = Date(now - 60_000L)
        val notAfter = Date(now + validity.toLong() * 24L * 60L * 60L * 1000L)
        val serial = BigInteger(160, random).abs().max(BigInteger.ONE)
        val extensions = JcaX509ExtensionUtils()

        val builder = JcaX509v3CertificateBuilder(
            issuer,
            serial,
            notBefore,
            notAfter,
            subject,
            subjectKeyPair.public
        )

        builder.addExtension(Extension.basicConstraints, true, BasicConstraints(isCa))
        val keyUsageBits = if (isCa) {
            KeyUsage.keyCertSign or KeyUsage.cRLSign or KeyUsage.digitalSignature
        } else {
            KeyUsage.digitalSignature
        }
        builder.addExtension(Extension.keyUsage, true, KeyUsage(keyUsageBits))
        builder.addExtension(
            Extension.subjectKeyIdentifier,
            false,
            extensions.createSubjectKeyIdentifier(subjectKeyPair.public)
        )
        builder.addExtension(
            Extension.authorityKeyIdentifier,
            false,
            extensions.createAuthorityKeyIdentifier(issuerCert?.publicKey ?: subjectKeyPair.public)
        )

        val signer = JcaContentSignerBuilder(SIGNATURE_ALGORITHM)
            .setProvider(bcProvider)
            .build(issuerPrivateKey)
        val holder = builder.build(signer)

        return JcaX509CertificateConverter()
            .setProvider(bcProvider)
            .getCertificate(holder)
    }


    /**
     * Generate ECDSA(SHA-256) signature over message
     *
     * Returns signature in DER format (as provided by Java)
     */
    fun signMessage(
        message: ByteArray,
        privateKey: PrivateKey
    ): ByteArray {
        val signer = Signature.getInstance(SIGNATURE_ALGORITHM)
        signer.initSign(privateKey)
        signer.update(message)
        return signer.sign()
    }

    /**
     * Convert DER signature to P1363 format (raw r || s)
     *
     * ECDSA signatures in Java come as DER SEQUENCE { r INTEGER, s INTEGER }
     * Some systems (like Botan) expect raw r || s bytes (P1363 format)
     */
    fun derToP1363(derSignature: ByteArray): ByteArray {
        if (derSignature.isEmpty() || derSignature[0] != 0x30.toByte()) {
            throw IllegalArgumentException("DER signature must start with SEQUENCE (0x30)")
        }

        var pos = 1
        pos += derLengthBytes(derSignature, pos)

        if (derSignature[pos++] != 0x02.toByte()) {
            throw IllegalArgumentException("Expected INTEGER (0x02) for r")
        }
        val rLengthBytes = derLengthBytes(derSignature, pos)
        val rLen = readDerLengthValue(derSignature, pos)
        pos += rLengthBytes
        val r = derSignature.copyOfRange(pos, pos + rLen)
        pos += rLen

        if (derSignature[pos++] != 0x02.toByte()) {
            throw IllegalArgumentException("Expected INTEGER (0x02) for s")
        }
        val sLengthBytes = derLengthBytes(derSignature, pos)
        val sLen = readDerLengthValue(derSignature, pos)
        pos += sLengthBytes
        val s = derSignature.copyOfRange(pos, pos + sLen)

        val r32 = padTo32Bytes(r)
        val s32 = padTo32Bytes(s)

        return r32 + s32
    }

    private fun derLengthBytes(data: ByteArray, offset: Int): Int {
        val first = data[offset].toInt() and 0xFF
        return if ((first and 0x80) == 0) 1 else 1 + (first and 0x7F)
    }

    private fun readDerLengthValue(data: ByteArray, offset: Int): Int {
        val first = data[offset].toInt() and 0xFF
        if ((first and 0x80) == 0) {
            return first
        }

        val count = first and 0x7F
        var value = 0
        for (i in 0 until count) {
            value = (value shl 8) or (data[offset + 1 + i].toInt() and 0xFF)
        }
        return value
    }

    /**
     * Pad byte array to 32 bytes (strip leading zeros, left-pad with zeros)
     */
    private fun padTo32Bytes(bytes: ByteArray): ByteArray {
        var start = 0
        while (start < bytes.size - 1 && bytes[start] == 0.toByte()) {
            start++
        }

        val stripped = bytes.copyOfRange(start, bytes.size)
        return if (stripped.size >= 32) {
            stripped.copyOfRange(stripped.size - 32, stripped.size)
        } else {
            ByteArray(32 - stripped.size) + stripped
        }
    }

    /**
     * Create a real certificate hierarchy.
     *
     * Depth 1: self-signed leaf signer
     * Depth 2: root CA -> leaf signer
     * Depth 3: root CA -> intermediate CA -> leaf signer
     */
    fun createCertificateChain(depth: Int = 1): List<Pair<X509Certificate, PrivateKey>> {
        require(depth in 1..3) { "Chain depth must be 1-3, got $depth" }

        if (depth == 1) {
            val leafKeyPair = generateKeyPair()
            val leafCert = createSelfSignedCertificate(
                subject = "CN=Custom Leaf Signer, O=Sentinel V2X, C=US",
                keyPair = leafKeyPair,
                validity = VALIDITY_DAYS
            )
            return listOf(leafCert to leafKeyPair.private)
        }

        val rootKeyPair = generateKeyPair()
        val rootCert = buildCertificate(
            subject = X500Principal("CN=Custom Root CA, O=Sentinel V2X, C=US"),
            subjectKeyPair = rootKeyPair,
            issuerCert = null,
            issuerPrivateKey = rootKeyPair.private,
            validity = VALIDITY_DAYS,
            isCa = true
        )

        if (depth == 2) {
            val leafKeyPair = generateKeyPair()
            val leafCert = buildCertificate(
                subject = X500Principal("CN=Custom Leaf Signer, O=Sentinel V2X, C=US"),
                subjectKeyPair = leafKeyPair,
                issuerCert = rootCert,
                issuerPrivateKey = rootKeyPair.private,
                validity = VALIDITY_DAYS,
                isCa = false
            )
            return listOf(
                rootCert to rootKeyPair.private,
                leafCert to leafKeyPair.private
            )
        }

        val intermediateKeyPair = generateKeyPair()
        val intermediateCert = buildCertificate(
            subject = X500Principal("CN=Custom Intermediate CA, O=Sentinel V2X, C=US"),
            subjectKeyPair = intermediateKeyPair,
            issuerCert = rootCert,
            issuerPrivateKey = rootKeyPair.private,
            validity = VALIDITY_DAYS,
            isCa = true
        )

        val leafKeyPair = generateKeyPair()
        val leafCert = buildCertificate(
            subject = X500Principal("CN=Custom Leaf Signer, O=Sentinel V2X, C=US"),
            subjectKeyPair = leafKeyPair,
            issuerCert = intermediateCert,
            issuerPrivateKey = intermediateKeyPair.private,
            validity = VALIDITY_DAYS,
            isCa = false
        )

        return listOf(
            rootCert to rootKeyPair.private,
            intermediateCert to intermediateKeyPair.private,
            leafCert to leafKeyPair.private
        )
    }

    /**
     * Generate complete signed COER message
     *
     * Returns: [header] [payload_length] [payload] [sig_algo] [sig_len] [signature]
     *          [issuer_cert_len] [issuer_cert] [chain_depth] [chain_certs...]
     */
    fun generateSignedMessage(
        payload: ByteArray,
        chainDepth: Int = 1,
        signatureFormatP1363: Boolean = false
    ): ByteArray {
        val chain = createCertificateChain(chainDepth)
        return generateSignedMessageWithChain(payload, chain, signatureFormatP1363)
    }

    fun generateSignedMessageWithChain(
        payload: ByteArray,
        chain: List<Pair<X509Certificate, PrivateKey>>,
        signatureFormatP1363: Boolean = false
    ): ByteArray {
        require(chain.isNotEmpty()) { "Certificate chain must not be empty" }

        val signingEntry = chain.last()
        val signature = signMessage(payload, signingEntry.second)

        val finalSignature = if (signatureFormatP1363) {
            derToP1363(signature)
        } else {
            signature
        }

        val signerCert = signingEntry.first
        val parentChain = if (chain.size > 1) {
            chain.dropLast(1).asReversed().map { it.first.encoded }
        } else {
            emptyList()
        }

        val builder = COERBinaryMessageBuilder()
        return builder.buildSignedMessage(
            payload = payload,
            signature = finalSignature,
            issuerCert = signerCert.encoded,
            chainCerts = parentChain,
            signatureAlgorithm = SIGNATURE_ALGORITHM_BYTE
        )
    }

    fun generateSignedBSM(chainDepth: Int = 1): ByteArray {
        val builder = COERBinaryMessageBuilder()
        val payload = builder.buildBSMPayload()
        return generateSignedMessage(payload, chainDepth)
    }

    fun generateSignedSPaT(chainDepth: Int = 1): ByteArray {
        val builder = COERBinaryMessageBuilder()
        val payload = builder.buildSPaTPayload()
        return generateSignedMessage(payload, chainDepth)
    }

    fun generateSignedPSM(chainDepth: Int = 1): ByteArray {
        val builder = COERBinaryMessageBuilder()
        val payload = builder.buildPSMPayload()
        return generateSignedMessage(payload, chainDepth)
    }
}

/**
 * Unit tests for V2XSignatureGenerator
 */
class V2XSignatureGeneratorTest {
    private val generator = V2XSignatureGenerator()

    fun testKeyPairGeneration() {
        val keyPair = generator.generateKeyPair()
        assert(keyPair.private != null) { "Private key should not be null" }
        assert(keyPair.public != null) { "Public key should not be null" }
    }

    fun testCertificateChainGeneration() {
        val chain1 = generator.createCertificateChain(1)
        assert(chain1.size == 1) { "Chain depth 1 should have 1 cert" }

        val chain2 = generator.createCertificateChain(2)
        assert(chain2.size == 2) { "Chain depth 2 should have 2 certs" }

        val chain3 = generator.createCertificateChain(3)
        assert(chain3.size == 3) { "Chain depth 3 should have 3 certs" }
    }

    fun testSignatureGeneration() {
        val keyPair = generator.generateKeyPair()
        val message = "Test message".toByteArray()

        val signature = generator.signMessage(message, keyPair.private)
        assert(signature.isNotEmpty()) { "Signature should not be empty" }
        assert(signature[0] == 0x30.toByte()) { "DER signature should start with SEQUENCE (0x30)" }
    }

    fun testDERToP1363Conversion() {
        val keyPair = generator.generateKeyPair()
        val message = "Test".toByteArray()

        val derSignature = generator.signMessage(message, keyPair.private)
        val p1363Signature = generator.derToP1363(derSignature)

        assert(p1363Signature.size == 64) { "P1363 signature should be 64 bytes for P-256, got ${p1363Signature.size}" }
    }

    fun testSignedBSMGeneration() {
        val signedBSM = generator.generateSignedBSM(chainDepth = 1)
        assert(signedBSM.isNotEmpty()) { "Signed BSM should not be empty" }
        assert(signedBSM.contains(0x04.toByte())) { "Should contain ECDSA P-256 algorithm byte" }
    }

    fun testSignedBSMWithChain() {
        val signedBSMChain1 = generator.generateSignedBSM(chainDepth = 1)
        val signedBSMChain2 = generator.generateSignedBSM(chainDepth = 2)

        assert(signedBSMChain1.isNotEmpty()) { "Chain depth 1 should generate message" }
        assert(signedBSMChain2.isNotEmpty()) { "Chain depth 2 should generate message" }
        assert(signedBSMChain2.size >= signedBSMChain1.size) {
            "Chain depth 2 should have more bytes than depth 1"
        }
    }

    fun runAllTests() {
        println("Running V2XSignatureGenerator tests...")
        testKeyPairGeneration()
        println("✓ Key pair generation")
        testCertificateChainGeneration()
        println("✓ Certificate chain generation")
        testSignatureGeneration()
        println("✓ Signature generation")
        testDERToP1363Conversion()
        println("✓ DER to P1363 conversion")
        testSignedBSMGeneration()
        println("✓ Signed BSM generation")
        testSignedBSMWithChain()
        println("✓ Signed BSM with chain")
        println("All tests passed!")
    }
}
