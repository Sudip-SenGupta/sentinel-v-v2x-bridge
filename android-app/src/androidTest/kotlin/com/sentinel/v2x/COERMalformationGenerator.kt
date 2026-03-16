package com.sentinel.v2x

import java.io.ByteArrayOutputStream

/**
 * COERMalformationGenerator: Build parser-breaking COER fixtures for JNI rejection tests.
 *
 * The goal is deterministic failure against the current native parser contract, not full
 * standards-faithful corruption modeling.
 */
class COERMalformationGenerator(
    private val builder: COERBinaryMessageBuilder = COERBinaryMessageBuilder(),
    private val signatureGenerator: V2XSignatureGenerator = V2XSignatureGenerator()
) {

    fun buildTruncatedHeaderMessage(): ByteArray {
        return byteArrayOf(builder.buildHeaderByte(version = 1, isSigned = false))
    }

    fun buildTruncatedPayloadMessage(): ByteArray {
        val valid = builder.buildTestBSM()
        return valid.copyOf(valid.size - 3)
    }

    fun buildIndefiniteLengthMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        return byteArrayOf(
            builder.buildHeaderByte(version = 1, isSigned = false),
            0x80.toByte()
        ) + payload
    }

    fun buildOverflowVarintMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        return byteArrayOf(
            builder.buildHeaderByte(version = 1, isSigned = false),
            0x85.toByte(),
            0x01,
            0x02,
            0x03,
            0x04,
            0x05
        ) + payload
    }

    fun buildTruncatedLongFormVarintMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        return byteArrayOf(
            builder.buildHeaderByte(version = 1, isSigned = false),
            0x82.toByte(),
            0x01
        ) + payload
    }

    fun buildUnsupportedVersionMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        return byteArrayOf(builder.buildHeaderByte(version = 4, isSigned = false)) +
            builder.encodeVarint(payload.size) + payload
    }

    fun buildUnsupportedFrameTypeMessage(): ByteArray {
        val payload = builder.buildBSMPayload().copyOf()
        payload[0] = 0x70.toByte()
        return builder.buildUnsignedMessage(payload)
    }


    fun buildPayloadLengthOverclaimMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        return byteArrayOf(builder.buildHeaderByte(version = 1, isSigned = false)) +
            builder.encodeVarint(payload.size + 8) + payload
    }

    fun buildSignatureLengthOverclaimMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        val chain = signatureGenerator.createCertificateChain(2)
        val signer = chain.last()
        val signature = signatureGenerator.signMessage(payload, signer.second)
        val issuerCert = signer.first.encoded
        val chainCerts = chain.dropLast(1).asReversed().map { it.first.encoded }

        val remainingAfterSignature = issuerCert.size + chainCerts.sumOf { it.size } + 32
        return buildSignedMessageWithOverrides(
            payload = payload,
            signature = signature,
            issuerCert = issuerCert,
            chainCerts = chainCerts,
            signatureLengthOverride = signature.size + remainingAfterSignature
        )
    }

    fun buildIssuerCertLengthOverclaimMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        val chain = signatureGenerator.createCertificateChain(2)
        val signer = chain.last()
        val signature = signatureGenerator.signMessage(payload, signer.second)
        val issuerCert = signer.first.encoded
        val chainCerts = chain.dropLast(1).asReversed().map { it.first.encoded }

        return buildSignedMessageWithOverrides(
            payload = payload,
            signature = signature,
            issuerCert = issuerCert,
            chainCerts = chainCerts,
            issuerCertLengthOverride = issuerCert.size + 32
        )
    }

    fun buildChainCertLengthOverclaimMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        val chain = signatureGenerator.createCertificateChain(3)
        val signer = chain.last()
        val signature = signatureGenerator.signMessage(payload, signer.second)
        val issuerCert = signer.first.encoded
        val chainCerts = chain.dropLast(1).asReversed().map { it.first.encoded }

        val remainingAfterFirstChainCert = chainCerts.drop(1).sumOf { it.size } + 32
        return buildSignedMessageWithOverrides(
            payload = payload,
            signature = signature,
            issuerCert = issuerCert,
            chainCerts = chainCerts,
            firstChainCertLengthOverride = chainCerts.first().size + remainingAfterFirstChainCert
        )
    }

    fun buildChainDepthCountMismatchMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        val chain = signatureGenerator.createCertificateChain(2)
        val signer = chain.last()
        val signature = signatureGenerator.signMessage(payload, signer.second)
        val issuerCert = signer.first.encoded
        val chainCerts = chain.dropLast(1).asReversed().map { it.first.encoded }

        return buildSignedMessageWithOverrides(
            payload = payload,
            signature = signature,
            issuerCert = issuerCert,
            chainCerts = chainCerts,
            chainDepthOverride = chainCerts.size + 1
        )
    }

    fun buildTruncatedSignedContainerMessage(): ByteArray {
        val signed = signatureGenerator.generateSignedBSM(chainDepth = 2)
        return signed.copyOf(signed.size - 10)
    }

    fun buildDanglingChainDepthMessage(): ByteArray {
        val signed = signatureGenerator.generateSignedBSM(chainDepth = 1)
        return signed + byteArrayOf(0x01)
    }

    fun buildMissingSignatureAlgorithmMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        return byteArrayOf(builder.buildHeaderByte(version = 1, isSigned = true)) +
            builder.encodeVarint(payload.size) + payload
    }

    fun buildTruncatedIssuerCertLengthVarintMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        val chain = signatureGenerator.createCertificateChain(2)
        val signer = chain.last()
        val signature = signatureGenerator.signMessage(payload, signer.second)

        val result = ByteArrayOutputStream()
        result.write(builder.buildHeaderByte(version = 1, isSigned = true).toInt())
        result.write(builder.encodeVarint(payload.size))
        result.write(payload)
        result.write(0x04)
        result.write(builder.encodeVarint(signature.size))
        result.write(signature)
        result.write(0x82)
        result.write(0x01)
        return result.toByteArray()
    }

    fun buildTruncatedChainCertLengthVarintMessage(): ByteArray {
        val payload = builder.buildBSMPayload()
        val chain = signatureGenerator.createCertificateChain(2)
        val signer = chain.last()
        val signature = signatureGenerator.signMessage(payload, signer.second)
        val issuerCert = signer.first.encoded

        val result = ByteArrayOutputStream()
        result.write(builder.buildHeaderByte(version = 1, isSigned = true).toInt())
        result.write(builder.encodeVarint(payload.size))
        result.write(payload)
        result.write(0x04)
        result.write(builder.encodeVarint(signature.size))
        result.write(signature)
        result.write(builder.encodeVarint(issuerCert.size))
        result.write(issuerCert)
        result.write(0x01)
        result.write(0x82)
        result.write(0x01)
        return result.toByteArray()
    }

    fun unsignedParserRejectionCases(): List<ByteArray> = listOf(
        buildTruncatedHeaderMessage(),
        buildTruncatedPayloadMessage(),
        buildIndefiniteLengthMessage(),
        buildOverflowVarintMessage(),
        buildTruncatedLongFormVarintMessage(),
        buildUnsupportedVersionMessage(),
        buildUnsupportedFrameTypeMessage(),
        buildPayloadLengthOverclaimMessage()
    )

    fun signedParserRejectionCases(): List<ByteArray> = listOf(
        buildMissingSignatureAlgorithmMessage(),
        buildSignatureLengthOverclaimMessage(),
        buildIssuerCertLengthOverclaimMessage(),
        buildTruncatedIssuerCertLengthVarintMessage(),
        buildChainCertLengthOverclaimMessage(),
        buildTruncatedChainCertLengthVarintMessage(),
        buildChainDepthCountMismatchMessage(),
        buildTruncatedSignedContainerMessage(),
        buildDanglingChainDepthMessage()
    )

    fun allProcessMessageRejectionCases(): List<ByteArray> =
        unsignedParserRejectionCases() + signedParserRejectionCases()


    private fun buildSignedMessageWithOverrides(
        payload: ByteArray,
        signature: ByteArray,
        issuerCert: ByteArray,
        chainCerts: List<ByteArray>,
        signatureLengthOverride: Int? = null,
        issuerCertLengthOverride: Int? = null,
        firstChainCertLengthOverride: Int? = null,
        chainDepthOverride: Int? = null
    ): ByteArray {
        val result = ByteArrayOutputStream()
        result.write(builder.buildHeaderByte(version = 1, isSigned = true).toInt())
        result.write(builder.encodeVarint(payload.size))
        result.write(payload)
        result.write(0x04)
        result.write(builder.encodeVarint(signatureLengthOverride ?: signature.size))
        result.write(signature)
        result.write(builder.encodeVarint(issuerCertLengthOverride ?: issuerCert.size))
        result.write(issuerCert)

        if (chainDepthOverride != null || chainCerts.isNotEmpty()) {
            result.write((chainDepthOverride ?: chainCerts.size) and 0xFF)
            chainCerts.forEachIndexed { index, cert ->
                val certLength = if (index == 0 && firstChainCertLengthOverride != null) {
                    firstChainCertLengthOverride
                } else {
                    cert.size
                }
                result.write(builder.encodeVarint(certLength))
                result.write(cert)
            }
        }

        return result.toByteArray()
    }
}
