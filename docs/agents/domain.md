# Domain Docs

How the engineering skills should consume this repo's domain documentation when exploring the codebase.

## Layout

This is a single-context repo.

- Shared context: `CONTEXT.md`
- PRDs and specs: `docs/prds/`
- ADRs: `docs/adrs/`

## Before Exploring, Read These

- `CONTEXT.md` at the repo root.
- Relevant PRDs or specs under `docs/prds/`.
- Relevant ADRs under `docs/adrs/`.

If any of these files do not exist yet, proceed silently. Do not flag their absence or suggest creating them upfront unless the task is explicitly about planning, domain modeling, or documentation setup.

## Use The Glossary's Vocabulary

When your output names a domain concept in an issue title, refactor proposal, hypothesis, or test name, use the term as defined in `CONTEXT.md`. Do not drift to synonyms the glossary explicitly avoids.

If the concept you need is not in the glossary yet, either reconsider whether the project uses that language or note it as a gap for future domain-modeling work.

## Flag ADR Conflicts

If your output contradicts an existing ADR, surface it explicitly rather than silently overriding it.
