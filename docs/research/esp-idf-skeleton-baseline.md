# ESP-IDF Skeleton And Dependency Baseline

Issue: [Research ESP-IDF skeleton and dependency baseline](https://github.com/ChaseWoodhams/watchpup-kvm/issues/8)

Date: 2026-07-09

## Question

What ESP-IDF version, component dependencies, `sdkconfig` defaults, and repository layout should WatchPup use for its first ESP32-P4 firmware skeleton, based on the p4kvm analysis and current ESP-IDF documentation?

## Short Answer

WatchPup should start with a dedicated `firmware/` ESP-IDF project targeting ESP32-P4, pinned to the current ESP-IDF stable release line. As of this research pass, Espressif's stable ESP32-P4 documentation is for ESP-IDF `v6.0.2`; the p4kvm reference used `v6.0.1`. The safest baseline is therefore:

- Use **ESP-IDF `v6.0.2`** for new WatchPup work, unless hardware bring-up reproduces only on `v6.0.1`.
- Keep the reference's dependency shape: `espressif/esp_tinyusb` and `espressif/mdns`.
- Use ESP-IDF built-in components for camera/CSI, ISP, JPEG, Ethernet, netif, event loop, NVS, timers, GPIO/I2C, PSRAM, and HTTP server.
- Commit `sdkconfig.defaults` and target-specific `sdkconfig.defaults.esp32p4` for reproducible project settings.
- Commit the generated firmware `dependencies.lock` after the first successful skeleton build, because transparency and repeatability matter for WatchPup.
- Do not include H.264, virtual media, Wi-Fi companion support, or production TLS/security hardening in the first skeleton.

## Source-Backed Facts

### ESP-IDF Version

Espressif's ESP32-P4 Get Started guide says the stable documentation is currently for ESP-IDF `v6.0.2`. The same page describes ESP32-P4 as having MIPI, USB, SDIO, Ethernet, and built-in security hardware.

The p4kvm reference README says to build with ESP-IDF `6.0.1`, and its `dependencies.lock` records `idf` version `6.0.1`. That means WatchPup should expect minor-version drift from the reference and should validate on hardware before treating any ESP-IDF version as permanent.

Sources:

- [ESP-IDF Get Started for ESP32-P4](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/get-started/index.html)
- `C:\code\p4kvm-reference\README.md`
- `C:\code\p4kvm-reference\dependencies.lock`

### Component Manager And Dependencies

ESP-IDF component dependencies are described in `idf_component.yml`, which must live in the root directory of a component. The manifest supports dependency declarations and ESP Component Registry dependencies.

The p4kvm reference uses `main/idf_component.yml` with:

```yaml
dependencies:
  espressif/esp_tinyusb:
    version: "^2.0.0"
  espressif/mdns:
    version: "^1.8.0"
```

Current registry versions visible in this research pass are:

- `espressif/esp_tinyusb` `v2.2.1`
- `espressif/mdns` `v1.11.3`

The USB Device Stack docs say the ESP32-P4 device stack is built around TinyUSB and distributed as a managed component through the ESP Component Registry. The USB docs list HID, CDC, MIDI, MSC, composite devices, vendor classes, and endpoint limits. The mDNS registry page describes mDNS as a multicast UDP service for local network service and host discovery.

Sources:

- [IDF Component Manager manifest docs](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html)
- [ESP32-P4 USB Device Stack docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/usb_device.html)
- [espressif/esp_tinyusb registry page](https://components.espressif.com/components/espressif/esp_tinyusb)
- [espressif/mdns registry page](https://components.espressif.com/component/espressif/mdns)
- `C:\code\p4kvm-reference\main\idf_component.yml`

### Camera, JPEG, Ethernet, And HTTP Built-Ins

ESP32-P4's camera controller docs list MIPI CSI, ISP DVP, and LCD_CAM DVP as external camera input hardware, and show `esp_driver_cam` as the component dependency for camera controller APIs. The docs also call out CSI setup, start/stop, receive, callback, ISR/cache-safety, and MIPI CSI power requirements.

ESP-IDF's JPEG docs cover the ESP32-P4 JPEG encoder engine and `jpeg_encoder_process()`. The ESP-IDF JPEG encode example demonstrates a 1280x720 raw image encoded with the JPEG hardware encoder and supports ESP32-P4.

ESP-IDF's Ethernet docs describe ESP-IDF Ethernet APIs for internal EMAC and external SPI Ethernet, and show the ESP-NETIF/LwIP path for getting IP networking. The HTTP server docs say WebSocket server support is available through `CONFIG_HTTPD_WS_SUPPORT`, with optional pre-handshake and post-handshake callbacks.

Sources:

- [ESP32-P4 Camera Controller Driver docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/camera_driver.html)
- [ESP32-P4 JPEG Encoder and Decoder docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/jpeg.html)
- [ESP-IDF JPEG encode example](https://github.com/espressif/esp-idf/blob/master/examples/peripherals/jpeg/jpeg_encode/README.md)
- [ESP32-P4 Ethernet docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/network/esp_eth.html)
- [ESP32-P4 HTTP Server docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/protocols/esp_http_server.html)

### `sdkconfig.defaults`

ESP-IDF uses Kconfig for project configuration. Espressif documents `sdkconfig.defaults` as a way to set project default values, with optional target-specific files such as `sdkconfig.defaults.esp32p4`; a base `sdkconfig.defaults` file must exist for target-specific defaults to be considered.

The p4kvm reference uses `sdkconfig.defaults` to set practical WatchPup-relevant values:

- larger lwIP socket and TCP buffers,
- `CONFIG_HTTPD_WS_SUPPORT=y`,
- larger HTTP request headers,
- TinyUSB HID count,
- PSRAM enablement and speed,
- JPEG quality default.

Sources:

- [ESP-IDF configuration file structure docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/kconfig/configuration_structure.html)
- `C:\code\p4kvm-reference\sdkconfig.defaults`

## Recommended Firmware Layout

Use this layout for the first skeleton:

```text
firmware/
  CMakeLists.txt
  sdkconfig.defaults
  sdkconfig.defaults.esp32p4
  main/
    CMakeLists.txt
    idf_component.yml
    app_main.c
  components/
    board_config/
    diagnostics/
    bridge_tc358743/
    capture/
    encoder_mjpeg/
    frame_store/
    stream_server/
    hid_device/
    control_protocol/
```

For the very first build ticket, only `main`, `board_config`, and `diagnostics` need real source. The remaining component directories can be created by later scoped tickets when there is a concrete API to define.

Keep `main` as the composition root because ESP-IDF gives the `main` component special treatment. The build-system docs say the `main` component is automatically added to the build and automatically requires all other components, which is helpful for a small skeleton. Local subsystem components should still declare their own dependencies explicitly in their `CMakeLists.txt`.

Source:

- [ESP-IDF build system docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html)

## Recommended `CMakeLists.txt` Shape

Top-level `firmware/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(watchpup_kvm)
```

Initial `firmware/main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_eth
        esp_http_server
        mdns
)
```

Add these `REQUIRES` only when the corresponding subsystem ticket lands:

- `esp_driver_gpio`
- `esp_driver_i2c`
- `esp_driver_cam`
- `esp_driver_isp`
- `esp_driver_jpeg`
- `esp_hw_support`
- `esp_psram`
- `esp_mm`
- `esp_timer`
- `espressif__esp_tinyusb`

The p4kvm reference used these built-in dependencies successfully, but WatchPup should add them incrementally so each subsystem ticket has a visible dependency change.

## Recommended `idf_component.yml`

Use the component manager from the start:

```yaml
dependencies:
  espressif/esp_tinyusb:
    version: "^2.2.1"
  espressif/mdns:
    version: "^1.11.3"
```

If first hardware bring-up hits a regression versus the p4kvm reference, temporarily test the reference-compatible range:

```yaml
dependencies:
  espressif/esp_tinyusb:
    version: "^2.0.0"
  espressif/mdns:
    version: "^1.8.0"
```

Commit the resulting `dependencies.lock` after the first successful build, and update it intentionally through dependency-update tickets.

## Recommended `sdkconfig` Defaults

Start with a small base `firmware/sdkconfig.defaults`:

```ini
# WebSocket support for browser/API control later.
CONFIG_HTTPD_WS_SUPPORT=y

# Keep request headers roomy enough for browser clients and auth experiments.
CONFIG_HTTPD_MAX_REQ_HDR_LEN=8192

# Ethernet/MJPEG headroom; inherited from p4kvm's practical defaults.
CONFIG_LWIP_MAX_SOCKETS=24
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=65535
CONFIG_LWIP_TCP_WND_DEFAULT=65535
CONFIG_LWIP_TCPIP_TASK_STACK_SIZE=6144
```

Use target-specific `firmware/sdkconfig.defaults.esp32p4` for ESP32-P4 assumptions:

```ini
# ESP32-P4 PSRAM required for KVM capture/encode buffering.
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_HEX=y
CONFIG_SPIRAM_SPEED_200M=y

# Composite HID keyboard/mouse once TinyUSB lands.
CONFIG_TINYUSB_HID_COUNT=1
```

Do not include production security settings such as secure boot or flash encryption in the first skeleton defaults. Those should wait for the update/key-custody design.

Do not include H.264-related settings in the first skeleton. H.264 remains post-v1/prototype work.

## First Skeleton Acceptance Criteria

The first implementation ticket generated from this research should be small:

- `firmware/` ESP-IDF project exists.
- Target is set to ESP32-P4 in build instructions.
- Build boots to serial logs.
- `app_main` initializes NVS, event loop, network stack placeholders, and diagnostics scaffolding only.
- No HDMI capture, JPEG streaming, HID control, or hardware-control behavior yet.
- `idf_component.yml`, `sdkconfig.defaults`, and `sdkconfig.defaults.esp32p4` are present.
- `dependencies.lock` is committed after the first successful build.
- README or firmware README documents the exact ESP-IDF version used.

## Decision Unlocked

This research supports an implementation ticket for a minimal ESP-IDF firmware skeleton using ESP-IDF `v6.0.2`, ESP32-P4 target, component-managed TinyUSB and mDNS dependencies, committed config defaults, and a `firmware/` layout that can grow into the subsystem boundaries already decided in the migration map.

