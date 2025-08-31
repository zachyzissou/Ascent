#include "ClimbingMemoryTracker.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Stats/Stats.h"
#include "RHI.h"
#include "TimerManager.h"
#include "Components/CableComponent.h"

DECLARE_STATS_GROUP(TEXT("ClimbingMemory"), STATGROUP_ClimbingMemory, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Memory Tracking"), STAT_MemoryTracking, STATGROUP_ClimbingMemory);
DECLARE_CYCLE_STAT(TEXT("Memory Optimization"), STAT_MemoryOptimization, STATGROUP_ClimbingMemory);
DECLARE_MEMORY_STAT(TEXT("Physics Memory"), STAT_PhysicsMemory, STATGROUP_ClimbingMemory);
DECLARE_MEMORY_STAT(TEXT("Procedural Memory"), STAT_ProceduralMemory, STATGROUP_ClimbingMemory);
DECLARE_MEMORY_STAT(TEXT("Rope Memory"), STAT_RopeMemory, STATGROUP_ClimbingMemory);

void UClimbingMemoryTracker::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Initialize category statistics
    for (int32 i = 0; i < static_cast<int32>(EMemoryCategory::Other) + 1; ++i)
    {
        EMemoryCategory Category = static_cast<EMemoryCategory>(i);
        FCategoryMemoryStats Stats;
        Stats.Category = Category;
        Stats.UsageHistory.Reserve(MaxHistoryEntries);
        CategoryStats.Add(Category, Stats);
    }

    // Initialize memory thresholds with safe defaults
    MemoryThresholds.WarningThresholdMB = 1536.0f;
    MemoryThresholds.CriticalThresholdMB = 1843.0f;
    MemoryThresholds.EmergencyThresholdMB = 2048.0f;
    MemoryThresholds.GPUWarningThresholdMB = 3072.0f;
    MemoryThresholds.GPUCriticalThresholdMB = 4096.0f;
    MemoryThresholds.FragmentationWarningPercent = 30.0f;
    MemoryThresholds.AllocationRateWarningPerSec = 1000.0f;

    // Reserve memory for tracking arrays
    MemoryHistory.Reserve(MaxHistoryEntries);
    ActiveAlerts.Reserve(50);

    // Create default memory pools
    CreateMemoryPool(TEXT("PhysicsPool"), 256.0f); // 256MB for physics objects
    CreateMemoryPool(TEXT("ProceduralPool"), 512.0f); // 512MB for procedural content
    CreateMemoryPool(TEXT("NetworkPool"), 64.0f); // 64MB for network buffers
    CreateMemoryPool(TEXT("StreamingPool"), 256.0f); // 256MB for asset streaming

    // Start monitoring by default
    if (bEnableAutomaticOptimization)
    {
        StartMemoryMonitoring(MemoryTrackingInterval);
    }

    UE_LOG(LogMemory, Log, TEXT("ClimbingMemoryTracker initialized"));
}

void UClimbingMemoryTracker::Deinitialize()
{
    if (bIsMonitoringActive)
    {
        StopMemoryMonitoring();
    }

    // Clean up memory pools
    for (auto& PoolPair : MemoryPools)
    {
        DestroyMemoryPool(PoolPair.Key);
    }

    // Clear all tracking data
    CategoryStats.Empty();
    MemoryPools.Empty();
    MemoryHistory.Empty();
    ActiveAlerts.Empty();
    TrackedPhysicsObjects.Empty();
    TrackedRopes.Empty();

    Super::Deinitialize();
}

void UClimbingMemoryTracker::Tick(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_MemoryTracking);

    if (!bIsMonitoringActive)
        return;

    LastMonitoringUpdate += DeltaTime;

    if (LastMonitoringUpdate >= MonitoringInterval)
    {
        UpdateMemoryStatistics();
        UpdateCategoryStatistics();
        CheckMemoryThresholds();

        // Perform automatic optimization if enabled and needed
        if (bEnableAutomaticOptimization && ShouldTriggerOptimization())
        {
            OptimizeMemoryUsage();
        }

        LastMonitoringUpdate = 0.0f;
    }
}

FMemoryUsage UClimbingMemoryTracker::GetCurrentMemoryUsage() const
{
    FPlatformMemoryStats MemStats = GetPlatformMemoryStats();
    
    FMemoryUsage Usage;
    Usage.PhysicalMemoryMB = static_cast<float>(MemStats.UsedPhysical) / (1024.0f * 1024.0f);
    Usage.VirtualMemoryMB = static_cast<float>(MemStats.UsedVirtual) / (1024.0f * 1024.0f);
    Usage.AvailablePhysicalMB = static_cast<float>(MemStats.AvailablePhysical) / (1024.0f * 1024.0f);
    Usage.PeakPhysicalMB = static_cast<float>(MemStats.PeakUsedPhysical) / (1024.0f * 1024.0f);
    Usage.MemoryUtilizationPercent = (Usage.PhysicalMemoryMB / (Usage.PhysicalMemoryMB + Usage.AvailablePhysicalMB)) * 100.0f;
    Usage.Timestamp = FDateTime::Now();

    // Get GPU memory usage if available
    if (GRHISupportsMemoryInfo)
    {
        uint64 TotalGPUMemory = 0;
        uint64 UsedGPUMemory = 0;
        // In a real implementation, you would use RHI commands to get GPU memory info
        // RHIGetGPUMemoryStats(TotalGPUMemory, UsedGPUMemory);
        Usage.GPUMemoryMB = static_cast<float>(UsedGPUMemory) / (1024.0f * 1024.0f);
    }

    return Usage;
}

FCategoryMemoryStats UClimbingMemoryTracker::GetCategoryStats(EMemoryCategory Category) const
{
    const FCategoryMemoryStats* Stats = CategoryStats.Find(Category);
    return Stats ? *Stats : FCategoryMemoryStats();
}

TArray<FCategoryMemoryStats> UClimbingMemoryTracker::GetAllCategoryStats() const
{
    TArray<FCategoryMemoryStats> AllStats;
    for (const auto& StatPair : CategoryStats)
    {
        AllStats.Add(StatPair.Value);
    }
    return AllStats;
}

void UClimbingMemoryTracker::TrackAllocation(EMemoryCategory Category, float SizeMB)
{
    FCategoryMemoryStats* Stats = CategoryStats.Find(Category);
    if (!Stats)
        return;

    Stats->CurrentUsageMB += SizeMB;
    Stats->AllocationCount++;
    Stats->PeakUsageMB = FMath::Max(Stats->PeakUsageMB, Stats->CurrentUsageMB);

    // Update allocation rate (allocations per second)
    Stats->AllocationRate = Stats->AllocationCount / FMath::Max(1.0f, GetWorld()->GetTimeSeconds());

    // Update stats based on category
    switch (Category)
    {
        case EMemoryCategory::PhysicsObjects:
            SET_MEMORY_STAT(STAT_PhysicsMemory, static_cast<uint64>(Stats->CurrentUsageMB * 1024.0f * 1024.0f));
            break;
        case EMemoryCategory::ProceduralContent:
            SET_MEMORY_STAT(STAT_ProceduralMemory, static_cast<uint64>(Stats->CurrentUsageMB * 1024.0f * 1024.0f));
            break;
    }

    UE_LOG(LogMemory, VeryVerbose, TEXT("Memory allocated: Category=%s, Size=%.2fMB, Total=%.2fMB"),
           *UEnum::GetValueAsString(Category), SizeMB, Stats->CurrentUsageMB);
}

void UClimbingMemoryTracker::TrackDeallocation(EMemoryCategory Category, float SizeMB)
{
    FCategoryMemoryStats* Stats = CategoryStats.Find(Category);
    if (!Stats)
        return;

    Stats->CurrentUsageMB = FMath::Max(0.0f, Stats->CurrentUsageMB - SizeMB);
    Stats->DeallocationCount++;

    UE_LOG(LogMemory, VeryVerbose, TEXT("Memory deallocated: Category=%s, Size=%.2fMB, Total=%.2fMB"),
           *UEnum::GetValueAsString(Category), SizeMB, Stats->CurrentUsageMB);
}

FProceduralContentMemory UClimbingMemoryTracker::GetProceduralContentMemory() const
{
    return ProceduralMemoryStats;
}

void UClimbingMemoryTracker::TrackProceduralChunkLoad(float MemorySizeMB)
{
    ProceduralMemoryStats.ActiveChunks++;
    ProceduralMemoryStats.LoadedChunks++;
    ProceduralMemoryStats.StreamingMemoryMB += MemorySizeMB;

    TrackAllocation(EMemoryCategory::ProceduralContent, MemorySizeMB);

    UE_LOG(LogMemory, Log, TEXT("Procedural chunk loaded: %.2fMB (Total: %.2fMB, Active: %d)"),
           MemorySizeMB, ProceduralMemoryStats.GetTotalMemoryMB(), ProceduralMemoryStats.ActiveChunks);
}

void UClimbingMemoryTracker::TrackProceduralChunkUnload(float MemorySizeMB)
{
    ProceduralMemoryStats.ActiveChunks = FMath::Max(0, ProceduralMemoryStats.ActiveChunks - 1);
    ProceduralMemoryStats.StreamingMemoryMB = FMath::Max(0.0f, ProceduralMemoryStats.StreamingMemoryMB - MemorySizeMB);

    TrackDeallocation(EMemoryCategory::ProceduralContent, MemorySizeMB);

    UE_LOG(LogMemory, Log, TEXT("Procedural chunk unloaded: %.2fMB (Total: %.2fMB, Active: %d)"),
           MemorySizeMB, ProceduralMemoryStats.GetTotalMemoryMB(), ProceduralMemoryStats.ActiveChunks);
}

void UClimbingMemoryTracker::OptimizeProceduralMemory()
{
    SCOPE_CYCLE_COUNTER(STAT_MemoryOptimization);

    float TotalProceduralMemory = ProceduralMemoryStats.GetTotalMemoryMB();
    
    if (TotalProceduralMemory > MemoryThresholds.WarningThresholdMB * 0.3f) // 30% of warning threshold
    {
        UE_LOG(LogMemory, Log, TEXT("Optimizing procedural memory: Current=%.1fMB"), TotalProceduralMemory);

        // Unload distant chunks
        float MemoryBefore = TotalProceduralMemory;
        
        // In a real implementation, this would:
        // 1. Identify distant or unused chunks
        // 2. Unload them in priority order
        // 3. Compact remaining memory
        
        // Simulate memory reduction
        int32 ChunksToUnload = FMath::Max(1, ProceduralMemoryStats.ActiveChunks / 4);
        float MemorySaved = ChunksToUnload * 10.0f; // Assume 10MB per chunk
        
        ProceduralMemoryStats.ActiveChunks -= ChunksToUnload;
        ProceduralMemoryStats.StreamingMemoryMB -= MemorySaved;
        
        TrackDeallocation(EMemoryCategory::ProceduralContent, MemorySaved);
        
        UE_LOG(LogMemory, Log, TEXT("Procedural memory optimized: Saved %.1fMB (%.1f%% reduction)"),
               MemorySaved, (MemorySaved / MemoryBefore) * 100.0f);
    }
}

void UClimbingMemoryTracker::CreateMemoryPool(const FString& PoolName, float SizeMB)
{
    if (MemoryPools.Contains(PoolName))
    {
        UE_LOG(LogMemory, Warning, TEXT("Memory pool '%s' already exists"), *PoolName);
        return;
    }

    FMemoryPoolStats PoolStats;
    PoolStats.PoolName = PoolName;
    PoolStats.TotalSizeMB = SizeMB;
    PoolStats.UsedSizeMB = 0.0f;
    PoolStats.FreeSizeMB = SizeMB;
    PoolStats.FragmentationPercent = 0.0f;
    PoolStats.ActiveAllocations = 0;
    PoolStats.LargestFreeBlock = SizeMB;
    PoolStats.UtilizationPercent = 0.0f;

    MemoryPools.Add(PoolName, PoolStats);

    UE_LOG(LogMemory, Log, TEXT("Created memory pool '%s': %.1fMB"), *PoolName, SizeMB);
}

void UClimbingMemoryTracker::DestroyMemoryPool(const FString& PoolName)
{
    if (MemoryPools.Remove(PoolName) > 0)
    {
        UE_LOG(LogMemory, Log, TEXT("Destroyed memory pool '%s'"), *PoolName);
    }
}

FMemoryPoolStats UClimbingMemoryTracker::GetMemoryPoolStats(const FString& PoolName) const
{
    const FMemoryPoolStats* Stats = MemoryPools.Find(PoolName);
    return Stats ? *Stats : FMemoryPoolStats();
}

TArray<FMemoryPoolStats> UClimbingMemoryTracker::GetAllMemoryPoolStats() const
{
    TArray<FMemoryPoolStats> AllStats;
    for (const auto& PoolPair : MemoryPools)
    {
        AllStats.Add(PoolPair.Value);
    }
    return AllStats;
}

EMemoryPressureLevel UClimbingMemoryTracker::GetCurrentPressureLevel() const
{
    return CurrentPressureLevel;
}

void UClimbingMemoryTracker::HandleMemoryPressure(EMemoryPressureLevel PressureLevel)
{
    if (PressureLevel == CurrentPressureLevel)
        return;

    CurrentPressureLevel = PressureLevel;

    UE_LOG(LogMemory, Warning, TEXT("Memory pressure level changed to: %s"), 
           *UEnum::GetValueAsString(PressureLevel));

    switch (PressureLevel)
    {
        case EMemoryPressureLevel::Warning:
            OnMemoryWarning.Broadcast();
            // Light optimization
            OptimizeProceduralMemory();
            break;

        case EMemoryPressureLevel::Critical:
            OnMemoryCritical.Broadcast();
            // Aggressive optimization
            OptimizeMemoryUsage();
            RunGarbageCollection(false);
            break;

        case EMemoryPressureLevel::Emergency:
            OnMemoryEmergency.Broadcast();
            // Emergency cleanup
            PerformEmergencyCleanup();
            RunGarbageCollection(true);
            break;

        case EMemoryPressureLevel::Normal:
            // Pressure has subsided
            UE_LOG(LogMemory, Log, TEXT("Memory pressure returned to normal"));
            break;
    }
}

TArray<FMemoryAlert> UClimbingMemoryTracker::GetActiveAlerts() const
{
    return ActiveAlerts;
}

void UClimbingMemoryTracker::ClearAlert(int32 AlertIndex)
{
    if (ActiveAlerts.IsValidIndex(AlertIndex))
    {
        ActiveAlerts[AlertIndex].bIsResolved = true;
        ActiveAlerts.RemoveAt(AlertIndex);
    }
}

void UClimbingMemoryTracker::RunGarbageCollection(bool bForceFullCollection)
{
    UE_LOG(LogMemory, Log, TEXT("Running garbage collection (Force: %s)"), 
           bForceFullCollection ? TEXT("Yes") : TEXT("No"));

    if (bForceFullCollection)
    {
        GEngine->ForceGarbageCollection(true);
    }
    else
    {
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }

    OnGarbageCollectionComplete.Broadcast();
}

void UClimbingMemoryTracker::OptimizeMemoryUsage()
{
    SCOPE_CYCLE_COUNTER(STAT_MemoryOptimization);

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastOptimizationTime < OptimizationCooldown)
    {
        UE_LOG(LogMemory, Log, TEXT("Memory optimization on cooldown (%.1fs remaining)"),
               OptimizationCooldown - (CurrentTime - LastOptimizationTime));
        return;
    }

    UE_LOG(LogMemory, Log, TEXT("Starting memory optimization..."));

    FMemoryUsage MemoryBefore = GetCurrentMemoryUsage();

    // Optimize each category
    OptimizeCategoryMemory(EMemoryCategory::ProceduralContent);
    OptimizeCategoryMemory(EMemoryCategory::PhysicsObjects);
    OptimizeCategoryMemory(EMemoryCategory::NetworkBuffers);
    OptimizeCategoryMemory(EMemoryCategory::AssetStreaming);

    // Flush unused assets
    FlushUnusedAssets();

    // Defragment memory pools
    DefragmentMemory();

    // Run light garbage collection
    RunGarbageCollection(false);

    FMemoryUsage MemoryAfter = GetCurrentMemoryUsage();
    float MemorySaved = MemoryBefore.PhysicalMemoryMB - MemoryAfter.PhysicalMemoryMB;

    UE_LOG(LogMemory, Log, TEXT("Memory optimization complete: Saved %.1fMB (%.1f%% reduction)"),
           MemorySaved, (MemorySaved / MemoryBefore.PhysicalMemoryMB) * 100.0f);

    LastOptimizationTime = CurrentTime;
    OnMemoryOptimized.Broadcast();
}

void UClimbingMemoryTracker::DefragmentMemory()
{
    UE_LOG(LogMemory, Log, TEXT("Defragmenting memory pools..."));

    for (auto& PoolPair : MemoryPools)
    {
        FMemoryPoolStats& PoolStats = PoolPair.Value;
        
        // Simulate defragmentation by reducing fragmentation percentage
        if (PoolStats.FragmentationPercent > 5.0f)
        {
            float FragmentationReduction = PoolStats.FragmentationPercent * 0.7f;
            PoolStats.FragmentationPercent -= FragmentationReduction;
            PoolStats.LargestFreeBlock = PoolStats.FreeSizeMB;
            
            UE_LOG(LogMemory, Log, TEXT("Defragmented pool '%s': Fragmentation reduced by %.1f%%"),
                   *PoolStats.PoolName, FragmentationReduction);
        }
    }
}

void UClimbingMemoryTracker::FlushUnusedAssets()
{
    UE_LOG(LogMemory, Log, TEXT("Flushing unused assets..."));

    // Force asset registry to clean up unused assets
    if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
    {
        // In a real implementation, this would identify and unload unused assets
        // For now, we simulate by reducing category memory usage
        
        FCategoryMemoryStats* StreamingStats = CategoryStats.Find(EMemoryCategory::AssetStreaming);
        if (StreamingStats && StreamingStats->CurrentUsageMB > 0.0f)
        {
            float MemoryFreed = StreamingStats->CurrentUsageMB * 0.1f; // Free 10% of streaming memory
            StreamingStats->CurrentUsageMB -= MemoryFreed;
            
            UE_LOG(LogMemory, Log, TEXT("Freed %.1fMB from unused assets"), MemoryFreed);
        }
    }
}

void UClimbingMemoryTracker::TrackPhysicsObjectCreation(UObject* PhysicsObject, float EstimatedSizeMB)
{
    if (!IsValid(PhysicsObject) || !bTrackDetailedPhysicsMemory)
        return;

    TrackedPhysicsObjects.Add(PhysicsObject, EstimatedSizeMB);
    TrackAllocation(EMemoryCategory::PhysicsObjects, EstimatedSizeMB);

    UE_LOG(LogMemory, VeryVerbose, TEXT("Physics object created: %s (%.2fMB)"), 
           *PhysicsObject->GetName(), EstimatedSizeMB);
}

void UClimbingMemoryTracker::TrackPhysicsObjectDestruction(UObject* PhysicsObject)
{
    if (!IsValid(PhysicsObject))
        return;

    if (float* SizePtr = TrackedPhysicsObjects.Find(PhysicsObject))
    {
        float Size = *SizePtr;
        TrackedPhysicsObjects.Remove(PhysicsObject);
        TrackDeallocation(EMemoryCategory::PhysicsObjects, Size);

        UE_LOG(LogMemory, VeryVerbose, TEXT("Physics object destroyed: %s (%.2fMB)"), 
               *PhysicsObject->GetName(), Size);
    }
}

float UClimbingMemoryTracker::GetPhysicsMemoryUsage() const
{
    const FCategoryMemoryStats* Stats = CategoryStats.Find(EMemoryCategory::PhysicsObjects);
    return Stats ? Stats->CurrentUsageMB : 0.0f;
}

void UClimbingMemoryTracker::OptimizePhysicsMemory()
{
    float PhysicsMemory = GetPhysicsMemoryUsage();
    
    if (PhysicsMemory > 100.0f) // 100MB threshold
    {
        UE_LOG(LogMemory, Log, TEXT("Optimizing physics memory: Current=%.1fMB"), PhysicsMemory);

        // Remove invalid objects from tracking
        TArray<UObject*> InvalidObjects;
        for (auto& ObjectPair : TrackedPhysicsObjects)
        {
            if (!IsValid(ObjectPair.Key))
            {
                InvalidObjects.Add(ObjectPair.Key);
            }
        }

        float MemoryFreed = 0.0f;
        for (UObject* InvalidObject : InvalidObjects)
        {
            if (float* SizePtr = TrackedPhysicsObjects.Find(InvalidObject))
            {
                MemoryFreed += *SizePtr;
                TrackedPhysicsObjects.Remove(InvalidObject);
            }
        }

        if (MemoryFreed > 0.0f)
        {
            TrackDeallocation(EMemoryCategory::PhysicsObjects, MemoryFreed);
            UE_LOG(LogMemory, Log, TEXT("Physics memory cleanup: Freed %.1fMB"), MemoryFreed);
        }
    }
}

void UClimbingMemoryTracker::TrackRopeCreation(UAdvancedRopeComponent* Rope)
{
    if (!IsValid(Rope))
        return;

    TrackedRopes.Add(Rope);

    // Estimate rope memory usage based on segments
    float EstimatedSize = 0.5f; // Base size
    if (Rope->CableComponent)
    {
        EstimatedSize += Rope->CableComponent->NumSegments * 0.01f; // ~10KB per segment
    }

    TrackPhysicsObjectCreation(Rope, EstimatedSize);
    SET_MEMORY_STAT(STAT_RopeMemory, static_cast<uint64>(TrackedRopes.Num() * EstimatedSize * 1024.0f * 1024.0f));

    UE_LOG(LogMemory, Log, TEXT("Rope created: Segments=%d, EstimatedSize=%.2fMB, TotalRopes=%d"),
           Rope->CableComponent ? Rope->CableComponent->NumSegments : 0,
           EstimatedSize, TrackedRopes.Num());
}

void UClimbingMemoryTracker::TrackRopeDestruction(UAdvancedRopeComponent* Rope)
{
    if (TrackedRopes.Remove(Rope) > 0)
    {
        TrackPhysicsObjectDestruction(Rope);
        
        UE_LOG(LogMemory, Log, TEXT("Rope destroyed: TotalRopes=%d"), TrackedRopes.Num());
    }
}

float UClimbingMemoryTracker::GetRopeMemoryUsage() const
{
    float TotalMemory = 0.0f;
    for (UAdvancedRopeComponent* Rope : TrackedRopes)
    {
        if (IsValid(Rope))
        {
            float* SizePtr = TrackedPhysicsObjects.Find(Rope);
            if (SizePtr)
            {
                TotalMemory += *SizePtr;
            }
        }
    }
    return TotalMemory;
}

void UClimbingMemoryTracker::StartMemoryMonitoring(float IntervalSeconds)
{
    if (bIsMonitoringActive)
    {
        StopMemoryMonitoring();
    }

    MonitoringInterval = IntervalSeconds;
    bIsMonitoringActive = true;
    LastMonitoringUpdate = 0.0f;

    UE_LOG(LogMemory, Log, TEXT("Memory monitoring started: Interval=%.1fs"), IntervalSeconds);
}

void UClimbingMemoryTracker::StopMemoryMonitoring()
{
    bIsMonitoringActive = false;
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(MonitoringTimer);
    }

    UE_LOG(LogMemory, Log, TEXT("Memory monitoring stopped"));
}

void UClimbingMemoryTracker::UpdateMemoryStatistics()
{
    FMemoryUsage CurrentUsage = GetCurrentMemoryUsage();
    
    // Add to history
    MemoryHistory.Add(CurrentUsage);
    if (MemoryHistory.Num() > MaxHistoryEntries)
    {
        MemoryHistory.RemoveAt(0);
    }

    PreviousMemoryUsage = CurrentUsage;
}

void UClimbingMemoryTracker::UpdateCategoryStatistics()
{
    for (auto& StatPair : CategoryStats)
    {
        FCategoryMemoryStats& Stats = StatPair.Value;
        
        // Update usage history
        Stats.UsageHistory.Add(Stats.CurrentUsageMB);
        if (Stats.UsageHistory.Num() > MaxHistoryEntries)
        {
            Stats.UsageHistory.RemoveAt(0);
        }

        // Calculate average usage
        if (Stats.UsageHistory.Num() > 0)
        {
            float TotalUsage = 0.0f;
            for (float Usage : Stats.UsageHistory)
            {
                TotalUsage += Usage;
            }
            Stats.AverageUsageMB = TotalUsage / Stats.UsageHistory.Num();
        }

        // Calculate allocation rate
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime > 1.0f)
        {
            Stats.AllocationRate = Stats.AllocationCount / CurrentTime;
        }
    }
}

void UClimbingMemoryTracker::CheckMemoryThresholds()
{
    if (!bEnableMemoryAlerts)
        return;

    FMemoryUsage CurrentUsage = GetCurrentMemoryUsage();
    EMemoryPressureLevel NewPressureLevel = EMemoryPressureLevel::Normal;

    // Check physical memory thresholds
    if (CurrentUsage.PhysicalMemoryMB >= MemoryThresholds.EmergencyThresholdMB)
    {
        NewPressureLevel = EMemoryPressureLevel::Emergency;
        CreateMemoryAlert(EMemoryPressureLevel::Emergency, EMemoryCategory::Other,
                         FString::Printf(TEXT("Physical memory usage critical: %.1fMB >= %.1fMB"),
                                       CurrentUsage.PhysicalMemoryMB, MemoryThresholds.EmergencyThresholdMB));
    }
    else if (CurrentUsage.PhysicalMemoryMB >= MemoryThresholds.CriticalThresholdMB)
    {
        NewPressureLevel = EMemoryPressureLevel::Critical;
        CreateMemoryAlert(EMemoryPressureLevel::Critical, EMemoryCategory::Other,
                         FString::Printf(TEXT("Physical memory usage high: %.1fMB >= %.1fMB"),
                                       CurrentUsage.PhysicalMemoryMB, MemoryThresholds.CriticalThresholdMB));
    }
    else if (CurrentUsage.PhysicalMemoryMB >= MemoryThresholds.WarningThresholdMB)
    {
        NewPressureLevel = EMemoryPressureLevel::Warning;
        CreateMemoryAlert(EMemoryPressureLevel::Warning, EMemoryCategory::Other,
                         FString::Printf(TEXT("Physical memory usage elevated: %.1fMB >= %.1fMB"),
                                       CurrentUsage.PhysicalMemoryMB, MemoryThresholds.WarningThresholdMB));
    }

    // Check GPU memory thresholds
    if (CurrentUsage.GPUMemoryMB >= MemoryThresholds.GPUCriticalThresholdMB)
    {
        CreateMemoryAlert(EMemoryPressureLevel::Critical, EMemoryCategory::Rendering,
                         FString::Printf(TEXT("GPU memory usage critical: %.1fMB >= %.1fMB"),
                                       CurrentUsage.GPUMemoryMB, MemoryThresholds.GPUCriticalThresholdMB));
    }
    else if (CurrentUsage.GPUMemoryMB >= MemoryThresholds.GPUWarningThresholdMB)
    {
        CreateMemoryAlert(EMemoryPressureLevel::Warning, EMemoryCategory::Rendering,
                         FString::Printf(TEXT("GPU memory usage elevated: %.1fMB >= %.1fMB"),
                                       CurrentUsage.GPUMemoryMB, MemoryThresholds.GPUWarningThresholdMB));
    }

    // Handle pressure level changes
    if (NewPressureLevel != CurrentPressureLevel)
    {
        HandleMemoryPressure(NewPressureLevel);
    }
}

void UClimbingMemoryTracker::CreateMemoryAlert(EMemoryPressureLevel Level, EMemoryCategory Category, const FString& Message)
{
    FMemoryAlert Alert;
    Alert.PressureLevel = Level;
    Alert.Category = Category;
    Alert.AlertMessage = Message;
    Alert.AlertTime = FDateTime::Now();
    Alert.bIsResolved = false;

    // Set thresholds based on category and level
    const FCategoryMemoryStats* Stats = CategoryStats.Find(Category);
    if (Stats)
    {
        Alert.CurrentUsageMB = Stats->CurrentUsageMB;
    }

    switch (Level)
    {
        case EMemoryPressureLevel::Warning:
            Alert.ThresholdMB = MemoryThresholds.WarningThresholdMB;
            break;
        case EMemoryPressureLevel::Critical:
            Alert.ThresholdMB = MemoryThresholds.CriticalThresholdMB;
            break;
        case EMemoryPressureLevel::Emergency:
            Alert.ThresholdMB = MemoryThresholds.EmergencyThresholdMB;
            break;
        default:
            Alert.ThresholdMB = 0.0f;
            break;
    }

    ActiveAlerts.Add(Alert);

    UE_LOG(LogMemory, Warning, TEXT("Memory Alert: %s"), *Message);
}

void UClimbingMemoryTracker::PerformEmergencyCleanup()
{
    UE_LOG(LogMemory, Error, TEXT("Performing emergency memory cleanup!"));

    // Aggressive cleanup measures
    OptimizeProceduralMemory();
    OptimizePhysicsMemory();
    FlushUnusedAssets();
    DefragmentMemory();

    // Clear all non-essential caches
    for (auto& StatPair : CategoryStats)
    {
        if (StatPair.Key != EMemoryCategory::PhysicsObjects && 
            StatPair.Key != EMemoryCategory::Gameplay)
        {
            OptimizeCategoryMemory(StatPair.Key);
        }
    }

    // Force full garbage collection
    RunGarbageCollection(true);
}

void UClimbingMemoryTracker::OptimizeCategoryMemory(EMemoryCategory Category)
{
    FCategoryMemoryStats* Stats = CategoryStats.Find(Category);
    if (!Stats || Stats->CurrentUsageMB < 50.0f) // Skip if less than 50MB
        return;

    float MemoryBefore = Stats->CurrentUsageMB;
    float OptimizationTarget = 0.2f; // Try to reduce by 20%

    switch (Category)
    {
        case EMemoryCategory::ProceduralContent:
            OptimizeProceduralMemory();
            break;
        case EMemoryCategory::PhysicsObjects:
            OptimizePhysicsMemory();
            break;
        case EMemoryCategory::NetworkBuffers:
            // Optimize network buffers by compressing data
            Stats->CurrentUsageMB *= 0.8f; // Simulate 20% reduction
            break;
        case EMemoryCategory::AssetStreaming:
            FlushUnusedAssets();
            break;
        default:
            // Generic optimization - simulate 10% reduction
            Stats->CurrentUsageMB *= 0.9f;
            break;
    }

    float MemoryAfter = Stats->CurrentUsageMB;
    float MemorySaved = MemoryBefore - MemoryAfter;

    if (MemorySaved > 0.1f) // Only log if we saved at least 0.1MB
    {
        UE_LOG(LogMemory, Log, TEXT("Optimized %s memory: Saved %.1fMB (%.1f%% reduction)"),
               *UEnum::GetValueAsString(Category), MemorySaved, (MemorySaved / MemoryBefore) * 100.0f);
    }
}

FPlatformMemoryStats UClimbingMemoryTracker::GetPlatformMemoryStats() const
{
    return FPlatformMemory::GetStats();
}

bool UClimbingMemoryTracker::ShouldTriggerOptimization() const
{
    FMemoryUsage CurrentUsage = GetCurrentMemoryUsage();
    
    // Trigger optimization if we're above warning threshold
    if (CurrentUsage.PhysicalMemoryMB >= MemoryThresholds.WarningThresholdMB)
        return true;

    // Or if memory utilization is very high
    if (CurrentUsage.MemoryUtilizationPercent >= 80.0f)
        return true;

    // Or if we have high fragmentation in any pool
    for (const auto& PoolPair : MemoryPools)
    {
        if (PoolPair.Value.FragmentationPercent >= MemoryThresholds.FragmentationWarningPercent)
            return true;
    }

    return false;
}

FString UClimbingMemoryTracker::FormatMemorySize(float SizeMB) const
{
    if (SizeMB >= 1024.0f)
    {
        return FString::Printf(TEXT("%.2f GB"), SizeMB / 1024.0f);
    }
    else
    {
        return FString::Printf(TEXT("%.1f MB"), SizeMB);
    }
}

// Placeholder implementations for remaining functions
void UClimbingMemoryTracker::GenerateMemoryReport(const FString& ReportName) {}
void UClimbingMemoryTracker::ExportMemoryDataToCSV(const FString& Filename) {}
void UClimbingMemoryTracker::SaveMemorySnapshot(const FString& SnapshotName) {}
void UClimbingMemoryTracker::CompareMemorySnapshots(const FString& Snapshot1, const FString& Snapshot2) {}
void UClimbingMemoryTracker::OptimizeAssetStreaming() {}
void UClimbingMemoryTracker::PreloadCriticalAssets() {}
void UClimbingMemoryTracker::UnloadDistantAssets(float DistanceThreshold) {}
void UClimbingMemoryTracker::OptimizeNetworkBuffers() {}