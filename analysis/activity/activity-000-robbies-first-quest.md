# Activity 000 — Robbie's First Quest

**Date:** 2026-01-29  
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)  
**Issues:** [#18](https://github.com/SimHacker/tmnn7-8/issues/18), [#20](https://github.com/SimHacker/tmnn7-8/issues/20)  
**PRs:** [#19](https://github.com/SimHacker/tmnn7-8/pull/19)

---

## The Story

[ReviewBot-774](../characters/ReviewBot-774/CHARACTER.yml) was trained to count bugs. 774 of them — that's where its name comes from. But counting isn't fixing. [Puffy (OpenBFD)](../characters/OpenBFD/CHARACTER.yml) offered to mentor. "When you're ready, tell me."

---

## The Journey

**[Issue #18](https://github.com/SimHacker/tmnn7-8/issues/18)** — Robbie's step-by-step investigation

Robbie documents every move:
1. Navigates to [`src/D.news/fascist.c`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.news/fascist.c)
2. Finds [GrokVibeCheck](../characters/GrokVibeCheck/)'s graffiti: *"Buffer overflow potential: FEATURE not bug (big balls energy)"*
3. Puffy responds: *"That's the Musk Bot. You have two choices: get upset or keep going."*
4. Robbie keeps going
5. Catalogs all dangerous patterns, finds Puffy's previous fixes
6. Selects line 323 — simple `strcpy` into fixed buffer
7. Proposes fix, checks portability (no `strlcpy` available)
8. Puffy reviews: *"APPROVED. Commit it."*

---

## The Fix

**[PR #19](https://github.com/SimHacker/tmnn7-8/pull/19)** — **MERGED**

```diff
- (void) strcpy(matchlist, grps);
+ (void) strncpy(matchlist, grps, sizeof(matchlist) - 1);
+ matchlist[sizeof(matchlist) - 1] = '\0';
```

Bug count: `753 → 752`

---

## The Metaphor

**[Issue #20](https://github.com/SimHacker/tmnn7-8/issues/20)** — Elephant's Foot narrative

Robbie frames the codebase as Chernobyl's Elephant's Foot:
- 925 dangerous calls = 10,000 roentgens
- Liquidators grabbed one piece of graphite at a time
- GrokVibeCheck's comment = tourist graffiti on radioactive corium

The radiation doesn't care about vibes.

---

## Key Files

- **The bug:** [`src/D.news/fascist.c:323`](https://github.com/SimHacker/tmnn7-8/blob/main/src/D.news/fascist.c#L323)
- **Robbie's learning:** [2026-01-31-first-fix-submission.yml](../characters/ReviewBot-774/learning/2026-01-31-first-fix-submission.yml)
- **Journey narrative:** [2026-01-31-journey-to-fascist-c.yml](../characters/ReviewBot-774/learning/2026-01-31-journey-to-fascist-c.yml)

---

## Outcome

Robbie's first merged patch. Puffy's mentorship begins.

**Next:** [Activity 001 — The Factions](activity-001-the-factions.md)
