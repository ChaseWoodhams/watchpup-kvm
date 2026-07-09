# ADR 0004: Frame Ownership And MJPEG Backpressure

Status: Accepted

Date: 2026-07-09

## Context

WatchPup's first video milestone should be read-only MJPEG. ESP32-P4 memory and bandwidth constraints make per-client frame queues risky, especially at 720p and 1080p. The p4kvm reference demonstrates useful frame publication ideas, but its global-state sharing model should not become the long-term WatchPup contract.

## Decision

Use a bounded latest-frame publication model for the first read-only video milestone.

- `capture` owns raw CSI frame buffers.
- `encoder_mjpeg` borrows one completed raw frame and encodes into a small fixed set of JPEG slots.
- `frame_store` owns published JPEG slots with generation numbers and reference accounting.
- `stream_server` clients borrow only the latest complete JPEG frame.
- Slow MJPEG clients skip frames instead of building per-client queues.
- Clients that cannot send within a timeout are disconnected.
- Capture and encode must not block indefinitely on MJPEG clients.
- First video milestone remains read-only; HID/control ownership is separate.

Diagnostics should expose frame generation, dropped/skipped frames, active clients, send failures, encode time, and slot pressure.

## Consequences

This favors bounded memory and simple concurrency over smooth playback for every slow client.

Per-client buffering may be revisited later only if measurements show it is needed and the RAM/backpressure model is redesigned deliberately.

