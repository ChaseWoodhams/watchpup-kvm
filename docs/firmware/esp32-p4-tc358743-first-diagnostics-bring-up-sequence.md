# First Diagnostics Bring-Up Sequence For ESP32-P4 And TC358743

Issue: [#38 Test advisor plan checkpoint on firmware-risk documentation slice](https://github.com/ChaseWoodhams/watchpup-kvm/issues/38)

Date: 2026-07-09

Related decisions and docs:

- [ADR 0002: Initial Firmware Platform And Hardware Path](../adrs/0002-initial-firmware-platform-and-hardware-path.md)
- [ADR 0003: Firmware Subsystems And Diagnostics First](../adrs/0003-firmware-subsystems-and-diagnostics-first.md)
- [Initial Hardware Configuration Contract](../hardware/initial-hardware-configuration-contract.md)
- [TC358743 Bridge Driver Design Notes](../hardware/tc358743-bridge-driver-design-notes.md)
- [Diagnostics Endpoint Contract](diagnostics-endpoint-contract.md)
- [WatchPup KVM Context](../../CONTEXT.md)

## Agent Implementation Plan

- Issue number and title: `#38 Test advisor plan checkpoint on firmware-risk documentation slice`
- Relevant docs read:
  - `AGENTS.md`
  - `CONTEXT.md`
  - `docs/agents/domain.md`
  - `docs/prds/watchpup-product-prd-outline.md`
  - `docs/adrs/0002-initial-firmware-platform-and-hardware-path.md`
  - `docs/adrs/0003-firmware-subsystems-and-diagnostics-first.md`
  - `docs/hardware/initial-hardware-configuration-contract.md`
  - `docs/hardware/tc358743-bridge-driver-design-notes.md`
  - `docs/firmware/diagnostics-endpoint-contract.md`
- Proposed approach: add one docs-only note that captures the intended first diagnostics bring-up order for the ESP32-P4 plus TC358743 path, using existing V1 and diagnostics-first decisions instead of introducing new firmware scope.
- Files expected to change:
  - `docs/firmware/esp32-p4-tc358743-first-diagnostics-bring-up-sequence.md`
- Non-goals:
  - No firmware, build-system, networking, HID, or auth implementation changes
  - No change to the accepted V1 hardware path or subsystem boundaries
  - No attempt to define register tables, exact delays, or board pin values before the hardware contract fill-ins exist
- Risks and assumptions:
  - Bring-up sequencing is hardware-risk work, so this note should preserve diagnostics-first ordering and avoid implying that capture or HID can proceed before bridge and `/diag` visibility are proven
  - Exact board SKU, adapter SKU, lane mapping, reset polarity, and timing budgets remain open until documented against the hardware contract
- Expected checks:
  - `npm run typecheck`
  - `npm run build`
- Hardware/testing expectations:
  - No physical hardware is required for this issue because it is docs-only
  - Later implementation should verify each step on the selected ESP32-P4 plus TC358743 board stack with serial logs and `/diag`
  - Read-only diagnostics should be proven before capture, MJPEG polish, or HID control are treated as stable milestones

## Purpose

This note captures the intended first diagnostics bring-up sequence for WatchPup's ESP32-P4 plus TC358743 path before firmware implementation starts.

It is a sequencing document, not a code ticket. The goal is to keep the first hardware-facing firmware work aligned with the repo's diagnostics-first posture and to reduce the chance of wiring capture or HID behavior before the bridge path can report stable health.

## Sequence

The intended first bring-up sequence is:

1. Confirm the selected board stack matches the documented hardware contract before adding hardware-dependent logic.
2. Boot into a minimal diagnostics-capable firmware skeleton that can emit serial logs, identify the firmware build, and expose a stable `/diag` endpoint shape.
3. Surface `board_config` facts first, including the chosen board identity and whether required fill-ins such as bridge reset control, CSI lane count, lane rate, Ethernet path, PSRAM, and USB device path are known.
4. Bring up PSRAM and Ethernet only far enough to support stable diagnostics reporting and local inspection; do not treat video or HID readiness as implied by those early successes.
5. Probe the TC358743 path in a diagnostics-first order: keep HPD low, complete bridge access and reset handling, load the initial EDID, configure the expected PLL and CSI assumptions, enable the bridge stream path, then raise HPD and poll status.
6. Publish bridge health through serial logs and `/diag`, including probe result, chip identity if readable, EDID load status, HPD state, HDMI signal presence, negotiated mode when readable, PLL or stream readiness, and recovery counters.
7. Treat bridge output as a bring-up checkpoint, not proof of full capture success; capture work should depend on the bridge diagnostics surface being stable enough to answer whether the source renegotiated and whether CSI-facing output looks ready.
8. Keep the first milestone read-only and diagnostics-led. If sequencing risk remains, postpone capture polish, MJPEG performance work, and HID control until the board, bridge, and `/diag` story are understandable without a debugger.

## Expected Diagnostics Checkpoints

The first diagnostics-oriented bring-up should be able to answer these questions clearly:

- Did the firmware boot with the expected ESP-IDF target and board assumptions?
- Is `/diag` alive and returning the agreed skeleton shape?
- Are PSRAM and Ethernet available enough for local diagnostics work?
- Did the bridge reset, probe, and accept the intended EDID?
- Did HPD timing allow the source to renegotiate against the intended configuration?
- Does the bridge report HDMI signal presence and CSI-facing readiness strongly enough for capture work to start?
- If something failed, does the firmware expose the failing step and whether recovery was attempted?

## Non-Goals For This Bring-Up Slice

This first diagnostics sequence does not attempt to prove:

- Stable MJPEG streaming throughput
- HID control ownership or stuck-key safety behavior
- Authentication behavior
- Production networking hardening
- Final EDID policy beyond the initial bring-up posture
- Integrated-board or custom-PCB decisions

Those remain separate milestones after the diagnostics and bridge-health story is trustworthy.

## Hardware Testing Expectations

When later implementation starts on real hardware, the sequence above should be validated on the selected ESP32-P4 dev-board class target plus the selected TC358743 adapter stack.

Expected evidence for that later work:

- Serial logs showing boot, subsystem init, bridge bring-up phases, and recovery attempts
- `/diag` output matching the documented contract and exposing bridge state transitions
- A reproducible note of the board SKU, adapter SKU, lane count, lane rate, reset behavior, and any timing adjustments required for stable HPD and EDID negotiation

If real hardware testing forces a different order than the one documented here, update this note and the bridge design notes before treating the code path as stable.
