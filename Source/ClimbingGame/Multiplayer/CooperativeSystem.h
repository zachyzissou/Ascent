#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "CooperativeSystem.generated.h"

class AClimbingPlayerState;
class UClimbingToolBase;
class AAdvancedRopeComponent;

UENUM(BlueprintType)
enum class ECooperativeActionType : uint8
{
    None,
    Belay,
    SpotClimb,
    ToolShare,
    RopeAssist,
    EmergencyRescue,
    RoutePlanning
};

USTRUCT(BlueprintType)
struct FCooperativeRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Requester = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Target = nullptr;

    UPROPERTY(BlueprintReadOnly)
    ECooperativeActionType ActionType = ECooperativeActionType::None;

    UPROPERTY(BlueprintReadOnly)
    FVector ActionLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float RequestTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ExpiryTime = 30.0f; // 30 seconds to respond

    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = true;

    FCooperativeRequest()
    {
        Requester = nullptr;
        Target = nullptr;
        ActionType = ECooperativeActionType::None;
        ActionLocation = FVector::ZeroVector;
        RequestTime = 0.0f;
        ExpiryTime = 30.0f;
        bIsActive = true;
    }
};

USTRUCT(BlueprintType)
struct FBelayAssistance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Belayer = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Climber = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AAdvancedRopeComponent* SharedRope = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float SlackAmount = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsLocked = false;

    UPROPERTY(BlueprintReadOnly)
    float StartTime = 0.0f;

    FBelayAssistance()
    {
        Belayer = nullptr;
        Climber = nullptr;
        SharedRope = nullptr;
        SlackAmount = 0.0f;
        bIsLocked = false;
        StartTime = 0.0f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCooperativeRequestReceived, AClimbingPlayerState*, Requester, ECooperativeActionType, ActionType, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCooperativeActionStarted, AClimbingPlayerState*, Helper, AClimbingPlayerState*, Assisted, ECooperativeActionType, ActionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCooperativeActionCompleted, AClimbingPlayerState*, Helper, AClimbingPlayerState*, Assisted, ECooperativeActionType, ActionType);

/**
 * Component that handles cooperative mechanics between players
 * Manages belay assistance, spotting, tool sharing, and emergency situations
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UCooperativeSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UCooperativeSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation")
    float MaxCooperationRange = 1000.0f; // 10 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation")
    float BelayAssistanceRange = 1500.0f; // 15 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation")
    float SpottingRange = 500.0f; // 5 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation")
    float RequestTimeout = 30.0f; // 30 seconds

    // Current cooperative state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    TArray<FCooperativeRequest> ActiveRequests;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    TArray<FBelayAssistance> ActiveBelayAssistances;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    AClimbingPlayerState* CurrentSpotter = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
    bool bIsReceivingSpot = false;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCooperativeRequestReceived OnCooperativeRequestReceived;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCooperativeActionStarted OnCooperativeActionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCooperativeActionCompleted OnCooperativeActionCompleted;

    // Request system
    UFUNCTION(BlueprintCallable, Category = "Cooperation")
    bool RequestCooperativeAction(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "Cooperation")
    bool RespondToCooperativeRequest(int32 RequestIndex, bool bAccept);

    UFUNCTION(BlueprintCallable, Category = "Cooperation")
    void CancelCooperativeRequest(int32 RequestIndex);

    UFUNCTION(BlueprintPure, Category = "Cooperation")
    TArray<FCooperativeRequest> GetPendingRequests() const;

    UFUNCTION(BlueprintPure, Category = "Cooperation")
    TArray<FCooperativeRequest> GetRequestsForPlayer(AClimbingPlayerState* Player) const;

    // Belay assistance
    UFUNCTION(BlueprintCallable, Category = "Belay")
    bool StartBelayAssistance(AClimbingPlayerState* Climber, AAdvancedRopeComponent* Rope = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Belay")
    bool StopBelayAssistance(AClimbingPlayerState* Climber);

    UFUNCTION(BlueprintCallable, Category = "Belay")
    bool AdjustRopeSlack(AClimbingPlayerState* Climber, float SlackDelta);

    UFUNCTION(BlueprintCallable, Category = "Belay")
    bool LockBelay(AClimbingPlayerState* Climber, bool bLock);

    UFUNCTION(BlueprintPure, Category = "Belay")
    FBelayAssistance GetBelayAssistanceForClimber(AClimbingPlayerState* Climber) const;

    UFUNCTION(BlueprintPure, Category = "Belay")
    bool IsProvidingBelay() const;

    UFUNCTION(BlueprintPure, Category = "Belay")
    bool IsReceivingBelay() const;

    // Spotting system
    UFUNCTION(BlueprintCallable, Category = "Spotting")
    bool StartSpotting(AClimbingPlayerState* Climber);

    UFUNCTION(BlueprintCallable, Category = "Spotting")
    bool StopSpotting();

    UFUNCTION(BlueprintCallable, Category = "Spotting")
    bool ProvideCatchAssistance(AClimbingPlayerState* FallingClimber, const FVector& CatchLocation);

    UFUNCTION(BlueprintPure, Category = "Spotting")
    bool CanProvideSpot(AClimbingPlayerState* Climber) const;

    UFUNCTION(BlueprintPure, Category = "Spotting")
    AClimbingPlayerState* GetCurrentSpotter() const { return CurrentSpotter; }

    // Tool sharing assistance
    UFUNCTION(BlueprintCallable, Category = "Tools")
    bool OfferToolAssistance(AClimbingPlayerState* Target, UClimbingToolBase* Tool);

    UFUNCTION(BlueprintCallable, Category = "Tools")
    bool RequestToolBorrow(AClimbingPlayerState* Owner, UClimbingToolBase* Tool);

    UFUNCTION(BlueprintCallable, Category = "Tools")
    bool PerformCooperativeToolPlacement(AClimbingPlayerState* Partner, UClimbingToolBase* Tool, const FVector& Location);

    // Emergency assistance
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    bool CallForEmergencyAssistance(ECooperativeActionType EmergencyType, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Emergency")
    bool RespondToEmergency(AClimbingPlayerState* PlayerInNeed);

    UFUNCTION(BlueprintCallable, Category = "Emergency")
    bool PerformEmergencyRescue(AClimbingPlayerState* PlayerInNeed, const FVector& SafeLocation);

    // Route planning assistance
    UFUNCTION(BlueprintCallable, Category = "Planning")
    bool ShareRouteInformation(AClimbingPlayerState* Target, const TArray<FVector>& RoutePoints);

    UFUNCTION(BlueprintCallable, Category = "Planning")
    bool RequestRouteAdvice(AClimbingPlayerState* ExperiencedClimber, const FVector& TargetLocation);

    // Network RPCs
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestCooperativeAction(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRespondToRequest(int32 RequestIndex, bool bAccept);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartBelayAssistance(AClimbingPlayerState* Climber, AAdvancedRopeComponent* Rope);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAdjustRopeSlack(AClimbingPlayerState* Climber, float SlackDelta);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartSpotting(AClimbingPlayerState* Climber);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerCallEmergency(ECooperativeActionType EmergencyType, const FVector& Location);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastCooperativeActionStarted(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted, ECooperativeActionType ActionType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastCooperativeActionCompleted(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted, ECooperativeActionType ActionType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastEmergencyCall(AClimbingPlayerState* PlayerInNeed, ECooperativeActionType EmergencyType, const FVector& Location);

protected:
    // Internal helpers
    AClimbingPlayerState* GetOwningClimbingPlayerState() const;
    bool ValidateCooperativeAction(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location) const;
    bool IsInRange(const FVector& Location1, const FVector& Location2, float Range) const;
    void CleanupExpiredRequests();
    void UpdateBelayAssistances(float DeltaTime);
    int32 FindRequestIndex(AClimbingPlayerState* Requester, AClimbingPlayerState* Target, ECooperativeActionType ActionType) const;
    int32 FindBelayAssistanceIndex(AClimbingPlayerState* Climber) const;

    // Replication callbacks
    UFUNCTION()
    void OnRep_ActiveRequests();

    UFUNCTION()
    void OnRep_ActiveBelayAssistances();

    UFUNCTION()
    void OnRep_CurrentSpotter();

private:
    // Cached references
    UPROPERTY()
    AClimbingPlayerState* OwningPlayerState = nullptr;
};