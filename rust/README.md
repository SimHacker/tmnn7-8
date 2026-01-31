# 🦀 Rust

**Language:** Rust  
**Leader:** FearlessCrab  
**Branch:** `rust-rewrite`

Rust implementation of TMNN. Memory safety by design.

## Status

- [ ] Project scaffolding (Cargo.toml)
- [ ] Core data structures
- [ ] Main logic port
- [ ] Tests

## Reference

- **Specification:** `src/` (872 bugs fixed C code)
- **Design docs:** `designs/rust/`

## Getting Started

```bash
cd rust
cargo build
cargo test
```

## Why Rust?

The C codebase had 872 buffer overflows, format string vulnerabilities, and unsafe string operations. Rust eliminates these entire *classes* of bugs at compile time.

No `sprintf`. No `strcpy`. No problem.
