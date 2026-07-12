import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

export const requiredSubsystems = [
  'diagnostics', 'board_config', 'psram', 'ethernet', 'bridge_tc358743',
  'capture', 'encoder_mjpeg', 'frame_store', 'stream_server', 'hid_device',
  'control_protocol', 'auth_security',
];

export function validateDiagnostics(diag, { requireHealthy = true } = {}) {
  assert.equal(diag.schema_version, 1);
  assert.ok(Number.isInteger(diag.generated_at_ms) && diag.generated_at_ms >= 0);
  assert.equal(diag.firmware.project_name, 'watchpup_kvm');
  assert.equal(diag.firmware.firmware_version, 'phase1-bench');
  assert.match(diag.firmware.idf_version, /^v?5\.4\.2$/);
  assert.equal(diag.firmware.target, 'esp32p4');
  assert.notEqual(diag.boot.reset_reason, 'unknown');
  assert.equal(typeof diag.boot.safe_mode, 'boolean');
  assert.ok(['ok', 'degraded', 'error'].includes(diag.overall_status));
  assert.ok(Number.isInteger(diag.uptime_ms) && diag.uptime_ms >= 0);
  assert.ok(diag.recovery && Number.isInteger(diag.recovery.total_recoveries));
  for (const name of requiredSubsystems) {
    assert.ok(diag.subsystems[name], `missing subsystem: ${name}`);
    assert.ok(['ok', 'degraded', 'error', 'unavailable'].includes(diag.subsystems[name].status), `invalid status: ${name}`);
    assert.equal(typeof diag.subsystems[name].implemented, 'boolean');
    assert.ok(Object.hasOwn(diag.subsystems[name], 'last_error'));
    assert.equal(typeof diag.subsystems[name].details, 'object');
  }
  assert.equal(diag.subsystems.diagnostics.implemented, true);
  assert.equal(diag.subsystems.diagnostics.details.endpoint_ready, true);
  assert.equal(diag.subsystems.diagnostics.details.serial_logging_ready, true);
  assert.equal(diag.subsystems.board_config.implemented, false);
  assert.equal(diag.subsystems.board_config.status, 'unavailable');
  assert.equal(diag.subsystems.psram.implemented, true);
  assert.equal(diag.subsystems.psram.details.psram_present, true);
  assert.equal(diag.subsystems.psram.details.psram_size_bytes, 33554432);
  assert.equal(diag.subsystems.psram.details.psram_test_passed, true);
  assert.equal(diag.subsystems.ethernet.implemented, true);
  assert.equal(diag.subsystems.ethernet.details.initialized, true);
  assert.equal(typeof diag.subsystems.ethernet.details.link_up, 'boolean');
  assert.equal(diag.subsystems.ethernet.details.dhcp_enabled, true);
  assert.ok(diag.subsystems.ethernet.details.ipv4_address === null || typeof diag.subsystems.ethernet.details.ipv4_address === 'string');
  for (const name of requiredSubsystems.slice(3)) {
    if (name === 'ethernet') continue;
    assert.equal(diag.subsystems[name].implemented, false);
    assert.equal(diag.subsystems[name].status, 'unavailable');
  }
  if (requireHealthy) {
    assert.equal(diag.overall_status, 'ok');
    assert.equal(diag.subsystems.ethernet.details.link_up, true);
    assert.match(diag.subsystems.ethernet.details.ipv4_address, /^\d+\.\d+\.\d+\.\d+$/);
  }
}

export async function validateRouteBoundary(diagUrl, fetchImpl = fetch) {
  const url = new URL(diagUrl);
  const rejected = [
    [url.href, { method: 'POST' }],
    [new URL('/snapshot.jpg', url).href, {}],
    [new URL('/stream', url).href, {}],
    [new URL('/control', url).href, {}],
    [new URL('/power', url).href, {}],
  ];
  for (const [target, options] of rejected) {
    const response = await fetchImpl(target, options);
    assert.ok(response.status >= 400, `unexpected exposed route: ${options.method ?? 'GET'} ${target}`);
  }
}

export function validateSerialLog(log) {
  for (const event of ['boot.start', 'subsystem.init subsystem=psram', 'subsystem.init subsystem=ethernet', 'subsystem.health subsystem=ethernet status=ok', 'boot.ready', 'health.summary']) {
    assert.ok(log.includes(`[diag] ${event}`), `missing serial event: ${event}`);
  }
  assert.match(log, /\[diag\] boot\.start .*reset_reason=\S+/);
}

const [mode, value] = process.argv.slice(2);
if (mode === '--url') {
  const response = await fetch(value);
  assert.equal(response.status, 200);
  assert.match(response.headers.get('content-type') ?? '', /^application\/json/);
  validateDiagnostics(await response.json());
  await validateRouteBoundary(value);
  console.log('Phase 1 diagnostics endpoint passed');
} else if (mode === '--serial-log') {
  validateSerialLog(await readFile(value, 'utf8'));
  console.log('Phase 1 serial diagnostics passed');
} else if (import.meta.url === `file:///${process.argv[1]?.replaceAll('\\', '/')}`) {
  throw new Error('usage: node tools/validate-phase1.mjs --url http://DEVICE/diag | --serial-log FILE');
}
