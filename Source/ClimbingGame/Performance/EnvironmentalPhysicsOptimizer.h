#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Chaos/ChaosEngineInterface.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "EnvironmentalPhysicsOptimizer.generated.h"

UENUM(BlueprintType)
enum class EPhysicsSimulationLevel : uint8
{
    Full            UMETA(DisplayName = "Full Simulation"),
    Reduced         UMETA(DisplayName = "Reduced Simulation"),
    Essential       UMETA(DisplayName = "Essential Only"),
    Kinematic       UMETA(DisplayName = "Kinematic Only"),
    Static          UMETA(DisplayName = "Static"),
    Disabled        UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class ECollisionOptimization : uint8
{
    Complex         UMETA(DisplayName = "Complex Collision"),
    Simple          UMETA(DisplayName = "Simple Collision"),
    Bounds          UMETA(DisplayName = "Bounds Only"),
    None            UMETA(DisplayName = "No Collision")
};

UENUM(BlueprintType)
enum class EPhysicsUpdateMode : uint8
{
    EveryFrame      UMETA(DisplayName = "Every Frame"),
    FixedInterval   UMETA(DisplayName = "Fixed Interval"),
    Adaptive        UMETA(DisplayName = "Adaptive Rate"),
    OnDemand        UMETA(DisplayName = "On Demand")
};

USTRUCT(BlueprintType)
struct FPhysicsLODLevel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    EPhysicsSimulationLevel SimulationLevel = EPhysicsSimulationLevel::Full;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    ECollisionOptimization CollisionType = ECollisionOptimization::Complex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    int32 MaxObjects = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float UpdateRate = 60.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    int32 SolverIterations = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float LinearDamping = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float AngularDamping = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    bool bEnableReplication = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    bool bEnableContactEvents = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float SleepThreshold = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float CullingDistance = 15000.0f;
};

USTRUCT(BlueprintType)
struct FPhysicsObjectPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
    TSubclassOf<AActor> ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
    UStaticMesh* StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
    int32 PoolSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
    int32 MaxActiveObjects = 50;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool")
    TArray<TWeakObjectPtr<AActor>> AvailableObjects;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool")
    TArray<TWeakObjectPtr<AActor>> ActiveObjects;

    UPROPERTY(BlueprintReadOnly, Category = "Object Pool")
    float MemoryUsageMB = 0.0f;
};

USTRUCT(BlueprintType)
struct FRockSlideSettings
{
    GENERATED_BODY()

    // Rock generation parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Generation")
    int32 MinRocks = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Generation")
    int32 MaxRocks = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Generation")
    FVector2D RockSizeRange = FVector2D(0.2f, 2.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Generation")
    float SpawnRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Generation")
    float SpawnHeight = 500.0f;

    // Physics parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float InitialVelocityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float BounceDamping = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float FrictionMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float LifetimeSeconds = 60.0f;

    // Performance optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bUseInstancedMeshes = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableSpatialPartitioning = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxSimulationDistance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxActiveRocksPerLOD = 200;
};

USTRUCT(BlueprintType)
struct FAvalancheSettings
{
    GENERATED_BODY()

    // Snow/debris generation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Generation")
    int32 MinDebrisObjects = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Generation")
    int32 MaxDebrisObjects = 2000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Generation")
    float DebrisSpawnArea = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Generation")
    float FlowSpeed = 1000.0f; // cm/s

    // Fluid simulation parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Simulation")
    bool bEnableFluidSimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Simulation")
    float FluidDensity = 1000.0f; // kg/m³

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Simulation")
    float Viscosity = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Simulation")
    int32 FluidParticles = 10000;

    // Performance settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bUseLevelOfDetailForFluid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxFluidSimulationDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxActiveFluidParticles = 5000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableGPUSimulation = true;
};

USTRUCT(BlueprintType)
struct FPhysicsPerformanceStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalPhysicsObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 SimulatingObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 SleepingObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 KinematicObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 StaticObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    float PhysicsUpdateTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CollisionDetectionTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ConstraintSolvingTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 CollisionPairs = 0;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageObjectDensity = 0.0f; // objects per cubic meter

    UPROPERTY(BlueprintReadOnly)
    int32 CulledObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    float OptimizationEfficiencyPercent = 0.0f;
};

UCLASS(BlueprintType, ClassGroup=(ClimbingGame), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalPhysicsOptimizer : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalPhysicsOptimizer();

    // Component interface
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Rock slide optimization
    UFUNCTION(BlueprintCallable, Category = "Rock Slide")
    void CreateRockSlide(const FVector& Location, const FVector& Direction, float Intensity);

    UFUNCTION(BlueprintCallable, Category = "Rock Slide")
    void OptimizeRockSlidePhysics();

    UFUNCTION(BlueprintCallable, Category = "Rock Slide")
    void SetRockSlideQuality(EPhysicsSimulationLevel Quality);

    // Avalanche optimization
    UFUNCTION(BlueprintCallable, Category = "Avalanche")
    void CreateAvalanche(const FVector& StartLocation, const FVector& FlowDirection, float Intensity);

    UFUNCTION(BlueprintCallable, Category = "Avalanche")
    void OptimizeAvalanchePhysics();

    UFUNCTION(BlueprintCallable, Category = "Avalanche")
    void SetAvalancheQuality(EPhysicsSimulationLevel Quality);

    // General physics optimization
    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void SetPhysicsLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void EnableSpatialOptimization(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void SetUpdateMode(EPhysicsUpdateMode UpdateMode);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizeCollisionDetection();

    // Object pooling
    UFUNCTION(BlueprintCallable, Category = "Object Pooling")
    void InitializeObjectPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pooling")
    AActor* GetPooledObject(const FString& PoolName);

    UFUNCTION(BlueprintCallable, Category = "Object Pooling")
    void ReturnObjectToPool(AActor* Object, const FString& PoolName);

    UFUNCTION(BlueprintCallable, Category = "Object Pooling")
    void CleanupObjectPools();

    // Spatial partitioning
    UFUNCTION(BlueprintCallable, Category = "Spatial Optimization")
    void UpdateSpatialPartitioning();

    UFUNCTION(BlueprintCallable, Category = "Spatial Optimization")
    TArray<AActor*> GetObjectsInRadius(const FVector& Location, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Spatial Optimization")
    void CullDistantObjects(const FVector& ViewerLocation, float CullingDistance);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance", BlueprintPure)
    FPhysicsPerformanceStats GetPerformanceStats() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void ResetPerformanceStats();

    UFUNCTION(BlueprintCallable, Category = "Performance")
    float GetPhysicsFrameTime() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void EnablePerformanceProfiling(bool bEnable);

    // Adaptive optimization
    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void EnableAdaptivePhysics(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void SetPerformanceTarget(float TargetFrameTimeMs);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Optimization")
    void ForceOptimization();

    // Debug and visualization
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowPhysicsDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogPhysicsStats();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpObjectStates();

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD Settings")
    TArray<FPhysicsLODLevel> PhysicsLODLevels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Slide Settings")
    FRockSlideSettings RockSlideSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Settings")
    FAvalancheSettings AvalancheSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pools")
    TMap<FString, FPhysicsObjectPool> ObjectPools;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableAdaptivePhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float TargetPhysicsFrameTimeMs = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableSpatialOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float SpatialGridSize = 1000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    int32 MaxObjectsPerGrid = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableMultithreading = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Physics Events")
    FSimpleMulticastDelegate OnPhysicsOptimized;

    UPROPERTY(BlueprintAssignable, Category = "Physics Events")
    FSimpleMulticastDelegate OnPerformanceThresholdExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Physics Events")
    FSimpleMulticastDelegate OnObjectPoolExhausted;

protected:
    // Internal state
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ManagedPhysicsObjects;

    UPROPERTY()
    TMap<FIntVector, TArray<TWeakObjectPtr<AActor>>> SpatialGrid;

    FPhysicsPerformanceStats CurrentStats;
    
    int32 CurrentLODLevel = 1;
    EPhysicsUpdateMode CurrentUpdateMode = EPhysicsUpdateMode::EveryFrame;
    
    // Timing
    float PhysicsUpdateTimer = 0.0f;
    float AdaptiveOptimizationTimer = 0.0f;
    float SpatialUpdateTimer = 0.0f;
    
    // Performance tracking
    double LastPhysicsUpdateTime = 0.0;
    bool bPerformanceProfilingEnabled = false;
    TArray<float> FrameTimeHistory;
    
    // Optimization state
    bool bOptimizationActive = false;
    float LastOptimizationTime = 0.0f;

    // Integration
    UPROPERTY()
    UClimbingPerformanceManager* PerformanceManager = nullptr;

private:
    // Core physics optimization functions
    void UpdatePhysicsObjects(float DeltaTime);
    void UpdateAdaptivePhysics(float DeltaTime);
    void UpdateSpatialGrid(float DeltaTime);
    
    // LOD management
    void ApplyPhysicsLOD(AActor* Actor, int32 LODLevel);
    int32 CalculateObjectLOD(AActor* Actor) const;
    void TransitionObjectLOD(AActor* Actor, int32 FromLOD, int32 ToLOD);
    
    // Object management
    void RegisterPhysicsObject(AActor* Actor);
    void UnregisterPhysicsObject(AActor* Actor);
    void CleanupInvalidObjects();
    
    // Spatial optimization
    FIntVector WorldLocationToGridCoord(const FVector& WorldLocation) const;
    void AddObjectToSpatialGrid(AActor* Actor);
    void RemoveObjectFromSpatialGrid(AActor* Actor);
    void UpdateObjectSpatialPosition(AActor* Actor);
    
    // Rock slide implementation
    void SpawnRockSlideObjects(const FVector& Location, const FVector& Direction, float Intensity);
    void UpdateRockSlidePhysics(float DeltaTime);
    void OptimizeRockCollisions();
    
    // Avalanche implementation
    void SpawnAvalancheObjects(const FVector& StartLocation, const FVector& FlowDirection, float Intensity);
    void UpdateAvalanchePhysics(float DeltaTime);
    void OptimizeAvalancheSimulation();
    
    // Performance optimization
    void PerformPhysicsOptimization();
    void ReducePhysicsComplexity();
    void ConvertToKinematic(AActor* Actor);
    void ConvertToStatic(AActor* Actor);
    void EnablePhysicsSimulation(AActor* Actor, bool bEnable);
    
    // Memory management
    void OptimizePhysicsMemory();
    float CalculateObjectMemoryUsage(AActor* Actor) const;
    void PoolInactiveObjects();
    
    // Collision optimization
    void OptimizeCollisionMeshes();
    void SimplifyCollisionGeometry(AActor* Actor);
    void DisableUnnecessaryCollisions(AActor* Actor);
    
    // Threading helpers
    void DispatchPhysicsUpdates();
    void ProcessPhysicsResultsOnGameThread();
    
    // Utility functions
    bool IsObjectVisible(AActor* Actor) const;
    bool ShouldObjectSimulatePhysics(AActor* Actor) const;
    float GetDistanceToNearestPlayer(const FVector& Location) const;
    void UpdatePerformanceStats(float DeltaTime);
    
    // Debug helpers
    void DrawSpatialGridDebug();
    void DrawPhysicsObjectDebug(AActor* Actor);
    void LogObjectState(AActor* Actor) const;
};