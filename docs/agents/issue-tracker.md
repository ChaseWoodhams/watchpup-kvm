# Issue Tracker: GitHub

Issues and PRDs for this repo live as GitHub issues in `ChaseWoodhams/watchpup-kvm`. Use the `gh` CLI for issue tracker operations.

## Conventions

- **Create an issue**: `gh issue create --title "..." --body "..."`. Use a heredoc for multi-line bodies.
- **Read an issue**: `gh issue view <number> --comments`, including labels and relevant comments.
- **List issues**: `gh issue list --state open --json number,title,body,labels,comments` with appropriate `--label` and `--state` filters.
- **Comment on an issue**: `gh issue comment <number> --body "..."`
- **Apply or remove labels**: `gh issue edit <number> --add-label "..."` / `--remove-label "..."`
- **Close an issue**: `gh issue close <number> --comment "..."`

Infer the repo from `git remote -v`; `gh` does this automatically when run inside this clone.

## Pull Requests As A Triage Surface

PRs as a request surface: no.

Do not pull external PRs into the triage queue. Human-created and agent-created PRs should follow the review labels documented in `AGENTS.md` and `.sandcastle/project.config.ts`.

GitHub shares one number space across issues and PRs, so a bare `#42` may be either. Resolve ambiguity with `gh pr view 42` and fall back to `gh issue view 42`.

## When A Skill Says "Publish To The Issue Tracker"

Create a GitHub issue.

## When A Skill Says "Fetch The Relevant Ticket"

Run `gh issue view <number> --comments`.

## Wayfinding Operations

Used by `/wayfinder`. The map is a single issue with child issues as tickets.

- **Map**: a single issue labelled `wayfinder:map`, holding the Notes / Decisions-so-far / Fog body.
- **Child ticket**: an issue linked to the map as a GitHub sub-issue where available. Where sub-issues are unavailable, add the child to a task list in the map body and put `Part of #<map>` at the top of the child body.
- **Blocking**: prefer GitHub's native issue dependencies. Where dependencies are unavailable, fall back to a `Blocked by: #<n>, #<n>` line at the top of the child body.
- **Frontier query**: list the map's open children, drop any with an open blocker or assignee, and take the first in map order.
- **Claim**: `gh issue edit <n> --add-assignee @me`.
- **Resolve**: comment with the answer, close the issue, and append a context pointer to the map's Decisions-so-far.
