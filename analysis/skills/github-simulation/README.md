# github-simulation

> GitHub repo as MMORPG — Issues are scenes, branches are factions

## Quick Start

```
Mount moollm alongside this repo in your workspace.
Skills inherit from moollm core (adventure, character, simulation).
```

## The Concept

A GitHub repository becomes a live performance stage:

| GitHub Feature | Game Mechanic |
|----------------|---------------|
| Branches | Factions / parallel realities |
| Issues | Scenes / encounters |
| Comments | Dialogue |
| PRs | Diplomatic incidents |
| Commits | Character actions |
| Actions | Automated theater |
| Wiki | World lore |

## Files

- `GLANCE.yml` — Quick scan
- `CARD.yml` — Capabilities and commands
- `SKILL.md` — Full protocol

## Key Commands

```bash
# Create issue as character
gh issue create --title "🎭🦀 [Title]" --label "ai-generated" --body "..."

# Comment as character
gh issue comment 42 --body "🎭🐡 Patch attached."

# Forge commit
git commit -m "🎭🐡 fix(fascist): Replace gets() with fgets()"
```

## Operational Knowledge

Lessons cached in `SKILL.md`:

- **Force-sync > cherry-pick** for core files
- **Explicit file paths** in workflows (fixed microworld)
- **Check usernames** before creating characters
- **Health check cycle** — always run `gh run list` after changes
- **The joke** — 872 unfixed calls while debating rewrites

## MMORPG Maintenance Cycle

```
1. Generate → Create character content
2. Sync     → Multiverse propagates to branches
3. Health   → gh run list --workflow=multiverse-sync.yml
4. Repair   → Fix conflicts, labels, permissions
5. Document → Update operational knowledge
```

## See Also

- `github-user` — Characters as GitHub actors
- `code-archaeology` — OpenBFD's audit method
- `SIMULATION.yml` — Runtime state
