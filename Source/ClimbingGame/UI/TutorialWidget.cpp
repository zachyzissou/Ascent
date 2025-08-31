#include "TutorialWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/RichTextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"

UTutorialWidget::UTutorialWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentTutorialType = ETutorialType::BasicMovement;
	CurrentSectionIndex = 0;
	CurrentStepIndex = 0;
	StepTimer = 0.0f;
	bWaitingForAction = false;
	TutorialSpeed = 1.0f;
	bSubtitlesEnabled = true;
	bAudioDescriptionEnabled = false;
	SelectedQuizAnswer = -1;
}

void UTutorialWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind navigation buttons
	if (PreviousButton)
	{
		PreviousButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnPreviousButtonClicked);
	}
	if (NextButton)
	{
		NextButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnNextButtonClicked);
	}
	if (SkipButton)
	{
		SkipButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnSkipButtonClicked);
	}
	if (HintButton)
	{
		HintButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnHintButtonClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnExitButtonClicked);
	}
	if (CompleteStepButton)
	{
		CompleteStepButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnCompleteStepButtonClicked);
	}
	if (SubmitAnswerButton)
	{
		SubmitAnswerButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnSubmitAnswerButtonClicked);
	}
	if (StartBasicTutorialButton)
	{
		StartBasicTutorialButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnStartBasicTutorialClicked);
	}

	// Load tutorial data and progress
	LoadTutorialData();
	LoadTutorialProgress();
	InitializeTutorialSections();

	// Show tutorial selection by default
	if (TutorialModeSwitcher)
	{
		TutorialModeSwitcher->SetActiveWidgetIndex(0); // Overview mode
	}

	CreateTutorialSectionWidgets();
}

void UTutorialWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Handle step timeout
	if (StepTimer > 0.0f)
	{
		StepTimer -= InDeltaTime * TutorialSpeed;
		
		if (StepTimer <= 0.0f && bAutoAdvanceInformation)
		{
			// Auto-advance information steps
			if (CurrentSectionIndex < TutorialSections.Num() && CurrentStepIndex < TutorialSections[CurrentSectionIndex].Steps.Num())
			{
				const FTutorialStep& CurrentStep = TutorialSections[CurrentSectionIndex].Steps[CurrentStepIndex];
				if (CurrentStep.StepType == ETutorialStepType::Information)
				{
					NextStep();
				}
			}
		}
	}
}

void UTutorialWidget::StartTutorial(ETutorialType TutorialType)
{
	CurrentTutorialType = TutorialType;
	CurrentSectionIndex = 0;
	CurrentStepIndex = 0;

	// Find the tutorial section
	for (int32 i = 0; i < TutorialSections.Num(); i++)
	{
		if (TutorialSections[i].TutorialType == TutorialType)
		{
			CurrentSectionIndex = i;
			break;
		}
	}

	// Switch to tutorial mode
	if (TutorialModeSwitcher)
	{
		TutorialModeSwitcher->SetActiveWidgetIndex(1); // Tutorial mode
	}

	ShowCurrentStep();
	UpdateProgress();
	UpdateNavigationButtons();

	UE_LOG(LogTemp, Log, TEXT("Started tutorial: %d"), static_cast<int32>(TutorialType));
}

void UTutorialWidget::NextStep()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	
	if (CurrentStepIndex < CurrentSection.Steps.Num() - 1)
	{
		CurrentStepIndex++;
	}
	else
	{
		// Section completed, check if there are more sections for this tutorial
		CheckSectionCompletion();
		return;
	}

	ShowCurrentStep();
	UpdateProgress();
	UpdateNavigationButtons();
}

void UTutorialWidget::PreviousStep()
{
	if (CurrentStepIndex > 0)
	{
		CurrentStepIndex--;
		ShowCurrentStep();
		UpdateProgress();
		UpdateNavigationButtons();
	}
}

void UTutorialWidget::SkipStep()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentStepIndex < CurrentSection.Steps.Num())
	{
		const FTutorialStep& CurrentStep = CurrentSection.Steps[CurrentStepIndex];
		
		// Only allow skipping if step is optional
		if (CurrentStep.bIsOptional)
		{
			NextStep();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot skip required tutorial step"));
		}
	}
}

void UTutorialWidget::CompleteTutorial()
{
	// Mark tutorial as completed
	if (!CompletedTutorials.Contains(CurrentTutorialType))
	{
		CompletedTutorials.Add(CurrentTutorialType);
	}

	// Save progress
	SaveTutorialProgress();

	// Broadcast completion
	OnTutorialCompleted.Broadcast();

	// Return to overview
	if (TutorialModeSwitcher)
	{
		TutorialModeSwitcher->SetActiveWidgetIndex(0);
	}

	CreateTutorialSectionWidgets(); // Refresh to show completion status

	UE_LOG(LogTemp, Log, TEXT("Tutorial completed: %d"), static_cast<int32>(CurrentTutorialType));
}

void UTutorialWidget::ExitTutorial()
{
	// Save current progress
	SaveTutorialProgress();

	// Return to overview
	if (TutorialModeSwitcher)
	{
		TutorialModeSwitcher->SetActiveWidgetIndex(0);
	}

	UE_LOG(LogTemp, Log, TEXT("Tutorial exited"));
}

void UTutorialWidget::CompleteCurrentStep()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentStepIndex >= CurrentSection.Steps.Num()) return;

	const FTutorialStep& CurrentStep = CurrentSection.Steps[CurrentStepIndex];

	// Mark step as completed
	if (!CompletedSteps.Contains(CurrentStep.StepID))
	{
		CompletedSteps.Add(CurrentStep.StepID);
	}

	// Broadcast step completion
	OnStepCompleted.Broadcast(CurrentStep);

	// Process completion
	ProcessStepCompletion();

	// Move to next step
	NextStep();
}

void UTutorialWidget::MarkActionCompleted(const FString& ActionName)
{
	if (bWaitingForAction && ExpectedAction == ActionName)
	{
		bWaitingForAction = false;
		CompleteCurrentStep();
	}
}

void UTutorialWidget::ShowHint()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentStepIndex >= CurrentSection.Steps.Num()) return;

	const FTutorialStep& CurrentStep = CurrentSection.Steps[CurrentStepIndex];
	
	// Show detailed instructions as hint
	if (StepDescriptionText && !CurrentStep.DetailedInstructions.IsEmpty())
	{
		StepDescriptionText->SetText(CurrentStep.DetailedInstructions);
		
		// Play audio description if enabled
		if (bAudioDescriptionEnabled)
		{
			PlayAudioDescription(CurrentStep.DetailedInstructions.ToString());
		}
	}
}

void UTutorialWidget::ToggleSubtitles(bool bEnabled)
{
	bSubtitlesEnabled = bEnabled;
	// Update subtitle display accordingly
}

float UTutorialWidget::GetTutorialProgress() const
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return 1.0f;

	const FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentSection.Steps.Num() == 0) return 1.0f;

	return static_cast<float>(CurrentStepIndex) / static_cast<float>(CurrentSection.Steps.Num());
}

bool UTutorialWidget::IsTutorialCompleted(ETutorialType TutorialType) const
{
	return CompletedTutorials.Contains(TutorialType);
}

bool UTutorialWidget::IsStepCompleted(const FName& StepID) const
{
	return CompletedSteps.Contains(StepID);
}

void UTutorialWidget::SetTutorialSpeed(float Speed)
{
	TutorialSpeed = FMath::Clamp(Speed, 0.5f, 2.0f);
}

void UTutorialWidget::SetAudioDescriptionEnabled(bool bEnabled)
{
	bAudioDescriptionEnabled = bEnabled;
}

void UTutorialWidget::RepeatCurrentInstruction()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	const FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentStepIndex >= CurrentSection.Steps.Num()) return;

	const FTutorialStep& CurrentStep = CurrentSection.Steps[CurrentStepIndex];
	PlayStepAudio(CurrentStep);
}

// Event handlers
void UTutorialWidget::OnPreviousButtonClicked()
{
	PreviousStep();
}

void UTutorialWidget::OnNextButtonClicked()
{
	NextStep();
}

void UTutorialWidget::OnSkipButtonClicked()
{
	SkipStep();
}

void UTutorialWidget::OnHintButtonClicked()
{
	ShowHint();
}

void UTutorialWidget::OnExitButtonClicked()
{
	ExitTutorial();
}

void UTutorialWidget::OnCompleteStepButtonClicked()
{
	CompleteCurrentStep();
}

void UTutorialWidget::OnSubmitAnswerButtonClicked()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	const FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentStepIndex >= CurrentSection.Steps.Num()) return;

	const FTutorialStep& CurrentStep = CurrentSection.Steps[CurrentStepIndex];
	
	if (CurrentStep.StepType == ETutorialStepType::Quiz)
	{
		// Check answer
		bool bCorrect = (SelectedQuizAnswer == CurrentStep.CorrectAnswer);
		
		if (bCorrect)
		{
			UE_LOG(LogTemp, Log, TEXT("Correct answer!"));
			CompleteCurrentStep();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Incorrect answer. Try again."));
			// Could show feedback here
		}
	}
}

void UTutorialWidget::OnStartBasicTutorialClicked()
{
	StartTutorial(ETutorialType::BasicMovement);
}

void UTutorialWidget::LoadTutorialData()
{
	if (TutorialDataTable)
	{
		// Load tutorial steps from data table
		TArray<FTutorialStep*> AllSteps;
		TutorialDataTable->GetAllRows<FTutorialStep>(TEXT("LoadTutorialData"), AllSteps);

		// Group steps by tutorial type (this would need to be implemented based on your data structure)
		UE_LOG(LogTemp, Log, TEXT("Loaded %d tutorial steps from data table"), AllSteps.Num());
	}
	else
	{
		// Create default tutorial data if no data table is provided
		InitializeTutorialSections();
	}
}

void UTutorialWidget::InitializeTutorialSections()
{
	TutorialSections.Empty();

	// Create basic movement tutorial
	FTutorialSection BasicMovement;
	BasicMovement.TutorialType = ETutorialType::BasicMovement;
	BasicMovement.SectionTitle = FText::FromString(TEXT("Basic Movement"));
	BasicMovement.SectionDescription = FText::FromString(TEXT("Learn the fundamentals of movement and interaction"));
	BasicMovement.bIsUnlocked = true;

	// Add steps to basic movement
	FTutorialStep Step1;
	Step1.StepID = FName("BasicMovement_01");
	Step1.StepType = ETutorialStepType::Information;
	Step1.Title = FText::FromString(TEXT("Welcome to Climbing"));
	Step1.Description = FText::FromString(TEXT("Welcome to ClimbingGame! In this tutorial, you'll learn the basics of movement and climbing."));
	Step1.TimeoutDuration = 5.0f;
	BasicMovement.Steps.Add(Step1);

	FTutorialStep Step2;
	Step2.StepID = FName("BasicMovement_02");
	Step2.StepType = ETutorialStepType::Interactive;
	Step2.Title = FText::FromString(TEXT("Look Around"));
	Step2.Description = FText::FromString(TEXT("Use your mouse to look around. Move your mouse to explore your surroundings."));
	Step2.RequiredAction = TEXT("LookAround");
	BasicMovement.Steps.Add(Step2);

	FTutorialStep Step3;
	Step3.StepID = FName("BasicMovement_03");
	Step3.StepType = ETutorialStepType::Interactive;
	Step3.Title = FText::FromString(TEXT("Move Forward"));
	Step3.Description = FText::FromString(TEXT("Press W to move forward. Walk to the climbing wall ahead."));
	Step3.RequiredAction = TEXT("MoveForward");
	BasicMovement.Steps.Add(Step3);

	TutorialSections.Add(BasicMovement);

	// Create climbing basics tutorial
	FTutorialSection ClimbingBasics;
	ClimbingBasics.TutorialType = ETutorialType::ClimbingBasics;
	ClimbingBasics.SectionTitle = FText::FromString(TEXT("Climbing Basics"));
	ClimbingBasics.SectionDescription = FText::FromString(TEXT("Learn how to climb and manage your stamina"));
	ClimbingBasics.bIsUnlocked = false; // Unlocked after basic movement
	TutorialSections.Add(ClimbingBasics);
}

void UTutorialWidget::UpdateTutorialDisplay()
{
	ShowCurrentStep();
	UpdateProgress();
	UpdateNavigationButtons();
}

void UTutorialWidget::ShowCurrentStep()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	const FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	if (CurrentStepIndex >= CurrentSection.Steps.Num()) return;

	const FTutorialStep& CurrentStep = CurrentSection.Steps[CurrentStepIndex];

	// Update step display
	if (StepTitleText)
	{
		StepTitleText->SetText(CurrentStep.Title);
	}

	if (StepDescriptionText)
	{
		StepDescriptionText->SetText(CurrentStep.Description);
	}

	if (StepIllustrationImage && CurrentStep.IllustrationImage)
	{
		StepIllustrationImage->SetBrushFromTexture(CurrentStep.IllustrationImage);
	}

	// Handle step type-specific logic
	switch (CurrentStep.StepType)
	{
	case ETutorialStepType::Information:
		HandleInformationStep(CurrentStep);
		break;
	case ETutorialStepType::Interactive:
		HandleInteractiveStep(CurrentStep);
		break;
	case ETutorialStepType::Video:
		HandleVideoStep(CurrentStep);
		break;
	case ETutorialStepType::Practice:
		HandlePracticeStep(CurrentStep);
		break;
	case ETutorialStepType::Quiz:
		HandleQuizStep(CurrentStep);
		break;
	case ETutorialStepType::Completion:
		HandleCompletionStep(CurrentStep);
		break;
	}

	// Play audio if available
	PlayStepAudio(CurrentStep);

	// Set timeout if specified
	if (CurrentStep.TimeoutDuration > 0.0f)
	{
		StepTimer = CurrentStep.TimeoutDuration;
	}
}

void UTutorialWidget::UpdateProgress()
{
	float Progress = GetTutorialProgress();
	
	if (TutorialProgressBar)
	{
		TutorialProgressBar->SetPercent(Progress);
	}

	if (ProgressText)
	{
		if (CurrentSectionIndex < TutorialSections.Num())
		{
			const FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
			FString ProgressString = FString::Printf(TEXT("Step %d of %d"), 
				CurrentStepIndex + 1, CurrentSection.Steps.Num());
			ProgressText->SetText(FText::FromString(ProgressString));
		}
	}
}

void UTutorialWidget::UpdateNavigationButtons()
{
	if (PreviousButton)
	{
		PreviousButton->SetIsEnabled(CurrentStepIndex > 0);
	}

	if (NextButton)
	{
		bool bCanAdvance = true;
		if (bWaitingForAction)
		{
			bCanAdvance = false; // Must complete action first
		}
		NextButton->SetIsEnabled(bCanAdvance);
	}

	if (SkipButton)
	{
		bool bCanSkip = false;
		if (CurrentSectionIndex < TutorialSections.Num() && CurrentStepIndex < TutorialSections[CurrentSectionIndex].Steps.Num())
		{
			const FTutorialStep& CurrentStep = TutorialSections[CurrentSectionIndex].Steps[CurrentStepIndex];
			bCanSkip = CurrentStep.bIsOptional;
		}
		SkipButton->SetIsEnabled(bCanSkip);
	}
}

void UTutorialWidget::CreateTutorialSectionWidgets()
{
	if (!TutorialSectionsContainer || !TutorialSectionWidgetClass) return;

	// Clear existing widgets
	TutorialSectionsContainer->ClearChildren();

	// Create widgets for each tutorial section
	for (const FTutorialSection& Section : TutorialSections)
	{
		if (UUserWidget* SectionWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), TutorialSectionWidgetClass))
		{
			TutorialSectionsContainer->AddChild(SectionWidget);
			// TODO: Initialize section widget with section data
		}
	}
}

void UTutorialWidget::CreateQuizOptions()
{
	if (!QuizOptionsContainer || !QuizOptionWidgetClass) return;
	if (CurrentSectionIndex >= TutorialSections.Num() || CurrentStepIndex >= TutorialSections[CurrentSectionIndex].Steps.Num()) return;

	const FTutorialStep& CurrentStep = TutorialSections[CurrentSectionIndex].Steps[CurrentStepIndex];
	
	// Clear existing options
	QuizOptionsContainer->ClearChildren();

	// Create option widgets
	for (int32 i = 0; i < CurrentStep.QuizOptions.Num(); i++)
	{
		if (UUserWidget* OptionWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), QuizOptionWidgetClass))
		{
			QuizOptionsContainer->AddChild(OptionWidget);
			// TODO: Initialize option widget with option text and index
		}
	}
}

void UTutorialWidget::ProcessStepCompletion()
{
	// Additional processing when a step is completed
	bWaitingForAction = false;
	ExpectedAction = TEXT("");
	StepTimer = 0.0f;
}

void UTutorialWidget::CheckSectionCompletion()
{
	if (CurrentSectionIndex >= TutorialSections.Num()) return;

	FTutorialSection& CurrentSection = TutorialSections[CurrentSectionIndex];
	CurrentSection.bIsCompleted = true;

	// Broadcast section completion
	OnSectionCompleted.Broadcast(CurrentSection.TutorialType);

	// Unlock next section if applicable
	if (CurrentSectionIndex + 1 < TutorialSections.Num())
	{
		TutorialSections[CurrentSectionIndex + 1].bIsUnlocked = true;
	}

	// Complete tutorial or move to next section
	CompleteTutorial();
}

void UTutorialWidget::SaveTutorialProgress()
{
	// Save progress to game save data
	UE_LOG(LogTemp, Log, TEXT("Saving tutorial progress"));
}

void UTutorialWidget::LoadTutorialProgress()
{
	// Load progress from game save data
	UE_LOG(LogTemp, Log, TEXT("Loading tutorial progress"));
}

// Step type handlers
void UTutorialWidget::HandleInformationStep(const FTutorialStep& Step)
{
	// Show information, enable auto-advance
	if (CompleteStepButton)
	{
		CompleteStepButton->SetVisibility(ESlateVisibility::Hidden);
	}
	if (ActionPromptText)
	{
		ActionPromptText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UTutorialWidget::HandleInteractiveStep(const FTutorialStep& Step)
{
	// Wait for player action
	bWaitingForAction = true;
	ExpectedAction = Step.RequiredAction;

	if (ActionPromptText)
	{
		ActionPromptText->SetText(FText::FromString(Step.RequiredAction));
		ActionPromptText->SetVisibility(ESlateVisibility::Visible);
	}

	if (CompleteStepButton)
	{
		CompleteStepButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTutorialWidget::HandleVideoStep(const FTutorialStep& Step)
{
	// Play video demonstration
	UE_LOG(LogTemp, Log, TEXT("Playing video: %s"), *Step.VideoPath);
}

void UTutorialWidget::HandlePracticeStep(const FTutorialStep& Step)
{
	// Set up practice scenario
	UE_LOG(LogTemp, Log, TEXT("Starting practice step"));
}

void UTutorialWidget::HandleQuizStep(const FTutorialStep& Step)
{
	// Show quiz options
	CreateQuizOptions();
	
	if (SubmitAnswerButton)
	{
		SubmitAnswerButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTutorialWidget::HandleCompletionStep(const FTutorialStep& Step)
{
	// Show completion message
	UE_LOG(LogTemp, Log, TEXT("Tutorial section completed"));
}

void UTutorialWidget::PlayStepAudio(const FTutorialStep& Step)
{
	// Play audio narration for the step
	if (bSubtitlesEnabled)
	{
		// Show subtitles
	}
}

void UTutorialWidget::PlayAudioDescription(const FString& Description)
{
	// Play audio description for accessibility
	UE_LOG(LogTemp, Log, TEXT("Audio description: %s"), *Description);
}