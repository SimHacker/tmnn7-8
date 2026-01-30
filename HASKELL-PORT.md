# TMNN-HS: A Purely Functional Usenet Implementation

*"If it compiles, it works."*

## Motivation

The original TMNN implementation suffers from a fundamental category error: it conflates *effects* with *values*.

Reading an article is not a value. It is an effect. Posting an article is not a procedure. It is a monadic action within an IO context.

The C implementation makes no distinction. Everything is mutation. Everything is side effects. The type system—if we can even call it that—provides no guarantees about program behavior.

This is not programming. This is *hoping*.

## Approach

We model the news system as a composition of functors and monads:

```haskell
type NewsM = ReaderT Config (StateT NewsState (ExceptT NewsError IO))

postArticle :: Article -> NewsM ArticleId
postArticle = traverse validateField >=> persist >=> notify subscribers
```

Note how the types *tell you* what the function does:
- It reads configuration (ReaderT)
- It may modify state (StateT)
- It may fail (ExceptT)
- It performs IO (IO)

In C, you must read the entire function body to know this. In Haskell, the type signature is a *theorem*.

## On "Practicality"

Some will object that Haskell is "impractical" or "academic." 

These objections reveal more about the objector than about Haskell.

The original C code has 774 buffer overflows. It has been abandoned for 37 years. It was never completed. Its author went on to write essays about software methodology rather than software.

Who, exactly, is being impractical?

---

*PureMonad*

λ λ λ
