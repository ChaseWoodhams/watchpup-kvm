import test from 'node:test';
import assert from 'node:assert/strict';
import { validateBridgeDiagnostics, validateBridgeSerialLog } from './validate-phase2.mjs';

function fixture({ signal = true } = {}) {
  return {
    schema_version: 1,
    firmware: { project_name: 'watchpup_kvm', firmware_version: 'phase2-bridge', idf_version: 'v5.4.2', target: 'esp32p4' },
    subsystems: {
      board_config: { status: 'ok', implemented: true, details: { board_id: 'waveshare_esp32_p4_eth', bridge_adapter_id: 'waveshare_hdmi_to_csi_tc358743xbg', bridge_i2c_port: 0, bridge_i2c_sda_gpio: 7, bridge_i2c_scl_gpio: 8, bridge_i2c_address: 15, configured_csi_lane_count: 2 } },
      bridge_tc358743: { status: signal ? 'ok' : 'degraded', implemented: true, details: { probe_ok: true, chip_id: '0x0000', i2c_address: 15, i2c_sda_gpio: 7, i2c_scl_gpio: 8, configured_csi_lane_count: 2, edid_loaded: true, edid_revision: 'waveshare-1080p30-v1', hpd_asserted: true, pll_locked: signal, csi_output_enabled: false, sys_status: signal ? 0x96 : 0x00, csi_status: 0, hdmi_signal_detected: signal, hdmi_sync_locked: signal, negotiated_width: signal ? 1920 : 0, negotiated_height: signal ? 1080 : 0, negotiated_frame_rate: signal ? 30 : 0 } },
    },
  };
}

test('accepts a locked TC358743 bridge diagnostics response', () => {
  assert.doesNotThrow(() => validateBridgeDiagnostics(fixture()));
});

test('accepts a probed bridge while the HDMI source is absent', () => {
  assert.doesNotThrow(() => validateBridgeDiagnostics(fixture({ signal: false }), { requireSignal: false }));
  assert.throws(() => validateBridgeDiagnostics(fixture({ signal: false })), assert.AssertionError);
});

test('requires bridge init, EDID, and status serial evidence', () => {
  const log = '[diag] subsystem.init subsystem=bridge_tc358743 status=ok chip_id=0x0000\n[diag] bridge.edid status=ok revision=waveshare-1080p30-v1\n[diag] subsystem.health subsystem=bridge_tc358743 status=ok chip_id=0x0000';
  assert.doesNotThrow(() => validateBridgeSerialLog(log));
});
