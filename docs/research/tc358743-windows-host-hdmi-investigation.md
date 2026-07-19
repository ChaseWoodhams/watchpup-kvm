# TC358743 Windows-host HDMI investigation

Date: 2026-07-13

## Scope and setup

This note investigates the exact bench path: a Waveshare ESP32-P4-ETH board, its
2-lane MIPI-CSI camera connector, a Waveshare HDMI-to-CSI adapter using
TC358743XBG, and HDMI from a Windows host with an NVIDIA GPU. It covers the
current live evidence, including the tested firmware changes that clear
TC358743 sleep mode and delay HPD after EDID load.

The Waveshare adapter is specified as an HDMI 1.4, RGB888 input up to 1080p30
and a 2-lane MIPI-CSI-2 output. The current Waveshare procedure uses an EDID
file, then captures with a TC358743-aware V4L2 path. Its newer Raspberry Pi 5
instructions explicitly use 1280x720p60 or 1080p30, and say that the HDMI source
must support the selected resolution. [Waveshare HDMI-to-CSI Adapter wiki](https://www.waveshare.com/wiki/HDMI_to_CSI_Adapter#software-settings),
[Waveshare Pi 5 capture procedure](https://www.waveshare.com/wiki/HDMI_to_CSI_Adapter#capture-video-with-raspberry-pi-5),
[ESP32-P4-ETH hardware description](https://docs.waveshare.com/ESP32-P4-ETH#hardware-description).

## Bottom line

The current failure is upstream of CSI capture. `SYS_STATUS=0x0D` is not a
successful HDMI lock: the Linux register header defines bit 0 as DDC5V and bit
2 as the HDMI PHY PLL, bit 3 as PHY SCDT, while TMDS, HDMI, and sync are
separate bits. Thus the bridge is powered/configurable and its PHY has detected
clock activity, but it is not seeing a valid decoded HDMI data stream.
[upstream TC358743 register definitions](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743_regs.h#L390-L398)

The firmware now clears the TC358743 sleep bit and waits 150 ms after EDID
loading before asserting HPD, matching the upstream driver's sequencing. The
host now reports `Display Attached: Yes`, `Display Active: Enabled`, and a
physical `Generic Monitor` target, but the bridge remains at `0x0D`. A direct
NVIDIA test also changed that physical target from 1024x768 to 800x600 and
back without producing TMDS or sync lock. HPD/EDID was therefore a real issue,
but it is no longer sufficient to explain the remaining failure. The leading
remaining possibilities are the HDMI/DVI physical path or a remaining PHY/
source-path initialization mismatch. CSI is still downstream and not yet
actionable.

`nvidia-smi`'s `Display Attached: Yes` should not be treated as proof that the
TC358743 is receiving pixels. NVIDIA documents that field as whether a
physical display is attached to any GPU connector; it separately defines
`Display Active` as whether a display is initialized, and says a display can be
active even with no physical monitor attached. Query both fields and correlate
them with Windows display topology and EDID. [NVIDIA `nvidia-smi` field definitions](https://docs.nvidia.com/deploy/nvidia-smi/index.html#display-attached).

Apollo/Sunshine can further confuse this correlation. Apollo's own first-party
documentation says its SudoVDA virtual display is created when a stream starts,
removed when the app quits, and behaves like a Windows PnP monitor. That virtual
target can supply Windows modes for streaming without putting TMDS transitions
on the specific physical NVIDIA HDMI connector wired to the TC358743. [Apollo virtual-display documentation](https://github.com/ClassicOldSong/Apollo#about-virtual-display).

## Live evidence mapped to the signal path

| Observation | What it proves | What it does not prove |
| --- | --- | --- |
| I2C probe succeeds at `0x0f` | ESP32 control bus, addressing, and some bridge power/reset path are alive | Correct TC358743 initialization, HDMI DDC access, HDMI lock, or CSI output |
| EDID length is 256 and EDID mode is configured | The firmware has selected a 256-byte EDID and an EDID/DDC operating mode | That the EDID RAM contents are valid, checksummed, exposed while HPD is high, or actually read by Windows |
| `HPD=1` | The chip's HPD output register reads high | That the host sampled a new HPD edge or that DDC is enabled at that moment |
| `ANA_CTL=0x31` | Per the upstream masks, analog is on (`0x01`) and PCS is in normal mode (`0x30`) | Valid TMDS data; this is analog front-end configuration, not signal-detect status |
| `PHY_EN=1` | The HDMI PHY enable bit is set | TMDS is present or decoded |
| PLL locked | The PHY PLL lock bit is set | TMDS validity, SCDT, HDMI mode detection, or sync |
| `SYS_STATUS=0x0D` | `0x01` DDC5V + `0x04` PHY PLL + `0x08` PHY SCDT | `0x02` TMDS, `0x10` HDMI, and `0x80` sync are still clear |
| CSI output disabled | No CSI pixels should be expected yet | Whether the HDMI input could be locked if CSI were enabled |
| Windows has Generic Default_Monitor, no EDID blob, and 1024x768/800x600 fallback | Windows did not obtain a normal monitor descriptor/mode list for the target being inspected | Which physical connector, virtual display, or stale target produced the visible mode |

Windows' monitor class driver obtains monitor identification from EDID and the
display adapter's PDO reads it over DDC/I2C. Windows also documents that the
target monitor name is derived from EDID and can be absent when the target is
forced without a monitor. [Microsoft monitor class function driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/monitor-class-function-driver),
[Microsoft target-device name and EDID](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_target_device_name).

## What the upstream driver says the firmware must do

The upstream Linux driver is useful as a behavioral reference even though this
project is not using Linux. Its `tc358743_initial_setup()` sequence:

1. Resets the relevant blocks and exits sleep mode.
2. Programs FIFO level and reference-clock parameters.
3. Sets the DDC 5V delay and selects E-DDC mode.
4. Configures the HDMI PHY, HDCP behavior, audio, infoframe handling, and video
   color mode.

See [`tc358743_initial_setup()`](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L937-L974).
The driver requires a supported reference clock of 26, 27, or 42 MHz and
calculates HDMI/CSI timing values from it; the board-specific oscillator and
the firmware's assumed reference-clock value therefore need to be verified,
not guessed. See [`tc358743_probe_of()`](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L2007-L2101).

### HPD, EDID, and DDC timing

The upstream sequence is especially relevant to the Windows symptom:

- Disabling HPD also disables DDC access to the EDID.
- If no EDID blocks have been written, the driver deliberately does not assert
  HPD.
- After EDID is written, it schedules HPD assertion 143 ms later; the driver
  comments that DDC access is enabled when HPD is enabled.
- The reference device-tree path selects a 100 ms DDC 5V delay. The register
  header exposes 0, 50, 100, and 200 ms choices.

See [`tc358743_enable_edid()` and `tc358743_disable_edid()`](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L429-L460),
[`initial_setup()` DDC configuration](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L957-L963),
and the [`DDC_CTL`, `HPD_CTL`, and `EDID_MODE` definitions](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743_regs.h#L460-L479).

Therefore, “EDID length = 256, EDID mode = E-DDC, HPD = 1” is not sufficient.
The host must be allowed to read the EDID while DDC is enabled, and it must see
an HPD transition after the final EDID bytes are valid. A firmware implementation
that raises HPD before writing/verifying EDID, never pulses HPD after a host is
already running, or leaves DDC gated can produce exactly the Windows fallback
behavior while the bridge still answers I2C.

The upstream driver writes the EDID length, copies each 128-byte block into the
TC358743 EDID RAM, records the block count, and only then enables EDID/HPD when
5V is present. See [`tc358743_s_edid()`](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L1879-L1928).
The 256-byte live length should consequently be checked as two complete blocks:
valid base-block header, valid extension-count byte, and a correct checksum for
each block. A length register alone does not validate the RAM.

### HDMI lock versus CSI enable

The upstream status helpers treat missing TMDS as “no signal” and missing sync
as “no sync”; detected timings are rejected if HPD is low or signal is absent.
See [`is_hdmi()`, `no_signal()`, `no_sync()`, and timing detection](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L286-L360).

Once HDMI lock exists, CSI still requires a separate configuration. The driver
calculates the number of lanes from timing, resets the CSI block, programs D-PHY
timing counters, starts the CSI transmitter, selects CSI-2 mode and lane count,
and enables CSI interrupts. See [`tc358743_set_csi()`](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L803-L865).

On stream enable it intentionally creates an LP11-to-HS transition, unmutes
video, and enables `VBUFEN`/`ABUFEN`; on disable it mutes video and leaves the
lanes in LP11. See [`enable_stream()`](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c#L640-L666).
This is the likely downstream firmware work if TMDS becomes good but the ESP32
camera receiver still sees no frames. It cannot be the cause of the current
`SYS_STATUS` failure because CSI configuration does not create HDMI TMDS input.

## Host modes to use first

Use a deliberately boring, known-good source mode before trying desktop defaults:

1. Program a valid 256-byte 1280x720@60 EDID and select 1280x720 progressive,
   RGB, 8-bit, SDR, with HDR/VRR/G-SYNC disabled for the test.
2. After that path locks, test 1920x1080@30. This is the adapter's declared
   upper video mode and is the mode selected by the Waveshare package's
   `set_edid.sh` (`1080p30edid`). The official package also contains
   `720p60edid` and `1080p30edid`. [Waveshare driver archive](https://files.waveshare.com/wiki/HDMI_to_CSI_Adapter/Hdmi_to_csi_driver.zip),
   [Waveshare `set_edid.sh` and mode files as distributed](https://www.waveshare.com/wiki/HDMI_to_CSI_Adapter#resources).
3. Do not use 1080p60 as the first acceptance mode. Although a file with that
   name is present in the download, the product page's declared input limit is
   1080p30 and the current capture procedure names 720p60 and 1080p30.
4. Do not use the Windows 1024x768/800x600 fallback as evidence that the
   adapter is compatible with a mode. First repair EDID/HPD, then select one of
   the explicitly supported modes.

The adapter's FAQ says it has no HDCP. For the first test, use an unprotected
desktop signal and disable content protection features where the NVIDIA driver
allows it. [Waveshare HDCP FAQ](https://www.waveshare.com/wiki/HDMI_to_CSI_Adapter#faq).

## Ranked hypotheses

| Rank | Hypothesis | Confidence | Evidence and falsifier |
| --- | --- | --- | --- |
| 1 | A remaining HPD/EDID/DDC or source-path initialization mismatch prevents a valid TMDS decode. | Medium | HPD/EDID timing was corrected and Windows now reports an attached physical target, but TMDS/HDMI/sync remain clear. Falsified when a complete reference initialization and a clean HPD/EDID cycle produce stable lock. |
| 2 | Firmware is missing part of the TC358743 initialization: reset release, reference-clock setup, DDC delay, PHY defaults, HDMI detection, or required status/interrupt setup. | High | I2C reachability plus `PHY_EN`/PLL lock shows partial initialization, while the upstream sequence is substantially more than EDID writes. Falsified when a register trace matches the required initialization and `TMDS/SCDT/HDMI/SYNC` assert. |
| 3 | The Windows target being inspected is Apollo/SudoVDA or another virtual/stale display, not the physical HDMI target wired to the adapter; or `nvidia-smi` is referring to another physical connector. | Medium | Apollo creates a Windows PnP-like virtual monitor only during a stream and Windows distinguishes `INDIRECT_VIRTUAL` from HDMI. Falsified by Apollo/Sunshine fully stopped, no virtual monitor present, and a Windows topology record that maps the tested target to the NVIDIA HDMI connector. |
| 4 | The NVIDIA source mode is outside the adapter's practical envelope or has a feature the bridge does not support: 1080p60, HDR, VRR, HDCP, unusual pixel encoding, or a timing not in the loaded EDID. | Medium | Product documentation declares HDMI 1.4 RGB888 up to 1080p30 and no HDCP. Falsified by stable lock at 720p60 with a matching EDID and a clean host configuration. |
| 5 | HDMI-side physical fault: wrong cable/port, HPD/DDC wiring, connector, power, or differential-pair integrity. | High | I2C, DDC5V, and host attachment do not validate the three TMDS pairs. Falsified by the same bridge locking from a known-good HDMI source and cable. |
| 6 | CSI ribbon orientation/lane mapping or missing CSI transmitter setup is preventing capture. | Low for the current symptom; high only after HDMI lock | It can explain no frames after `TMDS/SCDT/HDMI/SYNC` become good, but cannot explain those HDMI status bits remaining clear. Falsified by a CSI receiver trace showing valid LP11-to-HS and changing frame counters. |

## Concrete next tests

### Test 1 — Separate physical HDMI from virtual display state

Run this with Apollo and Sunshine stopped, and with any SudoVDA/other virtual
display disabled or removed for the test. Reboot if necessary so Windows starts
with one known physical NVIDIA output. Record:

```powershell
nvidia-smi -q -d DISPLAY
Get-PnpDevice -Class Monitor | Format-Table Status, FriendlyName, InstanceId -Auto
Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorID |
  Select-Object Active, InstanceName, ManufacturerName, UserFriendlyName
Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorRawEEdidV1Block |
  Select-Object Active, InstanceName, BlockType, Content
```

`WmiMonitorID` is specifically defined from the VESA E-EDID identification
block. For a definitive connector classification, use a small
`QueryDisplayConfig`/`DisplayConfigGetDeviceInfo` probe and record the target's
friendly name, monitor device path, and `DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY`:
`HDMI` is value 5; `INDIRECT_VIRTUAL` is value 17. [Microsoft `WmiMonitorID`](https://learn.microsoft.com/en-us/windows/win32/wmicoreprov/wmimonitorid),
[Microsoft `DisplayConfigGetDeviceInfo`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-displayconfiggetdeviceinfo),
[Microsoft output-technology enum](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ne-wingdi-displayconfig_video_output_technology).

Pass condition: the tested target is an HDMI target, the EDID block is present,
and the monitor identity is not merely a SudoVDA/Apollo target. If only an
`INDIRECT_VIRTUAL` target exists, it cannot validate this HDMI cable path.

### Test 2 — Prove the EDID transaction, not just the bridge register state

With the host powered and the bridge under I2C observation:

1. Hold HPD low or reset the bridge.
2. Write both EDID blocks; read the RAM back and validate header, extension
   count, and both checksums.
3. Program the 256-byte length and E-DDC mode.
4. Ensure the bridge's DDC access is enabled, wait at least the configured
   DDC 5V delay, then assert HPD only after EDID is valid. Use the upstream
   sequence's 100 ms DDC delay and 143 ms post-EDID HPD delay as the reference
   timing, not an immediate HPD write.
5. Capture the host's EDID read and Windows monitor re-enumeration. If the host
   never reads the EDID, inspect HPD electrical level/edge, DDC pull-ups,
   SDA/SCL continuity, voltage domain, and whether the host is connected to the
   intended NVIDIA connector.

Pass condition: the host reads the exact two blocks, Windows exposes the same
EDID through WMI/DisplayConfig, and the bridge begins reporting TMDS/SCDT/HDMI.

### Test 3 — Force the supported source mode

After Test 2 passes, load the Waveshare 720p60 EDID and set the physical NVIDIA
HDMI output to 1280x720@60, RGB 8-bit SDR. Disable HDR, VRR/G-SYNC, and HDCP for
this isolation test. Re-read the bridge status repeatedly during mode changes,
then repeat with 1920x1080@30.

Interpretation:

- `TMDS=0`: the source is not producing a usable HDMI stream on this connector,
  or the bridge's HDMI PHY/source configuration is wrong. CSI work is premature.
- `TMDS=1` but `SCDT=0`: the PHY sees something but cannot recover valid data;
  investigate mode, cable, signal integrity, PHY defaults, and clock/reference
  configuration.
- `TMDS=1`, `SCDT=1`, `HDMI=1`, `SYNC=1`: HDMI bring-up has passed; proceed to
  CSI configuration and receiver tracing.

Current result: the NVIDIA Control Panel exposed only fallback 1024x768 and
800x600 modes and labeled the target `DVI - PC display`. Applying 800x600 did
not change `SYS_STATUS=0x0D`, so the fallback mode itself is not the sole cause.
The temporary signed EDID override used for this test was removed afterward;
the host is back to its normal `Generic Monitor` identity.

### Test 5 — Raw source-status telemetry

The firmware was extended with read-only status fields from the TC358743's
source-side registers. With the HDMI cable connected, the live result was:

```text
SYS_STATUS=0x0D
SYS_INT=0x03
CLK_INT=0xF7
HDMI_INT0=0x00
HDMI_INT1=0x00
VI_STATUS1=0x00
VI_STATUS3=0x00
INIT_END=0x00
```

Using the upstream register definitions, `SYS_STATUS=0x0D` means DDC 5V,
PHY PLL lock, and PHY SCDT, while TMDS-valid, HDMI-mode, and stable-sync bits
remain clear. The latched `SYS_INT`/`CLK_INT` changes show that the source path
has toggled, but they do not establish a valid decoded video stream. Windows
also reports the target as `DISPLAY\\DEFAULT_MONITOR` / Generic Non-PnP Monitor,
not as a monitor with a readable EDID. This keeps the failure boundary before
CSI: the ESP32-P4 cannot receive valid frames until the TC358743 reports
TMDS/HDMI/sync lock.

The official Waveshare board documentation confirms that the corrected ribbon
location is the two-lane MIPI-CSI camera connector, and the adapter documentation
lists 720p60 and 1080p30 as the first supported HDMI modes. The firmware was
then switched to the exact 256-byte `720p60edid` shipped in the official
Waveshare driver archive; the host still enumerated the target as
`DISPLAY\\DEFAULT_MONITOR` and the bridge stayed at `SYS_STATUS=0x0D`. This
single-variable EDID test did not move the failure boundary.

The latest firmware test found an ESP32/TC358743 EDID-RAM write-framing issue:
the bridge only exposed a usable descriptor to the host when each EDID byte was
written as its own register transaction. With that compatibility path, Windows
now enumerates the physical target as `DISPLAY\\TSB8888...UID45315`, friendly name
`DCDZ-H2C MOD`, and the registry contains a 256-byte EDID whose bytes match the
firmware's Waveshare 720p60 descriptor exactly. The NVIDIA output is active at
1280x720@60. The bridge-side bulk readback still reports
`edid_readback_valid=false`, so that field is not a reliable acceptance test for
this EDID window on the ESP32 I2C controller; the host's exact registry copy is
the stronger end-to-end result.

Despite the corrected EDID and mode, the bridge remains at `SYS_STATUS=0x0D`:
PLL and PHY SCDT are set, but TMDS-valid, HDMI-mode, and stable-sync remain
clear. The one-shot PHY reset, optional PHY auto-reset bits, and upstream-style
latched `SYS_INT`/`CLK_INT` clearing did not change that state. The immediate
failure boundary is therefore now narrowed to HDMI TMDS decode or the physical
source/electrical path, not EDID, Apollo/SudoVDA identity, Windows mode
selection, or CSI capture.

Apollo/Sunshine isolation remains useful for attribution, but it is no longer
the ambiguity in this run: Windows exposes both the Apollo `SMKD1CE` virtual
monitor and the physical `TSB8888` HDMI target, and the EDID/mode verification
above is tied to the latter. The virtual display is therefore not evidence that
the HDMI-to-CSI input is receiving pixels.

The next decisive test is therefore a host power-cycle/BIOS test with the
adapter's HDMI input path isolated from Apollo/SudoVDA, followed by a direct
EDID/HPD check. No CSI receiver implementation should be treated as validated
while this source-side status remains red.

### Test 009 - Reference reset, HDCP, and source timing retests

The firmware was retested with three remaining software-side variables from
the Linux TC358743 driver: HDCP disabled for the adapter, the driver's exact
context/HDMI reset sequence while leaving IR/CEC reset asserted, and the
adapter's documented 1920x1080@30 source timing. The image built and flashed
successfully. Both 1280x720@60 and 1920x1080@30 were accepted by the physical
NVIDIA HDMI target, but the bridge remained at `SYS_STATUS=0x0D` with
`pll_locked=true`, `hdmi_signal_detected=false`, `hdmi_sync_locked=false`, and
`frame_count=0`.

This closes the remaining bounded firmware/mode checks available from Windows.
The failure is still before CSI: the bridge has no decoded TMDS stream to
forward, so the ESP32-P4 receiver correctly remains unstarted.

### Test 010 - Automatic PHY reset experiment

As a final firmware-side experiment, all three optional TC358743 automatic
PHY reset triggers (`PHY_CTL2=0x07`) were enabled. Three live samples still
reported `SYS_STATUS=0x0D`, with the PHY PLL and SCDT bits set but TMDS and
sync clear; the recovery counter increased without producing a lock. The
firmware was then restored and flashed with the upstream default
`PHY_CTL2=0x00`.

The final live image reports `probe_ok=true`, `hpd_asserted=true`,
`edid_loaded=true`, `pll_locked=true`, `hdmi_signal_detected=false`,
`hdmi_sync_locked=false`, and `frame_count=0`.

### Test 011 - Exact 27 MHz readback and extended HPD settling

The firmware now reads back every reference-clock register after source-path
configuration, after the explicit post-HPD PHY reset, and during each `/diag`
poll. The post-boot live readback exactly matches the proposed 27 MHz values:

| Register group | Readback |
| --- | --- |
| `SYS_FREQ1:0` | `0x0A8C` |
| `FH_MIN1:0` | `0x010E` |
| `FH_MAX1:0` | `0x06F6` |
| `LOCKDET_REF2:0` | `0x041EB0` |
| `NCO_F0_MOD` | `0x01` |

The new sequence holds HPD low through configuration and EDID loading, waits
500 ms before asserting HPD, waits 1 s for source negotiation, applies the
PHY reset, then waits another 1 s. The live result is still
`SYS_STATUS=0x0D`: the bridge reports DDC 5V, PHY PLL lock, and SCDT, but not
TMDS-valid or sync. `EDID_MODE=0x02`, `PHY_CTL1=0x80`, `PHY_CTL2=0x00`,
`refclk_readback_valid=true`, and `frame_count=0`.

This falsifies an incorrect 27 MHz register write and a short HPD settling
window as the current blocker. The remaining boundary is the HDMI receiver's
TMDS decode/electrical path: source output state, cable/connector, adapter
power or signal integrity, or the adapter's HDMI receiver hardware. CSI is
still not testable because no HDMI frames reach the bridge.

### Test 012 - Physical Windows target mode transition

The host-side loop forced the physical NVIDIA target (`\\.\\DISPLAY1`) to
640x480@60 and then restored 1280x720@60. Both transitions returned success,
and the final Windows mode was confirmed as 1280x720@60. After each transition
the bridge still reported `SYS_STATUS=0x0D`, `pll_locked=true`,
`hdmi_signal_detected=false`, `hdmi_sync_locked=false`, and `frame_count=0`.

Windows also reports the physical target as `DCDZ-H2C MOD` with the exact
256-byte EDID previously loaded by the bridge. Apollo/Sunshine is present on a
separate virtual display adapter, whose 2560x1440 mode does not explain the
physical target's unchanged HDMI receiver status.

This closes the software-controlled host-side checks. Completing the HDMI gate
now requires a physical comparison: a known-good HDMI cable/source or a
known-good monitor/repeater, plus adapter power measurement if the comparison
also fails.

### Test 013 - Controlled HPD retrain without host reboot

The firmware now exposes a bench-only `GET /bridge/retrain` action. It drives
HPD low for 500 ms, raises it, waits 1 s for source negotiation, resets the
HDMI PHY, waits another 1 s, and then polls status. The action completed with
no I2C error and incremented the recovery counter, but the bridge remained at
`SYS_STATUS=0x0D`, `hdmi_signal_detected=false`, `hdmi_sync_locked=false`, and
`frame_count=0`; reference-clock readback remained valid.

This rules out a missing software HPD retrain action as the immediate cause.
The receiver still needs a TMDS-valid source before CSI can be meaningfully
qualified.

### Test 014 - Final EDID and status-bit integrity check

The Windows registry copy of the physical target's EDID is 256 bytes and both
128-byte blocks pass their checksum (`0`). The final board snapshot remains
`SYS_STATUS=0x0D`: DDC 5V, PHY PLL, and PHY SCDT are set, while TMDS-valid,
HDMI-mode, and sync are clear. The 27 MHz readback is valid and CSI has
generated zero frames.

This is the narrowest reproducible failure: the receiver sees an HDMI clock
and source power but does not validate the TMDS data stream. No remaining
software-only test can prove or repair the differential data path; the next
decisive evidence must come from a known-good source/cable/repeater comparison
or electrical measurements at the adapter.

### Test 4 — BIOS/UEFI versus Windows split

Repeat the 720p60 test at a pre-boot screen before Apollo/Sunshine starts. A
lock in BIOS/UEFI but not Windows points to Windows topology, virtual-display,
driver, or mode selection. Failure in both environments points to the
HPD/EDID/DDC path, TC358743 initialization, source selection, cable, or power.

### Test 5 — Only after HDMI lock, qualify CSI

The firmware now contains both sides of the downstream test, gated behind
source lock: an ESP32-P4 2-lane CSI receiver at 1280x720 RGB888 and the
TC358743's upstream-derived 972 Mbps/lane CSI transmitter PLL, lane timing,
packet configuration, and stream-enable sequence. With the current source
status still red, the live receiver correctly remains
`waiting_for_bridge`, `receiver_started=false`, and `frame_count=0`; this is a
valid negative result, not evidence of a CSI ribbon failure.

Once HDMI status is good, capture the CSI clock/data lanes with a suitable
probe or the ESP32-P4 CSI diagnostics, and record LP11-to-HS transition, lane
rate, receiver lock, packet/error counters, frame dimensions, and changing
frame fingerprints. Verify FPC orientation/lane mapping against the exact
board connector documentation if HDMI locks but the receiver remains idle.

## Acceptance evidence to keep

For the next bring-up attempt, save one timestamped record containing:

- exact board/adapter revisions, HDMI cable and NVIDIA port;
- raw 256-byte EDID plus checksums and the bridge readback;
- HPD electrical trace and DDC transaction trace;
- Windows monitor PnP instance, WMI EDID blocks, and DisplayConfig output
  technology;
- `nvidia-smi -q -d DISPLAY` including both Attached and Active;
- source mode and feature settings;
- repeated `SYS_STATUS`, `HPD_CTL`, `EDID_MODE`, `EDID_LEN1/2`, `PHY_EN`,
  `ANA_CTL`, detected timing, and CSI status values;
- whether Apollo/Sunshine/SudoVDA was fully stopped or active.

The immediate gate is not “CSI frame captured.” It is: Windows reads the EDID,
the physical HDMI target is identified, and `TMDS/SCDT/HDMI/SYNC` all become
valid at 720p60 or 1080p30. Only then does CSI output become the next bounded
problem.

## Primary sources

- [Waveshare HDMI-to-CSI Adapter wiki](https://www.waveshare.com/wiki/HDMI_to_CSI_Adapter)
- [Waveshare HDMI-to-CSI driver/EDID archive](https://files.waveshare.com/wiki/HDMI_to_CSI_Adapter/Hdmi_to_csi_driver.zip)
- [Waveshare ESP32-P4-ETH documentation](https://docs.waveshare.com/ESP32-P4-ETH)
- [Linux upstream TC358743 driver](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743.c)
- [Linux upstream TC358743 register header](https://github.com/torvalds/linux/blob/master/drivers/media/i2c/tc358743_regs.h)
- [NVIDIA `nvidia-smi` documentation](https://docs.nvidia.com/deploy/nvidia-smi/index.html)
- [Microsoft monitor class function driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/monitor-class-function-driver)
- [Microsoft `DISPLAYCONFIG_TARGET_DEVICE_NAME`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_target_device_name)
- [Microsoft `DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ne-wingdi-displayconfig_video_output_technology)
- [Microsoft `DisplayConfigGetDeviceInfo`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-displayconfiggetdeviceinfo)
- [Microsoft `WmiMonitorID`](https://learn.microsoft.com/en-us/windows/win32/wmicoreprov/wmimonitorid)
- [Apollo first-party README](https://github.com/ClassicOldSong/Apollo)
- [Sunshine first-party getting-started documentation](https://github.com/LizardByte/Sunshine/blob/master/docs/getting_started.md)
