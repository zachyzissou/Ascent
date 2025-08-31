#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "ClimbingPlayerState.generated.h"

class UClimbingToolBase;
class AAdvancedRopeComponent;

USTRUCT(BlueprintType)
struct FClimbingStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float TotalClimbingTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 SuccessfulBelayAssists = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ToolsShared = 0;

    UPROPERTY(BlueprintReadOnly)
    float DistanceClimbed = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 RopesDeployed = 0;

    FClimbingStats()
    {
        TotalClimbingTime = 0.0f;
        SuccessfulBelayAssists = 0;
        ToolsShared = 0;
        DistanceClimbed = 0.0f;
        RopesDeployed = 0;
    }
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    UClimbingToolBase* Tool = nullptr;

    UPROPERTY(BlueprintReadOnly)
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bIsShared = false;

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* SharedFrom = nullptr;

    FInventorySlot()
    {
        Tool = nullptr;
        Quantity = 0;
        bIsShared = false;
        SharedFrom = nullptr;
    }
};

/**
 * Player state for ClimbingGame multiplayer sessions
 * Tracks player-specific data, cooperative relationships, and inventory
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AClimbingPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AClimbingPlayerState();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Player stats
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
    FClimbingStats SessionStats;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
    FClimbingStats CareerStats;

    // Cooperative relationships
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperative")
    AClimbingPlayerState* BelayPartner = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cooperative")
    TArray<AClimbingPlayerState*> TrustedPartners;

    // Inventory management
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> InventorySlots;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
    int32 MaxInventorySlots = 20;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
    float MaxCarryWeight = 25.0f; // 25kg

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
    float CurrentCarryWeight = 0.0f;

    // Player status
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
    bool bIsClimbing = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
    bool bIsProvingBelay = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
    bool bIsReceivingBelay = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
    float CurrentStamina = 100.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
    float MaxStamina = 100.0f;

    // Communication
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Communication")
    bool bVoiceChatEnabled = true;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Communication")
    bool bRadioCommunicationActive = false;

    // Belay system
    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void SetBelayPartner(AClimbingPlayerState* Partner);

    UFUNCTION(BlueprintPure, Category = "Cooperative")
    AClimbingPlayerState* GetBelayPartner() const { return BelayPartner; }

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    bool CanProvideBelay() const;

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    bool StartBelaying(AClimbingPlayerState* Climber);

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void StopBelaying();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartBelaying(AClimbingPlayerState* Climber);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStopBelaying();

    // Trust system
    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void AddTrustedPartner(AClimbingPlayerState* Partner);

    UFUNCTION(BlueprintCallable, Category = "Cooperative")
    void RemoveTrustedPartner(AClimbingPlayerState* Partner);

    UFUNCTION(BlueprintPure, Category = "Cooperative")
    bool IsTrustedPartner(AClimbingPlayerState* Partner) const;

    // Inventory system
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(UClimbingToolBase* Tool, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(UClimbingToolBase* Tool, int32 Quantity = 1);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool OwnsItem(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemQuantity(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool CanAcceptTool(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    float GetRemainingCarryCapacity() const;

    // Tool sharing
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TransferTool(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool ShareTool(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ReturnSharedTool(UClimbingToolBase* Tool);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ReturnAllSharedTools();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTransferTool(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerShareTool(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient);

    // Statistics tracking
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void IncrementBelayAssists();

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void IncrementToolsShared();

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void AddClimbingTime(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void AddDistanceClimbed(float Distance);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void IncrementRopesDeployed();

    // Status management
    UFUNCTION(BlueprintCallable, Category = "Status")
    void SetClimbingStatus(bool bClimbing);

    UFUNCTION(BlueprintCallable, Category = "Status")
    void SetStamina(float NewStamina);

    UFUNCTION(BlueprintPure, Category = "Status")
    float GetStaminaPercentage() const;

    // Communication
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SetVoiceChatEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Communication")
    void ToggleRadioCommunication();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetVoiceChatEnabled(bool bEnabled);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerToggleRadioCommunication();

protected:
    // Replication notifications
    UFUNCTION()
    void OnRep_BelayPartner();

    UFUNCTION()
    void OnRep_InventorySlots();

    UFUNCTION()
    void OnRep_SessionStats();

    UFUNCTION()
    void OnRep_PlayerStatus();

private:
    // Helper functions
    int32 FindEmptyInventorySlot() const;
    int32 FindInventorySlotWithTool(UClimbingToolBase* Tool) const;
    void UpdateCarryWeight();
    bool ValidateToolTransfer(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient) const;
};