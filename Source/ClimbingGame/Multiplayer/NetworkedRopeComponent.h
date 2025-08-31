#pragma once

#include "CoreMinimal.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "Engine/World.h"
#include "NetworkedRopeComponent.generated.h"

class AClimbingPlayerState;
class UCooperativeSystem;

UENUM(BlueprintType)
enum class ERopeNetworkPriority : uint8
{
    Low,        // Background update
    Medium,     // Normal priority
    High,       // Active use
    Critical    // Emergency/breaking
};

USTRUCT(BlueprintType)
struct FRopeNetworkState
{
    GENERATED_BODY()

    UPROPERTY()
    FVector StartPosition = FVector::ZeroVector;

    UPROPERTY()
    FVector EndPosition = FVector::ZeroVector;

    UPROPERTY()
    float Tension = 0.0f;

    UPROPERTY()
    float Slack = 0.0f;

    UPROPERTY()
    ERopeState State = ERopeState::Coiled;

    UPROPERTY()
    float Timestamp = 0.0f;

    FRopeNetworkState()
    {
        StartPosition = FVector::ZeroVector;
        EndPosition = FVector::ZeroVector;
        Tension = 0.0f;
        Slack = 0.0f;
        State = ERopeState::Coiled;
        Timestamp = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FRopeInteraction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Player = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FString InteractionType = TEXT("");

    UPROPERTY(BlueprintReadOnly)
    FVector InteractionPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Timestamp = 0.0f;

    FRopeInteraction()
    {
        Player = nullptr;
        InteractionType = TEXT("");
        InteractionPoint = FVector::ZeroVector;
        Timestamp = 0.0f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRopeShared, UNetworkedRopeComponent*, Rope, AClimbingPlayerState*, SharedWith, AClimbingPlayerState*, SharedBy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRopeSlackChanged, UNetworkedRopeComponent*, Rope, float, NewSlack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRopeTensionAlert, UNetworkedRopeComponent*, Rope, float, TensionLevel, bool, bCritical);

/**
 * Networked version of AdvancedRopeComponent with multiplayer-specific features
 * Handles cooperative rope sharing, synchronized physics, and network optimization
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UNetworkedRopeComponent : public UAdvancedRopeComponent
{
    GENERATED_BODY()

public:
    UNetworkedRopeComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Networking configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
    ERopeNetworkPriority NetworkPriority = ERopeNetworkPriority::Medium;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
    float NetworkUpdateRate = 10.0f; // Updates per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
    float NetworkCullDistance = 10000.0f; // 100 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
    bool bUseAdaptiveNetworking = true;

    // Cooperative sharing
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sharing")
    TArray<AClimbingPlayerState*> SharedWithPlayers;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sharing")
    AClimbingPlayerState* PrimaryUser = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sharing")
    bool bIsSharedRope = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sharing")
    float SharedRopeSlack = 0.0f;

    // Network state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    FRopeNetworkState CurrentNetworkState;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    TArray<FRopeInteraction> RecentInteractions;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    float LastSynchronizationTime = 0.0f;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Sharing Events")
    FOnRopeShared OnRopeShared;

    UPROPERTY(BlueprintAssignable, Category = "Sharing Events")
    FOnRopeSlackChanged OnRopeSlackChanged;

    UPROPERTY(BlueprintAssignable, Category = "Sharing Events")
    FOnRopeTensionAlert OnRopeTensionAlert;

    // Cooperative rope sharing
    UFUNCTION(BlueprintCallable, Category = "Rope Sharing")
    bool ShareRope(AClimbingPlayerState* TargetPlayer);

    UFUNCTION(BlueprintCallable, Category = "Rope Sharing")
    bool AcceptRopeShare(AClimbingPlayerState* SharingPlayer);

    UFUNCTION(BlueprintCallable, Category = "Rope Sharing")
    bool StopSharingRope(AClimbingPlayerState* Player = nullptr);

    UFUNCTION(BlueprintPure, Category = "Rope Sharing")
    bool IsRopeSharedWith(AClimbingPlayerState* Player) const;

    UFUNCTION(BlueprintPure, Category = "Rope Sharing")
    TArray<AClimbingPlayerState*> GetSharingPlayers() const { return SharedWithPlayers; }

    // Advanced cooperative features
    UFUNCTION(BlueprintCallable, Category = "Cooperative Rope")
    bool EstablishBelayPartnership(AClimbingPlayerState* Belayer, AClimbingPlayerState* Climber);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Rope")
    bool HandleCooperativeLoad(const TArray<AClimbingPlayerState*>& LoadSharing);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Rope")
    bool SynchronizeRopePhysics(float SyncTolerance = 5.0f);

    UFUNCTION(BlueprintCallable, Category = "Cooperative Rope")
    bool PredictRopeFailure(float PredictionWindow = 2.0f);

    UFUNCTION(BlueprintPure, Category = "Cooperative Rope")
    float GetCooperativeLoadCapacity() const;

    // Slack management
    UFUNCTION(BlueprintCallable, Category = "Rope Control")
    bool AdjustSlack(float SlackDelta);

    UFUNCTION(BlueprintCallable, Category = "Rope Control")
    bool SetSlack(float NewSlack);

    UFUNCTION(BlueprintPure, Category = "Rope Control")
    float GetCurrentSlack() const { return SharedRopeSlack; }

    UFUNCTION(BlueprintCallable, Category = "Rope Control")
    bool CanAdjustSlack(AClimbingPlayerState* Player) const;

    // Tension monitoring
    UFUNCTION(BlueprintCallable, Category = "Rope Monitoring")
    void StartTensionMonitoring(float AlertThreshold = 0.8f, float CriticalThreshold = 0.95f);

    UFUNCTION(BlueprintCallable, Category = "Rope Monitoring")
    void StopTensionMonitoring();

    UFUNCTION(BlueprintPure, Category = "Rope Monitoring")
    bool IsTensionCritical() const;

    UFUNCTION(BlueprintPure, Category = "Rope Monitoring")
    float GetTensionAsPercentage() const;

    // Network synchronization
    UFUNCTION(BlueprintCallable, Category = "Network Sync")
    void RequestFullSynchronization();

    UFUNCTION(BlueprintCallable, Category = "Network Sync")
    void SetNetworkPriority(ERopeNetworkPriority Priority);

    UFUNCTION(BlueprintCallable, Category = "Network Sync")
    void ForceNetworkUpdate();

    UFUNCTION(BlueprintPure, Category = "Network Sync")
    bool IsNetworkDirty() const;

    // Interaction tracking
    UFUNCTION(BlueprintCallable, Category = "Interactions")
    void RecordPlayerInteraction(AClimbingPlayerState* Player, const FString& InteractionType, const FVector& InteractionPoint);

    UFUNCTION(BlueprintPure, Category = "Interactions")
    TArray<FRopeInteraction> GetRecentInteractions() const { return RecentInteractions; }

    UFUNCTION(BlueprintPure, Category = "Interactions")
    FRopeInteraction GetLastInteractionByPlayer(AClimbingPlayerState* Player) const;

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForViewers(const TArray<FVector>& ViewerLocations);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetLODLevel(int32 LODLevel);

    UFUNCTION(BlueprintPure, Category = "Performance")
    int32 GetCurrentLODLevel() const;

    // Network RPCs
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerShareRope(AClimbingPlayerState* TargetPlayer);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAcceptRopeShare(AClimbingPlayerState* SharingPlayer);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStopSharingRope(AClimbingPlayerState* Player);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAdjustSlack(float SlackDelta);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetSlack(float NewSlack);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRecordInteraction(AClimbingPlayerState* Player, const FString& InteractionType, const FVector& InteractionPoint);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestFullSync();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastRopeShared(AClimbingPlayerState* SharedWith, AClimbingPlayerState* SharedBy);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSlackChanged(float NewSlack, AClimbingPlayerState* AdjustedBy);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastTensionAlert(float TensionLevel, bool bCritical);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSyncRopeState(const FRopeNetworkState& NetworkState);

    UFUNCTION(Client, Reliable)
    void ClientReceiveRopeUpdate(const FRopeNetworkState& NetworkState);

protected:
    // Network state management
    void UpdateNetworkState();
    void ProcessNetworkUpdate(const FRopeNetworkState& NewState);
    bool ShouldSendNetworkUpdate() const;
    void InterpolateNetworkState(float DeltaTime);

    // Cooperative mechanics
    bool ValidateRopeShare(AClimbingPlayerState* TargetPlayer) const;
    void NotifyPlayersOfShare(AClimbingPlayerState* NewSharedPlayer);
    void CleanupRopeShare(AClimbingPlayerState* Player);
    void UpdateSharedRopePhysics();

    // Tension monitoring
    void UpdateTensionMonitoring(float DeltaTime);
    void CheckTensionThresholds();
    void BroadcastTensionAlert(float TensionLevel, bool bCritical);

    // Performance optimization
    void UpdateNetworkPriority();
    float CalculateNetworkImportance() const;
    void OptimizePhysicsForNetwork();
    void AdjustUpdateRateBasedOnImportance();

    // Interaction management
    void CleanupExpiredInteractions();
    bool IsValidInteractionType(const FString& InteractionType) const;

    // Network interpolation
    void InterpolatePositions(float Alpha);
    void InterpolateTension(float Alpha);
    void InterpolateSlack(float Alpha);

    // Replication callbacks
    UFUNCTION()
    void OnRep_SharedWithPlayers();

    UFUNCTION()
    void OnRep_CurrentNetworkState();

    UFUNCTION()
    void OnRep_SharedRopeSlack();

    UFUNCTION()
    void OnRep_PhysicsState();

private:
    // Network optimization state
    float LastNetworkUpdateTime = 0.0f;
    float NetworkUpdateInterval = 0.1f; // 10 updates per second by default
    float NetworkImportanceScore = 0.5f;
    int32 CurrentLODLevel = 0;
    bool bNetworkStateDirty = false;

    // Tension monitoring
    bool bTensionMonitoringActive = false;
    float TensionAlertThreshold = 0.8f;
    float TensionCriticalThreshold = 0.95f;
    float LastTensionCheck = 0.0f;
    float TensionCheckInterval = 0.5f; // Check twice per second

    // Interpolation state
    FRopeNetworkState PreviousNetworkState;
    FRopeNetworkState TargetNetworkState;
    float InterpolationAlpha = 0.0f;
    float InterpolationSpeed = 5.0f;

    // Interaction history cleanup
    float InteractionHistoryDuration = 60.0f; // Keep interactions for 1 minute
    int32 MaxInteractionHistory = 20;

    // Cached calculations
    mutable float CachedNetworkImportance = 0.0f;
    mutable float LastImportanceCalculationTime = 0.0f;
    static constexpr float ImportanceCalculationInterval = 1.0f; // 1 second cache

    // Helper functions
    void CalculateOptimalNetworkSettings();
    bool IsPlayerInRange(AClimbingPlayerState* Player, float Range) const;
    float GetDistanceToPlayer(AClimbingPlayerState* Player) const;
};