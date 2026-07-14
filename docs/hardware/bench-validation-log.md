# Bench Validation Log

This log records reproducible hardware-validation evidence for the selected
WatchPup bench target. It supplements, rather than replaces, the ordered gates
in [the initial hardware configuration contract](initial-hardware-configuration-contract.md).

## Bench Identity

| Field | Observed value | Evidence | Status |
| --- | --- | --- | --- |
| Host serial interface | `USB-Enhanced-SERIAL CH343 (COM3)` | Windows Plug and Play instance `USB\\VID_1A86&PID_55D3\\5B7B032060` | Pass |
| SoC target | ESP32-P4 | ROM banner: `ESP-ROM:esp32p4-eco2-20240710` | Pass |
| Chip revision | v1.3 | Boot log: `chip revision: v1.3` | Pass |
| PSRAM | 32 MB, 200 MHz; memory test passed | Boot log: `Found 32MB PSRAM device` and `SPI SRAM memory test OK` | Pass |
| Physical flash | 32 MB | Boot log warning reports a 32 MB detected flash device | Pass |
| Running image | Vendor `esp_brookesia_demo` 1.0.1, ESP-IDF v5.4.2 | Application-information boot log | Informational |
| WatchPup image | Phase 1 compatibility image flashed twice | ESP-IDF v5.4.2 flash logs and serial captures | Pass |
| Silicon/toolchain compatibility | ESP32-P4 rev v1.3 requires ESP-IDF v5.4.2 | ESP-IDF v6.0.2 rejected its rev-v3.1 bootloader before writing | Compatibility build required |

## Test 001: Passive Serial Health Baseline

**Date:** 2026-07-11
**Purpose:** establish that the connected board can boot and emit readable
serial output before modifying its flash.

### Procedure

1. Confirm that Windows reports `COM3` with status `OK`.
2. Open `COM3` at 115200 baud, 8 data bits, no parity, one stop bit, no flow
   control; keep DTR and RTS disabled.
3. Capture the boot output for five seconds, then close the port.
4. Check the log for the ESP32-P4 ROM banner, bootloader, PSRAM discovery, and
   a completed application start.

### Result

**Pass.** The board emitted an ESP32-P4 boot log, completed PSRAM tuning and
the PSRAM memory test, then entered the vendor application. Serial-port access
may cause the board to reset even when DTR/RTS are configured off; treat every
serial-console attach as an observable reset until this board's auto-reset
circuit is characterized.

### Notes and risks

- The installed vendor image declares a 16 MB flash layout while the boot log
  detects a 32 MB flash device. This is not a WatchPup configuration and must
  not be copied into the WatchPup partition table.
- The observed image is ESP-IDF v5.4.2. WatchPup's documented baseline is
  ESP-IDF v6.0.2, but that release only supports ESP32-P4 revision v3.x. The
  bench board must use a separately generated ESP-IDF v5.4.2 compatibility
  image; force-flashing a v6 image is forbidden.

## Ordered Validation Gates

Do not skip a gate after a failure. Record the symptom, hypothesis, change,
and rerun result before advancing.

| Gate | Objective | Required evidence | Current state |
| --- | --- | --- | --- |
| 0. Physical capture-path qualification | Record board/bridge SKU, CSI connector and cable type, pin-1 orientation, lane count/mapping, bridge power path, and safe power arrangement. | Photos or diagrams plus completed configuration fields. | Not run |
| 1. WatchPup board-health baseline | Build and flash the ESP32-P4 image; prove reflash, serial `[diag]` boot/health events, PSRAM reporting, Ethernet DHCP, and `GET /diag` schema v1. | Build log, flash log, serial capture, `/diag` JSON. | Pass: flashed twice; serial and live endpoint validators passed after each boot. |
| 2. HDMI bridge diagnostics | Probe/reset TC358743, load EDID, control HPD, and report signal/mode/PLL/CSI readiness. Test canonical host in BIOS/UEFI and desktop. | Serial and `/diag` bridge state transitions. | Not implemented |
| 3. Raw CSI ingest | Prove changing frames and report dimensions, format, frame counters/drops, and a changing frame fingerprint. | Serial and `/diag` counters plus source-screen observation. | Not implemented |
| 4A. Authenticated HTTP foundation | Provision one administrator through physical serial; confirm anonymous rejection for all non-diagnostic routes. | Provisioning transcript and HTTP responses. | Not implemented |
| 4B. Still JPEG | Serve one authenticated current frame without stale-frame or auth bypass. | HTTP response, image, and diagnostics evidence. | Not implemented |
| 5A/5B. MJPEG and recovery | Prove one viewer first, then bounded latest-frame behavior, slow-client handling, and recovery. | Browser observation, counters, and soak evidence. | Not implemented |
| 6. Keyboard HID safety | Verify USB enumeration and intentional input in BIOS and desktop, then prove forced release for disconnect, timeout, ownership/auth loss, protocol error, and emergency release. | Host observations plus release counters/logs. | Not implemented |
| 7. Relative mouse safety | Verify intentional motion/buttons in BIOS and desktop, bounds validation, ownership, and forced release. | Host observations plus diagnostics. | Not implemented |
| 8. End-to-end soak and recovery | Run sustained canonical-host video with network disruption and recovery tests. | Timed test record, diagnostics, recovery results. | Not implemented |

## Test 002: Phase 1 Board Health

**Date:** 2026-07-11
**Result:** **Pass**

### Repeatable procedure

1. Source ESP-IDF v5.4.2, change to `firmware/`, and build the compatibility
   image with `idf.py -B build-idf5 -D SDKCONFIG=sdkconfig.idf5 build`.
2. Identify the board's serial port, then flash with
   `idf.py -B build-idf5 -p <PORT> flash`; retain the per-region hash-verification
   output. Repeat the same flash command once to prove reflash.
3. Reset the board and capture serial output. Run
   `node tools/validate-phase1.mjs --serial-log <SERIAL_LOG>` against that capture.
4. After Ethernet receives DHCP, run
   `node tools/validate-phase1.mjs --url http://<DEVICE_IP>/diag` from the isolated
   bench network. Save the JSON response and command output as the Phase 1 evidence.

- Factory recovery image: `C:\sandbox\watchpup-kvm-backups\esp32-p4-eth-factory-16mb-2026-07-11.bin`
- Factory-image size: 16,777,216 bytes
- Factory-image SHA-256: `3248DF24731FE44092FEC45ED6F4D9FB4FDA3C4B319CBA74D946B7FFC6437003`
- Compatibility build: ESP-IDF v5.4.2, supported chip range v0.1-v1.99
- Flash and reflash: both completed with per-region hash verification
- PSRAM: 33,554,432 bytes; firmware allocation/integrity test passed
- Ethernet: IP101 initialized, link up, DHCP address `192.168.10.27`
- `/diag`: schema version 1, `overall_status: "ok"`, board ID `waveshare_esp32_p4_eth`
- Final serial evidence: `C:\sandbox\watchpup-kvm-backups\phase1-final-serial-2026-07-11.log`
- Validators: five host contract tests, final serial-log validation, and live endpoint validation all passed
- Route boundary: POST `/diag` and GET `/snapshot.jpg`, `/stream`, `/control`, and `/power` were rejected
- Compatibility builds: ESP-IDF v5.4.2 and v6.0.2 both compile; only the v5.4.2 image is eligible for this revision-v1.3 board

## Next Executable Gate

Complete the remaining Phase 0 physical CSI qualification record if it has not
already been captured, then rerun Phase 2 with the canonical HDMI source
actively driving the adapter. The pass condition is repeatable TMDS/sync lock
with non-zero negotiated dimensions in both BIOS/UEFI and the desktop.

## Test 003: Phase 2 TC358743 Bridge Diagnostics

**Date:** 2026-07-13
**Result:** **Partial pass; source-signal gate pending**

The Phase 2 compatibility image was built with ESP-IDF v5.4.2 and flashed to
the ESP32-P4 v1.3 board on `COM3`. Flashing completed with hash verification
for the bootloader, application, and partition table.

The serial evidence proves:

- I2C0 on SDA GPIO7/SCL GPIO8 received an ACK at address `0x0f`.
- The TC358743 chip ID read as `0x0000`.
- The two-block Waveshare 1080p30 EDID loaded successfully.
- HPD was asserted after EDID setup.
- The HDMI PHY PLL reported locked.
- The bridge diagnostics endpoint reported the same state at
  `http://192.168.10.27/diag`.

The connected bridge reported `hdmi_signal_detected=false`,
`negotiated_width=0`, `negotiated_height=0`, and
`negotiated_frame_rate=0`. Therefore the source-negotiation acceptance gate
is not yet passed. `csi_output_enabled=false` is expected at this phase because
raw CSI capture has not been started; it is not evidence of captured pixels.

Serial capture: `C:\sandbox\watchpup-kvm-phase2-serial-reset.txt`.

## Test 004: Phase 2 Source-Path Bring-Up

**Date:** 2026-07-13
**Result:** **Partial pass; no active HDMI/TMDS source detected**

The bridge probe was rerun after adding the TC358743 reference-clock, PHY,
DDC, detector, and output-path initialization. The new image built cleanly
with ESP-IDF v5.4.2 and flashed to `COM3` with hash verification. Serial and
`/diag` evidence confirmed that the source path now completes, EDID loads,
HPD is asserted, and the PHY PLL remains locked.

The bridge still reports `sys_status=0x05`,
`hdmi_signal_detected=false`, `hdmi_sync_locked=false`, and zero negotiated
dimensions. `0x05` indicates the adapter-side 5V/PHY-PLL state without the
TMDS signal bit, so the remaining gate requires an actively outputting HDMI
source. The CSI transmitter remains disabled as expected before Phase 3
capture.

## Test 005: Host HDMI Output and EDID Probe

**Date:** 2026-07-13
**Result:** **Host output configured; bridge EDID not enumerated**

On the connected Windows host, NVIDIA display configuration reported an active
HDMI target at `1024x768@60`. A Plug-and-Play display rescan and an HDMI
unplug/replug did not change the enumeration: Windows continued to expose a
`Default_Monitor` rather than the bridge's `DCDZ-H2C MOD` EDID. The host's
registry contained a different cached `ACT8360` monitor profile named
`Transmitter`; it did not contain the bridge EDID identity.

The firmware EDID itself has valid base and extension-block checksums, and the
ESP32 still reports `edid_loaded=true` and `hpd_asserted=true`. The bridge
continues to report `sys_status=0x05`: DDC 5V and PHY PLL are present, but the
TMDS signal bit is clear. This separates host display configuration from a
working HDMI data path and leaves cable direction, adapter input/power, or
source-to-adapter physical connectivity as the next checks.

## Test 006: Apollo Virtual Display Separation

**Date:** 2026-07-13
**Result:** **Virtual and physical display paths separated**

The host is running Apollo/Sunshine. Its remote-stream display is the
`SudoMaker Virtual Display Adapter`, with a `Generic Monitor (TCL)` at
`2400x1600`; that monitor is parented by `ROOT\\DISPLAY\\0000` and is not the
physical HDMI path.

The physical path is separately enumerated as the `NVIDIA GeForce RTX 3060`
parent with `Generic Monitor (Default_Monitor)` at `1024x768`. The ESP32
bridge diagnostics are independent of the Apollo virtual monitor and continue
to report no TMDS signal. Subsequent host checks must use the NVIDIA physical
path only.

## Test 007: TC358743 Raw HDMI Register Probe

**Date:** 2026-07-13
**Result:** **PHY and HPD active; no TMDS data detected**

The extended diagnostics image was built and flashed to `COM3` with hash
verification. The live `/diag` response and serial output reported:

- `phy_en=0x01` and `phy_rst=0xb3`: HDMI PHY enabled and reset control
  deasserted.
- `hpd_ctl=0x01`: HPD asserted toward the host.
- `ddc_ctl=0x32`: the configured 100 ms DDC 5V delay is present.
- `hdmi_det=0xc1` and `hv_rst=0x06`: detector configuration is readable and
  horizontal/vertical reset controls are not asserted.
- `sys_status=0x05`: PHY PLL and DDC 5V are present, but TMDS and sync remain
  clear.

This rules out an unenabled or held-in-reset HDMI PHY in the current firmware.
The remaining failure is the source-to-adapter TMDS path or the host's
physical HDMI output/cable/input connection.

## Test 008: CSI Ribbon Re-Seated on Camera Connector

**Date:** 2026-07-13
**Result:** **Correct connector; HDMI source lock still absent**

The HDMI-to-CSI ribbon was moved from the ESP32-P4-ETH `Display` connector to
the `Camera` connector, which is the board's MIPI-CSI input. After a clean
reset, the bridge still passed I2C probe, EDID, HPD, and PHY PLL checks. The
raw status remained `sys_status=0x05`, `hdmi_signal_detected=false`, and
`hdmi_sync_locked=false`, with zero negotiated timing.

This corrects the CSI-side wiring for a later capture test, but it does not
change the HDMI receiver's source-side TMDS lock. CSI output remains disabled
because Phase 3 capture has not been started.

## Test 009: TC358743 Analog HDMI Path and Host Attach Retest

**Date:** 2026-07-13
**Result:** **Host attach recovered; HDMI video data still absent**

The firmware now initializes and reports the TC358743 analog HDMI control
register used by the upstream driver (`ana_ctl=0x31`). After flashing and
resetting, NVIDIA reported `Display Attached: Yes` for the physical output.
The bridge still reported `edid_loaded=true`, `edid_length_bytes=256`,
`hpd_ctl=0x01`, `ana_ctl=0x31`, and `pll_locked=true`, but remained at
`sys_status=0x05`, `hdmi_signal_detected=false`, `hdmi_sync_locked=false`, and
zero negotiated timing. Windows still has no EDID registry blob for the
physical `Default_Monitor` path.

This confirms that the ESP32 can initialize the TC358743, assert HPD, and
present an attached HDMI output, while the receiver still does not see TMDS
video. The next test must force a known supported source mode or isolate the
host HDMI cable/output from the adapter input; CSI capture cannot proceed until
the bridge reports TMDS and sync.
