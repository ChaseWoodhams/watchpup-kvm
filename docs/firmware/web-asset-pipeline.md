# Web Asset Build And Firmware Embedding Pipeline

Issue: [#27 Define web asset build and firmware embedding pipeline](https://github.com/ChaseWoodhams/watchpup-kvm/issues/27)

Date: 2026-07-09

Related decisions and research:

- [ADR 0005: Web Assets And HID Control Safety](../adrs/0005-web-assets-and-hid-control-safety.md)
- [p4kvm Reference Analysis](../research/p4kvm-reference-analysis.md)
- [Diagnostics Endpoint Contract](./diagnostics-endpoint-contract.md)

## Purpose

This document turns the decision in [ADR 0005](../adrs/0005-web-assets-and-hid-control-safety.md) into a concrete repository layout and build expectation.

It exists to avoid the `p4kvm-reference` pattern of hand-editing or copying generated browser assets into firmware source paths. WatchPup should keep authored browser UI files, generated web build output, and authored firmware source separate from the start.

## Repository Layout

Use these paths:

- `web/`: editable browser UI source
- `firmware/`: authored ESP-IDF project source
- `firmware/generated/web/`: generated browser build output intended for firmware embedding

Rules:

- Files under `web/` are human-authored source.
- Files under `firmware/` are human-authored firmware source unless they are explicitly under `firmware/generated/`.
- Files under `firmware/generated/web/` are build output only.
- Do not hand-edit files under `firmware/generated/web/`.
- Do not copy or rename generated/minified HTML into authored firmware source paths such as `firmware/main/`.

## Build Step

The browser UI should be built with Vite or an equivalently small frontend build step.

This repo now scaffolds Vite in `web/` with:

- source entry at `web/index.html`
- UI code under `web/src/`
- build output directed to `firmware/generated/web/`

At this stage the build contract is:

1. Edit source only in `web/`.
2. Run the web build to generate firmware-ready assets in `firmware/generated/web/`.
3. Have firmware CMake embed those generated files.

## Generated Output Policy

Generated output is ignored by Git at this stage.

That choice follows [ADR 0005](../adrs/0005-web-assets-and-hid-control-safety.md), which says generated assets are build output by default and that any checked-in release artifact policy should be a separate decision.

Current rule:

- Ignore `firmware/generated/web/` in Git.

If release packaging later requires committed generated assets for reproducibility, that should be decided in a dedicated follow-up ADR or issue rather than folded into the normal source layout.

## Firmware Embedding Expectation

The future ESP-IDF firmware project should embed generated files from `firmware/generated/web/` using ESP-IDF CMake embedding support.

Expected posture:

- Embed generated files, not authored `web/` source files.
- Keep embedding paths pointed at `firmware/generated/web/`.
- Treat embedded assets as part of the firmware build graph, not as hand-maintained firmware source.

Illustrative `firmware/main/CMakeLists.txt` shape:

```cmake
set(WATCHPUP_WEB_ASSET_DIR "${CMAKE_CURRENT_LIST_DIR}/../generated/web")

idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    EMBED_FILES
        "${WATCHPUP_WEB_ASSET_DIR}/index.html"
)
```

If the Vite output later includes hashed asset files under `assets/`, either:

- embed the additional files explicitly, or
- adjust the web build to emit a firmware-friendlier asset shape

That follow-up should preserve the same authored-versus-generated separation.

## First UI Milestone

Per [ADR 0005](../adrs/0005-web-assets-and-hid-control-safety.md), the first browser UI milestone is a diagnostics/status page backed by `GET /diag`.

The initial scaffold in `web/` should therefore assume:

- a read-only diagnostics/status page
- no HID control
- no MJPEG stream requirement yet
- polling or fetches against `/diag` JSON once firmware exists

This aligns with the diagnostics-first sequence documented in [Diagnostics Endpoint Contract](./diagnostics-endpoint-contract.md).

## Non-Goals For This Issue

This issue does not:

- implement the ESP-IDF firmware project
- implement `GET /diag`
- decide final production asset hashing or cache headers
- add HID or authenticated control behavior

It only defines the source/build/embed boundary so later issues can implement firmware and UI without mixing generated assets into authored firmware source.
