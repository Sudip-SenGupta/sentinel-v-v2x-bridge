# TEST Vectors

## Scope

This document indexes the deterministic Android instrumentation fixtures used to validate the V2X JNI bridge, COER parser, and native crypto policy paths.

Current verified checkpoint:
- Phase 1A, 1B, 1C complete
- Phase 1D aggregation verified
- Phase 1D end-to-end negative signed-message extension verified
- Phase 1E documentation wrap-up complete
- `67` tests is the final verified Phase 1 full-suite checkpoint

## Valid Unsigned Fixtures

| Fixture | Builder | Expected Result |
| --- | --- | --- |
| `unsigned-bsm` | `COERBinaryMessageBuilder.buildTestBSM()` | Frame detection and decode succeed |
| `unsigned-spat` | `COERBinaryMessageBuilder.buildTestSPaT()` | Frame detection and decode succeed |
| `unsigned-psm` | `COERBinaryMessageBuilder.buildTestPSM()` | Frame detection and decode succeed |

## Valid Signed Fixtures

| Fixture | Builder | Expected Result |
| --- | --- | --- |
| `signed-bsm-depth1` | `V2XSignatureGenerator.generateSignedBSM(1)` | Signed BSM parses |
| `signed-spat-depth1` | `V2XSignatureGenerator.generateSignedSPaT(1)` | Signed SPaT parses |
| `signed-psm-depth1` | `V2XSignatureGenerator.generateSignedPSM(1)` | Signed PSM parses |
| `signed-bsm-depth2` | `V2XSignatureGenerator.generateSignedBSM(2)` | Larger than depth 1 and parses |
| `signed-bsm-depth3` | `V2XSignatureGenerator.generateSignedBSM(3)` | Larger than depth 1 and parses |
| `signed-shared-chain-bsm` | `generateSignedMessageWithChain(buildBSMPayload(), sharedChain)` | `processMessage()` succeeds with configured root and DER ECDSA verification |
| `signed-shared-chain-spat` | `generateSignedMessageWithChain(buildSPaTPayload(), sharedChain)` | `processMessage()` succeeds with configured root |
| `signed-shared-chain-psm` | `generateSignedMessageWithChain(buildPSMPayload(), sharedChain)` | `processMessage()` succeeds with configured root |

## Crypto Policy Negative Fixtures

These cases now run both at chain-validation level and, for signed BSM fixtures, through end-to-end `V2X.processMessage(...)` rejection coverage.

| Category | Fixture Strategy | Expected Result |
| --- | --- | --- |
| Trust anchor | Valid chain with wrong configured root | `validateCertificateChain()` fails |
| Chain ordering | Reordered root/intermediate/leaf bytes | `validateCertificateChain()` fails |
| Intermediate policy | Intermediate marked non-CA | `validateCertificateChain()` fails |
| Leaf policy | Leaf marked CA | `validateCertificateChain()` fails |
| Leaf policy | Leaf without `digitalSignature` key usage | `validateCertificateChain()` fails |
| Time validity | Expired leaf | `validateCertificateChain()` fails |
| Time validity | Not-yet-valid leaf | `validateCertificateChain()` fails |
| Trust lifecycle | Clear trusted root then revalidate | Validation fails after clear |
| Signed message trust anchor | Valid signed BSM with wrong configured root | `processMessage()` fails |
| Signed message intermediate policy | Signed BSM with non-CA intermediate | `processMessage()` fails |
| Signed message time validity | Signed BSM with expired or not-yet-valid leaf | `processMessage()` fails |
| Signed message leaf policy | Signed BSM with missing `digitalSignature` key usage | `processMessage()` fails |

## Malformed COER Rejection Catalog

### Unsigned Parser Rejection Cases

- Truncated header
- Truncated payload
- Indefinite-length varint
- Oversized varint length-of-length
- Truncated long-form varint
- Unsupported protocol version
- Unsupported frame type
- Payload length overclaim

### Signed Parser Rejection Cases

- Missing signature algorithm byte
- Signature length overclaim
- Issuer certificate length overclaim
- Truncated issuer certificate length varint
- Chain certificate length overclaim
- Truncated chain certificate length varint
- Chain depth/count mismatch
- Truncated signed container
- Dangling chain depth byte

## Phase 1D Aggregation And Phase 1D Extension

### Phase 1D Orchestration (4 tests)
Grouped test flows:
- grouped valid unsigned fixture execution through `V2X.processMessage()`
- grouped valid signed fixture execution through `V2X.processMessage()` with a shared trusted root
- grouped malformed catalog rejection through `V2X.processMessage()`
- repeated valid unsigned batch execution through `V2X.processBatch()` for basic stability coverage

### Phase 1D Extension: Negative Signed-Message Integration (5 tests)
End-to-end rejection coverage through full `V2X.processMessage()` pipeline:

| Test | Certificate Issue | Failure Mode | JNI Test |
| --- | --- | --- | --- |
| Wrong Trust Root | Valid chain with different root CA | Chain validation fails | `testSignedBSMWithWrongTrustedRootRejectedThroughJNI` |
| Non-CA Intermediate | Intermediate marked non-CA | CA constraint violation | `testSignedBSMWithNonCAIntermediateRejectedThroughJNI` |
| Expired Leaf | Leaf notAfter < current time | Time validity check fails | `testSignedBSMWithExpiredLeafRejectedThroughJNI` |
| Not-Yet-Valid Leaf | Leaf notBefore > current time | Time validity check fails | `testSignedBSMWithNotYetValidLeafRejectedThroughJNI` |
| Leaf Policy Violation | Leaf missing `digitalSignature` key usage | Policy validation fails | `testSignedBSMWithLeafPolicyViolationRejectedThroughJNI` |

**Payload Validator Fix:**
- Conditional DER validation enables test payloads to parse correctly
- Parser-compatible payloads (frame types 0x10/0x20/0x30): accepted as-is
- DER-wrapped payloads (SEQUENCE tag 0x30): validated strictly

## Next Expansion Options

- Phase 2: semantic validation for BSM, SPaT, and PSM payload content
- production PKI hardening: revocation, EKU/profile enforcement, scoped trust ownership
- larger stress and soak suites with mixed valid and malformed fixture batches
- explicit parser/error taxonomy if JNI/native paths begin returning structured error codes


