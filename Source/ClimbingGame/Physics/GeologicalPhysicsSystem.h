#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Engine/StaticMeshActor.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "EnvironmentalHazardManager.h"
#include "GeologicalPhysicsSystem.generated.h"

UENUM(BlueprintType)
enum class ERockType : uint8
{
    Granite         UMETA(DisplayName = "Granite"),
    Limestone       UMETA(DisplayName = "Limestone"),
    Sandstone       UMETA(DisplayName = "Sandstone"),
    Basalt          UMETA(DisplayName = "Basalt"),
    Quartzite       UMETA(DisplayName = "Quartzite"),
    Schist          UMETA(DisplayName = "Schist"),
    Gneiss          UMETA(DisplayName = "Gneiss"),
    Shale           UMETA(DisplayName = "Shale")
};

UENUM(BlueprintType)
enum class EStructuralStability : uint8
{
    Stable          UMETA(DisplayName = "Stable"),
    SlightlyUnstable UMETA(DisplayName = "Slightly Unstable"),
    Unstable        UMETA(DisplayName = "Unstable"),
    VeryUnstable    UMETA(DisplayName = "Very Unstable"),
    Critical        UMETA(DisplayName = "Critical")
};

UENUM(BlueprintType)
enum class EAvalancheType : uint8
{
    None            UMETA(DisplayName = "None"),
    SlabAvalanche   UMETA(DisplayName = "Slab Avalanche"),
    LooseSnow       UMETA(DisplayName = "Loose Snow"),
    WetAvalanche    UMETA(DisplayName = "Wet Avalanche"),
    PowderAvalanche UMETA(DisplayName = "Powder Avalanche"),
    IceAvalanche    UMETA(DisplayName = "Ice Avalanche")
};

UENUM(BlueprintType)
enum class ESeismicWaveType : uint8
{
    Primary         UMETA(DisplayName = "P-Wave"),
    Secondary       UMETA(DisplayName = "S-Wave"),
    Love            UMETA(DisplayName = "Love Wave"),
    Rayleigh        UMETA(DisplayName = "Rayleigh Wave")
};

USTRUCT(BlueprintType)
struct FRockProperties
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    ERockType Type = ERockType::Granite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float Density = 2650.0f; // kg/m³

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float CompressiveStrength = 200.0f; // MPa

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float TensileStrength = 15.0f; // MPa

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float ShearStrength = 25.0f; // MPa

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float YoungsModulus = 70000.0f; // MPa

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float PoissonsRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float FractureResistance = 1.0f; // 0-1, resistance to fracturing

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock Properties")
    float WeatheringRate = 0.01f; // Per year under normal conditions
};

USTRUCT(BlueprintType)
struct FGeologicalStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structure")
    FString StructureID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structure")
    FVector CenterLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structure")
    FVector Bounds = FVector(1000.0f, 1000.0f, 1000.0f); // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structure")
    FRockProperties RockProperties;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Structure")
    EStructuralStability Stability = EStructuralStability::Stable;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Structure")
    float StressAccumulation = 0.0f; // 0-1, accumulated stress

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Structure")
    TArray<FVector> FractureLines; // Active fracture locations

    UPROPERTY(BlueprintReadWrite, Category = "Structure")
    TArray<FString> ConnectedStructures; // Adjacent structures

    UPROPERTY(BlueprintReadWrite, Category = "Structure")
    float LoadBearing = 1.0f; // How much load this structure supports
};

USTRUCT(BlueprintType)
struct FRockfallData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Rockfall")
    FVector Origin = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Rockfall")
    float Volume = 1.0f; // m³

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Rockfall")
    float InitialVelocity = 0.0f; // m/s

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Rockfall")
    FVector Direction = FVector(0.0f, 0.0f, -1.0f);

    UPROPERTY(BlueprintReadWrite, Category = "Rockfall")
    TArray<FVector> BouncePoints;

    UPROPERTY(BlueprintReadWrite, Category = "Rockfall")
    float CoefficientOfRestitution = 0.3f; // Bounciness

    UPROPERTY(BlueprintReadWrite, Category = "Rockfall")
    float RollingFriction = 0.7f;

    UPROPERTY(BlueprintReadWrite, Category = "Rockfall")
    float AirResistance = 0.47f; // Drag coefficient
};

USTRUCT(BlueprintType)
struct FAvalancheData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    EAvalancheType Type = EAvalancheType::None;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    FVector StartLocation = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    FVector FlowDirection = FVector(0.0f, 0.0f, -1.0f);

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    float Volume = 1000.0f; // m³

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    float Density = 300.0f; // kg/m³ (varies by type)

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    float Speed = 0.0f; // Current speed m/s

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Avalanche")
    float MaxSpeed = 80.0f; // Maximum possible speed m/s

    UPROPERTY(BlueprintReadWrite, Category = "Avalanche")
    TArray<FVector> FlowPath; // Path the avalanche follows

    UPROPERTY(BlueprintReadWrite, Category = "Avalanche")
    float FluidityFactor = 0.5f; // How fluid the avalanche behaves

    UPROPERTY(BlueprintReadWrite, Category = "Avalanche")
    float EntrainmentRate = 0.1f; // How much material it picks up
};

USTRUCT(BlueprintType)
struct FSeismicData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Seismic")
    FVector Epicenter = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Seismic")
    float Magnitude = 0.0f; // Richter scale

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Seismic")
    float Depth = 1000.0f; // cm below surface

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Seismic")
    float Duration = 30.0f; // seconds

    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Seismic")
    ESeismicWaveType PrimaryWaveType = ESeismicWaveType::Primary;

    UPROPERTY(BlueprintReadWrite, Category = "Seismic")
    float PWaveVelocity = 6000.0f; // m/s in rock

    UPROPERTY(BlueprintReadWrite, Category = "Seismic")
    float SWaveVelocity = 3500.0f; // m/s in rock

    UPROPERTY(BlueprintReadWrite, Category = "Seismic")
    float AttenuationFactor = 0.1f; // How quickly intensity decreases with distance

    UPROPERTY(BlueprintReadWrite, Category = "Seismic")
    TArray<FVector> FaultLines; // Associated fault lines
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UGeologicalPhysicsSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UGeologicalPhysicsSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Geological structures
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Geological Structures")
    TArray<FGeologicalStructure> GeologicalStructures;

    // Active events
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Active Events")
    TArray<FRockfallData> ActiveRockfalls;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Active Events")
    TArray<FAvalancheData> ActiveAvalanches;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Active Events")
    FSeismicData CurrentSeismicActivity;

    // Environmental hazard manager reference
    UPROPERTY(BlueprintReadWrite, Category = "System Integration")
    UEnvironmentalHazardManager* HazardManager;

    // Structural analysis
    UFUNCTION(BlueprintCallable, Category = "Structural Analysis")
    void RegisterGeologicalStructure(const FGeologicalStructure& Structure);

    UFUNCTION(BlueprintCallable, Category = "Structural Analysis")
    void UnregisterGeologicalStructure(const FString& StructureID);

    UFUNCTION(BlueprintCallable, Category = "Structural Analysis")
    EStructuralStability AnalyzeStructuralStability(const FString& StructureID);

    UFUNCTION(BlueprintCallable, Category = "Structural Analysis")
    void ApplyStressToStructure(const FString& StructureID, float StressAmount);

    UFUNCTION(BlueprintCallable, Category = "Structural Analysis")
    TArray<FString> GetStructuresInRadius(const FVector& Center, float Radius) const;

    // Rockfall simulation
    UFUNCTION(BlueprintCallable, Category = "Rockfall Simulation")
    void InitiateRockfall(const FRockfallData& RockfallData);

    UFUNCTION(BlueprintCallable, Category = "Rockfall Simulation")
    void UpdateRockfallPhysics(int32 RockfallIndex, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rockfall Simulation")
    TArray<FVector> CalculateRockfallTrajectory(const FRockfallData& RockfallData) const;

    UFUNCTION(BlueprintCallable, Category = "Rockfall Simulation")
    bool CheckRockfallCollision(const FVector& Position, const FVector& Velocity, FVector& OutNewPosition, FVector& OutNewVelocity) const;

    // Avalanche simulation
    UFUNCTION(BlueprintCallable, Category = "Avalanche Simulation")
    void InitiateAvalanche(const FAvalancheData& AvalancheData);

    UFUNCTION(BlueprintCallable, Category = "Avalanche Simulation")
    void UpdateAvalanchePhysics(int32 AvalancheIndex, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Avalanche Simulation")
    float CalculateAvalancheHazardLevel(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche Simulation")
    TArray<FVector> PredictAvalanchePath(const FAvalancheData& AvalancheData) const;

    // Seismic activity
    UFUNCTION(BlueprintCallable, Category = "Seismic Activity")
    void InitiateEarthquake(const FSeismicData& SeismicData);

    UFUNCTION(BlueprintCallable, Category = "Seismic Activity")
    void UpdateSeismicActivity(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Seismic Activity")
    float CalculateSeismicIntensity(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Seismic Activity")
    FVector CalculateSeismicForce(const FVector& Location, float Mass) const;

    // Integration with climbing systems
    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    void ApplyGeologicalEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    void ApplyGeologicalEffectsToRope(UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Climbing Integration")
    bool IsLocationSafeForClimbing(const FVector& Location) const;

    // Environmental factors
    UFUNCTION(BlueprintCallable, Category = "Environmental Factors")
    void ProcessWeatheringEffects(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Factors")
    void ApplyThermalStressToStructures(float TemperatureChange);

    UFUNCTION(BlueprintCallable, Category = "Environmental Factors")
    void ProcessFreezeThawCycles(float Temperature);

    // Hazard prediction
    UFUNCTION(BlueprintCallable, Category = "Hazard Prediction")
    float PredictRockfallProbability(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Hazard Prediction")
    float PredictAvalancheProbability(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Hazard Prediction")
    TArray<FVector> IdentifyHazardZones(float HazardThreshold = 0.5f) const;

    // Debris simulation
    UFUNCTION(BlueprintCallable, Category = "Debris Simulation")
    void SpawnDebrisActor(const FVector& Location, float Mass, const FVector& InitialVelocity);

    UFUNCTION(BlueprintCallable, Category = "Debris Simulation")
    void CleanupDebrisActors();

    UFUNCTION(BlueprintCallable, Category = "Debris Simulation")
    void SetMaxDebrisCount(int32 MaxCount);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetGeologicalLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeGeologicalSimulation(float ViewerDistance);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Geological Events")
    FSimpleMulticastDelegate OnRockfallInitiated;

    UPROPERTY(BlueprintAssignable, Category = "Geological Events")
    FSimpleMulticastDelegate OnAvalancheInitiated;

    UPROPERTY(BlueprintAssignable, Category = "Geological Events")
    FSimpleMulticastDelegate OnEarthquakeStart;

    UPROPERTY(BlueprintAssignable, Category = "Geological Events")
    FSimpleMulticastDelegate OnStructuralFailure;

    UPROPERTY(BlueprintAssignable, Category = "Geological Events")
    FSimpleMulticastDelegate OnHazardZoneEntered;

protected:
    // Internal simulation functions
    void UpdateAllGeologicalEvents(float DeltaTime);
    void ProcessStructuralStress(float DeltaTime);
    void UpdateDebrisPhysics(float DeltaTime);
    void CheckHazardZoneProximity();

    // Rockfall physics calculations
    FVector CalculateRockfallDrag(const FVector& Velocity, float CrossSectionalArea, float DragCoeff) const;
    float CalculateBounceVelocity(float IncomingVelocity, float Restitution, float SurfaceHardness) const;
    FVector CalculateRollingDeceleration(const FVector& Velocity, float Friction, float SlopeAngle) const;

    // Avalanche physics calculations
    void CalculateAvalancheFlowDynamics(FAvalancheData& AvalancheData, float DeltaTime);
    float CalculateAvalancheEntrainment(const FAvalancheData& AvalancheData, const FVector& Location) const;
    FVector CalculateAvalancheFlowVelocity(const FAvalancheData& AvalancheData, const FVector& TerrainNormal) const;

    // Seismic wave propagation
    float CalculateSeismicWaveAmplitude(const FVector& Location, ESeismicWaveType WaveType, float Time) const;
    float CalculateSeismicWaveArrivalTime(const FVector& Location, ESeismicWaveType WaveType) const;
    void PropagateSeismicWaves(float DeltaTime);

    // Structural mechanics
    void CalculateStructuralLoads();
    void PropagateStructuralFailure(const FString& FailedStructureID);
    float CalculateStressConcentration(const FGeologicalStructure& Structure, const FVector& LoadPoint) const;

    // Terrain interaction
    FVector GetTerrainNormal(const FVector& Location) const;
    float GetSlopeAngle(const FVector& Location) const;
    float GetTerrainHardness(const FVector& Location) const;

private:
    // Performance optimization state
    int32 CurrentLOD = 0;
    float LastLODUpdate = 0.0f;
    float LODUpdateInterval = 2.0f;

    // Update frequencies
    float StructuralUpdateInterval = 1.0f; // 1Hz
    float DebrisUpdateInterval = 0.1f;     // 10Hz
    float SeismicUpdateInterval = 0.05f;   // 20Hz

    float LastStructuralUpdate = 0.0f;
    float LastDebrisUpdate = 0.0f;
    float LastSeismicUpdate = 0.0f;

    // Debris management
    UPROPERTY()
    TArray<AStaticMeshActor*> SpawnedDebris;

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxDebrisObjects = 50;

    // Seismic wave tracking
    struct FSeismicWave
    {
        FVector Origin;
        float StartTime;
        float Magnitude;
        ESeismicWaveType Type;
        bool bActive;
    };
    TArray<FSeismicWave> ActiveSeismicWaves;

    // Physics simulation cache
    mutable TMap<FVector, float> TerrainHardnessCache;
    mutable TMap<FVector, float> SlopeAngleCache;
    mutable float LastTerrainCacheUpdate = 0.0f;
    static constexpr float TerrainCacheTimeout = 5.0f;

    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxSimulationDistance = 20000.0f; // 200m

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseDetailedPhysics = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseTerrainCache = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxActiveRockfalls = 20;

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxActiveAvalanches = 5;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float GlobalStressMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float WeatheringRate = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableStructuralFailurePropagation = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float HazardZoneProximityDistance = 1000.0f; // cm

public:
    // Static utility functions
    UFUNCTION(BlueprintCallable, Category = "Geological Utilities", CallInEditor = true)
    static FRockProperties GetDefaultRockProperties(ERockType RockType);

    UFUNCTION(BlueprintCallable, Category = "Geological Utilities", CallInEditor = true)
    static float ConvertRichterToMomentMagnitude(float RichterMagnitude);

    UFUNCTION(BlueprintCallable, Category = "Geological Utilities", CallInEditor = true)
    static FString GetEarthquakeDescription(float Magnitude);

    UFUNCTION(BlueprintCallable, Category = "Geological Utilities", CallInEditor = true)
    static float CalculateRockfallEnergy(float Mass, float Height);

    UFUNCTION(BlueprintCallable, Category = "Geological Utilities", CallInEditor = true)
    static float EstimateAvalancheDamage(float Volume, float Speed);
};