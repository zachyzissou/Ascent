#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "ClimbingUIManager.generated.h"

class UClimbingHUD;
class UInventoryWidget;
class UCooperativeWidget;
class USettingsWidget;
class UTutorialWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStateChanged, FName, UIState);

/**
 * UI Manager responsible for handling all UI state transitions and widget management
 * Designed for VR-ready implementation and accessibility
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UClimbingUIManager : public UObject
{
	GENERATED_BODY()

public:
	UClimbingUIManager();

	// Initialize UI Manager with player controller
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Initialize(APlayerController* PlayerController);

	// Show/Hide specific UI elements
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowClimbingHUD(bool bShow = true);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowInventory(bool bShow = true);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowCooperativeUI(bool bShow = true);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSettings(bool bShow = true);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowTutorial(bool bShow = true);

	// UI State Management
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetUIState(FName NewState);

	UFUNCTION(BlueprintPure, Category = "UI")
	FName GetCurrentUIState() const { return CurrentUIState; }

	// Accessibility functions
	UFUNCTION(BlueprintCallable, Category = "UI|Accessibility")
	void SetUIScale(float Scale);

	UFUNCTION(BlueprintCallable, Category = "UI|Accessibility")
	void SetHighContrastMode(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "UI|Accessibility")
	void SetColorBlindMode(int32 ModeIndex);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnUIStateChanged OnUIStateChanged;

protected:
	// Widget classes - set in Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Widget Classes")
	TSubclassOf<UUserWidget> ClimbingHUDClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Widget Classes")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Widget Classes")
	TSubclassOf<UUserWidget> CooperativeWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Widget Classes")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Widget Classes")
	TSubclassOf<UUserWidget> TutorialWidgetClass;

	// Widget instances
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> ClimbingHUD;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InventoryWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> CooperativeWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> SettingsWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> TutorialWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<APlayerController> OwningPlayerController;

private:
	// Current UI state tracking
	FName CurrentUIState;

	// UI state names
	static const FName UI_STATE_CLIMBING;
	static const FName UI_STATE_INVENTORY;
	static const FName UI_STATE_SETTINGS;
	static const FName UI_STATE_TUTORIAL;
	static const FName UI_STATE_COOPERATIVE;

	// Helper functions
	void CreateWidgetIfNeeded(TObjectPtr<UUserWidget>& Widget, TSubclassOf<UUserWidget> WidgetClass);
	void HideAllWidgets();
};