#include "RLHeroAnimInstance.h"
#include "GameFramework/Pawn.h"

void URLHeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		return;
	}

	// GetBaseAimRotation is the controller's view for player pawns, so this
	// tracks the camera without caring how the body is currently turned.
	const FRotator Delta = (Pawn->GetBaseAimRotation() - Pawn->GetActorRotation()).GetNormalized();
	const float TargetYaw = FMath::Clamp(Delta.Yaw, -MaxTorsoYaw, MaxTorsoYaw);
	const float TargetPitch = FMath::Clamp(Delta.Pitch, -MaxTorsoPitch, MaxTorsoPitch);

	TorsoYaw = FMath::FInterpTo(TorsoYaw, TargetYaw, DeltaSeconds, AimInterpSpeed);
	TorsoPitch = FMath::FInterpTo(TorsoPitch, TargetPitch, DeltaSeconds, AimInterpSpeed);
}
