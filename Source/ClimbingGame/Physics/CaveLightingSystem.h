#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Components/LightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialParameterCollection.h"
#include "Particles/ParticleSystemComponent.h"
#include "CaveEnvironmentPhysics.h"
#include "WaterPhysicsComponent.h"
#include "CaveLightingSystem.generated.h"

UENUM(BlueprintType)
enum class ELightSourceType : uint8
{
    Headlamp        UMETA(DisplayName = "LED Headlamp"),
    Flashlight      UMETA(DisplayName = "Handheld Flashlight"),
    LanternLED      UMETA(DisplayName = "LED Lantern"),
    CarbideLamp     UMETA(DisplayName = "Carbide Lamp"),
    Torch           UMETA(DisplayName = "Torch/Flare"),
    ChemLight       UMETA(DisplayName = "Chemical Light"),
    ElectricBeacon  UMETA(DisplayName = "Electric Beacon"),
    NaturalLight    UMETA(DisplayName = "Natural Light")
};

UENUM(BlueprintType)
enum class ELightCondition : uint8
{
    Optimal         UMETA(DisplayName = "Optimal"),
    Dimming         UMETA(DisplayName = "Dimming"),
    Flickering      UMETA(DisplayName = "Flickering"),
    LowBattery      UMETA(DisplayName = "Low Battery"),
    WaterDamaged    UMETA(DisplayName = "Water Damaged"),
    Broken          UMETA(DisplayName = "Broken"),
    Extinguished    UMETA(DisplayName = "Extinguished")
};

UENUM(BlueprintType)
enum class EVisibilityLevel : uint8
{
    Excellent       UMETA(DisplayName = "Excellent Visibility"),
    Good            UMETA(DisplayName = "Good Visibility"),
    Limited         UMETA(DisplayName = "Limited Visibility"),
    Poor            UMETA(DisplayName = "Poor Visibility"),
    ZeroVisibility  UMETA(DisplayName = "Zero Visibility"),
    Blackout        UMETA(DisplayName = "Complete Blackout")
};

USTRUCT(BlueprintType)
struct FLightSourceData
{
    GENERATED_BODY()

    // Light source properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    ELightSourceType SourceType = ELightSourceType::Headlamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    ELightCondition Condition = ELightCondition::Optimal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    float Intensity = 1000.0f; // Lumens

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    float Range = 20.0f; // meters effective range

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    float BeamAngle = 60.0f; // degrees for focused lights

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    float BatteryLevel = 100.0f; // % charge remaining

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    float BatteryDrainRate = 0.1f; // % per minute

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    float WaterResistance = 0.9f; // 0-1 (1 = waterproof)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Source")
    FVector Direction = FVector::ForwardVector;

    UPROPERTY(BlueprintReadWrite, Category = "Light Source")
    bool bIsUnderwater = false;

    UPROPERTY(BlueprintReadWrite, Category = "Light Source")
    float OperatingTime = 0.0f; // Total runtime in seconds
};

USTRUCT(BlueprintType)
struct FVisibilityData
{
    GENERATED_BODY()

    // Visibility calculation results
    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    EVisibilityLevel VisibilityLevel = EVisibilityLevel::Blackout;

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    float VisibilityRange = 0.0f; // meters

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    float LightLevelLux = 0.0f; // Lux at location

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    float ShadowDensity = 1.0f; // 0-1 (1 = complete darkness)

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    float LightScattering = 0.1f; // 0-1 from particles/moisture

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    FVector ClosestLightSource = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    float DistanceToClosestLight = 1000.0f; // meters

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    bool bInLightBeam = false;

    UPROPERTY(BlueprintReadOnly, Category = "Visibility")
    float ContrastRatio = 0.0f; // Visual contrast available
};

USTRUCT(BlueprintType)
struct FLightingEnvironmentEffects
{
    GENERATED_BODY()

    // Environmental effects on lighting
    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float MoistureScattering = 0.8f; // High humidity reduces light penetration

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float DustParticleScattering = 0.1f; // Dust reduces visibility

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float CondensationBuildup = 0.0f; // Water on light lens

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float TemperatureEffectOnBattery = 1.0f; // Cold reduces battery life

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float VibrationDamage = 0.0f; // Climbing vibrations damage equipment

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    bool bLensIcing = false; // Ice formation on lights

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float UnderwaterIntensityLoss = 0.7f; // Light loss underwater

    UPROPERTY(BlueprintReadOnly, Category = "Environmental Effects")
    float WaterRefractionError = 0.0f; // Visual distortion underwater
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UCaveLightingSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UCaveLightingSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Lighting system state
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Lighting State")
    TArray<FLightSourceData> ActiveLightSources;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lighting State")
    FLightingEnvironmentEffects EnvironmentEffects;

    // Cave lighting components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPostProcessVolume* CaveDarknessVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMaterialParameterCollection* LightingParameters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* DustParticles;

    // Light source management
    UFUNCTION(BlueprintCallable, Category = "Light Management")
    void RegisterLightSource(class AClimbingToolBase* LightTool, ELightSourceType SourceType);

    UFUNCTION(BlueprintCallable, Category = "Light Management")
    void UnregisterLightSource(class AClimbingToolBase* LightTool);

    UFUNCTION(BlueprintCallable, Category = "Light Management")
    void UpdateLightSource(class AClimbingToolBase* LightTool, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Light Management")
    void ProcessLightFailure(class AClimbingToolBase* LightTool, const FString& FailureReason);

    UFUNCTION(BlueprintCallable, Category = "Light Management")
    bool IsLightSourceFunctional(class AClimbingToolBase* LightTool) const;

    // Visibility calculation system
    UFUNCTION(BlueprintCallable, Category = "Visibility System")
    FVisibilityData CalculateVisibilityAtLocation(const FVector& Location, AActor* Observer = nullptr) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility System")
    float CalculateLightLevelAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility System")
    EVisibilityLevel GetVisibilityLevel(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility System")
    float CalculateVisualRange(const FVector& ObserverLocation, const FVector& ObserverDirection) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility System")
    bool CanSeeTarget(const FVector& ObserverLocation, const FVector& TargetLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility System")
    void ProcessLightScattering(float DeltaTime);

    // Environmental lighting effects
    UFUNCTION(BlueprintCallable, Category = "Environmental Lighting")
    void ProcessMoistureEffects(float Humidity, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Lighting")
    void ProcessDustEffects(float AirMovement, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Lighting")
    void ProcessCondensationEffects(float Temperature, float Humidity, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Lighting")
    void ProcessUnderwaterLightEffects(UWaterPhysicsComponent* WaterPhysics, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Lighting")
    void ProcessTemperatureEffectsOnLights(float Temperature, float DeltaTime);

    // Emergency lighting systems
    UFUNCTION(BlueprintCallable, Category = "Emergency Lighting")
    void ActivateEmergencyLighting();

    UFUNCTION(BlueprintCallable, Category = "Emergency Lighting")
    void DeactivateEmergencyLighting();

    UFUNCTION(BlueprintCallable, Category = "Emergency Lighting")
    void ProcessBackupLightActivation(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Emergency Lighting")
    bool HasFunctionalLights(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "Emergency Lighting")
    void TriggerLightEmergency(AActor* Actor, const FString& Reason);

    // Dynamic lighting effects
    UFUNCTION(BlueprintCallable, Category = "Dynamic Lighting")
    void ProcessFlickeringLights(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Lighting")
    void ProcessShadowDynamics(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Lighting")
    void UpdateLightBeamIntersections();

    UFUNCTION(BlueprintCallable, Category = "Dynamic Lighting")
    void ProcessReflectedLight(const FVector& SurfaceLocation, const FVector& SurfaceNormal);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Lighting")
    void ProcessLightAbsorption(float DeltaTime);

    // Navigation assistance
    UFUNCTION(BlueprintCallable, Category = "Navigation Assistance")
    void ProcessLightBasedNavigation(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Navigation Assistance")
    TArray<FVector> GetVisibleLandmarks(const FVector& ObserverLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Navigation Assistance")
    void HighlightNavigationPath(const TArray<FVector>& PathPoints);

    UFUNCTION(BlueprintCallable, Category = "Navigation Assistance")
    void ProcessDepthPerceptionEffects(AActor* Actor, float VisibilityLevel);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetLightingLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForDistance(float ViewerDistance);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    bool ShouldSimulateFullLighting() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void CullDistantLights(float ViewerDistance);

    // Network synchronization
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateLightState(class AClimbingToolBase* LightTool, const FLightSourceData& NewState);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnLightFailure(AActor* Actor, const FString& FailureType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnEmergencyLighting(bool bActivated);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnVisibilityChange(const FVector& Location, EVisibilityLevel NewLevel);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Lighting Events")
    FSimpleMulticastDelegate OnLightSourceAdded;

    UPROPERTY(BlueprintAssignable, Category = "Lighting Events")
    FSimpleMulticastDelegate OnLightSourceRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Lighting Events")
    FSimpleMulticastDelegate OnLightFailure;

    UPROPERTY(BlueprintAssignable, Category = "Lighting Events")
    FSimpleMulticastDelegate OnEmergencyLighting;

    UPROPERTY(BlueprintAssignable, Category = "Lighting Events")
    FSimpleMulticastDelegate OnZeroVisibility;

    UPROPERTY(BlueprintAssignable, Category = "Lighting Events")
    FSimpleMulticastDelegate OnVisibilityRestored;

    // Actor-specific lighting data
    UPROPERTY()
    TMap<AActor*, TArray<FLightSourceData>> ActorLightSources;

    UPROPERTY()
    TMap<AActor*, FVisibilityData> ActorVisibilityStates;

protected:
    // Internal lighting calculations
    void UpdateLightingSystem(float DeltaTime);
    void UpdateVisibilityCalculations(float DeltaTime);
    void UpdateEnvironmentalEffects(float DeltaTime);
    void UpdateLightSourceStates(float DeltaTime);

    // Light physics helpers
    float CalculateLightAttenuation(float Distance, float Intensity) const;
    float CalculateLightScattering(const FVector& LightLocation, const FVector& ObserverLocation, float ParticleDensity) const;
    FVector CalculateLightReflection(const FVector& LightDirection, const FVector& SurfaceNormal, float Reflectivity) const;
    float CalculateLightAbsorption(float WaterDepth, float Turbidity) const;

    // Visibility calculation helpers
    EVisibilityLevel CalculateVisibilityLevel(float LightLevel, float ScatteringFactor, float Range) const;
    float CalculateContrastSensitivity(float AmbientLight, float TargetIllumination) const;
    void ProcessVisualAdaptation(AActor* Actor, float CurrentLightLevel, float DeltaTime);
    bool IsLocationIlluminated(const FVector& Location, float MinimumLux = 1.0f) const;

    // Environmental effect helpers
    void ProcessMoistureOnLenses(class AClimbingToolBase* LightTool, float Humidity, float DeltaTime);
    void ProcessCondensationFogging(class AClimbingToolBase* LightTool, float TemperatureDifference);
    void ProcessVibrationDamage(class AClimbingToolBase* LightTool, float VibrationIntensity, float DeltaTime);
    void ProcessColdWeatherEffects(class AClimbingToolBase* LightTool, float Temperature, float DeltaTime);

    // Emergency lighting helpers
    void ActivateBackupSystems(AActor* Actor);
    void ProcessLightConservation(AActor* Actor, float RemainingBattery);
    void CalculateEmergencyLightDuration(AActor* Actor);
    bool IsEmergencyLightingAdequate(const FVector& Location) const;

    // Performance optimization helpers
    bool ShouldUpdateLightSource(const FLightSourceData& LightSource, float DeltaTime) const;
    float GetLightUpdateFrequencyForDistance(float Distance) const;
    void OptimizeLightCalculations(float ViewerDistance);

private:
    // Performance tracking
    float LastFullUpdate = 0.0f;
    float FullUpdateInterval = 0.1f; // 10Hz for full lighting simulation
    float LastVisibilityUpdate = 0.0f;
    float VisibilityUpdateInterval = 0.2f; // 5Hz for visibility updates
    float LastEnvironmentUpdate = 0.0f;
    float EnvironmentUpdateInterval = 0.5f; // 2Hz for environmental effects
    
    int32 CurrentLODLevel = 0;
    float ViewerDistance = 0.0f;

    // Lighting simulation state
    bool bSimulateFullLighting = true;
    bool bSimulateEnvironmentalEffects = true;
    bool bSimulateLightScattering = true;
    int32 MaxSimulatedLights = 20;

    // Emergency lighting state
    bool bEmergencyLightingActive = false;
    float EmergencyLightingDuration = 0.0f;
    TArray<FLightSourceData> EmergencyLights;

    // Visual adaptation tracking
    TMap<AActor*, float> ActorLightAdaptationLevels; // 0-1 adaptation to current light
    TMap<AActor*, float> ActorAdaptationTimers; // Time spent at current light level

    // Environmental particle tracking
    float DustParticleDensity = 0.1f;
    float MoistureParticleDensity = 0.8f;
    float SiltParticleDensity = 0.0f; // For underwater caves

    // Light beam intersection data for performance
    TArray<FVector> LightBeamIntersections;
    float LastBeamUpdate = 0.0f;
    float BeamUpdateInterval = 0.5f; // 2Hz

    // Cached calculations for performance
    mutable FVisibilityData CachedVisibility;
    mutable FVector CachedVisibilityLocation = FVector::ZeroVector;
    mutable float CachedVisibilityTime = 0.0f;
    static constexpr float VisibilityCacheTimeout = 0.2f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float BaseCaveDarkness = 0.0f; // Lux (caves are naturally dark)

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float LightEfficiencyUnderwater = 0.3f; // Light penetration in water

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float AdaptationRate = 0.1f; // Rate of visual adaptation

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MinimumUsableLightLevel = 0.1f; // Lux minimum for navigation

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float BatteryDegradationRate = 0.01f; // % efficiency loss per hour

    UPROPERTY(EditAnywhere, Category = "Configuration")
    int32 MaxTrackedLightSources = 50;

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Configuration")
    bool bEnableDynamicLighting = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Configuration")
    bool bEnableEnvironmentalEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Configuration")
    bool bEnableEmergencyLighting = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Configuration")
    bool bEnableVisualAdaptation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting Configuration")
    float GlobalLightingIntensity = 1.0f;

    // Integration with other systems
    UPROPERTY()
    UCaveEnvironmentPhysics* CavePhysics;

    UPROPERTY()
    UWaterPhysicsComponent* WaterPhysics;

    UPROPERTY()
    class UEnvironmentalHazardManager* EnvironmentalManager;

    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;
};