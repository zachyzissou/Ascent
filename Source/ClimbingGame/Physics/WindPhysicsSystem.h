#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Chaos/ChaosEngineInterface.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "WindPhysicsSystem.generated.h"

UENUM(BlueprintType)
enum class EWindType : uint8
{
    Constant        UMETA(DisplayName = "Constant"),
    Variable        UMETA(DisplayName = "Variable"),
    Gusty           UMETA(DisplayName = "Gusty"),
    Turbulent       UMETA(DisplayName = "Turbulent"),
    Thermal         UMETA(DisplayName = "Thermal"),
    Downdraft       UMETA(DisplayName = "Downdraft")
};

UENUM(BlueprintType)
enum class ETerrainWindEffect : uint8
{
    None            UMETA(DisplayName = "None"),
    Channeling      UMETA(DisplayName = "Channeling"),
    Venturi         UMETA(DisplayName = "Venturi Effect"),
    Rotor           UMETA(DisplayName = "Rotor Wind"),
    LeeWave         UMETA(DisplayName = "Lee Wave"),
    Convergence     UMETA(DisplayName = "Convergence")
};

USTRUCT(BlueprintType)
struct FWindLayer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Layer")
    float Altitude = 0.0f; // cm above ground

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Layer")
    FVector Direction = FVector(1.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Layer")
    float Speed = 0.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Layer")
    float Turbulence = 0.0f; // 0-1

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Layer")
    float Thickness = 1000.0f; // cm
};

USTRUCT(BlueprintType)
struct FWindField
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    float Radius = 1000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    TArray<FWindLayer> Layers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    ETerrainWindEffect TerrainEffect = ETerrainWindEffect::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Field")
    float TerrainEffectStrength = 1.0f;
};

USTRUCT(BlueprintType)
struct FWindInteractionData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Wind Interaction")
    float CrossSectionalArea = 1.0f; // m²

    UPROPERTY(BlueprintReadOnly, Category = "Wind Interaction")
    float DragCoefficient = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Wind Interaction")
    float Mass = 1.0f; // kg

    UPROPERTY(BlueprintReadOnly, Category = "Wind Interaction")
    FVector CenterOfPressure = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Wind Interaction")
    bool bIsFlexible = false; // For ropes and flexible objects

    UPROPERTY(BlueprintReadOnly, Category = "Wind Interaction")
    float FlexibilityFactor = 1.0f; // How much the object deforms in wind
};

USTRUCT(BlueprintType)
struct FWindForceResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Wind Force")
    FVector Force = FVector::ZeroVector; // Newtons

    UPROPERTY(BlueprintReadOnly, Category = "Wind Force")
    FVector Torque = FVector::ZeroVector; // Newton-meters

    UPROPERTY(BlueprintReadOnly, Category = "Wind Force")
    FVector ApplicationPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Wind Force")
    float Magnitude = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly, Category = "Wind Force")
    float Pressure = 0.0f; // Pa (Pascals)
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UWindPhysicsSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UWindPhysicsSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Wind field configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Configuration")
    TArray<FWindField> WindFields;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Configuration")
    EWindType WindType = EWindType::Variable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Configuration")
    float GlobalWindScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Configuration")
    bool bUseReynoldsNumber = true; // More accurate aerodynamics

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Configuration")
    bool bSimulateWindShear = true;

    // Atmospheric properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Properties")
    float AirDensity = 1.225f; // kg/m³ at sea level

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Properties")
    float Temperature = 288.15f; // Kelvin (15°C)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Properties")
    float Pressure = 101325.0f; // Pa (1 atmosphere)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Properties")
    float Humidity = 0.5f; // 0-1

    // Core wind calculation functions
    UFUNCTION(BlueprintCallable, Category = "Wind Physics")
    FVector GetWindVelocityAtLocation(const FVector& Location, float Altitude = 0.0f) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Physics")
    FWindForceResult CalculateWindForce(const FVector& Location, const FWindInteractionData& ObjectData) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Physics")
    float GetWindPressureAtLocation(const FVector& Location, float Altitude = 0.0f) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Physics")
    FVector GetWindShearAtLocation(const FVector& Location, float AltitudeDelta = 100.0f) const;

    // Specialized wind effects for different object types
    UFUNCTION(BlueprintCallable, Category = "Wind Effects")
    void ApplyWindToRope(UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Wind Effects")
    void ApplyWindToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Wind Effects")
    void ApplyWindToRigidBody(UPrimitiveComponent* Component, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Wind Effects")
    void ApplyWindToTool(AActor* Tool, float DeltaTime);

    // Advanced wind simulation
    UFUNCTION(BlueprintCallable, Category = "Advanced Wind")
    void SimulateTurbulence(const FVector& Location, float Intensity, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Advanced Wind")
    void CreateWindGust(const FVector& Origin, float Intensity, float Duration, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Advanced Wind")
    void SimulateTerrainWindEffects(const FVector& Location, ETerrainWindEffect EffectType, float Strength);

    // Wind field management
    UFUNCTION(BlueprintCallable, Category = "Wind Field Management")
    void AddWindField(const FWindField& NewField);

    UFUNCTION(BlueprintCallable, Category = "Wind Field Management")
    void RemoveWindField(int32 FieldIndex);

    UFUNCTION(BlueprintCallable, Category = "Wind Field Management")
    void UpdateWindField(int32 FieldIndex, const FWindField& UpdatedField);

    UFUNCTION(BlueprintCallable, Category = "Wind Field Management")
    FWindField GetNearestWindField(const FVector& Location) const;

    // Rope-specific wind physics
    UFUNCTION(BlueprintCallable, Category = "Rope Wind Physics")
    void CalculateRopeWindForces(UAdvancedRopeComponent* Rope, TArray<FVector>& SegmentForces) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Wind Physics")
    void ApplyRopeFlutter(UAdvancedRopeComponent* Rope, float WindSpeed, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rope Wind Physics")
    float CalculateRopeResonance(UAdvancedRopeComponent* Rope, float WindFrequency) const;

    // Climber-specific wind physics
    UFUNCTION(BlueprintCallable, Category = "Climber Wind Physics")
    FVector CalculateClimberWindResistance(UAdvancedClimbingComponent* ClimbingComponent) const;

    UFUNCTION(BlueprintCallable, Category = "Climber Wind Physics")
    void ApplyWindBalance(UAdvancedClimbingComponent* ClimbingComponent, const FVector& WindForce, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Climber Wind Physics")
    float CalculateGripStrengthRequirement(const FVector& WindForce, const FVector& ClimberPosition) const;

    // Environmental interaction
    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    void UpdateAtmosphericProperties(float NewTemperature, float NewPressure, float NewHumidity);

    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    float GetReynoldsNumber(float CharacteristicLength, float Velocity) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    float GetDragCoefficient(float ReynoldsNumber, const FString& ObjectShape) const;

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetWindLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeWindSimulation(float ViewerDistance);

protected:
    // Internal wind calculation helpers
    FVector CalculateBaseWind(const FVector& Location, float Altitude) const;
    FVector ApplyTurbulence(const FVector& BaseWind, const FVector& Location, float Turbulence) const;
    FVector ApplyTerrainEffects(const FVector& Wind, const FVector& Location, const FWindField& Field) const;
    FVector ApplyWindShear(const FVector& Wind, float Altitude, float ShearFactor) const;

    // Aerodynamic calculations
    float CalculateAirDensityAtAltitude(float Altitude) const;
    float CalculateDynamicPressure(float Velocity, float Density) const;
    FVector CalculateDragForce(const FVector& Velocity, float Area, float DragCoeff, float Density) const;
    FVector CalculateLiftForce(const FVector& Velocity, const FVector& Normal, float Area, float LiftCoeff, float Density) const;

    // Rope wind physics helpers
    void CalculateRopeSegmentWind(const FVector& SegmentPos, const FVector& SegmentVel, float SegmentLength, 
                                  float RopeDiameter, FVector& OutForce) const;
    float CalculateRopeStrouhalNumber(float RopeDiameter, float WindSpeed) const;
    void ApplyVortexSheddingForces(UAdvancedRopeComponent* Rope, float DeltaTime);

    // Climber wind physics helpers
    void CalculateClimberDragAndLift(const FVector& WindVelocity, const FVector& ClimberOrientation,
                                    FVector& OutDrag, FVector& OutLift) const;
    float CalculateClimberCrossSectionalArea(const FVector& WindDirection, const FVector& ClimberOrientation) const;
    
    // Terrain interaction helpers
    FVector CalculateVenturiEffect(const FVector& Location, const FVector& WindDirection) const;
    FVector CalculateRotorEffect(const FVector& Location, const FVector& TerrainNormal) const;
    FVector CalculateConvergenceEffect(const FVector& Location, const TArray<FVector>& TerrainFeatures) const;

private:
    // Performance optimization state
    int32 CurrentLOD = 0;
    float LastLODUpdate = 0.0f;
    float LODUpdateInterval = 1.0f;

    // Wind simulation cache for performance
    mutable TMap<uint32, FVector> WindVelocityCache;
    mutable TMap<uint32, float> WindPressureCache;
    mutable float LastCacheUpdate = 0.0f;
    static constexpr float CacheTimeout = 0.1f; // Cache for 100ms

    // Turbulence generation
    TArray<FVector> TurbulenceSeeds;
    float TurbulenceTimer = 0.0f;
    void GenerateTurbulenceSeeds();
    FVector SampleTurbulence(const FVector& Location, float Scale) const;

    // Gust simulation
    struct FActiveGust
    {
        FVector Origin;
        float Intensity;
        float Duration;
        float ElapsedTime;
        float Radius;
    };
    TArray<FActiveGust> ActiveGusts;
    void UpdateActiveGusts(float DeltaTime);
    FVector CalculateGustContribution(const FVector& Location) const;

    // Physics constants
    static constexpr float KinematicViscosity = 1.48e-5f; // m²/s for air at 15°C
    static constexpr float SpecificGasConstant = 287.0f; // J/(kg·K) for dry air
    static constexpr float StandardPressure = 101325.0f; // Pa
    static constexpr float StandardTemperature = 288.15f; // K

    // Drag coefficients for common shapes
    TMap<FString, float> DragCoefficientTable;
    void InitializeDragCoefficientTable();

    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxWindSimulationDistance = 10000.0f; // cm

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 MaxRopeSegmentsForWind = 32;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseWindCache = true;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bUseMultithreadedWind = true;

public:
    // Static utility functions for external use
    UFUNCTION(BlueprintCallable, Category = "Wind Utilities", CallInEditor = true)
    static float ConvertBeaufortToMS(int32 BeaufortScale);

    UFUNCTION(BlueprintCallable, Category = "Wind Utilities", CallInEditor = true)
    static int32 ConvertMSToBeaufort(float MetersPerSecond);

    UFUNCTION(BlueprintCallable, Category = "Wind Utilities", CallInEditor = true)
    static FString GetWindDescription(float WindSpeed);

    UFUNCTION(BlueprintCallable, Category = "Wind Utilities", CallInEditor = true)
    static float CalculateWindChill(float AirTemp, float WindSpeed);

    UFUNCTION(BlueprintCallable, Category = "Wind Utilities", CallInEditor = true)
    static float CalculateApparentWindSpeed(float TrueWindSpeed, float ObjectSpeed, float AngleBetween);
};