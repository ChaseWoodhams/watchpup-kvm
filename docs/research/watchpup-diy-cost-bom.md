# WatchPup KVM DIY Cost And BOM Research

Issue: [Research the DIY cost and BOM target](https://github.com/ChaseWoodhams/watchpup-kvm/issues/17)

Date: 2026-07-09

## Question

Is the proposed DIY dev-kit cost target of about `$50-$80` plausible when accounting for ESP32-P4 board/module, HDMI-to-CSI bridge, Ethernet, USB/HDMI connectors, power, ATX isolation, ESD protection, PCB, enclosure, cables, assembly overhead, and sourcing risk?

## Short Answer

The `$50-$80` target is plausible for an early DIY/dev-kit WatchPup KVM build, but only if it is framed carefully: a lean prototype stack, excluding shipping, tax, tools, spares, enclosure polish, assembly labor, compliance, and support.

The honest v1 cost promise should be:

- **DIY prototype/dev-kit target:** around `$50-$80` before shipping/tax/tools.
- **Useful positioning goal:** cheap enough that a homelab user can imagine putting one on every important mini PC or lab machine.
- **Integrated-board target:** promising but not proven. It needs a later hardware/BOM plan with real schematic, PCB, assembly, test, enclosure, cable, and sourcing assumptions.

## Source Snapshot

Prices are volatile and should be rechecked before any public claim, kit decision, or purchase recommendation.

### ESP32-P4 Board Or Module

An off-the-shelf ESP32-P4 Ethernet development board is already visible in the low `$20s`. A Waveshare ESP32-P4 Ethernet development board listing showed `$20.15`, with 100M RJ45 Ethernet, MIPI-CSI/DSI, USB OTG 2.0 HS, PSRAM, flash, JPEG/H.264-related hardware capability claims, and security features listed on the product page.

DigiKey's ESP32-P4-EYE page pointed to an ESP32-P4X-EYE substitute around `$33.68`, which is a better sanity check for distributor pricing than a single marketplace listing.

SpotPear listed an ESP32-P4 module/core board at `$11.90`. That is useful for integrated-board thinking, but it is not a complete KVM board; it shifts connector, Ethernet, power, layout, and assembly work into WatchPup's hardware design.

Sources:

- [Waveshare ESP32-P4 Ethernet development board on Amazon](https://www.amazon.com/ESP32-P4-Ethernet-Development-MIPI-CSI-Microphone/dp/B0FN4DLKLB)
- [ESP32-P4-EYE / ESP32-P4X-EYE on DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-P4-EYE/27555429)
- [SpotPear ESP32-P4 module core board](https://spotpear.com/shop/ESP32-P4-C6-Module-32MB-PSRAM-16MB-Flash-MIPI-DSI-CSI.html)

### HDMI-To-CSI Bridge

The HDMI capture side is the expensive part of the early DIY stack. A 52Pi HDMI-to-CSI2 adapter board for Raspberry Pi was listed at `$29.99` and describes use of the Toshiba TC358743XBG bridge with HDMI input support up to 1080p25fps.

At component level, LCSC listed `TC358743XBG(EL,NOK)` around `$11.7058` at quantity 1 and `$9.3086` at quantity 100. That suggests an integrated board can eventually beat the dev-kit-plus-adapter stack, but the chip price alone hides BGA assembly, MIPI signal integrity, supporting passives, power rails, clocking, ESD/protection, connector placement, firmware bring-up, and yield risk.

Sources:

- [52Pi HDMI-to-CSI2 adapter board at Micro Center](https://www.microcenter.com/product/688934/52pi-hdmi-to-csi2-adapter-board-for-raspberry-pi)
- [LCSC video processing category showing TC358743XBG pricing](https://www.lcsc.com/category/969.html)

### Connectors And ATX Control Parts

Commodity connector and isolation parts are not the dominant cost, but they matter once WatchPup becomes a board rather than a dev-kit stack.

Representative low-volume LCSC pricing showed:

- USB-C connectors from a few cents for simple 6-pin parts to roughly `$0.42` at quantity 1 for a 24-pin connector category example.
- An HDMI connector around `$0.2448` at low quantity.
- A combined RJ45/USB connector part around `$1.03` at quantity 1 and about `$0.64` at quantity 100.
- PC817 optocouplers around a few cents each in quantity.

These numbers support the idea that connectors and ATX isolation are manageable in the BOM. They do not include ESD arrays, magnetics selection details, USB-C role/power circuitry, Ethernet PHY choices if not embedded on the module/board, mechanical reliability, or assembly/test overhead.

Sources:

- [LCSC USB/DVI/HDMI connector category](https://www.lcsc.com/category/842.html)
- [LCSC HDMI connector example](https://www.lcsc.com/product-detail/C2906135.html)
- [LCSC RJ45/USB connector example](https://www.lcsc.com/product-detail/ethernet-connectors-modular-connectors-rj45-rj11_fg-rj-060-1usb2-0-4_C7205181.html)
- [LCSC PC817 optocoupler example](https://www.lcsc.com/product-detail/C3008368.html)

### PCB And Assembly

JLCPCB advertises low-cost PCB fabrication and assembly services, including low headline prices for PCB boards and component sourcing/assembly. That supports the general feasibility of cheap prototype runs, but it is not enough to declare a finished integrated kit cost.

For WatchPup, the hard costs that are easy to undercount are:

- multiple PCB spins,
- BGA assembly quality for TC358743,
- MIPI and HDMI layout/debug time,
- ESD/protection choices,
- enclosure and cable set,
- test fixtures,
- rework/scrap/yield,
- shipping, tariffs, and stocking,
- documentation and support.

Source:

- [JLCPCB PCB fabrication and assembly services](https://jlcpcb.com/)

## Rough Cost Models

### Lean DIY Dev-Kit Stack

This is the right public target for early WatchPup experiments:

| Item | Plausible range | Notes |
| --- | ---: | --- |
| ESP32-P4 Ethernet board | `$20-$34` | Marketplace and distributor sanity-check range. |
| HDMI-to-CSI adapter | `$30` | Off-the-shelf TC358743 adapter board. |
| USB/HDMI/FPC cables, power, jumpers | `$5-$15` | Varies heavily by what the builder already owns. |
| ATX power/reset tap prototype parts | `$3-$10` | Optocouplers and connectors are cheap; cable/mechanical details vary. |
| Enclosure | `$0-$15` | None, printed, or rough off-the-shelf box. |

This yields roughly `$58-$104` depending on how much is counted. With a lean shopping cart and reused cables/enclosure, `$50-$80` is credible. With all accessories, shipping, tax, and enclosure included, it can easily leave that range.

### Integrated Board Direction

An integrated WatchPup board could lower the recurring BOM by replacing the dev board plus adapter board stack with one PCB. The source snapshot suggests the core silicon and commodity connectors do not make a sub-`$80` hardware BOM impossible.

But integrated cost is not yet a decision. It requires a hardware-specific plan that covers:

- exact ESP32-P4 module/chip approach,
- TC358743 sourcing and lifecycle risk,
- Ethernet PHY/magnetics or module-board choice,
- USB-C/device-port design,
- HDMI input and ESD protection,
- ATX control connector and isolation policy,
- power input and target-side isolation rules,
- enclosure and cable bundle,
- assembly/test assumptions and yield target.

## Recommended Product Wording

Use this wording for the product strategy:

> WatchPup KVM targets a lean DIY/dev-kit build around `$50-$80` before shipping, tax, tools, and polish, with the longer-term goal of an integrated open-hardware board cheap enough to put on every important homelab computer.

Avoid wording that implies:

- a finished supported kit can definitely sell for `$50-$80`,
- the integrated BOM has already been proven,
- shipping, tax, cables, enclosure, and assembly are included,
- WatchPup will always beat the cheapest Linux-SoC KVMs on price.

## Decision Unlocked

This research supports keeping cost as a core product constraint, but as a disciplined constraint:

- V1 strategy should target a cheap, transparent, reproducible DIY path first.
- Product language should lean into "cheap enough to put on every computer" as the wedge, not "the cheapest KVM on the market."
- A separate hardware/PCB wayfinder map is likely needed after product strategy closes, because integrated-board cost cannot be resolved responsibly from catalog pricing alone.

