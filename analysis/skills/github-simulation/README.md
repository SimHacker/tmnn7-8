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

## See Also

- `github-user` — Characters as GitHub actors
- `code-archaeology` — OpenBFD's audit method
- `SIMULATION.yml` — Runtime state
