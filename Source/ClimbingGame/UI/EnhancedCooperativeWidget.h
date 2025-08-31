#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/EditableTextBox.h"
#include "Engine/DataTable.h"
#include "EnhancedCooperativeWidget.generated.h"

UENUM(BlueprintType)
enum class ECooperativeActionType : uint8
{
	Belay,              // Providing belay for partner
	Spot,               // Spotting a boulder problem
	Boost,              // Giving physical boost to reach hold
	Anchor,             // Acting as human anchor
	Rescue,             // Emergency rescue operation
	ShareEquipment,     // Passing tools to partner
	RouteGuidance,      // Providing route beta
	EmergencyFirst Aid, // Medical assistance
	Evacuation,         // Emergency descent/evacuation
	WeatherAlert        // Sharing weather information
};

UENUM(BlueprintType)
enum class ETeammateRole : uint8
{
	Leader,            // Leading the climb
	Follower,          // Following/seconding
	Belayer,           // Providing belay
	Spotter,           // Spotting climber
	Support,           // General support role
	Injured,           // Requiring assistance
	Evacuation         // Being evacuated
};

USTRUCT(BlueprintType)
struct FAdvancedTeammateInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerID; // Network ID

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETeammateRole CurrentRole;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Health;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Stamina;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Altitude;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector WorldLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DistanceFromPlayer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsClimbing;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsBelaying;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bNeedsAssistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsInjured;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString CurrentActivity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SkillLevel; // 1-10 climbing skill

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 ClimbingExperience; // Years of experience

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> Certifications; // Rescue, first aid, etc.

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RopeLength; // Available rope length

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHasEmergencyGear;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float LastCommunicationTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsInCommunicationRange;

	FAdvancedTeammateInfo()
	{
		PlayerName = TEXT("Unknown");
		PlayerID = TEXT("");
		CurrentRole = ETeammateRole::Support;
		Health = 100.0f;
		Stamina = 100.0f;
		Altitude = 0.0f;
		WorldLocation = FVector::ZeroVector;
		DistanceFromPlayer = 0.0f;
		bIsClimbing = false;
		bIsBelaying = false;
		bNeedsAssistance = false;
		bIsInjured = false;
		CurrentActivity = TEXT("Ready");
		SkillLevel = 5.0f;
		ClimbingExperience = 1;
		RopeLength = 60.0f;
		bHasEmergencyGear = false;
		LastCommunicationTime = 0.0f;
		bIsInCommunicationRange = true;
	}
};

USTRUCT(BlueprintType)
struct FRopeSystemInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString RopeID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ERopeStatus Status;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TensionForce; // Newtons

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TotalLength;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float UsedLength;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 AnchorPointCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float LastFallForce; // Force from last fall

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 FallCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ElasticStretch; // Current stretch percentage

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsShared; // Multi-person rope system

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> ConnectedPlayers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SafetyMargin; // Distance to max safe force

	FRopeSystemInfo()
	{
		RopeID = TEXT("Primary");
		Status = ERopeStatus::Secure;
		TensionForce = 0.0f;
		TotalLength = 60.0f;
		UsedLength = 0.0f;
		AnchorPointCount = 0;
		LastFallForce = 0.0f;
		FallCount = 0;
		ElasticStretch = 0.0f;
		bIsShared = false;
		SafetyMargin = 100.0f;
	}
};

USTRUCT(BlueprintType)
struct FEmergencyProcedure
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ProcedureID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText ProcedureName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FText> Steps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float EstimatedTime; // Minutes to complete

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RequiredPersonnel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> RequiredEquipment;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRequiresExpertise;

	FEmergencyProcedure()
	{
		ProcedureID = NAME_None;
		ProcedureName = FText::GetEmpty();
		Description = FText::GetEmpty();
		EstimatedTime = 10.0f;
		RequiredPersonnel = 2;
		bRequiresExpertise = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCooperativeAction, ECooperativeActionType, ActionType, const FString&, TargetPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEmergencyProcedure, const FEmergencyProcedure&, Procedure, bool, bStartProcedure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamFormation, const TArray<FString>&, TeamMembers);

/**
 * Enhanced cooperative climbing widget with advanced team management
 * Handles complex rope systems, emergency procedures, and team coordination
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UEnhancedCooperativeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UEnhancedCooperativeWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Advanced team management
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Team")
	void UpdateAdvancedTeammateInfo(const FString& PlayerName, const FAdvancedTeammateInfo& Info);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Team")
	void AssignTeammateRole(const FString& PlayerName, ETeammateRole NewRole);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Team")
	void RequestRoleChange(ETeammateRole DesiredRole);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Team")
	void InitiateCooperativeAction(ECooperativeActionType ActionType, const FString& TargetPlayer);

	// Advanced rope system management
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Rope")
	void UpdateRopeSystemInfo(const FRopeSystemInfo& RopeInfo);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Rope")
	void EstablishSharedRopeSystem(const TArray<FString>& PlayerNames);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Rope")
	void MonitorRopeTension(float MaxSafeTension);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Rope")
	void ShowRopeSystemDiagram();

	// Emergency procedures
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Emergency")
	void TriggerEmergencyProtocol(const FString& EmergencyType);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Emergency")
	void StartEmergencyProcedure(const FEmergencyProcedure& Procedure);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Emergency")
	void UpdateProcedureProgress(const FName& ProcedureID, int32 CurrentStep);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Emergency")
	void RequestEmergencyAssistance(const FString& AssistanceType);

	// Team coordination
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Coordination")
	void ProposeClimbingOrder(const TArray<FString>& ClimbingSequence);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Coordination")
	void ShareRouteInformation(const FString& RouteBeta, const FString& Difficulty);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Coordination")
	void CoordinateEquipmentSharing(const FString& EquipmentType, const FString& RequestingPlayer);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Coordination")
	void SetupMultiPitchStrategy(const TArray<FString>& PitchLeaders);

	// Advanced communication
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Communication")
	void SendDetailedCallout(const FCalloutMessage& Message, const TArray<FString>& Recipients);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Communication")
	void BroadcastWeatherAlert(const FString& WeatherCondition, float Severity);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Communication")
	void ShareTechnicalAdvice(const FString& Technique, const FString& Context);

	// Team performance tracking
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Performance")
	void UpdateTeamPerformance(const FString& MetricName, float Value);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Performance")
	void TrackClimbingEfficiency(float TimeToComplete, float DifficultyRating);

	UFUNCTION(BlueprintPure, Category = "Cooperative|Performance")
	float GetTeamSafetyScore() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Enhanced Cooperative")
	FOnCooperativeAction OnCooperativeAction;

	UPROPERTY(BlueprintAssignable, Category = "Enhanced Cooperative")
	FOnEmergencyProcedure OnEmergencyProcedure;

	UPROPERTY(BlueprintAssignable, Category = "Enhanced Cooperative")
	FOnTeamFormation OnTeamFormation;

protected:
	// Main UI containers
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> EnhancedCooperativeContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> AdvancedTeammateList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> RopeSystemPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EmergencyProcedurePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TeamCoordinationPanel;

	// Advanced teammate display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> TeammateDetailsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> TeamMapOverlay;

	// Role assignment interface
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RoleAssignmentContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaderRoleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BelayerRoleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SpotterRoleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SupportRoleButton;

	// Rope system display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RopeSystemDiagram;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RopeTensionBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RopeUsageBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnchorCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SafetyMarginText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ConnectedPlayersContainer;

	// Emergency interface
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> EmergencyProceduresList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ActiveProcedureSteps;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProcedureProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EmergencyAlertButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FirstAidButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RescueButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EvacuationButton;

	// Coordination tools
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ClimbingOrderContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> RouteBetaTextBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ShareBetaButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> EquipmentRequestsContainer;

	// Performance metrics
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TeamSafetyScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ClimbingEfficiencyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> TeamCohesionBar;

	// Advanced communication
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> DetailedCalloutHistory;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> CommunicationRangeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BroadcastButton;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Cooperative Config")
	TSubclassOf<UUserWidget> AdvancedTeammateWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Cooperative Config")
	TSubclassOf<UUserWidget> EmergencyProcedureWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Cooperative Config")
	TObjectPtr<UDataTable> EmergencyProceduresTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Cooperative Config")
	float MaxCommunicationRange = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Cooperative Config")
	float RopeTensionWarningThreshold = 8000.0f; // Newtons

private:
	// Enhanced state management
	TMap<FString, FAdvancedTeammateInfo> AdvancedTeammates;
	FRopeSystemInfo CurrentRopeSystem;
	TArray<FEmergencyProcedure> AvailableEmergencyProcedures;
	FEmergencyProcedure ActiveProcedure;
	int32 CurrentProcedureStep;
	bool bEmergencyProcedureActive;

	// Team coordination state
	TArray<FString> ProposedClimbingOrder;
	TMap<FString, float> TeamPerformanceMetrics;
	float CurrentTeamSafetyScore;
	float CommunicationRange;

	// Event handlers
	UFUNCTION()
	void OnLeaderRoleClicked();

	UFUNCTION()
	void OnBelayerRoleClicked();

	UFUNCTION()
	void OnSpotterRoleClicked();

	UFUNCTION()
	void OnSupportRoleClicked();

	UFUNCTION()
	void OnEmergencyAlertClicked();

	UFUNCTION()
	void OnFirstAidClicked();

	UFUNCTION()
	void OnRescueClicked();

	UFUNCTION()
	void OnEvacuationClicked();

	UFUNCTION()
	void OnShareBetaClicked();

	UFUNCTION()
	void OnBroadcastClicked();

	UFUNCTION()
	void OnCommunicationRangeChanged(float Value);

	// Helper functions
	void UpdateAdvancedTeammateDisplay();
	void UpdateRopeSystemDisplay();
	void UpdateEmergencyProcedureDisplay();
	void UpdateTeamCoordinationDisplay();
	void UpdatePerformanceMetrics();
	void LoadEmergencyProcedures();
	void CreateAdvancedTeammateWidget(const FAdvancedTeammateInfo& TeammateInfo);
	void CalculateTeamSafetyScore();
	void ProcessEmergencyProcedureStep();
	bool IsTeammateInRange(const FAdvancedTeammateInfo& Teammate) const;
	FLinearColor GetRoleColor(ETeammateRole Role) const;
	FLinearColor GetTensionColor(float TensionForce, float MaxSafe) const;
};