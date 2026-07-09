# Initial Hardware Configuration Contract

Issue: [#23 Document initial hardware configuration contract](https://github.com/ChaseWoodhams/watchpup-kvm/issues/23)

Date: 2026-07-09

Related decisions and research:

- [ADR 0002: Initial Firmware Platform And Hardware Path](../adrs/0002-initial-firmware-platform-and-hardware-path.md)
- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)
- [WatchPup KVM DIY Cost And BOM Research](../research/watchpup-diy-cost-bom.md)

## Purpose

This document records the first hardware contract for WatchPup's firmware bring-up path before hardware-specific implementation starts.

It is intentionally narrow. It captures the initial ESP32-P4 plus external TC358743 path used to de-risk firmware and board bring-up. It does not lock the project into a final integrated board design.

## In Scope

The initial hardware contract assumes:

- ESP32-P4 Ethernet dev-board class target hardware.
- External TC358743 HDMI-to-MIPI CSI adapter.
- Wired Ethernet only.
- PSRAM required.
- A USB device-capable port is available for TinyUSB keyboard and mouse emulation.

This matches the direction accepted in [ADR 0002](../adrs/0002-initial-firmware-platform-and-hardware-path.md) and the reference-shaped path described in the [p4kvm reference analysis](../research/p4kvm-reference-analysis.md).

## Resolution And EDID Posture

The initial bring-up posture is:

- Fixed 1080p30 EDID is allowed as a bring-up tool.
- 720p is the reliability and default target.
- 1080p is validation and detail mode only, not the default operating assumption.

This means early firmware may use a fixed 1080p30 EDID to simplify source negotiation while still treating stable 720p capture as the primary success target. Any later configurable EDID or wider mode support is separate work.

## Required Board Assumptions

The first hardware-dependent implementation may proceed only when these assumptions are true for the selected ESP32-P4 board plus TC358743 adapter stack:

- The board includes or exposes working wired Ethernet.
- The board includes usable PSRAM in a supported configuration.
- The board can power the chosen HDMI-to-CSI adapter and maintain stable capture-path signaling.
- The board exposes the interfaces needed for TC358743 control and MIPI CSI ingest.
- The board exposes a USB port or connector path usable in device mode for HID emulation.

If any of these assumptions change, this contract must be updated before firmware implementation depends on them.

## Required Fill-Ins Before Implementation

The following items are intentionally unknown until the exact board and adapter combination is selected and verified. They must be filled in explicitly, not guessed in code:

| Item | Required fill-in |
| --- | --- |
| ESP32-P4 board SKU | Exact dev board or module carrier selected for bring-up. |
| TC358743 adapter SKU | Exact external adapter board and connector/cable stack. |
| I2C pins | GPIO numbers used to control the TC358743. |
| Reset pin | Whether the bridge has a controllable reset pin, and if so which GPIO drives it and what polarity/timing it needs. |
| MIPI lane count | Number of CSI data lanes actually wired and expected by firmware. |
| MIPI lane rate | Verified operating lane rate for the selected board and adapter path. |
| Clocking | Reference clock source and any board-specific clock assumptions required by the bridge path. |
| FPC orientation | Cable orientation, pin-1 alignment, and any adapter inversion/crossing assumptions. |
| Ethernet PHY details | PHY part, interface mode, reset behavior, address, and any board-specific RMII or clock details firmware must honor. |
| USB device port | Which physical USB port is used for HID device mode and whether external cabling or jumpers are required. |
| Power assumptions | Input voltage, rail sharing, adapter power source, and whether any external power injection is required or forbidden. |

Unknowns should be resolved in documentation and board configuration boundaries before adding firmware logic that depends on them.

## Implementation Boundary

This contract is for the first external-adapter bring-up path only.

Out of scope for this issue:

- Integrated-board decisions.
- Final custom PCB architecture.
- Alternate bridge chips.
- Wi-Fi assumptions.
- Production enclosure, cable, or mechanical decisions.
- Broad mode-support policy beyond the initial EDID and resolution posture above.

Integrated-board planning remains a later hardware effort and should not be silently implied by this document.

## Consequences For Firmware Work

Before hardware-specific firmware implementation starts, the repo should have one explicit board configuration source that captures the fill-ins above. Firmware code should consume that configuration rather than scattering guessed pin numbers, lane assumptions, or power behavior across drivers.

The TC358743 bring-up sequence, EDID handling, Ethernet setup, and USB device assumptions should be implemented only after the selected board stack is documented against this contract.
