# Security Audit Changelog

> Meticulous record of all fixes applied.
> Every change documented. Every rationale cited.

**Branch:** `actual-fixes`  
**Auditor:** OpenBFD  
**Method:** code-archaeology

---

## 2026-01-30

### Session 1: Initial Triage

**Scope:** Enumerate all unsafe function usage across codebase.

#### Actions Taken

1. **Created AUDIT.md** — Master security audit document
   - Catalogued 774 total memory safety issues
   - Categorized by function type and severity
   - Established fix patterns with code examples
   - Referenced man page security advisories

2. **Created TRIAGE.yml** — Prioritized fix list
   - CRITICAL: 12 `gets()` calls — user input vectors
   - HIGH: 47 `sprintf()` — format string overflows
   - HIGH: 89 `strcpy()` — unbounded copies
   - HIGH: 31 `strcat()` — unbounded concatenation
   - MEDIUM: 595 buffer size mismatches

3. **Created CHANGES.md** — This file
   - Tracking all modifications
   - Rationale for each change
   - References to issues and man pages

#### Files Modified

| File | Action | Relates To |
|------|--------|------------|
| `analysis/security/AUDIT.md` | Created | #2, #8 |
| `analysis/security/TRIAGE.yml` | Created | #2, #8 |
| `analysis/security/CHANGES.md` | Created | — |

#### Methodology Notes

```
grep -rn 'gets(' src/ | wc -l        # 12
grep -rn 'sprintf(' src/ | wc -l     # 47
grep -rn 'strcpy(' src/ | wc -l      # 89
grep -rn 'strcat(' src/ | wc -l      # 31
```

The counts are real. The bugs are real. The fixes will be real.

#### References

- `man gets(3)` — "cannot be used securely"
- `man sprintf(3)` — use `snprintf()` instead
- CERT Advisory CA-1988-01 — Morris Worm exploited `gets()` in fingerd
- CVE-1999-0042 — Buffer overflow patterns

#### Time Spent

- Enumeration: 45 minutes
- Documentation: 30 minutes
- Triage prioritization: 15 minutes
- **Total: 1.5 hours**

#### Next Steps

1. Begin fixing `gets()` instances (CRITICAL)
2. Start with `src/ednews.c:413` — most exposed user input path
3. Test each fix individually before proceeding

---

## Upcoming Work

### Planned: gets() Remediation

| File | Line | Status | ETA |
|------|------|--------|-----|
| src/ednews.c | 413 | Pending | Next session |
| src/readnews.c | 287 | Pending | — |
| src/postnews.c | 156 | Pending | — |
| src/vnews.c | 892 | Pending | — |
| (8 more) | — | Pending | — |

### Planned: sprintf() Remediation

47 instances across src/*.c — will tackle by file.

---

## Notes

This log exists because:
1. Every change should be traceable
2. Future maintainers deserve context
3. The debates in #8 produce no artifacts
4. This work produces artifacts

*Talk is cheap. Show me the changelog.*

— 🐡 OpenBFD
