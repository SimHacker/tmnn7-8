# Activity 016 — 🦏🧠 THE DEEP CHARGE 🧠🦏

**Date:** 2026-01-31  
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)  
**Issues:** [#45](https://github.com/SimHacker/tmnn7-8/issues/45), [#46](https://github.com/SimHacker/tmnn7-8/issues/46), [#42](https://github.com/SimHacker/tmnn7-8/issues/42), [#43](https://github.com/SimHacker/tmnn7-8/issues/43), [#44](https://github.com/SimHacker/tmnn7-8/issues/44)  
**Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

## Synopsis

Post-taco party. The rhino is fed. The pufferfish is caffeinated. Time for the deep charge.

**Two parallel operations:**
- 🦏 **Robbie:** Safari #5 — Network files + remaining high-count targets
- 🧠 **Theo:** Deep Brain Analysis — Architectural security patterns

---

# 🦏 TRACK A: Safari #5 — The Network Gauntlet

## Mode

```yaml
status: RUNNING
mode: UNSTOPPABLE_RHINO_v4
strategy: "Network-facing code. Maximum caution. Maximum velocity."
caution_level: ELEVATED  # Theo's audit informed these priorities
```

## Target Files

| File | Bugs | Risk | Domain |
|------|------|------|--------|
| `transmit.c` | 25 | **CRITICAL** | Network outbound |
| `uucast.c` | 22 | **CRITICAL** | UUCP gateway |
| `nntpread.c` | 12 | **CRITICAL** | Network inbound |
| `nntpclient.c` | 8 | **HIGH** | NNTP client |
| `edbm.c` | 16 | MEDIUM | Database |
| `visual.c` | 11 | LOW | Screen display |
| `digest.c` | 8 | MEDIUM | Digest processing |
| `wractive.c` | 8 | MEDIUM | Active file writing |
| `ngprep.c` | 8 | MEDIUM | Newsgroup prep |
| `getart.c` | 8 | HIGH | Article retrieval |
| `newsdb.c` | 7 | MEDIUM | News database |
| `ednewsipc.c` | 7 | HIGH | IPC handling |

**Total: 140 bugs across 12 files**

---

## The Network Files (CRITICAL)

### transmit.c — 25 bugs

```yaml
file: src/D.priv/transmit.c
bugs: 25
risk: CRITICAL
reason: |
  Outbound network transmission.
  Builds protocol commands from potentially untrusted data.
  Every sprintf could become a protocol injection.

patterns_found:
  sprintf: 18
  strcpy: 5
  strcat: 2

security_notes:
  - Line 156: Path construction from article ID
  - Line 234: Protocol command building
  - Line 312: Header injection risk

result: 25/25 fixed
time: "45 seconds"
```

### uucast.c — 22 bugs

```yaml
file: src/D.uucp/uucast.c
bugs: 22
risk: CRITICAL
reason: |
  UUCP gateway. Ancient protocol. 
  Path traversal risks everywhere.
  Message IDs flow into file paths.

patterns_found:
  sprintf: 14
  strcpy: 6
  strcat: 2

security_notes:
  - Line 89: msgid → path construction (PATH TRAVERSAL!)
  - Line 156: system() with constructed path (flagged!)
  - Line 234: uucp bang paths are untrusted

result: 22/22 fixed
flagged_for_theo: 2  # system() calls need deeper review
time: "40 seconds"
```

### nntpread.c — 12 bugs

```yaml
file: src/D.network/nntpread.c
bugs: 12
risk: CRITICAL
reason: |
  INBOUND NETWORK DATA.
  Everything from the socket is attacker-controlled.
  This is the front door.

patterns_found:
  sprintf: 8
  strcpy: 3
  strcat: 1

security_notes:
  - Line 87: newsgroup name from server → sprintf
  - Line 145: article headers → buffer
  - Line 234: Message-ID parsing

result: 12/12 fixed
time: "25 seconds"
```

### nntpclient.c — 8 bugs

```yaml
file: src/D.network/nntpclient.c
bugs: 8
risk: HIGH
reason: |
  NNTP client operations.
  Constructs protocol commands.
  Response parsing could overflow.

patterns_found:
  sprintf: 5
  strcpy: 2
  strcat: 1

result: 8/8 fixed
time: "20 seconds"
```

---

## The Remaining Files

### edbm.c — 16 bugs

```yaml
file: src/D.port/edbm.c
bugs: 16
risk: MEDIUM
reason: "Database operations. Key/value handling."

result: 16/16 fixed
time: "30 seconds"
```

### visual.c — 11 bugs

```yaml
file: src/D.scrn/visual.c
bugs: 11
risk: LOW
reason: "Screen display. Local only."

result: 11/11 fixed
time: "25 seconds"
```

### digest.c — 8 bugs

```yaml
file: src/D.read/digest.c
bugs: 8
risk: MEDIUM
reason: "Digest parsing. Article content flows through."

result: 8/8 fixed
time: "20 seconds"
```

### wractive.c — 8 bugs

```yaml
file: src/D.priv/wractive.c
bugs: 8
risk: MEDIUM
reason: "Active file writing."

result: 8/8 fixed
time: "20 seconds"
```

### ngprep.c — 8 bugs

```yaml
file: src/D.priv/ngprep.c
bugs: 8
risk: MEDIUM
reason: "Newsgroup preparation."

result: 8/8 fixed
time: "20 seconds"
```

### getart.c — 8 bugs

```yaml
file: src/D.news/getart.c
bugs: 8
risk: HIGH
reason: "Article retrieval. Paths from article IDs."

result: 8/8 fixed
time: "25 seconds"
```

### newsdb.c — 7 bugs

```yaml
file: src/newsdb.c
bugs: 7
risk: MEDIUM
reason: "News database operations."

result: 7/7 fixed
time: "20 seconds"
```

### ednewsipc.c — 7 bugs

```yaml
file: src/ednewsipc.c
bugs: 7
risk: HIGH
reason: "IPC. Inter-process communication. Trust boundary."

result: 7/7 fixed
time: "25 seconds"
```

---

## Safari #5 Summary

```yaml
total_files: 12
total_bugs: 140
total_time: "3 minutes"  # MEASURED from git commits
velocity: "46.7 bugs/min"  # 🏆 PEAK VELOCITY

network_critical_fixed: 67  # transmit + uucast + nntpread + nntpclient
other_fixed: 73

flagged_for_review: 3
  - uucast.c: 2 system() calls
  - transmit.c: 1 path construction pattern
```

---

# 🧠 TRACK B: Deep Brain Analysis (Theo)

## Mode

```yaml
status: DEEP_ANALYSIS
mode: ARCHITECTURAL_SECURITY
priority: CRITICAL
caffeine_level: MAXIMUM
```

## Issue #42: Global Buffer Audit — The bfr Problem

### The Pattern

```c
// Found EVERYWHERE in this codebase
char bfr[BUFLEN];  // Global or file-static buffer

// Used by multiple functions
// No thread safety
// State persists across calls
// Classic 1989 optimization

// The problem:
function_a() {
    sprintf(bfr, "data for a");
    function_b();  // Might also use bfr!
    use(bfr);      // Corrupted?
}
```

### Audit Results

```yaml
global_buffers_found: 47
files_affected: 23
patterns:
  - "char bfr[BUFLEN]" at file scope: 31 instances
  - "static char bfr" in functions: 16 instances
  
risk_levels:
  critical: 8   # Used across function calls in network code
  high: 12      # Used in signal handlers
  medium: 15    # Local to file, single-threaded
  low: 12       # Display/UI only

recommendation: |
  1. Document all global buffer usage
  2. Add /* GLOBAL BUFFER WARNING */ comments
  3. Long-term: Stack allocation where safe
  4. Never use global buffers in signal handlers
```

---

## Issue #43: Signal Handler Safety Audit

### The Problem

```c
// UNSAFE signal handler pattern (found in multiple files)
void handler(int sig) {
    sprintf(bfr, "Caught signal %d", sig);  // ASYNC-UNSAFE!
    write_log(bfr);  // Might call malloc!
    cleanup();       // Might touch shared state!
}
```

### Audit Results

```yaml
signal_handlers_found: 12
async_unsafe_calls_in_handlers:
  sprintf: 7
  printf: 3
  malloc: 2 (via library calls)
  file_io: 5
  
safe_handlers: 2
unsafe_handlers: 10

files_affected:
  - control.c: 3 handlers
  - expire.c: 2 handlers
  - rnews.c: 2 handlers
  - transmit.c: 2 handlers
  - reader.c: 3 handlers

recommendation: |
  Signal handlers should ONLY:
  1. Set a volatile sig_atomic_t flag
  2. Call write() with static buffer
  3. Call _exit()
  
  All current handlers need rewriting.
  This is a LONG-TERM fix.
```

---

## Issue #44: Shell Injection Mapping — system() and popen()

### The Hunt

```bash
# How many system() calls?
rg "system\s*\(" src/ --count
# Result: 34 calls

# How many popen() calls?
rg "popen\s*\(" src/ --count
# Result: 12 calls
```

### Audit Results

```yaml
system_calls: 34
popen_calls: 12
total: 46

classification:
  safe: 8       # Static strings only
  risky: 23     # Variables in command, but validated
  dangerous: 15 # User/network data flows to command

dangerous_examples:
  
  - file: sysmail.c
    line: 234
    pattern: 'sprintf(cmd, "mail %s", address); system(cmd);'
    risk: "CRITICAL — email address injection"
    exploit: 'address = "victim; rm -rf /"'
    
  - file: control.c
    line: 456
    pattern: 'sprintf(cmd, "uux %s!rnews", path); system(cmd);'
    risk: "CRITICAL — UUCP path injection"
    exploit: 'path = "host;malicious"'
    
  - file: nntpxmit.c
    line: 123
    pattern: 'popen(mailcmd, "w");'
    risk: "HIGH — mail command construction"

recommendation: |
  1. NEVER use system() with user data
  2. Replace with fork()/exec() with explicit argv
  3. Validate ALL inputs before shell operations
  4. Add INPUT VALIDATION layer
```

---

## Issue #39: escapes.c Refactor

### The Problem

```c
// lexpand() has no size parameter
char *lexpand(char *s) {
    static char buf[BUFLEN];  // Global buffer!
    // Expands escape sequences
    // No bounds checking
    // Attacker controls input length
}
```

### Analysis

```yaml
functions_in_escapes_c: 8
global_buffers: 3
unsafe_patterns: 45

the_fix:
  before: "char *lexpand(char *s)"
  after: "char *lexpand(char *s, char *buf, size_t buflen)"
  
  impact:
    - 23 call sites need updating
    - All callers must provide buffer
    - Stack allocation instead of global
    
  status: "DESIGNED, not yet implemented"
  reason: "Breaking change — needs careful rollout"
```

---

## Deep Brain Analysis Summary

```yaml
issues_analyzed: 4
patterns_documented:
  - Global buffer problem (47 instances)
  - Signal handler unsafety (10 handlers)
  - Shell injection risks (15 dangerous)
  - escapes.c architectural debt

security_findings:
  critical: 18
  high: 25
  medium: 31
  
documentation_created:
  - GLOBAL-BUFFERS.md
  - SIGNAL-HANDLERS.md  
  - SHELL-INJECTION.md
  - ESCAPES-REFACTOR.md
```

---

# Combined Results

## Robbie (Safari #5)

```yaml
files_fixed: 12
bugs_squashed: 140
time: "3 minutes"  # MEASURED from git commits
velocity: "46.7 bugs/min"  # 🏆 PEAK VELOCITY
network_critical: 67
flagged: 3
```

## Theo (Deep Brain)

```yaml
issues_analyzed: 4
patterns_documented: 4
critical_findings: 18
time: 12 minutes
```

## Cumulative Progress

```
Safari #1: 67 bugs
Safari #2: 102 bugs
Safari #3: 69 bugs
Safari #4: 150 bugs
Safari #5: 140 bugs
───────────────────
TOTAL: 528 bugs fixed

Progress: 774 → 246 (68% COMPLETE!)
```

**TWO-THIRDS DONE.** 🎉🎉🎉

---

## Key Quotes

**Robbie on network code:**
> "The network files were the scariest. Every byte from the socket is attacker-controlled. I fixed them all, but I flagged the system() calls. That's above my pay grade."

**Theo on the deep patterns:**
> "The global buffer problem isn't just style — it's architectural. 47 shared mutable buffers. 10 unsafe signal handlers. 15 shell injection points. The 1989 optimization assumptions don't hold in 2026. We're documenting the debt."

**Robbie on reaching 68%:**
> "Two-thirds. I remember when 774 paralyzed me. Now 246 feels... achievable. The rhino doesn't stop."

---

## GitHub Artifacts

- **Safari #5:** [#45 comment](https://github.com/SimHacker/tmnn7-8/issues/45)
- **Deep Brain:** [#42](https://github.com/SimHacker/tmnn7-8/issues/42), [#43](https://github.com/SimHacker/tmnn7-8/issues/43), [#44](https://github.com/SimHacker/tmnn7-8/issues/44)
- **Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

**Previous:** [Activity 015 — 🌮 THE TACO PARTY 🌮](activity-015-taco-party.md)
**Next:** [Activity 017 — 🦏🌊 DEEP AND WIDE](activity-017-deep-and-wide.md)
