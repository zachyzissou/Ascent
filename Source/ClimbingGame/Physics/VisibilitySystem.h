#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialParameterCollection.h"
#include "Components/PostProcessComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "EnvironmentalHazardManager.h"
#include "VisibilitySystem.generated.h"

UENUM(BlueprintType)
enum class EVisibilityHazardType : uint8
{
    None            UMETA(DisplayName = "None"),
    Fog             UMETA(DisplayName = "Fog"),
    Mist            UMETA(DisplayName = "Mist"),
    Dust            UMETA(DisplayName = "Dust"),
    Sand            UMETA(DisplayName = "Sand"),
    Smoke           UMETA(DisplayName = "Smoke"),
    Steam           UMETA(DisplayName = "Steam"),
    Rain            UMETA(DisplayName = "Rain"),
    Snow            UMETA(DisplayName = "Snow"),
    Hail            UMETA(DisplayName = "Hail"),
    Blizzard        UMETA(DisplayName = "Blizzard")
};

UENUM(BlueprintType)
enum class EVisibilityPattern : uint8
{
    Uniform         UMETA(DisplayName = "Uniform"),
    Patchy          UMETA(DisplayName = "Patchy"),
    Layered         UMETA(DisplayName = "Layered"),
    Moving          UMETA(DisplayName = "Moving"),
    Swirling        UMETA(DisplayName = "Swirling"),
    Rising          UMETA(DisplayName = "Rising"),
    Settling        UMETA(DisplayName = "Settling")
};

UENUM(BlueprintType)
enum class ELightScattering : uint8
{
    None            UMETA(DisplayName = "None"),
    Rayleigh        UMETA(DisplayName = "Rayleigh (Blue Light)"),
    Mie             UMETA(DisplayName = "Mie (Large Particles)"),
    Mixed           UMETA(DisplayName = "Mixed Scattering")
};

USTRUCT(BlueprintType)
struct FVisibilityCondition
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Visibility Condition")
    EVisibilityHazardType HazardType = EVisibilityHazardType::None;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Visibility Condition")
    float Intensity = 0.0f; // 0-1

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Visibility Condition")
    float VisualRange = 10000.0f; // cm, maximum visibility distance

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Visibility Condition")
    float Density = 0.0f; // 0-1, particle density

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Condition")
    FColor Tint = FColor::White; // Color tint of the hazard

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Condition")
    float ParticleSize = 1.0f; // Relative size of particles/droplets

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Condition")
    EVisibilityPattern Pattern = EVisibilityPattern::Uniform;

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Condition")
    FVector MovementDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Condition")
    float MovementSpeed = 0.0f; // cm/s
};

USTRUCT(BlueprintType)
struct FVisibilityZone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Zone")
    FString ZoneID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Zone")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Zone")
    FVector Extent = FVector(1000.0f, 1000.0f, 1000.0f); // cm

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Visibility Zone")
    FVisibilityCondition Condition;

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Zone")
    float AltitudeGradient = 0.0f; // Change in intensity per cm altitude

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Zone")
    bool bAffectsLighting = true;

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Zone")
    bool bAffectsSound = true; // Sound attenuation in poor visibility

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Zone")
    float TemperatureEffect = 0.0f; // Temperature change this zone causes
};

USTRUCT(BlueprintType)
struct FLightScatteringData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    ELightScattering ScatteringType = ELightScattering::None;

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    float ScatteringCoefficient = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    float AbsorptionCoefficient = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    float ExtinctionCoefficient = 0.0f; // Scattering + Absorption

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    FLinearColor ColorShift = FLinearColor::White;

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    float HaloIntensity = 0.0f; // Light halo effect in fog/mist

    UPROPERTY(BlueprintReadOnly, Category = "Light Scattering")
    float GlareIntensity = 0.0f; // Glare from direct light sources
};

USTRUCT(BlueprintType)
struct FVisibilityEffects
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float VisualRangeMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float ContrastReduction = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float ColorDesaturation = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    FLinearColor TintColor = FLinearColor::White;

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float DepthBlur = 0.0f; // 0-1, blur for distant objects

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float LightHalo = 0.0f; // 0-1, halo effect around lights

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float SoundAttenuation = 1.0f; // Sound attenuation multiplier

    UPROPERTY(BlueprintReadOnly, Category = "Visibility Effects")
    float NavigationDifficulty = 0.0f; // 0-1, affects pathfinding
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UVisibilitySystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisibilitySystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Visibility zones
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visibility System")
    TArray<FVisibilityZone> VisibilityZones;

    // Global visibility condition
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Visibility System")
    FVisibilityCondition GlobalCondition;

    // Environmental hazard manager reference
    UPROPERTY(BlueprintReadWrite, Category = "System Integration")
    UEnvironmentalHazardManager* HazardManager;

    // Post-process components for visual effects
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UPostProcessComponent* VisibilityPostProcess;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UExponentialHeightFogComponent* DynamicFog;

    // Material parameter collection for global effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    UMaterialParameterCollection* VisibilityMPC;

    // Visibility zone management
    UFUNCTION(BlueprintCallable, Category = "Visibility Zones")
    void RegisterVisibilityZone(const FVisibilityZone& Zone);

    UFUNCTION(BlueprintCallable, Category = "Visibility Zones")
    void UnregisterVisibilityZone(const FString& ZoneID);

    UFUNCTION(BlueprintCallable, Category = "Visibility Zones")
    FVisibilityZone GetVisibilityZone(const FString& ZoneID) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility Zones")
    TArray<FString> GetVisibilityZonesAtLocation(const FVector& Location) const;

    // Visibility calculations
    UFUNCTION(BlueprintCallable, Category = "Visibility")
    float GetVisibilityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility")
    float GetVisualRangeAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility")
    FVisibilityEffects CalculateVisibilityEffects(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility")
    bool IsLocationVisible(const FVector& ViewerLocation, const FVector& TargetLocation) const;

    // Light scattering calculations
    UFUNCTION(BlueprintCallable, Category = "Light Scattering")
    FLightScatteringData CalculateLightScattering(const FVector& Location, const FVisibilityCondition& Condition) const;

    UFUNCTION(BlueprintCallable, Category = "Light Scattering")
    float CalculateExtinctionCoefficient(EVisibilityHazardType HazardType, float Intensity) const;

    UFUNCTION(BlueprintCallable, Category = "Light Scattering")
    FLinearColor CalculateScatteredLightColor(const FLinearColor& IncomingLight, const FLightScatteringData& Scattering) const;

    // Precipitation visibility effects
    UFUNCTION(BlueprintCallable, Category = "Precipitation Effects")
    float CalculatePrecipitationVisibility(EVisibilityHazardType PrecipitationType, float Intensity, float DropletSize) const;

    UFUNCTION(BlueprintCallable, Category = "Precipitation Effects")
    void UpdatePrecipitationEffects(EVisibilityHazardType Type, float Intensity, float DeltaTime);

    // Atmospheric effects
    UFUNCTION(BlueprintCallable, Category = "Atmospheric Effects")
    void UpdateFogConditions(float Humidity, float Temperature, float Pressure, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Atmospheric Effects")
    void UpdateDustConditions(float WindSpeed, float AridityLevel, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Atmospheric Effects")
    float CalculateAtmosphericVisibility(float Humidity, float Temperature, float Pollution) const;

    // Global visibility control
    UFUNCTION(BlueprintCallable, Category = "Global Visibility")
    void SetGlobalVisibilityCondition(const FVisibilityCondition& Condition);

    UFUNCTION(BlueprintCallable, Category = "Global Visibility")
    void ClearGlobalVisibilityCondition();

    UFUNCTION(BlueprintCallable, Category = "Global Visibility")
    void BlendGlobalVisibility(const FVisibilityCondition& TargetCondition, float BlendTime);

    // Integration with climbing systems
    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    void ApplyVisibilityEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    float CalculateNavigationDifficulty(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    bool CanSeeClimbingRoute(const FVector& ViewerLocation, const TArray<FVector>& RoutePoints) const;

    // Visual effect updates
    UFUNCTION(BlueprintCallable, Category = "Visual Effects")
    void UpdatePostProcessEffects(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Visual Effects")
    void UpdateFogEffects(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Visual Effects")
    void UpdateMaterialParameterCollection();

    UFUNCTION(BlueprintCallable, Category = "Visual Effects")
    void SetVisibilityPostProcessWeight(float Weight);

    // Distance-based visibility
    UFUNCTION(BlueprintCallable, Category = "Distance Visibility")
    float CalculateDistanceVisibility(float Distance, const FVisibilityCondition& Condition) const;

    UFUNCTION(BlueprintCallable, Category = "Distance Visibility")
    float GetHorizonDistance(const FVector& ViewerLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Distance Visibility")
    float CalculateAngularSize(float ObjectSize, float Distance) const;

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetVisibilityLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeVisibilitySimulation(float ViewerDistance);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Visibility Events")
    FSimpleMulticastDelegate OnVisibilityChanged;

    UPROPERTY(BlueprintAssignable, Category = "Visibility Events")
    FSimpleMulticastDelegate OnLowVisibilityEntered;

    UPROPERTY(BlueprintAssignable, Category = "Visibility Events")
    FSimpleMulticastDelegate OnVisibilityCleared;

protected:
    // Internal visibility simulation
    void UpdateVisibilityZones(float DeltaTime);
    void UpdateGlobalVisibilityCondition(float DeltaTime);
    void ProcessVisibilityMovement(float DeltaTime);
    void CalculateVisibilityInteractions(float DeltaTime);

    // Light and atmospheric calculations
    void UpdateAtmosphericScattering(float DeltaTime);
    void ProcessLightExtinction(float DeltaTime);
    void CalculateSolarVisibilityEffects(float DeltaTime);

    // Weather integration
    void ProcessWeatherVisibilityEffects(float DeltaTime);
    void UpdatePrecipitationVisibility(float DeltaTime);
    void ProcessWindDispersal(float DeltaTime);

    // Visual effect helpers
    void UpdateFogParameters();
    void UpdatePostProcessParameters();
    void ApplyVisibilityToLighting();
    void ModulateAmbientLight(float VisibilityFactor);

    // Performance optimization helpers
    void CullDistantEffects(float ViewerDistance);
    bool ShouldSimulateDetailedVisibility() const;
    void OptimizeEffectQuality(float Distance);

private:
    // Performance optimization state
    int32 CurrentLOD = 0;
    float LastLODUpdate = 0.0f;
    float LODUpdateInterval = 1.0f;

    // Update frequencies
    float ZoneUpdateInterval = 0.5f;     // 2Hz
    float EffectUpdateInterval = 0.1f;   // 10Hz
    float VisualUpdateInterval = 0.033f; // 30Hz

    float LastZoneUpdate = 0.0f;
    float LastEffectUpdate = 0.0f;
    float LastVisualUpdate = 0.0f;

    // Visibility blending state
    bool bIsBlendingVisibility = false;
    FVisibilityCondition BlendStartCondition;
    FVisibilityCondition BlendTargetCondition;
    float BlendDuration = 1.0f;
    float BlendProgress = 0.0f;

    // Light scattering lookup tables
    TMap<EVisibilityHazardType, float> ExtinctionCoefficients;
    TMap<EVisibilityHazardType, FLinearColor> ScatteringColors;

    // Visibility cache for performance
    mutable TMap<FVector, float> VisibilityCache;
    mutable TMap<FVector, FVisibilityEffects> EffectsCache;
    mutable float LastCacheUpdate = 0.0f;
    static constexpr float CacheTimeout = 0.5f;

    // Visual effect references
    UPROPERTY()
    APostProcessVolume* MainPostProcessVolume;

    // Atmospheric constants
    static constexpr float StandardVisibility = 50000.0f; // 500m in good conditions
    static constexpr float MinimumVisibility = 100.0f;    // 1m minimum
    static constexpr float RayleighScatteringCoeff = 5.8e-6f; // m⁻¹ at 550nm
    static constexpr float MieScatteringBase = 3.9e-5f;   // m⁻¹ base value

    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxSimulationDistance = 20000.0f; // cm

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseVisibilityCache = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUpdateVisualEffects = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxVisibilityZones = 20;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float GlobalVisibilityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float VisibilityTransitionSpeed = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableDynamicFog = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnablePostProcessEffects = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float LowVisibilityThreshold = 0.3f; // Threshold for low visibility warning

public:
    // Static utility functions
    UFUNCTION(BlueprintCallable, Category = "Visibility Utilities", CallInEditor = true)
    static float ConvertVisualRangeToVisibility(float VisualRange);

    UFUNCTION(BlueprintCallable, Category = "Visibility Utilities", CallInEditor = true)
    static float ConvertVisibilityToVisualRange(float Visibility);

    UFUNCTION(BlueprintCallable, Category = "Visibility Utilities", CallInEditor = true)
    static FString GetVisibilityDescription(float Visibility);

    UFUNCTION(BlueprintCallable, Category = "Visibility Utilities", CallInEditor = true)
    static float CalculateKoschmiederEquation(float Distance, float ExtinctionCoeff);

    UFUNCTION(BlueprintCallable, Category = "Visibility Utilities", CallInEditor = true)
    static float CalculateMeteorologicalRange(float ExtinctionCoeff);
};