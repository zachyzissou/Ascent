#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Stats/Stats.h"
#include "ProfilingDebugging/ScopedTimers.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "../Performance/ClimbingFrameRateManager.h"
#include "ClimbingEnvironmentalHazardManager.h"
#include "EnvironmentalPerformanceBudgetSystem.generated.h"

UENUM(BlueprintType)
enum class EPerformanceBudgetType : uint8
{
    FrameTime           UMETA(DisplayName = "Frame Time Budget"),
    Memory              UMETA(DisplayName = "Memory Budget"),
    GPU                 UMETA(DisplayName = "GPU Budget"),
    Network             UMETA(DisplayName = "Network Budget"),
    Physics             UMETA(DisplayName = "Physics Budget"),
    Rendering           UMETA(DisplayName = "Rendering Budget"),
    Audio               UMETA(DisplayName = "Audio Budget"),
    Streaming           UMETA(DisplayName = "Streaming Budget")
};

UENUM(BlueprintType)
enum class EBudgetPriority : uint8
{
    Critical            UMETA(DisplayName = "Critical Priority"),
    High                UMETA(DisplayName = "High Priority"),
    Medium              UMETA(DisplayName = "Medium Priority"),
    Low                 UMETA(DisplayName = "Low Priority"),
    Background          UMETA(DisplayName = "Background Priority")
};

UENUM(BlueprintType)
enum class EAdaptiveResponse : uint8
{
    None                UMETA(DisplayName = "No Response"),
    ReduceQuality       UMETA(DisplayName = "Reduce Quality"),
    CullEffects         UMETA(DisplayName = "Cull Effects"),
    ReduceDistance      UMETA(DisplayName = "Reduce Distance"),
    LowerUpdateRate     UMETA(DisplayName = "Lower Update Rate"),
    DisableFeatures     UMETA(DisplayName = "Disable Features"),
    Emergency           UMETA(DisplayName = "Emergency Mode")
};

UENUM(BlueprintType)
enum class EBudgetStatus : uint8
{
    Optimal             UMETA(DisplayName = "Optimal"),
    Warning             UMETA(DisplayName = "Warning"),
    Critical            UMETA(DisplayName = "Critical"),
    Emergency           UMETA(DisplayName = "Emergency"),
    Failure             UMETA(DisplayName = "Budget Failure")
};

USTRUCT(BlueprintType)
struct FPerformanceBudget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Configuration")
    EPerformanceBudgetType BudgetType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Configuration")
    FString BudgetName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Configuration")
    EBudgetPriority Priority = EBudgetPriority::Medium;

    // Budget limits
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Limits")
    float OptimalThreshold = 0.6f; // 60% of budget

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Limits")
    float WarningThreshold = 0.8f; // 80% of budget

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Limits")
    float CriticalThreshold = 0.95f; // 95% of budget

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Limits")
    float MaxBudget = 100.0f; // Maximum allowed value

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Limits")
    FString Unit = TEXT("ms"); // Unit of measurement

    // Current usage
    UPROPERTY(BlueprintReadOnly, Category = "Budget Status")
    float CurrentUsage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Budget Status")
    float PeakUsage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Budget Status")
    float AverageUsage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Budget Status")
    EBudgetStatus Status = EBudgetStatus::Optimal;

    UPROPERTY(BlueprintReadOnly, Category = "Budget Status")
    float UtilizationPercent = 0.0f;

    // Adaptive responses
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Response")
    TArray<EAdaptiveResponse> WarningResponses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Response")
    TArray<EAdaptiveResponse> CriticalResponses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Response")
    TArray<EAdaptiveResponse> EmergencyResponses;

    // Timing
    UPROPERTY(BlueprintReadOnly, Category = "Timing")
    float LastUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Timing")
    float TimeOverBudget = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Timing")
    int32 OverBudgetCount = 0;

    // History tracking
    UPROPERTY(BlueprintReadOnly, Category = "History")
    TArray<float> UsageHistory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    int32 MaxHistorySize = 300; // 5 minutes at 60 FPS

    // Recovery settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
    float RecoveryThreshold = 0.7f; // Return to normal when usage drops below 70%

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
    float RecoveryTime = 3.0f; // Seconds to stay under threshold before recovery

    UPROPERTY(BlueprintReadOnly, Category = "Recovery")
    float TimeUnderRecoveryThreshold = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Recovery")
    bool bInRecoveryMode = false;
};

USTRUCT(BlueprintType)
struct FEnvironmentalBudgetAllocation
{
    GENERATED_BODY()

    // Frame time budgets (milliseconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Time Budgets")
    float WeatherEffectsBudgetMs = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Time Budgets")
    float PhysicsHazardsBudgetMs = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Time Budgets")
    float ParticleRenderingBudgetMs = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Time Budgets")
    float NetworkSyncBudgetMs = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Time Budgets")
    float AssetStreamingBudgetMs = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Time Budgets")
    float LODUpdateBudgetMs = 0.5f;

    // Memory budgets (MB)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float WeatherEffectsMemoryMB = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float PhysicsHazardsMemoryMB = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float ParticleSystemsMemoryMB = 192.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float NetworkBuffersMemoryMB = 32.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float AssetStreamingMemoryMB = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float AudioMemoryMB = 64.0f;

    // GPU budgets (percentage of total GPU time)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float ParticleRenderingGPUPercent = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float PostProcessingGPUPercent = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float LightingGPUPercent = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float ShadowsGPUPercent = 5.0f;

    // Network budgets (KBps)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Budgets")
    float HazardSyncBandwidthKBps = 64.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Budgets")
    float AssetStreamingBandwidthKBps = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Budgets")
    float AudioStreamingBandwidthKBps = 32.0f;

    // Physics budgets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Budgets")
    int32 MaxPhysicsObjects = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Budgets")
    int32 MaxConstraints = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Budgets")
    float PhysicsUpdateRateHz = 60.0f;
};

USTRUCT(BlueprintType)
struct FAdaptiveQualitySettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Settings")
    bool bEnableAdaptiveQuality = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Settings")
    float ResponseTime = 1.0f; // Seconds before responding to budget issues

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Settings")
    float AggressionLevel = 0.5f; // 0 = conservative, 1 = aggressive

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Settings")
    bool bAllowEmergencyMode = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Settings")
    float EmergencyModeThreshold = 0.95f; // Trigger emergency at 95% budget usage

    // Quality reduction steps
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Steps")
    TArray<float> QualityReductionSteps = {0.9f, 0.75f, 0.5f, 0.25f, 0.1f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Steps")
    TArray<float> DistanceReductionSteps = {0.9f, 0.8f, 0.6f, 0.4f, 0.2f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Steps")
    TArray<float> UpdateRateReductionSteps = {0.8f, 0.6f, 0.4f, 0.2f, 0.1f};

    // Recovery settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
    bool bEnableAutoRecovery = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
    float RecoveryDelay = 5.0f; // Seconds to wait before attempting recovery

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
    float RecoveryRate = 0.1f; // How fast to recover quality (per second)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
    float MinQualityLevel = 0.1f; // Never go below 10% quality
};

USTRUCT(BlueprintType)
struct FBudgetPerformanceStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalBudgets = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 OptimalBudgets = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 WarningBudgets = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CriticalBudgets = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EmergencyBudgets = 0;

    UPROPERTY(BlueprintReadOnly)
    float OverallBudgetUtilization = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageResponseTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 AdaptiveResponsesTriggered = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EmergencyModeActivations = 0;

    UPROPERTY(BlueprintReadOnly)
    float TimeInEmergencyMode = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float EfficiencyScore = 100.0f; // 0-100 scale

    UPROPERTY(BlueprintReadOnly)
    TMap<EPerformanceBudgetType, float> BudgetUtilizationByType;

    UPROPERTY(BlueprintReadOnly)
    TMap<EAdaptiveResponse, int32> ResponseCountByType;
};

UCLASS(BlueprintType, ClassGroup=(ClimbingGame), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalPerformanceBudgetSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalPerformanceBudgetSystem();

    // Component interface
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Budget management
    UFUNCTION(BlueprintCallable, Category = "Budget Management")
    void CreateBudget(const FPerformanceBudget& BudgetConfig);

    UFUNCTION(BlueprintCallable, Category = "Budget Management")
    void RemoveBudget(EPerformanceBudgetType BudgetType);

    UFUNCTION(BlueprintCallable, Category = "Budget Management")
    void UpdateBudgetUsage(EPerformanceBudgetType BudgetType, float Usage);

    UFUNCTION(BlueprintCallable, Category = "Budget Management")
    void SetBudgetLimit(EPerformanceBudgetType BudgetType, float NewLimit);

    UFUNCTION(BlueprintCallable, Category = "Budget Management")
    void ResetAllBudgets();

    // Budget monitoring
    UFUNCTION(BlueprintCallable, Category = "Budget Monitoring", BlueprintPure)
    FPerformanceBudget GetBudget(EPerformanceBudgetType BudgetType) const;

    UFUNCTION(BlueprintCallable, Category = "Budget Monitoring", BlueprintPure)
    EBudgetStatus GetBudgetStatus(EPerformanceBudgetType BudgetType) const;

    UFUNCTION(BlueprintCallable, Category = "Budget Monitoring", BlueprintPure)
    bool IsBudgetExceeded(EPerformanceBudgetType BudgetType) const;

    UFUNCTION(BlueprintCallable, Category = "Budget Monitoring", BlueprintPure)
    float GetBudgetUtilization(EPerformanceBudgetType BudgetType) const;

    UFUNCTION(BlueprintCallable, Category = "Budget Monitoring", BlueprintPure)
    TArray<EPerformanceBudgetType> GetOverBudgetTypes() const;

    // Adaptive quality management
    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void EnableAdaptiveQuality(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void TriggerAdaptiveResponse(EPerformanceBudgetType BudgetType, EBudgetStatus Status);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void SetAdaptiveAggressiveness(float AggressionLevel);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void ForceEmergencyMode(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    bool IsInEmergencyMode() const { return bEmergencyModeActive; }

    // Budget allocation
    UFUNCTION(BlueprintCallable, Category = "Budget Allocation")
    void SetBudgetAllocation(const FEnvironmentalBudgetAllocation& Allocation);

    UFUNCTION(BlueprintCallable, Category = "Budget Allocation")
    void ScaleBudgetAllocation(float ScaleFactor);

    UFUNCTION(BlueprintCallable, Category = "Budget Allocation")
    void RebalanceBudgets();

    UFUNCTION(BlueprintCallable, Category = "Budget Allocation")
    void PrioritizeCriticalBudgets();

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void OptimizePerformance();

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void ReduceQuality(float ReductionFactor);

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void CullNonCriticalEffects();

    UFUNCTION(BlueprintCallable, Category = "Performance Optimization")
    void RecoverQuality();

    // Timing and profiling
    UFUNCTION(BlueprintCallable, Category = "Profiling")
    void StartBudgetTimer(EPerformanceBudgetType BudgetType);

    UFUNCTION(BlueprintCallable, Category = "Profiling")
    void EndBudgetTimer(EPerformanceBudgetType BudgetType);

    UFUNCTION(BlueprintCallable, Category = "Profiling")
    void StartFrameTimeBudgetMeasurement();

    UFUNCTION(BlueprintCallable, Category = "Profiling")
    void EndFrameTimeBudgetMeasurement();

    // Performance statistics
    UFUNCTION(BlueprintCallable, Category = "Performance Stats", BlueprintPure)
    FBudgetPerformanceStats GetPerformanceStats() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Stats")
    void ResetPerformanceStats();

    UFUNCTION(BlueprintCallable, Category = "Performance Stats")
    float GetOverallEfficiency() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Stats")
    bool IsPerformanceOptimal() const;

    // Hardware tier adaptation
    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void AdaptToHardwareTier(EHardwareTier HardwareTier);

    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void UpdateBudgetsForPlayerCount(int32 PlayerCount);

    UFUNCTION(BlueprintCallable, Category = "Hardware Adaptation")
    void ScaleBudgetsForResolution(int32 ScreenWidth, int32 ScreenHeight);

    // Debug and profiling
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowBudgetDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogBudgetStats();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpBudgetHistory();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RunBudgetBenchmark(float Duration = 10.0f);

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budget Configuration")
    FEnvironmentalBudgetAllocation DefaultBudgetAllocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Quality")
    FAdaptiveQualitySettings AdaptiveSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableBudgetSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float BudgetUpdateInterval = 0.1f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bLogBudgetViolations = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableAutoOptimization = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Budget Events")
    FSimpleMulticastDelegate OnBudgetExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Budget Events")
    FSimpleMulticastDelegate OnEmergencyModeActivated;

    UPROPERTY(BlueprintAssignable, Category = "Budget Events")
    FSimpleMulticastDelegate OnQualityReduced;

    UPROPERTY(BlueprintAssignable, Category = "Budget Events")
    FSimpleMulticastDelegate OnPerformanceOptimized;

    UPROPERTY(BlueprintAssignable, Category = "Budget Events")
    FSimpleMulticastDelegate OnBudgetRecovered;

protected:
    // Budget storage
    UPROPERTY()
    TMap<EPerformanceBudgetType, FPerformanceBudget> ActiveBudgets;

    // Performance tracking
    FBudgetPerformanceStats CurrentStats;
    float LastBudgetUpdate = 0.0f;
    float LastOptimization = 0.0f;

    // Adaptive quality state
    bool bAdaptiveQualityEnabled = true;
    bool bEmergencyModeActive = false;
    float EmergencyModeStartTime = 0.0f;
    float CurrentQualityLevel = 1.0f;
    int32 CurrentAdaptiveStep = 0;

    // Timing state
    TMap<EPerformanceBudgetType, double> BudgetTimers;
    double FrameTimeBudgetStart = 0.0;

    // Recovery state
    TMap<EPerformanceBudgetType, float> RecoveryTimers;
    float LastRecoveryAttempt = 0.0f;

    // Debug state
    bool bShowDebugInfo = false;
    bool bIsRunningBenchmark = false;
    float BenchmarkStartTime = 0.0f;

    // Integration references
    UPROPERTY()
    UClimbingPerformanceManager* PerformanceManager = nullptr;

    UPROPERTY()
    UClimbingFrameRateManager* FrameRateManager = nullptr;

    UPROPERTY()
    UClimbingEnvironmentalHazardManager* HazardManager = nullptr;

private:
    // Core update functions
    void UpdateBudgetSystem(float DeltaTime);
    void UpdateAdaptiveQuality(float DeltaTime);
    void UpdateBudgetStates();
    void UpdatePerformanceStats(float DeltaTime);
    
    // Budget management implementation
    void InitializeDefaultBudgets();
    void CreateBudgetInternal(EPerformanceBudgetType BudgetType, const FPerformanceBudget& Config);
    void UpdateBudgetState(FPerformanceBudget& Budget);
    EBudgetStatus CalculateBudgetStatus(const FPerformanceBudget& Budget) const;
    
    // Adaptive response implementation
    void ExecuteAdaptiveResponse(EPerformanceBudgetType BudgetType, EAdaptiveResponse Response);
    void ActivateEmergencyMode();
    void DeactivateEmergencyMode();
    void PerformQualityReduction(float ReductionFactor);
    void PerformEffectCulling();
    void PerformDistanceReduction(float ReductionFactor);
    void PerformUpdateRateReduction(float ReductionFactor);
    
    // Recovery implementation
    void AttemptBudgetRecovery();
    void RecoverQualityStep();
    bool CanRecoverQuality() const;
    void ResetAdaptiveState();
    
    // Budget allocation implementation
    void ApplyBudgetAllocation(const FEnvironmentalBudgetAllocation& Allocation);
    void ScaleBudgets(float ScaleFactor);
    void RebalanceBudgetWeights();
    void PrioritizeByBudgetPriority();
    
    // Hardware adaptation implementation
    FEnvironmentalBudgetAllocation GetBudgetAllocationForHardwareTier(EHardwareTier Tier) const;
    float GetPlayerCountScalingFactor(int32 PlayerCount) const;
    float GetResolutionScalingFactor(int32 Width, int32 Height) const;
    
    // Performance optimization implementation
    void OptimizeFrameTimeBudgets();
    void OptimizeMemoryBudgets();
    void OptimizeGPUBudgets();
    void OptimizeNetworkBudgets();
    
    // Utility functions
    float CalculateOverallUtilization() const;
    float CalculateEfficiencyScore() const;
    bool ShouldTriggerAdaptiveResponse(const FPerformanceBudget& Budget) const;
    float GetAdaptiveResponseDelay(EBudgetStatus Status) const;
    
    // Integration helpers
    void InitializeIntegrationReferences();
    void UpdateIntegratedSystems();
    void NotifySystemsOfBudgetChanges();
    
    // Debug helpers
    void DrawBudgetDebugInfo();
    void DrawBudgetUtilizationBars();
    void LogBudgetState(const FPerformanceBudget& Budget) const;
    
    // Benchmark helpers
    void UpdateBenchmark(float DeltaTime);
    void CompleteBenchmark();
};