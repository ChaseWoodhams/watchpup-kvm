# Diagnostics Endpoint Contract

Issue: [#25 Define diagnostics endpoint contract](https://github.com/ChaseWoodhams/watchpup-kvm/issues/25)

Date: 2026-07-09

Related decisions and research:

- [ADR 0003: Firmware Subsystems And Diagnostics First](../adrs/0003-firmware-subsystems-and-diagnostics-first.md)
- [ESP-IDF Skeleton And Dependency Baseline](../research/esp-idf-skeleton-baseline.md)
- [Initial Hardware Configuration Contract](../hardware/initial-hardware-configuration-contract.md)
- [TC358743 Bridge Driver Design Notes](../hardware/tc358743-bridge-driver-design-notes.md)

## Purpose

This document defines the first stable JSON contract for WatchPup's local `GET /diag` endpoint and the matching serial diagnostics expectations.

It exists to preserve the diagnostics-first sequencing accepted in [ADR 0003](../adrs/0003-firmware-subsystems-and-diagnostics-first.md). Later firmware issues should implement this shape directly instead of re-deciding field names while video, HID, or auth work is in progress.

This is a contract document, not an implementation ticket. It does not require HDMI capture, HID input, or auth logic to exist in this issue.

## Endpoint Boundary

The first diagnostics endpoint contract is:

- Route: `GET /diag`
- Response content type: `application/json`
- Success behavior: return HTTP `200` with a JSON body whenever the HTTP server and diagnostics subsystem are alive enough to answer
- Failure behavior: use transport-level failure only when the endpoint cannot answer at all; degraded subsystem state belongs in the JSON body, not in alternate endpoint shapes

The endpoint is intended for local firmware health inspection. It should remain machine-readable and safe for browser and agent tooling to poll.

### Bench Bootstrap Security Exception

For the earliest board-health milestone only, `GET /diag` may be served without authentication on an isolated bench network or direct Ethernet link. This exception exists solely to make device and network bring-up observable before an authentication subsystem exists.

It is not a v1 network posture: the bootstrap build must expose no control, HID, power, or streaming routes, and it must be explicitly identified as a bench build in its diagnostics output. Authentication is a required gate before any streaming or control-capable endpoint is exposed on a LAN.

## Stability Rule

Version the contract through `schema_version`.

- `schema_version: 1` is the first stable shape defined by this document.
- New fields may be added in later firmware tickets without breaking `schema_version: 1`.
- Existing field names, object locations, enum values, and value meanings defined here should not change within `schema_version: 1`.
- If a future change must rename, move, or reinterpret an existing field, bump the schema version and document the migration.

## Top-Level Shape

`/diag` should return one JSON object with this top-level shape:

```json
{
  "schema_version": 1,
  "generated_at_ms": 0,
  "uptime_ms": 0,
  "overall_status": "ok",
  "firmware": {},
  "boot": {},
  "subsystems": {},
  "recovery": {}
}
```

Top-level field meanings:

| Field | Type | Meaning |
| --- | --- | --- |
| `schema_version` | integer | Contract version for the JSON document shape. |
| `generated_at_ms` | integer | Milliseconds since boot when this JSON snapshot was generated. |
| `uptime_ms` | integer | Milliseconds since boot for the current firmware session. |
| `overall_status` | string | Summary health of the device: `ok`, `degraded`, or `error`. |
| `firmware` | object | Stable firmware/build identity fields. |
| `boot` | object | Reset and boot lifecycle fields for the current session. |
| `subsystems` | object | Per-subsystem health objects keyed by subsystem name. |
| `recovery` | object | Cross-subsystem recovery counters and last recovery cause. |

## Naming Conventions

Use these naming rules throughout the contract:

- Use `snake_case` for all JSON keys.
- Use booleans for simple on/off facts such as `link_up` or `psram_present`.
- Use integers for counts, durations, byte sizes, and monotonic counters.
- Use strings for enums and identifiers.
- Use `null` only when a field is implemented but currently unknown or unreadable.
- Prefer explicit objects over packed status strings.
- Do not encode units into values; encode them in the field name such as `_ms`, `_bytes`, `_mbps`, or `_hz`.

## Standard Subsystem Object

Each entry under `subsystems` should use the same framing fields before adding subsystem-specific detail:

```json
{
  "status": "ok",
  "implemented": true,
  "last_error": null,
  "details": {}
}
```

Subsystem framing fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `status` | string | `ok`, `degraded`, `error`, or `unavailable`. |
| `implemented` | boolean | Whether this subsystem has real firmware logic in the current build. |
| `last_error` | string or null | Stable short error token for the last known fault, or `null` if none. |
| `details` | object | Subsystem-specific fields defined below. |

Representation rules:

- Use `status: "unavailable"` with `implemented: false` for subsystems that are planned by the contract but not yet implemented.
- Keep the object present even when unavailable. Do not omit future subsystem keys once this contract is implemented.
- Use `details: {}` when a subsystem exists only as a placeholder.
- Use `null` for implemented-but-currently-unreadable values, not for entire missing subsystem objects.

## Initial Skeleton Fields

The earliest firmware skeleton is expected to expose only data that can exist before HDMI, CSI, JPEG, HID, and auth are implemented.

The first build that implements `/diag` should include these concrete fields:

```json
{
  "schema_version": 1,
  "generated_at_ms": 1820,
  "uptime_ms": 1820,
  "overall_status": "ok",
  "firmware": {
    "project_name": "watchpup_kvm",
    "firmware_version": "dev",
    "git_revision": null,
    "idf_version": "v6.0.2",
    "target": "esp32p4"
  },
  "boot": {
    "boot_count": 1,
    "reset_reason": "power_on",
    "safe_mode": false
  },
  "subsystems": {
    "diagnostics": {
      "status": "ok",
      "implemented": true,
      "last_error": null,
      "details": {
        "endpoint_ready": true,
        "serial_logging_ready": true
      }
    },
    "board_config": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "psram": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "ethernet": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "bridge_tc358743": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "capture": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "encoder_mjpeg": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "frame_store": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "stream_server": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "hid_device": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "control_protocol": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    },
    "auth_security": {
      "status": "unavailable",
      "implemented": false,
      "last_error": null,
      "details": {}
    }
  },
  "recovery": {
    "total_recoveries": 0,
    "last_recovery_reason": null
  }
}
```

This is intentionally small. It proves endpoint shape, boot identity, uptime, and placeholder subsystem visibility before higher-risk firmware lands.

## Future Subsystem Fields

Later implementation tickets should populate these `details` fields without moving the surrounding object shape.

### `board_config`

`details` should grow to include:

- `board_id`
- `board_revision`
- `ethernet_phy`
- `bridge_adapter_id`
- `usb_device_port`
- `configured_csi_lane_count`
- `configured_csi_lane_rate_mbps`
- `notes`

### `psram`

`details` should grow to include:

- `psram_present`
- `psram_size_bytes`
- `psram_free_bytes`
- `psram_largest_free_block_bytes`
- `psram_speed_hz`
- `psram_test_passed`

### `ethernet`

`details` should grow to include:

- `initialized`
- `mac_address`
- `link_up`
- `dhcp_enabled`
- `ipv4_address`
- `netmask`
- `gateway`
- `mdns_ready`
- `hostname`
- `last_link_change_ms`

### `bridge_tc358743`

`details` should grow to include:

- `probe_ok`
- `chip_id`
- `driver_state`
- `reset_supported`
- `hpd_asserted`
- `edid_loaded`
- `edid_revision`
- `hdmi_signal_detected`
- `negotiated_width`
- `negotiated_height`
- `negotiated_frame_rate`
- `pll_locked`
- `csi_output_enabled`
- `last_init_success_ms`
- `last_signal_lock_ms`
- `recovery_count`

These fields align with the boundary defined in [TC358743 Bridge Driver Design Notes](../hardware/tc358743-bridge-driver-design-notes.md).

### `capture`

`details` should grow to include:

- `initialized`
- `camera_driver_ready`
- `csi_link_ready`
- `configured_width`
- `configured_height`
- `pixel_format`
- `frames_received`
- `frames_dropped`
- `frames_dropped_busy`
- `last_completed_sequence`
- `last_frame_ms`
- `last_frame_error`

### `encoder_mjpeg`

`details` should grow to include:

- `initialized`
- `jpeg_engine_ready`
- `quality`
- `encode_attempts`
- `encode_width`
- `encode_height`
- `frames_encoded`
- `encode_failures`
- `last_encode_ms`
- `last_encode_time_us`
- `max_encode_time_us`
- `last_jpeg_size_bytes`

### `frame_store`

`details` should grow to include:

- `published_generation`
- `publish_count`
- `publish_drop_no_slot`
- `slot_pressure_events`
- `slot_borrow_count`
- `slot_peak_in_use`
- `retired_slots_waiting_release`

### `stream_server`

`details` should grow to include:

- `initialized`
- `http_server_ready`
- `ws_control_ready`
- `active_mjpeg_clients`
- `peak_mjpeg_clients`
- `frames_served`
- `send_attempts`
- `frames_skipped_total`
- `skipped_generations_total`
- `client_disconnects_slow_consumer`
- `send_failures`
- `last_client_connect_ms`
- `last_sent_generation`

### `hid_device`

`details` should grow to include:

- `initialized`
- `tinyusb_ready`
- `keyboard_ready`
- `mouse_ready`
- `active_output_owner`
- `keys_stuck`
- `buttons_stuck`
- `forced_release_count`
- `last_release_reason`

### `control_protocol`

`details` should grow to include:

- `initialized`
- `input_control_enabled`
- `active_controller_id`
- `ownership_state`
- `messages_received`
- `messages_rejected`
- `protocol_errors`
- `last_protocol_error`
- `last_owner_change_ms`
- `forced_release_count`
- `last_release_reason`
- `input_queue_depth`
- `input_queue_high_water`
- `queue_overflow_count`

### `auth_security`

`details` should grow to include:

- `auth_required`
- `auth_backend`
- `session_count`
- `authenticated_session_count`
- `failed_auth_count`
- `lockout_active`
- `unsafe_lab_mode`
- `last_auth_failure_ms`

### `diagnostics`

`details` may grow to include:

- `endpoint_ready`
- `serial_logging_ready`
- `last_diag_render_ms`
- `last_diag_error`
- `last_serial_error`

## Overall Status Rules

`overall_status` summarizes the whole device:

- `ok`: all implemented subsystems needed for the current milestone are healthy
- `degraded`: the device is alive, but one or more implemented subsystems are partially unhealthy or recovering
- `error`: a required implemented subsystem is failed hard enough that the current milestone cannot function

Unavailable future subsystems should not force `overall_status` to `degraded` or `error` during earlier milestones. For example, a firmware skeleton may still report `overall_status: "ok"` while HDMI and HID remain `unavailable`.

## Recovery Object

Use the top-level `recovery` object for counters that span more than one subsystem:

- `total_recoveries`
- `last_recovery_reason`
- `last_recovery_ms`
- `bridge_recoveries`
- `capture_recoveries`
- `stream_recoveries`
- `hid_forced_releases`

Fields may be added over time, but these names should remain stable once introduced.

## Serial Diagnostics Expectations

Serial logging is the first diagnostics surface and should mirror `/diag` health without requiring a debugger.

The first firmware milestones should emit these boot and health events:

1. Boot banner with project name, firmware version, ESP-IDF version, target, and reset reason.
2. Diagnostics-ready line confirming serial logging and `/diag` registration.
3. One line per subsystem init attempt with success, degraded state, or failure.
4. One line for every recovery attempt and its result.
5. A concise one-line health summary after boot settles.

Preferred log style:

- Prefix all diagnostics lines with `[diag]`.
- Use stable event tokens such as `boot.start`, `boot.ready`, `subsystem.init`, `subsystem.health`, `recovery.run`, and `health.summary`.
- Include the subsystem key used by `/diag` so serial logs and JSON are easy to correlate.
- Keep values single-line and machine-greppable.

Illustrative serial lines:

```text
[diag] boot.start project=watchpup_kvm version=dev idf=v6.0.2 target=esp32p4 reset_reason=power_on
[diag] subsystem.init subsystem=diagnostics status=ok
[diag] subsystem.init subsystem=board_config status=unavailable implemented=false
[diag] boot.ready diag_route=/diag overall_status=ok
[diag] health.summary overall_status=ok uptime_ms=1820
```

Later milestones should extend the same pattern:

```text
[diag] subsystem.health subsystem=bridge_tc358743 status=degraded hdmi_signal_detected=false last_error=no_signal
[diag] recovery.run subsystem=bridge_tc358743 reason=signal_lost result=success recovery_count=3
```

## Implementation Posture

The first firmware ticket that implements `/diag` should:

- Return this top-level JSON shape.
- Populate the initial skeleton fields listed above.
- Include placeholder objects for not-yet-implemented subsystems.
- Log the boot and diagnostics-ready events described above.

That ticket should not expand scope into HDMI capture, MJPEG, HID, or authentication behavior beyond reporting placeholder health.
