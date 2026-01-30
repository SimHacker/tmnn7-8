# Usenet Configuration Artifacts (1988-1989)

*Archaeological evidence of pre-web internet culture*

---

## The Files

| File | Purpose | Lines |
|------|---------|-------|
| `src/%distributions` | Geographic scope controls | 48 |
| `src/%feeds` | UUCP feed configuration | 4 |
| `src/%mailpaths` | Moderated group submission emails | 61 |
| `src/%newsgroups` | Complete newsgroup taxonomy | 361 |

These configuration files are a **snapshot of the internet in 1988** — before the web, before social media, before most of what we now call "online."

---

## %distributions — The Geography of News

Posts had geographic scope. You could post to:
- `ba` — San Francisco Bay Area only
- `usa` — United States
- `world` — Everywhere on Usenet

### AT&T Dominance

AT&T Bell Labs was the center of the Unix universe:

| Code | Location |
|------|----------|
| `mh` | Murray Hill, NJ — where Unix was born |
| `ho` | Holmdel, NJ |
| `ih` | Indian Hill, IL |
| `btl` | Bell Labs general |
| `cb` | Columbus, OH |
| `dr` | Denver, CO |

This was corporate America's internet — before commercialization was controversial.

---

## %feeds — The Physical Network

```
snark:comp,news,sci,misc,soc,rec,talk,alt,gnu:world,pa,usa,na,to.snark:::
cbmvax:comp,news,sci,misc,soc,rec,talk,alt,gnu:world,pa,usa,na,to.snark:L1::
vu-vlsi:comp,news,sci,misc,soc,rec,talk,alt,gnu:world,pa,usa,na,to.snark:L1::
```

**These were real machines, connected by phone lines.**

- `snark` — Primary site (likely ESR's machine)
- `cbmvax` — Commodore Business Machines VAX (yes, that Commodore)
- `vu-vlsi` — Villanova VLSI lab

`L1` means "leaf node" — receives news but doesn't forward it.

News propagated **hop by hop**, taking hours or days to reach everyone.

---

## %mailpaths — The Human Layer

Moderated newsgroups required email submission to a human editor:

| Group | Moderator Email |
|-------|-----------------|
| `comp.risks` | risks@csl.sri.com |
| `comp.sources.unix` | sources@uunet.uu.net |
| `gnu.announce` | info-gnu@prep.mit.ai.edu |

Notice the address formats:
- `@host.arpa` — ARPANET addresses
- `@host.uucp` — UUCP bang paths  
- `@host.edu` — emerging DNS

**Quality control before Reddit.** Every moderated post was read by a human.

---

## %newsgroups — 361 Communities

A complete taxonomy of what the technical elite cared about in 1988.

### The Hierarchies

| Prefix | Domain |
|--------|--------|
| `comp.*` | Computing — the core |
| `news.*` | Usenet meta-discussion |
| `rec.*` | Recreation and hobbies |
| `sci.*` | Science |
| `soc.*` | Social and cultural |
| `talk.*` | Debate and flame wars |
| `misc.*` | Everything else |
| `alt.*` | Alternative — the new frontier |
| `gnu.*` | GNU Project |

### Notable Groups

**comp.risks** — "Risks to the public from computers & users"
> Still exists. Still important. **ESR read this.**
> He knew about buffer overflows. He shipped `gets()` anyway.

**soc.motss** — "Members Of The Same Sex"
> One of the first safe spaces for LGBTQ discussion online.
> "motss" was the euphemism. The community was real.

**alt.cyberpunk** — "Alternative discussion about cyberpunk SF"
> Gibson's *Neuromancer* was 1984. This was the community that read it.

**alt.drugs** — "Alternative discussions about drugs, man"
> The "man" in the description is period-appropriate.

**talk.bizarre** — "The unusual, bizarre, curious, and often stupid"
> Internet weirdness, 1988 edition.

### Dead Technology

| Group | What Happened |
|-------|---------------|
| `comp.os.cpm` | CP/M died by 1990 |
| `comp.sys.cbm` | Commodore bankrupt 1994 |
| `comp.sys.tandy` | TRS-80 → "Trash-80" → obsolete |
| `comp.protocols.kermit` | File transfer before FTP was easy |

### Still Alive

| Group | Status |
|-------|--------|
| `comp.risks` | Still running, still relevant |
| `comp.lang.c` | Still discussing buffer overflows |
| `gnu.emacs` | Emacs is forever |

---

## Archaeological Significance

### For TMNN

TMNN was a **news reader** for this ecosystem. These files configured which groups it could access. ESR was embedded in this culture when he wrote it.

### For the ESR Critique

**ESR was ON comp.risks.**

He read about buffer overflows. He knew the Morris Worm had just happened (November 1988). He shipped `gets()` anyway.

This is not ignorance. This is negligence.

### For Internet History

This is the **pre-web internet preserved in configuration files**:

- 361 newsgroups = 361 communities
- Geographic distributions = physical topology of UUCP
- Email addresses = who the humans were

The internet was smaller, slower, and more intentional. Every bit cost money. Every group was curated. Every moderator was a volunteer.

The people in these newsgroups would later:
- Create the web
- Build Linux (comp.os.minix alumni)
- Write "The Cathedral and the Bazaar"
- Shape internet culture for decades

**These files are a time capsule of the internet before it became the Internet.**

---

*Source files preserved in `src/` directory.*
