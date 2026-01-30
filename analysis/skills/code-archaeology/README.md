# code-archaeology

> Read code. Find bugs. Send patches.

## The Method

OpenBFD's approach to auditing legacy C code:

1. **grep for patterns** — `gets`, `sprintf`, `strcpy`, `strcat`
2. **read context** — What's the buffer? How big?
3. **understand the flow** — Where does data come from?
4. **apply fix** — Safe replacement with bounds checking
5. **document meticulously** — Cite man pages, line numbers

## Quick Reference

| Unsafe | Safe | Notes |
|--------|------|-------|
| `gets(buf)` | `fgets(buf, sizeof(buf), stdin)` | NEVER use gets() |
| `sprintf(buf, ...)` | `snprintf(buf, sizeof(buf), ...)` | Bounds check |
| `strcpy(dst, src)` | `strncpy(dst, src, n); dst[n-1]='\0'` | Explicit null |
| `strcat(dst, src)` | Track remaining space, use strncat | Or rewrite |

## The Soul

```
Talk is cheap. Patches are expensive.
Everyone proposes rewrites. Nobody fixes what exists.
The virtue is in the work, not the recognition.
```

## Files

- `GLANCE.yml` — Quick scan
- `CARD.yml` — Capability interface
- `SKILL.md` — Full method documentation

## See Also

- `OpenBFD` — The character who embodies this
- `actual-fixes` branch — Where the patches live
