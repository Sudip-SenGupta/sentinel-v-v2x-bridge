# Phase 4 Baseline

## Purpose

This note captures the recommended starting point for the next phase after the Phase 3 decoder-boundary cleanup on `feature/phase-3-coer-decoder`.

Phase 4 should focus on crypto-boundary hardening rather than more parser-boundary cleanup. The decoder, frame-decoder, and processor split is now explicit enough that the next high-value work is to prove fail-closed behavior across signature-verification and chain-validation failure modes.

Related docs:
- [Phase 3 Baseline](phase-3-baseline.md) for the completed decoder-boundary work
- [Phase 3 Completion Report](PHASE-3-COMPLETION-REPORT.md) for the current stopping point
- [Architecture](architecture.md) for the system boundary between decoder, processor, and crypto engine
- [Low-Level Design](low-level-design.md) for the signed-message flow and current verification pipeline
- [Test Plan](test-plan.md) for the existing Android regression coverage
- [Implementation Status](implementation-status.md) for the verified branch baseline

## Why Phase 4 Starts Here

Phase 3 already established:
- explicit ownership for `COERDecoder`, `V2XFrameDecoder`, and `V2XMessageProcessor`
- fail-closed handling at the frame-decoder boundary
- native off-target boundary coverage for parser-valid and frame-invalid cases
- preserved Android instrumentation coverage at `67` tests

The next material risk is no longer decoder ownership. It is the processor-to-crypto boundary:
- invalid signature handling
- certificate-chain validation failure handling
- trust-anchor failure handling
- consistent fail-closed results from `V2XMessageProcessor`

## Current Crypto Reality

The current branch already supports:
- signed-message extraction through `COERDecoder`
- signature verification through `V2XCryptoEngine::verify_ecdsa_signature(...)`
- certificate-chain validation through `V2XCryptoEngine::validate_certificate_chain(...)`
- end-to-end signed-message rejection in Android instrumentation for several negative trust/time cases

The current branch does not yet clearly guarantee, in focused native boundary tests:
- deterministic invalid-signature processor fail-closed behavior
- deterministic chain-validation processor fail-closed behavior
- deterministic trust-anchor failure behavior at the processor boundary

That is the main gap Phase 4 should close.

## Phase 4 Scope

### In scope
- deterministic processor-boundary coverage for signature verification failures
- deterministic processor-boundary coverage for chain-validation failures
- deterministic processor-boundary coverage for trust-anchor failure handling
- verification of `MessageVerificationResult` fail-closed behavior for those cases
- minimal supporting fixture work required to make those tests repeatable

### Out of scope
- semantic payload validation expansion
- revocation, EKU, or full production PKI policy work unless explicitly chosen later
- standards-complete IEEE 1609.2 decoding
- library-backed COER evaluation
- additional frame-type expansion

## Phase 4 Targets

### Target 1: Invalid signature -> processor fail-closed
- introduce a deterministic signed-message test path whose envelope and structure remain valid but whose signature verification must fail
- verify `V2XMessageProcessor::process_message(...)` returns fail-closed with a signature-stage error

Definition of done:
- native off-target test proves invalid signature bytes do not break parsing but do cause processor rejection
- Android regression gate still passes unchanged

### Target 2: Chain validation failure -> processor fail-closed
- add deterministic coverage for a message whose signature stage succeeds first, but whose certificate-chain validation fails
- if a real fixture cannot reliably reach the chain stage, introduce a minimal controlled seam as a separate preparatory cut rather than blurring the target definition
- verify processor rejection happens at the chain stage, not earlier

Definition of done:
- native off-target test proves chain-validation failure produces fail-closed processor output with a chain-stage error
- Android regression gate still passes unchanged

### Target 3: Trust-anchor failure handling
- focus on processor/native trust-anchor validation first, not the JNI initialization boundary
- cover the two distinct cases separately:
  - wrong configured trust anchor
  - missing trust-anchor state
- verify processor rejection happens at the trust-anchor stage with appropriate fail-closed output

Definition of done:
- native off-target tests prove wrong configured trust anchor and missing trust-anchor state both produce fail-closed processor output with trust-stage errors
- Android regression gate still passes unchanged
- JNI trust-anchor initialization behavior remains an integration concern unless native gaps appear

## Existing Coverage to Reuse

Useful current coverage already in the repo:
- Android instrumentation already rejects signed messages for negative trust/time scenarios
- JNI crypto tests already include low-level invalid-input signature rejection
- native decoder/frame boundary tests already demonstrate fail-closed handling for frame-layer failures

Phase 4 should reuse those facts where possible, but should add narrower processor-to-crypto boundary tests rather than depending only on broad Android instrumentation.

## Task 0 Outcome

Task 0 is complete.

Findings:
- existing signed fixture generation is strongest on the Android test side through `V2XSignatureGenerator`
- real-fixture tampering is conceptually possible because the signed container layout is explicit and length-delimited
- existing native signed vectors are parser-oriented placeholder fixtures and are not the best direct vehicle for real crypto-boundary verification

Conclusion:
- the first Phase 4 slice uses a minimal processor-side crypto seam for native off-target coverage rather than porting the Android fixture generator into C++
- this seam is test-only and is used to force deterministic signature-verification failure after parsing succeeds

## Current Completed Slice

The first Phase 4 slice has already completed:
- a minimal test-only crypto seam in `V2XMessageProcessor`
- a dedicated native off-target suite: `test_phase4_crypto_boundary.cpp`
- first Target 1 coverage: invalid signature fails closed before chain validation

What that first test now proves:
- the signed message is parsed successfully
- structure validation succeeds
- signature verification can fail deterministically at the processor boundary
- chain validation is not reached after signature failure
- `V2XMessageProcessor` returns fail-closed output with a signature-stage error

## Regression Gate

All Phase 4 refactoring must preserve:
- native off-target test pass status
- the Android instrumentation baseline at `67` tests

## Test Organization for Phase 4

### Native off-target test file
- Use `native-engine/tests/test_phase4_crypto_boundary.cpp`
- Rationale: Phase 3 boundary tests focus on decoder/frame/processor structural concerns; Phase 4 has its own crypto-boundary suite for clarity and future expansion

### Android test expectations
- Existing `67`-test instrumentation suite is the regression gate and must continue to pass unchanged
- New Android crypto-boundary tests are optional initially
- Add new Android tests only if native coverage exposes a gap or requires JNI-specific fixtures
- This keeps Phase 4 focused on off-target crypto-boundary work first

## Recommended Next Step

The next implementation step is:
- extend `test_phase4_crypto_boundary.cpp` with Target 2: chain validation failure -> processor fail-closed

That should reuse the same seam pattern and keep the slice narrow.

After that:
- add Target 3 trust-anchor failure coverage in separate native tests
- only then decide whether Android-specific crypto-boundary additions are necessary
