# ADR 0003: Firmware Subsystems And Diagnostics First

Status: Accepted

Date: 2026-07-09

## Context

The p4kvm reference is useful, but it relies heavily on file-static globals and broad cross-module shared state. WatchPup needs boundaries that make later hardware, streaming, control, and diagnostics work independently reviewable.

KVM firmware also becomes difficult to debug once video and HID are intertwined. Diagnostics must exist before the riskiest behavior is added.

## Decision

Use explicit first-firmware subsystem boundaries:

- `board_config`
- `bridge_tc358743`
- `capture`
- `encoder_mjpeg`
- `frame_store`
- `stream_server`
- `hid_device`
- `control_protocol`
- `diagnostics`
- `app_main`
- `web_ui`

Build diagnostics before read-only streaming. The first diagnostics surface should include serial logs and a local `/diag` JSON endpoint.

Diagnostics should grow to expose firmware version, board configuration, PSRAM, Ethernet, TC358743/HDMI/EDID state, CSI state, JPEG state, stream state, recovery counters, HID/control state, and auth/security state as those subsystems come online.

## Consequences

`app_main` remains the ESP-IDF composition root, but subsystem state should not leak into unrelated modules through unchecked globals.

Early implementation tickets should prefer diagnostics and explicit contracts before visible video or input control.

If a subsystem cannot expose basic health state, it is not ready to become a hidden dependency for later milestones.

