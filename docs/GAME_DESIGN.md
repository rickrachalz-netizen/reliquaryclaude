# RELIQUARY — Game Design & Code Map

> A roguelike-extraction action RPG about scavenging a beautiful, hostile world — and growing strong enough to escape it.

This document maps every system from the production map to its implementation. Gameplay inspiration: **Risk of Rain 2** (moment-to-moment feel, difficulty scaling, director, teleporter ritual), **Helldivers 2** (extraction stakes, banking under pressure), **V Rising** (gather → craft → grow permanently stronger), **World of Warcraft: Battle for Azeroth 8.3** (specs, talents, Heart of Azeroth-style essence system).

## The Core Loop

1. **Embark** from base camp (the corpse of a fallen Wild God) — `ARLEmbarkPortal` → `URLRunManagerSubsystem::EmbarkOnRun()`. The run seed is rolled and locked here; no map fishing.
2. **Push your luck** — difficulty creeps up with time and jumps per zone (`RLBalance::DifficultyCoefficient`), while the world feeds you excess mana and materials.
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
| Heart of Azeroth-style essences | Reliquary Shard: `FRLSocketedEssence`, `Data/Essences.csv`, major slot at 10, minors at 18/26/30. Enemy-sourced essences unlock on the first kill of that type (`ARLEnemyBase::EnemyTypeId` → `ARLEssenceShardPickup` → `URLGameInstance::UnlockEssence`); major slot grants an active on a dedicated 5th input (`Ability.Essence`, Q), any slot grants the ranked passive. Socketed from the character panel (`URLCharacterPanelWidget`). |
| Fun, crunchy combat | GAS pipeline: `URLGameplayAbility` → `URLDamageEffect` → `URLDamageExecution` (stat scale, crit, Adaptability, armor) |
| Destructible world → resources | `ARLResourceNode::TakeDamage` — abilities damage non-GAS blockers as world objects |
| Ten interconnected levels | `Data/Zones.csv` (Verdant Threshold → The Last Door), traveled by `URLRunManagerSubsystem` |
| Challenge altars | `ARLChallengeAltar`: activate → empowered boss + charge radius → extract or teleport onward |
| Banking every 3 maps | `URLRunManagerSubsystem::ShouldOfferBankingCrate` → `ARLBankingCrate` |
| Simple, lethal enemies | `ARLEnemyBase` (difficulty-scaled), elites drop Manalith Shards |
| Enemies with personality (RoR2) | `ARLEnemyGroup` coordinators steer pack members by pinning orders on the base brain: `ARLWolfPack` roams in formation, surrounds the hero, and cycles one lunging biter at a time; `ARLGoblinGang` circles up at camp, roams, merges with other gangs, and picks fight/hunt/flee purely by headcount (1 flees, 2-3 fight leashed to camp, 4+ hunt, last survivor runs) |
| Enemy spawning | `ARLEnemyDirector` — RoR2-style credits, spawn cards in `Data/SpawnCards.csv`; pack cards (`GroupClass`, `PackSizeMin/Max`) roll a headcount, cost per member, and the director banks credits toward packs it can't yet afford |
| Death forfeits resources | `URLRunManagerSubsystem::HandleHeroDeath` (banked crates stay safe) |
| Autosave at base camp only, locked RNG | `ARLBaseCampGameMode::BeginPlay` saves; seed locked at embark |
| Excess mana + altar choices | `URLRunManagerSubsystem` currency, dropped as physical magnetized orbs (`ARLManaOrbPickup`) from nodes and kills, totals scaled by the difficulty coefficient (elite ×2, boss ×4; `RLBalance` tuning). `ARLUpgradeAltar` offers 3 seeded boons from `Data/Boons.csv`, priced by `GetOfferedPrice` (base cost × difficulty at roll time). |
| Temporary by design | `URLRunPowerComponent`; everything resets in `URLRunManagerSubsystem::ResetRunState` |
| Crafting is the heart | `Data/Items.csv` + `Data/Recipes.csv` + `URLCraftingLibrary` + `ARLCraftingStation` |
| Distinct materials, planned routes | No generic "wood": Oakheart, Ironwood, Feywood... each zone lists its reliable spawns |
| Gear: cantrips, set bonuses | `FRLItemRow::CantripEffect`, `Data/SetBonuses.csv`, applied by `URLEquipmentComponent` |
| Semi-procedural maps | `ARLZoneScatterVolume` — seeded scatter of nodes/altars/crates per zone |
| Minimalist HUD | `URLRunHUDWidget` — timer, location, vitals, mana, resources, danger label |
| Run win condition | Extract at any charged altar; zone 10's boss ends the run victorious |
| Game win condition | `ARLWildGodBoss` — challengeable at any time via portal, tuned for geared level 30 |

## Balance levers (all data-driven)

- **Difficulty**: +8%/minute, +20%/zone (`RLBalance::DifficultyCoefficient`). Enemy HP scales linearly with the coefficient, damage with its square root.
- **XP**: level N→N+1 costs `100 × 1.35^(N-1)`; kills/gathers scale up with depth so leveling never slogs.
- **Elites**: promotion chance grows with difficulty, capped at 30%; cost 3× on the director's wallet.
- **The Wild God**: 25,000 HP, 120 base damage, 200 armor — bounce off until you're geared.

Numbers live in the CSVs under `/Data` and in `RLBalance` (`RLTypes.h`); tuning passes shouldn't need to touch system code.
