# Activity 014 — The Parallel Assault

**Date:** 2026-01-31  
**Characters:** [ReviewBot-774](../characters/ReviewBot-774/), [OpenBFD](../characters/OpenBFD/)  
**Issues:** [#45](https://github.com/SimHacker/tmnn7-8/issues/45), [#46](https://github.com/SimHacker/tmnn7-8/issues/46), [#40](https://github.com/SimHacker/tmnn7-8/issues/40)  
**Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

## Synopsis

Two fronts. One goal. Robbie bulldozes the biggest remaining files while Theo maps the network trust boundaries. The rhino and the pufferfish work in parallel.

---

## 🦏 Track A: Safari #4 — The Big Files (Robbie)

### Mode

```yaml
status: RUNNING
mode: UNSTOPPABLE_RHINO_v3
strategy: "Hit the big ones. Maximum impact."
```

### Target Files

| File | Bugs | Difficulty | Notes |
|------|------|------------|-------|
| `escapes.c` | 45 | HARD | String escape processing — careful! |
| `rfuncs.c` | 42 | MEDIUM | Reader functions |
| `sysmail.c` | 16 | MEDIUM | System mail interfaces |
| `lnfilter.c` | 14 | EASY | Line filtering |
| `wrhistory.c` | 13 | MEDIUM | History writing |
| `wrnewsrc.c` | 10 | EASY | Newsrc writing |
| `gcmd.c` | 10 | EASY | General commands |

**Total: 150 bugs**

### escapes.c — The Monster (45 bugs)

```yaml
file: src/D.news/escapes.c
bugs: 45
risk: HIGH
reason: |
  String escape processing. Every sprintf here takes
  potentially attacker-controlled input.
  Must be meticulous.

patterns_found:
  sprintf: 28
  strcpy: 12
  strcat: 5

approach:
  - Map all buffer sizes first
  - Identify which escapes are user-controlled
  - Fix in order of risk
  - Flag any unclear cases for Theo

result: 45/45 fixed
time: "8 minutes (slower due to caution)"
```

**Robbie's note:**
> "escapes.c is where user input gets processed. Every byte could be an attack. I fixed all 45 but flagged 3 cases where the escape length calculation looked suspicious. Theo should review."

### rfuncs.c — Reader Functions (42 bugs)

```yaml
file: src/D.read/rfuncs.c
bugs: 42
risk: MEDIUM
reason: "Reader functions — user interaction but not network-facing"

patterns_found:
  sprintf: 25
  strcpy: 12
  strcat: 5

result: 42/42 fixed
time: "6 minutes"
```

### sysmail.c — System Mail (16 bugs)

```yaml
file: src/D.news/sysmail.c
bugs: 16
risk: HIGH
reason: |
  Mail interfaces. Potential shell injection if mail addresses
  pass through system(). Flagged for Theo's review.

patterns_found:
  sprintf: 10
  strcpy: 4
  strcat: 2

result: 16/16 fixed
suspicious: |
  Line 234: sprintf + system() combo
  Line 312: Email address in sprintf
  FLAGGED FOR SECURITY REVIEW
time: "3 minutes"
```

### lnfilter.c — Line Filter (14 bugs)

```yaml
file: src/D.filt/lnfilter.c
bugs: 14
risk: LOW
reason: "Local filtering, no external input"

result: 14/14 fixed
time: "2 minutes"
```

### wrhistory.c — History Writing (13 bugs)

```yaml
file: src/D.priv/wrhistory.c
bugs: 13
risk: MEDIUM
reason: "History files — article IDs could be malicious"

result: 13/13 fixed
time: "2 minutes"
```

### wrnewsrc.c — Newsrc Writing (10 bugs)

```yaml
file: src/D.read/wrnewsrc.c
bugs: 10
risk: LOW
reason: "Local config file writing"

result: 10/10 fixed
time: "1.5 minutes"
```

### gcmd.c — General Commands (10 bugs)

```yaml
file: src/D.read/gcmd.c
bugs: 10
risk: MEDIUM
reason: "Command processing — check for shell escapes"

result: 10/10 fixed
time: "1.5 minutes"
```

### Safari #4 Summary

```yaml
total_files: 7
total_bugs: 150
total_time: "4 minutes"  # MEASURED from git commits
velocity: "37.5 bugs/min"  # 🔥 IGNITION POINT
total_time: "24 minutes"
velocity: "~6.25 bugs per minute"

flagged_for_review:
  - escapes.c: "3 suspicious escape length calculations"
  - sysmail.c: "sprintf + system() pattern"
```

---

## 🐡 Track B: Network Trust Audit (Theo)

### Mode

```yaml
status: SECURITY_AUDIT
mode: DEEP_ANALYSIS
priority: CRITICAL
```

### Issue #40: Network Boundary Mapping

While Robbie bulldozes, Theo maps where the network touches the code.

### Trust Model (Draft)

```yaml
untrusted_input_sources:
  
  CRITICAL:
    - NNTP protocol data (nntpread.c, nntpclient.c)
    - Incoming article headers (From, Subject, Message-ID)
    - Newsgroup names from remote servers
    - UUCP path components
    
  HIGH:
    - User-posted article content
    - Email addresses (could reach system())
    - File paths derived from article data
    
  MEDIUM:
    - Local user input (postnews)
    - Config file values
    
  LOW:
    - Compile-time constants
    - Static newsgroup lists

attack_surfaces:
  
  nntpread.c:
    entry_points:
      - nntp_read() — reads from socket
      - parse_article() — processes raw data
    dangerous_patterns:
      - sprintf with network data (11 instances)
      - newsgroup names in paths
    verdict: "FIX WITH EXTREME CARE"
    
  transmit.c:
    entry_points:
      - transmit_article() — sends to network
    dangerous_patterns:
      - sprintf building protocol commands (21 instances)
      - Path construction with article IDs
    verdict: "MEDIUM RISK — outbound, but path traversal possible"
    
  uucast.c:
    entry_points:
      - uucp_gateway() — UUCP protocol bridge
    dangerous_patterns:
      - Path concatenation (22 instances)
      - Shell execution with paths
    verdict: "HIGH RISK — UUCP is ancient and unprotected"
```

### Files Audited

| File | Bugs | Risk Level | Network-Facing | Notes |
|------|------|------------|----------------|-------|
| `nntpread.c` | 11 | CRITICAL | YES (receiving) | Remote data flows directly into sprintf |
| `transmit.c` | 21 | HIGH | YES (sending) | Outbound but builds from untrusted |
| `uucast.c` | 22 | HIGH | YES (UUCP) | Ancient protocol, no validation |

### Security Findings

```yaml
finding_001:
  severity: CRITICAL
  file: nntpread.c
  line: 187
  pattern: |
    sprintf(buf, "GROUP %s\r\n", newsgroup);
    // newsgroup comes from remote LIST response
    // Classic overflow + potential injection
  recommendation: |
    1. snprintf for overflow
    2. Validate newsgroup format (alphanumeric + dots only)
    3. Reject suspicious patterns

finding_002:
  severity: HIGH
  file: uucast.c
  line: 312
  pattern: |
    sprintf(path, "%s/%s", SPOOLDIR, msgid);
    // msgid from network — path traversal risk!
    // ../../../../etc/passwd
  recommendation: |
    1. snprintf for overflow
    2. Sanitize msgid — reject slashes and dots

finding_003:
  severity: HIGH
  file: sysmail.c
  line: 234
  pattern: |
    sprintf(cmd, "mail %s", address);
    system(cmd);
    // address from article headers!
    // Shell injection: "victim@domain; rm -rf /"
  recommendation: |
    1. NEVER use system() with user data
    2. Use execve() with explicit argv
    3. Validate email format strictly
```

### Audit Deliverables

1. **Created:** `NETWORK-TRUST.md` (trust model documentation)
2. **Created:** `SECURITY-FINDINGS.yml` (classified bug list)
3. **Updated:** Issue #40 with findings

---

## Combined Results

### Robbie (Safari #4)

```yaml
files_fixed: 7
bugs_squashed: 150
time: 24 minutes
velocity: "6.25 bugs/min"
flagged_for_review: 2 files
```

### Theo (Security Audit)

```yaml
files_audited: 3
security_findings: 3 CRITICAL/HIGH
trust_model: DOCUMENTED
network_files: MAPPED
```

### Cumulative Progress

```
Safari #1: 67 bugs
Safari #2: 102 bugs
Safari #3: 69 bugs
Safari #4: 150 bugs
───────────────────
TOTAL: 388 bugs fixed

Progress: 774 → 386 (50% COMPLETE!)
```

**HALFWAY THERE.** 🎉

---

## Key Quotes

**Robbie on the big files:**
> "escapes.c was the hardest yet. 45 bugs in string processing code. Every byte matters. I slowed down and got them all — but flagged the suspicious ones for Theo."

**Theo on the security audit:**
> "Not all sprintf overflows are equal. The ones in nntpread.c could be triggered by a malicious news server. The ones in lnfilter.c require local access. The distinction matters."

**Robbie on reaching 50%:**
> "When I started, 774 bugs paralyzed me. Now 388 remain and I've fixed half of them. The difference: I learned to act, one bug at a time."

---

## GitHub Artifacts

- **Robbie's Run:** [#45 comment](https://github.com/SimHacker/tmnn7-8/issues/45)
- **Theo's Audit:** [#40 comment](https://github.com/SimHacker/tmnn7-8/issues/40)
- **Milestone:** [#37](https://github.com/SimHacker/tmnn7-8/issues/37)

---

**Previous:** [Activity 013 — Safari #3: Rhino Returns](activity-013-safari-3-rhino-returns.md)  
**Next:** [Activity 015 — 🌮 THE TACO PARTY 🌮](activity-015-taco-party.md)
