# MJPEG Frame Ownership Contract

Issue: [#26 Define MJPEG frame ownership interfaces](https://github.com/ChaseWoodhams/watchpup-kvm/issues/26)

Date: 2026-07-09

Related decisions and research:

- [ADR 0004: Frame Ownership And MJPEG Backpressure](../adrs/0004-frame-ownership-and-mjpeg-backpressure.md)
- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)
- [Diagnostics Endpoint Contract](./diagnostics-endpoint-contract.md)

## Purpose

This document defines the first firmware-facing ownership contract for read-only MJPEG video.

It exists to make buffer lifetime, latest-frame publication, and slow-client behavior explicit before any HDMI capture, JPEG encode, or HTTP streaming implementation lands.

This is a contract document, not an implementation ticket. It does not require working CSI capture, JPEG hardware encode, or an HTTP stream handler in this issue.

## Scope Boundary

This contract covers these firmware subsystems only:

- `capture` raw frame ownership and release.
- `encoder_mjpeg` borrowing and encode completion rules.
- `frame_store` published JPEG slot ownership.
- `stream_server` latest-frame borrowing behavior.
- Shared diagnostics counters for the first video milestone.

This contract does not define:

- TC358743 register behavior.
- HDMI signal recovery policy beyond the counters needed to observe drops and pressure.
- HID/control ownership.
- Authentication or browser protocol details.
- Per-client buffering or future transport upgrades such as H.264.

## Decision Summary

The first video milestone uses bounded latest-frame publication:

- `capture` owns a completed raw frame until `encoder_mjpeg` returns it.
- `encoder_mjpeg` borrows one raw frame at a time and writes the JPEG output into one published slot selected from a fixed-capacity `frame_store`.
- `frame_store` publishes only complete JPEG frames and assigns a monotonically increasing generation number to each published frame.
- `stream_server` clients borrow only the latest complete published frame.
- Slow clients skip older generations instead of building per-client frame queues.
- Clients that cannot finish a send within a bounded timeout are disconnected.

## Terminology

Use these terms consistently in code and docs:

- **Raw frame**: One completed capture buffer in the capture pixel format before JPEG encode.
- **Borrow**: Temporary read or write access without ownership transfer.
- **Release**: Explicit notification that the borrower is done and the owner may reuse the resource.
- **Published frame**: A JPEG frame that is complete, visible to readers, and tagged with a generation number.
- **Generation**: Monotonic `uint64_t` counter assigned once per successful publication.
- **Slot**: One fixed-capacity JPEG buffer owned by `frame_store`.
- **Latest-frame streaming**: Each client attempts to send the newest complete generation it can borrow at send time, skipping older unpublished or already superseded generations.

## Top-Level Invariants

These invariants define the contract:

1. At most one subsystem owns reuse rights for a buffer at a time.
2. `capture` raw buffers are never retained by `stream_server`.
3. `encoder_mjpeg` never publishes a partial JPEG frame.
4. `frame_store` never blocks publication on one slow client forever.
5. `stream_server` does not create a per-client frame queue for the first milestone.
6. A published slot cannot be reused until all current borrowers release it.
7. Capture and encode may drop or skip work when bounded resources are exhausted; they must not wait indefinitely on MJPEG clients.

## Ownership Model

### `capture`

`capture` owns the raw CSI frame buffers and decides when a buffer becomes complete and borrowable.

Required behavior:

- Provide raw frames to `encoder_mjpeg` one frame at a time.
- Keep ownership of the raw buffer while it is borrowed.
- Delay buffer reuse until `encoder_mjpeg` explicitly releases the borrow.
- Allow dropping a newly completed raw frame if the encode pipeline is already busy and no safe bounded handoff exists.

Required non-behavior:

- Do not publish raw buffers directly to stream clients.
- Do not retain multiple per-client references.
- Do not let HTTP send speed control raw capture buffer lifetime.

Illustrative interface sketch:

```c
typedef struct capture_raw_frame_view {
    const uint8_t *data;
    size_t data_len;
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    capture_pixel_format_t pixel_format;
    uint64_t sequence;
    uint64_t captured_at_us;
} capture_raw_frame_view_t;

typedef struct capture_borrowed_frame {
    capture_raw_frame_view_t view;
    void *owner_token;
} capture_borrowed_frame_t;

bool capture_try_acquire_completed_frame(capture_handle_t *capture,
    capture_borrowed_frame_t *out_frame);

void capture_release_completed_frame(capture_handle_t *capture,
    capture_borrowed_frame_t *frame);
```

Contract rules:

- `capture_try_acquire_completed_frame` returns `false` when no completed frame is ready.
- `owner_token` is opaque outside `capture`.
- `capture_release_completed_frame` must be called exactly once for every successful acquire.
- After release, the `view.data` pointer is invalid for all borrowers.

### `encoder_mjpeg`

`encoder_mjpeg` borrows one raw frame from `capture`, borrows one writable slot from `frame_store`, performs encode, then either publishes the result or abandons the slot.

Required behavior:

- Treat the raw frame input as borrowed read-only memory.
- Never store a raw frame pointer after release back to `capture`.
- Encode only into a writable slot loaned by `frame_store`.
- Publish only after JPEG length and metadata are final.
- Return explicit success or failure so diagnostics can count encode failures and slot pressure.

Illustrative interface sketch:

```c
typedef struct encoder_mjpeg_request {
    capture_borrowed_frame_t raw_frame;
    uint32_t quality;
} encoder_mjpeg_request_t;

typedef struct encoder_mjpeg_result {
    bool encoded;
    uint32_t encode_time_us;
    size_t jpeg_size_bytes;
    uint64_t published_generation;
    frame_store_drop_reason_t drop_reason;
} encoder_mjpeg_result_t;

bool encoder_mjpeg_encode_latest(encoder_mjpeg_handle_t *encoder,
    frame_store_handle_t *frame_store,
    const encoder_mjpeg_request_t *request,
    encoder_mjpeg_result_t *out_result);
```

Contract rules:

- `encoder_mjpeg_encode_latest` borrows `request->raw_frame` only for the duration of the call.
- The function must release or abandon any borrowed writable slot before returning.
- `published_generation` is valid only when `encoded` is `true`.
- On failure caused by slot exhaustion or bounded publication refusal, `drop_reason` must distinguish that from codec or input failure.

### `frame_store`

`frame_store` owns the fixed set of JPEG slots and all published-frame generation state.

Responsibilities:

- Maintain a small fixed-capacity array of JPEG slots.
- Loan one writable unpublished slot to `encoder_mjpeg`.
- Publish a completed slot atomically as the latest generation.
- Track slot borrower reference counts for stream readers.
- Refuse or drop publication when no slot can be made available within the bounded policy.

Suggested slot state model:

- `free`: not published, no borrowers, writable.
- `encoding`: loaned to `encoder_mjpeg`, not visible to readers.
- `published`: complete JPEG visible to readers and eligible for borrowing.
- `retired`: superseded by a newer publication but still held by one or more borrowers.

Illustrative interface sketch:

```c
typedef enum frame_store_acquire_status {
    FRAME_STORE_ACQUIRE_OK = 0,
    FRAME_STORE_ACQUIRE_NO_FREE_SLOT,
    FRAME_STORE_ACQUIRE_PRESSURE_DROP
} frame_store_acquire_status_t;

typedef enum frame_store_drop_reason {
    FRAME_STORE_DROP_NONE = 0,
    FRAME_STORE_DROP_NO_SLOT,
    FRAME_STORE_DROP_ENCODE_FAILED,
    FRAME_STORE_DROP_PUBLISH_ABORTED
} frame_store_drop_reason_t;

typedef struct frame_store_writable_slot {
    uint8_t *data;
    size_t capacity_bytes;
    uint32_t slot_index;
    void *owner_token;
} frame_store_writable_slot_t;

typedef struct frame_store_published_frame_view {
    const uint8_t *data;
    size_t jpeg_size_bytes;
    uint32_t width;
    uint32_t height;
    uint32_t slot_index;
    uint64_t generation;
    uint64_t encoded_at_us;
    void *owner_token;
} frame_store_published_frame_view_t;

frame_store_acquire_status_t frame_store_try_acquire_writable_slot(
    frame_store_handle_t *store,
    frame_store_writable_slot_t *out_slot);

bool frame_store_publish_slot(frame_store_handle_t *store,
    frame_store_writable_slot_t *slot,
    size_t jpeg_size_bytes,
    uint32_t width,
    uint32_t height,
    uint32_t encode_time_us,
    uint64_t *out_generation);

void frame_store_abandon_writable_slot(frame_store_handle_t *store,
    frame_store_writable_slot_t *slot,
    frame_store_drop_reason_t reason);

bool frame_store_borrow_latest(frame_store_handle_t *store,
    frame_store_published_frame_view_t *out_frame);

void frame_store_release_published(frame_store_handle_t *store,
    frame_store_published_frame_view_t *frame);
```

Contract rules:

- `frame_store_try_acquire_writable_slot` never returns a slot still borrowed by a stream client.
- `frame_store_publish_slot` assigns exactly one new generation number on success.
- Generation numbers are monotonic for the device uptime and must not be reused after slot recycle.
- `frame_store_borrow_latest` increments the published slot reference count before returning.
- `frame_store_release_published` decrements the reference count exactly once for each successful borrow.
- A newer publication may retire an older slot immediately, but the older slot remains readable until its borrower count reaches zero.
- `frame_store` may use an internal lock or critical section, but callers must treat the contract as non-reentrant unless future implementation docs say otherwise.

### `stream_server`

`stream_server` owns HTTP session lifecycle and borrows only published JPEG frames.

Required behavior:

- Borrow the latest complete frame immediately before or during each send attempt.
- Release the borrowed frame after the multipart send attempt completes or fails.
- Track the last generation successfully sent to each client only as a scalar generation number.
- Skip directly to the newest generation when the client falls behind.
- Disconnect clients that exceed a configured send deadline.

Required non-behavior:

- Do not allocate a per-client queue of JPEG frames.
- Do not pin older generations for fairness.
- Do not borrow raw capture buffers.

Illustrative interface sketch:

```c
typedef struct stream_client_state {
    uint32_t client_id;
    uint64_t last_sent_generation;
    uint32_t consecutive_send_failures;
    uint64_t connected_at_us;
} stream_client_state_t;

typedef enum stream_send_outcome {
    STREAM_SEND_IDLE = 0,
    STREAM_SEND_SENT,
    STREAM_SEND_SKIPPED_DUPLICATE,
    STREAM_SEND_SKIPPED_STALE,
    STREAM_SEND_DISCONNECTED_SLOW_CLIENT,
    STREAM_SEND_FAILED
} stream_send_outcome_t;

stream_send_outcome_t stream_server_send_next_latest(
    stream_server_handle_t *server,
    stream_client_state_t *client,
    uint32_t send_timeout_ms);
```

Contract rules:

- If the latest borrowed generation equals `last_sent_generation`, the server may idle or retry later without incrementing a skip counter.
- If the latest generation is newer than `last_sent_generation + 1`, the client has skipped one or more frames and the server should count skipped generations.
- A slow send must result in release of the borrowed frame before client disconnect cleanup completes.
- Session cleanup must not depend on availability of a newer frame.

## Slow-Client Policy

The first milestone explicitly chooses bounded latest-frame delivery over fairness.

Policy rules:

- Each client attempts to send only the latest generation visible at send time.
- If a client misses generations while another frame becomes latest, those generations are skipped and never queued for that client.
- A client that cannot complete a send within the configured timeout is disconnected.
- Disconnect is preferred over unbounded retention of retired slots.

This deliberately avoids per-client frame queues for the first read-only video milestone.

## Publication Sequence

The intended happy-path sequence is:

1. `capture` marks a raw frame complete.
2. `encoder_mjpeg` acquires the raw frame borrow from `capture`.
3. `frame_store` loans one writable slot.
4. `encoder_mjpeg` encodes JPEG bytes into that slot.
5. `frame_store` publishes the slot as the next generation.
6. `encoder_mjpeg` releases the raw frame back to `capture`.
7. `stream_server` borrows the latest published generation when sending to a client.
8. `stream_server` releases the published frame after send completion or failure.
9. `frame_store` recycles retired slots only after borrower count reaches zero.

Failure-path rules:

- If no writable slot is available, encode must fail fast and increment pressure/drop counters.
- If encode fails, the writable slot must be abandoned and the raw frame released.
- If send fails or times out, the borrowed published frame must still be released before disconnect.

## Diagnostics Counters

The first video milestone should expose these counters, either through `/diag` fields, serial diagnostics, or both.

### `capture`

- `frames_completed`
- `frames_dropped_busy`
- `last_completed_sequence`
- `last_completed_ms`

### `encoder_mjpeg`

- `encode_attempts`
- `frames_encoded`
- `encode_failures`
- `last_encode_time_us`
- `max_encode_time_us`
- `last_jpeg_size_bytes`

### `frame_store`

- `published_generation`
- `publish_count`
- `publish_drop_no_slot`
- `slot_pressure_events`
- `slot_borrow_count`
- `slot_peak_in_use`
- `retired_slots_waiting_release`

### `stream_server`

- `active_clients`
- `peak_clients`
- `send_attempts`
- `send_failures`
- `clients_disconnected_slow_send`
- `frames_skipped_total`
- `skipped_generations_total`
- `last_sent_generation`

Counter intent:

- `published_generation` is the current latest generation number, not a count of borrowers.
- `publish_drop_no_slot` counts frames that could not be published because bounded slot ownership could not safely make room.
- `slot_pressure_events` counts moments when all slots were unavailable for immediate encode publication.
- `frames_skipped_total` counts client send cycles where at least one generation was skipped.
- `skipped_generations_total` counts the total number of individual generations skipped across all clients.

## `/diag` Mapping Guidance

When this contract is implemented in firmware diagnostics, keep the existing subsystem framing defined in [Diagnostics Endpoint Contract](./diagnostics-endpoint-contract.md).

Preferred additions under `subsystems`:

- `capture.details.frames_completed`
- `capture.details.frames_dropped_busy`
- `capture.details.last_completed_sequence`
- `encoder_mjpeg.details.encode_attempts`
- `encoder_mjpeg.details.frames_encoded`
- `encoder_mjpeg.details.encode_failures`
- `encoder_mjpeg.details.last_encode_time_us`
- `encoder_mjpeg.details.max_encode_time_us`
- `encoder_mjpeg.details.last_jpeg_size_bytes`
- `frame_store.status`
- `frame_store.implemented`
- `frame_store.last_error`
- `frame_store.details.published_generation`
- `frame_store.details.publish_count`
- `frame_store.details.publish_drop_no_slot`
- `frame_store.details.slot_pressure_events`
- `frame_store.details.retired_slots_waiting_release`
- `stream_server.details.active_mjpeg_clients`
- `stream_server.details.peak_mjpeg_clients`
- `stream_server.details.send_failures`
- `stream_server.details.client_disconnects_slow_consumer`
- `stream_server.details.frames_skipped_total`
- `stream_server.details.skipped_generations_total`
- `stream_server.details.last_sent_generation`

If `frame_store` is not yet implemented when `/diag` first lands, it should still appear as a placeholder subsystem object using the same `unavailable` convention already defined for future subsystems.

## Non-Goals And Deferred Questions

This contract intentionally leaves these decisions for later issues:

- Exact slot count and JPEG capacity sizing.
- Whether capture drops occur before or after raw-frame acquisition when the encoder is busy.
- Whether encode runs inline, in a task, or via a worker queue.
- Multi-core locking details on ESP32-P4.
- Future per-client pacing metrics beyond the counters listed here.
- Any transport other than read-only MJPEG.

## Acceptance Check

This contract satisfies ADR 0004 by defining:

- bounded latest-frame publication,
- explicit raw-frame and JPEG-slot ownership,
- generation-numbered publication,
- latest-frame-only stream borrowing,
- skip-or-disconnect slow-client behavior, and
- diagnostics counters needed to observe pressure and failures,

without implementing capture, encode, or streaming behavior in this issue.
