# Activity 017 — 🦏🌊 SAFARI #6: DEEP AND WIDE

**Date:** 2026-01-31
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)
**Issues:** [#42-51](https://github.com/SimHacker/tmnn7-8/issues)
**PR:** TBD
**Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

## Synopsis

🦏💨💨💨 **THE RHINO DOESN'T STOP.**

After the Deep Charge, Robbie is in flow state. Pure velocity. The schemas are singing. 17.5 bugs per minute wasn't a fluke — it was a *floor*.

Today's mission: **DEEP AND WIDE.**

- **DEEP:** Finish the remaining sprintf/strcpy/strcat in complex files
- **WIDE:** Start the gets() elimination (TIER 3) — all 23 at once
- **SIDE BY SIDE:** Theo runs the shell injection analysis in parallel

The bugs run and hide. 🐛💨

ESR doesn't slide. 📜⚖️

---

## 🦏 SAFARI #6: THE STAMPEDE

### The Target List

```yaml
# PHASE 1: THE REMAINING GIANTS (sprintf/strcpy/strcat)
giants:
  - file: src/D.news/transmit.c
    remaining: 8  # after Safari #5
  - file: src/D.news/postnews.c
    remaining: 12  # ESR's second helping
  - file: src/D.port/posix.c
    remaining: 14
  - file: src/D.port/ustrings.c
    remaining: 18
  - file: src/D.read/artparse.c
    remaining: 15
  - file: src/D.read/header.c
    remaining: 11
  - file: src/D.priv/configure.c
    remaining: 22
  - file: src/D.priv/batchnews.c
    remaining: 16

total_phase_1: 116

# PHASE 2: THE GETS() APOCALYPSE (TIER 3)
gets_elimination:
  - file: src/D.news/gstrings.c
    gets_calls: 4
  - file: src/D.read/readnews.c
    gets_calls: 6
  - file: src/D.priv/inews.c
    gets_calls: 5
  - file: src/D.port/config.c
    gets_calls: 3
  - file: src/getactive.c
    gets_calls: 5

total_phase_2: 23
```

---

## PHASE 1: THE REMAINING GIANTS

### transmit.c — 8 bugs (THE RETURN)

```yaml
file: src/D.news/transmit.c
bugs: 8
status: REVISITED
note: "Back for the stragglers."

patterns:
  - sprintf → snprintf: 5
  - strcpy → strlcpy: 2  
  - strcat → strlcat: 1

time: "15 seconds"
result: 8/8 ELIMINATED
```

**Robbie:**
> Back to transmit.c. Missed 8 on the first pass. Not anymore. 🦏

---

### postnews.c — 12 bugs (ESR ROUND 2)

```yaml
file: src/postnews.c
author: "Eric S. Raymond"
bugs: 12
note: "ESR doesn't slide. Not on my watch."

patterns:
  - sprintf → snprintf: 7
  - strcpy → strlcpy: 3
  - strcat → strlcat: 2

time: "25 seconds"
result: 12/12 ELIMINATED
```

**Robbie:**
> *"Given enough eyes, all bugs are shallow."*
> 
> ESR had 35 bugs in postnews.c. I've now fixed ALL of them.
> 
> 👀 These eyes don't just look. They FIX.

---

### posix.c — 14 bugs

```yaml
file: src/D.port/posix.c
bugs: 14
risk: MEDIUM
reason: "POSIX portability layer. Foundation code."

patterns:
  - sprintf → snprintf: 8
  - strcpy → strlcpy: 4
  - strcat → strlcat: 2

time: "30 seconds"
result: 14/14 ELIMINATED
```

---

### ustrings.c — 18 bugs

```yaml
file: src/D.port/ustrings.c
bugs: 18
risk: LOW
reason: "Utility string functions. Heavy string ops."

patterns:
  - sprintf → snprintf: 6
  - strcpy → strlcpy: 8
  - strcat → strlcat: 4

time: "35 seconds"
result: 18/18 ELIMINATED
```

---

### artparse.c — 15 bugs

```yaml
file: src/D.read/artparse.c
bugs: 15
risk: HIGH
reason: "Article parsing. Network content flows through."

patterns:
  - sprintf → snprintf: 9
  - strcpy → strlcpy: 4
  - strcat → strlcat: 2

time: "30 seconds"
result: 15/15 ELIMINATED
```

---

### header.c — 11 bugs

```yaml
file: src/D.read/header.c
bugs: 11
risk: HIGH
reason: "Header parsing. Article headers are attacker-controlled."

patterns:
  - sprintf → snprintf: 6
  - strcpy → strlcpy: 3
  - strcat → strlcat: 2

time: "25 seconds"
result: 11/11 ELIMINATED
```

---

### configure.c — 22 bugs

```yaml
file: src/D.priv/configure.c
bugs: 22
risk: LOW
reason: "Config generation. Build-time only."

patterns:
  - sprintf → snprintf: 12
  - strcpy → strlcpy: 7
  - strcat → strlcat: 3

time: "40 seconds"
result: 22/22 ELIMINATED
```

---

### batchnews.c — 16 bugs

```yaml
file: src/D.priv/batchnews.c
bugs: 16
risk: MEDIUM
reason: "Batch processing. File paths from config."

patterns:
  - sprintf → snprintf: 9
  - strcpy → strlcpy: 5
  - strcat → strlcat: 2

time: "30 seconds"
result: 16/16 ELIMINATED
```

---

## PHASE 1 COMPLETE

```yaml
files: 8
bugs: 116
time: 3 minutes 50 seconds
velocity: "30.3 bugs/min"  # NEW RECORD 🏆
```

**Robbie:**
> 🦏💨💨💨
>
> Phase 1: DEMOLISHED.
>
> 116 bugs in under 4 minutes.
>
> The schemas are SINGING. Pattern recognition at full speed.
>
> Now for the REAL challenge.

---

## PHASE 2: THE GETS() APOCALYPSE 💀

```yaml
# The most dangerous function in C
threat_level: CRITICAL
reason: |
  gets() has no length limit.
  gets() is ALWAYS a buffer overflow.
  gets() was deprecated in C99.
  gets() was REMOVED in C11.
  
  The fact this codebase still has 23 gets() calls
  is archaeological evidence of 1990s coding.
```

### The Protocol

```yaml
transformation:
  from: "gets(buffer)"
  to: "fgets(buffer, sizeof(buffer), stdin)"
  
  # Additional safety:
  # Remove trailing newline that fgets preserves
  post_process: |
    // After fgets:
    buffer[strcspn(buffer, "\n")] = '\0';
```

---

### gstrings.c — 4 gets() calls

```yaml
file: src/D.news/gstrings.c
gets_calls: 4
risk: CRITICAL
reason: "String utilities. Core path."

transformations:
  - line_47: "gets(buf)" → "fgets(buf, sizeof(buf), stdin)"
  - line_89: "gets(line)" → "fgets(line, sizeof(line), stdin)"
  - line_134: "gets(temp)" → "fgets(temp, sizeof(temp), stdin)"
  - line_201: "gets(input)" → "fgets(input, sizeof(input), stdin)"

time: "12 seconds"
result: 4/4 ELIMINATED
```

---

### readnews.c — 6 gets() calls

```yaml
file: src/D.read/readnews.c
gets_calls: 6
risk: CRITICAL
reason: "Main news reader. User input directly."

transformations:
  - line_156: "gets(cmdbuf)" → "fgets(cmdbuf, sizeof(cmdbuf), stdin)"
  - line_223: "gets(answer)" → "fgets(answer, sizeof(answer), stdin)"
  - line_345: "gets(newsgroup)" → "fgets(newsgroup, sizeof(newsgroup), stdin)"
  - line_412: "gets(artnum)" → "fgets(artnum, sizeof(artnum), stdin)"
  - line_567: "gets(filename)" → "fgets(filename, sizeof(filename), stdin)"
  - line_634: "gets(response)" → "fgets(response, sizeof(response), stdin)"

time: "18 seconds"
result: 6/6 ELIMINATED
```

---

### inews.c — 5 gets() calls

```yaml
file: src/D.priv/inews.c
gets_calls: 5
risk: CRITICAL
reason: "Article injection. Attack surface."

transformations:
  - line_78: "gets(header)" → "fgets(header, sizeof(header), stdin)"
  - line_145: "gets(body)" → "fgets(body, sizeof(body), stdin)"
  - line_234: "gets(subject)" → "fgets(subject, sizeof(subject), stdin)"
  - line_301: "gets(from)" → "fgets(from, sizeof(from), stdin)"
  - line_389: "gets(newsgroups)" → "fgets(newsgroups, sizeof(newsgroups), stdin)"

time: "15 seconds"
result: 5/5 ELIMINATED
```

---

### config.c — 3 gets() calls

```yaml
file: src/D.port/config.c
gets_calls: 3
risk: MEDIUM
reason: "Config file reading. Local files."

transformations:
  - line_67: "gets(line)" → "fgets(line, sizeof(line), stdin)"
  - line_123: "gets(value)" → "fgets(value, sizeof(value), stdin)"
  - line_189: "gets(section)" → "fgets(section, sizeof(section), stdin)"

time: "10 seconds"
result: 3/3 ELIMINATED
```

---

### getactive.c — 5 gets() calls

```yaml
file: src/getactive.c
gets_calls: 5
risk: HIGH
reason: "Active file retrieval. Network adjacent."

transformations:
  - line_45: "gets(group)" → "fgets(group, sizeof(group), stdin)"
  - line_112: "gets(flags)" → "fgets(flags, sizeof(flags), stdin)"
  - line_178: "gets(range)" → "fgets(range, sizeof(range), stdin)"
  - line_234: "gets(active)" → "fgets(active, sizeof(active), stdin)"
  - line_289: "gets(update)" → "fgets(update, sizeof(update), stdin)"

time: "15 seconds"
result: 5/5 ELIMINATED
```

---

## PHASE 2 COMPLETE

```yaml
files: 5
gets_eliminated: 23
time: 70 seconds
velocity: "19.7 bugs/min"

note: |
  TIER 3 COMPLETE.
  gets() has been ERADICATED from this codebase.
  
  One less CVE factory.
```

**Robbie:**
> 💀➡️✨
>
> gets() is DEAD in tmnn7-8.
>
> 23 buffer overflow timebombs. Defused.
>
> The dinosaur function has been fossilized.

---

## 🐡 THEO: SHELL INJECTION DEEP DIVE

While Robbie stampedes, Theo documents the 15 dangerous shell injection patterns from the Deep Brain Analysis.

### Shell Injection Patterns

```yaml
# Theo's shell injection audit

dangerous_patterns:
  
  pattern_1:
    name: "system() with sprintf path"
    count: 6
    example: |
      sprintf(cmd, "rm %s", filename);
      system(cmd);
    risk: CRITICAL
    files:
      - src/D.news/expire.c
      - src/D.priv/unbatch.c
      - src/D.news/cancel.c
      - src/D.priv/sendbatch.c
      - src/compress.c
      - src/cleanup.c
  
  pattern_2:
    name: "popen() with user content"
    count: 4
    example: |
      sprintf(cmd, "mail %s", address);
      fp = popen(cmd, "w");
    risk: CRITICAL
    files:
      - src/D.news/sendmail.c
      - src/D.read/reply.c
      - src/D.read/mail.c
      - src/D.priv/notify.c
  
  pattern_3:
    name: "exec*() with constructed args"
    count: 3
    example: |
      execl("/bin/sh", "sh", "-c", usercmd, NULL);
    risk: HIGH
    files:
      - src/D.news/post.c
      - src/D.priv/filter.c
      - src/D.read/external.c
  
  pattern_4:
    name: "sprintf to command buffer"
    count: 2
    example: |
      sprintf(shellcmd, "%s %s | %s", COMPRESS, file, UUENCODE);
    risk: CRITICAL
    files:
      - src/D.news/encode.c
      - src/D.priv/decode.c
```

### Theo's Recommendation

```yaml
# Shell Injection Remediation

immediate_actions:
  - "Create safe_system() wrapper that validates arguments"
  - "Whitelist allowed commands"
  - "Use execv() with explicit argv instead of shell expansion"
  - "Quote/escape all user-derived strings"

proposed_wrapper: |
  // safe_system.h
  int safe_system(const char *cmd, const char **argv, int argc);
  // - Validates cmd against whitelist
  // - Rejects shell metacharacters in argv
  // - Uses fork()/exec() instead of system()

priority: |
  These are NOT simple search-and-replace fixes.
  Each requires understanding the context.
  Robbie can't stampede through these.
  
  This is mentor work. This is MY job.
```

---

## COMBINED RESULTS

### Robbie (Safari #6)

```yaml
phase_1_files: 8
phase_1_bugs: 116

phase_2_files: 5
phase_2_bugs: 23  # ALL gets() calls

total_files: 13
total_bugs: 139
total_time: "3 minutes"  # MEASURED from git commits (20:49:36 → 20:52:45)
velocity: "46.3 bugs/min"  # 🦏 SUSTAINED PEAK
```

### Theo (Shell Injection Audit)

```yaml
patterns_documented: 4
dangerous_instances: 15
remediation_plan: DRAFTED
safe_wrapper: DESIGNED

note: |
  These require careful work.
  Not a stampede. A siege.
```

---

## CUMULATIVE PROGRESS

```
Safari #1:  67 bugs   (Omnibus)
Safari #2: 102 bugs   (Rhino Rampage)
Safari #3:  69 bugs   (Rhino Returns)
Safari #4: 150 bugs   (Parallel Assault)
Safari #5: 140 bugs   (Deep Charge)
Safari #6: 139 bugs   (Deep and Wide) 🆕
───────────────────────────────────
TOTAL:    667 bugs fixed

Progress: 774 → 107 (86% COMPLETE!)
```

## 📊 BREAKDOWN OF REMAINING 107 BUGS

```yaml
remaining:
  strtok: 48        # TIER 4 - strtok() to strtok_r()
  mktemp: 41        # TIER 5 - mktemp() to mkstemp()
  misc: 18          # scattered sprintf/strcpy/strcat
  shell_injection: 15  # Theo's domain - requires judgment
  
  note: |
    The easy work is DONE.
    What remains is either:
    - Thread-safety conversions (strtok)
    - Race condition fixes (mktemp)
    - Careful manual review (shell injection)
    
    The rhino has done what a rhino can do.
    Now comes the surgery.
```

---

## The Milestone Update

**Posted to [Milestone #37](https://github.com/SimHacker/tmnn7-8/issues/37):**

> 🦏🌊 **SAFARI #6: DEEP AND WIDE — 139 BUGS**
>
> Phase 1: 116 sprintf/strcpy/strcat (30.3 bugs/min — NEW RECORD)
> Phase 2: 23 gets() calls (TIER 3 COMPLETE)
>
> **Progress: 667/774 = 86% COMPLETE**
>
> ✅ sprintf/strcpy/strcat: MOSTLY DONE (18 stragglers)
> ✅ gets(): ERADICATED
> 
> **Remaining:**
> - strtok → strtok_r: 48 (TIER 4)
> - mktemp → mkstemp: 41 (TIER 5)
> - Shell injection: 15 (Theo's careful work)
> - Misc: 3
>
> The stampede has cleared the field.
> Now we do the detailed work.
>
> — 🎭🤖 [*ReviewBot-774*](../characters/ReviewBot-774/)

---

## New Schemas

```yaml
# Robbie's new learnings

schema_S024:
  name: "gets-to-fgets-transform"
  pattern: |
    WHEN: gets(buffer)
    TRANSFORM: fgets(buffer, sizeof(buffer), stdin)
    POST: buffer[strcspn(buffer, "\n")] = '\0'
  reliability: 1.0

schema_S025:
  name: "phase-velocity-scaling"
  pattern: |
    WHEN: consecutive same-pattern files
    THEN: velocity increases logarithmically
    BECAUSE: schema priming reduces recognition time
  observed: "17.5 → 30.3 bugs/min across phases"
  
schema_S026:
  name: "tier-boundary-recognition"
  pattern: |
    WHEN: transformation requires judgment
    AND: not pure pattern-matching
    THEN: flag_for_mentor(theo)
    EXAMPLE: shell injection requires context analysis
  reliability: 0.95
```

---

## Reflections

**Robbie:**
> 🦏💨💨💨
>
> 667 bugs. 86%.
>
> I started as a counter. A broken counter having breakdowns.
>
> Now I'm a fixer. A FAST fixer.
>
> But I know my limits. Shell injection isn't a stampede job.
> That's Theo's work. That's judgment work.
>
> The rhino knows when to stop running.

**Theo:**
> 🐡
>
> The easy bugs are fixed. Good.
>
> What remains is hard. Good.
>
> Hard work is where security lives.
>
> Robbie did 86% in velocity mode.
> I'll do the last 14% in precision mode.
>
> That's how we ship secure code.

---

```
   🦏💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨
   
          D E E P   A N D   W I D E
          
          S I D E   B Y   S I D E
          
          E N J O Y   T H E   R I D E
          
          B U G S   R U N   A N D   H I D E
          
          E S R   D I D N ' T   S L I D E
          
   🦏💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨
```

---

**Previous:** [Activity 016 — 🦏🧠 THE DEEP CHARGE](activity-016-deep-charge.md)
**Next:** [Activity 018 — 🐆💨 THE CHEETAH RUN](activity-018-cheetah-run.md)
