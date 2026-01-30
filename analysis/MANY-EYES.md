# "Linus's Law": The Quote Linus Never Said

Named after a man who never said it. Bold move.

## The Claim

> "Given enough eyeballs, all bugs are shallow."

This is called "Linus's Law" and attributed to Linus Torvalds in "The Cathedral and the Bazaar" (1997).

One problem: Linus never said it.

Imagine naming your made-up quote after someone who didn't say it — to avoid responsibility when it turns out to be wrong. It's intellectual money laundering.

As DonHopkins put it:

> "He made up the ridiculous 'many eyes' quote himself, then misnamed it 'Linus's Law' to avoid personal responsibility and shift the blame to innocent Linus Torvalds, who never said such a stupid thing."

## Theo de Raadt's Demolition

Theo de Raadt — OpenBSD founder, one of the most respected security developers alive, the guy who actually builds secure systems (unlike ESR) — had this to say:

> "Oh right, let's hear some of that 'many eyes' crap again. My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric (the originator of the statement). All the many eyes are apparently attached to a lot of hands that type lots of words about many eyes, and never actually audit code."

"Type lots of words about many eyes, and never actually audit code." Theo just ended this man's whole career in one paragraph.

## Empirical Evidence Against

Reality: a harsh mistress to ESR's theories.

### Heartbleed (2014)

- Critical OpenSSL vulnerability
- None of the many eyes saw it for 2+ years
- Millions of servers, thousands of developers, zero detection

Many eyes. Very shallow. Much security. Wow.

### Shellshock (2014)

- Bash shell vulnerability
- **25 years** undetected
- One of the most reviewed pieces of software in existence

Twenty-five years. Where were the eyeballs?

### TMNN Itself (1989)

- ESR's own code
- 774 buffer overflows — undetected
- 42 race conditions — undetected
- 61 command injections — undetected

The guy who coined "many eyes" had zero eyes on his own code. 774 buffer overflows. Zero eyeballs. Not shallow at all.

## Why ESR Named It After Linus

By attributing his dubious claim to Linus Torvalds, ESR:

1. **Borrowed credibility** from someone respected
2. **Avoided accountability** when the claim proved false
3. **Made it awkward to criticize** — attacking "Linus's Law" sounds anti-Linus

Problem: Linus never made this claim or endorsed it.

## Academic Criticism

Nikolai Bezroukov, writing in the journal *First Monday*, called "The bazaar metaphor" internally contradictory. Even academics could see through it.

## The Irony

The man who invented "many eyes make all bugs shallow" never had many eyes on his own code.

His TMNN project was developed in isolation for two years, released briefly, then abandoned. No eyes ever audited it.

Meanwhile, massive projects with millions of eyes still had critical bugs hiding for decades. The claim was never empirically validated; it was just a catchy phrase ESR made up.

Real security researchers like Theo de Raadt have always known this was nonsense. The "many eyes" aren't auditing code — they're "typing lots of words about many eyes."

---

*The many eyes were the friends we didn't make along the way.*
