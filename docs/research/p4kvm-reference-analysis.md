# p4kvm Reference Analysis

Issue: [#3 Analyze p4kvm reference code for reusable parts](https://github.com/ChaseWoodhams/watchpup-kvm/issues/3)

Reference inspected read-only: `C:\code\p4kvm-reference`

## Executive Summary

`p4kvm-reference` appears to be an ESP32-P4 proof-of-concept IP KVM. It captures HDMI through a Toshiba TC358743 HDMI-to-MIPI CSI bridge, encodes captured frames to JPEG on-device, serves an MJPEG stream and control UI over Ethernet, and forwards browser keyboard/mouse input to a target machine through USB HID.

The most valuable thing to reuse is not the code verbatim, but the working knowledge embedded in it: the ESP-IDF dependency set, the TC358743 bring-up sequence, ESP32-P4 CSI/JPEG buffer sizing, the HTTP endpoint shape, the browser-to-HID packet format, and the observed recovery/debug paths. The reference README explicitly says it is proof-of-concept software with many bugs and no security, so WatchPup should treat it as a lab notebook and driver spike, not as production architecture.

## What The Reference Appears To Do

- Boots an ESP32-P4 firmware app, initializes NVS, Ethernet, TinyUSB HID, an HTTP server, and then starts the capture task. Source: `main/main.c`.
- Captures 1920x1080 RGB888 frames over MIPI CSI from a TC358743 bridge into PSRAM-backed buffers. Source: `main/capture_hw.c`, `main/p4kvm_hw_defaults.h`.
- Encodes captured frames as JPEG and publishes the latest completed frame through a triple-buffered shared frame slot. Source: `main/capture_mjpeg.c`, `main/jpeg_frame.c`, `main/jpeg_frame.h`.
- Serves a single-page web UI, `/stream` MJPEG, `/jpeg-quality`, and `/ws` browser input over ESP-IDF `esp_http_server`. Source: `main/http_server.c`.
- Converts WebSocket mouse/keyboard packets into TinyUSB boot keyboard and mouse HID reports. Source: `main/usb_hid.c`, `main/usb_hid.h`.
- Uses Ethernet DHCP and mDNS to expose `http://p4kvm.local/`. Source: `main/ethernet.c`, `main/Kconfig.projbuild`.
- Builds the web UI with Vite, bundles it into one HTML file, and copies the built assets into `main/` for firmware embedding. Source: `web/package.json`, `main/CMakeLists.txt`.

## Central Modules And Files

`main/main.c`

The firmware composition root. It initializes NVS, networking, HID, JPEG global state, HTTP, then capture. This is useful as a boot-order reference, but the global initialization style should not become WatchPup's long-term architecture.

`main/capture_hw.c` and `main/capture_priv.h`

The hardware capture core. It configures MIPI LDO, I2C, TC358743, ESP camera CSI, ISP bypass, PSRAM DMA frame buffers, CSI callbacks, bridge registers, and HDMI recovery. This is the highest-value technical reference because it encodes ESP32-P4 and TC358743 sequencing that is hard to rediscover.

`main/tc358743.c`, `main/tc358743.h`, `main/tc358743_edid_1080p30.h`

The TC358743 bridge driver and fixed 1080p30 EDID. This is worth mining carefully for register order, timing delays, HPD behavior, CSI lane setup, color-space decisions, status/debug reads, and HDMI recovery. Rewrite it behind a cleaner WatchPup driver boundary rather than copying it whole.

`main/capture_mjpeg.c`, `main/jpeg_frame.c`, `main/jpeg_frame.h`

The frame pipeline. It waits on CSI completion, encodes JPEG, tracks buffer slots, supports a runtime JPEG quality setting in NVS, and notifies streaming clients. Reuse the concepts, but redesign ownership and backpressure deliberately.

`main/http_server.c`

The current API surface: `/`, `/favicon.ico`, `/stream`, `/jpeg-quality`, and `/ws`. It also contains important operational lessons about long-lived MJPEG responses, async HTTP handlers, socket exhaustion, and single active WebSocket input ownership.

`main/usb_hid.c`, `main/usb_hid.h`

Composite keyboard/mouse HID implementation using TinyUSB. This has reusable behavior for queueing input, coalescing mouse motion, converting absolute pointer updates into relative motion, and emitting boot keyboard reports.

`main/ethernet.c`, `main/Kconfig.projbuild`, `sdkconfig.defaults`

Network and build-time configuration. These files show the practical ESP-IDF options for Ethernet, mDNS, HTTP header sizing, TinyUSB, PSRAM, and lwIP TCP buffers.

`web/src/main.js`, `web/src/style.css`, `web/package.json`

The browser client. It parses multipart MJPEG manually, draws to canvas, handles pointer lock, translates keyboard events into HID usage codes, sends compact binary WebSocket packets, supports clipboard typing, and exposes JPEG quality and FPS UI controls.

## Important Dependencies And Configuration

- ESP-IDF version is locked at 6.0.1 in `dependencies.lock`; README build instructions also call out ESP-IDF 6.0.1.
- ESP-IDF component dependencies are `espressif/esp_tinyusb` and `espressif/mdns` in `main/idf_component.yml`; `dependencies.lock` resolves `esp_tinyusb` to 2.1.1, `tinyusb` to 0.19.0~2, and `mdns` to 1.11.0.
- Firmware requires ESP32-P4 target support and PSRAM. `sdkconfig.defaults` enables `CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_HEX`, and `CONFIG_SPIRAM_SPEED_200M`.
- HTTP/WebSocket support is enabled through `CONFIG_HTTPD_WS_SUPPORT=y`, larger request headers, and increased lwIP buffers in `sdkconfig.defaults`.
- Ethernet is configurable through `P4KVM_ETH_ENABLE` and RMII/PHY GPIO Kconfig entries. Defaults are ESP32-P4-specific.
- Hardcoded board-level defaults live in `main/p4kvm_hw_defaults.h`: TC358743 I2C pins, CSI resolution, 27 MHz reference clock, and 972 Mbps MIPI lane rate.
- Web build dependencies are intentionally minimal: Vite, esbuild, and `vite-plugin-singlefile`.

## Worth Reusing

Reuse as reference behavior or migration knowledge:

- TC358743 bring-up sequencing: HPD low, initial setup, EDID write, PLL/CSI setup, stream enable, HPD high, and CSI start. This is the most valuable hardware-specific knowledge.
- ESP32-P4 CSI and ISP bypass configuration for RGB888 1080p capture.
- PSRAM/DMA buffer sizing and alignment approach for full-frame capture.
- JPEG pipeline concepts: separate capture completion, encode slot selection, latest-frame publication, and runtime quality.
- MJPEG endpoint shape for a first visible-video milestone.
- WebSocket binary control protocol shape: compact fixed-size messages for mouse and keyboard.
- TinyUSB HID queueing and motion coalescing ideas.
- Debugging probes for TC358743 status, CSI timeouts, AVI infoframes, and HDMI recovery.
- Kconfig/sdkconfig defaults as a checklist for the first firmware skeleton.

## Should Be Rewritten

Rewrite these areas for WatchPup rather than porting directly:

- Application architecture. Current code relies heavily on file-static globals and cross-module shared state. WatchPup should define explicit subsystem boundaries for capture, encoder, stream server, HID input, device config, and diagnostics.
- Error handling and lifecycle management. Many initialization paths use `ESP_ERROR_CHECK` or task deletion. Production-oriented code should return typed failures, expose health state, and support controlled restart/recovery where possible.
- HTTP and WebSocket ownership. The current server mixes static file serving, MJPEG streaming, JPEG quality mutation, and HID control in one file. Split routing, stream sessions, input sessions, and device settings.
- Browser UI. The current client is a functional POC. Keep the protocol lessons, but rebuild the UI with maintainable state structure and explicit UX states.
- HID mapping. The current mapping is US-keyboard-centric and limited. WatchPup should treat keyboard layout, modifiers, key rollover behavior, stuck-key recovery, and pointer mode as first-class design decisions.
- Device configuration. Board constants and runtime settings should move behind a clear configuration model instead of hardcoded defaults plus scattered Kconfig.
- Security model. The reference has none by design. Even if early WatchPup stays LAN-only, explicitly document what is and is not protected.

## Should Not Be Brought Over

- Do not copy source files wholesale into WatchPup.
- Do not bring over the proof-of-concept "no security" posture as a product default.
- Do not bring the monolithic `http_server.c` shape forward as the permanent server architecture.
- Do not bring generated/minified `main/index.html` as source; generated web output should stay generated.
- Do not treat the fixed 1080p30 EDID as the final display model. It may be a useful first milestone, but it should become a configurable/replaceable asset.
- Do not copy affiliate-link hardware assumptions or enclosure assumptions into project docs as product requirements.
- Do not bring the exact global-state frame sharing model forward without a concurrency review.

## Risks And Messy Areas

- HDMI recovery is already called out as unreliable in the README, and the code contains a timeout-triggered hotplug/CSI recovery path. This should be treated as an open research area, not solved behavior.
- The README explicitly warns that the reference has no security. Any browser-accessible HID control plane is dangerous outside a trusted local network.
- The capture path is fixed around 1920x1080 RGB888 and a specific bridge/board configuration. Resolution, color format, and chip revision support need product decisions.
- The browser manually parses MJPEG multipart streams. This works, but it is easy to make memory/backpressure mistakes in long-lived sessions.
- Mouse behavior bridges browser pointer-lock deltas, canvas coordinates, and boot mouse relative reports. Small mistakes here produce drift, acceleration mismatch, or stuck buttons.
- The firmware currently assumes single-user KVM behavior for the WebSocket owner. Multi-client viewing versus single-client control needs an explicit policy.
- Build flow couples web output into firmware source paths. That is convenient for a POC but should be isolated to generated output in WatchPup.
- Licensing must be checked before any reuse beyond ideas; the reference has a LICENSE file and SPDX headers, but this research intentionally avoids copying code.

## Recommended First Migration Tickets

1. Create a firmware architecture spike doc for WatchPup's subsystems: capture, bridge driver, encoder, streaming, input, config, diagnostics, and web UI.
2. Add an ESP-IDF firmware skeleton with no hardware behavior beyond boot logging and build configuration for ESP32-P4.
3. Add a hardware configuration ADR covering target board, TC358743 adapter, I2C pins, reset pin, MIPI lane rate, PSRAM, Ethernet PHY, and initial resolution.
4. Port the TC358743 bring-up knowledge into a new WatchPup driver design document, with register sequence notes but no copied source.
5. Implement a minimal diagnostics endpoint or serial diagnostic plan before implementing video streaming, so HDMI/CSI state can be inspected early.
6. Implement a capture-pipeline prototype ticket: CSI frame acquisition to an internal buffer, no HTTP and no HID yet.
7. Implement a JPEG encode prototype ticket with explicit buffer ownership and backpressure rules.
8. Implement a read-only MJPEG stream milestone before any keyboard or mouse control.
9. Design the WebSocket HID protocol as a documented WatchPup protocol, including security assumptions and stuck-input recovery.
10. Implement TinyUSB HID as a separate subsystem after the video path is observable and stable.

## Recommended Near-Term ADRs

These are now captured in `docs/adrs/`:

- ADR: product vision and v1 boundary.
- ADR: initial firmware platform, hardware target, and supported capture mode.
- ADR: diagnostics-first subsystem boundaries.
- ADR: web asset build and embedding strategy.
- ADR: frame ownership and streaming backpressure model.
- ADR: input-control policy, including mandatory auth, single active controller, and emergency release behavior.

## Bottom Line

Use `p4kvm-reference` as a hardware and protocol reference, not as a codebase seed. The first WatchPup milestone should be documentation and architecture around the known-good hardware path, followed by small firmware tickets that prove one subsystem at a time. The riskiest parts are TC358743/CSI stability, frame-buffer ownership under MJPEG load, and HID safety; those should be isolated early instead of hidden inside a direct port.
