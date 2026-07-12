import test from 'node:test';
import assert from 'node:assert/strict';
import { requiredSubsystems, validateDiagnostics, validateRouteBoundary, validateSerialLog } from './validate-phase1.mjs';

function healthySubsystems() {
  const subsystems = Object.fromEntries(requiredSubsystems.map((name) => [name, { status: 'unavailable', implemented: false, last_error: null, details: {} }]));
  subsystems.diagnostics = { status: 'ok', implemented: true, last_error: null, details: { endpoint_ready: true, serial_logging_ready: true } };
  subsystems.psram = { status: 'ok', implemented: true, last_error: null, details: { psram_present: true, psram_size_bytes: 33554432, psram_test_passed: true } };
  subsystems.ethernet = { status: 'ok', implemented: true, last_error: null, details: { initialized: true, link_up: true, dhcp_enabled: true, ipv4_address: '192.0.2.2' } };
  return subsystems;
}

test('accepts a healthy Phase 1 diagnostics response', () => {
  const subsystems = healthySubsystems();
  assert.doesNotThrow(() => validateDiagnostics({ schema_version: 1, generated_at_ms: 1, uptime_ms: 1, overall_status: 'ok', firmware: { project_name: 'watchpup_kvm', firmware_version: 'phase1-bench', idf_version: 'v5.4.2', target: 'esp32p4' }, boot: { reset_reason: 'power_on', safe_mode: false }, subsystems, recovery: { total_recoveries: 0 } }));
});

test('rejects missing subsystem evidence', () => {
  const incomplete = healthySubsystems();
  delete incomplete.ethernet;
  assert.throws(() => validateDiagnostics({ schema_version: 1, generated_at_ms: 1, uptime_ms: 1, overall_status: 'ok', firmware: { project_name: 'watchpup_kvm', firmware_version: 'phase1-bench', idf_version: 'v5.4.2', target: 'esp32p4' }, boot: { reset_reason: 'power_on', safe_mode: false }, subsystems: incomplete, recovery: { total_recoveries: 0 } }), /missing subsystem: ethernet/);
});

test('accepts a structurally valid degraded response before DHCP', () => {
  const subsystems = healthySubsystems();
  subsystems.ethernet.status = 'degraded';
  subsystems.ethernet.details.link_up = true;
  subsystems.ethernet.details.ipv4_address = null;
  const diag = { schema_version: 1, generated_at_ms: 1, uptime_ms: 1, overall_status: 'degraded', firmware: { project_name: 'watchpup_kvm', firmware_version: 'phase1-bench', idf_version: 'v5.4.2', target: 'esp32p4' }, boot: { reset_reason: 'power_on', safe_mode: false }, subsystems, recovery: { total_recoveries: 0 } };
  assert.doesNotThrow(() => validateDiagnostics(diag, { requireHealthy: false }));
  assert.throws(() => validateDiagnostics(diag), assert.AssertionError);
});

test('requires the agreed serial events', () => {
  const log = '[diag] boot.start reset_reason=power_on\n[diag] subsystem.init subsystem=psram\n[diag] subsystem.init subsystem=ethernet\n[diag] subsystem.health subsystem=ethernet status=ok\n[diag] boot.ready\n[diag] health.summary';
  assert.doesNotThrow(() => validateSerialLog(log));
});

test('proves the bench route boundary rejects writes and forbidden capabilities', async () => {
  const requests = [];
  await validateRouteBoundary('http://192.0.2.2/diag', async (url, options) => {
    requests.push([new URL(url).pathname, options.method ?? 'GET']);
    return { status: 404 };
  });
  assert.deepEqual(requests, [
    ['/diag', 'POST'], ['/snapshot.jpg', 'GET'], ['/stream', 'GET'], ['/control', 'GET'], ['/power', 'GET'],
  ]);
});
