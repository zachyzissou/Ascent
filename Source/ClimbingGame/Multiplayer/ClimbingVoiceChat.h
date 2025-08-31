#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "ClimbingVoiceChat.generated.h"

class AClimbingPlayerState;
class USoundAttenuation;
class UAudioComponent;

UENUM(BlueprintType)
enum class EVoiceChatChannel : uint8
{
    Proximity,      // 3D positional voice chat
    Radio,          // Team-wide radio communication
    Emergency,      // High-priority emergency channel
    Whisper,        // Close-range quiet communication
    Shout          // Extended range communication
};

UENUM(BlueprintType)
enum class EVoiceChatQuality : uint8
{
    Low,            // 8kHz, low bandwidth
    Medium,         // 16kHz, balanced
    High,           // 44kHz, high quality
    Adaptive        // Adapts based on network conditions
};

USTRUCT(BlueprintType)
struct FVoiceChatSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ProximityRange = 1000.0f; // 10 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WhisperRange = 300.0f; // 3 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShoutRange = 2000.0f; // 20 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float VolumeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableProximityChat = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableRadioChat = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableEmergencyChannel = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EVoiceChatQuality Quality = EVoiceChatQuality::Medium;

    FVoiceChatSettings()
    {
        ProximityRange = 1000.0f;
        WhisperRange = 300.0f;
        ShoutRange = 2000.0f;
        VolumeMultiplier = 1.0f;
        bEnableProximityChat = true;
        bEnableRadioChat = true;
        bEnableEmergencyChannel = true;
        Quality = EVoiceChatQuality::Medium;
    }
};

USTRUCT(BlueprintType)
struct FVoiceTransmission
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Speaker = nullptr;

    UPROPERTY(BlueprintReadOnly)
    EVoiceChatChannel Channel = EVoiceChatChannel::Proximity;

    UPROPERTY(BlueprintReadOnly)
    FVector SourceLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Volume = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    float Timestamp = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = false;

    FVoiceTransmission()
    {
        Speaker = nullptr;
        Channel = EVoiceChatChannel::Proximity;
        SourceLocation = FVector::ZeroVector;
        Volume = 1.0f;
        Timestamp = 0.0f;
        bIsActive = false;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceTransmissionStarted, AClimbingPlayerState*, Speaker, EVoiceChatChannel, Channel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceTransmissionEnded, AClimbingPlayerState*, Speaker, EVoiceChatChannel, Channel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVoiceChatSettingsChanged, AClimbingPlayerState*, Player, FVoiceChatSettings, NewSettings, FVoiceChatSettings, OldSettings);

/**
 * Component that handles proximity-based voice chat and radio communication
 * Provides 3D positional voice chat with range-based attenuation
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UClimbingVoiceChat : public UActorComponent
{
    GENERATED_BODY()

public:
    UClimbingVoiceChat();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Voice chat settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Chat")
    FVoiceChatSettings VoiceSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Chat")
    USoundAttenuation* ProximityAttenuation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Chat")
    USoundAttenuation* RadioAttenuation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Chat")
    USoundAttenuation* WhisperAttenuation = nullptr;

    // Current state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    bool bIsTransmitting = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    EVoiceChatChannel CurrentChannel = EVoiceChatChannel::Proximity;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    TArray<FVoiceTransmission> ActiveTransmissions;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    bool bIsMuted = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    float MicrophoneGain = 1.0f;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnVoiceTransmissionStarted OnVoiceTransmissionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnVoiceTransmissionEnded OnVoiceTransmissionEnded;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnVoiceChatSettingsChanged OnVoiceChatSettingsChanged;

    // Voice transmission control
    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    bool StartTransmission(EVoiceChatChannel Channel = EVoiceChatChannel::Proximity);

    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    bool StopTransmission();

    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    bool SwitchChannel(EVoiceChatChannel NewChannel);

    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    bool IsTransmittingOnChannel(EVoiceChatChannel Channel) const;

    // Audio settings
    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    void SetMuted(bool bMute);

    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    void SetMicrophoneGain(float Gain);

    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    void SetVoiceSettings(const FVoiceChatSettings& NewSettings);

    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    bool IsMuted() const { return bIsMuted; }

    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    float GetMicrophoneGain() const { return MicrophoneGain; }

    // Proximity calculations
    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    float CalculateVoiceVolume(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel) const;

    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    bool CanHearPlayer(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel) const;

    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    TArray<AClimbingPlayerState*> GetPlayersInRange(EVoiceChatChannel Channel) const;

    // Emergency communication
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    bool BroadcastEmergencyMessage(const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "Emergency")
    bool ActivateEmergencyChannel();

    UFUNCTION(BlueprintCallable, Category = "Emergency")
    void DeactivateEmergencyChannel();

    // Radio communication
    UFUNCTION(BlueprintCallable, Category = "Radio")
    bool TuneRadioFrequency(int32 Frequency);

    UFUNCTION(BlueprintCallable, Category = "Radio")
    bool IsOnSameRadioFrequency(AClimbingPlayerState* OtherPlayer) const;

    UFUNCTION(BlueprintPure, Category = "Radio")
    int32 GetCurrentRadioFrequency() const { return RadioFrequency; }

    // Network quality adaptation
    UFUNCTION(BlueprintCallable, Category = "Quality")
    void AdaptQualityToNetworkConditions(float Latency, float PacketLoss);

    UFUNCTION(BlueprintCallable, Category = "Quality")
    void SetVoiceQuality(EVoiceChatQuality Quality);

    // Audio processing
    UFUNCTION(BlueprintCallable, Category = "Processing")
    void EnableNoiseReduction(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Processing")
    void EnableEchoCancellation(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Processing")
    void SetAudioCompression(float CompressionRatio);

    // Network RPCs
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartTransmission(EVoiceChatChannel Channel);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStopTransmission();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSwitchChannel(EVoiceChatChannel NewChannel);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetMuted(bool bMute);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerBroadcastEmergency(const FString& Message);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTuneRadioFrequency(int32 Frequency);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastVoiceTransmissionStarted(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel, const FVector& Location);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastVoiceTransmissionEnded(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastEmergencyBroadcast(AClimbingPlayerState* Speaker, const FString& Message);

    UFUNCTION(Client, Reliable)
    void ClientReceiveVoiceData(AClimbingPlayerState* Speaker, EVoiceChatChannel Channel, const TArray<uint8>& VoiceData);

protected:
    // Internal state
    UPROPERTY(Replicated)
    int32 RadioFrequency = 1;

    UPROPERTY(Replicated)
    bool bEmergencyChannelActive = false;

    UPROPERTY(Replicated)
    bool bNoiseReductionEnabled = true;

    UPROPERTY(Replicated)
    bool bEchoCancellationEnabled = true;

    UPROPERTY(Replicated)
    float CompressionRatio = 0.5f;

    // Audio components for playback
    UPROPERTY()
    TMap<AClimbingPlayerState*, UAudioComponent*> PlayerAudioComponents;

    // Cached references
    UPROPERTY()
    AClimbingPlayerState* OwningPlayerState = nullptr;

    // Helper functions
    AClimbingPlayerState* GetOwningClimbingPlayerState() const;
    float GetChannelRange(EVoiceChatChannel Channel) const;
    USoundAttenuation* GetChannelAttenuation(EVoiceChatChannel Channel) const;
    void UpdateActiveTransmissions();
    void ProcessVoiceInput(float DeltaTime);
    void UpdateAudioComponents();
    bool ValidateTransmissionChannel(EVoiceChatChannel Channel) const;
    float CalculateDistanceAttenuation(float Distance, float MaxRange) const;
    void CleanupInactiveTransmissions();

    // Replication callbacks
    UFUNCTION()
    void OnRep_IsTransmitting();

    UFUNCTION()
    void OnRep_CurrentChannel();

    UFUNCTION()
    void OnRep_ActiveTransmissions();

    UFUNCTION()
    void OnRep_VoiceSettings();

private:
    // Voice processing state
    float LastTransmissionTime = 0.0f;
    float TransmissionTimeout = 5.0f; // 5 seconds timeout
    
    // Network optimization
    float LastNetworkQualityCheck = 0.0f;
    float NetworkQualityCheckInterval = 10.0f; // 10 seconds
};