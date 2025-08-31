#include "SettingsWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/VerticalBox.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "AudioDevice.h"

USettingsWidget::USettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentCategory = ESettingsCategory::Gameplay;
	bHasUnsavedChanges = false;
	AwaitingKeyBindAction = TEXT("");
}

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind navigation buttons
	if (GameplayButton)
	{
		GameplayButton->OnClicked.AddDynamic(this, &USettingsWidget::OnGameplayButtonClicked);
	}
	if (ControlsButton)
	{
		ControlsButton->OnClicked.AddDynamic(this, &USettingsWidget::OnControlsButtonClicked);
	}
	if (AudioButton)
	{
		AudioButton->OnClicked.AddDynamic(this, &USettingsWidget::OnAudioButtonClicked);
	}
	if (VideoButton)
	{
		VideoButton->OnClicked.AddDynamic(this, &USettingsWidget::OnVideoButtonClicked);
	}
	if (AccessibilityButton)
	{
		AccessibilityButton->OnClicked.AddDynamic(this, &USettingsWidget::OnAccessibilityButtonClicked);
	}
	if (MultiplayerButton)
	{
		MultiplayerButton->OnClicked.AddDynamic(this, &USettingsWidget::OnMultiplayerButtonClicked);
	}

	// Bind action buttons
	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &USettingsWidget::OnApplyButtonClicked);
	}
	if (SaveButton)
	{
		SaveButton->OnClicked.AddDynamic(this, &USettingsWidget::OnSaveButtonClicked);
	}
	if (ResetButton)
	{
		ResetButton->OnClicked.AddDynamic(this, &USettingsWidget::OnResetButtonClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &USettingsWidget::OnCancelButtonClicked);
	}

	// Initialize settings
	InitializeSettings();
	LoadCurrentSettings();
	ShowSettingsCategory(ESettingsCategory::Gameplay);
}

void USettingsWidget::ShowSettingsCategory(ESettingsCategory Category)
{
	CurrentCategory = Category;
	
	if (CategorySwitcher)
	{
		CategorySwitcher->SetActiveWidgetIndex(static_cast<int32>(Category));
	}

	UpdateCategoryButtonStyles();
}

void USettingsWidget::ApplySettings()
{
	// Apply all current settings without saving to disk
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->ApplySettings(false); // Don't save to config yet
	}

	UE_LOG(LogTemp, Log, TEXT("Settings applied"));
}

void USettingsWidget::SaveSettings()
{
	// Apply and save settings to disk
	ApplySettings();
	SaveSettingsToConfig();
	
	bHasUnsavedChanges = false;
	UpdateButtonStates();

	UE_LOG(LogTemp, Log, TEXT("Settings saved"));
}

void USettingsWidget::ResetToDefaults()
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->ResetToCurrentSettings();
		LoadCurrentSettings(); // Refresh UI
	}

	UE_LOG(LogTemp, Log, TEXT("Settings reset to defaults"));
}

void USettingsWidget::CancelChanges()
{
	// Reload settings from config, discarding changes
	LoadCurrentSettings();
	bHasUnsavedChanges = false;
	UpdateButtonStates();
}

// Gameplay Settings
void USettingsWidget::SetDifficultyLevel(int32 Level)
{
	if (DifficultyComboBox)
	{
		DifficultyComboBox->SetSelectedIndex(Level);
	}
	
	OnSettingChanged.Broadcast(FName("DifficultyLevel"), FString::FromInt(Level));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetClimbingAssistance(bool bEnabled)
{
	if (ClimbingAssistanceCheckBox)
	{
		ClimbingAssistanceCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("ClimbingAssistance"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetFallDamage(bool bEnabled)
{
	if (FallDamageCheckBox)
	{
		FallDamageCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("FallDamage"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetStaminaDepletion(float Rate)
{
	if (StaminaDepletionSlider)
	{
		StaminaDepletionSlider->SetValue(Rate);
	}
	
	OnSettingChanged.Broadcast(FName("StaminaDepletion"), FString::SanitizeFloat(Rate));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

// Control Settings
void USettingsWidget::SetMouseSensitivity(float Sensitivity)
{
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetValue(Sensitivity);
	}
	
	OnSettingChanged.Broadcast(FName("MouseSensitivity"), FString::SanitizeFloat(Sensitivity));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetControllerSensitivity(float Sensitivity)
{
	if (ControllerSensitivitySlider)
	{
		ControllerSensitivitySlider->SetValue(Sensitivity);
	}
	
	OnSettingChanged.Broadcast(FName("ControllerSensitivity"), FString::SanitizeFloat(Sensitivity));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetInvertMouseY(bool bInvert)
{
	if (InvertMouseYCheckBox)
	{
		InvertMouseYCheckBox->SetIsChecked(bInvert);
	}
	
	OnSettingChanged.Broadcast(FName("InvertMouseY"), bInvert ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::BeginKeyBinding(const FString& ActionName)
{
	AwaitingKeyBindAction = ActionName;
	// In a full implementation, this would show a "Press any key" dialog
	UE_LOG(LogTemp, Log, TEXT("Waiting for key binding for action: %s"), *ActionName);
}

void USettingsWidget::SetKeyBinding(const FString& ActionName, FKey NewKey, bool bIsPrimary)
{
	// Update key binding in input settings
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (InputSettings)
	{
		// This would involve modifying the input action mappings
		UE_LOG(LogTemp, Log, TEXT("Set key binding for %s to %s"), *ActionName, *NewKey.ToString());
	}
	
	OnSettingChanged.Broadcast(FName(*ActionName), NewKey.ToString());
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

// Audio Settings
void USettingsWidget::SetMasterVolume(float Volume)
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(Volume);
	}
	
	// Apply to audio system
	if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice())
	{
		AudioDevice->SetTransientMasterVolume(Volume);
	}
	
	OnSettingChanged.Broadcast(FName("MasterVolume"), FString::SanitizeFloat(Volume));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetMusicVolume(float Volume)
{
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(Volume);
	}
	
	OnSettingChanged.Broadcast(FName("MusicVolume"), FString::SanitizeFloat(Volume));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetSFXVolume(float Volume)
{
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(Volume);
	}
	
	OnSettingChanged.Broadcast(FName("SFXVolume"), FString::SanitizeFloat(Volume));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetVoiceChatVolume(float Volume)
{
	if (VoiceChatVolumeSlider)
	{
		VoiceChatVolumeSlider->SetValue(Volume);
	}
	
	OnSettingChanged.Broadcast(FName("VoiceChatVolume"), FString::SanitizeFloat(Volume));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetAudioDescription(bool bEnabled)
{
	if (AudioDescriptionCheckBox)
	{
		AudioDescriptionCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("AudioDescription"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

// Video Settings
void USettingsWidget::SetResolution(const FString& Resolution)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings && ResolutionComboBox)
	{
		ResolutionComboBox->SetSelectedOption(Resolution);
		
		// Parse resolution string (e.g., "1920x1080")
		FString Width, Height;
		if (Resolution.Split(TEXT("x"), &Width, &Height))
		{
			FIntPoint NewResolution(FCString::Atoi(*Width), FCString::Atoi(*Height));
			GameUserSettings->SetScreenResolution(NewResolution);
		}
	}
	
	OnSettingChanged.Broadcast(FName("Resolution"), Resolution);
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetFullscreenMode(int32 Mode)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->SetFullscreenMode(static_cast<EWindowMode::Type>(Mode));
	}
	
	if (FullscreenModeComboBox)
	{
		FullscreenModeComboBox->SetSelectedIndex(Mode);
	}
	
	OnSettingChanged.Broadcast(FName("FullscreenMode"), FString::FromInt(Mode));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetFrameRateLimit(int32 FPS)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->SetFrameRateLimit(static_cast<float>(FPS));
	}
	
	OnSettingChanged.Broadcast(FName("FrameRateLimit"), FString::FromInt(FPS));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetVSync(bool bEnabled)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->SetVSyncEnabled(bEnabled);
	}
	
	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("VSync"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetGraphicsQuality(int32 Quality)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->SetOverallScalabilityLevel(Quality);
	}
	
	if (GraphicsQualityComboBox)
	{
		GraphicsQualityComboBox->SetSelectedIndex(Quality);
	}
	
	OnSettingChanged.Broadcast(FName("GraphicsQuality"), FString::FromInt(Quality));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

// Accessibility Settings
void USettingsWidget::SetUIScale(float Scale)
{
	if (UIScaleSlider)
	{
		UIScaleSlider->SetValue(Scale);
	}
	
	OnSettingChanged.Broadcast(FName("UIScale"), FString::SanitizeFloat(Scale));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetFontSize(int32 Size)
{
	if (FontSizeComboBox)
	{
		FontSizeComboBox->SetSelectedIndex(Size);
	}
	
	OnSettingChanged.Broadcast(FName("FontSize"), FString::FromInt(Size));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetHighContrastMode(bool bEnabled)
{
	if (HighContrastCheckBox)
	{
		HighContrastCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("HighContrast"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetColorBlindMode(int32 Mode)
{
	if (ColorBlindModeComboBox)
	{
		ColorBlindModeComboBox->SetSelectedIndex(Mode);
	}
	
	OnSettingChanged.Broadcast(FName("ColorBlindMode"), FString::FromInt(Mode));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetSubtitlesEnabled(bool bEnabled)
{
	if (SubtitlesCheckBox)
	{
		SubtitlesCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("Subtitles"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetMotionSickness(bool bReduceMotion)
{
	if (MotionSicknessCheckBox)
	{
		MotionSicknessCheckBox->SetIsChecked(bReduceMotion);
	}
	
	OnSettingChanged.Broadcast(FName("ReduceMotion"), bReduceMotion ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetScreenReader(bool bEnabled)
{
	if (ScreenReaderCheckBox)
	{
		ScreenReaderCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("ScreenReader"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

// Multiplayer Settings
void USettingsWidget::SetPlayerName(const FString& Name)
{
	// In a real implementation, this would update an EditableTextBox
	OnSettingChanged.Broadcast(FName("PlayerName"), Name);
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetProximityChatEnabled(bool bEnabled)
{
	if (ProximityChatCheckBox)
	{
		ProximityChatCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("ProximityChat"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

void USettingsWidget::SetCrossplatformPlay(bool bEnabled)
{
	if (CrossplatformCheckBox)
	{
		CrossplatformCheckBox->SetIsChecked(bEnabled);
	}
	
	OnSettingChanged.Broadcast(FName("Crossplatform"), bEnabled ? TEXT("true") : TEXT("false"));
	bHasUnsavedChanges = true;
	UpdateButtonStates();
}

// Event handlers
void USettingsWidget::OnGameplayButtonClicked()
{
	ShowSettingsCategory(ESettingsCategory::Gameplay);
}

void USettingsWidget::OnControlsButtonClicked()
{
	ShowSettingsCategory(ESettingsCategory::Controls);
}

void USettingsWidget::OnAudioButtonClicked()
{
	ShowSettingsCategory(ESettingsCategory::Audio);
}

void USettingsWidget::OnVideoButtonClicked()
{
	ShowSettingsCategory(ESettingsCategory::Video);
}

void USettingsWidget::OnAccessibilityButtonClicked()
{
	ShowSettingsCategory(ESettingsCategory::Accessibility);
}

void USettingsWidget::OnMultiplayerButtonClicked()
{
	ShowSettingsCategory(ESettingsCategory::Multiplayer);
}

void USettingsWidget::OnApplyButtonClicked()
{
	ApplySettings();
}

void USettingsWidget::OnSaveButtonClicked()
{
	SaveSettings();
}

void USettingsWidget::OnResetButtonClicked()
{
	ResetToDefaults();
}

void USettingsWidget::OnCancelButtonClicked()
{
	CancelChanges();
}

void USettingsWidget::InitializeSettings()
{
	PopulateComboBoxes();
	PopulateKeyBindings();
}

void USettingsWidget::LoadCurrentSettings()
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (!GameUserSettings) return;

	// Load video settings
	if (ResolutionComboBox)
	{
		FIntPoint Resolution = GameUserSettings->GetScreenResolution();
		FString ResolutionString = FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y);
		ResolutionComboBox->SetSelectedOption(ResolutionString);
	}

	if (FullscreenModeComboBox)
	{
		FullscreenModeComboBox->SetSelectedIndex(static_cast<int32>(GameUserSettings->GetFullscreenMode()));
	}

	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(GameUserSettings->IsVSyncEnabled());
	}

	if (GraphicsQualityComboBox)
	{
		GraphicsQualityComboBox->SetSelectedIndex(GameUserSettings->GetOverallScalabilityLevel());
	}

	// Load custom settings from config
	LoadSettingsFromConfig();
}

void USettingsWidget::UpdateButtonStates()
{
	if (ApplyButton)
	{
		ApplyButton->SetIsEnabled(bHasUnsavedChanges);
	}
	
	if (SaveButton)
	{
		SaveButton->SetIsEnabled(bHasUnsavedChanges);
	}
}

void USettingsWidget::PopulateKeyBindings()
{
	if (!KeyBindingsContainer || !KeyBindingWidgetClass) return;

	// This would populate key binding widgets based on input settings
	// Implementation would depend on your input action setup
	UE_LOG(LogTemp, Log, TEXT("Populating key bindings"));
}

void USettingsWidget::PopulateComboBoxes()
{
	// Resolution options
	if (ResolutionComboBox)
	{
		ResolutionComboBox->AddOption(TEXT("1280x720"));
		ResolutionComboBox->AddOption(TEXT("1920x1080"));
		ResolutionComboBox->AddOption(TEXT("2560x1440"));
		ResolutionComboBox->AddOption(TEXT("3840x2160"));
	}

	// Fullscreen mode options
	if (FullscreenModeComboBox)
	{
		FullscreenModeComboBox->AddOption(TEXT("Windowed"));
		FullscreenModeComboBox->AddOption(TEXT("Windowed Fullscreen"));
		FullscreenModeComboBox->AddOption(TEXT("Fullscreen"));
	}

	// Graphics quality options
	if (GraphicsQualityComboBox)
	{
		GraphicsQualityComboBox->AddOption(TEXT("Low"));
		GraphicsQualityComboBox->AddOption(TEXT("Medium"));
		GraphicsQualityComboBox->AddOption(TEXT("High"));
		GraphicsQualityComboBox->AddOption(TEXT("Epic"));
	}

	// Difficulty options
	if (DifficultyComboBox)
	{
		DifficultyComboBox->AddOption(TEXT("Easy"));
		DifficultyComboBox->AddOption(TEXT("Normal"));
		DifficultyComboBox->AddOption(TEXT("Hard"));
		DifficultyComboBox->AddOption(TEXT("Extreme"));
	}

	// Font size options
	if (FontSizeComboBox)
	{
		FontSizeComboBox->AddOption(TEXT("Small"));
		FontSizeComboBox->AddOption(TEXT("Normal"));
		FontSizeComboBox->AddOption(TEXT("Large"));
		FontSizeComboBox->AddOption(TEXT("Extra Large"));
	}

	// Color blind mode options
	if (ColorBlindModeComboBox)
	{
		ColorBlindModeComboBox->AddOption(TEXT("None"));
		ColorBlindModeComboBox->AddOption(TEXT("Protanopia"));
		ColorBlindModeComboBox->AddOption(TEXT("Deuteranopia"));
		ColorBlindModeComboBox->AddOption(TEXT("Tritanopia"));
	}
}

void USettingsWidget::UpdateCategoryButtonStyles()
{
	// Update button appearances to show current selection
	// This would typically be handled through styling or materials
}

void USettingsWidget::LoadSettingsFromConfig()
{
	// Load custom settings from game config files
	// This would use GConfig to read from GameUserSettings.ini
	UE_LOG(LogTemp, Log, TEXT("Loading settings from config"));
}

void USettingsWidget::SaveSettingsToConfig()
{
	// Save custom settings to config files
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GameUserSettings)
	{
		GameUserSettings->SaveSettings();
	}
	
	UE_LOG(LogTemp, Log, TEXT("Settings saved to config"));
}