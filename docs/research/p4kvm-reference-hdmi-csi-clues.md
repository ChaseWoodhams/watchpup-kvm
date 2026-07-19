# p4kvm-reference clues for the HDMI-to-CSI bring-up

Date: 2026-07-14  
Reference tree: `C:\code\p4kvm-reference` (read-only)  
Target tree: `C:\code\watchpup-kvm`

## Conclusion

The reference project confirms the broad WatchPup assumptions—TC358743 at I2C0
`GPIO7/GPIO8`, 27 MHz reference clock, two CSI lanes at 972 Mbps/lane—but it
does not provide a checked-in known-good HDMI capture log. Its capture task
waits five seconds for HDMI lock and then starts CSI anyway on timeout. Therefore
the reference is useful as a register/sequencing comparison, not proof that the
current physical HDMI path is known to work.

The reference does not overturn the current diagnosis. Its status logger defines
TMDS as `SYS_STATUS` bit `0x02`, HDMI mode as `0x10`, sync as `0x80`, and DDC5V
as `0x01` ([`main/tc358743_hdmi_debug.h:7-18`](C:/code/p4kvm-reference/main/tc358743_hdmi_debug.h:7)).
WatchPup's live `SYS_STATUS=0x0D` therefore still has TMDS, HDMI-mode, and sync
clear; the zero CSI frame count remains downstream evidence, not the cause
([`tc358743-windows-host-hdmi-investigation.md:64`](C:/code/watchpup-kvm/docs/research/tc358743-windows-host-hdmi-investigation.md:64)).

## 1. TC358743 initialization and PHY comparison

The reference's 27 MHz calculations are exactly the values already used and
read back by WatchPup:

- `SYS_FREQ = refclk / 10000`
- `FH_MIN = refclk / 100000`
- `FH_MAX = FH_MIN * 66 / 10`
- `LOCKDET_REF = refclk / 100`
- NCO 27 MHz mode enabled

These are implemented in [`main/tc358743.c:378-405`](C:/code/p4kvm-reference/main/tc358743.c:378) and in WatchPup's write/readback path at [`bridge_tc358743.c:237-276`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:237). The current live readback is therefore not a reference-clock discrepancy.

The reference initialization order is: keep HPD low, perform the block reset,
leave sleep, set FIFO/reference clock/DDC/EDID mode, configure and enable the
HDMI PHY, then configure the video output ([`main/tc358743.c:496-517`](C:/code/p4kvm-reference/main/tc358743.c:496)). Its PHY defaults are `PHY_CTL1=0x80`, PHY bias `0x40`, `PHY_CSQ=0x0A`, `AVM_CTL=45`, no HDMI detector delay, and no automatic H/V reset bits ([`main/tc358743.c:445-463`](C:/code/p4kvm-reference/main/tc358743.c:445)). WatchPup matches those values at [`bridge_tc358743.c:319-340`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:319).

Two real implementation differences deserve an A/B test:

1. The reference clears only the three low automatic-reset bits in `PHY_CTL2`
   while preserving the rest of the register ([`main/tc358743.c:451-454`](C:/code/p4kvm-reference/main/tc358743.c:451)). WatchPup currently writes the entire byte as `0x00` ([`bridge_tc358743.c:322-324`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:322)). The live value is currently zero, but the reference-preserving write is the closer match and avoids changing unrelated/reserved bits.
2. The reference does not pulse `PHY_RST` after HPD goes high. It enables the
   stream, waits 150 ms, raises HPD, waits 50 ms, and pulses `CSI_START`
   ([`main/tc358743.c:705-716`](C:/code/p4kvm-reference/main/tc358743.c:705)). WatchPup waits one second after HPD and then applies an explicit HDMI PHY
   reset before its final status poll ([`bridge_tc358743.c:678-700`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:678)). That reset may be harmless, but it is the clearest sequencing difference to test by capturing status immediately before and after it or by running one reference-style boot.

The reset-block order itself is substantially aligned: the reference keeps IR
and CEC reset, pulses context and HDMI reset, and leaves sleep before applying
the source path ([`main/tc358743.c:496-505`](C:/code/p4kvm-reference/main/tc358743.c:496); [`bridge_tc358743.c:289-300`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:289)).

## 2. HPD, EDID, and source-mode clues

The reference keeps HPD low while writing the two-block EDID and raises it only
after PLL/CSI setup ([`main/tc358743.c:606-624`](C:/code/p4kvm-reference/main/tc358743.c:606); [`main/tc358743.c:676-716`](C:/code/p4kvm-reference/main/tc358743.c:676)). WatchPup follows the same dependency and uses longer settling delays, so HPD timing is not an obvious missing step.

The reference EDID is explicitly a Waveshare 1080p30 descriptor
([`main/tc358743_edid_1080p30.h:1-14`](C:/code/p4kvm-reference/main/tc358743_edid_1080p30.h:1)). WatchPup instead loads Waveshare's 720p60 descriptor
([`bridge_tc358743.c:123-130`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:123)). The current host was successfully forced to 1280x720@60, but the reference does not establish that 720p60 actually locks on this adapter. A high-value controlled firmware/source test is therefore the reference combination: exact 1080p30 EDID plus a host mode of 1920x1080@30, with HDR/VRR/HDCP disabled. This is a test hypothesis, not a reason to copy the reference EDID into firmware without review.

The reference README's own troubleshooting guidance says that source sleep can
prevent recovery, recommends connecting the target before boot, trying another
HDMI/USB cable, and requires all four displayed status categories to be one
([`README.md:52-58`](C:/code/p4kvm-reference/README.md:52)). This supports a clean cold-boot/source-mode/cable comparison, but it is anecdotal guidance rather than a successful log.

## 3. Board, connector, ribbon, and power evidence

The reference hardware defaults document only these electrical assumptions:

- ESP32-P4 MIPI LDO channel 3 at 2500 mV;
- I2C0 control pins SDA GPIO7 and SCL GPIO8;
- 27 MHz bridge reference clock;
- two CSI lanes at 972 Mbps/lane;
- 1920x1080 capture dimensions.

Source: [`main/p4kvm_hw_defaults.h:7-17`](C:/code/p4kvm-reference/main/p4kvm_hw_defaults.h:7).

The reference capture path acquires that 2.5 V LDO before resetting/probing the
bridge ([`main/capture_hw.c:189-213`](C:/code/p4kvm-reference/main/capture_hw.c:189)). WatchPup configures the same I2C/lane/rate/clock values in [`board_config.c:3-24`](C:/code/watchpup-kvm/firmware/main/board_config.c:3), but its current board-health startup only enables the Ethernet PHY rail and has no MIPI-LDO acquisition ([`board_health.c:75-82`](C:/code/watchpup-kvm/firmware/main/board_health.c:75)). This is a concrete CSI-side discrepancy to resolve before trusting frame capture; it cannot explain missing HDMI TMDS/SYNC because the LDO is the ESP32 CSI receiver supply, not evidence of HDMI differential-pair lock.

The reference also defines an optional active-low TC358743 RESETN GPIO, defaulting
to GPIO23 ([`main/Kconfig.projbuild:5-8`](C:/code/p4kvm-reference/main/Kconfig.projbuild:5)). Its reset helper drives low for 10 ms, high, and waits 200 ms; when unused it waits 500 ms for internal POR ([`main/capture_hw.c:45-65`](C:/code/p4kvm-reference/main/capture_hw.c:45)). WatchPup records `bridge_reset_gpio=-1` and performs no external reset pulse ([`board_config.c:17-24`](C:/code/watchpup-kvm/firmware/main/board_config.c:17)). If the actual adapter wiring exposes RESETN to GPIO23, this is worth verifying against the board schematic and testing; GPIO23 must not be assumed to be connected just because it is the reference default.

There is no reference-project documentation of FPC pin-1 orientation, ribbon
contact direction, the ESP32 `Display` versus `Camera` connector, or the HDMI-to-
CSI adapter's external power source. The reference README lists the boards but
does not specify those mechanical/electrical details ([`README.md:9-18`](C:/code/p4kvm-reference/README.md:9)). The current WatchPup hardware contract correctly treats cable orientation, lane mapping, and adapter power path as items requiring physical evidence ([`initial-hardware-configuration-contract.md:77-87`](C:/code/watchpup-kvm/docs/hardware/initial-hardware-configuration-contract.md:77); [`initial-hardware-configuration-contract.md:118-126`](C:/code/watchpup-kvm/docs/hardware/initial-hardware-configuration-contract.md:118)). The reference cannot settle those questions.

One smaller control-bus difference: the reference enables ESP32 I2C internal
pull-ups, while WatchPup disables them ([`main/capture_hw.c:200-210`](C:/code/p4kvm-reference/main/capture_hw.c:200); [`bridge_tc358743.c:624-630`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:624)). Because WatchPup already probes, reads, and writes the bridge reliably, this is low priority for the present HDMI symptom.

## 4. CSI configuration and test behavior

The reference configures two CSI lanes at 972 Mbps/lane and RGB888
([`main/capture_hw.c:246-273`](C:/code/p4kvm-reference/main/capture_hw.c:246)). WatchPup uses the same two-lane/972 Mbps/RGB888 settings when it eventually creates the CSI controller ([`capture_csi.c:58-71`](C:/code/watchpup-kvm/firmware/main/capture_csi.c:58)).

The reference writes `TXOPTIONCNTRL=0`, explicitly documenting a
non-continuous MIPI clock ([`main/tc358743.c:591-604`](C:/code/p4kvm-reference/main/tc358743.c:591)). WatchPup writes `0` and then immediately overwrites it with `1`
([`bridge_tc358743.c:419-436`](C:/code/watchpup-kvm/firmware/main/bridge_tc358743.c:419)). This is a concrete CSI-side discrepancy; align it with the reference only after or alongside a successful HDMI lock test. It cannot create the missing HDMI TMDS bit.

The reference starts the ESP camera controller after a five-second HDMI-lock
wait, but logs a warning and starts CSI even if the wait expires
([`main/capture_hw.c:68-82`](C:/code/p4kvm-reference/main/capture_hw.c:68); [`main/capture_hw.c:281-293`](C:/code/p4kvm-reference/main/capture_hw.c:281)). WatchPup intentionally leaves CSI idle until both HDMI signal and sync are locked ([`capture_csi.c:49-56`](C:/code/watchpup-kvm/firmware/main/capture_csi.c:49)). This difference explains why the reference may show a running CSI task without proving valid frames; it does not make CSI responsible for `SYS_STATUS=0x0D`.

## Recommended next experiments, in order

1. Add a diagnostic A/B that matches the reference exactly for one boot: preserve
   `PHY_CTL2` upper bits, omit the post-HPD `PHY_RST` pulse, and record status
   before/after HPD. Keep the existing 27 MHz readback.
2. Verify whether the adapter's RESETN is actually wired to ESP32 GPIO23. If it
   is, perform the reference 10 ms low / 200 ms release reset before I2C setup.
3. Test the reference's 1080p30 EDID/mode combination on a cold boot. Do not
   infer success from Windows EDID enumeration; require TMDS, HDMI mode, sync,
   and a nonzero CSI frame count.
4. Once HDMI lock exists, acquire the ESP32-P4 MIPI 2.5 V LDO and change
   `TXOPTIONCNTRL` to the reference's non-continuous-clock value `0`, then
   validate CSI status and frame counters.
5. If steps 1–3 leave the status at `0x0D`, the reference provides no further
   software clue. The remaining discriminating tests are a known-good HDMI
   source/cable/repeater and direct verification of adapter power, connector
   orientation, and the TMDS path.

No source files in `C:\code\p4kvm-reference` were modified, and no reference
source was copied into WatchPup firmware.
