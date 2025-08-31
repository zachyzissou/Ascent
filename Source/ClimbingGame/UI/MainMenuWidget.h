#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ProgressBar.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/RichTextBlock.h"
#include "Engine/DataTable.h"
#include "MainMenuWidget.generated.h"

UENUM(BlueprintType)
enum class EMenuState : uint8
{
	MainMenu,          // Primary menu
	PlayModes,         // Game mode selection
	MultiplayerLobby,  // Multiplayer setup
	Settings,          // Settings and options
	Tutorials,         // Learning and tutorials
	Community,         // Community features
	Progression,       // Player progression and achievements
	Credits,           // Game credits and acknowledgments
	Loading,           // Loading screens
	Quit               // Exit confirmation
};

UENUM(BlueprintType)
enum class EGameMode : uint8
{
	SinglePlayer,      // Solo climbing experience
	Cooperative,       // Co-op with friends
	Competitive,       // Competitive climbing
	Training,          // Practice and skill building
	FreeClimb,        // Open exploration
	Challenge,         // Specific challenges/routes
	Tutorial,          // Guided learning
	Sandbox           // Creative/custom modes
};

USTRUCT(BlueprintType)
struct FRouteInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName RouteID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText RouteName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DifficultyGrade;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RouteType; // Sport, Trad, Boulder, etc.

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float EstimatedTime; // Hours

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RecommendedPlayerCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> RequiredEquipment;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* RouteImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsCompleted;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BestTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 AttemptCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PersonalRating; // 1-5 stars

	FRouteInfo()
	{
		RouteID = NAME_None;
		RouteName = FText::GetEmpty();
		Description = FText::GetEmpty();
		DifficultyGrade = TEXT("5.6");
		RouteType = TEXT("Sport");
		Location = TEXT("Unknown");
		EstimatedTime = 2.0f;
		RecommendedPlayerCount = 1;
		RouteImage = nullptr;
		bIsCompleted = false;
		BestTime = 0.0f;
		AttemptCount = 0;
		PersonalRating = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FPlayerProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SkillLevel; // 1-10

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TotalClimbingTime; // Hours

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RoutesCompleted;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HighestGradeClimbed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> Achievements;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FString, int32> Statistics;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* ProfilePicture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDateTime LastPlayed;

	FPlayerProfile()
	{
		PlayerName = TEXT("Climber");
		SkillLevel = 1;
		TotalClimbingTime = 0.0f;
		RoutesCompleted = 0;
		HighestGradeClimbed = TEXT("5.0");
		ProfilePicture = nullptr;
		LastPlayed = FDateTime::Now();
	}
};

USTRUCT(BlueprintType)
struct FMultiplayerSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SessionName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HostPlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CurrentPlayers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxPlayers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SelectedRoute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EGameMode GameMode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasPassword;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Ping;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bInProgress;

	FMultiplayerSession()
	{
		SessionName = TEXT("Climbing Session");
		HostPlayerName = TEXT("Unknown");
		CurrentPlayers = 1;
		MaxPlayers = 4;
		SelectedRoute = TEXT("Training Route");
		GameMode = EGameMode::Cooperative;
		bHasPassword = false;
		Ping = 0.0f;
		bInProgress = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuStateChanged, EMenuState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameModeSelected, EGameMode, GameMode, const FRouteInfo&, SelectedRoute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerSessionJoin, const FMultiplayerSession&, Session);

/**
 * Comprehensive main menu widget with full navigation and game setup
 * Handles all menu interactions, route selection, and multiplayer lobby
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Menu navigation
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void SetMenuState(EMenuState NewState);

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ReturnToPreviousMenu();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowConfirmationDialog(const FText& Message, FName ConfirmAction);

	// Game mode selection
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Game")
	void SelectGameMode(EGameMode GameMode);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Game")
	void SelectRoute(const FName& RouteID);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Game")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Game")
	void LoadQuickPlay();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Game")
	void ContinueLastSession();

	// Route management
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Routes")
	void RefreshRouteList();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Routes")
	void FilterRoutesByDifficulty(const FString& MinGrade, const FString& MaxGrade);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Routes")
	void FilterRoutesByType(const FString& RouteType);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Routes")
	void SearchRoutes(const FString& SearchTerm);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Routes")
	void ShowRouteDetails(const FName& RouteID);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Routes")
	void RateRoute(const FName& RouteID, float Rating);

	// Multiplayer functionality
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Multiplayer")
	void CreateMultiplayerSession(const FString& SessionName, int32 MaxPlayers, bool bUsePassword);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Multiplayer")
	void JoinMultiplayerSession(const FMultiplayerSession& Session, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Multiplayer")
	void RefreshServerList();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Multiplayer")
	void LeaveMultiplayerSession();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Multiplayer")
	void InviteFriend(const FString& FriendName);

	// Player profile
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Profile")
	void UpdatePlayerProfile(const FPlayerProfile& NewProfile);

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Profile")
	void ShowPlayerStatistics();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Profile")
	void ShowAchievements();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Profile")
	void ExportProgress();

	// Settings integration
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Settings")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Settings")
	void ApplyQuickSettings(const FString& PresetName);

	// Community features
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Community")
	void ShowLeaderboards();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Community")
	void ShowCommunityRoutes();

	UFUNCTION(BlueprintCallable, Category = "Main Menu|Community")
	void ShareAchievement(const FString& AchievementName);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Main Menu")
	FOnMenuStateChanged OnMenuStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Main Menu")
	FOnGameModeSelected OnGameModeSelected;

	UPROPERTY(BlueprintAssignable, Category = "Main Menu")
	FOnMultiplayerSessionJoin OnMultiplayerSessionJoin;

protected:
	// Main navigation
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> MainMenuSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> BackgroundContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BackgroundImage;

	// Main menu buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MultiplayerButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TutorialsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CommunityButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ProgressionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	// Game mode selection
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> GameModeContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SinglePlayerButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CooperativeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CompetitiveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TrainingButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FreeClimbButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ChallengeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuickPlayButton;

	// Route selection
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> RouteListScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> RouteListContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> RouteDetailsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RouteNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RouteDifficultyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> RouteDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RoutePreviewImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> RouteStatsContainer;

	// Route filtering
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> RouteSearchBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> DifficultyFilterDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> TypeFilterDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CompletedOnlyCheckBox;

	// Multiplayer lobby
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ServerListScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ServerListContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateSessionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RefreshServersButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SessionNameTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MaxPlayersSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> PrivateSessionCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SessionPasswordTextBox;

	// Player profile display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> PlayerProfilePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillLevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ExperienceBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ProfilePictureImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ViewStatsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ViewAchievementsButton;

	// Loading screen
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> LoadingPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LoadingProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LoadingStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> LoadingTipsText;

	// Confirmation dialog
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ConfirmationDialog;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ConfirmationMessageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

	// Back navigation
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu Config")
	TObjectPtr<UDataTable> RoutesDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu Config")
	TSubclassOf<UUserWidget> RouteWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu Config")
	TSubclassOf<UUserWidget> ServerWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu Config")
	TArray<UTexture2D*> BackgroundImages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu Config")
	float BackgroundTransitionTime = 30.0f;

private:
	// Current state
	EMenuState CurrentMenuState;
	TArray<EMenuState> MenuHistory;
	EGameMode SelectedGameMode;
	FRouteInfo SelectedRoute;
	FPlayerProfile CurrentPlayerProfile;
	FMultiplayerSession CurrentSession;

	// Route filtering
	TArray<FRouteInfo> AllRoutes;
	TArray<FRouteInfo> FilteredRoutes;
	FString CurrentSearchTerm;
	FString CurrentDifficultyFilter;
	FString CurrentTypeFilter;
	bool bShowCompletedOnly;

	// Multiplayer
	TArray<FMultiplayerSession> AvailableSessions;
	bool bIsHosting;

	// Background management
	int32 CurrentBackgroundIndex;
	float BackgroundTimer;

	// Confirmation dialog
	FName PendingConfirmAction;

	// Event handlers
	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void OnMultiplayerClicked();

	UFUNCTION()
	void OnTutorialsClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnCommunityClicked();

	UFUNCTION()
	void OnProgressionClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnSinglePlayerClicked();

	UFUNCTION()
	void OnCooperativeClicked();

	UFUNCTION()
	void OnCompetitiveClicked();

	UFUNCTION()
	void OnTrainingClicked();

	UFUNCTION()
	void OnFreeClimbClicked();

	UFUNCTION()
	void OnChallengeClicked();

	UFUNCTION()
	void OnQuickPlayClicked();

	UFUNCTION()
	void OnCreateSessionClicked();

	UFUNCTION()
	void OnRefreshServersClicked();

	UFUNCTION()
	void OnViewStatsClicked();

	UFUNCTION()
	void OnViewAchievementsClicked();

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	// Helper functions
	void LoadPlayerProfile();
	void LoadAvailableRoutes();
	void UpdateRouteDisplay();
	void ApplyRouteFilters();
	void CreateRouteWidget(const FRouteInfo& RouteInfo);
	void UpdateMultiplayerDisplay();
	void CreateServerWidget(const FMultiplayerSession& Session);
	void UpdateBackgroundImage();
	void ShowLoadingScreen(const FText& StatusText);
	void HideLoadingScreen();
	void ProcessConfirmationAction();
	void NavigateToMenu(EMenuState NewState);
	void SaveMenuPreferences();
	void LoadMenuPreferences();
};