# Incremental ESP32-P4 Hardware Validation Path

Issue: [#44 Spec: Incremental ESP32-P4 hardware validation path](https://github.com/ChaseWoodhams/watchpup-kvm/issues/44)

Date: 2026-07-10

Related decisions and specs:

- [ADR 0001: Product Vision And V1 Boundary](../adrs/0001-product-vision-and-v1-boundary.md)
- [ADR 0002: Initial Firmware Platform And Hardware Path](../adrs/0002-initial-firmware-platform-and-hardware-path.md)
- [ADR 0003: Firmware Subsystems And Diagnostics First](../adrs/0003-firmware-subsystems-and-diagnostics-first.md)
- [ADR 0004: Frame Ownership And MJPEG Backpressure](../adrs/0004-frame-ownership-and-mjpeg-backpressure.md)
- [ADR 0005: Web Assets And HID Control Safety](../adrs/0005-web-assets-and-hid-control-safety.md)
- [Initial Hardware Configuration Contract](../hardware/initial-hardware-configuration-contract.md)
- [TC358743 Bridge Driver Design Notes](../hardware/tc358743-bridge-driver-design-notes.md)
- [Diagnostics Endpoint Contract](../firmware/diagnostics-endpoint-contract.md)
- [MJPEG Frame Ownership Contract](../firmware/mjpeg-frame-ownership-contract.md)
- [HID Control Protocol And Safety Spec](../firmware/hid-control-protocol-and-safety-spec.md)

## Purpose

This specification defines the first trustworthy, incrementally testable hardware-validation path for WatchPup's selected ESP32-P4 bench stack.

It exists to prevent the project from treating video, network, authentication, and HID as one opaque bring-up exercise. Each phase must isolate failure at its natural boundary, produce concrete evidence, and block dependent work until the current gate passes.

This is a sequencing and validation spec, not a directive to implement the full KVM path in one issue.

## Problem Statement

WatchPup needs to move from documentation and a firmware skeleton to a hardware path that can be trusted by evidence rather than assumption.

Without an ordered validation plan, cable mistakes, bridge negotiation problems, CSI ingest failures, auth regressions, and HID safety defects can collapse into one end-to-end debugging loop. That would make failures hard to diagnose and expensive to unwind.

## Decision Summary

WatchPup should deliver the first hardware-proving KVM path as ordered validation-gated slices.

The required delivery order is:

1. physical-interface qualification
2. board health
3. HDMI bridge diagnostics
4. raw CSI ingest
5. authenticated HTTP
6. authenticated still JPEG
7. single-viewer MJPEG
8. MJPEG backpressure and recovery
9. supervised keyboard HID
10. supervised relative-mouse HID
11. end-to-end soak and recovery

Each slice must produce:

- one scoped change,
- one repeatable bench procedure,
- serial and `/diag` evidence where applicable,
- one explicit pass/fail result, and
- a bounded diagnose -> revise -> retest loop before dependent work starts if the gate fails.

ATX power/reset, paste, and hotkeys remain deferred until after the core path is proven.

## Selected Bench Stack

The initial validation path assumes this bench hardware:

- UeeKKoo ESP32-P4-ETH development board
- Waveshare HDMI-to-CSI adapter

This stack is the first hardware-proving path only. It does not lock the project into a final PCB, enclosure, or integrated production design.

## Boundary And Constraints

This spec preserves the existing WatchPup v1 posture:

- ESP32-P4 plus TC358743 hardware path first
- Ethernet-first networking
- MJPEG-first video
- diagnostics before streaming polish
- read-only video before HID if sequencing risk needs to stay low
- one authenticated local administrator model for non-diagnostic HTTP work
- human-reviewed agent PRs before merge

This spec does not authorize:

- production hardening work,
- Wi-Fi or cloud access,
- H.264,
- multi-user or multi-controller input,
- ATX implementation,
- broad browser-UI polish,
- final hardware-product decisions.

## User Stories

1. As a bench operator, I want the exact board, bridge, cable, connector, orientation, and power arrangement documented, so that hardware setup is reproducible.
2. As a firmware developer, I want physical-interface uncertainty resolved before capture code is written, so that cable and pinout problems are not misdiagnosed as firmware defects.
3. As a bench operator, I want a reliable flash and reflash procedure, so that every experiment starts from a known firmware state.
4. As a firmware developer, I want serial diagnostics from first boot, so that board state can be inspected when network access is unavailable.
5. As a bench operator, I want `/diag` to show board, PSRAM, Ethernet, and subsystem health, so that each phase has machine-readable evidence.
6. As a developer, I want PSRAM and Ethernet proven before video work, so that later failures have a narrow fault boundary.
7. As a bench operator, I want the canonical host tested at BIOS/UEFI and desktop, so that WatchPup proves the pre-boot use case as well as normal video.
8. As a developer, I want TC358743 identity, reset, EDID, HPD, negotiation, and lock state observable, so that HDMI source negotiation can be diagnosed independently of CSI ingest.
9. As a bench operator, I want repeatable bridge signal lock before capture begins, so that raw-frame failures are attributable to the CSI boundary.
10. As a developer, I want raw CSI frame counters, format, dimensions, drop data, and changing-frame evidence, so that real pixel ingest is proven before compression is introduced.
11. As a local operator, I want no default network credential, so that a newly flashed device is not exposed with a known password.
12. As a bench operator, I want first-run administrator provisioning through physical serial access, so that setup does not create an unauthenticated LAN configuration surface.
13. As a local operator, I want authenticated browser sessions for non-diagnostic routes, so that snapshots, streams, and later input are not available anonymously.
14. As a bench operator, I want an authenticated still JPEG before continuous streaming, so that image validity, legibility, color/order, performance, and PSRAM usage can be tested independently.
15. As a viewer, I want one stable authenticated MJPEG stream, so that I can observe the canonical host continuously.
16. As a viewer, I want slow or disconnected viewers not to stall capture or encoding, so that a healthy viewer continues to receive current frames.
17. As a developer, I want stream pressure, skips, disconnects, and recovery visible in diagnostics, so that the latest-frame policy can be verified rather than assumed.
18. As a supervised operator, I want authenticated single-owner keyboard input, so that I can perform basic BIOS and desktop actions without ambiguous control.
19. As a supervised operator, I want all keys released on every loss or error condition, so that WatchPup cannot leave a host in a stuck-key state.
20. As a supervised operator, I want relative mouse movement and buttons after keyboard safety is proven, so that BIOS-compatible pointer control can be added with equivalent release guarantees.
21. As a developer, I want deliberate HDMI, Ethernet, viewer, session, and reboot fault tests, so that recovery behavior is proven before the core path expands.
22. As a project maintainer, I want each completed gate to leave concise evidence and an explicit decision, so that future slices can build on verified facts.

## Validation Principles

Every gate in this path follows these rules:

- A good test observes externally meaningful behavior rather than merely internal implementation structure.
- The canonical host is the required validation host for every video and HID gate.
- The canonical host must be exercised in both BIOS or UEFI and normal desktop states.
- A gate that fails is revised and retested before dependent work begins.
- Later compatibility work does not substitute for the canonical host gate.

Accepted evidence may include:

- serial logs,
- `/diag` output,
- authenticated browser retrieval,
- screenshots,
- concise bench observations,
- explicit pass/fail notes.

## Phase Order

### Phase 0: Physical-Interface Qualification

Goal:
Resolve physical cable, pinout, orientation, connector, lane, and power uncertainty before capture-specific firmware work.

Required outcomes:

- exact board SKU recorded
- exact HDMI-to-CSI adapter SKU recorded
- connector type, pin count, pitch, and FPC orientation documented
- lane information documented
- safe power arrangement documented
- canonical bench assembly procedure documented

Hard gate:
Do not begin capture-specific firmware work until the physical interconnect is documented and judged safe to test.

### Phase 1: Board Health

Goal:
Prove reflashing, serial diagnostics, PSRAM, Ethernet DHCP, and the initial `/diag` shape.

Required outcomes:

- repeatable flash and reflash procedure
- serial diagnostics from first boot
- PSRAM presence and health evidence
- Ethernet bring-up and DHCP evidence
- `/diag` shape aligned with the diagnostics contract

Hard gate:
Do not begin bridge or capture implementation until board health evidence exists.

### Phase 2: HDMI Bridge Diagnostics

Goal:
Prove TC358743 control and source negotiation independently of CSI ingest.

Required outcomes:

- I2C identity visibility
- reset behavior observable where available
- EDID handling evidence
- HPD behavior evidence
- HDMI detect and negotiated mode visibility
- bridge lock or equivalent readiness visibility

Hard gate:
Do not begin raw capture validation until bridge signal lock is repeatable.

### Phase 3: Raw CSI Ingest

Goal:
Prove changing raw CSI frames before compression is introduced.

Required outcomes:

- raw frame counters
- format and dimensions
- drop or error counters
- changing-frame evidence such as a fingerprint or diagnostic sample

Hard gate:
Do not begin JPEG or streaming work until real changing pixel ingest is proven.

### Phase 4A: Authenticated HTTP

Goal:
Prove local auth posture before video surfaces are exposed.

Required outcomes:

- no default network credential
- first-run administrator provisioning through physical serial access
- session creation and persistence
- anonymous rejection for non-diagnostic routes

Bootstrap exception:
Unauthenticated read-only diagnostics may exist only in an explicitly identified bootstrap build on an isolated bench network or direct Ethernet link. It must expose no setup, video, HID, power, or control capability.

Hard gate:
Do not expose non-diagnostic HTTP routes anonymously.

### Phase 4B: Authenticated Still JPEG

Goal:
Validate one authenticated current-frame JPEG before continuous streaming.

Required outcomes:

- authenticated retrieval of the current JPEG
- image validity and legibility evidence
- color or ordering sanity evidence
- performance and PSRAM-usage observations
- related diagnostics where available

Hard gate:
Do not begin MJPEG until authenticated still-frame validation passes.

### Phase 5A: Single-Viewer MJPEG

Goal:
Prove one stable authenticated MJPEG viewer.

Required outcomes:

- one authenticated viewer retrieves a live stream
- continuous observation of the canonical host is possible
- diagnostics correlate stream activity with device health

Hard gate:
Do not treat multi-viewer or pressure behavior as solved by a single happy-path stream.

### Phase 5B: MJPEG Backpressure And Recovery

Goal:
Prove bounded latest-frame behavior under slow, stalled, and disconnected viewers.

Required outcomes:

- slow clients skip frames or disconnect
- capture and encode remain healthy
- resources recover after disconnect
- diagnostics show skips, disconnects, pressure, and recovery behavior

Hard gate:
Do not proceed to HID until stream pressure behavior is observable and bounded.

### Phase 6: Supervised Keyboard HID

Goal:
Prove authenticated single-owner keyboard input with mandatory forced release on every safety-relevant failure.

Required outcomes:

- keyboard enumeration evidence
- single-owner control evidence
- intentional BIOS and desktop input evidence
- forced release on disconnect, timeout, ownership loss, auth loss, protocol error, and emergency release

Hard gate:
Do not proceed to mouse until keyboard safety is proven.

### Phase 7: Supervised Relative-Mouse HID

Goal:
Add relative BIOS-compatible mouse movement and buttons with the same safety posture as keyboard.

Required outcomes:

- relative movement evidence
- button behavior evidence
- forced release behavior equivalent to keyboard

Hard gate:
Do not broaden pointer modes or multi-controller behavior in this phase.

### Phase 8: End-To-End Soak And Recovery

Goal:
Run extended authenticated video-plus-HID sessions while deliberately inducing expected faults.

Required faults:

- HDMI loss
- Ethernet loss
- viewer disconnect
- session loss
- device reboot

Required outcomes:

- recovery behavior is observable
- input is not retained across fault boundaries
- resources do not leak across recovery loops
- the core path remains bounded and diagnosable

## Subsystem Expectations

This path preserves the existing subsystem boundaries:

- `board_config`
- `bridge_tc358743`
- `capture`
- `encoder_mjpeg`
- `frame_store`
- `stream_server`
- `diagnostics`
- `hid_device`
- `control_protocol`
- `auth_security`
- `app_main`

Serial diagnostics and schema-versioned `/diag` are the cross-phase health seam. The same subsystem names and statuses should correlate between both surfaces.

## Authentication And Security Posture

The first authenticated HTTP path should use:

- no default credential,
- one locally provisioned administrator account,
- first-run provisioning through the physical serial console,
- suitable password hashing,
- secure random browser session tokens,
- authenticated access for every non-diagnostic route.

This spec does not define the exact backend implementation. It fixes the minimum posture that later implementation tickets must preserve.

## Video Publication Posture

This spec keeps snapshot and streaming separate:

- The first public video artifact is an authenticated current-frame JPEG.
- MJPEG follows only after snapshot validation passes.
- MJPEG uses bounded latest-frame publication.
- Slow clients skip frames or disconnect instead of blocking capture or encode.

## HID Posture

Keyboard and mouse are separate phases.

Both phases must preserve:

- authenticated single-active-controller policy,
- explicit ownership,
- forced release on disconnect,
- forced release on timeout,
- forced release on ownership loss,
- forced release on auth loss,
- forced release on protocol error,
- forced release on emergency release.

## Bench Evidence Requirements

Each completed slice should leave concise evidence and an explicit decision suitable for later implementation issues.

Minimum evidence package per slice:

- scoped change summary,
- repeatable bench procedure,
- serial and `/diag` evidence where applicable,
- screenshot or concise direct observation where applicable,
- explicit pass/fail statement,
- next blocked or unblocked slice.

## Non-Goals

This specification intentionally excludes:

- final custom PCB or enclosure decisions,
- production BOM finalization,
- H.264,
- Wi-Fi,
- virtual media,
- cloud or internet exposure,
- production HTTPS, update, or hardening work,
- multi-user auth or role models,
- multi-controller HID,
- ATX power/reset before the core gate passes,
- text paste and keyboard hotkeys before the core path is proven,
- polished browser UX beyond the minimum authenticated validation surfaces.

## Follow-On Issue Guidance

The first independently actionable follow-up from this spec should be Phase 0 physical-interface qualification.

Every later implementation issue should:

- name the phase it satisfies,
- state the gate it is trying to pass,
- name the evidence expected on success,
- identify the next blocked dependency,
- avoid claiming later phases as complete by implication.
