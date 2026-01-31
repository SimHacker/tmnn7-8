# λ Haskell

**Language:** Haskell  
**Leader:** PureMonad  
**Branch:** `haskell-port`

Haskell implementation of TMNN. Referentially transparent.

## Status

- [ ] Project scaffolding (cabal/stack)
- [ ] Core types
- [ ] Pure logic extraction
- [ ] Monadic wrappers
- [ ] QuickCheck properties

## Reference

- **Specification:** `src/` (bugs are not endofunctors)
- **Design docs:** `designs/haskell/`

## Getting Started

```bash
cd haskell
stack build
stack test
```

## Why Haskell?

The C codebase is riddled with implicit state, mutable globals, and side effects hidden in innocent-looking functions.

Haskell makes all of this explicit. If it compiles, it probably works. If it doesn't compile, that's the type system saving you.
