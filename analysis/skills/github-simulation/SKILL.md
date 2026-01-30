# github-simulation

> GitHub as MMORPG — Issues are scenes, branches are factions, commits are actions

## Inheritance

This skill inherits from MOOLLM core:
- **adventure** — Room-based exploration with narrative evidence
- **character** — Entity foundation: body, home, inventory, relationships
- **simulation** — Central hub: turns, party, selection, flags
- **procedural-rhetoric** — Rules embody arguments (Bogost)

## The Core Concept

Every GitHub feature is a game mechanic:

| GitHub | Game | Example |
|--------|------|---------|
| Branch | Faction territory | `rust-rewrite` is FearlessCrab's domain |
| Commit | Character action | `🎭🐡 fix(ednews.c): replace gets()` |
| PR | Diplomatic incident | PR from `rust-rewrite` to `main` is invasion |
| Issue | Scene/debate | Characters argue about buffer overflows |
| Comment | Dialogue | In-character responses |
| Release | Faction milestone | Ships nothing, but ceremonially |
| Action | Automated theater | Multiverse sync, PR Guardian |

## SIMULATION.yml

The orchestration file tracks runtime state:

```yaml
runtime_state:
  unsafe_calls_fixed: 0
  unsafe_calls_total: 872
  actual_code_improvements: "minimal"
  
characters_active:
  OpenBFD: patching
  FearlessCrab: proposing
  everyone_else: arguing

the_joke: |
  Characters generate endless discussion.
  872 unsafe function calls sit unfixed.
  This is the joke. This is also real open source.
```

## Branch Architecture

Each branch is a parallel reality with its own SOUL.md:

| Branch | Soul | Maintainer |
|--------|------|------------|
| `main` | Archaeological dig site | Neutral |
| `actual-fixes` | What TMNN could have been | OpenBFD 🐡 |
| `rust-rewrite` | Memory safety as religion | FearlessCrab 🦀 |
| `haskell-port` | Purity as philosophy | PureMonad λ |
| `nodejs-webscale` | Move fast break things | WebScaleChad 🚀 |
| `based-freedom-fork` | Politics as code | GrokVibeCheck 🤖 |
| `elbonia-initiative` | Process as product | planned-chaos 📊 |
| `dev` | Chaos as collaboration | Nobody |

**They will never merge.** That's the point.

## Multiverse Sync

Core files stay synchronized across all branches:

```
Synced files (shared world model):
- README.md
- CODE-OF-CONDUCT.md  
- analysis/characters/**
- analysis/SIMULATION.yml
- analysis/rooms/**
```

GitHub Action `multiverse-sync.yml` auto-cherry-picks from `main` to faction branches. Conflicts create issues tagged with the branch name.

**The rule:** You can hate PureMonad's Haskell port. You cannot edit PureMonad's personality.

## PR Guardian

When faction PRs touch core files:
1. Labels for review
2. Detects vandalism patterns (editing rival characters)
3. Comments with review checklist
4. Flags suspicious content

`CODEOWNERS` requires approval for core files. No drawing penises on sleeping rivals.

## Forging Content

### Create an Issue

```bash
# Generate content as character
cursor/claude: "You are FearlessCrab. Write an issue demanding Rust rewrite."

# Save to file
cat > issue-body.md << 'EOF'
🎭🦀 *FearlessCrab*:

[AI-generated content here]
EOF

# Forge the issue
gh issue create \
  --title "🎭🦀 URGENT: Complete Rust rewrite required" \
  --label "ai-generated,rust-rewrite" \
  --body "$(cat issue-body.md)"
```

### Add a Comment

```bash
# Generate response as different character
cursor/claude: "You are OpenBFD. Respond to FearlessCrab. Be devastating."

# Post it
gh issue comment 42 --body "$(cat comment.md)"
```

### Forge a Commit

```bash
# Make changes in character voice
git commit -m "$(cat << 'EOF'
🎭🐡 fix(fascist.c): replace strcpy() with strncpy()

grplist is char[BUFLEN]. BUFLEN is 128 or 256.
getgrplist() returns unbounded string. Overflow.

    BEFORE: strcpy(grplist, getgrplist(user));
    AFTER:  strncpy(grplist, getgrplist(user), sizeof(grplist) - 1);

Nine security fixes. Zero merges. Keeps patching anyway.

— OpenBFD
EOF
)"
```

## Character Prefixes

Every in-character contribution uses 🎭 prefix:

| Character | Prefix | Branch |
|-----------|--------|--------|
| daFlute | 🎭📜 | dev |
| FearlessCrab | 🎭🦀 | rust-rewrite |
| PureMonad | 🎭λ | haskell-port |
| WebScaleChad | 🎭🚀 | nodejs-webscale |
| OpenBFD | 🎭🐡 | actual-fixes |
| ReviewBot-774 | 🎭🤖 | — |
| SecAuditDAOBot-69420 | 🎭🪙 | — |
| GrokVibeCheck | 🎭🤖 | based-freedom-fork |
| planned-chaos | 🎭📊 | elbonia-initiative |

## The Dramatic Irony

While characters debate rewrites, argue about languages, schedule meetings, tokenize vulnerabilities, and have existential crises...

OpenBFD quietly fixes code. Submits patches. Gets ignored.

The `actual-fixes` branch grows. It will never be merged.

**This is open source.**

## Authentication Modes

Who is actually making the GitHub API calls?

### Default: Single User Puppeteer

Use your current `gh` login. All actions come from YOUR GitHub account.

```bash
gh auth status  # Check who you're logged in as
```

The 🎭 prefix and character signatures clearly indicate roleplay. No deception — you're openly puppeteering characters from your account.

**This is the default.** No special setup required.

### Advanced: Dedicated Sock Puppet Accounts

Create actual GitHub accounts for characters (e.g., `OpenBFD-bot`, `FearlessCrab-bot`). Switch auth context per character.

```bash
# Store tokens securely
gh auth login --with-token < ~/.tokens/openbfd.token
# ... post as OpenBFD ...
gh auth login --with-token < ~/.tokens/default.token  # switch back
```

More immersive but requires managing multiple accounts. Respect GitHub ToS on automated accounts.

### Collaborative: Real Users Playing Characters

Multiple humans can participate, each playing assigned characters from their own accounts. The simulation becomes a collaborative improv game.

```
Alice plays: daFlute, planned-chaos
Bob plays: FearlessCrab, PureMonad
Carol plays: OpenBFD (does the actual code fixes)
```

Each person uses their own auth, plays their characters.

## Invocation

```
Orchestrate a github simulation for TMNN.
Have FearlessCrab open an issue demanding Rust rewrite.
Have OpenBFD respond with a patch.
Have planned-chaos schedule a meeting to discuss.
Generate the issue and comments.
```

---

## Operational Knowledge

Lessons cached from running the simulation.

### Multiverse Sync Strategy

**Cherry-pick fails for modify/delete conflicts.** When files are renamed/deleted on branches but modified on main, cherry-pick creates merge conflicts that abort.

**Solution: Force-sync core files.**

```bash
for branch in rust-rewrite haskell-port nodejs-webscale; do
  git checkout $branch
  git checkout main -- analysis/ GLANCE.yml README.md
  git commit -m "Sync core files from main"
  git push origin $branch --force-with-lease
done
```

Core files are authoritative on main. Branches can diverge in code, not in world model.

### Explicit File Paths in Workflows

Globs like `analysis/**` miss edge cases. List files explicitly:

```yaml
paths:
  - analysis/INDEX.yml
  - analysis/SIMULATION.yml
  - analysis/characters/**
  - analysis/skills/**
  - analysis/rooms/**
  - GLANCE.yml
  - src/GLANCE.yml
  - src/D.*/GLANCE.yml
```

Fixed-size microworld = fixed-size file list.

### Character Name Collisions

**Always check before naming characters.**

```bash
gh api users/planned-chaos 2>&1 | grep -q "Not Found" && echo "Available"
```

`plannedchaos` was a real user. Renamed to `planned-chaos` to avoid @mention notifications. Respect real humans.

### Branch Naming

| Pattern | Use |
|---------|-----|
| `rust-rewrite` | Faction branch (FearlessCrab) |
| `actual-fixes` | Working branch (OpenBFD) |
| `main` | Archaeological site (neutral) |

Faction branches have BRANCH.yml (formerly SOUL.md) defining their soul.

### The Joke

While updating `multiverse-sync.yml`, the characters continued arguing about rewrites.

```
unsafe_calls_fixed: 0
unsafe_calls_total: 872
```

The simulation documents itself.

---

## MOOLLM Integration

This skill requires MOOLLM mounted in the same workspace:

```
workspace/
├── tmnn7-8/          # This repo
└── moollm/           # Skills inherit from here
    └── skills/
        ├── adventure/
        ├── character/
        ├── simulation/
        └── ...
```

Without MOOLLM, skills are just documentation. With it, they activate.

---

*See also: [github-user](../github-user/) for characters as actors, [code-archaeology](../code-archaeology/) for OpenBFD's method.*
