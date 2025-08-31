#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetDriver.h"
#include "Engine/NetConnection.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "../Multiplayer/ClimbingNetworkOptimizer.h"
#include "ClimbingEnvironmentalHazardManager.h"
#include "EnvironmentalNetworkOptimizer.generated.h"

UENUM(BlueprintType)
enum class EHazardNetworkMode : uint8
{
    AuthorityOnly       UMETA(DisplayName = "Authority Only"),
    ClientSidePrediction UMETA(DisplayName = "Client Side Prediction"),
    ServerAuthoritative UMETA(DisplayName = "Server Authoritative"),
    Hybrid              UMETA(DisplayName = "Hybrid Sync"),
    P2P                 UMETA(DisplayName = "Peer to Peer")
};

UENUM(BlueprintType)
enum class EHazardRelevancy : uint8
{
    Always              UMETA(DisplayName = "Always Relevant"),
    Distance            UMETA(DisplayName = "Distance Based"),
    Importance          UMETA(DisplayName = "Importance Based"),
    PlayerCount         UMETA(DisplayName = "Player Count Based"),
    Performance         UMETA(DisplayName = "Performance Based"),
    Conditional         UMETA(DisplayName = "Conditional")
};

UENUM(BlueprintType)
enum class ENetworkCompressionLevel : uint8
{
    None                UMETA(DisplayName = "No Compression"),
    Light               UMETA(DisplayName = "Light Compression"),
    Medium              UMETA(DisplayName = "Medium Compression"),
    Heavy               UMETA(DisplayName = "Heavy Compression"),
    Aggressive          UMETA(DisplayName = "Aggressive Compression"),
    Adaptive            UMETA(DisplayName = "Adaptive Compression")
};

UENUM(BlueprintType)
enum class EHazardSyncMethod : uint8
{
    FullState           UMETA(DisplayName = "Full State Sync"),
    DeltaCompression    UMETA(DisplayName = "Delta Compression"),
    EventBased          UMETA(DisplayName = "Event Based"),
    Interpolated        UMETA(DisplayName = "Interpolated"),
    Predicted           UMETA(DisplayName = "Client Predicted")
};

USTRUCT(BlueprintType)
struct FHazardNetworkSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
    EHazardNetworkMode NetworkMode = EHazardNetworkMode::ServerAuthoritative;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
    EHazardRelevancy RelevancyMethod = EHazardRelevancy::Distance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
    EHazardSyncMethod SyncMethod = EHazardSyncMethod::DeltaCompression;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
    ENetworkCompressionLevel CompressionLevel = ENetworkCompressionLevel::Medium;

    // Update rates (Hz)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float CriticalHazardUpdateRate = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float StandardHazardUpdateRate = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float LowPriorityHazardUpdateRate = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float BackgroundHazardUpdateRate = 2.0f;

    // Relevancy settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
    float MaxRelevancyDistance = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
    float ImportanceThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
    int32 MaxHazardsPerClient = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
    bool bUsePlayerCountScaling = true;

    // Bandwidth optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    float MaxBandwidthPerClientKBps = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    float CompressionRatio = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    bool bEnableDeltaCompression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    bool bEnableAdaptiveBandwidth = true;

    // Prediction settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction")
    bool bEnableClientPrediction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction")
    float PredictionToleranceDistance = 500.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction")
    float PredictionTimeWindow = 0.5f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prediction")
    float CorrectionStrength = 0.8f;

    // Quality scaling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    bool bEnableNetworkQualityScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    float HighLatencyThresholdMs = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    float LowBandwidthThresholdKBps = 64.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    bool bReduceQualityOnHighLatency = true;
};

USTRUCT(BlueprintType)
struct FNetworkedHazardState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 HazardID = -1;

    UPROPERTY(BlueprintReadOnly)
    EEnvironmentalHazardType HazardType;

    UPROPERTY(BlueprintReadOnly)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly)
    FVector Scale = FVector::OneVector;

    UPROPERTY(BlueprintReadOnly)
    EHazardIntensity Intensity;

    UPROPERTY(BlueprintReadOnly)
    float ElapsedTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Duration = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    uint32 StateHash = 0;

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = true;

    UPROPERTY(BlueprintReadOnly)
    TArray<uint8> CompressedData;

    // Prediction data
    UPROPERTY(BlueprintReadOnly)
    FVector PredictedLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float PredictionConfidence = 1.0f;

    // Network relevancy
    UPROPERTY(BlueprintReadOnly)
    bool bIsRelevantToClient = false;

    UPROPERTY(BlueprintReadOnly)
    float RelevancyScore = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> RelevantPlayerIDs;
};

USTRUCT(BlueprintType)
struct FClientNetworkState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerID = -1;

    UPROPERTY(BlueprintReadOnly)
    FVector PlayerLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Latency = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float BandwidthKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PacketLossPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 RelevantHazardCount = 0;

    UPROPERTY(BlueprintReadOnly)
    float CurrentBandwidthUsageKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> RelevantHazardIDs;

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime = 0.0f;

    // Quality scaling state
    UPROPERTY(BlueprintReadOnly)
    float NetworkQualityMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsHighLatency = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsLowBandwidth = false;

    UPROPERTY(BlueprintReadOnly)
    EHazardQuality ForcedQualityLevel = EHazardQuality::High;
};

USTRUCT(BlueprintType)
struct FNetworkPerformanceStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float TotalBandwidthUsageKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageLatencyMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PacketLossPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ConnectedClients = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalNetworkedHazards = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveNetworkUpdates = 0;

    UPROPERTY(BlueprintReadOnly)
    float CompressionRatio = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DeltaCompressionSavingsPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 PredictionCorrections = 0;

    UPROPERTY(BlueprintReadOnly)
    float PredictionAccuracy = 100.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 QualityReductions = 0;

    UPROPERTY(BlueprintReadOnly)
    float NetworkOptimizationEfficiency = 100.0f;

    UPROPERTY(BlueprintReadOnly)
    TMap<EEnvironmentalHazardType, int32> NetworkedHazardsByType;

    UPROPERTY(BlueprintReadOnly)
    TMap<int32, FClientNetworkState> ClientStates;
};

UCLASS(BlueprintType, ClassGroup=(ClimbingGame), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalNetworkOptimizer : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalNetworkOptimizer();

    // Component interface
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Network replication support
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

    // Hazard network management
    UFUNCTION(BlueprintCallable, Category = "Hazard Network")
    void RegisterNetworkedHazard(int32 HazardID, const FActiveHazardInstance& HazardInstance);

    UFUNCTION(BlueprintCallable, Category = "Hazard Network")
    void UnregisterNetworkedHazard(int32 HazardID);

    UFUNCTION(BlueprintCallable, Category = "Hazard Network")
    void UpdateNetworkedHazard(int32 HazardID, const FActiveHazardInstance& HazardInstance);

    UFUNCTION(BlueprintCallable, Category = "Hazard Network")
    void SynchronizeAllHazards();

    // Network optimization
    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeNetworkTraffic();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void UpdateRelevancyCalculations();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void EnableAdaptiveBandwidth(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void SetNetworkQualityScaling(bool bEnable);

    // Compression and delta sync
    UFUNCTION(BlueprintCallable, Category = "Compression")
    void SetCompressionLevel(ENetworkCompressionLevel CompressionLevel);

    UFUNCTION(BlueprintCallable, Category = "Compression")
    void EnableDeltaCompression(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Compression")
    TArray<uint8> CompressHazardData(const FNetworkedHazardState& HazardState);

    UFUNCTION(BlueprintCallable, Category = "Compression")
    bool DecompressHazardData(const TArray<uint8>& CompressedData, FNetworkedHazardState& OutHazardState);

    // Client prediction
    UFUNCTION(BlueprintCallable, Category = "Client Prediction")
    void EnableClientPrediction(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Client Prediction")
    void PredictHazardMovement(FNetworkedHazardState& HazardState, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Client Prediction")
    void CorrectPredictionError(int32 HazardID, const FNetworkedHazardState& AuthoritativeState);

    // Bandwidth management
    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void SetBandwidthLimit(float LimitKBps);

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void UpdateBandwidthUsage();

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void PrioritizeHazardUpdates();

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void ThrottleUpdatesForClient(int32 ClientID, float ThrottlePercent);

    // Relevancy management
    UFUNCTION(BlueprintCallable, Category = "Relevancy Management")
    void UpdateHazardRelevancy(int32 HazardID);

    UFUNCTION(BlueprintCallable, Category = "Relevancy Management")
    bool IsHazardRelevantToClient(int32 HazardID, int32 ClientID) const;

    UFUNCTION(BlueprintCallable, Category = "Relevancy Management")
    void SetRelevancyDistance(float Distance);

    UFUNCTION(BlueprintCallable, Category = "Relevancy Management")
    void UpdateClientRelevantHazards(int32 ClientID);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Network Performance", BlueprintPure)
    FNetworkPerformanceStats GetNetworkStats() const;

    UFUNCTION(BlueprintCallable, Category = "Network Performance")
    void ResetNetworkStats();

    UFUNCTION(BlueprintCallable, Category = "Network Performance")
    float GetCurrentBandwidthUsage() const;

    UFUNCTION(BlueprintCallable, Category = "Network Performance")
    float GetAverageLatency() const;

    UFUNCTION(BlueprintCallable, Category = "Network Performance")
    bool IsNetworkPerformanceOptimal() const;

    // Client state management
    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void RegisterClient(int32 ClientID, const FVector& ClientLocation);

    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void UpdateClientState(int32 ClientID, const FClientNetworkState& ClientState);

    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void UnregisterClient(int32 ClientID);

    UFUNCTION(BlueprintCallable, Category = "Client Management")
    FClientNetworkState GetClientState(int32 ClientID) const;

    // Network events (RPCs)
    UFUNCTION(Server, Reliable)
    void ServerCreateHazard(int32 HazardID, const FNetworkedHazardState& HazardState);

    UFUNCTION(Server, Reliable)
    void ServerUpdateHazard(int32 HazardID, const FNetworkedHazardState& HazardState);

    UFUNCTION(Server, Reliable)
    void ServerDestroyHazard(int32 HazardID);

    UFUNCTION(Client, Reliable)
    void ClientCreateHazard(int32 HazardID, const FNetworkedHazardState& HazardState);

    UFUNCTION(Client, Reliable)
    void ClientUpdateHazard(int32 HazardID, const FNetworkedHazardState& HazardState);

    UFUNCTION(Client, Reliable)
    void ClientDestroyHazard(int32 HazardID);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastSyncHazardBatch(const TArray<FNetworkedHazardState>& HazardStates);

    // Debug and profiling
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowNetworkDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogNetworkStats();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpNetworkedHazards();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RunNetworkBenchmark(float Duration = 10.0f);

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
    FHazardNetworkSettings NetworkSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableNetworkOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float NetworkOptimizationInterval = 1.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    int32 MaxBatchSize = 50; // hazards per batch

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bLogNetworkActivity = false;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnHazardNetworked;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnNetworkOptimized;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnBandwidthExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnHighLatencyDetected;

protected:
    // Replicated properties
    UPROPERTY(Replicated)
    TArray<FNetworkedHazardState> ReplicatedHazardStates;

    UPROPERTY(Replicated)
    FNetworkPerformanceStats ReplicatedNetworkStats;

    // Network state tracking
    UPROPERTY()
    TMap<int32, FNetworkedHazardState> NetworkedHazards;

    UPROPERTY()
    TMap<int32, FClientNetworkState> ConnectedClients;

    // Performance tracking
    FNetworkPerformanceStats CurrentStats;
    float LastNetworkOptimization = 0.0f;
    float LastRelevancyUpdate = 0.0f;
    float LastBandwidthUpdate = 0.0f;

    // Delta compression state
    TMap<int32, FNetworkedHazardState> LastSentStates;
    TMap<int32, uint32> StateHashes;

    // Prediction state
    TMap<int32, FVector> PredictedLocations;
    TMap<int32, float> PredictionErrors;

    // Bandwidth tracking
    float CurrentBandwidthUsage = 0.0f;
    TArray<float> BandwidthHistory;
    int32 BandwidthHistoryIndex = 0;

    // Debug state
    bool bShowDebugInfo = false;
    bool bIsRunningBenchmark = false;
    float BenchmarkStartTime = 0.0f;

    // Integration references
    UPROPERTY()
    UClimbingEnvironmentalHazardManager* HazardManager = nullptr;

    UPROPERTY()
    UClimbingNetworkOptimizer* NetworkOptimizer = nullptr;

private:
    // Core network functions
    void UpdateNetworkedHazards(float DeltaTime);
    void UpdateClientStates(float DeltaTime);
    void UpdateNetworkOptimization(float DeltaTime);
    void UpdateBandwidthManagement(float DeltaTime);
    
    // Hazard synchronization
    void SyncHazardToClients(int32 HazardID, const FNetworkedHazardState& HazardState);
    void SyncHazardBatch(const TArray<int32>& HazardIDs);
    FNetworkedHazardState CreateNetworkedHazardState(const FActiveHazardInstance& HazardInstance);
    
    // Relevancy calculation
    void CalculateHazardRelevancy(int32 HazardID);
    float CalculateRelevancyScore(int32 HazardID, int32 ClientID) const;
    bool IsWithinRelevancyDistance(const FVector& HazardLocation, const FVector& ClientLocation) const;
    void UpdateRelevantClientsForHazard(int32 HazardID);
    
    // Compression implementation
    TArray<uint8> CompressWithLevel(const FNetworkedHazardState& HazardState, ENetworkCompressionLevel Level);
    bool DecompressWithLevel(const TArray<uint8>& CompressedData, FNetworkedHazardState& OutState, ENetworkCompressionLevel Level);
    TArray<uint8> CreateDeltaCompression(const FNetworkedHazardState& Current, const FNetworkedHazardState& Previous);
    bool ApplyDeltaCompression(const TArray<uint8>& DeltaData, FNetworkedHazardState& BaseState);
    
    // Prediction implementation
    FVector PredictHazardLocation(const FNetworkedHazardState& HazardState, float DeltaTime);
    void ValidatePrediction(int32 HazardID, const FNetworkedHazardState& AuthoritativeState);
    void ApplyPredictionCorrection(int32 HazardID, const FVector& CorrectionVector);
    
    // Bandwidth optimization
    void CalculateBandwidthUsage();
    void PrioritizeHazardUpdates(TArray<int32>& HazardIDs);
    void ThrottleClientUpdates(int32 ClientID);
    bool ShouldSendUpdateToClient(int32 HazardID, int32 ClientID) const;
    
    // Quality scaling
    void UpdateNetworkQualityScaling();
    void ApplyQualityScalingToClient(int32 ClientID);
    float CalculateNetworkQualityMultiplier(const FClientNetworkState& ClientState) const;
    
    // Performance monitoring
    void UpdateNetworkStats(float DeltaTime);
    void CalculateLatencyStats();
    void CalculatePacketLossStats();
    void CalculateCompressionStats();
    
    // Utility functions
    bool IsServer() const;
    bool IsClient() const;
    int32 GetLocalPlayerID() const;
    FVector GetPlayerLocation(int32 ClientID) const;
    void UpdateStateHash(FNetworkedHazardState& HazardState);
    bool HasStateChanged(const FNetworkedHazardState& Current, const FNetworkedHazardState& Previous) const;
    
    // Integration helpers
    void InitializeIntegrationReferences();
    void UpdateIntegrationSystems();
    
    // Debug helpers
    void DrawNetworkDebugInfo();
    void DrawHazardRelevancyDebug(int32 HazardID);
    void LogHazardNetworkState(int32 HazardID) const;
    void LogClientNetworkState(int32 ClientID) const;
    
    // Benchmark helpers
    void UpdateBenchmark(float DeltaTime);
    void CompleteBenchmark();
};