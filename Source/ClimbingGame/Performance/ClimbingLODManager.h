#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "../Tools/ClimbingToolBase.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "ClimbingLODManager.generated.h"

UENUM(BlueprintType)
enum class ELODUpdateFrequency : uint8
{
    EveryFrame      UMETA(DisplayName = "Every Frame"),
    High            UMETA(DisplayName = "High (10 FPS)"),
    Medium          UMETA(DisplayName = "Medium (5 FPS)"),
    Low             UMETA(DisplayName = "Low (2 FPS)"),
    VeryLow         UMETA(DisplayName = "Very Low (1 FPS)")
};

UENUM(BlueprintType)
enum class ECullingMethod : uint8
{
    Distance        UMETA(DisplayName = "Distance Based"),
    Frustum         UMETA(DisplayName = "Frustum Culling"),
    Occlusion       UMETA(DisplayName = "Occlusion Culling"),
    Combined        UMETA(DisplayName = "Combined Methods"),
    Adaptive        UMETA(DisplayName = "Adaptive Culling")
};

UENUM(BlueprintType)
enum class ELODTransition : uint8
{
    Instant         UMETA(DisplayName = "Instant Transition"),
    Smooth          UMETA(DisplayName = "Smooth Fade"),
    PopIn           UMETA(DisplayName = "Pop-in with Delay"),
    Dithered        UMETA(DisplayName = "Dithered Transition")
};

USTRUCT(BlueprintType)
struct FLODSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LOD0_Distance = 500.0f;    // Ultra detail

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LOD1_Distance = 1500.0f;   // High detail

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LOD2_Distance = 3000.0f;   // Medium detail

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LOD3_Distance = 5000.0f;   // Low detail

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float LOD4_Distance = 8000.0f;   // Very low detail

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Distances")
    float CullDistance = 12000.0f;   // Culling distance

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
    ELODTransition TransitionType = ELODTransition::Smooth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
    float TransitionDuration = 0.3f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bias")
    float LODBias = 1.0f;            // Global LOD bias multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnablePerformanceScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float PerformanceScalingFactor = 1.0f;
};

USTRUCT(BlueprintType)
struct FCullingSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling Method")
    ECullingMethod CullingMethod = ECullingMethod::Combined;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Culling")
    float MaxRenderDistance = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Culling")
    float MinObjectSize = 10.0f;     // Minimum size to render (world units)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frustum Culling")
    bool bEnableFrustumCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frustum Culling")
    float FrustumCullingMargin = 100.0f; // Extra margin for frustum culling

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Culling")
    bool bEnableOcclusionCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Culling")
    float OcclusionQueryDelay = 0.1f; // Delay before occlusion queries

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Culling")
    bool bEnableAdaptiveCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adaptive Culling")
    float PerformanceThresholdFPS = 45.0f; // FPS threshold for adaptive culling

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxObjectsPerFrame = 1000; // Max objects to process per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bUseBatchProcessing = true;
};

USTRUCT(BlueprintType)
struct FManagedLODObject
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Actor;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<UPrimitiveComponent> Component;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentLODLevel = 0;

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToViewer = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsVisible = true;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCulled = false;

    UPROPERTY(BlueprintReadOnly)
    FVector LastKnownLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float ImportanceScore = 1.0f; // Higher score = more important

    // Custom LOD settings for this object
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLODSettings CustomLODSettings;

    UPROPERTY(BlueprintReadOnly)
    bool bUseCustomLODSettings = false;

    // Performance tracking
    UPROPERTY(BlueprintReadOnly)
    float AverageUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 UpdateCount = 0;
};

USTRUCT(BlueprintType)
struct FLODPerformanceStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalManagedObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 VisibleObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CulledObjects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LOD0_Objects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LOD1_Objects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LOD2_Objects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LOD3_Objects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LOD4_Objects = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageUpdateTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MaxUpdateTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 ObjectsUpdatedThisFrame = 0;

    UPROPERTY(BlueprintReadOnly)
    float TriangleReductionPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float DrawCallReductionPercent = 0.0f;
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingLODManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Main update function
    UFUNCTION(CallInEditor = true)
    void TickLODManager(float DeltaTime);

    // Object management
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void RegisterActor(AActor* Actor, float ImportanceScore = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UnregisterActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void RegisterComponent(UPrimitiveComponent* Component, float ImportanceScore = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UnregisterComponent(UPrimitiveComponent* Component);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void SetObjectImportance(AActor* Actor, float ImportanceScore);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void SetCustomLODSettings(AActor* Actor, const FLODSettings& CustomSettings);

    // Rope-specific LOD management
    UFUNCTION(BlueprintCallable, Category = "Rope LOD")
    void RegisterRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope LOD")
    void UnregisterRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope LOD")
    void UpdateRopeLOD(UAdvancedRopeComponent* Rope, int32 LODLevel);

    // Tool-specific LOD management
    UFUNCTION(BlueprintCallable, Category = "Tool LOD")
    void RegisterTool(AClimbingToolBase* Tool);

    UFUNCTION(BlueprintCallable, Category = "Tool LOD")
    void UnregisterTool(AClimbingToolBase* Tool);

    // LOD control
    UFUNCTION(BlueprintCallable, Category = "LOD Control")
    void SetGlobalLODBias(float Bias);

    UFUNCTION(BlueprintCallable, Category = "LOD Control")
    void SetLODUpdateFrequency(ELODUpdateFrequency Frequency);

    UFUNCTION(BlueprintCallable, Category = "LOD Control")
    void ForceLODUpdate();

    UFUNCTION(BlueprintCallable, Category = "LOD Control")
    void SetLODEnabled(bool bEnabled);

    // Culling control
    UFUNCTION(BlueprintCallable, Category = "Culling Control")
    void SetCullingMethod(ECullingMethod Method);

    UFUNCTION(BlueprintCallable, Category = "Culling Control")
    void SetMaxRenderDistance(float Distance);

    UFUNCTION(BlueprintCallable, Category = "Culling Control")
    void EnableOcclusionCulling(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Culling Control")
    void EnableFrustumCulling(bool bEnable);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForPerformance(float TargetFPS);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void EnableAdaptiveLOD(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetPerformanceTarget(float TargetFPS, float MaxFrameTime);

    // Batch processing
    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void EnableBatchProcessing(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void SetBatchSize(int32 BatchSize);

    UFUNCTION(BlueprintCallable, Category = "Batch Processing")
    void SetUpdateBudget(float MaxTimeMs);

    // Query functions
    UFUNCTION(BlueprintCallable, Category = "LOD Query", BlueprintPure)
    int32 GetObjectLODLevel(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "LOD Query", BlueprintPure)
    bool IsObjectCulled(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "LOD Query", BlueprintPure)
    float GetObjectDistance(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "LOD Query", BlueprintPure)
    FLODPerformanceStats GetPerformanceStats() const;

    UFUNCTION(BlueprintCallable, Category = "LOD Query", BlueprintPure)
    TArray<AActor*> GetManagedActors() const;

    // Debug functions
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowLODDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowCullingDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogLODStatistics();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpManagedObjects();

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FLODSettings DefaultLODSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FCullingSettings DefaultCullingSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    ELODUpdateFrequency UpdateFrequency = ELODUpdateFrequency::Medium;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableLODSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableAdaptiveLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float AdaptiveLODTargetFPS = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MaxUpdateTimeBudgetMs = 2.0f; // Max time to spend on LOD updates per frame

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxObjectsPerBatch = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bLogPerformanceWarnings = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnLODSystemEnabled;

    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnLODSystemDisabled;

    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnPerformanceTargetMissed;

    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnAdaptiveLODTriggered;

protected:
    // Managed objects storage
    UPROPERTY()
    TArray<FManagedLODObject> ManagedObjects;

    // Performance tracking
    FLODPerformanceStats CurrentStats;
    float LastPerformanceCheck = 0.0f;
    float AverageFrameTime = 16.67f; // 60 FPS default

    // Update timing
    float LODUpdateInterval = 0.2f;  // 5 FPS default
    float LastLODUpdate = 0.0f;
    int32 CurrentBatchIndex = 0;

    // Viewer tracking
    TArray<FVector> ViewerLocations;
    float LastViewerUpdate = 0.0f;

    // Adaptive LOD state
    bool bAdaptiveLODActive = false;
    float AdaptiveLODBias = 1.0f;
    int32 PerformanceMissCount = 0;

    // Debug state
    bool bShowDebugInfo = false;
    bool bShowCullingDebug = false;

private:
    // Core LOD functions
    void UpdateLODLevels(float DeltaTime);
    void BatchUpdateObjects(float DeltaTime);
    void UpdateViewerLocations();
    
    // LOD calculation
    int32 CalculateLODLevel(const FManagedLODObject& Object, const FVector& ViewerLocation) const;
    bool ShouldCullObject(const FManagedLODObject& Object, const FVector& ViewerLocation) const;
    float CalculateObjectImportance(const FManagedLODObject& Object) const;
    
    // Distance calculations
    float GetDistanceToViewers(const FVector& ObjectLocation) const;
    FVector GetClosestViewerLocation(const FVector& ObjectLocation) const;
    
    // Culling methods
    bool IsInViewFrustum(const FManagedLODObject& Object, const FVector& ViewerLocation) const;
    bool IsOccluded(const FManagedLODObject& Object, const FVector& ViewerLocation) const;
    
    // Object management helpers
    FManagedLODObject* FindManagedObject(AActor* Actor);
    FManagedLODObject* FindManagedComponent(UPrimitiveComponent* Component);
    void RemoveInvalidObjects();
    
    // LOD application
    void ApplyLODToActor(AActor* Actor, int32 LODLevel);
    void ApplyLODToComponent(UPrimitiveComponent* Component, int32 LODLevel);
    void ApplyRopeLOD(UAdvancedRopeComponent* Rope, int32 LODLevel);
    void ApplyToolLOD(AClimbingToolBase* Tool, int32 LODLevel);
    
    // Performance monitoring
    void UpdatePerformanceStats(float DeltaTime);
    void CheckPerformanceTargets();
    void AdjustAdaptiveLOD();
    
    // Optimization helpers
    void OptimizeBatchSize();
    void OptimizeUpdateFrequency();
    void PrioritizeObjects();
    
    // Debug helpers
    void DrawDebugLODInfo();
    void DrawDebugCullingInfo();
    FString GetLODDebugString(const FManagedLODObject& Object) const;
};