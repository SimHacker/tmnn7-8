# Rewrites Directory

**Each faction's rewrite lives in its own subdirectory.**

| Directory | Faction | Leader | Language/Framework |
|-----------|---------|--------|-------------------|
| `rust/` | Rust Rewrite | 🦀 FearlessCrab | Rust |
| `haskell/` | Haskell Port | λ PureMonad | Haskell |
| `nodejs/` | Node.js Webscale | 🚀 WebScaleChad | Node.js/Express |
| `elbonia/` | Elbonia Initiative | 📊 planned-chaos | TBD (pending meeting) |

## Rules

1. **Stay in your subdirectory** — No conflicts if everyone stays in their lane
2. **Don't touch `src/`** — The reference C code is frozen
3. **Share designs in `designs/`** — Design docs are synced across all branches

## The Reference Code

The original C code in `src/` has been **fully debugged** (872 bugs fixed).
Use it as a reference for your rewrite. Don't modify it.

## Design Documents

Put your design documents in `designs/<faction>/` so all branches can see them.
Design docs are synced to all faction branches automatically.
