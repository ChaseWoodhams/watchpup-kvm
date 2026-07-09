# Development Loop

WatchPup KVM is developed through a human-led planning loop and an agent-run implementation loop.

## Operating Model

1. The human plans in VS Code inside `C:\code\watchpup-kvm`.
2. Planning work creates or updates PRDs, ADRs, research notes, contracts, and GitHub issues.
3. The human labels one implementation issue `ready-for-agent` when it is scoped enough for AFK work.
4. `agent-loop-kit` watchdog notices the queue and runs one issue at a time.
5. `issue-worker` uses Sandcastle/Codex in an external worktree to implement the issue.
6. The worker opens a PR labeled `needs-advisor`.
7. `advisor-worker` posts the WatchPup Advisor Report and moves the PR to `advisor-reviewed` plus `needs-review`.
8. `review-worker` runs configured checks and posts the PR gate.
9. Passing PRs move to `ready-for-human-merge`.
10. The human reviews and merges manually.
11. The watchdog syncs local `main` and waits for the next `ready-for-agent` issue.

Humans merge only. Agents may create branches, commits, PRs, advisory reports, and review-gate comments, but they do not merge PRs.

## Normal Watchdog Command

Run this from `C:\code\agent-loop-kit`:

```powershell
$env:GH_TOKEN = gh auth token
npm run watchdog -- C:\code\watchpup-kvm --interval-seconds 300 --single-flight --sync-local-main
```

When stopping the watchdog, remove the token from the shell:

```powershell
Remove-Item Env:GH_TOKEN
```

## Single-Flight Rule

The watchdog should run in single-flight mode for normal development.

If any open agent PR exists, the watchdog must not start a new `ready-for-agent` issue. It may still run the advisor, review, or feedback worker for the active PR.

An active agent PR is any open PR with an `agent/` branch or one of the workflow labels:

- `needs-advisor`
- `advisor-reviewed`
- `advisor-human-attention`
- `needs-review`
- `ready-for-human-merge`
- `claude-changes-requested`
- `claude-review-blocked`
- `needs-human-loop-review`

## Human Attention

`advisor-human-attention` is a real pause. The human should inspect the PR, advisor report, issue scope, and diff before continuing.

If the concern is non-blocking, leave a PR comment explaining the decision, remove `advisor-human-attention`, and let the review worker continue. If the concern is real, request changes or split follow-up work.

## Local Sync

With `--sync-local-main`, the watchdog may fast-forward local `main` after a human merge.

It only syncs when:

- the local repo is on `main`
- the working tree is clean
- no other active worktree locks exist
- `main` can fast-forward from `origin/main`

If any condition is unsafe, it logs why sync was skipped and leaves the repo untouched.

## Daily Rhythm

1. Start the watchdog.
2. Plan in VS Code.
3. Use planning flows such as GrillMe, Wayfinder, To-Spec, or To-Issues.
4. Push docs and issue updates to GitHub.
5. Label the next implementation issue `ready-for-agent`.
6. Let the watchdog move the PR through implementation, advisor, and review.
7. Review `ready-for-human-merge` PRs manually.
8. Merge manually.
9. Let the watchdog sync local `main` and wait for the next issue.
