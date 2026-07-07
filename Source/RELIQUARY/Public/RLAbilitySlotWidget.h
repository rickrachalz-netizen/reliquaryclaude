// RELIQUARY — one RoR2-style ability icon: icon, keybind label, and an
// animated clock-dial cooldown sweep with the seconds remaining.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "RLAbilitySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class URLAbilitySystemComponent;
class URLGameplayAbility;

/**
 * One slot of the ability bar. Every frame it resolves the owning pawn's
 * ability in ActionTag's slot (the same newest-first lookup input uses, so
 * keystone/essence overrides show automatically) and displays its icon —
 * or the keybind label on a dark tile when the ability has no icon yet.
 *
 * The cooldown dial is painted natively (a triangle-fan pie wedge), so no
 * material or texture assets are needed. The icon dims blue when the owner
 * can't afford the ability's mana cost.
 *
 * Works with zero content. To restyle, reparent a WBP to this class with an
 * Image named "IconImage" and TextBlocks named "CooldownText"/"KeyText" —
 * the native tree steps aside and the radial sweep still paints on top.
 */
UCLASS()
class RELIQUARY_API URLAbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which kit slot this icon watches (Ability.Primary ... Ability.Essence). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Slot")
	FGameplayTag ActionTag;

	/** Corner label ("LMB", "R", "Q", ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Slot")
	FText KeybindLabel;

	/** Collapse the whole slot while nothing occupies it (the essence slot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RELIQUARY|Slot")
	bool bCollapseWhenEmpty = false;

	/** Tile size of the native fallback tree (WBP trees size themselves). */
	UPROPERTY(EditDefaultsOnly, Category = "RELIQUARY|Slot")
	float SlotSize = 64.f;

protected:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** The ability currently answering to ActionTag on the owning pawn. */
	URLGameplayAbility* ResolveSlotAbility() const;
	URLAbilitySystemComponent* GetOwnerASC() const;

	/** Bound from a WBP subclass, or built natively when absent. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Slot")
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Slot")
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Slot")
	TObjectPtr<UTextBlock> KeyText;

	// Updated in NativeTick, read by NativePaint (paint is const).
	float CooldownFraction = 0.f;

	// Avoids rebuilding the icon brush every frame.
	TWeakObjectPtr<UTexture2D> LastIconTexture;
	bool bBrushInitialized = false;
};
