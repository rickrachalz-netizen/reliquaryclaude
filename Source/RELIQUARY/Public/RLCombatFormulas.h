// RELIQUARY — combat math, borrowed from the two games that did it best.
//
// DAMAGE feels like Classic World of Warcraft:
//   - Attack Power by class (Warrior Str x2, Rogue Str+Agi), AP/14 per
//     weapon-speed second on every swing
//   - Spell damage = base + SpellPower x (CastTime / 3.5), instants at the
//     1.5s floor coefficient
//   - Melee crits x2.0, spell crits x1.5; Agility feeds melee crit
//     (1% per 20), Intellect feeds spell crit (1% per 60)
//   - Armor DR = Armor / (Armor + 400 + 85 x AttackerLevel), capped 75%;
//     spells ignore armor entirely
//   - No miss/dodge/parry table: whiffing feels bad in an action game
//
// SCALING feels like Risk of Rain 2:
//   - DifficultyCoefficient = (PlayerFactor + minutes x 0.0506 x
//     DifficultyValue) x 1.15^stagesCompleted
//   - AmbientLevel = 1 + (coeff - PlayerFactor) / 0.33
//   - Monsters: +30% HP and +20% damage per ambient level above 1
//   - Tier-1 elites: 6x director cost for 4x HP / 2x damage

#pragma once

#include "CoreMinimal.h"

namespace RLCombat
{
	// --- Classic WoW: attack power & weapon damage ---

	/** DPS granted per point of Attack Power (Classic: 14 AP = 1 DPS). */
	constexpr float APPerDPS = 14.f;

	/** Warrior: 2 AP per Strength. */
	FORCEINLINE float WarriorAttackPower(float Strength) { return Strength * 2.f; }

	/** Rogue: 1 AP per Strength + 1 AP per Agility. */
	FORCEINLINE float RogueAttackPower(float Strength, float Agility) { return Strength + Agility; }

	/** Everything else (enemies included): 2 AP per Strength. */
	FORCEINLINE float DefaultAttackPower(float Strength) { return Strength * 2.f; }

	/** Classic swing: WeaponDamage + AP/14 x WeaponSpeed. */
	FORCEINLINE float MeleeSwingDamage(float WeaponDamage, float AttackPower, float WeaponSpeed)
	{
		return WeaponDamage + (AttackPower / APPerDPS) * WeaponSpeed;
	}

	// --- Classic WoW: spells ---

	/** RELIQUARY adaptation: Intellect doubles as spell power, 1:1. */
	FORCEINLINE float SpellPowerFromIntellect(float Intellect) { return Intellect; }

	/** Classic coefficient: CastTime / 3.5, instants at the 1.5s floor. */
	FORCEINLINE float SpellCoefficient(float CastTimeSeconds)
	{
		return FMath::Clamp(CastTimeSeconds, 1.5f, 3.5f) / 3.5f;
	}

	FORCEINLINE float SpellHitDamage(float BaseDamage, float SpellPower, float CastTimeSeconds)
	{
		return BaseDamage + SpellPower * SpellCoefficient(CastTimeSeconds);
	}

	// --- Classic WoW: crit ---

	/** Melee: 1% crit per 20 Agility. */
	FORCEINLINE float MeleeCritFromAgility(float Agility) { return Agility / 2000.f; }

	/** Spells: 1% crit per 60 Intellect. */
	FORCEINLINE float SpellCritFromIntellect(float Intellect) { return Intellect / 6000.f; }

	/** Classic: melee crits deal double... */
	constexpr float MeleeCritMultiplier = 2.0f;
	/** ...but spell crits only deal 1.5x. */
	constexpr float SpellCritMultiplier = 1.5f;

	// --- Classic WoW: armor ---

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
