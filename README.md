# WatchPup KVM

WatchPup KVM is an open-source, microcontroller-class KVM-over-IP project built around an ESP32-P4 plus TC358743 HDMI-to-MIPI CSI path, wired Ethernet, and an MJPEG-first browser access model.

The target user is a technical operator who wants BIOS-level access, crash recovery, OS install help, and machine recovery on homelab or lab machines without paying for a full Linux-based KVM appliance on every box.

The goal is to build a cheap-enough, transparent, inspectable KVM node that technical users can understand, reproduce, debug, and improve.

## Status

This repository is still at a baseline stage.

- Product direction, research, ADRs, and initial implementation boundaries are documented.
- Firmware, web UI, hardware design files, and enclosure files are not fully implemented yet.
- The repo currently contains early scaffolding and planning artifacts, not a finished device.
- The current documented path is ESP32-P4 + TC358743, with diagnostics-first sequencing and read-only MJPEG before broader control features if needed to reduce risk.

Start with [CONTEXT.md](CONTEXT.md) for the current shared project context.

## Product Direction

WatchPup is aimed at people who want BIOS-level access, crash recovery, OS install help, and machine recovery without paying for a premium KVM on every box.

The v1 boundary is intentionally narrow:

- Ethernet-first networking
- ESP32-P4 plus TC358743 as the first hardware-proving path
- MJPEG-first streaming
- Diagnostics-first implementation sequencing
- LAN-only authenticated HTTP for local access
- A single active HID controller at a time

More specifically, v1 prioritizes a trustworthy microcontroller KVM path over polish:

- 720p is the default reliability target.
- 1080p is only a validation/detail mode if stable on real hardware.
- Read-only MJPEG may land before HID control if that reduces execution risk.
- USB keyboard and mouse come first, with conservative stuck-input safety behavior.

Out of scope for v1 includes cloud access, Wi-Fi, H.264-first ambitions, virtual media, and broader production hardening promises that do not exist yet.

## Architecture And Layout

This repo is organized around a documented target architecture plus early scaffolding:

- [`firmware/`](firmware/) contains the ESP-IDF firmware project targeting ESP32-P4. See [firmware/README.md](firmware/README.md).
- [`web/`](web/) contains authored browser UI source intended to stay separate from generated firmware assets.
- [`docs/prds/`](docs/prds/) contains PRDs and spec-oriented planning documents.
- [`docs/research/`](docs/research/) contains research notes, reference analysis, and feasibility work.
- [`docs/adrs/`](docs/adrs/) contains architecture decision records that define settled project direction.
- [`docs/agents/`](docs/agents/) contains issue-tracker, triage, and agent workflow guidance.
- [`docs/firmware/`](docs/firmware/) contains firmware contracts and subsystem docs such as diagnostics and HID safety.
- [`docs/hardware/`](docs/hardware/) contains hardware contracts and bridge notes for the planned device path.

If you want the shortest path through the current documentation:

1. Read [CONTEXT.md](CONTEXT.md).
2. Read the [product PRD outline](docs/prds/watchpup-product-prd-outline.md).
3. Read the relevant [research docs](docs/research/README.md) and [ADRs](docs/adrs/README.md).

## Collaboration

This project is run as a public, issue-driven repository.

- Work should start from a GitHub issue, not an open-ended implementation idea.
- Implementation agents should only act on issues labeled `ready-for-agent`.
- Agents may open or update pull requests, but every agent PR is human-reviewed before merge.
- Humans merge only.

Important review labels are documented in [AGENTS.md](AGENTS.md) and [docs/agents/triage-labels.md](docs/agents/triage-labels.md).

Issue tracker conventions and `gh` CLI guidance live in [docs/agents/issue-tracker.md](docs/agents/issue-tracker.md).

## Contributor Safety

This repo intentionally keeps a tight boundary around sensitive work.

- Do not commit secrets, tokens, auth files, `.env` files, generated artifacts, build outputs, or Sandcastle logs.
- Do not take on firmware-control, hardware-control, networking, auth, deployment, or security-sensitive changes unless a small scoped issue explicitly asks for them.
- Keep changes small, reviewable, and limited to the assigned issue.
- If a proposed change conflicts with an ADR or the documented v1 boundary, raise it instead of silently overriding the docs.

## Current Docs

- Shared context: [CONTEXT.md](CONTEXT.md)
- PRDs and specs: [docs/prds/](docs/prds/)
- Research: [docs/research/](docs/research/)
- ADRs: [docs/adrs/](docs/adrs/)
- Agent workflow docs: [docs/agents/](docs/agents/)
- Firmware contracts: [docs/firmware/](docs/firmware/)
- Hardware docs: [docs/hardware/](docs/hardware/)

## Local Checks

The repo currently defines lightweight top-level verification commands:

```sh
npm run typecheck
npm run build
```

Those checks should pass before issue work is considered done.
