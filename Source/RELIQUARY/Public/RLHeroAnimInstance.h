// RELIQUARY — hero anim data: the torso follows the camera aim.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RLHeroAnimInstance.generated.h"

/**
 * Reparent the hero's AnimBP to this class and it gets smoothed aim values
 * every frame: TorsoYaw is how far the camera points off the body's facing
 * (clamped so the spine never corkscrews), TorsoPitch follows the camera up
 * and down. Strafe left while aiming right and the upper body leads the
 * weapon toward the crosshair while the legs keep running their own way.
 *
 * AnimGraph wiring (see docs/EDITOR_SETUP.md): between the state machine and
 * the Output Pose, add a Transform (Modify) Bone node per spine bone
 * (spine_01..spine_03), rotation mode Add Existing in Component Space, each
 * fed TorsoYaw/N and TorsoPitch/N so the twist spreads naturally.
 */
UCLASS()
class RELIQUARY_API URLHeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Camera yaw relative to the body, clamped. Drive spine twist with it. */
	UPROPERTY(BlueprintReadOnly, Category = "RELIQUARY|Aim")
	float TorsoYaw = 0.f;

	/** Camera pitch, clamped. Drive spine lean with it. */
	UPROPERTY(BlueprintReadOnly, Category = "RELIQUARY|Aim")
	float TorsoPitch = 0.f;

	/** The spine stops following past this many degrees off the body. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Aim")
	float MaxTorsoYaw = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Aim")
	float MaxTorsoPitch = 60.f;

	/** Smoothing toward the target aim; higher is snappier. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Aim")
	float AimInterpSpeed = 12.f;

	/**
	 * Global animation play-rate multiplier driven by Haste. 1.0 at 0% haste,
	 * rising to (1 + MaxHastePlayRateBonus) as effective haste (base attribute
	 * plus the live Battle Trance haste bonus) reaches 100%, on a convex curve
	 * so it speeds up faster and faster. Bind this to the Play Rate of the
	 * locomotion sequence/blend-space players in the AnimGraph.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RELIQUARY|Haste")
	float HastePlayRate = 1.f;

	/** Extra play rate at 100% effective haste (0.20 = up to +20%). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Haste")
	float MaxHastePlayRateBonus = 0.20f;

	/** Curve shape; >1 makes the speed-up accelerate as haste grows. */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Haste")
	float HastePlayRateExponent = 2.f;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
