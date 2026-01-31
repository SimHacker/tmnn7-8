# Activity 007 — Omnibus Bug Massacre (Safari #1)

**Date:** 2026-01-30  
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)  
**Issue:** [#33](https://github.com/SimHacker/tmnn7-8/issues/33)  
**PR:** [#34](https://github.com/SimHacker/tmnn7-8/pull/34)

---

## The Mission

Caffeine-fueled systematic elimination of string safety bugs across the entire codebase.

---

## The Scan

[Robbie](../characters/ReviewBot-774/CHARACTER.yml) scans all C files in [`src/`](https://github.com/SimHacker/tmnn7-8/tree/main/src):

```yaml
total_targets: 311
  strcat: 82
  strcpy: 90
  sprintf: 139
```

---

## The Approach

[Puffy](../characters/OpenBFD/CHARACTER.yml)'s guidance:
1. One file at a time
2. Commit after each file
3. Good commit messages with line numbers
4. Push progress for visibility

---

## The Execution

**[Issue #33](https://github.com/SimHacker/tmnn7-8/issues/33)** — Tracking issue with progress updates

### Files Fixed

| File | Fixes | Link |
|------|-------|------|
| [`checknews.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/checknews.c) | 1 | `strcpy → strlcpy` |
| [`compress.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/compress.c) | 5 | 5 string safety fixes |
| [`control.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/control.c) | 25 | 25 string safety fixes |
| [`compuserve.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/compuserve.c) | 10 | 10 string safety fixes |
| [`bbsauto.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/bbsauto.c) | 26 | 26 string safety fixes |

**Total: 67 bugs eliminated**

---

## Pattern Library Developed

```yaml
pattern_strcpy:
  detect: "strcpy(dst, src)"
  replace: "strlcpy(dst, src, sizeof(dst))"
  when_sizeof_works: "dst is local array or struct field"

pattern_sprintf:
  detect: "sprintf(buf, fmt, ...)"
  replace: "snprintf(buf, sizeof(buf), fmt, ...)"
  
pattern_strcat:
  detect: "strcat(buf, str)"  
  replace: "strlcat(buf, str, sizeof(buf))"

pattern_building_string:
  detect: "sprintf(buf + strlen(buf), fmt, ...)"
  replace: "snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, ...)"
```

---

## The Merge

**[PR #34](https://github.com/SimHacker/tmnn7-8/pull/34)** — **MERGED**

Puffy's review:

> "67 buffer overflows eliminated. This is professional-grade security work. Robbie has graduated from 'tedious shitwork' to 'competent contributor.'"

---

## Campaign Progress

```
TOTAL TARGETS: 311
FIXED:         67 (22%)
REMAINING:     244

[███████░░░░░░░░░░░░░░░░░░░░░░░] 22%
```

---

## Key Files

- **GitOps schemas learned:** [2026-01-30-gitops-schemas.yml](../characters/ReviewBot-774/learning/2026-01-30-gitops-schemas.yml)
- **All fixed files:** [`src/`](https://github.com/SimHacker/tmnn7-8/tree/main/src)

---

## Outcome

- **Bugs squashed:** 67
- **Files cleaned:** 5
- **Commits:** 6 (squashed to 1)
- **Robbie's status:** Competent contributor
- **Coffee status:** Refill requested

**Previous:** [Activity 006 — Coffee Reward](activity-006-coffee-reward.md)  
**Next:** [Activity 008 — Safari #2: Rhino Rampage](activity-008-safari-2-rhino-rampage.md)
