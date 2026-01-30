# TMNN Analysis Summary

> **Note:** This file is preserved for backward compatibility. For the full analysis, see [README.md](README.md).

---

A comprehensive archaeological examination of Eric S. Raymond's abandoned "Teenage Mutant Ninja Netnews" project (1987-1989).

## Quick Links

| Topic | Document |
|-------|----------|
| The Code | [fascist-analysis.md](fascist-analysis.md) — The infamous file |
| The Promises | [bragsheet.md](bragsheet.md) — Marketing vs reality |
| The Contradictions | [ironies.md](ironies.md) — Full catalog |
| The Philosophy | [catb-irony.md](catb-irony.md) — Cathedral & Bazaar |
| The Quote | [many-eyes-myth.md](many-eyes-myth.md) — Linus never said it |
| The Pattern | [jargon-file.md](jargon-file.md) — Hijacking culture |
| The Ban | [osi-ban.md](osi-ban.md) — Kicked from his own org |
| The Quotes | [esr-quotes.md](esr-quotes.md) — For charity fundraising |

## The Numbers

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

## The Irony

ESR later became famous for:

1. **"The Cathedral and the Bazaar" (1997)** — Advocating "release early, release often"
   - TMNN: 2 years in "secret laboratories" before any release

2. **"Linus's Law"** — "Given enough eyeballs, all bugs are shallow"
   - TMNN: No community review, 774+ vulnerabilities remained

3. **"The Art of Unix Programming" (2003)** — Unix best practices
   - TMNN: Violates most of those practices

4. **Co-founding Open Source Initiative** — Promoting open development
   - TMNN: Proprietary cathedral development, never shared

---

*For the full narrative with diagrams and cross-references, see [README.md](README.md).*

*← Back to [repository root](../README.md)*
