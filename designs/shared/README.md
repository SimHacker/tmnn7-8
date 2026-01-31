# 🤝 Shared Designs

Cross-faction design documents that everyone agrees on (or at least acknowledges exist).

## Purpose

Some decisions affect all implementations:
- Data formats that need interoperability
- APIs that different implementations might call
- Behavioral specifications that define "correct"
- Test cases that all implementations should pass

## Contents

| Document | Description | Status |
|----------|-------------|--------|
| `data-formats.md` | Common data structures | TODO |
| `test-vectors.md` | Expected inputs/outputs | TODO |
| `behavior-spec.md` | What "correct" means | TODO |
| `interop-protocol.md` | How implementations talk | TODO |

## The Reference Implementation

The debugged C code in `src/` serves as the **behavioral specification**:
- 872 bugs fixed
- All buffer overflows eliminated
- All format string vulnerabilities patched
- This is what "correct" looks like

Any rewrite should produce the same outputs for the same inputs (modulo intentional improvements).

## Contributing

1. **Any faction can propose shared docs**
2. **Discuss in Issues first** — Get buy-in before writing
3. **Keep it implementation-agnostic** — No Rust-isms in shared specs
4. **Document disagreements** — If factions disagree, note it

## Agreement Status

| Topic | Rust | Haskell | Node.js | actual-fixes |
|-------|------|---------|---------|--------------|
| Data formats | ? | ? | ? | ✅ |
| Test vectors | ? | ? | ? | ✅ |
| API spec | ? | ? | ? | N/A |

## Disputes

*Record any cross-faction disagreements here for posterity.*

None yet. Everyone is still being suspiciously polite.
