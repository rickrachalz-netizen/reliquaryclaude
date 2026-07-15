#include "RLEnemyGroup.h"
#include "RLEnemyBase.h"
#include "RELIQUARY.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

ARLEnemyGroup::ARLEnemyGroup()
{
	PrimaryActorTick.bCanEverTick = true;

	// A bare AActor has no root, so the spawn transform would be discarded —
	// and the anchor starts at the pack's spawn point.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ARLEnemyGroup::BeginPlay()
{
	Super::BeginPlay();
	AnchorLocation = GetActorLocation();
}

void ARLEnemyGroup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Release survivors back to the solo brain (guarding against members that
	// already migrated to another group, e.g. through a gang merge).
	for (ARLEnemyBase* Member : Members)
	{
		if (IsValid(Member) && Member->GetGroup() == this)
		{
			Member->SetGroup(nullptr);
		}
	}
	Members.Empty();

	Super::EndPlay(EndPlayReason);
}

void ARLEnemyGroup::RegisterMember(ARLEnemyBase* Enemy)
{
	if (!IsValid(Enemy) || Enemy->IsDead())
	{
		return;
	}
	Members.AddUnique(Enemy);
	Enemy->SetGroup(this);
}

void ARLEnemyGroup::NotifyMemberDamaged(ARLEnemyBase* /*Member*/, AActor* /*InstigatorActor*/)
{
	LastDamagedSeconds = GetWorld()->GetTimeSeconds();
}

int32 ARLEnemyGroup::GetAliveCount() const
{
	int32 Alive = 0;
	for (const ARLEnemyBase* Member : Members)
	{
		if (IsValid(Member) && !Member->IsDead())
		{
			++Alive;
		}
	}
	return Alive;
}

void ARLEnemyGroup::GetAliveMembers(TArray<ARLEnemyBase*>& Out) const
{
	Out.Reset();
	for (const TObjectPtr<ARLEnemyBase>& Member : Members)
	{
		if (IsValid(Member) && !Member->IsDead())
		{
			Out.Add(Member);
		}
	}
}

APawn* ARLEnemyGroup::GetHero() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

FVector ARLEnemyGroup::GetCentroid() const
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	for (const TObjectPtr<ARLEnemyBase>& Member : Members)
	{
		if (IsValid(Member) && !Member->IsDead())
		{
			Sum += Member->GetActorLocation();
			++Count;
		}
	}
	return Count > 0 ? Sum / static_cast<float>(Count) : AnchorLocation;
}

float ARLEnemyGroup::GetMinHeroDistance() const
{
	const APawn* Hero = GetHero();
	if (!Hero)
	{
		return MAX_flt;
	}

	float MinDistance = MAX_flt;
	for (const TObjectPtr<ARLEnemyBase>& Member : Members)
	{
		if (IsValid(Member) && !Member->IsDead())
		{
			MinDistance = FMath::Min(MinDistance,
				static_cast<float>(FVector::Dist(Member->GetActorLocation(), Hero->GetActorLocation())));
		}
	}
	return MinDistance;
}

bool ARLEnemyGroup::WasDamagedWithin(float Seconds) const
{
	return GetWorld()->GetTimeSeconds() - LastDamagedSeconds <= Seconds;
}

FVector ARLEnemyGroup::ProjectPoint(const FVector& Point) const
{
	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Projected;
		if (NavSystem->ProjectPointToNavigation(Point, Projected, FVector(500.f, 500.f, 1500.f)))
		{
			return Projected.Location;
		}
	}

	FHitResult Hit;
	const FVector Start = Point + FVector(0.f, 0.f, 2000.f);
	const FVector End = Point - FVector(0.f, 0.f, 2000.f);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic))
	{
		return Hit.ImpactPoint;
	}
	return Point;
}

FVector ARLEnemyGroup::PickWanderPoint(const FVector& Origin, float MinRange, float MaxRange)
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const bool bWorldHasNav = NavSystem && NavSystem->GetDefaultNavDataInstance() != nullptr;

	// Wander targets must land ON the navmesh. Long legs near the map edge
	// used to fail projection, fall back to a ground trace past the mesh, and
	// send whole packs marching off the world. Reroll with shrinking range
	// until something sticks; a group that finds nothing stays put this leg.
	float Range = Rng.FRandRange(MinRange, MaxRange);
	for (int32 Try = 0; Try < 6; ++Try)
	{
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const FVector Candidate = Origin +
			FVector(FMath::Cos(Angle) * Range, FMath::Sin(Angle) * Range, 0.f);

		if (bWorldHasNav)
		{
			FNavLocation Projected;
			if (NavSystem->ProjectPointToNavigation(Candidate, Projected, FVector(800.f, 800.f, 2000.f)))
			{
				return Projected.Location;
			}
			Range = FMath::Max(MinRange * 0.5f, Range * 0.6f);
			continue;
		}

		// No navmesh in this map: trace to ground and call it good.
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit,
			Candidate + FVector(0.f, 0.f, 2000.f), Candidate - FVector(0.f, 0.f, 2000.f), ECC_WorldStatic))
		{
			return Hit.ImpactPoint;
		}
		return Candidate;
	}
	return Origin;
}

bool ARLEnemyGroup::RingMatches(const TArray<ARLEnemyBase*>& Alive, ARLEnemyBase* Exclude) const
{
	if (RingExclude.Get() != Exclude)
	{
		return false;
	}

	int32 Expected = 0;
	for (ARLEnemyBase* Member : Alive)
	{
		if (Member != Exclude)
		{
			++Expected;
		}
	}
	if (RingMembers.Num() != Expected)
	{
		return false;
	}

	for (const TWeakObjectPtr<ARLEnemyBase>& Weak : RingMembers)
	{
		ARLEnemyBase* Member = Weak.Get();
		if (!Member || Member->IsDead() || Member == Exclude || !Alive.Contains(Member))
		{
			return false;
		}
	}
	return true;
}

void ARLEnemyGroup::AssignRing(const TArray<ARLEnemyBase*>& Alive, const FVector& Center, ARLEnemyBase* Exclude)
{
	struct FRingEntry
	{
		ARLEnemyBase* Member;
		float Bearing;
	};

	// Assign by current bearing so the fresh layout shuffles nobody, then keep
	// the assignment until the roster changes again.
	TArray<FRingEntry> Ring;
	Ring.Reserve(Alive.Num());
	for (ARLEnemyBase* Member : Alive)
	{
		if (Member == Exclude)
		{
			continue;
		}
		const FVector To = Member->GetActorLocation() - Center;
		Ring.Add({ Member, static_cast<float>(FMath::Atan2(To.Y, To.X)) });
	}
	Ring.Sort([](const FRingEntry& A, const FRingEntry& B) { return A.Bearing < B.Bearing; });

	RingMembers.Reset(Ring.Num());
	for (const FRingEntry& Entry : Ring)
	{
		RingMembers.Add(Entry.Member);
	}
	RingExclude = Exclude;
	RingPhase = Ring.Num() > 0 ? Ring[0].Bearing : 0.f;
}

void ARLEnemyGroup::OrderRing(const TArray<ARLEnemyBase*>& Alive, const FVector& Center, float Radius,
	float SpeedScale, float Tolerance, ARLEnemyBase* Exclude, float PhaseDeltaRadians, bool bFaceCenterAtSlot)
{
	if (!RingMatches(Alive, Exclude))
	{
		AssignRing(Alive, Center, Exclude);
	}
	if (RingMembers.Num() == 0)
	{
		return;
	}

	RingPhase += PhaseDeltaRadians;

	const float Step = 2.f * PI / RingMembers.Num();
	for (int32 i = 0; i < RingMembers.Num(); ++i)
	{
		ARLEnemyBase* Member = RingMembers[i].Get();
		if (!Member)
		{
			continue;
		}

		const float SlotAngle = RingPhase + Step * i;
		const FVector Slot = Center +
			FVector(FMath::Cos(SlotAngle) * Radius, FMath::Sin(SlotAngle) * Radius, 0.f);

		// The member's own move/stop latch handles arrival and idling; idle
		// members turn toward the ring's center when asked to.
		Member->SetGroupOrder(ERLGroupOrder::MoveTo, Slot, SpeedScale, Tolerance * 0.6f,
			bFaceCenterAtSlot ? Center : FVector::ZeroVector);

		if (bDrawDebug)
		{
			DrawDebugSphere(GetWorld(), Slot, 25.f, 8, FColor::Cyan, false, -1.f, 0, 1.5f);
			DrawDebugLine(GetWorld(), Member->GetActorLocation(), Slot, FColor::Cyan, false, -1.f, 0, 1.f);
		}
	}
}

void ARLEnemyGroup::PruneMembers()
{
	Members.RemoveAll([](const TObjectPtr<ARLEnemyBase>& Member)
	{
		return !IsValid(Member) || Member->IsDead();
	});
}

void ARLEnemyGroup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PruneMembers();
	if (Members.Num() == 0)
	{
		Destroy();
		return;
	}

	TickGroup(DeltaSeconds);

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), AnchorLocation, 40.f, 10, FColor::Yellow, false, -1.f, 0, 2.f);
	}
}
