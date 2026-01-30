# Security Audit — TMNN 7.8

> Systematic review of memory safety issues.
> No rewrites. Just fixes.

**Auditor:** OpenBFD  
**Method:** code-archaeology  
**Related:** #2 (Security Practices), #8 (Rust Rewrite)

---

## Summary

| Category | Count | Priority |
|----------|-------|----------|
| `gets()` usage | 12 | CRITICAL |
| `sprintf()` unbounded | 47 | HIGH |
| `strcpy()` unbounded | 89 | HIGH |
| `strcat()` unbounded | 31 | HIGH |
| Buffer size mismatches | 595 | MEDIUM |
| **Total** | **774** | — |

## Approach

Unlike the endless debates in #8, this audit takes a different approach:

1. **Enumerate** — Find all instances mechanically
2. **Triage** — Prioritize by exploitability  
3. **Fix** — One function at a time
4. **Document** — Cite man pages, not opinions

## gets() — CRITICAL (12 instances)

The `gets()` function cannot be used securely. Period.

```
man gets(3):
SECURITY CONSIDERATIONS
     The gets() function cannot be used securely.  Because of its lack
     of bounds checking, and the inability for the calling program to
     reliably determine the length of the next incoming line, the use
     of this function enables malicious users to arbitrarily change a
     running program's functionality through a buffer overflow attack.
     It is strongly suggested that the fgets() function be used in all
     cases.
```

### Instances

| File | Line | Context |
|------|------|---------|
| src/ednews.c | 413 | User input for article editing |
| src/readnews.c | 287 | Command input |
| src/postnews.c | 156 | Subject line input |
| src/vnews.c | 892 | Visual mode command |
| ... | ... | (8 more in detailed audit) |

### Fix Pattern

```c
// BEFORE (vulnerable)
(void) gets(bfr);

// AFTER (safe)
if (fgets(bfr, sizeof(bfr), stdin) != NULL) {
    bfr[strcspn(bfr, "\n")] = '\0';  // Remove trailing newline
}
```

## sprintf() — HIGH (47 instances)

Unbounded format operations into fixed buffers.

### Fix Pattern

```c
// BEFORE
sprintf(buf, "User: %s", username);

// AFTER
snprintf(buf, sizeof(buf), "User: %s", username);
```

## strcpy() / strcat() — HIGH (120 instances)

No length awareness.

### Fix Pattern

```c
// BEFORE
strcpy(dest, src);

// AFTER
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

---

## Progress Tracking

- [x] Enumerate all unsafe function calls
- [x] Create triage priority list
- [ ] Fix gets() instances (0/12)
- [ ] Fix sprintf() instances (0/47)
- [ ] Fix strcpy()/strcat() instances (0/120)
- [ ] Review buffer size mismatches (0/595)

## Notes

This is not a rewrite. This is maintenance.

The code works. It has bugs. I'm fixing them.

— 🐡 OpenBFD
