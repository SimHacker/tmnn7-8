# The Cathedral and the Bazaar: Irony Analysis

How ESR's most famous essay contradicts his own development practices.

**Source data:** [catb-irony.yml](catb-irony.yml) | **Related:** [many-eyes-myth.md](many-eyes-myth.md)

---

## The Thesis

"The Cathedral and the Bazaar" was published in 1997 — eight years after TMNN died. Convenient timing.

Its core claims:

- **"Release early. Release often."** — Said the man who kept his code secret for two years.
- **"Given enough eyeballs, all bugs are shallow."** — Said the man whose code had zero eyeballs and infinite bugs.
- **"Open development with many contributors is superior to closed 'cathedral' development."** — Said the man who built a cathedral alone in his basement.

The essay is credited with influencing Netscape to open-source Communicator. ESR got famous telling others to do what he never did.

## TMNN Reality

ESR's own classification would condemn his own project. TMNN was pure Cathedral.

### "Release Early, Release Often"

**The claim**: "Release early. Release often."  
**The practice**: Two years in "secret laboratories" before any release.

"Secret laboratories" is not "release early." Words mean things, Eric.

From the BRAGSHEET:

> "After two years of development the software construct known as TEENAGE MUTANT NINJA NETNEWS has escaped from the secret laboratories of Thyrsus Enterprises"

"Escaped" — like it was a prisoner. Which it was.

### "Given Enough Eyeballs, All Bugs Are Shallow"

**The claim**: Many eyes make bugs shallow.  
**The practice**: Code had essentially zero external reviewers.

Zero eyeballs. Maximum bugs. Shallow? Not so much.

Evidence:
- 874 unsafe string function calls — never caught
- 42 mktemp() race conditions — never caught
- 61 command injection vulnerabilities — never caught
- `gets()` function — so dangerous it was removed from C11 — listed in system.h

That's a lot of bugs for zero eyeballs to miss.

As Theo de Raadt — founder of the most secure OS — put it:

> "My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric (the originator of the statement)."

### "The Bazaar Model Works Better"

**The claim**: Open development beats closed cathedral development.  
**The practice**: TMNN was developed in isolation, abandoned without community.

No bazaar. No community. Just one guy and his ego.

Wikipedia's epitaph — being diplomatic:

> "A rough version of the software was released and drew attention from around the network, but the project was abandoned shortly thereafter."

## Timeline of Hypocrisy

| Year | Action | Model |
|------|--------|-------|
| 1987-1989 | Develops TMNN in "secret laboratories" | Cathedral nobody asked for |
| 1989 | Releases beta 7.8, abandons project | Bazaar rejected his cathedral |
| 1997 | Publishes "The Cathedral and the Bazaar" | 8 years — long enough to forget |
| 2003 | Publishes "The Art of Unix Programming" | His own Unix code riddled with overflows |

Those who can't do, write books about doing.

## Expert Assessments

**Thomas Ptacek** (actual security expert):

> "CATB has just not held up at all; it's actively bad, and it has a weirdly outsized reputation."

**DonHopkins** (first-hand witness from the 1980s):

> "His own failed proprietary closed source 'cathedral' project, that he was notorious for insufferably and arrogantly bragging about during the 80's, but never releasing, and finally giving up on because he didn't have the skills to finish and deliver it."

## The Fundamental Irony

ESR became famous by writing about how open source development works, after his own closed-source cathedral project failed catastrophically.

The guy who preached "release early, release often" kept his code secret for two years.

The guy who coined "given enough eyeballs, all bugs are shallow" never had any eyeballs review his code.

He learned nothing from his failure except how to lecture others about what they should do instead.

---

*The real cathedral was the ego we built along the way.*

---

*See also: [ironies.md](ironies.md) — Full contradiction catalog | [bragsheet.md](bragsheet.md) — The "secret laboratories"*

*← Back to [README](README.md)*
