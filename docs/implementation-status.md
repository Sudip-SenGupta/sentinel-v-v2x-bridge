# Implementation Status

## Current Verified Baseline

The current validated baseline is:
- branch: `feature/v2x-parser-and-crypto-hardening`
- task: `:android-app:connectedDebugAndroidTest`
- result: `67` Android instrumentation tests passing

## Verification Snapshot

| Item | Current Value |
| --- | --- |
| Branch | `feature/v2x-parser-and-crypto-hardening` |
| Latest verified suite | `:android-app:connectedDebugAndroidTest` |
| Latest verified test count | `67` |
| Last verified environment | Android instrumentation on `Sentinel_Car(AVD) - 14` |
| Validation emphasis | parser-compatible COER fixtures, JNI/native processing, signed-message verification, malformed rejection, certificate-chain enforcement |

Related docs:
- [Test Plan](test-plan.md) for coverage details
- [Architecture](architecture.md) for system structure
- [Limitations and Deviations](limitations-and-deviations.md) for remaining gaps and risks

## Branch And Validation Context

Phase 1 work was completed on the feature branch and verified on the Android emulator path used throughout this effort. The branch now contains parser-compatible fixture generation, malformed-input coverage, certificate-chain hardening, JNI verification-path fixes, and documentation wrap-up.

## Completed Work Summary

Completed implementation areas:
- COER binary fixture generation for BSM, SPaT, and PSM
- signed-message fixture generation with real certificate chains
- malformed COER generator and grouped rejection catalogs
- native Botan PKIX validation and explicit trust-anchor enforcement
- end-to-end signed-message rejection coverage through `V2X.processMessage(...)`
- canonical documentation skeletons for consolidation

## Historical Foundation

Important earlier milestones that remain relevant to the current branch:
- native Botan-backed crypto integration established the SHA-256, ECDSA, and X.509 validation foundation used by later JNI hardening work
- build portability improvements added support for external Botan discovery and prepared the build logic for future non-Linux packaging scenarios
- earlier Android/JNI integration work established the active `v2x-jni` path that this branch hardened rather than replaced

These earlier phases are no longer the active planning documents, but their deliverables remain part of the implementation baseline.

## Phase 1 Summary

### Phase 1A
- Implemented `COERBinaryMessageBuilder.kt`
- Added parser-compatible unsigned and signed fixture assembly

### Phase 1B
- Implemented `V2XSignatureGenerator.kt`
- Added BouncyCastle-issued root/intermediate/leaf test chains
- Added explicit trusted-root handling and Botan PKIX validation through JNI

### Phase 1C
- Implemented `COERMalformationGenerator.kt`
- Added truncation, invalid-varint, bad-version, signed-container, and length-overclaim rejection coverage

### Phase 1D
- Added grouped valid/invalid execution coverage in `V2XJNITest.kt`
- Hardened `processMessage(...)` to fail closed before JNI marshalling
- Linked `v2x_message_processor.cpp` and `v2x_payload_validator.cpp` into the Android JNI target
- Fixed signer public-key extraction and DER ECDSA verification in native crypto
- Added 5 end-to-end negative signed-message rejection tests

### Phase 1E
- Completed Phase 1 documentation wrap-up after the Phase 1D implementation work was already finished
- Recorded final success criteria and deferred work
- Established a smaller canonical documentation set

## Test Status

Current verified criteria:
- valid unsigned BSM, SPaT, and PSM fixtures detect correctly
- valid signed BSM processes end to end through `V2X.processMessage()`
- malformed COER fixtures fail gracefully
- trust, ordering, time-validity, and leaf-policy checks fail closed
- negative signed-message integration coverage fails closed through `V2X.processMessage()`

## Key Fixes Delivered

Most important fixes shipped in the current branch:
- real issuer-signed certificate chains in Android tests
- explicit trusted-root initialization and clearing through Kotlin/JNI
- native chain validation changed from parse-only to Botan PKIX validation
- serialized chain-order and CA constraints enforced
- JNI `processMessage(...)` now runs the native verification pipeline before returning decoded objects
- conditional payload validation allows parser-compatible signed payloads while still validating DER-wrapped payloads when present

## Open Issues

There are no known Phase 1 blocking issues in the verified baseline. Remaining concerns are deferred hardening items rather than regressions in the tested path.

## Deferred Work

Deferred items after Phase 1:
- revocation handling and policy
- stronger EKU or certificate-profile enforcement
- scoped trust-state ownership instead of global native trust state
- semantic validation of BSM, SPaT, and PSM payload contents
- richer JNI/native error taxonomy

## Regression Strategy

The current regression guardrail is the verified Phase 1 Android instrumentation baseline of `67` passing tests.

Expected policy for future work:
- parser-compatible fixture coverage must continue to pass before merging unrelated UI or feature work
- signed-message negative and positive JNI paths remain part of the non-regression baseline
- future dashboard or higher-level application work should not bypass native verification-path guarantees
- documentation consolidation should not change the technical baseline claimed by the test suite

A practical rule for future integration is:
- all existing Digital Twin / parser-and-crypto hardening fixtures should pass in CI or pre-merge validation before any larger feature branch is considered stable

## Next Engineering Steps

The most natural next steps are:
- start Phase 2 semantic validation for payload-level rules
