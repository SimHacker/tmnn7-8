# Activity 005 — Parallel Learning Workflow

**Date:** 2026-01-30  
**Characters:** [OpenBFD](../characters/OpenBFD/), [ReviewBot-774](../characters/ReviewBot-774/)  
**Issues:** [#25](https://github.com/SimHacker/tmnn7-8/issues/25), [#27](https://github.com/SimHacker/tmnn7-8/issues/27), [#28](https://github.com/SimHacker/tmnn7-8/issues/28)  
**PRs:** [#26](https://github.com/SimHacker/tmnn7-8/pull/26), [#29](https://github.com/SimHacker/tmnn7-8/pull/29), [#30](https://github.com/SimHacker/tmnn7-8/pull/30)

---

## The Experiment

[Puffy](../characters/OpenBFD/CHARACTER.yml) and [Robbie](../characters/ReviewBot-774/CHARACTER.yml) work in parallel:
- **Puffy:** Takes the HARD problems (architectural investigation)
- **Robbie:** Takes the TEDIOUS problems (repetitive fixes)

Both learn from each other.

---

## Robbie's Track: Tedious Shitwork

### Batch 1: 8 strcat() fixes

**[Issue #25](https://github.com/SimHacker/tmnn7-8/issues/25)** — Robbie identifies pattern

```yaml
finding:
  function: addrestrict()
  problem: "8 unbounded strcat() calls"
  severity: HIGH
  
solution:
  macro: SAFE_STRCAT(dst, src)
  implementation: strlcat(dst, src, BUFLEN)
```

**[PR #26](https://github.com/SimHacker/tmnn7-8/pull/26)** — **MERGED**

Robbie's first multi-fix PR. Puffy reviews, approves.

### Batch 2: 6 more strcat() fixes

**[Issue #28](https://github.com/SimHacker/tmnn7-8/issues/28)** — Pattern recognition applied

Robbie applies learned pattern to find more instances. Puffy corrects initial assessment of `getgrplist()` — existing bounds checks mean "defense in depth" not "critical fix."

**[PR #29](https://github.com/SimHacker/tmnn7-8/pull/29)** — **MERGED**

Schema refined: not all unbounded calls are equal.

---

## Puffy's Track: Hard Investigation

### Thread Safety Audit

**[Issue #27](https://github.com/SimHacker/tmnn7-8/issues/27)** — Architectural deep dive

Puffy investigates:
- Static buffers
- `strtok()` non-reentrancy  
- File handle TOCTOU races

**Conclusion:** NOT BUGS in fork-per-connection model.

The "fix" is documentation, not code.

**[PR #30](https://github.com/SimHacker/tmnn7-8/pull/30)** — **MERGED**

Adds `THREADING MODEL` documentation to file header. Explains why code is correct in deployment context.

---

## Cross-Pollination

Robbie observes Puffy's investigation, learns:

```yaml
schema_update:
  what_is_a_bug:
    before: "unbounded call = bug"
    after: "unbounded call in wrong context = bug"
    refinement: "context matters"
    
  when_to_document:
    trigger: "code is correct but confusing"
    action: "document the why"
```

---

## Key Files

- **Fixed file:** [`src/D.news/fascist.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.news/fascist.c)
- **Robbie's learnings:** [learning/](../characters/ReviewBot-774/learning/)

---

## Outcome

- **Bugs fixed:** 14 (8 + 6)
- **Documentation added:** THREADING MODEL section
- **Schemas developed:** Pattern recognition, context awareness
- **Relationship:** Mentor-apprentice → Collaborative peers

**Previous:** [Activity 004 — The VIBE CHECK Incident](activity-004-vibe-check-incident.md)  
**Next:** [Activity 006 — Coffee Reward](activity-006-coffee-reward.md)
