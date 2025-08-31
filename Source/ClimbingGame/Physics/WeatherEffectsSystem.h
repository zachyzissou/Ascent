#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialParameterCollection.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "EnvironmentalHazardManager.h"
#include "WeatherEffectsSystem.generated.h"

UENUM(BlueprintType)
enum class EPrecipitationType : uint8
{
    None        UMETA(DisplayName = "None"),
    Drizzle     UMETA(DisplayName = "Drizzle"),
    Rain        UMETA(DisplayName = "Rain"),
    HeavyRain   UMETA(DisplayName = "Heavy Rain"),
    Snow        UMETA(DisplayName = "Snow"),
    HeavySnow   UMETA(DisplayName = "Heavy Snow"),
    Sleet       UMETA(DisplayName = "Sleet"),
    Hail        UMETA(DisplayName = "Hail")
};

UENUM(BlueprintType)
enum class ESurfaceWetness : uint8
{
    Dry         UMETA(DisplayName = "Dry"),
    Damp        UMETA(DisplayName = "Damp"),
    Wet         UMETA(DisplayName = "Wet"),
    Soaked      UMETA(DisplayName = "Soaked"),
    Flooded     UMETA(DisplayName = "Flooded")
};

UENUM(BlueprintType)
enum class EIceFormation : uint8
{
    None        UMETA(DisplayName = "None"),
    Frost       UMETA(DisplayName = "Frost"),
    ThinIce     UMETA(DisplayName = "Thin Ice"),
    ThickIce    UMETA(DisplayName = "Thick Ice"),
    BlackIce    UMETA(DisplayName = "Black Ice"),
    Verglas     UMETA(DisplayName = "Verglas")  // Thin ice coating
};

USTRUCT(BlueprintType)
struct FSurfaceWeatherData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    ESurfaceWetness WetnessLevel = ESurfaceWetness::Dry;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    EIceFormation IceLevel = EIceFormation::None;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    float Temperature = 20.0f; // Celsius

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    float FrictionMultiplier = 1.0f;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    float GripStrengthMultiplier = 1.0f;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    float WetnessAccumulation = 0.0f; // 0-1, how much water has accumulated

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Surface Weather")
    float SnowDepth = 0.0f; // cm

    UPROPERTY(BlueprintReadWrite, Category = "Surface Weather")
    float DrainageRate = 0.1f; // How quickly water drains away

    UPROPERTY(BlueprintReadWrite, Category = "Surface Weather")
    float AbsorptionRate = 0.05f; // How quickly surface absorbs water
};

USTRUCT(BlueprintType)
struct FPrecipitationData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Precipitation")
    EPrecipitationType Type = EPrecipitationType::None;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Precipitation")
    float Intensity = 0.0f; // 0-1

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Precipitation")
    float Rate = 0.0f; // mm/hour

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Precipitation")
    float DropletSize = 2.0f; // mm average

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Precipitation")
    FVector WindDrift = FVector::ZeroVector; // Wind effect on precipitation

    UPROPERTY(BlueprintReadWrite, Category = "Precipitation")
    float TemperatureThreshold = 0.0f; // Temperature below which rain becomes snow
};

USTRUCT(BlueprintType)
struct FThermalData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal")
    float AmbientTemperature = 20.0f; // Celsius

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal")
    float SurfaceTemperature = 20.0f; // Celsius

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal")
    float ThermalConductivity = 1.0f; // How quickly surface temperature changes

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal")
    float SpecificHeat = 1.0f; // Thermal mass of surface

    UPROPERTY(BlueprintReadWrite, Category = "Thermal")
    float SolarHeating = 0.0f; // W/m² from sun

    UPROPERTY(BlueprintReadWrite, Category = "Thermal")
    float RadiativeCooling = 0.0f; // W/m² lost to radiation

    UPROPERTY(BlueprintReadWrite, Category = "Thermal")
    float ConvectiveCooling = 0.0f; // W/m² lost to air convection
};

USTRUCT(BlueprintType)
struct FClimberComfort
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Climber Comfort")
    float BodyTemperature = 37.0f; // Celsius

    UPROPERTY(BlueprintReadOnly, Category = "Climber Comfort")
    float ComfortLevel = 1.0f; // 0-1, affects performance

    UPROPERTY(BlueprintReadOnly, Category = "Climber Comfort")
    float HypothermiaRisk = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Climber Comfort")
    float HeatStressRisk = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Climber Comfort")
    float DehydrationLevel = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Climber Comfort")
    float ShiveringIntensity = 0.0f; // 0-1, affects grip precision
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UWeatherEffectsSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeatherEffectsSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Current weather conditions
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Weather State")
    FPrecipitationData CurrentPrecipitation;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Weather State")
    FThermalData CurrentThermal;

    UPROPERTY(BlueprintReadOnly, Category = "Weather State")
    TMap<FString, FSurfaceWeatherData> SurfaceWeatherMap;

    // Environmental hazard manager reference
    UPROPERTY(BlueprintReadWrite, Category = "System Integration")
    UEnvironmentalHazardManager* HazardManager;

    // Precipitation simulation
    UFUNCTION(BlueprintCallable, Category = "Precipitation")
    void SetPrecipitation(EPrecipitationType Type, float Intensity, float Rate);

    UFUNCTION(BlueprintCallable, Category = "Precipitation")
    void UpdatePrecipitationEffects(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Precipitation")
    float GetWetnessAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Precipitation")
    void ApplyPrecipitationToSurface(const FString& SurfaceID, float DeltaTime);

    // Surface weather effects
    UFUNCTION(BlueprintCallable, Category = "Surface Weather")
    FSurfaceWeatherData GetSurfaceWeatherData(const FString& SurfaceID) const;

    UFUNCTION(BlueprintCallable, Category = "Surface Weather")
    void UpdateSurfaceWeather(const FString& SurfaceID, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Surface Weather")
    float CalculateFrictionReduction(const FSurfaceWeatherData& SurfaceData) const;

    UFUNCTION(BlueprintCallable, Category = "Surface Weather")
    float CalculateGripReduction(const FSurfaceWeatherData& SurfaceData) const;

    // Thermal effects
    UFUNCTION(BlueprintCallable, Category = "Thermal Effects")
    void UpdateThermalConditions(float AmbientTemp, float SolarHeating, float WindSpeed);

    UFUNCTION(BlueprintCallable, Category = "Thermal Effects")
    float GetSurfaceTemperature(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Thermal Effects")
    void ProcessThermalExpansion(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Thermal Effects")
    float CalculateThermalStress(const FString& MaterialType, float TempChange) const;

    // Ice formation and melting
    UFUNCTION(BlueprintCallable, Category = "Ice Physics")
    void UpdateIceFormation(const FString& SurfaceID, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Ice Physics")
    EIceFormation DetermineIceType(float Temperature, float WetnessLevel, float WindSpeed) const;

    UFUNCTION(BlueprintCallable, Category = "Ice Physics")
    float CalculateIceFriction(EIceFormation IceType) const;

    UFUNCTION(BlueprintCallable, Category = "Ice Physics")
    void ProcessIceMelting(const FString& SurfaceID, float Temperature, float DeltaTime);

    // Equipment effects
    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    void ApplyWeatherToEquipment(AClimbingToolBase* Tool, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    void ApplyWeatherToRope(UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    float CalculateMetalExpansion(float TemperatureChange, float ObjectLength) const;

    // Climber physiological effects
    UFUNCTION(BlueprintCallable, Category = "Physiological Effects")
    void UpdateClimberComfort(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Physiological Effects")
    FClimberComfort CalculateClimberComfort(const FVector& ClimberLocation, float ActivityLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Physiological Effects")
    void ApplyTemperatureEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, const FClimberComfort& Comfort);

    // Visibility effects
    UFUNCTION(BlueprintCallable, Category = "Visibility Effects")
    float CalculateVisibilityReduction(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Visibility Effects")
    void UpdateFogEffects(float Humidity, float Temperature, float Pressure);

    UFUNCTION(BlueprintCallable, Category = "Visibility Effects")
    float GetPrecipitationVisibilityReduction() const;

    // Material interaction
    UFUNCTION(BlueprintCallable, Category = "Material Interaction")
    void RegisterSurface(const FString& SurfaceID, const FString& MaterialType, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Material Interaction")
    void UnregisterSurface(const FString& SurfaceID);

    UFUNCTION(BlueprintCallable, Category = "Material Interaction")
    float GetMaterialWeatherResistance(const FString& MaterialType) const;

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetWeatherLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeWeatherSimulation(float ViewerDistance);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Weather Events")
    FSimpleMulticastDelegate OnPrecipitationStart;

    UPROPERTY(BlueprintAssignable, Category = "Weather Events")
    FSimpleMulticastDelegate OnPrecipitationEnd;

    UPROPERTY(BlueprintAssignable, Category = "Weather Events")
    FSimpleMulticastDelegate OnFreezingConditions;

    UPROPERTY(BlueprintAssignable, Category = "Weather Events")
    FSimpleMulticastDelegate OnIceFormation;

    UPROPERTY(BlueprintAssignable, Category = "Weather Events")
    FSimpleMulticastDelegate OnExtremeTemperature;

protected:
    // Internal weather simulation
    void SimulatePrecipitationPhysics(float DeltaTime);
    void UpdateSurfaceWetness(float DeltaTime);
    void ProcessEvaporation(float DeltaTime);
    void CalculateRunoff(const FString& SurfaceID, float DeltaTime);

    // Thermal modeling
    void UpdateSurfaceTemperatures(float DeltaTime);
    void ProcessThermalTransfer(float DeltaTime);
    void CalculateHeatExchange(const FString& SurfaceID, float DeltaTime);

    // Ice physics
    void ProcessFreezingEffects(float DeltaTime);
    void CalculateIceGrowth(const FString& SurfaceID, float DeltaTime);
    void SimulateIceCreep(const FString& SurfaceID, float DeltaTime);

    // Material properties
    void LoadMaterialProperties();
    float GetMaterialThermalConductivity(const FString& MaterialType) const;
    float GetMaterialSpecificHeat(const FString& MaterialType) const;
    float GetMaterialPorosity(const FString& MaterialType) const;

    // Weather pattern simulation
    void UpdateWeatherPatterns(float DeltaTime);
    void ProcessAtmosphericPressureChanges(float DeltaTime);
    void SimulateOrographicEffects(const FVector& Location);

private:
    // Performance optimization
    int32 CurrentLOD = 0;
    float LastLODUpdate = 0.0f;
    float LODUpdateInterval = 1.0f;

    // Update frequencies based on LOD
    float SurfaceUpdateInterval = 0.5f; // 2Hz default
    float ThermalUpdateInterval = 1.0f; // 1Hz default
    float IceUpdateInterval = 2.0f;     // 0.5Hz default

    float LastSurfaceUpdate = 0.0f;
    float LastThermalUpdate = 0.0f;
    float LastIceUpdate = 0.0f;

    // Material property tables
    TMap<FString, float> MaterialThermalConductivity;
    TMap<FString, float> MaterialSpecificHeat;
    TMap<FString, float> MaterialPorosity;
    TMap<FString, float> MaterialWeatherResistance;

    // Surface tracking
    TMap<FString, FString> SurfaceMaterialTypes; // SurfaceID -> MaterialType
    TMap<FString, FVector> SurfaceLocations;     // SurfaceID -> Location

    // Weather simulation cache
    mutable TMap<FVector, float> TemperatureCache;
    mutable TMap<FVector, float> WetnessCache;
    mutable float LastCacheUpdate = 0.0f;
    static constexpr float CacheTimeout = 1.0f; // Cache for 1 second

    // Precipitation physics
    struct FPrecipitationParticle
    {
        FVector Position;
        FVector Velocity;
        float Size;
        float Mass;
        EPrecipitationType Type;
    };
    TArray<FPrecipitationParticle> ActivePrecipitation;

    // Thermal simulation
    struct FThermalNode
    {
        FVector Location;
        float Temperature;
        float ThermalMass;
        TArray<int32> ConnectedNodes;
    };
    TArray<FThermalNode> ThermalNodes;

    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxWeatherSimulationDistance = 10000.0f; // cm

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxSurfaceUpdatesPerFrame = 10;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseWeatherCache = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bSimulateDetailedThermals = true;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Weather Configuration")
    float GlobalWetnessMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Weather Configuration")
    float GlobalThermalMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Weather Configuration")
    float IceFormationThreshold = 0.0f; // Celsius

    UPROPERTY(EditAnywhere, Category = "Weather Configuration")
    float EvaporationRate = 0.1f; // Base evaporation per hour

    UPROPERTY(EditAnywhere, Category = "Weather Configuration")
    bool bEnableIcePhysics = true;

    UPROPERTY(EditAnywhere, Category = "Weather Configuration")
    bool bEnableThermalExpansion = true;

public:
    // Static utility functions
    UFUNCTION(BlueprintCallable, Category = "Weather Utilities", CallInEditor = true)
    static float ConvertFahrenheitToCelsius(float Fahrenheit);

    UFUNCTION(BlueprintCallable, Category = "Weather Utilities", CallInEditor = true)
    static float ConvertCelsiusToFahrenheit(float Celsius);

    UFUNCTION(BlueprintCallable, Category = "Weather Utilities", CallInEditor = true)
    static float CalculateDewPoint(float Temperature, float RelativeHumidity);

    UFUNCTION(BlueprintCallable, Category = "Weather Utilities", CallInEditor = true)
    static float CalculateHeatIndex(float Temperature, float RelativeHumidity);

    UFUNCTION(BlueprintCallable, Category = "Weather Utilities", CallInEditor = true)
    static FString GetTemperatureDescription(float Temperature);

    UFUNCTION(BlueprintCallable, Category = "Weather Utilities", CallInEditor = true)
    static float CalculateWaterVaporPressure(float Temperature, float RelativeHumidity);
};