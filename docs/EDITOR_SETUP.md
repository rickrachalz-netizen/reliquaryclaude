# RELIQUARY — Editor Wiring Guide

The C++ framework is complete and self-contained; this checklist connects it to content in the Unreal Editor (UE 5.7). Do these in order — the first three make the game loop functional, the rest add content and polish.

## 1. Build the project

Right-click `RELIQUARY.uproject` → *Generate Visual Studio project files*, then build the `RELIQUARYEditor` target (or just open the .uproject and accept the compile prompt).

## 2. Import the DataTables (required)

For each CSV in `/Data`, in the Content Browser under a new folder **`/Game/Data`**:

*Right-click → Miscellaneous → Data Table*, pick the row struct, name it exactly as below, then open it and use **Reimport / Import CSV** pointing at the file (or drag the CSV in and choose the struct):

| Asset name | Row struct | Source CSV |
|---|---|---|
| `DT_Items` | `RLItemRow` | `Data/Items.csv` |
| `DT_SetBonuses` | `RLSetBonusRow` | `Data/SetBonuses.csv` |
| `DT_Recipes` | `RLRecipeRow` | `Data/Recipes.csv` |
| `DT_Boons` | `RLBoonRow` | `Data/Boons.csv` |
| `DT_Zones` | `RLZoneRow` | `Data/Zones.csv` |
| `DT_SpawnCards` | `RLSpawnCardRow` | `Data/SpawnCards.csv` |
| `DT_Classes` | `RLClassRow` | `Data/Classes.csv` |
| `DT_Specs` | `RLSpecRow` | `Data/Specs.csv` |
| `DT_Talents` | `RLTalentRow` | `Data/Talents.csv` |
| `DT_Essences` | `RLEssenceRow` | `Data/Essences.csv` |

Paths are configured in `Config/DefaultGame.ini` under `[/Script/RELIQUARY.RLDataSubsystem]` if you prefer different locations.

> **Schema note:** `Data/Essences.csv` gained a **`SourceEnemyId`** column (the spawn-card row name whose first kill drops that essence's shard) and now carries 12 enemy-themed essences alongside the 6 realm ones. If `DT_Essences` already exists, **reimport it** so the new column and rows land.

## 3. Reparent the existing Blueprints (required)

Open each asset → File → Reparent Blueprint:

- `Content/BP_RLGameInstance` → **`URLGameInstance`** (`RLGameInstance`)
- `Content/UI/WBP_RunHUD2` (or `WBP_RunHUD`) → **`URLRunHUDWidget`** — then bind its text/progress widgets to the `Get*` pure functions
- `Content/Resources/BP_ResourceNode`, `BP_Tree`, `BP_Stone` etc. → **`ARLResourceNode`** (set `MaterialItemId` per blueprint — for the vertical slice: `OakheartTimber` on trees, `DuskironOre` on rocks — and make sure `PickupClass` points at your pickup BP; a node with either unset shatters without dropping anything and logs a warning). Scatter-spawned nodes get `MaterialItemId` re-stamped from the volume's `ResourceScatter` entry, so the BP default mainly covers hand-placed copies.
  - ⚠️ **Remove any pre-framework damage logic from these Blueprints.** After reparenting, C++ owns node health, yield, and lifetime. A leftover `Event AnyDamage` chain with its own hit counters and `DestroyActor` runs *in parallel* with the C++ node and destroys the actor before it can shatter — eating the drops (the node logs "destroyed without shattering" when this happens). Blueprints should implement only **`OnChipped`** (wobble/chip feedback) and **`OnShattered`** (fall animation, stump/debris spawns) — with **no DestroyActor**; the node removes itself `ShatterLifeSpan` seconds after shattering.
- `Content/Resources/BP_ResourcePickup` → **`ARLResourcePickup`**
- `Content/Resources/BP_ManaOrb` → **`ARLManaOrbPickup`** (create it at exactly this path — every mana burst in the game auto-uses it; give it a small mesh/VFX with the mesh's collision off, the overlap sphere is the root. Without it, orbs are collected but invisible.)
- `Content/Maps/BP_RunPortal` → **`ARLEmbarkPortal`**
- `Content/Maps/BP_ExtractPoint` → can be retired; extraction now flows through `ARLChallengeAltar`

## 4. World Settings per map (required)

- `L_Lobby` (base camp): GameMode Override → **`ARLBaseCampGameMode`** (autosaves on arrival). Place: an `ARLEmbarkPortal` (Destination=RealmPath), an `ARLCraftingStation`, optionally a second portal with Destination=WildGod.
- `L_Run` (zone shell): GameMode Override → **`ARLRunGameMode`**. Place one **`ARLZoneScatterVolume`** stretched over the playable area — it scatters resource nodes, upgrade altars, the challenge altar, and banking crates from the run seed. Add a NavMeshBoundsVolume so the director can place spawns. Resource spawning is authored on the volume's **`ResourceScatter`** array: each entry names a material, the node Blueprint to spawn (`BP_Tree`/`BP_Stone`), a distribution (Clustered for forests, Uniform for ore), and min/max counts. Don't leave foliage-painted stand-ins in the map — foliage instances aren't actors, so they can't be damaged and never drop materials.
- Both game modes should use your character BP (reparent of `ARELIQUARYCharacter`) as Default Pawn — set that on the GameMode BPs or subclass in BP.

## 5. Input (required for the kit)

Add Input Actions (`IA_Primary`, `IA_Secondary`, `IA_Utility`, `IA_Special`, `IA_Interact`) to the existing Input Mapping Context (suggested: LMB, RMB, LShift, R, E — RoR2 muscle memory), then assign them on the character Blueprint's *Input|Abilities* properties.

Also add three UI actions:

| Input Action | Key(s) | Assign on character BP | Notes |
|---|---|---|---|
| `IA_EssenceAbility` | Q | *Input\|Abilities → Essence Ability Action* | Fires the major essence's active (the 5th kit slot). |
| `IA_ToggleCharacterPanel` | C | *Input\|UI → Toggle Character Panel Action* | Opens/closes the character sheet. |
| `IA_PauseMenu` | Esc **and** P | *Input\|UI → Pause Menu Action* | **In the IA asset, tick "Trigger When Paused"** or the key can't unpause. Map **both** Esc (works in a packaged build) and P (Esc ends a PIE session, so use P to test the menu in-editor). |

All three are `UInputAction` assets added to the same Input Mapping Context. Each key press is `Started`.

## 6. Enemy Blueprints (content)

Create Blueprints under **`/Game/Enemies`** parented to **`ARLEnemyBase`**, named to match `Data/SpawnCards.csv`: `BP_Wolf`, `BP_Goblin`, `BP_GoblinPack`, `BP_Orc`, `BP_Necromancer`, `BP_Skeleton`, `BP_DuskWolf`, `BP_StormWisp`, plus bosses `BP_BossAlphaWolf`, `BP_BossOrcWarlord`, `BP_BossBoneTyrant`, `BP_BossGodspawn`. Assign meshes from `Content/Fab` (wolf, goblin, orc, skeleton necromancer are already in the project) and tune `BaseHealth`/`BaseDamage`. Enemies chase the hero and strike in melee out of the box — `ARLEnemyBase` self-possesses an AI controller and runs a built-in chase (navmesh pathing when available, straight-line otherwise). Tune `AggroRange`/`AttackRange`/`AttackInterval` per Blueprint, implement `OnTouchAttack` for swing montages/VFX, or untick `bChaseHero` to drive movement with your own StateTree/BT instead. The Wild God arena needs a map (`L_WildGodArena`) containing a Blueprint of **`ARLWildGodBoss`**.

> **Essence drops:** enemies spawned by the director or a challenge altar get their `EnemyTypeId` stamped automatically (from the spawn-card row name), so first-kill essence shards work with no extra setup. Only **hand-placed** enemy Blueprints need `EnemyTypeId` set in their defaults (e.g. `Wolf`) to drop an essence.

Until these exist the director quietly spawns nothing — the loop still runs (gather → altar → extract).

## 7. Ability Blueprints (content)

Warrior/Rogue primaries and the Mage bolt work natively out of the box. To upgrade a melee primary into an animated combo, make a Blueprint child of `URLMeleeAttackAbility` (e.g. `GA_WarriorPrimary`), fill its **Combo Montages** array, and drop an **RL Melee Hit** anim notify on each montage's impact frame — damage then fires on the notify, re-pressing the input buffers the next stage, and hit-stop/knockback/shake/sound trigger on contact (all tunable in the **Feel** category). The remaining kit slots referenced by `Data/Classes.csv` (`/Game/Abilities/GA_*`) are Blueprints parented to **`URLGameplayAbility`** (or the melee/projectile subclasses): set `ActionTag` to `Ability.Secondary` / `Ability.Utility` / `Ability.Special`, add montage + VFX, and call `ApplyDamageToTarget` on hit. Missing ones are skipped gracefully.

## 8. UI flows (content)

- **Altar choice**: on `ARLChallengeAltar.OnChoiceRequested`, show two buttons calling `ChooseExtract` / `ChooseContinue`.
- **Boon choice**: on `ARLUpgradeAltar.OnBoonsOffered`, show the three boon rows (`URLDataSubsystem` lookups) and call `PurchaseBoon(PlayerPawn, Index)`. Show the price from **`GetOfferedPrice(Index)`** — it is the base cost scaled by the run's difficulty, which is what `PurchaseBoon` actually charges.
- **Crafting**: on `ARLCraftingStation.OnCraftingOpened`, list `URLCraftingLibrary::GetCraftableRecipes` and call `Craft`.
- **Hero creation** (main menu): call `URLGameInstance::CreateHero(Name, Class, SpecId)` then open `L_Lobby`.
- **Talents** (base camp): `SpendTalentPoint` on the game instance; call `RefreshHeroBuild` on the character afterwards.
- **Essence unlock toast** (optional): bind `URLGameInstance.OnEssenceUnlocked` to pop a "New Essence" notification. Essences are learned by picking up the shard a first-of-its-type kill drops — no altar needed.

## 8b. Built-in HUD/menu widgets (mostly zero-setup)

These new widgets all build their own tree in C++, so they work before any WBP exists. Reparent a WBP to the class to restyle (bind the optionally-bound sub-widgets by name).

- **Ability icon bar** (`URLAbilityBarWidget`): RoR2-style five-slot bar (Primary/Secondary/Utility/Special/Essence) with a radial clock-dial cooldown sweep and seconds remaining. Add it once: open `WBP_RunHUD`, drag a **Create Widget** of class `RLAbilityBarWidget` into the layout (bottom-center), or place it directly if `WBP_RunHUD` is a Widget BP you can add children to. Give each `URLGameplayAbility` an `AbilityIcon` texture to replace the fallback keybind tile. The Essence slot stays hidden until a major essence is socketed.
- **Character panel** (`URLCharacterPanelWidget`): press **C**. Left column lists every live stat; right column is the essence loadout — socket into Major / a free Minor, unsocket, and upgrade, all from here (works mid-run). No wiring needed beyond the `IA_ToggleCharacterPanel` input.
- **Pause menu** (`URLPauseMenuWidget`): press **Esc** (or **P** in PIE). Resume / Return to Main Menu / Exit to Desktop. No wiring beyond `IA_PauseMenu` (remember "Trigger When Paused").

## 9. Feel pass: torso aim + enemy health bars

- **Torso follows the camera** (hero): reparent the hero's Animation Blueprint
  to **`URLHeroAnimInstance`** (Class Settings → Parent Class). In the
  AnimGraph, between the state machine's output and the Output Pose, chain a
  **Transform (Modify) Bone** node per spine bone (`spine_01`, `spine_02`,
  `spine_03`), Rotation Mode **Add to Existing**, Rotation Space **Component
  Space**, and feed each one `TorsoYaw / 3` (as Roll or Yaw depending on the
  skeleton's bone axes — flip until the twist looks right) plus
  `TorsoPitch / 3`. Now strafing left while aiming right swings the weapon
  toward the crosshair. Clamp/smoothing tunables (`MaxTorsoYaw`,
  `AimInterpSpeed`) live on the anim instance class defaults.
- **Enemy health bars** work with zero setup: every `ARLEnemyBase` shows a
  bar above its head when damaged and hides it `HealthBarLingerSeconds` (5s)
  after the last hit — elites violet, bosses orange. To restyle, make a WBP
  reparented to **`URLEnemyHealthBarWidget`** containing a ProgressBar named
  `Bar`, and set it as `HealthBarWidgetClass` on the enemy Blueprint.

## Smoke test

PIE in `L_Run`: a run auto-embarks in place (seeded), nodes and altars scatter, primary attack shatters trees into pickups, excess mana accrues, an upgrade altar sells a boon, the challenge altar charges (boss skipped if no enemy BPs yet), and choosing *Extract* returns to `L_Lobby`, banks the haul, and autosaves.
