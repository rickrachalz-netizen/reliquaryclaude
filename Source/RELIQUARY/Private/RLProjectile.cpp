#include "RLProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/DamageEvents.h"

ARLProjectile::ARLProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(16.f);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(Collision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2200.f;
	Movement->MaxSpeed = 2200.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
}

void ARLProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSeconds);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ARLProjectile::HandleOverlap);
	if (AActor* MyInstigator = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(MyInstigator, true);
	}
}

void ARLProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator() || OtherActor == GetOwner())
	{
		return;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		if (DamageSpec.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
		}
	}
	else if (OtherComp && OtherComp->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block)
	{
		// Destructible world objects (trees, rocks) take plain damage.
		OtherActor->TakeDamage(WorldDamage, FDamageEvent(), nullptr, GetInstigator());
	}
	else
	{
		// Ignore pure triggers/volumes.
		return;
	}

	OnImpact(OtherActor, GetActorLocation());
	Destroy();
}
