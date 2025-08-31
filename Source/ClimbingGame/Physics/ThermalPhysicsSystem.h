#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "EnvironmentalHazardManager.h"
#include "ThermalPhysicsSystem.generated.h"

UENUM(BlueprintType)
enum class EThermalZoneType : uint8
{
    Ambient         UMETA(DisplayName = "Ambient"),
    Solar           UMETA(DisplayName = "Solar Heated"),
    Shade           UMETA(DisplayName = "Shade"),
    Underground     UMETA(DisplayName = "Underground"),
    Water           UMETA(DisplayName = "Water Source"),
    Geothermal      UMETA(DisplayName = "Geothermal"),
    Artificial      UMETA(DisplayName = "Artificial Heating/Cooling")
};

UENUM(BlueprintType)
enum class EThermalMaterial : uint8
{
    Rock            UMETA(DisplayName = "Rock"),
    Metal           UMETA(DisplayName = "Metal"),
    Wood            UMETA(DisplayName = "Wood"),
    Plastic         UMETA(DisplayName = "Plastic"),
    Rubber          UMETA(DisplayName = "Rubber"),
    Fabric          UMETA(DisplayName = "Fabric"),
    Ice             UMETA(DisplayName = "Ice"),
    Water           UMETA(DisplayName = "Water"),
    Air             UMETA(DisplayName = "Air")
};

UENUM(BlueprintType)
enum class EThermalComfortLevel : uint8
{
    HypothermiaRisk UMETA(DisplayName = "Hypothermia Risk"),
    Cold            UMETA(DisplayName = "Cold"),
    Cool            UMETA(DisplayName = "Cool"),
    Comfortable     UMETA(DisplayName = "Comfortable"),
    Warm            UMETA(DisplayName = "Warm"),
    Hot             UMETA(DisplayName = "Hot"),
    HeatStrokeRisk  UMETA(DisplayName = "Heat Stroke Risk")
};

USTRUCT(BlueprintType)
struct FThermalProperties
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float ThermalConductivity = 1.0f; // W/m·K

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float SpecificHeat = 1000.0f; // J/kg·K

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float Density = 1000.0f; // kg/m³

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float ThermalDiffusivity = 1.0f; // m²/s (calculated from other properties)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float LinearExpansionCoeff = 0.000012f; // 1/K

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float VolumetricExpansionCoeff = 0.000036f; // 1/K

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float Emissivity = 0.9f; // 0-1, for radiation calculations

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Properties")
    float SolarAbsorption = 0.7f; // 0-1, fraction of solar energy absorbed
};

USTRUCT(BlueprintType)
struct FThermalZone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Zone")
    FString ZoneID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Zone")
    EThermalZoneType ZoneType = EThermalZoneType::Ambient;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Zone")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Zone")
    FVector Extent = FVector(1000.0f, 1000.0f, 1000.0f); // cm

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal Zone")
    float BaseTemperature = 20.0f; // Celsius

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal Zone")
    float CurrentTemperature = 20.0f; // Celsius

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Zone")
    float TemperatureVariation = 2.0f; // Random variation range

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Zone")
    float HeatGeneration = 0.0f; // W/m³ - internal heat generation

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Zone")
    float HeatLoss = 0.0f; // W/m³ - heat loss to environment

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Zone")
    TArray<FString> ConnectedZones; // Adjacent thermal zones
};

USTRUCT(BlueprintType)
struct FThermalObject
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Object")
    FString ObjectID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Object")
    EThermalMaterial Material = EThermalMaterial::Rock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Object")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Object")
    FVector Size = FVector(100.0f, 100.0f, 100.0f); // cm

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Thermal Object")
    float CurrentTemperature = 20.0f; // Celsius

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Object")
    float Mass = 100.0f; // kg

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Object")
    FThermalProperties ThermalProps;

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Object")
    float ThermalStress = 0.0f; // Pa - current thermal stress

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Object")
    FVector ThermalExpansion = FVector::ZeroVector; // cm - current expansion

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Object")
    bool bHasConstraints = false; // Whether expansion is constrained
};

USTRUCT(BlueprintType)
struct FHeatTransferData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Heat Transfer")
    float ConductionRate = 0.0f; // W

    UPROPERTY(BlueprintReadOnly, Category = "Heat Transfer")
    float ConvectionRate = 0.0f; // W

    UPROPERTY(BlueprintReadOnly, Category = "Heat Transfer")
    float RadiationRate = 0.0f; // W

    UPROPERTY(BlueprintReadOnly, Category = "Heat Transfer")
    float TotalHeatFlow = 0.0f; // W

    UPROPERTY(BlueprintReadOnly, Category = "Heat Transfer")
    FVector HeatFlowDirection = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FClimberThermalState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float CoreTemperature = 37.0f; // Celsius

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float SkinTemperature = 33.0f; // Celsius

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    EThermalComfortLevel ComfortLevel = EThermalComfortLevel::Comfortable;

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float SweatRate = 0.0f; // L/hour

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float ShiveringIntensity = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float MetabolicHeatGeneration = 100.0f; // W (resting rate)

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float ClothingInsulation = 1.0f; // Clo units

    UPROPERTY(BlueprintReadOnly, Category = "Climber Thermal")
    float HeatLossRate = 0.0f; // W
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UThermalPhysicsSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UThermalPhysicsSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Thermal zones and objects
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thermal System")
    TArray<FThermalZone> ThermalZones;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thermal System")
    TArray<FThermalObject> ThermalObjects;

    // Environmental hazard manager reference
    UPROPERTY(BlueprintReadWrite, Category = "System Integration")
    UEnvironmentalHazardManager* HazardManager;

    // Thermal zone management
    UFUNCTION(BlueprintCallable, Category = "Thermal Zones")
    void RegisterThermalZone(const FThermalZone& Zone);

    UFUNCTION(BlueprintCallable, Category = "Thermal Zones")
    void UnregisterThermalZone(const FString& ZoneID);

    UFUNCTION(BlueprintCallable, Category = "Thermal Zones")
    FThermalZone GetThermalZone(const FString& ZoneID) const;

    UFUNCTION(BlueprintCallable, Category = "Thermal Zones")
    TArray<FString> GetThermalZonesAtLocation(const FVector& Location) const;

    // Thermal object management
    UFUNCTION(BlueprintCallable, Category = "Thermal Objects")
    void RegisterThermalObject(const FThermalObject& Object);

    UFUNCTION(BlueprintCallable, Category = "Thermal Objects")
    void UnregisterThermalObject(const FString& ObjectID);

    UFUNCTION(BlueprintCallable, Category = "Thermal Objects")
    FThermalObject GetThermalObject(const FString& ObjectID) const;

    // Temperature calculations
    UFUNCTION(BlueprintCallable, Category = "Temperature")
    float GetTemperatureAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    FVector GetTemperatureGradient(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    float CalculateApparentTemperature(const FVector& Location, float WindSpeed, float Humidity) const;

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    float CalculateWindChillTemperature(float AirTemp, float WindSpeed) const;

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    float CalculateHeatIndexTemperature(float AirTemp, float Humidity) const;

    // Heat transfer calculations
    UFUNCTION(BlueprintCallable, Category = "Heat Transfer")
    FHeatTransferData CalculateHeatTransfer(const FString& FromObjectID, const FString& ToObjectID) const;

    UFUNCTION(BlueprintCallable, Category = "Heat Transfer")
    float CalculateConductiveHeatTransfer(float TempDifference, float ThermalConductivity, float Area, float Distance) const;

    UFUNCTION(BlueprintCallable, Category = "Heat Transfer")
    float CalculateConvectiveHeatTransfer(float TempDifference, float HeatTransferCoeff, float Area) const;

    UFUNCTION(BlueprintCallable, Category = "Heat Transfer")
    float CalculateRadiativeHeatTransfer(float TempHot, float TempCold, float Emissivity, float Area) const;

    // Thermal expansion
    UFUNCTION(BlueprintCallable, Category = "Thermal Expansion")
    FVector CalculateThermalExpansion(const FString& ObjectID, float TemperatureChange) const;

    UFUNCTION(BlueprintCallable, Category = "Thermal Expansion")
    float CalculateThermalStress(const FString& ObjectID, float TemperatureChange, bool bConstrained) const;

    UFUNCTION(BlueprintCallable, Category = "Thermal Expansion")
    void ApplyThermalExpansionToObject(const FString& ObjectID, float DeltaTime);

    // Solar heating
    UFUNCTION(BlueprintCallable, Category = "Solar Heating")
    float CalculateSolarHeatGain(const FVector& Location, const FVector& SurfaceNormal, float SolarIntensity) const;

    UFUNCTION(BlueprintCallable, Category = "Solar Heating")
    void UpdateSolarHeating(float SolarIntensity, const FVector& SunDirection, float DeltaTime);

    // Integration with climbing systems
    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    void ApplyThermalEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    void ApplyThermalEffectsToRope(UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    void ApplyThermalEffectsToEquipment(AActor* Equipment, float DeltaTime);

    // Climber thermal physiology
    UFUNCTION(BlueprintCallable, Category = "Climber Physiology")
    FClimberThermalState CalculateClimberThermalState(const FVector& ClimberLocation, float ActivityLevel, float ClothingLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Climber Physiology")
    EThermalComfortLevel DetermineComfortLevel(float EffectiveTemp) const;

    UFUNCTION(BlueprintCallable, Category = "Climber Physiology")
    float CalculateMetabolicHeatGeneration(float ActivityLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Climber Physiology")
    float CalculateSweatRate(float CoreTemp, float SkinTemp, float ActivityLevel) const;

    // Thermal comfort calculations
    UFUNCTION(BlueprintCallable, Category = "Thermal Comfort")
    float CalculatePMV(float AirTemp, float RadiantTemp, float WindSpeed, float Humidity, float ActivityLevel, float ClothingLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Thermal Comfort")
    float CalculatePPD(float PMV) const;

    // Material property system
    UFUNCTION(BlueprintCallable, Category = "Material Properties")
    FThermalProperties GetMaterialThermalProperties(EThermalMaterial Material) const;

    UFUNCTION(BlueprintCallable, Category = "Material Properties")
    void SetCustomMaterialProperties(EThermalMaterial Material, const FThermalProperties& Properties);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetThermalLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeThermalSimulation(float ViewerDistance);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Thermal Events")
    FSimpleMulticastDelegate OnExtremeTemperature;

    UPROPERTY(BlueprintAssignable, Category = "Thermal Events")
    FSimpleMulticastDelegate OnThermalStressLimit;

    UPROPERTY(BlueprintAssignable, Category = "Thermal Events")
    FSimpleMulticastDelegate OnHypothermiaRisk;

    UPROPERTY(BlueprintAssignable, Category = "Thermal Events")
    FSimpleMulticastDelegate OnHeatStrokeRisk;

protected:
    // Internal thermal simulation
    void UpdateThermalZones(float DeltaTime);
    void UpdateThermalObjects(float DeltaTime);
    void ProcessHeatTransfer(float DeltaTime);
    void UpdateThermalExpansion(float DeltaTime);

    // Heat transfer calculations
    void CalculateZoneToZoneHeatTransfer(FThermalZone& FromZone, FThermalZone& ToZone, float DeltaTime);
    void CalculateObjectToZoneHeatTransfer(FThermalObject& Object, FThermalZone& Zone, float DeltaTime);
    void CalculateObjectToObjectHeatTransfer(FThermalObject& ObjectA, FThermalObject& ObjectB, float DeltaTime);

    // Temperature distribution
    void SolveThermalDiffusion(float DeltaTime);
    void ApplyThermalBoundaryConditions();
    float CalculateFiniteDifference(const FVector& Location, float GridSpacing) const;

    // Material property loading
    void InitializeMaterialProperties();
    void LoadThermalPropertiesTable();

    // Environmental integration
    void ProcessEnvironmentalHeating(float DeltaTime);
    void ApplyWeatherThermalEffects(float DeltaTime);
    void CalculateGroundTemperature();

private:
    // Performance optimization state
    int32 CurrentLOD = 0;
    float LastLODUpdate = 0.0f;
    float LODUpdateInterval = 2.0f;

    // Update frequencies
    float ZoneUpdateInterval = 1.0f;      // 1Hz
    float ObjectUpdateInterval = 0.5f;    // 2Hz
    float HeatTransferInterval = 0.2f;    // 5Hz
    float ExpansionUpdateInterval = 2.0f; // 0.5Hz

    float LastZoneUpdate = 0.0f;
    float LastObjectUpdate = 0.0f;
    float LastHeatTransferUpdate = 0.0f;
    float LastExpansionUpdate = 0.0f;

    // Material property tables
    TMap<EThermalMaterial, FThermalProperties> MaterialPropertiesTable;

    // Thermal simulation cache
    mutable TMap<FVector, float> TemperatureCache;
    mutable TMap<FVector, FVector> TemperatureGradientCache;
    mutable float LastCacheUpdate = 0.0f;
    static constexpr float CacheTimeout = 1.0f;

    // Thermal diffusion grid (for advanced simulation)
    struct FThermalGridNode
    {
        FVector Location;
        float Temperature;
        float ThermalMass;
        EThermalMaterial Material;
    };
    TArray<FThermalGridNode> ThermalGrid;
    float GridSpacing = 1000.0f; // 10m grid spacing
    bool bUseThermalGrid = false;

    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxSimulationDistance = 10000.0f; // cm

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseAdvancedHeatTransfer = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseThermalCache = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bSimulateThermalExpansion = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxThermalObjects = 100;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float GlobalThermalMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float AmbientTemperature = 20.0f; // Default ambient temperature

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float GroundTemperature = 15.0f; // Default ground temperature

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float ThermalTimeConstant = 3600.0f; // Seconds for thermal equilibrium

    // Physical constants
    static constexpr float StefanBoltzmannConstant = 5.67e-8f; // W/(m²·K⁴)
    static constexpr float AbsoluteZero = -273.15f; // Celsius
    static constexpr float AirDensity = 1.225f; // kg/m³ at sea level
    static constexpr float AirSpecificHeat = 1005.0f; // J/(kg·K)

public:
    // Static utility functions
    UFUNCTION(BlueprintCallable, Category = "Thermal Utilities", CallInEditor = true)
    static float ConvertCelsiusToKelvin(float Celsius);

    UFUNCTION(BlueprintCallable, Category = "Thermal Utilities", CallInEditor = true)
    static float ConvertKelvinToCelsius(float Kelvin);

    UFUNCTION(BlueprintCallable, Category = "Thermal Utilities", CallInEditor = true)
    static float ConvertCelsiusToFahrenheit(float Celsius);

    UFUNCTION(BlueprintCallable, Category = "Thermal Utilities", CallInEditor = true)
    static float ConvertFahrenheitToCelsius(float Fahrenheit);

    UFUNCTION(BlueprintCallable, Category = "Thermal Utilities", CallInEditor = true)
    static float CalculateThermalDiffusivity(float Conductivity, float Density, float SpecificHeat);

    UFUNCTION(BlueprintCallable, Category = "Thermal Utilities", CallInEditor = true)
    static FString GetTemperatureDescription(float Temperature);
};