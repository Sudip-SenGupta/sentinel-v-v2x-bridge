# Limitations And Deviations

## Standards Deviations

The current implementation does not attempt full standards-faithful IEEE 1609.2 or J2735 semantic encoding. It validates a parser-compatible COER-like framing model that matches the current native parser contract.

Related docs:
- [High-Level Design](high-level-design.md) for the rationale behind these choices
- [Low-Level Design](low-level-design.md) for the implementation consequences
- [Implementation Status](implementation-status.md) for what is currently verified

## Parser-Compatible Versus Standards-Faithful Behavior

Phase 1 deliberately optimized for the existing parser path:
- synthetic fixtures mirror the current COER frame layout
- signed messages mirror the trailer structure the parser and JNI path expect
- message content is treated primarily as parser input, not as fully modeled standards-faithful payload semantics

This means the current test suite proves frame-level robustness and crypto-policy enforcement more strongly than semantic correctness of traffic data.

## Current JNI Scope Limits

The Android JNI path is focused on message processing and crypto validation for the active Android library module. End-to-end signed-message processing is strongest for BSM, while SPaT and PSM remain better covered at frame-detection and parsing depth.

## Current Trust Model Limits

Trust-anchor handling is explicit and fail-closed, but the trusted root is still stored in shared native process state so new crypto-engine instances and message-processing flows can use the same configuration. This is acceptable for the current design, but it is not the final form of trust-state ownership.

## PKI Hardening Not Yet Implemented

The following PKI features remain deferred:
- revocation checking
- broader certificate-profile enforcement
- stronger EKU-based signer policy
- richer trust-store management and lifecycle
- more expressive native/JNI error classification for trust failures

## No Revocation Handling

There is no CRL or OCSP integration today. Trust decisions currently rely on:
- signature verification
- explicit trust-anchor configuration
- chain structure and CA constraints
- certificate validity windows

## Limited Semantic Payload Validation

The current phase does not validate whether decoded data is semantically correct or physically plausible. Examples of still-missing semantic checks:
- BSM latitude and longitude ranges
- BSM speed and heading plausibility
- SPaT state and phase-transition validity
- PSM attribute-level semantic correctness

This is the main gap between parser correctness and domain correctness.

## Transport And Dataset Assumptions

The parser accepts raw COER message bytes. It does not abstract transport-layer extraction.

This matters for future real-data ingestion:
- PCAP or live transport sources must first extract the raw V2X payload bytes
- transport encapsulation is deployment-specific
- UDP 4500 should not be assumed as a universal V2X transport format

Any future PCAP-based validation path must document the extraction assumptions separately from parser behavior.

## Tooling And Packaging Constraints

Current tooling and packaging constraints include:
- Linux/WSL remains the most reliable environment for full native development
- Android validation is centered on JNI integration and instrumentation tests
- BouncyCastle is test-only in the Android instrumentation source set
- Android packaging excludes duplicate BouncyCastle metadata resources
- full production Android packaging for all native crypto configurations is still not the focus of this branch

## Dependency And Licensing Strategy

The current dependency strategy is intentional:
- Botan is treated as an external dependency, not vendored into the repository
- third-party attribution is maintained through `THIRD-PARTY-LICENSES/`
- repository size and crypto transparency are preferred over bundling library source
- the preferred model is to link against a known Botan installation rather than embed third-party source directly into the repository

This keeps the repository smaller, keeps library ownership clearer, and avoids pretending project code and third-party crypto source are the same thing.

This also makes security and compliance review simpler because the project can point clearly to dependency attribution and installation guidance instead of hiding a crypto library snapshot inside project source.

## Known Gaps By Category

| Category | Current State | Impact | Planned Direction |
| --- | --- | --- | --- |
| Semantic payload validation | BSM, SPaT, and PSM payloads are only lightly validated beyond parser compatibility | Domain-invalid values may still parse successfully | Add payload-level semantic rules in the next phase |
| Revocation handling | No CRL or OCSP support | Trust decisions do not account for revoked certificates | Define revocation policy and data sources |
| Trust-state ownership | Trusted root is stored in shared native process state | Multi-context or concurrent trust isolation is weak | Move to scoped trust-state ownership |
| Certificate policy enforcement | Basic chain, CA, and time-validity checks exist, but richer EKU/profile enforcement is deferred | Some policy-invalid signers may not be rejected as strictly as a production stack would require | Add stronger signer-policy checks |
| Error reporting | Fail-closed behavior exists, but JNI/native error taxonomy is still narrow | Troubleshooting and UI-level handling are less precise | Introduce structured failure categories |
| Performance tuning | Current JNI marshalling and verification path favors correctness over throughput optimization | High-rate runtime traffic may need additional tuning | Revisit batching, buffer reuse, and caching once semantics stabilize |
| Transport/data ingestion | Parser expects raw V2X payload bytes and does not model transport extraction | Real-world capture ingestion requires external preprocessing assumptions | Document and implement transport extraction paths separately |
## Known Technical Debt

Known follow-up items include:
- replace process-global trust state with scoped ownership
- continue clarifying legacy API names where semantics are narrower than the name implies
- keep the canonical documentation set aligned with future implementation changes
- define stronger performance and batching guidance for higher-frequency message processing

## Productionization Risks

Before a production-grade deployment, the project still needs decisions and implementation in these areas:
- revocation policy and online/offline behavior
- semantic payload validation policy
- production signer-profile enforcement
- transport and dataset handling for real-world message capture inputs
- long-lived trust-state and lifecycle management
- clear merge and regression gates as additional features are added above the validated parser and crypto baseline

A practical engineering lesson from the earlier roadmap material is that future options or feature branches should be judged by what they actually validate: frame handling, crypto enforcement, semantic correctness, UI/demo value, or real-data ingestion. Those are different goals and should remain documented as such.
