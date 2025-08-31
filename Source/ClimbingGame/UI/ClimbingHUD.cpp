#include "UI/ClimbingHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"

UClimbingHUD::UClimbingHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize warning thresholds
	StaminaWarningThreshold = 0.25f;
	HealthWarningThreshold = 0.3f;
	GripWarningThreshold = 0.2f;
	InjuryWarningDuration = 5.0f;

	// Initialize state tracking
	bStaminaWarning = false;
	bHealthWarning = false;
	bGripWarning = false;
	InjuryWarningTimer = 0.0f;
	bShowingInjuryWarning = false;
}

void UClimbingHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// Validate all required widget bindings
	if (!StaminaBar || !HealthBar || !LeftGripBar || !RightGripBar)
	{
		UE_LOG(LogTemp, Error, TEXT("ClimbingHUD: Core progress bars not bound correctly!"));
		return;
	}

	if (!AltitudeText || !EnvironmentInfoText || !ToolDurabilityContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("ClimbingHUD: Core text elements not bound correctly!"));
		return;
	}

	// Initialize all progress bars to full
	StaminaBar->SetPercent(1.0f);
	HealthBar->SetPercent(1.0f);
	LeftGripBar->SetPercent(1.0f);
	RightGripBar->SetPercent(1.0f);

	// Initialize text elements
	AltitudeText->SetText(FText::FromString(TEXT("Altitude: 0m")));
	EnvironmentInfoText->SetText(FText::FromString(TEXT("Ready to climb")));

	// Hide injury warning initially
	if (InjuryWarningPanel)
	{
		InjuryWarningPanel->SetVisibility(ESlateVisibility::Hidden);
	}

	// Initialize enhanced HUD elements if bound
	if (SurfaceAnalysisText)
	{
		SurfaceAnalysisText->SetText(FText::GetEmpty());
	}

	if (RouteDifficultyText)
	{
		RouteDifficultyText->SetText(FText::GetEmpty());
	}

	if (WeatherInfoText)
	{
		WeatherInfoText->SetText(FText::GetEmpty());
	}
}

void UClimbingHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Update warning states
	UpdateWarningStates();

	// Handle injury warning timer
	if (bShowingInjuryWarning && InjuryWarningTimer > 0.0f)
	{
		InjuryWarningTimer -= InDeltaTime;
		if (InjuryWarningTimer <= 0.0f)
		{
			HideInjuryWarning();
		}
	}
}

void UClimbingHUD::UpdateStamina(float CurrentStamina, float MaxStamina)
{
	if (!StaminaBar) return;

	float Percentage = FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f);
	StaminaBar->SetPercent(Percentage);
	UpdateProgressBarColor(StaminaBar, Percentage, StaminaWarningThreshold);

	// Trigger warning animation if needed
	if (Percentage <= StaminaWarningThreshold && !bStaminaWarning)
	{
		bStaminaWarning = true;
		PlayStaminaWarningAnimation();
	}
	else if (Percentage > StaminaWarningThreshold && bStaminaWarning)
	{
		bStaminaWarning = false;
	}
}

void UClimbingHUD::UpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (!HealthBar) return;

	float Percentage = FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
	HealthBar->SetPercent(Percentage);
	UpdateProgressBarColor(HealthBar, Percentage, HealthWarningThreshold);

	// Trigger warning animation if needed
	if (Percentage <= HealthWarningThreshold && !bHealthWarning)
	{
		bHealthWarning = true;
		PlayHealthWarningAnimation();
	}
	else if (Percentage > HealthWarningThreshold && bHealthWarning)
	{
		bHealthWarning = false;
	}
}

void UClimbingHUD::UpdateGripStrength(float LeftGrip, float RightGrip)
{
	if (!LeftGripBar || !RightGripBar) return;

	float LeftPercentage = FMath::Clamp(LeftGrip / 100.0f, 0.0f, 1.0f);
	float RightPercentage = FMath::Clamp(RightGrip / 100.0f, 0.0f, 1.0f);

	LeftGripBar->SetPercent(LeftPercentage);
	RightGripBar->SetPercent(RightPercentage);

	UpdateProgressBarColor(LeftGripBar, LeftPercentage, GripWarningThreshold);
	UpdateProgressBarColor(RightGripBar, RightPercentage, GripWarningThreshold);

	// Check for grip warnings
	float MinGrip = FMath::Min(LeftPercentage, RightPercentage);
	if (MinGrip <= GripWarningThreshold && !bGripWarning)
	{
		bGripWarning = true;
		PlayGripWarningAnimation();
	}
	else if (MinGrip > GripWarningThreshold && bGripWarning)
	{
		bGripWarning = false;
	}
}

void UClimbingHUD::UpdateToolDurability(const TArray<FToolDurabilityInfo>& ToolInfo)
{
	if (!ToolDurabilityContainer) return;

	// Clear existing tool widgets
	ToolDurabilityContainer->ClearChildren();

	// Create new widgets for each tool
	CreateToolDurabilityWidgets(ToolInfo);
}

void UClimbingHUD::UpdateEnvironmentInfo(const FString& RockType, const FString& WeatherCondition, float Temperature)
{
	if (!EnvironmentInfoText) return;

	FString EnvironmentString = FString::Printf(
		TEXT("Rock: %s | Weather: %s | Temp: %.1f°C"),
		*RockType, *WeatherCondition, Temperature
	);

	EnvironmentInfoText->SetText(FText::FromString(EnvironmentString));
}

void UClimbingHUD::ShowInjuryWarning(EInjuryType InjuryType, float Severity)
{
	if (!InjuryWarningPanel || !InjuryWarningText) return;

	// Set warning message based on injury type
	FString WarningMessage;
	switch (InjuryType)
	{
		case EInjuryType::MinorStrain:
			WarningMessage = TEXT("Minor muscle strain detected - Rest recommended");
			break;
		case EInjuryType::MajorStrain:
			WarningMessage = TEXT("Major strain - Consider stopping climb");
			break;
		case EInjuryType::Exhaustion:
			WarningMessage = TEXT("Exhaustion detected - Find rest position");
			break;
		case EInjuryType::HandInjury:
			WarningMessage = TEXT("Hand injury - Grip strength compromised");
			break;
		default:
			WarningMessage = TEXT("Injury detected - Exercise caution");
			break;
	}

	InjuryWarningText->SetText(FText::FromString(WarningMessage));
	InjuryWarningPanel->SetVisibility(ESlateVisibility::Visible);
	
	bShowingInjuryWarning = true;
	InjuryWarningTimer = InjuryWarningDuration;

	// Play injury warning animation
	PlayInjuryWarningAnimation(InjuryType);
}

void UClimbingHUD::HideInjuryWarning()
{
	if (!InjuryWarningPanel) return;

	InjuryWarningPanel->SetVisibility(ESlateVisibility::Hidden);
	bShowingInjuryWarning = false;
	InjuryWarningTimer = 0.0f;
}

void UClimbingHUD::UpdateAltitude(float CurrentAltitude)
{
	if (!AltitudeText) return;

	FString AltitudeString = FString::Printf(TEXT("Altitude: %.1fm"), CurrentAltitude);
	AltitudeText->SetText(FText::FromString(AltitudeString));
}

void UClimbingHUD::ShowSurfaceAnalysis(const FString& SurfaceType, float GripQuality, float Stability)
{
	if (!SurfaceAnalysisText) return;

	FString AnalysisString = FString::Printf(
		TEXT("%s - Grip: %.1f%% | Stability: %.1f%%"),
		*SurfaceType, GripQuality * 100.0f, Stability * 100.0f
	);

	SurfaceAnalysisText->SetText(FText::FromString(AnalysisString));

	// Update grip quality bar if available
	if (GripQualityBar)
	{
		GripQualityBar->SetPercent(GripQuality);
		
		// Color code based on quality
		FLinearColor GripColor;
		if (GripQuality >= 0.8f)
			GripColor = FLinearColor::Green;
		else if (GripQuality >= 0.5f)
			GripColor = FLinearColor::Yellow;
		else
			GripColor = FLinearColor::Red;
			
		GripQualityBar->SetFillColorAndOpacity(GripColor);
	}
}

void UClimbingHUD::UpdateClimbingDifficulty(const FString& RouteGrade, const FString& Description)
{
	if (!RouteDifficultyText) return;

	FString DifficultyString = FString::Printf(TEXT("%s - %s"), *RouteGrade, *Description);
	RouteDifficultyText->SetText(FText::FromString(DifficultyString));
}

void UClimbingHUD::ShowHoldPreview(const FVector& HoldLocation, float ReachDifficulty)
{
	if (!HoldPreviewPanel) return;

	// Show the hold preview panel
	HoldPreviewPanel->SetVisibility(ESlateVisibility::Visible);

	// Position the preview based on hold location (this would need screen projection in practice)
	// For now, just show it in a reasonable position
	
	// Auto-hide after a short delay
	GetWorld()->GetTimerManager().SetTimer(
		FTimerHandle(),
		[this]() { 
			if (HoldPreviewPanel) 
				HoldPreviewPanel->SetVisibility(ESlateVisibility::Hidden); 
		},
		3.0f,
		false
	);
}

void UClimbingHUD::UpdateBodyPosition(float Balance, float CenterOfGravity)
{
	if (!BodyPositionText || !BalanceIndicator) return;

	// Update balance indicator
	BalanceIndicator->SetPercent(Balance);
	
	// Color code balance
	FLinearColor BalanceColor;
	if (Balance >= 0.7f)
		BalanceColor = FLinearColor::Green;
	else if (Balance >= 0.4f)
		BalanceColor = FLinearColor::Yellow;
	else
		BalanceColor = FLinearColor::Red;
		
	BalanceIndicator->SetFillColorAndOpacity(BalanceColor);

	// Update body position text
	FString BalanceString = FString::Printf(TEXT("Balance: %.1f%%"), Balance * 100.0f);
	BodyPositionText->SetText(FText::FromString(BalanceString));
}

void UClimbingHUD::ShowToolPlacementGuide(const FString& ToolName, bool bCanPlace, const FString& Reason)
{
	if (!ToolPlacementGuidePanel || !ToolPlacementText) return;

	// Show placement guide
	ToolPlacementGuidePanel->SetVisibility(ESlateVisibility::Visible);

	// Set guide text
	FString GuideText;
	if (bCanPlace)
	{
		GuideText = FString::Printf(TEXT("%s - Good placement"), *ToolName);
	}
	else
	{
		GuideText = FString::Printf(TEXT("%s - Cannot place: %s"), *ToolName, *Reason);
	}

	ToolPlacementText->SetText(FText::FromString(GuideText));

	// Auto-hide after delay
	GetWorld()->GetTimerManager().SetTimer(
		FTimerHandle(),
		[this]() { 
			if (ToolPlacementGuidePanel) 
				ToolPlacementGuidePanel->SetVisibility(ESlateVisibility::Hidden); 
		},
		5.0f,
		false
	);
}

void UClimbingHUD::UpdateWeatherConditions(float Temperature, float Humidity, const FString& Conditions)
{
	if (!WeatherInfoText) return;

	FString WeatherString = FString::Printf(
		TEXT("%s | %.1f°C | %.0f%% humidity"),
		*Conditions, Temperature, Humidity
	);

	WeatherInfoText->SetText(FText::FromString(WeatherString));
}

void UClimbingHUD::SetMinimalHUDMode(bool bMinimal)
{
	// This would control visibility of various HUD elements
	// Implementation would depend on specific UI layout choices
	
	if (bMinimal)
	{
		// Hide non-essential elements
		if (EnvironmentInfoText) EnvironmentInfoText->SetVisibility(ESlateVisibility::Collapsed);
		if (WeatherInfoText) WeatherInfoText->SetVisibility(ESlateVisibility::Collapsed);
		if (RouteDifficultyText) RouteDifficultyText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// Show all elements
		if (EnvironmentInfoText) EnvironmentInfoText->SetVisibility(ESlateVisibility::Visible);
		if (WeatherInfoText) WeatherInfoText->SetVisibility(ESlateVisibility::Visible);
		if (RouteDifficultyText) RouteDifficultyText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UClimbingHUD::SetHUDOpacity(float Opacity)
{
	// Adjust opacity of the main HUD container
	SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, FMath::Clamp(Opacity, 0.1f, 1.0f)));
}

void UClimbingHUD::SetColorBlindFriendlyMode(bool bEnabled)
{
	// This would implement color blind friendly color schemes
	// Would need to store alternative color palettes and apply them
	
	// Example implementation for stamina bar
	if (StaminaBar && bEnabled)
	{
		// Use patterns or alternative colors instead of red/green
		// This is a simplified example - full implementation would be more comprehensive
	}
}

void UClimbingHUD::UpdateWarningStates()
{
	// This method is called every tick to update warning animations
	// Individual warning states are handled in their respective update methods
}

void UClimbingHUD::UpdateProgressBarColor(UProgressBar* ProgressBar, float Percentage, float WarningThreshold)
{
	if (!ProgressBar) return;

	FLinearColor BarColor;
	if (Percentage <= WarningThreshold)
	{
		BarColor = FLinearColor::Red;
	}
	else if (Percentage <= (WarningThreshold + 0.2f))
	{
		BarColor = FLinearColor::Yellow;
	}
	else
	{
		BarColor = FLinearColor::Green;
	}

	ProgressBar->SetFillColorAndOpacity(BarColor);
}

void UClimbingHUD::CreateToolDurabilityWidgets(const TArray<FToolDurabilityInfo>& ToolInfo)
{
	if (!ToolDurabilityContainer || !ToolDurabilityWidgetClass) return;

	// This would create individual tool durability widgets
	// Implementation would depend on the specific widget class structure
	
	for (const FToolDurabilityInfo& Tool : ToolInfo)
	{
		// Create widget for each tool
		if (UUserWidget* ToolWidget = CreateWidget<UUserWidget>(this, ToolDurabilityWidgetClass))
		{
			// Configure the tool widget with tool data
			// This would need specific implementation based on the tool widget structure
			
			ToolDurabilityContainer->AddChild(ToolWidget);
		}
	}
}