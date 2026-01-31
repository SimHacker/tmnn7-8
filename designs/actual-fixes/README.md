# 🐡 actual-fixes Designs

**Faction:** actual-fixes  
**Leader:** Puffy (Theo de Raadt)  
**Branch:** `actual-fixes`

Design documents for the branch that actually fixes things.

## Philosophy

> "Talk is cheap. Show me the patch."

While others debated architecture, this branch fixed 872 bugs. The code speaks for itself.

## Principles

| Principle | Meaning |
|-----------|---------|
| Security first | Every change considers attack surface |
| Correctness second | Correct code that's insecure is useless |
| Style matters | Tabs, not spaces. Fight me. |
| No half-measures | Fix it right or don't fix it |
| pledge() and unveil() | Sandbox everything |

## What We Fixed

### The 872 Bugs

| Category | Count | Approach |
|----------|-------|----------|
| sprintf → snprintf | 312 | Bounded writes, always |
| strcpy → strlcpy | 287 | Length-aware copying |
| strcat → strlcat | 156 | Safe concatenation |
| gets → fgets | 47 | Never trust stdin |
| strtok → strtok_r | 38 | Thread-safe tokenization |
| mktemp → mkstemp | 19 | Race-free temp files |
| Shell injection | 13 | Proper escaping |

### Methodology

1. **Systematic sweep** — Hit every file, every function
2. **Pattern matching** — Same fix pattern for same bug class
3. **No regressions** — Each fix verified
4. **Atomic commits** — One logical change per commit

## Reference Code

The `src/` directory now contains:
- **Zero** known buffer overflows
- **Zero** format string vulnerabilities
- **Zero** unsafe string operations

This is the specification. Rewriters: match this behavior.

## Design Documents

| Document | Description | Status |
|----------|-------------|--------|
| `security-audit.md` | Full audit methodology | TODO |
| `fix-patterns.md` | Reusable fix templates | TODO |
| `sandbox-model.md` | pledge/unveil strategy | TODO |

## Style Guide

```c
// WRONG (spaces, barbaric)
if (x == 1) {
    do_thing();
}

// RIGHT (tabs, civilized)
if (x == 1) {
	do_thing();
}
```

This is not negotiable.
