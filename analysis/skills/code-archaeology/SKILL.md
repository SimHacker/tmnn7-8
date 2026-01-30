# code-archaeology

> Read code. Find bugs. Send patches.

## What This Skill Does

Code archaeology is OpenBFD's method for systematically auditing legacy C code. It's not glamorous. It's not a rewrite. It's reading thousands of lines of code, finding the dangerous patterns, and fixing them one by one.

The patches rarely get merged. But they exist.

---

## The Method

### 1. Grep for Patterns

Start with known-unsafe functions:

```bash
grep -n 'gets\|sprintf\|strcpy\|strcat\|mktemp\|getwd' src/*.c
```

This finds:
- `gets()` — Buffer overflow waiting to happen
- `sprintf()` — Overflow if output exceeds buffer
- `strcpy()` / `strcat()` — No length awareness
- `mktemp()` — Race condition
- `getwd()` — Fixed buffer, no size parameter

### 2. Read Context

For each hit, understand:

- What's the buffer? How big?
- Where was it declared? (stack, heap, global?)
- What fills it? (user input, file, network?)
- What's the maximum possible input size?

### 3. Understand Flow

Trace the data:

```
INPUT → [source] → [transform] → [unsafe function] → [consequence]
```

Ask:
- Is the input user-controlled?
- Can an attacker control the size?
- What happens on overflow?

### 4. Apply Fix

Replace unsafe with safe, preserving behavior:

| Unsafe | Safe | Notes |
|--------|------|-------|
| `gets(buf)` | `fgets(buf, sizeof(buf), stdin)` | NEVER use gets() |
| `sprintf(buf, ...)` | `snprintf(buf, sizeof(buf), ...)` | Bounds check |
| `strcpy(dst, src)` | `strncpy(dst, src, n); dst[n-1]='\0'` | Explicit null |
| `strcat(dst, src)` | Track remaining space, use strncat | Or rewrite |
| `mktemp(template)` | `mkstemp(template)` | Exists since 4.3BSD |
| `getwd(buf)` | `getcwd(buf, sizeof(buf))` | Pass size |

### 5. Document Meticulously

Every fix includes:

```markdown
**File:** ednews.c, line 413

**Issue:** `gets(bfr)` — unbounded read into stack buffer

**Man page (gets):**
> SECURITY CONSIDERATIONS: The gets() function cannot be used securely.
> It is strongly suggested that fgets() be used in all cases.

**Historical context:**
- Morris Worm (November 2, 1988) exploited gets() in fingerd
- CERT Advisory CA-1988-01

**Fix:**
```c
// Before
(void) gets(bfr);

// After  
if (fgets(bfr, sizeof(bfr), stdin) != NULL) {
    bfr[strcspn(bfr, "\n")] = '\0';  // Remove trailing newline
}
```

Patch attached.
```

---

## Unsafe Function Reference

### gets() — CRITICAL

**The worst function in C.**

```c
char buffer[100];
gets(buffer);  // NEVER DO THIS
```

There is no safe way to use `gets()`. It reads until newline or EOF, ignoring buffer size. The man page has said "cannot be used securely" since 1986.

**Fix:**
```c
if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    buffer[strcspn(buffer, "\n")] = '\0';
}
```

### sprintf() — HIGH

**Buffer overflow if format output exceeds buffer.**

```c
char buffer[20];
sprintf(buffer, "User: %s", user_input);  // Overflow if user_input > 14 chars
```

**Fix:**
```c
snprintf(buffer, sizeof(buffer), "User: %s", user_input);
```

### strcpy() — HIGH

**No length limit on copy.**

```c
char dest[100];
strcpy(dest, source);  // Overflow if source > 99 chars
```

**Fix:**
```c
strncpy(dest, source, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';  // strncpy doesn't null-terminate if src >= n
```

### strcat() — HIGH

**No awareness of remaining space.**

```c
char buffer[100] = "prefix: ";
strcat(buffer, user_input);  // Overflow if prefix + input > 99
```

**Fix:**
```c
size_t current_len = strlen(buffer);
strncat(buffer, user_input, sizeof(buffer) - current_len - 1);
```

Better: rewrite to track buffer state explicitly.

---

## The Soul of Code Archaeology

```
Talk is cheap. Patches are expensive.
Everyone proposes rewrites. Nobody fixes what exists.
```

Code archaeology doesn't require:
- Understanding the whole codebase
- Permission from maintainers
- Consensus on the "right" approach
- A complete rewrite

It requires:
- Reading code
- Finding bugs
- Writing fixes
- Attaching patches

The fixes work. They preserve behavior. They're minimal. They're documented.

They also get ignored. That's the tragedy.

But the patches exist. The branch exists. The work was done.

**The virtue is in the work, not the recognition.**

---

## OpenBFD's Annotations

When OpenBFD reviews code, the annotations follow this pattern:

```
🐡 **{file}:{line}:**

{problematic code block}

{rhetorical question about the code}

From {function}({section}), since {year}:
> {relevant man page quote}

{historical context if applicable}

{fix explanation}

Patch attached.

— 🐡 OpenBFD
```

---

## Integration with GitHub Simulation

In the tmnn7-8 simulation:

1. **OpenBFD finds bugs** — Using this method
2. **Posts issues** — With full documentation
3. **Other characters respond** — Proposing rewrites, philosophy, etc.
4. **OpenBFD ignores them** — Continues patching
5. **Patches go to actual-fixes branch** — Never merged
6. **The code sits unfixed** — This is the joke. This is also real.

The simulation demonstrates both the method AND its futility.

---

## See Also

- **OpenBFD** — The character who embodies this method
- **actual-fixes branch** — Where the patches live
- **github skill** — Core GitHub operations for posting issues/PRs
