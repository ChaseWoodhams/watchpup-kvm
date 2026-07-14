# WatchPup Firmware Skeleton

This directory contains the initial ESP-IDF firmware skeleton for WatchPup KVM.

## Expected ESP-IDF Version

Use **ESP-IDF v6.0.2** for revision 3.x ESP32-P4 hardware. That baseline comes from:

- `docs/research/esp-idf-skeleton-baseline.md`
- `docs/adrs/0002-initial-firmware-platform-and-hardware-path.md`

The selected Waveshare ESP32-P4-ETH bench board reports chip revision v1.3.
ESP-IDF v6.0.2 only builds ESP32-P4 images for revision v3.0 or newer and
therefore rejects this board before flashing. Use **ESP-IDF v5.4.2** for this
specific compatibility bench target; do not force-flash the v6.0.2 image.

## Target

The firmware skeleton targets **ESP32-P4**. The top-level CMake file sets `IDF_TARGET` to `esp32p4`.

## Included In This Skeleton

- ESP-IDF project layout under `firmware/`
- stable `[diag]` serial boot and subsystem events
- NVS, event loop, PSRAM self-test, and Waveshare IP101 Ethernet/DHCP initialization
- a bench-only, read-only `GET /diag` schema-v1 endpoint
- a diagnostics-only TC358743 I2C probe, fixed bring-up EDID load, HPD
  sequencing, and HDMI/PLL/CSI status reporting
- component manager dependencies for `espressif/esp_tinyusb` and `espressif/mdns`
- `sdkconfig.defaults` and `sdkconfig.defaults.esp32p4` derived from `docs/research/esp-idf-skeleton-baseline.md`

## Explicitly Not Included Yet

- Raw CSI frame capture
- JPEG or MJPEG streaming
- HID control
- ATX control
- TC358743 or CSI board-specific drivers

## Local Build Notes

Typical ESP-IDF commands:

```sh
idf.py set-target esp32p4
idf.py build
```

For the revision-v1.3 compatibility build, keep its generated configuration and
build tree separate:

```sh
idf.py -B build-idf5 -D SDKCONFIG=sdkconfig.idf5 build
```

Both the v6.0.2 build and the host-side Phase 1 contract tests have been run
locally. Only the v5.4.2 compatibility image may be flashed to the selected
revision-v1.3 bench board.
