#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "MultiplayerNetworkOptimizer.generated.h"

class ANetworkedClimbingCharacter;
class UNetworkedRopeComponent;
class AClimbingPlayerState;
class UCooperativeInventory;

UENUM(BlueprintType)
enum class ENetworkPriorityLevel : uint8
{
    Critical,           // Essential for gameplay (player movement, rope physics during falls)
    High,              // Important but not critical (tool usage, belay actions)
    Medium,            // Regular updates (inventory, UI state)
    Low,               // Background updates (statistics, non-critical notifications)
    Background         // Minimal priority (long-term stats, cleanup)
};

UENUM(BlueprintType)
enum class ENetworkOptimizationMode : uint8
{
    Performance,        // Prioritize smooth gameplay
    Bandwidth,         // Minimize bandwidth usage
    Balanced,          // Balance between performance and bandwidth
    Adaptive,          // Adapt based on network conditions
    HighFidelity       // Maximum accuracy, higher bandwidth
};

USTRUCT(BlueprintType)
struct FNetworkStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float AverageLatency = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PacketLoss = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Bandwidth = 0.0f; // KB/s

    UPROPERTY(BlueprintReadOnly)
    int32 UpdatesPerSecond = 0;

    UPROPERTY(BlueprintReadOnly)
    float JitterVariation = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveConnections = 0;

    UPROPERTY(BlueprintReadOnly)
    float NetworkQualityScore = 1.0f; // 0.0 to 1.0

    FNetworkStats()
    {
        AverageLatency = 0.0f;
        PacketLoss = 0.0f;
        Bandwidth = 0.0f;
        UpdatesPerSecond = 0;
        JitterVariation = 0.0f;
        ActiveConnections = 0;
        NetworkQualityScore = 1.0f;
    }
};

USTRUCT(BlueprintType)
struct FPlayerNetworkProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Player = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FNetworkStats NetworkStats;

    UPROPERTY(BlueprintReadOnly)
    ENetworkPriorityLevel CurrentPriority = ENetworkPriorityLevel::Medium;

    UPROPERTY(BlueprintReadOnly)
    float UpdateFrequency = 20.0f; // Hz

    UPROPERTY(BlueprintReadOnly)
    float CullingDistance = 5000.0f; // cm

    UPROPERTY(BlueprintReadOnly)
    bool bIsOptimized = false;

    UPROPERTY(BlueprintReadOnly)
    float ImportanceScore = 0.5f; // 0.0 to 1.0

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime = 0.0f;

    FPlayerNetworkProfile()
    {
        Player = nullptr;
        CurrentPriority = ENetworkPriorityLevel::Medium;
        UpdateFrequency = 20.0f;
        CullingDistance = 5000.0f;
        bIsOptimized = false;
        ImportanceScore = 0.5f;
        LastUpdateTime = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FPhysicsOptimizationSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseAdaptiveLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HighDetailDistance = 2000.0f; // 20m

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MediumDetailDistance = 5000.0f; // 50m

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LowDetailDistance = 10000.0f; // 100m

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRopeSimulationNodes = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PhysicsUpdateRate = 60.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float NetworkPhysicsUpdateRate = 20.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUsePhysicsPrediction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PredictionAccuracy = 0.95f; // 95% accuracy target

    FPhysicsOptimizationSettings()
    {
        bUseAdaptiveLOD = true;
        HighDetailDistance = 2000.0f;
        MediumDetailDistance = 5000.0f;
        LowDetailDistance = 10000.0f;
        MaxRopeSimulationNodes = 20;
        PhysicsUpdateRate = 60.0f;
        NetworkPhysicsUpdateRate = 20.0f;
        bUsePhysicsPrediction = true;
        PredictionAccuracy = 0.95f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNetworkQualityChanged, float, QualityScore, FNetworkStats, Stats);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerOptimized, AClimbingPlayerState*, Player, ENetworkPriorityLevel, NewPriority, float, ImportanceScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhysicsLODChanged, int32, NewLODLevel, float, Distance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkOptimizationModeChanged, ENetworkOptimizationMode, NewMode);

/**
 * Advanced network optimization system for multiplayer climbing gameplay
 * Manages bandwidth, update rates, physics LOD, and player priority systems
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UMultiplayerNetworkOptimizer : public UActorComponent
{
    GENERATED_BODY()

public:
    UMultiplayerNetworkOptimizer();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    ENetworkOptimizationMode OptimizationMode = ENetworkOptimizationMode::Balanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float TargetBandwidth = 128.0f; // KB/s per player

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float MaxLatencyThreshold = 150.0f; // ms

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float MaxPacketLossThreshold = 5.0f; // %

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    bool bUseAdaptiveOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float OptimizationUpdateInterval = 1.0f; // seconds

    // Physics optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Config")
    FPhysicsOptimizationSettings PhysicsSettings;

    // Current network state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    FNetworkStats GlobalNetworkStats;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    TArray<FPlayerNetworkProfile> PlayerProfiles;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    int32 CurrentPhysicsLOD = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Network State")
    float CurrentBandwidthUsage = 0.0f;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnNetworkQualityChanged OnNetworkQualityChanged;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnPlayerOptimized OnPlayerOptimized;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnPhysicsLODChanged OnPhysicsLODChanged;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnNetworkOptimizationModeChanged OnNetworkOptimizationModeChanged;

    // Network optimization management
    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void SetOptimizationMode(ENetworkOptimizationMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeForPlayer(AClimbingPlayerState* Player);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeAllPlayers();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void ResetOptimizations();

    // Player priority management
    UFUNCTION(BlueprintCallable, Category = "Player Priority")
    void SetPlayerPriority(AClimbingPlayerState* Player, ENetworkPriorityLevel Priority);

    UFUNCTION(BlueprintPure, Category = "Player Priority")
    ENetworkPriorityLevel GetPlayerPriority(AClimbingPlayerState* Player) const;

    UFUNCTION(BlueprintCallable, Category = "Player Priority")
    void UpdatePlayerImportanceScores();

    UFUNCTION(BlueprintPure, Category = "Player Priority")
    float GetPlayerImportanceScore(AClimbingPlayerState* Player) const;

    // Bandwidth management
    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void SetTargetBandwidth(float NewBandwidth);

    UFUNCTION(BlueprintPure, Category = "Bandwidth Management")
    float GetCurrentBandwidthUsage() const { return CurrentBandwidthUsage; }

    UFUNCTION(BlueprintPure, Category = "Bandwidth Management")
    float GetBandwidthAllocation(AClimbingPlayerState* Player) const;

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void ThrottleBandwidthForPlayer(AClimbingPlayerState* Player, float ThrottlePercent);

    // Physics optimization
    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void SetPhysicsLODLevel(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizeRopePhysics(UNetworkedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizePlayerPhysics(ANetworkedClimbingCharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void UpdatePhysicsLODBasedOnDistance();

    // Prediction and interpolation
    UFUNCTION(BlueprintCallable, Category = "Prediction")
    void EnablePredictionForPlayer(AClimbingPlayerState* Player, bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    void SetPredictionAccuracy(float Accuracy);

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    FVector PredictPlayerMovement(AClimbingPlayerState* Player, float PredictionTime);

    // Culling and LOD
    UFUNCTION(BlueprintCallable, Category = "Culling")
    void SetCullingDistance(AClimbingPlayerState* Player, float Distance);

    UFUNCTION(BlueprintCallable, Category = "Culling")
    void UpdateNetworkCulling();

    UFUNCTION(BlueprintPure, Category = "Culling")
    bool IsPlayerCulled(AClimbingPlayerState* Player, AClimbingPlayerState* Observer) const;

    // Network statistics
    UFUNCTION(BlueprintPure, Category = "Network Stats")
    FNetworkStats GetNetworkStats() const { return GlobalNetworkStats; }

    UFUNCTION(BlueprintPure, Category = "Network Stats")
    FPlayerNetworkProfile GetPlayerNetworkProfile(AClimbingPlayerState* Player) const;

    UFUNCTION(BlueprintCallable, Category = "Network Stats")
    void UpdateNetworkStatistics();

    UFUNCTION(BlueprintPure, Category = "Network Stats")
    float CalculateNetworkQuality() const;

    // Adaptive optimization
    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void EnableAdaptiveOptimization(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void AdaptToNetworkConditions();

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void HandleNetworkDegradation();

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void HandleNetworkImprovement();

    // Emergency optimization
    UFUNCTION(BlueprintCallable, Category = "Emergency Optimization")
    void ActivateEmergencyMode();

    UFUNCTION(BlueprintCallable, Category = "Emergency Optimization")
    void DeactivateEmergencyMode();

    UFUNCTION(BlueprintPure, Category = "Emergency Optimization")
    bool IsInEmergencyMode() const { return bEmergencyModeActive; }

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void StartPerformanceMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void StopPerformanceMonitoring();

    UFUNCTION(BlueprintPure, Category = "Performance Monitoring")
    TArray<float> GetRecentLatencyHistory() const;

    UFUNCTION(BlueprintPure, Category = "Performance Monitoring")
    float GetAverageFrameTime() const;

    // Network RPCs
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetOptimizationMode(ENetworkOptimizationMode NewMode);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerOptimizePlayer(AClimbingPlayerState* Player);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetPlayerPriority(AClimbingPlayerState* Player, ENetworkPriorityLevel Priority);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateNetworkStats(const FNetworkStats& ClientStats);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOptimizationModeChanged(ENetworkOptimizationMode NewMode);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPhysicsLODChanged(int32 NewLODLevel);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastEmergencyModeActivated(bool bActivated);

    UFUNCTION(Client, Reliable)
    void ClientReceiveOptimizationSettings(const FPlayerNetworkProfile& Profile);

    UFUNCTION(Client, Reliable)
    void ClientUpdatePhysicsLOD(int32 NewLODLevel);

protected:
    // Internal optimization algorithms
    void CalculatePlayerImportance(AClimbingPlayerState* Player, float& OutImportanceScore);
    void OptimizeBandwidthDistribution();
    void UpdatePlayerUpdateRates();
    void CalculateOptimalLOD();

    // Network analysis
    void AnalyzeNetworkConditions();
    void DetectNetworkProblems();
    void PredictNetworkTrends();

    // Physics optimization internals
    void OptimizeRopeSimulation(UNetworkedRopeComponent* Rope, float Distance);
    void OptimizePlayerMovement(ANetworkedClimbingCharacter* Player, float Distance);
    void AdjustPhysicsQuality(float QualityLevel);

    // Adaptive algorithms
    void AdaptUpdateRates();
    void AdaptCompressionLevels();
    void AdaptCullingDistances();

    // Performance tracking
    void TrackPerformanceMetrics();
    void AnalyzePerformanceTrends();
    void OptimizeBasedOnPerformance();

    // Emergency handling
    void HandleCriticalNetworkFailure();
    void ImplementEmergencyProtocols();
    void RestoreNormalOperation();

    // Replication callbacks
    UFUNCTION()
    void OnRep_GlobalNetworkStats();

    UFUNCTION()
    void OnRep_PlayerProfiles();

    UFUNCTION()
    void OnRep_CurrentPhysicsLOD();

private:
    // Internal state
    bool bEmergencyModeActive = false;
    bool bPerformanceMonitoringActive = false;
    float LastOptimizationTime = 0.0f;
    float LastStatisticsUpdate = 0.0f;

    // Performance tracking
    TArray<float> LatencyHistory;
    TArray<float> FrameTimeHistory;
    TArray<float> BandwidthHistory;
    int32 MaxHistorySize = 60; // 1 minute at 1Hz

    // Optimization state
    TMap<AClimbingPlayerState*, float> PlayerImportanceScores;
    TMap<AClimbingPlayerState*, float> LastPlayerOptimization;
    
    // Physics optimization tracking
    TMap<UNetworkedRopeComponent*, int32> RopeLODLevels;
    TMap<ANetworkedClimbingCharacter*, int32> PlayerLODLevels;

    // Network condition tracking
    float NetworkQualityTrend = 0.0f;
    float LastNetworkAnalysis = 0.0f;
    float NetworkAnalysisInterval = 5.0f; // 5 seconds

    // Cached calculations
    mutable float CachedNetworkQuality = 1.0f;
    mutable float LastQualityCalculation = 0.0f;
    static constexpr float QualityCalculationInterval = 1.0f;

    // Helper functions
    void InitializeOptimizationProfiles();
    void UpdateOptimizationMetrics();
    void ValidateNetworkSettings();
    float CalculateDistanceToPlayer(AClimbingPlayerState* Player1, AClimbingPlayerState* Player2) const;
    bool ShouldOptimizePlayer(AClimbingPlayerState* Player) const;
    void LogOptimizationEvent(const FString& Event, const FString& Details = TEXT("")) const;
};