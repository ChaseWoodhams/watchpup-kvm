# WatchPup Firmware Skeleton

This directory contains the initial ESP-IDF firmware skeleton for WatchPup KVM.

## Expected ESP-IDF Version

Use **ESP-IDF v6.0.2** for this skeleton. That baseline comes from:

- `docs/research/esp-idf-skeleton-baseline.md`
- `docs/adrs/0002-initial-firmware-platform-and-hardware-path.md`

If hardware bring-up later shows a reference-specific regression, test against `v6.0.1` as documented in the research note. This skeleton itself is authored against the `v6.0.2` baseline.

## Target

The firmware skeleton targets **ESP32-P4**. The top-level CMake file sets `IDF_TARGET` to `esp32p4`.

## Included In This Skeleton

- ESP-IDF project layout under `firmware/`
- `main/app_main.c` boot logging
- minimal NVS, event loop, and network stack initialization
- placeholder logs for later diagnostics, mDNS, TinyUSB, and subsystem bring-up
- component manager dependencies for `espressif/esp_tinyusb` and `espressif/mdns`
- `sdkconfig.defaults` and `sdkconfig.defaults.esp32p4` derived from `docs/research/esp-idf-skeleton-baseline.md`

## Explicitly Not Included Yet

- HDMI capture
- JPEG or MJPEG streaming
- HID control
- ATX control
- board-specific hardware drivers
- `/diag` endpoint implementation

## Local Build Notes

Typical ESP-IDF commands:

```sh
idf.py set-target esp32p4
idf.py build
```

`idf.py` was **not available** in the local issue workspace environment during this change, so `idf.py build` was not run here.
