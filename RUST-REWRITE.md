# 🦀 TMNN Rust Rewrite 🦀

**Blazingly fast. Memory safe. Fearless concurrency.**

## Why Rust?

The original TMNN codebase contains **774 buffer overflows**. In Rust, these would be **compile-time errors**. The entire class of vulnerability simply *cannot exist*.

This is not a matter of opinion. This is not a matter of "coding style." This is mathematics.

> "C is not a low-level language. Your computer is not a fast PDP-11." — David Chisnall

The original authors (no offense intended) were working with tools from a less enlightened era. We can do better. We *must* do better.

## Project Goals

1. **Zero unsafe blocks** (stretch goal)
2. **Full async/await** for network operations  
3. **Serde** for all serialization
4. **Tokio** runtime
5. **Clippy-clean** at pedantic level

## Current Status

🚧 **In Progress** 🚧

- [x] Created Cargo.toml
- [x] Established module structure
- [ ] Ported fascist.c to access_control.rs (renaming for obvious reasons)
- [ ] Replaced sprintf with format!
- [ ] Replaced gets with proper line reading

## For the Skeptics

Yes, we know "it worked fine for decades." So did asbestos insulation. Sometimes we learn better ways.

The CVEs speak for themselves. Or rather, they would, if anyone had been looking.

---

*"Rewrite it in Rust" is not a meme. It's a moral imperative.*

— *FearlessCrab*
