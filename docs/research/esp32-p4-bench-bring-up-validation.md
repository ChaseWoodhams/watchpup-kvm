# ESP32-P4 Bench Bring-Up Validation

Date: 2026-07-11

## Scope and current boundary

This is the safe, evidence-led order for testing a host-connected board against the
current repository. The firmware target is ESP32-P4 and the documented bench stack
is the UeeKKoo ESP32-P4-ETH development board plus Waveshare HDMI-to-CSI adapter;
the exact board pinout, bridge revision, FPC interconnect/orientation, and power
arrangement must be recorded before CSI work. [Firmware README](../../firmware/README.md),
[hardware contract](../hardware/initial-hardware-configuration-contract.md#selected-bench-bring-up-stack).

The present `firmware/` application implements the Phase 1 board-health baseline:
it emits diagnostics logs, initializes NVS and networking, checks PSRAM, brings up
the IP101 Ethernet PHY with DHCP, and exposes a bench-only read-only `/diag` route.
It still has no bridge, capture, streaming, HID, or ATX implementation. Therefore
the completed board-health evidence does not satisfy later hardware-contract gates.
[app_main.c](../../firmware/main/app_main.c),
[diagnostics contract](../firmware/diagnostics-endpoint-contract.md#initial-skeleton-fields).

## Preconditions and evidence record

Before applying power to the video stack, record the board SKU/revision, bridge
adapter SKU/revision, USB serial port, USB cable, ESP-IDF version, host OS, and
canonical HDMI source. Photograph and label the CSI connectors, pin-1 marks, FPC
orientation at both ends, cable/adapter type, lane-count information, and power
path. Do not guess GPIOs, lane rate, reset polarity, or power injection.
[required fill-ins](../hardware/initial-hardware-configuration-contract.md#required-fill-ins-before-implementation),
[Phase 0](../hardware/initial-hardware-configuration-contract.md#phase-0-physical-interface-qualification).

Use the same representative HDMI host for both BIOS/UEFI and operating-system
desktop tests. A pass must have a repeatable procedure plus captured serial output,
`/diag`, screenshot, or recorded observation; a failed dependent gate stops the
sequence until the failed boundary is diagnosed and retested.
[canonical source and discipline](../hardware/initial-hardware-configuration-contract.md#canonical-video-test-source).

## Step-by-step validation sequence

1. **Physical safety gate.** With the board unpowered, complete the Phase 0 record.
   Verify only the approved power voltage/path and connector orientation. **Pass:**
   safe, reproducible, documented physical stack. **Fail/blocker:** unknown cable,
   orientation, lane mapping, or unsafe power; do not flash capture firmware.
2. **Toolchain gate.** Use ESP-IDF **v5.4.2** for the selected revision-v1.3 bench
   board, and keep its build configuration separate. ESP-IDF v6.0.2 remains the
   project baseline for revision-v3.x hardware but must not be force-flashed to this
   board. Build output and the resolved ESP-IDF version are the evidence.
   [firmware build notes](../../firmware/README.md#local-build-notes),
   [CMake target](../../firmware/CMakeLists.txt).
3. **Flash/reflash gate.** Use the discovered serial port in `idf.py -p <PORT> flash`
   and repeat once. Capture the full command, flash result, and boot log. **Pass:**
   both flashes complete and the board resets into the expected application. This is
   the first required Phase 1 criterion. [Phase 1](../hardware/initial-hardware-configuration-contract.md#phase-1-board-health-baseline).
4. **Board-health gate.** Open `idf.py -p <PORT> monitor` (or an equivalent
   115200-baud serial monitor) and reset the board. Confirm `[diag]` boot and
   subsystem events, then retrieve `GET /diag` after Ethernet obtains DHCP.
   **Pass:** clean serial output plus a schema-v1 response reporting the PSRAM and
   Ethernet evidence. [app_main.c](../../firmware/main/app_main.c).
5. **Do not overclaim the baseline.** Mark bridge, CSI, JPEG, streaming, HID, and
   ATX as **not yet testable with this firmware**. The temporary unauthenticated
   `/diag` route is isolated-bench-only and must expose no video or control route.
   [skeleton exclusions](../../firmware/README.md#explicitly-not-included-yet).
6. **Implement-and-test Phase 1 before video work.** A future diagnostics slice must
   report PSRAM presence/size/test result, Ethernet link/DHCP/IPv4, and schema-v1
   `GET /diag`; bridge/capture/HID remain unavailable. On an isolated bench network,
   unauthenticated `/diag` is permitted only for this bootstrap build and must expose
   no video or control route. [Phase 1](../hardware/initial-hardware-configuration-contract.md#phase-1-board-health-baseline),
   [bench bootstrap exception](../firmware/diagnostics-endpoint-contract.md#bench-bootstrap-security-exception).
7. **Phase 2 bridge gate.** Only after Phase 1 passes, verify board configuration,
   I2C/control access, reset (if available), EDID, HPD, signal detect, negotiated
   mode, PLL lock, and CSI-output-enabled state in serial and `/diag`. Test the
   canonical host in BIOS/UEFI and desktop. **Pass:** repeatable signal lock in both;
   this does not prove captured pixels. [Phase 2](../hardware/initial-hardware-configuration-contract.md#phase-2-hdmi-bridge-diagnostics),
   [bridge sequence](../firmware/esp32-p4-tc358743-first-diagnostics-bring-up-sequence.md#sequence).
8. **Phase 3 pixel-ingest gate.** Before JPEG or browser work, prove changed frames
   from both source screens. Record dimensions, pixel format, sequence/FPS,
   completed/dropped frames, last error, and a changing frame fingerprint. **Pass:**
   repeated changing frames with no unreconciled errors or steadily rising drops.
   [Phase 3](../hardware/initial-hardware-configuration-contract.md#phase-3-raw-csi-frame-ingest-proof).
9. **Later gates.** Follow the contract in order: authenticated HTTP (4A), still JPEG
   (4B), one-viewer MJPEG (5A), slow-viewer recovery (5B), supervised keyboard (6),
   relative mouse (7), then fault-injection soak/recovery (8). Do not introduce HID
   until read-only streaming is healthy, and do not add ATX, paste, or hotkeys before
   the Phase 8 core gate. [later phases](../hardware/initial-hardware-configuration-contract.md#phase-4a-authenticated-http-foundation).

## Immediate test sheet

| Gate | Evidence to save | Result |
| --- | --- | --- |
| Board enumerates over USB serial | Port name and device description | Pass |
| ESP-IDF v5.4.2 compatibility build | Command and build log | Pass |
| Two successful flashes | Commands and flash logs | Pass |
| Board-health boot and `/diag` | Complete serial capture and JSON after reset | Pass |
| Physical CSI qualification | Photos/diagram and filled-in contract facts | Pending |
| Phase 1 diagnostics | Serial plus `/diag` response | Pass |

## Safety stop conditions

Stop and record the symptom rather than proceeding if the board resets repeatedly,
the USB serial connection is unstable, the actual target is not ESP32-P4, a power
path or FPC orientation is uncertain, or a phase lacks its required evidence. The
project explicitly treats a failed gate as a bounded diagnose/revise/retest loop,
not permission to start a dependent phase. [bench validation discipline](../hardware/initial-hardware-configuration-contract.md#bench-validation-discipline).
