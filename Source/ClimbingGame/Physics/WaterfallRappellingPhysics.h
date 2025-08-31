#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "AdvancedRopeComponent.h"
#include "WaterPhysicsComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "WaterfallRappellingPhysics.generated.h"

UENUM(BlueprintType)
enum class EWaterfallType : uint8
{
    Cascade         UMETA(DisplayName = "Cascade Waterfall"),
    Plunge          UMETA(DisplayName = "Plunge Waterfall"),
    Tiered          UMETA(DisplayName = "Tiered Waterfall"),
    Horsetail       UMETA(DisplayName = "Horsetail Waterfall"),
    Fan             UMETA(DisplayName = "Fan Waterfall"),
    Punchbowl       UMETA(DisplayName = "Punchbowl Waterfall"),
    Block           UMETA(DisplayName = "Block Waterfall")
};

UENUM(BlueprintType)
enum class ERappellingCondition : uint8
{
    Dry             UMETA(DisplayName = "Dry Conditions"),
    LightSpray      UMETA(DisplayName = "Light Water Spray"),
    HeavySpray      UMETA(DisplayName = "Heavy Water Spray"),
    DirectFlow      UMETA(DisplayName = "Direct Water Flow"),
    Torrent         UMETA(DisplayName = "Torrential Flow"),
    Dangerous       UMETA(DisplayName = "Dangerous Conditions")
};

USTRUCT(BlueprintType)
struct FWaterfallProperties
{
    GENERATED_BODY()

    // Physical characteristics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    EWaterfallType WaterfallType = EWaterfallType::Cascade;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float Height = 30.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float Width = 10.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float FlowRate = 100.0f; // cubic meters per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float WaterVelocity = 15.0f; // m/s at impact

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float SprayRadius = 5.0f; // meters from main flow

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float MistIntensity = 0.8f; // 0-1

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float WindDisplacement = 2.0f; // meters wind can push water

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    float PoolDepth = 3.0f; // meters depth of pool at base

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Properties")
    bool bSeasonalFlow = true; // Flow varies with season
};

USTRUCT(BlueprintType)
struct FWaterPressureData
{
    GENERATED_BODY()

    // Water pressure calculations
    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    float ImpactPressure = 0.0f; // Pascals

    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    float SprayPressure = 0.0f; // Pascals

    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    float HydrostaticPressure = 0.0f; // Pascals

    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    FVector PressureDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    float EffectiveArea = 0.0f; // m² of surface under pressure

    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    float TotalForce = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly, Category = "Water Pressure")
    float MomentumTransfer = 0.0f; // kg⋅m/s impact
};

USTRUCT(BlueprintType)
struct FRappellingHazards
{
    GENERATED_BODY()

    // Environmental hazards during waterfall rappelling
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    ERappellingCondition CurrentCondition = ERappellingCondition::Dry;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float VisibilityReduction = 0.0f; // 0-1 (spray reduces visibility)

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float SlipperySurface = 0.0f; // 0-1 friction reduction on wet rock

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float EquipmentSlippage = 0.0f; // 0-1 risk of gear slipping

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float RockInstability = 0.0f; // 0-1 water undermines rock stability

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hazards")
    float HypothermiaRisk = 0.0f; // 0-1 risk from cold water

    UPROPERTY(BlueprintReadOnly, Category = "Hazards")
    float NoiseLevel = 0.0f; // dB (waterfalls are loud, affect communication)

    UPROPERTY(BlueprintReadOnly, Category = "Hazards")
    bool bLedgeObscured = false; // Water spray blocks view of landing areas
};

USTRUCT(BlueprintType)
struct FEquipmentWaterEffects
{
    GENERATED_BODY()

    // Effects of water on climbing equipment
    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    float FrictionReduction = 0.0f; // Reduced grip from wet gear

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    float WeightIncrease = 0.0f; // kg from water absorption

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    float CorrosionRate = 0.0f; // Metal equipment degradation

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    float FreezeRisk = 0.0f; // Risk of equipment freezing

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    float ElectricalFailureRisk = 0.0f; // Electronic equipment failure

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    bool bRopeSwelling = false; // Wet rope becomes harder to handle

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    bool bCarabinerIcing = false; // Carabiners may ice up

    UPROPERTY(BlueprintReadOnly, Category = "Equipment Effects")
    float DryingTime = 3600.0f; // Seconds to dry completely
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UWaterfallRappellingPhysics : public UActorComponent
{
    GENERATED_BODY()

public:
    UWaterfallRappellingPhysics();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Waterfall configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Configuration")
    FWaterfallProperties WaterfallProperties;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Waterfall State")
    FRappellingHazards CurrentHazards;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waterfall State")
    FEquipmentWaterEffects EquipmentEffects;

    // Waterfall visual components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WaterfallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* WaterSprayParticles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* MistParticles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* WaterfallAudio;

    // Water flow zones
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Zones")
    TArray<FVector> DirectFlowZones; // Areas of direct water impact

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Zones")
    TArray<FVector> SprayZones; // Areas affected by water spray

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Zones")
    TArray<FVector> MistZones; // Areas with heavy mist

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Zones")
    FVector PoolLocation = FVector::ZeroVector; // Base pool location

    // Water pressure system
    UFUNCTION(BlueprintCallable, Category = "Water Pressure")
    FWaterPressureData CalculateWaterPressure(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Water Pressure")
    void ApplyWaterPressureToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Water Pressure")
    void ApplyWaterPressureToRope(UAdvancedRopeComponent* RopeComponent, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Water Pressure")
    float CalculateImpactForce(const FVector& Location, float TargetArea) const;

    UFUNCTION(BlueprintCallable, Category = "Water Pressure")
    bool IsLocationInDirectFlow(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Water Pressure")
    bool IsLocationInSprayZone(const FVector& Location) const;

    // Rappelling mechanics
    UFUNCTION(BlueprintCallable, Category = "Rappelling Mechanics")
    void ProcessWaterfallRappel(AActor* Climber, UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rappelling Mechanics")
    ERappellingCondition GetRappellingCondition(const FVector& ClimberLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Rappelling Mechanics")
    void ApplyWaterDisplacement(AActor* Climber, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rappelling Mechanics")
    void ProcessBreathingDifficulty(AActor* Climber, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rappelling Mechanics")
    void CalculateDescentRate(UAdvancedClimbingComponent* ClimbingComponent, float WaterPressure);

    // Equipment effects
    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    void ProcessEquipmentInWaterfall(class AClimbingToolBase* Tool, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    FEquipmentWaterEffects CalculateEquipmentEffects(class AClimbingToolBase* Tool) const;

    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    void ApplyWetEquipmentPhysics(class AClimbingToolBase* Tool, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    void ProcessEquipmentDrying(class AClimbingToolBase* Tool, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Equipment Effects")
    bool IsEquipmentSafeToUse(class AClimbingToolBase* Tool) const;

    // Environmental interaction
    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    void ProcessRockErosion(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    void CheckAnchorStability(class AActor* Anchor);

    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    float CalculateRockFriction(const FVector& SurfaceLocation, bool bWetSurface) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Interaction")
    void ProcessVegetationEffects(const FVector& Location);

    // Safety and emergency systems
    UFUNCTION(BlueprintCallable, Category = "Safety Systems")
    bool IsRappelSafe(const FVector& StartLocation, const FVector& EndLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Safety Systems")
    void AssessWaterfallHazards();

    UFUNCTION(BlueprintCallable, Category = "Safety Systems")
    void TriggerEmergencyProcedures(AActor* Climber, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category = "Safety Systems")
    TArray<FVector> CalculateSafeZones() const;

    UFUNCTION(BlueprintCallable, Category = "Safety Systems")
    bool CanPerformEmergencyEscape(const FVector& ClimberLocation) const;

    // Water volume and flow dynamics
    UFUNCTION(BlueprintCallable, Category = "Flow Dynamics")
    void UpdateFlowDynamics(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Flow Dynamics")
    FVector CalculateWaterVelocityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Flow Dynamics")
    float CalculateWaterDensityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Flow Dynamics")
    void ProcessAeration(float DeltaTime); // Air bubbles in water

    UFUNCTION(BlueprintCallable, Category = "Flow Dynamics")
    void SimulateWaterRebound(const FVector& ImpactLocation);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetWaterfallSimulationLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForDistance(float ViewerDistance);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    bool ShouldSimulateFullWaterfallPhysics() const;

    // Network synchronization
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateWaterfallFlow(float NewFlowRate);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnRappellerEnterWaterfall(AActor* Climber);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnEquipmentFailure(AActor* Climber, const FString& EquipmentType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnEmergencyConditions(const FVector& Location, const FString& HazardType);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Waterfall Events")
    FSimpleMulticastDelegate OnRappellerEnterWaterfall;

    UPROPERTY(BlueprintAssignable, Category = "Waterfall Events")
    FSimpleMulticastDelegate OnRappellerExitWaterfall;

    UPROPERTY(BlueprintAssignable, Category = "Waterfall Events")
    FSimpleMulticastDelegate OnEquipmentCompromised;

    UPROPERTY(BlueprintAssignable, Category = "Waterfall Events")
    FSimpleMulticastDelegate OnEmergencyConditions;

    UPROPERTY(BlueprintAssignable, Category = "Waterfall Events")
    FSimpleMulticastDelegate OnFlowRateChange;

    // Active rappellers tracking
    UPROPERTY(Replicated)
    TArray<AActor*> ActiveRappellers;

    UPROPERTY()
    TMap<AActor*, float> RappellerEntryTimes;

    UPROPERTY()
    TMap<AActor*, FVector> RappellerLastSafePositions;

protected:
    // Internal physics calculations
    void UpdateWaterfallPhysics(float DeltaTime);
    void UpdateWaterPressureField(float DeltaTime);
    void UpdateEquipmentEffects(float DeltaTime);
    void UpdateEnvironmentalHazards(float DeltaTime);

    // Water pressure helpers
    float CalculateDirectImpactPressure(const FVector& Location) const;
    float CalculateSprayPressure(const FVector& Location) const;
    float CalculateWindShear(const FVector& Location) const;
    FVector CalculatePressureGradient(const FVector& Location) const;

    // Flow dynamics simulation
    void SimulateWaterTrajectory(float DeltaTime);
    void ProcessFlowVariations(float DeltaTime);
    void UpdateAerationEffects(float DeltaTime);
    void CalculateSprayPattern();

    // Equipment interaction helpers
    void ProcessRopeInWaterfall(UAdvancedRopeComponent* Rope, float DeltaTime);
    void ProcessDeviceWaterContact(class AClimbingToolBase* Device, float WaterContactIntensity);
    void UpdateEquipmentFriction(class AClimbingToolBase* Tool, float WetnessLevel);
    float CalculateEquipmentReliability(class AClimbingToolBase* Tool) const;

    // Safety assessment helpers
    bool AssessStructuralSafety(const FVector& AnchorLocation) const;
    float CalculateRockSlipperiness(const FVector& SurfaceLocation) const;
    void MonitorRappellerSafety(AActor* Climber);
    bool CheckEmergencyEvacuationNeed(AActor* Climber) const;

    // Performance optimization helpers
    bool ShouldUpdateWaterfall(float DeltaTime) const;
    float GetUpdateFrequencyForDistance(float Distance) const;
    void CullDistantWaterfallEffects(float ViewerDistance);

private:
    // Flow simulation state
    float FlowTimer = 0.0f;
    float FlowVariationPeriod = 30.0f; // Seconds
    float BaseFlowRate = 100.0f;
    
    // Seasonal flow tracking
    float SeasonalMultiplier = 1.0f;
    float DailyFlowVariation = 0.1f;

    // Water pressure field cache
    TArray<FWaterPressureData> PressureField;
    float LastPressureUpdate = 0.0f;
    float PressureUpdateInterval = 0.2f; // 5Hz

    // Performance tracking
    float LastFullUpdate = 0.0f;
    float FullUpdateInterval = 0.1f; // 10Hz for full simulation
    float LastLightUpdate = 0.0f;
    float LightUpdateInterval = 0.5f; // 2Hz for basic updates
    
    int32 CurrentLODLevel = 0;
    float ViewerDistance = 0.0f;

    // Physics simulation control
    bool bSimulateFullPhysics = true;
    bool bSimulateWaterPressure = true;
    bool bSimulateEquipmentEffects = true;

    // Environmental interaction tracking
    TMap<class AClimbingToolBase*, float> EquipmentWetnessLevels;
    TMap<FVector, float> RockStabilityMap; // Location -> stability (0-1)

    // Emergency system state
    bool bEmergencyProtocolsActive = false;
    float EmergencyEvacuationTime = 0.0f;
    TArray<FVector> EmergencyAnchorPoints;

    // Cached calculations for performance
    mutable FWaterPressureData CachedPressure;
    mutable FVector CachedPressureLocation = FVector::ZeroVector;
    mutable float CachedPressureTime = 0.0f;
    static constexpr float PressureCacheTimeout = 0.1f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float WaterPressureMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float EquipmentWetnessRate = 0.1f; // Rate equipment gets wet

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float HypothermiaRate = 0.02f; // °C per second in waterfall

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float SafeFlowRateThreshold = 50.0f; // m³/s above which rappelling is dangerous

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MaxSafeWindSpeed = 15.0f; // m/s wind that makes rappelling unsafe

    UPROPERTY(EditAnywhere, Category = "Configuration")
    int32 MaxSimulatedParticles = 2000;

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Configuration")
    bool bEnableWaterPressurePhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Configuration")
    bool bEnableEquipmentEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Configuration")
    bool bEnableFlowDynamics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Configuration")
    bool bEnableSafetyMonitoring = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waterfall Configuration")
    float GlobalWaterfallIntensity = 1.0f;

    // Integration with other systems
    UPROPERTY()
    UWaterPhysicsComponent* WaterPhysics;

    UPROPERTY()
    class UEnvironmentalHazardManager* EnvironmentalManager;

    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;
};