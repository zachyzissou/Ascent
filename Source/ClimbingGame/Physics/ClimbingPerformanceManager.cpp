#include "ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "Stats/Stats.h"
#include "RHI.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameViewportClient.h"

DECLARE_STATS_GROUP(TEXT("ClimbingGame"), STATGROUP_ClimbingGame, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Rope Physics"), STAT_RopePhysics, STATGROUP_ClimbingGame);
DECLARE_CYCLE_STAT(TEXT("Climbing Physics"), STAT_ClimbingPhysics, STATGROUP_ClimbingGame);
DECLARE_CYCLE_STAT(TEXT("Performance Manager"), STAT_PerformanceManager, STATGROUP_ClimbingGame);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Ropes"), STAT_ActiveRopes, STATGROUP_ClimbingGame);
DECLARE_DWORD_COUNTER_STAT(TEXT("Physics Constraints"), STAT_PhysicsConstraints, STATGROUP_ClimbingGame);

void UClimbingPerformanceManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    CachedWorld = GetWorld();
    FrameTimeHistory.Reserve(300); // Store 5 seconds of frame time data at 60fps
    
    // Initialize performance targets with sensible defaults
    PerformanceTargets.TargetFPS = 60.0f;
    PerformanceTargets.MinimumFPS = 30.0f;
    PerformanceTargets.MaxFrameTimeMs = 16.67f;
    PerformanceTargets.MaxPhysicsTimeMs = 5.0f;
    PerformanceTargets.MaxActiveRopes = 50;
    PerformanceTargets.MaxPhysicsConstraints = 200;
    PerformanceTargets.MaxMemoryUsageMB = 2048.0f;
    
    // Initialize LOD distances
    LODDistances.UltraDistance = 500.0f;
    LODDistances.HighDistance = 1500.0f;
    LODDistances.MediumDistance = 3000.0f;
    LODDistances.LowDistance = 5000.0f;
    LODDistances.MinimalDistance = 8000.0f;
    LODDistances.DisabledDistance = 12000.0f;

    // Initialize physics settings
    PhysicsSettings.UltraRopeSegments = 64;
    PhysicsSettings.HighRopeSegments = 32;
    PhysicsSettings.MediumRopeSegments = 16;
    PhysicsSettings.LowRopeSegments = 8;
    PhysicsSettings.MinimalRopeSegments = 4;

    UE_LOG(LogTemp, Log, TEXT("ClimbingPerformanceManager initialized"));
}

void UClimbingPerformanceManager::Deinitialize()
{
    // Clean up tracked objects
    TrackedRopes.Empty();
    TrackedTools.Empty();
    TrackedClimbers.Empty();
    ActorLODCache.Empty();
    ActorDistanceCache.Empty();
    FrameTimeHistory.Empty();

    UE_LOG(LogTemp, Log, TEXT("ClimbingPerformanceManager deinitialized"));
    Super::Deinitialize();
}

bool UClimbingPerformanceManager::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UClimbingPerformanceManager::TickPerformanceManager(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_PerformanceManager);

    if (!CachedWorld)
        return;

    StartFrameProfiler();

    // Update performance metrics
    UpdatePerformanceMetrics(DeltaTime);

    // Update LOD system
    if (bEnableLODOptimization)
    {
        UpdateLODSystem(DeltaTime);
    }

    // Batch process objects
    if (bEnableBatchProcessing)
    {
        BatchUpdateRopes(DeltaTime);
        BatchUpdateTools(DeltaTime);
        BatchUpdateClimbers(DeltaTime);
    }

    // Check adaptive quality
    if (bEnableAdaptiveQuality)
    {
        LastAdaptiveQualityCheck += DeltaTime;
        if (LastAdaptiveQualityCheck >= AdaptiveQualityCheckInterval)
        {
            AdjustQualityForPerformance();
            LastAdaptiveQualityCheck = 0.0f;
        }
    }

    // Check performance thresholds
    CheckPerformanceThresholds();

    EndFrameProfiler();
}

FPerformanceMetrics UClimbingPerformanceManager::GetCurrentMetrics() const
{
    return CurrentMetrics;
}

void UClimbingPerformanceManager::StartFrameProfiler()
{
    FrameStartTime = FPlatformTime::Seconds();
}

void UClimbingPerformanceManager::EndFrameProfiler()
{
    double FrameEndTime = FPlatformTime::Seconds();
    double FrameTime = (FrameEndTime - FrameStartTime) * 1000.0; // Convert to milliseconds

    // Add to history
    FrameTimeHistory.Add(static_cast<float>(FrameTime));
    if (FrameTimeHistory.Num() > 300) // Keep last 300 frames
    {
        FrameTimeHistory.RemoveAt(0);
    }

    // Calculate running averages
    float TotalTime = 0.0f;
    for (float Time : FrameTimeHistory)
    {
        TotalTime += Time;
    }
    CurrentMetrics.AverageFrameTime = TotalTime / FrameTimeHistory.Num();
    CurrentMetrics.CurrentFPS = FrameTimeHistory.Num() > 0 ? 1000.0f / CurrentMetrics.AverageFrameTime : 0.0f;
}

EPerformanceLOD UClimbingPerformanceManager::GetObjectLOD(const AActor* Actor, const FVector& ViewerLocation) const
{
    if (!Actor)
        return EPerformanceLOD::Disabled;

    float Distance = FVector::Dist(Actor->GetActorLocation(), ViewerLocation);
    return CalculateLODFromDistance(Distance);
}

void UClimbingPerformanceManager::UpdateObjectLOD(AActor* Actor, EPerformanceLOD NewLOD)
{
    if (!Actor)
        return;

    EPerformanceLOD* CachedLOD = ActorLODCache.Find(Actor);
    if (CachedLOD && *CachedLOD == NewLOD)
        return; // No change needed

    // Update cache
    ActorLODCache.Add(Actor, NewLOD);

    // Apply LOD-specific optimizations
    if (UAdvancedRopeComponent* Rope = Actor->FindComponentByClass<UAdvancedRopeComponent>())
    {
        ApplyRopeLOD(Rope, NewLOD);
    }
    else if (AClimbingToolBase* Tool = Cast<AClimbingToolBase>(Actor))
    {
        ApplyToolLOD(Tool, NewLOD);
    }
    else if (UAdvancedClimbingComponent* Climber = Actor->FindComponentByClass<UAdvancedClimbingComponent>())
    {
        ApplyClimberLOD(Climber, NewLOD);
    }
}

void UClimbingPerformanceManager::UpdateAllObjectLODs()
{
    FVector ViewerLocation = GetAverageViewerLocation();

    // Update rope LODs
    for (UAdvancedRopeComponent* Rope : TrackedRopes)
    {
        if (IsValid(Rope) && IsValid(Rope->GetOwner()))
        {
            EPerformanceLOD LOD = GetObjectLOD(Rope->GetOwner(), ViewerLocation);
            UpdateObjectLOD(Rope->GetOwner(), LOD);
        }
    }

    // Update tool LODs
    for (AClimbingToolBase* Tool : TrackedTools)
    {
        if (IsValid(Tool))
        {
            EPerformanceLOD LOD = GetObjectLOD(Tool, ViewerLocation);
            UpdateObjectLOD(Tool, LOD);
        }
    }

    // Update climber LODs
    for (UAdvancedClimbingComponent* Climber : TrackedClimbers)
    {
        if (IsValid(Climber) && IsValid(Climber->GetOwner()))
        {
            EPerformanceLOD LOD = GetObjectLOD(Climber->GetOwner(), ViewerLocation);
            UpdateObjectLOD(Climber->GetOwner(), LOD);
        }
    }
}

void UClimbingPerformanceManager::OptimizeRopePhysics(UAdvancedRopeComponent* Rope, EPerformanceLOD LOD)
{
    if (!IsValid(Rope))
        return;

    ApplyRopeLOD(Rope, LOD);
}

void UClimbingPerformanceManager::OptimizeClimberPhysics(UAdvancedClimbingComponent* Climber, EPerformanceLOD LOD)
{
    if (!IsValid(Climber))
        return;

    ApplyClimberLOD(Climber, LOD);
}

void UClimbingPerformanceManager::OptimizeToolPhysics(AClimbingToolBase* Tool, EPerformanceLOD LOD)
{
    if (!IsValid(Tool))
        return;

    ApplyToolLOD(Tool, LOD);
}

void UClimbingPerformanceManager::BatchUpdateRopes(float DeltaTime)
{
    if (TrackedRopes.Num() == 0)
        return;

    int32 StartIndex = RopeBatchIndex;
    int32 EndIndex = FMath::Min(StartIndex + BatchSize, TrackedRopes.Num());

    for (int32 i = StartIndex; i < EndIndex; ++i)
    {
        if (IsValid(TrackedRopes[i]))
        {
            // Perform batch update operations
            EPerformanceLOD* CachedLOD = ActorLODCache.Find(TrackedRopes[i]->GetOwner());
            if (CachedLOD && *CachedLOD >= EPerformanceLOD::Medium)
            {
                // Only update medium quality and above
                TrackedRopes[i]->GetOwner()->Tick(DeltaTime);
            }
        }
    }

    RopeBatchIndex = (EndIndex >= TrackedRopes.Num()) ? 0 : EndIndex;
}

void UClimbingPerformanceManager::BatchUpdateTools(float DeltaTime)
{
    if (TrackedTools.Num() == 0)
        return;

    int32 StartIndex = ToolBatchIndex;
    int32 EndIndex = FMath::Min(StartIndex + BatchSize, TrackedTools.Num());

    for (int32 i = StartIndex; i < EndIndex; ++i)
    {
        if (IsValid(TrackedTools[i]))
        {
            EPerformanceLOD* CachedLOD = ActorLODCache.Find(TrackedTools[i]);
            if (CachedLOD && *CachedLOD >= EPerformanceLOD::Low)
            {
                TrackedTools[i]->Tick(DeltaTime);
            }
        }
    }

    ToolBatchIndex = (EndIndex >= TrackedTools.Num()) ? 0 : EndIndex;
}

void UClimbingPerformanceManager::BatchUpdateClimbers(float DeltaTime)
{
    if (TrackedClimbers.Num() == 0)
        return;

    int32 StartIndex = ClimberBatchIndex;
    int32 EndIndex = FMath::Min(StartIndex + BatchSize, TrackedClimbers.Num());

    for (int32 i = StartIndex; i < EndIndex; ++i)
    {
        if (IsValid(TrackedClimbers[i]))
        {
            // Climbers are always high priority, but we can adjust update frequency
            float UpdateRate = 1.0f;
            EPerformanceLOD* CachedLOD = ActorLODCache.Find(TrackedClimbers[i]->GetOwner());
            if (CachedLOD)
            {
                switch (*CachedLOD)
                {
                    case EPerformanceLOD::Ultra:
                        UpdateRate = 1.0f;
                        break;
                    case EPerformanceLOD::High:
                        UpdateRate = 0.75f;
                        break;
                    case EPerformanceLOD::Medium:
                        UpdateRate = 0.5f;
                        break;
                    default:
                        UpdateRate = 0.25f;
                        break;
                }
            }
            
            if (FMath::RandRange(0.0f, 1.0f) < UpdateRate)
            {
                TrackedClimbers[i]->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
            }
        }
    }

    ClimberBatchIndex = (EndIndex >= TrackedClimbers.Num()) ? 0 : EndIndex;
}

void UClimbingPerformanceManager::AdjustQualityForPerformance()
{
    // Check if we're hitting performance targets
    bool bPerformancePoor = (CurrentMetrics.CurrentFPS < PerformanceTargets.MinimumFPS) ||
                           (CurrentMetrics.AverageFrameTime > PerformanceTargets.MaxFrameTimeMs) ||
                           (CurrentMetrics.PhysicsTime > PerformanceTargets.MaxPhysicsTimeMs);

    bool bPerformanceGood = (CurrentMetrics.CurrentFPS > PerformanceTargets.TargetFPS * 0.9f) &&
                           (CurrentMetrics.AverageFrameTime < PerformanceTargets.MaxFrameTimeMs * 0.8f);

    if (bPerformancePoor)
    {
        // Reduce quality globally
        AdjustGlobalQuality(-0.1f);
        OnPerformanceCritical.Broadcast();
    }
    else if (bPerformanceGood)
    {
        // We can afford to increase quality slightly
        AdjustGlobalQuality(0.05f);
    }
}

void UClimbingPerformanceManager::SetGlobalLODBias(float LODBias)
{
    // Adjust all LOD distances by the bias
    float BiasMultiplier = 1.0f + LODBias;
    
    LODDistances.UltraDistance *= BiasMultiplier;
    LODDistances.HighDistance *= BiasMultiplier;
    LODDistances.MediumDistance *= BiasMultiplier;
    LODDistances.LowDistance *= BiasMultiplier;
    LODDistances.MinimalDistance *= BiasMultiplier;
    LODDistances.DisabledDistance *= BiasMultiplier;
    
    // Force LOD update
    UpdateAllObjectLODs();
    OnQualityAdjusted.Broadcast();
}

void UClimbingPerformanceManager::RunGarbageCollection()
{
    // Force garbage collection to free up memory
    GEngine->ForceGarbageCollection(true);
    
    // Update memory metrics after GC
    TrackMemoryUsage();
}

void UClimbingPerformanceManager::OptimizeMemoryUsage()
{
    // Remove invalid objects from tracking arrays
    TrackedRopes.RemoveAll([](UAdvancedRopeComponent* Rope) {
        return !IsValid(Rope);
    });
    
    TrackedTools.RemoveAll([](AClimbingToolBase* Tool) {
        return !IsValid(Tool);
    });
    
    TrackedClimbers.RemoveAll([](UAdvancedClimbingComponent* Climber) {
        return !IsValid(Climber);
    });

    // Clean up actor caches
    TArray<AActor*> InvalidActors;
    for (auto& Pair : ActorLODCache)
    {
        if (!IsValid(Pair.Key))
        {
            InvalidActors.Add(Pair.Key);
        }
    }
    
    for (AActor* Actor : InvalidActors)
    {
        ActorLODCache.Remove(Actor);
        ActorDistanceCache.Remove(Actor);
    }

    // Optimize texture memory
    OptimizeTextureMemory();
    
    // Optimize mesh memory
    OptimizeMeshMemory();
}

void UClimbingPerformanceManager::RegisterRope(UAdvancedRopeComponent* Rope)
{
    if (IsValid(Rope) && !TrackedRopes.Contains(Rope))
    {
        TrackedRopes.Add(Rope);
        CurrentMetrics.ActiveRopes = TrackedRopes.Num();
        SET_DWORD_STAT(STAT_ActiveRopes, CurrentMetrics.ActiveRopes);
    }
}

void UClimbingPerformanceManager::UnregisterRope(UAdvancedRopeComponent* Rope)
{
    if (TrackedRopes.Remove(Rope) > 0)
    {
        CurrentMetrics.ActiveRopes = TrackedRopes.Num();
        SET_DWORD_STAT(STAT_ActiveRopes, CurrentMetrics.ActiveRopes);
        
        if (IsValid(Rope->GetOwner()))
        {
            ActorLODCache.Remove(Rope->GetOwner());
            ActorDistanceCache.Remove(Rope->GetOwner());
        }
    }
}

void UClimbingPerformanceManager::RegisterTool(AClimbingToolBase* Tool)
{
    if (IsValid(Tool) && !TrackedTools.Contains(Tool))
    {
        TrackedTools.Add(Tool);
        CurrentMetrics.ActiveTools = TrackedTools.Num();
    }
}

void UClimbingPerformanceManager::UnregisterTool(AClimbingToolBase* Tool)
{
    if (TrackedTools.Remove(Tool) > 0)
    {
        CurrentMetrics.ActiveTools = TrackedTools.Num();
        ActorLODCache.Remove(Tool);
        ActorDistanceCache.Remove(Tool);
    }
}

void UClimbingPerformanceManager::RegisterClimber(UAdvancedClimbingComponent* Climber)
{
    if (IsValid(Climber) && !TrackedClimbers.Contains(Climber))
    {
        TrackedClimbers.Add(Climber);
        CurrentMetrics.ActiveClimbers = TrackedClimbers.Num();
    }
}

void UClimbingPerformanceManager::UnregisterClimber(UAdvancedClimbingComponent* Climber)
{
    if (TrackedClimbers.Remove(Climber) > 0)
    {
        CurrentMetrics.ActiveClimbers = TrackedClimbers.Num();
        
        if (IsValid(Climber->GetOwner()))
        {
            ActorLODCache.Remove(Climber->GetOwner());
            ActorDistanceCache.Remove(Climber->GetOwner());
        }
    }
}

void UClimbingPerformanceManager::UpdatePerformanceMetrics(float DeltaTime)
{
    // Update object counts
    CurrentMetrics.ActiveRopes = TrackedRopes.Num();
    CurrentMetrics.ActiveTools = TrackedTools.Num();
    CurrentMetrics.ActiveClimbers = TrackedClimbers.Num();

    // Calculate physics constraints
    CurrentMetrics.PhysicsConstraints = 0;
    for (UAdvancedRopeComponent* Rope : TrackedRopes)
    {
        if (IsValid(Rope))
        {
            CurrentMetrics.PhysicsConstraints += 2; // Assume 2 constraints per rope
        }
    }
    
    SET_DWORD_STAT(STAT_PhysicsConstraints, CurrentMetrics.PhysicsConstraints);

    // Track memory usage
    TrackMemoryUsage();

    // Track physics timing
    PhysicsTimeAccumulator += CurrentMetrics.PhysicsTime;
    PhysicsFrameCounter++;
    
    if (PhysicsFrameCounter >= 60) // Average over 60 frames
    {
        CurrentMetrics.PhysicsTime = PhysicsTimeAccumulator / PhysicsFrameCounter;
        PhysicsTimeAccumulator = 0.0f;
        PhysicsFrameCounter = 0;
    }

    // Update rendering metrics
    if (CachedWorld && CachedWorld->GetGameViewport())
    {
        // These would typically come from the rendering subsystem
        CurrentMetrics.DrawCalls = 2000; // Placeholder
        CurrentMetrics.Triangles = 500000; // Placeholder
    }

    // Update network metrics
    CurrentMetrics.NetworkBandwidthKBps = 32.0f; // Placeholder
    CurrentMetrics.NetworkLatencyMs = 50.0f; // Placeholder
}

void UClimbingPerformanceManager::UpdateLODSystem(float DeltaTime)
{
    LastLODUpdate += DeltaTime;
    if (LastLODUpdate >= LODUpdateInterval)
    {
        UpdateAllObjectLODs();
        LastLODUpdate = 0.0f;
    }
}

void UClimbingPerformanceManager::CheckPerformanceThresholds()
{
    // Check for performance warnings
    bool bWarning = false;
    bool bCritical = false;

    if (CurrentMetrics.CurrentFPS < PerformanceTargets.TargetFPS * 0.8f)
    {
        bWarning = true;
    }

    if (CurrentMetrics.CurrentFPS < PerformanceTargets.MinimumFPS)
    {
        bCritical = true;
    }

    if (CurrentMetrics.AverageFrameTime > PerformanceTargets.MaxFrameTimeMs * 1.2f)
    {
        bWarning = true;
    }

    if (CurrentMetrics.PhysicsTime > PerformanceTargets.MaxPhysicsTimeMs)
    {
        bWarning = true;
    }

    if (CurrentMetrics.MemoryUsageMB > PerformanceTargets.MaxMemoryUsageMB)
    {
        bWarning = true;
    }

    if (bCritical)
    {
        OnPerformanceCritical.Broadcast();
        UE_LOG(LogTemp, Error, TEXT("CRITICAL PERFORMANCE: FPS: %.1f, Frame Time: %.1fms"), 
               CurrentMetrics.CurrentFPS, CurrentMetrics.AverageFrameTime);
    }
    else if (bWarning)
    {
        OnPerformanceWarning.Broadcast();
        UE_LOG(LogTemp, Warning, TEXT("Performance Warning: FPS: %.1f, Frame Time: %.1fms"), 
               CurrentMetrics.CurrentFPS, CurrentMetrics.AverageFrameTime);
    }
}

EPerformanceLOD UClimbingPerformanceManager::CalculateLODFromDistance(float Distance) const
{
    if (Distance <= LODDistances.UltraDistance)
        return EPerformanceLOD::Ultra;
    else if (Distance <= LODDistances.HighDistance)
        return EPerformanceLOD::High;
    else if (Distance <= LODDistances.MediumDistance)
        return EPerformanceLOD::Medium;
    else if (Distance <= LODDistances.LowDistance)
        return EPerformanceLOD::Low;
    else if (Distance <= LODDistances.MinimalDistance)
        return EPerformanceLOD::Minimal;
    else
        return EPerformanceLOD::Disabled;
}

FVector UClimbingPerformanceManager::GetAverageViewerLocation() const
{
    if (!CachedWorld)
        return FVector::ZeroVector;

    FVector TotalLocation = FVector::ZeroVector;
    int32 PlayerCount = 0;

    for (FConstPlayerControllerIterator Iterator = CachedWorld->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            TotalLocation += PC->GetPawn()->GetActorLocation();
            PlayerCount++;
        }
    }

    return PlayerCount > 0 ? TotalLocation / PlayerCount : FVector::ZeroVector;
}

void UClimbingPerformanceManager::ApplyRopeLOD(UAdvancedRopeComponent* Rope, EPerformanceLOD LOD)
{
    if (!IsValid(Rope) || !IsValid(Rope->CableComponent))
        return;

    // Adjust rope segments based on LOD
    int32 Segments = PhysicsSettings.MediumRopeSegments;
    float UpdateRate = PhysicsSettings.MediumUpdateRate;
    int32 SolverIterations = PhysicsSettings.MediumSolverIterations;

    switch (LOD)
    {
        case EPerformanceLOD::Ultra:
            Segments = PhysicsSettings.UltraRopeSegments;
            UpdateRate = PhysicsSettings.UltraUpdateRate;
            SolverIterations = PhysicsSettings.UltraSolverIterations;
            break;
        case EPerformanceLOD::High:
            Segments = PhysicsSettings.HighRopeSegments;
            UpdateRate = PhysicsSettings.HighUpdateRate;
            SolverIterations = PhysicsSettings.HighSolverIterations;
            break;
        case EPerformanceLOD::Medium:
            Segments = PhysicsSettings.MediumRopeSegments;
            UpdateRate = PhysicsSettings.MediumUpdateRate;
            SolverIterations = PhysicsSettings.MediumSolverIterations;
            break;
        case EPerformanceLOD::Low:
            Segments = PhysicsSettings.LowRopeSegments;
            UpdateRate = PhysicsSettings.LowUpdateRate;
            SolverIterations = PhysicsSettings.LowSolverIterations;
            break;
        case EPerformanceLOD::Minimal:
            Segments = PhysicsSettings.MinimalRopeSegments;
            UpdateRate = PhysicsSettings.MinimalUpdateRate;
            SolverIterations = PhysicsSettings.MinimalSolverIterations;
            break;
        case EPerformanceLOD::Disabled:
            Rope->CableComponent->SetSimulatePhysics(false);
            return;
    }

    // Apply settings
    Rope->CableComponent->NumSegments = Segments;
    Rope->CableComponent->SolverIterations = SolverIterations;
    Rope->CableComponent->SetSimulatePhysics(true);
    
    // Adjust update frequency
    if (Rope->GetOwner())
    {
        Rope->GetOwner()->SetActorTickInterval(1.0f / UpdateRate);
    }
}

void UClimbingPerformanceManager::ApplyToolLOD(AClimbingToolBase* Tool, EPerformanceLOD LOD)
{
    if (!IsValid(Tool))
        return;

    switch (LOD)
    {
        case EPerformanceLOD::Ultra:
        case EPerformanceLOD::High:
            Tool->SetActorTickInterval(0.0f); // Full rate
            Tool->SetActorHiddenInGame(false);
            break;
        case EPerformanceLOD::Medium:
            Tool->SetActorTickInterval(0.1f); // 10 FPS
            Tool->SetActorHiddenInGame(false);
            break;
        case EPerformanceLOD::Low:
            Tool->SetActorTickInterval(0.2f); // 5 FPS
            Tool->SetActorHiddenInGame(false);
            break;
        case EPerformanceLOD::Minimal:
            Tool->SetActorTickInterval(0.5f); // 2 FPS
            Tool->SetActorHiddenInGame(false);
            break;
        case EPerformanceLOD::Disabled:
            Tool->SetActorTickEnabled(false);
            Tool->SetActorHiddenInGame(true);
            break;
    }
}

void UClimbingPerformanceManager::ApplyClimberLOD(UAdvancedClimbingComponent* Climber, EPerformanceLOD LOD)
{
    if (!IsValid(Climber))
        return;

    // Climbers are always important, but we can adjust update frequencies for non-local players
    float TickInterval = 0.0f;
    
    switch (LOD)
    {
        case EPerformanceLOD::Ultra:
        case EPerformanceLOD::High:
            TickInterval = 0.0f; // Full rate
            break;
        case EPerformanceLOD::Medium:
            TickInterval = 0.033f; // 30 FPS
            break;
        case EPerformanceLOD::Low:
            TickInterval = 0.05f; // 20 FPS
            break;
        case EPerformanceLOD::Minimal:
            TickInterval = 0.1f; // 10 FPS
            break;
        case EPerformanceLOD::Disabled:
            TickInterval = 0.2f; // 5 FPS minimum for climbers
            break;
    }

    Climber->SetComponentTickInterval(TickInterval);
}

void UClimbingPerformanceManager::TrackMemoryUsage()
{
    // Get memory statistics
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    CurrentMetrics.MemoryUsageMB = static_cast<float>(MemStats.UsedPhysical) / (1024.0f * 1024.0f);
}

void UClimbingPerformanceManager::OptimizeTextureMemory()
{
    // Force texture LOD optimization
    // This would typically involve streaming lower resolution textures for distant objects
}

void UClimbingPerformanceManager::OptimizeMeshMemory()
{
    // Force mesh LOD optimization
    // This would typically involve using lower poly meshes for distant objects
}

void UClimbingPerformanceManager::AdjustGlobalQuality(float QualityDelta)
{
    // Adjust LOD distances based on quality delta
    SetGlobalLODBias(QualityDelta);
    
    // Adjust physics quality
    if (QualityDelta < 0.0f)
    {
        // Reduce physics quality
        for (UAdvancedRopeComponent* Rope : TrackedRopes)
        {
            if (IsValid(Rope) && IsValid(Rope->CableComponent))
            {
                Rope->CableComponent->SolverIterations = FMath::Max(1, Rope->CableComponent->SolverIterations - 1);
                Rope->CableComponent->NumSegments = FMath::Max(4, static_cast<int32>(Rope->CableComponent->NumSegments * 0.8f));
            }
        }
    }
    else if (QualityDelta > 0.0f)
    {
        // Increase physics quality (carefully)
        for (UAdvancedRopeComponent* Rope : TrackedRopes)
        {
            if (IsValid(Rope) && IsValid(Rope->CableComponent))
            {
                Rope->CableComponent->SolverIterations = FMath::Min(8, Rope->CableComponent->SolverIterations + 1);
                Rope->CableComponent->NumSegments = FMath::Min(64, static_cast<int32>(Rope->CableComponent->NumSegments * 1.1f));
            }
        }
    }
}

void UClimbingPerformanceManager::RecordPerformanceData()
{
    // Record performance data for analytics
    // This could be sent to a telemetry service for analysis
}

void UClimbingPerformanceManager::AnalyzePerformanceTrends()
{
    // Analyze performance trends over time
    // Could detect patterns like performance drops during certain gameplay scenarios
}

void UClimbingPerformanceManager::DispatchPhysicsUpdates()
{
    PhysicsStartTime = FPlatformTime::Seconds();
    
    // Physics updates would typically be dispatched to worker threads here
    // For now, we'll just track the timing
}

void UClimbingPerformanceManager::SynchronizePhysicsResults()
{
    PhysicsEndTime = FPlatformTime::Seconds();
    float PhysicsTimeMs = static_cast<float>((PhysicsEndTime - PhysicsStartTime) * 1000.0);
    CurrentMetrics.PhysicsTime = PhysicsTimeMs;
}