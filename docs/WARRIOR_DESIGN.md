# Warrior v0.1 — kit & talents

Design source: war.txt + talents/connections CSVs (2026-07). This doc tracks
what the design says, what is live in the build, and what is still stubbed.

## Kit

| Slot | Ability | Design | Status |
|---|---|---|---|
| Passive | **Battle Trance** | Below 75% HP: up to 30% damage reduction, 90% CDR, +100% healing received, +10% haste, +30% move speed — ramping exponentially from 1% of max effect at 74.99% HP toward full effect near 1% HP | **Not built.** Needs a passive GameplayEffect driven by health fraction; planned as C++ pass |
| Primary | **3-slash combo** (Mercenary-style) | 3rd hit +10% damage and applies a stacking debuff: +50% Secondary damage per stack | **Combo live** (`GA_WarriorPrimary`, montage chain + buffer + grace + cooldown). Set `FinisherMultiplier = 1.1` to match the +10% design. Expose debuff not built |
| Secondary | **Mortal Strike** | Slow heavy hit, longish CD; -10% healing received for 10s; +50% damage below 20% HP | **Not built** (slot currently GA_WarriorCleave placeholder) |
| Special | **Reckless Abandon** | Hold-to-charge line dash (Loader punch + WoW dragon charge): windup dig-in, big hitstun on first enemy, Crushed Armor (+30% damage taken, 45s), AoE + Demoralizing Shout (-30% damage dealt, 10s), self-recoil 5% current HP + brief slow, air-usable with reduced range | **Not built** (slot currently GA_WarriorAvatar placeholder) |
| Utility | **Heroic Leap** | Hold-to-aim leap, small AoE damage + 10% slow for 5s on landing; 10 kills reset the CD | **Not built** (slot currently GA_WarriorCharge placeholder) |

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
- Iron Strength "+10% of armor as strength" → flat `Strength +5` (no
  cross-stat scaling yet)
- Second Wind (stun on Secondary) → no-op until Mortal Strike exists
- Improved Battle Trance → no-op until the Battle Trance passive exists
- Bladestorm keystone → grants `/Game/Abilities/GA_Bladestorm` (create the BP
  when ready; missing assets are skipped gracefully)

### Editor checklist for this pass

1. Reimport **DT_Talents** after pulling (schema gained `Column`,
   `Prerequisites`, `bKeystone`).
2. Old saves hold ranks for deleted talent ids (e.g. `Jug_Might`) which
   still count as spent points — add a **Reset** button to WBP_Talents
   calling `ResetTalents`, or delete `Saved/SaveGames`.
3. Point WBP_Talents buttons at the new row names (`War_T1A` …). The
   `Tier`/`Column` fields are the grid coordinates for a future tree UI.
4. To test the Avatar keystone: set `GA_WarriorAvatar`'s ActionTag to
   `Ability.Utility` (per design it replaces Utility).
5. Set `GA_WarriorPrimary` FinisherMultiplier to 1.1 (design: 3rd hit +10%).
