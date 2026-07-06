#include "RLEnemyHealthBarWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"

bool URLEnemyHealthBarWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// Pure native use (no WBP tree): build the bar programmatically.
	if (!Bar && WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(BarWidth);
		Root->SetHeightOverride(BarHeight);

		Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar"));
		Bar->SetPercent(1.f);
		Bar->SetFillColorAndOpacity(FLinearColor(0.85f, 0.12f, 0.10f));
		Root->AddChild(Bar);

		WidgetTree->RootWidget = Root;
	}
	return true;
}

void URLEnemyHealthBarWidget::SetHealthFraction(float Fraction)
{
	if (Bar)
	{
		Bar->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
	}
}

void URLEnemyHealthBarWidget::SetBarColor(FLinearColor Color)
{
	if (Bar)
	{
		Bar->SetFillColorAndOpacity(Color);
	}
}
