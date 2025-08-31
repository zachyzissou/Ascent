#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "AdvancedRopeComponent.h"
#include "../Tools/ClimbingToolBase.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "ClimbingPerformanceManager.generated.h"

UENUM(BlueprintType)
enum class EPerformanceLOD : uint8
{
    Ultra      UMETA(DisplayName = "Ultra Quality"),
    High       UMETA(DisplayName = "High Quality"),
    Medium     UMETA(DisplayName = "Medium Quality"),
    Low        UMETA(DisplayName = "Low Quality"),
    Minimal    UMETA(DisplayName = "Minimal Quality"),
    Disabled   UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class EPhysicsQuality : uint8
{
    Full       UMETA(DisplayName = "Full Physics"),
    Reduced    UMETA(DisplayName = "Reduced Physics"),
    Essential  UMETA(DisplayName = "Essential Only"),
    Disabled   UMETA(DisplayName = "Disabled")
};

USTRUCT(BlueprintType)
struct FPerformanceMetrics
{
    GENERATED_BODY()

    // Frame timing
    UPROPERTY(BlueprintReadOnly)
    float CurrentFPS = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageFrameTime = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly)
    float PhysicsTime = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly)
    float RenderTime = 0.0f; // milliseconds

    // Physics metrics
    UPROPERTY(BlueprintReadOnly)
    int32 ActiveRopes = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveTools = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveClimbers = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PhysicsConstraints = 0;

    // Memory metrics
    UPROPERTY(BlueprintReadOnly)
    float MemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 DrawCalls = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Triangles = 0;

    // Network metrics
    UPROPERTY(BlueprintReadOnly)
    float NetworkBandwidthKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float NetworkLatencyMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FLODDistanceSettings
{
    GENERATED_BODY()

    // Distance thresholds for different LOD levels
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float UltraDistance = 500.0f;    // 5 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float HighDistance = 1500.0f;    // 15 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float MediumDistance = 3000.0f;  // 30 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LowDistance = 5000.0f;     // 50 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float MinimalDistance = 8000.0f; // 80 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float DisabledDistance = 12000.0f; // 120 meters
};

USTRUCT(BlueprintType)
struct FPhysicsOptimizationSettings
{
    GENERATED_BODY()

    // Physics simulation settings per LOD
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Settings")
    int32 UltraRopeSegments = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Settings")
    int32 HighRopeSegments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Settings")
    int32 MediumRopeSegments = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Settings")
    int32 LowRopeSegments = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Settings")
    int32 MinimalRopeSegments = 4;

    // Update rates per LOD (Hz)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float UltraUpdateRate = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float HighUpdateRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float MediumUpdateRate = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float LowUpdateRate = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Rates")
    float MinimalUpdateRate = 5.0f;

    // Physics solver settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solver Settings")
    int32 UltraSolverIterations = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solver Settings")
    int32 HighSolverIterations = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solver Settings")
    int32 MediumSolverIterations = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solver Settings")
    int32 LowSolverIterations = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solver Settings")
    int32 MinimalSolverIterations = 1;
};

USTRUCT(BlueprintType)
struct FPerformanceTargets
{
    GENERATED_BODY()

    // Target performance metrics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    float TargetFPS = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    float MinimumFPS = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    float MaxFrameTimeMs = 16.67f; // 60 FPS

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    float MaxPhysicsTimeMs = 5.0f;

    // Resource limits
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Limits")
    int32 MaxActiveRopes = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Limits")
    int32 MaxPhysicsConstraints = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Limits")
    float MaxMemoryUsageMB = 2048.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Limits")
    int32 MaxDrawCalls = 3000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Limits")
    float MaxNetworkBandwidthKBps = 256.0f;
};

UCLASS()
class CLIMBINGGAME_API UClimbingPerformanceManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Main update function
    UFUNCTION(CallInEditor = true)
    void TickPerformanceManager(float DeltaTime);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    FPerformanceMetrics GetCurrentMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void StartFrameProfiler();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void EndFrameProfiler();

    // LOD management
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    EPerformanceLOD GetObjectLOD(const AActor* Actor, const FVector& ViewerLocation) const;

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UpdateObjectLOD(AActor* Actor, EPerformanceLOD NewLOD);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UpdateAllObjectLODs();

    // Physics optimization
    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizeRopePhysics(UAdvancedRopeComponent* Rope, EPerformanceLOD LOD);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizeClimberPhysics(UAdvancedClimbingComponent* Climber, EPerformanceLOD LOD);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizeToolPhysics(AClimbingToolBase* Tool, EPerformanceLOD LOD);

    // Batch processing
    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void BatchUpdateRopes(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void BatchUpdateTools(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void BatchUpdateClimbers(float DeltaTime);

    // Adaptive quality
    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void AdjustQualityForPerformance();

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void SetGlobalLODBias(float LODBias);

    // Memory management
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void RunGarbageCollection();

    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void OptimizeMemoryUsage();

    // Object registration
    UFUNCTION(BlueprintCallable, Category = "Object Management")
    void RegisterRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Object Management")
    void UnregisterRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Object Management")
    void RegisterTool(AClimbingToolBase* Tool);

    UFUNCTION(BlueprintCallable, Category = "Object Management")
    void UnregisterTool(AClimbingToolBase* Tool);

    UFUNCTION(BlueprintCallable, Category = "Object Management")
    void RegisterClimber(UAdvancedClimbingComponent* Climber);

    UFUNCTION(BlueprintCallable, Category = "Object Management")
    void UnregisterClimber(UAdvancedClimbingComponent* Climber);

    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FLODDistanceSettings LODDistances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FPhysicsOptimizationSettings PhysicsSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FPerformanceTargets PerformanceTargets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableAdaptiveQuality = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableLODOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableBatchProcessing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float AdaptiveQualityCheckInterval = 1.0f; // seconds

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnPerformanceWarning;

    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnPerformanceCritical;

    UPROPERTY(BlueprintAssignable, Category = "Performance Events")
    FSimpleMulticastDelegate OnQualityAdjusted;

protected:
    // Internal tracking
    UPROPERTY()
    TArray<UAdvancedRopeComponent*> TrackedRopes;

    UPROPERTY()
    TArray<AClimbingToolBase*> TrackedTools;

    UPROPERTY()
    TArray<UAdvancedClimbingComponent*> TrackedClimbers;

    // Performance data
    FPerformanceMetrics CurrentMetrics;
    TArray<float> FrameTimeHistory;
    float LastAdaptiveQualityCheck = 0.0f;

    // LOD caching
    TMap<AActor*, EPerformanceLOD> ActorLODCache;
    TMap<AActor*, float> ActorDistanceCache;
    float LODUpdateInterval = 0.2f; // Update LODs every 200ms
    float LastLODUpdate = 0.0f;

    // Batch processing
    int32 RopeBatchIndex = 0;
    int32 ToolBatchIndex = 0;
    int32 ClimberBatchIndex = 0;
    int32 BatchSize = 10; // Objects per batch

    // Physics optimization
    float PhysicsTimeAccumulator = 0.0f;
    int32 PhysicsFrameCounter = 0;

private:
    // Internal functions
    void UpdatePerformanceMetrics(float DeltaTime);
    void UpdateLODSystem(float DeltaTime);
    void CheckPerformanceThresholds();
    EPerformanceLOD CalculateLODFromDistance(float Distance) const;
    FVector GetAverageViewerLocation() const;
    void ApplyRopeLOD(UAdvancedRopeComponent* Rope, EPerformanceLOD LOD);
    void ApplyToolLOD(AClimbingToolBase* Tool, EPerformanceLOD LOD);
    void ApplyClimberLOD(UAdvancedClimbingComponent* Climber, EPerformanceLOD LOD);

    // Memory tracking
    void TrackMemoryUsage();
    void OptimizeTextureMemory();
    void OptimizeMeshMemory();

    // Performance analytics
    void RecordPerformanceData();
    void AnalyzePerformanceTrends();

    // Threading helpers
    void DispatchPhysicsUpdates();
    void SynchronizePhysicsResults();

    // World reference
    UWorld* CachedWorld = nullptr;

    // Frame timing
    double FrameStartTime = 0.0;
    double PhysicsStartTime = 0.0;
    double PhysicsEndTime = 0.0;
};