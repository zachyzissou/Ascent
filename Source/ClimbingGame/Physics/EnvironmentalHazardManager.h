#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Engine/PostProcessVolume.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "EnvironmentalHazardManager.generated.h"

// Forward declarations for water and cave systems
class UWaterPhysicsComponent;
class UCaveEnvironmentPhysics;
class UWaterfallRappellingPhysics;
class UCaveDivingPhysics;

UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear       UMETA(DisplayName = "Clear"),
    Overcast    UMETA(DisplayName = "Overcast"),
    LightRain   UMETA(DisplayName = "Light Rain"),
    HeavyRain   UMETA(DisplayName = "Heavy Rain"),
    Snow        UMETA(DisplayName = "Snow"),
    Blizzard    UMETA(DisplayName = "Blizzard"),
    Fog         UMETA(DisplayName = "Fog"),
    Sandstorm   UMETA(DisplayName = "Sandstorm"),
    Flood       UMETA(DisplayName = "Flash Flood"),
    FlashFreeze UMETA(DisplayName = "Flash Freeze")
};

UENUM(BlueprintType)
enum class EGeologicalHazard : uint8
{
    None        UMETA(DisplayName = "None"),
    Rockfall    UMETA(DisplayName = "Rockfall"),
    Landslide   UMETA(DisplayName = "Landslide"),
    Avalanche   UMETA(DisplayName = "Avalanche"),
    Earthquake  UMETA(DisplayName = "Earthquake"),
    CaveIn      UMETA(DisplayName = "Cave-in"),
    FloodingCave UMETA(DisplayName = "Cave Flooding"),
    Sinkhole    UMETA(DisplayName = "Sinkhole")
};

UENUM(BlueprintType)
enum class EHazardSeverity : uint8
{
    Minimal     UMETA(DisplayName = "Minimal"),
    Light       UMETA(DisplayName = "Light"),
    Moderate    UMETA(DisplayName = "Moderate"),
    Severe      UMETA(DisplayName = "Severe"),
    Extreme     UMETA(DisplayName = "Extreme")
};

USTRUCT(BlueprintType)
struct FWindData
{
    GENERATED_BODY()

    // Wind properties
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Wind")
    FVector Direction = FVector(1.0f, 0.0f, 0.0f);

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Wind")
    float Speed = 0.0f; // m/s

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Wind")
    float Gusts = 0.0f; // Additional gust strength

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Wind")
    float Turbulence = 0.0f; // 0-1, randomness factor

    UPROPERTY(BlueprintReadWrite, Category = "Wind")
    float GustFrequency = 1.0f; // Gusts per minute

    UPROPERTY(BlueprintReadWrite, Category = "Wind")
    float DirectionVariation = 15.0f; // Degrees of direction change
};

USTRUCT(BlueprintType)
struct FWeatherData
{
    GENERATED_BODY()

    // Weather properties
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weather")
    EWeatherType Type = EWeatherType::Clear;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weather")
    float Intensity = 0.0f; // 0-1

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weather")
    float Temperature = 20.0f; // Celsius

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weather")
    float Humidity = 50.0f; // %

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weather")
    float Precipitation = 0.0f; // mm/hour

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Weather")
    float Visibility = 1.0f; // 0-1, 1 = perfect visibility

    UPROPERTY(BlueprintReadWrite, Category = "Weather")
    float PressureChange = 0.0f; // hPa/hour (for storm prediction)
};

USTRUCT(BlueprintType)
struct FGeologicalData
{
    GENERATED_BODY()

    // Geological hazard properties
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Geological")
    EGeologicalHazard HazardType = EGeologicalHazard::None;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Geological")
    EHazardSeverity Severity = EHazardSeverity::Minimal;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Geological")
    FVector EpicenterLocation = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Geological")
    float Magnitude = 0.0f; // Richter scale for earthquakes, 0-10 for others

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Geological")
    float Duration = 0.0f; // Seconds

    UPROPERTY(BlueprintReadWrite, Category = "Geological")
    float AffectedRadius = 1000.0f; // cm

    UPROPERTY(BlueprintReadWrite, Category = "Geological")
    TArray<FVector> FallZones; // Areas where debris will fall
};

USTRUCT(BlueprintType)
struct FEnvironmentalEffects
{
    GENERATED_BODY()

    // Combined effects on gameplay
    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float GripStrengthMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float FrictionMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float StaminaDrainMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float ToolAccuracyMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float RopeSwayMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float VisibilityMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float EquipmentDurabilityMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    float MovementSpeedMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effects")
    FVector BalanceOffset = FVector::ZeroVector; // Wind-induced balance adjustment
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalHazardManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalHazardManager();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Current environmental state
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Environmental State")
    FWindData CurrentWind;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Environmental State")
    FWeatherData CurrentWeather;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Environmental State")
    FGeologicalData CurrentGeological;

    UPROPERTY(BlueprintReadOnly, Category = "Environmental State")
    FEnvironmentalEffects CombinedEffects;

    // Environmental simulation control
    UFUNCTION(BlueprintCallable, Category = "Environmental Control")
    void SetWindConditions(const FWindData& NewWind);

    UFUNCTION(BlueprintCallable, Category = "Environmental Control")
    void SetWeatherConditions(const FWeatherData& NewWeather);

    UFUNCTION(BlueprintCallable, Category = "Environmental Control")
    void TriggerGeologicalEvent(const FGeologicalData& EventData);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetWindConditions(const FWindData& NewWind);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetWeatherConditions(const FWeatherData& NewWeather);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTriggerGeologicalEvent(const FGeologicalData& EventData);

    // Environmental effects calculation
    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    FEnvironmentalEffects CalculateEnvironmentalEffects(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    float GetWindForceAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    FVector GetWindVelocityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    float GetTemperatureAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    float GetVisibilityAtLocation(const FVector& Location) const;

    // Integration with existing systems
    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void ApplyWindToRope(UAdvancedRopeComponent* RopeComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void ApplyEnvironmentalEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "System Integration")
    void UpdateEquipmentDurability(class AClimbingToolBase* Tool, float DeltaTime);

    // Water and cave system integration
    UFUNCTION(BlueprintCallable, Category = "Water Integration")
    void ProcessFloodConditions(UWaterPhysicsComponent* WaterPhysics, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Water Integration")
    void ApplyFlashFloodHazard(const FVector& FloodOrigin, float FloodSeverity);

    UFUNCTION(BlueprintCallable, Category = "Water Integration")
    void ProcessWaterfallEnvironmentalEffects(UWaterfallRappellingPhysics* WaterfallPhysics, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Integration")
    void ProcessCaveEnvironmentalHazards(UCaveEnvironmentPhysics* CavePhysics, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Integration")
    void ApplyCaveInHazard(const FVector& CaveLocation, float Severity);

    UFUNCTION(BlueprintCallable, Category = "Cave Integration")
    void ProcessUnderwaterCaveHazards(UCaveDivingPhysics* CaveDivingPhysics, float DeltaTime);

    // Weather transition system
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void StartWeatherTransition(EWeatherType TargetWeather, float TransitionDuration = 60.0f);

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void UpdateWeatherTransition(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    bool IsWeatherTransitioning() const { return bIsTransitioning; }

    // Geological hazard simulation
    UFUNCTION(BlueprintCallable, Category = "Geological Hazards")
    void SimulateRockfall(const FVector& Origin, float Intensity, float Duration);

    UFUNCTION(BlueprintCallable, Category = "Geological Hazards")
    void SimulateEarthquake(const FVector& Epicenter, float Magnitude, float Duration);

    UFUNCTION(BlueprintCallable, Category = "Geological Hazards")
    void SimulateAvalanche(const FVector& StartLocation, const FVector& Direction, float Volume);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetEnvironmentalLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForDistance(float ViewerDistance);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Environmental Events")
    FSimpleMulticastDelegate OnWeatherChanged;

    UPROPERTY(BlueprintAssignable, Category = "Environmental Events")
    FSimpleMulticastDelegate OnGeologicalEventStart;

    UPROPERTY(BlueprintAssignable, Category = "Environmental Events")
    FSimpleMulticastDelegate OnGeologicalEventEnd;

    UPROPERTY(BlueprintAssignable, Category = "Environmental Events")
    FSimpleMulticastDelegate OnSevereWeatherWarning;

protected:
    // Internal calculation functions
    void UpdateWindEffects(float DeltaTime);
    void UpdateWeatherEffects(float DeltaTime);
    void UpdateGeologicalEffects(float DeltaTime);
    void UpdateCombinedEffects();

    // Wind simulation
    void SimulateWindTurbulence(float DeltaTime);
    void ApplyWindToPhysicsObjects(float DeltaTime);
    FVector CalculateWindForceOnObject(const FVector& ObjectLocation, const FVector& ObjectSize) const;

    // Weather simulation
    void ProcessPrecipitationEffects(float DeltaTime);
    void ProcessTemperatureEffects(float DeltaTime);
    void ProcessVisibilityEffects(float DeltaTime);
    float CalculateWetnessLevel(const FVector& Location) const;

    // Geological simulation
    void ProcessActiveGeologicalEvent(float DeltaTime);
    void SpawnRockfallDebris(const FVector& Location, float Intensity);
    void ApplySeismicForces(float DeltaTime);
    TArray<AActor*> GetAffectedActors(const FVector& Center, float Radius) const;

    // Performance optimization helpers
    bool ShouldSimulateFullDetail() const;
    float GetUpdateFrequencyForDistance(float Distance) const;
    void CullDistantEffects(float ViewerDistance);

private:
    // Weather transition system
    bool bIsTransitioning = false;
    EWeatherType TransitionTarget = EWeatherType::Clear;
    float TransitionDuration = 60.0f;
    float TransitionProgress = 0.0f;
    FWeatherData TransitionStartWeather;

    // Geological event tracking
    float GeologicalEventTimer = 0.0f;
    bool bGeologicalEventActive = false;
    TArray<AActor*> SpawnedDebris;

    // Performance optimization
    float LastFullUpdate = 0.0f;
    float FullUpdateInterval = 0.1f; // 10Hz for full simulation
    float LastLightUpdate = 0.0f;
    float LightUpdateInterval = 0.5f; // 2Hz for basic updates
    
    int32 CurrentLODLevel = 0;
    float ViewerDistance = 0.0f;

    // Wind variation tracking
    float WindDirectionTimer = 0.0f;
    float WindGustTimer = 0.0f;
    FVector BaseWindDirection = FVector(1.0f, 0.0f, 0.0f);

    // Cached calculations for performance
    mutable float CachedWindForce = 0.0f;
    mutable FVector CachedWindLocation = FVector::ZeroVector;
    mutable float CachedWindTime = 0.0f;
    static constexpr float WindCacheTimeout = 0.1f;

    // Environmental effect multipliers (configurable)
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float WindToRopeSwayMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float RainToGripMultiplier = 0.7f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float SnowToFrictionMultiplier = 0.6f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float ColdToStaminaMultiplier = 1.3f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float FogToVisibilityMultiplier = 0.3f;

    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxSimulationDistance = 10000.0f; // 100m

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxDebrisObjects = 100;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseMultithreading = true;

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableWindSimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableWeatherEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableGeologicalHazards = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    float GlobalIntensityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableWaterPhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableCavePhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableWaterfallPhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Configuration")
    bool bEnableCaveDivingPhysics = true;

    // Water and cave physics system references
    UPROPERTY()
    TArray<UWaterPhysicsComponent*> WaterPhysicsComponents;

    UPROPERTY()
    TArray<UCaveEnvironmentPhysics*> CavePhysicsComponents;

    UPROPERTY()
    TArray<UWaterfallRappellingPhysics*> WaterfallPhysicsComponents;

    UPROPERTY()
    TArray<UCaveDivingPhysics*> CaveDivingPhysicsComponents;
};