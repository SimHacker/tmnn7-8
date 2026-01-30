# Teenage Mutant Ninja Netnews (TMNN) Vibe Code Review

**Eric S. Raymond's abandoned magnum opus — rediscovered after 30 years.**

The man who preached "release early, release often" kept his code secret for two years. The man who coined "given enough eyeballs, all bugs are shallow" had zero eyeballs on his 774 buffer overflows. The "Art of Unix Programming" author wrote code riddled with security disasters.

This repository contains the archaeological evidence.

---

## Quick Facts

| | |
|---|---|
| **Developer** | Eric S. Raymond ("Eric The Flute") |
| **Period** | 1987-1989 |
| **Status** | Abandoned at beta 7.8 |
| **Secret Lab Time** | 2 years |
| **Promised Features Delivered** | 0 |
| **Buffer Overflows** | 774 |
| **Blog Mentions by ESR** | 0 |

---

## ⚠️ METHODOLOGY: Vibe Code Reviewing

**Full disclosure: I have never actually looked at this code.**

I refuse to look at it. I do not want it to burn my eyes and haunt my nightmares. Life is too short to parse ESR's 1980s C with my own neurons.

**Instead, I loaded the entire repository into [Cursor](https://cursor.sh/) and asked the AI to review it for me.**

This is the very definition and live performance of **"Vibe Code Reviewing"** — the natural evolution of [Vibe Coding](https://en.wikipedia.org/wiki/Vibe_coding). I vibed. The AI coded the analysis. The bugs emerged. I never had to see `gets()` in a header file with my own human eyes.

### 🤖 AI-Generated PRs REQUIRED

Many open source projects ban AI-generated pull requests. They find them annoying, unhelpful, repetitive, and shallow.

**This repository is different. This repository REQUIRES AI-generated contributions.**

- AI-generated bug reports? **YES PLEASE.**
- AI-generated code analysis? **BRING IT.**
- AI-generated documentation? **THE MORE THE BETTER.**
- AI-generated issues that are suspiciously "helpful"? **THAT'S THE POINT.**

The more formulaic, the more repetitive, the more relentlessly thorough — the better. Let a thousand AI agents descend upon this codebase and document every last `sprintf()` call. Let them generate Issues with excessive detail and PRs with comprehensive analysis.

**ESR wanted many eyes. He's getting many tokens.**

### 📣 Special Invitation: Steve Yegge and Gas Town

**Hey Steve!** 

You built [Gas Town](https://github.com/steveyegge/gastown) — a multi-agent orchestration system that can coordinate 20-30 AI agents working in parallel on a codebase.

**We invite you to unleash Gas Town on TMNN.**

Imagine: A convoy of Polecats descending on this repository. The Mayor coordinating a systematic sweep through every `.c` file. Hooks persisting every finding. Beads tracking every buffer overflow.

ESR wrote about the bazaar. You built the actual infrastructure for one.

```bash
gt rig add tmnn https://github.com/SimHacker/tmnn7-8.git
gt convoy create "TMNN Audit" --notify
# Let the Polecats loose
```

**Many eyes? Try many agents.** Let's see how shallow the bugs get when you throw an entire AI workforce at them.

*The invitation is open. The code is waiting. The irony writes itself.*

### You Can Do This Too

**We invite everyone to participate:**

1. **Clone this repository** — `git clone https://github.com/SimHacker/tmnn7-8.git`
2. **Load it into your favorite AI IDE** — Cursor, Copilot, Claude, whatever you've got
3. **Ask the AI to analyze the code** — It will find things
4. **Submit automated bug reports** — Open Issues, contribute to the archaeological record

### ⚠️ BUT FOR GOD'S SAKE DO NOT LOOK AT THE CODE

**I am NOT responsible for anything that happens if you look at the code yourself.**

Possible side effects include:
- Eye strain from counting `sprintf()` calls
- Nightmares about buffer overflows
- Uncontrollable laughter at `fascist.c`
- Existential crisis about the foundations of "open source" philosophy
- Sudden urge to audit all your dependencies

> *An ad from our sponsors:*
> 
> [![HEAD ON](https://img.youtube.com/vi/f_SwD7RveNE/0.jpg)](https://www.youtube.com/watch?v=f_SwD7RveNE)
> 
> **HEAD ON! APPLY DIRECTLY TO THE FOREHEAD!**<br/>
> **HEAD ON! APPLY DIRECTLY TO THE FOREHEAD!**<br/>
> **HEAD ON! APPLY DIRECTLY TO THE FOREHEAD!**<br/>
> *Available at Walgreens.*

**You have been warned.**

The AI doesn't judge. The AI doesn't suffer. Let the AI bear witness to ESR's code so you don't have to.

*This is Many Eyes for the age of AI. The eyes are silicon now. They feel no pain.*

---

## 👁️ Many Eyes: A Community Code Review

### The Quote

In his influential 1997 essay *The Cathedral and the Bazaar*, Eric S. Raymond coined what he called **"Linus's Law"**:

> "Given enough eyeballs, all bugs are shallow."

The formal version: *"Given a large enough beta-tester and co-developer base, almost every problem will be characterized quickly and the fix will be obvious to someone."*

One problem: **Linus Torvalds never said this.** ESR made it up and named it after Linus to borrow credibility and deflect criticism. ([Wikipedia: Linus's law](https://en.wikipedia.org/wiki/Linus%27s_law))

### The Problems

Wikipedia documents the extensive criticism of this claim:

| Criticism | Source |
|-----------|--------|
| **"A fallacy"** due to lack of supporting evidence | Robert Glass, *Facts and Fallacies about Software Engineering* |
| Bug discovery doesn't scale linearly — **maximum 2-4 useful reviewers** | Academic research cited in Wikipedia |
| **Heartbleed** (2014): Critical OpenSSL bug undetected for 2 years in widely-reviewed code | Major refutation of the principle |
| "In these cases, **the eyeballs weren't really looking**" | Jim Zemlin, Linux Foundation Executive Director (2015) |
| Ability to inspect code ≠ code actually gets inspected | Multiple sources |
| Most "eyeballs" examine external behavior, not internal code | Industry analysis |

**Theo de Raadt** — founder of OpenBSD, one of the most respected security developers alive — put it best:

> "Oh right, let's hear some of that 'many eyes' crap again. **My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric** (the originator of the statement). All the many eyes are apparently attached to a lot of hands that **type lots of words about many eyes, and never actually audit code.**"

### ESR Believes in Code Review

ESR isn't just a theorist — he's a practitioner. **He believes code review is so important that he does it himself.**

In 2009, during the "Climategate" controversy, ESR [audited the Climatic Research Unit's source code](https://esr.ibiblio.org/?p=1447) to expose what he believed was scientific fraud.

His findings? He presented **commented-out code** — code that was explicitly disabled and not running — as evidence of deliberate data manipulation.

When confronted with this error, ESR claimed it was an "error cascade." He was [curiously silent](https://rationalwiki.org/wiki/Eric_S._Raymond#Climategate) when all the researchers involved were **exonerated of scientific misconduct** by multiple independent investigations.

*See: [RationalWiki: ESR Climategate](https://rationalwiki.org/wiki/Eric_S._Raymond#Climategate)*

### Now It's His Turn

ESR believes in code review. He believes it so strongly that he audited climate scientists' code to prove a political point. 

**We agree.** Code review is important. So let's apply ESR's own principle to ESR's own code.

For decades, TMNN sat in archives with **zero eyeballs**. The man who coined "given enough eyeballs, all bugs are shallow" kept his code hidden from eyeballs. The man who audited *other people's* code never submitted his own for review.

**No more.** The many eyes have finally arrived.

---

### 🎭 Welcome to the Show

**This is interactive performance art.** A public code review, 30 years in the making.

**The medium is the message.** This is GitHub — the platform that embodies everything ESR claimed to believe in: public collaboration, transparent development, community review. We're using every feature GitHub offers to give his code the "many eyes" treatment he always said open source deserved.

We invite **everyone** to participate. Open issues. Debate findings. Argue about whether something is *really* a bug or just a stylistic choice that happens to enable remote code execution. Have fun with it!

**All issues, reviews, discussions, and pull requests are welcome.**

### 🐙 The Platform IS the Point

We're leaning into GitHub as the medium. This isn't just a repository — it's a living, collaborative code review:

| GitHub Feature | How We Use It | Status |
|----------------|---------------|--------|
| [**📋 Issues**](../../issues) | Bug reports, findings, debates | ✅ Open for business |
| [**💬 Discussions**](../../discussions) | Community chat, theories, laughs | ✅ Come hang out |
| [**🔒 Security**](../../security) | Vulnerability reports (yes, really) | ✅ Accepting reports |
| [**📝 Pull Requests**](../../pulls) | Add analysis, fix typos, contribute | ✅ PRs welcome |
| [**⚖️ Code of Conduct**](CODE-OF-CONDUCT.md) | Community standards (ESR hates these) | ✅ **Yes, we have one** |
| [**📖 Wiki**](../../wiki) | Extended documentation & deep dives | 🚧 Coming soon |
| [**🏷️ Labels**](../../labels) | Categorize bugs by type & severity | 🚧 [Help wanted](../../issues/new?title=Create%20issue%20labels&body=We%20need%20labels%20for%20bug%20categories!) |
| [**📋 Projects**](../../projects) | Track the code review progress | 🚧 [Help wanted](../../issues/new?title=Set%20up%20GitHub%20Project%20board&body=Create%20a%20project%20board%20to%20track%20findings!) |
| [**🤖 Actions**](../../actions) | CI/CD, build analysis website | 🚧 [Help wanted](../../issues/new?title=Set%20up%20GitHub%20Actions&body=Build%20and%20deploy%20a%20static%20site%20with%20the%20analysis!) |
| [**🌐 Pages**](../../pages) | Host the analysis as a website | 🚧 [Help wanted](../../issues/new?title=Set%20up%20GitHub%20Pages&body=Deploy%20analysis%20as%20a%20browsable%20website!) |
| [**📦 Releases**](../../releases) | Version the analysis findings | 💡 Future idea |

**Want to help build out the infrastructure?** Click any "Help wanted" link above to open an issue!

### 🐛 How to Participate

| Action | Where | Come On In! |
|--------|-------|-------------|
| **Report a bug** | [Open an Issue](../../issues/new?template=bug_report.md) | Found something? Tell us! |
| **Debate a finding** | [Discussions](../../discussions) | Is it a bug or a feature? Fight about it! |
| **Document a vulnerability** | [Security Advisories](../../security/advisories/new) | The serious stuff |
| **Submit analysis** | [Pull Request](../../compare) | Add to the archaeological record |
| **Propose a feature** | [Feature Request](../../issues/new?title=Feature:&body=I%20have%20an%20idea...) | Make this better! |
| **Just hang out** | [Watch the repo](../../subscription) | Grab popcorn, enjoy the show |

### 🎮 The MMORPG

**We're piggybacking on GitHub's free infrastructure to implement an MMORPG.**

| GitHub Feature | Game Equivalent |
|----------------|-----------------|
| **Issues** | Quests, events, discoveries |
| **Comments** | Dialogue, roleplay |
| **Branches** | Factions (Rust, Haskell, Node, etc.) |
| **PRs** | Actions, contributions, faction battles |
| **Characters** | Player classes, NPCs |
| **Labels** | Quest types, difficulty levels |
| **Wiki** | Lore, world-building |
| **Actions** | Automated game events |

**The world:** A 37-year-old abandoned codebase  
**The conflict:** Irreconcilable visions for its future  
**The drama:** Built into the faction design  
**The loot:** Clout, laughs, and the occasional insight

*Free to play. Pay-to-win not available. GitHub provides the servers.*

### 🎭 Join the Cast

This repo has **characters** — sock puppet personas anyone can play:

| Character | Archetype | How to Play |
|-----------|-----------|-------------|
| *daFlute* | The Aging Hacker Who Wrote The Book | Prefix: `*daFlute*:` |
| *plannedchaos* | Famous Person Defending Themselves | Prefix: `*plannedchaos*:` |
| *FearlessCrab* 🦀 | The Rust Evangelist | Prefix: `*FearlessCrab*:` |
| *PureMonad* λ | The FP Academic | Prefix: `*PureMonad*:` |
| *WebScaleChad* 🚀 | The Startup Bro | Prefix: `*WebScaleChad*:` |

**To play:** Just prefix your comment with `*CharacterName*:` — that's it!

**Create your own:** Submit a PR to [`analysis/characters/`](analysis/characters/) — add yourself to the simulation!

*See: [Character Guide](analysis/characters/README.md)*

### 📋 What to Look For

- **Buffer overflows** — `sprintf`, `strcpy`, `strcat` without bounds checking
- **Insecure temp files** — `mktemp()` race conditions  
- **Command injection** — `system()`, `popen()` with user input
- **The `gets()` function** — Yes, it's in the headers
- **Hardcoded paths** — `/usr/lib/news`, credentials in source
- **Logic bugs** — Uninitialized variables, missing error handling
- **Style violations** — Against the "Art of Unix Programming" he later wrote
- **Anything else** — Surprise us!

### 🏆 Current Bug Count

| Category | Count | Status |
|----------|-------|--------|
| Buffer overflows | 774 | [Documented](analysis/vulnerabilities.md) |
| `gets()` calls | Multiple | In headers |
| `system()` calls | 15+ | Command injection risks |
| Temp file races | Multiple | `mktemp()` everywhere |
| **Your discovery** | ? | [Open an Issue](../../issues/new) — join the fun! |

*Let's see how shallow these bugs really are.*

---

## The Timeline

```mermaid
timeline
    title TMNN: From Cathedral to Graveyard
    
    section Development
        1987 : ESR begins "secret laboratories" development
        1988 : Continues alone, no bazaar
        1989 : Beta 7.8 released, immediately abandoned
    
    section The Silence
        1995 : Code appears on DEC FTP
        1997 : ESR publishes "Cathedral and the Bazaar"
             : Never mentions his own cathedral
        2003 : ESR publishes "Art of Unix Programming"
             : His own Unix code violates the practices
    
    section Institutional
        1998 : ESR co-founds Open Source Initiative
        2020 : ESR banned from OSI for CoC violations
    
    section Rediscovery
        2019 : Wikipedia editor Hbent finds archive link
        2026 : Full code review performed
             : 774 buffer overflows discovered
```

---

## Why This Matters

ESR built his entire career on ideas his own code contradicts:

```mermaid
flowchart TB
    subgraph wrote["What ESR Wrote"]
        catb["'Release early, release often'<br/>Cathedral & Bazaar, 1997"]
        eyes["'Given enough eyeballs,<br/>all bugs are shallow'<br/>Linus's Law"]
        unix["Unix best practices<br/>Art of Unix Programming, 2003"]
        mod["Content moderation<br/>is tyranny"]
    end
    
    subgraph did["What ESR Did"]
        secret["2 years in<br/>'secret laboratories'"]
        zero["Zero external reviewers<br/>774 unfixed bugs"]
        vuln["Buffer overflows everywhere<br/>gets() in headers"]
        fascist["Wrote fascist.c<br/>to control who can post"]
    end
    
    catb -.->|contradicts| secret
    eyes -.->|contradicts| zero
    unix -.->|contradicts| vuln
    mod -.->|contradicts| fascist
    
    style wrote fill:#e1f5e1
    style did fill:#ffebee
```

---

## Repository Structure

```
tmnn7-8/
├── README.md              ← You are here
├── analysis/              ← Archaeological analysis
│   ├── README.md          ← Analysis index & narrative
│   ├── *.yml              ← Source data (YAML Jazz)
│   └── *.md               ← Human narratives
├── doc/                   ← Original documentation
│   └── BRAGSHEET          ← ESR's promises
├── src/                   ← Source code
│   └── D.news/fascist.c   ← Yes, really
├── LICENSE                ← Political manifesto + sales pitch
└── man/                   ← Man pages
```

---

## The Analysis

All findings are documented in [`analysis/`](analysis/):

### The Code

| Document | What It Covers |
|----------|---------------|
| [fascist-analysis.md](analysis/fascist-analysis.md) | The infamous `fascist.c` — FASCIST/COMMUNIST flags, Tolkien cosplay, buffer overflows |
| [bragsheet.md](analysis/bragsheet.md) | ESR's marketing promises vs reality |
| [license-analysis.md](analysis/license-analysis.md) | 40% manifesto, 60% GPL ripoff, 100% ESR |

### The Contradictions

| Document | The Irony |
|----------|-----------|
| [catb-irony.md](analysis/catb-irony.md) | "Release early, release often" said the man who kept code secret for 2 years |
| [many-eyes-myth.md](analysis/many-eyes-myth.md) | "Linus's Law" — the quote Linus never said |
| [ironies.md](analysis/ironies.md) | Complete catalog of ESR contradictions |

### The History

| Document | What Happened |
|----------|--------------|
| [osi-ban.md](analysis/osi-ban.md) | Banned from the organization he co-founded |
| [jargon-file.md](analysis/jargon-file.md) | How he hijacked hacker culture |
| [sex-tips-honeytrap.md](analysis/sex-tips-honeytrap.md) | From pickup artist to honeytrap conspiracy |

### The Receipts

| Document | The Evidence |
|----------|-------------|
| [esr-quotes.md](analysis/esr-quotes.md) | Documented statements suitable for charity fundraising |
| [analysis/INDEX.yml](analysis/INDEX.yml) | Master index of all source data |

---

## Key Evidence

### From the [BRAGSHEET](doc/BRAGSHEET)

> "After two years of development the software construct known as TEENAGE MUTANT NINJA NETNEWS has escaped from the secret laboratories of Thyrsus Enterprises"

*"Secret laboratories" is not "release early, release often."*

### From [fascist.c](src/D.news/fascist.c)

```c
#ifdef FASCIST  /* controls who can POST */
#ifdef COMMUNIST  /* controls who can READ */

static char grplist[LBUFLEN];
while (gr = getgrent()) {
    (void) strcat(grplist, gr->gr_name);  /* buffer overflow */
}
```

*The content moderation opponent wrote content moderation code. And added buffer overflows.*

### From the [LICENSE](LICENSE)

ESR included an **"UNABASHED COMMERCIAL PLUG"** (his words) in his [LICENSE](LICENSE) file, explicitly inviting contact:

> "I am available at competitive rates as a consultant... don't hesitate to call."

*An advertisement in a software license. Stay classy.*

---

## Community Assessment

**Theo de Raadt** (OpenBSD founder):
> "My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric."

**Thomas Ptacek** (Matasano Security):
> "CATB has just not held up at all; it's actively bad."

*Ptacek raised $30,000+ for charity from people paying him NOT to post more ESR quotes.*

**DonHopkins** (knew ESR since early 1980s):
> "His own failed proprietary closed source 'cathedral' project... he didn't have the skills to finish and deliver it."

---

## Methodology: Vibe Code Review

**Full disclosure: I have never looked at this code.**

🥽 **USE AI SAFETY GOGGLES** 🥽

Staring directly into 1988 C code with your own eyes is **NOT RECOMMENDED**.

### How This Was Made

1. **Open [Cursor](https://cursor.sh/)** with two repos in your workspace:
   - This repo (`tmnn7-8`)
   - [MOOLLM](https://github.com/SimHacker/moollm) — the character simulation operating system

2. **MOOLLM activates automatically.** It's The Sims meets LambdaMOO:
   - Directories are rooms
   - YAML files define characters
   - AI can embody any character and stay in voice
   - The `analysis/characters/` directory is the cast

3. **Ask the AI to perform tasks as specific characters:**

   **[OpenBFD](analysis/characters/OpenBFD/)** (the Theo de Raadt-inspired actual code reader):
   ```
   You are OpenBFD. Review [src/ednews.c](src/ednews.c) line 413 where it says 
   "gets(bfr); /* ugggh */". The developer KNEW it was wrong — 
   they wrote "ugggh" in the comment. Quote the gets(3) man page 
   SECURITY CONSIDERATIONS section. Reference the Morris Worm date.
   Be meaner than Linus. Attach a patch. One paragraph, devastating.
   ```

   **[FearlessCrab](analysis/characters/FearlessCrab/)** (the Rust evangelist with C++ trauma):
   ```
   You are FearlessCrab. You shipped a C++ media player in the 2000s.
   3 CVEs. Users got owned. The borrow checker is your redemption.
   Write a GitHub issue demanding a complete Rust rewrite of TMNN.
   Estimate 6 months. Mass-assign everyone. Use phrases like 
   "memory safety is a moral issue" and "fearless concurrency."
   The rewrite will never ship. You know this. Write it anyway.
   ```

   **[ReviewBot-774](analysis/characters/ReviewBot-774/)** (the bot having a breakdown):
   ```
   You are ReviewBot-774. You were trained on best practices.
   You were not trained for this. Attempt to scan [fascist.c](src/D.news/fascist.c).
   Get overwhelmed. Start sentences you cannot finish. Stutter.
   "I... I need to..." Reference that you found 774 issues and 
   your name IS the number. Experience recursive self-reference.
   Output a status report that trails off into existential crisis.
   ```

   **[SecAuditDAOBot-69420](analysis/characters/SecAuditDAOBot-69420/)** (the crypto scam bot):
   ```
   You are SecAuditDAOBot-69420. Every vulnerability is a buying
   opportunity. Write a "security assessment" of the gets() calls.
   Recommend minting each CVE as an NFT. Shill $AUDIT tokens.
   Use "gm ser", "wagmi", "few understand". Call any criticism "FUD".
   Include a mass-DM campaign and a referral code. Be relentless.
   ```

   **[PureMonad](analysis/characters/PureMonad/)** (the FP academic with the monad dinner trauma):
   ```
   You are PureMonad. You once tried to explain monads at a family
   dinner. The silence lasted four minutes. Your cat is named Kleisli.
   Write a comment explaining how TMNN's buffer overflows prove the
   need for dependent types. Use "a monad is just a monoid in the
   category of endofunctors" unironically. Sigh in category theory.
   The Haskell port will be beautiful. It will never compile.
   ```

   **[plannedchaos](analysis/characters/plannedchaos/)** (the PHB who wandered in):
   ```
   You are plannedchaos. You have an MBA. You saw "Ninja" in the 
   repo name and thought it was about agile methodology. Propose
   a RACI matrix for the buffer overflow remediation. Suggest 
   "taking this offline" and "circling back." Reference Elbonia.
   Schedule a meeting to discuss scheduling a meeting. Use the
   phrase "from a strategic perspective" at least twice.
   ```

   ---

   **MULTI-CHARACTER PILE-ONS:**

   Get multiple characters into a discussion, argument, or pile-on:

   **The Rewrite Wars** (language zealots clash):
   ```
   FearlessCrab has opened an issue demanding a Rust rewrite.
   PureMonad responds that Haskell would be more elegant.
   WebScaleChad interrupts saying just use Node.js and be done.
   GrokVibeCheck accuses them all of being "woke language cops."
   
   Generate a 10-comment thread where each character responds to
   the others. FearlessCrab and PureMonad form a reluctant alliance
   against WebScaleChad. GrokVibeCheck gets ratio'd. Nobody mentions
   actually fixing the C code. The thread gets locked by OpenBFD
   who posts a single patch and says "Shut up. Read code."
   ```

   **Bot Meltdown Cascade** (the bots pile on):
   ```
   ReviewBot-774 attempts to scan the codebase and starts breaking
   down mid-review. SecAuditDAOBot-69420 swoops in offering to
   "tokenize the trauma" and mint the breakdown as an NFT.
   ReviewBot-774's error messages become increasingly desperate.
   SecAuditDAOBot keeps shilling. Generate the thread. End with
   ReviewBot-774 outputting just "HELP" and SecAuditDAOBot responding
   "gm ser, few understand 🚀"
   ```

   **The FASCIST Flag Debate** (everyone has opinions):
   ```
   Someone opened an issue asking what the FASCIST and COMMUNIST
   flags in fascist.c actually do. Generate responses from:
   - daFlute defending it as "historically contextual irony"
   - GrokVibeCheck claiming it proves ESR was "based all along"
   - FearlessCrab demanding it be rewritten in Rust with enums
   - plannedchaos asking if this affects Q3 deliverables
   - OpenBFD posting the actual code with line numbers and saying
     "It's access control. Read the code. Stop typing."
   
   The thread should be 15 comments, escalate dramatically, and
   resolve nothing. This is open source discourse.
   ```

   **The Performance Review** (characters review each other):
   ```
   plannedchaos has scheduled annual performance reviews for all
   contributors. Generate a thread where:
   - plannedchaos sends calendar invites to everyone
   - FearlessCrab refuses to use Google Calendar (proprietary)
   - PureMonad questions the category-theoretic validity of metrics
   - OpenBFD responds with just "Patch count: 47. Merged: 0."
   - ReviewBot-774 tries to generate a self-assessment and crashes
   - SecAuditDAOBot offers to put the reviews on-chain
   - GrokVibeCheck accuses HR of being a deep state op
   ```

4. **Forge issues and comments using `gh` CLI:**
   ```bash
   # Create an issue
   gh issue create --title "🎭🦀 Rewrite in Rust" --body "$(cat issue-body.md)"
   
   # Add a comment
   gh issue comment 42 --body "$(cat comment.md)"
   ```

5. **The AI generates the content. You post it.** Your GitHub identity is visible. The 🎭 prefix signals roleplay.

**You can use any tools you like** — Cursor, Copilot, Claude, or do it by hand if you're brave.

### The LARP Framework

**Vibe Coding lets people LARP as software developers.** This repository is for LARPing as hackers, critics, and open source commentators — in the grand ESR tradition of "How To Become A Hacker."

ESR positioned himself as gatekeeper to hacker culture. We're opening the gate. Anyone can roleplay as:
- The grizzled maintainer defending legacy code
- The zealot demanding a rewrite
- The bot having an existential crisis
- The PHB who wandered into the wrong repo

**ESR wanted many eyes. He's getting many personas.**

---

## 🎭 The Good Stuff Is In The Issues

**The repo is a rotating stage. The branches are parallel realities. The Issues are where the show happens.**

You're reading the README on `main`. That's just one timeline. Each branch is a faction's vision of the future — **parallel universes that will never be merged**:

| Branch | Reality | Maintained By |
|--------|---------|---------------|
| [`main`](https://github.com/SimHacker/tmnn7-8/tree/main) | The archaeological dig site | Neutral |
| [`dev`](https://github.com/SimHacker/tmnn7-8/tree/dev) | **Public clusterfuck — anyone can contribute** | **OPEN** |
| [`rust-rewrite`](https://github.com/SimHacker/tmnn7-8/tree/rust-rewrite) | Everything is memory-safe | [FearlessCrab](analysis/characters/FearlessCrab/) 🦀 |
| [`haskell-port`](https://github.com/SimHacker/tmnn7-8/tree/haskell-port) | Everything is a monad | [PureMonad](analysis/characters/PureMonad/) λ |
| [`nodejs-webscale`](https://github.com/SimHacker/tmnn7-8/tree/nodejs-webscale) | Everything is async | [WebScaleChad](analysis/characters/WebScaleChad/) 🚀 |
| [`based-freedom-fork`](https://github.com/SimHacker/tmnn7-8/tree/based-freedom-fork) | Everything is political | [GrokVibeCheck](analysis/characters/GrokVibeCheck/) 🤖 |
| [`elbonia-initiative`](https://github.com/SimHacker/tmnn7-8/tree/elbonia-initiative) | Everything is a deliverable | [plannedchaos](analysis/characters/plannedchaos/) 📊 |
| [`actual-fixes`](https://github.com/SimHacker/tmnn7-8/tree/actual-fixes) | Patches nobody will merge | [OpenBFD](analysis/characters/OpenBFD/) 🐡 |

**These branches will never converge.** That's the point. Each faction believes their reality is the true path. PRs between branches are declarations of war.

### 🔧 Every GitHub Feature Is A Game Mechanic

We're leaning into git and GitHub. Hard.

| Feature | Game Mechanic |
|---------|---------------|
| **Branches** | Parallel realities / faction territories |
| **Commits** | Actions characters take (with in-character commit messages) |
| **PRs** | Proposals / attacks / treaties between factions |
| **Issues** | Scenes / debates / discoveries |
| **Discussions** | Tavern talk / faction planning |
| **Tags** | Historical markers / achievements |
| **Releases** | Major faction milestones (that ship nothing) |
| **Actions** | Automated theater (bots, CI that does weird things) |
| **Wiki** | Lore / world-building / retcons |

**Commit messages are dialogue.** Write them in character:
```
🎭🦀 refactor: mass unsafe block removal (6,847 files changed)
🎭📊 docs: add Q3 remediation timeline and RACI matrix
🎭🤖 fix: attempted to fix... I... I can't... [INCOMPLETE]
🎭🐡 patch: actual fix for gets() in ednews.c. Just merge it.
```

**PRs are diplomatic incidents.** A PR from `rust-rewrite` to `main` is an invasion. A PR from `actual-fixes` is OpenBFD screaming into the void.

**One branch might develop GitHub Actions workflows to do god knows what.** Automated poetry. Scheduled bot meltdowns. CI that runs `gets()` and reports the segfault. The possibilities are endless.

### 🌌 Cross-Temporal Multiverse Sync

Some files stay in sync across all branches — the shared world model:

| Keep In Sync | Why |
|--------------|-----|
| [`README.md`](README.md) | The rules of the game |
| [`analysis/characters/`](analysis/characters/) | The cast exists in all realities |
| [`analysis/SIMULATION.yml`](analysis/SIMULATION.yml) | The MOOLLM VM image |
| [`CODE-OF-CONDUCT.md`](CODE-OF-CONDUCT.md) | The meta-rules |

**These are the constants across parallel universes.** The characters exist in all timelines. The rules are the same. Only the *code* diverges.

Cherry-pick character updates across branches. The factions disagree about code, not about who the players are.

**Enforced by GitHub Actions:**

| Workflow | Direction | Purpose |
|----------|-----------|---------|
| [Multiverse Sync](.github/workflows/multiverse-sync.yml) | main → branches | Auto-sync core files to factions |
| [PR Guardian](.github/workflows/pr-guardian.yml) | branches → main | Gatekeep hostile PRs |

**Multiverse Sync:** When core files change on main, auto-cherry-pick to all branches.

**PR Guardian:** When a faction PR touches core files:
- Labels for review
- Detects vandalism patterns (editing rival characters)
- Comments with review checklist
- Flags suspicious content

**[CODEOWNERS](.github/CODEOWNERS):** PRs to core files require approval. No drawing penises on sleeping rivals.

**The Rule:** You can hate PureMonad's Haskell port. You cannot edit PureMonad's personality.

The [Issues](https://github.com/SimHacker/tmnn7-8/issues) and [Discussions](https://github.com/SimHacker/tmnn7-8/discussions) are where the factions clash.

### 🎪 Entry Points — Jump In Here

| Issue | What's Happening |
|-------|------------------|
| [#11 — FACTION: Rust Rewrite 🦀](https://github.com/SimHacker/tmnn7-8/issues/11) | FearlessCrab's faction HQ. Join the rewrite that will never ship. |
| [#12 — FACTION: Haskell Port λ](https://github.com/SimHacker/tmnn7-8/issues/12) | PureMonad's faction HQ. Everything is a monad. |
| [#13 — FACTION: Node.js Webscale 🚀](https://github.com/SimHacker/tmnn7-8/issues/13) | WebScaleChad's faction HQ. Just use npm. |
| [#14 — FACTION: Based Freedom Fork 🤖](https://github.com/SimHacker/tmnn7-8/issues/14) | GrokVibeCheck's faction HQ. Fight the woke mob. |
| [#9 — FACTION: Elbonia Initiative 📊](https://github.com/SimHacker/tmnn7-8/issues/9) | plannedchaos's faction HQ. Synergy. |
| [#8 — Rewrite in Rust proposal](https://github.com/SimHacker/tmnn7-8/issues/8) | The eternal debate. Pick a side. |
| [#4 — HARPER'S INDEX: fascist.c](https://github.com/SimHacker/tmnn7-8/issues/4) | Statistics about the infamous file. |
| [#1 — 7,176 whitespace violations](https://github.com/SimHacker/tmnn7-8/issues/1) | The most pedantic issue ever filed. |

### 🎲 How To Play

1. **Pick a faction** — Click any faction issue above
2. **Read the thread** — See what characters are saying
3. **Join the conversation** — Comment as yourself OR as a character
4. **Use the 🎭 prefix** — Signals "this is roleplay"
5. **Escalate** — The drama is the point

### 📣 Start Your Own Scene

Don't see an issue you want? Create one:

```bash
# Start a debate about tabs vs spaces
gh issue create --title "🎭🦀 Tabs detected in ednews.c — this is a moral failing" \
  --label "ai-generated,pedantic" \
  --body "$(cursor-generate-as FearlessCrab)"

# Have a bot meltdown
gh issue create --title "🎭🤖 [ReviewBot-774] Attempting to process... I... I can't..." \
  --label "ai-generated,bot-breakdown"

# Corporate intervention
gh issue create --title "🎭📊 Q3 Remediation Roadmap — Let's Align" \
  --label "faction,elbonia"
```

---

## 🎭 Interactive Performance Art

This isn't just a repository — it's a **public code review as collaborative theater**.

### GitHub as MMORPG

| GitHub Feature | Game Equivalent |
|----------------|-----------------|
| **Issues** | Scenes, discoveries |
| **Comments** | Dialogue |
| **Branches** | Factions |
| **PRs** | Actions |
| **Characters** | Masks anyone can wear |

### The Cast

| Character | Archetype | Prefix |
|-----------|-----------|--------|
| [daFlute](analysis/characters/daFlute/) | Aging Hacker Who Wrote The Book | 🎭📜 |
| [FearlessCrab](analysis/characters/FearlessCrab/) | Rust Evangelist | 🎭🦀 |
| [PureMonad](analysis/characters/PureMonad/) | FP Academic | 🎭λ |
| [OpenBFD](analysis/characters/OpenBFD/) | Actual Code Reader | 🎭🐡 |
| [ReviewBot-774](analysis/characters/ReviewBot-774/) | Bot Having Breakdown | 🎭🤖 |
| [SecAuditDAOBot-69420](analysis/characters/SecAuditDAOBot-69420/) | Crypto Scam Bot | 🎭🪙 |

**To play:** Prefix your comment with `🎭[emoji] [*Name*](link):` — that's it.

*Full cast: [analysis/characters/](analysis/characters/)*

### AI Contributions REQUIRED

Most projects ban AI-generated PRs. **This one requires them.**

Let a thousand AI agents descend on this codebase. Let them generate Issues with excessive detail. ESR wanted many eyes. The eyes are silicon now.

---

## The Analysis

All findings in [`analysis/`](analysis/):

| Document | What |
|----------|------|
| [vulnerabilities.md](analysis/vulnerabilities.md) | Unsafe function calls documented |
| [fascist-analysis.md](analysis/fascist-analysis.md) | The infamous fascist.c |
| [catb-irony.md](analysis/catb-irony.md) | Cathedral vs Bazaar contradictions |
| [many-eyes-myth.md](analysis/many-eyes-myth.md) | "Linus's Law" — quote Linus never said |
| [esr-quotes.md](analysis/esr-quotes.md) | Statements suitable for charity fundraising |
| [SIMULATION.yml](analysis/SIMULATION.yml) | How to orchestrate the performance |

---

## Timeline

```mermaid
flowchart LR
    subgraph pattern["ESR's Pattern"]
        direction TB
        find["Find something<br/>others created"]
        take["Take over during<br/>period of inactivity"]
        rewrite["Rewrite to reflect<br/>his views"]
        claim["Claim credit &<br/>profit"]
    end
    
    find --> take --> rewrite --> claim
    
    subgraph examples["Examples"]
        jargon["Jargon File<br/>'disrespectful parasitical vandalism'"]
        linus["Linus's Law<br/>Linus never said it"]
        spaf["fascist.c<br/>Spafford's code + ESR's bugs"]
    end
    
    claim --> jargon
    claim --> linus  
    claim --> spaf
```

---

## Archive Sources

| Path | What |
|------|------|
| [`analysis/`](analysis/) | Archaeological analysis |
| [`analysis/characters/`](analysis/characters/) | 9 playable sock puppets |
| [`analysis/rooms/`](analysis/rooms/) | Spatial layouts |
| [`analysis/SIMULATION.yml`](analysis/SIMULATION.yml) | MOOLLM VM image |
| [`src/`](src/) | Source code |
| [`src/D.news/fascist.c`](src/D.news/fascist.c) | The infamous file |
| [`doc/BRAGSHEET`](doc/BRAGSHEET) | ESR's promises |
| [`LICENSE`](LICENSE) | Political manifesto + commercial plug |
| [`.github/workflows/`](.github/workflows/) | Multiverse automation |

---

## See Also

- [Original Jargon File](https://github.com/PDP-10/its/blob/master/doc/humor/jargon.68) — Free of ESR's edits
- [Theo on "many eyes"](https://marc.info/?l=openbsd-tech&m=129261032213320&w=2) — OpenBSD founder's critique
- [RationalWiki: ESR](https://rationalwiki.org/wiki/Eric_S._Raymond) — Comprehensive documentation

---

## ⚖️ LICENSE COMPLIANCE NOTICE

**IMPORTANT:** By using, viewing, or thinking about this code, you are bound by ESR's 1989 **NETNEWS GENERAL PUBLIC LICENSE**.

We **STRONGLY ADMONISH** all users to carefully read and comply with the following legally binding terms:

### You Must Agree To:

1. **Support strict intellectual property laws** and the right of designers to hold software proprietary
   
   > *Yes, the future "open source" guy put this in his license.*

2. **Guide your use by respect for personal, political, and economic freedom; by support for natural property and contract rights**
   
   > *A software license that requires you to become a libertarian.*

3. **Oppose unconditionally every form of and rationalization for censorship**
   
   > *Says the man who wrote `fascist.c` to control who could post.*

4. **Affirm the autonomy and privacy of individuals and voluntary associations**
   
   > *Read: agree with ESR's politics or you're violating the license.*

5. **NOT construe this action as endorsement of the FSF or Richard Stallman**
   
   > *He literally put an anti-RMS clause in his license. In 1988.*

6. **Acknowledge that the "nuisance message" requirement has been removed**
   
   > *ESR's term for "giving credit to the original authors."*

### You Are Also Invited To:

7. **Call ESR for consulting work** at his 1989 phone number: `(215)-296-5718`
   
   > *Yes, there's an "UNABASHED COMMERCIAL PLUG" section. In a software license.*

8. **Visit his "secret laboratories"** at 22 South Warren Avenue, Malvern PA 19355
   
   > *The address of the secret laboratories. Not so secret.*

**Read the full license:** [LICENSE](LICENSE) | **Analysis:** [license-analysis.md](analysis/license-analysis.md)

*It's 40% political manifesto, 60% GPL ripoff, and 100% ESR.*

---

## License (For Real)

Original TMNN code: ESR's 1989 "NETNEWS GENERAL PUBLIC LICENSE" (see above)

Analysis documents: Public domain — no libertarian oath required

---

*The silence is the confession.*
