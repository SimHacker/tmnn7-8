# λ Haskell Port Designs

**Faction:** haskell-port  
**Leader:** PureMonad  
**Branch:** `haskell-port`

Design documents for the Haskell port of TMNN.

## Philosophy

> "If it compiles, it probably works. If it doesn't compile, that's the type system saving you."

The C codebase is riddled with implicit state, mutable globals, and side effects hidden in innocent-looking functions. Haskell makes all of this explicit.

## Key Decisions

| Decision | Rationale |
|----------|-----------|
| Pure functional approach | State is explicit, not hidden |
| Monadic I/O | Side effects are tracked in types |
| Strong typing | Bugs are not endofunctors |
| Referential transparency | Same input = same output, always |
| Lazy evaluation | Compute only what you need |

## Architecture Approach

1. **Extract the pure core** — Identify what can be pure functions
2. **Wrap impure operations in IO** — File I/O, network, etc.
3. **State monad for threading state** — No mutable globals
4. **Property-based testing** — QuickCheck everything

## Type Strategy

```haskell
-- The C code has implicit buffer sizes everywhere
-- Haskell: make it explicit in the type

newtype BoundedString (n :: Nat) = BoundedString Text
  deriving (Show, Eq)

-- Can't overflow what you can't create
mkBoundedString :: forall n. KnownNat n => Text -> Maybe (BoundedString n)
mkBoundedString t
  | Text.length t <= fromIntegral (natVal (Proxy @n)) = Just (BoundedString t)
  | otherwise = Nothing
```

## Directory Structure

```
haskell/
├── src/
│   └── TMNN/          # Main module hierarchy
├── test/              # HSpec + QuickCheck
├── app/               # Executable entry point
├── tmnn.cabal
└── stack.yaml
```

## Open Questions

- [ ] GHC version target?
- [ ] Streaming library choice? (conduit vs pipes vs streamly)
- [ ] Effect system? (mtl vs effect-handlers vs polysemy)

## Documents

*Add design documents as separate files in this directory.*

| Document | Status |
|----------|--------|
| `monad-stack.md` | TODO |
| `type-level-safety.md` | TODO |
| `performance-considerations.md` | TODO |
