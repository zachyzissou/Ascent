#include "ClimbingUIManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"

const FName UClimbingUIManager::UI_STATE_CLIMBING = FName("Climbing");
const FName UClimbingUIManager::UI_STATE_INVENTORY = FName("Inventory");
const FName UClimbingUIManager::UI_STATE_SETTINGS = FName("Settings");
const FName UClimbingUIManager::UI_STATE_TUTORIAL = FName("Tutorial");
const FName UClimbingUIManager::UI_STATE_COOPERATIVE = FName("Cooperative");

UClimbingUIManager::UClimbingUIManager()
{
	CurrentUIState = UI_STATE_CLIMBING;
}

void UClimbingUIManager::Initialize(APlayerController* PlayerController)
{
	OwningPlayerController = PlayerController;
	
	if (!OwningPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ClimbingUIManager: Failed to initialize - PlayerController is null"));
		return;
	}

	// Set initial state to climbing
	SetUIState(UI_STATE_CLIMBING);
}

void UClimbingUIManager::ShowClimbingHUD(bool bShow)
{
	if (!OwningPlayerController) return;

	CreateWidgetIfNeeded(ClimbingHUD, ClimbingHUDClass);
	
	if (ClimbingHUD)
	{
		if (bShow)
		{
			ClimbingHUD->AddToViewport(0); // Lowest Z-order for HUD
		}
		else
		{
			ClimbingHUD->RemoveFromParent();
		}
	}
}

void UClimbingUIManager::ShowInventory(bool bShow)
{
	if (!OwningPlayerController) return;

	CreateWidgetIfNeeded(InventoryWidget, InventoryWidgetClass);
	
	if (InventoryWidget)
	{
		if (bShow)
		{
			InventoryWidget->AddToViewport(10); // Higher Z-order for overlay
		}
		else
		{
			InventoryWidget->RemoveFromParent();
		}
	}
}

void UClimbingUIManager::ShowCooperativeUI(bool bShow)
{
	if (!OwningPlayerController) return;

	CreateWidgetIfNeeded(CooperativeWidget, CooperativeWidgetClass);
	
	if (CooperativeWidget)
	{
		if (bShow)
		{
			CooperativeWidget->AddToViewport(5); // Mid Z-order
		}
		else
		{
			CooperativeWidget->RemoveFromParent();
		}
	}
}

void UClimbingUIManager::ShowSettings(bool bShow)
{
	if (!OwningPlayerController) return;

	CreateWidgetIfNeeded(SettingsWidget, SettingsWidgetClass);
	
	if (SettingsWidget)
	{
		if (bShow)
		{
			SettingsWidget->AddToViewport(20); // Highest Z-order for modal
		}
		else
		{
			SettingsWidget->RemoveFromParent();
		}
	}
}

void UClimbingUIManager::ShowTutorial(bool bShow)
{
	if (!OwningPlayerController) return;

	CreateWidgetIfNeeded(TutorialWidget, TutorialWidgetClass);
	
	if (TutorialWidget)
	{
		if (bShow)
		{
			TutorialWidget->AddToViewport(15); // High Z-order for overlay
		}
		else
		{
			TutorialWidget->RemoveFromParent();
		}
	}
}

void UClimbingUIManager::SetUIState(FName NewState)
{
	if (CurrentUIState == NewState) return;

	FName PreviousState = CurrentUIState;
	CurrentUIState = NewState;

	// Hide all widgets first
	HideAllWidgets();

	// Show appropriate widgets for new state
	if (NewState == UI_STATE_CLIMBING)
	{
		ShowClimbingHUD(true);
	}
	else if (NewState == UI_STATE_INVENTORY)
	{
		ShowClimbingHUD(true); // Keep HUD visible
		ShowInventory(true);
	}
	else if (NewState == UI_STATE_COOPERATIVE)
	{
		ShowClimbingHUD(true); // Keep HUD visible
		ShowCooperativeUI(true);
	}
	else if (NewState == UI_STATE_SETTINGS)
	{
		ShowSettings(true);
	}
	else if (NewState == UI_STATE_TUTORIAL)
	{
		ShowTutorial(true);
	}

	// Broadcast state change
	OnUIStateChanged.Broadcast(NewState);
}

void UClimbingUIManager::SetUIScale(float Scale)
{
	// Clamp scale to reasonable values
	Scale = FMath::Clamp(Scale, 0.5f, 2.0f);
	
	// Apply scale to all active widgets
	TArray<UUserWidget*> Widgets = {ClimbingHUD, InventoryWidget, CooperativeWidget, SettingsWidget, TutorialWidget};
	
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget && Widget->IsInViewport())
		{
			Widget->SetRenderScale(FVector2D(Scale, Scale));
		}
	}
}

void UClimbingUIManager::SetHighContrastMode(bool bEnabled)
{
	// This would typically modify material parameters or widget styles
	// Implementation depends on your UI styling system
	UE_LOG(LogTemp, Log, TEXT("High Contrast Mode: %s"), bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

void UClimbingUIManager::SetColorBlindMode(int32 ModeIndex)
{
	// Implement colorblind-friendly palette switching
	// 0 = Normal, 1 = Protanopia, 2 = Deuteranopia, 3 = Tritanopia
	UE_LOG(LogTemp, Log, TEXT("Colorblind Mode set to: %d"), ModeIndex);
}

void UClimbingUIManager::CreateWidgetIfNeeded(TObjectPtr<UUserWidget>& Widget, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!Widget && WidgetClass && OwningPlayerController)
	{
		Widget = CreateWidget<UUserWidget>(OwningPlayerController, WidgetClass);
		if (!Widget)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create widget from class"));
		}
	}
}

void UClimbingUIManager::HideAllWidgets()
{
	if (ClimbingHUD && ClimbingHUD->IsInViewport())
	{
		ClimbingHUD->RemoveFromParent();
	}
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
	}
	if (CooperativeWidget && CooperativeWidget->IsInViewport())
	{
		CooperativeWidget->RemoveFromParent();
	}
	if (SettingsWidget && SettingsWidget->IsInViewport())
	{
		SettingsWidget->RemoveFromParent();
	}
	if (TutorialWidget && TutorialWidget->IsInViewport())
	{
		TutorialWidget->RemoveFromParent();
	}
}