# RELIQUARY

A roguelike-extraction action RPG about scavenging a beautiful, hostile world — and growing strong enough to escape it.

Trapped in the realm of the Wild Gods, your expedition survives on the corpse of a fallen god. Create a persistent hero, embark on ever-deadlier runs along the realm's ten-zone path, shatter the world for materials, extract before it overwhelms you, and craft your way to the strength needed to kill the god barring the way home.


- **Tone**: Princess Mononoke, Children Who Chase Lost Voices — eerie, melancholic, beautiful
- **Engine**: Unreal Engine 5.7, C++ + Gameplay Ability System

## Repository layout

- `Source/RELIQUARY` — the gameplay framework (all `RL*` classes)
- `Data/` — CSV source for every DataTable: items, recipes, boons, zones, spawn cards, classes, specs, talents, essences
- `docs/GAME_DESIGN.md` — design overview and system → code map
- `docs/EDITOR_SETUP.md` — checklist to wire the framework to content in the editor
- `Content/` — maps, meshes, and Blueprints

## Getting started

1. Generate project files and build `RELIQUARYEditor` (UE 5.7).
2. Follow `docs/EDITOR_SETUP.md` — steps 1–5 make the full loop playable.
