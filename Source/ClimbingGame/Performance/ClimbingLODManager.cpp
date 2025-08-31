#include "ClimbingLODManager.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CollisionQueryParams.h"

void UClimbingLODManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Initialize default settings
    DefaultLODSettings.LOD0_Distance = 500.0f;
    DefaultLODSettings.LOD1_Distance = 1500.0f;
    DefaultLODSettings.LOD2_Distance = 3000.0f;
    DefaultLODSettings.LOD3_Distance = 5000.0f;
    DefaultLODSettings.LOD4_Distance = 8000.0f;
    DefaultLODSettings.CullDistance = 12000.0f;
    DefaultLODSettings.LODBias = 1.0f;
    
    DefaultCullingSettings.CullingMethod = ECullingMethod::Combined;
    DefaultCullingSettings.MaxRenderDistance = 15000.0f;
    DefaultCullingSettings.bEnableFrustumCulling = true;
    DefaultCullingSettings.bEnableOcclusionCulling = true;
    DefaultCullingSettings.MaxObjectsPerFrame = 1000;
    
    // Set update frequency
    SetLODUpdateFrequency(UpdateFrequency);
    
    // Reserve memory for managed objects
    ManagedObjects.Reserve(1000);
    ViewerLocations.Reserve(8); // Support up to 8 viewers
    
    // Initialize stats
    CurrentStats = FLODPerformanceStats();
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingLODManager initialized: MaxObjects=%d, UpdateFreq=%.1fHz"),
           MaxObjectsPerBatch, 1.0f / LODUpdateInterval);
}

void UClimbingLODManager::Deinitialize()
{
    ManagedObjects.Empty();
    ViewerLocations.Empty();
    
    Super::Deinitialize();
}

bool UClimbingLODManager::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UClimbingLODManager::TickLODManager(float DeltaTime)
{
    if (!bEnableLODSystem)
        return;
        
    SCOPE_CYCLE_COUNTER_UOBJECT(TickLODManager, this);
    
    // Update viewer locations
    UpdateViewerLocations();
    
    // Update performance tracking
    UpdatePerformanceStats(DeltaTime);
    
    // Check if we should update LODs this frame
    LastLODUpdate += DeltaTime;
    if (LastLODUpdate >= LODUpdateInterval)
    {
        // Remove invalid objects
        RemoveInvalidObjects();
        
        // Update LOD levels
        if (DefaultCullingSettings.bUseBatchProcessing)
        {
            BatchUpdateObjects(DeltaTime);
        }
        else
        {
            UpdateLODLevels(DeltaTime);
        }
        
        LastLODUpdate = 0.0f;
    }
    
    // Check performance targets
    if (bEnableAdaptiveLOD)
    {
        CheckPerformanceTargets();
    }
    
    // Draw debug info
    if (bShowDebugInfo)
    {
        DrawDebugLODInfo();
    }
    
    if (bShowCullingDebug)
    {
        DrawDebugCullingInfo();
    }
}

void UClimbingLODManager::RegisterActor(AActor* Actor, float ImportanceScore)
{
    if (!IsValid(Actor))
        return;
        
    // Check if already registered
    if (FindManagedObject(Actor))
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor %s already registered with LOD manager"), 
               *Actor->GetName());
        return;
    }
    
    FManagedLODObject NewObject;
    NewObject.Actor = Actor;
    NewObject.Component = Actor->GetRootComponent();
    NewObject.CurrentLODLevel = 0;
    NewObject.LastUpdateTime = GetWorld()->GetTimeSeconds();
    NewObject.LastKnownLocation = Actor->GetActorLocation();
    NewObject.ImportanceScore = FMath::Clamp(ImportanceScore, 0.1f, 10.0f);
    NewObject.bIsVisible = true;
    NewObject.bIsCulled = false;
    NewObject.CustomLODSettings = DefaultLODSettings;
    NewObject.bUseCustomLODSettings = false;
    
    ManagedObjects.Add(NewObject);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Registered actor %s with LOD manager (Importance: %.1f)"),
           *Actor->GetName(), ImportanceScore);
}

void UClimbingLODManager::UnregisterActor(AActor* Actor)
{
    if (!IsValid(Actor))
        return;
        
    int32 RemovedCount = ManagedObjects.RemoveAll([Actor](const FManagedLODObject& Object)
    {
        return Object.Actor == Actor;
    });
    
    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Unregistered actor %s from LOD manager"), 
               *Actor->GetName());
    }
}

void UClimbingLODManager::RegisterComponent(UPrimitiveComponent* Component, float ImportanceScore)
{
    if (!IsValid(Component))
        return;
        
    FManagedLODObject NewObject;
    NewObject.Actor = Component->GetOwner();
    NewObject.Component = Component;
    NewObject.CurrentLODLevel = 0;
    NewObject.LastUpdateTime = GetWorld()->GetTimeSeconds();
    NewObject.LastKnownLocation = Component->GetComponentLocation();
    NewObject.ImportanceScore = FMath::Clamp(ImportanceScore, 0.1f, 10.0f);
    NewObject.bIsVisible = true;
    NewObject.bIsCulled = false;
    NewObject.CustomLODSettings = DefaultLODSettings;
    NewObject.bUseCustomLODSettings = false;
    
    ManagedObjects.Add(NewObject);
}

void UClimbingLODManager::RegisterRope(UAdvancedRopeComponent* Rope)
{
    if (!IsValid(Rope))
        return;
        
    // Ropes have high importance due to gameplay impact
    RegisterComponent(Rope, 2.0f);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Registered rope component with LOD manager"));
}

void UClimbingLODManager::RegisterTool(AClimbingToolBase* Tool)
{
    if (!IsValid(Tool))
        return;
        
    // Tools have medium-high importance
    RegisterActor(Tool, 1.5f);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Registered tool %s with LOD manager"), 
           *Tool->GetName());
}

void UClimbingLODManager::SetGlobalLODBias(float Bias)
{
    DefaultLODSettings.LODBias = FMath::Clamp(Bias, 0.1f, 5.0f);
    
    // Apply to all objects using default settings
    for (FManagedLODObject& Object : ManagedObjects)
    {
        if (!Object.bUseCustomLODSettings)
        {
            Object.CustomLODSettings.LODBias = DefaultLODSettings.LODBias;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Global LOD bias set to %.2f"), DefaultLODSettings.LODBias);
}

void UClimbingLODManager::SetLODUpdateFrequency(ELODUpdateFrequency Frequency)
{
    UpdateFrequency = Frequency;
    
    switch (Frequency)
    {
        case ELODUpdateFrequency::EveryFrame:
            LODUpdateInterval = 0.0f;
            break;
        case ELODUpdateFrequency::High:
            LODUpdateInterval = 0.1f; // 10 FPS
            break;
        case ELODUpdateFrequency::Medium:
            LODUpdateInterval = 0.2f; // 5 FPS
            break;
        case ELODUpdateFrequency::Low:
            LODUpdateInterval = 0.5f; // 2 FPS
            break;
        case ELODUpdateFrequency::VeryLow:
            LODUpdateInterval = 1.0f; // 1 FPS
            break;
    }
    
    UE_LOG(LogTemp, Log, TEXT("LOD update frequency set to %s (%.1fs interval)"), 
           *UEnum::GetValueAsString(Frequency), LODUpdateInterval);
}

void UClimbingLODManager::OptimizeForPerformance(float TargetFPS)
{
    AdaptiveLODTargetFPS = TargetFPS;
    
    // Calculate current performance and adjust LOD bias accordingly
    float CurrentFPS = 1.0f / FMath::Max(0.001f, AverageFrameTime);
    
    if (CurrentFPS < TargetFPS * 0.9f) // 10% tolerance
    {
        // Performance is below target, be more aggressive with LOD
        float PerformanceRatio = CurrentFPS / TargetFPS;
        AdaptiveLODBias = FMath::Lerp(0.5f, 1.0f, PerformanceRatio);
        
        // Also increase culling distance if performance is really bad
        if (CurrentFPS < TargetFPS * 0.7f)
        {
            DefaultCullingSettings.MaxRenderDistance *= 0.8f;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Performance optimization triggered: FPS=%.1f, Target=%.1f, LODBias=%.2f"),
               CurrentFPS, TargetFPS, AdaptiveLODBias);
    }
    else if (CurrentFPS > TargetFPS * 1.2f)
    {
        // Performance is good, we can afford better LOD
        AdaptiveLODBias = FMath::Min(1.5f, AdaptiveLODBias * 1.1f);
        
        UE_LOG(LogTemp, Log, TEXT("Performance headroom detected: FPS=%.1f, increasing LOD quality"), CurrentFPS);
    }
    
    SetGlobalLODBias(AdaptiveLODBias);
}

void UClimbingLODManager::UpdateLODLevels(float DeltaTime)
{
    if (ManagedObjects.Num() == 0 || ViewerLocations.Num() == 0)
        return;
        
    double StartTime = FPlatformTime::Seconds();
    int32 ObjectsProcessed = 0;
    
    // Process all objects
    for (FManagedLODObject& Object : ManagedObjects)
    {
        if (!Object.Actor.IsValid() && !Object.Component.IsValid())
            continue;
            
        // Update object location
        if (Object.Actor.IsValid())
        {
            Object.LastKnownLocation = Object.Actor->GetActorLocation();
        }
        else if (Object.Component.IsValid())
        {
            Object.LastKnownLocation = Object.Component->GetComponentLocation();
        }
        
        // Calculate distance to closest viewer
        Object.DistanceToViewer = GetDistanceToViewers(Object.LastKnownLocation);
        
        // Determine LOD level
        FVector ClosestViewer = GetClosestViewerLocation(Object.LastKnownLocation);
        int32 NewLODLevel = CalculateLODLevel(Object, ClosestViewer);
        
        // Check if object should be culled
        bool bShouldCull = ShouldCullObject(Object, ClosestViewer);
        
        // Apply LOD changes if needed
        if (Object.CurrentLODLevel != NewLODLevel || Object.bIsCulled != bShouldCull)
        {
            Object.CurrentLODLevel = NewLODLevel;
            Object.bIsCulled = bShouldCull;
            Object.LastUpdateTime = GetWorld()->GetTimeSeconds();
            
            // Apply the LOD
            if (Object.Actor.IsValid())
            {
                if (bShouldCull)
                {
                    Object.Actor->SetActorHiddenInGame(true);
                }
                else
                {
                    Object.Actor->SetActorHiddenInGame(false);
                    ApplyLODToActor(Object.Actor.Get(), NewLODLevel);
                }
            }
            else if (Object.Component.IsValid())
            {
                Object.Component->SetVisibility(!bShouldCull);
                if (!bShouldCull)
                {
                    ApplyLODToComponent(Object.Component.Get(), NewLODLevel);
                }
            }
        }
        
        ObjectsProcessed++;
        
        // Check time budget
        if (MaxUpdateTimeBudgetMs > 0.0f)
        {
            double CurrentTime = FPlatformTime::Seconds();
            if ((CurrentTime - StartTime) * 1000.0 > MaxUpdateTimeBudgetMs)
            {
                break;
            }
        }
    }
    
    // Update stats
    CurrentStats.ObjectsUpdatedThisFrame = ObjectsProcessed;
    CurrentStats.AverageUpdateTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
}

void UClimbingLODManager::BatchUpdateObjects(float DeltaTime)
{
    if (ManagedObjects.Num() == 0)
        return;
        
    int32 BatchSize = FMath::Min(MaxObjectsPerBatch, ManagedObjects.Num());
    int32 StartIndex = CurrentBatchIndex;
    int32 EndIndex = FMath::Min(StartIndex + BatchSize, ManagedObjects.Num());
    
    double StartTime = FPlatformTime::Seconds();
    
    // Process current batch
    for (int32 i = StartIndex; i < EndIndex; ++i)
    {
        FManagedLODObject& Object = ManagedObjects[i];
        
        if (!Object.Actor.IsValid() && !Object.Component.IsValid())
            continue;
            
        // Update object (same logic as UpdateLODLevels but for batch)
        if (Object.Actor.IsValid())
        {
            Object.LastKnownLocation = Object.Actor->GetActorLocation();
        }
        else if (Object.Component.IsValid())
        {
            Object.LastKnownLocation = Object.Component->GetComponentLocation();
        }
        
        Object.DistanceToViewer = GetDistanceToViewers(Object.LastKnownLocation);
        
        FVector ClosestViewer = GetClosestViewerLocation(Object.LastKnownLocation);
        int32 NewLODLevel = CalculateLODLevel(Object, ClosestViewer);
        bool bShouldCull = ShouldCullObject(Object, ClosestViewer);
        
        // Apply changes
        if (Object.CurrentLODLevel != NewLODLevel || Object.bIsCulled != bShouldCull)
        {
            Object.CurrentLODLevel = NewLODLevel;
            Object.bIsCulled = bShouldCull;
            Object.LastUpdateTime = GetWorld()->GetTimeSeconds();
            
            if (Object.Actor.IsValid())
            {
                if (bShouldCull)
                {
                    Object.Actor->SetActorHiddenInGame(true);
                }
                else
                {
                    Object.Actor->SetActorHiddenInGame(false);
                    ApplyLODToActor(Object.Actor.Get(), NewLODLevel);
                }
            }
            else if (Object.Component.IsValid())
            {
                Object.Component->SetVisibility(!bShouldCull);
                if (!bShouldCull)
                {
                    ApplyLODToComponent(Object.Component.Get(), NewLODLevel);
                }
            }
        }
    }
    
    // Update batch index
    CurrentBatchIndex = (EndIndex >= ManagedObjects.Num()) ? 0 : EndIndex;
    
    // Update stats
    CurrentStats.ObjectsUpdatedThisFrame = EndIndex - StartIndex;
    CurrentStats.AverageUpdateTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
}

void UClimbingLODManager::UpdateViewerLocations()
{
    ViewerLocations.Empty();
    
    if (!GetWorld())
        return;
        
    // Get all player controllers and their camera locations
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            // Try to get camera location
            FVector ViewLocation = PC->PlayerCameraManager ? 
                                  PC->PlayerCameraManager->GetCameraLocation() : 
                                  PC->GetPawn()->GetActorLocation();
            ViewerLocations.Add(ViewLocation);
        }
    }
    
    // If no players found, use world origin
    if (ViewerLocations.Num() == 0)
    {
        ViewerLocations.Add(FVector::ZeroVector);
    }
}

int32 UClimbingLODManager::CalculateLODLevel(const FManagedLODObject& Object, const FVector& ViewerLocation) const
{
    const FLODSettings& Settings = Object.bUseCustomLODSettings ? 
                                  Object.CustomLODSettings : DefaultLODSettings;
    
    float Distance = Object.DistanceToViewer;
    float AdjustedDistance = Distance / (Settings.LODBias * Object.ImportanceScore);
    
    // Apply performance scaling if enabled
    if (Settings.bEnablePerformanceScaling)
    {
        AdjustedDistance *= Settings.PerformanceScalingFactor;
    }
    
    // Determine LOD level based on distance
    if (AdjustedDistance <= Settings.LOD0_Distance)
        return 0;
    else if (AdjustedDistance <= Settings.LOD1_Distance)
        return 1;
    else if (AdjustedDistance <= Settings.LOD2_Distance)
        return 2;
    else if (AdjustedDistance <= Settings.LOD3_Distance)
        return 3;
    else if (AdjustedDistance <= Settings.LOD4_Distance)
        return 4;
    else
        return 5; // Beyond max LOD, should be culled
}

bool UClimbingLODManager::ShouldCullObject(const FManagedLODObject& Object, const FVector& ViewerLocation) const
{
    const FLODSettings& LODSettings = Object.bUseCustomLODSettings ? 
                                     Object.CustomLODSettings : DefaultLODSettings;
    
    // Distance culling
    if (Object.DistanceToViewer > LODSettings.CullDistance)
        return true;
    
    if (Object.DistanceToViewer > DefaultCullingSettings.MaxRenderDistance)
        return true;
    
    // Frustum culling
    if (DefaultCullingSettings.bEnableFrustumCulling && !IsInViewFrustum(Object, ViewerLocation))
        return true;
    
    // Occlusion culling
    if (DefaultCullingSettings.bEnableOcclusionCulling && IsOccluded(Object, ViewerLocation))
        return true;
    
    // Size-based culling (object too small at this distance)
    if (Object.DistanceToViewer > 100.0f) // Only check for distant objects
    {
        // This is a simplified check; in practice you'd calculate projected size
        float ApproximateSize = DefaultCullingSettings.MinObjectSize;
        float ProjectedSize = ApproximateSize * 100.0f / Object.DistanceToViewer;
        if (ProjectedSize < 1.0f) // Less than 1 pixel
            return true;
    }
    
    return false;
}

float UClimbingLODManager::GetDistanceToViewers(const FVector& ObjectLocation) const
{
    if (ViewerLocations.Num() == 0)
        return 0.0f;
    
    float MinDistance = FLT_MAX;
    for (const FVector& ViewerLoc : ViewerLocations)
    {
        float Distance = FVector::Dist(ObjectLocation, ViewerLoc);
        MinDistance = FMath::Min(MinDistance, Distance);
    }
    
    return MinDistance;
}

FVector UClimbingLODManager::GetClosestViewerLocation(const FVector& ObjectLocation) const
{
    if (ViewerLocations.Num() == 0)
        return FVector::ZeroVector;
    
    FVector ClosestLocation = ViewerLocations[0];
    float MinDistance = FVector::Dist(ObjectLocation, ClosestLocation);
    
    for (int32 i = 1; i < ViewerLocations.Num(); ++i)
    {
        float Distance = FVector::Dist(ObjectLocation, ViewerLocations[i]);
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            ClosestLocation = ViewerLocations[i];
        }
    }
    
    return ClosestLocation;
}

bool UClimbingLODManager::IsInViewFrustum(const FManagedLODObject& Object, const FVector& ViewerLocation) const
{
    // Simplified frustum check - in practice you'd use proper camera frustum
    // For now, use a cone-based approximation
    
    if (ViewerLocations.Num() == 0)
        return true;
    
    // Get player camera manager for actual frustum
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->PlayerCameraManager)
        {
            FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
            FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
            
            // Simple cone check - 120 degree FOV
            FVector ToObject = (Object.LastKnownLocation - CameraLocation).GetSafeNormal();
            FVector CameraForward = CameraRotation.Vector();
            
            float DotProduct = FVector::DotProduct(CameraForward, ToObject);
            return DotProduct > -0.5f; // ~120 degree cone
        }
    }
    
    return true; // Default to visible if we can't determine
}

bool UClimbingLODManager::IsOccluded(const FManagedLODObject& Object, const FVector& ViewerLocation) const
{
    if (!GetWorld())
        return false;
    
    // Simple occlusion test using line trace
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = false;
    
    // Add the object itself to ignore list
    if (Object.Actor.IsValid())
    {
        QueryParams.AddIgnoredActor(Object.Actor.Get());
    }
    
    FHitResult HitResult;
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        ViewerLocation,
        Object.LastKnownLocation,
        ECC_Visibility,
        QueryParams
    );
    
    // Object is occluded if the trace hit something before reaching it
    return bHit && HitResult.bBlockingHit;
}

void UClimbingLODManager::ApplyLODToActor(AActor* Actor, int32 LODLevel)
{
    if (!IsValid(Actor))
        return;
    
    // Handle specific actor types
    if (UAdvancedRopeComponent* Rope = Actor->FindComponentByClass<UAdvancedRopeComponent>())
    {
        ApplyRopeLOD(Rope, LODLevel);
    }
    else if (AClimbingToolBase* Tool = Cast<AClimbingToolBase>(Actor))
    {
        ApplyToolLOD(Tool, LODLevel);
    }
    else
    {
        // Generic actor LOD
        TArray<UActorComponent*> Components = Actor->GetComponents().Array();
        for (UActorComponent* Component : Components)
        {
            if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
            {
                ApplyLODToComponent(PrimComp, LODLevel);
            }
        }
    }
}

void UClimbingLODManager::ApplyLODToComponent(UPrimitiveComponent* Component, int32 LODLevel)
{
    if (!IsValid(Component))
        return;
    
    // Handle static mesh components
    if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Component))
    {
        StaticMeshComp->SetForcedLodModel(FMath::Clamp(LODLevel + 1, 1, 5)); // Unreal LOD is 1-based
    }
    // Handle skeletal mesh components
    else if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
    {
        SkeletalMeshComp->SetForcedLOD(FMath::Clamp(LODLevel + 1, 1, 5));
    }
    
    // Adjust component settings based on LOD level
    switch (LODLevel)
    {
        case 0: // Ultra
            Component->SetCastShadow(true);
            Component->SetReceivesDecals(true);
            break;
        case 1: // High
            Component->SetCastShadow(true);
            Component->SetReceivesDecals(true);
            break;
        case 2: // Medium
            Component->SetCastShadow(true);
            Component->SetReceivesDecals(false);
            break;
        case 3: // Low
            Component->SetCastShadow(false);
            Component->SetReceivesDecals(false);
            break;
        case 4: // Very Low
            Component->SetCastShadow(false);
            Component->SetReceivesDecals(false);
            break;
    }
}

void UClimbingLODManager::ApplyRopeLOD(UAdvancedRopeComponent* Rope, int32 LODLevel)
{
    if (!IsValid(Rope) || !IsValid(Rope->CableComponent))
        return;
    
    // Adjust rope segments based on LOD
    int32 SegmentCount = 32; // Default
    switch (LODLevel)
    {
        case 0: SegmentCount = 64; break;  // Ultra
        case 1: SegmentCount = 32; break;  // High
        case 2: SegmentCount = 16; break;  // Medium
        case 3: SegmentCount = 8; break;   // Low
        case 4: SegmentCount = 4; break;   // Very Low
    }
    
    Rope->CableComponent->NumSegments = SegmentCount;
    
    // Adjust physics settings
    if (LODLevel >= 3)
    {
        // Reduce physics accuracy for distant ropes
        Rope->CableComponent->SolverIterations = FMath::Max(1, Rope->CableComponent->SolverIterations / 2);
    }
}

void UClimbingLODManager::ApplyToolLOD(AClimbingToolBase* Tool, int32 LODLevel)
{
    if (!IsValid(Tool))
        return;
    
    // Adjust tool update frequency based on LOD
    switch (LODLevel)
    {
        case 0: Tool->SetActorTickInterval(0.0f); break;   // Every frame
        case 1: Tool->SetActorTickInterval(0.033f); break; // 30 FPS
        case 2: Tool->SetActorTickInterval(0.1f); break;   // 10 FPS
        case 3: Tool->SetActorTickInterval(0.2f); break;   // 5 FPS
        case 4: Tool->SetActorTickInterval(0.5f); break;   // 2 FPS
    }
    
    // Apply LOD to tool components
    TArray<UActorComponent*> Components = Tool->GetComponents().Array();
    for (UActorComponent* Component : Components)
    {
        if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
        {
            ApplyLODToComponent(PrimComp, LODLevel);
        }
    }
}

void UClimbingLODManager::RemoveInvalidObjects()
{
    int32 RemovedCount = ManagedObjects.RemoveAll([](const FManagedLODObject& Object)
    {
        return !Object.Actor.IsValid() && !Object.Component.IsValid();
    });
    
    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Removed %d invalid objects from LOD manager"), RemovedCount);
    }
}

void UClimbingLODManager::UpdatePerformanceStats(float DeltaTime)
{
    // Update frame time tracking
    AverageFrameTime = FMath::FInterpTo(AverageFrameTime, DeltaTime, DeltaTime, 2.0f);
    
    // Count objects by LOD level and visibility
    CurrentStats.TotalManagedObjects = ManagedObjects.Num();
    CurrentStats.VisibleObjects = 0;
    CurrentStats.CulledObjects = 0;
    CurrentStats.LOD0_Objects = 0;
    CurrentStats.LOD1_Objects = 0;
    CurrentStats.LOD2_Objects = 0;
    CurrentStats.LOD3_Objects = 0;
    CurrentStats.LOD4_Objects = 0;
    
    for (const FManagedLODObject& Object : ManagedObjects)
    {
        if (Object.bIsCulled)
        {
            CurrentStats.CulledObjects++;
        }
        else
        {
            CurrentStats.VisibleObjects++;
            
            switch (Object.CurrentLODLevel)
            {
                case 0: CurrentStats.LOD0_Objects++; break;
                case 1: CurrentStats.LOD1_Objects++; break;
                case 2: CurrentStats.LOD2_Objects++; break;
                case 3: CurrentStats.LOD3_Objects++; break;
                case 4: CurrentStats.LOD4_Objects++; break;
            }
        }
    }
    
    // Calculate performance savings
    int32 TotalObjects = CurrentStats.TotalManagedObjects;
    if (TotalObjects > 0)
    {
        // Rough estimate of triangle reduction based on LOD levels
        float TriangleReduction = 
            (CurrentStats.LOD1_Objects * 0.25f + 
             CurrentStats.LOD2_Objects * 0.5f + 
             CurrentStats.LOD3_Objects * 0.75f + 
             CurrentStats.LOD4_Objects * 0.9f + 
             CurrentStats.CulledObjects * 1.0f) / TotalObjects;
        
        CurrentStats.TriangleReductionPercent = TriangleReduction * 100.0f;
        CurrentStats.DrawCallReductionPercent = (CurrentStats.CulledObjects / static_cast<float>(TotalObjects)) * 100.0f;
    }
}

void UClimbingLODManager::CheckPerformanceTargets()
{
    float CurrentFPS = 1.0f / FMath::Max(0.001f, AverageFrameTime);
    
    if (CurrentFPS < AdaptiveLODTargetFPS * 0.9f) // 10% tolerance
    {
        PerformanceMissCount++;
        
        if (PerformanceMissCount >= 3) // 3 consecutive misses
        {
            AdjustAdaptiveLOD();
            PerformanceMissCount = 0;
            OnAdaptiveLODTriggered.Broadcast();
        }
        
        if (CurrentFPS < AdaptiveLODTargetFPS * 0.7f) // Critical performance
        {
            OnPerformanceTargetMissed.Broadcast();
        }
    }
    else
    {
        PerformanceMissCount = FMath::Max(0, PerformanceMissCount - 1);
    }
}

void UClimbingLODManager::AdjustAdaptiveLOD()
{
    // Increase LOD bias to reduce quality and improve performance
    AdaptiveLODBias = FMath::Clamp(AdaptiveLODBias * 0.8f, 0.3f, 2.0f);
    SetGlobalLODBias(AdaptiveLODBias);
    
    // Reduce update frequency if performance is really bad
    if (UpdateFrequency == ELODUpdateFrequency::High)
    {
        SetLODUpdateFrequency(ELODUpdateFrequency::Medium);
    }
    else if (UpdateFrequency == ELODUpdateFrequency::Medium)
    {
        SetLODUpdateFrequency(ELODUpdateFrequency::Low);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Adaptive LOD adjustment: Bias=%.2f, Update=%s"), 
           AdaptiveLODBias, *UEnum::GetValueAsString(UpdateFrequency));
}

// Query functions
int32 UClimbingLODManager::GetObjectLODLevel(AActor* Actor) const
{
    const FManagedLODObject* Object = FindManagedObject(const_cast<AActor*>(Actor));
    return Object ? Object->CurrentLODLevel : -1;
}

bool UClimbingLODManager::IsObjectCulled(AActor* Actor) const
{
    const FManagedLODObject* Object = FindManagedObject(const_cast<AActor*>(Actor));
    return Object ? Object->bIsCulled : false;
}

float UClimbingLODManager::GetObjectDistance(AActor* Actor) const
{
    const FManagedLODObject* Object = FindManagedObject(const_cast<AActor*>(Actor));
    return Object ? Object->DistanceToViewer : 0.0f;
}

FLODPerformanceStats UClimbingLODManager::GetPerformanceStats() const
{
    return CurrentStats;
}

TArray<AActor*> UClimbingLODManager::GetManagedActors() const
{
    TArray<AActor*> Actors;
    for (const FManagedLODObject& Object : ManagedObjects)
    {
        if (Object.Actor.IsValid())
        {
            Actors.Add(Object.Actor.Get());
        }
    }
    return Actors;
}

FManagedLODObject* UClimbingLODManager::FindManagedObject(AActor* Actor)
{
    return ManagedObjects.FindByPredicate([Actor](const FManagedLODObject& Object)
    {
        return Object.Actor == Actor;
    });
}

// Debug functions
void UClimbingLODManager::ShowLODDebugInfo(bool bShow)
{
    bShowDebugInfo = bShow;
}

void UClimbingLODManager::DrawDebugLODInfo()
{
    if (!GetWorld() || ViewerLocations.Num() == 0)
        return;
    
    FVector ViewerLoc = ViewerLocations[0];
    
    for (const FManagedLODObject& Object : ManagedObjects)
    {
        if (!Object.Actor.IsValid())
            continue;
        
        FColor DebugColor = FColor::White;
        switch (Object.CurrentLODLevel)
        {
            case 0: DebugColor = FColor::Green; break;   // Ultra
            case 1: DebugColor = FColor::Blue; break;    // High
            case 2: DebugColor = FColor::Yellow; break;  // Medium
            case 3: DebugColor = FColor::Orange; break;  // Low
            case 4: DebugColor = FColor::Red; break;     // Very Low
        }
        
        if (Object.bIsCulled)
        {
            DebugColor = FColor::Black;
        }
        
        DrawDebugSphere(GetWorld(), Object.LastKnownLocation, 50.0f, 8, DebugColor, false, 0.0f);
        
        FString DebugText = FString::Printf(TEXT("LOD%d\n%.0fm"), 
                                          Object.CurrentLODLevel, Object.DistanceToViewer);
        if (Object.bIsCulled)
        {
            DebugText = TEXT("CULLED\n") + DebugText;
        }
        
        DrawDebugString(GetWorld(), Object.LastKnownLocation + FVector(0, 0, 100), 
                       DebugText, nullptr, DebugColor, 0.0f);
    }
}

void UClimbingLODManager::LogLODStatistics()
{
    UE_LOG(LogTemp, Log, TEXT("=== LOD Manager Statistics ==="));
    UE_LOG(LogTemp, Log, TEXT("Total Objects: %d"), CurrentStats.TotalManagedObjects);
    UE_LOG(LogTemp, Log, TEXT("Visible Objects: %d"), CurrentStats.VisibleObjects);
    UE_LOG(LogTemp, Log, TEXT("Culled Objects: %d"), CurrentStats.CulledObjects);
    UE_LOG(LogTemp, Log, TEXT("LOD Distribution - 0:%d, 1:%d, 2:%d, 3:%d, 4:%d"), 
           CurrentStats.LOD0_Objects, CurrentStats.LOD1_Objects, CurrentStats.LOD2_Objects,
           CurrentStats.LOD3_Objects, CurrentStats.LOD4_Objects);
    UE_LOG(LogTemp, Log, TEXT("Triangle Reduction: %.1f%%"), CurrentStats.TriangleReductionPercent);
    UE_LOG(LogTemp, Log, TEXT("DrawCall Reduction: %.1f%%"), CurrentStats.DrawCallReductionPercent);
    UE_LOG(LogTemp, Log, TEXT("Average Update Time: %.2fms"), CurrentStats.AverageUpdateTimeMs);
    UE_LOG(LogTemp, Log, TEXT("==============================="));
}

// Placeholder implementations for remaining functions
void UClimbingLODManager::UnregisterComponent(UPrimitiveComponent* Component) {}
void UClimbingLODManager::UnregisterRope(UAdvancedRopeComponent* Rope) {}
void UClimbingLODManager::UnregisterTool(AClimbingToolBase* Tool) {}
void UClimbingLODManager::UpdateRopeLOD(UAdvancedRopeComponent* Rope, int32 LODLevel) {}
void UClimbingLODManager::SetObjectImportance(AActor* Actor, float ImportanceScore) {}
void UClimbingLODManager::SetCustomLODSettings(AActor* Actor, const FLODSettings& CustomSettings) {}
void UClimbingLODManager::ForceLODUpdate() { LastLODUpdate = LODUpdateInterval; }
void UClimbingLODManager::SetLODEnabled(bool bEnabled) { bEnableLODSystem = bEnabled; }
void UClimbingLODManager::SetCullingMethod(ECullingMethod Method) { DefaultCullingSettings.CullingMethod = Method; }
void UClimbingLODManager::SetMaxRenderDistance(float Distance) { DefaultCullingSettings.MaxRenderDistance = Distance; }
void UClimbingLODManager::EnableOcclusionCulling(bool bEnable) { DefaultCullingSettings.bEnableOcclusionCulling = bEnable; }
void UClimbingLODManager::EnableFrustumCulling(bool bEnable) { DefaultCullingSettings.bEnableFrustumCulling = bEnable; }
void UClimbingLODManager::EnableAdaptiveLOD(bool bEnable) { bEnableAdaptiveLOD = bEnable; }
void UClimbingLODManager::SetPerformanceTarget(float TargetFPS, float MaxFrameTime) { AdaptiveLODTargetFPS = TargetFPS; }
void UClimbingLODManager::EnableBatchProcessing(bool bEnable) { DefaultCullingSettings.bUseBatchProcessing = bEnable; }
void UClimbingLODManager::SetBatchSize(int32 BatchSize) { MaxObjectsPerBatch = BatchSize; }
void UClimbingLODManager::SetUpdateBudget(float MaxTimeMs) { MaxUpdateTimeBudgetMs = MaxTimeMs; }
void UClimbingLODManager::ShowCullingDebugInfo(bool bShow) { bShowCullingDebug = bShow; }
void UClimbingLODManager::DumpManagedObjects() {}
void UClimbingLODManager::DrawDebugCullingInfo() {}
FString UClimbingLODManager::GetLODDebugString(const FManagedLODObject& Object) const { return FString(); }
FManagedLODObject* UClimbingLODManager::FindManagedComponent(UPrimitiveComponent* Component) { return nullptr; }
float UClimbingLODManager::CalculateObjectImportance(const FManagedLODObject& Object) const { return Object.ImportanceScore; }
void UClimbingLODManager::OptimizeBatchSize() {}
void UClimbingLODManager::OptimizeUpdateFrequency() {}
void UClimbingLODManager::PrioritizeObjects() {}