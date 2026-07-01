#include "RLChallengeAltar.h"
#include "RLEnemyBase.h"
#include "RLRunManagerSubsystem.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ARLChallengeAltar::ARLChallengeAltar()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);

	ChargeZone = CreateDefaultSubobject<USphereComponent>(TEXT("ChargeZone"));
	ChargeZone->SetupAttachment(Mesh);
	ChargeZone->SetSphereRadius(ChargeRadius);
	ChargeZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARLChallengeAltar::BeginPlay()
{
	Super::BeginPlay();
	ChargeZone->SetSphereRadius(ChargeRadius);
}

void ARLChallengeAltar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AltarState != ERLAltarState::Charging)
	{
		return;
	}

	// Charge only while the hero holds the zone — no hiding at the map edge.
	if (ChargeFraction < 1.f)
	{
		APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Hero && FVector::Dist(Hero->GetActorLocation(), GetActorLocation()) <= ChargeRadius)
		{
			ChargeFraction = FMath::Min(ChargeFraction + DeltaSeconds / ChargeDuration, 1.f);
		}
	}

	if (ChargeFraction >= 1.f && !IsBossAlive())
	{
		AltarState = ERLAltarState::Charged;
		if (URLRunManagerSubsystem* RunManager =
			GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
		{
			RunManager->SetRunState(ERLRunState::AltarCharged);
		}
		OnFullyCharged();
	}
}

bool ARLChallengeAltar::IsBossAlive() const
{
	return ChallengeBoss && !ChallengeBoss->IsDead();
}

// --- IRLInteractable ---

bool ARLChallengeAltar::CanInteract_Implementation(AActor* Interactor) const
{
	return AltarState == ERLAltarState::Dormant || AltarState == ERLAltarState::Charged;
}

FText ARLChallengeAltar::GetInteractionPrompt_Implementation() const
{
	switch (AltarState)
	{
	case ERLAltarState::Dormant:  return NSLOCTEXT("RL", "AltarBegin", "Begin the Challenge");
	case ERLAltarState::Charging: return NSLOCTEXT("RL", "AltarCharging", "The altar drinks...");
	case ERLAltarState::Charged:  return NSLOCTEXT("RL", "AltarUse", "Use the Altar");
	default:                      return FText::GetEmpty();
	}
}

void ARLChallengeAltar::Interact_Implementation(AActor* Interactor)
{
	if (AltarState == ERLAltarState::Dormant)
	{
		BeginChallenge(Interactor);
	}
	else if (AltarState == ERLAltarState::Charged)
	{
		// Hand the extract-or-continue decision to the UI.
		OnChoiceRequested();
	}
}

// --- Challenge flow ---

void ARLChallengeAltar::BeginChallenge(AActor* Interactor)
{
	AltarState = ERLAltarState::Charging;
	ChargeFraction = 0.f;

	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		RunManager->SetRunState(ERLRunState::AltarChallenge);
	}

	SpawnChallengeBoss(Interactor);
	OnChallengeStarted(ChallengeBoss);
}

void ARLChallengeAltar::SpawnChallengeBoss(AActor* Interactor)
{
	UGameInstance* GI = GetGameInstance();
	URLRunManagerSubsystem* RunManager = GI ? GI->GetSubsystem<URLRunManagerSubsystem>() : nullptr;
	URLDataSubsystem* Data = GI ? GI->GetSubsystem<URLDataSubsystem>() : nullptr;

	// Resolve the zone's boss card; fall back to the editor-set class.
	UClass* BossClass = FallbackBossClass.Get();
	if (Data && RunManager)
	{
		if (const FRLZoneRow* Zone = Data->FindZone(RunManager->GetCurrentZoneIndex()))
		{
			if (const FRLSpawnCardRow* Card = Data->FindSpawnCard(Zone->BossCardId))
			{
				if (UClass* Loaded = Card->EnemyClass.LoadSynchronous())
				{
					BossClass = Loaded;
				}
			}
		}
	}

	if (!BossClass)
	{
		// No boss available at all: don't soft-lock the run.
		ChargeFraction = FMath::Max(ChargeFraction, 0.f);
		return;
	}

	const FVector Forward = Interactor
		? (GetActorLocation() - Interactor->GetActorLocation()).GetSafeNormal2D()
		: GetActorForwardVector();
	const FVector SpawnLoc = GetActorLocation() + Forward * 800.f + FVector(0, 0, 120.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ChallengeBoss = GetWorld()->SpawnActor<ARLEnemyBase>(BossClass, SpawnLoc, FRotator::ZeroRotator, Params);
	if (ChallengeBoss)
	{
		ChallengeBoss->bEmpowered = true;
		ChallengeBoss->OnEnemyKilled.AddDynamic(this, &ARLChallengeAltar::HandleBossKilled);
	}
}

void ARLChallengeAltar::HandleBossKilled(ARLEnemyBase* Enemy, AActor* Killer)
{
	// Tick handles the Charging -> Charged transition once charge completes.
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		if (RunManager->GetCurrentZoneIndex() >= RLBalance::ZonesPerRun)
		{
			RunManager->HandleFinalBossKilled(/*bWildGod=*/false);
		}
	}
}

// --- Choice ---

void ARLChallengeAltar::ChooseExtract()
{
	if (AltarState != ERLAltarState::Charged)
	{
		return;
	}
	AltarState = ERLAltarState::Consumed;
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		RunManager->ExtractToBaseCamp();
	}
}

void ARLChallengeAltar::ChooseContinue()
{
	if (AltarState != ERLAltarState::Charged)
	{
		return;
	}
	AltarState = ERLAltarState::Consumed;
	if (URLRunManagerSubsystem* RunManager =
		GetGameInstance()->GetSubsystem<URLRunManagerSubsystem>())
	{
		RunManager->AdvanceToNextZone();
	}
}
