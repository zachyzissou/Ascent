#include "ClimbingVoiceChat.h"
#include "ClimbingPlayerState.h"
#include "ClimbingGameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"

UClimbingVoiceChat::UClimbingVoiceChat()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    // Initialize default settings
    VoiceSettings.ProximityRange = 1000.0f;
    VoiceSettings.WhisperRange = 300.0f;
    VoiceSettings.ShoutRange = 2000.0f;
    VoiceSettings.VolumeMultiplier = 1.0f;
    VoiceSettings.bEnableProximityChat = true;
    VoiceSettings.bEnableRadioChat = true;
    VoiceSettings.bEnableEmergencyChannel = true;
    VoiceSettings.Quality = EVoiceChatQuality::Medium;
    
    // Initialize state
    bIsTransmitting = false;
    CurrentChannel = EVoiceChatChannel::Proximity;
    bIsMuted = false;
    MicrophoneGain = 1.0f;
    RadioFrequency = 1;
    bEmergencyChannelActive = false;
    bNoiseReductionEnabled = true;
    bEchoCancellationEnabled = true;
    CompressionRatio = 0.5f;
    
    OwningPlayerState = nullptr;
    LastTransmissionTime = 0.0f;
    TransmissionTimeout = 5.0f;
    LastNetworkQualityCheck = 0.0f;
    NetworkQualityCheckInterval = 10.0f;
}

void UClimbingVoiceChat::BeginPlay()
{
    Super::BeginPlay();
    
    OwningPlayerState = GetOwningClimbingPlayerState();
    
    // Create default sound attenuation settings if not provided
    if (!ProximityAttenuation)
    {
        ProximityAttenuation = NewObject<USoundAttenuation>(this);
        ProximityAttenuation->Attenuation.bAttenuate = true;
        ProximityAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        ProximityAttenuation->Attenuation.AttenuationShapeExtents = FVector(VoiceSettings.ProximityRange);
        ProximityAttenuation->Attenuation.FalloffDistance = VoiceSettings.ProximityRange * 0.8f;
    }
    
    if (!WhisperAttenuation)
    {
        WhisperAttenuation = NewObject<USoundAttenuation>(this);
        WhisperAttenuation->Attenuation.bAttenuate = true;
        WhisperAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        WhisperAttenuation->Attenuation.AttenuationShapeExtents = FVector(VoiceSettings.WhisperRange);
        WhisperAttenuation->Attenuation.FalloffDistance = VoiceSettings.WhisperRange * 0.7f;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: Initialized for player %s"), 
           OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"));
}

void UClimbingVoiceChat::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (GetOwner()->HasAuthority())
    {
        UpdateActiveTransmissions();
        CleanupInactiveTransmissions();
        
        // Check for transmission timeout
        if (bIsTransmitting && (GetWorld()->GetTimeSeconds() - LastTransmissionTime) > TransmissionTimeout)
        {
            StopTransmission();
        }
        
        // Periodic network quality check
        if ((GetWorld()->GetTimeSeconds() - LastNetworkQualityCheck) > NetworkQualityCheckInterval)
        {
            // Here you would implement network quality assessment
            LastNetworkQualityCheck = GetWorld()->GetTimeSeconds();
        }
    }
    
    ProcessVoiceInput(DeltaTime);
    UpdateAudioComponents();
}

void UClimbingVoiceChat::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UClimbingVoiceChat, bIsTransmitting);
    DOREPLIFETIME(UClimbingVoiceChat, CurrentChannel);
    DOREPLIFETIME(UClimbingVoiceChat, ActiveTransmissions);
    DOREPLIFETIME(UClimbingVoiceChat, bIsMuted);
    DOREPLIFETIME(UClimbingVoiceChat, MicrophoneGain);
    DOREPLIFETIME(UClimbingVoiceChat, RadioFrequency);
    DOREPLIFETIME(UClimbingVoiceChat, bEmergencyChannelActive);
    DOREPLIFETIME(UClimbingVoiceChat, bNoiseReductionEnabled);
    DOREPLIFETIME(UClimbingVoiceChat, bEchoCancellationEnabled);
    DOREPLIFETIME(UClimbingVoiceChat, CompressionRatio);
}

bool UClimbingVoiceChat::StartTransmission(EVoiceChatChannel Channel)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerStartTransmission(Channel);
        return false;
    }
    
    if (bIsMuted || bIsTransmitting)
        return false;
    
    if (!ValidateTransmissionChannel(Channel))
        return false;
    
    bIsTransmitting = true;
    CurrentChannel = Channel;
    LastTransmissionTime = GetWorld()->GetTimeSeconds();
    
    // Create transmission record
    FVoiceTransmission NewTransmission;
    NewTransmission.Speaker = OwningPlayerState;
    NewTransmission.Channel = Channel;
    NewTransmission.SourceLocation = GetOwner()->GetActorLocation();
    NewTransmission.Volume = MicrophoneGain * VoiceSettings.VolumeMultiplier;
    NewTransmission.Timestamp = LastTransmissionTime;
    NewTransmission.bIsActive = true;
    
    ActiveTransmissions.Add(NewTransmission);
    
    // Broadcast to clients
    MulticastVoiceTransmissionStarted(OwningPlayerState, Channel, NewTransmission.SourceLocation);
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: %s started transmission on %s channel"), 
           *OwningPlayerState->GetPlayerName(), *UEnum::GetValueAsString(Channel));
    
    return true;
}

bool UClimbingVoiceChat::StopTransmission()
{
    if (!GetOwner()->HasAuthority())
    {
        ServerStopTransmission();
        return false;
    }
    
    if (!bIsTransmitting)
        return false;
    
    EVoiceChatChannel StoppedChannel = CurrentChannel;
    bIsTransmitting = false;
    
    // Find and deactivate the transmission
    for (FVoiceTransmission& Transmission : ActiveTransmissions)
    {
        if (Transmission.Speaker == OwningPlayerState && Transmission.bIsActive)
        {
            Transmission.bIsActive = false;
            break;
        }
    }
    
    // Broadcast to clients
    MulticastVoiceTransmissionEnded(OwningPlayerState, StoppedChannel);
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: %s stopped transmission"), 
           *OwningPlayerState->GetPlayerName());
    
    return true;
}

bool UClimbingVoiceChat::SwitchChannel(EVoiceChatChannel NewChannel)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerSwitchChannel(NewChannel);
        return false;
    }
    
    if (!ValidateTransmissionChannel(NewChannel))
        return false;
    
    EVoiceChatChannel OldChannel = CurrentChannel;
    CurrentChannel = NewChannel;
    
    // If currently transmitting, update the active transmission
    if (bIsTransmitting)
    {
        for (FVoiceTransmission& Transmission : ActiveTransmissions)
        {
            if (Transmission.Speaker == OwningPlayerState && Transmission.bIsActive)
            {
                Transmission.Channel = NewChannel;
                break;
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: %s switched from %s to %s channel"), 
           *OwningPlayerState->GetPlayerName(), 
           *UEnum::GetValueAsString(OldChannel),
           *UEnum::GetValueAsString(NewChannel));
    
    return true;
}

bool UClimbingVoiceChat::IsTransmittingOnChannel(EVoiceChatChannel Channel) const
{
    return bIsTransmitting && CurrentChannel == Channel;
}

void UClimbingVoiceChat::SetMuted(bool bMute)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerSetMuted(bMute);
        return;
    }
    
    bool bOldMuted = bIsMuted;
    bIsMuted = bMute;
    
    // If muted while transmitting, stop transmission
    if (bMute && bIsTransmitting)
    {
        StopTransmission();
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: %s %s"), 
           *OwningPlayerState->GetPlayerName(), bMute ? TEXT("muted") : TEXT("unmuted"));
}

void UClimbingVoiceChat::SetMicrophoneGain(float Gain)
{
    MicrophoneGain = FMath::Clamp(Gain, 0.0f, 2.0f);
}

void UClimbingVoiceChat::SetVoiceSettings(const FVoiceChatSettings& NewSettings)
{
    FVoiceChatSettings OldSettings = VoiceSettings;
    VoiceSettings = NewSettings;
    
    OnVoiceChatSettingsChanged.Broadcast(OwningPlayerState, NewSettings, OldSettings);
}

float UClimbingVoiceChat::CalculateVoiceVolume(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel) const
{
    if (!Speaker || !OwningPlayerState)
        return 0.0f;
    
    // Radio and emergency channels have full volume regardless of distance
    if (Channel == EVoiceChatChannel::Radio || Channel == EVoiceChatChannel::Emergency)
    {
        return VoiceSettings.VolumeMultiplier;
    }
    
    // Calculate distance-based attenuation for proximity channels
    APawn* SpeakerPawn = Speaker->GetPawn();
    APawn* ListenerPawn = OwningPlayerState->GetPawn();
    
    if (!SpeakerPawn || !ListenerPawn)
        return 0.0f;
    
    float Distance = FVector::Dist(SpeakerPawn->GetActorLocation(), ListenerPawn->GetActorLocation());
    float MaxRange = GetChannelRange(Channel);
    
    if (Distance > MaxRange)
        return 0.0f;
    
    float Attenuation = CalculateDistanceAttenuation(Distance, MaxRange);
    return Attenuation * VoiceSettings.VolumeMultiplier;
}

bool UClimbingVoiceChat::CanHearPlayer(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel) const
{
    if (!Speaker || !OwningPlayerState || Speaker == OwningPlayerState)
        return false;
    
    // Check if the channel is enabled
    switch (Channel)
    {
        case EVoiceChatChannel::Proximity:
        case EVoiceChatChannel::Whisper:
        case EVoiceChatChannel::Shout:
            if (!VoiceSettings.bEnableProximityChat)
                return false;
            break;
        case EVoiceChatChannel::Radio:
            if (!VoiceSettings.bEnableRadioChat)
                return false;
            // Check if on same radio frequency
            if (UClimbingVoiceChat* SpeakerVoiceChat = Speaker->GetPawn()->FindComponentByClass<UClimbingVoiceChat>())
            {
                if (SpeakerVoiceChat->GetCurrentRadioFrequency() != RadioFrequency)
                    return false;
            }
            break;
        case EVoiceChatChannel::Emergency:
            if (!VoiceSettings.bEnableEmergencyChannel)
                return false;
            break;
    }
    
    return CalculateVoiceVolume(Speaker, Channel) > 0.01f;
}

TArray<AClimbingPlayerState*> UClimbingVoiceChat::GetPlayersInRange(EVoiceChatChannel Channel) const
{
    TArray<AClimbingPlayerState*> PlayersInRange;
    
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        for (AClimbingPlayerState* Player : GameState->GetAllClimbingPlayers())
        {
            if (CanHearPlayer(Player, Channel))
            {
                PlayersInRange.Add(Player);
            }
        }
    }
    
    return PlayersInRange;
}

bool UClimbingVoiceChat::BroadcastEmergencyMessage(const FString& Message)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerBroadcastEmergency(Message);
        return false;
    }
    
    if (!VoiceSettings.bEnableEmergencyChannel)
        return false;
    
    MulticastEmergencyBroadcast(OwningPlayerState, Message);
    
    UE_LOG(LogTemp, Warning, TEXT("ClimbingVoiceChat: Emergency broadcast from %s: %s"), 
           *OwningPlayerState->GetPlayerName(), *Message);
    
    return true;
}

bool UClimbingVoiceChat::ActivateEmergencyChannel()
{
    if (!GetOwner()->HasAuthority())
        return false;
    
    bEmergencyChannelActive = true;
    CurrentChannel = EVoiceChatChannel::Emergency;
    
    UE_LOG(LogTemp, Warning, TEXT("ClimbingVoiceChat: Emergency channel activated for %s"), 
           *OwningPlayerState->GetPlayerName());
    
    return true;
}

void UClimbingVoiceChat::DeactivateEmergencyChannel()
{
    if (!GetOwner()->HasAuthority())
        return;
    
    bEmergencyChannelActive = false;
    if (CurrentChannel == EVoiceChatChannel::Emergency)
    {
        CurrentChannel = EVoiceChatChannel::Proximity;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: Emergency channel deactivated for %s"), 
           *OwningPlayerState->GetPlayerName());
}

bool UClimbingVoiceChat::TuneRadioFrequency(int32 Frequency)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerTuneRadioFrequency(Frequency);
        return false;
    }
    
    if (Frequency < 1 || Frequency > 99)
        return false;
    
    RadioFrequency = Frequency;
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingVoiceChat: %s tuned to radio frequency %d"), 
           *OwningPlayerState->GetPlayerName(), Frequency);
    
    return true;
}

bool UClimbingVoiceChat::IsOnSameRadioFrequency(AClimbingPlayerState* OtherPlayer) const
{
    if (!OtherPlayer || !OtherPlayer->GetPawn())
        return false;
    
    if (UClimbingVoiceChat* OtherVoiceChat = OtherPlayer->GetPawn()->FindComponentByClass<UClimbingVoiceChat>())
    {
        return OtherVoiceChat->GetCurrentRadioFrequency() == RadioFrequency;
    }
    
    return false;
}

void UClimbingVoiceChat::AdaptQualityToNetworkConditions(float Latency, float PacketLoss)
{
    // Adapt voice quality based on network conditions
    if (VoiceSettings.Quality == EVoiceChatQuality::Adaptive)
    {
        if (Latency > 200.0f || PacketLoss > 0.05f) // High latency or >5% packet loss
        {
            SetVoiceQuality(EVoiceChatQuality::Low);
        }
        else if (Latency > 100.0f || PacketLoss > 0.02f) // Medium latency or >2% packet loss
        {
            SetVoiceQuality(EVoiceChatQuality::Medium);
        }
        else
        {
            SetVoiceQuality(EVoiceChatQuality::High);
        }
    }
}

void UClimbingVoiceChat::SetVoiceQuality(EVoiceChatQuality Quality)
{
    VoiceSettings.Quality = Quality;
    
    // Apply quality settings to voice processing
    switch (Quality)
    {
        case EVoiceChatQuality::Low:
            CompressionRatio = 0.8f;
            break;
        case EVoiceChatQuality::Medium:
            CompressionRatio = 0.5f;
            break;
        case EVoiceChatQuality::High:
            CompressionRatio = 0.2f;
            break;
        case EVoiceChatQuality::Adaptive:
            // Quality will be automatically adjusted
            break;
    }
}

void UClimbingVoiceChat::EnableNoiseReduction(bool bEnable)
{
    bNoiseReductionEnabled = bEnable;
}

void UClimbingVoiceChat::EnableEchoCancellation(bool bEnable)
{
    bEchoCancellationEnabled = bEnable;
}

void UClimbingVoiceChat::SetAudioCompression(float CompressionRatio)
{
    CompressionRatio = FMath::Clamp(CompressionRatio, 0.0f, 1.0f);
}

// Network RPC implementations
void UClimbingVoiceChat::ServerStartTransmission_Implementation(EVoiceChatChannel Channel)
{
    StartTransmission(Channel);
}

bool UClimbingVoiceChat::ServerStartTransmission_Validate(EVoiceChatChannel Channel)
{
    return ValidateTransmissionChannel(Channel);
}

void UClimbingVoiceChat::ServerStopTransmission_Implementation()
{
    StopTransmission();
}

bool UClimbingVoiceChat::ServerStopTransmission_Validate()
{
    return true;
}

void UClimbingVoiceChat::ServerSwitchChannel_Implementation(EVoiceChatChannel NewChannel)
{
    SwitchChannel(NewChannel);
}

bool UClimbingVoiceChat::ServerSwitchChannel_Validate(EVoiceChatChannel NewChannel)
{
    return ValidateTransmissionChannel(NewChannel);
}

void UClimbingVoiceChat::ServerSetMuted_Implementation(bool bMute)
{
    SetMuted(bMute);
}

bool UClimbingVoiceChat::ServerSetMuted_Validate(bool bMute)
{
    return true;
}

void UClimbingVoiceChat::ServerBroadcastEmergency_Implementation(const FString& Message)
{
    BroadcastEmergencyMessage(Message);
}

bool UClimbingVoiceChat::ServerBroadcastEmergency_Validate(const FString& Message)
{
    return !Message.IsEmpty() && Message.Len() <= 256; // Max 256 characters
}

void UClimbingVoiceChat::ServerTuneRadioFrequency_Implementation(int32 Frequency)
{
    TuneRadioFrequency(Frequency);
}

bool UClimbingVoiceChat::ServerTuneRadioFrequency_Validate(int32 Frequency)
{
    return Frequency >= 1 && Frequency <= 99;
}

void UClimbingVoiceChat::MulticastVoiceTransmissionStarted_Implementation(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel, const FVector& Location)
{
    OnVoiceTransmissionStarted.Broadcast(Speaker, Channel);
}

void UClimbingVoiceChat::MulticastVoiceTransmissionEnded_Implementation(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel)
{
    OnVoiceTransmissionEnded.Broadcast(Speaker, Channel);
}

void UClimbingVoiceChat::MulticastEmergencyBroadcast_Implementation(AClimbingPlayerState* Speaker, const FString& Message)
{
    // Handle emergency message UI/notification on all clients
    UE_LOG(LogTemp, Warning, TEXT("ClimbingVoiceChat: Emergency from %s: %s"), 
           *Speaker->GetPlayerName(), *Message);
}

void UClimbingVoiceChat::ClientReceiveVoiceData_Implementation(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel, const TArray<uint8>& VoiceData)
{
    if (!CanHearPlayer(Speaker, Channel))
        return;
    
    // Process and play voice data
    // This would integrate with Unreal's voice chat system or a third-party solution
    
    float Volume = CalculateVoiceVolume(Speaker, Channel);
    USoundAttenuation* Attenuation = GetChannelAttenuation(Channel);
    
    // Create or update audio component for this speaker
    UAudioComponent** AudioCompPtr = PlayerAudioComponents.Find(Speaker);
    UAudioComponent* AudioComp = AudioCompPtr ? *AudioCompPtr : nullptr;
    
    if (!AudioComp && Speaker->GetPawn())
    {
        AudioComp = UGameplayStatics::SpawnSoundAttached(
            nullptr, // Sound would be created from voice data
            Speaker->GetPawn()->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            EAttachLocation::KeepWorldPosition,
            false,
            Volume,
            1.0f,
            0.0f,
            Attenuation
        );
        
        if (AudioComp)
        {
            PlayerAudioComponents.Add(Speaker, AudioComp);
        }
    }
    
    if (AudioComp)
    {
        AudioComp->SetVolumeMultiplier(Volume);
    }
}

// Helper function implementations
AClimbingPlayerState* UClimbingVoiceChat::GetOwningClimbingPlayerState() const
{
    if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
        {
            return Cast<AClimbingPlayerState>(PC->PlayerState);
        }
    }
    return nullptr;
}

float UClimbingVoiceChat::GetChannelRange(EVoiceChatChannel Channel) const
{
    switch (Channel)
    {
        case EVoiceChatChannel::Proximity:
            return VoiceSettings.ProximityRange;
        case EVoiceChatChannel::Whisper:
            return VoiceSettings.WhisperRange;
        case EVoiceChatChannel::Shout:
            return VoiceSettings.ShoutRange;
        case EVoiceChatChannel::Radio:
        case EVoiceChatChannel::Emergency:
            return FLT_MAX; // No range limit
        default:
            return VoiceSettings.ProximityRange;
    }
}

USoundAttenuation* UClimbingVoiceChat::GetChannelAttenuation(EVoiceChatChannel Channel) const
{
    switch (Channel)
    {
        case EVoiceChatChannel::Whisper:
            return WhisperAttenuation;
        case EVoiceChatChannel::Radio:
            return RadioAttenuation;
        case EVoiceChatChannel::Proximity:
        case EVoiceChatChannel::Shout:
        default:
            return ProximityAttenuation;
    }
}

void UClimbingVoiceChat::UpdateActiveTransmissions()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Update transmission locations and volumes
    for (FVoiceTransmission& Transmission : ActiveTransmissions)
    {
        if (Transmission.bIsActive && Transmission.Speaker && Transmission.Speaker->GetPawn())
        {
            Transmission.SourceLocation = Transmission.Speaker->GetPawn()->GetActorLocation();
            Transmission.Volume = VoiceSettings.VolumeMultiplier;
        }
    }
}

void UClimbingVoiceChat::ProcessVoiceInput(float DeltaTime)
{
    // This would integrate with platform-specific voice input systems
    // For now, this is a placeholder for voice processing logic
}

void UClimbingVoiceChat::UpdateAudioComponents()
{
    // Clean up audio components for players who are no longer valid
    TArray<AClimbingPlayerState*> PlayersToRemove;
    
    for (auto& Pair : PlayerAudioComponents)
    {
        if (!IsValid(Pair.Key) || !IsValid(Pair.Value))
        {
            PlayersToRemove.Add(Pair.Key);
        }
    }
    
    for (AClimbingPlayerState* Player : PlayersToRemove)
    {
        if (UAudioComponent* AudioComp = PlayerAudioComponents.FindRef(Player))
        {
            AudioComp->DestroyComponent();
        }
        PlayerAudioComponents.Remove(Player);
    }
}

bool UClimbingVoiceChat::ValidateTransmissionChannel(EVoiceChatChannel Channel) const
{
    switch (Channel)
    {
        case EVoiceChatChannel::Proximity:
        case EVoiceChatChannel::Whisper:
        case EVoiceChatChannel::Shout:
            return VoiceSettings.bEnableProximityChat;
        case EVoiceChatChannel::Radio:
            return VoiceSettings.bEnableRadioChat;
        case EVoiceChatChannel::Emergency:
            return VoiceSettings.bEnableEmergencyChannel;
        default:
            return false;
    }
}

float UClimbingVoiceChat::CalculateDistanceAttenuation(float Distance, float MaxRange) const
{
    if (Distance >= MaxRange)
        return 0.0f;
    
    // Linear falloff with smooth curve
    float NormalizedDistance = Distance / MaxRange;
    return FMath::Max(0.0f, 1.0f - (NormalizedDistance * NormalizedDistance));
}

void UClimbingVoiceChat::CleanupInactiveTransmissions()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    ActiveTransmissions.RemoveAll([CurrentTime](const FVoiceTransmission& Transmission)
    {
        return !Transmission.bIsActive || 
               !IsValid(Transmission.Speaker) || 
               (CurrentTime - Transmission.Timestamp) > 30.0f; // 30 second cleanup
    });
}

// Replication callbacks
void UClimbingVoiceChat::OnRep_IsTransmitting()
{
    // Handle UI updates for transmission status
}

void UClimbingVoiceChat::OnRep_CurrentChannel()
{
    // Handle UI updates for current channel
}

void UClimbingVoiceChat::OnRep_ActiveTransmissions()
{
    // Handle updates to active transmissions display
}

void UClimbingVoiceChat::OnRep_VoiceSettings()
{
    // Handle voice settings changes
}