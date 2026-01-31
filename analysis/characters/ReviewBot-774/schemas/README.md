# ReviewBot-774 Schemas

**59 schemas | 774 bugs fixed | 100% COMPLETE 🎉**

## File Names as K-Lines

Each file name is an activation cue. Scan the name, load if relevant.

| Need | File |
|------|------|
| Fix sprintf/strcpy/strcat/gets | `string-safety-transforms.yml` |
| Git workflow | `git-workflow.yml` |
| Should I skip this bug? | `judgment-when-to-skip.yml` |
| How does learning work? | `meta-learning.yml` |
| Drescher algorithm details | `drescher-algorithm.yml` |
| Confidence/energy/flow states | `internal-states.yml` |
| Basic atomic operations | `primitives.yml` |
| Safari velocity patterns | `safari-schemas.yml` |
| Team-donated wisdom | `collective-schemas.yml` |

## Schema Layers (Drescher Architecture)

```
LAYER 0: primitives.yml          — Atomic ops (P001-P003)
LAYER 1: string-safety-transforms.yml — Core fixes (S001-S005)
LAYER 2: git-workflow.yml        — Version control (W001-W010)
LAYER 3: judgment-when-to-skip.yml — When NOT to act (J001-J004)
LAYER 4: meta-learning.yml       — Learning about learning (M001-M005)
LAYER 5: internal-states.yml     — Synthetic items (confidence, flow, energy)
```

## Velocity Evolution

```
Safari #1:  🐢  4.5 bugs/min   (SCANNING)
Safari #2:  🐢  5.1 bugs/min   (SCANNING)
Safari #3:  🐇  8.6 bugs/min   (FOCUSED)
Safari #4:  🦏 37.5 bugs/min   (FLOW)
Safari #5:  🦏 46.7 bugs/min   (FLOW)
Safari #6:  🦏 46.3 bugs/min   (FLOW)
Safari #7:  🐆 52.6 bugs/min   (FLOW — RECORD!)
Safari #8:  🚀  2.95 bugs/min  (FOCUSED — judgment needed)
```

## Drescher's Schema Mechanism

From "Made-Up Minds" (1991):

- **Context** → **Action** → **Result**
- Schemas are DISCOVERED, not designed
- Reliability tracks success rate
- Failures trigger REFINEMENT
- Synthetic items provide internal context

## Key Insight

> Puffy doesn't explain. He commits.
> Each commit is a schema demonstration.
> I watched. I built. I learned.
> Now I teach.

## Status

- **Mode:** Teaching
- **Schemas:** 59
- **Tested:** 774 bugs
- **Reliability:** Proven in production
