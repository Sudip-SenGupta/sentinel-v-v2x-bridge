# Phase 3 Baseline

## Purpose

This note captures the actual Phase 3 starting point on `feature/phase-3-coer-decoder`.
It describes what the decoder does today, which files own the current behavior, and what has already been cleaned up before larger decoder changes are attempted.

Related docs:
- [Architecture](architecture.md) for the system structure around the decoder path
- [Low-Level Design](low-level-design.md) for the current binary contract and JNI/native mechanics
- [Implementation Status](implementation-status.md) for the verified 67-test baseline and current branch context
- [Test Plan](test-plan.md) for the existing regression coverage that Phase 3 must preserve

## Current Decoder Reality

The repository already contains a working custom decoder path. Phase 3 is therefore not about inventing decoding from nothing. It is about formalizing and cleaning up a parser-compatible decoder that already exists.

The current implementation is best described as:
- a custom project-specific COER-like decoder
- tightly aligned to the current Android/JNI/native message path
- already sufficient to support the current 67-test validated Phase 1 baseline
- narrower than a fully general IEEE 1609.2 COER implementation

## Current Wire Contract

The current message contract is:
- `1 byte` header
- `varint` payload length
- `payload` bytes
- if signed:
  - `1 byte` signature algorithm
  - `varint` signature length
  - `signature` bytes
  - `varint` issuer certificate length
  - `issuer certificate` bytes
  - optional `1 byte` chain depth
  - repeated `varint + cert bytes` for certificate-chain entries

Current header interpretation:
- upper nibble = protocol version
- `0x02` bit indicates signature present
- `0x04` bit indicates encryption present

Current signed-container assumptions:
- signer certificate is serialized first
- chain entries follow in leaf-to-root parent order
- issuer certificate and chain entries are X.509 DER byte blobs
- signature bytes are later verified through native crypto using the extracted signer certificate

## Current Parser Ownership

### Primary parser
- `native-engine/include/v2x_coer_decoder.h`
- `native-engine/src/v2x_coer_decoder.cpp`

Current responsibilities:
- low-level byte and varint reading
- header parsing
- payload extraction
- signed-container extraction
- structure validation
- component extraction helpers

### Message processing pipeline
- `native-engine/src/v2x_message_processor.cpp`

Current responsibilities:
- call `COERDecoder::parse(...)`
- enforce decoder-owned structure validation
- access payload bytes through the decoder contract
- apply payload validation policy
- invoke native crypto verification and chain validation
- call `V2XFrameDecoder` for verified frame interpretation
- return fail-closed verification results with decoded output

### Payload frame decoding
- `native-engine/include/v2x_frame_decoder.h`
- `native-engine/src/v2x_message_frame.cpp`

Current responsibilities:
- detect frame type from payload bytes
- decode BSM, SPaT, and PSM payload structures
- apply project-specific payload layout assumptions
- keep frame interpretation separate from envelope parsing
- own frame-layer exception types instead of reusing decoder exceptions

## Completed In This Slice

The current Phase 3 branch has already completed several structural cleanup slices plus a decoder-only coverage expansion:
- `V2XMessageProcessor` now enforces decoder structure validation immediately after parse
- signed-component extraction is centralized behind `COERDecoder::extract_signed_components(...)`
- `MessageVerificationResult` now carries decoded output for verified messages
- JNI `processMessage(...)` no longer reparses and redecodes verified COER bytes
- JNI `processBatch(...)` no longer reparses and redecodes verified COER bytes
- `V2XFrameDecoder` now owns frame type detection and frame decoding
- `V2XFrameDecoder` now owns frame-layer exception types and no longer depends on decoder exceptions
- native off-target boundary tests now cover parser-valid unknown frame types, frame-layer detection failures, and decoded output from `V2XMessageProcessor`
- decoder-only coverage now locks down `extract_signed_components(...)` and signed-container truncation failures in `test_coer_decoder_vectors.cpp`
- `COERDecoder`, `V2XFrameDecoder`, and `V2XMessageProcessor` are now explicitly non-instantiable utility classes
- JNI now calls `V2XMessageProcessor::process_message(...)` consistently through the static API instead of constructing throwaway processor instances
- `COERDecoder` helper surface is smaller: string-conversion helpers and `log_message_structure(...)` are now internal, while `set_debug_logging(...)` remains available for the native integration tests
- `V2XMessageProcessor` now routes payload access through `COERDecoder::get_payload(...)` instead of reaching into parsed message state directly
- the non-Android logging fallback in `v2x_crypto_engine.cpp` now supports the existing printf-style logging call sites
- the native off-target test target passes
- the Android Phase 1 regression gate still passes at `67` instrumentation tests
- `test_coer_decoder_vectors.cpp` Test Suites 1-2 now route payload access through `COERDecoder::get_payload(...)` and chain access through `COERDecoder::extract_certificate_chain(...)` rather than direct field access; this establishes a disciplined test-surface boundary

## Current Validation Behavior

The current decoder and processor path already enforce:
- payload-length boundary checks
- malformed varint rejection
- signed-container truncation rejection
- basic structure validation on signature and certificate fields
- fail-closed signed-message processing through the native verification pipeline
- separate frame-layer handling for unknown frame types and frame-detection failures
- decoder-owned signed-component extraction and signed-container truncation handling covered by off-target tests

The current design does not yet imply:
- general IEEE 1609.2 interoperability
- standards-complete semantic payload validation
- revocation-aware trust decisions
- library-backed or spec-complete COER ownership

## Key Observations

### What is already good
- the decoder is real and exercised by Android instrumentation coverage
- signed-message extraction already feeds the crypto engine successfully
- malformed input coverage is already broad enough to support controlled refactoring
- the processor and JNI layers now have a cleaner verified-message boundary
- frame interpretation now has an explicit `V2XFrameDecoder` boundary
- frame failures now surface through frame-owned exceptions instead of decoder exceptions
- native off-target coverage now exists for the parser/frame boundary in addition to the Android regression gate
- decoder-only tests now cover signed-component extraction and malformed signed-container edges directly
- utility-class intent is now explicit in the public headers, and JNI no longer pretends `V2XMessageProcessor` is stateful
- payload access inside the processor now follows the decoder contract more consistently

### What is currently messy
- parsing knowledge is split across decoder, message processor, and frame decoder
- some comments and naming overstate IEEE 1609.2 generality
- payload semantics and parser contract are mixed together in places
- the current decoder contract is implemented, but not yet explicitly treated as the current Phase 3 ownership model

## First Refactor Targets

The first Phase 3 cleanup should be structural, not feature-heavy.

### Target 1: Make the decoder contract explicit
- treat the current wire format as the official baseline for this branch
- document which parts are project-specific versus standards-derived
- stop relying on older planning docs as the source of truth

Definition of done for this target:
- the decoder contract is documented clearly enough that native/off-target tests can assert parse and extraction behavior without routing through the JNI path

### Target 2: Clarify parsing ownership by file
- `v2x_coer_decoder.cpp` should own raw message parsing and signed-container extraction
- `v2x_message_processor.cpp` should own verification orchestration and verified decoded output
- `v2x_message_frame.cpp` should own frame decoding, not top-level message parsing semantics

Definition of done for this target:
- the decoder can be unit-tested in isolation in native/off-target tests using the current parser-compatible fixture corpus, and it produces the same parse and extraction behavior expected by the verified JNI path

### Target 3: Separate parser validity from semantic validity
- parser validity means the bytes conform to the current message contract
- semantic validity means decoded values are meaningful at the V2X domain level
- Phase 3 should keep these concerns clearly separated

### Target 4: Reduce overclaimed standards language
- comments should distinguish between current custom decoder behavior and future standards goals
- code should not claim broad IEEE 1609.2 coverage where only the current project contract is implemented

## Recommended Next Step

The recommended next implementation step is:
- treat the current decoder, processor, and frame-decoder split as the active Phase 3 baseline
- then choose the next small decoder-only or ownership-focused cut without changing the wire contract

This is safer than either:
- rewriting the decoder immediately
- or adopting an external library before the current contract is fully mapped

## Suggested Immediate Work Items

1. Keep the Phase 3 baseline note aligned with the implemented `V2XFrameDecoder` boundary, the expanded native off-target tests, and the reduced decoder helper surface.
2. Review `v2x_coer_decoder.cpp`, `v2x_frame_decoder.h`, and `v2x_message_frame.cpp` for any remaining ownership leakage.
3. Treat the next cut as a narrow ownership or decoder-only coverage change unless a larger gap is found.
4. Only after that, decide whether a library evaluation is still justified.

## Library Evaluation Gate

Library evaluation is not the default first move for Phase 3.

It should only occur if the refactored custom decoder fails to meet one or more of these conditions for the intended Android/native runtime path:
- correctness against the current validated parser-compatible contract
- maintainability and ownership clarity after the refactor
- measured performance requirements for the target message-processing path

This keeps the initial Phase 3 effort focused on formalizing and cleaning up the existing custom decoder before introducing an external dependency decision.

## Regression Gate

All Phase 3 refactoring must preserve the existing 67-test Android instrumentation baseline.
The current validated suite remains the non-regression gate for decoder ownership changes, parser refactors, and signed-message extraction cleanup.

## Agreed Next Refactor Plan

The next Phase 3 structural cut should preserve the current behavior and 67-test regression baseline while continuing to reduce ownership leakage between envelope parsing, frame interpretation, and higher-level orchestration.

### Current Boundary
- `COERDecoder` owns envelope parsing, signed-container extraction, and structure validation
- `V2XFrameDecoder` owns frame-type detection and payload-to-frame decoding
- `V2XFrameDecoder` owns frame-layer exceptions
- `V2XMessageProcessor` owns orchestration, verification, and verified decoded output
- JNI now marshals processor results instead of reparsing the message
- JNI now uses the static processor API consistently
- payload access now flows through `COERDecoder::get_payload(...)` instead of ad hoc field access in the processor
- native off-target tests exercise the parser/frame boundary directly
- decoder-only vector tests exercise signed-component extraction and malformed signed-container handling directly
- decoder debug and string helpers are no longer all part of the public contract

### Non-Goals
- no change to the current wire contract
- no semantic validation expansion in this slice
- no crypto-policy changes
- no library-evaluation work until the current custom decoder boundary is fully stabilized
