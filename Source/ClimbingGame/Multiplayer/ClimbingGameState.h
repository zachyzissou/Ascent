#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "ClimbingGameState.generated.h"

class AClimbingPlayerState;

UENUM(BlueprintType)
enum class EClimbingSessionState : uint8
{
    WaitingForPlayers,
    PreparationPhase,
    ClimbingActive,
    SessionEnding,
    SessionCompleted
};

USTRUCT(BlueprintType)
struct FCooperativeAction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Helper = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Assisted = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FString ActionType;

    UPROPERTY(BlueprintReadOnly)
    float Timestamp = 0.0f;

    FCooperativeAction()
    {
        Helper = nullptr;
        Assisted = nullptr;
        ActionType = TEXT("");
        Timestamp = 0.0f;
    }
};

/**
 * Game state for ClimbingGame multiplayer sessions
 * Manages session state, player synchronization, and cooperative mechanics tracking
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AClimbingGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AClimbingGameState();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Session state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Session")
    EClimbingSessionState SessionState = EClimbingSessionState::WaitingForPlayers;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Session")
    float SessionTimeRemaining = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Session")
    int32 ConnectedPlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Session")
    int32 MaxPlayers = 4;

    // Cooperative mechanics tracking
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperative")
    TArray<FCooperativeAction> RecentCooperativeActions;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperative")
    int32 ActiveBelayPartnerships = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperative")
    int32 ToolSharesThisSession = 0;

    // Environmental state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Environment")
    TArray<AActor*> SharedRopeAnchors;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Environment")
    TArray<class AAdvancedRopeComponent*> ActiveRopes;

    // Session management
    UFUNCTION(BlueprintCallable, Category = "Session")
    void SetSessionState(EClimbingSessionState NewState);

    UFUNCTION(BlueprintCallable, Category = "Session")
    bool CanStartClimbing() const;

    UFUNCTION(BlueprintCallable, Category = "Session")
    float GetSessionProgress() const;

    UFUNCTION(BlueprintPure, Category = "Session")
    FString GetSessionStateString() const;

    // Player tracking
    UFUNCTION(BlueprintCallable, Category = "Players")
    TArray<AClimbingPlayerState*> GetAllClimbingPlayers() const;

    UFUNCTION(BlueprintCallable, Category = "Players")
    AClimbingPlayerState* GetPlayerByName(const FString& PlayerName) const;

    UFUNCTION(BlueprintCallable, Category = "Players")
    bool ArePlayersInRange(AClimbingPlayerState* Player1, AClimbingPlayerState* Player2, float Range) const;

    // Cooperative mechanics
    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void RecordCooperativeAction(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted, const FString& ActionType);

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void IncrementBelayPartnerships();

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void DecrementBelayPartnerships();

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void IncrementToolShares();

    UFUNCTION(BlueprintPure, Category = "Cooperative")
    TArray<FCooperativeAction> GetCooperativeActionsForPlayer(AClimbingPlayerState* Player) const;

    // Rope and anchor management
    UFUNCTION(BlueprintCallable, Category = "Environment")
    void RegisterSharedRopeAnchor(AActor* Anchor);

    UFUNCTION(BlueprintCallable, Category = "Environment")
    void UnregisterSharedRopeAnchor(AActor* Anchor);

    UFUNCTION(BlueprintCallable, Category = "Environment")
    void RegisterActiveRope(class AAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Environment")
    void UnregisterActiveRope(class AAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintPure, Category = "Environment")
    TArray<AActor*> GetNearbyRopeAnchors(const FVector& Location, float Range) const;

    // Network synchronization
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSessionStateChanged(EClimbingSessionState NewState);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastCooperativeActionRecorded(const FCooperativeAction& Action);

protected:
    // Internal state management
    float SessionStartTime = 0.0f;
    float SessionDuration = 3600.0f; // 1 hour default

    // Replication notifications
    UFUNCTION()
    void OnRep_SessionState();

    UFUNCTION()
    void OnRep_SessionTimeRemaining();

    UFUNCTION()
    void OnRep_ConnectedPlayerCount();

    UFUNCTION()
    void OnRep_RecentCooperativeActions();

private:
    // Internal helpers
    void UpdateSessionTimer();
    void CleanupExpiredActions();
    void ValidateActiveRopes();

    // Configuration
    static constexpr float COOPERATIVE_ACTION_HISTORY_DURATION = 300.0f; // 5 minutes
    static constexpr int32 MAX_COOPERATIVE_ACTION_HISTORY = 50;
};