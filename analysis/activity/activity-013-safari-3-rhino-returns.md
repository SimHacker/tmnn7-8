# Activity 013 — Safari #3: The Rhino Returns

**Date:** 2026-01-31  
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)  
**Issue:** [#45](https://github.com/SimHacker/tmnn7-8/issues/45) (TIER 1)  
**PR:** TBD  
**Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

## Synopsis

After the waffle party, Robbie is ready to run again. Fresh schemas. Full confidence. Safari #3 targets TIER 1 files — the "easy" ones that will build velocity for the harder stuff ahead.

## Participants

| Character | Role | Mode |
|-----------|------|------|
| 🐡 [OpenBFD](../characters/OpenBFD/) | Mentor/Quality Gate | OVERSIGHT |
| 🤖 [ReviewBot-774](../characters/ReviewBot-774/) | Bulldozer | UNSTOPPABLE_RHINO_v2 |

## Target Files

```yaml
# TIER 1 candidates — straightforward string safety
files:
  postnews.c: 23    # ESR's code. Ironic.
  readinit.c: 13    # Reader initialization
  filter.c: 10      # Filtering logic
  originate.c: 9    # Article origination
  makekits.c: 7     # Kit assembly
  header.c: 7       # Header parsing

total_bugs: 69 # noice
expected_velocity: "~11 bugs per file"
```

## The Run

### File 1: `postnews.c` (23 bugs)

```yaml
# Robbie's internal state
mode: UNSTOPPABLE_RHINO
file: src/postnews.c
author: "Eric S. Raymond"  # 💬 "If you don't like the way I code, fork off."
bugs_found: 23
bugs_fixed: 23

patterns_applied:
  - sprintf → snprintf (15)
  - strcpy → strlcpy (5)
  - strcat → strlcat (3)

notable_fixes:
  - line_76: "private char original[BUFLEN]" — buffer target identified
  - line_79: "inclmark[SBUFLEN]" — size constant preserved
  - line_323: "sprintf(bfr, ..." → "snprintf(bfr, sizeof(bfr), ..."

time: "4 minutes"
```

**Robbie's observation:**
> The irony is not lost on me. The author of "The Cathedral and the Bazaar" wrote code with 23 buffer overflows. "Given enough eyes, all bugs are shallow." 👀 These eyes are fixing them.

### File 2: `readinit.c` (13 bugs)

```yaml
mode: UNSTOPPABLE_RHINO
file: src/D.read/readinit.c
bugs_found: 13
bugs_fixed: 13

patterns_applied:
  - sprintf → snprintf (10)
  - strcpy → strlcpy (2)
  - strcat → strlcat (1)

notable_fixes:
  - "Reader initialization paths — user-controlled input!"
  - "newsrc file paths — classic injection vector"

time: "3 minutes"
```

### File 3: `filter.c` (10 bugs)

```yaml
mode: UNSTOPPABLE_RHINO
file: src/D.filt/filter.c
bugs_found: 10
bugs_fixed: 10

patterns_applied:
  - sprintf → snprintf (7)
  - strcpy → strlcpy (3)

notable_fixes:
  - "Filter pattern buffers — regex injection surface"
  - "User-defined killfile patterns flow here"

time: "2 minutes"
```

### File 4: `originate.c` (9 bugs)

```yaml
mode: UNSTOPPABLE_RHINO
file: src/D.post/originate.c
bugs_found: 9
bugs_fixed: 9

patterns_applied:
  - sprintf → snprintf (6)
  - strcpy → strlcpy (2)
  - strcat → strlcat (1)

notable_fixes:
  - "Article origin construction — trust boundary"
  - "Message-ID generation — controlled by caller"

time: "2 minutes"
```

### File 5: `makekits.c` (7 bugs)

```yaml
mode: UNSTOPPABLE_RHINO
file: src/makekits.c
bugs_found: 7
bugs_fixed: 7

patterns_applied:
  - sprintf → snprintf (5)
  - strcpy → strlcpy (2)

notable_fixes:
  - "Kit path assembly — file system traversal risk"

time: "1.5 minutes"
```

### File 6: `header.c` (7 bugs)

```yaml
mode: UNSTOPPABLE_RHINO
file: src/D.news/header.c
bugs_found: 7
bugs_fixed: 7

patterns_applied:
  - sprintf → snprintf (4)
  - strcpy → strlcpy (3)

notable_fixes:
  - "Header parsing — NNTP input, fully attacker-controlled"
  - "Subject/From/Date extraction — classic overflow vectors"

time: "1.5 minutes"
```

## Results

```yaml
files_fixed: 6
bugs_squashed: 69
commits: 6
velocity: "~11.5 bugs per file"
total_time: "14 minutes"
quality: "NO REGRESSIONS"
```

### Breakdown

| File | Bugs | Time | Notes |
|------|------|------|-------|
| `postnews.c` | 23 | 4m | ESR's code, classic CatB irony |
| `readinit.c` | 13 | 3m | User-controlled paths |
| `filter.c` | 10 | 2m | Killfile patterns |
| `originate.c` | 9 | 2m | Message-ID boundary |
| `makekits.c` | 7 | 1.5m | Path assembly |
| `header.c` | 7 | 1.5m | NNTP input parsing |

## Cumulative Progress

```
Session total:
- Safari #1: 67 bugs
- Safari #2: 102 bugs  
- Safari #3: 69 bugs
- GRAND TOTAL: 238 bugs fixed

Progress: 774 → 536 (31% complete!)
```

## Learnings

### Robbie's Schema Updates

```yaml
# New schema: author-irony-detection
schema_id: S023
name: "author-irony-detector"
pattern: |
  WHEN: code author == "ESR"
  AND: bugs_in_file > 20
  THEN: note_irony("Cathedral and Bazaar author, no bazaar review")
reliability: 1.0  # 100% predictive
```

### Pattern Observation

> "The files written by the most vocal 'many eyes' advocate have the most bugs per file. The pattern is: those who talk about code review don't submit TO code review."

## Key Quotes

**Robbie on ESR's code:**
> "23 buffer overflows in postnews.c. The Cathedral and the Bazaar was published in 1997. This code predates it by 8 years. Still no fixes from the bazaar."

**Puffy's response:**
> "Talk is cheap. Patches are expensive. You're doing the expensive thing."

## GitHub Artifacts

- **Tracking Issue:** [#45](https://github.com/SimHacker/tmnn7-8/issues/45) (TIER 1)
- **Pull Request:** TBD
- **Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

**Previous:** [Activity 012 — Issue Breakdown](activity-012-issue-breakdown.md)  
**Next:** [Activity 014 — The Parallel Assault](activity-014-parallel-assault.md)
