#include "RLAbilitySlotWidget.h"
#include "RLAbilitySystemComponent.h"
#include "RLAttributeSet.h"
#include "RLGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor EmptyTileColor(0.05f, 0.05f, 0.07f, 0.85f);
	const FLinearColor ReadyTint(1.f, 1.f, 1.f, 1.f);
	const FLinearColor OutOfManaTint(0.35f, 0.45f, 0.95f, 1.f);
}

bool URLAbilitySlotWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// The cooldown wedge overshoots the tile corners; trim it to the square.
	SetClipping(EWidgetClipping::ClipToBounds);

	// Pure native use (no WBP tree): build the tile programmatically.
	if (!IconImage && WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(SlotSize);
		Root->SetHeightOverride(SlotSize);

		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay"));
		Root->AddChild(Overlay);

		IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
		IconImage->SetColorAndOpacity(EmptyTileColor);
		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(IconImage);
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);

		CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CooldownText"));
		CooldownText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
		CooldownText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		CooldownText->SetJustification(ETextJustify::Center);
		CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* CooldownSlot = Overlay->AddChildToOverlay(CooldownText);
		CooldownSlot->SetHorizontalAlignment(HAlign_Center);
		CooldownSlot->SetVerticalAlignment(VAlign_Center);

		KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyText"));
		KeyText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f, 0.9f)));
		UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(KeyText);
		KeySlot->SetHorizontalAlignment(HAlign_Right);
		KeySlot->SetVerticalAlignment(VAlign_Bottom);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 3.f, 2.f));

		WidgetTree->RootWidget = Root;
	}

	return true;
}

URLAbilitySystemComponent* URLAbilitySlotWidget::GetOwnerASC() const
{
	APawn* Pawn = GetOwningPlayerPawn();
	return Pawn
		? Cast<URLAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		: nullptr;
}

URLGameplayAbility* URLAbilitySlotWidget::ResolveSlotAbility() const
{
	URLAbilitySystemComponent* ASC = GetOwnerASC();
	return ASC ? ASC->FindAbilityByActionTag(ActionTag) : nullptr;
}

void URLAbilitySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	URLGameplayAbility* Ability = ResolveSlotAbility();

	if (bCollapseWhenEmpty)
	{
		SetVisibility(Ability ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!Ability)
		{
			CooldownFraction = 0.f;
			return;
		}
	}

	if (KeyText)
	{
		KeyText->SetText(KeybindLabel);
	}

	// Cooldown state feeds both the seconds text and the painted dial.
	float Remaining = 0.f;
	float Duration = 0.f;
	if (Ability)
	{
		Remaining = Ability->GetCooldownRemaining();
		Duration = Ability->GetCooldownDuration();
	}
	CooldownFraction = Duration > KINDA_SMALL_NUMBER ? FMath::Clamp(Remaining / Duration, 0.f, 1.f) : 0.f;

	if (CooldownText)
	{
		if (Remaining > 0.05f)
		{
			CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
			CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IconImage)
	{
		UTexture2D* Icon = Ability ? Ability->AbilityIcon.Get() : nullptr;
		if (Icon != LastIconTexture.Get() || !bBrushInitialized)
		{
			if (Icon)
			{
				IconImage->SetBrushFromTexture(Icon);
			}
			LastIconTexture = Icon;
			bBrushInitialized = true;
		}

		bool bAffordable = true;
		if (Ability && Ability->ManaCost > 0.f)
		{
			if (URLAbilitySystemComponent* ASC = GetOwnerASC())
			{
				bAffordable = ASC->GetNumericAttribute(URLAttributeSet::GetManaAttribute()) >= Ability->ManaCost;
			}
		}

		if (!Icon)
		{
			IconImage->SetColorAndOpacity(EmptyTileColor);
		}
		else
		{
			IconImage->SetColorAndOpacity(bAffordable ? ReadyTint : OutOfManaTint);
		}
	}
}

int32 URLAbilitySlotWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (CooldownFraction <= KINDA_SMALL_NUMBER)
	{
		return MaxLayer;
	}

	// A solid-color brush renders untextured, so the vertex color is all
	// that matters — no material or texture asset required for the dial.
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateResourceHandle Handle = WhiteBrush->GetRenderingResource();
	if (!Handle.IsValid())
	{
		return MaxLayer;
	}

	const FVector2f LocalSize = FVector2f(AllottedGeometry.GetLocalSize());
	const FVector2f Center = LocalSize * 0.5f;
	// Long enough to reach the tile's corners; ClipToBounds trims the spill.
	const float Radius = Center.Size() + 2.f;

	const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
	const FColor DimColor(0, 0, 0, 160);
	const FVector2f WhiteUV(0.f, 0.f);

	// Clock dial: the dark wedge is the unrecovered slice. Its trailing edge
	// stays at 12 o'clock and its leading edge sweeps clockwise toward it as
	// the cooldown recovers (screen Y is down, so increasing angle = CW).
	const float TwoPi = 2.f * PI;
	const float EndAngle = 1.5f * PI; // 12 o'clock
	const float StartAngle = EndAngle - CooldownFraction * TwoPi;
	const int32 ArcSegments = FMath::Max(2, FMath::CeilToInt(48.f * CooldownFraction));

	TArray<FSlateVertex> Verts;
	TArray<SlateIndex> Indexes;
	Verts.Reserve(ArcSegments + 2);
	Indexes.Reserve(ArcSegments * 3);

	Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Center, WhiteUV, DimColor));
	for (int32 i = 0; i <= ArcSegments; ++i)
	{
		const float Angle = FMath::Lerp(StartAngle, EndAngle, float(i) / float(ArcSegments));
		const FVector2f Pos = Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Pos, WhiteUV, DimColor));
		if (i > 0)
		{
			Indexes.Add(0);
			Indexes.Add(static_cast<SlateIndex>(i));
			Indexes.Add(static_cast<SlateIndex>(i + 1));
		}
	}

	FSlateDrawElement::MakeCustomVerts(OutDrawElements, MaxLayer + 1, Handle, Verts, Indexes, nullptr, 0, 0);
	return MaxLayer + 1;
}
