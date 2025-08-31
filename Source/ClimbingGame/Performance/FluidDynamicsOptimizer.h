#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "HAL/ThreadSafeBool.h"
#include "Async/TaskGraphInterfaces.h"
#include "../Physics/WaterPhysicsComponent.h"
#include "../Physics/CaveEnvironmentPhysics.h"
#include "../Physics/WaterfallRappellingPhysics.h"
#include "../Physics/CaveDivingPhysics.h"
#include "../Physics/CaveLightingSystem.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "FluidDynamicsOptimizer.generated.h"

UENUM(BlueprintType)
enum class EFluidDynamicsLOD : uint8
{
    Ultra           UMETA(DisplayName = "Ultra Quality"),
    High            UMETA(DisplayName = "High Quality"),
    Medium          UMETA(DisplayName = "Medium Quality"),
    Low             UMETA(DisplayName = "Low Quality"),
    Performance     UMETA(DisplayName = "Performance Mode"),
    Minimal         UMETA(DisplayName = "Minimal Quality")
};

UENUM(BlueprintType)
enum class EFluidSimulationPriority : uint8
{
    Critical        UMETA(DisplayName = "Critical - Always Simulate"),
    High            UMETA(DisplayName = "High Priority"),
    Medium          UMETA(DisplayName = "Medium Priority"),
    Low             UMETA(DisplayName = "Low Priority"),
    Background      UMETA(DisplayName = "Background Only"),
    Disabled        UMETA(DisplayName = "Disabled")
};

USTRUCT(BlueprintType)
struct FFluidPerformanceMetrics
{
    GENERATED_BODY()

    // Current performance data
    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float CurrentFrameRate = 60.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float FluidPhysicsFrameTime = 0.0f; // ms per frame

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float WaterSimulationTime = 0.0f; // ms per frame

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float CaveSimulationTime = 0.0f; // ms per frame

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float RopePhysicsTime = 0.0f; // ms per frame

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float LightingSystemTime = 0.0f; // ms per frame

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 ActiveWaterComponents = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 ActiveCaveComponents = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 ActiveRopeComponents = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    int32 MemoryUsageMB = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float GPUUtilization = 0.0f; // % GPU usage

    UPROPERTY(BlueprintReadOnly, Category = "Performance Metrics")
    float PhysicsThreadUsage = 0.0f; // % physics thread usage
};

USTRUCT(BlueprintType)
struct FFluidOptimizationSettings
{
    GENERATED_BODY()

    // LOD configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    EFluidDynamicsLOD CurrentLOD = EFluidDynamicsLOD::High;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float LODTransitionDistance = 5000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    bool bAutoAdjustLOD = true;

    // Update frequency scaling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Frequency")
    float WaterPhysicsFrequency = 10.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Frequency")
    float CavePhysicsFrequency = 2.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Frequency")
    float LightingFrequency = 5.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Frequency")
    float UnderwaterRopeFrequency = 20.0f; // Hz

    // Simulation complexity limits
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Complexity Limits")
    int32 MaxWaterParticles = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Complexity Limits")
    int32 MaxCaveAirCells = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Complexity Limits")
    int32 MaxRopeSegments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Complexity Limits")
    int32 MaxLightSources = 20;

    // Threading configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threading")
    bool bUseMultithreading = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threading")
    int32 MaxPhysicsThreads = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threading")
    bool bAsyncWaterPhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threading")
    bool bAsyncCavePhysics = true;
};

USTRUCT(BlueprintType)
struct FPerformanceBudget
{
    GENERATED_BODY()

    // Performance budget allocation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    float TargetFrameRate = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    float MaxFluidPhysicsTime = 5.0f; // ms per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    float MaxWaterPhysicsTime = 2.0f; // ms per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    float MaxCavePhysicsTime = 1.0f; // ms per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    float MaxLightingTime = 1.5f; // ms per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    float MaxRopePhysicsTime = 1.0f; // ms per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    int32 MaxMemoryUsageMB = 500; // Total memory budget for fluid systems

    UPROPERTY(BlueprintReadOnly, Category = "Performance Budget")
    float CurrentBudgetUsage = 0.0f; // % of budget used

    UPROPERTY(BlueprintReadOnly, Category = "Performance Budget")
    bool bOverBudget = false;
};

UCLASS(ClassGroup=(Performance), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UFluidDynamicsOptimizer : public UActorComponent
{
    GENERATED_BODY()

public:
    UFluidDynamicsOptimizer();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Performance monitoring and control
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance State")
    FFluidPerformanceMetrics CurrentMetrics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization Settings")
    FFluidOptimizationSettings OptimizationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Budget")
    FPerformanceBudget PerformanceBudget;

    // System component references
    UPROPERTY()
    TArray<UWaterPhysicsComponent*> ManagedWaterComponents;

    UPROPERTY()
    TArray<UCaveEnvironmentPhysics*> ManagedCaveComponents;

    UPROPERTY()
    TArray<UWaterfallRappellingPhysics*> ManagedWaterfallComponents;

    UPROPERTY()
    TArray<UCaveDivingPhysics*> ManagedCaveDivingComponents;

    UPROPERTY()
    TArray<UCaveLightingSystem*> ManagedLightingComponents;

    UPROPERTY()
    TArray<UAdvancedRopeComponent*> ManagedRopeComponents;

    // Performance optimization functions
    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void OptimizeFluidDynamics(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void AdjustLODBasedOnPerformance();

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void SetGlobalFluidLOD(EFluidDynamicsLOD NewLOD);

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void OptimizeForTargetFrameRate(float TargetFPS);

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void SetPerformancePriorities(const TArray<EFluidSimulationPriority>& Priorities);

    // System registration
    UFUNCTION(BlueprintCallable, Category = "System Management")
    void RegisterWaterPhysicsComponent(UWaterPhysicsComponent* WaterComponent);

    UFUNCTION(BlueprintCallable, Category = "System Management")
    void RegisterCavePhysicsComponent(UCaveEnvironmentPhysics* CaveComponent);

    UFUNCTION(BlueprintCallable, Category = "System Management")
    void RegisterWaterfallComponent(UWaterfallRappellingPhysics* WaterfallComponent);

    UFUNCTION(BlueprintCallable, Category = "System Management")
    void RegisterCaveDivingComponent(UCaveDivingPhysics* CaveDivingComponent);

    UFUNCTION(BlueprintCallable, Category = "System Management")
    void RegisterLightingComponent(UCaveLightingSystem* LightingComponent);

    UFUNCTION(BlueprintCallable, Category = "System Management")
    void RegisterRopeComponent(UAdvancedRopeComponent* RopeComponent);

    // Unregistration
    UFUNCTION(BlueprintCallable, Category = "System Management")
    void UnregisterComponent(UActorComponent* Component);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void UpdatePerformanceMetrics(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsPerformanceWithinBudget() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetSystemPerformanceRatio() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void LogPerformanceWarning(const FString& SystemName, float OverheadMs);

    // Adaptive optimization
    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void ProcessAdaptiveOptimization(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void ReduceSimulationComplexity(float TargetReduction);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void IncreaseSimulationQuality(float QualityIncrease);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void CullDistantSimulations(const FVector& ViewerLocation, float CullDistance);

    // Threading and async optimization
    UFUNCTION(BlueprintCallable, Category = "Threading")
    void OptimizeThreadingConfiguration();

    UFUNCTION(BlueprintCallable, Category = "Threading")
    void ProcessAsyncWaterPhysics();

    UFUNCTION(BlueprintCallable, Category = "Threading")
    void ProcessAsyncCavePhysics();

    UFUNCTION(BlueprintCallable, Category = "Threading")
    void SynchronizeAsyncResults();

    // Memory optimization
    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void OptimizeMemoryUsage();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void ClearUnusedCaches();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void CompactPhysicsGrids();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    int32 GetEstimatedMemoryUsage() const;

    // Quality vs performance balancing
    UFUNCTION(BlueprintCallable, Category = "Quality Balance")
    void BalanceQualityForPerformance(float DesiredFrameTime);

    UFUNCTION(BlueprintCallable, Category = "Quality Balance")
    void SetPrioritySystemsOnly(bool bPriorityOnly);

    UFUNCTION(BlueprintCallable, Category = "Quality Balance")
    void AdjustSimulationRates(float PerformanceMultiplier);

    // Platform-specific optimization
    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void OptimizeForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void ApplyConsoleOptimizations();

    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void ApplyPCOptimizations();

    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void ApplyMobileOptimizations();

    // Events for performance monitoring
    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnPerformanceWarning;

    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnLODChanged;

    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnBudgetExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnOptimizationComplete;

protected:
    // Internal optimization functions
    void UpdateOptimizationStrategies(float DeltaTime);
    void MonitorSystemPerformance(float DeltaTime);
    void ApplyDynamicOptimizations();
    void ProcessDistanceCulling(const FVector& ViewerLocation);

    // System-specific optimization
    void OptimizeWaterPhysics(float PerformanceRatio);
    void OptimizeCavePhysics(float PerformanceRatio);
    void OptimizeRopePhysics(float PerformanceRatio);
    void OptimizeLightingSystem(float PerformanceRatio);

    // LOD management helpers
    EFluidDynamicsLOD CalculateOptimalLOD(float CurrentFrameTime) const;
    void ApplyLODToSystems(EFluidDynamicsLOD NewLOD);
    void ProcessLODTransition(EFluidDynamicsLOD FromLOD, EFluidDynamicsLOD ToLOD);

    // Threading optimization helpers
    void DistributeWorkloadAcrossThreads();
    void BalancePhysicsThreadWork();
    void OptimizeAsyncTaskScheduling();

    // Memory management helpers
    void OptimizeWaterGridMemory();
    void OptimizeCaveGridMemory();
    void OptimizeRopeSegmentMemory();
    void ClearOldCacheData();

    // Performance prediction
    float PredictFrameTime(const FFluidOptimizationSettings& TestSettings) const;
    bool WillSettingsCauseLag(const FFluidOptimizationSettings& TestSettings) const;
    float CalculateOptimalUpdateFrequency(UActorComponent* Component) const;

private:
    // Performance tracking
    TArray<float> RecentFrameTimes;
    float AverageFrameTime = 16.67f; // Target 60 FPS
    float PerformanceWindowDuration = 5.0f; // seconds
    int32 PerformanceSampleCount = 300; // 5 seconds at 60 FPS

    // Optimization timers
    float LastOptimizationUpdate = 0.0f;
    float OptimizationUpdateInterval = 1.0f; // 1Hz optimization checks
    float LastLODUpdate = 0.0f;
    float LODUpdateInterval = 2.0f; // 0.5Hz LOD changes

    // System performance history
    TMap<UActorComponent*, TArray<float>> ComponentFrameTimes;
    TMap<UActorComponent*, float> ComponentAverageFrameTimes;
    TMap<UActorComponent*, EFluidSimulationPriority> ComponentPriorities;

    // Threading management
    TArray<FGraphEventRef> AsyncTasks;
    FThreadSafeBool bAsyncTasksComplete;
    int32 AvailablePhysicsThreads = 2;

    // Memory tracking
    TMap<UActorComponent*, int32> ComponentMemoryUsage;
    int32 TotalMemoryUsage = 0;
    int32 PeakMemoryUsage = 0;

    // Dynamic optimization state
    bool bAdaptiveOptimizationEnabled = true;
    float OptimizationAggressiveness = 0.5f; // 0-1
    TArray<FString> AppliedOptimizations;

    // Platform detection
    bool bIsConsole = false;
    bool bIsMobile = false;
    bool bIsHighEndPC = true;

    // Cached optimization calculations
    mutable EFluidDynamicsLOD CachedOptimalLOD = EFluidDynamicsLOD::High;
    mutable float CachedOptimalLODTime = 0.0f;
    static constexpr float LODCacheTimeout = 1.0f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float FrameTimeThreshold = 20.0f; // ms - trigger optimization above this

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MemoryThresholdMB = 400.0f; // Trigger memory optimization above this

    UPROPERTY(EditAnywhere, Category = "Configuration")
    int32 PerformanceHistoryLength = 300; // Frames to track

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float HysteresisMargin = 0.1f; // Prevent oscillation in LOD changes

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bLogPerformanceDetails = false;

public:
    // Blueprint-accessible optimization controls
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization Configuration")
    bool bEnableAdaptiveOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization Configuration")
    bool bEnableDistanceCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization Configuration")
    bool bEnableMemoryOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization Configuration")
    bool bEnableThreadingOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimization Configuration")
    float GlobalOptimizationIntensity = 1.0f;

    // Performance debugging
    UFUNCTION(BlueprintCallable, Category = "Performance Debugging")
    void DumpPerformanceReport() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Debugging")
    void ResetPerformanceCounters();

    UFUNCTION(BlueprintCallable, Category = "Performance Debugging")
    FString GetDetailedPerformanceReport() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Debugging")
    void SetPerformanceLogging(bool bEnabled);

    // Integration with performance manager
    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;
};