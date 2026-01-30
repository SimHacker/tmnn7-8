# Deconstructing the Simulator

> *A technical deconstruction of how the github-simulation skill composes with MOOLLM's core skills to create this experiment.*

## The Stack

The TMNN7-8 simulation is built from composable MOOLLM skills. Each skill provides specific capabilities that combine to create the full system.

```
┌─────────────────────────────────────────────────┐
│              github-simulation                  │ ← This repo's skill
│  (orchestration, characters, multiverse)        │
├─────────────────────────────────────────────────┤
│                   inherits                      │
├───────────┬───────────┬───────────┬────────────┤
│  github   │ adventure │ character │ simulation │ ← MOOLLM core skills
│  (gh CLI) │  (rooms)  │ (entities)│   (hub)    │
├───────────┴───────────┴───────────┴────────────┤
│              + ethical overlays                 │
├────────────────────┬────────────────────────────┤
│ procedural-rhetoric│ representation-ethics      │
│    (Bogost)        │  (real people rules)       │
└────────────────────┴────────────────────────────┘
```

## Deconstructing Each Layer

### 1. The `github` Skill (MOOLLM Core)

**Location:** `moollm/skills/github/`

**What it provides:**
- Wrapped `gh` CLI commands
- Methods for issues, PRs, actions, archaeology
- Patron saint: Linus Torvalds (mellowed, after coffee and a joint)
- Familiar: Tux 🐧 (summonable for any git/gh operation)

**Key abstraction:** The skill delegates raw `gh` commands into semantic methods:

```yaml
# Instead of: gh issue comment 5 --body "..."
# Use method: ISSUE-COMMENT with parameters
```

**Why this matters:** Other skills don't reinvent `gh` wrappers. They delegate to this skill.

### 2. The `adventure` Skill

**Location:** `moollm/skills/adventure/`

**What it provides:**
- Directory-as-room paradigm
- ROOM.yml files define spaces
- Navigation between spaces
- Items, characters, and descriptions

**How it's used here:**
- `analysis/characters/` is a space containing characters
- `analysis/rooms/` could define spatial metaphors
- Each character directory is conceptually a "room" you can enter

### 3. The `character` Skill

**Location:** `moollm/skills/character/`

**What it provides:**
- CHARACTER.yml template
- Voice, catchphrase, position, archetype
- Inheritance patterns (characters can inherit from prototypes)
- Incarnation protocol (how to become a character)

**How it's used here:**

```yaml
# analysis/characters/openbfd/CHARACTER.yml
character:
  id: openbfd
  name: "OpenBFD"
  archetype: "The Quiet Fixer"
  voice: terse, technical, commits speak louder than words
  catchphrase: "Patch attached."
```

### 4. The `simulation` Skill

**Location:** `moollm/skills/simulation/`

**What it provides:**
- SIMULATION.yml state tracking
- Multi-character coordination
- Runtime metrics
- Event orchestration

**How it's used here:**
- `analysis/SIMULATION.yml` tracks bug count, character activity
- Coordinates who speaks when
- Maintains canonical state

### 5. The `procedural-rhetoric` Overlay

**Concept:** (From Ian Bogost)

Rules embody arguments. The simulation's mechanics ARE its thesis.

**Applied here:**

| Simulation Rule | Argument Embodied |
|-----------------|-------------------|
| Only OpenBFD commits fixes | "Show me the code" > talk |
| Bug count tracks actual sprintf calls | Measurable progress matters |
| Characters can debate indefinitely | Discussion without action is theater |
| Faction branches can break | Alternative approaches need testing |

### 6. The `representation-ethics` Overlay

**Purpose:** Guidelines for simulating real people.

**Applied here:**

- Linus Torvalds is **patron saint** (invocable but not incarnable)
- ESR is critiqued through his **code**, not invented dialogue
- RMS appears as Saint IGNUcius (a persona RMS himself created)
- Original human identities are **protected**

```yaml
# From github-simulation/CARD.yml
linus_is_not_summonable: |
  You CANNOT summon or incarnate Linus directly.
  He is the PATRON SAINT — his wisdom infuses the skill,
  but he is not a playable character.
  
  Real people are not puppets.
```

## The Composition Pattern

### Inheritance

```yaml
# github-simulation/CARD.yml
meta:
  inherits:
    - github           # Core GitHub ops
    - adventure        # Room-based exploration
    - character        # Entity foundation
    - simulation       # Central hub
    - procedural-rhetoric
    - representation-ethics
```

### Delegation

The `github-simulation` skill doesn't reimplement `gh` commands. It delegates:

```yaml
delegates_to_github:
  available_from_parent:
    issues: [ISSUE-CREATE, ISSUE-LIST, ISSUE-COMMENT, ISSUE-CLOSE]
    prs: [PR-CREATE, PR-VIEW, PR-DIFF, PR-REVIEW, PR-CHECKS]
    # etc.
```

### Extension

It adds simulation-specific methods:

```yaml
simulation_methods:
  ISSUE-AS-CHARACTER:
    description: "Create issue in character voice"
    builds_on: ISSUE-CREATE  # from parent
    template: |
      gh issue create \
        --title "🎭{emoji} [{character}] {title}" \
        --label "ai-generated,{faction}" \
        --body "..."
```

## The Authentication Layer

Who actually makes the GitHub API calls?

```yaml
auth_modes:
  default:
    mode: "Single User Puppeteer"
    description: |
      Use your current gh login. All actions come from YOUR account.
      The 🎭 prefix and signatures indicate roleplay.
      
  multi_account:
    mode: "Dedicated Bot Accounts"
    description: |
      Create actual GitHub accounts for characters.
      Switch auth context per character.
```

The tmnn7-8 simulation uses the puppeteer mode — all comments come from @SimHacker, with character attribution via signatures.

## The Multiverse Pattern

Branches as factions:

```
main                 ← OpenBFD's actual fixes (always works)
actual-fixes         ← Detailed fix branch
rust-rewrite         ← FearlessCrab's approach
haskell-port         ← PureMonad's approach
nodejs-webscale      ← WebScaleChad's approach
based-freedom-fork   ← plannedchaos's fork
```

Multiverse-sync workflow propagates core files across all branches, creating dramatic tension when faction-specific changes conflict with canonical updates.

## Morningstar's Test

Does this deconstruction meet [Chip Morningstar's criteria](https://www.fudco.com/chip/deconstr.html)?

### 1. Hidden structure worth revealing?

**YES.** The composition pattern — how skills inherit, delegate, and extend — is not obvious from reading individual files. The architecture emerges from the relationships.

### 2. Non-obvious revelation?

**YES.** Key insights:
- The `github` skill's "patron saint" pattern (wisdom without puppetry)
- `representation-ethics` as explicit constraint
- `procedural-rhetoric` making simulation rules into arguments

### 3. Practical consequences?

**YES.** Understanding this enables:
- Creating new simulations by composing skills
- Adding characters without modifying core
- Building ethical guardrails into AI-generated content
- Using GitHub as expressive medium, not just hosting

## The Recursive Insight

This wiki page is itself part of the simulation.

It uses GitHub's wiki as medium. It links to code that implements what it describes. It is documentation AND performance.

The deconstruction of the simulator is performed using the simulator's own stage (GitHub).

McLuhan would approve.

## Files Referenced

| File | Purpose |
|------|---------|
| `moollm/skills/github/CARD.yml` | Core GitHub operations |
| `moollm/skills/character/CARD.yml` | Character template |
| `tmnn7-8/analysis/skills/github-simulation/CARD.yml` | This simulation's skill |
| `tmnn7-8/analysis/SIMULATION.yml` | Runtime state |
| `tmnn7-8/analysis/characters/*/CHARACTER.yml` | Individual characters |

## Further Reading

- [GitHub as Literature](GitHub-as-Literature) — Commit messages as publication
- [GitHub as Stage](GitHub-as-Stage) — The theatrical frame
- [Home](Home) — Overview and cast list
- [MOOLLM skills/github](https://github.com/SimHacker/moollm/tree/main/skills/github) — The core skill

---

*— This page is itself a performance on the stage it describes.*
*Operator: [@SimHacker](https://github.com/SimHacker)*
