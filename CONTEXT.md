# WatchPup KVM Context

WatchPup KVM is a clean, issue-driven project. This file is the shared domain context for agents and humans working in the repo.

## Current State

The repo is still at its baseline stage. Product strategy and the first p4kvm-to-WatchPup migration decisions are documented, but the firmware, web UI, hardware files, and enclosure files have not been implemented yet.

Agent-assisted development is configured through `agent-loop-kit` and Sandcastle. Implementation should happen through small GitHub issues labeled `ready-for-agent`, isolated agent worktrees, and human-reviewed pull requests.

## Product Direction

WatchPup KVM is aimed at homelabs, mini PCs, small labs, makers, and agent-assisted machine recovery. The wedge is a cheap-enough KVM node that is transparent and inspectable enough to put on every important computer.

WatchPup should compete on trust, hackability, reproducibility, and fleet affordability rather than trying to beat Linux-SoC KVM products on every video or cloud feature in v1.

## V1 Boundary

V1 should prove the trustworthy microcontroller KVM path:

- Ethernet-first networking.
- ESP32-P4 plus TC358743 as the first hardware-proving path.
- MJPEG before H.264.
- 720p as the reliability/default target.
- 1080p as a detail/validation mode only if stable on real hardware.
- Diagnostics before streaming polish.
- Read-only MJPEG before HID control if sequencing needs to reduce risk.
- USB keyboard and mouse with relative BIOS-compatible pointer behavior first.
- LAN-only local HTTP plus mandatory authentication.
- Single active controller for HID input.
- ATX power/reset as a later controlled feature behind the same authenticated control boundary.

Post-v1 or later work includes H.264, Wi-Fi, virtual media, cloud access, HTTPS-by-default production hardening, polished kit packaging, broader keyboard layouts, and multi-controller input.

## Technical Decisions

- Use a dedicated `firmware/` ESP-IDF project targeting ESP32-P4. Start from current stable ESP-IDF `v6.0.2` unless hardware bring-up requires reference-compatible `v6.0.1`.
- Treat `p4kvm-reference` as a lab notebook and behavior reference, not a source-code seed.
- Keep firmware subsystem boundaries explicit: `board_config`, `bridge_tc358743`, `capture`, `encoder_mjpeg`, `frame_store`, `stream_server`, `hid_device`, `control_protocol`, `diagnostics`, `app_main`, and `web_ui`.
- Build diagnostics first with serial logs and a local `/diag` JSON endpoint.
- Use bounded latest-frame MJPEG publication. Slow clients skip frames or disconnect; capture and encode do not block indefinitely on stream clients.
- Keep editable browser UI source in `web/`; generated firmware assets stay isolated from authored source and are embedded by firmware CMake.
- HID input must pass through a documented `control_protocol` boundary and release all keys/buttons on disconnect, timeout, ownership loss, auth loss, protocol error, or emergency release.

## Documentation Map

- Product PRD outline: `docs/prds/watchpup-product-prd-outline.md`
- p4kvm reference analysis: `docs/research/p4kvm-reference-analysis.md`
- Market gap research: `docs/research/watchpup-market-gap.md`
- ESP32-P4 realism research: `docs/research/esp32-p4-realism.md`
- DIY cost/BOM research: `docs/research/watchpup-diy-cost-bom.md`
- ESP-IDF skeleton baseline: `docs/research/esp-idf-skeleton-baseline.md`
- Initial hardware configuration contract: `docs/hardware/initial-hardware-configuration-contract.md`
- Diagnostics endpoint contract: `docs/firmware/diagnostics-endpoint-contract.md`
- HID control protocol and safety spec: `docs/firmware/hid-control-protocol-and-safety-spec.md`
- Architecture decisions: `docs/adrs/`

## Vocabulary

- **WatchPup KVM**: The product and project in this repository: an ultra-cheap, transparent, open-source, microcontroller-class KVM-over-IP node.
- **Full-suite openness**: WatchPup KVM's transparency goal: firmware, web UI, hardware design files, BOM, sourcing notes, enclosure files, build and flashing docs, release/update docs, security docs, public issue tracking, and third-party component inventory should be public as each artifact exists.
- **Microcontroller-class KVM**: A KVM-over-IP design centered on MCU firmware and small, inspectable hardware rather than a general-purpose Linux appliance.
- **LAN-first security**: The v1 posture where WatchPup is intended for a trusted local network or user-owned VPN, requires authentication, has no default credentials, and must not be exposed directly to the internet.
- **Single active controller**: The input policy where multiple clients may view, but only one authenticated client owns keyboard/mouse control at a time.
- **Diagnostics-first**: The development sequence where serial logs and `/diag` health state exist before video streaming or HID control polish.
- **Latest-frame streaming**: The MJPEG model where stream clients borrow the newest complete JPEG frame and skip frames when slow instead of accumulating per-client frame queues.
- **AFK-ready issue**: A small, fully specified GitHub issue labeled `ready-for-agent`.
- **Agent PR**: A pull request opened by the agent workflow for human review.
