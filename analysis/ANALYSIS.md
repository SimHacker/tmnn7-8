# TMNN Analysis Summary

A comprehensive archaeological examination of Eric S. Raymond's abandoned "Teenage Mutant Ninja Netnews" project (1987-1989).

## The Discovery

In January 2026, while researching a Hacker News discussion about Usenet interfaces and Eric S. Raymond, the source code for TMNN was rediscovered in a Finnish Ubuntu archive mirror. The code had been essentially buried for 30 years.

**Archive source:** [fi.archive.ubuntu.com](https://web.archive.org/web/20191205160937/https://fi.archive.ubuntu.com/index/unix/news/tmnn7-8.tar.Z)

## What We Found

### The Numbers

| Metric | Value |
|--------|-------|
| Lines of C code | 23,549 |
| Unsafe string functions | 774 |
| mktemp() race conditions | 42 |
| system()/popen() injections | 61 |
| gets() in header | 1 |
| Beta level at abandonment | 7.8 |
| Years in "secret laboratories" | 2 |
| Promised features delivered | 0 |

### Key Files

| File | Significance |
|------|--------------|
| `doc/BRAGSHEET` | ESR's grandiose promises |
| `LICENSE` | Political manifesto + sales pitch |
| `src/D.news/fascist.c` | FASCIST/COMMUNIST access control |
| `src/D.priv/lock.c` | Self-described "ugly and flaky" |
| `src/version.h` | Confirms beta 7.8 abandonment |

## The Irony

ESR later became famous for:

1. **"The Cathedral and the Bazaar" (1997)** - Advocating "release early, release often"
   - TMNN: 2 years in "secret laboratories" before any release

2. **"Linus's Law"** - "Given enough eyeballs, all bugs are shallow"
   - TMNN: No community review, 774+ vulnerabilities remained

3. **"The Art of Unix Programming" (2003)** - Unix best practices
   - TMNN: Violates most of those practices

4. **Co-founding Open Source Initiative** - Promoting open development
   - TMNN: Proprietary cathedral development, never shared

## The fascist.c File

Yes, that's really the filename. ESR took Eugene Spafford's access control code and:

- Added FASCIST and COMMUNIST compile flags
- Added Tolkien cosplay (wizards group: gandalf, radagast; banned sites: mordor, orthanc)
- Added the "miscreant" restricted user example
- Added the "This routine is a HOG!!!!!" comment
- Added buffer overflows

The original author (Spafford) is one of the most respected security researchers in computing history.

## The Resume Sanitization

How TMNN appears on ESR's official resume (catb.org):

> "A rewrite of the USENET netnews software."

What's missing:
- "Teenage Mutant Ninja Netnews" name
- Beta status
- Abandonment
- Any promised features
- Any delivered features

## Analysis Files

| File | Contents |
|------|----------|
| [bragsheet.yml](bragsheet.yml) | Structured extraction of promises |
| [license-analysis.yml](license-analysis.yml) | Political statements breakdown |
| [fascist-analysis.yml](fascist-analysis.yml) | Deep dive into fascist.c |
| [vulnerabilities.yml](vulnerabilities.yml) | Security vulnerability catalog |
| [code-index.yml](code-index.yml) | File-by-file risk assessment |
| [hn-receipts.yml](hn-receipts.yml) | Community quotes and citations |
| [esr-resume-analysis.yml](esr-resume-analysis.yml) | How he buries TMNN |
| [timeline.yml](timeline.yml) | Archaeological timeline |
| [by-the-numbers.yml](by-the-numbers.yml) | Harper's Index statistics |

## Community Receipts

### Theo de Raadt (OpenBSD founder)
> "Oh right, let's hear some of that 'many eyes' crap again. My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric (the originator of the statement)."

### Thomas Ptacek (Matasano Security)
> "CATB has just not held up at all; it's actively bad, and it has a weirdly outsized reputation."

Ptacek raised $30,000+ for charity from people paying him NOT to post more ESR quotes.

### DonHopkins (knew ESR since early 1980s)
> "His own failed proprietary closed source 'cathedral' project, that he was notorious for insufferably and arrogantly bragging about during the 80's, but never releasing, and finally giving up on because he didn't have the skills to finish and deliver it."

## Conclusion

The Bazaar guy's own magnum opus was a Cathedral that never got built. He just kept giving talks about it.

---

*Analysis performed January 2026. Code dates from July-August 1989.*
