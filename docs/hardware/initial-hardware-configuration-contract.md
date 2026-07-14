# Initial Hardware Configuration Contract

Issue: [#23 Document initial hardware configuration contract](https://github.com/ChaseWoodhams/watchpup-kvm/issues/23)

Date: 2026-07-09

Related decisions and research:

- [ADR 0002: Initial Firmware Platform And Hardware Path](../adrs/0002-initial-firmware-platform-and-hardware-path.md)
- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)
- [WatchPup KVM DIY Cost And BOM Research](../research/watchpup-diy-cost-bom.md)

## Purpose

This document records the first hardware contract for WatchPup's firmware bring-up path before hardware-specific implementation starts.

It is intentionally narrow. It captures the initial ESP32-P4 plus external TC358743 path used to de-risk firmware and board bring-up. It does not lock the project into a final integrated board design.

## In Scope

The initial hardware contract assumes:

- ESP32-P4 Ethernet dev-board class target hardware.
- External TC358743 HDMI-to-MIPI CSI adapter.
- Wired Ethernet only.
- PSRAM required.
- A USB device-capable port is available for TinyUSB keyboard and mouse emulation.

This matches the direction accepted in [ADR 0002](../adrs/0002-initial-firmware-platform-and-hardware-path.md) and the reference-shaped path described in the [p4kvm reference analysis](../research/p4kvm-reference-analysis.md).

## Resolution And EDID Posture

The initial bring-up posture is:

- Fixed 1080p30 EDID is allowed as a bring-up tool.
- 720p is the reliability and default target.
- 1080p is validation and detail mode only, not the default operating assumption.

This means early firmware may use a fixed 1080p30 EDID to simplify source negotiation while still treating stable 720p capture as the primary success target. Any later configurable EDID or wider mode support is separate work.

## Required Board Assumptions

The first hardware-dependent implementation may proceed only when these assumptions are true for the selected ESP32-P4 board plus TC358743 adapter stack:

- The board includes or exposes working wired Ethernet.
- The board includes usable PSRAM in a supported configuration.
- The board can power the chosen HDMI-to-CSI adapter and maintain stable capture-path signaling.
- The board exposes the interfaces needed for TC358743 control and MIPI CSI ingest.
- The board exposes a USB port or connector path usable in device mode for HID emulation.

If any of these assumptions change, this contract must be updated before firmware implementation depends on them.

## Required Fill-Ins Before Implementation

The following items are intentionally unknown until the exact board and adapter combination is selected and verified. They must be filled in explicitly, not guessed in code:

| Item | Required fill-in |
| --- | --- |
| ESP32-P4 board SKU | Exact dev board or module carrier selected for bring-up. |
| TC358743 adapter SKU | Exact external adapter board and connector/cable stack. |
| I2C pins | GPIO numbers used to control the TC358743. |
| Reset pin | Whether the bridge has a controllable reset pin, and if so which GPIO drives it and what polarity/timing it needs. |
| MIPI lane count | Number of CSI data lanes actually wired and expected by firmware. |
| MIPI lane rate | Verified operating lane rate for the selected board and adapter path. |
| Clocking | Reference clock source and any board-specific clock assumptions required by the bridge path. |
| FPC orientation | Cable orientation, pin-1 alignment, and any adapter inversion/crossing assumptions. |
| Ethernet PHY details | PHY part, interface mode, reset behavior, address, and any board-specific RMII or clock details firmware must honor. |
| USB device port | Which physical USB port is used for HID device mode and whether external cabling or jumpers are required. |
| Power assumptions | Input voltage, rail sharing, adapter power source, and whether any external power injection is required or forbidden. |

Unknowns should be resolved in documentation and board configuration boundaries before adding firmware logic that depends on them.

## Selected Bench Bring-Up Stack

The initial hardware-validation stack is now selected for bench work:

- **ESP32-P4 board:** Waveshare ESP32-P4-ETH (SKU `32086`). The connected board has 32 MB PSRAM, 32 MB NOR flash, 100 Mbps IP101 Ethernet, a 22-pin two-lane MIPI-CSI connector, and USB 2.0 OTG.
- **HDMI-to-CSI adapter:** Waveshare HDMI to CSI Camera Adapter (ASIN `B08TR7R6RL`), advertised for HDMI input up to 1080p30.
- **CSI interconnect:** Raspberry Pi 5-style CSI camera cable, 22-pin 0.5 mm pitch to 15-pin 1 mm pitch. A DSI/display cable is not an acceptable substitute.

This locks the first bench-validation target, not a production board design. The board schematic establishes IP101 PHY address 1, PHY power GPIO51, MDC GPIO31, MDIO GPIO52, external RMII clock GPIO50, and the ESP32-P4 default RMII data pins. Cable contact orientation and the adapter's bridge revision remain to be physically inspected before CSI capture.

Phase 2 firmware fill-ins for this bench stack are now recorded as follows:

- TC358743 control bus: I2C0, SDA GPIO7, SCL GPIO8, 7-bit address `0x0f`.
- CSI output: two lanes, with `972 Mbps` per lane retained as the configured
  bring-up value from the reference path; this remains to be validated by the
  raw-ingest gate.
- Bridge reference clock: 27 MHz.
- External bridge reset: not exposed by the selected adapter/cable path, so
  reset is reported as unsupported; HPD is controlled through the bridge's
  register interface.

## Canonical Video Test Source

Bench validation will use one representative host as the canonical HDMI source. Every video milestone must be checked against both:

- the host's BIOS/UEFI screen, which represents the pre-boot KVM use case; and
- a normal operating-system desktop.

This source is the reference for repeatable bring-up evidence. Additional hosts and unusual modes are compatibility testing, not a substitute for passing the canonical source gate.

## Bench Validation Discipline

Each hardware-dependent implementation slice is a validation gate. A slice is complete only when it includes:

1. the scoped firmware or configuration change;
2. a repeatable bench procedure using the selected hardware and canonical video source;
3. evidence such as serial diagnostics, `/diag` output, a screenshot, or a concise recorded observation; and
4. an explicit pass or fail result against that slice's acceptance criteria.

On failure, do not begin a dependent slice. Run a bounded diagnose, revise, and retest loop for the failing boundary, recording the observed symptom and the next hypothesis. The loop ends either with passing evidence or a documented blocker that changes the planned slice.

## Phase 0: Physical-Interface Qualification

Before flashing capture-specific firmware, qualify the physical capture path without assuming that Raspberry Pi CSI accessories are electrically or mechanically interchangeable with the selected ESP32-P4 board.

Phase 0 must record:

- the exact MIPI-CSI connector type, pin count, pitch, and pin-1 marking on the ESP32-P4 board;
- the Waveshare adapter's output connector type and the exact FPC cable or adapter used between the two boards;
- FPC orientation at both ends, including photographs or diagrams that make reversed insertion obvious;
- the intended lane count and any information available about lane mapping; and
- the power source, voltage path, and whether the adapter is powered by the ESP32-P4 board or separately.

The Phase 0 pass condition is a physically connected, safely powered, reproducibly documented bench stack. It does not claim that CSI data is yet captured; a missing compatible interconnect, unknown orientation, or unsafe power arrangement is a blocker rather than a firmware defect.

## Phase 1: Board-Health Baseline

Before bridge or capture work, prove that the selected ESP32-P4 board is a dependable test platform. This phase implements only the diagnostics-first foundation described in the firmware skeleton and diagnostics endpoint contract.

Phase 1 pass criteria:

- a documented flash and reflash procedure works on the selected board;
- serial output contains the documented diagnostics boot and health events;
- PSRAM presence, size, and a basic allocation or integrity check are reported through the `psram` diagnostics object;
- Ethernet obtains a DHCP address and reports link and IPv4 state through the `ethernet` diagnostics object; and
- `GET /diag` returns the schema-version-1 shape, with bridge, capture, encoder, streaming, HID, control, and authentication subsystems still reported as unavailable.

No HDMI bridge configuration, CSI ingest, encoding, or HID behavior belongs in this phase.

## Phase 2: HDMI Bridge Diagnostics

Implement `board_config` and `bridge_tc358743` only far enough to prove the control plane and source negotiation. CSI capture, JPEG encoding, and HTTP streaming remain out of scope.

Phase 2 must produce serial and `/diag` evidence for:

- the selected board and bridge-adapter identity, including the verified I2C pins and bridge chip identity;
- bridge reset behavior, when a controllable reset is available;
- EDID loading and hot-plug-detect assertion;
- HDMI signal detection, negotiated width, height, and frame rate; and
- PLL lock and CSI-output-enabled state, if exposed by the bridge.

The pass condition is repeatable signal lock from the canonical host in both BIOS/UEFI and desktop modes. A locked signal is not evidence of captured pixels; do not start encoder or streaming work until the following CSI-ingest phase passes.

## Phase 3: Raw CSI Frame-Ingest Proof

Implement the `capture` subsystem without JPEG encoding, frame publication, or browser delivery. The purpose is to prove that the ESP32-P4 receives changing pixels from the bridge under the configuration established by Phase 2.

Phase 3 diagnostics must report the active dimensions, pixel format, frame sequence, recent frame rate, completed-frame count, drop counters, and the last capture error. Include a lightweight frame fingerprint, checksum, or diagnostic sample that changes when the canonical host screen visibly changes.

The pass condition is repeated receipt of changing frames from both the BIOS/UEFI and desktop screens of the canonical host, with no unreconciled capture errors or steadily rising drop count. This phase establishes pixel ingest only; it does not establish image quality, JPEG validity, or browser access.

## Phase 4A: Authenticated HTTP Foundation

Implement the local single-administrator boundary defined in ADR 0006 before exposing any video route. The first-run credential must be provisioned only through the documented physical serial-console procedure.

Phase 4A passes when an operator can provision an administrator over serial, authenticate from the isolated bench network, retain a valid session, and observe anonymous rejection for every non-diagnostic route. The bootstrap `GET /diag` exception remains read-only and bench-only; it must not provide credential setup or access to snapshots, streams, HID, or power actions.

## Phase 4B: Authenticated Still-JPEG Proof

Add only an authenticated `GET /snapshot.jpg` route. It encodes and serves one current captured frame at a time; it is not an MJPEG stream and does not establish multi-client frame ownership behavior.

The pass condition is a valid, legible JPEG retrieved through an authenticated session from the canonical host's BIOS/UEFI and desktop screens. Record image dimensions, JPEG size, encode time, encoder failures, and relevant PSRAM allocation data in diagnostics. Verify that an unauthenticated snapshot request is rejected.

## Phase 5A: Single-Viewer MJPEG Proof

Add an authenticated, read-only MJPEG stream for one browser viewer. This phase proves continuous delivery only; it does not claim resilience to multiple, slow, or disconnected clients.

The pass condition is a stable, legible stream from the canonical host in BIOS/UEFI and desktop modes, with stream-client count, frame generation, send failures, and capture/encode health visible in diagnostics. Capture, encoding, and streaming must remain read-only: HID and power routes remain absent.

## Phase 5B: Latest-Frame Backpressure And Recovery Proof

Exercise the latest-frame streaming policy under deliberately slow, stalled, and disconnected authenticated viewers. Capture and encoding must never wait indefinitely for a stream client or accumulate an unbounded per-client frame queue.

The pass condition is that the healthy viewer continues receiving current frames while slow viewers skip frames or disconnect, with diagnostics showing skipped generations, slow-consumer disconnects, publication pressure, and recovery outcomes. After each client disconnect, verify that stream resources return to a healthy baseline without a firmware restart.

## Phase 6: Supervised Keyboard HID Safety Proof

Introduce control only after read-only streaming is healthy. Start with keyboard HID only, exercised on a physically supervised canonical host; mouse, paste, hotkeys, power control, and full UI polish are explicitly out of scope.

The phase must prove USB keyboard enumeration, an authenticated and single-owner input path, basic intentional key events in BIOS/UEFI and the desktop, and release of every pressed key on disconnect, ownership loss, session expiry, timeout, protocol error, and an explicit emergency-release action. Serial and `/diag` evidence must include active owner state, forced-release count, release reason, and any stuck-key indication.

The pass condition is a repeatable input-and-release test that leaves no key held after every induced failure condition. Do not add pointer or power control until this safety gate passes.

## Phase 7: Supervised Relative-Mouse Safety Proof

Add only relative mouse movement and button events through the same authenticated, single-owner control boundary. This phase does not add absolute/tablet mode, paste, hotkeys, or power control.

The pass condition is useful pointer behavior in both BIOS/UEFI and desktop contexts, with bounded movement handling and verified release of every pressed button on disconnect, ownership loss, session expiry, timeout, protocol error, and emergency release. Diagnostics must show owner state, forced-release count, release reason, and stuck-button indication.

## Phase 8: End-To-End Soak And Recovery Gate

Run authenticated read-only streaming plus keyboard and relative-mouse control against the supervised canonical host for extended sessions before adding further capabilities.

The test procedure must deliberately induce HDMI signal loss and restoration, Ethernet loss and restoration, viewer disconnects, authentication/session loss, and device reboot. For each fault, record diagnostics and the observed recovery outcome. The pass condition is restored healthy operation without an unsafe retained keyboard or mouse state, resource leakage, or a firmware restart beyond the deliberately induced reboot.

## Deferred Until The Core Gate Passes

ATX power/reset, paste text, and keyboard hotkeys are intentionally outside this bench sequence. They may be planned only after Phase 8 passes, because they add high-consequence machine actuation or additional keyboard-state complexity to a path that must first demonstrate safe video, authenticated keyboard, and relative-mouse recovery.

## Implementation Boundary

This contract is for the first external-adapter bring-up path only.

Out of scope for this issue:

- Integrated-board decisions.
- Final custom PCB architecture.
- Alternate bridge chips.
- Wi-Fi assumptions.
- Production enclosure, cable, or mechanical decisions.
- Broad mode-support policy beyond the initial EDID and resolution posture above.

Integrated-board planning remains a later hardware effort and should not be silently implied by this document.

## Consequences For Firmware Work

Before hardware-specific firmware implementation starts, the repo should have one explicit board configuration source that captures the fill-ins above. Firmware code should consume that configuration rather than scattering guessed pin numbers, lane assumptions, or power behavior across drivers.

The TC358743 bring-up sequence, EDID handling, Ethernet setup, and USB device assumptions should be implemented only after the selected board stack is documented against this contract.
