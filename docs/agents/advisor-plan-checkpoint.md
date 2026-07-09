# Advisor Plan Checkpoint

This page explains the lightweight advisor plan checkpoint used for high-risk WatchPup agent work.

The checkpoint exists to keep expensive architectural mistakes visible before implementation moves too far, without introducing a separate planning PR or a second branch.

## When To Use `advisor-plan-required`

Use the `advisor-plan-required` label when an issue includes high-risk work where a wrong implementation direction would be expensive to unwind.

Typical cases include:

- firmware implementation
- hardware behavior
- TC358743, capture, or encoder behavior
- HID or control behavior
- auth or security changes
- networking changes
- build-system changes
- broad refactors

Do not require this label for low-risk work such as README edits, typo fixes, narrow planning docs, or other small docs-only changes unless the workflow is being exercised intentionally.

## Default Checkpoint

The default checkpoint is a visible `## Agent Implementation Plan` in the PR body or in issue-facing comments before the implementation summary.

This is not a separate pre-code PR.

This is not a separate branch or workflow.

The intent is to make the proposed approach reviewable in the same place as the code review, while keeping the implementation scoped to the assigned issue.

## What The Plan Should Include

The `## Agent Implementation Plan` should include:

- issue number and title
- relevant docs read
- proposed approach
- files expected to change
- non-goals
- risks and assumptions
- expected checks
- hardware or testing expectations when applicable

The plan should stay lightweight. It is a checkpoint for direction, not a long-form design document unless the issue explicitly asks for one.

## Workflow

1. `issue-worker` implements the assigned issue and includes `## Agent Implementation Plan` in the PR body or equivalent visible issue-facing output.
2. The PR enters the normal advisor step with `needs-advisor`.
3. `advisor-worker` evaluates whether the plan fits the repo's documented boundaries, issue scope, and risk posture before focusing on code-level details.
4. If the plan is sound, the PR can proceed through the normal review path.
5. If the plan is weak or drifts from repo decisions, the advisor feedback should say so clearly and request correction before human merge.

This keeps the advisor loop visible inside the normal PR flow rather than creating a separate approval stage outside the repo's existing labels.

## Review Verdict

`review-worker` should verify that the implemented change followed the visible plan, not just that checks passed.

Its output should include a `## Plan Followed Verdict` section that states whether:

- the implementation matched the proposed approach
- any material deviations occurred
- any deviation was justified by repo evidence or issue comments

If the implementation deviated from the plan, the review output should call that out explicitly so the human reviewer can decide whether the change still fits issue scope.

## Scope Guardrails

The checkpoint does not authorize broader changes.

It exists to make high-risk work reviewable while preserving WatchPup's normal rules:

- work one assigned issue at a time
- make the smallest safe change
- preserve documented V1 boundaries
- keep human review and merge as the final gate
