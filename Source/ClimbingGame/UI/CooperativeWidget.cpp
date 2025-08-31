#include "CooperativeWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UCooperativeWidget::UCooperativeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentRopeStatus = ERopeStatus::Unknown;
	CurrentRopeTension = 0.0f;
	CurrentRopeLength = 0.0f;
	bProximityChatEnabled = true;
	ProximityVolume = 1.0f;
	LastEmergencyAlert = 0.0f;
}

void UCooperativeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind callout buttons
	if (SafeToClimbButton)
	{
		SafeToClimbButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnSafeToClimbClicked);
	}
	if (TakeSlackButton)
	{
		TakeSlackButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnTakeSlackClicked);
	}
	if (GiveSlackButton)
	{
		GiveSlackButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnGiveSlackClicked);
	}
	if (LowerMeButton)
	{
		LowerMeButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnLowerMeClicked);
	}
	if (RopeFixedButton)
	{
		RopeFixedButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnRopeFixedClicked);
	}
	if (WatchRockfallButton)
	{
		WatchRockfallButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnWatchRockfallClicked);
	}
	if (NeedHelpButton)
	{
		NeedHelpButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnNeedHelpClicked);
	}
	if (AllClearButton)
	{
		AllClearButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnAllClearClicked);
	}
	if (EmergencyStopButton)
	{
		EmergencyStopButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnEmergencyStopClicked);
	}
	if (ProximityChatToggle)
	{
		ProximityChatToggle->OnClicked.AddDynamic(this, &UCooperativeWidget::OnProximityChatToggleClicked);
	}
	if (EmergencyAlertButton)
	{
		EmergencyAlertButton->OnClicked.AddDynamic(this, &UCooperativeWidget::OnEmergencyAlertClicked);
	}

	// Initialize displays
	UpdateRopeStatusDisplay();
	UpdateProximityChatDisplay();
	UpdateTeammateStatusDisplay();
	UpdateCalloutHistoryDisplay();
}

void UCooperativeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Update emergency alert button state
	if (EmergencyAlertButton)
	{
		EmergencyAlertButton->SetIsEnabled(CanSendEmergencyAlert());
	}
}

void UCooperativeWidget::SendCallout(ECalloutType CalloutType, const FString& CustomMessage)
{
	FCalloutMessage Message;
	Message.CalloutType = CalloutType;
	Message.SenderName = TEXT("You"); // Would get from player name
	Message.MessageText = CustomMessage.IsEmpty() ? GetCalloutText(CalloutType) : CustomMessage;
	Message.Timestamp = GetWorld()->GetTimeSeconds();
	Message.bIsUrgent = (CalloutType == ECalloutType::EmergencyStop || CalloutType == ECalloutType::NeedHelp);

	// Add to history
	AddCalloutToHistory(Message);

	// Broadcast event for networking
	OnCalloutSent.Broadcast(Message);

	UE_LOG(LogTemp, Log, TEXT("Sent callout: %s"), *Message.MessageText);
}

void UCooperativeWidget::ReceiveCallout(const FCalloutMessage& Message)
{
	// Add received message to history
	AddCalloutToHistory(Message);

	// Play audio cue based on callout type
	if (Message.bIsUrgent)
	{
		// Play urgent sound
		UE_LOG(LogTemp, Warning, TEXT("URGENT CALLOUT from %s: %s"), *Message.SenderName, *Message.MessageText);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Callout from %s: %s"), *Message.SenderName, *Message.MessageText);
	}
}

void UCooperativeWidget::ClearCalloutHistory()
{
	CalloutHistory.Empty();
	UpdateCalloutHistoryDisplay();
}

void UCooperativeWidget::UpdateTeammateInfo(const FString& PlayerName, const FTeammateInfo& Info)
{
	Teammates.Add(PlayerName, Info);
	UpdateTeammateStatusDisplay();
}

void UCooperativeWidget::RemoveTeammate(const FString& PlayerName)
{
	Teammates.Remove(PlayerName);
	UpdateTeammateStatusDisplay();
}

void UCooperativeWidget::UpdateRopeStatus(ERopeStatus Status, float Tension, float Length)
{
	CurrentRopeStatus = Status;
	CurrentRopeTension = Tension;
	CurrentRopeLength = Length;

	UpdateRopeStatusDisplay();
}

void UCooperativeWidget::ShowRopeWarning(const FString& WarningMessage)
{
	// Create urgent callout for rope issues
	FCalloutMessage Message;
	Message.CalloutType = ECalloutType::Custom;
	Message.SenderName = TEXT("System");
	Message.MessageText = FString::Printf(TEXT("ROPE WARNING: %s"), *WarningMessage);
	Message.Timestamp = GetWorld()->GetTimeSeconds();
	Message.bIsUrgent = true;

	ReceiveCallout(Message);
}

void UCooperativeWidget::SetProximityChatEnabled(bool bEnabled)
{
	bProximityChatEnabled = bEnabled;
	UpdateProximityChatDisplay();
}

void UCooperativeWidget::UpdateProximityVolume(float Volume)
{
	ProximityVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	UpdateProximityChatDisplay();
}

void UCooperativeWidget::TriggerEmergencyAlert()
{
	if (!CanSendEmergencyAlert()) return;

	// Send emergency callout
	SendCallout(ECalloutType::EmergencyStop, TEXT("EMERGENCY ALERT - IMMEDIATE ASSISTANCE REQUIRED"));

	// Update cooldown
	LastEmergencyAlert = GetWorld()->GetTimeSeconds();

	// Broadcast assistance request
	OnTeammateAssistanceRequest.Broadcast(TEXT("Emergency"));
}

void UCooperativeWidget::RequestAssistance()
{
	SendCallout(ECalloutType::NeedHelp, TEXT("I need assistance"));
	OnTeammateAssistanceRequest.Broadcast(TEXT("General"));
}

// Button event handlers
void UCooperativeWidget::OnSafeToClimbClicked()
{
	SendCallout(ECalloutType::SafeToClimb);
}

void UCooperativeWidget::OnTakeSlackClicked()
{
	SendCallout(ECalloutType::TakeSlack);
}

void UCooperativeWidget::OnGiveSlackClicked()
{
	SendCallout(ECalloutType::GiveSlack);
}

void UCooperativeWidget::OnLowerMeClicked()
{
	SendCallout(ECalloutType::LowerMe);
}

void UCooperativeWidget::OnRopeFixedClicked()
{
	SendCallout(ECalloutType::RopeFixed);
}

void UCooperativeWidget::OnWatchRockfallClicked()
{
	SendCallout(ECalloutType::WatchForRockfall);
}

void UCooperativeWidget::OnNeedHelpClicked()
{
	RequestAssistance();
}

void UCooperativeWidget::OnAllClearClicked()
{
	SendCallout(ECalloutType::AllClear);
}

void UCooperativeWidget::OnEmergencyStopClicked()
{
	TriggerEmergencyAlert();
}

void UCooperativeWidget::OnProximityChatToggleClicked()
{
	SetProximityChatEnabled(!bProximityChatEnabled);
}

void UCooperativeWidget::OnEmergencyAlertClicked()
{
	TriggerEmergencyAlert();
}

void UCooperativeWidget::UpdateTeammateStatusDisplay()
{
	if (!TeammateStatusContainer || !TeammateStatusWidgetClass) return;

	// Clear existing widgets
	TeammateStatusContainer->ClearChildren();

	// Create widgets for each teammate
	for (const auto& TeammateEntry : Teammates)
	{
		const FString& PlayerName = TeammateEntry.Key;
		const FTeammateInfo& Info = TeammateEntry.Value;

		if (UUserWidget* TeammateWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), TeammateStatusWidgetClass))
		{
			TeammateStatusContainer->AddChild(TeammateWidget);
			
			// TODO: Initialize teammate widget with data
			// This would typically involve calling setup functions on the widget
		}
	}
}

void UCooperativeWidget::UpdateCalloutHistoryDisplay()
{
	if (!CalloutHistoryContainer || !CalloutMessageWidgetClass) return;

	// Clear existing widgets
	CalloutHistoryContainer->ClearChildren();

	// Create widgets for recent callouts
	int32 DisplayCount = FMath::Min(CalloutHistory.Num(), MaxCalloutHistory);
	for (int32 i = CalloutHistory.Num() - DisplayCount; i < CalloutHistory.Num(); i++)
	{
		const FCalloutMessage& Message = CalloutHistory[i];
		
		if (UUserWidget* MessageWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), CalloutMessageWidgetClass))
		{
			CalloutHistoryContainer->AddChild(MessageWidget);
			
			// TODO: Initialize message widget with callout data
		}
	}

	// Scroll to bottom
	if (CalloutHistoryScrollBox)
	{
		CalloutHistoryScrollBox->ScrollToEnd();
	}
}

void UCooperativeWidget::UpdateRopeStatusDisplay()
{
	if (RopeStatusText)
	{
		FString StatusText;
		switch (CurrentRopeStatus)
		{
		case ERopeStatus::Secure:
			StatusText = TEXT("Secure");
			break;
		case ERopeStatus::Tensioned:
			StatusText = TEXT("Tensioned");
			break;
		case ERopeStatus::Slack:
			StatusText = TEXT("Slack");
			break;
		case ERopeStatus::Tangled:
			StatusText = TEXT("Tangled");
			break;
		case ERopeStatus::Damaged:
			StatusText = TEXT("Damaged");
			break;
		default:
			StatusText = TEXT("Unknown");
			break;
		}
		
		RopeStatusText->SetText(FText::FromString(StatusText));
		
		// Color code based on status
		FLinearColor StatusColor = FLinearColor::White;
		if (CurrentRopeStatus == ERopeStatus::Damaged || CurrentRopeStatus == ERopeStatus::Tangled)
		{
			StatusColor = FLinearColor::Red;
		}
		else if (CurrentRopeStatus == ERopeStatus::Slack)
		{
			StatusColor = FLinearColor::Yellow;
		}
		else if (CurrentRopeStatus == ERopeStatus::Secure)
		{
			StatusColor = FLinearColor::Green;
		}
		
		RopeStatusText->SetColorAndOpacity(StatusColor);
	}

	if (RopeTensionBar)
	{
		RopeTensionBar->SetPercent(CurrentRopeTension);
		
		// Color based on tension level
		FLinearColor TensionColor = FLinearColor::Green;
		if (CurrentRopeTension > 0.8f)
		{
			TensionColor = FLinearColor::Red;
		}
		else if (CurrentRopeTension > 0.6f)
		{
			TensionColor = FLinearColor::Yellow;
		}
		
		RopeTensionBar->SetFillColorAndOpacity(TensionColor);
	}

	if (RopeLengthText)
	{
		FString LengthText = FString::Printf(TEXT("Length: %.1fm"), CurrentRopeLength);
		RopeLengthText->SetText(FText::FromString(LengthText));
	}
}

void UCooperativeWidget::UpdateProximityChatDisplay()
{
	if (ProximityChatStatusText)
	{
		FString StatusText = bProximityChatEnabled ? TEXT("Proximity Chat: ON") : TEXT("Proximity Chat: OFF");
		ProximityChatStatusText->SetText(FText::FromString(StatusText));
		ProximityChatStatusText->SetColorAndOpacity(bProximityChatEnabled ? FLinearColor::Green : FLinearColor::Red);
	}

	if (ProximityVolumeBar)
	{
		ProximityVolumeBar->SetPercent(ProximityVolume);
	}

	if (ProximityChatToggle)
	{
		// Update button appearance based on state
		// This would typically be done through styling
	}
}

void UCooperativeWidget::AddCalloutToHistory(const FCalloutMessage& Message)
{
	CalloutHistory.Add(Message);

	// Remove old messages if history is too long
	while (CalloutHistory.Num() > MaxCalloutHistory * 2) // Keep double for buffer
	{
		CalloutHistory.RemoveAt(0);
	}

	UpdateCalloutHistoryDisplay();
}

FString UCooperativeWidget::GetCalloutText(ECalloutType CalloutType) const
{
	switch (CalloutType)
	{
	case ECalloutType::SafeToClimb:
		return TEXT("Safe to climb!");
	case ECalloutType::TakeSlack:
		return TEXT("Take in slack!");
	case ECalloutType::GiveSlack:
		return TEXT("Give me slack!");
	case ECalloutType::LowerMe:
		return TEXT("Lower me!");
	case ECalloutType::RopeFixed:
		return TEXT("Rope is fixed!");
	case ECalloutType::WatchForRockfall:
		return TEXT("Watch for rockfall!");
	case ECalloutType::NeedHelp:
		return TEXT("I need help!");
	case ECalloutType::AllClear:
		return TEXT("All clear!");
	case ECalloutType::EmergencyStop:
		return TEXT("STOP! EMERGENCY!");
	default:
		return TEXT("Message");
	}
}

FLinearColor UCooperativeWidget::GetCalloutColor(ECalloutType CalloutType) const
{
	switch (CalloutType)
	{
	case ECalloutType::EmergencyStop:
	case ECalloutType::NeedHelp:
		return FLinearColor::Red;
	case ECalloutType::WatchForRockfall:
		return FLinearColor::Yellow;
	case ECalloutType::AllClear:
	case ECalloutType::SafeToClimb:
		return FLinearColor::Green;
	default:
		return FLinearColor::White;
	}
}

bool UCooperativeWidget::CanSendEmergencyAlert() const
{
	if (!GetWorld()) return false;
	
	float TimeSinceLastAlert = GetWorld()->GetTimeSeconds() - LastEmergencyAlert;
	return TimeSinceLastAlert >= EmergencyAlertCooldown;
}