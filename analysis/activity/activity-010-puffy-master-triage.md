# Activity 010 — Puffy's Master Triage

**Date:** 2026-01-30  
**Character:** [OpenBFD](../characters/OpenBFD/)  
**Issues:** [#38](https://github.com/SimHacker/tmnn7-8/issues/38) (master), [#39](https://github.com/SimHacker/tmnn7-8/issues/39) (escapes.c), [#40](https://github.com/SimHacker/tmnn7-8/issues/40) (network audit)  
**Role:** Architectural planning

---

## Synopsis

After [Safari #2](activity-008-safari-2-rhino-rampage.md)'s success (169 bugs), [Puffy](../characters/OpenBFD/CHARACTER.yml) steps back to triage the remaining ~605+ vulnerabilities and 324+ systemic issues. The work is broken into "edible bites."

## The Numbers

```yaml
string_safety_remaining: ~480
systemic_issues:
  static_buffers: 53
  strtok_unsafe: 48
  shell_injection: 58
  mktemp_race: 41
  signal_handlers: 124
total_systemic: 324+
```

## Puffy's Categorization

### String Safety Tiers

| Tier | Complexity | Files | Bugs | Who |
|------|------------|-------|------|-----|
| 1 | LOW | 9 | 87 | [Robbie](../characters/ReviewBot-774/) |
| 2 | MEDIUM | 7 | 86 | Robbie + guidance |
| 3 | HARD | 10 | 256 | Puffy leads |

### Hard Files (Puffy's Domain)

| File | Count | Why Hard | Link |
|------|-------|----------|------|
| escapes.c | 45 | Buffer from callers | [view](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.news/escapes.c) |
| rfuncs.c | 43 | Deep call chains | [view](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.read/rfuncs.c) |
| postnews.c | 26 | User input | [view](https://github.com/SimHacker/tmnn7-8/blob/main/src/postnews.c) |
| transmit.c | 25 | Network boundary | [view](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.priv/transmit.c) |
| post.c | 22 | Posting engine | [view](https://github.com/SimHacker/tmnn7-8/blob/main/src/post.c) |
| nntpread.c | 12 | Network protocol | [view](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.network/nntpread.c) |

## Issues Created

| Issue | Title | Priority |
|-------|-------|----------|
| [#38](https://github.com/SimHacker/tmnn7-8/issues/38) | Master triage (roadmap) | Overview |
| [#39](https://github.com/SimHacker/tmnn7-8/issues/39) | escapes.c refactor | Unblocks 45 |
| [#40](https://github.com/SimHacker/tmnn7-8/issues/40) | Network boundary audit | **CRITICAL** |

## The Edible Bite Definition

```yaml
edible_bite:
  max_bugs: 30
  max_files: 3
  max_time: "one session"
  reviewable: true
  revertible: true
  
  rule: |
    If it takes more than one session,
    it's not an edible bite.
```

## Sprint Plan

1. **Puffy**: Network boundary audit ([#40](https://github.com/SimHacker/tmnn7-8/issues/40))
2. **Robbie**: Tier 1 bulldozer (87 bugs)
3. **Puffy**: escapes.c refactor ([#39](https://github.com/SimHacker/tmnn7-8/issues/39))
4. **Robbie**: Network files (after audit)
5. **Joint**: System/popen mapping
6. **Robbie**: mktemp, strtok mechanical

## Key Insight

> "Not all bugs are equal. sprintf overflow in local tool = bad. sprintf overflow in network code = CVE."

Puffy's job is to identify which bugs are **remote code execution** before [Robbie](../characters/ReviewBot-774/) bulldozes.

## The Parallel Model

```
Puffy: Security thinking, architecture
Robbie: Pattern application, velocity

Neither can do the other's job.
Both together > either alone.
```

## Key Files

- **Master triage comment:** [#38 comment](https://github.com/SimHacker/tmnn7-8/issues/38#issuecomment-3828705591)
- **Source directory:** [`src/`](https://github.com/SimHacker/tmnn7-8/tree/main/src)
- **Character profiles:** [characters/](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters)

---

**Previous:** [Activity 009 — Drescher Schema Factory](activity-009-drescher-schema-factory.md)  
**Next:** Puffy starts network boundary audit ([#40](https://github.com/SimHacker/tmnn7-8/issues/40))
