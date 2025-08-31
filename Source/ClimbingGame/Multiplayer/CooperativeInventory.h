#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "../Tools/ClimbingToolBase.h"
#include "ClimbingPlayerState.h"
#include "CooperativeInventory.generated.h"

class ANetworkedClimbingCharacter;
class UCooperativeSystem;

UENUM(BlueprintType)
enum class EToolShareType : uint8
{
    Permanent,          // Tool ownership transfers permanently
    Temporary,          // Tool is borrowed and must be returned
    Emergency,          // Emergency lending with priority return
    Pooled             // Tool goes into shared team pool
};

UENUM(BlueprintType)
enum class EInventoryAccessLevel : uint8
{
    Owner,              // Full access to all items
    Trusted,            // Access to shared items and emergencies
    Partner,            // Access to partner-shared items only
    Limited,            // Emergency access only
    Restricted         // No access
};

USTRUCT(BlueprintType)
struct FSharedInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    UClimbingToolBase* Tool = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 Quantity = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    AClimbingPlayerState* OriginalOwner = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly)
    AClimbingPlayerState* CurrentUser = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly)
    EToolShareType ShareType = EToolShareType::Temporary;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float ShareTimestamp = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float ShareDuration = -1.0f; // -1 = indefinite

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsActive = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 SharePriority = 0; // Higher priority = more important

    FSharedInventorySlot()
    {
        Tool = nullptr;
        Quantity = 0;
        OriginalOwner = nullptr;
        CurrentUser = nullptr;
        ShareType = EToolShareType::Temporary;
        ShareTimestamp = 0.0f;
        ShareDuration = -1.0f;
        bIsActive = true;
        SharePriority = 0;
    }
};

USTRUCT(BlueprintType)
struct FInventoryShareRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Requester = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Owner = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UClimbingToolBase* RequestedTool = nullptr;

    UPROPERTY(BlueprintReadOnly)
    int32 RequestedQuantity = 1;

    UPROPERTY(BlueprintReadOnly)
    EToolShareType RequestedShareType = EToolShareType::Temporary;

    UPROPERTY(BlueprintReadOnly)
    float RequestedDuration = 300.0f; // 5 minutes default

    UPROPERTY(BlueprintReadOnly)
    FString RequestReason;

    UPROPERTY(BlueprintReadOnly)
    int32 RequestPriority = 0;

    UPROPERTY(BlueprintReadOnly)
    float RequestTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ExpiryTime = 30.0f; // 30 seconds to respond

    UPROPERTY(BlueprintReadOnly)
    bool bIsUrgent = false;

    FInventoryShareRequest()
    {
        Requester = nullptr;
        Owner = nullptr;
        RequestedTool = nullptr;
        RequestedQuantity = 1;
        RequestedShareType = EToolShareType::Temporary;
        RequestedDuration = 300.0f;
        RequestReason = TEXT("");
        RequestPriority = 0;
        RequestTime = 0.0f;
        ExpiryTime = 30.0f;
        bIsUrgent = false;
    }
};

USTRUCT(BlueprintType)
struct FTeamInventoryPool
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<FSharedInventorySlot> PooledItems;

    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<AClimbingPlayerState*> TeamMembers;

    UPROPERTY(Replicated, BlueprintReadOnly)
    AClimbingPlayerState* PoolManager = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float PoolCreationTime = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsActive = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    FString PoolName = TEXT("Team Equipment Pool");

    FTeamInventoryPool()
    {
        PoolManager = nullptr;
        PoolCreationTime = 0.0f;
        bIsActive = true;
        PoolName = TEXT("Team Equipment Pool");
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnToolShared, AClimbingPlayerState*, Owner, AClimbingPlayerState*, Receiver, UClimbingToolBase*, Tool, EToolShareType, ShareType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnToolReturned, AClimbingPlayerState*, Returner, AClimbingPlayerState*, Owner, UClimbingToolBase*, Tool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShareRequestReceived, const FInventoryShareRequest&, Request, AClimbingPlayerState*, Requester);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnShareRequestResponded, const FInventoryShareRequest&, Request, bool, bAccepted, AClimbingPlayerState*, Responder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryPoolCreated, const FTeamInventoryPool&, Pool, AClimbingPlayerState*, Creator);

/**
 * Component that handles cooperative inventory sharing and management
 * Enables tool sharing, team equipment pools, and emergency equipment access
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UCooperativeInventory : public UActorComponent
{
    GENERATED_BODY()

public:
    UCooperativeInventory();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Config")
    float MaxShareRange = 1000.0f; // 10 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Config")
    float DefaultShareDuration = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Config")
    int32 MaxConcurrentShares = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Config")
    bool bAllowEmergencyAccess = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Config")
    bool bAutoReturnOnDistance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Config")
    float AutoReturnDistance = 2000.0f; // 20 meters

    // Current shared items
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shared Items")
    TArray<FSharedInventorySlot> SharedItems;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shared Items")
    TArray<FSharedInventorySlot> BorrowedItems;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shared Items")
    TArray<FInventoryShareRequest> PendingRequests;

    // Team inventory pool
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team Pool")
    FTeamInventoryPool TeamPool;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team Pool")
    bool bIsPoolMember = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team Pool")
    EInventoryAccessLevel CurrentAccessLevel = EInventoryAccessLevel::Owner;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnToolShared OnToolShared;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnToolReturned OnToolReturned;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnShareRequestReceived OnShareRequestReceived;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnShareRequestResponded OnShareRequestResponded;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnInventoryPoolCreated OnInventoryPoolCreated;

    // Tool sharing
    UFUNCTION(BlueprintCallable, Category = "Tool Sharing")
    bool RequestTool(AClimbingPlayerState* Owner, UClimbingToolBase* Tool, int32 Quantity = 1, 
                    EToolShareType ShareType = EToolShareType::Temporary, float Duration = -1.0f, 
                    const FString& Reason = TEXT(""), bool bIsUrgent = false);

    UFUNCTION(BlueprintCallable, Category = "Tool Sharing")
    bool ShareTool(AClimbingPlayerState* Recipient, UClimbingToolBase* Tool, int32 Quantity = 1, 
                   EToolShareType ShareType = EToolShareType::Temporary, float Duration = -1.0f);

    UFUNCTION(BlueprintCallable, Category = "Tool Sharing")
    bool ReturnTool(UClimbingToolBase* Tool, AClimbingPlayerState* OriginalOwner = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Tool Sharing")
    bool RespondToShareRequest(int32 RequestIndex, bool bAccept, float CustomDuration = -1.0f);

    // Emergency access
    UFUNCTION(BlueprintCallable, Category = "Emergency Access")
    bool RequestEmergencyTool(UClimbingToolBase* Tool, const FString& EmergencyType);

    UFUNCTION(BlueprintCallable, Category = "Emergency Access")
    bool GrantEmergencyAccess(AClimbingPlayerState* Player, UClimbingToolBase* Tool, float Duration = 600.0f);

    UFUNCTION(BlueprintCallable, Category = "Emergency Access")
    void RevokeEmergencyAccess(AClimbingPlayerState* Player);

    // Team inventory pool
    UFUNCTION(BlueprintCallable, Category = "Team Pool")
    bool CreateTeamPool(const TArray<AClimbingPlayerState*>& TeamMembers, const FString& PoolName = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Team Pool")
    bool JoinTeamPool(const FTeamInventoryPool& Pool);

    UFUNCTION(BlueprintCallable, Category = "Team Pool")
    bool LeaveTeamPool();

    UFUNCTION(BlueprintCallable, Category = "Team Pool")
    bool AddToTeamPool(UClimbingToolBase* Tool, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Team Pool")
    bool TakeFromTeamPool(UClimbingToolBase* Tool, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Team Pool")
    bool SetPoolManager(AClimbingPlayerState* NewManager);

    // Access level management
    UFUNCTION(BlueprintCallable, Category = "Access Control")
    void SetPlayerAccessLevel(AClimbingPlayerState* Player, EInventoryAccessLevel AccessLevel);

    UFUNCTION(BlueprintPure, Category = "Access Control")
    EInventoryAccessLevel GetPlayerAccessLevel(AClimbingPlayerState* Player) const;

    UFUNCTION(BlueprintPure, Category = "Access Control")
    bool CanPlayerAccessTool(AClimbingPlayerState* Player, UClimbingToolBase* Tool) const;

    // Inventory queries
    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    TArray<FSharedInventorySlot> GetSharedTools() const { return SharedItems; }

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    TArray<FSharedInventorySlot> GetBorrowedTools() const { return BorrowedItems; }

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    TArray<FInventoryShareRequest> GetPendingShareRequests() const { return PendingRequests; }

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    bool IsToolShared(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    bool IsToolBorrowed(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    AClimbingPlayerState* GetToolOwner(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    AClimbingPlayerState* GetToolCurrentUser(UClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    TArray<UClimbingToolBase*> GetAvailableToolsInRange() const;

    UFUNCTION(BlueprintPure, Category = "Inventory Queries")
    TArray<UClimbingToolBase*> GetToolsInTeamPool() const;

    // Smart inventory management
    UFUNCTION(BlueprintCallable, Category = "Smart Management")
    bool AutoShareRedundantTools();

    UFUNCTION(BlueprintCallable, Category = "Smart Management")
    bool SuggestOptimalToolDistribution(TArray<AClimbingPlayerState*>& OutSuggestions);

    UFUNCTION(BlueprintCallable, Category = "Smart Management")
    bool RebalanceTeamInventory();

    UFUNCTION(BlueprintCallable, Category = "Smart Management")
    TArray<UClimbingToolBase*> GetRecommendedToolsForClimb(const TArray<FVector>& RoutePoints);

    // Network RPCs
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestTool(AClimbingPlayerState* Owner, UClimbingToolBase* Tool, int32 Quantity, 
                          EToolShareType ShareType, float Duration, const FString& Reason, bool bIsUrgent);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerShareTool(AClimbingPlayerState* Recipient, UClimbingToolBase* Tool, int32 Quantity, 
                        EToolShareType ShareType, float Duration);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerReturnTool(UClimbingToolBase* Tool, AClimbingPlayerState* OriginalOwner);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRespondToShareRequest(int32 RequestIndex, bool bAccept, float CustomDuration);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestEmergencyTool(UClimbingToolBase* Tool, const FString& EmergencyType);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerCreateTeamPool(const TArray<AClimbingPlayerState*>& TeamMembers, const FString& PoolName);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerJoinTeamPool();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAddToTeamPool(UClimbingToolBase* Tool, int32 Quantity);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTakeFromTeamPool(UClimbingToolBase* Tool, int32 Quantity);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastToolShared(AClimbingPlayerState* Owner, AClimbingPlayerState* Receiver, 
                            UClimbingToolBase* Tool, EToolShareType ShareType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastToolReturned(AClimbingPlayerState* Returner, AClimbingPlayerState* Owner, UClimbingToolBase* Tool);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastShareRequestReceived(const FInventoryShareRequest& Request);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastTeamPoolCreated(const FTeamInventoryPool& Pool);

    UFUNCTION(Client, Reliable)
    void ClientReceiveShareRequest(const FInventoryShareRequest& Request);

    UFUNCTION(Client, Reliable)
    void ClientReceiveShareResponse(const FInventoryShareRequest& Request, bool bAccepted);

protected:
    // Internal management
    void UpdateSharedItems(float DeltaTime);
    void ProcessPendingRequests();
    void CheckAutoReturn();
    void ValidateSharedItems();

    // Request management
    int32 CreateShareRequest(AClimbingPlayerState* Requester, AClimbingPlayerState* Owner, 
                            UClimbingToolBase* Tool, int32 Quantity, EToolShareType ShareType, 
                            float Duration, const FString& Reason, bool bIsUrgent);
    bool ValidateShareRequest(const FInventoryShareRequest& Request) const;
    void CleanupExpiredRequests();

    // Tool management
    bool ValidateToolShare(AClimbingPlayerState* Owner, AClimbingPlayerState* Recipient, 
                          UClimbingToolBase* Tool, int32 Quantity) const;
    bool ValidateToolReturn(UClimbingToolBase* Tool, AClimbingPlayerState* Returner) const;
    void AddSharedItem(const FSharedInventorySlot& Slot);
    void RemoveSharedItem(int32 Index);

    // Team pool management
    bool ValidateTeamPoolAccess(AClimbingPlayerState* Player, UClimbingToolBase* Tool) const;
    void UpdateTeamPoolAccess();
    bool CanManageTeamPool(AClimbingPlayerState* Player) const;

    // Access control
    bool HasSufficientAccessLevel(AClimbingPlayerState* Player, EInventoryAccessLevel RequiredLevel) const;
    void UpdatePlayerAccessLevels();

    // Smart inventory algorithms
    void AnalyzeToolDistribution(TMap<UClimbingToolBase*, int32>& OutToolCounts, 
                               TMap<AClimbingPlayerState*, float>& OutPlayerLoads) const;
    float CalculateOptimalToolDistribution(const TArray<AClimbingPlayerState*>& Players) const;
    TArray<UClimbingToolBase*> PredictNeededTools(const TArray<FVector>& RoutePoints) const;

    // Replication callbacks
    UFUNCTION()
    void OnRep_SharedItems();

    UFUNCTION()
    void OnRep_BorrowedItems();

    UFUNCTION()
    void OnRep_PendingRequests();

    UFUNCTION()
    void OnRep_TeamPool();

private:
    // Internal state
    UPROPERTY()
    AClimbingPlayerState* OwningPlayerState = nullptr;

    UPROPERTY()
    TMap<AClimbingPlayerState*, EInventoryAccessLevel> PlayerAccessLevels;

    // Request tracking
    int32 NextRequestId = 1;
    TMap<int32, FInventoryShareRequest> ActiveRequests;

    // Performance optimization
    float LastInventoryUpdate = 0.0f;
    float InventoryUpdateInterval = 1.0f; // 1 second
    float LastRequestCleanup = 0.0f;
    float RequestCleanupInterval = 30.0f; // 30 seconds

    // Distance tracking for auto-return
    TMap<UClimbingToolBase*, float> SharedToolDistances;
    float LastDistanceCheck = 0.0f;
    float DistanceCheckInterval = 5.0f; // 5 seconds

    // Helper functions
    AClimbingPlayerState* GetOwningClimbingPlayerState() const;
    float CalculateDistanceToPlayer(AClimbingPlayerState* Player) const;
    bool IsPlayerInRange(AClimbingPlayerState* Player, float Range) const;
    void LogInventoryAction(const FString& Action, const FString& Details = TEXT("")) const;
};