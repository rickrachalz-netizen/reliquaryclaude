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

## 3. Reparent the existing Blueprints (required)

Open each asset → File → Reparent Blueprint:

- `Content/BP_RLGameInstance` → **`URLGameInstance`** (`RLGameInstance`)
- `Content/UI/WBP_RunHUD2` (or `WBP_RunHUD`) → **`URLRunHUDWidget`** — then bind its text/progress widgets to the `Get*` pure functions
- `Content/Resources/BP_ResourceNode`, `BP_Tree`, `BP_Stone` etc. → **`ARLResourceNode`** (set `MaterialItemId` per blueprint, e.g. `OakheartTimber` on trees, `Riverstone` on rocks)
- `Content/Resources/BP_ResourcePickup` → **`ARLResourcePickup`**
- `Content/Maps/BP_RunPortal` → **`ARLEmbarkPortal`**
- `Content/Maps/BP_ExtractPoint` → can be retired; extraction now flows through `ARLChallengeAltar`

## 4. World Settings per map (required)

- `L_Lobby` (base camp): GameMode Override → **`ARLBaseCampGameMode`** (autosaves on arrival). Place: an `ARLEmbarkPortal` (Destination=RealmPath), an `ARLCraftingStation`, optionally a second portal with Destination=WildGod.
- `L_Run` (zone shell): GameMode Override → **`ARLRunGameMode`**. Place one **`ARLZoneScatterVolume`** stretched over the playable area — it scatters resource nodes, upgrade altars, the challenge altar, and banking crates from the run seed. Add a NavMeshBoundsVolume so the director can place spawns.
- Both game modes should use your character BP (reparent of `ARELIQUARYCharacter`) as Default Pawn — set that on the GameMode BPs or subclass in BP.

## 5. Input (required for the kit)

Add Input Actions (`IA_Primary`, `IA_Secondary`, `IA_Utility`, `IA_Special`, `IA_Interact`) to the existing Input Mapping Context (suggested: LMB, RMB, LShift, R, E — RoR2 muscle memory), then assign them on the character Blueprint's *Input|Abilities* properties.

## 6. Enemy Blueprints (content)

Create Blueprints under **`/Game/Enemies`** parented to **`ARLEnemyBase`**, named to match `Data/SpawnCards.csv`: `BP_Wolf`, `BP_Goblin`, `BP_GoblinPack`, `BP_Orc`, `BP_Necromancer`, `BP_Skeleton`, `BP_DuskWolf`, `BP_StormWisp`, plus bosses `BP_BossAlphaWolf`, `BP_BossOrcWarlord`, `BP_BossBoneTyrant`, `BP_BossGodspawn`. Assign meshes from `Content/Fab` (wolf, goblin, orc, skeleton necromancer are already in the project) and tune `BaseHealth`/`BaseDamage`. Enemies chase the hero and strike in melee out of the box — `ARLEnemyBase` self-possesses an AI controller and runs a built-in chase (navmesh pathing when available, straight-line otherwise). Tune `AggroRange`/`AttackRange`/`AttackInterval` per Blueprint, implement `OnTouchAttack` for swing montages/VFX, or untick `bChaseHero` to drive movement with your own StateTree/BT instead. The Wild God arena needs a map (`L_WildGodArena`) containing a Blueprint of **`ARLWildGodBoss`**.

Until these exist the director quietly spawns nothing — the loop still runs (gather → altar → extract).

## 7. Ability Blueprints (content)

Warrior/Rogue primaries and the Mage bolt work natively out of the box. The remaining kit slots referenced by `Data/Classes.csv` (`/Game/Abilities/GA_*`) are Blueprints parented to **`URLGameplayAbility`** (or the melee/projectile subclasses): set `ActionTag` to `Ability.Secondary` / `Ability.Utility` / `Ability.Special`, add montage + VFX, and call `ApplyDamageToTarget` on hit. Missing ones are skipped gracefully.

## 8. UI flows (content)

- **Altar choice**: on `ARLChallengeAltar.OnChoiceRequested`, show two buttons calling `ChooseExtract` / `ChooseContinue`.
- **Boon choice**: on `ARLUpgradeAltar.OnBoonsOffered`, show the three boon rows (`URLDataSubsystem` lookups) and call `PurchaseBoon(PlayerPawn, Index)`.
- **Crafting**: on `ARLCraftingStation.OnCraftingOpened`, list `URLCraftingLibrary::GetCraftableRecipes` and call `Craft`.
- **Hero creation** (main menu): call `URLGameInstance::CreateHero(Name, Class, SpecId)` then open `L_Lobby`.
- **Talents/Essences** (base camp): `SpendTalentPoint`, `SocketEssence`, `UpgradeEssence` on the game instance; call `RefreshHeroBuild` on the character afterwards.

## Smoke test

PIE in `L_Run`: a run auto-embarks in place (seeded), nodes and altars scatter, primary attack shatters trees into pickups, excess mana accrues, an upgrade altar sells a boon, the challenge altar charges (boss skipped if no enemy BPs yet), and choosing *Extract* returns to `L_Lobby`, banks the haul, and autosaves.
