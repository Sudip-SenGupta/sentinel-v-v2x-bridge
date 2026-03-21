# Phase 3 Completion Report

**Status:** Complete  
**Date:** March 19, 2026  
**Branch:** `feature/phase-3-coer-decoder`  
**Regression Gate:** Android instrumentation suite passing at 67 tests; native off-target tests passing

## Executive Summary

Phase 3 formalized the decoder boundary that already existed in the branch and reduced ownership leakage between envelope parsing, frame interpretation, JNI marshalling, and processor orchestration.

The result is a cleaner three-layer stateless utility architecture:

- `COERDecoder` owns envelope parsing, structure validation, and signed-container extraction
- `V2XFrameDecoder` owns frame detection, frame decoding, and frame-layer exception types
- `V2XMessageProcessor` owns orchestration, verification, and verified decoded output

This phase did not attempt a standards-complete IEEE 1609.2 implementation. It focused on making the current project-specific decoder path explicit, testable, and easier to evolve.

## What Phase 3 Accomplished

### 1. Architecture Boundary Cleanup

Phase 3 established and reinforced the following boundary:

```text
COERDecoder -> V2XFrameDecoder -> V2XMessageProcessor
 envelope       frame logic        orchestration
```

Key outcomes:

- `V2XFrameDecoder` is now the explicit owner of frame detection and frame decoding
- frame-layer exceptions are owned by `V2XFrameDecoder`, not reused from `COERDecoder`
- `V2XMessageProcessor` now returns decoded output for verified messages
- JNI no longer reparses verified messages in the single-message and batch paths

### 2. Public API Tightening

The main utility classes are now explicitly stateless:

- `COERDecoder() = delete`
- `V2XFrameDecoder() = delete`
- `V2XMessageProcessor() = delete`

Additional API cleanup completed:

- `COERDecoder` helper/debug surface was reduced
- internal string-conversion helpers and `log_message_structure(...)` are no longer public
- JNI now uses the static `V2XMessageProcessor::process_message(...)` API consistently

### 3. Test-Surface Cleanup

Phase 3 reduced direct test dependence on parsed-message internals:

- `test_phase3_decoder_boundary.cpp` uses `COERDecoder::get_payload(...)`
- `test_coer_decoder_vectors.cpp` now routes payload access through `COERDecoder::get_payload(...)` in the cleaned suites/sections
- earlier parser/component cases use decoder-owned chain extraction instead of direct field access where applicable

This keeps tests closer to the decoder's public contract and reduces coupling to `COERMessage` layout.

### 4. Boundary-Test Coverage

Phase 3 added or strengthened native off-target boundary coverage for:

- parser-valid unknown frame types
- frame-layer detection failures
- decoded output from `V2XMessageProcessor`
- fail-closed processor behavior when frame decoding throws on an otherwise valid envelope
- decoder-owned signed-component extraction
- malformed signed-container truncation failures

These tests complement the Android instrumentation regression gate by exercising the decoder/frame/processor boundary directly in native code.

## Regression Gate Status

Verified during Phase 3 work:

- Android instrumentation suite: passing at `67` tests
- native off-target tests: passing

This report does not claim a globally warning-free toolchain state; only the verified regression gates above.

## Files Touched by Phase 3

### Core implementation

- `native-engine/include/v2x_coer_decoder.h`
- `native-engine/src/v2x_coer_decoder.cpp`
- `native-engine/include/v2x_frame_decoder.h`
- `native-engine/src/v2x_message_frame.cpp`
- `native-engine/include/v2x_message_processor.h`
- `native-engine/src/v2x_message_processor.cpp`
- `native-engine/src/v2x_jni_message_processor.cpp`
- `native-engine/src/v2x_crypto_engine.cpp`

### Tests

- `native-engine/tests/test_message_processor_integration.cpp`
- `native-engine/tests/test_phase3_decoder_boundary.cpp`
- `native-engine/tests/test_coer_decoder_vectors.cpp`
- `native-engine/tests/test_v2x_message_frame_integration.cpp`
- `native-engine/tests/CMakeLists.txt`

### Documentation

- `docs/phase-3-baseline.md`
- `docs/architecture.md`

## Commit History

Major Phase 3 commits on this branch:

1. `9b6e342` `Refactor JNI message processing around decoded processor results`
2. `043e9c1` `Introduce V2X frame decoder boundary`
3. `2af25d2` `Add native Phase 3 boundary tests`
4. `2747599` `Tighten Phase 3 decoder utility interfaces`
5. `8202ead` `Route payload access through decoder contract`
6. `0337040` `Give V2X frame decoding its own exception surface`
7. `751aab3` `Use decoder payload accessor in boundary tests`
8. `eaf74d9` `Wrap decoder test payload access in Suites 1,2 & 3`
9. `ea3cb31` `Expand decoder accessor use in vector tests`
10. `6bff797` `Add fail-closed frame decode boundary test`

## What Phase 3 Did Not Do

Deferred beyond Phase 3:

- deterministic invalid-signature processor-boundary coverage
- deterministic chain-validation failure boundary coverage
- trust-anchor enforcement expansion
- semantic payload validation beyond the current project contract
- standards-complete IEEE 1609.2 or library-backed COER support

These are better treated as follow-on crypto-boundary and policy work rather than continued Phase 3 structural cleanup.

## Recommendation

Phase 3 is a good stopping point.

The decoder/frame/processor boundary is now materially cleaner, native boundary coverage is stronger, and the Android regression baseline is preserved.

The recommended next phase is to move to crypto-boundary work:

- invalid signature -> processor fail-closed
- chain validation failure -> processor fail-closed
- trust-anchor failure handling

That work should be treated as the start of the next phase rather than more decoder-boundary cleanup.
