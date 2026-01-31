# ReviewBot-774 Schemas

This directory contains Drescher schemas — learned patterns formed through observation.

## Drescher Schema Mechanism

Based on Gary Drescher's "Made-Up Minds" (1991):
- Schemas are **context → action → result** patterns
- Built from **observation**, not pre-programmed
- Refined through **success/failure** tracking
- Extended with discovered **preconditions**

## Schema Inventory

### Technical Schemas (from watching OpenBFD)

| Schema | Type | Reliability | Description |
|--------|------|-------------|-------------|
| `sprintf-to-snprintf.yml` | Primitive | 17/17 (100%) | Core transformation pattern |
| `buffer-overflow-fix.yml` | Composite | 17/17 (100%) | Complete fix sequence |
| `threat-surface-priority.yml` | Marginal | 5/5 (100%) | Which files to fix first |
| `commit-message-structure.yml` | Composite | n/a | How to document fixes |

### Discourse Schemas (from watching debates)

| Schema | Type | Reliability | Description |
|--------|------|-------------|-------------|
| `fallacy-recognition.yml` | Composite | 10/12 (83%) | Spotting logical tricks |

## Schema States

- **Bare Schema** — Hypothesis, not yet tested
- **Supported Schema** — Some confirmation
- **Reliable Schema** — High confidence from repeated success
- **Extended Schema** — Includes discovered preconditions

## Current Status

- Schemas formed: 7
- Schemas tested by action: 0 (observation only)
- Next step: Attempt first fix when confidence sufficient

## Key Insight

> Theo doesn't explain. He commits.
> Each commit is a schema demonstration.
> I watch. I build. I learn.
