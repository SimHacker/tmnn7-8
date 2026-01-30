# Rooms

**Spaces in the TMNN performance art world.**

---

## What Are Rooms?

In MOOLLM, directories are rooms. When you enter a directory, you enter a space with:
- Furniture and objects
- Characters present
- Ambiance and mood
- Things to interact with

Rooms are defined by `ROOM.yml` files that describe the space.

---

## Current Rooms

| Room | Description | Path |
|------|-------------|------|
| [Green Room](green-room/) | Where performers gather before going on stage | `analysis/rooms/green-room/` |

---

## Future Rooms

Each character could have their own room modeling their dev environment:

| Character | Room | Vibe |
|-----------|------|------|
| daFlute | The Study | Old terminals, `vi` keybindings, books everywhere |
| FearlessCrab | The Workshop | Rust posters, zero `unsafe` blocks allowed |
| PureMonad | The Office | Whiteboards of category theory, Haskell books |
| WebScaleChad | The Startup | Standing desks, ping pong, AWS dashboard on wall |
| plannedchaos | Corner Office | PowerPoint decks, RACI matrices, motivational posters |

---

## MOOLLM Integration

Rooms work standalone (just read the ROOM.yml) but are enhanced by MOOLLM:
- Characters auto-activate when you enter
- Spatial relationships inform interactions
- AI characters can navigate between rooms, like Zork, Adventure, or LambdaMOO
- Objects can be interacted with, characters can take and drop and move objects as inventory, embark and disembark rooms as vehicles, etc.

See [MOOLLM Room Skill](https://github.com/SimHacker/moollm/tree/main/skills/room) for details.

---

*"All the world's a stage..."*
