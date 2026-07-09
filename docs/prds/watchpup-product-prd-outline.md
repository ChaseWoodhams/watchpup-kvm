# WatchPup KVM Product PRD Outline

Issue: [Prototype the product PRD outline](https://github.com/ChaseWoodhams/watchpup-kvm/issues/20)

Map: [Wayfinder: transparent microcontroller KVM product strategy](https://github.com/ChaseWoodhams/watchpup-kvm/issues/12)

Status: Lightweight outline for human review. This is not the final PRD.

## 1. Product Summary

WatchPup KVM is an ultra-cheap, transparent, open-source, microcontroller-class KVM-over-IP node for homelabs, mini PCs, labs, and agent-assisted machine recovery.

The product wedge is not merely "cheap KVM." The wedge is a cheap-enough KVM node that is boringly inspectable: firmware, web UI, hardware files, BOM, sourcing notes, enclosure files, build and flashing docs, release/update docs, security docs, public issue tracking, and third-party inventory are public as each artifact exists.

## 2. Problem

Homelab and small-lab users often want BIOS-level access, crash recovery, OS install help, and remote power control across several small machines. Mature open solutions such as PiKVM are excellent but too expensive to put on every computer. Cheaper Linux-SoC KVMs exist, but they can create trust concerns around opaque stacks, surprising hardware capabilities, update behavior, network behavior, and security defaults.

WatchPup should make the cheap-KVM trade-off explicit: fewer mature features at first, but a smaller transparent system that a technical user can inspect, build, flash, debug, and improve.

## 3. Target Users

Primary v1 audience:

- Homelab users with multiple mini PCs or servers.
- Small-lab and maker users who manage Linux boxes, render nodes, CNC/CAD machines, 3D printer hosts, or test machines.
- Users who want a KVM node cheap enough to attach to every important machine rather than one premium unit they move around.

Secondary audience:

- AI-agent users who want hardware-level recovery and OS install automation.
- Open-hardware and embedded developers who want a transparent ESP32-P4 KVM platform to hack on.

## 4. Competitive Gap

WatchPup should position itself as:

- Cheaper and more fleet-deployable than official PiKVM units.
- More transparent and hackable than cheap Linux-SoC appliances.
- More network-capable than USB crash-cart KVMs.
- More understandable than a general-purpose Linux appliance.

WatchPup should not claim to beat every cheap KVM on raw price, video specs, cloud access, or mature features in v1.

Research inputs:

- [WatchPup KVM Market Gap Research](../research/watchpup-market-gap.md)
- [ESP32-P4 Video And Security Realism](../research/esp32-p4-realism.md)
- [WatchPup KVM DIY Cost And BOM Research](../research/watchpup-diy-cost-bom.md)

## 5. V1 Scope

V1 should prove the trustworthy microcontroller KVM path.

In scope:

- Ethernet-first networking.
- MJPEG-first video.
- 720p as the reliability/default target.
- 1080p detail mode only if stable on real hardware.
- USB keyboard and mouse.
- Relative BIOS-compatible pointer behavior first.
- Paste text and core hotkeys, including Ctrl+Alt+Del.
- LAN-only local HTTP plus mandatory authentication.
- Single active controller for keyboard and mouse input.
- Local API for screenshot, keypress, type text, mouse, click, and power actions.
- Diagnostics/health visibility before polish.
- ATX power/reset tap.
- Full-suite openness deliverables for artifacts that exist at v1.

Post-v1:

- H.264 streaming.
- Wi-Fi.
- Virtual media.
- Cloud access.
- HTTPS-by-default and stronger production hardening.
- Polished integrated enclosure or finished kit.
- Broader keyboard layout support.
- Multi-user control beyond a single active controller policy.
- Absolute tablet pointer mode.

## 6. Security And Trust Posture

V1 security posture is transparent LAN-first security:

- Local HTTP plus mandatory auth is acceptable for v1/prototype work.
- The device must be documented as LAN-only and unsafe to expose directly to the internet.
- No default credentials.
- No telemetry.
- No cloud dependency.
- No microphone/audio hardware or surprise sensors.
- Remote access guidance should prefer user-owned VPN or equivalent private access.
- HTTPS, update signing, secure boot, and flash encryption are production-hardening goals, not v1 blockers.
- Unsafe lab modes must be explicitly named and documented as unsafe.

## 7. Openness Promise

WatchPup should use the strong/full-suite transparency promise as the goal, even before dedicated hardware exists.

Public artifacts should include:

- Firmware source.
- Web UI source.
- Hardware design files once WatchPup hardware exists.
- BOM and sourcing notes once WatchPup hardware exists.
- Enclosure files once an enclosure exists.
- Build and flashing instructions.
- Release artifacts and update-path documentation.
- Security posture and threat-model documentation.
- Public issue tracking and planning by default.
- Third-party and closed-component inventory.

The product should avoid hidden cloud dependencies, hidden telemetry, surprise sensors, and undocumented factory/update paths.

## 8. Cost Target

V1 cost language should be disciplined:

- DIY prototype/dev-kit target: around `$50-$80` before shipping, tax, tools, spares, enclosure polish, assembly labor, compliance, and support.
- Product goal: cheap enough that a homelab user can imagine putting one on every important computer or mini PC.
- Integrated-board cost target: promising but unproven until a later hardware/BOM plan validates schematic, PCB, assembly, test, enclosure, cable, and sourcing assumptions.

Avoid claiming that a finished supported kit can definitely sell for `$50-$80`.

## 9. Technical Direction

V1 should follow the ESP32-P4 / TC358743 path as a hardware-proving route:

- ESP32-P4 for Ethernet, USB HID, MIPI-CSI, JPEG/MJPEG, and security primitives.
- TC358743 HDMI-to-MIPI CSI bridge as the first capture bridge.
- MJPEG before H.264.
- Diagnostics before video polish.
- Read-only MJPEG milestone before HID control if sequencing needs to reduce risk.
- Bounded latest-frame MJPEG publication, where slow clients skip frames or disconnect rather than forcing capture/encode to block.
- Editable browser UI source under `web/`, with generated firmware assets isolated from authored source.
- Conservative HID/control boundary: auth required, one active controller, documented protocol validation, and stuck-input release behavior.

The p4kvm reference should be treated as a lab notebook and driver spike, not a codebase seed.

Related research:

- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)
- [ESP-IDF Skeleton And Dependency Baseline](../research/esp-idf-skeleton-baseline.md)

Related ADRs:

- [ADR 0001: Product Vision And V1 Boundary](../adrs/0001-product-vision-and-v1-boundary.md)
- [ADR 0002: Initial Firmware Platform And Hardware Path](../adrs/0002-initial-firmware-platform-and-hardware-path.md)
- [ADR 0003: Firmware Subsystems And Diagnostics First](../adrs/0003-firmware-subsystems-and-diagnostics-first.md)
- [ADR 0004: Frame Ownership And MJPEG Backpressure](../adrs/0004-frame-ownership-and-mjpeg-backpressure.md)
- [ADR 0005: Web Assets And HID Control Safety](../adrs/0005-web-assets-and-hid-control-safety.md)

## 10. Initial Milestones

Suggested roadmap for implementation planning:

1. Write the full PRD from this outline.
2. Create implementation tickets from the PRD.
3. Keep the existing firmware migration Wayfinder map as the technical migration layer.
4. Create a separate hardware/PCB Wayfinder map for integrated-board decisions.
5. Create an AI-agent/API experience prototype map only after the v1 feature boundary is stable enough to support it.

Suggested first implementation milestone groups:

- ESP-IDF skeleton and build baseline.
- Hardware target and capture-mode ADR.
- TC358743 driver design notes without wholesale source copying.
- Diagnostics endpoint or serial diagnostic plan.
- Capture pipeline prototype.
- JPEG encode prototype.
- Read-only MJPEG stream.
- HID control protocol and safety boundary.
- TinyUSB HID subsystem.
- ATX power/reset subsystem.
- Local API and web UI skeleton.

## 11. Open Questions For Full PRD

These are intentionally not resolved in this outline:

- Exact hardware board/module choice for the first reproducible DIY build.
- Exact HDMI-to-CSI adapter or integrated TC358743 circuit.
- Exact authentication mechanism for first v1 prototype.
- Exact local API shape and command authorization model.
- Exact diagnostic field names and JSON schema for first firmware milestone.
- Whether the first PRD should split hardware, firmware, web UI, and API into separate specs.
