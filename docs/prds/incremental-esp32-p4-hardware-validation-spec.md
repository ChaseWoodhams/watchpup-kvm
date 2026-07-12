# Incremental ESP32-P4 Hardware Validation Spec

## Problem Statement

WatchPup needs to move from documentation and a firmware skeleton to a trustworthy, incrementally testable hardware path on the selected ESP32-P4-ETH and Waveshare HDMI-to-CSI bench stack. Video, network, authentication, and HID failures must be isolated at their natural boundaries so that a failure produces evidence and a bounded rework loop rather than an opaque end-to-end debugging exercise.

## Solution

Deliver the first hardware-proving KVM path as ordered validation-gated slices. Every slice produces a scoped change, a repeatable bench procedure using one canonical host in BIOS/UEFI and desktop states, serial and `/diag` evidence where applicable, and an explicit pass/fail result. A failed gate is diagnosed, revised, and retested before dependent work starts.

## Validation Slices

1. **Physical-interface qualification:** document CSI connectors, FPC/cable orientation, lane information, and safe power before capture-specific firmware.
2. **Board health:** prove flash/reflash, serial diagnostics, PSRAM, Ethernet DHCP, and the schema-versioned `/diag` baseline.
3. **HDMI bridge diagnostics:** prove I2C identity, reset where available, EDID, HPD, source negotiation, and lock state.
4. **Raw CSI ingest:** prove changing pixels with capture dimensions, format, counters, errors, and a frame fingerprint or diagnostic sample.
5. **Authenticated HTTP and still JPEG:** first prove serial-only administrator provisioning, authenticated sessions, and anonymous rejection; then prove an authenticated current-frame JPEG.
6. **MJPEG streaming:** first prove one authenticated viewer; then induce slow/stalled/disconnected viewers and verify latest-frame backpressure and recovery.
7. **HID safety:** prove keyboard enumeration, ownership, and forced key release; then separately prove relative-mouse movement, button release, and BIOS/UEFI usability.
8. **End-to-end soak and recovery:** run video plus HID while inducing HDMI loss, Ethernet loss, viewer/session loss, and reboot. Verify recovery and absence of retained input or resource leakage.

## Implementation Decisions

- Use the selected Waveshare ESP32-P4-ETH and Waveshare HDMI-to-CSI bench stack. This is a validation target, not a production-board decision.
- Use serial diagnostics and `/diag` as the shared evidence seam across the existing firmware subsystems.
- Allow unauthenticated, read-only `/diag` only as an explicitly identified isolated-bench bootstrap; it exposes no setup, video, HID, power, or control behavior.
- Use a locally provisioned single administrator: no default credential, physical serial-console provisioning, suitable password hashing, secure random sessions, and authentication for all non-diagnostic routes.
- Validate one still JPEG before MJPEG, and validate single-viewer delivery before slow-client backpressure.
- Keep keyboard and relative mouse as separate safety gates with single active controller and forced-release handling.

## Testing Decisions

- Test externally meaningful behavior rather than implementation detail.
- Each slice is complete only with a scoped change, repeatable bench procedure, saved evidence, and explicit pass/fail result.
- On failure, run a bounded diagnose → revise → retest loop before beginning dependent work.
- Use the canonical host at both BIOS/UEFI and desktop for every video and HID gate; other hosts/modes are compatibility work.

## Out of Scope

Until the core soak gate passes, defer ATX power/reset, paste, hotkeys, absolute pointer mode, multi-controller input, Wi-Fi, cloud access, virtual media, H.264, production hardening, and a polished browser UI.

## Further Notes

This local planning specification is the source for creating small, independently actionable GitHub implementation issues. Only those implementation issues should receive the `ready-for-agent` label.
