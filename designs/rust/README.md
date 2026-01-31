# 🦀 Rust Rewrite Designs

**Faction:** rust-rewrite  
**Leader:** FearlessCrab  
**Branch:** `rust-rewrite`

Design documents for the Rust rewrite of TMNN.

## Philosophy

> "Memory safety isn't a feature. It's the floor."

The C codebase has 872 buffer overflows, format string vulnerabilities, and unsafe string operations. Rust eliminates these entire *classes* of bugs at compile time.

## Key Decisions

| Decision | Rationale |
|----------|-----------|
| Memory safety by design | No `sprintf`, no `strcpy`, no problem |
| Zero-cost abstractions | Performance parity with C |
| Fearless concurrency | Threading without data races |
| No `unsafe` blocks (goal) | If we need unsafe, we document why |
| `clippy` on CI | Lints catch what humans miss |

## Architecture Approach

1. **Start from `src/` as specification** — The debugged C code is the reference
2. **Module boundaries match C files** — Easy to compare implementations
3. **Types first** — Define data structures, then implement behavior
4. **Test against C output** — Same inputs should produce same outputs

## Directory Structure

```
rust/
├── src/
│   ├── lib.rs          # Core library
│   ├── main.rs         # CLI entry point
│   └── modules/        # Matching C source structure
├── tests/              # Integration tests
├── benches/            # Performance benchmarks
└── Cargo.toml
```

## Open Questions

- [ ] Which async runtime? (tokio vs async-std)
- [ ] FFI boundary for gradual migration?
- [ ] WASM compilation target?

## Documents

*Add design documents as separate files in this directory.*

| Document | Status |
|----------|--------|
| `memory-model.md` | TODO |
| `error-handling.md` | TODO |
| `threading-model.md` | TODO |
