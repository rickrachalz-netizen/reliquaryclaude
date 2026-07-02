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

## Damage: Classic World of Warcraft (`RLCombatFormulas.h`)

The number pipeline is lifted from Classic WoW so damage *feels* like WoW:

- **Attack Power by class** — Warrior AP = Strength × 2; Rogue AP = Strength + Agility; enemies default to Strength × 2.
- **Melee swing** = WeaponDamage + (AP / 14) × WeaponSpeed — the classic 14-AP-per-DPS normalization. Every physical ability declares a `WeaponSpeed`.
- **Spells** = Base + SpellPower × (CastTime / 3.5), instants at the 1.5 s floor coefficient (≈0.43). SpellPower = Intellect (RELIQUARY adaptation, since Classic sourced it from gear only). Spells **ignore armor**.
- **Crit** — melee: 5% base + gear + 1% per 20 Agility, crits ×2.0. Spells: 5% base + gear + 1% per 60 Intellect, crits ×1.5 (the classic asymmetry).
- **Armor** = Armor / (Armor + 400 + 85 × AttackerLevel), capped 75% — deep-run monsters partially shred your armor via the level term, just like fighting higher-level mobs.
- **No attack table** — no miss/dodge/parry/glancing; whiffing feels bad in an action game. **Adaptability** (our signature stat) multiplies either school after the WoW math: ×(1 + X% per non-repeated action, 5 stacks).

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
