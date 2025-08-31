#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformMemory.h"
#include "Stats/Stats.h"
#include "Misc/DateTime.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "ClimbingMemoryTracker.generated.h"

UENUM(BlueprintType)
enum class EMemoryCategory : uint8
{
    PhysicsObjects      UMETA(DisplayName = "Physics Objects"),
    ProceduralContent   UMETA(DisplayName = "Procedural Content"),
    NetworkBuffers      UMETA(DisplayName = "Network Buffers"),
    AssetStreaming      UMETA(DisplayName = "Asset Streaming"),
    Rendering           UMETA(DisplayName = "Rendering"),
    Audio               UMETA(DisplayName = "Audio"),
    Gameplay            UMETA(DisplayName = "Gameplay"),
    Other               UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EMemoryPressureLevel : uint8
{
    Normal      UMETA(DisplayName = "Normal"),
    Warning     UMETA(DisplayName = "Warning"),
    Critical    UMETA(DisplayName = "Critical"),
    Emergency   UMETA(DisplayName = "Emergency")
};

USTRUCT(BlueprintType)
struct FMemoryUsage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float PhysicalMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float VirtualMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float GPUMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AvailablePhysicalMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PeakPhysicalMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUtilizationPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;
};

USTRUCT(BlueprintType)
struct FCategoryMemoryStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EMemoryCategory Category;

    UPROPERTY(BlueprintReadOnly)
    float CurrentUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PeakUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 AllocationCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DeallocationCount = 0;

    UPROPERTY(BlueprintReadOnly)
    float AllocationRate = 0.0f; // Allocations per second

    UPROPERTY(BlueprintReadOnly)
    float FragmentationPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> UsageHistory;
};

USTRUCT(BlueprintType)
struct FMemoryPoolStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PoolName;

    UPROPERTY(BlueprintReadOnly)
    float TotalSizeMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float UsedSizeMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FreeSizeMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FragmentationPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveAllocations = 0;

    UPROPERTY(BlueprintReadOnly)
    float LargestFreeBlock = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float UtilizationPercent = 0.0f;
};

USTRUCT(BlueprintType)
struct FProceduralContentMemory
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float TerrainMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ClimbingRoutesMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ProceduralTexturesMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float LODSystemMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float StreamingMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveChunks = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LoadedChunks = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PendingChunks = 0;

    float GetTotalMemoryMB() const
    {
        return TerrainMemoryMB + ClimbingRoutesMemoryMB + ProceduralTexturesMemoryMB + 
               LODSystemMemoryMB + StreamingMemoryMB;
    }
};

USTRUCT(BlueprintType)
struct FMemoryAlert
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EMemoryPressureLevel PressureLevel;

    UPROPERTY(BlueprintReadOnly)
    EMemoryCategory Category;

    UPROPERTY(BlueprintReadOnly)
    FString AlertMessage;

    UPROPERTY(BlueprintReadOnly)
    float ThresholdMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CurrentUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FDateTime AlertTime;

    UPROPERTY(BlueprintReadOnly)
    bool bIsResolved = false;
};

USTRUCT(BlueprintType)
struct FMemoryThresholds
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WarningThresholdMB = 1536.0f; // 1.5GB

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalThresholdMB = 1843.0f; // 1.8GB

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EmergencyThresholdMB = 2048.0f; // 2GB

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GPUWarningThresholdMB = 3072.0f; // 3GB

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GPUCriticalThresholdMB = 4096.0f; // 4GB

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FragmentationWarningPercent = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AllocationRateWarningPerSec = 1000.0f;
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingMemoryTracker : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;

    // Memory tracking
    UFUNCTION(BlueprintCallable, Category = "Memory Tracking")
    FMemoryUsage GetCurrentMemoryUsage() const;

    UFUNCTION(BlueprintCallable, Category = "Memory Tracking")
    FCategoryMemoryStats GetCategoryStats(EMemoryCategory Category) const;

    UFUNCTION(BlueprintCallable, Category = "Memory Tracking")
    TArray<FCategoryMemoryStats> GetAllCategoryStats() const;

    UFUNCTION(BlueprintCallable, Category = "Memory Tracking")
    void TrackAllocation(EMemoryCategory Category, float SizeMB);

    UFUNCTION(BlueprintCallable, Category = "Memory Tracking")
    void TrackDeallocation(EMemoryCategory Category, float SizeMB);

    // Procedural content tracking
    UFUNCTION(BlueprintCallable, Category = "Procedural Content")
    FProceduralContentMemory GetProceduralContentMemory() const;

    UFUNCTION(BlueprintCallable, Category = "Procedural Content")
    void TrackProceduralChunkLoad(float MemorySizeMB);

    UFUNCTION(BlueprintCallable, Category = "Procedural Content")
    void TrackProceduralChunkUnload(float MemorySizeMB);

    UFUNCTION(BlueprintCallable, Category = "Procedural Content")
    void OptimizeProceduralMemory();

    // Memory pools
    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void CreateMemoryPool(const FString& PoolName, float SizeMB);

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    void DestroyMemoryPool(const FString& PoolName);

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    FMemoryPoolStats GetMemoryPoolStats(const FString& PoolName) const;

    UFUNCTION(BlueprintCallable, Category = "Memory Pools")
    TArray<FMemoryPoolStats> GetAllMemoryPoolStats() const;

    // Memory pressure management
    UFUNCTION(BlueprintCallable, Category = "Memory Pressure")
    EMemoryPressureLevel GetCurrentPressureLevel() const;

    UFUNCTION(BlueprintCallable, Category = "Memory Pressure")
    void HandleMemoryPressure(EMemoryPressureLevel PressureLevel);

    UFUNCTION(BlueprintCallable, Category = "Memory Pressure")
    TArray<FMemoryAlert> GetActiveAlerts() const;

    UFUNCTION(BlueprintCallable, Category = "Memory Pressure")
    void ClearAlert(int32 AlertIndex);

    // Memory optimization
    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void RunGarbageCollection(bool bForceFullCollection = false);

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void OptimizeMemoryUsage();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void DefragmentMemory();

    UFUNCTION(BlueprintCallable, Category = "Memory Optimization")
    void FlushUnusedAssets();

    // Streaming optimization
    UFUNCTION(BlueprintCallable, Category = "Streaming Optimization")
    void OptimizeAssetStreaming();

    UFUNCTION(BlueprintCallable, Category = "Streaming Optimization")
    void PreloadCriticalAssets();

    UFUNCTION(BlueprintCallable, Category = "Streaming Optimization")
    void UnloadDistantAssets(float DistanceThreshold);

    // Physics memory management
    UFUNCTION(BlueprintCallable, Category = "Physics Memory")
    void TrackPhysicsObjectCreation(UObject* PhysicsObject, float EstimatedSizeMB);

    UFUNCTION(BlueprintCallable, Category = "Physics Memory")
    void TrackPhysicsObjectDestruction(UObject* PhysicsObject);

    UFUNCTION(BlueprintCallable, Category = "Physics Memory")
    float GetPhysicsMemoryUsage() const;

    UFUNCTION(BlueprintCallable, Category = "Physics Memory")
    void OptimizePhysicsMemory();

    // Rope-specific memory tracking
    UFUNCTION(BlueprintCallable, Category = "Rope Memory")
    void TrackRopeCreation(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Memory")
    void TrackRopeDestruction(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Memory")
    float GetRopeMemoryUsage() const;

    // Reporting and analytics
    UFUNCTION(BlueprintCallable, Category = "Memory Reporting")
    void GenerateMemoryReport(const FString& ReportName);

    UFUNCTION(BlueprintCallable, Category = "Memory Reporting")
    void ExportMemoryDataToCSV(const FString& Filename);

    UFUNCTION(BlueprintCallable, Category = "Memory Reporting")
    void SaveMemorySnapshot(const FString& SnapshotName);

    UFUNCTION(BlueprintCallable, Category = "Memory Reporting")
    void CompareMemorySnapshots(const FString& Snapshot1, const FString& Snapshot2);

    // Real-time monitoring
    UFUNCTION(BlueprintCallable, Category = "Memory Monitoring")
    void StartMemoryMonitoring(float IntervalSeconds = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Memory Monitoring")
    void StopMemoryMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Memory Monitoring")
    bool IsMonitoringActive() const { return bIsMonitoringActive; }

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Config")
    FMemoryThresholds MemoryThresholds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Config")
    bool bEnableAutomaticOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Config")
    bool bEnableMemoryAlerts = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Config")
    float MemoryTrackingInterval = 1.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Config")
    int32 MaxHistoryEntries = 300; // 5 minutes at 1Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Config")
    bool bTrackDetailedPhysicsMemory = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnMemoryWarning;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnMemoryCritical;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnMemoryEmergency;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnMemoryOptimized;

    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FSimpleMulticastDelegate OnGarbageCollectionComplete;

protected:
    // Internal memory tracking
    UPROPERTY()
    TMap<EMemoryCategory, FCategoryMemoryStats> CategoryStats;

    UPROPERTY()
    TMap<FString, FMemoryPoolStats> MemoryPools;

    UPROPERTY()
    TArray<FMemoryUsage> MemoryHistory;

    UPROPERTY()
    TArray<FMemoryAlert> ActiveAlerts;

    UPROPERTY()
    FProceduralContentMemory ProceduralMemoryStats;

    // Physics object tracking
    UPROPERTY()
    TMap<UObject*, float> TrackedPhysicsObjects;

    UPROPERTY()
    TArray<UAdvancedRopeComponent*> TrackedRopes;

    // Monitoring state
    bool bIsMonitoringActive = false;
    float MonitoringInterval = 1.0f;
    float LastMonitoringUpdate = 0.0f;
    FTimerHandle MonitoringTimer;

    // Performance tracking
    FMemoryUsage PreviousMemoryUsage;
    EMemoryPressureLevel CurrentPressureLevel = EMemoryPressureLevel::Normal;
    float LastOptimizationTime = 0.0f;
    static const float OptimizationCooldown = 30.0f; // 30 seconds between optimizations

private:
    // Internal tracking functions
    void UpdateMemoryStatistics();
    void UpdateCategoryStatistics();
    void CheckMemoryThresholds();
    void CreateMemoryAlert(EMemoryPressureLevel Level, EMemoryCategory Category, const FString& Message);
    
    // Optimization helpers
    void PerformEmergencyCleanup();
    void OptimizeCategoryMemory(EMemoryCategory Category);
    void ReduceProceduralContentMemory();
    void OptimizePhysicsObjectMemory();
    void OptimizeNetworkBuffers();
    
    // Pool management
    void* AllocateFromPool(const FString& PoolName, size_t Size);
    void DeallocateFromPool(const FString& PoolName, void* Ptr);
    void UpdatePoolStatistics(const FString& PoolName);
    
    // Streaming helpers
    void PrioritizeAssetStreaming();
    void EvictLeastRecentlyUsedAssets();
    
    // Analytics helpers
    void RecordMemoryEvent(const FString& EventType, float Value);
    void CalculateMemoryTrends();
    
    // Platform-specific memory functions
    FPlatformMemoryStats GetPlatformMemoryStats() const;
    void* PlatformAllocateAligned(size_t Size, size_t Alignment);
    void PlatformFreeAligned(void* Ptr);
    
    // Utility functions
    FString FormatMemorySize(float SizeMB) const;
    float CalculateFragmentation(const FMemoryPoolStats& PoolStats) const;
    bool ShouldTriggerOptimization() const;
    
    // File I/O helpers
    bool SaveMemoryStatsToFile(const FString& Filename, const TArray<FCategoryMemoryStats>& Stats) const;
    bool LoadMemoryStatsFromFile(const FString& Filename, TArray<FCategoryMemoryStats>& OutStats) const;
};