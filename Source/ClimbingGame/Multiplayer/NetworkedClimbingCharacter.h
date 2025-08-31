#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "CooperativeSystem.h"
#include "ClimbingVoiceChat.h"
#include "NetworkedRopeComponent.h"
#include "ClimbingPlayerState.h"
#include "NetworkedClimbingCharacter.generated.h"

class AClimbingPlayerState;
class UCooperativeInventory;
class UAdvancedClimbingComponent;

UENUM(BlueprintType)
enum class EPlayerNetworkRole : uint8
{
    ClimbingLeader,     // Sets routes and anchors
    BelaySpecialist,    // Focuses on rope management
    SupportClimber,     // Assists with tools and spotting
    Follower           // Regular team member
};

USTRUCT(BlueprintType)
struct FNetworkPlayerStats
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    float NetworkLatency = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float PacketLoss = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 ClimbingSyncErrors = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float LastSyncTime = 0.0f;

    FNetworkPlayerStats()
    {
        NetworkLatency = 0.0f;
        PacketLoss = 0.0f;
        ClimbingSyncErrors = 0;
        LastSyncTime = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FClimbingPredictionState
{
    GENERATED_BODY()

    UPROPERTY()
    FVector PredictedLocation = FVector::ZeroVector;

    UPROPERTY()
    FRotator PredictedRotation = FRotator::ZeroRotator;

    UPROPERTY()
    FClimbingState PredictedClimbingState;

    UPROPERTY()
    float PredictionTimestamp = 0.0f;

    UPROPERTY()
    float PredictionConfidence = 1.0f;

    FClimbingPredictionState()
    {
        PredictedLocation = FVector::ZeroVector;
        PredictedRotation = FRotator::ZeroRotator;
        PredictionTimestamp = 0.0f;
        PredictionConfidence = 1.0f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClimbingSyncError, ANetworkedClimbingCharacter*, Character, int32, ErrorType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCooperativeClimbingAction, ANetworkedClimbingCharacter*, Helper, ANetworkedClimbingCharacter*, Assisted, FString, ActionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerRoleChanged, ANetworkedClimbingCharacter*, Player, EPlayerNetworkRole, NewRole);

/**
 * Networked climbing character with multiplayer synchronization and cooperative features
 * Extends base climbing with prediction, lag compensation, and team coordination
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API ANetworkedClimbingCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ANetworkedClimbingCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;

public:
    // Core multiplayer components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
    UAdvancedClimbingComponent* ClimbingComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooperation")
    UCooperativeSystem* CooperativeSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice Chat")
    UClimbingVoiceChat* VoiceChat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    UCooperativeInventory* CooperativeInventory;

    // Network synchronization
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network")
    FNetworkPlayerStats NetworkStats;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network")
    EPlayerNetworkRole NetworkRole = EPlayerNetworkRole::Follower;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network")
    bool bIsNetworkOptimized = true;

    // Climbing prediction and smoothing
    UPROPERTY(BlueprintReadOnly, Category = "Prediction")
    FClimbingPredictionState ClientPrediction;

    UPROPERTY(BlueprintReadOnly, Category = "Prediction")
    bool bUseClientPrediction = true;

    UPROPERTY(BlueprintReadOnly, Category = "Prediction")
    float PredictionCorrectionThreshold = 10.0f; // cm

    // Cooperative climbing state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperation")
    ANetworkedClimbingCharacter* BelayPartner = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperation")
    TArray<ANetworkedClimbingCharacter*> TrustedClimbingPartners;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperation")
    UNetworkedRopeComponent* SharedRope = nullptr;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnClimbingSyncError OnClimbingSyncError;

    UPROPERTY(BlueprintAssignable, Category = "Cooperation Events")
    FOnCooperativeClimbingAction OnCooperativeClimbingAction;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnPlayerRoleChanged OnPlayerRoleChanged;

    // Network role management
    UFUNCTION(BlueprintCallable, Category = "Network Role")
    void SetNetworkRole(EPlayerNetworkRole NewRole);

    UFUNCTION(BlueprintPure, Category = "Network Role")
    EPlayerNetworkRole GetNetworkRole() const { return NetworkRole; }

    UFUNCTION(BlueprintCallable, Category = "Network Role")
    bool CanPerformRoleAction(EPlayerNetworkRole RequiredRole) const;

    // Climbing synchronization
    UFUNCTION(BlueprintCallable, Category = "Climbing Sync")
    void RequestClimbingSynchronization();

    UFUNCTION(BlueprintCallable, Category = "Climbing Sync")
    void ValidateClimbingState();

    UFUNCTION(BlueprintCallable, Category = "Climbing Sync")
    bool IsClimbingStateSynced() const;

    // Cooperative climbing actions
    UFUNCTION(BlueprintCallable, Category = "Cooperative Climbing")
    bool RequestBelay(ANetworkedClimbingCharacter* Climber);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Climbing")
    bool AcceptBelayRequest(ANetworkedClimbingCharacter* Belayer);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Climbing")
    bool StartBelaying(ANetworkedClimbingCharacter* Climber);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Climbing")
    bool StopBelaying();

    UFUNCTION(BlueprintCallable, Category = "Cooperative Climbing")
    bool ShareRope(ANetworkedClimbingCharacter* Partner, UNetworkedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Climbing")
    bool AssistClimbing(ANetworkedClimbingCharacter* Climber, const FVector& AssistLocation);

    // Advanced cooperative moves
    UFUNCTION(BlueprintCallable, Category = "Advanced Cooperation")
    bool PerformCooperativeMantle(ANetworkedClimbingCharacter* Partner);

    UFUNCTION(BlueprintCallable, Category = "Advanced Cooperation")
    bool CreateHumanLadder(ANetworkedClimbingCharacter* BottomClimber);

    UFUNCTION(BlueprintCallable, Category = "Advanced Cooperation")
    bool PerformTension(ANetworkedClimbingCharacter* Partner, float TensionForce);

    // Communication integration
    UFUNCTION(BlueprintCallable, Category = "Communication")
    bool SendClimbingSignal(const FString& SignalType, ANetworkedClimbingCharacter* TargetPlayer = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Communication")
    bool BroadcastRouteInformation(const TArray<FVector>& RoutePoints);

    UFUNCTION(BlueprintCallable, Category = "Communication")
    void CallForHelp(const FString& EmergencyType, const FVector& Location);

    // Network optimization
    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeNetworkUpdates(bool bOptimize);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void SetUpdateFrequency(float NewFrequency);

    UFUNCTION(BlueprintPure, Category = "Network Optimization")
    float GetNetworkImportance() const;

    // Prediction and interpolation
    UFUNCTION(BlueprintCallable, Category = "Prediction")
    void EnableClientPrediction(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    FVector PredictClimbingMovement(float PredictionTime) const;

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    void CorrectPredictionError(const FVector& ServerLocation);

    // Trust system integration
    UFUNCTION(BlueprintCallable, Category = "Trust System")
    void AddTrustedPartner(ANetworkedClimbingCharacter* Partner);

    UFUNCTION(BlueprintCallable, Category = "Trust System")
    void RemoveTrustedPartner(ANetworkedClimbingCharacter* Partner);

    UFUNCTION(BlueprintPure, Category = "Trust System")
    bool IsTrustedPartner(ANetworkedClimbingCharacter* Partner) const;

    UFUNCTION(BlueprintPure, Category = "Trust System")
    float GetTrustLevel(ANetworkedClimbingCharacter* Partner) const;

    // Network RPCs for climbing actions
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetNetworkRole(EPlayerNetworkRole NewRole);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestBelay(ANetworkedClimbingCharacter* Climber);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartBelaying(ANetworkedClimbingCharacter* Climber);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerShareRope(ANetworkedClimbingCharacter* Partner, UNetworkedRopeComponent* Rope);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAssistClimbing(ANetworkedClimbingCharacter* Climber, const FVector& AssistLocation);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPerformCooperativeMantle(ANetworkedClimbingCharacter* Partner);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSendClimbingSignal(const FString& SignalType, ANetworkedClimbingCharacter* TargetPlayer);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerCallForHelp(const FString& EmergencyType, const FVector& Location);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerValidateClimbingState(const FClimbingState& ClientState);

    // Multicast RPCs for synchronized events
    UFUNCTION(NetMulticast, Reliable)
    void MulticastBelayStarted(ANetworkedClimbingCharacter* Belayer, ANetworkedClimbingCharacter* Climber);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastRopeShared(ANetworkedClimbingCharacter* Sharer, ANetworkedClimbingCharacter* Receiver, UNetworkedRopeComponent* Rope);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastCooperativeAction(ANetworkedClimbingCharacter* Helper, ANetworkedClimbingCharacter* Assisted, const FString& ActionType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastEmergencyCall(ANetworkedClimbingCharacter* Caller, const FString& EmergencyType, const FVector& Location);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastClimbingSignal(ANetworkedClimbingCharacter* Sender, const FString& SignalType, const FVector& Location);

    // Client correction RPCs
    UFUNCTION(Client, Reliable)
    void ClientCorrectClimbingState(const FClimbingState& AuthorativeState, float Timestamp);

    UFUNCTION(Client, Reliable)
    void ClientReceiveClimbingSignal(ANetworkedClimbingCharacter* Sender, const FString& SignalType);

    UFUNCTION(Client, Reliable)
    void ClientReceiveEmergencyCall(ANetworkedClimbingCharacter* Caller, const FString& EmergencyType, const FVector& Location);

protected:
    // Network synchronization internals
    void UpdateNetworkStats();
    void ProcessClientPrediction(float DeltaTime);
    void InterpolateMovement(float DeltaTime);
    void ValidateNetworkState();

    // Cooperative action validation
    bool ValidateBelayRequest(ANetworkedClimbingCharacter* Climber) const;
    bool ValidateRopeShare(ANetworkedClimbingCharacter* Partner, UNetworkedRopeComponent* Rope) const;
    bool ValidateCooperativeAction(ANetworkedClimbingCharacter* Partner, const FString& ActionType) const;

    // Network optimization internals
    void UpdateNetworkImportance();
    void AdjustNetworkFrequency();
    bool ShouldSkipNetworkUpdate() const;

    // Trust system internals
    float CalculateTrustLevel(ANetworkedClimbingCharacter* Partner) const;
    void UpdateTrustRelationships();

    // Input handling
    void OnStartClimbing();
    void OnStopClimbing();
    void OnRequestBelay();
    void OnCallForHelp();
    void OnSendSignal();

    // Replication callbacks
    UFUNCTION()
    void OnRep_NetworkRole();

    UFUNCTION()
    void OnRep_BelayPartner();

    UFUNCTION()
    void OnRep_SharedRope();

    UFUNCTION()
    void OnRep_NetworkStats();

private:
    // Network optimization state
    float LastNetworkUpdate = 0.0f;
    float NetworkUpdateFrequency = 20.0f; // 20Hz default
    float NetworkImportanceScore = 0.5f;
    bool bNetworkStateDirty = false;

    // Prediction state
    TArray<FClimbingPredictionState> PredictionHistory;
    int32 MaxPredictionHistory = 10;
    float LastPredictionCorrection = 0.0f;

    // Cooperative climbing state
    TMap<ANetworkedClimbingCharacter*, float> PartnerTrustLevels;
    float LastTrustUpdate = 0.0f;
    float TrustUpdateInterval = 5.0f; // 5 seconds

    // Network statistics tracking
    float LastLatencyCheck = 0.0f;
    float LatencyCheckInterval = 1.0f; // 1 second
    TArray<float> RecentLatencies;
    int32 MaxLatencyHistory = 10;

    // Cached references for performance
    UPROPERTY()
    AClimbingPlayerState* CachedClimbingPlayerState = nullptr;

    // Helper functions
    void InitializeNetworkedComponents();
    void CachePlayerReferences();
    void CleanupNetworkState();
    AClimbingPlayerState* GetClimbingPlayerState() const;
    float CalculateNetworkDistance(ANetworkedClimbingCharacter* OtherPlayer) const;
    bool IsPlayerInRelevantRange(ANetworkedClimbingCharacter* OtherPlayer, float Range) const;
};