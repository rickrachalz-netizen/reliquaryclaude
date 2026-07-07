#include "RLPauseMenuWidget.h"
#include "RLGameInstance.h"
#include "RLRunManagerSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"

namespace
{
	UButton* MakeMenuButton(UWidgetTree* Tree, UVerticalBox* Parent, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.f, 6.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Button;
	}
}

bool URLPauseMenuWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// So NativeOnKeyDown can fire and the menu can always close itself.
	SetIsFocusable(true);

	// Pure native use (no WBP tree): build the dim overlay + three buttons.
	if (!ResumeButton && WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		Dim->SetPadding(FMargin(0.f));

		UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Dim->AddChild(Menu);

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(NSLOCTEXT("RL", "Paused", "PAUSED"));
		Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 30));
		Title->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitleSlot = Menu->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		ResumeButton = MakeMenuButton(WidgetTree, Menu, NSLOCTEXT("RL", "Resume", "Resume"));
		MainMenuButton = MakeMenuButton(WidgetTree, Menu, NSLOCTEXT("RL", "MainMenu", "Return to Main Menu"));
		QuitButton = MakeMenuButton(WidgetTree, Menu, NSLOCTEXT("RL", "QuitDesktop", "Exit to Desktop"));

		WidgetTree->RootWidget = Dim;
	}

	// Works for both native-built buttons and WBP-bound ones.
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &URLPauseMenuWidget::HandleResumeClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &URLPauseMenuWidget::HandleMainMenuClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &URLPauseMenuWidget::HandleQuitClicked);
	}

	return true;
}

FReply URLPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::P)
	{
		ResumeGame();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void URLPauseMenuWidget::ResumeGame()
{
	APlayerController* PC = GetOwningPlayer();
	UGameplayStatics::SetGamePaused(this, false);
	SetVisibility(ESlateVisibility::Collapsed);
	if (PC)
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void URLPauseMenuWidget::ReturnToMainMenu()
{
	UGameplayStatics::SetGamePaused(this, false);

	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->SaveToDisk();
		if (URLRunManagerSubsystem* RunManager = GI->GetSubsystem<URLRunManagerSubsystem>())
		{
			RunManager->AbandonRun();	// forfeit the haul; do not bank or travel home
		}
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("L_MainMenu")));
}

void URLPauseMenuWidget::QuitToDesktop()
{
	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->SaveToDisk();
	}
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
}

void URLPauseMenuWidget::HandleResumeClicked()   { ResumeGame(); }
void URLPauseMenuWidget::HandleMainMenuClicked() { ReturnToMainMenu(); }
void URLPauseMenuWidget::HandleQuitClicked()     { QuitToDesktop(); }
