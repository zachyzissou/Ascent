#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/EditableTextBox.h"
#include "Engine/DataTable.h"
#include "AccessibilityWidget.generated.h"

UENUM(BlueprintType)
enum class EAccessibilityFeature : uint8
{
	VisualAssist,      // Visual impairment assistance
	HearingAssist,     // Hearing impairment assistance  
	MotorAssist,       // Motor impairment assistance
	CognitiveAssist,   // Cognitive assistance
	MotionComfort,     // Motion sickness/comfort
	CustomControls,    // Customizable control schemes
	ColorAdjustments,  // Color vision adjustments
	TextAdjustments    // Text/UI scaling
};

UENUM(BlueprintType)
enum class EColorBlindType : uint8
{
	None,             // No color vision deficiency
	Protanopia,       // Red-blind
	Protanomaly,      // Red-weak
	Deuteranopia,     // Green-blind
	Deuteranomaly,    // Green-weak
	Tritanopia,       // Blue-blind
	Tritanomaly,      // Blue-weak
	Monochromacy      // Complete color blindness
};

UENUM(BlueprintType)
enum class EMotionComfortLevel : uint8
{
	None,            // No motion comfort features
	Minimal,         // Basic comfort features
	Moderate,        // Standard comfort package
	Maximum,         // All comfort features enabled
	Custom           // User-defined settings
};

USTRUCT(BlueprintType)
struct FAccessibilityProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ProfileName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<EAccessibilityFeature, bool> EnabledFeatures;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float UIScale;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float FontSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ContrastLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EColorBlindType ColorBlindMode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EMotionComfortLevel MotionComfort;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHighContrastMode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bScreenReaderSupport;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSubtitlesEnabled;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAudioDescriptions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AudioCueVolume;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVibrationFeedback;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float HoldTime; // Time required to hold buttons

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bStickyKeys;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSlowKeys;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RepeatRate;

	FAccessibilityProfile()
	{
		ProfileName = TEXT("Default");
		UIScale = 1.0f;
		FontSize = 16.0f;
		ContrastLevel = 1.0f;
		ColorBlindMode = EColorBlindType::None;
		MotionComfort = EMotionComfortLevel::None;
		bHighContrastMode = false;
		bScreenReaderSupport = false;
		bSubtitlesEnabled = false;
		bAudioDescriptions = false;
		AudioCueVolume = 1.0f;
		bVibrationFeedback = false;
		HoldTime = 0.5f;
		bStickyKeys = false;
		bSlowKeys = false;
		RepeatRate = 1.0f;
	}
};

USTRUCT(BlueprintType)
struct FControlCustomization
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ActionName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FKey> PrimaryKeys;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FKey> SecondaryKeys;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresHold;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float HoldDuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAllowRepeat;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RepeatDelay;

	FControlCustomization()
	{
		ActionName = NAME_None;
		bRequiresHold = false;
		HoldDuration = 0.5f;
		bAllowRepeat = false;
		RepeatDelay = 0.3f;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAccessibilityProfileChanged, const FAccessibilityProfile&, NewProfile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAccessibilityFeatureToggled, EAccessibilityFeature, Feature, bool, bEnabled);

/**
 * Comprehensive accessibility widget for climbing game
 * Provides extensive customization for players with different needs
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UAccessibilityWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAccessibilityWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

public:
	// Profile management
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Profile")
	void CreateAccessibilityProfile(const FString& ProfileName);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Profile")
	void LoadAccessibilityProfile(const FString& ProfileName);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Profile")
	void SaveCurrentProfile();

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Profile")
	void DeleteProfile(const FString& ProfileName);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Profile")
	void ApplyQuickSetupPreset(const FString& PresetName);

	// Visual accessibility
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Visual")
	void SetUIScale(float Scale);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Visual")
	void SetFontSize(float Size);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Visual")
	void SetContrastLevel(float Contrast);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Visual")
	void SetColorBlindMode(EColorBlindType ColorBlindMode);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Visual")
	void ToggleHighContrastMode(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Visual")
	void ConfigureColorFilters(const FLinearColor& FilterColor, float Intensity);

	// Audio accessibility
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Audio")
	void ToggleSubtitles(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Audio")
	void ToggleAudioDescriptions(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Audio")
	void SetAudioCueVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Audio")
	void ConfigureAudioChannelRouting(bool bMonoAudio);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Audio")
	void SetupCustomAudioCues(const TMap<FString, class USoundBase*>& CustomCues);

	// Motor accessibility
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motor")
	void ConfigureHoldTimeSettings(float HoldTime);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motor")
	void ToggleStickyKeys(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motor")
	void ToggleSlowKeys(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motor")
	void SetRepeatRate(float Rate);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motor")
	void ConfigureCustomControls(const TArray<FControlCustomization>& Customizations);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motor")
	void EnableOneHandedMode(bool bEnabled, bool bLeftHanded = false);

	// Motion comfort
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motion")
	void SetMotionComfortLevel(EMotionComfortLevel ComfortLevel);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motion")
	void ToggleReduceMotion(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motion")
	void SetFieldOfViewReduction(float FOVReduction);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motion")
	void ConfigureComfortVignette(bool bEnabled, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Motion")
	void SetTeleportMovementOnly(bool bEnabled);

	// Cognitive assistance
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Cognitive")
	void ToggleSimplifiedUI(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Cognitive")
	void SetInstructionComplexity(int32 ComplexityLevel);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Cognitive")
	void ToggleAutoAdvance(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Cognitive")
	void ConfigureMemoryAssists(bool bShowReminders, float ReminderFrequency);

	// Testing and validation
	UFUNCTION(BlueprintCallable, Category = "Accessibility|Testing")
	void RunAccessibilityTest();

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Testing")
	void PreviewColorBlindMode(EColorBlindType TestMode);

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Testing")
	void TestMotionComfort();

	UFUNCTION(BlueprintCallable, Category = "Accessibility|Testing")
	void ValidateControlCustomizations();

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Accessibility")
	FOnAccessibilityProfileChanged OnProfileChanged;

	UPROPERTY(BlueprintAssignable, Category = "Accessibility")
	FOnAccessibilityFeatureToggled OnFeatureToggled;

protected:
	// Main UI components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> AccessibilityCategorySwitcher;

	// Profile management
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ProfileDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateProfileButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SaveProfileButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DeleteProfileButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> NewProfileNameBox;

	// Quick setup presets
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> QuickSetupContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> VisionAssistPreset;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HearingAssistPreset;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MotorAssistPreset;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MotionComfortPreset;

	// Visual accessibility controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> UIScaleSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> FontSizeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> ContrastSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ColorBlindModeDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> HighContrastCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ColorBlindTestImage;

	// Audio accessibility controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> SubtitlesCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> AudioDescriptionCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> AudioCueVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> MonoAudioCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TestAudioCuesButton;

	// Motor accessibility controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> HoldTimeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> StickyKeysCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> SlowKeysCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> RepeatRateSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> OneHandedModeCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> LeftHandedCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CustomizeControlsButton;

	// Motion comfort controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> MotionComfortDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> ReduceMotionCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> FOVReductionSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> ComfortVignetteCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> VignetteIntensitySlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> TeleportOnlyCheckBox;

	// Cognitive assistance controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> SimplifiedUICheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> InstructionComplexitySlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> AutoAdvanceCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> MemoryAssistsCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> ReminderFrequencySlider;

	// Testing controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RunAccessibilityTestButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TestColorBlindButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TestMotionComfortButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ValidateControlsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> TestResultsScrollBox;

	// Category navigation buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> VisualCategoryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AudioCategoryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MotorCategoryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MotionCategoryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CognitiveCategoryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TestingCategoryButton;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accessibility Config")
	TSubclassOf<UUserWidget> ControlCustomizationWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accessibility Config")
	TObjectPtr<UDataTable> AccessibilityPresetsTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Accessibility Config")
	TMap<EColorBlindType, FLinearColor> ColorBlindFilters;

private:
	// Current state
	FAccessibilityProfile CurrentProfile;
	TArray<FAccessibilityProfile> SavedProfiles;
	TArray<FControlCustomization> CustomControls;
	bool bTestingMode;
	EAccessibilityFeature CurrentCategory;

	// Event handlers
	UFUNCTION()
	void OnCreateProfileClicked();

	UFUNCTION()
	void OnSaveProfileClicked();

	UFUNCTION()
	void OnDeleteProfileClicked();

	UFUNCTION()
	void OnProfileChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnVisionAssistPresetClicked();

	UFUNCTION()
	void OnHearingAssistPresetClicked();

	UFUNCTION()
	void OnMotorAssistPresetClicked();

	UFUNCTION()
	void OnMotionComfortPresetClicked();

	UFUNCTION()
	void OnUIScaleChanged(float Value);

	UFUNCTION()
	void OnFontSizeChanged(float Value);

	UFUNCTION()
	void OnContrastChanged(float Value);

	UFUNCTION()
	void OnColorBlindModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnHighContrastToggled(bool bIsChecked);

	UFUNCTION()
	void OnSubtitlesToggled(bool bIsChecked);

	UFUNCTION()
	void OnAudioDescriptionToggled(bool bIsChecked);

	UFUNCTION()
	void OnCustomizeControlsClicked();

	UFUNCTION()
	void OnRunAccessibilityTestClicked();

	UFUNCTION()
	void OnTestColorBlindClicked();

	UFUNCTION()
	void OnTestMotionComfortClicked();

	// Helper functions
	void LoadSavedProfiles();
	void PopulateProfileDropdown();
	void UpdateUIFromProfile();
	void ApplyCurrentProfile();
	void LoadAccessibilityPresets();
	void CreateQuickSetupPresets();
	void UpdateCategoryButtonStyles();
	void RunColorBlindnessTest();
	void RunMotionComfortTest();
	void GenerateAccessibilityReport();
	void ApplyColorFilters();
	void ConfigureAudioRouting();
	void SetupCustomControlScheme();
	void ValidateAccessibilityCompliance();
};