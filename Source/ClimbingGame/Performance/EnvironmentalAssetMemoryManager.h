#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "../Memory/ClimbingMemoryTracker.h"
#include "EnvironmentalAssetMemoryManager.generated.h"

UENUM(BlueprintType)
enum class EEnvironmentalAssetType : uint8
{
    ParticleSystem      UMETA(DisplayName = "Particle System"),
    NiagaraSystem       UMETA(DisplayName = "Niagara System"),
    StaticMesh          UMETA(DisplayName = "Static Mesh"),
    Material            UMETA(DisplayName = "Material"),
    Texture             UMETA(DisplayName = "Texture"),
    SoundWave           UMETA(DisplayName = "Sound Wave"),
    SkeletalMesh        UMETA(DisplayName = "Skeletal Mesh"),
    AnimationSequence   UMETA(DisplayName = "Animation Sequence"),
    PhysicsAsset        UMETA(DisplayName = "Physics Asset"),
    Blueprint           UMETA(DisplayName = "Blueprint")
};

UENUM(BlueprintType)
enum class EAssetStreamingStrategy : uint8
{
    Immediate           UMETA(DisplayName = "Load Immediately"),
    Proximity           UMETA(DisplayName = "Proximity Based"),
    OnDemand            UMETA(DisplayName = "On Demand"),
    Predictive          UMETA(DisplayName = "Predictive Loading"),
    Background          UMETA(DisplayName = "Background Loading"),
    Cached              UMETA(DisplayName = "Cached Loading")
};

UENUM(BlueprintType)
enum class EAssetPriority : uint8
{
    Critical            UMETA(DisplayName = "Critical"),
    High                UMETA(DisplayName = "High"),
    Medium              UMETA(DisplayName = "Medium"),
    Low                 UMETA(DisplayName = "Low"),
    Background          UMETA(DisplayName = "Background")
};

UENUM(BlueprintType)
enum class EMemoryPoolType : uint8
{
    ParticlePool        UMETA(DisplayName = "Particle Pool"),
    MeshPool            UMETA(DisplayName = "Mesh Pool"),
    TexturePool         UMETA(DisplayName = "Texture Pool"),
    AudioPool           UMETA(DisplayName = "Audio Pool"),
    PhysicsPool         UMETA(DisplayName = "Physics Pool"),
    GeneralPool         UMETA(DisplayName = "General Pool")
};

USTRUCT(BlueprintType)
struct FEnvironmentalAssetInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Info")
    TSoftObjectPtr<UObject> AssetReference;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    EEnvironmentalAssetType AssetType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Info")
    EAssetStreamingStrategy StreamingStrategy = EAssetStreamingStrategy::Proximity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Info")
    EAssetPriority Priority = EAssetPriority::Medium;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    float EstimatedSizeMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    float ActualSizeMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    bool bIsLoaded = false;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    bool bIsStreaming = false;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    float LastAccessTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Asset Info")
    int32 ReferenceCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
    float StreamingDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
    float UnloadDistance = 10000.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageLoadTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 LoadCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
    bool bUseObjectPooling = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling")
    EMemoryPoolType PoolType = EMemoryPoolType::GeneralPool;

    UPROPERTY(BlueprintReadOnly, Category = "Quality")
    int32 CurrentLODLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    TArray<TSoftObjectPtr<UObject>> LODVariants;

    UPROPERTY(BlueprintReadOnly, Category = "Network")
    bool bIsNetworkRelevant = false;

    UPROPERTY(BlueprintReadOnly, Category = "Network")
    float NetworkRelevancyDistance = 15000.0f;
};

USTRUCT(BlueprintType)
struct FAssetMemoryPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    EMemoryPoolType PoolType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    float MaxSizeMB = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    int32 MaxObjects = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Configuration")
    float PrewarmPercentage = 0.5f; // Prewarm 50% of pool

    UPROPERTY(BlueprintReadOnly, Category = "Pool Status")
    float CurrentSizeMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Status")
    int32 ActiveObjects = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Status")
    int32 AvailableObjects = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Status")
    TArray<TWeakObjectPtr<UObject>> PooledObjects;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Status")
    TMap<UObject*, float> ObjectSizes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Behavior")
    bool bAutoExpand = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Behavior")
    bool bAutoShrink = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Behavior")
    float ShrinkThreshold = 0.2f; // Shrink when usage < 20%

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Behavior")
    float ExpandThreshold = 0.8f; // Expand when usage > 80%

    UPROPERTY(BlueprintReadOnly, Category = "Pool Performance")
    float AverageAllocationTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Performance")
    float FragmentationPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Performance")
    int32 AllocationCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Pool Performance")
    int32 DeallocationCount = 0;
};

USTRUCT(BlueprintType)
struct FStreamingRegion
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
    float Radius = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
    EAssetPriority Priority = EAssetPriority::Medium;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Region")
    TArray<FEnvironmentalAssetInfo> AssetsToLoad;

    UPROPERTY(BlueprintReadOnly, Category = "Region Status")
    bool bIsActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Region Status")
    float LastActivationTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Region Status")
    int32 PlayersInRegion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
    float PreloadDistance = 1000.0f; // Start loading before entering

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
    float UnloadDistance = 2000.0f; // Extra distance before unloading

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float RegionMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageLoadTimeMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FAssetMemoryStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float TotalMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float StreamingMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PooledMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CachedMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalLoadedAssets = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 StreamingAssets = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CachedAssets = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageLoadTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MemoryFragmentationPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float StreamingBandwidthMBps = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 FailedLoads = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 UnloadedAssets = 0;

    UPROPERTY(BlueprintReadOnly)
    float CacheHitRatio = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PoolUtilizationPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TMap<EEnvironmentalAssetType, float> MemoryUsageByType;

    UPROPERTY(BlueprintReadOnly)
    TMap<EAssetPriority, int32> AssetCountByPriority;
};

UCLASS(BlueprintType, ClassGroup=(ClimbingGame), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalAssetMemoryManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalAssetMemoryManager();

    // Component interface
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Asset registration and management
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void RegisterAsset(const TSoftObjectPtr<UObject>& AssetReference, const FEnvironmentalAssetInfo& AssetInfo);

    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void UnregisterAsset(const TSoftObjectPtr<UObject>& AssetReference);

    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void UnregisterAllAssets();

    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void UpdateAssetPriority(const TSoftObjectPtr<UObject>& AssetReference, EAssetPriority NewPriority);

    // Asset streaming
    UFUNCTION(BlueprintCallable, Category = "Asset Streaming")
    void LoadAsset(const TSoftObjectPtr<UObject>& AssetReference, bool bHighPriority = false);

    UFUNCTION(BlueprintCallable, Category = "Asset Streaming")
    void UnloadAsset(const TSoftObjectPtr<UObject>& AssetReference);

    UFUNCTION(BlueprintCallable, Category = "Asset Streaming")
    void LoadAssetsInRadius(const FVector& Location, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Asset Streaming")
    void UnloadAssetsOutsideRadius(const FVector& Location, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Asset Streaming")
    void PreloadCriticalAssets();

    UFUNCTION(BlueprintCallable, Category = "Asset Streaming")
    void FlushAllStreamingRequests();

    // Memory pool management
    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void InitializeMemoryPools();

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void CreateMemoryPool(EMemoryPoolType PoolType, float MaxSizeMB, int32 MaxObjects);

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void DestroyMemoryPool(EMemoryPoolType PoolType);

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    UObject* GetPooledObject(EMemoryPoolType PoolType, const TSoftObjectPtr<UObject>& AssetReference);

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void ReturnPooledObject(UObject* Object, EMemoryPoolType PoolType);

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void OptimizeMemoryPools();

    // Streaming regions
    UFUNCTION(BlueprintCallable, Category = "Streaming Regions")
    void CreateStreamingRegion(const FStreamingRegion& Region);

    UFUNCTION(BlueprintCallable, Category = "Streaming Regions")
    void UpdateStreamingRegions(const TArray<FVector>& ViewerLocations);

    UFUNCTION(BlueprintCallable, Category = "Streaming Regions")
    void EnableStreamingRegion(int32 RegionIndex, bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Streaming Regions")
    void RemoveStreamingRegion(int32 RegionIndex);

    // Predictive loading
    UFUNCTION(BlueprintCallable, Category = "Predictive Loading")
    void EnablePredictiveLoading(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Predictive Loading")
    void UpdatePlayerMovementPrediction(const FVector& PlayerLocation, const FVector& PlayerVelocity);

    UFUNCTION(BlueprintCallable, Category = "Predictive Loading")
    void PredictAndLoadAssets(float PredictionTimeSeconds);

    // Memory optimization
    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void OptimizeMemoryUsage();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void DefragmentMemory();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void FlushUnusedAssets();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void SetMemoryBudget(float MemoryBudgetMB);

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void TriggerGarbageCollection();

    // LOD management for assets
    UFUNCTION(BlueprintCallable, Category = "Asset LOD")
    void SetAssetLODLevel(const TSoftObjectPtr<UObject>& AssetReference, int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Asset LOD")
    void UpdateAssetLODs(const FVector& ViewerLocation);

    UFUNCTION(BlueprintCallable, Category = "Asset LOD")
    void EnableDynamicLOD(bool bEnable);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring", BlueprintPure)
    FAssetMemoryStats GetMemoryStats() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void ResetMemoryStats();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetCurrentMemoryUsage() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsMemoryBudgetExceeded() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void StartMemoryProfiling();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void StopMemoryProfiling();

    // Query functions
    UFUNCTION(BlueprintCallable, Category = "Asset Query", BlueprintPure)
    bool IsAssetLoaded(const TSoftObjectPtr<UObject>& AssetReference) const;

    UFUNCTION(BlueprintCallable, Category = "Asset Query", BlueprintPure)
    float GetAssetSize(const TSoftObjectPtr<UObject>& AssetReference) const;

    UFUNCTION(BlueprintCallable, Category = "Asset Query", BlueprintPure)
    TArray<TSoftObjectPtr<UObject>> GetLoadedAssets() const;

    UFUNCTION(BlueprintCallable, Category = "Asset Query", BlueprintPure)
    TArray<TSoftObjectPtr<UObject>> GetAssetsInRadius(const FVector& Location, float Radius) const;

    // Debug and profiling
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowMemoryDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogMemoryStats();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpLoadedAssets();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RunMemoryBenchmark(float Duration = 10.0f);

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Settings")
    float TotalMemoryBudgetMB = 512.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Settings")
    float StreamingMemoryBudgetMB = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Settings")
    float PoolMemoryBudgetMB = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Settings")
    float CacheMemoryBudgetMB = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming Settings")
    bool bEnablePredictiveLoading = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming Settings")
    float DefaultStreamingDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming Settings")
    float DefaultUnloadDistance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming Settings")
    int32 MaxConcurrentStreamingRequests = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
    TMap<EMemoryPoolType, FAssetMemoryPool> DefaultMemoryPools;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableMemoryOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float MemoryOptimizationInterval = 5.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float GarbageCollectionThreshold = 0.8f; // Trigger at 80% memory usage

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnAssetLoaded;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnAssetUnloaded;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnMemoryBudgetExceeded;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnMemoryOptimized;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnPoolExhausted;

protected:
    // Asset tracking
    UPROPERTY()
    TMap<TSoftObjectPtr<UObject>, FEnvironmentalAssetInfo> RegisteredAssets;

    // Memory pools
    UPROPERTY()
    TMap<EMemoryPoolType, FAssetMemoryPool> ActiveMemoryPools;

    // Streaming regions
    UPROPERTY()
    TArray<FStreamingRegion> StreamingRegions;

    // Performance tracking
    FAssetMemoryStats CurrentStats;
    float LastMemoryOptimization = 0.0f;
    float LastStreamingUpdate = 0.0f;

    // Streaming state
    TArray<FStreamableHandle> ActiveStreamingHandles;
    TMap<TSoftObjectPtr<UObject>, FStreamableHandle> AssetStreamingHandles;

    // Predictive loading
    TArray<FVector> PlayerLocationHistory;
    TArray<FVector> PlayerVelocityHistory;
    float LastPredictionUpdate = 0.0f;

    // Debug state
    bool bShowDebugInfo = false;
    bool bMemoryProfilingActive = false;
    bool bIsRunningBenchmark = false;
    float BenchmarkStartTime = 0.0f;

    // Integration references
    UPROPERTY()
    UClimbingMemoryTracker* MemoryTracker = nullptr;

    UPROPERTY()
    FStreamableManager StreamableManager;

private:
    // Core update functions
    void UpdateAssetStreaming(float DeltaTime);
    void UpdateMemoryPools(float DeltaTime);
    void UpdatePredictiveLoading(float DeltaTime);
    void UpdateMemoryOptimization(float DeltaTime);
    
    // Asset loading implementation
    void LoadAssetInternal(const TSoftObjectPtr<UObject>& AssetReference, EAssetPriority Priority);
    void UnloadAssetInternal(const TSoftObjectPtr<UObject>& AssetReference);
    void HandleAssetLoadComplete(TSoftObjectPtr<UObject> AssetReference);
    
    // Memory pool implementation
    void InitializePool(EMemoryPoolType PoolType, const FAssetMemoryPool& PoolConfig);
    UObject* AllocateFromPool(EMemoryPoolType PoolType, const TSoftObjectPtr<UObject>& AssetReference);
    void DeallocateFromPool(UObject* Object, EMemoryPoolType PoolType);
    void ResizePool(EMemoryPoolType PoolType, bool bExpand);
    
    // Streaming region implementation
    void UpdateStreamingRegion(FStreamingRegion& Region, const TArray<FVector>& ViewerLocations);
    bool IsLocationInRegion(const FVector& Location, const FStreamingRegion& Region) const;
    void ActivateStreamingRegion(FStreamingRegion& Region);
    void DeactivateStreamingRegion(FStreamingRegion& Region);
    
    // Predictive loading implementation
    FVector PredictPlayerLocation(const FVector& CurrentLocation, const FVector& Velocity, float TimeSeconds) const;
    TArray<TSoftObjectPtr<UObject>> GetAssetsAlongPath(const FVector& Start, const FVector& End, float PathWidth) const;
    void LoadPredictedAssets(const TArray<TSoftObjectPtr<UObject>>& AssetsToLoad);
    
    // Memory optimization implementation
    void PerformMemoryOptimization();
    void EvictLeastRecentlyUsedAssets();
    void CompactMemoryPools();
    void ReduceAssetQuality();
    
    // LOD management implementation
    void UpdateAssetLOD(FEnvironmentalAssetInfo& AssetInfo, const FVector& ViewerLocation);
    TSoftObjectPtr<UObject> GetLODVariant(const FEnvironmentalAssetInfo& AssetInfo, int32 LODLevel) const;
    void SwapAssetLOD(FEnvironmentalAssetInfo& AssetInfo, int32 NewLODLevel);
    
    // Utility functions
    float CalculateAssetSize(UObject* Asset) const;
    EEnvironmentalAssetType DetermineAssetType(UObject* Asset) const;
    bool ShouldLoadAsset(const FEnvironmentalAssetInfo& AssetInfo, const TArray<FVector>& ViewerLocations) const;
    bool ShouldUnloadAsset(const FEnvironmentalAssetInfo& AssetInfo, const TArray<FVector>& ViewerLocations) const;
    void UpdateMemoryStats(float DeltaTime);
    
    // Integration helpers
    void InitializeMemoryTrackerIntegration();
    void UpdateMemoryTrackerStats();
    
    // Debug helpers
    void DrawMemoryDebugInfo();
    void DrawStreamingRegionDebug(const FStreamingRegion& Region);
    void LogAssetInfo(const FEnvironmentalAssetInfo& AssetInfo) const;
    
    // Benchmark helpers
    void UpdateBenchmark(float DeltaTime);
    void CompleteBenchmark();
};