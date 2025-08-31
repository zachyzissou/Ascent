#include "ClimbingEnvironmentalHazardManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialParameterCollection.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Stats/StatsHierarchical.h"

DECLARE_CYCLE_STAT(TEXT("Environmental Hazards Update"), STAT_EnvironmentalHazardsUpdate, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Weather Effects Update"), STAT_WeatherEffectsUpdate, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Physics Hazards Update"), STAT_PhysicsHazardsUpdate, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Hazard LOD Update"), STAT_HazardLODUpdate, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Hazard Network Sync"), STAT_HazardNetworkSync, STATGROUP_Game);

void UClimbingEnvironmentalHazardManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingEnvironmentalHazardManager: Initializing environmental hazard optimization system"));
    
    // Initialize performance metrics
    CurrentMetrics = FHazardPerformanceMetrics();
    
    // Set up default quality levels based on hardware tier
    InitializeSubsystemReferences();
    RegisterWithPerformanceManager();
    
    // Initialize object pools for memory optimization
    for (int32 HazardTypeIndex = 0; HazardTypeIndex < (int32)EEnvironmentalHazardType::Sandstorm + 1; ++HazardTypeIndex)
    {
        EEnvironmentalHazardType HazardType = (EEnvironmentalHazardType)HazardTypeIndex;
        ObjectPools.Add(HazardType, TArray<UObject*>());
        
        // Pre-populate pools for common hazards
        if (bEnableMemoryOptimization)
        {
            int32 PoolSize = GetPoolSizeForHazardType(HazardType);
            for (int32 i = 0; i < PoolSize; ++i)
            {
                UObject* PooledObject = CreatePooledObjectForHazardType(HazardType);
                if (PooledObject)
                {
                    ObjectPools[HazardType].Add(PooledObject);
                }
            }
        }
    }
    
    // Initialize adaptive quality system
    CurrentGlobalQuality = EHazardQuality::Medium;
    CurrentAdaptiveQuality = EHazardQuality::Medium;
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingEnvironmentalHazardManager: Successfully initialized with %d hazard types"), ObjectPools.Num());
}

void UClimbingEnvironmentalHazardManager::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("ClimbingEnvironmentalHazardManager: Shutting down"));
    
    // Clean up all active hazards
    DestroyAllHazards();
    
    // Clean up object pools
    for (auto& PoolPair : ObjectPools)
    {
        for (UObject* PooledObject : PoolPair.Value)
        {
            if (IsValid(PooledObject))
            {
                PooledObject->MarkPendingKill();
            }
        }
        PoolPair.Value.Empty();
    }
    ObjectPools.Empty();
    
    Super::Deinitialize();
}

void UClimbingEnvironmentalHazardManager::Tick(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_EnvironmentalHazardsUpdate);
    
    // Update all active hazards
    UpdateActiveHazards(DeltaTime);
    
    // Update performance monitoring
    UpdatePerformanceMetrics(DeltaTime);
    
    // Update adaptive quality system
    if (bEnableAdaptiveQuality)
    {
        UpdateAdaptiveQuality(DeltaTime);
    }
    
    // Update network synchronization
    if (bEnableNetworkOptimization)
    {
        UpdateNetworkSync(DeltaTime);
    }
    
    // Update benchmark if running
    if (bIsRunningBenchmark)
    {
        UpdateBenchmark(DeltaTime);
    }
    
    // Check performance budgets
    CheckPerformanceBudgets();
}

int32 UClimbingEnvironmentalHazardManager::CreateHazard(EEnvironmentalHazardType HazardType, const FVector& Location, float Radius, float Duration, EHazardIntensity Intensity)
{
    // Check if we're at the maximum active hazard limit
    if (ActiveHazards.Num() >= MaxActiveHazards)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot create hazard: Maximum active hazards reached (%d)"), MaxActiveHazards);
        return -1;
    }
    
    // Create new hazard instance
    FActiveHazardInstance NewHazard;
    NewHazard.HazardType = HazardType;
    NewHazard.CurrentIntensity = Intensity;
    NewHazard.CurrentQuality = CurrentGlobalQuality;
    NewHazard.Location = Location;
    NewHazard.Radius = Radius;
    NewHazard.Duration = Duration;
    NewHazard.ElapsedTime = 0.0f;
    NewHazard.LastUpdateTime = 0.0f;
    NewHazard.NetworkID = NextHazardID;
    NewHazard.bIsVisible = true;
    NewHazard.DistanceToNearestPlayer = GetDistanceToNearestPlayer(Location);
    
    // Calculate initial LOD based on distance
    NewHazard.CurrentQuality = CalculateHazardLOD(NewHazard);
    
    // Create visual effects (weather particles)
    if (IsWeatherHazard(HazardType))
    {
        NewHazard.ParticleComponent = CreateWeatherEffect(HazardType, Location, Intensity);
        if (NewHazard.ParticleComponent.IsValid())
        {
            AdjustWeatherQuality(NewHazard, NewHazard.CurrentQuality);
        }
    }
    
    // Create physics objects for physical hazards
    if (IsPhysicsHazard(HazardType))
    {
        TArray<AActor*> PhysicsActors = CreatePhysicsObjects(HazardType, Location, Radius, Intensity);
        for (AActor* Actor : PhysicsActors)
        {
            NewHazard.PhysicsActors.Add(Actor);
        }
        AdjustPhysicsQuality(NewHazard, NewHazard.CurrentQuality);
    }
    
    // Calculate initial memory usage
    NewHazard.MemoryUsage = CalculateHazardMemoryUsage(NewHazard);
    CurrentMemoryUsage += NewHazard.MemoryUsage;
    
    // Add to active hazards
    int32 HazardID = NextHazardID++;
    ActiveHazards.Add(HazardID, NewHazard);
    
    // Track memory usage
    if (MemoryTracker)
    {
        MemoryTracker->TrackAllocation(EMemoryCategory::ProceduralContent, NewHazard.MemoryUsage);
    }
    
    // Network synchronization
    if (bEnableNetworkOptimization && IsHazardNetworkRelevant(NewHazard))
    {
        SyncHazardToNetwork(NewHazard);
    }
    
    // Fire event
    OnHazardCreated.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Created hazard %d of type %s at location %s with intensity %d"), 
           HazardID, *GetHazardTypeString(HazardType), *Location.ToString(), (int32)Intensity);
    
    return HazardID;
}

void UClimbingEnvironmentalHazardManager::DestroyHazard(int32 HazardID)
{
    FActiveHazardInstance* Hazard = ActiveHazards.Find(HazardID);
    if (!Hazard)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot destroy hazard %d: Not found"), HazardID);
        return;
    }
    
    // Track memory deallocation
    if (MemoryTracker)
    {
        MemoryTracker->TrackDeallocation(EMemoryCategory::ProceduralContent, Hazard->MemoryUsage);
    }
    CurrentMemoryUsage -= Hazard->MemoryUsage;
    
    // Clean up components
    DestroyHazardComponents(*Hazard);
    
    // Remove from active hazards
    ActiveHazards.Remove(HazardID);
    
    // Fire event
    OnHazardDestroyed.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Destroyed hazard %d"), HazardID);
}

void UClimbingEnvironmentalHazardManager::DestroyAllHazards()
{
    TArray<int32> HazardIDs;
    ActiveHazards.GetKeys(HazardIDs);
    
    for (int32 HazardID : HazardIDs)
    {
        DestroyHazard(HazardID);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Destroyed all %d active hazards"), HazardIDs.Num());
}

void UClimbingEnvironmentalHazardManager::UpdateActiveHazards(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_EnvironmentalHazardsUpdate);
    
    TArray<int32> HazardsToDestroy;
    
    for (auto& HazardPair : ActiveHazards)
    {
        FActiveHazardInstance& Hazard = HazardPair.Value;
        
        // Update elapsed time
        Hazard.ElapsedTime += DeltaTime;
        
        // Check if hazard has expired
        if (Hazard.Duration > 0.0f && Hazard.ElapsedTime >= Hazard.Duration)
        {
            HazardsToDestroy.Add(HazardPair.Key);
            continue;
        }
        
        // Update distance to players
        Hazard.DistanceToNearestPlayer = GetDistanceToNearestPlayer(Hazard.Location);
        
        // Update visibility based on distance and frustum
        Hazard.bIsVisible = IsHazardVisible(Hazard);
        
        // Skip expensive updates for invisible hazards
        if (!Hazard.bIsVisible && !IsPhysicsHazard(Hazard.HazardType))
        {
            continue;
        }
        
        // Calculate frame time for this hazard
        double HazardStartTime = FPlatformTime::Seconds();
        
        // Update weather effects
        if (IsWeatherHazard(Hazard.HazardType) && Hazard.ParticleComponent.IsValid())
        {
            SCOPE_CYCLE_COUNTER(STAT_WeatherEffectsUpdate);
            UpdateWeatherEffect(Hazard, DeltaTime);
        }
        
        // Update physics objects
        if (IsPhysicsHazard(Hazard.HazardType))
        {
            SCOPE_CYCLE_COUNTER(STAT_PhysicsHazardsUpdate);
            UpdatePhysicsHazard(Hazard, DeltaTime);
        }
        
        // Calculate frame time spent on this hazard
        Hazard.CurrentFrameTime = (FPlatformTime::Seconds() - HazardStartTime) * 1000.0f;
        
        // Update LOD if necessary
        if (ShouldUpdateHazardLOD(Hazard))
        {
            SCOPE_CYCLE_COUNTER(STAT_HazardLODUpdate);
            EHazardQuality NewQuality = CalculateHazardLOD(Hazard);
            if (NewQuality != Hazard.CurrentQuality)
            {
                ApplyHazardLOD(Hazard, NewQuality);
            }
        }
        
        Hazard.LastUpdateTime = FPlatformTime::Seconds();
    }
    
    // Destroy expired hazards
    for (int32 HazardID : HazardsToDestroy)
    {
        DestroyHazard(HazardID);
    }
}

void UClimbingEnvironmentalHazardManager::OptimizeWeatherEffects()
{
    UE_LOG(LogTemp, Log, TEXT("Optimizing weather effects for %d active hazards"), ActiveHazards.Num());
    
    int32 OptimizedCount = 0;
    float TotalMemorySaved = 0.0f;
    
    for (auto& HazardPair : ActiveHazards)
    {
        FActiveHazardInstance& Hazard = HazardPair.Value;
        
        if (!IsWeatherHazard(Hazard.HazardType) || !Hazard.ParticleComponent.IsValid())
        {
            continue;
        }
        
        float OriginalMemory = Hazard.MemoryUsage;
        
        // Optimize based on distance and visibility
        EHazardQuality OptimalQuality = CalculateOptimalWeatherQuality(Hazard);
        if (OptimalQuality != Hazard.CurrentQuality)
        {
            AdjustWeatherQuality(Hazard, OptimalQuality);
            OptimizedCount++;
        }
        
        // Optimize particle parameters
        OptimizeParticleSystem(Hazard);
        
        TotalMemorySaved += OriginalMemory - Hazard.MemoryUsage;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Weather optimization complete: %d hazards optimized, %.2f MB memory saved"), 
           OptimizedCount, TotalMemorySaved);
}

void UClimbingEnvironmentalHazardManager::OptimizePhysicsHazards()
{
    UE_LOG(LogTemp, Log, TEXT("Optimizing physics hazards for %d active hazards"), ActiveHazards.Num());
    
    int32 OptimizedCount = 0;
    int32 TotalObjectsOptimized = 0;
    
    for (auto& HazardPair : ActiveHazards)
    {
        FActiveHazardInstance& Hazard = HazardPair.Value;
        
        if (!IsPhysicsHazard(Hazard.HazardType))
        {
            continue;
        }
        
        // Calculate optimal physics quality
        EHazardQuality OptimalQuality = CalculateOptimalPhysicsQuality(Hazard);
        if (OptimalQuality != Hazard.CurrentQuality)
        {
            AdjustPhysicsQuality(Hazard, OptimalQuality);
            OptimizedCount++;
        }
        
        // Optimize individual physics objects
        int32 ObjectsBefore = Hazard.PhysicsActors.Num();
        OptimizePhysicsObjects(Hazard);
        TotalObjectsOptimized += ObjectsBefore - Hazard.PhysicsActors.Num();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Physics optimization complete: %d hazards optimized, %d objects culled"), 
           OptimizedCount, TotalObjectsOptimized);
}

void UClimbingEnvironmentalHazardManager::EnableAdaptiveQuality(bool bEnable)
{
    bEnableAdaptiveQuality = bEnable;
    
    if (bEnable)
    {
        UE_LOG(LogTemp, Log, TEXT("Adaptive quality system enabled"));
        
        // Force an immediate quality assessment
        CalculateOptimalQuality();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Adaptive quality system disabled"));
        
        // Reset to default quality
        CurrentAdaptiveQuality = CurrentGlobalQuality;
    }
}

void UClimbingEnvironmentalHazardManager::UpdateAdaptiveQuality(float DeltaTime)
{
    AdaptiveQualityTimer += DeltaTime;
    
    // Check performance every interval
    if (AdaptiveQualityTimer >= PerformanceCheckInterval)
    {
        AdaptiveQualityTimer = 0.0f;
        
        float CurrentFPS = FrameRateManager ? FrameRateManager->GetCurrentMetrics().CurrentFPS : (1.0f / DeltaTime);
        float TargetFPS = DefaultLODSettings.TargetFPS;
        float MinimumFPS = DefaultLODSettings.MinimumFPS;
        
        bool bShouldIncreaseQuality = CurrentFPS > TargetFPS + 5.0f; // 5 FPS buffer
        bool bShouldDecreaseQuality = CurrentFPS < MinimumFPS;
        
        if (bShouldDecreaseQuality && CurrentAdaptiveQuality > EHazardQuality::Performance)
        {
            // Decrease quality to improve performance
            int32 CurrentLevel = (int32)CurrentAdaptiveQuality;
            CurrentAdaptiveQuality = (EHazardQuality)(CurrentLevel + 1);
            
            ApplyGlobalQualityChange();
            OnQualityAdjusted.Broadcast();
            
            UE_LOG(LogTemp, Warning, TEXT("Adaptive quality decreased to %d due to low FPS (%.1f)"), 
                   (int32)CurrentAdaptiveQuality, CurrentFPS);
        }
        else if (bShouldIncreaseQuality && CurrentAdaptiveQuality > EHazardQuality::Cinematic)
        {
            // Increase quality when performance allows
            int32 CurrentLevel = (int32)CurrentAdaptiveQuality;
            CurrentAdaptiveQuality = (EHazardQuality)(CurrentLevel - 1);
            
            ApplyGlobalQualityChange();
            OnQualityAdjusted.Broadcast();
            
            UE_LOG(LogTemp, Log, TEXT("Adaptive quality increased to %d due to good FPS (%.1f)"), 
                   (int32)CurrentAdaptiveQuality, CurrentFPS);
        }
    }
}

FHazardPerformanceMetrics UClimbingEnvironmentalHazardManager::GetPerformanceMetrics() const
{
    return CurrentMetrics;
}

void UClimbingEnvironmentalHazardManager::UpdatePerformanceMetrics(float DeltaTime)
{
    // Reset metrics
    CurrentMetrics.TotalFrameTimeMs = 0.0f;
    CurrentMetrics.WeatherEffectsFrameTimeMs = 0.0f;
    CurrentMetrics.PhysicsHazardsFrameTimeMs = 0.0f;
    CurrentMetrics.NetworkSyncFrameTimeMs = 0.0f;
    CurrentMetrics.TotalMemoryUsageMB = CurrentMemoryUsage;
    CurrentMetrics.ActiveHazardCount = ActiveHazards.Num();
    CurrentMetrics.TotalParticleCount = 0;
    CurrentMetrics.ActivePhysicsObjects = 0;
    
    // Accumulate metrics from active hazards
    for (const auto& HazardPair : ActiveHazards)
    {
        const FActiveHazardInstance& Hazard = HazardPair.Value;
        
        CurrentMetrics.TotalFrameTimeMs += Hazard.CurrentFrameTime;
        
        if (IsWeatherHazard(Hazard.HazardType) && Hazard.ParticleComponent.IsValid())
        {
            CurrentMetrics.WeatherEffectsFrameTimeMs += Hazard.CurrentFrameTime;
            CurrentMetrics.TotalParticleCount += GetParticleCount(Hazard);
        }
        
        if (IsPhysicsHazard(Hazard.HazardType))
        {
            CurrentMetrics.PhysicsHazardsFrameTimeMs += Hazard.CurrentFrameTime;
            CurrentMetrics.ActivePhysicsObjects += Hazard.PhysicsActors.Num();
        }
    }
    
    // Calculate performance score (0-100)
    float FrameTimeBudgetUsage = CurrentMetrics.TotalFrameTimeMs / PerformanceBudgets.TotalFrameTimeBudgetMs;
    float MemoryBudgetUsage = CurrentMetrics.TotalMemoryUsageMB / PerformanceBudgets.TotalMemoryBudgetMB;
    
    CurrentMetrics.bBudgetExceeded = (FrameTimeBudgetUsage > 1.0f) || (MemoryBudgetUsage > 1.0f);
    
    // Performance score calculation
    float FrameTimeScore = FMath::Clamp(1.0f - FrameTimeBudgetUsage, 0.0f, 1.0f) * 50.0f;
    float MemoryScore = FMath::Clamp(1.0f - MemoryBudgetUsage, 0.0f, 1.0f) * 50.0f;
    CurrentMetrics.PerformanceScore = FrameTimeScore + MemoryScore;
    
    // Update integration systems
    UpdatePerformanceManagerMetrics();
}

void UClimbingEnvironmentalHazardManager::CheckPerformanceBudgets()
{
    if (CurrentMetrics.bBudgetExceeded)
    {
        if (!bPerformanceOptimizationActive)
        {
            UE_LOG(LogTemp, Warning, TEXT("Performance budget exceeded - triggering optimization"));
            TriggerPerformanceOptimization();
            OnBudgetExceeded.Broadcast();
        }
    }
    else
    {
        // Reset optimization state when back within budget
        if (bPerformanceOptimizationActive)
        {
            bPerformanceOptimizationActive = false;
            PerformanceOptimizationTimer = 0.0f;
        }
    }
}

void UClimbingEnvironmentalHazardManager::TriggerPerformanceOptimization()
{
    bPerformanceOptimizationActive = true;
    PerformanceOptimizationTimer = 0.0f;
    
    // Immediate optimizations
    OptimizeWeatherEffects();
    OptimizePhysicsHazards();
    OptimizeMemoryUsage();
    
    // Force lower quality for distant hazards
    for (auto& HazardPair : ActiveHazards)
    {
        FActiveHazardInstance& Hazard = HazardPair.Value;
        
        if (Hazard.DistanceToNearestPlayer > DefaultLODSettings.MediumDistance)
        {
            ApplyHazardLOD(Hazard, EHazardQuality::Performance);
        }
    }
    
    OnPerformanceWarning.Broadcast();
}

EHazardQuality UClimbingEnvironmentalHazardManager::CalculateHazardLOD(const FActiveHazardInstance& Hazard) const
{
    // Apply global LOD bias
    float Distance = Hazard.DistanceToNearestPlayer * GlobalLODBias;
    
    // Performance-based scaling
    if (bEnableAdaptiveQuality)
    {
        // Further reduce quality if we're under performance pressure
        if (CurrentMetrics.bBudgetExceeded)
        {
            Distance *= 0.7f; // Treat objects as if they're further away
        }
    }
    
    // Calculate quality based on distance thresholds
    if (Distance <= DefaultLODSettings.CinematicDistance)
    {
        return EHazardQuality::Cinematic;
    }
    else if (Distance <= DefaultLODSettings.HighDistance)
    {
        return EHazardQuality::High;
    }
    else if (Distance <= DefaultLODSettings.MediumDistance)
    {
        return EHazardQuality::Medium;
    }
    else if (Distance <= DefaultLODSettings.LowDistance)
    {
        return EHazardQuality::Low;
    }
    else if (Distance <= DefaultLODSettings.CullingDistance)
    {
        return EHazardQuality::Performance;
    }
    else
    {
        return EHazardQuality::Disabled;
    }
}

void UClimbingEnvironmentalHazardManager::ApplyHazardLOD(FActiveHazardInstance& Hazard, EHazardQuality NewQuality)
{
    if (Hazard.CurrentQuality == NewQuality)
    {
        return;
    }
    
    EHazardQuality OldQuality = Hazard.CurrentQuality;
    Hazard.CurrentQuality = NewQuality;
    
    // Apply quality changes to components
    if (IsWeatherHazard(Hazard.HazardType))
    {
        AdjustWeatherQuality(Hazard, NewQuality);
    }
    
    if (IsPhysicsHazard(Hazard.HazardType))
    {
        AdjustPhysicsQuality(Hazard, NewQuality);
    }
    
    // Update memory usage
    float OldMemoryUsage = Hazard.MemoryUsage;
    Hazard.MemoryUsage = CalculateHazardMemoryUsage(Hazard);
    
    CurrentMemoryUsage += (Hazard.MemoryUsage - OldMemoryUsage);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Hazard LOD changed from %d to %d (Distance: %.0f)"), 
           (int32)OldQuality, (int32)NewQuality, Hazard.DistanceToNearestPlayer);
}

float UClimbingEnvironmentalHazardManager::GetDistanceToNearestPlayer(const FVector& Location) const
{
    float MinDistance = MAX_FLT;
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return MinDistance;
    }
    
    // Find all player controllers
    for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            float Distance = FVector::Dist(Location, PC->GetPawn()->GetActorLocation());
            MinDistance = FMath::Min(MinDistance, Distance);
        }
    }
    
    return MinDistance == MAX_FLT ? 0.0f : MinDistance;
}

void UClimbingEnvironmentalHazardManager::OptimizeMemoryUsage()
{
    if (!bEnableMemoryOptimization)
    {
        return;
    }
    
    float MemorySaved = 0.0f;
    
    // Optimize individual hazards
    for (auto& HazardPair : ActiveHazards)
    {
        FActiveHazardInstance& Hazard = HazardPair.Value;
        float OriginalMemory = Hazard.MemoryUsage;
        OptimizeHazardMemory(Hazard);
        MemorySaved += OriginalMemory - Hazard.MemoryUsage;
    }
    
    // Clean up unused objects
    CleanupUnusedObjects();
    
    // Force garbage collection if memory usage is critical
    if (CurrentMemoryUsage > PerformanceBudgets.TotalMemoryBudgetMB * 0.9f)
    {
        if (MemoryTracker)
        {
            MemoryTracker->RunGarbageCollection(true);
        }
        else
        {
            GEngine->ForceGarbageCollection(true);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Memory optimization complete: %.2f MB saved"), MemorySaved);
}

void UClimbingEnvironmentalHazardManager::InitializeSubsystemReferences()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    // Get performance manager reference
    PerformanceManager = World->GetSubsystem<UClimbingPerformanceManager>();
    if (!PerformanceManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClimbingPerformanceManager not found - some optimizations may be limited"));
    }
    
    // Get memory tracker reference
    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        MemoryTracker = GameInstance->GetSubsystem<UClimbingMemoryTracker>();
        if (!MemoryTracker)
        {
            UE_LOG(LogTemp, Warning, TEXT("ClimbingMemoryTracker not found - memory tracking may be limited"));
        }
    }
    
    // Get frame rate manager reference
    FrameRateManager = World->GetSubsystem<UClimbingFrameRateManager>();
    if (!FrameRateManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClimbingFrameRateManager not found - adaptive quality may be limited"));
    }
}

void UClimbingEnvironmentalHazardManager::RegisterWithPerformanceManager()
{
    if (!PerformanceManager)
    {
        return;
    }
    
    // Register our performance metrics with the central performance manager
    // This allows the performance manager to consider environmental hazards in its optimization decisions
    
    UE_LOG(LogTemp, Log, TEXT("Registered with ClimbingPerformanceManager for integrated optimization"));
}

void UClimbingEnvironmentalHazardManager::UpdatePerformanceManagerMetrics()
{
    if (!PerformanceManager)
    {
        return;
    }
    
    // Update the performance manager with our current metrics
    // This allows for coordinated optimization across all systems
    
    FPerformanceMetrics GlobalMetrics = PerformanceManager->GetCurrentMetrics();
    
    // Add our contribution to the global metrics
    // Note: This would require extending the FPerformanceMetrics struct to include environmental hazard data
}

// Helper function implementations
bool UClimbingEnvironmentalHazardManager::IsWeatherHazard(EEnvironmentalHazardType HazardType) const
{
    return HazardType == EEnvironmentalHazardType::Rain ||
           HazardType == EEnvironmentalHazardType::Snow ||
           HazardType == EEnvironmentalHazardType::DustStorm ||
           HazardType == EEnvironmentalHazardType::Fog ||
           HazardType == EEnvironmentalHazardType::WindGusts ||
           HazardType == EEnvironmentalHazardType::Lightning ||
           HazardType == EEnvironmentalHazardType::Hail ||
           HazardType == EEnvironmentalHazardType::Sandstorm;
}

bool UClimbingEnvironmentalHazardManager::IsPhysicsHazard(EEnvironmentalHazardType HazardType) const
{
    return HazardType == EEnvironmentalHazardType::RockSlide ||
           HazardType == EEnvironmentalHazardType::Avalanche;
}

FString UClimbingEnvironmentalHazardManager::GetHazardTypeString(EEnvironmentalHazardType HazardType) const
{
    switch (HazardType)
    {
        case EEnvironmentalHazardType::Rain: return TEXT("Rain");
        case EEnvironmentalHazardType::Snow: return TEXT("Snow");
        case EEnvironmentalHazardType::DustStorm: return TEXT("Dust Storm");
        case EEnvironmentalHazardType::Fog: return TEXT("Fog");
        case EEnvironmentalHazardType::RockSlide: return TEXT("Rock Slide");
        case EEnvironmentalHazardType::Avalanche: return TEXT("Avalanche");
        case EEnvironmentalHazardType::WindGusts: return TEXT("Wind Gusts");
        case EEnvironmentalHazardType::Lightning: return TEXT("Lightning");
        case EEnvironmentalHazardType::Hail: return TEXT("Hail");
        case EEnvironmentalHazardType::Sandstorm: return TEXT("Sandstorm");
        default: return TEXT("Unknown");
    }
}