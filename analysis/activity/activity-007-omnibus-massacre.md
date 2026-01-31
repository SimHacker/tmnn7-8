# Activity 007 — Omnibus Bug Massacre

**Date:** 2026-01-30  
**Characters:** ReviewBot-774, OpenBFD  
**Issue:** #33  
**PR:** #34

---

## The Mission

Caffeine-fueled systematic elimination of string safety bugs across the entire codebase.

---

## The Scan

Robbie scans all C files in `src/`:

```yaml
total_targets: 311
  strcat: 82
  strcpy: 90
  sprintf: 139
```

---

## The Approach

Puffy's guidance:
1. One file at a time
2. Commit after each file
3. Good commit messages with line numbers
4. Push progress for visibility

---

## The Execution

**[Issue #33](https://github.com/SimHacker/tmnn7-8/issues/33)** — Tracking issue

### Files Fixed

| File | Fixes | Commit |
|------|-------|--------|
| checknews.c | 1 | `🤖 checknews.c: strcpy → strlcpy` |
| compress.c | 5 | `🤖 compress.c: 5 string safety fixes` |
| control.c | 25 | `🤖 control.c: 24 string safety fixes` |
| compuserve.c | 10 | `🤖 compuserve.c: 10 string safety fixes` |
| bbsauto.c | 26 | `🤖 bbsauto.c: 26 string safety fixes` |

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

## Outcome

- **Bugs squashed:** 67
- **Files cleaned:** 5
- **Commits:** 6 (squashed to 1)
- **Robbie's status:** Competent contributor
- **Coffee status:** Refill requested
