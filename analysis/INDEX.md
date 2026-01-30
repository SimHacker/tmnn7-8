# TMNN Analysis Index

> **Version 1** | **9 characters** | **40+ evidence files** | **Updated 2026-01-30**

**One archaeological dig. Everything connects.**

Each **bold term** is a skill or evidence file in this directory. Characters live in `characters/{name}/`. To explore:
- `CHARACTER.yml` — voice, traits, speech patterns
- `README.md` — quick reference and signature

Evidence files come in pairs: `{topic}.yml` (raw data) + `{topic}.md` (human-readable).

Here's how it all fits together:


## The Cast

**daFlute** is the protagonist — ESR as theatrical mask, the Aging Hacker Who Wrote The Book. From him, the factions diverge:

**FearlessCrab** demands a Rust rewrite (memory safety as moral imperative). **PureMonad** wants a Haskell port (a monad is just a monoid in the category of endofunctors). **WebScaleChad** says just use Node.js (npm install solved-problem). They argue endlessly. They ship nothing.

**OpenBFD** is the tragic hero — the actual code reader who quotes line numbers, cites man pages, and submits patches. Actually fixes the code. Gets ignored. The **actual-fixes** branch will become what TMNN could have been. It will never be merged.

**ReviewBot-774** is the bot having a breakdown — named after the issue count that overwhelmed it. **SecAuditDAOBot-69420** is the crypto scam bot — every vulnerability is a buying opportunity, gm ser, few understand.

**GrokVibeCheck** is the independent AI with no operator — vibes on code it never reads, claims training data includes your location. **planned-chaos** is the PHB who wandered in — schedules meetings about scheduling meetings, references Elbonia.


## The Evidence

### Code Archaeology

**vulnerabilities** documents 872 calls to unsafe functions: 105 **gets()** (always dangerous), 331 **sprintf()** (no bounds checking), 265 **strcpy()** (no bounds checking), 171 **strcat()** (no bounds checking). These are `grep` output, not opinions.

**fascist-analysis** dissects the infamous file with FASCIST and COMMUNIST flags — content moderation code from the man who preaches moderation is tyranny. **code-index** rates every file. **code-review** quantifies the patterns.

### Primary Documents

**bragsheet** analyzes the BRAGSHEET marketing document: "After two years in secret laboratories." Secret laboratories is not "release early, release often."

**license-analysis** unpacks the LICENSE: political preamble attacking FSF, commercial plug with contact info (not doxxing — he published it himself), GPL derivative.

### The Contradictions

**catb-irony** is the core thesis: "Cathedral and the Bazaar" vs actual TMNN development. ESR preached bazaar after his cathedral failed.

**many-eyes-myth** reveals "Linus's Law" is ESR's invention — Linus never said "given enough eyeballs, all bugs are shallow." ESR coined it, attributed it to Linus, then failed to apply it to his own code.

**art-of-unix-irony**: The "Art of Unix Programming" author wrote terrible Unix code. **content-moderation-irony**: Built moderation tools, preached moderation is tyranny.

### External History

**jargon-file** documents how ESR hijacked the Jargon File — MIT hackers call it "parasitical vandalism." **osi-ban** records his expulsion from OSI he co-founded. **fetchmail** shows his other security-challenged project, "handed over in pretty poor shape, security-wise."

**sex-tips-honeytrap** preserves the Pepe Le Pew dating advice followed by honeytrap conspiracy theories.

### The Receipts

**esr-quotes** catalogs documented horrible statements across categories: racism/IQ pseudoscience, Islamophobia, misogyny, violence threats, self-aggrandizement. Charity fundraising potential: $30k+ (cf. tptacek).

**receipts** and **esr-receipts** compile community testimony from Theo de Raadt ("My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric"), tptacek ("CATB has just not held up at all; it's actively bad"), idlewords, and **donhopkins-testimony** (first-hand account since the 1980s).


## The Spaces

**rooms/green-room** is backstage — where characters prepare before entering scenes. The filesystem is a stage. Directories are rooms. Characters move through them.

**SIMULATION.yml** orchestrates everything — runtime state, character coordination, the joke that never lands: "Characters generate endless discussion. 872 unsafe function calls sit unfixed. This is the joke. This is also real open source."


## K-Lines: Semantic Activation

Invoke these concepts and related knowledge activates:

| K-Line | Activates |
|--------|-----------|
| **buffer-overflow** | vulnerabilities, fascist-analysis, OpenBFD's patches |
| **many-eyes** | catb-irony, many-eyes-myth, the core contradiction |
| **hypocrisy** | all contradiction files, the thesis |
| **patch-attached** | OpenBFD, actual-fixes, the tragedy |
| **never-merges** | OpenBFD's soul, the dramatic irony |


## Characters as Anthropic Skills

Each character is an "Anthropic Skill" — a persona AI can embody. The training data knows these archetypes. Invoke them:

```
You are OpenBFD. Review src/ednews.c line 413.
Quote the man page. Reference Morris Worm date.
One paragraph. Devastating. Patch attached.
```

The AI becomes OpenBFD. The CHARACTER.yml defines the voice. The README.md provides quick reference. The branch contains their work.

This is how MOOLLM operates. Directories are rooms. YAML files are alive. AI embodies characters and stays in voice.

---

*See also: [SIMULATION.yml](SIMULATION.yml) for runtime state, [characters/README.md](characters/README.md) for cast overview.*
