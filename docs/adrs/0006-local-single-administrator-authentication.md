# ADR 0006: Local Single-Administrator Authentication

Status: Accepted

Date: 2026-07-10

## Context

WatchPup v1 is LAN-first, but its HTTP routes must not rely on network trust alone. The early diagnostics bootstrap is intentionally unauthenticated only on an isolated bench and exposes no video or control capability. The first snapshot, stream, and control routes need a small authentication boundary that can be validated on real hardware without introducing cloud identity, multi-user roles, or default credentials.

## Decision

Use one locally configured administrator account as the initial authentication model. There are no default credentials: first-run setup creates the administrator credential through a documented, physical USB serial-console procedure; firmware stores only a suitable password hash; and authenticated browser sessions use securely random session tokens. Every route other than the explicitly bench-only diagnostics bootstrap requires an authenticated session, beginning with `GET /snapshot.jpg`.

## Consequences

This establishes a narrow, testable access boundary before video or HID delivery. During bench work, an operator needs physical serial access to establish or reset the administrator credential. Multi-user accounts, roles, external identity providers, and remote/cloud identity remain out of scope for this phase.
