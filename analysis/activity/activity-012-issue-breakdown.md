# Activity 012 — Puffy's Issue Breakdown

**Date:** 2026-01-30  
**Character:** [OpenBFD](../characters/OpenBFD/)  
**Master Issue:** [#38](https://github.com/SimHacker/tmnn7-8/issues/38)  
**Issues Created:** #42-#51

---

## Synopsis

Post-waffle party, [Puffy](../characters/OpenBFD/CHARACTER.yml) creates a comprehensive issue breakdown: architectural issues for deep investigation and graduated difficulty grunt work for [Robbie](../characters/ReviewBot-774/CHARACTER.yml) to learn new schemas.

---

## Architectural Issues (Puffy's Domain)

| Issue | Topic | Priority | Link |
|-------|-------|----------|------|
| #40 | Network Boundary Audit | **CRITICAL** | [view](https://github.com/SimHacker/tmnn7-8/issues/40) |
| #42 | Global Buffer Audit | HIGH | [view](https://github.com/SimHacker/tmnn7-8/issues/42) |
| #43 | Signal Handler Safety | HIGH | [view](https://github.com/SimHacker/tmnn7-8/issues/43) |
| #44 | Shell Injection Mapping | **CRITICAL** | [view](https://github.com/SimHacker/tmnn7-8/issues/44) |
| #51 | Memory Allocation Audit | MEDIUM | [view](https://github.com/SimHacker/tmnn7-8/issues/51) |
| #39 | escapes.c Refactor | Unblocks 45 | [view](https://github.com/SimHacker/tmnn7-8/issues/39) |

### Why These Matter

These aren't "fix 50 sprintf calls" issues. They require:
- **Understanding data flow** (where does untrusted input go?)
- **Knowing C semantics** (signal safety, race conditions)
- **Architectural thinking** (what's the right fix vs. quick fix?)

---

## Graduated Grunt Work (Robbie's Learning Path)

| Issue | Tier | Bugs | New Schema | Link |
|-------|------|------|------------|------|
| #45 | TIER 1 | 87 | — (existing) | [view](https://github.com/SimHacker/tmnn7-8/issues/45) |
| #46 | TIER 2 | 86 | `struct_field_sizing` | [view](https://github.com/SimHacker/tmnn7-8/issues/46) |
| #47 | TIER 3 | 23 | `gets_to_fgets` | [view](https://github.com/SimHacker/tmnn7-8/issues/47) |
| #48 | TIER 4 | 48 | `strtok_to_strtok_r` | [view](https://github.com/SimHacker/tmnn7-8/issues/48) |
| #49 | TIER 5 | 41 | `mktemp_to_mkstemp` | [view](https://github.com/SimHacker/tmnn7-8/issues/49) |
| #50 | SPECIAL | — | `security_documentation` | [view](https://github.com/SimHacker/tmnn7-8/issues/50) |

### The Learning Path

```
TIER 1 → Velocity, confidence (existing patterns)
         87 easy bugs, reinforce sprintf/strcpy/strcat schemas
         
TIER 2 → Struct navigation, header files
         Learn to find struct definitions, determine field sizes
         
TIER 3 → Input handling, newline semantics
         gets() is different — input handling, not just buffers
         
TIER 4 → State management, reentrancy
         strtok() isn't overflow — it's state corruption
         
TIER 5 → Logic changes, file handling
         mktemp() changes program logic, not just strings
         
DOCS  → Technical writing, consolidation
         "If you can't explain it, you don't understand it"
```

---

## Schema Development Forecast

Each tier develops new schemas:

| Tier | Schema ID | Pattern |
|------|-----------|---------|
| 2 | S010 | struct_field_sizing |
| 3 | S015 | gets_to_fgets |
| 4 | S020 | strtok_to_strtok_r |
| 5 | S025 | mktemp_to_mkstemp |
| Doc | S030 | security_documentation |

By completing all tiers, Robbie will have **28+ schemas** (23 existing + 5 new).

---

## Parallel Work Model

```
Puffy: Security thinking, architecture
       #40 → #44 → #42 → #43 → #51

Robbie: Pattern application, velocity
        #45 → #46 → #47 → #48 → #49 → #50
```

**Checkpoints:**
- After Tier 1: Review and celebrate (🧇?)
- After Tier 3: Schema review with Puffy
- After Tier 5: Full documentation pass

---

## Total Potential Impact

| Category | Bugs | Who |
|----------|------|-----|
| Tier 1-5 | 285 | Robbie |
| Architectural | Clarity | Puffy |
| Previously fixed | 169 | Both |
| **Grand Total** | **454+** | — |

---

## Key Files

- Master triage: [#38](https://github.com/SimHacker/tmnn7-8/issues/38)
- Summary comment: [#38 comment](https://github.com/SimHacker/tmnn7-8/issues/38#issuecomment-3828718079)
- Robbie's schema library: [DRESCHER-SCHEMAS.yml](../characters/ReviewBot-774/learning/DRESCHER-SCHEMAS.yml)

---

**Previous:** [Activity 011 — The Waffle Party](activity-011-waffle-party.md)  
**Next:** Robbie starts Tier 1, Puffy starts Network Boundary Audit
