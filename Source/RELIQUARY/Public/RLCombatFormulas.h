// RELIQUARY — combat math.
//
// DAMAGE is an action-RPG pipeline (no auto attacks, no cast times) that
// keeps Classic WoW's bones where they still fit:
//   - Power by class, Classic ratios: Warrior AP = Str x2, Rogue AP =
//     Str + Agi, SpellPower = Intellect
//   - Every ability declares a PowerCoefficient, Risk of Rain 2 style
//     ("this skill hits for 300%"): Hit = Base + Coefficient x Power
//   - Armor DR = Armor / (Armor + 400 + 85 x AttackerLevel), capped 75%;
//     spells ignore armor (still pure Classic)
//   - Crit multiplier = CritDamage + Force; Agility feeds physical crit
//     (1% per 20), Intellect feeds spell crit (1% per 60)
//   - No miss/dodge/parry table: whiffing feels bad in an action game
//
// The proc suite layers on top (live state on URLAbilitySystemComponent):
//   - Adaptability: +X% damage per non-repeated action, 5 stacks
//   - Multistrike:  X% chance for the hit to echo at reduced effect
//   - Hatred:       +X% haste per consecutive hit on the same enemy
//   - Sanguination: pay % of current health, gain +X% damage per hit
//   - Force:        +X% critical strike damage
//   - Synergy:      consecutive crits raise Multistrike/Hatred/
//                   Adaptability by +X% each until a non-crit
//   - Frenzy:       3 damage instances inside 1s -> +100% crit and
//                   +20% Force for 5s, scaled by the Frenzy stat
//
// SCALING feels like Risk of Rain 2 (unchanged):
//   - DifficultyCoefficient = (PlayerFactor + minutes x 0.0506 x
//     DifficultyValue) x 1.15^stagesCompleted
//   - AmbientLevel = 1 + (coeff - PlayerFactor) / 0.33
//   - Monsters: +30% HP and +20% damage per ambient level above 1
//   - Tier-1 elites: 6x director cost for 4x HP / 2x damage

#pragma once

#include "CoreMinimal.h"

namespace RLCombat
{
	// --- Power (Classic WoW class ratios) ---

	/** Warrior: 2 AP per Strength. */
	FORCEINLINE float WarriorAttackPower(float Strength) { return Strength * 2.f; }

	/** Rogue: 1 AP per Strength + 1 AP per Agility. */
	FORCEINLINE float RogueAttackPower(float Strength, float Agility) { return Strength + Agility; }

	/** Everything else (enemies included): 2 AP per Strength. */
	FORCEINLINE float DefaultAttackPower(float Strength) { return Strength * 2.f; }

	/** RELIQUARY adaptation: Intellect doubles as spell power, 1:1. */
	FORCEINLINE float SpellPowerFromIntellect(float Intellect) { return Intellect; }

	/**
	 * The ARPG hit formula, RoR2 style: abilities are a coefficient on your
	 * power stat (primary ~0.8, big payoffs 2.5+). Base keeps level-1 hits
	 * from feeling like wet noodles before power comes online.
	 */
	FORCEINLINE float AbilityHitDamage(float BaseDamage, float PowerCoefficient, float Power)
	{
		return BaseDamage + PowerCoefficient * Power;
	}

	// --- Crit ---

	/** Physical: 1% crit per 20 Agility. */
	FORCEINLINE float MeleeCritFromAgility(float Agility) { return Agility / 2000.f; }

	/** Spells: 1% crit per 60 Intellect. */
	FORCEINLINE float SpellCritFromIntellect(float Intellect) { return Intellect / 6000.f; }

	// --- Armor (Classic WoW) ---

	/**
	 * Classic mitigation vs a same-or-higher-level attacker:
	 * DR = Armor / (Armor + 400 + 85 x AttackerLevel), hard-capped at 75%.
	 * Spells ignore armor and never pass through here.
	 */
	FORCEINLINE float ArmorDamageReduction(float Armor, float AttackerLevel)
	{
		const float ClampedArmor = FMath::Max(Armor, 0.f);
		const float Level = FMath::Max(AttackerLevel, 1.f);
		const float DR = ClampedArmor / (ClampedArmor + 400.f + 85.f * Level);
		return FMath::Min(DR, 0.75f);
	}

	// --- Proc suite tuning ---

	/** Multistrike echoes land at this fraction of the original hit. */
	constexpr float MultistrikeEffectiveness = 0.4f;

	/** Sanguination: blood paid per point of damage bonus.
	 *  At 0.25, +10% damage costs 2.5% of current health per hit. */
	constexpr float SanguinationBloodCostRatio = 0.25f;

	/** Hatred stacks cap (haste bonus = Hatred x stacks). */
	constexpr int32 HatredMaxStacks = 20;

	/** Synergy stacks cap (bonus = Synergy x stacks to each affected stat). */
	constexpr int32 SynergyMaxStacks = 10;

	/** Frenzy trigger: this many damage instances... */
	constexpr int32 FrenzyInstanceCount = 3;
	/** ...inside this window (seconds)... */
	constexpr float FrenzyWindowSeconds = 1.f;
	/** ...grants, scaled by the Frenzy stat, this much crit chance... */
	constexpr float FrenzyCritBonus = 1.f;
	/** ...and this much Force... */
	constexpr float FrenzyForceBonus = 0.2f;
	/** ...for this long. */
	constexpr float FrenzyDurationSeconds = 5.f;

	// --- Risk of Rain 2: difficulty & ambient level ---

	/** RoR2 difficulty value: 1 = Drizzle, 2 = Rainstorm, 3 = Monsoon. */
	constexpr float DifficultyValue = 2.f;

	/** Single-player: 1 + 0.3 x (playerCount - 1). */
	constexpr float PlayerFactor = 1.f;

	/**
	 * The RoR2 stopwatch formula:
	 * coeff = (PlayerFactor + minutes x 0.0506 x DifficultyValue x
	 *          playerCount^0.2) x 1.15^stagesCompleted
	 */
	FORCEINLINE float DifficultyCoefficient(float RunSeconds, int32 StagesCompleted)
	{
		const float Minutes = RunSeconds / 60.f;
		const float TimeFactor = Minutes * 0.0506f * DifficultyValue;	// playerCount^0.2 = 1
		const float StageFactor = FMath::Pow(1.15f, static_cast<float>(FMath::Max(StagesCompleted, 0)));
		return (PlayerFactor + TimeFactor) * StageFactor;
	}

	/** RoR2 ambient monster level: 1 + (coeff - playerFactor) / 0.33, cap 99. */
	FORCEINLINE float AmbientLevel(float Coefficient)
	{
		return FMath::Clamp(1.f + (Coefficient - PlayerFactor) / 0.33f, 1.f, 99.f);
	}

	/** RoR2 monsters gain +30% health per ambient level above 1. */
	FORCEINLINE float MonsterHealthMultiplier(float Level) { return 1.f + 0.3f * (Level - 1.f); }

	/** ...and +20% damage per ambient level above 1. */
	FORCEINLINE float MonsterDamageMultiplier(float Level) { return 1.f + 0.2f * (Level - 1.f); }

	// --- Risk of Rain 2: director & elites ---

	/** Director income: credits/second = multiplier x (1 + 0.4 x coeff). */
	FORCEINLINE float DirectorCreditsPerSecond(float CreditMultiplier, float Coefficient)
	{
		return CreditMultiplier * (1.f + 0.4f * Coefficient);
	}

	/** Tier-1 elite (RoR2): 6x the card's cost... */
	constexpr float EliteCostMultiplier = 6.f;
	/** ...for 4x health... */
	constexpr float EliteHealthMultiplier = 4.f;
	/** ...and 2x damage. */
	constexpr float EliteDamageMultiplier = 2.f;
}
