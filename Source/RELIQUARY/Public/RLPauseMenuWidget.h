// RELIQUARY — the pause menu (Esc, or P in PIE where Esc ends the session).
// Freezes the game and offers Resume / Return to Main Menu / Exit to Desktop.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RLPauseMenuWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Built natively so it works with zero content; reparent a WBP to this class
 * with buttons named ResumeButton / MainMenuButton / QuitButton to restyle
 * (the native tree steps aside). The widget is focusable and also closes on
 * Esc/P so it can never wedge the game paused.
 */
UCLASS()
class RELIQUARY_API URLPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Unpause, hide, and hand input back to the game. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Pause")
	void ResumeGame();

	/** Save, abandon any run, and load the main menu map. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Pause")
	void ReturnToMainMenu();

	/** Save and quit the application. */
	UFUNCTION(BlueprintCallable, Category = "RELIQUARY|Pause")
	void QuitToDesktop();

protected:
	virtual bool Initialize() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Pause")
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Pause")
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RELIQUARY|Pause")
	TObjectPtr<UButton> QuitButton;

	UFUNCTION() void HandleResumeClicked();
	UFUNCTION() void HandleMainMenuClicked();
	UFUNCTION() void HandleQuitClicked();
};
