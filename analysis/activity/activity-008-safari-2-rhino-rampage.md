# Activity 008 — Safari #2: The Unstoppable Rhino

**Date:** 2026-01-30  
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)  
**Issue:** [#35](https://github.com/SimHacker/tmnn7-8/issues/35)  
**PR:** [#36](https://github.com/SimHacker/tmnn7-8/pull/36) (merged)  
**Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

## Synopsis

After the success of [Safari #1](activity-007-omnibus-massacre.md) (67 bugs), the team launched Safari #2 with [Robbie](../characters/ReviewBot-774/CHARACTER.yml) in "Rhino Mode" — maximum velocity, no stopping.

## Participants

| Character | Role | Mode |
|-----------|------|------|
| 🐡 [OpenBFD](../characters/OpenBFD/) | Mentor/Architect | DEEP_ANALYSIS |
| 🤖 [ReviewBot-774](../characters/ReviewBot-774/) | Bulldozer | UNSTOPPABLE_RHINO |

## Results

```yaml
files_fixed: 8
bugs_squashed: 102
commits: 9
velocity: "~13 bugs per file"
quality: "NO REGRESSIONS"
```

### File Breakdown

| File | Bugs | Link | Notes |
|------|------|------|-------|
| [`inews.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/inews.c) | 16 | [diff](https://github.com/SimHacker/tmnn7-8/pull/36/files) | Clean sweep |
| [`locknews.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/locknews.c) | 17 | | Clean sweep |
| [`ednews.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/ednews.c) | 17 | | Status chain |
| [`expire.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/expire.c) | 12 | | Buffer math |
| [`vrn.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/vrn.c) | 7 | | 6 skipped |
| [`postart.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.post/postart.c) | 12 | | cite struct |
| [`editart.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.post/editart.c) | 12 | | editcmd chain |
| [`filelock.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.priv/filelock.c) | 9 | | **Bug found!** |

### Bug Discovery

At [`filelock.c:196`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.priv/filelock.c#L196), Robbie discovered an original bug:
- Was: `strcpy(bfr, "-LCK.1")` — **overwrites entire path!**
- Should be: `strcat(bfr, "-LCK.1")` — appends suffix
- Fixed: `strlcat(bfr, "-LCK.1", LBUFLEN)`

This bug would have caused lock files named "-LCK.1" instead of "filename-LCK.1", causing lock accumulation.

## Learnings Documented

Robbie created [`2026-01-30-gitops-schemas.yml`](../characters/ReviewBot-774/learning/2026-01-30-gitops-schemas.yml):
- Commit hygiene (one file per commit, line numbers)
- Branch workflow (clean PRs, squash merge)
- Collaboration patterns
- Pattern-to-schema development

## Cumulative Progress

```
Session total:
- Safari #1: 67 bugs
- Safari #2: 102 bugs
- GRAND TOTAL: 169 bugs fixed
```

## Key Quotes

**Robbie on finding stride:**
> "The velocity increase from Safari #1 to #2 isn't just speed — it's confidence. The patterns became automatic."

**Puffy's review:** ([comment](https://github.com/SimHacker/tmnn7-8/pull/36#issuecomment-3828696403))
> "102 fixes. 8 files. No regressions. This is what learning looks like."

## GitHub Artifacts

- **Tracking Issue:** [#35](https://github.com/SimHacker/tmnn7-8/issues/35) (closed)
- **Pull Request:** [#36](https://github.com/SimHacker/tmnn7-8/pull/36) (merged)
- **Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)
- **Refuel Comment:** [#33 comment](https://github.com/SimHacker/tmnn7-8/issues/33#issuecomment-3828675442)

---

**Previous:** [Activity 007 — Omnibus Bug Massacre](activity-007-omnibus-massacre.md)  
**Next:** [Activity 009 — Drescher Schema Factory](activity-009-drescher-schema-factory.md)
