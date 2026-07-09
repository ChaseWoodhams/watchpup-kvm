# ADR 0001: Product Vision And V1 Boundary

Status: Accepted

Date: 2026-07-09

## Context

WatchPup KVM is intended to occupy a narrow but useful gap: cheaper and more fleet-deployable than official PiKVM units, more transparent and hackable than low-cost Linux-SoC KVM appliances, and more network-capable than USB crash-cart tools.

The market already contains capable sub-$100-ish KVM products, so WatchPup should not depend on being the only cheap option. The more durable opportunity is a cheap-enough KVM node whose firmware, web UI, hardware files, BOM, sourcing notes, enclosure files, build process, update path, security model, and issue history are public as those artifacts exist.

## Decision

WatchPup KVM will optimize v1 around a transparent, microcontroller-class KVM path for homelabs, mini PCs, labs, makers, and agent-assisted recovery.

V1 will target:

- Ethernet-first networking.
- MJPEG-first video.
- 720p as the reliability/default target.
- 1080p only as a detail/validation mode if stable on real hardware.
- USB keyboard and mouse.
- Relative BIOS-compatible pointer behavior first.
- Paste text and core hotkeys, including Ctrl+Alt+Del.
- LAN-only local HTTP plus mandatory authentication.
- Local API direction for screenshot, keypress, type text, mouse, click, and power actions.
- Diagnostics and health visibility before polish.
- ATX power/reset as a controlled feature, not a first firmware skeleton behavior.
- Full-suite openness for artifacts that exist at each stage.

Post-v1 work includes H.264 streaming, Wi-Fi, virtual media, cloud access, HTTPS-by-default production hardening, broader keyboard layout support, polished kit packaging, and multi-controller input.

## Consequences

WatchPup should avoid making v1 promises around 4K, H.264, virtual media, cloud access, or finished commercial kit pricing.

Implementation tickets should be small and ordered around proving the transparent basics first: firmware skeleton, diagnostics, hardware/capture design notes, MJPEG path, HID safety, ATX control, and local API.

Product language should say the early DIY/dev-kit target is around `$50-$80` before shipping, tax, tools, spares, enclosure polish, assembly labor, compliance, and support. It should not claim a finished supported kit can definitely sell for that range.

