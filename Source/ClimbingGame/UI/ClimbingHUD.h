#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "ClimbingHUD.generated.h"

UENUM(BlueprintType)
enum class EInjuryType : uint8
{
	None,
	MinorStrain,
	MajorStrain,
	Exhaustion,
	HandInjury
};

USTRUCT(BlueprintType)
struct FToolDurabilityInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ToolName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CurrentDurability;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxDurability;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* ToolIcon;

	FToolDurabilityInfo()
	{
		ToolName = NAME_None;
		CurrentDurability = 100.0f;
		MaxDurability = 100.0f;
		ToolIcon = nullptr;
	}
};

/**
 * Main climbing HUD widget - displays essential climbing information
 * Designed to be minimal and non-intrusive during gameplay
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UClimbingHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	UClimbingHUD(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Update functions called by game systems
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateStamina(float CurrentStamina, float MaxStamina);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateGripStrength(float LeftGrip, float RightGrip);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateToolDurability(const TArray<FToolDurabilityInfo>& ToolInfo);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateEnvironmentInfo(const FString& RockType, const FString& WeatherCondition, float Temperature);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowInjuryWarning(EInjuryType InjuryType, float Severity);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideInjuryWarning();

	// Climbing-specific UI updates
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowClimbingRoute(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateAltitude(float CurrentAltitude);

	// Enhanced climbing feedback
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowSurfaceAnalysis(const FString& SurfaceType, float GripQuality, float Stability);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateClimbingDifficulty(const FString& RouteGrade, const FString& Description);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowHoldPreview(const FVector& HoldLocation, float ReachDifficulty);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateBodyPosition(float Balance, float CenterOfGravity);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowToolPlacementGuide(const FString& ToolName, bool bCanPlace, const FString& Reason);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateWeatherConditions(float Temperature, float Humidity, const FString& Conditions);

	// Accessibility and immersion options
	UFUNCTION(BlueprintCallable, Category = "HUD|Accessibility")
	void SetMinimalHUDMode(bool bMinimal);

	UFUNCTION(BlueprintCallable, Category = "HUD|Accessibility")
	void SetHUDOpacity(float Opacity);

	UFUNCTION(BlueprintCallable, Category = "HUD|Accessibility")
	void SetColorBlindFriendlyMode(bool bEnabled);

protected:
	// Core HUD elements - bound in Blueprint
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LeftGripBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RightGripBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> ToolDurabilityContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AltitudeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnvironmentInfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> InjuryWarningPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InjuryWarningText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> InjuryWarningIcon;

	// Enhanced HUD elements
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SurfaceAnalysisText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> GripQualityBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RouteDifficultyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> HoldPreviewPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BalanceIndicator;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ToolPlacementText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> ToolPlacementGuidePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeatherInfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BodyPositionText;

	// HUD Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Config")
	float StaminaWarningThreshold = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Config")
	float HealthWarningThreshold = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Config")
	float GripWarningThreshold = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Config")
	float InjuryWarningDuration = 5.0f;

	// Tool durability widget class
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Config")
	TSubclassOf<UUserWidget> ToolDurabilityWidgetClass;

private:
	// Internal state tracking
	bool bStaminaWarning;
	bool bHealthWarning;
	bool bGripWarning;
	
	float InjuryWarningTimer;
	bool bShowingInjuryWarning;

	// Helper functions
	void UpdateWarningStates();
	void UpdateProgressBarColor(UProgressBar* ProgressBar, float Percentage, float WarningThreshold);
	void CreateToolDurabilityWidgets(const TArray<FToolDurabilityInfo>& ToolInfo);

	// Animation functions
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD Animation")
	void PlayStaminaWarningAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD Animation")
	void PlayHealthWarningAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD Animation")
	void PlayGripWarningAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD Animation")
	void PlayInjuryWarningAnimation(EInjuryType InjuryType);
};