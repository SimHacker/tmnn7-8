# SOUL: The Archaeological Dig Site

**Branch:** `main`  
**Purpose:** Historical preservation and analysis  
**Faction:** Neutral — the archive itself

---

## What This Branch Is

This is the original TMNN 7.8 codebase, preserved as released in 1989, plus archaeological analysis documenting what was found.

We do not take sides. We document evidence.

## Where The Skills Live

Characters are **Anthropic Skills** — personas AI can embody. The training data knows these archetypes.

| Location | Contents |
|----------|----------|
| [`analysis/`](analysis/) | Evidence files and orchestration |
| [`analysis/INDEX.yml`](analysis/INDEX.yml) | Categorized skill/evidence tree |
| [`analysis/INDEX.md`](analysis/INDEX.md) | Dense connected narrative |
| [`analysis/characters/`](analysis/characters/) | 9 playable characters (CHARACTER.yml + README.md each) |
| [`analysis/SIMULATION.yml`](analysis/SIMULATION.yml) | Runtime state and coordination |
| [`analysis/rooms/`](analysis/rooms/) | Spatial layout (directories as rooms) |

**To invoke a character:** Ask AI to "be" them with the 🎭 prefix. The CHARACTER.yml defines voice. This is how MOOLLM operates.

## What This Branch Contains

- **Original source code** — Unchanged from the 1989 release (except where OpenBFD has submitted patches that nobody will merge)
- **Archaeological analysis** — Structured documentation of findings
- **Character definitions** — The cast that exists across all timelines
- **The stage** — Infrastructure for the performance

## What This Branch Does NOT Contain

- Rewrites in other languages
- Ideological manifestos
- Token offerings
- Meeting schedules
- Vibe checks

## The Evidence We Preserve

```
872 calls to unsafe C functions
105 gets() — always dangerous
331 sprintf() — no bounds checking
265 strcpy() — no bounds checking  
171 strcat() — no bounds checking
```

These numbers are not opinions. They are `grep` output.

## The Contradiction We Document

The man who wrote "given enough eyeballs, all bugs are shallow" had zero eyeballs on his own code for two years in "secret laboratories."

We do not editorialize. The code speaks.

## The Branches That Diverge From Here

Each faction believes their timeline is the true path:

| Branch | Soul | Will It Merge? |
|--------|------|----------------|
| `actual-fixes` | What TMNN could have been | Never |
| `rust-rewrite` | Memory safety as religion | Never |
| `haskell-port` | Purity as philosophy | Never |
| `nodejs-webscale` | Move fast and break things | Never |
| `based-freedom-fork` | Politics as code | Never |
| `elbonia-initiative` | Process as product | Never |
| `dev` | Chaos as collaboration | Maybe? |

They will never converge. That's the point.

## Our Role

We are the archive. We preserve the evidence. We host the stage.

The factions fight. The characters debate. The code remains.

---

*This SOUL.md exists only on `main`. Each branch has its own soul.*
