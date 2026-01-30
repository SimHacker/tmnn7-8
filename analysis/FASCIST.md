# fascist.c: ESR's Political Code

Yes, he really named it `fascist.c`. No, we're not making this up.

**Source data:** [fascist-analysis.yml](fascist-analysis.yml) | **Evidence:** [`src/D.news/fascist.c`](../src/D.news/fascist.c)

---

## Attribution

The original code was written by **Eugene Spafford** — one of the most respected security researchers in computing history. Founder of CERIAS at Purdue. The Spafford who co-discovered and analyzed the Morris Worm. Author of foundational computer ethics work. A legend. An actual security expert. Unlike ESR.

ESR claims he "rewrote" it. What did his "rewrite" add?

- Political dog-whistle naming
- Tolkien cosplay examples  
- Buffer overflows
- FASCIST/COMMUNIST compile flags

He took security code from a security expert and added insecurity and politics.

## Political Naming

ESR embedded his libertarian worldview directly into the code — because why write clean code when you can write manifestos?

### Compile Flags

**`#ifdef FASCIST`** — Controls posting permissions. Who can WRITE to newsgroups. ESR's view: state control of speech = fascism. Also ESR: *writes content moderation code*.

**`#ifdef COMMUNIST`** — Controls reading permissions. Who can READ newsgroups. Peak Cold War brain from someone who thinks they're enlightened.

### Data Structures

**`nasty_t`** — Holds user restrictions. Users with restrictions are literally called "nasty" in the code. His code has opinions about the users it restricts.

### Functions

**`nasty_t *fascist(user)`** — Returns data on security restrictions. The main function is literally called `fascist()`. This is not a drill.

## The Tolkien Cosplay

ESR's example configuration is pure 1980s nerd fantasy projection. He is the hero of his own epic. Always.

The **wizards group** includes "gandalf" and "radagast" with "saruman" as gatekeeper — ESR imagines himself among the wise wizards. Make of that what you will.

**Evil sites** — "mordor" and "orthanc" — are suppressed from posting to "alt.goodguys." Bad guys are literally Sauron's forces. Subtlety: not ESR's strong suit.

The example **restricted user** is called "miscreant" — ESR's word for users he wants to control. Not "restricted_user" or "limited_access" but "miscreant." They can post to "junk" and "talk.politics" but not "local.security." The irony writes itself.

## The "HOG!!!!!" Comment

At line 117, in the `getgrplist()` function, we find:

```
/* This routine is a HOG!!!!! */
```

Five exclamation points. Very professional. Peak software craftsmanship from the "Art of Unix Programming" guy.

What does this "HOG!!!!!" actually do? It concatenates group names into a fixed buffer using `strcat()` with no bounds checking. If a user is in many groups, the buffer overflows. The "HOG" is also a security vulnerability. The exclamation points don't fix the buffer overflow.

## Security Vulnerabilities

The file contains 15+ buffer overflows:

- `getgrplist()` — strcat to fixed buffer in loop (HOG!!!!! indeed)
- `addrestrict()` — multiple strcat calls to n_post and n_read
- `fascist()` — strcpy and sprintf with grplist (the fascist function has vulnerabilities — metaphor?)

Safe alternatives were available in 1989. `snprintf` existed. `strncat` existed. ESR didn't use them.

## The Ironies

| He Wrote | He Later Argued |
|----------|-----------------|
| fascist.c to restrict who could post | Any form of content moderation is tyranny |
| Named restriction system FASCIST and COMMUNIST | Complained about being "cancelled" |
| Took Spafford's security code | Added security vulnerabilities |
| This code | "The Art of Unix Programming" |

Sir, you literally wrote code to cancel people.

---

---

**File**: [`src/D.news/fascist.c`](../src/D.news/fascist.c)
**Lines**: 305 (107 political commentary, 198 actual code)
**Timestamp**: 1989-08-01

More than 1/3 of a C source file is political commentary. For news server software.

---

*See also: [IRONIES.md](IRONIES.md) — The content moderation contradiction | [vulnerabilities.yml](vulnerabilities.yml) — Full security catalog*

*← Back to [README](README.md)*
