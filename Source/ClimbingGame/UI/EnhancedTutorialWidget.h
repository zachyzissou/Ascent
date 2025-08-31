#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/RichTextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/MediaPlayer.h"
#include "MediaTexture.h"
#include "Engine/DataTable.h"
#include "EnhancedTutorialWidget.generated.h"

UENUM(BlueprintType)
enum class ELearningStyle : uint8
{
	Visual,           // Learn through images and diagrams
	Auditory,        // Learn through audio explanations
	Kinesthetic,     // Learn through hands-on practice
	Reading,         // Learn through text-based content
	Mixed,           // Combination approach
	Adaptive         // System adapts based on performance
};

UENUM(BlueprintType)
enum class ETutorialComplexity : uint8
{
	Beginner,        // New to climbing
	Intermediate,    // Some climbing experience
	Advanced,        // Experienced climber learning new techniques
	Expert,          // Master-level content
	Adaptive         // Adjusts based on user performance
};

UENUM(BlueprintType)
enum class EAssessmentType : uint8
{
	MultipleChoice,   // Traditional quiz format
	Interactive,      // Hands-on demonstration
	Scenario,         // Problem-solving scenario
	SkillDemo,        // Physical skill demonstration
	SafetyCheck,      // Safety knowledge verification
	PeerReview        // Multiplayer peer assessment
};

UENUM(BlueprintType)
enum class ETutorialMode : uint8
{
	Structured,       // Fixed sequence of lessons
	Exploratory,      // Free-form learning
	Guided,           // AI-guided personalized path
	Challenge,        // Problem-based learning
	Collaborative     // Team learning exercises
};

USTRUCT(BlueprintType)
struct FLearningPathProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName PathID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText PathName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CompletedSteps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 TotalSteps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MasteryScore; // 0-100

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TimeSpent; // Minutes

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> UnlockedSkills;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> MasteredSkills;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDateTime LastAccessed;

	FLearningPathProgress()
	{
		PathID = NAME_None;
		PathName = FText::GetEmpty();
		CompletedSteps = 0;
		TotalSteps = 0;
		MasteryScore = 0.0f;
		TimeSpent = 0.0f;
		LastAccessed = FDateTime::Now();
	}
};

USTRUCT(BlueprintType)
struct FAdvancedTutorialStep : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName StepID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETutorialStepType StepType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Title;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DetailedInstructions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<UTexture2D*> IllustrationImages;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UMediaPlayer> VideoContent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<class USoundBase*> AudioNarration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RequiredAction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> AlternativeActions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SafetyInformation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> CommonMistakes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> ProTips;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETutorialComplexity MinimumComplexity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ELearningStyle OptimalLearningStyle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float EstimatedDuration; // Minutes

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 DifficultyLevel; // 1-10

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> Prerequisites;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> UnlocksSkills;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresPhysicalSpace;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresEquipment;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> RequiredEquipment;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSupportsMultiplayer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 OptimalGroupSize;

	FAdvancedTutorialStep()
	{
		StepID = NAME_None;
		StepType = ETutorialStepType::Information;
		Title = FText::GetEmpty();
		Description = FText::GetEmpty();
		DetailedInstructions = FText::GetEmpty();
		VideoContent = nullptr;
		RequiredAction = TEXT("");
		SafetyInformation = TEXT("");
		MinimumComplexity = ETutorialComplexity::Beginner;
		OptimalLearningStyle = ELearningStyle::Mixed;
		EstimatedDuration = 5.0f;
		DifficultyLevel = 1;
		bRequiresPhysicalSpace = false;
		bRequiresEquipment = false;
		bSupportsMultiplayer = false;
		OptimalGroupSize = 1;
	}
};

USTRUCT(BlueprintType)
struct FSkillAssessment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName SkillID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText SkillName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EAssessmentType AssessmentType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> Questions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TArray<FString>> AnswerOptions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<int32> CorrectAnswers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ScenarioDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> RequiredActions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PassingScore; // 0-100

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxAttempts;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TimeLimit; // Minutes

	FSkillAssessment()
	{
		SkillID = NAME_None;
		SkillName = FText::GetEmpty();
		AssessmentType = EAssessmentType::MultipleChoice;
		PassingScore = 80.0f;
		MaxAttempts = 3;
		TimeLimit = 10.0f;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLearningPathCompleted, const FName&, PathID, float, FinalScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillMastered, const FName&, SkillID, float, MasteryScore, float, TimeToMaster);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTutorialAdaptation, ELearningStyle, DetectedStyle, ETutorialComplexity, RecommendedLevel);

/**
 * Enhanced tutorial system with adaptive learning, skill assessment, and personalized paths
 * Provides comprehensive climbing education with multiple learning modalities
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UEnhancedTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UEnhancedTutorialWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Learning path management
	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Learning Path")
	void StartLearningPath(const FName& PathID, ETutorialMode Mode = ETutorialMode::Structured);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Learning Path")
	void CreateCustomLearningPath(const FString& PathName, const TArray<FName>& StepSequence);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Learning Path")
	void GenerateAdaptivePath(ELearningStyle LearningStyle, ETutorialComplexity Complexity);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Learning Path")
	void SaveLearningProgress();

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Learning Path")
	void LoadLearningProgress();

	// Adaptive learning
	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Adaptive")
	void AnalyzeLearningStyle();

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Adaptive")
	void AdjustTutorialComplexity(float PerformanceScore);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Adaptive")
	void PersonalizeTutorialContent();

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Adaptive")
	void TrackLearningMetrics(const FString& MetricName, float Value);

	// Skill assessment
	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Assessment")
	void StartSkillAssessment(const FName& SkillID);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Assessment")
	void SubmitAssessmentAnswer(int32 QuestionIndex, int32 AnswerIndex);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Assessment")
	void CompleteSkillDemo(const FName& SkillID, const TArray<FString>& DemonstratedActions);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Assessment")
	void RequestPeerReview(const FName& SkillID);

	// Multi-modal content delivery
	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Content")
	void SetContentDeliveryMode(ELearningStyle Mode);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Content")
	void ToggleAudioNarration(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Content")
	void ShowInteractiveDiagram(UTexture2D* DiagramTexture, const TArray<FVector2D>& InteractivePoints);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Content")
	void PlayInstructionalVideo(UMediaPlayer* VideoPlayer);

	// Safety education
	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Safety")
	void StartSafetyModule(const FString& SafetyTopic);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Safety")
	void RunSafetyScenario(const FString& ScenarioName);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Safety")
	void TestEmergencyResponse();

	// Collaborative learning
	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Collaborative")
	void StartGroupTutorial(const TArray<FString>& Participants);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Collaborative")
	void AssignGroupRoles(const TMap<FString, FString>& RoleAssignments);

	UFUNCTION(BlueprintCallable, Category = "Enhanced Tutorial|Collaborative")
	void ShareLearningProgress(const FString& PlayerName);

	// Progress tracking and analytics
	UFUNCTION(BlueprintPure, Category = "Enhanced Tutorial|Progress")
	float GetOverallProgress() const;

	UFUNCTION(BlueprintPure, Category = "Enhanced Tutorial|Progress")
	TArray<FName> GetMasteredSkills() const;

	UFUNCTION(BlueprintPure, Category = "Enhanced Tutorial|Progress")
	float GetSkillMastery(const FName& SkillID) const;

	UFUNCTION(BlueprintPure, Category = "Enhanced Tutorial|Progress")
	FLearningPathProgress GetCurrentPathProgress() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Enhanced Tutorial")
	FOnLearningPathCompleted OnLearningPathCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Enhanced Tutorial")
	FOnSkillMastered OnSkillMastered;

	UPROPERTY(BlueprintAssignable, Category = "Enhanced Tutorial")
	FOnTutorialAdaptation OnTutorialAdaptation;

protected:
	// Main UI components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> TutorialModeSwitcher;

	// Learning path selection
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> LearningPathsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateCustomPathButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AdaptivePathButton;

	// Content delivery
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ContentDeliveryModeSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> TextContentDisplay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DiagramDisplay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> InteractiveCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> VideoDisplay;

	// Learning style customization
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> LearningStyleButtons;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> VisualLearningButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AuditoryLearningButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KinestheticLearningButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReadingLearningButton;

	// Skill assessment interface
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> AssessmentPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AssessmentQuestionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> AssessmentOptionsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> AssessmentProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AssessmentScoreText;

	// Interactive practice area
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> PracticeArea;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PracticeInstructionsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartPracticeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CompletePracticeButton;

	// Progress display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> OverallProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MasteredSkillsList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeSpentText;

	// Safety module
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> SafetyModulePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SafetyTopicText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> SafetyContentScroll;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SafetyTestButton;

	// Collaborative features
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> CollaborativePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ParticipantsList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GroupActivityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ShareProgressButton;

	// Accessibility features
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> AudioNarrationCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> ContentSpeedSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> SimplifiedModeCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RepeatInstructionButton;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Tutorial Config")
	TObjectPtr<UDataTable> AdvancedTutorialStepsTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Tutorial Config")
	TObjectPtr<UDataTable> SkillAssessmentsTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Tutorial Config")
	TObjectPtr<UDataTable> LearningPathsTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Tutorial Config")
	TSubclassOf<UUserWidget> InteractivePointWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Tutorial Config")
	float AdaptationSensitivity = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Tutorial Config")
	int32 MinimumStepsForAdaptation = 3;

private:
	// Learning state
	FLearningPathProgress CurrentPathProgress;
	TArray<FLearningPathProgress> AllPathProgress;
	TMap<FName, float> SkillMasteryLevels;
	ELearningStyle DetectedLearningStyle;
	ETutorialComplexity CurrentComplexity;
	ETutorialMode CurrentMode;

	// Assessment state
	FSkillAssessment CurrentAssessment;
	int32 CurrentQuestionIndex;
	TArray<int32> UserAnswers;
	int32 AssessmentAttempts;
	float AssessmentStartTime;

	// Adaptive learning metrics
	TMap<FString, TArray<float>> LearningMetrics;
	float AverageCompletionTime;
	float AverageAccuracy;
	int32 PreferredContentType; // 0=Text, 1=Images, 2=Video, 3=Audio

	// Content delivery state
	bool bAudioNarrationEnabled;
	float ContentPlaybackSpeed;
	bool bSimplifiedMode;

	// Collaborative state
	TArray<FString> GroupParticipants;
	TMap<FString, FString> ParticipantRoles;
	bool bInGroupMode;

	// Event handlers
	UFUNCTION()
	void OnCreateCustomPathClicked();

	UFUNCTION()
	void OnAdaptivePathClicked();

	UFUNCTION()
	void OnVisualLearningClicked();

	UFUNCTION()
	void OnAuditoryLearningClicked();

	UFUNCTION()
	void OnKinestheticLearningClicked();

	UFUNCTION()
	void OnReadingLearningClicked();

	UFUNCTION()
	void OnStartPracticeClicked();

	UFUNCTION()
	void OnCompletePracticeClicked();

	UFUNCTION()
	void OnSafetyTestClicked();

	UFUNCTION()
	void OnShareProgressClicked();

	UFUNCTION()
	void OnRepeatInstructionClicked();

	UFUNCTION()
	void OnAudioNarrationToggled(bool bIsChecked);

	UFUNCTION()
	void OnContentSpeedChanged(float Value);

	UFUNCTION()
	void OnSimplifiedModeToggled(bool bIsChecked);

	// Helper functions
	void LoadTutorialData();
	void UpdateProgressDisplay();
	void AdaptTutorialToUser();
	void AnalyzeUserPerformance();
	void CreateInteractivePoints(const TArray<FVector2D>& Points);
	void ProcessAssessmentResults();
	void UpdateSkillMastery(const FName& SkillID, float NewScore);
	void GeneratePersonalizedRecommendations();
	void SetupCollaborativeSession();
	ELearningStyle DetermineLearningStyle() const;
	ETutorialComplexity RecommendComplexityLevel() const;
	void SaveUserPreferences();
	void LoadUserPreferences();
};