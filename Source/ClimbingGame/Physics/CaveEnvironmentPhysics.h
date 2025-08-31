#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/PostProcessVolume.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "CaveEnvironmentPhysics.generated.h"

UENUM(BlueprintType)
enum class ECaveType : uint8
{
    Limestone       UMETA(DisplayName = "Limestone Cave"),
    Sandstone       UMETA(DisplayName = "Sandstone Cave"),
    Lava            UMETA(DisplayName = "Lava Tube"),
    Ice             UMETA(DisplayName = "Ice Cave"),
    Sea             UMETA(DisplayName = "Sea Cave"),
    Mine            UMETA(DisplayName = "Mine Shaft"),
    Underground     UMETA(DisplayName = "Underground Chamber")
};

UENUM(BlueprintType)
enum class EAirQuality : uint8
{
    Fresh           UMETA(DisplayName = "Fresh Air"),
    Stale           UMETA(DisplayName = "Stale Air"),
    ThinOxygen      UMETA(DisplayName = "Thin Oxygen"),
    Toxic           UMETA(DisplayName = "Toxic Gases"),
    Methane         UMETA(DisplayName = "Methane Present"),
    CarbonMonoxide  UMETA(DisplayName = "Carbon Monoxide"),
    HydrogenSulfide UMETA(DisplayName = "Hydrogen Sulfide")
};

UENUM(BlueprintType)
enum class ECaveHazard : uint8
{
    None            UMETA(DisplayName = "No Hazards"),
    Rockfall        UMETA(DisplayName = "Rockfall Risk"),
    FloodRisk       UMETA(DisplayName = "Flood Risk"),
    GasAccumulation UMETA(DisplayName = "Gas Accumulation"),
    StructuralWeak  UMETA(DisplayName = "Structural Weakness"),
    BatColony       UMETA(DisplayName = "Bat Colony"),
    Underground     UMETA(DisplayName = "Underground River"),
    Hypothermia     UMETA(DisplayName = "Hypothermia Risk")
};

USTRUCT(BlueprintType)
struct FCaveProperties
{
    GENERATED_BODY()

    // Physical characteristics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    ECaveType CaveType = ECaveType::Limestone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    float Temperature = 12.0f; // Celsius (typical cave temp)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    float Humidity = 90.0f; // % (caves are typically very humid)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    float AirPressure = 101325.0f; // Pascals

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    float CeilingHeight = 300.0f; // cm average

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    float Volume = 10000.0f; // cubic meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    int32 NumberOfEntrances = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    bool bHasUndergroundWater = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Properties")
    float WaterLevel = 0.0f; // cm from cave floor
};

USTRUCT(BlueprintType)
struct FAirCirculationData
{
    GENERATED_BODY()

    // Air flow properties
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    FVector AirFlowDirection = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    float AirFlowVelocity = 0.1f; // m/s (caves have slow air flow)

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    float OxygenLevel = 21.0f; // % by volume

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    float CarbonDioxideLevel = 0.04f; // % by volume

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    float MethaneLevel = 0.0f; // % by volume

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    float CarbonMonoxideLevel = 0.0f; // ppm

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Air Circulation")
    float HydrogenSulfideLevel = 0.0f; // ppm

    UPROPERTY(BlueprintReadWrite, Category = "Air Circulation")
    float AirExchangeRate = 0.5f; // Air changes per hour

    UPROPERTY(BlueprintReadWrite, Category = "Air Circulation")
    float ThermalGradient = 0.5f; // °C per 100m depth
};

USTRUCT(BlueprintType)
struct FCaveLightingData
{
    GENERATED_BODY()

    // Lighting and visibility
    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    float AmbientLightLevel = 0.0f; // Lux (caves are naturally dark)

    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    float ArtificialLightLevel = 0.0f; // Lux from equipment

    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    float VisibilityRange = 0.0f; // meters without light

    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    float DustParticles = 0.1f; // Affects light scattering

    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    float MoistureHaze = 0.8f; // High humidity reduces visibility

    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    TArray<FVector> LightSourceLocations;

    UPROPERTY(BlueprintReadWrite, Category = "Lighting")
    TArray<float> LightSourceIntensities; // Lumens
};

USTRUCT(BlueprintType)
struct FCaveHazardState
{
    GENERATED_BODY()

    // Active hazards
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    TArray<ECaveHazard> ActiveHazards;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float StructuralStability = 1.0f; // 0-1

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float FloodRiskLevel = 0.0f; // 0-1

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float RockfallProbability = 0.01f; // Per hour

    UPROPERTY(BlueprintReadOnly, Category = "Hazards")
    float TimeInCave = 0.0f; // seconds

    UPROPERTY(BlueprintReadOnly, Category = "Hazards")
    bool bEmergencyEvacuation = false;

    UPROPERTY(BlueprintReadOnly, Category = "Hazards")
    float LastGasReading = 0.0f; // Time since last safe reading
};

USTRUCT(BlueprintType)
struct FCavePhysicsGrid
{
    GENERATED_BODY()

    // 3D grid for air circulation and gas distribution
    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    TArray<FAirCirculationData> GridCells;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    FVector GridSize = FVector(2000, 2000, 500); // cm

    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    int32 GridResolutionX = 10;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    int32 GridResolutionY = 10;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    int32 GridResolutionZ = 5;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Grid")
    float UpdateFrequency = 0.1f; // Hz (slow updates for performance)
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UCaveEnvironmentPhysics : public UActorComponent
{
    GENERATED_BODY()

public:
    UCaveEnvironmentPhysics();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Cave configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Configuration")
    FCaveProperties CaveProperties;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Cave State")
    FCavePhysicsGrid PhysicsGrid;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Cave State")
    FCaveHazardState HazardState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cave State")
    FCaveLightingData LightingData;

    // Cave volume definition
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* CaveVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPostProcessVolume* CavePostProcess;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* CaveAmbientAudio;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* AirParticles;

    // Cave entrance/exit points
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Entrances")
    TArray<FVector> EntranceLocations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Entrances")
    TArray<float> EntranceSizes; // Effective diameter in cm

    // Air circulation system
    UFUNCTION(BlueprintCallable, Category = "Air Circulation")
    FAirCirculationData GetAirQualityAtLocation(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Air Circulation")
    void UpdateAirCirculation(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Air Circulation")
    void InitializeAirGrid();

    UFUNCTION(BlueprintCallable, Category = "Air Circulation")
    FVector CalculateAirFlowDirection(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Air Circulation")
    void ProcessGasAccumulation(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Air Circulation")
    void ProcessOxygenDepletion(int32 NumOccupants, float DeltaTime);

    // Gas detection and monitoring
    UFUNCTION(BlueprintCallable, Category = "Gas Detection")
    EAirQuality GetAirQualityRating(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Gas Detection")
    bool IsLocationSafeForBreathing(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Gas Detection")
    float GetToxicGasLevel(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Gas Detection")
    void TriggerGasLeakEvent(const FVector& Source, float Amount, float Duration);

    // Cave lighting system
    UFUNCTION(BlueprintCallable, Category = "Cave Lighting")
    void UpdateLightingSources();

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting")
    float CalculateVisibilityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting")
    void AddLightSource(const FVector& Location, float Intensity);

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting")
    void RemoveLightSource(const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting")
    void ProcessLightEquipmentFailure(class AClimbingToolBase* LightTool);

    // Environmental effects on equipment and characters
    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    void ApplyCaveConditionsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    void ApplyCaveConditionsToRope(UAdvancedRopeComponent* RopeComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    void ProcessHypothermiaInCave(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    void ProcessEquipmentCondensation(class AClimbingToolBase* Tool, float DeltaTime);

    // Cave hazard management
    UFUNCTION(BlueprintCallable, Category = "Cave Hazards")
    void CheckForStructuralHazards(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Hazards")
    void ProcessRockfallRisk(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Hazards")
    void CheckFloodRisk(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Hazards")
    void TriggerEmergencyEvacuation(const FString& Reason);

    UFUNCTION(BlueprintCallable, Category = "Cave Hazards")
    bool IsEvacuationRouteViable() const;

    // Cave exploration mechanics
    UFUNCTION(BlueprintCallable, Category = "Cave Exploration")
    void EnterCave(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Cave Exploration")
    void ExitCave(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Cave Exploration")
    void ProcessCaveNavigation(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Exploration")
    float CalculateGettingLostProbability(AActor* Actor) const;

    // Sound and acoustics
    UFUNCTION(BlueprintCallable, Category = "Cave Acoustics")
    void ProcessCaveAcoustics(const FVector& SoundSource, float Volume);

    UFUNCTION(BlueprintCallable, Category = "Cave Acoustics")
    float CalculateEchoDelay(const FVector& Source, const FVector& Listener) const;

    UFUNCTION(BlueprintCallable, Category = "Cave Acoustics")
    void ProcessRockfallSoundDetection();

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetCaveSimulationLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForDistance(float ViewerDistance);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    bool ShouldSimulateFullCavePhysics() const;

    // Network synchronization
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateCaveState(const FCavePhysicsGrid& NewGrid);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnEmergencyEvacuation(const FString& Reason);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnGasHazardDetected(const FVector& Location, float Severity);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Cave Events")
    FSimpleMulticastDelegate OnActorEnterCave;

    UPROPERTY(BlueprintAssignable, Category = "Cave Events")
    FSimpleMulticastDelegate OnActorExitCave;

    UPROPERTY(BlueprintAssignable, Category = "Cave Events")
    FSimpleMulticastDelegate OnGasHazardDetected;

    UPROPERTY(BlueprintAssignable, Category = "Cave Events")
    FSimpleMulticastDelegate OnStructuralHazard;

    UPROPERTY(BlueprintAssignable, Category = "Cave Events")
    FSimpleMulticastDelegate OnEmergencyEvacuation;

    UPROPERTY(BlueprintAssignable, Category = "Cave Events")
    FSimpleMulticastDelegate OnLightSourceFailure;

    // Actor tracking in cave
    UPROPERTY(Replicated)
    TArray<AActor*> ActorsInCave;

    UPROPERTY()
    TMap<AActor*, float> ActorCaveEntryTimes;

protected:
    // Internal physics calculations
    void UpdateCavePhysics(float DeltaTime);
    void UpdateAirFlow(float DeltaTime);
    void UpdateGasDistribution(float DeltaTime);
    void UpdateTemperatureDistribution(float DeltaTime);

    // Air circulation helpers
    void SimulateConvectionCurrents(float DeltaTime);
    void ProcessVentilationFromEntrances(float DeltaTime);
    void CalculatePressureDifferentials();
    FAirCirculationData InterpolateAirData(const FVector& WorldLocation) const;

    // Gas physics simulation
    void SimulateGasDispersion(float DeltaTime);
    void ProcessGasReactions(float DeltaTime);
    void UpdateOxygenConsumption(int32 NumOccupants, float DeltaTime);
    void SimulateGasDensityLayers(float DeltaTime);

    // Hazard assessment
    void AssessStructuralStability();
    void MonitorGasLevels();
    void CheckEquipmentStatus();
    bool IsPathToExitViable() const;

    // Performance optimization helpers
    bool ShouldUpdateGridCell(int32 CellIndex, float DeltaTime) const;
    float GetUpdateFrequencyForDistance(float Distance) const;
    void CullDistantCaveEffects(float ViewerDistance);

private:
    // Performance tracking
    float LastFullUpdate = 0.0f;
    float FullUpdateInterval = 1.0f; // 1Hz for full cave simulation
    float LastLightUpdate = 0.0f;
    float LightUpdateInterval = 0.2f; // 5Hz for lighting updates
    float LastGasUpdate = 0.0f;
    float GasUpdateInterval = 2.0f; // 0.5Hz for gas simulation
    
    int32 CurrentLODLevel = 0;
    float ViewerDistance = 0.0f;

    // Physics simulation state
    bool bSimulateFullPhysics = true;
    bool bSimulateGasFlow = true;
    bool bSimulateThermalEffects = true;
    
    // Cave mapping and navigation
    TArray<FVector> MappedLocations;
    TArray<FVector> ExploredPaths;
    TMap<AActor*, float> ActorExplorationProgress;

    // Emergency systems
    bool bEmergencyProtocolsActive = false;
    float EmergencyEvacuationTime = 0.0f;
    TArray<FVector> EmergencyExitRoutes;

    // Cached calculations for performance
    mutable FAirCirculationData CachedAirData;
    mutable FVector CachedAirLocation = FVector::ZeroVector;
    mutable float CachedAirTime = 0.0f;
    static constexpr float AirDataCacheTimeout = 1.0f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float AirCirculationStrength = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float GasDispersionRate = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float HypothermiaRate = 0.005f; // °C per second in cold cave

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MaxSafeGasConcentration = 1.0f; // % for toxic gases

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MinSafeOxygenLevel = 16.0f; // % oxygen

    UPROPERTY(EditAnywhere, Category = "Configuration")
    int32 MaxSimulatedGridCells = 500;

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Configuration")
    bool bEnableAirCirculation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Configuration")
    bool bEnableGasSimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Configuration")
    bool bEnableHazardDetection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Configuration")
    bool bEnableLightingSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Configuration")
    float GlobalCaveIntensity = 1.0f;

    // Integration with other systems
    UPROPERTY()
    class UEnvironmentalHazardManager* EnvironmentalManager;

    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;

    UPROPERTY()
    class UWaterPhysicsComponent* WaterPhysics; // For flooded caves
};