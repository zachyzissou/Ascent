#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TriggerVolume.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "WaterPhysicsComponent.generated.h"

UENUM(BlueprintType)
enum class EWaterType : uint8
{
    Freshwater      UMETA(DisplayName = "Freshwater"),
    Saltwater       UMETA(DisplayName = "Saltwater"),
    Whitewater      UMETA(DisplayName = "Whitewater Rapids"),
    StillPool       UMETA(DisplayName = "Still Pool"),
    Underground     UMETA(DisplayName = "Underground Water"),
    Thermal         UMETA(DisplayName = "Thermal Springs")
};

UENUM(BlueprintType)
enum class EWaterFlowType : uint8
{
    Still           UMETA(DisplayName = "Still Water"),
    LaminarFlow     UMETA(DisplayName = "Laminar Flow"),
    TurbulentFlow   UMETA(DisplayName = "Turbulent Flow"),
    Rapids          UMETA(DisplayName = "Rapids"),
    Waterfall       UMETA(DisplayName = "Waterfall"),
    Whirlpool       UMETA(DisplayName = "Whirlpool")
};

UENUM(BlueprintType)
enum class ESwimmingState : uint8
{
    Surface         UMETA(DisplayName = "Surface Swimming"),
    Submerged       UMETA(DisplayName = "Submerged"),
    Diving          UMETA(DisplayName = "Diving"),
    Emergency       UMETA(DisplayName = "Emergency Ascent"),
    Drowning        UMETA(DisplayName = "Drowning")
};

USTRUCT(BlueprintType)
struct FWaterProperties
{
    GENERATED_BODY()

    // Physical properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    EWaterType WaterType = EWaterType::Freshwater;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    float Density = 1000.0f; // kg/m³

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    float Viscosity = 0.001f; // Pa·s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    float Temperature = 15.0f; // Celsius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    float Salinity = 0.0f; // % for saltwater

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    float TurbidityLevel = 0.1f; // 0-1, affects visibility

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Properties")
    float pH = 7.0f; // Acidity/alkalinity

    // Flow properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Properties")
    EWaterFlowType FlowType = EWaterFlowType::Still;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Properties")
    FVector FlowDirection = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Properties")
    float FlowVelocity = 0.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Properties")
    float Turbulence = 0.0f; // 0-1

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Properties")
    float WaveAmplitude = 0.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Properties")
    float WaveFrequency = 0.0f; // Hz
};

USTRUCT(BlueprintType)
struct FBuoyancyData
{
    GENERATED_BODY()

    // Buoyancy calculations
    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    float BuoyantForce = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    float SubmergedVolume = 0.0f; // m³

    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    float SubmersionPercentage = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    FVector CenterOfBuoyancy = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    FVector BuoyancyForceVector = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    float DragCoefficient = 0.47f; // Sphere approximation

    UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
    float HydrodynamicDrag = 0.0f; // Newtons
};

USTRUCT(BlueprintType)
struct FUnderwaterState
{
    GENERATED_BODY()

    // Underwater conditions
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    ESwimmingState SwimmingState = ESwimmingState::Surface;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    float CurrentDepth = 0.0f; // meters below surface

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    float MaxDepthReached = 0.0f; // meters

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    float TimeSubmerged = 0.0f; // seconds

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    float OxygenLevel = 100.0f; // % remaining

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    float NitrogenLoad = 0.0f; // Decompression tracking

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Underwater")
    float AscentRate = 0.0f; // m/s (for decompression)

    UPROPERTY(BlueprintReadOnly, Category = "Underwater")
    float WaterPressure = 101325.0f; // Pascals (1 atm at surface)

    UPROPERTY(BlueprintReadOnly, Category = "Underwater")
    float VisibilityRange = 10.0f; // meters
};

USTRUCT(BlueprintType)
struct FWaterCurrentData
{
    GENERATED_BODY()

    // 3D current flow field
    UPROPERTY(BlueprintReadWrite, Category = "Current")
    TArray<FVector> FlowVectors; // Grid of flow directions

    UPROPERTY(BlueprintReadWrite, Category = "Current")
    TArray<float> FlowSpeeds; // Grid of flow speeds

    UPROPERTY(BlueprintReadWrite, Category = "Current")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "Current")
    FVector GridSize = FVector(1000, 1000, 200); // cm

    UPROPERTY(BlueprintReadWrite, Category = "Current")
    int32 GridResolutionX = 20;

    UPROPERTY(BlueprintReadWrite, Category = "Current")
    int32 GridResolutionY = 20;

    UPROPERTY(BlueprintReadWrite, Category = "Current")
    int32 GridResolutionZ = 10;
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UWaterPhysicsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWaterPhysicsComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Water body configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Configuration")
    FWaterProperties WaterProperties;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Water State")
    FWaterCurrentData CurrentData;

    // Water volume definition
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WaterMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Volume")
    ATriggerVolume* WaterVolume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Volume")
    FVector WaterSurfaceLevel = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Volume")
    bool bInfiniteDepth = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Volume")
    float MaxDepth = 50.0f; // meters

    // Buoyancy system
    UFUNCTION(BlueprintCallable, Category = "Water Physics")
    FBuoyancyData CalculateBuoyancy(UPrimitiveComponent* Component, const FVector& ComponentLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Water Physics")
    void ApplyBuoyancyToComponent(UPrimitiveComponent* Component, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Water Physics")
    void ApplyBuoyancyToRope(UAdvancedRopeComponent* RopeComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Water Physics")
    void ApplyBuoyancyToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    // Current flow system
    UFUNCTION(BlueprintCallable, Category = "Water Currents")
    FVector GetCurrentFlowAtLocation(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Water Currents")
    float GetCurrentSpeedAtLocation(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Water Currents")
    void ApplyCurrentForce(UPrimitiveComponent* Component, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Water Currents")
    void InitializeCurrentGrid();

    UFUNCTION(BlueprintCallable, Category = "Water Currents")
    void UpdateCurrentFlow(float DeltaTime);

    // Underwater mechanics
    UFUNCTION(BlueprintCallable, Category = "Underwater Mechanics")
    bool IsActorSubmerged(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "Underwater Mechanics")
    float GetSubmersionDepth(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Underwater Mechanics")
    float CalculateWaterPressure(float Depth) const;

    UFUNCTION(BlueprintCallable, Category = "Underwater Mechanics")
    float CalculateVisibilityRange(float Depth, float Turbidity) const;

    UFUNCTION(BlueprintCallable, Category = "Underwater Mechanics")
    void ProcessUnderwaterMovement(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    // Swimming and diving system
    UFUNCTION(BlueprintCallable, Category = "Swimming")
    void EnterWater(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Swimming")
    void ExitWater(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Swimming")
    void UpdateSwimmingState(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Swimming")
    void ProcessOxygenConsumption(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Swimming")
    void ProcessDecompression(AActor* Actor, float DeltaTime);

    // Equipment interaction
    UFUNCTION(BlueprintCallable, Category = "Equipment Interaction")
    void ProcessEquipmentInWater(class AClimbingToolBase* Tool, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Equipment Interaction")
    float GetEquipmentBuoyancy(class AClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintCallable, Category = "Equipment Interaction")
    void ApplyWaterDamageToEquipment(class AClimbingToolBase* Tool, float DeltaTime);

    // Environmental effects
    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    void ApplyTemperatureEffects(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    void ProcessHypothermia(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    float CalculateHeatLoss(float WaterTemperature, float ExposureTime) const;

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetWaterSimulationLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForDistance(float ViewerDistance);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    bool ShouldSimulateFullWaterPhysics() const;

    // Network synchronization
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateWaterState(const FWaterCurrentData& NewCurrentData);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnActorEnterWater(AActor* Actor);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnActorExitWater(AActor* Actor);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Water Events")
    FSimpleMulticastDelegate OnActorEnterWater;

    UPROPERTY(BlueprintAssignable, Category = "Water Events")
    FSimpleMulticastDelegate OnActorExitWater;

    UPROPERTY(BlueprintAssignable, Category = "Water Events")
    FSimpleMulticastDelegate OnUnderwaterEmergency;

    UPROPERTY(BlueprintAssignable, Category = "Water Events")
    FSimpleMulticastDelegate OnDecompressionWarning;

    // Rope interaction tracking
    UPROPERTY()
    TArray<UAdvancedRopeComponent*> SubmergedRopes;

    // Actor state tracking
    UPROPERTY(Replicated)
    TMap<AActor*, FUnderwaterState> UnderwaterStates;

protected:
    // Internal physics calculations
    void UpdateWaterPhysics(float DeltaTime);
    void UpdateBuoyancyForces(float DeltaTime);
    void UpdateCurrentForces(float DeltaTime);
    void UpdateUnderwaterStates(float DeltaTime);

    // Buoyancy helpers
    float CalculateSubmergedVolume(UPrimitiveComponent* Component, const FVector& ComponentLocation) const;
    FVector CalculateCenterOfBuoyancy(UPrimitiveComponent* Component, const FVector& ComponentLocation) const;
    float CalculateDragForce(const FVector& Velocity, float CrossSectionalArea) const;

    // Current flow helpers
    void GenerateTurbulentFlow(float DeltaTime);
    void ProcessWaterfallFlow(float DeltaTime);
    void ProcessWhirlpoolFlow(float DeltaTime);
    FVector InterpolateCurrentFlow(const FVector& WorldLocation) const;

    // Performance optimization helpers
    bool ShouldUpdateComponent(UPrimitiveComponent* Component, float DeltaTime) const;
    float GetUpdateFrequencyForDistance(float Distance) const;
    void CullDistantWaterEffects(float ViewerDistance);

private:
    // Performance tracking
    float LastFullUpdate = 0.0f;
    float FullUpdateInterval = 0.1f; // 10Hz for full simulation
    float LastLightUpdate = 0.0f;
    float LightUpdateInterval = 0.5f; // 2Hz for basic updates
    
    int32 CurrentLODLevel = 0;
    float ViewerDistance = 0.0f;

    // Physics optimization
    bool bSimulateFullPhysics = true;
    int32 MaxSimulatedObjects = 50;
    float MaxSimulationDistance = 10000.0f; // 100m

    // Current flow generation
    float TurbulenceTimer = 0.0f;
    float WaveTimer = 0.0f;
    TArray<FVector> TurbulenceSeeds;

    // Temperature tracking for hypothermia
    TMap<AActor*, float> ActorTemperatures;
    TMap<AActor*, float> ExposureTimes;

    // Cached calculations for performance
    mutable FBuoyancyData CachedBuoyancy;
    mutable UPrimitiveComponent* CachedComponent = nullptr;
    mutable float CachedBuoyancyTime = 0.0f;
    static constexpr float BuoyancyCacheTimeout = 0.1f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float BuoyancyStrength = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float CurrentStrength = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float HypothermiaRate = 0.01f; // °C per second in cold water

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float OxygenConsumptionRate = 0.33f; // % per second while submerged

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float SafeAscentRate = 10.0f; // meters per minute

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float DecompressionLimit = 30.0f; // meters depth before decompression needed

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Configuration")
    bool bEnableBuoyancy = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Configuration")
    bool bEnableCurrents = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Configuration")
    bool bEnableUnderwaterMechanics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Configuration")
    bool bEnableTemperatureEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Configuration")
    float GlobalWaterIntensity = 1.0f;

    // Integration with environmental systems
    UPROPERTY()
    class UEnvironmentalHazardManager* EnvironmentalManager;

    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;
};