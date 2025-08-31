#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/NetConnection.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "ClimbingNetworkOptimizer.generated.h"

UENUM(BlueprintType)
enum class ENetworkPriority : uint8
{
    Critical    UMETA(DisplayName = "Critical"),
    High        UMETA(DisplayName = "High"),
    Medium      UMETA(DisplayName = "Medium"),
    Low         UMETA(DisplayName = "Low"),
    Deferred    UMETA(DisplayName = "Deferred")
};

UENUM(BlueprintType)
enum class ENetworkCompressionLevel : uint8
{
    None        UMETA(DisplayName = "No Compression"),
    Light       UMETA(DisplayName = "Light Compression"),
    Medium      UMETA(DisplayName = "Medium Compression"),
    Heavy       UMETA(DisplayName = "Heavy Compression"),
    Maximum     UMETA(DisplayName = "Maximum Compression")
};

USTRUCT(BlueprintType)
struct FNetworkMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float BandwidthUsageKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float LatencyMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PacketLossPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 PacketsPerSecond = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 BytesPerSecond = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveConnections = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ReplicatedActors = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ReplicatedComponents = 0;

    UPROPERTY(BlueprintReadOnly)
    float CompressionRatio = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 DroppedPackets = 0;

    UPROPERTY(BlueprintReadOnly)
    float JitterMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FQuantizedPlayerState
{
    GENERATED_BODY()

    UPROPERTY()
    uint16 LocationX;

    UPROPERTY()
    uint16 LocationY;

    UPROPERTY()
    uint16 LocationZ;

    UPROPERTY()
    uint8 RotationYaw;

    UPROPERTY()
    uint8 RotationPitch;

    UPROPERTY()
    uint8 VelocityMagnitude;

    UPROPERTY()
    uint8 GripState;

    UPROPERTY()
    uint8 StaminaLevel;

    static FQuantizedPlayerState FromPlayerState(const FVector& Location, const FRotator& Rotation, 
                                                const FVector& Velocity, uint8 Grip, uint8 Stamina);
    void ToPlayerState(FVector& OutLocation, FRotator& OutRotation, FVector& OutVelocity, 
                      uint8& OutGrip, uint8& OutStamina) const;
};

USTRUCT(BlueprintType)
struct FQuantizedRopeState
{
    GENERATED_BODY()

    UPROPERTY()
    uint8 RopeID;

    UPROPERTY()
    uint8 NumSegments;

    UPROPERTY()
    TArray<uint16> SegmentPositions; // Delta compressed positions

    UPROPERTY()
    uint8 TensionLevel;

    UPROPERTY()
    uint8 OwnerPlayerID;

    static FQuantizedRopeState FromRopeComponent(const UAdvancedRopeComponent* Rope, uint8 RopeID, uint8 OwnerID);
    void ApplyToRopeComponent(UAdvancedRopeComponent* Rope) const;
};

USTRUCT(BlueprintType)
struct FNetworkUpdatePolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalUpdateRate = 60.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HighUpdateRate = 30.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MediumUpdateRate = 15.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LowUpdateRate = 10.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DeferredUpdateRate = 5.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalDistance = 1000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HighDistance = 3000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MediumDistance = 8000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LowDistance = 15000.0f; // cm
};

USTRUCT(BlueprintType)
struct FBandwidthBudget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TotalBudgetKBps = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PlayerMovementBudget = 0.4f; // 40% of budget

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RopePhysicsBudget = 0.3f; // 30% of budget

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ToolInteractionBudget = 0.2f; // 20% of budget

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ChatAndUIBudget = 0.1f; // 10% of budget

    float GetBudgetForCategory(const FString& Category) const;
    void AdjustBudgets(float TotalUsage);
};

USTRUCT(BlueprintType)
struct FConnectionProfile
{
    GENERATED_BODY()

    UPROPERTY()
    APlayerController* PlayerController;

    UPROPERTY()
    float CurrentBandwidthKBps;

    UPROPERTY()
    float AverageLatencyMs;

    UPROPERTY()
    float PacketLossRate;

    UPROPERTY()
    ENetworkCompressionLevel CompressionLevel;

    UPROPERTY()
    TArray<float> LatencyHistory;

    UPROPERTY()
    TArray<float> BandwidthHistory;

    UPROPERTY()
    FDateTime LastUpdateTime;

    UPROPERTY()
    int32 DroppedUpdates;

    UPROPERTY()
    bool bIsUnreliableConnection;
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingNetworkOptimizer : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // Network optimization
    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeNetworkTraffic(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void UpdateNetworkPriorities();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void AdaptToNetworkConditions();

    // Bandwidth management
    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    float GetCurrentBandwidthUsage() const;

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    bool IsWithinBandwidthBudget() const;

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void EnforceBandwidthLimits();

    // Compression and quantization
    UFUNCTION(BlueprintCallable, Category = "Compression")
    FQuantizedPlayerState QuantizePlayerState(ACharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "Compression")
    FQuantizedRopeState QuantizeRopeState(UAdvancedRopeComponent* Rope, uint8 RopeID, uint8 OwnerID);

    UFUNCTION(BlueprintCallable, Category = "Compression")
    void SetCompressionLevel(APlayerController* PC, ENetworkCompressionLevel Level);

    // Priority management
    UFUNCTION(BlueprintCallable, Category = "Priority Management")
    ENetworkPriority CalculateActorPriority(AActor* Actor, APlayerController* Viewer) const;

    UFUNCTION(BlueprintCallable, Category = "Priority Management")
    float CalculateUpdateRate(AActor* Actor, APlayerController* Viewer) const;

    UFUNCTION(BlueprintCallable, Category = "Priority Management")
    void UpdateReplicationPriorities();

    // Connection management
    UFUNCTION(BlueprintCallable, Category = "Connection Management")
    void RegisterPlayerConnection(APlayerController* PC);

    UFUNCTION(BlueprintCallable, Category = "Connection Management")
    void UnregisterPlayerConnection(APlayerController* PC);

    UFUNCTION(BlueprintCallable, Category = "Connection Management")
    void UpdateConnectionProfiles();

    // Latency compensation
    UFUNCTION(BlueprintCallable, Category = "Latency Compensation")
    FVector PredictPlayerPosition(ACharacter* Player, float PredictionTime) const;

    UFUNCTION(BlueprintCallable, Category = "Latency Compensation")
    void CompensateForLatency(APlayerController* PC, float ClientTimeStamp);

    UFUNCTION(BlueprintCallable, Category = "Latency Compensation")
    bool ValidatePlayerAction(ACharacter* Player, const FVector& ActionLocation, float Timestamp) const;

    // Metrics and monitoring
    UFUNCTION(BlueprintCallable, Category = "Metrics")
    FNetworkMetrics GetCurrentNetworkMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Metrics")
    void LogNetworkPerformance();

    UFUNCTION(BlueprintCallable, Category = "Metrics")
    void SaveNetworkTelemetry(const FString& Filename);

    // Rope-specific network optimization
    UFUNCTION(BlueprintCallable, Category = "Rope Optimization")
    void OptimizeRopeReplication(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Optimization")
    void BatchUpdateRopeStates(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rope Optimization")
    bool ShouldReplicateRopeToClient(UAdvancedRopeComponent* Rope, APlayerController* Client) const;

    // Anti-cheat and validation
    UFUNCTION(BlueprintCallable, Category = "Anti-Cheat")
    bool ValidateMovement(ACharacter* Player, const FVector& OldPos, const FVector& NewPos, float DeltaTime) const;

    UFUNCTION(BlueprintCallable, Category = "Anti-Cheat")
    bool ValidateRopeInteraction(UAdvancedRopeComponent* Rope, ACharacter* Player, const FVector& InteractionPoint) const;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    FNetworkUpdatePolicy UpdatePolicy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    FBandwidthBudget BandwidthBudget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    bool bEnableAdaptiveCompression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    bool bEnablePrioritySystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    bool bEnableLatencyCompensation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float NetworkUpdateInterval = 0.033f; // 30 Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float MaxAcceptableLatency = 200.0f; // ms

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float MaxAcceptablePacketLoss = 0.05f; // 5%

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnBandwidthExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnHighLatencyDetected;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnPacketLossDetected;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnConnectionOptimized;

protected:
    // Internal state
    UPROPERTY()
    TMap<APlayerController*, FConnectionProfile> ConnectionProfiles;

    UPROPERTY()
    TArray<UAdvancedRopeComponent*> TrackedRopes;

    FNetworkMetrics CurrentMetrics;
    float LastNetworkUpdate = 0.0f;
    float TotalBytesThisSecond = 0.0f;
    int32 PacketsThisSecond = 0;
    float NetworkUpdateAccumulator = 0.0f;

    // Performance tracking
    TArray<float> BandwidthHistory;
    TArray<float> LatencyHistory;
    int32 HistoryIndex = 0;
    static const int32 MaxHistorySize = 300; // 5 minutes at 1Hz

    // Compression state
    TMap<APlayerController*, TArray<uint8>> CompressionBuffers;
    TMap<ENetworkCompressionLevel, float> CompressionRatios;

private:
    // Internal optimization functions
    void OptimizePlayerReplication(APlayerController* PC, float DeltaTime);
    void OptimizeRopeReplication(float DeltaTime);
    void OptimizeToolReplication(float DeltaTime);
    
    // Bandwidth calculations
    float CalculateActorBandwidthCost(AActor* Actor, APlayerController* Viewer) const;
    void TrackBandwidthUsage(float BytesPerSecond);
    
    // Compression helpers
    TArray<uint8> CompressData(const TArray<uint8>& Data, ENetworkCompressionLevel Level) const;
    TArray<uint8> DecompressData(const TArray<uint8>& CompressedData, ENetworkCompressionLevel Level) const;
    
    // Priority calculations
    float CalculateDistanceBasedPriority(AActor* Actor, const FVector& ViewerLocation) const;
    float CalculateVisibilityBasedPriority(AActor* Actor, APlayerController* Viewer) const;
    float CalculateGameplayBasedPriority(AActor* Actor, APlayerController* Viewer) const;
    
    // Connection analysis
    void AnalyzeConnectionQuality(FConnectionProfile& Profile);
    ENetworkCompressionLevel GetOptimalCompressionLevel(const FConnectionProfile& Profile) const;
    
    // Delta compression
    void ApplyDeltaCompression(TArray<uint8>& Data, const TArray<uint8>& PreviousData) const;
    void RestoreFromDelta(TArray<uint8>& Data, const TArray<uint8>& DeltaData, const TArray<uint8>& BaseData) const;
    
    // Statistics and telemetry
    void UpdateNetworkStatistics(float DeltaTime);
    void RecordNetworkEvent(const FString& EventType, float Value);
    
    // Helper functions
    bool IsActorRelevantToPlayer(AActor* Actor, APlayerController* Player) const;
    float GetNetworkDistance(AActor* Actor, APlayerController* Player) const;
    bool IsWithinNetworkBudget(float AdditionalBytes) const;
};