#include "RLHeroAnimInstance.h"
#include "GameFramework/Pawn.h"
#include "RELIQUARYCharacter.h"
#include "RLAttributeSet.h"
#include "AbilitySystemComponent.h"

void URLHeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		return;
	}

	// Effective haste (base attribute + live Battle Trance bonus, the same sum
	// GetHastedDuration uses for cooldowns) nudges the global play rate up on a
	// convex curve so motion quickens faster and faster, capped near +20%.
	if (const ARELIQUARYCharacter* Hero = Cast<ARELIQUARYCharacter>(Pawn))
	{
		float Haste = 0.f;
		if (const UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent())
		{
			Haste = ASC->GetNumericAttribute(URLAttributeSet::GetHasteAttribute());
		}
		Haste += Hero->GetTranceHasteBonus();
		const float H = FMath::Clamp(Haste, 0.f, 1.f);
		HastePlayRate = 1.f + MaxHastePlayRateBonus * FMath::Pow(H, HastePlayRateExponent);
	}

	// GetBaseAimRotation is the controller's view for player pawns, so this
	// tracks the camera without caring how the body is currently turned.
	const FRotator Delta = (Pawn->GetBaseAimRotation() - Pawn->GetActorRotation()).GetNormalized();
	const float TargetYaw = FMath::Clamp(Delta.Yaw, -MaxTorsoYaw, MaxTorsoYaw);
	const float TargetPitch = FMath::Clamp(Delta.Pitch, -MaxTorsoPitch, MaxTorsoPitch);

	TorsoYaw = FMath::FInterpTo(TorsoYaw, TargetYaw, DeltaSeconds, AimInterpSpeed);
	TorsoPitch = FMath::FInterpTo(TorsoPitch, TargetPitch, DeltaSeconds, AimInterpSpeed);
}
