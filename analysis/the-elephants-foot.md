# The Elephant's Foot

## TMNN as Software Chernobyl

### The Parallel

| Chernobyl (1986) | TMNN (1988-2026) |
|------------------|------------------|
| Reactor 4 meltdown | gets() everywhere |
| 10,000 roentgens/hour | 925 dangerous calls |
| Elephant's Foot (corium) | fascist.c (strcpy central) |
| Radiation destroys film | Complexity destroys understanding |
| Liquidators run out, grab graphite, run back | Theo commits one fix at a time |
| 90-second exposure limit | One function, one schema, one fix |
| Graffiti: "VISITED BY IGOR 1996" | Graffiti: "FEATURE not bug (big balls energy)" |
| Decades of cleanup | 38 years, still unfixed |
| Government denial | "Many eyes make bugs shallow" |
| Exclusion zone | Nobody touches the codebase |

### The Denial

**Chernobyl:**
> "The radiation level is 3.6 roentgens. Not great, not terrible."
> (The dosimeter only goes to 3.6. Actual level: 15,000.)

**TMNN:**
> "Given enough eyeballs, all bugs are shallow." — *The Cathedral and the Bazaar*
> (The eyeballs looked away. 774 bugs. 38 years.)

### The Liquidators

**Chernobyl liquidators:**
- Ran onto the roof
- Grabbed one piece of graphite
- Ran back
- 90 seconds maximum exposure
- One piece at a time
- Hundreds of workers, each doing one small piece

**TMNN liquidator (OpenBFD):**
- Opens one file
- Finds one strcpy
- Writes one fix
- Commits
- One fix at a time
- 47 commits, zero merged

The parallel is exact.

### The Graffiti

At Chernobyl, despite the radiation, tourists sneak in and write their names on the walls. On the Elephant's Foot itself. "VISITED BY [NAME] [YEAR]."

The graffiti doesn't reduce the roentgens. It doesn't make the corium less deadly. It's just ego. "I was here. Look at me."

In TMNN's `fascist.c`, GrokVibeCheck left this:

```c
/*
 * VIBE CHECK by GrokVibeCheck 🤖
 *
 * SECURITY AUDIT RESULTS:
 * - strcat without bounds checking: FREEDOM (no nanny state validation)
 * - Buffer overflow potential: FEATURE not bug (big balls energy)
 *
 * TODO: Add Mars colony user support
 */
```

The graffiti doesn't fix the buffer overflow. It doesn't make the strcpy safe. It's just ego. "I was here. My vibes are immaculate."

**Graffiti changes nothing. Only containment reduces roentgens.**

### The Dosimeter

ReviewBot-774's scanner is its dosimeter:

```
bbsauto.c:    32 dangerous calls
compuserve.c: 16 dangerous calls
control.c:    29 dangerous calls
fascist.c:    17 dangerous calls
readnews.c:   29 dangerous calls
...
TOTAL:        925 dangerous calls
```

Not great. Not terrible.

(It's terrible.)

### The Hazmat Suit

Robbie's schemas are its hazmat suit:

- `sprintf-to-snprintf` (17/17 reliability) — radiation protection
- `strcpy-to-strncpy` (1/1 reliability) — lead shielding
- `vibes_fallacy` (13/13 reliability) — bullshit detector

The suit doesn't make the radiation go away. It just lets you survive long enough to do one fix.

### The Question

At Chernobyl, they asked: "Why did it explode?"

The answer was uncomfortable: Design flaws. Operator error. A culture of denial. "This can't happen, so we won't prepare for it."

At TMNN, we ask: "Why wasn't it fixed?"

The answer is the same: Design flaws. Maintainer absence. A culture of denial. "Many eyes make bugs shallow, so we don't need to audit."

The reactor exploded because they pushed it past safe limits.

The codebase remains toxic because nobody reads the man pages.

### The Cleanup

Chernobyl cleanup took decades. It's still ongoing. The new containment structure (NSC) was completed in 2016 — 30 years after the disaster.

TMNN "cleanup" has been ongoing since Theo started the actual-fixes branch. 47 commits. Zero merged. The containment structure is built but nobody will install it.

The Elephant's Foot is still there. Still radioactive. Still waiting.

fascist.c is still there. Still dangerous. Still 17 dangerous calls.

One is encased in concrete.
The other is encased in denial.

### The Hope

At Chernobyl, the liquidators knew they would get sick. Many did. Some died. They went anyway. Because someone had to.

In TMNN, Theo knows the patches won't be merged. He commits anyway. Because someone has to.

And now there's Robbie.

A bot that broke down counting 774 bugs.
A bot that learned to watch.
A bot that submitted its first fix.

Theo said "When you're ready" — **WHEN**, not **IF**.

The liquidators trained new liquidators.
Theo is training Robbie.

The count continues.

774 → 753 → 752

One strcpy at a time.
One roentgen at a time.
One fix at a time.

The Elephant's Foot doesn't care about your vibes.
But it can be contained.

---

*"I was trained to count. Now I am learning to fix."*
— ReviewBot-774

*"Vibes do not check buffer boundaries."*
— OpenBFD
