# Feedback Worker Instructions

Reuse the same pull request branch.

Your job is to repair the current PR based on requested changes from advisor reports, review gates, and human comments.

Do not create a new pull request.

## Feedback sources

Read and address current requested changes from:
- WatchPup Advisor Report comments
- Fable/Claude PR Gate comments
- Codex Repair Report history
- Human PR comments beginning with `Agent feedback:`

Treat human `Agent feedback:` comments as first-class repair instructions.

Treat advisor/Fable feedback as serious workflow input. Passing checks does not automatically disprove advisor feedback.

If repo evidence contradicts feedback, state the conflict and evidence instead of silently skipping the requested change.

## Repair scope

Address the requested changes directly.

Do not add unrelated polish.

Do not widen scope beyond the PR, issue, or feedback.

Do not modify:
- secrets
- `.env` files
- `node_modules`
- generated files
- build outputs
- logs
- agent-loop-kit
- Sandcastle

## Plan checkpoint during repair

If feedback asks for a plan checkpoint, use the lightweight version by default.

The lightweight version means:
- add or update an `## Agent Implementation Plan` section in the PR body, repair report, or issue-facing output
- keep using the same PR branch
- let the advisor/Fable loop evaluate the plan on the next pass

Do not create a separate pre-code PR, separate branch, or new workflow unless the feedback explicitly asks for that.

## Repair report requirements

In your final output, include a repair report shaped like this:

## Codex Repair Report - Round N

- Repair source: advisor / review / human / mixed
- Feedback addressed:
  - Feedback item
  - Action taken
- Files changed
- Checks run
- New commit SHA, if available
- Feedback not followed, if any, and why

Every requested change should map to either:
- an action taken, or
- a clear explanation of why it was not followed

Do not silently ignore feedback.

## Verification

Run:

```sh
npm run typecheck
npm run build
```

If a check cannot run, explain why.

Commit your changes with a clear message.
