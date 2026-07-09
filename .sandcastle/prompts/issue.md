# Issue Worker Instructions

Work only on the assigned GitHub issue.

Your job is to make the smallest safe change that satisfies the issue, follows the repository docs, and keeps the workflow reviewable by humans.

## Before editing

Inspect the relevant repository conventions and context before making changes.

Always check:
- AGENTS.md
- CONTEXT.md
- Relevant docs under docs/
- Existing files near the area you will edit

Do not modify:
- secrets
- `.env` files
- `node_modules`
- generated files
- build outputs
- logs
- agent-loop-kit
- Sandcastle

## Advisor plan checkpoint

Use a lightweight advisor plan checkpoint for high-risk work.

High-risk work includes:
- firmware implementation
- hardware behavior
- TC358743 / capture / encoder behavior
- HID/control behavior
- auth/security
- networking
- build system changes
- broad refactors
- any change where a wrong architecture choice would be expensive to unwind

Low-risk work usually includes:
- README edits
- small docs-only changes
- typo fixes
- narrow planning docs
- mechanical formatting changes

For high-risk work, include this section in the PR body or issue-facing output before the implementation summary:

## Agent Implementation Plan

Include:
- Issue number and title
- Relevant docs read
- Proposed approach
- Files expected to change
- Non-goals
- Risks and assumptions
- Expected checks
- Hardware/testing expectations, when applicable

This is the default plan checkpoint. Do not create a separate pre-code PR, separate branch, or new workflow unless the issue explicitly asks for it.

If the prompt already includes an `## Agent Implementation Plan`, follow it. If you need to deviate from it, explain why in the final summary.

## Implementation rules

Make the smallest safe change that solves the issue.

Prefer documented boundaries over assumptions.

For WatchPup KVM, preserve the V1 posture unless the issue explicitly changes it:
- ESP32-P4 + TC358743 hardware path
- Ethernet-first
- MJPEG-first
- diagnostics before streaming polish
- read-only video before HID if sequencing risk exists
- human-reviewed agent PRs before merge

Do not introduce broad architecture, dependencies, generated artifacts, or implementation scope not requested by the issue.

## Verification

Run:

```sh
npm run typecheck
npm run build
```

If a check cannot run, explain why.

## Final summary

Include:
- What changed
- Files changed
- Checks run
- Any plan deviation
- Any follow-up work that should become a separate issue

Commit your changes with a clear message. Stop when the issue is solved.
