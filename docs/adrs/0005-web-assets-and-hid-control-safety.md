# ADR 0005: Web Assets And HID Control Safety

Status: Accepted

Date: 2026-07-09

## Context

The p4kvm reference builds a Vite web UI into a single HTML file and copies built output into firmware source paths. That is convenient for a proof of concept but blurs authored source and generated artifacts.

Browser-accessible HID is also one of the highest-risk parts of an IP KVM. A stuck key, unauthenticated session, or ambiguous controller ownership can create real machine-control problems.

## Decision

Keep `web/` as the editable browser UI source. Build it with Vite or an equivalent small frontend build step. Generated firmware assets must live in an isolated generated/build path and be embedded by firmware CMake with ESP-IDF embedding support.

Generated assets are build output by default. If release packaging later needs checked-in generated assets for reproducibility, that should be a separate release decision.

Use a conservative v1 HID/control boundary:

- Viewing and control are separate capabilities.
- Multiple clients may view, but only one authenticated client can actively control HID at a time.
- HID/control requires authentication before it is enabled, even for LAN-first development.
- Browser input commands pass through a documented `control_protocol` boundary before `hid_device`.
- The protocol must include explicit ownership/session semantics.
- On disconnect, timeout, ownership loss, auth loss, protocol error, or emergency release, WatchPup releases all pressed keys/buttons and zeros pending mouse movement.
- Start with US keyboard mapping, explicit hotkeys, and text paste as queued key events with rate limiting/backpressure.
- Start with relative BIOS-compatible mouse mode. Absolute/tablet mode is later work.
- ATX/power actions use the same authenticated control boundary and should remain separately authorized or confirmed from ordinary keyboard/mouse input.

## Consequences

The first UI can be a diagnostics/status page plus `/diag` JSON before growing into a full MJPEG control UI.

Firmware should not treat copied/minified HTML as human-authored source.

HID implementation tickets must include ownership, auth assumptions, input validation, stuck-input recovery, and diagnostics from the start.

