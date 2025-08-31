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
#include "Engine/DataTable.h"
#include "CooperativeWidget.generated.h"

UENUM(BlueprintType)
enum class ECalloutType : uint8
{
	SafeToClimb,        // "Safe to climb"
	TakeSlack,          // "Take in slack"
	GiveSlack,          // "Give me slack"
	LowerMe,            // "Lower me"
	RopeFixed,          // "Rope is fixed"
	WatchForRockfall,   // "Watch for rockfall"
	NeedHelp,           // "I need help"
	AllClear,           // "All clear"
	EmergencyStop,      // "STOP!"
	Custom              // Custom voice/text message
};

UENUM(BlueprintType)
enum class ERopeStatus : uint8
{
	Secure,
	Tensioned,
	Slack,
	Tangled,
	Damaged,
	Unknown
};

USTRUCT(BlueprintType)
struct FTeammateInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Health;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Stamina;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Altitude;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsClimbing;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsBelaying;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bNeedsAssistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector WorldLocation;

	FTeammateInfo()
	{
		PlayerName = TEXT("Unknown");
		Health = 100.0f;
		Stamina = 100.0f;
		Altitude = 0.0f;
		bIsClimbing = false;
		bIsBelaying = false;
		bNeedsAssistance = false;
		WorldLocation = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct FCalloutMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ECalloutType CalloutType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SenderName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MessageText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Timestamp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsUrgent;

	FCalloutMessage()
	{
		CalloutType = ECalloutType::SafeToClimb;
		SenderName = TEXT("Unknown");
		MessageText = TEXT("");
		Timestamp = 0.0f;
		bIsUrgent = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCalloutSent, const FCalloutMessage&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeammateAssistanceRequest, const FString&, PlayerName);

/**
 * Cooperative communication widget for team climbing
 * Handles callouts, teammate status, rope information, and proximity chat
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UCooperativeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCooperativeWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Callout system
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Callouts")
	void SendCallout(ECalloutType CalloutType, const FString& CustomMessage = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Callouts")
	void ReceiveCallout(const FCalloutMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Callouts")
	void ClearCalloutHistory();

	// Teammate tracking
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Team")
	void UpdateTeammateInfo(const FString& PlayerName, const FTeammateInfo& Info);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Team")
	void RemoveTeammate(const FString& PlayerName);

	// Rope status
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Rope")
	void UpdateRopeStatus(ERopeStatus Status, float Tension, float Length);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Rope")
	void ShowRopeWarning(const FString& WarningMessage);

	// Proximity chat
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Chat")
	void SetProximityChatEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Chat")
	void UpdateProximityVolume(float Volume);

	// Emergency functions
	UFUNCTION(BlueprintCallable, Category = "Cooperative|Emergency")
	void TriggerEmergencyAlert();

	UFUNCTION(BlueprintCallable, Category = "Cooperative|Emergency")
	void RequestAssistance();

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Cooperative")
	FOnCalloutSent OnCalloutSent;

	UPROPERTY(BlueprintAssignable, Category = "Cooperative")
	FOnTeammateAssistanceRequest OnTeammateAssistanceRequest;

protected:
	// Widget components - bound in Blueprint
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> TeammateStatusContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> CalloutHistoryScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> CalloutHistoryContainer;

	// Quick callout buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SafeToClimbButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TakeSlackButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GiveSlackButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LowerMeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RopeFixedButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WatchRockfallButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NeedHelpButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AllClearButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EmergencyStopButton;

	// Rope status display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RopeStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RopeTensionBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RopeLengthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RopeStatusIcon;

	// Proximity chat controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ProximityChatToggle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProximityVolumeBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ProximityChatStatusText;

	// Emergency alert
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EmergencyAlertButton;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative Config")
	TSubclassOf<UUserWidget> TeammateStatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative Config")
	TSubclassOf<UUserWidget> CalloutMessageWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative Config")
	int32 MaxCalloutHistory = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative Config")
	float CalloutDisplayDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative Config")
	float EmergencyAlertCooldown = 30.0f;

private:
	// Current state
	UPROPERTY()
	TMap<FString, FTeammateInfo> Teammates;

	UPROPERTY()
	TArray<FCalloutMessage> CalloutHistory;

	ERopeStatus CurrentRopeStatus;
	float CurrentRopeTension;
	float CurrentRopeLength;
	bool bProximityChatEnabled;
	float ProximityVolume;
	float LastEmergencyAlert;

	// Button event handlers
	UFUNCTION()
	void OnSafeToClimbClicked();

	UFUNCTION()
	void OnTakeSlackClicked();

	UFUNCTION()
	void OnGiveSlackClicked();

	UFUNCTION()
	void OnLowerMeClicked();

	UFUNCTION()
	void OnRopeFixedClicked();

	UFUNCTION()
	void OnWatchRockfallClicked();

	UFUNCTION()
	void OnNeedHelpClicked();

	UFUNCTION()
	void OnAllClearClicked();

	UFUNCTION()
	void OnEmergencyStopClicked();

	UFUNCTION()
	void OnProximityChatToggleClicked();

	UFUNCTION()
	void OnEmergencyAlertClicked();

	// Helper functions
	void UpdateTeammateStatusDisplay();
	void UpdateCalloutHistoryDisplay();
	void UpdateRopeStatusDisplay();
	void UpdateProximityChatDisplay();
	void AddCalloutToHistory(const FCalloutMessage& Message);
	FString GetCalloutText(ECalloutType CalloutType) const;
	FLinearColor GetCalloutColor(ECalloutType CalloutType) const;
	bool CanSendEmergencyAlert() const;
};