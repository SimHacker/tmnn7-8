# The Timeline: From Secret Labs to Rediscovery

37 years of code archaeology.

**Source data:** [timeline.yml](timeline.yml)

---

## The Full Timeline

```mermaid
timeline
    title TMNN: 37 Years of History
    
    section Secret Labs
        1987 : Development begins
             : "Secret laboratories"
             : Solo work, no community
        1988 : Continued isolation
             : ESR brags at SF cons
             : "Mad mastermind"
        1989-07 : LICENSE created
               : Political manifesto included
        1989-08 : Source code dated
               : fascist.c written
               : "This routine is a HOG!!!!!"
        1989-12 : "mad mastermind of TMN-Netnews"
               : Usenet signature
    
    section Abandonment
        1990 : Project abandoned
             : Beta 7.8
             : Zero features delivered
             : Nobody wanted to collaborate
    
    section The Silence
        1995 : Archive appears on DEC FTP
             : ftp.digital.com/pub/news/tmnn/
        1997 : ESR publishes CatB
             : Never mentions TMNN
             : "Release early, release often"
        2003 : Art of Unix Programming
             : Unix best practices
             : His code violates them
    
    section Institutional
        1998 : OSI founded
             : ESR co-founder
        2020 : ESR banned from OSI
             : CoC violations
    
    section Rediscovery
        2005 : First Wikipedia mention
             : IMeowbot adds TMNN
        2019 : Archive link found
             : Hbent adds URL
             : Wayback captures it
        2026 : Full code review
             : 774 buffer overflows found
             : DonHopkins + Cursor AI
```

---

## Development Era (1987-1989)

### The "Secret Laboratories"

ESR began development around 1987 in what he would later call "secret laboratories" — his apartment at 22 South Warren Avenue, Malvern, PA.

For two years, he worked alone. No bazaar. No community. No "release early, release often."

### The Usenet Signature

By December 1989, ESR was signing his Usenet posts:

> "mad mastermind of TMN-Netnews"  
> — eric@snark.uu.net, comp.lang.misc, 1989-12-13

### The SF Convention Campaign

According to [DonHopkins' testimony](TESTIMONY.md), ESR spent these years cornering people at science fiction conventions to brag about TMNN:

> "He would corner people and drone on endlessly about it at inappropriate times."

### Key Dates from File Timestamps

| Date | Event | Evidence |
|------|-------|----------|
| 1989-07-10 | LICENSE and READ.ME created | File timestamps |
| 1989-08-01 | Bulk of source code | Timestamps on `src/*.c` |
| 1989-08-28 | Last modification | Makefile timestamp |

**Version at abandonment:** B3.0 (beta level 7.8)  
**Source:** [`src/version.h`](../src/version.h)

---

## The Silence (1990-2019)

### Promised Features Never Delivered

From the [BRAGSHEET](BRAGSHEET.md):

- ❌ Hypertext integration
- ❌ Public-key encryption
- ❌ Intelligent filtering agents
- ❌ "Distributed hypertext service"

**Delivered:** Nothing.

### The Archive Journey

```mermaid
flowchart TB
    subgraph era1["1989-1995"]
        abandon["Project abandoned"]
        unknown["Unknown distribution<br/>(Usenet? Direct sharing?)"]
    end
    
    subgraph era2["1995-2009"]
        dec["DEC FTP server<br/>ftp.digital.com/pub/news/tmnn/"]
        compaq["DEC → Compaq → HP<br/>Server preserved"]
    end
    
    subgraph era3["2009-2019"]
        ubuntu["Finnish Ubuntu mirror<br/>fi.archive.ubuntu.com"]
        buried["Code effectively buried<br/>30 years"]
    end
    
    subgraph era4["2019-2026"]
        wiki["Wikipedia editor Hbent<br/>finds and documents link"]
        wayback["Wayback Machine<br/>captures archive"]
        review["Full code review<br/>774 vulnerabilities found"]
    end
    
    abandon --> unknown --> dec --> compaq --> ubuntu --> buried
    buried --> wiki --> wayback --> review
```

### Meanwhile, ESR Became Famous

| Year | ESR Publication | The Irony |
|------|-----------------|-----------|
| 1997 | "Cathedral and the Bazaar" | Preached "release early" after 2 years in secret labs |
| 1998 | Co-founds OSI | Would be banned from it in 2020 |
| 2003 | "Art of Unix Programming" | His own code violates the practices |

*See: [CATB-IRONY.md](CATB-IRONY.md) | [OSI-BAN.md](OSI-BAN.md)*

---

## Rediscovery (2019-2026)

### Wikipedia Documentation

| Date | Editor | Action |
|------|--------|--------|
| 2005-01-11 | IMeowbot | First mention of TMNN added to B News article |
| 2019-12-05 | Hbent | Live archive link added |
| 2019-12-05 | Wayback | Archive captured |

### The 2026 Code Review

**Date:** January 30, 2026  
**Context:** Hacker News discussion about Usenet and ESR  
**Reviewers:** DonHopkins + Cursor AI

**Findings:**

| Category | Count |
|----------|-------|
| Buffer overflows | 774 |
| mktemp() race conditions | 42 |
| Command injections | 61 |
| gets() in headers | 1 |

**Notable discovery:** `fascist.c` with FASCIST/COMMUNIST compile flags

*Full details: [VULNERABILITIES.md](VULNERABILITIES.md)*

---

## The Resume Sanitization

How TMNN appears on [ESR's resume](http://catb.org/~esr/resume.html):

> "A rewrite of the USENET netnews software."

**7 words.** Compare to 47 words for martial arts.

What's missing:
- "Teenage Mutant Ninja Netnews" name
- "TMNN" acronym
- Beta status
- Abandonment
- Any promised features

*See: [RESUME.md](RESUME.md)*

---

## By The Numbers

| Metric | Value |
|--------|-------|
| Years in secret labs | 2 |
| Years code buried | 30 |
| Years until first public analysis | 37 |
| Many eyes on his code | ~0 |
| Bugs found shallow | None |

---

*See also: [VULNERABILITIES.md](VULNERABILITIES.md) — What the code review found | [IRONIES.md](IRONIES.md) — The contradictions*

*← Back to [README](README.md)*
