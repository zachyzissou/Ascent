#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "../Performance/ClimbingLODManager.h"
#include "EnvironmentalEffectsLODSystem.generated.h"

UENUM(BlueprintType)
enum class EEnvironmentalEffectType : uint8
{
    ParticleEffect      UMETA(DisplayName = "Particle Effect"),
    NiagaraEffect       UMETA(DisplayName = "Niagara Effect"),
    AudioEffect         UMETA(DisplayName = "Audio Effect"),
    PostProcessEffect   UMETA(DisplayName = "Post Process Effect"),
    MaterialEffect      UMETA(DisplayName = "Material Effect"),
    LightingEffect      UMETA(DisplayName = "Lighting Effect"),
    WindEffect          UMETA(DisplayName = "Wind Effect"),
    TemperatureEffect   UMETA(DisplayName = "Temperature Effect")
};

UENUM(BlueprintType)
enum class EEffectCullingMethod : uint8
{
    Distance            UMETA(DisplayName = "Distance Based"),
    Frustum             UMETA(DisplayName = "Frustum Culling"),
    Occlusion           UMETA(DisplayName = "Occlusion Culling"),
    Performance         UMETA(DisplayName = "Performance Based"),
    Importance          UMETA(DisplayName = "Importance Based"),
    Hybrid              UMETA(DisplayName = "Hybrid Culling")
};

UENUM(BlueprintType)
enum class EEffectQualityLevel : uint8
{
    Cinematic           UMETA(DisplayName = "Cinematic"),
    Ultra               UMETA(DisplayName = "Ultra"),
    High                UMETA(DisplayName = "High"),
    Medium              UMETA(DisplayName = "Medium"),
    Low                 UMETA(DisplayName = "Low"),
    Minimal             UMETA(DisplayName = "Minimal"),
    Disabled            UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class EEffectUpdateFrequency : uint8
{
    RealTime            UMETA(DisplayName = "Real Time"),
    High                UMETA(DisplayName = "High (30 FPS)"),
    Medium              UMETA(DisplayName = "Medium (15 FPS)"),
    Low                 UMETA(DisplayName = "Low (10 FPS)"),
    Minimal             UMETA(DisplayName = "Minimal (5 FPS)"),
    Static              UMETA(DisplayName = "Static")
};

USTRUCT(BlueprintType)
struct FEffectLODParameters
{
    GENERATED_BODY()

    // Distance thresholds for different quality levels
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float CinematicDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float UltraDistance = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float HighDistance = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float MediumDistance = 7000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float LowDistance = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float MinimalDistance = 15000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds")
    float CullingDistance = 20000.0f;

    // Quality scaling parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    TArray<float> ParticleCountMultipliers = {1.0f, 0.8f, 0.6f, 0.4f, 0.2f, 0.1f, 0.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    TArray<float> EffectIntensityMultipliers = {1.0f, 0.9f, 0.7f, 0.5f, 0.3f, 0.15f, 0.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    TArray<float> UpdateRateMultipliers = {1.0f, 0.8f, 0.6f, 0.4f, 0.25f, 0.15f, 0.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    TArray<bool> EnableCollisionPerLevel = {true, true, true, false, false, false, false};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    TArray<bool> EnableLightingPerLevel = {true, true, true, true, false, false, false};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Scaling")
    TArray<bool> EnableShadowsPerLevel = {true, true, false, false, false, false, false};

    // Performance impact weights
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Weights")
    float CPUWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Weights")
    float GPUWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Weights")
    float MemoryWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Weights")
    float ImportanceMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FWeatherEffectLOD
{
    GENERATED_BODY()

    // Particle system settings per LOD level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Settings")
    TArray<int32> MaxParticlesPerLevel = {10000, 5000, 2500, 1000, 500, 100, 0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Settings")
    TArray<float> SpawnRateMultipliers = {1.0f, 0.8f, 0.6f, 0.4f, 0.2f, 0.1f, 0.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Settings")
    TArray<float> ParticleSizeMultipliers = {1.0f, 0.95f, 0.9f, 0.8f, 0.7f, 0.6f, 0.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Settings")
    TArray<float> LifetimeMultipliers = {1.0f, 0.9f, 0.8f, 0.6f, 0.4f, 0.2f, 0.0f};

    // Rendering settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    TArray<bool> EnableTranslucencyPerLevel = {true, true, true, true, false, false, false};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    TArray<bool> EnableDistortionPerLevel = {true, true, false, false, false, false, false};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    TArray<int32> TextureResolutionPerLevel = {1024, 512, 256, 128, 64, 32, 0};

    // Audio settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TArray<float> VolumeMultipliers = {1.0f, 0.9f, 0.8f, 0.6f, 0.4f, 0.2f, 0.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TArray<bool> EnableAudioPerLevel = {true, true, true, true, false, false, false};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TArray<float> AudioRangeMultipliers = {1.0f, 0.9f, 0.8f, 0.7f, 0.5f, 0.3f, 0.0f};
};

USTRUCT(BlueprintType)
struct FManagedEnvironmentalEffect
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EEnvironmentalEffectType EffectType;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<UActorComponent> EffectComponent;

    UPROPERTY(BlueprintReadOnly)
    EEffectQualityLevel CurrentQualityLevel = EEffectQualityLevel::High;

    UPROPERTY(BlueprintReadOnly)
    EEffectUpdateFrequency UpdateFrequency = EEffectUpdateFrequency::RealTime;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToViewer = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ImportanceScore = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsVisible = true;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCulled = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsNetworkRelevant = false;

    UPROPERTY(BlueprintReadOnly)
    FVector LastKnownLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float LastUpdateTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageFrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 UpdateCount = 0;

    // Custom LOD parameters for this specific effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEffectLODParameters CustomLODParams;

    UPROPERTY(BlueprintReadOnly)
    bool bUseCustomLODParams = false;
};

USTRUCT(BlueprintType)
struct FEnvironmentalEffectStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalManagedEffects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveEffects = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CulledEffects = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> EffectsPerQualityLevel;

    UPROPERTY(BlueprintReadOnly)
    float TotalFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageEffectFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float TotalMemoryUsageMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ParticleRenderingTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AudioProcessingTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalParticleCount = 0;

    UPROPERTY(BlueprintReadOnly)
    float CullingEfficiencyPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float LODTransitionsPerSecond = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float NetworkBandwidthKBps = 0.0f;
};

UCLASS(BlueprintType, ClassGroup=(ClimbingGame), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalEffectsLODSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalEffectsLODSystem();

    // Component interface
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Effect registration and management
    UFUNCTION(BlueprintCallable, Category = "Effect Management")
    void RegisterEffect(UActorComponent* EffectComponent, EEnvironmentalEffectType EffectType, float ImportanceScore = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Effect Management")
    void UnregisterEffect(UActorComponent* EffectComponent);

    UFUNCTION(BlueprintCallable, Category = "Effect Management")
    void UnregisterAllEffects();

    UFUNCTION(BlueprintCallable, Category = "Effect Management")
    void SetEffectImportance(UActorComponent* EffectComponent, float ImportanceScore);

    // LOD management
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UpdateEffectLODs();

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void SetGlobalQualityLevel(EEffectQualityLevel QualityLevel);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void SetEffectQualityLevel(UActorComponent* EffectComponent, EEffectQualityLevel QualityLevel);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void SetLODBias(float LODBias);

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void SetCustomLODParameters(UActorComponent* EffectComponent, const FEffectLODParameters& CustomParams);

    // Quality optimization
    UFUNCTION(BlueprintCallable, Category = "Quality Optimization")
    void OptimizeAllEffects();

    UFUNCTION(BlueprintCallable, Category = "Quality Optimization")
    void EnableAdaptiveQuality(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Quality Optimization")
    void SetPerformanceTarget(float TargetFrameTimeMs);

    UFUNCTION(BlueprintCallable, Category = "Quality Optimization")
    void ForceQualityAdjustment(bool bIncreaseQuality);

    // Culling management
    UFUNCTION(BlueprintCallable, Category = "Culling Management")
    void SetCullingMethod(EEffectCullingMethod CullingMethod);

    UFUNCTION(BlueprintCallable, Category = "Culling Management")
    void EnableFrustumCulling(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Culling Management")
    void EnableOcclusionCulling(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Culling Management")
    void SetCullingDistances(float NearDistance, float FarDistance);

    UFUNCTION(BlueprintCallable, Category = "Culling Management")
    void UpdateCulling();

    // Weather-specific optimization
    UFUNCTION(BlueprintCallable, Category = "Weather Effects")
    void OptimizeWeatherEffects();

    UFUNCTION(BlueprintCallable, Category = "Weather Effects")
    void SetWeatherIntensity(float Intensity);

    UFUNCTION(BlueprintCallable, Category = "Weather Effects")
    void EnableWeatherLOD(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Weather Effects")
    void SetWeatherLODParameters(const FWeatherEffectLOD& LODParams);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring", BlueprintPure)
    FEnvironmentalEffectStats GetEffectStats() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void ResetPerformanceStats();

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetTotalFrameTime() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsPerformanceTargetMet() const;

    // Query functions
    UFUNCTION(BlueprintCallable, Category = "Effect Query", BlueprintPure)
    EEffectQualityLevel GetEffectQualityLevel(UActorComponent* EffectComponent) const;

    UFUNCTION(BlueprintCallable, Category = "Effect Query", BlueprintPure)
    bool IsEffectCulled(UActorComponent* EffectComponent) const;

    UFUNCTION(BlueprintCallable, Category = "Effect Query", BlueprintPure)
    TArray<UActorComponent*> GetManagedEffects() const;

    UFUNCTION(BlueprintCallable, Category = "Effect Query", BlueprintPure)
    TArray<UActorComponent*> GetEffectsInRadius(const FVector& Location, float Radius) const;

    // Debug and profiling
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor = true)
    void ShowLODDebugInfo(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogEffectStats();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DumpManagedEffects();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RunPerformanceBenchmark(float Duration = 10.0f);

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    FEffectLODParameters DefaultLODParameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Settings")
    FWeatherEffectLOD WeatherLODSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling Settings")
    EEffectCullingMethod CullingMethod = EEffectCullingMethod::Hybrid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling Settings")
    bool bEnableFrustumCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling Settings")
    bool bEnableOcclusionCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableAdaptiveQuality = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float TargetFrameTimeMs = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float AdaptiveQualityInterval = 1.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    float LODUpdateInterval = 0.2f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    int32 MaxEffectsPerFrame = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Settings")
    bool bEnableMultithreading = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnQualityLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnEffectCulled;

    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnPerformanceTargetMissed;

    UPROPERTY(BlueprintAssignable, Category = "LOD Events")
    FSimpleMulticastDelegate OnAdaptiveQualityTriggered;

protected:
    // Managed effects storage
    UPROPERTY()
    TArray<FManagedEnvironmentalEffect> ManagedEffects;

    // Performance tracking
    FEnvironmentalEffectStats CurrentStats;
    float LastLODUpdate = 0.0f;
    float LastAdaptiveQualityCheck = 0.0f;
    float LastCullingUpdate = 0.0f;

    // Quality state
    EEffectQualityLevel GlobalQualityLevel = EEffectQualityLevel::High;
    float GlobalLODBias = 1.0f;
    bool bAdaptiveQualityActive = false;

    // Viewer tracking
    TArray<FVector> ViewerLocations;
    float LastViewerLocationUpdate = 0.0f;

    // Debug state
    bool bShowDebugInfo = false;
    bool bIsRunningBenchmark = false;
    float BenchmarkStartTime = 0.0f;
    float BenchmarkDuration = 0.0f;

    // Integration references
    UPROPERTY()
    UClimbingLODManager* LODManager = nullptr;

private:
    // Core update functions
    void UpdateManagedEffects(float DeltaTime);
    void UpdateAdaptiveQuality(float DeltaTime);
    void UpdateViewerLocations();
    
    // LOD calculation and application
    EEffectQualityLevel CalculateEffectLOD(const FManagedEnvironmentalEffect& Effect) const;
    void ApplyEffectLOD(FManagedEnvironmentalEffect& Effect, EEffectQualityLevel NewQuality);
    void TransitionEffectQuality(FManagedEnvironmentalEffect& Effect, EEffectQualityLevel FromQuality, EEffectQualityLevel ToQuality);
    
    // Effect-specific optimization
    void OptimizeParticleEffect(UParticleSystemComponent* ParticleComponent, EEffectQualityLevel QualityLevel);
    void OptimizeNiagaraEffect(UNiagaraComponent* NiagaraComponent, EEffectQualityLevel QualityLevel);
    void OptimizeAudioEffect(UAudioComponent* AudioComponent, EEffectQualityLevel QualityLevel);
    void OptimizePostProcessEffect(UPostProcessComponent* PostProcessComponent, EEffectQualityLevel QualityLevel);
    
    // Culling implementation
    void UpdateEffectCulling();
    bool ShouldCullEffect(const FManagedEnvironmentalEffect& Effect) const;
    bool IsEffectInFrustum(const FManagedEnvironmentalEffect& Effect) const;
    bool IsEffectOccluded(const FManagedEnvironmentalEffect& Effect) const;
    
    // Performance optimization
    void PerformAdaptiveQualityAdjustment();
    void ReduceEffectQuality();
    void IncreaseEffectQuality();
    void OptimizeEffectMemory();
    
    // Utility functions
    float GetDistanceToNearestViewer(const FVector& EffectLocation) const;
    FManagedEnvironmentalEffect* FindManagedEffect(UActorComponent* Component);
    void RemoveInvalidEffects();
    void UpdateEffectStats(float DeltaTime);
    
    // Threading helpers
    void DispatchLODUpdates();
    void ProcessLODResultsOnGameThread();
    
    // Debug helpers
    void DrawEffectDebugInfo(const FManagedEnvironmentalEffect& Effect);
    void LogEffectState(const FManagedEnvironmentalEffect& Effect) const;
    
    // Benchmark helpers
    void UpdateBenchmark(float DeltaTime);
    void CompleteBenchmark();
    
    // Integration helpers
    void InitializeLODManagerIntegration();
    void UpdateLODManagerMetrics();
};