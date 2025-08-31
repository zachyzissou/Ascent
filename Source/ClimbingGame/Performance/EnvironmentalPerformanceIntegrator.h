#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "../Performance/ClimbingFrameRateManager.h"
#include "../Performance/ClimbingLODManager.h"
#include "../Memory/ClimbingMemoryTracker.h"
#include "ClimbingEnvironmentalHazardManager.h"
#include "EnvironmentalPhysicsOptimizer.h"
#include "EnvironmentalEffectsLODSystem.h"
#include "EnvironmentalAssetMemoryManager.h"
#include "EnvironmentalNetworkOptimizer.h"
#include "EnvironmentalPerformanceBudgetSystem.h"
#include "EnvironmentalPerformanceIntegrator.generated.h"

UENUM(BlueprintType)
enum class EIntegrationMode : uint8
{
    Standalone          UMETA(DisplayName = "Standalone Mode"),
    Integrated          UMETA(DisplayName = "Integrated Mode"),
    MasterController    UMETA(DisplayName = "Master Controller"),
    SlaveController     UMETA(DisplayName = "Slave Controller")
};

UENUM(BlueprintType)
enum class ESystemPriority : uint8
{
    Core                UMETA(DisplayName = "Core System"),
    Environmental       UMETA(DisplayName = "Environmental System"),
    Supplementary       UMETA(DisplayName = "Supplementary System"),
    Optional            UMETA(DisplayName = "Optional System")
};

UENUM(BlueprintType)
enum class EOptimizationStrategy : uint8
{
    Conservative        UMETA(DisplayName = "Conservative"),
    Balanced            UMETA(DisplayName = "Balanced"),
    Aggressive          UMETA(DisplayName = "Aggressive"),
    Emergency           UMETA(DisplayName = "Emergency"),
    Custom              UMETA(DisplayName = "Custom Strategy")
};

USTRUCT(BlueprintType)
struct FSystemIntegrationConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    bool bIsEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    ESystemPriority Priority = ESystemPriority::Environmental;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    float UpdateWeight = 1.0f; // Relative importance in performance calculations

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    bool bAllowAutomaticOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    float OptimizationThreshold = 0.8f; // Trigger optimization at 80% budget usage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    bool bParticipateInGlobalLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    bool bParticipateInMemoryManagement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
    bool bParticipateInNetworkOptimization = true;

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    bool bIsInitialized = false;

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    float LastUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float CurrentPerformanceImpact = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AveragePerformanceImpact = 0.0f;
};

USTRUCT(BlueprintType)
struct FIntegratedPerformanceMetrics
{
    GENERATED_BODY()

    // Overall system performance
    UPROPERTY(BlueprintReadOnly)
    float OverallFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float EnvironmentalFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CoreSystemsFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PerformanceImpactPercent = 0.0f;

    // Memory metrics
    UPROPERTY(BlueprintReadOnly)
    float TotalMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float EnvironmentalMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MemoryEfficiencyPercent = 0.0f;

    // GPU metrics
    UPROPERTY(BlueprintReadOnly)
    float GPUUsagePercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float EnvironmentalGPUUsagePercent = 0.0f;

    // Network metrics
    UPROPERTY(BlueprintReadOnly)
    float NetworkBandwidthKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float NetworkLatencyMs = 0.0f;

    // Quality metrics
    UPROPERTY(BlueprintReadOnly)
    float AverageQualityLevel = 1.0f; // 0-1 scale

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveOptimizations = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bIsOptimalPerformance = true;

    // System-specific metrics
    UPROPERTY(BlueprintReadOnly)
    int32 ActiveHazards = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveParticleEffects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActivePhysicsObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 NetworkedObjects = 0;

    // Performance trends
    UPROPERTY(BlueprintReadOnly)
    float PerformanceTrend = 0.0f; // Positive = improving, Negative = degrading

    UPROPERTY(BlueprintReadOnly)
    int32 OptimizationEvents = 0;
};

USTRUCT(BlueprintType)
struct FGlobalOptimizationState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EOptimizationStrategy CurrentStrategy = EOptimizationStrategy::Balanced;

    UPROPERTY(BlueprintReadOnly)
    bool bGlobalOptimizationActive = false;

    UPROPERTY(BlueprintReadOnly)
    float OptimizationStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float QualityReductionLevel = 0.0f; // 0 = no reduction, 1 = maximum reduction

    UPROPERTY(BlueprintReadOnly)
    int32 SystemsOptimized = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bEmergencyModeActive = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> ActiveOptimizations;

    UPROPERTY(BlueprintReadOnly)
    float PerformanceGainPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ExpectedRecoveryTime = 0.0f;
};

UCLASS(BlueprintType, ClassGroup=(ClimbingGame), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalPerformanceIntegrator : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalPerformanceIntegrator();

    // Component interface
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // System integration
    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void InitializeIntegration();

    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void RegisterEnvironmentalSystem(UActorComponent* SystemComponent, const FSystemIntegrationConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void UnregisterEnvironmentalSystem(UActorComponent* SystemComponent);

    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void SetIntegrationMode(EIntegrationMode Mode);

    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void UpdateSystemPriorities();

    // Performance coordination
    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void CoordinatePerformanceOptimization();

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void TriggerGlobalOptimization(EOptimizationStrategy Strategy);

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void EndGlobalOptimization();

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void SynchronizeQualityLevels();

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void DistributePerformanceBudgets();

    // LOD integration
    UFUNCTION(BlueprintCallable, Category = "LOD Integration")
    void SynchronizeLODSystems();

    UFUNCTION(BlueprintCallable, Category = "LOD Integration")
    void SetGlobalLODBias(float LODBias);

    UFUNCTION(BlueprintCallable, Category = "LOD Integration")
    void UpdateIntegratedLODs();

    UFUNCTION(BlueprintCallable, Category = "LOD Integration")
    void EnableCascadingLOD(bool bEnable);

    // Memory integration
    UFUNCTION(BlueprintCallable, Category = "Memory Integration")
    void CoordinateMemoryManagement();

    UFUNCTION(BlueprintCallable, Category = "Memory Integration")
    void DistributeMemoryBudgets();

    UFUNCTION(BlueprintCallable, Category = "Memory Integration")
    void TriggerGlobalMemoryOptimization();

    UFUNCTION(BlueprintCallable, Category = "Memory Integration")
    void SynchronizeMemoryPooling();

    // Network integration
    UFUNCTION(BlueprintCallable, Category = "Network Integration")
    void CoordinateNetworkOptimization();

    UFUNCTION(BlueprintCallable, Category = "Network Integration")
    void SynchronizeNetworkRelevancy();

    UFUNCTION(BlueprintCallable, Category = "Network Integration")
    void DistributeNetworkBandwidth();

    UFUNCTION(BlueprintCallable, Category = "Network Integration")
    void UpdateNetworkQualityScaling();

    // Quality management
    UFUNCTION(BlueprintCallable, Category = "Quality Management")
    void SetGlobalQualityLevel(float QualityLevel);

    UFUNCTION(BlueprintCallable, Category = "Quality Management")
    void EnableAdaptiveQuality(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Quality Management")
    void ForceQualityAdjustment(bool bIncreaseQuality);

    UFUNCTION(BlueprintCallable, Category = "Quality Management")
    float CalculateOptimalQualityLevel() const;

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring", BlueprintPure)
    FIntegratedPerformanceMetrics GetIntegratedMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring", BlueprintPure)
    FGlobalOptimizationState GetOptimizationState() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void ResetIntegratedMetrics();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsPerformanceOptimal() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetSystemPerformanceImpact(UActorComponent* SystemComponent) const;

    // Hardware adaptation
    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void AdaptToHardwareTier(EHardwareTier HardwareTier);

    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void UpdateForPlayerCount(int32 PlayerCount);

    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void ScaleForResolution(int32 Width, int32 Height);

    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void OptimizeForPlatform(const FString& PlatformName);

    // Emergency management
    UFUNCTION(BlueprintCallable, Category = "Emergency Management")
    void ActivateEmergencyMode();

    UFUNCTION(BlueprintCallable, Category = "Emergency Management")
    void DeactivateEmergencyMode();

    UFUNCTION(BlueprintCallable, Category = "Emergency Management")
    bool IsInEmergencyMode() const { return GlobalOptimizationState.bEmergencyModeActive; }

    UFUNCTION(BlueprintCallable, Category = "Emergency Management")
    void SetEmergencyThresholds(float FrameTimeThreshold, float MemoryThreshold);

    // Debug and profiling
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowIntegrationDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogIntegratedPerformance();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpSystemStates();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RunIntegrationBenchmark(float Duration = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ValidateSystemIntegration();

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration Settings")
    EIntegrationMode IntegrationMode = EIntegrationMode::Integrated;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration Settings")
    TMap<FString, FSystemIntegrationConfig> SystemConfigs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    EOptimizationStrategy DefaultOptimizationStrategy = EOptimizationStrategy::Balanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float GlobalOptimizationThreshold = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableAutomaticOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float IntegrationUpdateInterval = 0.2f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency Settings")
    float EmergencyFrameTimeThreshold = 33.0f; // 30 FPS

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency Settings")
    float EmergencyMemoryThreshold = 1536.0f; // 1.5 GB

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency Settings")
    bool bAllowEmergencyMode = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Integration Events")
    FSimpleMulticastDelegate OnGlobalOptimizationStarted;

    UPROPERTY(BlueprintAssignable, Category = "Integration Events")
    FSimpleMulticastDelegate OnGlobalOptimizationEnded;

    UPROPERTY(BlueprintAssignable, Category = "Integration Events")
    FSimpleMulticastDelegate OnEmergencyModeActivated;

    UPROPERTY(BlueprintAssignable, Category = "Integration Events")
    FSimpleMulticastDelegate OnSystemIntegrated;

    UPROPERTY(BlueprintAssignable, Category = "Integration Events")
    FSimpleMulticastDelegate OnPerformanceTargetMissed;

protected:
    // System references
    UPROPERTY()
    UClimbingPerformanceManager* CorePerformanceManager = nullptr;

    UPROPERTY()
    UClimbingFrameRateManager* FrameRateManager = nullptr;

    UPROPERTY()
    UClimbingLODManager* LODManager = nullptr;

    UPROPERTY()
    UClimbingMemoryTracker* MemoryTracker = nullptr;

    UPROPERTY()
    UClimbingEnvironmentalHazardManager* HazardManager = nullptr;

    UPROPERTY()
    UEnvironmentalPhysicsOptimizer* PhysicsOptimizer = nullptr;

    UPROPERTY()
    UEnvironmentalEffectsLODSystem* EffectsLODSystem = nullptr;

    UPROPERTY()
    UEnvironmentalAssetMemoryManager* AssetMemoryManager = nullptr;

    UPROPERTY()
    UEnvironmentalNetworkOptimizer* NetworkOptimizer = nullptr;

    UPROPERTY()
    UEnvironmentalPerformanceBudgetSystem* BudgetSystem = nullptr;

    // Integration state
    UPROPERTY()
    TMap<UActorComponent*, FSystemIntegrationConfig> RegisteredSystems;

    FIntegratedPerformanceMetrics CurrentMetrics;
    FGlobalOptimizationState GlobalOptimizationState;

    float LastIntegrationUpdate = 0.0f;
    float LastQualitySync = 0.0f;
    float LastBudgetDistribution = 0.0f;

    // Debug state
    bool bShowDebugInfo = false;
    bool bIsRunningBenchmark = false;
    float BenchmarkStartTime = 0.0f;

private:
    // Core integration functions
    void UpdateIntegration(float DeltaTime);
    void UpdatePerformanceCoordination(float DeltaTime);
    void UpdateSystemSynchronization(float DeltaTime);
    void UpdateMetrics(float DeltaTime);
    
    // System initialization
    void InitializeSystemReferences();
    void InitializeSystemConfigs();
    void ValidateSystemReferences();
    void EstablishSystemConnections();
    
    // Performance coordination implementation
    void AnalyzeGlobalPerformance();
    void DetermineOptimizationStrategy();
    void ExecuteGlobalOptimization();
    void MonitorOptimizationProgress();
    void CompleteOptimization();
    
    // LOD coordination
    void CalculateGlobalLODBias();
    void SynchronizeSystemLODs();
    void UpdateCascadingLOD();
    void OptimizeLODDistribution();
    
    // Memory coordination
    void CalculateMemoryDistribution();
    void EnforceMemoryBudgets();
    void OptimizeGlobalMemoryUsage();
    void HandleMemoryPressure();
    
    // Network coordination
    void CalculateNetworkBandwidthDistribution();
    void OptimizeGlobalNetworkUsage();
    void SynchronizeNetworkQuality();
    void HandleNetworkCongestion();
    
    // Quality management implementation
    void CalculateGlobalQuality();
    void ApplyQualityAdjustments();
    void ValidateQualityLevels();
    void RecoverQualityWhenPossible();
    
    // Emergency mode implementation
    void CheckEmergencyConditions();
    void ExecuteEmergencyOptimizations();
    void MonitorEmergencyRecovery();
    void RestoreFromEmergency();
    
    // Performance analysis
    void AnalyzePerformanceTrends();
    void IdentifyPerformanceBottlenecks();
    void CalculateSystemImpacts();
    void PredictPerformanceChanges();
    
    // Hardware adaptation implementation
    void ApplyHardwareTierSettings(EHardwareTier Tier);
    void ScaleSystemsForPlayerCount(int32 PlayerCount);
    void AdjustForResolution(int32 Width, int32 Height);
    void OptimizeForPlatformSpecifics(const FString& Platform);
    
    // System communication
    void BroadcastOptimizationRequest(EOptimizationStrategy Strategy);
    void NotifySystemsOfQualityChange(float NewQualityLevel);
    void RequestSystemOptimization(UActorComponent* System, float OptimizationLevel);
    void SynchronizeSystemStates();
    
    // Utility functions
    float CalculateOverallPerformanceScore() const;
    bool ShouldTriggerGlobalOptimization() const;
    float GetSystemWeight(UActorComponent* System) const;
    EOptimizationStrategy DetermineOptimalStrategy() const;
    
    // Debug helpers
    void DrawIntegrationDebugInfo();
    void DrawSystemConnections();
    void DrawPerformanceMetrics();
    void LogSystemState(UActorComponent* System) const;
    
    // Benchmark helpers
    void UpdateBenchmark(float DeltaTime);
    void CompleteBenchmark();
    void AnalyzeBenchmarkResults();
};