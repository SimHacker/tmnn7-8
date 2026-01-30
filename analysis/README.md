# TMNN Analysis

**The archaeological record of ESR's contradictions.**

---

## The Big Picture

```mermaid
graph TB
    subgraph tmnn["TMNN (1987-1989)"]
        secret["2 years in<br/>'secret laboratories'"]
        abandon["Abandoned at<br/>beta 7.8"]
        bugs["774 buffer<br/>overflows"]
    end
    
    subgraph career["ESR's Later Career"]
        catb["Cathedral & Bazaar<br/>'release early, release often'"]
        eyes["Linus's Law<br/>'many eyes'"]
        unix["Art of Unix<br/>best practices"]
        osi["Co-founds OSI<br/>open collaboration"]
    end
    
    subgraph reality["Reality Check"]
        contradiction["Every claim<br/>contradicted by<br/>his own code"]
        banned["Banned from<br/>OSI in 2020"]
    end
    
    secret -->|contradicts| catb
    abandon -->|contradicts| catb
    bugs -->|contradicts| eyes
    bugs -->|contradicts| unix
    tmnn -->|leads to| contradiction
    osi -->|becomes| banned
    
    style tmnn fill:#ffebee
    style career fill:#e8f5e9
    style reality fill:#fff3e0
```

---

## Quick Navigation

| I want to... | Start here |
|-------------|------------|
| Understand what happened | [timeline.md](timeline.md) |
| See the code disasters | [fascist-analysis.md](fascist-analysis.md) |
| Count the vulnerabilities | [vulnerabilities.md](vulnerabilities.md) |
| Read the contradictions | [ironies.md](ironies.md) |
| See first-hand testimony | [donhopkins-testimony.md](donhopkins-testimony.md) |
| Get the receipts | [esr-quotes.md](esr-quotes.md) |
| Just show me numbers | [by-the-numbers.md](by-the-numbers.md) |

---

## The Story

### Act I: The Cathedral

ESR built TMNN alone in "secret laboratories" for two years, bragging about it at every SF convention while never releasing it.

```mermaid
timeline
    title Act I: The Cathedral (1987-1989)
    
    1987 : Development begins
         : "Secret laboratories"
         : (his apartment)
    1988 : Continues alone
         : Brags at SF cons
         : "Mad mastermind"
    1989 : Releases beta 7.8
         : Immediately abandons
         : Zero features delivered
```

📖 [bragsheet.md](bragsheet.md) — The promises  
📖 [license-analysis.md](license-analysis.md) — The political manifesto

### Act II: The Code

The code tells the story: 774 buffer overflows, FASCIST/COMMUNIST compile flags, "This routine is a HOG!!!!!"

```mermaid
pie showData
    title Security Disasters in TMNN
    "Buffer Overflows" : 774
    "Command Injections" : 61
    "Race Conditions" : 42
```

📖 [fascist-analysis.md](fascist-analysis.md) — The infamous file  
📖 [vulnerabilities.md](vulnerabilities.md) — Full catalog

### Act III: The Silence

ESR never mentioned TMNN again. Zero blog posts. Seven words on his resume.

```mermaid
xychart-beta
    title Resume Word Count
    x-axis ["TMNN", "Martial Arts"]
    y-axis "Words" 0 --> 50
    bar [7, 47]
```

📖 [esr-resume-analysis.md](esr-resume-analysis.md) — The erasure

### Act IV: The Essays

Eight years later, ESR became famous preaching everything his code contradicted.

```mermaid
flowchart LR
    subgraph said["What ESR Preached"]
        s1["Release early"]
        s2["Many eyes"]
        s3["Best practices"]
    end
    
    subgraph did["What ESR Did"]
        d1["2 years secret"]
        d2["Zero reviewers"]
        d3["774 overflows"]
    end
    
    s1 -.->|vs| d1
    s2 -.->|vs| d2
    s3 -.->|vs| d3
```

📖 [catb-irony.md](catb-irony.md) — "Release early, release often"  
📖 [many-eyes-myth.md](many-eyes-myth.md) — The quote Linus never said

### Act V: The Pattern

TMNN isn't the exception. It's the pattern.

```mermaid
flowchart TB
    subgraph pattern["ESR's Career Pattern"]
        find["Find others' work"]
        take["Take over"]
        rewrite["Inject ideology"]
        claim["Claim credit"]
        rejected["Get rejected"]
    end
    
    find --> take --> rewrite --> claim --> rejected
    
    subgraph examples["Examples"]
        jargon["Jargon File<br/>'parasitical vandalism'"]
        linus["'Linus's Law'<br/>Linus never said it"]
        spaf["Spafford's code<br/>+ ESR's bugs"]
        osi["OSI<br/>co-founded → banned"]
    end
    
    claim --> jargon
    claim --> linus
    claim --> spaf
    rejected --> osi
```

📖 [jargon-file.md](jargon-file.md) — The hijacking  
📖 [osi-ban.md](osi-ban.md) — Banned from his own org  
📖 [sex-tips-honeytrap.md](sex-tips-honeytrap.md) — The later years

---

## The Evidence

### The Code

| Document | What It Shows |
|----------|---------------|
| [fascist-analysis.md](fascist-analysis.md) | The infamous `fascist.c` — FASCIST/COMMUNIST flags, Tolkien cosplay, "HOG!!!!!" |
| [vulnerabilities.md](vulnerabilities.md) | 774 buffer overflows, 61 injections, 42 race conditions |
| [bragsheet.md](bragsheet.md) | Marketing promises vs zero delivery |
| [license-analysis.md](license-analysis.md) | Political manifesto + consulting advertisement |

### The Contradictions

| Document | The Irony |
|----------|-----------|
| [ironies.md](ironies.md) | **Complete catalog of contradictions** |
| [catb-irony.md](catb-irony.md) | "Release early" after 2 years in secret |
| [many-eyes-myth.md](many-eyes-myth.md) | "Linus's Law" — Linus never said it |

### The History

| Document | What Happened |
|----------|---------------|
| [timeline.md](timeline.md) | **Full 37-year chronology** |
| [esr-resume-analysis.md](esr-resume-analysis.md) | How he buries TMNN |
| [osi-ban.md](osi-ban.md) | Banned from OSI he co-founded |
| [jargon-file.md](jargon-file.md) | "Disrespectful parasitical vandalism" |
| [sex-tips-honeytrap.md](sex-tips-honeytrap.md) | Sex tips to honeytrap conspiracy |

### The Receipts

| Document | The Evidence |
|----------|--------------|
| [donhopkins-testimony.md](donhopkins-testimony.md) | **First-hand witness accounts** |
| [esr-quotes.md](esr-quotes.md) | Quotes for charity fundraising |
| [by-the-numbers.md](by-the-numbers.md) | Harper's Index statistics |

---

## By The Numbers

| Metric | Value | Details |
|--------|-------|---------|
| Years in secret labs | **2** | [timeline.md](timeline.md) |
| Beta level at abandonment | **7.8** | [bragsheet.md](bragsheet.md) |
| Features delivered | **0** | [bragsheet.md](bragsheet.md) |
| Buffer overflows | **774** | [vulnerabilities.md](vulnerabilities.md) |
| Command injections | **61** | [vulnerabilities.md](vulnerabilities.md) |
| Race conditions | **42** | [vulnerabilities.md](vulnerabilities.md) |
| Resume words for TMNN | **7** | [esr-resume-analysis.md](esr-resume-analysis.md) |
| Resume words for martial arts | **47** | [esr-resume-analysis.md](esr-resume-analysis.md) |
| Blog mentions by ESR | **0** | [esr-resume-analysis.md](esr-resume-analysis.md) |
| Years code buried | **30** | [timeline.md](timeline.md) |
| Charity raised from ESR quotes | **$30,000+** | [esr-quotes.md](esr-quotes.md) |

---

## The Community's Verdict

> "My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric."  
> — **Theo de Raadt**, OpenBSD founder

> "CATB has just not held up at all; it's actively bad."  
> — **Thomas Ptacek**, Matasano Security

> "His own failed proprietary closed source 'cathedral' project... he didn't have the skills to finish and deliver it."  
> — **DonHopkins**, knew ESR since early 1980s

*See: [donhopkins-testimony.md](donhopkins-testimony.md)*

---

## Source Data

For those who want the raw annotated data, the YAML Jazz source files are available:

<details>
<summary>YAML source files (click to expand)</summary>

| Source File | Corresponding Narrative |
|-------------|------------------------|
| [timeline.yml](timeline.yml) | [timeline.md](timeline.md) |
| [vulnerabilities.yml](vulnerabilities.yml) | [vulnerabilities.md](vulnerabilities.md) |
| [fascist-analysis.yml](fascist-analysis.yml) | [fascist-analysis.md](fascist-analysis.md) |
| [bragsheet.yml](bragsheet.yml) | [bragsheet.md](bragsheet.md) |
| [license-analysis.yml](license-analysis.yml) | [license-analysis.md](license-analysis.md) |
| [catb-irony.yml](catb-irony.yml) | [catb-irony.md](catb-irony.md) |
| [many-eyes-myth.yml](many-eyes-myth.yml) | [many-eyes-myth.md](many-eyes-myth.md) |
| [jargon-file.yml](jargon-file.yml) | [jargon-file.md](jargon-file.md) |
| [osi-ban.yml](osi-ban.yml) | [osi-ban.md](osi-ban.md) |
| [sex-tips-honeytrap.yml](sex-tips-honeytrap.yml) | [sex-tips-honeytrap.md](sex-tips-honeytrap.md) |
| [esr-resume-analysis.yml](esr-resume-analysis.yml) | [esr-resume-analysis.md](esr-resume-analysis.md) |
| [esr-silence.yml](esr-silence.yml) | [esr-resume-analysis.md](esr-resume-analysis.md) |
| [donhopkins-testimony.yml](donhopkins-testimony.yml) | [donhopkins-testimony.md](donhopkins-testimony.md) |
| [esr-quotes.yml](esr-quotes.yml) | [esr-quotes.md](esr-quotes.md) |
| [by-the-numbers.yml](by-the-numbers.yml) | [by-the-numbers.md](by-the-numbers.md) |

</details>

---

*The silence is the confession.*

*← Back to [repository root](../README.md)*
