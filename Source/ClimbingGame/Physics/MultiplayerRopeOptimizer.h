#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "AdvancedRopeComponent.h"
#include "../Multiplayer/ClimbingGameState.h"
#include "MultiplayerRopeOptimizer.generated.h"

UENUM(BlueprintType)
enum class ERopeOptimizationLevel : uint8
{
    Ultra           UMETA(DisplayName = "Ultra Quality"),
    High            UMETA(DisplayName = "High Quality"),
    Medium          UMETA(DisplayName = "Medium Quality"),
    Low             UMETA(DisplayName = "Low Quality"),
    Minimal         UMETA(DisplayName = "Minimal Quality"),
    ClientPredicted UMETA(DisplayName = "Client Predicted")
};

UENUM(BlueprintType)
enum class ERopeUpdateStrategy : uint8
{
    FullSimulation  UMETA(DisplayName = "Full Physics Simulation"),
    KeyFrames       UMETA(DisplayName = "Key Frame Interpolation"),
    SplineApprox    UMETA(DisplayName = "Spline Approximation"),
    StaticPositions UMETA(DisplayName = "Static Positions"),
    ClientPredict   UMETA(DisplayName = "Client Side Prediction"),
    Hybrid          UMETA(DisplayName = "Hybrid Approach")
};

UENUM(BlueprintType)
enum class ERopePriority : uint8
{
    Critical        UMETA(DisplayName = "Critical Priority"),
    High            UMETA(DisplayName = "High Priority"),
    Medium          UMETA(DisplayName = "Medium Priority"),
    Low             UMETA(DisplayName = "Low Priority"),
    Background      UMETA(DisplayName = "Background Priority"),
    Disabled        UMETA(DisplayName = "Disabled")
};

USTRUCT(BlueprintType)
struct FRopeOptimizationSettings
{
    GENERATED_BODY()

    // Distance-based LOD settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance LOD")
    float UltraQualityDistance = 500.0f;      // 5 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance LOD")
    float HighQualityDistance = 1500.0f;      // 15 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance LOD")
    float MediumQualityDistance = 3000.0f;    // 30 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance LOD")
    float LowQualityDistance = 5000.0f;       // 50 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance LOD")
    float MinimalQualityDistance = 8000.0f;   // 80 meters

    // Performance targets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxActiveRopes = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float TargetFrameTime = 16.67f; // 60 FPS in milliseconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxPhysicsTime = 5.0f; // milliseconds per frame for rope physics

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxSegmentsPerFrame = 1000; // Maximum total segments to update per frame

    // Network optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    float NetworkUpdateRate = 20.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    float PriorityUpdateRate = 60.0f; // Hz for high priority ropes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    float MaxNetworkBandwidth = 1024.0f; // bytes per second per rope

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    bool bEnableClientPrediction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    bool bEnableCompression = true;

    // Culling settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling")
    bool bEnableFrustumCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling")
    bool bEnableOcclusionCulling = false; // Expensive for ropes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling")
    float CullingMargin = 100.0f; // cm

    // Quality scaling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    TMap<ERopeOptimizationLevel, int32> SegmentCounts = {
        {ERopeOptimizationLevel::Ultra, 64},
        {ERopeOptimizationLevel::High, 32},
        {ERopeOptimizationLevel::Medium, 16},
        {ERopeOptimizationLevel::Low, 8},
        {ERopeOptimizationLevel::Minimal, 4},
        {ERopeOptimizationLevel::ClientPredicted, 16}
    };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    TMap<ERopeOptimizationLevel, float> UpdateRates = {
        {ERopeOptimizationLevel::Ultra, 60.0f},
        {ERopeOptimizationLevel::High, 30.0f},
        {ERopeOptimizationLevel::Medium, 20.0f},
        {ERopeOptimizationLevel::Low, 10.0f},
        {ERopeOptimizationLevel::Minimal, 5.0f},
        {ERopeOptimizationLevel::ClientPredicted, 60.0f}
    };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    TMap<ERopeOptimizationLevel, int32> SolverIterations = {
        {ERopeOptimizationLevel::Ultra, 8},
        {ERopeOptimizationLevel::High, 6},
        {ERopeOptimizationLevel::Medium, 4},
        {ERopeOptimizationLevel::Low, 2},
        {ERopeOptimizationLevel::Minimal, 1},
        {ERopeOptimizationLevel::ClientPredicted, 4}
    };

    // Adaptive settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive")
    bool bEnableAdaptiveQuality = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive")
    float PerformanceCheckInterval = 1.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive")
    float QualityAdjustmentRate = 0.1f; // How quickly to adjust quality
};

USTRUCT(BlueprintType)
struct FRopeNetworkState
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVector> KeyPoints; // Compressed rope positions

    UPROPERTY()
    float Tension = 0.0f;

    UPROPERTY()
    float Length = 0.0f;

    UPROPERTY()
    uint8 UpdateFlags = 0; // Bitfield for what changed

    UPROPERTY()
    float Timestamp = 0.0f;
};

USTRUCT(BlueprintType)
struct FRopePerformanceData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float PhysicsTime = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly)
    float NetworkTime = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly)
    float RenderTime = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveSegments = 0;

    UPROPERTY(BlueprintReadOnly)
    float UpdateFrequency = 0.0f; // Hz

    UPROPERTY(BlueprintReadOnly)
    float NetworkBandwidth = 0.0f; // bytes per second

    UPROPERTY(BlueprintReadOnly)
    ERopeOptimizationLevel CurrentLevel = ERopeOptimizationLevel::Medium;

    UPROPERTY(BlueprintReadOnly)
    bool bIsVisible = true;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToViewer = 0.0f;
};

USTRUCT(BlueprintType)
struct FRopeOptimizationData
{
    GENERATED_BODY()

    UPROPERTY()
    UAdvancedRopeComponent* RopeComponent = nullptr;

    UPROPERTY()
    ERopeOptimizationLevel CurrentLevel = ERopeOptimizationLevel::Medium;

    UPROPERTY()
    ERopePriority Priority = ERopePriority::Medium;

    UPROPERTY()
    ERopeUpdateStrategy UpdateStrategy = ERopeUpdateStrategy::FullSimulation;

    UPROPERTY()
    float LastUpdateTime = 0.0f;

    UPROPERTY()
    float NextUpdateTime = 0.0f;

    UPROPERTY()
    FRopePerformanceData PerformanceData;

    UPROPERTY()
    FRopeNetworkState NetworkState;

    // Client prediction data
    UPROPERTY()
    TArray<FVector> PredictedPositions;

    UPROPERTY()
    float PredictionAccuracy = 1.0f;

    // Spline approximation data
    UPROPERTY()
    TArray<FVector> SplineControlPoints;

    UPROPERTY()
    bool bSplineNeedsUpdate = true;

    // Performance tracking
    UPROPERTY()
    TArray<float> FrameTimeHistory;

    UPROPERTY()
    int32 UpdatesThisFrame = 0;

    UPROPERTY()
    bool bIsActiveThisFrame = false;
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UMultiplayerRopeOptimizer : public UActorComponent
{
    GENERATED_BODY()

public:
    UMultiplayerRopeOptimizer();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Optimization")
    FRopeOptimizationSettings Settings;

    // Rope registration and management
    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    void RegisterRope(UAdvancedRopeComponent* Rope, ERopePriority Priority = ERopePriority::Medium);

    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    void UnregisterRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    void SetRopePriority(UAdvancedRopeComponent* Rope, ERopePriority Priority);

    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    void SetRopeUpdateStrategy(UAdvancedRopeComponent* Rope, ERopeUpdateStrategy Strategy);

    // Optimization control
    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void OptimizeAllRopes();

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void SetGlobalOptimizationLevel(ERopeOptimizationLevel Level);

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    ERopeOptimizationLevel GetOptimalLevelForDistance(float Distance) const;

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    void UpdateRopeQuality(UAdvancedRopeComponent* Rope, ERopeOptimizationLevel Level);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance")
    FRopePerformanceData GetRopePerformanceData(UAdvancedRopeComponent* Rope) const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    float GetTotalPhysicsTime() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    float GetAverageNetworkBandwidth() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    int32 GetActiveRopeCount() const;

    // Network optimization
    UFUNCTION(BlueprintCallable, Category = "Network")
    void CompressRopeState(UAdvancedRopeComponent* Rope, FRopeNetworkState& OutState);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void DecompressRopeState(UAdvancedRopeComponent* Rope, const FRopeNetworkState& State);

    UFUNCTION(BlueprintCallable, Category = "Network")
    bool ShouldSendRopeUpdate(UAdvancedRopeComponent* Rope) const;

    UFUNCTION(Server, Unreliable, WithValidation)
    void ServerUpdateRopeState(UAdvancedRopeComponent* Rope, const FRopeNetworkState& State);

    // Client prediction
    UFUNCTION(BlueprintCallable, Category = "Client Prediction")
    void PredictRopePosition(UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Client Prediction")
    void CorrectRopePrediction(UAdvancedRopeComponent* Rope, const FRopeNetworkState& AuthoritativeState);

    UFUNCTION(BlueprintCallable, Category = "Client Prediction")
    float CalculatePredictionAccuracy(UAdvancedRopeComponent* Rope) const;

    // Adaptive quality
    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void UpdateAdaptiveQuality();

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void AdjustQualityForPerformance(float TargetFrameTime);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void ScaleQualityByPlayerCount(int32 PlayerCount);

    // Spline approximation
    UFUNCTION(BlueprintCallable, Category = "Spline Approximation")
    void GenerateSplineApproximation(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Spline Approximation")
    void UpdateRopeFromSpline(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Spline Approximation")
    bool IsSplineAccurate(UAdvancedRopeComponent* Rope, float Tolerance = 5.0f) const;

    // Batch processing
    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void ProcessRopeBatch(const TArray<UAdvancedRopeComponent*>& RopeBatch, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void DistributeWorkloadAcrossFrames(float AvailableTime);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Optimization Events")
    FSimpleMulticastDelegate OnQualityLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "Optimization Events")
    FSimpleMulticastDelegate OnPerformanceThresholdExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Optimization Events")
    FSimpleMulticastDelegate OnNetworkBandwidthLimitReached;

protected:
    // Internal optimization data
    UPROPERTY(Replicated)
    TArray<FRopeOptimizationData> OptimizedRopes;

    // Performance tracking
    float TotalPhysicsTimeThisFrame = 0.0f;
    float TotalNetworkBandwidth = 0.0f;
    int32 RopesUpdatedThisFrame = 0;
    float LastPerformanceCheck = 0.0f;

    // Workload distribution
    int32 CurrentBatchIndex = 0;
    float WorkloadBudgetPerFrame = 5.0f; // milliseconds
    TArray<UAdvancedRopeComponent*> PendingUpdates;

    // Network state management
    TMap<UAdvancedRopeComponent*, float> LastNetworkUpdate;
    TMap<UAdvancedRopeComponent*, FRopeNetworkState> NetworkStateCache;

private:
    // Internal methods
    FRopeOptimizationData* FindOptimizationData(UAdvancedRopeComponent* Rope);
    const FRopeOptimizationData* FindOptimizationData(UAdvancedRopeComponent* Rope) const;
    
    void UpdateRopeOptimizationLevel(FRopeOptimizationData& OptData);
    void ApplyOptimizationLevel(FRopeOptimizationData& OptData);
    
    bool IsRopeVisible(UAdvancedRopeComponent* Rope) const;
    float CalculateRopeDistance(UAdvancedRopeComponent* Rope) const;
    
    void UpdateRopePhysics(FRopeOptimizationData& OptData, float DeltaTime);
    void UpdateRopeNetwork(FRopeOptimizationData& OptData, float DeltaTime);
    
    ERopeOptimizationLevel DetermineOptimalLevel(const FRopeOptimizationData& OptData) const;
    bool ShouldUpdateRope(const FRopeOptimizationData& OptData, float CurrentTime) const;
    
    void CompressRopePositions(const TArray<FVector>& Positions, TArray<FVector>& OutCompressed) const;
    void DecompressRopePositions(const TArray<FVector>& Compressed, TArray<FVector>& OutPositions, int32 TargetCount) const;
    
    void PredictRopePhysics(FRopeOptimizationData& OptData, float DeltaTime);
    void UpdateSplineApproximation(FRopeOptimizationData& OptData);
    
    void PerformCulling();
    void BalanceWorkload(float AvailableTime);
    
    // Performance monitoring
    void TrackPhysicsTime(const FRopeOptimizationData& OptData, float PhysicsTime);
    void TrackNetworkBandwidth(const FRopeOptimizationData& OptData, float Bandwidth);
    void UpdatePerformanceMetrics(float DeltaTime);

    // Cache for frequently accessed data
    mutable TMap<UAdvancedRopeComponent*, float> DistanceCache;
    mutable TMap<UAdvancedRopeComponent*, bool> VisibilityCache;
    float LastCacheUpdate = 0.0f;
    float CacheUpdateInterval = 0.2f; // Update cache 5 times per second
};

// Global optimization manager
UCLASS()
class CLIMBINGGAME_API ARopeOptimizationManager : public AActor
{
    GENERATED_BODY()

public:
    ARopeOptimizationManager();

    // Global rope optimization settings
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMultiplayerRopeOptimizer* GlobalOptimizer;

    // Singleton access
    UFUNCTION(BlueprintCallable, Category = "Rope Optimization", CallInEditor = true)
    static ARopeOptimizationManager* GetGlobalOptimizationManager(const UWorld* World);

    // Global controls
    UFUNCTION(BlueprintCallable, Category = "Global Control")
    void SetGlobalRopeQuality(ERopeOptimizationLevel Quality);

    UFUNCTION(BlueprintCallable, Category = "Global Control")
    void AdaptToPlayerCount(int32 PlayerCount);

    UFUNCTION(BlueprintCallable, Category = "Global Control")
    void OptimizeForNetworkConditions(float Latency, float PacketLoss);

protected:
    virtual void BeginPlay() override;

private:
    static ARopeOptimizationManager* Instance;
};