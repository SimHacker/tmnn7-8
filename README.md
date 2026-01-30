# Teenage Mutant Ninja Netnews (TMNN) - An Archaeological Analysis

**Eric S. Raymond's abandoned magnum opus, rediscovered after 30 years.**

## Discovery

In January 2026, while researching a Hacker News discussion about Usenet interfaces, the source code for Eric S. Raymond's "Teenage Mutant Ninja Netnews" (TMNN) project was rediscovered in a Finnish Ubuntu archive mirror that preserves historic Unix software from the DEC FTP era.

ESR spent the late 1980s obsessively promoting this project at science fiction conventions, calling himself the "mad mastermind of TMN-Netnews" in his Usenet signature. After two years in "secret laboratories," the project was abandoned at beta level 7.8. None of the promised features (hypertext, encryption, intelligent filtering agents) ever materialized.

The code sat essentially undiscovered for 30 years - from 1989 to 2019 - when Wikipedia editor Hbent found and documented the archive link.

## Why This Matters

ESR later became famous for:
- "The Cathedral and the Bazaar" - advocating "release early, release often"
- "Linus's Law" - "given enough eyeballs, all bugs are shallow"
- "The Art of Unix Programming" - Unix best practices
- "How to Become a Hacker" - defining hacker culture

His own code tells a different story:
- **Cathedral development**: 2 years in "secret laboratories" before any release
- **No eyeballs**: Abandoned without community review
- **Security disasters**: 874 unsafe string functions, 42 mktemp() calls, 61 shell injections
- **Political ideology in code**: `fascist.c` with FASCIST/COMMUNIST compile flags

## Archive Source

- **Original**: `ftp.digital.com/pub/news/tmnn/` (DEC FTP, now offline)
- **Archived**: [fi.archive.ubuntu.com](https://web.archive.org/web/20191205160937/https://fi.archive.ubuntu.com/index/unix/news/tmnn7-8.tar.Z)
- **Code dates**: July-August 1989
- **Version**: B3.0 (beta level 7.8)

## Repository Structure

```
tmnn7-8/
├── README.md           # This file
├── analysis/           # Archaeological analysis artifacts
│   ├── hn-post.txt     # Original Hacker News post
│   ├── timeline.yml    # Discovery and code timeline
│   ├── code-review.yml # Security vulnerabilities found
│   ├── receipts.yml    # Community quotes and citations
│   └── by-the-numbers.yml  # Harper's Index style statistics
├── LICENSE             # ESR's 1989 license (with consulting plug)
├── doc/                # Original documentation
│   ├── BRAGSHEET       # ESR's marketing document
│   ├── BUGS            # Known bugs (abandoned)
│   └── ...
├── src/                # Source code
│   ├── D.news/
│   │   └── fascist.c   # Yes, really
│   ├── D.priv/
│   │   └── lock.c      # "ugly and flaky" per comments
│   └── ...
└── man/                # Man pages
```

## Key Files

| File | Description |
|------|-------------|
| `doc/BRAGSHEET` | ESR's marketing promises - hypertext, encryption, AI filtering |
| `src/version.h` | Confirms "beta level 7.8" when abandoned |
| `src/D.news/fascist.c` | Access control with FASCIST/COMMUNIST flags |
| `src/D.priv/lock.c` | Self-described "ugly and flaky" locking |
| `LICENSE` | Includes consulting services advertisement |

## Analysis Documents

### Overview & Summaries
- [analysis/INDEX.yml](analysis/INDEX.yml) - **Master index of all analysis files**
- [analysis/ANALYSIS.md](analysis/ANALYSIS.md) - Human-readable summary of all findings
- [analysis/IRONIES.md](analysis/IRONIES.md) - **Catalog of ESR contradictions**
- [analysis/hn-post.txt](analysis/hn-post.txt) - Full Hacker News post documenting this discovery

### TMNN Source Code Analysis (YAML Jazz)

| File | Contents |
|------|----------|
| [bragsheet.yml](analysis/bragsheet.yml) | ESR's promises - hypertext, encryption, AI filtering |
| [license-analysis.yml](analysis/license-analysis.yml) | Political manifesto and consulting plug |
| [fascist-analysis.yml](analysis/fascist-analysis.yml) | Deep dive into fascist.c with buffer overflows |
| [vulnerabilities.yml](analysis/vulnerabilities.yml) | Complete security vulnerability catalog |
| [code-index.yml](analysis/code-index.yml) | File-by-file risk assessment |
| [code-review.yml](analysis/code-review.yml) | Original code review findings |

### ESR Contradictions & Ironies (YAML Jazz)

| File | Contents |
|------|----------|
| [catb-irony.yml](analysis/catb-irony.yml) | "Cathedral and the Bazaar" vs TMNN reality |
| [many-eyes-myth.yml](analysis/many-eyes-myth.yml) | "Linus's Law" - the quote Linus never said |
| [art-of-unix-irony.yml](analysis/art-of-unix-irony.yml) | Unix best practices book vs terrible code |
| [content-moderation-irony.yml](analysis/content-moderation-irony.yml) | Built fascist.c, preached moderation is tyranny |

### ESR External History (YAML Jazz)

| File | Contents |
|------|----------|
| [jargon-file.yml](analysis/jargon-file.yml) | How ESR hijacked the Jargon File |
| [osi-ban.yml](analysis/osi-ban.yml) | Banned from OSI he co-founded (2020) |
| [fetchmail.yml](analysis/fetchmail.yml) | His other security-challenged project |
| [sex-tips-honeytrap.yml](analysis/sex-tips-honeytrap.yml) | "Sex Tips for Geeks" and honeytrap conspiracy |
| [sf-con-behavior.yml](analysis/sf-con-behavior.yml) | Notorious SF convention self-promotion |
| [esr-resume-analysis.yml](analysis/esr-resume-analysis.yml) | How he sanitizes TMNN on his resume |

### Quotes & Receipts

| File | Contents |
|------|----------|
| [ESR-QUOTES.md](analysis/ESR-QUOTES.md) | **Human-readable quote collection** |
| [esr-quotes.yml](analysis/esr-quotes.yml) | **Sourced quotes for charity fundraising** |
| [receipts.yml](analysis/receipts.yml) | Community quotes with provenance |
| [esr-receipts.yml](analysis/esr-receipts.yml) | ESR community receipts with full citations |

### Timeline & Statistics

| File | Contents |
|------|----------|
| [timeline.yml](analysis/timeline.yml) | Archaeological timeline 1987-2026 |
| [by-the-numbers.yml](analysis/by-the-numbers.yml) | Harper's Index style statistics |

## The Irony

> "Release early, release often" - Eric S. Raymond, 1997

> "After two years of development the software construct known as TEENAGE MUTANT NINJA NETNEWS has escaped from the secret laboratories of Thyrsus Enterprises" - Eric S. Raymond, 1989

The Bazaar guy's own magnum opus was a Cathedral that never got built.

## Community Receipts

**Theo de Raadt** (OpenBSD founder):
> "My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric."

**Thomas Ptacek** (Matasano Security):
> "CATB has just not held up at all; it's actively bad."

*Ptacek raised $30,000+ for charity from people paying him NOT to post more ESR quotes.*

**DonHopkins** (knew ESR since early 1980s):
> "His own failed proprietary closed source 'cathedral' project... he didn't have the skills to finish and deliver it."

## The Quote Collection

See [analysis/ESR-QUOTES.md](analysis/ESR-QUOTES.md) for a comprehensive, sourced collection of ESR's documented statements on race, Islam, LGBTQ+ people, and women - suitable for charity fundraising in the tptacek tradition.

## See Also

- [Original Jargon File](https://github.com/PDP-10/its/blob/master/doc/humor/jargon.68) - Free of ESR's edits
- [B News Wikipedia](https://en.wikipedia.org/wiki/B_News) - Where TMNN is documented
- [Theo de Raadt on "many eyes"](https://marc.info/?l=openbsd-tech&m=129261032213320&w=2) - OpenBSD founder's critique

## License

The original TMNN code is under ESR's 1989 "NETNEWS GENERAL PUBLIC LICENSE."

Analysis documents in `analysis/` are public domain.
