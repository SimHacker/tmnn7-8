# The Green Room 🎭

**Where the players gather before going on stage.**

---

## What Is This Place?

This is the **Green Room** — the backstage area where performers wait before their entrance. In theater tradition, it's where actors prepare, rehearse lines, and get into character.

In this repository, it's where our **sock puppet characters** live. Each subdirectory contains a character that anyone can play.

---

## ⚠️ Ethical Framework & Disclaimers

### These Are Not Real People

| Character | Is NOT |
|-----------|--------|
| daFlute | Eric S. Raymond |
| plannedchaos | Scott Adams |
| FearlessCrab | Any specific Rust developer |
| PureMonad | Any specific Haskell developer |
| WebScaleChad | Any specific startup founder |

**These are archetypes.** Patterns of behavior observed in online technical communities, distilled into playable masks.

### Why Masks?

1. **Defuse hostility** — When conflict is theater, it's not personal
2. **Surface patterns** — Making archetypes explicit makes them visible
3. **Enable participation** — Anyone can join without creating drama
4. **Create comedy** — The drama writes itself when archetypes collide

### The Rules

1. **Never claim a character IS a real person**
2. **Always use the 🎭 prefix** — It signals "this is roleplay"
3. **Your real identity is visible** — GitHub shows who you are
4. **Play respectfully** — Satire, not harassment

---

## 🎭 The Emoji Protocol

When posting as a character, use this format:

```
🎭📜 [*daFlute*](link): Your message here
```

### Emoji Layers

| Position | Purpose | Example |
|----------|---------|---------|
| 1st | 🎭 Theater mask | Always first — "This is roleplay" |
| 2nd | Primary character emoji | 📜 📊 🦀 λ 🚀 |
| 3rd+ | Secondary character emojis | Character-specific flavor |
| Last | Statement emojis (optional) | 💩 🔥 ❤️ etc. |

### Examples

```markdown
🎭📜 [*daFlute*](link): I wrote the book on this.
🎭📜📚 [*daFlute*](link): As I explained in my essay...
🎭📜💢 [*daFlute*](link): The SJWs are at it again.
🎭🦀🔥 [*FearlessCrab*](link): This code is on FIRE with buffer overflows!
🎭📊📈💼 [*plannedchaos*](link): Let's align on Q3 deliverables.
🎭🚀💩 [*WebScaleChad*](link): Your architecture is garbage, just use Node.
```

---

## The Cast

| Prefix | Character | Archetype | Branch |
|--------|-----------|-----------|--------|
| 🎭📜 | [daFlute](daFlute/) | The Aging Hacker Who Wrote The Book | `dev` |
| 🎭📊 | [plannedchaos](plannedchaos/) | The PHB Who Doesn't Know It | `elbonia-initiative` |
| 🎭🦀 | [FearlessCrab](FearlessCrab/) | The Rust Evangelist | `rust-rewrite` |
| 🎭λ | [PureMonad](PureMonad/) | The FP Academic | `haskell-port` |
| 🎭🚀 | [WebScaleChad](WebScaleChad/) | The Startup Bro | `nodejs-webscale` |

### Copy-Paste Signatures

**Open the CHARACTER.yml file for any character and copy the signature from the box at the top.**

Each character file starts with:

```
# ┌─────────────────────────────────────────────────────────────────────────────┐
# │ COPY THIS TO PLAY AS [character]:                                           │
# ├─────────────────────────────────────────────────────────────────────────────┤
# │                                                                             │
# │ 🎭[emoji] [*Name*](link):                                                   │
# │                                                                             │
# │ Add more emojis for narrative flavor: [secondary emojis]                    │
# └─────────────────────────────────────────────────────────────────────────────┘
```

**Quick reference:**

```markdown
🎭📜 [*daFlute*](https://github.com/SimHacker/tmnn7-8/blob/main/analysis/characters/daFlute/): 
🎭📊 [*plannedchaos*](https://github.com/SimHacker/tmnn7-8/blob/main/analysis/characters/plannedchaos/): 
🎭🦀 [*FearlessCrab*](https://github.com/SimHacker/tmnn7-8/blob/main/analysis/characters/FearlessCrab/): 
🎭λ [*PureMonad*](https://github.com/SimHacker/tmnn7-8/blob/main/analysis/characters/PureMonad/): 
🎭🚀 [*WebScaleChad*](https://github.com/SimHacker/tmnn7-8/blob/main/analysis/characters/WebScaleChad/): 
```

### Secondary Emojis (Narrative Flavor)

Add emojis after the primary to express emotion/context:

| Character | Primary | Secondary Options |
|-----------|---------|-------------------|
| daFlute | 📜 | 📚💢🤬😤🙄🎸🔫🏆 |
| plannedchaos | 📊 | 📈💼🤝📋🎯📆😏🌍☕ |
| FearlessCrab | 🦀 | 🔥⚡🛡️🔒📦🎯💪🙅✅ |
| PureMonad | λ | 📐🎓😌🤔📚∀∃→🧮😔 |
| WebScaleChad | 🚀 | 💰📈😂💩☁️⚡🏃📱🎉🤷 |

**Examples with secondary emojis:**

```markdown
🎭📜💢 [*daFlute*](...): The SJWs are at it again!
🎭🦀🔥 [*FearlessCrab*](...): This code is ON FIRE with buffer overflows!
🎭📊😏 [*plannedchaos*](...): I've been following daFlute's work...
🎭λ😔 [*PureMonad*](...): *sighs in category theory*
🎭🚀💩 [*WebScaleChad*](...): Your architecture is garbage, just use Node
```

---

## How to Play (No Special Tools Required)

### Method 1: Just Post

1. Copy a character prefix from above
2. Paste it at the start of your GitHub comment
3. Write in character
4. Post

**That's it.** No software required. No accounts to create. Just GitHub.

### Method 2: Create Your Own Character

Want to add a character? Just copy and edit:

```bash
# 1. Copy an existing character
cp -r analysis/characters/FearlessCrab analysis/characters/YourHandle

# 2. Edit the files
#    - CHARACTER.yml  (the character sheet)
#    - README.md      (quick reference)

# 3. Submit a PR
```

**The CHARACTER.yml format:**

```yaml
# YourHandle - A Sock Puppet Character
# This is a MASK. Anyone can wear it.

# ROLEPLAY FORMAT - Copy this to play:
# 🎭🎯 [*YourHandle*](https://github.com/SimHacker/tmnn7-8/blob/main/analysis/characters/YourHandle/): 

meta:
  type: sock_puppet
  playable_by: anyone
  emoji: 🎯           # Your primary emoji
  prefix: "🎭🎯"      # Theater + your emoji
  
character:
  name: YourHandle
  archetype: "The [Your Archetype]"
  
personality:
  traits:
    - Trait 1
    - Trait 2
    
  speech_patterns:
    - "Catchphrase 1"
    - "Catchphrase 2"
    
  blind_spots:
    - What makes them funny
    
catchphrases:
  - "Your signature line"
```

---

## Advanced: MOOLLM Integration (Optional)

This repository is designed to work **standalone** — you don't need any special tools.

However, the character files are also compatible with **[MOOLLM](https://github.com/SimHacker/moollm)** — a microworld OS for LLMs where directories are rooms, YAML files are alive, and AI can embody characters with full context.

### What is MOOLLM?

MOOLLM treats the filesystem as a navigable world:
- **Directories are rooms** — Enter a directory to activate its context
- **YAML files are semantic** — Comments carry meaning ([YAML Jazz](https://github.com/SimHacker/moollm/tree/main/skills/yaml-jazz))
- **Characters are playable** — AI can embody them with personality, speech patterns, blind spots
- **Skills are inheritable** — Build on patterns, don't reinvent them

### Key MOOLLM Resources

| Resource | What It Teaches |
|----------|-----------------|
| [**Character Skill**](https://github.com/SimHacker/moollm/tree/main/skills/character) | How to define playable characters |
| [**Room Skill**](https://github.com/SimHacker/moollm/tree/main/skills/room) | Spatial layouts, furniture, positioning |
| [**YAML Jazz Skill**](https://github.com/SimHacker/moollm/tree/main/skills/yaml-jazz) | Comments as semantic data |
| [**Incarnation Skill**](https://github.com/SimHacker/moollm/tree/main/skills/incarnation) | Gold-standard character creation |
| [**Adventure Skill**](https://github.com/SimHacker/moollm/tree/main/skills/adventure) | Room-based exploration games |
| [**K-Lines Skill**](https://github.com/SimHacker/moollm/tree/main/skills/k-lines) | Symbolic activators (like emoji protocols!) |

### Example: Adventure-4 Characters

See how characters are structured in a full MOOLLM adventure:

- [**Adventure-4 Characters Directory**](https://github.com/SimHacker/moollm/tree/main/examples/adventure-4/characters) — Example character organization
- [**CHARACTERS.yml**](https://github.com/SimHacker/moollm/blob/main/examples/adventure-4/CHARACTERS.yml) — Master character registry
- [**Fictional Characters**](https://github.com/SimHacker/moollm/tree/main/examples/adventure-4/characters/fictional) — AI personas, archetypes
- [**Abstract Characters**](https://github.com/SimHacker/moollm/tree/main/examples/adventure-4/characters/abstract) — Conceptual entities

### What MOOLLM Adds

| Feature | Without MOOLLM | With MOOLLM |
|---------|----------------|-------------|
| Play characters | Copy-paste prefix | AI embodiment |
| Character context | Read the YAML | Auto-loaded on entry |
| Inheritance | Copy and edit | `inherits_from: ../base/` |
| Spatial positioning | Descriptive | Functional |
| Cross-repo characters | Manual | `path: @other-repo/characters/` |

### How to Use with MOOLLM

**Option 1: Cursor IDE + MOOLLM**
1. Clone [MOOLLM](https://github.com/SimHacker/moollm) alongside this repo
2. MOOLLM skills auto-activate in Cursor
3. AI can embody characters when you enter their directory

**Option 2: Link as a World**
```yaml
# In your MOOLLM config
worlds:
  tmnn7-8:
    path: ../tmnn7-8
    characters: analysis/characters/
```

**Option 3: Just Read the Skills**

Even without using MOOLLM, reading the skill documentation teaches you:
- How to structure character files
- What fields matter and why
- Patterns that make characters "playable" by AI

### Future Rooms

Each character could have their own room modeling their dev environment:
- **daFlute's Study** — Books everywhere, old terminals, `vi` keybindings
- **FearlessCrab's Workshop** — Rust posters, zero `unsafe` blocks allowed
- **PureMonad's Office** — Whiteboards full of category theory, Haskell books
- **WebScaleChad's Startup** — Standing desks, ping pong, AWS dashboard
- **plannedchaos's Corner Office** — PowerPoint decks, RACI matrices, motivational posters

*Coming soon: Character dev environments as ROOM.yml files.*

### Future Platforms

We plan to support:
- **Cursor IDE** — Native integration with MOOLLM skills *(current)*
- **Other AI IDEs** — As they emerge
- **Standalone CLI** — For terminal-based roleplay
- **Discord bots** — Character embodiment in chat
- **Web interface** — Browser-based character play

**But you don't need any of this.** The copy-paste format works everywhere GitHub does.

---

## The Files

Each character directory contains:

| File | Purpose | Required? |
|------|---------|-----------|
| `CHARACTER.yml` | Full character specification | Yes |
| `README.md` | Quick reference for players | Yes |
| `ROOM.yml` | MOOLLM spatial data | Optional |

### CHARACTER.yml Structure

```yaml
meta:           # Metadata about the character
  type: sock_puppet
  playable_by: anyone
  emoji: 🎯
  
character:      # Who they are
  name: Handle
  archetype: "The Archetype"
  
personality:    # How they behave
  traits: []
  speech_patterns: []
  blind_spots: []
  
catchphrases: [] # Signature lines

behaviors:      # Specific behaviors
  code_review: {}
  conflict_style: []
```

---

## This Room

You're in the **Green Room**. Look around:

- The **loveseat** where daFlute and plannedchaos sit uncomfortably close
- The **standing desk** where FearlessCrab types furiously
- The **beanbag** where WebScaleChad checks his phone
- The **armchair** where PureMonad reads category theory papers
- The **refreshments table** with coffee, energy drinks, and craft beer
- The **mirror** where everyone checks their mask before going on stage
- The **call board** showing which issues are "live"

See [ROOM.yml](ROOM.yml) for the full spatial layout.

---

## Join the Cast

**Want to add yourself?**

Create a character that:
- Embodies your actual views (sincerely)
- Parodies your own community's tropes (self-aware)
- Is a total invention (fiction)
- Explores an archetype (theater)

Submit a PR. Join the show.

---

*"All the world's a stage, and all the men and women merely players."*

*"YES AND."*

---

## See Also

- [Main README](../../README.md) — Project overview
- [Code of Conduct](../../CODE-OF-CONDUCT.md) — The satirical CoC
- [Faction Issues](https://github.com/SimHacker/tmnn7-8/issues?q=label%3Afaction) — Join a faction
