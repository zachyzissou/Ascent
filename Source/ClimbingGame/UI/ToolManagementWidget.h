#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/GridPanel.h"
#include "Components/Border.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Engine/DataTable.h"
#include "ToolManagementWidget.generated.h"

UENUM(BlueprintType)
enum class EToolPlacementQuality : uint8
{
	Excellent,      // Perfect placement, maximum strength
	Good,          // Solid placement, full strength
	Fair,          // Acceptable placement, reduced strength
	Poor,          // Questionable placement, low strength
	Dangerous,     // Unsafe placement, failure risk
	Impossible     // Cannot be placed here
};

UENUM(BlueprintType)
enum class EToolState : uint8
{
	Stored,        // In inventory/rack
	InHand,        // Currently being held
	Placed,        // Placed in environment (anchored)
	Equipped,      // Equipped for quick access
	Broken,        // Damaged beyond use
	Lost          // Dropped or left behind
};

USTRUCT(BlueprintType)
struct FToolPlacementInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector PlacementLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EToolPlacementQuality Quality;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float StrengthRating; // kN force rating

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PlacementTime; // Time taken to place

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RockType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlacementNotes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresHammer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsRemovable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RemovalDifficulty; // 0-10 scale

	FToolPlacementInfo()
	{
		PlacementLocation = FVector::ZeroVector;
		Quality = EToolPlacementQuality::Good;
		StrengthRating = 10.0f;
		PlacementTime = 5.0f;
		RockType = TEXT("Unknown");
		PlacementNotes = TEXT("");
		bRequiresHammer = false;
		bIsRemovable = true;
		RemovalDifficulty = 3.0f;
	}
};

USTRUCT(BlueprintType)
struct FAdvancedToolInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ToolID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText ToolName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Manufacturer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EToolCategory Category;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EToolState CurrentState;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CurrentDurability;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxDurability;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Weight; // In grams

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float StrengthRating; // kN

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SizeDimensions; // e.g., "0.5-2 inches" for cams

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> CompatibleSurfaces; // Rock types where tool works best

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresSpecialSkill;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SkillRequirement; // 0-10 scale

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 UsageCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxUsages;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FToolPlacementInfo> PlacementHistory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDateTime LastInspection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bNeedsInspection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText SafetyNotes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* DetailedIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* PlacementDiagram;

	FAdvancedToolInfo()
	{
		ToolID = NAME_None;
		ToolName = FText::GetEmpty();
		Manufacturer = FText::GetEmpty();
		Category = EToolCategory::Utility;
		CurrentState = EToolState::Stored;
		CurrentDurability = 100.0f;
		MaxDurability = 100.0f;
		Weight = 0.0f;
		StrengthRating = 10.0f;
		SizeDimensions = TEXT("");
		bRequiresSpecialSkill = false;
		SkillRequirement = 1.0f;
		UsageCount = 0;
		MaxUsages = 100;
		bNeedsInspection = false;
		SafetyNotes = FText::GetEmpty();
		DetailedIcon = nullptr;
		PlacementDiagram = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FRackConfiguration
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ConfigurationName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> SelectedTools;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TotalWeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RouteType; // Sport, Trad, Alpine, etc.

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Difficulty;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsDefault;

	FRackConfiguration()
	{
		ConfigurationName = NAME_None;
		TotalWeight = 0.0f;
		RouteType = TEXT("Mixed");
		Difficulty = TEXT("5.6");
		bIsDefault = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnToolInteraction, const FAdvancedToolInfo&, Tool, FName, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRackConfigurationChanged, const FRackConfiguration&, NewConfiguration);

/**
 * Advanced tool management widget for climbing equipment
 * Provides detailed tool analysis, placement simulation, and rack configuration
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UToolManagementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UToolManagementWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

public:
	// Tool management
	UFUNCTION(BlueprintCallable, Category = "Tool Management")
	void RefreshToolInventory();

	UFUNCTION(BlueprintCallable, Category = "Tool Management")
	void SelectTool(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management")
	void InspectTool(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management")
	void RepairTool(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management")
	void RetireTool(const FName& ToolID);

	// Placement simulation
	UFUNCTION(BlueprintCallable, Category = "Tool Management|Placement")
	void SimulatePlacement(const FName& ToolID, const FVector& PlacementLocation);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Placement")
	void ShowPlacementGuide(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Placement")
	void AnalyzePlacementSite(const FVector& Location);

	// Rack configuration
	UFUNCTION(BlueprintCallable, Category = "Tool Management|Rack")
	void CreateRackConfiguration(const FString& ConfigurationName, const FString& RouteType);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Rack")
	void LoadRackConfiguration(const FName& ConfigurationName);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Rack")
	void SaveCurrentRackConfiguration();

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Rack")
	void OptimizeRackForRoute(const FString& RouteGrade, const FString& RouteStyle);

	// Tool categorization and filtering
	UFUNCTION(BlueprintCallable, Category = "Tool Management|Filter")
	void FilterByCategory(EToolCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Filter")
	void FilterByCondition(float MinDurability, float MaxDurability);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Filter")
	void FilterByWeight(float MaxWeight);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Filter")
	void ShowOnlyToolsNeedingInspection();

	// Maintenance and care
	UFUNCTION(BlueprintCallable, Category = "Tool Management|Maintenance")
	void ScheduleToolInspection(const FName& ToolID, int32 DaysFromNow);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Maintenance")
	void MarkToolForReplacement(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Maintenance")
	void UpdateToolCondition(const FName& ToolID, float NewDurability);

	// Education and guidance
	UFUNCTION(BlueprintCallable, Category = "Tool Management|Education")
	void ShowToolUsageGuide(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Education")
	void ShowSafetyInformation(const FName& ToolID);

	UFUNCTION(BlueprintCallable, Category = "Tool Management|Education")
	void ShowPlacementTechniques(EToolCategory Category);

	// Statistics and analytics
	UFUNCTION(BlueprintPure, Category = "Tool Management|Stats")
	float GetRackTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Tool Management|Stats")
	int32 GetToolsNeedingInspection() const;

	UFUNCTION(BlueprintPure, Category = "Tool Management|Stats")
	TArray<FAdvancedToolInfo> GetMostUsedTools(int32 Count) const;

	UFUNCTION(BlueprintPure, Category = "Tool Management|Stats")
	float GetAverageToolCondition() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Tool Management")
	FOnToolInteraction OnToolInteraction;

	UPROPERTY(BlueprintAssignable, Category = "Tool Management")
	FOnRackConfigurationChanged OnRackConfigurationChanged;

protected:
	// Main UI panels
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MainToolContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ToolListScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridPanel> ToolGrid;

	// Tool details panel
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ToolDetailsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ToolNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ToolManufacturerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ToolDetailImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> DurabilityBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StrengthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> UsageCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DimensionsText;

	// Placement simulation panel
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> PlacementSimulationPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PlacementDiagram;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlacementQualityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlacementStrengthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlacementNotesText;

	// Rack configuration panel
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> RackConfigPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> RackConfigDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RackWeightText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> RackToolList;

	// Filter controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> FilterContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> CategoryFilter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> DurabilityFilter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> WeightFilter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> InspectionNeededFilter;

	// Action buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InspectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RepairButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RetireButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ShowGuideButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SimulatePlacementButton;

	// Statistics display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalToolsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalWeightText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AverageConditionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InspectionDueText;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool Config")
	TSubclassOf<UUserWidget> ToolSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool Config")
	TObjectPtr<UDataTable> ToolDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool Config")
	TMap<EToolCategory, FLinearColor> CategoryColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tool Config")
	float WeightWarningThreshold = 12000.0f; // 12kg in grams

private:
	// Current state
	TArray<FAdvancedToolInfo> AllTools;
	TArray<FAdvancedToolInfo> FilteredTools;
	FAdvancedToolInfo SelectedTool;
	FRackConfiguration CurrentRackConfig;
	TArray<FRackConfiguration> SavedConfigurations;

	// Filter state
	EToolCategory CurrentCategoryFilter;
	float CurrentDurabilityFilter;
	float CurrentWeightFilter;
	bool bShowOnlyInspectionNeeded;

	// Event handlers
	UFUNCTION()
	void OnInspectButtonClicked();

	UFUNCTION()
	void OnRepairButtonClicked();

	UFUNCTION()
	void OnRetireButtonClicked();

	UFUNCTION()
	void OnShowGuideButtonClicked();

	UFUNCTION()
	void OnSimulatePlacementButtonClicked();

	UFUNCTION()
	void OnCategoryFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnDurabilityFilterChanged(float Value);

	UFUNCTION()
	void OnWeightFilterChanged(float Value);

	UFUNCTION()
	void OnInspectionFilterChanged(bool bIsChecked);

	UFUNCTION()
	void OnRackConfigChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	// Helper functions
	void PopulateToolGrid();
	void UpdateToolDetails();
	void UpdatePlacementSimulation();
	void UpdateRackConfiguration();
	void UpdateStatistics();
	void ApplyFilters();
	void LoadToolsFromDataTable();
	void CreateToolWidget(const FAdvancedToolInfo& ToolInfo);
	void CalculateOptimalRack(const FString& RouteGrade, const FString& RouteStyle);
	EToolPlacementQuality AnalyzePlacementQuality(const FAdvancedToolInfo& Tool, const FVector& Location);
	FLinearColor GetConditionColor(float Durability) const;
	bool ShouldShowTool(const FAdvancedToolInfo& Tool) const;
};