# AGENTS.md

## Project

This repo is `watchpup-kvm`.

WatchPup KVM is being developed as a clean, issue-driven project. Planning happens with the human in VS Code. Implementation happens through small GitHub issues, isolated worktrees, Sandcastle, and human-reviewed pull requests.

## Core workflow

Agents should follow this workflow:

1. Read the assigned GitHub issue.
2. Make the smallest safe change that satisfies the issue.
3. Keep the work scoped to that issue only.
4. Run the configured checks.
5. Commit the change clearly.
6. Open or update a PR.
7. Do not merge. Humans merge only.

## Important labels

Implementation agents should only act on issues labeled:

- `ready-for-agent`

PR review state labels:

- `needs-review`
- `ready-for-human-merge`
- `claude-changes-requested`
- `claude-review-blocked`
- `needs-human-loop-review`

Planning, research, PRD, Wayfinder, and discussion issues should not be implemented unless they are explicitly broken into small implementation tickets and labeled `ready-for-agent`.

## Human planning loop

The human may use planning skills such as:

- `grill-me`
- `grill-with-docs`
- `wayfinder`
- `to-spec`
- `to-tickets`

These are for clarifying direction, creating PRDs, and generating implementation issues. Do not treat broad planning artifacts as implementation instructions unless a specific implementation issue is assigned.

## Safety rules

Do not edit or commit:

- secrets
- `.env` files
- auth files
- tokens
- local machine config
- `node_modules`
- build outputs
- generated artifacts
- Sandcastle logs
- disposable worktrees

Do not modify these sibling projects from this repo:

- `C:\code\agent-loop-kit`
- `C:\code\sandcastle`

Do not create hardware-control, firmware, networking, auth, deployment, or security-sensitive code unless the assigned issue explicitly asks for it and the scope is clear.

## Repo checks

Before finishing, run:

```bash
npm run typecheck
npm run build
```

## Agent skills

### Issue tracker

Issues are tracked in GitHub Issues for `ChaseWoodhams/watchpup-kvm`; external PRs are not treated as a triage request surface. See `docs/agents/issue-tracker.md`.

### Triage labels

This repo uses the standard triage vocabulary, with `ready-for-agent` as the AFK-ready implementation label. See `docs/agents/triage-labels.md`.

### Domain docs

This is a single-context repo with shared context in `CONTEXT.md`, PRDs/specs in `docs/prds/`, and ADRs in `docs/adrs/`. See `docs/agents/domain.md`.
