# ADR 0002: Initial Firmware Platform And Hardware Path

Status: Accepted

Date: 2026-07-09

## Context

The p4kvm reference proves a useful primitive: ESP32-P4 plus TC358743 HDMI-to-MIPI CSI capture, JPEG/MJPEG streaming, Ethernet, browser control, and TinyUSB HID. The reference is explicitly proof-of-concept software, so WatchPup needs to reuse the hardware knowledge without inheriting its architecture.

ESP32-P4 has the right blocks for this experiment: MIPI-CSI, USB OTG 2.0 HS, Ethernet, hardware JPEG, H.264 capability, PSRAM support, and security primitives. The risk is end-to-end integration, not the block diagram.

## Decision

The first WatchPup firmware milestones will target the reference-shaped path:

- ESP32-P4 Ethernet dev-board class hardware.
- External TC358743 HDMI-to-MIPI CSI adapter for early bring-up.
- Wired Ethernet.
- PSRAM required.
- MJPEG first.
- Fixed 1080p30 EDID initially, treated as replaceable later.
- 720p reliability/default target.
- Board-specific pins, reset behavior, MIPI lane rate, and configuration verified before hardware-dependent implementation.

The first ESP-IDF skeleton should live under `firmware/`, target ESP32-P4, and start from ESP-IDF `v6.0.2` unless hardware bring-up requires reference-compatible `v6.0.1`.

## Consequences

The first firmware skeleton should not include HDMI capture, JPEG streaming, HID control, ATX control, Wi-Fi, H.264, virtual media, secure boot defaults, or flash encryption defaults.

Hardware-specific code should be isolated behind subsystem boundaries instead of copied from p4kvm. The TC358743 bring-up sequence should be rewritten from documented behavior and local research notes, not pasted wholesale.

An integrated board remains a later hardware/PCB planning effort.

