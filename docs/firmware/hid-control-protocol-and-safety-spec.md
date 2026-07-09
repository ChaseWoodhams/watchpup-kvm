# HID Control Protocol And Safety Spec

Issue: [#28 Define HID control protocol and safety spec](https://github.com/ChaseWoodhams/watchpup-kvm/issues/28)

Date: 2026-07-09

Related decisions and research:

- [ADR 0005: Web Assets And HID Control Safety](../adrs/0005-web-assets-and-hid-control-safety.md)
- [WatchPup KVM Product PRD Outline](../prds/watchpup-product-prd-outline.md)
- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)
- [Diagnostics Endpoint Contract](./diagnostics-endpoint-contract.md)
- [MJPEG Frame Ownership Contract](./mjpeg-frame-ownership-contract.md)

## Purpose

This document defines the first WatchPup HID control protocol and safety boundary before keyboard, mouse, or ATX control implementation begins.

It exists so later firmware, web UI, and local API tickets do not re-decide the dangerous parts of browser-accessible machine control: who is allowed to control, when control is enabled, how ownership changes, when input must be forcibly released, what input forms are allowed in v1, and what diagnostics must expose when things go wrong.

This is a contract and policy document, not an implementation ticket. It does not define TinyUSB report bytes, HID usage tables beyond the supported v1 input surface, browser UX polish, or the exact auth backend.

## Scope Boundary

This spec covers these subsystem boundaries only:

- Viewer versus controller capability separation.
- `auth_security` gating conditions required before HID/control is enabled.
- `control_protocol` session, ownership, validation, and release behavior.
- `hid_device` safety obligations once control commands are accepted.
- First diagnostics fields needed to observe ownership, enablement, errors, releases, and queue pressure.
- High-level authorization expectations for ATX and power actions.

This spec does not define:

- TinyUSB report layout or transport internals.
- Keyboard scan code tables beyond the declared v1 support posture.
- Exact websocket, HTTP, or local API endpoint routes.
- Browser video streaming behavior except where it interacts with controller ownership.
- Production auth mechanism details such as password storage or session crypto.
- Multi-controller collaboration modes.
- Absolute/tablet mouse mode.

## Decision Summary

WatchPup v1 HID control follows these rules:

- Viewing and control are separate capabilities.
- Multiple clients may view at once, but only one authenticated client may own active keyboard and mouse control at a time.
- `control_protocol` is the only browser-to-device boundary for control commands. Browser or API clients do not talk to `hid_device` directly.
- HID/control remains disabled until authentication and control enablement preconditions are satisfied.
- Loss of connection, timeout, ownership, auth, protocol validity, or operator-triggered emergency release forces immediate release of all pressed keys and mouse buttons and discards pending relative movement or queued input.
- V1 starts with US keyboard mapping, explicit hotkeys, text paste as rate-limited queued key events, and relative BIOS-compatible mouse mode only.
- Diagnostics must expose controller ownership, HID enabled state, last release reason, release counters, protocol errors, and input queue pressure.
- ATX and power actions share the authenticated control boundary but should remain separately authorized or separately confirmed from normal keyboard and mouse traffic.

## Terminology

Use these terms consistently in docs and code:

- **Viewer**: A connected client that may observe device status and video but does not own active input control.
- **Controller**: The single authenticated client that currently holds keyboard and mouse ownership.
- **Control session**: The server-side session state that tracks one connected client through auth, ownership, liveness, and release.
- **Ownership token**: An opaque value or server-assigned identifier that proves which session currently owns control.
- **Control enablement**: The state where the device is willing to accept validated control commands from the current controller.
- **Forced release**: A safety action that releases all active keys and mouse buttons, zeros relative movement, and drops queued input.
- **Emergency release**: An operator-triggered forced release independent of normal disconnect or timeout handling.
- **Queued text paste**: Text expansion into a bounded sequence of key press and release events rather than a device-side text injection primitive.
- **Protocol error**: Any malformed, out-of-order, unauthorized, or semantically invalid control message rejected by `control_protocol`.

## Top-Level Invariants

These invariants define the safety policy:

1. Viewing never implies control.
2. `hid_device` accepts input only from `control_protocol`, never directly from browser or API code.
3. At most one control session may own keyboard and mouse output at a time.
4. Control ownership is meaningful only for an authenticated session.
5. Any loss of trust in the active controller requires immediate forced release.
6. Forced release clears active key state, active mouse button state, pending relative mouse movement, and queued not-yet-sent input.
7. V1 prefers dropping or rejecting input over risking ambiguous or stuck machine control.
8. ATX and power actions are not ordinary keyboard or mouse events and must remain separately authorized or confirmed.

## Subsystem Boundary

### Browser and local clients

Browser UI and any future local API clients may:

- request viewer access,
- authenticate,
- request control ownership,
- send validated control commands through `control_protocol`,
- request explicit release, and
- observe diagnostics and ownership state.

They may not:

- write HID reports directly,
- assume viewer access grants control,
- retain control after auth loss or disconnect, or
- bypass queue limits or release rules.

### `control_protocol`

`control_protocol` is the only command boundary between remote clients and input execution.

Responsibilities:

- track authenticated control sessions,
- grant or reject single-controller ownership,
- validate control messages before forwarding them,
- enforce enabled versus disabled control state,
- enforce liveness timeouts and ownership changes,
- translate accepted commands into higher-level `hid_device` actions,
- trigger forced release on all mandatory release conditions,
- count protocol errors, rejected messages, and queue pressure,
- expose ownership and safety state to diagnostics.

Required non-behavior:

- do not emit raw HID reports directly from browser payload bytes,
- do not leave release policy to browser cooperation,
- do not accept commands from unauthenticated sessions,
- do not allow more than one active controller to drive keyboard or mouse state concurrently.

### `hid_device`

`hid_device` owns USB-facing keyboard and mouse execution.

Responsibilities:

- accept only already-validated commands from `control_protocol`,
- maintain current pressed-key and pressed-button state,
- apply explicit release requests immediately,
- support an idempotent "release everything now" operation,
- discard or refuse work after ownership loss or control disablement,
- expose enough state for diagnostics to reveal stuck-input recovery behavior.

Required non-behavior:

- do not perform auth checks itself as the primary policy boundary,
- do not infer session ownership from transport handles,
- do not keep emitting queued input after `control_protocol` has forced release,
- do not broaden v1 into absolute pointer or locale-flexible keyboard behavior.

## Capability Separation

The first WatchPup control model explicitly separates observation from machine control.

Rules:

- A connected client may be a viewer without being a controller.
- Multiple viewers may coexist.
- Viewer presence does not block the active controller.
- Loss or churn of viewers must not affect HID state unless the active controller is the session that disconnects or loses auth.
- Control-related UI state should be explicit enough that a human can tell whether they are viewing only or actively controlling.

This keeps read-only MJPEG milestones compatible with later control milestones and preserves the "view first, control later" sequencing from the product direction.

## Authentication And Control Enablement

HID/control must remain disabled unless all required preconditions are true.

Minimum v1 preconditions:

- `auth_security` is implemented and configured to require auth.
- the session attempting control is authenticated,
- the session has been granted controller ownership,
- no control-wide lockout or safety hold is active,
- the control protocol transport for that session is alive,
- `hid_device` reports itself ready enough to accept release-capable input.

Control enablement rules:

- Authentication is necessary but not sufficient; an authenticated viewer still does not control until ownership is granted.
- If auth expires, is revoked, or becomes unreadable for the active controller, control is disabled first and forced release happens immediately.
- If `hid_device` readiness drops while a controller exists, `control_protocol` must disable further input acceptance and issue forced release.
- If the implementation ever offers an unsafe lab mode, that mode must still require explicit documentation and must not silently bypass release policy or ownership rules.

## Control Session And Ownership Model

The first ownership model is single-controller and explicit.

### Ownership states

Recommended `ownership_state` values:

- `disabled`: control path is not available yet or is administratively disabled.
- `idle`: control is enabled in principle but no controller is active.
- `pending_grant`: a session requested control and the device is deciding or confirming.
- `active`: one authenticated session owns control.
- `releasing`: forced release is in progress before returning to `idle` or assigning a new owner.
- `blocked`: a safety hold, auth issue, or subsystem fault currently prevents ownership activation.

### Ownership rules

- Only one session may be `active` at a time.
- A new ownership grant must either wait for the current owner to release or explicitly revoke the old owner and force release before the new owner becomes active.
- Ownership changes are serialized by `control_protocol`.
- A session that loses underlying transport liveness immediately loses ownership.
- A session that loses auth immediately loses ownership.
- Session-local viewer state may survive ownership loss, but control commands from that session must be rejected until ownership is reacquired.

### Ownership transfer policy

V1 should choose conservative transfer behavior:

- explicit acquire request from an authenticated session,
- explicit grant or denial result,
- forced release before ownership moves from one controller to another,
- monotonic owner-change bookkeeping for diagnostics.

The spec does not require whether grants are automatic, manual, or first-come-first-served. It does require that ownership never becomes ambiguous and never overlaps.

## Message Classes And Validation Boundary

The exact transport can be websocket, HTTP post, or another local channel later, but the control protocol should preserve a small stable set of message classes:

- session/auth state messages,
- ownership acquire/release messages,
- keyboard key event messages,
- keyboard hotkey messages,
- text paste messages,
- relative mouse move messages,
- mouse button and wheel messages,
- emergency release messages,
- ATX or power action messages.

Each accepted message must pass all of these checks before `hid_device` sees it:

1. Session exists and is transport-alive.
2. Session is authenticated if the message is control-bearing.
3. Session currently owns control if the message is keyboard, mouse, paste, hotkey, or ATX.
4. Message type is known and allowed in the current control state.
5. Message fields are structurally valid and within bounded size/range limits.
6. Ordering is safe enough to avoid impossible state, duplicate owner transitions, or queue explosion.

Protocol error categories should distinguish at least:

- `unknown_message_type`
- `invalid_shape`
- `not_authenticated`
- `not_owner`
- `control_disabled`
- `queue_overflow`
- `range_violation`
- `stale_or_out_of_order`
- `internal_reject`

Illustrative message envelope:

```json
{
  "message_type": "keyboard.key",
  "session_id": "opaque-session-id",
  "sequence": 42,
  "sent_at_ms": 123456,
  "payload": {}
}
```

This envelope is illustrative only. The contract is the validation boundary and message class separation, not the literal wire schema.

## Release Semantics

Forced release behavior is the heart of the safety spec.

### Mandatory release triggers

WatchPup must force release on all of these conditions:

- controller disconnect,
- controller heartbeat timeout or liveness timeout,
- ownership loss,
- auth loss,
- protocol error severe enough to invalidate trust in current control state,
- explicit emergency release.

### Forced release behavior

On forced release, the device must:

1. release all currently pressed keyboard keys,
2. release all currently pressed mouse buttons,
3. zero any pending relative mouse movement or wheel deltas not yet emitted,
4. discard queued text-paste or queued key events that have not reached `hid_device`,
5. mark the control session as no longer active,
6. record the release reason and increment release counters,
7. reject further control-bearing input until control is re-enabled and ownership is valid again.

Release must be idempotent. Repeating the same forced-release action should not recreate presses or produce inconsistent state.

### Release reason vocabulary

Use stable release reason tokens:

- `disconnect`
- `timeout`
- `ownership_lost`
- `auth_lost`
- `protocol_error`
- `emergency_release`
- `subsystem_not_ready`
- `operator_disable`

Additional reasons may be added later, but existing reason meanings should remain stable.

## Liveness, Timeouts, And Heartbeats

The control path must not depend on the browser behaving perfectly.

Rules:

- `control_protocol` must track whether the active controller transport is still alive.
- If transport-level disconnect is reliable, it may be sufficient for one release trigger, but a bounded liveness timeout is still required in case the session stalls without a clean close.
- The timeout budget should be implementation-defined later, but it must be short enough to reduce stuck-input risk and long enough to avoid routine false release during normal LAN jitter.
- Heartbeat or liveness messages are allowed as a transport detail, but loss of heartbeat is a control safety event, not merely a UI warning.

## Input Surface For V1

### Keyboard

V1 keyboard rules:

- Support US keyboard mapping first.
- Support explicit key press and key release semantics rather than implicit toggles.
- Support explicit hotkey requests for common system combos that deserve clear intent, such as `ctrl_alt_del`.
- Do not claim general multi-layout support in v1.
- Reject unsupported key identifiers rather than guessing.

Hotkey policy:

- Hotkeys should be named and explicit.
- Hotkeys still require controller ownership and auth.
- A hotkey expands to a bounded internal key sequence inside `control_protocol` or `hid_device`; the browser should not need to guess precise timing.

### Text paste

Text paste is allowed only as queued key events.

Rules:

- Text paste is not a privileged clipboard-sync channel.
- The implementation must expand text into a bounded sequence of keyboard events using the v1 US layout assumptions.
- Pasted input must enter the same bounded queue and pressure rules as other keyboard input.
- Very large paste requests must be rejected or chunked by policy rather than creating unbounded queue growth.
- If forced release occurs, unsent queued paste content must be discarded.

### Mouse

V1 mouse rules:

- Support relative BIOS-compatible pointer mode first.
- Support button press and release events.
- Support wheel deltas only if they fit the same bounded relative-event model.
- Do not implement absolute or tablet mode in this spec.

Relative-mode rules:

- Relative motion is authoritative in v1 because BIOS screens and pre-OS environments are the primary target.
- Any queued relative movement must be zeroed on forced release.
- Validation must clamp or reject out-of-range deltas before they reach `hid_device`.

## Input Queue And Backpressure Policy

The first control implementation should use bounded queues and fail-safe rejection.

Rules:

- Control input queues must have fixed capacity.
- Queue pressure must be observable in diagnostics.
- When the queue is full or near full, WatchPup should reject or shed new control input rather than allowing unbounded latency or memory growth.
- Text paste must respect stricter bounds than single key or small mouse inputs because it can expand rapidly.
- Queue overflow or queue corruption that affects trust in current state must trigger forced release.

Recommended counters:

- accepted messages,
- rejected messages,
- queue depth current,
- queue depth high-water mark,
- queue overflow count,
- dropped paste events,
- forced release count.

## Illustrative Internal Interface Boundary

These interface sketches are illustrative. They define intent, not required C signatures.

```c
typedef enum control_release_reason {
    CONTROL_RELEASE_NONE = 0,
    CONTROL_RELEASE_DISCONNECT,
    CONTROL_RELEASE_TIMEOUT,
    CONTROL_RELEASE_OWNERSHIP_LOST,
    CONTROL_RELEASE_AUTH_LOST,
    CONTROL_RELEASE_PROTOCOL_ERROR,
    CONTROL_RELEASE_EMERGENCY,
    CONTROL_RELEASE_SUBSYSTEM_NOT_READY,
    CONTROL_RELEASE_OPERATOR_DISABLE
} control_release_reason_t;

typedef enum control_ownership_state {
    CONTROL_OWNERSHIP_DISABLED = 0,
    CONTROL_OWNERSHIP_IDLE,
    CONTROL_OWNERSHIP_PENDING_GRANT,
    CONTROL_OWNERSHIP_ACTIVE,
    CONTROL_OWNERSHIP_RELEASING,
    CONTROL_OWNERSHIP_BLOCKED
} control_ownership_state_t;

typedef struct control_session_id {
    uint64_t value;
} control_session_id_t;

typedef struct control_owner_snapshot {
    bool control_enabled;
    bool authenticated;
    bool owns_control;
    control_session_id_t active_controller_id;
    control_ownership_state_t ownership_state;
} control_owner_snapshot_t;

typedef struct control_protocol_stats {
    uint64_t messages_received;
    uint64_t messages_rejected;
    uint64_t protocol_errors;
    uint64_t forced_release_count;
    uint64_t queue_overflow_count;
    uint32_t input_queue_depth;
    uint32_t input_queue_high_water;
    control_release_reason_t last_release_reason;
} control_protocol_stats_t;

bool control_protocol_try_accept_message(control_protocol_handle_t *protocol,
    const control_message_t *message,
    control_owner_snapshot_t *out_snapshot);

void control_protocol_force_release(control_protocol_handle_t *protocol,
    control_release_reason_t reason);

void hid_device_release_all(hid_device_handle_t *hid);
```

Contract rules:

- `control_protocol_force_release` must be callable from disconnect, timeout, auth, and protocol-error paths.
- `hid_device_release_all` must be safe to call redundantly.
- `control_protocol_try_accept_message` must reject control-bearing input from non-owners even if the transport is otherwise alive.

## Diagnostics Requirements

This spec depends on diagnostics-first visibility. Later implementation tickets should populate `/diag` and serial logs with enough information to debug ownership, enablement, and release behavior without a debugger.

### Required `control_protocol` diagnostics

Expose at least:

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

### Required `hid_device` diagnostics

Expose at least:

- `initialized`
- `tinyusb_ready`
- `keyboard_ready`
- `mouse_ready`
- `active_output_owner`
- `keys_stuck`
- `buttons_stuck`
- `forced_release_count`
- `last_release_reason`

### Required `auth_security` diagnostics

Expose at least:

- `auth_required`
- `auth_backend`
- `session_count`
- `authenticated_session_count`
- `failed_auth_count`
- `lockout_active`
- `unsafe_lab_mode`
- `last_auth_failure_ms`

### `/diag` mapping guidance

When this contract is implemented in [Diagnostics Endpoint Contract](./diagnostics-endpoint-contract.md), prefer these fields under `subsystems`:

- `control_protocol.details.input_control_enabled`
- `control_protocol.details.active_controller_id`
- `control_protocol.details.ownership_state`
- `control_protocol.details.messages_received`
- `control_protocol.details.messages_rejected`
- `control_protocol.details.protocol_errors`
- `control_protocol.details.last_protocol_error`
- `control_protocol.details.last_owner_change_ms`
- `control_protocol.details.forced_release_count`
- `control_protocol.details.last_release_reason`
- `control_protocol.details.input_queue_depth`
- `control_protocol.details.input_queue_high_water`
- `control_protocol.details.queue_overflow_count`
- `hid_device.details.active_output_owner`
- `hid_device.details.keys_stuck`
- `hid_device.details.buttons_stuck`
- `hid_device.details.forced_release_count`
- `hid_device.details.last_release_reason`
- `auth_security.details.auth_required`
- `auth_security.details.lockout_active`
- `auth_security.details.unsafe_lab_mode`

If control is not yet implemented, keep the placeholder subsystem objects present and use the existing `unavailable` convention rather than omitting them.

### Serial diagnostics expectations

Serial logs should make control safety events easy to grep.

Recommended event tokens:

- `control.owner.request`
- `control.owner.granted`
- `control.owner.denied`
- `control.owner.released`
- `control.release.run`
- `control.protocol.reject`
- `control.queue.pressure`

Illustrative lines:

```text
[diag] control.owner.granted session=17 ownership_state=active
[diag] control.release.run reason=timeout forced_release_count=3
[diag] control.protocol.reject session=17 error=not_owner
```

## ATX And Power Actions

ATX and power actions are part of the same trust boundary but are not equivalent to keyboard or mouse events.

Rules:

- Power actions require authenticated control-path authorization.
- Power actions should remain separately authorized or separately confirmed from normal keyboard and mouse traffic.
- A future implementation may share the same ownership session, but it must still treat power actions as a distinct command class with explicit intent.
- Browser convenience must not weaken the safety boundary around power control.

This spec does not define the exact confirmation UX or power pulse timing. It only fixes the security posture so later ATX implementation tickets do not silently treat power actions as ordinary HID input.

## Non-Goals And Deferred Questions

This spec intentionally leaves these decisions for later tickets:

- exact wire format and route shape for control messages,
- exact auth backend and credential model,
- exact ownership grant UX or arbitration policy,
- exact queue sizes and timeout values,
- HID report packing and TinyUSB implementation details,
- broader keyboard layouts,
- absolute pointer mode,
- macro systems beyond explicit hotkeys,
- full power-control contract details.

## Acceptance Check

This spec satisfies the HID/control policy from [ADR 0005](../adrs/0005-web-assets-and-hid-control-safety.md) and the v1 scope in the [product PRD outline](../prds/watchpup-product-prd-outline.md) by defining:

- viewing versus control separation,
- mandatory auth before HID/control enablement,
- single active controller ownership,
- browser-to-device control flow through `control_protocol` before `hid_device`,
- forced-release behavior for disconnect, timeout, ownership loss, auth loss, protocol error, and emergency release,
- v1 input limits for US keyboard, explicit hotkeys, queued text paste, and relative BIOS-compatible mouse mode,
- diagnostics fields for ownership, enablement, releases, errors, and queue pressure, and
- high-level ATX authorization expectations,

without implementing HID reports, TinyUSB behavior, or a concrete auth backend in this issue.
