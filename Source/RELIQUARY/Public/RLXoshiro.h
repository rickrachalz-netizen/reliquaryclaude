// RELIQUARY — xoshiro256++ 1.0 PRNG (Blackman & Vigna, public domain
// reference implementation) with SplitMix64 seeding.
//
// Replaces FRandomStream everywhere gameplay randomness matters: 256-bit
// state, high statistical quality, and identical sequences for identical
// seeds on every platform — one 64-bit seed per system (zone layout,
// altar offers, spawn cards, crits) yields decorrelated streams.

#pragma once

#include "CoreMinimal.h"

struct FRLXoshiro256
{
	uint64 State[4];

	FRLXoshiro256() { SeedFrom(0x52454C4951554152ull); }
	explicit FRLXoshiro256(uint64 Seed) { SeedFrom(Seed); }

	/** SplitMix64: expands one 64-bit seed into well-mixed state words. */
	static uint64 SplitMix64(uint64& X)
	{
		X += 0x9E3779B97F4A7C15ull;
		uint64 Z = X;
		Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
		Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
		return Z ^ (Z >> 31);
	}

	void SeedFrom(uint64 Seed)
	{
		State[0] = SplitMix64(Seed);
		State[1] = SplitMix64(Seed);
		State[2] = SplitMix64(Seed);
		State[3] = SplitMix64(Seed);
	}

	static FORCEINLINE uint64 RotL(uint64 Value, int32 Bits)
	{
		return (Value << Bits) | (Value >> (64 - Bits));
	}

	uint64 NextUInt64()
	{
		const uint64 Result = RotL(State[0] + State[3], 23) + State[0];
		const uint64 T = State[1] << 17;
		State[2] ^= State[0];
		State[3] ^= State[1];
		State[1] ^= State[2];
		State[0] ^= State[3];
		State[2] ^= T;
		State[3] = RotL(State[3], 45);
		return Result;
	}

	/** [0,1) with 53 bits of precision. */
	double NextDouble()
	{
		return (NextUInt64() >> 11) * (1.0 / 9007199254740992.0);
	}

	/** [0,1) as float, FRandomStream-style. */
	float FRand()
	{
		return static_cast<float>(NextDouble());
	}

	float FRandRange(float Min, float Max)
	{
		return Min + (Max - Min) * static_cast<float>(NextDouble());
	}

	/** Inclusive on both ends, like FRandomStream::RandRange. */
	int32 RandRange(int32 Min, int32 Max)
	{
		if (Max <= Min)
		{
			return Min;
		}
		const uint64 Span = static_cast<uint64>(static_cast<int64>(Max) - static_cast<int64>(Min)) + 1;
		return Min + static_cast<int32>(NextUInt64() % Span);
	}
};
