#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "../Memory/ClimbingMemoryTracker.h"
#include "ClimbingFrameRateManager.h"
#include "ClimbingEnvironmentalHazardManager.generated.h"

UENUM(BlueprintType)
enum class EEnvironmentalHazardType : uint8
{
    Rain            UMETA(DisplayName = "Rain"),
    Snow            UMETA(DisplayName = "Snow"),
    DustStorm       UMETA(DisplayName = "Dust Storm"),
    Fog             UMETA(DisplayName = "Fog"),
    RockSlide       UMETA(DisplayName = "Rock Slide"),
    Avalanche       UMETA(DisplayName = "Avalanche"),
    WindGusts       UMETA(DisplayName = "Wind Gusts"),
    Lightning       UMETA(DisplayName = "Lightning"),
    Hail            UMETA(DisplayName = "Hail"),
    Sandstorm       UMETA(DisplayName = "Sandstorm")
};

UENUM(BlueprintType)
enum class EHazardIntensity : uint8
{
    None            UMETA(DisplayName = "None"),
    Light           UMETA(DisplayName = "Light"),
    Moderate        UMETA(DisplayName = "Moderate"),
    Heavy           UMETA(DisplayName = "Heavy"),
    Extreme         UMETA(DisplayName = "Extreme")
};

UENUM(BlueprintType)
enum class EHazardQuality : uint8
{
    Cinematic       UMETA(DisplayName = "Cinematic Quality"),
    High            UMETA(DisplayName = "High Quality"),
    Medium          UMETA(DisplayName = "Medium Quality"),
    Low             UMETA(DisplayName = "Low Quality"),
    Performance     UMETA(DisplayName = "Performance Mode"),
    Disabled        UMETA(DisplayName = "Disabled")
};

USTRUCT(BlueprintType)
struct FWeatherParticleSettings
{
    GENERATED_BODY()

    // Particle count scaling per quality level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Counts")
    int32 CinematicParticleCount = 5000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Counts")
    int32 HighParticleCount = 2500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Counts")
    int32 MediumParticleCount = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Counts")
    int32 LowParticleCount = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Counts")
    int32 PerformanceParticleCount = 200;

    // Rendering settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableLighting = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableShadows = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    float ParticleSize = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    float ViewDistance = 5000.0f;

    // Performance optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float UpdateRate = 60.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableOcclusion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float CullingDistance = 8000.0f;
};

USTRUCT(BlueprintType)
struct FPhysicsHazardSettings
{
    GENERATED_BODY()

    // Physics simulation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Quality")
    int32 MaxSimulatedObjects = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Quality")
    float PhysicsUpdateRate = 30.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Quality")
    int32 SolverIterations = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Quality")
    bool bEnableComplexCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Quality")
    bool bEnableInterObjectCollision = true;

    // LOD settings for physics objects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float HighDetailDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float MediumDetailDistance = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float LowDetailDistance = 6000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics LOD")
    float CullingDistance = 10000.0f;

    // Memory management
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    float MaxMemoryUsageMB = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    bool bEnablePooling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    int32 ObjectPoolSize = 500;
};

USTRUCT(BlueprintType)
struct FHazardLODSettings
{
    GENERATED_BODY()

    // Distance-based LOD thresholds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float CinematicDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float HighDistance = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float MediumDistance = 7000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LowDistance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float CullingDistance = 15000.0f;

    // Quality scaling factors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    float ParticleCountScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    float PhysicsObjectScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    float EffectIntensityScale = 1.0f;

    // Performance-based scaling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Scaling")
    bool bEnableAdaptiveScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Scaling")
    float TargetFPS = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Scaling")
    float MinimumFPS = 30.0f;
};

USTRUCT(BlueprintType)
struct FNetworkHazardSettings
{
    GENERATED_BODY()

    // Network synchronization settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Sync")
    float SyncUpdateRate = 10.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Sync")
    bool bEnableCompression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Sync")
    float RelevancyDistance = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Sync")
    bool bSyncParticleEffects = false; // Usually client-side only

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Sync")
    bool bSyncPhysicsObjects = true;

    // Bandwidth optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    float MaxBandwidthKBps = 64.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    bool bEnablePrediction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    bool bEnableDeltaCompression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandwidth")
    float PredictionTolerance = 100.0f; // cm
};

USTRUCT(BlueprintType)
struct FHazardPerformanceBudget
{
    GENERATED_BODY()

    // Frame time budgets (milliseconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Budgets")
    float WeatherEffectsFrameTimeBudgetMs = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Budgets")
    float PhysicsHazardsFrameTimeBudgetMs = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Budgets")
    float NetworkSyncFrameTimeBudgetMs = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Budgets")
    float TotalFrameTimeBudgetMs = 6.0f;

    // Memory budgets (MB)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float WeatherEffectsMemoryBudgetMB = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float PhysicsHazardsMemoryBudgetMB = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float NetworkBuffersMemoryBudgetMB = 32.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Budgets")
    float TotalMemoryBudgetMB = 416.0f;

    // GPU budgets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float ParticleRenderingGPUBudgetPercent = 15.0f; // % of total GPU time

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float EffectsRenderingGPUBudgetPercent = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPU Budgets")
    float PhysicsGPUBudgetPercent = 5.0f;
};

USTRUCT(BlueprintType)
struct FActiveHazardInstance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EEnvironmentalHazardType HazardType;

    UPROPERTY(BlueprintReadOnly)
    EHazardIntensity CurrentIntensity;

    UPROPERTY(BlueprintReadOnly)
    EHazardQuality CurrentQuality;

    UPROPERTY(BlueprintReadOnly)
    FVector Location;

    UPROPERTY(BlueprintReadOnly)
    float Radius;

    UPROPERTY(BlueprintReadOnly)
    float Duration;

    UPROPERTY(BlueprintReadOnly)
    float ElapsedTime;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<UNiagaraComponent> ParticleComponent;

    UPROPERTY(BlueprintReadOnly)
    TArray<TWeakObjectPtr<AActor>> PhysicsActors;

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime;

    UPROPERTY(BlueprintReadOnly)
    float CurrentFrameTime;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUsage;

    UPROPERTY(BlueprintReadOnly)
    bool bNetworkSynced;

    UPROPERTY(BlueprintReadOnly)
    int32 NetworkID;

    UPROPERTY(BlueprintReadOnly)
    bool bIsVisible;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToNearestPlayer;
};

USTRUCT(BlueprintType)
struct FHazardPerformanceMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float TotalFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float WeatherEffectsFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PhysicsHazardsFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float NetworkSyncFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float TotalMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float GPUUsagePercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveHazardCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalParticleCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActivePhysicsObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    float NetworkBandwidthUsageKBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bBudgetExceeded = false;

    UPROPERTY(BlueprintReadOnly)
    float PerformanceScore = 100.0f; // 0-100 scale
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingEnvironmentalHazardManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // Hazard creation and management
    UFUNCTION(BlueprintCallable, Category = "Hazard Management")
    int32 CreateHazard(EEnvironmentalHazardType HazardType, const FVector& Location, float Radius, float Duration, EHazardIntensity Intensity = EHazardIntensity::Moderate);

    UFUNCTION(BlueprintCallable, Category = "Hazard Management")
    void DestroyHazard(int32 HazardID);

    UFUNCTION(BlueprintCallable, Category = "Hazard Management")
    void DestroyAllHazards();

    UFUNCTION(BlueprintCallable, Category = "Hazard Management")
    void UpdateHazardIntensity(int32 HazardID, EHazardIntensity NewIntensity);

    UFUNCTION(BlueprintCallable, Category = "Hazard Management")
    void UpdateHazardLocation(int32 HazardID, const FVector& NewLocation);

    // Weather effects optimization
    UFUNCTION(BlueprintCallable, Category = "Weather Optimization")
    void OptimizeWeatherEffects();

    UFUNCTION(BlueprintCallable, Category = "Weather Optimization")
    void SetWeatherQuality(EHazardQuality Quality);

    UFUNCTION(BlueprintCallable, Category = "Weather Optimization")
    void EnableWeatherLOD(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Weather Optimization")
    void SetWeatherViewDistance(float Distance);

    // Physics hazard optimization
    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void OptimizePhysicsHazards();

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void SetPhysicsHazardQuality(EHazardQuality Quality);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void EnablePhysicsLOD(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Physics Optimization")
    void SetMaxPhysicsObjects(int32 MaxObjects);

    // Adaptive quality system
    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void EnableAdaptiveQuality(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void SetPerformanceTarget(float TargetFPS, float MinimumFPS);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void ForceQualityAdjustment(bool bIncreaseQuality);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    EHazardQuality GetAdaptiveQualityLevel() const { return CurrentAdaptiveQuality; }

    // Memory management
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void OptimizeMemoryUsage();

    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void SetMemoryBudget(float MemoryBudgetMB);

    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void EnableMemoryPooling(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    float GetCurrentMemoryUsage() const;

    // Network optimization
    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeNetworkSync();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void SetNetworkSyncRate(float SyncRateHz);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void EnableNetworkCompression(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void SetRelevancyDistance(float Distance);

    // LOD system integration
    UFUNCTION(BlueprintCallable, Category = "LOD System")
    void UpdateHazardLODs();

    UFUNCTION(BlueprintCallable, Category = "LOD System")
    EHazardQuality GetHazardLOD(int32 HazardID) const;

    UFUNCTION(BlueprintCallable, Category = "LOD System")
    void SetGlobalHazardLODBias(float LODBias);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    FHazardPerformanceMetrics GetPerformanceMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsBudgetExceeded() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetPerformanceScore() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void ResetPerformanceMetrics();

    // Query functions
    UFUNCTION(BlueprintCallable, Category = "Hazard Query", BlueprintPure)
    TArray<int32> GetActiveHazards() const;

    UFUNCTION(BlueprintCallable, Category = "Hazard Query", BlueprintPure)
    FActiveHazardInstance GetHazardInfo(int32 HazardID) const;

    UFUNCTION(BlueprintCallable, Category = "Hazard Query", BlueprintPure)
    TArray<int32> GetHazardsInRadius(const FVector& Location, float Radius) const;

    UFUNCTION(BlueprintCallable, Category = "Hazard Query", BlueprintPure)
    EEnvironmentalHazardType GetDominantHazardAtLocation(const FVector& Location) const;

    // Debug and profiling
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowHazardDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogPerformanceStats();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpActiveHazards();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RunPerformanceBenchmark(float Duration = 10.0f);

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FWeatherParticleSettings DefaultWeatherSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FPhysicsHazardSettings DefaultPhysicsSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FHazardLODSettings DefaultLODSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FNetworkHazardSettings DefaultNetworkSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FHazardPerformanceBudget PerformanceBudgets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableAdaptiveQuality = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableMemoryOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableNetworkOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float PerformanceCheckInterval = 1.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxActiveHazards = 20;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Hazard Events")
    FSimpleMulticastDelegate OnHazardCreated;

    UPROPERTY(BlueprintAssignable, Category = "Hazard Events")
    FSimpleMulticastDelegate OnHazardDestroyed;

    UPROPERTY(BlueprintAssignable, Category = "Hazard Events")
    FSimpleMulticastDelegate OnQualityAdjusted;

    UPROPERTY(BlueprintAssignable, Category = "Hazard Events")
    FSimpleMulticastDelegate OnBudgetExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Hazard Events")
    FSimpleMulticastDelegate OnPerformanceWarning;

protected:
    // Active hazard tracking
    UPROPERTY()
    TMap<int32, FActiveHazardInstance> ActiveHazards;

    int32 NextHazardID = 1;

    // Performance tracking
    FHazardPerformanceMetrics CurrentMetrics;
    float LastPerformanceCheck = 0.0f;

    // Quality state
    EHazardQuality CurrentGlobalQuality = EHazardQuality::Medium;
    EHazardQuality CurrentAdaptiveQuality = EHazardQuality::Medium;
    float AdaptiveQualityTimer = 0.0f;

    // Memory management
    TMap<EEnvironmentalHazardType, TArray<UObject*>> ObjectPools;
    float CurrentMemoryUsage = 0.0f;

    // Network state
    TMap<int32, float> NetworkHazardStates;
    float LastNetworkSync = 0.0f;

    // Performance optimization state
    bool bPerformanceOptimizationActive = false;
    float PerformanceOptimizationTimer = 0.0f;

    // LOD state
    TMap<int32, EHazardQuality> HazardLODOverrides;
    float GlobalLODBias = 1.0f;

    // Debug state
    bool bShowDebugInfo = false;
    bool bIsRunningBenchmark = false;
    float BenchmarkStartTime = 0.0f;
    float BenchmarkDuration = 0.0f;

    // Integration references
    UPROPERTY()
    UClimbingPerformanceManager* PerformanceManager = nullptr;

    UPROPERTY()
    UClimbingMemoryTracker* MemoryTracker = nullptr;

    UPROPERTY()
    UClimbingFrameRateManager* FrameRateManager = nullptr;

private:
    // Core update functions
    void UpdateActiveHazards(float DeltaTime);
    void UpdatePerformanceMetrics(float DeltaTime);
    void UpdateAdaptiveQuality(float DeltaTime);
    void UpdateNetworkSync(float DeltaTime);

    // Hazard management helpers
    UNiagaraComponent* CreateWeatherEffect(EEnvironmentalHazardType HazardType, const FVector& Location, EHazardIntensity Intensity);
    TArray<AActor*> CreatePhysicsObjects(EEnvironmentalHazardType HazardType, const FVector& Location, float Radius, EHazardIntensity Intensity);
    void DestroyHazardComponents(FActiveHazardInstance& Hazard);

    // Quality adjustment functions
    void AdjustWeatherQuality(FActiveHazardInstance& Hazard, EHazardQuality NewQuality);
    void AdjustPhysicsQuality(FActiveHazardInstance& Hazard, EHazardQuality NewQuality);
    void CalculateOptimalQuality();

    // LOD calculation and application
    EHazardQuality CalculateHazardLOD(const FActiveHazardInstance& Hazard) const;
    void ApplyHazardLOD(FActiveHazardInstance& Hazard, EHazardQuality NewQuality);
    float GetDistanceToNearestPlayer(const FVector& Location) const;

    // Memory optimization
    void OptimizeHazardMemory(FActiveHazardInstance& Hazard);
    void CleanupUnusedObjects();
    UObject* GetPooledObject(EEnvironmentalHazardType HazardType);
    void ReturnObjectToPool(EEnvironmentalHazardType HazardType, UObject* Object);

    // Network optimization
    void SyncHazardToNetwork(const FActiveHazardInstance& Hazard);
    void CompressHazardData(const FActiveHazardInstance& Hazard, TArray<uint8>& OutData);
    bool IsHazardNetworkRelevant(const FActiveHazardInstance& Hazard) const;

    // Performance monitoring
    void CheckPerformanceBudgets();
    void TriggerPerformanceOptimization();
    void RecordPerformanceMetrics();

    // Utility functions
    FString GetHazardTypeString(EEnvironmentalHazardType HazardType) const;
    float CalculateHazardComplexity(const FActiveHazardInstance& Hazard) const;
    bool ShouldCullHazard(const FActiveHazardInstance& Hazard) const;

    // Integration helpers
    void InitializeSubsystemReferences();
    void RegisterWithPerformanceManager();
    void UpdatePerformanceManagerMetrics();

    // Debug helpers
    void DrawHazardDebugInfo(const FActiveHazardInstance& Hazard);
    void LogHazardPerformance(const FActiveHazardInstance& Hazard);

    // Benchmark helpers
    void UpdateBenchmark(float DeltaTime);
    void CompleteBenchmark();
};