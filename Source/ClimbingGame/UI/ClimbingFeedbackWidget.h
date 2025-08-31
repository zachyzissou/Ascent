#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/WidgetAnimation.h"
#include "ClimbingFeedbackWidget.generated.h"

UENUM(BlueprintType)
enum class EClimbingFeedbackType : uint8
{
	HoldQuality,        // Information about current hold
	RoutePreview,       // Next holds and route guidance
	TechniqueHint,      // Movement suggestions
	SafetyAlert,        // Warning about dangerous situations
	ToolGuidance,       // Tool placement suggestions
	WeatherUpdate,      // Environmental condition changes
	TeamStatus,         // Multiplayer team information
	Achievement        // Progress and accomplishment feedback
};

UENUM(BlueprintType)
enum class EHoldType : uint8
{
	Jug,              // Large, positive hold
	Crimp,            // Small edge requiring fingertip strength
	Sloper,           // Rounded hold requiring open-hand grip
	Pocket,           // Hole in the rock for fingers
	Pinch,            // Hold requiring thumb opposition
	Side_Pull,        // Vertical hold pulled sideways
	Under_Cling,      // Upside-down hold
	Mantle,           // Top-out ledge requiring pressing motion
	Arete,            // Corner edge of rock
	Crack,            // Fissure in rock requiring jamming
	Volume,           // Artificial hold on indoor walls
	Unknown
};

USTRUCT(BlueprintType)
struct FHoldAnalysis
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EHoldType HoldType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float QualityRating; // 0-10 scale

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float GripStrengthRequired; // 0-100%

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector WorldLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsLeftHandOptimal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsRightHandOptimal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsFootHold;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ReachDifficulty; // How hard it is to reach from current position

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SurfaceMaterial; // Granite, limestone, sandstone, etc.

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsWet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsChalkable;

	FHoldAnalysis()
	{
		HoldType = EHoldType::Unknown;
		QualityRating = 5.0f;
		GripStrengthRequired = 50.0f;
		WorldLocation = FVector::ZeroVector;
		bIsLeftHandOptimal = false;
		bIsRightHandOptimal = false;
		bIsFootHold = false;
		ReachDifficulty = 1.0f;
		SurfaceMaterial = TEXT("Unknown");
		bIsWet = false;
		bIsChalkable = true;
	}
};

USTRUCT(BlueprintType)
struct FRouteGuidance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FHoldAnalysis> NextHolds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RouteGrade;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RouteStyle; // Sport, trad, boulder, etc.

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RecommendedSequence;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float EstimatedDifficulty; // 0-10 based on player skill

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresSpecialTechnique;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString TechniqueDescription;

	FRouteGuidance()
	{
		RouteGrade = TEXT("5.6");
		RouteStyle = TEXT("Sport");
		RecommendedSequence = TEXT("");
		EstimatedDifficulty = 5.0f;
		bRequiresSpecialTechnique = false;
		TechniqueDescription = TEXT("");
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFeedbackInteraction, EClimbingFeedbackType, FeedbackType, const FString&, ActionRequested);

/**
 * Advanced climbing feedback widget providing contextual information
 * Displays hold analysis, route guidance, technique hints, and environmental data
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UClimbingFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UClimbingFeedbackWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Main feedback functions
	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowHoldAnalysis(const FHoldAnalysis& HoldData, float DisplayDuration = 3.0f);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowRouteGuidance(const FRouteGuidance& RouteData, bool bPersistent = false);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowTechniqueHint(const FString& TechniqueTitle, const FString& Description, UTexture2D* DiagramImage = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowSafetyAlert(const FString& AlertMessage, float Severity, bool bRequiresAcknowledgment = false);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowToolGuidance(const FString& ToolName, const FVector& PlacementLocation, bool bIsOptimalPlacement);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowWeatherUpdate(const FString& WeatherChange, float ImpactSeverity);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ShowAchievementNotification(const FString& Achievement, const FString& Description);

	// Feedback management
	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ClearAllFeedback();

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void ClearFeedbackType(EClimbingFeedbackType FeedbackType);

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback")
	void SetFeedbackEnabled(EClimbingFeedbackType FeedbackType, bool bEnabled);

	// Customization
	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback|Customization")
	void SetFeedbackIntensity(float Intensity); // 0.0 = minimal, 1.0 = maximum detail

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback|Customization")
	void SetExpertMode(bool bExpert); // Reduces beginner hints

	UFUNCTION(BlueprintCallable, Category = "Climbing Feedback|Customization")
	void SetFeedbackPosition(EClimbingFeedbackType FeedbackType, const FVector2D& ScreenPosition);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Climbing Feedback")
	FOnFeedbackInteraction OnFeedbackInteraction;

protected:
	// Main container panels for different feedback types
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MainFeedbackContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> HoldAnalysisPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> RouteGuidancePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TechniqueHintPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> SafetyAlertPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ToolGuidancePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> WeatherUpdatePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> AchievementPanel;

	// Hold Analysis components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HoldTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HoldQualityBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> GripRequiredBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HoldDiagramImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HoldDetailsText;

	// Route Guidance components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RouteGradeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RouteSequenceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> NextHoldsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RouteDifficultyBar;

	// Technique Hint components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TechniqueTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TechniqueDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TechniqueDiagramImage;

	// Safety Alert components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SafetyAlertText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SafetyAlertIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> AlertSeverityBar;

	// Tool Guidance components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ToolNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlacementQualityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ToolPlacementIcon;

	// Weather Update components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeatherChangeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> WeatherIcon;

	// Achievement components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AchievementTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AchievementDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AchievementIcon;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback Config")
	float DefaultDisplayDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback Config")
	float FadeAnimationDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback Config")
	bool bAllowOverlappingFeedback = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback Config")
	TMap<EClimbingFeedbackType, bool> EnabledFeedbackTypes;

private:
	// Current state
	TMap<EClimbingFeedbackType, float> FeedbackTimers;
	TMap<EClimbingFeedbackType, bool> PersistentFeedback;
	float CurrentFeedbackIntensity;
	bool bIsExpertMode;

	// Animation system
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeInAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeOutAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PulseAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> SlideInAnimation;

	// Helper functions
	void UpdateFeedbackTimers(float DeltaTime);
	void HideFeedbackPanel(EClimbingFeedbackType FeedbackType);
	void ShowFeedbackPanel(EClimbingFeedbackType FeedbackType);
	void PlayFeedbackAnimation(EClimbingFeedbackType FeedbackType, bool bShowAnimation);
	FLinearColor GetSeverityColor(float Severity) const;
	FString GetHoldTypeDisplayName(EHoldType HoldType) const;
	UTexture2D* GetHoldTypeDiagram(EHoldType HoldType) const;
	bool ShouldShowFeedback(EClimbingFeedbackType FeedbackType) const;
	void UpdateFeedbackIntensity();
};