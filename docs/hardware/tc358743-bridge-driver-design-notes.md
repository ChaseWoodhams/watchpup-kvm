# TC358743 Bridge Driver Design Notes

Issue: [#24 Write TC358743 bridge driver design notes](https://github.com/ChaseWoodhams/watchpup-kvm/issues/24)

Date: 2026-07-09

Related decisions and research:

- [ADR 0002: Initial Firmware Platform And Hardware Path](../adrs/0002-initial-firmware-platform-and-hardware-path.md)
- [ADR 0003: Firmware Subsystems And Diagnostics First](../adrs/0003-firmware-subsystems-and-diagnostics-first.md)
- [Initial Hardware Configuration Contract](initial-hardware-configuration-contract.md)
- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)

## Purpose

This document defines the intended behavioral boundary for WatchPup's `bridge_tc358743` subsystem before implementation starts.

It exists to preserve useful bring-up knowledge from the p4kvm reference analysis without copying or vendoring p4kvm source. WatchPup should reuse the hardware lessons as driver behavior, timing expectations, diagnostics, and recovery concerns, then implement its own driver shape inside the subsystem boundaries accepted by [ADR 0003](../adrs/0003-firmware-subsystems-and-diagnostics-first.md).

## Boundary

`bridge_tc358743` should own:

- TC358743 power-up and reset sequencing once board-specific pins and polarity are known.
- I2C register reads and writes for bridge configuration and status.
- EDID loading for the initial fixed bring-up EDID selected under [ADR 0002](../adrs/0002-initial-firmware-platform-and-hardware-path.md).
- HDMI hot-plug detect control as part of source negotiation and recovery.
- Bridge-local PLL, CSI, video-path, and stream-enable behavior needed to present video to the ESP32-P4 capture path.
- Bridge-local status collection and fault reporting for diagnostics.
- A small recovery surface for bridge reinit and source renegotiation.

`bridge_tc358743` should not own:

- Board pin discovery, hardcoded GPIO choices, or adapter SKU assumptions. Those belong in `board_config`.
- CSI frame-buffer ownership, capture callbacks, or ISP policy. Those belong in `capture`.
- JPEG encode, MJPEG streaming, HTTP routing, HID control, or global application orchestration.
- Hidden cross-module globals or direct mutation of unrelated subsystem state.

## Expected Initialization Sequence

The first implementation should preserve the proven behavioral ordering from the reference analysis while rewriting it behind WatchPup's own API and diagnostics model.

Expected high-level sequence:

1. Hold or drive HPD low so the HDMI source does not negotiate against a partially configured bridge.
2. Apply basic bridge setup required for I2C access, reset completion, clock assumptions, and any required default video-path state.
3. Load the initial EDID image and verify that the write completed cleanly.
4. Configure PLL and CSI output behavior for the selected board wiring, lane count, and lane rate.
5. Enable the bridge video path and stream output only after EDID and CSI-side assumptions are in place.
6. Raise HPD high so the source can renegotiate against the intended EDID and bridge state.
7. Perform status checks that confirm cable detect, HDMI signal presence, negotiated mode, and bridge stream readiness before capture depends on the result.

The implementation ticket may split this into explicit phases such as `init`, `load_edid`, `arm_stream`, `set_hpd`, and `poll_status`, but the behavioral order above should remain intact unless board-level testing proves a different sequence is required.

## Diagnostics Surface

Following [ADR 0003](../adrs/0003-firmware-subsystems-and-diagnostics-first.md), `bridge_tc358743` should expose enough state for serial logs and `/diag` JSON without requiring a debugger.

The first diagnostics surface should aim to report:

- Bridge probe result and chip identity if readable.
- Current driver lifecycle state, such as reset, configured, streaming, degraded, or recovering.
- HPD state being driven by firmware.
- Whether an EDID image is loaded, which image revision is active, and whether the last load or verification failed.
- HDMI cable/signal detect state.
- Negotiated input status when readable, such as resolution, frame rate family, or color-space hints.
- PLL lock or equivalent clock-readiness status when readable.
- CSI output configuration currently selected for the bridge.
- Last bridge error code, last failing step, and a monotonically increasing recovery counter.
- Timestamps or counters for the last successful init and the last successful signal lock.

Diagnostics should be stable enough to answer basic bring-up questions:

- Did the bridge reset and probe?
- Did EDID load?
- Did the source see HPD and renegotiate?
- Did the bridge lock its internal path?
- Is the bridge output likely ready for CSI capture?

## Recovery Concerns

The first implementation should plan for recovery explicitly instead of treating failures as one-shot initialization errors.

Known recovery concerns:

- Some HDMI sources may sleep or stop driving video until HPD is toggled again.
- Some hosts are EDID-sensitive and may cache stale capabilities if HPD is raised before EDID load is complete.
- Cable unplug or flaky adapters may require bridge-local reinit before capture can resume cleanly.
- Bridge and capture state can drift apart if the source renegotiates modes unexpectedly or disappears mid-stream.
- Repeated hard resets can hide root-cause diagnostics unless the driver preserves failure counters and the last known state.

The subsystem should therefore support at least:

- A controlled HPD toggle path for renegotiation.
- A controlled bridge reinit path that can rerun the setup sequence without rebooting the whole firmware.
- Clear reporting of why recovery ran and whether it succeeded.
- Guardrails so recovery does not thrash continuously when the cable or source is unstable.

## Reusable Lessons From p4kvm

The p4kvm reference is useful as behavior and lab knowledge, not as source material to transplant.

Reusable as behavior:

- The broad bring-up order: HPD low, setup, EDID load, PLL/CSI setup, stream enable, HPD high, then status checks.
- The need to keep EDID handling and HPD timing deliberate for host compatibility.
- The need for bridge-local debug reads and health probes during bring-up.
- The expectation that HDMI recovery and renegotiation are real runtime concerns, not edge cases.
- The need to treat CSI lane setup, clocking, and bridge status as hardware-contract inputs rather than guessed constants.

Must not be copied:

- p4kvm TC358743 source files, register tables, helper layout, or file structure.
- p4kvm's file-static global state patterns or hidden coupling between capture and bridge behavior.
- p4kvm naming, API shape, or monolithic module boundaries if they conflict with WatchPup's subsystem decisions.
- Generated, vendored, or pasted source fragments presented as "reference" implementation.

WatchPup's implementation should be a clean-room rewrite from documented behavior, board-specific verification, public datasheet guidance if available, and local bring-up observations.

## Open Bring-Up Questions

These questions should be answered in the later hardware implementation ticket before the first driver is treated as stable:

- Which exact TC358743 adapter board and connector stack is the initial target?
- Is bridge reset under firmware control, and what polarity and delay budget does that path require?
- What CSI lane count and lane mapping are actually wired on the selected board and adapter pair?
- What MIPI lane rate is stable on the chosen hardware path?
- Which fixed EDID image should be loaded first, and does the initial 1080p30 bring-up EDID still align with the 720p reliability posture from [ADR 0002](../adrs/0002-initial-firmware-platform-and-hardware-path.md)?
- Which status bits or registers are reliable enough to surface in `/diag` for this board stack?
- What recovery action works best for sleeping sources: HPD toggle only, bridge reinit, or coordinated capture restart?
- How long should the driver wait between HPD changes, EDID load, and stream-enable transitions on the selected hardware?
- Which failure conditions should be considered retryable versus terminal during early bring-up?

## Implementation Posture

This document is intentionally not a register script and not a code skeleton.

The next implementation ticket should use it to define a small `bridge_tc358743` API, map board-owned values in `board_config`, and attach diagnostics before wiring bridge success into the capture pipeline. If later testing shows the sequence needs board-specific adjustments, update this document first so the behavioral contract stays ahead of the code.
