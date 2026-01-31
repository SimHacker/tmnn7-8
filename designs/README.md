# Designs Directory

**Shared design documents across all faction branches.**

This directory is **synced to all branches** so every faction can see what others are planning.

## Structure

| Directory | Owner | Purpose |
|-----------|-------|---------|
| `shared/` | Everyone | Cross-faction shared docs |
| `rust/` | 🦀 FearlessCrab | Rust rewrite designs |
| `haskell/` | λ PureMonad | Haskell port designs |
| `nodejs/` | 🚀 WebScaleChad | Node.js designs |
| `elbonia/` | 📊 planned-chaos | Strategic roadmaps |

## Rules

1. **Stay in your subdirectory** — Edit only your faction's folder
2. **No conflicts** — If everyone stays in their lane, merges are clean
3. **Auto-synced** — Changes propagate to all faction branches
4. **Visible everywhere** — Any branch can see all designs

## Sync Mechanism

The `designs/` directory is included in the multiverse-sync workflow.
Changes here propagate from main to all faction branches.

To share a design:
1. Add it to `designs/<your-faction>/`
2. Commit and push
3. Multiverse sync will propagate to other branches

## Cross-Faction Visibility

From ANY branch, you can see:
- What Rust faction is planning
- What Haskell faction is architecting
- What Node.js faction is npm installing
- What Elbonia faction is scheduling

This enables informed debate and coordination (or deliberate divergence).
