# RELIQUARY — Game Design & Code Map

> A roguelike-extraction action RPG about scavenging a beautiful, hostile world — and growing strong enough to escape it.

This document maps every system from the production map to its implementation. Gameplay inspiration: **Risk of Rain 2** (moment-to-moment feel, difficulty scaling, director, teleporter ritual), **Helldivers 2** (extraction stakes, banking under pressure), **V Rising** (gather → craft → grow permanently stronger), **World of Warcraft: Battle for Azeroth 8.3** (specs, talents, Heart of Azeroth-style essence system).

## The Core Loop

1. **Embark** from base camp (the corpse of a fallen Wild God) — `ARLEmbarkPortal` → `URLRunManagerSubsystem::EmbarkOnRun()`. The run seed is rolled and locked here; no map fishing.
2. **Push your luck** — difficulty climbs on RoR2's stopwatch formula (`RLCombat::DifficultyCoefficient`), while the world feeds you excess mana and materials.
3. **Fight and harvest** — enemies from the credit-based `ARLEnemyDirector`; destructible `ARLResourceNode` trees/rocks shatter into distinct materials.
4. **Extract** — charge a challenge altar (`ARLChallengeAltar`), kill its empowered boss, then choose: home with the haul, or deeper.
5. **Craft** — at base camp, `URLCraftingLibrary` turns stashed materials into gear with stats, cantrips, and set bonuses.
6. **Head out again — stronger.**

## System → Code Map

| Production map system | Implementation |
|---|---|
| Persistent player-created heroes | `FRLHeroData` (`RLTypes.h`), `URLGameInstance`, `URLSaveGame` |
| Three classes, specs, talents | `Data/Classes.csv`, `Data/Specs.csv`, `Data/Talents.csv`, spend logic in `URLGameInstance::SpendTalentPoint` |
| Evolving identity at max level | `URLGameInstance::GetDisplayedClassName()` — dominant talent tree's `EvolvedTitle` at level 30 |
| Wide stat set + Adaptability | `URLAttributeSet` (13 attributes + meta damage); Adaptability stacks on `URLAbilitySystemComponent` |
| Leveling 1–30, exponential XP | `RLBalance::XPForNextLevel` (+35%/level), `URLGameInstance::AddExperience` |
| Heart of Azeroth-style essences | Reliquary Shard: `FRLSocketedEssence`, `Data/Essences.csv`, major slot at 10, minors at 18/26 |
| Fun, crunchy combat | Classic-WoW damage pipeline: `URLGameplayAbility` → `URLDamageEffect` → `URLDamageExecution` (see `RLCombatFormulas.h`) |
| Destructible world → resources | `ARLResourceNode::TakeDamage` — abilities damage non-GAS blockers as world objects |
| Ten interconnected levels | `Data/Zones.csv` (Verdant Threshold → The Last Door), traveled by `URLRunManagerSubsystem` |
| Challenge altars | `ARLChallengeAltar`: activate → empowered boss + charge radius → extract or teleport onward |
| Banking every 3 maps | `URLRunManagerSubsystem::ShouldOfferBankingCrate` → `ARLBankingCrate` |
| Simple, lethal enemies | `ARLEnemyBase` (difficulty-scaled), elites drop Manalith Shards |
| Enemy spawning | `ARLEnemyDirector` — RoR2 combat-director algorithm, spawn cards in `Data/SpawnCards.csv` |
| Death forfeits resources | `URLRunManagerSubsystem::HandleHeroDeath` (banked crates stay safe) |
| Autosave at base camp only, locked RNG | `ARLBaseCampGameMode::BeginPlay` saves; seed locked at embark |
| Excess mana + altar choices | `URLRunManagerSubsystem` currency; `ARLUpgradeAltar` offers 3 seeded boons from `Data/Boons.csv` |
| Temporary by design | `URLRunPowerComponent`; everything resets in `URLRunManagerSubsystem::ResetRunState` |
| Crafting is the heart | `Data/Items.csv` + `Data/Recipes.csv` + `URLCraftingLibrary` + `ARLCraftingStation` |
| Distinct materials, planned routes | No generic "wood": Oakheart, Ironwood, Feywood... each zone lists its reliable spawns |
| Gear: cantrips, set bonuses | `FRLItemRow::CantripEffect`, `Data/SetBonuses.csv`, applied by `URLEquipmentComponent` |
| Semi-procedural maps | `ARLZoneScatterVolume` — seeded scatter of nodes/altars/crates per zone |
| Minimalist HUD | `URLRunHUDWidget` — timer, location, vitals, mana, resources, danger label |
| Run win condition | Extract at any charged altar; zone 10's boss ends the run victorious |
| Game win condition | `ARLWildGodBoss` — challengeable at any time via portal, tuned for geared level 30 |

## Damage: action-RPG pipeline with Classic WoW bones (`RLCombatFormulas.h`)

No auto attacks, no cast times — abilities are the only source of damage, RoR2 style. What survives from Classic WoW is what still fits an action game:

- **Power by class, Classic ratios** — Warrior AP = Strength × 2; Rogue AP = Strength + Agility; SpellPower = Intellect; enemies default to Strength × 2.
- **Hit = Base + PowerCoefficient × Power** — every ability declares a coefficient (bread-and-butter ~0.8, big payoffs 2.5+), exactly how RoR2 skills are "X% damage".
- **Schools** — Physical is mitigated by armor; Spell **ignores armor** (Classic). Armor DR = Armor / (Armor + 400 + 85 × AttackerLevel), capped 75% — deep-run monsters partially shred your armor via the level term.
- **Crit** — chance = 5% base + gear + 1% per 20 Agility (physical) or 1% per 60 Intellect (spell); multiplier = CritDamage + **Force**.
- **No attack table** — no miss/dodge/parry/glancing; whiffing feels bad in an action game.

### The proc suite (`URLAbilitySystemComponent` tracks the live state)

Pipeline order: Sanguination → Adaptability → Crit → Armor → Multistrike, with state advancing *after* each hit so "consecutive" effects never retroactively buff the hit that built them.

| Stat | Effect |
|---|---|
| **Adaptability** | +X% damage per non-repeated action, 5 stacks |
| **Multistrike** | X% chance for the hit to echo again at 40% effectiveness (the echo counts as its own damage instance) |
| **Hatred** | +X% haste per consecutive hit on the same enemy (cap 20); switching targets resets the chain |
| **Sanguination** | Every hit: +X% damage, paid for with current health (X × 0.25 of current HP; can never kill you) |
| **Force** | +X% critical strike damage, added to the crit multiplier |
| **Synergy** | Each consecutive crit: Multistrike, Hatred, and Adaptability all +X% (cap 10 stacks); first non-crit spends them |
| **Frenzy** | 3 damage instances inside 1 s → +100% crit chance and +20% Force for 5 s, scaled by the stat (Frenzy 0.5 = half effect) — Multistrike echoes help trigger it |

The stats deliberately loop into each other: Multistrike feeds Frenzy's instance count, Frenzy feeds crits, crits feed Synergy, Synergy feeds Multistrike/Hatred/Adaptability — stacking several of them is where builds get absurd, per the exponential-power pillar.

## Scaling: Risk of Rain 2 (`RLCombatFormulas.h`)

Difficulty over time uses RoR2's actual stopwatch formulas:

- **Coefficient** = (PlayerFactor + minutes × 0.0506 × DifficultyValue) × 1.15^zonesCleared — time creeps it up, every cleared zone multiplies by 1.15, so lingering *and* rushing both cost you. DifficultyValue = 2 (Rainstorm).
- **Ambient level** = 1 + (coeff − 1) / 0.33, capped 99. Monsters gain **+30% HP and +20% damage per level** — RoR2's monster growth.
- **Directors** — two per zone like RoR2: a fast one (7.5–10 s waves) and a slow one (22.5–30 s, bigger waves). Income = multiplier × (1 + 0.4 × coeff) credits/sec, scaled by the zone's `DirectorCreditRate`. Each wave the director **commits to one weighted spawn card** and spends until broke.
- **Elites** — a wave promotes to tier-1 elites at **6× cost for 4× HP / 2× damage** (RoR2's numbers); the slow director promotes more eagerly.

## Other balance levers (all data-driven)

- **XP**: level N→N+1 costs `100 × 1.35^(N-1)`; kills/gathers scale up with depth so leveling never slogs.
- **The Wild God**: 25,000 HP, 120 base damage, 200 armor — bounce off until you're geared.

Numbers live in the CSVs under `/Data`, in `RLCombat` (`RLCombatFormulas.h`), and in `RLBalance` (`RLTypes.h`); tuning passes shouldn't need to touch system code.
