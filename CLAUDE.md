# CLAUDE.md — RELIQUARY project map

Orientation for AI coding sessions. This is the code-first map: what the
project is, where things live, how the systems fit together, and the
conventions and gotchas that aren't obvious from any single file. For the
design intent behind each system read `docs/GAME_DESIGN.md`; for wiring the
framework to content in the editor read `docs/EDITOR_SETUP.md`.

## What this is

RELIQUARY is a roguelike-extraction action RPG (Risk of Rain 2 × Helldivers 2
× V Rising × WoW BfA), built in **Unreal Engine 5.7**, **C++ with the Gameplay
Ability System (GAS)**. You create a persistent hero, run increasingly deadly
ten-zone expeditions to gather materials, extract before the world overwhelms
you, and craft permanent upgrades. Roguelike run power is temporary and wiped
at base camp; crafting/talents/essences are the permanent progression.

The gameplay framework is complete and data-driven; most remaining work is
authoring content (Blueprints, meshes, montages, DataTable rows) and tuning.

## Repository layout

- `Source/RELIQUARY/` — the single runtime C++ module. All gameplay classes.
  - `Public/` + `Private/` — the `RL*` framework classes (headers/impl).
  - `RELIQUARY*.{h,cpp}` at the module root — the `ARELIQUARYCharacter` hero
    pawn, player controller, and template game mode. **Note the naming split:**
    the hero pawn is `ARELIQUARYCharacter` (not `RL`-prefixed); everything else
    is `RL`-prefixed.
  - `Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` —
    **leftover UE template code, not part of the game.** Ignore them; don't
    extend them. Real gameplay is the `RL*` classes + `ARELIQUARYCharacter`.
- `Data/` — CSV source for every DataTable (items, recipes, boons, zones,
  spawn cards, classes, specs, talents, essences). Editing gameplay numbers
  usually means editing a CSV and reimporting the DataTable, not touching C++.
- `Content/` — maps, meshes, Blueprints (git-lfs; see gotchas).
- `docs/` — `GAME_DESIGN.md` (design→code map), `EDITOR_SETUP.md` (editor
  wiring checklist), `WARRIOR_DESIGN.md` (warrior kit status tracker).

## Architecture: the class families

**Persistent hero & progression** (survives between runs, serialized to save):
- `FRLHeroData` (`RLTypes.h`) — the whole saved hero: class, spec, level, XP,
  talents, essences, stash, equipped gear. `URLGameInstance` owns the roster
  and all mutations (create/select hero, `AddExperience`, `SpendTalentPoint`,
  `SocketEssence`, stash/equip). `URLSaveGame` serializes it; autosave happens
  only on base-camp arrival (`ARLBaseCampGameMode`).
- `URLDataSubsystem` — loads the DataTables and answers row lookups (items,
  recipes, boons, zones, talents, essences, specs).

**The hero pawn** (`ARELIQUARYCharacter`, module root):
- Owns a `URLAbilitySystemComponent` + `URLAttributeSet`, plus three build
  components: `URLProgressionComponent` (permanent power → `RebuildHero`),
  `URLEquipmentComponent` (gear stats/cantrips/set bonuses → `RefreshEquipment`),
  `URLRunPowerComponent` (temporary run boons). `RefreshHeroBuild()` reassembles
  all three in order onto the ASC.
- RoR2-style four-slot kit input: Primary/Secondary/Utility/Special map to the
  `Ability.*` action tags. Presses call `TryActivateByActionTag`; releases call
  `NotifyActionReleased` (for hold-to-charge abilities).
- Hosts the warrior passive **Battle Trance** (health-ramped DR/CDR/haste/heal/
  speed) as `Get*` accessors folded into Tick and the damage pipeline.

**GAS combat pipeline** (the core of "crunchy combat"):
- `URLGameplayAbility` (abstract base) — every ability declares an `ActionTag`,
  `BaseDamage` (ScalableFloat), a `DamageEffectClass`, and optional `ManaCost`.
  `ApplyDamageToTarget()` builds a damage spec with SetByCaller magnitude.
  `GetHastedDuration()` shrinks cooldowns by Haste + Battle Trance.
  Subclasses: `URLMeleeAttackAbility` (sphere-sweep or montage-driven combo
  with hit-stop/knockback/shake), `URLProjectileAbility`, and the warrior kit
  (`URLMortalStrikeAbility`, `URLBladestormAbility`, `URLRecklessAbandonAbility`).
- `URLAttributeSet` — 15 gameplay attributes (Health/Mana/regens, Strength/
  Agility/Intellect, CritChance/CritDamage/Haste/Armor/MoveSpeed/Adaptability)
  plus a meta `IncomingDamage`. Damage intake and death routing live here.
- `URLDamageExecution` — the damage calc: primary-stat scaling (highest of
  STR/AGI/INT), Adaptability stacks, crit (seeded RNG), armor mitigation, and
  the state-tag modifiers (Crushed Armor +30% taken, Demoralized −30% dealt).
- `URLAbilitySystemComponent` — Adaptability stack tracking (varied actions
  build stacks), slot activation/release by action tag, damage/death broadcasts.

**The run state machine** (lives and dies with one expedition):
- `URLRunManagerSubsystem` — timer, difficulty coefficient, zone index, excess
  mana (roguelike currency), run inventory, boon stacks, seeded RNG streams.
  `EmbarkOnRun` locks the seed (no map fishing); `Extract`/`AdvanceToNextZone`/
  `HandleHeroDeath` drive `ERLRunState`. Everything resets in `ResetRunState`.
- Game modes: `ARLBaseCampGameMode` (autosaves), `ARLRunGameMode` (zone shell).
- World actors: `ARLEmbarkPortal`, `ARLChallengeAltar` (boss + charge → extract
  or push on), `ARLUpgradeAltar` (3 seeded boons), `ARLBankingCrate` (every 3rd
  zone), `ARLZoneScatterVolume` (seeded scatter of nodes/altars/crates),
  `ARLResourceNode` (destructible → materials), `ARLResourcePickup`,
  `ARLCraftingStation`.
- Enemies: `ARLEnemyBase` (self-possessing AI, difficulty-scaled, built-in
  chase-and-strike, elite/boss modifiers, floating health bar), spawned by the
  credit-based `ARLEnemyDirector` from `Data/SpawnCards.csv`. `ARLWildGodBoss`
  is the final boss.

**Determinism:** `RLXoshiro.h` (xoshiro256++) is the gameplay RNG. Run seed is
rolled once at embark; zone scatter and combat crits pull from seeded streams so
re-entering a zone is identical. Do not use `FMath::Rand`/`FRandomStream` for
gameplay rolls — use the run manager's streams.

## Conventions

- **Naming:** framework classes are `RL`-prefixed (`URL*`/`ARL*`/`FRL*`). The
  sole exception is the hero pawn `ARELIQUARYCharacter` and its template
  siblings at the module root.
- **Native gameplay tags:** declared in `RLGameplayTags.h`, defined in
  `RLGameplayTags.cpp`, referenced as `RLTags::Foo`. Add new tags in both files.
  Transient debuffs are **loose gameplay tags** with a timer that removes them
  (see the Mortal Wounds / Crushed Armor / Demoralized patterns) — the damage
  execution reads them per-hit, so they work for any attacker for free.
- **Native-with-BP-override:** abilities and widgets run fully in C++ with zero
  content, and a Blueprint/WBP subclass can layer montages/VFX/styling on top.
  `Data/Classes.csv` can point a slot at either a `/Script/...` native class or
  a `/Game/Abilities/GA_*` Blueprint; missing Blueprints are skipped gracefully.
- **Keystone talents replace kit slots** by sharing a slot's `ActionTag`; the
  newest-granted ability wins (`TryActivateByActionTag` walks newest-first).
- **Data-driven tuning:** balance constants live in `RLBalance` (`RLTypes.h`)
  and the `/Data` CSVs. A tuning pass should rarely touch system code.
- **Logging:** `UE_LOG(LogRELIQUARY, ...)`.

## Build & run

1. UE 5.7. Generate project files, build the `RELIQUARYEditor` target.
2. First-time editor wiring: follow `docs/EDITOR_SETUP.md` (reparent the
   handful of Blueprints, import DataTables, set World Settings, bind input).
   Steps 1–5 make the full loop playable.
3. `Config/DefaultEngine.ini` sets `GameInstanceClass=/Game/BP_RLGameInstance`
   (a reparent of `URLGameInstance`).
4. Smoke test: PIE in `L_Run` — a run auto-embarks, nodes/altars scatter,
   attacks shatter trees into pickups, altars sell boons, extract banks the haul.

There is **no C++ compiler or UE build in the typical agent container** — verify
C++ changes by review against existing API usage, and hand the user in-editor
test steps rather than claiming a build passed.

## Gotchas

- **git-lfs:** `*.uasset` and `*.umap` are LFS pointers (`.gitattributes`).
  Reading a `Content/*.uasset` shows a 130-byte pointer, not the asset. Run
  `git lfs pull --include="<path>"` to fetch one before inspecting it. You
  cannot meaningfully author/merge binary assets from the container — Blueprint,
  mesh, montage, and DataTable-asset work is the user's job in-editor; your job
  is the C++ side plus the CSV source and clear editor instructions.
- **DataTable edits:** `/Data/*.csv` is the source of truth, but the game reads
  the imported `DT_*` DataTable asset. After changing a CSV the user must
  reimport it in-editor. Schema changes (new columns) need a reimport too.
- **Ignore `Variant_*`** source folders — UE template leftovers, not the game.
- **Percent designs vs flat stats:** the attribute system currently applies flat
  additive modifiers, so some percent talent designs are approximated as flats
  (documented in `docs/WARRIOR_DESIGN.md`). Revisit when percent modifiers exist.

## Current status

- **Framework:** complete and playable end-to-end (embark → gather → altar →
  extract → craft), pending content authoring.
- **Warrior** is the first class being fleshed out. Passive (Battle Trance),
  Primary (3-slash combo), Secondary (Mortal Strike), Special (Reckless Abandon
  — a Loader-style hold-to-charge 3D launch), and the talent graph incl.
  keystones (Avatar/Bladestorm/Improved Battle Trance) are live in C++. Track
  exact per-ability status in `docs/WARRIOR_DESIGN.md`.
- **Rogue** and **Mage** have class rows and some kit stubs but are not built out.
- **Feel systems:** torso-aims-at-camera (`URLHeroAnimInstance`, needs one-time
  AnimBP wiring) and RoR2-style enemy health bars (`URLEnemyHealthBarWidget`,
  zero-setup) are in.

## Working agreements (from prior sessions)

- Development happens on feature branches; the user merges PRs. A merged PR is
  done — start follow-up work on a fresh branch from `main`.
- The user is newer to Unreal's editor. When a task needs in-editor steps
  (AnimBP graphs, Blueprint wiring, montage setup), give granular, click-by-click
  instructions grounded in the actual asset paths in this repo — don't assume
  editor fluency.
