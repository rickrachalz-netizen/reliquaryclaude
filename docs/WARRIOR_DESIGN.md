# Warrior v0.1 — kit & talents

Design source: war.txt + talents/connections CSVs (2026-07). This doc tracks
what the design says, what is live in the build, and what is still stubbed.

## Kit

| Slot | Ability | Design | Status |
|---|---|---|---|
| Passive | **Battle Trance** | Below 75% HP: up to 30% damage reduction, 90% CDR, +100% healing received, +10% haste, +30% move speed — ramping exponentially from 1% of max effect at 74.99% HP toward full effect near 1% HP | **LIVE** (`ARELIQUARYCharacter::GetBattleTranceIntensity`, warrior heroes only). DR hooks GAS damage intake; healing bonus boosts regen; CDR+haste compress hasted cooldowns; speed bonus in Tick. `GetBattleTranceIntensity` is BlueprintPure — drive a vignette/glow from it |
| Primary | **3-slash combo** (Mercenary-style) | 3rd hit +10% damage and applies a stacking debuff: +50% Secondary damage per stack | **LIVE incl. Expose**: the finisher now applies a stacking `State.Exposed` tag that Mortal Strike consumes (+50% each). Set `FinisherMultiplier = 1.1` on GA_WarriorPrimary to match the +10% design |
| Secondary | **Mortal Strike** | Slow heavy hit, longish CD; -10% healing received for 10s; +50% damage below 20% HP | **LIVE** (`RLMortalStrikeAbility`, wired into Classes.csv). Executes below 20%, consumes Exposed, applies `State.MortalWounds` for 10s (healing hookup pending a healing system), stuns 2s with War_T3A. Optional: BP subclass with one montage in ComboMontages for the swing anim |
| Special | **Reckless Abandon** | Hold-to-charge line dash (Loader punch + WoW dragon charge): windup dig-in, big hitstun on first enemy, Crushed Armor (+30% damage taken, 45s), AoE + Demoralizing Shout (-30% damage dealt, 10s), self-recoil 5% current HP + brief slow, air-usable with reduced range | **LIVE, Loader-feel** (`RLRecklessAbandonAbility`, wired into Classes.csv). Hold Special to charge (feet plant while grounded), release to launch along the **full 3D camera aim** — aim up and it throws you airborne; held time scales distance (600→2400) and damage (40%→100%). Walkable ground (normal Z ≥ 0.7) never stops it: the dash skims flats and rides up ramps; a vertical dive into flat ground is a soft landing. Objects do stop it: enemy hit = full package (stun, Crushed Armor 45s via `State.CrushedArmor` +30% in the damage execution, AoE 600 + `State.Demoralized` -30% dealt for 10s); wall/tree hit = direct damage to destructibles + recoil only. Recoil on enemy/object impact = 5% *current* HP (unmitigated) + 10% slow 1.5s + a small bounce up-and-backward (350/400). A clean whiff is free and **keeps its momentum** — the dash velocity hands off to gravity for the fling. From the ground, downward aim flattens to horizontal; midair dives are allowed and range is ×0.6. CD 12s hasted. BP hooks: OnChargeStarted/Tick, OnDashStarted, OnChargeImpact + `GetChargeAlpha()` for HUD |
| Utility | **Heroic Leap** | Hold-to-aim leap, small AoE damage + 10% slow for 5s on landing; 10 kills reset the CD | **Not built** — same reason (slot currently GA_WarriorCharge placeholder) |

## Talent tree (graph-gated)

Nodes unlock when ANY connected earlier node holds a rank (data:
`Prerequisites` column in `Data/Talents.csv`; rule: `URLGameInstance::SpendTalentPoint`).
Keystones replace kit abilities by sharing the slot's ActionTag — the newest
granted ability wins the slot (`URLAbilitySystemComponent::TryActivateByActionTag`).

```
Tier 1:   Killer Instinct     Swift Advance      Heavy Armor
              |    \           /   |   \           /    |
Tier 2:   Brutal Strikes      Vitality          Battle Tempo
              |    \           /   |   \           /    |
Tier 3:   Second Wind        Regeneration      Iron Strength
              |                    |                    |
Keystone: Avatar             Bladestorm       Improved Battle Trance
```

### Design → data conversions (v0.1 approximations)

The stat system applies flat additive values, so percent designs are
converted; revisit when percent modifiers exist:

- Swift Advance 5%/rank → `MoveSpeed +30`/rank (5% of base 600)
- Heavy Armor 5%/rank → flat `Armor +5`/rank (percent of base 20 was negligible)
- Vitality +30% HP → flat `Health +60` (~30% of early-game pool)
- Battle Tempo -10% CD → `Haste +0.1` (cooldowns divide by 1+haste)
- Regeneration +20% → flat `HealthRegen +0.5`
- Iron Strength "+10% of armor as strength" → **real** (RebuildHero converts
  base+talent armor; gear armor lands after the conversion for now)
- Second Wind → **real**: Mortal Strike stuns 2s when War_T3A is owned
- Improved Battle Trance → **real**: War_K3 switches the trance to the
  90% threshold / stronger maximums
- Bladestorm keystone → **class ready** (`RLBladestormAbility`): create
  `/Game/Abilities/GA_Bladestorm` parented to it, assign `AM_Bladestorm`
  as SpinMontage. 3 damage ticks over 3s; final tick grants
  `State.BladestormEmpowered`, consumed by Mortal Strike for +150%

### Editor checklist for this pass

1. Reimport **DT_Talents** (schema gained `Column`, `Prerequisites`,
   `bKeystone`) and **DT_Classes** (Warrior Secondary is now Mortal Strike).
2. Old saves hold ranks for deleted talent ids (e.g. `Jug_Might`) which
   still count as spent points — add a **Reset** button to WBP_Talents
   calling `ResetTalents`, or delete `Saved/SaveGames`.
3. Point WBP_Talents buttons at the new row names (`War_T1A` …). The
   `Tier`/`Column` fields are the grid coordinates for a future tree UI.
4. To test the Avatar keystone: set `GA_WarriorAvatar`'s ActionTag to
   `Ability.Utility` (per design it replaces Utility).
5. Set `GA_WarriorPrimary` FinisherMultiplier to 1.1 (design: 3rd hit +10%).
6. Create `GA_Bladestorm` in `/Game/Abilities` parented to
   `RLBladestormAbility`; set SpinMontage to `AM_Bladestorm`.
7. Optional: `GA_MortalStrike` parented to `RLMortalStrikeAbility` with a
   heavy-swing montage in ComboMontages (then point the Warrior row of
   DT_Classes at it instead of the native class).
8. Battle Trance testing: take hits until below 75% HP — regen visibly
   accelerates, speed rises, cooldowns shrink as health drops. `showdebug
   abilitysystem` lists the live tags (Exposed/MortalWounds/Empowered).
9. Reckless Abandon: reimport **DT_Classes** (Warrior Special now points at
   the native `RLRecklessAbandonAbility`). The kit inputs now forward
   button releases too — no editor change needed as long as IA_Special has
   no trigger that suppresses `Completed` (default Down trigger is fine).
10. Reckless Abandon testing: tap vs full 1s hold (short/weak vs 2400cm
    full-damage dash), use midair (shorter), charge into an enemy
    (`showdebug abilitysystem` on it shows `State.CrushedArmor`; nearby
    enemies gain `State.Demoralized`), charge into a tree (it shatters,
    you bounce back and recoil), whiff into open air (no recoil). Optional
    polish: BP subclass `GA_RecklessAbandon` with windup/dash montages +
    VFX on the OnCharge*/OnDash* hooks, then point DT_Classes at it.
11. Loader-feel testing: aim ~45° up and full-charge — you launch along
    the aim and keep sailing in an arc when the distance runs out
    (momentum carry). Dash at a ramp from the ground — it rides up the
    slope instead of bonking. Midair, dive steeply at open ground — soft
    landing, no recoil. Punch an enemy/tree — hit-stop, then a visible
    pop up-and-backward. Tunables live on the ability: MomentumCarry,
    Skim*, ImpactBounce*.
