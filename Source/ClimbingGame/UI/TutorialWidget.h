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
#include "Engine/DataTable.h"
#include "TutorialWidget.generated.h"

UENUM(BlueprintType)
enum class ETutorialType : uint8
{
	BasicMovement,      // Walking, looking, basic interaction
	ClimbingBasics,     // Grabbing holds, grip strength, stamina
	ToolUsage,          // Placing anchors, using ropes, carabiners
	SafetyProtocols,    // Belaying, communication, fall protection
	Cooperative,        // Team climbing, assistance, callouts
	Advanced,           // Complex routes, lead climbing, rescue
	VRSpecific         // VR-only tutorials for hand tracking
};

UENUM(BlueprintType)
enum class ETutorialStepType : uint8
{
	Information,        // Text/image information display
	Interactive,        // Player must perform action
	Video,             // Video demonstration
	Practice,          // Guided practice session
	Quiz,              // Knowledge check
	Completion         // Tutorial section complete
};

USTRUCT(BlueprintType)
struct FTutorialStep : public FTableRowBase
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
	UTexture2D* IllustrationImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString VideoPath; // For video demonstrations

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RequiredAction; // Action player must perform

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsOptional;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TimeoutDuration; // Auto-advance after this time

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> QuizOptions; // For quiz steps

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CorrectAnswer; // Index of correct answer for quiz

	FTutorialStep()
	{
		StepID = NAME_None;
		StepType = ETutorialStepType::Information;
		Title = FText::GetEmpty();
		Description = FText::GetEmpty();
		DetailedInstructions = FText::GetEmpty();
		IllustrationImage = nullptr;
		VideoPath = TEXT("");
		RequiredAction = TEXT("");
		bIsOptional = false;
		TimeoutDuration = 0.0f;
		CorrectAnswer = 0;
	}
};

USTRUCT(BlueprintType)
struct FTutorialSection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETutorialType TutorialType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText SectionTitle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText SectionDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FTutorialStep> Steps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsCompleted;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsUnlocked;

	FTutorialSection()
	{
		TutorialType = ETutorialType::BasicMovement;
		SectionTitle = FText::GetEmpty();
		SectionDescription = FText::GetEmpty();
		bIsCompleted = false;
		bIsUnlocked = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialStepCompleted, const FTutorialStep&, Step);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialSectionCompleted, ETutorialType, TutorialType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialCompleted);

/**
 * Comprehensive tutorial and onboarding system
 * Supports interactive tutorials, video demonstrations, and progress tracking
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTutorialWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Tutorial navigation
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorial(ETutorialType TutorialType);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void NextStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void PreviousStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SkipStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ExitTutorial();

	// Step management
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteCurrentStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void MarkActionCompleted(const FString& ActionName);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ShowHint();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ToggleSubtitles(bool bEnabled);

	// Progress tracking
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	float GetTutorialProgress() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsTutorialCompleted(ETutorialType TutorialType) const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsStepCompleted(const FName& StepID) const;

	// Accessibility
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Accessibility")
	void SetTutorialSpeed(float Speed);

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Accessibility")
	void SetAudioDescriptionEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Accessibility")
	void RepeatCurrentInstruction();

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialStepCompleted OnStepCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialSectionCompleted OnSectionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialCompleted OnTutorialCompleted;

protected:
	// Main UI components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> TutorialModeSwitcher;

	// Tutorial selection (overview mode)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> TutorialSectionsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WelcomeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartBasicTutorialButton;

	// Active tutorial display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StepTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> StepDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> StepIllustrationImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> TutorialProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ProgressText;

	// Navigation controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PreviousButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SkipButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HintButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;

	// Interactive step components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionPromptText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CompleteStepButton;

	// Quiz components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> QuizOptionsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SubmitAnswerButton;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Config")
	TObjectPtr<UDataTable> TutorialDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Config")
	TSubclassOf<UUserWidget> TutorialSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Config")
	TSubclassOf<UUserWidget> QuizOptionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Config")
	float DefaultStepTimeout = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Config")
	bool bAutoAdvanceInformation = true;

private:
	// Current state
	ETutorialType CurrentTutorialType;
	TArray<FTutorialSection> TutorialSections;
	int32 CurrentSectionIndex;
	int32 CurrentStepIndex;
	float StepTimer;
	bool bWaitingForAction;
	FString ExpectedAction;
	
	// Settings
	float TutorialSpeed;
	bool bSubtitlesEnabled;
	bool bAudioDescriptionEnabled;

	// Completed tutorials tracking
	TArray<ETutorialType> CompletedTutorials;
	TArray<FName> CompletedSteps;

	// Quiz state
	int32 SelectedQuizAnswer;

	// Event handlers
	UFUNCTION()
	void OnPreviousButtonClicked();

	UFUNCTION()
	void OnNextButtonClicked();

	UFUNCTION()
	void OnSkipButtonClicked();

	UFUNCTION()
	void OnHintButtonClicked();

	UFUNCTION()
	void OnExitButtonClicked();

	UFUNCTION()
	void OnCompleteStepButtonClicked();

	UFUNCTION()
	void OnSubmitAnswerButtonClicked();

	UFUNCTION()
	void OnStartBasicTutorialClicked();

	// Helper functions
	void LoadTutorialData();
	void InitializeTutorialSections();
	void UpdateTutorialDisplay();
	void ShowCurrentStep();
	void UpdateProgress();
	void UpdateNavigationButtons();
	void CreateTutorialSectionWidgets();
	void CreateQuizOptions();
	void ProcessStepCompletion();
	void CheckSectionCompletion();
	void SaveTutorialProgress();
	void LoadTutorialProgress();
	
	// Step type handlers
	void HandleInformationStep(const FTutorialStep& Step);
	void HandleInteractiveStep(const FTutorialStep& Step);
	void HandleVideoStep(const FTutorialStep& Step);
	void HandlePracticeStep(const FTutorialStep& Step);
	void HandleQuizStep(const FTutorialStep& Step);
	void HandleCompletionStep(const FTutorialStep& Step);

	// Audio/accessibility
	void PlayStepAudio(const FTutorialStep& Step);
	void PlayAudioDescription(const FString& Description);
};