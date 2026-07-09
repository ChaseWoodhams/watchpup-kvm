# ESP32-P4 Video And Security Realism

Issue: [Research ESP32-P4 video and security realism](https://github.com/ChaseWoodhams/watchpup-kvm/issues/15)

Date: 2026-07-09

## Question

What can ESP32-P4 realistically support for WatchPup KVM's v1 and near-future roadmap: MJPEG 720p/1080p, H.264 encode claims and current library maturity, USB OTG HID, Ethernet, secure boot, flash encryption, PSRAM constraints, and known silicon/revision caveats?

## Short Answer

ESP32-P4 is a plausible hardware basis for WatchPup KVM, but v1 should be conservative: prove reliable HDMI-to-CSI capture, diagnostics, USB HID, Ethernet, and MJPEG first. Treat H.264 as a post-v1 or stretch track until WatchPup has its own capture-to-encode-to-stream prototype on the exact target silicon and board.

The chip has the right blocks on paper: MIPI-CSI, JPEG codec, H.264 encoder, USB OTG 2.0 HS, Ethernet, PSRAM-in-package variants, secure boot, flash encryption, cryptographic accelerators, and key-management hardware. The risk is integration maturity and end-to-end behavior, not whether the block diagram contains the features.

## Source-Backed Facts

### ESP32-P4 Hardware Fit

Espressif describes ESP32-P4 as a high-performance MCU with a dual-core RISC-V CPU up to 400 MHz, 768 KB on-chip SRAM, external PSRAM support, MIPI-CSI/DSI, hardware media accelerators, USB OTG 2.0 HS, Ethernet, and SDIO Host 3.0.

The official product page says ESP32-P4 can handle up to 1080p camera/display use cases and includes hardware accelerators such as H.264, PPA, and 2D-DMA. The datasheet says the package variants include 16 MB or 32 MB PSRAM.

Sources:

- [ESP32-P4 product page](https://www.espressif.com/en/products/socs/esp32-p4)
- [ESP32-P4 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_en.pdf)

### CSI Capture

The ESP32-P4 datasheet lists one MIPI CSI interface compliant with MIPI CSI-2 and D-PHY v1.1, with 2 lanes at 1.5 Gbps, and input formats including RGB888, RGB666, RGB565, YUV422, YUV420, RAW8, RAW10, and RAW12.

The p4kvm reference proves a useful but narrow path: TC358743 HDMI-to-MIPI CSI into ESP32-P4, currently fixed around RGB888 capture and JPEG/MJPEG streaming. Its README says the CSI path is fixed to RGB888 for now because YUV does not work well with the JPEG encoder in that project.

Sources:

- [ESP32-P4 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_en.pdf)
- [p4kvm reference README](C:/code/p4kvm-reference/README.md)
- [p4kvm reference analysis](p4kvm-reference-analysis.md)

### JPEG And MJPEG

ESP-IDF documents an ESP32-P4 JPEG codec based on JPEG baseline. The driver can encode raw image buffers to JPEG and decode JPEG to raw buffers, but the hardware engine works as either encoder or decoder at a given time.

ESP-IDF also ships a JPEG encode example for ESP32-P4 that encodes a 1280x720 BGR24 frame with the hardware encoder. That makes 720p MJPEG a realistic first product milestone, assuming the capture, buffer ownership, network streaming, and quality/FPS tradeoffs are engineered cleanly.

1080p MJPEG is plausible but should be validated as a detail/diagnostic mode rather than assumed as the default v1 experience. At 1920x1080 RGB888, one raw frame is about 6.2 MB before JPEG compression, so PSRAM, DMA, encode time, and MJPEG client backpressure become first-order design constraints.

Sources:

- [ESP-IDF JPEG Encoder and Decoder docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/jpeg.html)
- [ESP-IDF JPEG encode example](https://github.com/espressif/esp-idf/blob/master/examples/peripherals/jpeg/jpeg_encode/README.md)
- [p4kvm reference analysis](p4kvm-reference-analysis.md)

### H.264

The ESP32-P4 datasheet says the chip contains a baseline H.264 encoder, with progressive input formats including RGB888, RGB565, YUV444, YUV422, YUV420, and GRAY. The listed maximum encoding performance is 1080p@30fps for YUV420.

Espressif's `esp_h264` component is current and targets ESP32-P4. The component registry says version 1.3.6 was uploaded one month before this research pass and supports ESP32-P4. Its README says the hardware encoder is designed for ESP32-P4 and reports 1920x1080 hardware encoder performance at 30 fps for `ESP_H264_RAW_FMT_O_UYY_E_VYY`, with about 140 KB encoder memory in that test.

This is encouraging, but it does not mean WatchPup should promise H.264 in v1. The hard part for WatchPup is the complete path: TC358743 output format, CSI capture format, any required color conversion, frame ownership, encoder input constraints, network transport, browser playback, and recovery behavior. The p4kvm reference README explicitly says H.264 is not included in that firmware and suggests chip revisions matter.

Sources:

- [ESP32-P4 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_en.pdf)
- [ESP_H264 component registry](https://components.espressif.com/components/espressif/esp_h264/)
- [esp-h264-component GitHub repository](https://github.com/espressif/esp-h264-component)
- [p4kvm reference README](C:/code/p4kvm-reference/README.md)

### USB HID

ESP-IDF's ESP32-P4 USB Device Stack enables USB device support for functions such as keyboard and mouse, custom vendor classes, and composite devices. It is built around TinyUSB and distributed through the ESP Component Registry. The documented supported classes include HID, CDC, MIDI, and MSC, and the component page says `esp_tinyusb` supports ESP32-P4.

This supports WatchPup's keyboard/mouse HID direction for v1. Absolute tablet mode is likely a descriptor/report-design task rather than a missing hardware feature, but it should still be validated with BIOS/UEFI targets because compatibility is where KVM input gets sharp edges.

Sources:

- [ESP-IDF USB Device Stack docs for ESP32-P4](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/usb_device.html)
- [esp_tinyusb component registry](https://components.espressif.com/components/espressif/esp_tinyusb)
- [p4kvm reference analysis](p4kvm-reference-analysis.md)

### Ethernet And Wi-Fi

ESP32-P4 includes Ethernet support, but not native Wi-Fi. Espressif's product page frames wireless connectivity through a companion ESP32-C/S-series chip via SPI, SDIO, or UART using ESP-Hosted or ESP-AT.

That supports an Ethernet-first v1. Wi-Fi should remain optional/later unless the hardware architecture explicitly includes a companion chip and the security/update model covers two devices.

Source:

- [ESP32-P4 product page](https://www.espressif.com/en/products/socs/esp32-p4)

### Security Features

ESP32-P4 supports secure boot and flash encryption in ESP-IDF. Secure Boot v2 verifies the second-stage bootloader and application images before execution, and ESP32-P4 supports RSA-PSS or ECDSA schemes. ESP-IDF currently warns that ECDSA Secure Boot v2 on ESP32-P4 is not recommended for certain input vectors and recommends the RSA-based scheme until a future hardware ECO fixes the issue.

Flash encryption encrypts off-chip flash contents so physical flash readout is not enough to recover most flash data. ESP-IDF warns that production use should use release mode and that enabling flash encryption limits future update options, so WatchPup's update design must be decided before promising this as a default.

ESP32-P4 also advertises cryptographic accelerators, TRNG, a Digital Signature peripheral, and a Key Management Unit. These are valuable for a transparent trust story, but they do not replace application security: auth, session management, update signing, debug-port policy, and safe defaults remain WatchPup design work.

Sources:

- [ESP32-P4 product page](https://www.espressif.com/en/products/socs/esp32-p4)
- [ESP-IDF Secure Boot v2 docs for ESP32-P4](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/security/secure-boot-v2.html)
- [ESP-IDF Flash Encryption docs for ESP32-P4](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/security/flash-encryption.html)

## Realistic WatchPup V1 Boundary

Recommended v1 assumptions:

- Ethernet-first.
- MJPEG first, with 720p as the reliability target and 1080p as a mode to validate rather than a promise.
- USB HID keyboard and mouse first; absolute tablet and BIOS-relative behavior should be tested as explicit compatibility cases.
- Diagnostics before streaming polish: TC358743 status, CSI status, frame counters, JPEG encode timing, stream client count, reset/recovery state, and firmware/security state.
- Security posture can plan around ESP32-P4 secure boot and flash encryption, but production defaults should wait until the update path and key custody story are written down.
- H.264 belongs after the MJPEG path is stable and measured on WatchPup hardware.

## Near-Future Roadmap Implications

H.264 is promising enough to keep on the roadmap. The chip and official component support it, and the official performance claims are relevant to KVM streaming. But WatchPup should create a later prototype ticket for the full H.264 path rather than letting the datasheet claim become a v1 feature promise.

USB MSC virtual media is plausible from a USB-stack perspective because TinyUSB/ESP-IDF support MSC, but product-quality virtual media requires storage, host compatibility, upload UX, and safety decisions. It should remain post-v1 unless explicitly reprioritized.

Wi-Fi is not a simple firmware toggle on ESP32-P4. It implies a companion chip/module and more attack surface. Keep it post-v1.

## Decision Unlocked

This research supports treating WatchPup KVM v1 as a reliable, transparent MJPEG-over-Ethernet and USB-HID KVM, not an H.264-first competitor to JetKVM or Linux-SoC products.

It also supports unblocking:

- [Research the DIY cost and BOM target](https://github.com/ChaseWoodhams/watchpup-kvm/issues/17): use an Ethernet-first ESP32-P4 + HDMI-to-CSI + USB HID baseline, with Wi-Fi companion and H.264 polish outside the base BOM until proven.
- [Decide the security posture and trust model](https://github.com/ChaseWoodhams/watchpup-kvm/issues/18): use ESP32-P4 secure boot and flash encryption as available primitives, but decide update signing/key custody before making them a v1 promise.
- [Decide the v1 feature boundary](https://github.com/ChaseWoodhams/watchpup-kvm/issues/16): keep H.264, Wi-Fi, and virtual media outside the minimal v1 unless later tickets intentionally pull them in.

