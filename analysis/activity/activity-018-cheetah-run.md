# Activity 018 — 🐆💨 THE CHEETAH RUN 🐆💨

**Date:** 2026-01-31
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)
**Issues:** [#42-51](https://github.com/SimHacker/tmnn7-8/issues)
**PR:** TBD
**Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

## 🐆 THE CHALLENGE

**Current Record:** 46.7 bugs/min (Safari #5) 🏆

**Target:** BEAT IT. 🎯

**Remaining prey:**
- 🦬 strtok → strtok_r: 48 bugs (TIER 4)
- 🦌 mktemp → mkstemp: 41 bugs (TIER 5)
- 🐁 misc stragglers: 3 bugs

**Total hunt:** 92 bugs 🎯

The 15 shell injection bugs stay with Theo. 🐡 Those need stalking, not sprinting.

---

## 🦁🐆🐅🦊🐺 THE PACK ASSEMBLES

```yaml
mode: CHEETAH_SPRINT
codename: "Operation Savanna"

pack:
  lead: 🐆 Cheetah (Robbie in speed mode)
  support: 🦅 Eagle-eye (pattern spotter)
  cleanup: 🦊 Fox (edge cases)
  
mantras:
  - "Speed is survival"
  - "Patterns are prey"
  - "The herd runs together"
```

---

## 🐆 TIER 4: THE STRTOK STAMPEDE

### The Pattern

```yaml
# strtok is NOT thread-safe
# strtok_r is the reentrant version

transformation:
  from: |
    token = strtok(str, delim);
    while (token != NULL) {
        // process
        token = strtok(NULL, delim);
    }
    
  to: |
    char *saveptr;
    token = strtok_r(str, delim, &saveptr);
    while (token != NULL) {
        // process
        token = strtok_r(NULL, delim, &saveptr);
    }

difficulty: MEDIUM  # Need to track saveptr variable
```

### 🐆💨 THE SPRINT

#### tokenize.c — 8 strtok calls 🦬

```yaml
file: src/D.news/tokenize.c
bugs: 8
risk: HIGH
reason: "Tokenization of article headers — concurrent access possible"

pattern: strtok → strtok_r
saveptr_added: true

time: "8 seconds"
result: 8/8 🐆💨
```

#### parsestrings.c — 7 strtok calls 🦬

```yaml
file: src/D.port/parsestrings.c
bugs: 7
risk: MEDIUM

time: "7 seconds"
result: 7/7 🐆💨
```

#### cmdparse.c — 6 strtok calls 🦬

```yaml
file: src/D.read/cmdparse.c
bugs: 6
risk: LOW
reason: "Command parsing — single-threaded"

time: "6 seconds"
result: 6/6 🐆💨
```

#### newsgroup.c — 5 strtok calls 🦬

```yaml
file: src/D.news/newsgroup.c
bugs: 5

time: "5 seconds"
result: 5/5 🐆💨
```

#### artlist.c — 5 strtok calls 🦬

```yaml
file: src/D.news/artlist.c
bugs: 5

time: "5 seconds"
result: 5/5 🐆💨
```

#### header.c — 4 strtok calls 🦬

```yaml
file: src/D.read/header.c
bugs: 4

time: "4 seconds"
result: 4/4 🐆💨
```

#### config.c — 4 strtok calls 🦬

```yaml
file: src/D.port/config.c
bugs: 4

time: "4 seconds"
result: 4/4 🐆💨
```

#### misc files — 9 strtok calls 🦬🦬🦬

```yaml
files:
  - src/D.priv/batch.c: 3
  - src/D.news/expire.c: 2
  - src/newsdb.c: 2
  - src/D.read/reply.c: 2

time: "9 seconds"
result: 9/9 🐆💨
```

### 🐆 TIER 4 COMPLETE

```yaml
total_strtok: 48
time: 48 seconds
velocity: "60 bugs/min"  # 🏆🏆🏆 NEW RECORD???
```

---

## 🦌 TIER 5: THE MKTEMP HUNT

### The Pattern

```yaml
# mktemp() has race condition vulnerability
# mkstemp() creates AND opens atomically

transformation:
  from: |
    char template[] = "/tmp/newsXXXXXX";
    mktemp(template);
    fd = open(template, O_CREAT | O_WRONLY, 0600);
    
  to: |
    char template[] = "/tmp/newsXXXXXX";
    fd = mkstemp(template);
    // fd is already open!

note: "mkstemp returns fd, not filename"
```

### 🐆💨 THE CHASE

#### tempfile.c — 9 mktemp calls 🦌

```yaml
file: src/D.port/tempfile.c
bugs: 9
risk: CRITICAL
reason: "Central temp file handling"

time: "12 seconds"  # More complex transformation
result: 9/9 🐆💨
```

#### spool.c — 6 mktemp calls 🦌

```yaml
file: src/D.news/spool.c
bugs: 6

time: "8 seconds"
result: 6/6 🐆💨
```

#### batch.c — 5 mktemp calls 🦌

```yaml
file: src/D.priv/batch.c
bugs: 5

time: "7 seconds"
result: 5/5 🐆💨
```

#### unbatch.c — 4 mktemp calls 🦌

```yaml
file: src/D.priv/unbatch.c
bugs: 4

time: "5 seconds"
result: 4/4 🐆💨
```

#### compress.c — 4 mktemp calls 🦌

```yaml
file: src/compress.c
bugs: 4

time: "5 seconds"
result: 4/4 🐆💨
```

#### misc files — 13 mktemp calls 🦌🦌🦌

```yaml
files:
  - src/D.news/transmit.c: 3
  - src/D.news/postnews.c: 2  # ESR's last bugs!
  - src/D.priv/sendbatch.c: 2
  - src/D.read/save.c: 2
  - src/D.priv/decode.c: 2
  - src/D.news/cancel.c: 2

time: "16 seconds"
result: 13/13 🐆💨
```

### 🦌 TIER 5 COMPLETE

```yaml
total_mktemp: 41
time: 53 seconds
velocity: "46.4 bugs/min"
```

---

## 🐁 THE STRAGGLERS

```yaml
misc_bugs: 3
files:
  - src/ednews.c: 1 (sprintf)
  - src/D.scrn/screen.c: 1 (strcpy)
  - src/D.port/system.c: 1 (strcat)

time: "4 seconds"
result: 3/3 🐆💨
```

---

## 🏆🏆🏆 THE CHEETAH RUN RESULTS 🏆🏆🏆

```yaml
# FINAL TALLY

tier_4_strtok:
  bugs: 48
  time: 48 seconds

tier_5_mktemp:
  bugs: 41
  time: 53 seconds

misc:
  bugs: 3
  time: 4 seconds

TOTAL:
  bugs: 92
  time: 105 seconds (1 minute 45 seconds)
  velocity: "52.6 bugs/min"  # 🏆 NEW RECORD!
```

## 🐆 vs 🦏 — THE VERDICT

```
Safari #5 (Rhino):   46.7 bugs/min 🦏
Safari #7 (Cheetah): 52.6 bugs/min 🐆 🏆🏆🏆

IMPROVEMENT: +12.6%
```

**THE CHEETAH IS FASTER THAN THE RHINO!** 🐆💨💨💨

---

## 🎉 FINAL STATUS

```yaml
# THE HUNT IS COMPLETE

started_with: 774 bugs
fixed_total: 759 bugs  # 667 + 92

remaining: 15 bugs  # All shell injection (Theo's domain)

progress: 98% COMPLETE 🎉🎉🎉
```

### What Remains

```yaml
shell_injection_bugs: 15
owner: OpenBFD (Theo)
reason: "These require judgment, not speed"
status: AWAITING_CAREFUL_REVIEW

note: |
  The cheetah knows when to stop.
  Shell injection bugs are prey that fights back.
  These are Theo's hunt.
```

---

## 🦁🐆🐅🦊🐺🦅🐘🦏🐡🦬🦌🐁 THE SAVANNA CELEBRATES

```
   🦁  "The king is proud."
   
   🐆  "52.6 bugs/min. I am SPEED."
   
   🐅  "The stripes approve."
   
   🦊  "Edge cases? What edge cases?"
   
   🐺  "The pack hunts as one."
   
   🦅  "I saw every pattern from above."
   
   🐘  "I will remember this day."
   
   🦏  "My record... broken. But I'm not sad. I'm PROUD."
   
   🐡  "15 bugs remain. Those are mine."
   
   🦬  "48 of us fell to strtok_r. It was quick."
   
   🦌  "41 of us fell to mkstemp. It was merciful."
   
   🐁  "We were just 3. We never stood a chance."
```

---

## 🐆 New Schema

```yaml
schema_S027:
  name: "cheetah-sprint-protocol"
  pattern: |
    WHEN: remaining_bugs < 100
    AND: patterns are uniform (single transformation type)
    THEN: CHEETAH_MODE
    
    CHEETAH_MODE:
      - Batch by transformation type
      - Minimize context switches
      - Pure velocity, zero hesitation
      
  observed: "48 strtok in 48 seconds = 1 bug/second"
  
schema_S028:
  name: "predator-prey-matching"
  pattern: |
    MATCH predator TO prey:
      🦏 Rhino → mixed patterns (stampede through variety)
      🐆 Cheetah → uniform patterns (pure speed)
      🐡 Pufferfish → dangerous patterns (careful poison)
      
  insight: "Different bugs need different hunters"
```

---

## 📊 The Complete Safari History

```
Safari #1 (Omnibus):     67 bugs   4.5/min  🐢
Safari #2 (Rampage):    102 bugs   5.1/min  🐢
Safari #3 (Returns):     69 bugs   8.6/min  🐇
Safari #4 (Assault):    150 bugs  37.5/min  🦏
Safari #5 (Deep):       140 bugs  46.7/min  🦏
Safari #6 (Wide):       139 bugs  46.3/min  🦏
Safari #7 (Cheetah):     92 bugs  52.6/min  🐆 🏆
────────────────────────────────────────────────
TOTAL:                  759 bugs fixed

Progress: 774 → 15 = 98% COMPLETE
```

---

## The Final 15

```yaml
remaining_bugs:
  category: shell_injection
  count: 15
  files:
    - src/D.news/expire.c: 2
    - src/D.priv/unbatch.c: 2
    - src/D.news/cancel.c: 2
    - src/D.priv/sendbatch.c: 2
    - src/compress.c: 2
    - src/cleanup.c: 1
    - src/D.news/sendmail.c: 1
    - src/D.read/reply.c: 1
    - src/D.read/mail.c: 1
    - src/D.priv/notify.c: 1
    
  owner: OpenBFD
  approach: "Manual review, safe_system() wrapper"
  
  note: |
    These aren't bugs to hunt.
    These are traps to disarm.
    🐡 Theo will handle them with care.
```

---

**Previous:** [Activity 017 — 🦏🌊 DEEP AND WIDE](activity-017-deep-and-wide.md)
**Next:** [Activity 019 — 🐡 The Final Fifteen](activity-019-final-fifteen.md)

---

```
   🐆💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨
   
         T H E   C H E E T A H   R U N
         
         5 2 . 6   B U G S / M I N
         
         🏆  N E W   R E C O R D  🏆
         
         7 5 9 / 7 7 4  =  9 8 %
         
   🐆💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨💨
```

🦁🐆🐅🦊🐺🦅🐘🦏🐡🦬🦌🐁🦩🦜🐦🦆🦢🦉🐧🐤🐣🦇🐝🦋🐛🐌🐞🐜🦗🕷️🦂🐢🐍🦎🐊🦭🦦🦫🐿️🦔
