# WatchPup KVM Web Dashboard Design Specification

## Problem

WatchPup KVM needs a clear finished-product vision before its browser interface is implemented. The interface must make remote video, keyboard, mouse, power control, and hardware health understandable without feeling like a developer tool. It must also make dangerous or unavailable actions unmistakable.

This document is a visual and interaction reference for creating mockups. It describes the intended version-one product, not the capabilities of the current bench firmware. The current firmware exposes a read-only diagnostics endpoint; video, input control, authentication, and power controls remain planned work.

## Product Character

WatchPup should feel like a dependable piece of local network equipment: compact, calm, technical, and trustworthy. It should resemble a focused appliance console rather than a general-purpose admin portal.

- Visual tone: dark control-room interface with excellent contrast.
- Personality: alert but not alarming; dense enough for technical users without appearing cluttered.
- Primary use: desktop and laptop browsers on a local network.
- Secondary use: tablet for observation and simple power actions.
- Product promise: the user can understand video, control ownership, network health, and target power state at a glance.

## Visual Direction

### Color system

Use a charcoal background with slightly lighter layered surfaces. Reserve bright colors for state and action.

| Token | Suggested value | Use |
| --- | --- | --- |
| Canvas | `#0B0F14` | Application background |
| Surface | `#121821` | Navigation and panels |
| Raised surface | `#19222D` | Cards, menus, dialogs |
| Primary text | `#F1F5F9` | Titles and important values |
| Secondary text | `#94A3B8` | Labels and supporting copy |
| Accent | `#38BDF8` | Selection, links, focus, normal primary action |
| Healthy | `#34D399` | Connected and healthy states |
| Warning | `#FBBF24` | Degraded states and guarded actions |
| Critical | `#FB7185` | Failure, disconnect, emergency release |

Color must never be the only state indicator. Pair it with an icon and a short label such as “Healthy,” “Degraded,” or “Offline.”

### Typography

Use a modern sans-serif typeface for the interface and a monospaced face for IP addresses, firmware versions, counters, key combinations, and raw diagnostics.

- Page title: 24–28 px, semibold.
- Section title: 16–18 px, semibold.
- Body and controls: 14–16 px.
- Status labels: 12–13 px, medium weight.
- Minimum interactive target: 40 by 40 px.

### Shape and depth

- Panels use 8–12 px corner radii.
- Borders are subtle and more important than shadows.
- Status pills are compact, not decorative.
- Icons use one consistent outlined family.
- Animation is limited to connection progress, live status, and brief feedback.

## Information Architecture

The finished product has four primary destinations.

| Destination | Purpose | Main contents |
| --- | --- | --- |
| Overview | Confirm that the appliance is ready | Device health, target preview, connection summary, quick actions |
| Console | View and operate the target computer | Live video, control ownership, keyboard, mouse, paste, hotkeys, power controls |
| Diagnostics | Investigate hardware or connection trouble | Ethernet, memory, video, USB HID, ATX, counters, reset information |
| Settings | Manage the local appliance | Administrator session, display preferences, device identity, safe maintenance information |

“Console” is the visual and functional center of the product. Input and power tools belong beside the console instead of becoming separate top-level pages.

## Global Application Shell

### Desktop layout

The application uses a narrow left navigation rail, a top status bar, and a flexible content area.

```text
┌──────────────┬──────────────────────────────────────────────────────┐
│ WatchPup     │ watchpup-kvm       Ethernet ●     Admin ▾          │
│              ├──────────────────────────────────────────────────────┤
│ Overview     │                                                      │
│ Console      │                  Page content                        │
│ Diagnostics  │                                                      │
│ Settings     │                                                      │
│              │                                                      │
│ v1.x.x       │                                                      │
└──────────────┴──────────────────────────────────────────────────────┘
```

The top bar always displays appliance identity, Ethernet state, and signed-in state. While viewing the Console, it also displays the current control state: “Viewing,” “You have control,” or “Controlled elsewhere.”

### Responsive behavior

- Below tablet width, the navigation rail becomes a menu button.
- The Console remains video-first and moves tools below the video.
- Mobile layouts may inspect status but should warn that full keyboard and mouse control requires a larger device.
- No essential action may exist only on hover.

## Screen Specifications

### 1. Sign In

The sign-in screen is deliberately minimal: WatchPup mark, device name, local IP address, password field, and Sign In button. It should explain that authentication is local to this device.

The appliance ships without default credentials. First-time administrator provisioning occurs through the approved setup process, not through an open browser registration screen.

Required states:

- Ready for sign-in.
- Incorrect password without revealing account details.
- Session expired.
- Device unreachable.
- Administrator not yet provisioned, with setup instructions only.

### 2. Overview

The Overview answers one question: “Can I safely use this KVM right now?”

The upper section contains a large readiness card with one plain-language result:

- Ready — video, Ethernet, and input systems are available.
- Limited — viewing works but one or more control systems are unavailable.
- Offline — the appliance or target cannot be reached.

Below it, use a two-column layout:

- Left: a 16:9 target preview with signal status and an “Open Console” action.
- Right: compact cards for appliance health, Ethernet, video input, USB HID, and target power.

The bottom area contains uptime, IP address, firmware version, last reset reason, and a link to full diagnostics. Avoid charts unless a historical data source exists; simple current values are more honest and useful.

### 3. Console

The Console should resemble a focused remote-workstation surface.

```text
┌─────────────────────────────────────────────────────────────────────┐
│ Target Console                  ● Live   Viewing   Request control  │
├───────────────────────────────────────────────┬─────────────────────┤
│                                               │ Session             │
│                                               │ Viewer / Controller │
│              16:9 VIDEO AREA                  ├─────────────────────┤
│                                               │ Input tools         │
│                                               │ Paste text          │
│                                               │ Send hotkey         │
│                                               ├─────────────────────┤
│                                               │ Target power        │
├───────────────────────────────────────────────┤ Power / Reset       │
│  1280×720  30 fps  18 ms      Fit ▾  Fullscreen│                    │
└───────────────────────────────────────────────┴─────────────────────┘
```

#### Video area

- Default aspect ratio is 16:9 with a black matte around mismatched sources.
- The image is the largest element on the screen.
- A small bottom strip shows resolution, frame rate, estimated latency, scaling mode, and fullscreen.
- Pointer capture is visually explicit. The border changes and a temporary message explains how to release it.
- No-signal and reconnecting states appear inside the video area without rearranging the page.

#### Control ownership

Viewing and controlling are separate concepts. Many sessions may view, but only one authenticated session may control input.

- “Viewing” uses a neutral status and a Request Control button.
- “You have control” uses a healthy status and a Release Control button.
- “Controlled elsewhere” disables input tools and explains why.
- Losing the session or connection immediately changes the visible state and releases held inputs.

An always-visible “Release all keys and mouse buttons” emergency action appears when the user has control. It must be visually distinct from target power actions.

#### Input tools

The side panel includes:

- Keyboard capture on/off.
- Mouse capture on/off with relative-pointer status.
- Paste text field with a visible length limit and send progress.
- Hotkey menu with named actions such as Ctrl+Alt+Delete.
- Emergency input release.

The interface must never imply that a key, paste operation, or mouse command was delivered until the appliance acknowledges it. Long paste operations must be cancelable and bounded.

#### Target power

Power controls occupy a separate, clearly labeled panel. Power, reset, and long-press actions must not resemble ordinary blue interface buttons.

- Short press may use a guarded warning-style action.
- Reset and forced power-off require confirmation describing the physical action.
- Confirmation text names the target device and action.
- Power controls show unavailable when ATX hardware is absent or unhealthy.

### 4. Diagnostics

Diagnostics uses a summary-first layout. The top row contains Overall Health, Uptime, Ethernet, and Memory cards. Below it, subsystem cards show:

- Appliance identity and firmware build.
- Ethernet link, negotiated speed, IP address, and recovery counters.
- Internal memory and PSRAM availability.
- Video input and stream health.
- USB HID connection and forced-release counters.
- ATX controller availability and last action.
- Reset reason and watchdog or recovery information.

Every subsystem uses the same state vocabulary: Healthy, Degraded, Unavailable, or Not Installed. Expandable details may expose raw values. A “Raw JSON” drawer supports developers without making raw data the primary interface.

The page has manual Refresh and optional low-frequency automatic refresh. It must show when values were last updated.

### 5. Settings

Version one Settings should remain intentionally small:

- Device: friendly appliance name and read-only hardware identity.
- Console preferences: video fit, preferred quality, fullscreen behavior, and pointer sensitivity if supported.
- Account: change the single local administrator password and end other sessions.
- About: firmware version, licenses, documentation link, and restart information.

Settings must not imply support for Wi-Fi, cloud accounts, multiple roles, or remote telemetry.

## Shared Components

### Status card

Each status card contains a name, icon, state label, one primary value, one supporting line, and an optional details link. Cards should be comparable at a glance and must not use decorative gauges.

### Confirmation dialog

Dialogs state the exact effect, target, and risk. The safe action receives initial focus. Destructive confirmations never close merely because the user clicks outside the dialog.

### Toast and activity feedback

Toasts confirm completed background actions, not critical ongoing state. Persistent conditions such as disconnection or lost control remain visible in the page. Error messages include a recovery action where one exists.

### Empty and unavailable state

Every unavailable feature explains whether it is disconnected, unsupported by this hardware, disabled, or still initializing. Avoid a generic “Something went wrong” message.

## User Stories

1. As a local administrator, I can sign in and immediately see whether WatchPup is ready.
2. As an observer, I can view the target without accidentally sending input.
3. As an operator, I can explicitly request control and see whether I own it.
4. As an operator, I can capture and release keyboard and relative mouse input predictably.
5. As an operator, I can paste bounded text and send common hotkeys with delivery feedback.
6. As an operator, I can release every held key and mouse button during an input problem.
7. As an administrator, I can perform guarded target power actions without confusing them with appliance actions.
8. As a troubleshooter, I can distinguish Ethernet, memory, video, HID, and ATX failures.
9. As a user on an unstable connection, I can tell whether the product is reconnecting, viewing only, or fully offline.
10. As a prototype designer, I can mock unavailable and degraded states without inventing unsupported features.

## Implementation Decisions

- The browser dashboard is served by the WatchPup appliance and optimized for a local Ethernet connection.
- The first implemented dashboard slice is read-only diagnostics backed by the existing diagnostics contract.
- Video viewing and input control remain separate states throughout the interface.
- Only one authenticated session can own keyboard and mouse control at a time.
- Input is forcibly released when ownership, authentication, or connectivity is lost.
- Version-one video is MJPEG, with 720p reliability prioritized over optional 1080p detail.
- Version-one pointer control is relative rather than absolute-tablet positioning.
- A single local administrator model is used; no default credentials are provided.
- Authenticated product routes replace any bench-only open diagnostics access before release.
- Safety-critical UI state must be derived from acknowledged appliance state, not optimistic animation alone.

## Testing Decisions

- Validate the design first at the browser seam using realistic fixtures for healthy, degraded, offline, unauthorized, and reconnecting appliance states.
- Verify that a viewer cannot activate keyboard, mouse, paste, hotkey, or power controls.
- Verify all transitions among Viewing, Control Available, Control Owned, and Controlled Elsewhere.
- Verify emergency input release is reachable in fullscreen and standard layouts.
- Verify disconnect, sign-out, and session expiry visibly revoke control and release inputs.
- Verify destructive power actions require intentional confirmation and cannot be confused with appliance restart.
- Verify diagnostics rendering against missing fields, unknown future fields, stale data, and partial subsystem failure.
- Verify keyboard-only navigation, visible focus, screen-reader labels, contrast, and reduced-motion behavior.
- Verify desktop, tablet, and narrow viewport layouts without hiding essential state.

## Prototype Deliverables

Create these frames before visual refinement:

1. Sign In — normal and error.
2. Overview — ready, limited, and offline.
3. Console — viewer-only.
4. Console — active controller with captured pointer.
5. Console — controlled by another session.
6. Console — no video signal and reconnecting.
7. Console — power-action confirmation.
8. Diagnostics — healthy and mixed-failure states.
9. Settings — desktop and narrow layouts.

After those states work as low-fidelity wireframes, apply the color, typography, spacing, and component system in this document.

## Out of Scope for Version One

- H.264 or other advanced video codecs.
- Wi-Fi operation.
- Audio capture or playback.
- Cloud relay, cloud accounts, or internet discovery.
- Multiple users, roles, or permissions.
- Virtual media mounting.
- Absolute tablet-style pointer input.
- Historical analytics and remote telemetry.
- Native mobile applications.

## Further Notes

The dashboard should never hide operational truth for visual cleanliness. A slightly denser interface that clearly shows signal, ownership, health, and risk is preferable to a minimal interface that leaves the user guessing.

The current bench milestone should visually reuse the final Diagnostics language and components. This allows the early read-only page to grow into the finished product without becoming a disposable design.
