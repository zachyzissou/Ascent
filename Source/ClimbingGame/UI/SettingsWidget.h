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
#include "GameFramework/InputSettings.h"
#include "SettingsWidget.generated.h"

UENUM(BlueprintType)
enum class ESettingsCategory : uint8
{
	Gameplay,
	Controls,
	Audio,
	Video,
	Accessibility,
	Multiplayer
};

USTRUCT(BlueprintType)
struct FKeyBindingInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ActionName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKey PrimaryKey;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKey SecondaryKey;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsAxisMapping;

	FKeyBindingInfo()
	{
		ActionName = TEXT("");
		DisplayName = TEXT("");
		PrimaryKey = EKeys::Invalid;
		SecondaryKey = EKeys::Invalid;
		bIsAxisMapping = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSettingChanged, FName, SettingName, FString, NewValue);

/**
 * Comprehensive settings widget with accessibility features
 * Handles gameplay, controls, audio, video, and accessibility options
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USettingsWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

public:
	// Settings navigation
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ShowSettingsCategory(ESettingsCategory Category);

	// Apply/Save settings
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetToDefaults();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void CancelChanges();

	// Gameplay settings
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetDifficultyLevel(int32 Level);

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetClimbingAssistance(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetFallDamage(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetStaminaDepletion(float Rate);

	// Control settings
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float Sensitivity);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetControllerSensitivity(float Sensitivity);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertMouseY(bool bInvert);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void BeginKeyBinding(const FString& ActionName);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetKeyBinding(const FString& ActionName, FKey NewKey, bool bIsPrimary = true);

	// Audio settings
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetVoiceChatVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetAudioDescription(bool bEnabled);

	// Video settings
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetResolution(const FString& Resolution);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetFullscreenMode(int32 Mode);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetFrameRateLimit(int32 FPS);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetVSync(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetGraphicsQuality(int32 Quality);

	// Accessibility settings
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetUIScale(float Scale);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetFontSize(int32 Size);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetHighContrastMode(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetColorBlindMode(int32 Mode);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetSubtitlesEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetMotionSickness(bool bReduceMotion);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetScreenReader(bool bEnabled);

	// Multiplayer settings
	UFUNCTION(BlueprintCallable, Category = "Settings|Multiplayer")
	void SetPlayerName(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Settings|Multiplayer")
	void SetProximityChatEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Multiplayer")
	void SetCrossplatformPlay(bool bEnabled);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnSettingChanged OnSettingChanged;

protected:
	// Main UI components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> CategorySwitcher;

	// Category navigation buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GameplayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ControlsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AudioButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> VideoButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AccessibilityButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MultiplayerButton;

	// Action buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

	// Gameplay settings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> DifficultyComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> ClimbingAssistanceCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> FallDamageCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> StaminaDepletionSlider;

	// Control settings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> ControllerSensitivitySlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> InvertMouseYCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> KeyBindingsContainer;

	// Audio settings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> VoiceChatVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> AudioDescriptionCheckBox;

	// Video settings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ResolutionComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> FullscreenModeComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> FrameRateLimitComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> VSyncCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> GraphicsQualityComboBox;

	// Accessibility settings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> UIScaleSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> FontSizeComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> HighContrastCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ColorBlindModeComboBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> SubtitlesCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> MotionSicknessCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> ScreenReaderCheckBox;

	// Multiplayer settings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameTextBox; // Would be EditableTextBox in Blueprint

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> ProximityChatCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CrossplatformCheckBox;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings Config")
	TSubclassOf<UUserWidget> KeyBindingWidgetClass;

private:
	// Current state
	ESettingsCategory CurrentCategory;
	TArray<FKeyBindingInfo> KeyBindings;
	bool bHasUnsavedChanges;
	FString AwaitingKeyBindAction;

	// Event handlers
	UFUNCTION()
	void OnGameplayButtonClicked();

	UFUNCTION()
	void OnControlsButtonClicked();

	UFUNCTION()
	void OnAudioButtonClicked();

	UFUNCTION()
	void OnVideoButtonClicked();

	UFUNCTION()
	void OnAccessibilityButtonClicked();

	UFUNCTION()
	void OnMultiplayerButtonClicked();

	UFUNCTION()
	void OnApplyButtonClicked();

	UFUNCTION()
	void OnSaveButtonClicked();

	UFUNCTION()
	void OnResetButtonClicked();

	UFUNCTION()
	void OnCancelButtonClicked();

	// Helper functions
	void InitializeSettings();
	void LoadCurrentSettings();
	void UpdateButtonStates();
	void PopulateKeyBindings();
	void PopulateComboBoxes();
	void UpdateCategoryButtonStyles();

	// Settings persistence
	void LoadSettingsFromConfig();
	void SaveSettingsToConfig();
};