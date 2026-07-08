#include "RLDamageNumberWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

bool URLDamageNumberWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// Pure native use (no WBP tree): a single text block is the whole widget.
	if (!DamageText && WidgetTree && !WidgetTree->RootWidget)
	{
		DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
		WidgetTree->RootWidget = DamageText;
	}
	return true;
}

void URLDamageNumberWidget::InitDamageNumber(float Amount, bool bCritical, const FVector& InWorldAnchor)
{
	WorldAnchor = InWorldAnchor;
	Elapsed = 0.f;
	bActive = true;
	JitterX = FMath::FRandRange(-HorizontalSpread, HorizontalSpread);

	if (DamageText)
	{
		const int32 Rounded = FMath::Max(1, FMath::RoundToInt(Amount));
		DamageText->SetText(FText::AsNumber(Rounded));
		DamageText->SetColorAndOpacity(FSlateColor(bCritical ? CritColor : NormalColor));

		FSlateFontInfo Font = DamageText->GetFont();
		Font.Size = bCritical ? CritFontSize : NormalFontSize;
		Font.TypefaceFontName = bCritical ? FName("Bold") : FName("Regular");
		DamageText->SetFont(Font);
		DamageText->SetJustification(ETextJustify::Center);
	}
}

void URLDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bActive)
	{
		return;
	}

	Elapsed += InDeltaTime;
	if (Elapsed >= Lifetime)
	{
		bActive = false;
		RemoveFromParent();
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPos;
	if (PC->ProjectWorldLocationToScreen(WorldAnchor, ScreenPos))
	{
		// Drift upward as it ages; nudge sideways so stacked hits fan out.
		ScreenPos.X += JitterX;
		ScreenPos.Y -= Elapsed * RiseSpeed;
		SetPositionInViewport(ScreenPos, false);
		SetRenderOpacity(FMath::Clamp(1.f - (Elapsed / Lifetime), 0.f, 1.f));
	}
	else
	{
		// Behind the camera / off screen — hide this frame.
		SetRenderOpacity(0.f);
	}
}
