import assert from 'node:assert/strict';

export function validateBridgeDiagnostics(diag, { requireSignal = true } = {}) {
  assert.equal(diag.schema_version, 1);
  assert.equal(diag.firmware.project_name, 'watchpup_kvm');
  assert.equal(diag.firmware.firmware_version, 'phase2-bridge');
  assert.match(diag.firmware.idf_version, /^v?5\.4\.2$/);
  assert.equal(diag.firmware.target, 'esp32p4');

  const board = diag.subsystems.board_config;
  assert.equal(board.status, 'ok');
  assert.equal(board.implemented, true);
  assert.equal(board.details.board_id, 'waveshare_esp32_p4_eth');
  assert.equal(board.details.bridge_adapter_id, 'waveshare_hdmi_to_csi_tc358743xbg');
  assert.equal(board.details.bridge_i2c_port, 0);
  assert.equal(board.details.bridge_i2c_sda_gpio, 7);
  assert.equal(board.details.bridge_i2c_scl_gpio, 8);
  assert.equal(board.details.bridge_i2c_address, 15);
  assert.equal(board.details.configured_csi_lane_count, 2);

  const bridge = diag.subsystems.bridge_tc358743;
  assert.equal(bridge.implemented, true);
  assert.ok(['ok', 'degraded', 'error'].includes(bridge.status));
  const details = bridge.details;
  assert.equal(details.probe_ok, true);
  assert.match(details.chip_id, /^0x[0-9a-f]{4}$/i);
  assert.equal(details.i2c_address, 15);
  assert.equal(details.i2c_sda_gpio, 7);
  assert.equal(details.i2c_scl_gpio, 8);
  assert.equal(details.configured_csi_lane_count, 2);
  assert.equal(details.edid_loaded, true);
  assert.equal(details.edid_revision, 'waveshare-1080p30-v1');
  assert.equal(details.hpd_asserted, true);
  assert.equal(typeof details.pll_locked, 'boolean');
  assert.equal(typeof details.csi_output_enabled, 'boolean');
  assert.ok(Number.isInteger(details.sys_status));
  assert.ok(Number.isInteger(details.csi_status));
  if (requireSignal) {
    assert.equal(details.hdmi_signal_detected, true);
    assert.equal(details.hdmi_sync_locked, true);
    assert.equal(details.pll_locked, true);
    assert.ok(details.negotiated_width > 0);
    assert.ok(details.negotiated_height > 0);
    assert.ok(details.negotiated_frame_rate > 0);
    assert.equal(bridge.status, 'ok');
  }
}

export function validateBridgeSerialLog(log) {
  for (const event of [
    'subsystem.init subsystem=bridge_tc358743',
    'bridge.edid status=ok',
    'subsystem.health subsystem=bridge_tc358743',
  ]) {
    assert.ok(log.includes(`[diag] ${event}`), `missing bridge serial event: ${event}`);
  }
  assert.match(log, /chip_id=0x[0-9a-f]+/i);
}

const [mode, value] = process.argv.slice(2);
if (mode === '--url') {
  const response = await fetch(value);
  assert.equal(response.status, 200);
  assert.match(response.headers.get('content-type') ?? '', /^application\/json/);
  validateBridgeDiagnostics(await response.json());
  console.log('Phase 2 bridge diagnostics passed');
} else if (mode === '--serial-log') {
  const { readFile } = await import('node:fs/promises');
  validateBridgeSerialLog(await readFile(value, 'utf8'));
  console.log('Phase 2 bridge serial diagnostics passed');
}
