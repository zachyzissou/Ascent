#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "Components/LightComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "WaterPhysicsComponent.h"
#include "CaveEnvironmentPhysics.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "CaveDivingPhysics.generated.h"

UENUM(BlueprintType)
enum class ECaveDiveType : uint8
{
    ShallowCave     UMETA(DisplayName = "Shallow Cave Dive"),
    DeepCave        UMETA(DisplayName = "Deep Cave Dive"),
    Sump            UMETA(DisplayName = "Sump Dive"),
    TechnicalSump   UMETA(DisplayName = "Technical Sump"),
    Restriction     UMETA(DisplayName = "Restriction Dive"),
    SiphonTunnel    UMETA(DisplayName = "Siphon Tunnel"),
    FloodedCavern   UMETA(DisplayName = "Flooded Cavern")
};

UENUM(BlueprintType)
enum class EDivingEmergency : uint8
{
    None            UMETA(DisplayName = "No Emergency"),
    LowAir          UMETA(DisplayName = "Low Air Supply"),
    EquipmentFailure UMETA(DisplayName = "Equipment Failure"),
    LostGuideline   UMETA(DisplayName = "Lost Guideline"),
    Entanglement    UMETA(DisplayName = "Entanglement"),
    SiltOut         UMETA(DisplayName = "Silt Out"),
    NitrogenNarcosis UMETA(DisplayName = "Nitrogen Narcosis"),
    Panic           UMETA(DisplayName = "Panic"),
    DecompressionSickness UMETA(DisplayName = "Decompression Sickness")
};

UENUM(BlueprintType)
enum class EDecompressionStatus : uint8
{
    Safe            UMETA(DisplayName = "Safe Ascent"),
    CautionRequired UMETA(DisplayName = "Caution Required"),
    StopsRequired   UMETA(DisplayName = "Decompression Stops Required"),
    SlowAscent      UMETA(DisplayName = "Slow Ascent Only"),
    Emergency       UMETA(DisplayName = "Emergency Ascent"),
    Critical        UMETA(DisplayName = "Critical - DCS Risk")
};

USTRUCT(BlueprintType)
struct FCaveDivingProperties
{
    GENERATED_BODY()

    // Cave dive characteristics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    ECaveDiveType DiveType = ECaveDiveType::ShallowCave;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float MaxDepth = 30.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float CaveLength = 200.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float MinimumWidth = 1.5f; // meters (narrowest point)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float AverageWidth = 3.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float WaterTemperature = 10.0f; // Celsius (cave water is cold)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float CurrentStrength = 0.2f; // m/s typical cave current

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    float SiltLevel = 0.3f; // 0-1 amount of disturb-able sediment

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    bool bHasAirBells = false; // Pockets of trapped air

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Properties")
    bool bHasRestrictions = true; // Narrow passages
};

USTRUCT(BlueprintType)
struct FDivingPhysiologyData
{
    GENERATED_BODY()

    // Physiological effects of cave diving
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Physiology")
    float OxygenConsumptionRate = 0.5f; // L/min at rest

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Physiology")
    float NitrogenAbsorption = 0.0f; // Tissue nitrogen loading

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Physiology")
    float CarbonDioxideLevel = 4.0f; // % in bloodstream

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Physiology")
    float CoreTemperature = 37.0f; // Celsius

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Physiology")
    float StressLevel = 0.1f; // 0-1 psychological stress

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Physiology")
    bool bNitrogenNarcosis = false; // Impaired judgment at depth

    UPROPERTY(BlueprintReadOnly, Category = "Physiology")
    float FinMotorSkill = 1.0f; // 0-1 dexterity level

    UPROPERTY(BlueprintReadOnly, Category = "Physiology")
    float CognitiveFunction = 1.0f; // 0-1 decision making ability
};

USTRUCT(BlueprintType)
struct FDivingEquipmentState
{
    GENERATED_BODY()

    // Diving equipment status
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    float AirSupply = 200.0f; // bar pressure

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    float AirConsumptionRate = 15.0f; // L/min

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    float BuoyancyControl = 0.0f; // +/- kg effective weight

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    bool bRegulatorFunctioning = true;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    bool bBCDFunctioning = true; // Buoyancy Control Device

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    bool bLightsFunctioning = true;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    float WetSuitInsulation = 1.0f; // 0-1 thermal protection

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    bool bMaskClearing = true; // Can clear water from mask

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    float EquipmentDrag = 0.3f; // Drag coefficient for gear
};

USTRUCT(BlueprintType)
struct FDecompressionProfile
{
    GENERATED_BODY()

    // Decompression calculation data
    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    float MaxDepthReached = 0.0f; // meters

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    float TimeAtDepth = 0.0f; // minutes

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    float BottomTime = 0.0f; // minutes at maximum depth

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    TArray<float> DecompressionStops; // Depths requiring stops (meters)

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    TArray<float> StopDurations; // Minutes at each stop

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    float SafeAscentRate = 9.0f; // meters per minute

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    float EmergencyAscentRate = 18.0f; // meters per minute (dangerous)

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    EDecompressionStatus Status = EDecompressionStatus::Safe;

    UPROPERTY(BlueprintReadOnly, Category = "Decompression")
    float DCSRisk = 0.0f; // 0-1 decompression sickness probability
};

USTRUCT(BlueprintType)
struct FCaveNavigationData
{
    GENERATED_BODY()

    // Navigation in underwater caves
    UPROPERTY(BlueprintReadWrite, Category = "Navigation")
    TArray<FVector> GuidelinePoints; // Guideline anchors

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    float DistanceFromGuideline = 0.0f; // meters

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    float DistanceToExit = 0.0f; // meters along guideline

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    FVector LastKnownSafeLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    bool bGuidelineVisible = true;

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    float SiltCloudRadius = 0.0f; // meters of zero visibility

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    bool bInRestriction = false; // In narrow passage

    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    float RestrictionDiameter = 2.0f; // meters
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UCaveDivingPhysics : public UActorComponent
{
    GENERATED_BODY()

public:
    UCaveDivingPhysics();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Cave diving configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Configuration")
    FCaveDivingProperties CaveDivingProperties;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Diving State")
    FDivingPhysiologyData PhysiologyData;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Diving State")
    FDivingEquipmentState EquipmentState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diving State")
    FDecompressionProfile DecompressionProfile;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Diving State")
    FCaveNavigationData NavigationData;

    // Cave dive volume components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* DiveZoneVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* SiltParticles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* UnderwaterAmbientAudio;

    // Guideline system
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidelines")
    TArray<FVector> GuidelineAnchorPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guidelines")
    UAdvancedRopeComponent* GuidelineRope;

    // Physiological systems
    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    void UpdateDivingPhysiology(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    void ProcessOxygenConsumption(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    void ProcessNitrogenAbsorption(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    void ProcessCarbonDioxideBuildup(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    void ProcessHypothermia(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    void ProcessStressEffects(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Physiology")
    bool CheckForNitrogenNarcosis(float Depth) const;

    // Decompression system
    UFUNCTION(BlueprintCallable, Category = "Decompression")
    void UpdateDecompressionProfile(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Decompression")
    FDecompressionProfile CalculateDecompressionNeeds(float MaxDepth, float BottomTime) const;

    UFUNCTION(BlueprintCallable, Category = "Decompression")
    EDecompressionStatus GetDecompressionStatus(AActor* Diver) const;

    UFUNCTION(BlueprintCallable, Category = "Decompression")
    bool CanPerformDirectAscent(AActor* Diver) const;

    UFUNCTION(BlueprintCallable, Category = "Decompression")
    void ProcessEmergencyAscent(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Decompression")
    float CalculateDCSRisk(AActor* Diver) const;

    UFUNCTION(BlueprintCallable, Category = "Decompression")
    void ProcessDecompressionSickness(AActor* Diver, float Severity);

    // Cave navigation and safety
    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    void UpdateCaveNavigation(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    void DeployGuideline(AActor* Diver, const FVector& AnchorPoint);

    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    void FollowGuideline(AActor* Diver, bool bTowardsExit);

    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    float GetDistanceToExit(const FVector& CurrentLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    void ProcessSiltOut(AActor* Diver, float IntensityLevel);

    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    void ClearSiltCloud(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Navigation")
    bool IsPathToExitClear(const FVector& DiverLocation) const;

    // Equipment management
    UFUNCTION(BlueprintCallable, Category = "Diving Equipment")
    void UpdateDivingEquipment(AActor* Diver, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Diving Equipment")
    void ProcessEquipmentFailure(AActor* Diver, const FString& EquipmentType);

    UFUNCTION(BlueprintCallable, Category = "Diving Equipment")
    void CalculateAirConsumption(AActor* Diver, float WorkRate, float Depth);

    UFUNCTION(BlueprintCallable, Category = "Diving Equipment")
    void ProcessBuoyancyControl(AActor* Diver, float TargetDepth);

    UFUNCTION(BlueprintCallable, Category = "Diving Equipment")
    bool CheckEquipmentReliability(class AClimbingToolBase* DivingGear) const;

    UFUNCTION(BlueprintCallable, Category = "Diving Equipment")
    void ProcessEquipmentEntanglement(AActor* Diver, UAdvancedRopeComponent* EntangledRope);

    // Emergency procedures
    UFUNCTION(BlueprintCallable, Category = "Emergency Procedures")
    void TriggerDivingEmergency(AActor* Diver, EDivingEmergency EmergencyType);

    UFUNCTION(BlueprintCallable, Category = "Emergency Procedures")
    void ProcessAirSharingProcedure(AActor* PrimaryDiver, AActor* BuddyDiver);

    UFUNCTION(BlueprintCallable, Category = "Emergency Procedures")
    void InitiateEmergencyAscent(AActor* Diver);

    UFUNCTION(BlueprintCallable, Category = "Emergency Procedures")
    void ProcessPanicResponse(AActor* Diver, float PanicLevel);

    UFUNCTION(BlueprintCallable, Category = "Emergency Procedures")
    void CalculateRescueViability(AActor* VictimDiver, AActor* RescuerDiver);

    // Underwater rope and guideline physics
    UFUNCTION(BlueprintCallable, Category = "Underwater Rope Physics")
    void ProcessGuidelinePhysics(UAdvancedRopeComponent* Guideline, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Underwater Rope Physics")
    void ProcessRopeEntanglement(UAdvancedRopeComponent* Rope, AActor* Diver);

    UFUNCTION(BlueprintCallable, Category = "Underwater Rope Physics")
    void CalculateRopeVisibility(UAdvancedRopeComponent* Rope, float SiltLevel);

    UFUNCTION(BlueprintCallable, Category = "Underwater Rope Physics")
    void ProcessGuidelineFollowing(AActor* Diver, UAdvancedRopeComponent* Guideline);

    // Environmental interaction
    UFUNCTION(BlueprintCallable, Category = "Cave Environment")
    void ProcessRestrictionPassage(AActor* Diver, float RestrictionWidth);

    UFUNCTION(BlueprintCallable, Category = "Cave Environment")
    void ProcessAirBellInteraction(AActor* Diver, const FVector& AirBellLocation);

    UFUNCTION(BlueprintCallable, Category = "Cave Environment")
    void ProcessCurrentResistance(AActor* Diver, const FVector& CurrentDirection, float CurrentSpeed);

    UFUNCTION(BlueprintCallable, Category = "Cave Environment")
    void ProcessCaveGeology(AActor* Diver, const FVector& WallContact);

    // Performance optimization
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void SetCaveDivingLOD(int32 LODLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void OptimizeForDistance(float ViewerDistance);

    UFUNCTION(BlueprintCallable, Category = "Performance")
    bool ShouldSimulateFullDivingPhysics() const;

    // Network synchronization
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateDivingState(AActor* Diver, const FDivingPhysiologyData& NewPhysiology);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnDivingEmergency(AActor* Diver, EDivingEmergency EmergencyType);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnDecompressionAlert(AActor* Diver, EDecompressionStatus Status);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnEquipmentFailure(AActor* Diver, const FString& EquipmentType);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnDiverEnterCave;

    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnDiverExitCave;

    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnDivingEmergency;

    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnDecompressionAlert;

    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnLowAirWarning;

    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnEquipmentFailure;

    UPROPERTY(BlueprintAssignable, Category = "Cave Diving Events")
    FSimpleMulticastDelegate OnGuidelineLost;

    // Active cave divers tracking
    UPROPERTY(Replicated)
    TArray<AActor*> ActiveCaveDivers;

    UPROPERTY()
    TMap<AActor*, FDivingPhysiologyData> DiverPhysiologyStates;

    UPROPERTY()
    TMap<AActor*, FDivingEquipmentState> DiverEquipmentStates;

    UPROPERTY()
    TMap<AActor*, float> DiverCaveEntryTimes;

protected:
    // Internal physics calculations
    void UpdateCaveDivingPhysics(float DeltaTime);
    void UpdatePhysiologicalEffects(float DeltaTime);
    void UpdateEquipmentStates(float DeltaTime);
    void UpdateDecompressionProfiles(float DeltaTime);

    // Physiology simulation helpers
    void CalculateNitrogenLoading(AActor* Diver, float Depth, float DeltaTime);
    void ProcessTemperatureLoss(AActor* Diver, float WaterTemperature, float DeltaTime);
    void UpdateCognitiveImpairment(AActor* Diver, float Depth, float StressLevel);
    float CalculateWorkOfBreathing(AActor* Diver, float Depth, float Activity) const;

    // Equipment simulation helpers
    void SimulateRegulatorPerformance(AActor* Diver, float Depth, float DeltaTime);
    void ProcessBuoyancyCompensation(AActor* Diver, float TargetBuoyancy);
    void UpdateLightPerformance(AActor* Diver, float DeltaTime);
    bool CheckEquipmentColdWaterFailure(class AClimbingToolBase* Equipment, float Temperature) const;

    // Navigation simulation helpers
    void ProcessGuidelineDeployment(AActor* Diver, const FVector& NewAnchor);
    void UpdateSiltDisturbance(AActor* Diver, float MovementIntensity);
    void CalculateVisibilityReduction(const FVector& Location, float SiltLevel);
    bool IsGuidelineReachable(const FVector& DiverLocation) const;

    // Emergency response helpers
    void AssessDivingEmergencyRisk(AActor* Diver);
    void CalculateRescueTime(AActor* VictimDiver, AActor* RescuerDiver);
    void ProcessAirSharingPhysics(AActor* DonorDiver, AActor* ReceiverDiver);
    bool CanPerformBuddyRescue(AActor* Victim, AActor* Rescuer) const;

    // Performance optimization helpers
    bool ShouldUpdateDiver(AActor* Diver, float DeltaTime) const;
    float GetUpdateFrequencyForDistance(float Distance) const;
    void CullDistantDivingEffects(float ViewerDistance);

private:
    // Performance tracking
    float LastFullUpdate = 0.0f;
    float FullUpdateInterval = 0.5f; // 2Hz for full diving simulation
    float LastPhysiologyUpdate = 0.0f;
    float PhysiologyUpdateInterval = 1.0f; // 1Hz for physiology
    float LastEquipmentUpdate = 0.0f;
    float EquipmentUpdateInterval = 0.2f; // 5Hz for equipment checks
    
    int32 CurrentLODLevel = 0;
    float ViewerDistance = 0.0f;

    // Physics simulation control
    bool bSimulateFullPhysics = true;
    bool bSimulatePhysiology = true;
    bool bSimulateDecompression = true;
    bool bSimulateEquipmentFailure = true;

    // Emergency state tracking
    bool bEmergencyProtocolsActive = false;
    float EmergencyStartTime = 0.0f;
    TMap<AActor*, EDivingEmergency> ActiveEmergencies;
    TMap<AActor*, float> EmergencyTimers;

    // Silt and visibility tracking
    TMap<FVector, float> SiltCloudLocations; // Location -> Intensity
    TMap<FVector, float> SiltCloudTimers; // Location -> Time remaining
    float SiltSettlingRate = 0.1f; // m/s settling velocity

    // Decompression table data (simplified)
    TMap<float, TArray<float>> DecompressionTables; // Depth -> Stop depths

    // Cached calculations for performance
    mutable FDivingPhysiologyData CachedPhysiology;
    mutable AActor* CachedDiver = nullptr;
    mutable float CachedPhysiologyTime = 0.0f;
    static constexpr float PhysiologyCacheTimeout = 1.0f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float PhysiologyUpdateRate = 1.0f; // Hz

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float DecompressionSafety = 1.2f; // Safety factor for decompression

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MaxSafeDepth = 40.0f; // meters for recreational cave diving

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float CriticalAirReserve = 50.0f; // bar reserve for emergencies

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MaxDiveTime = 45.0f; // minutes maximum dive duration

    UPROPERTY(EditAnywhere, Category = "Configuration")
    int32 MaxSimulatedDivers = 8;

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Configuration")
    bool bEnablePhysiologySimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Configuration")
    bool bEnableDecompressionTracking = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Configuration")
    bool bEnableEquipmentFailures = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Configuration")
    bool bEnableEmergencyProcedures = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave Diving Configuration")
    float GlobalDivingIntensity = 1.0f;

    // Integration with other systems
    UPROPERTY()
    UWaterPhysicsComponent* WaterPhysics;

    UPROPERTY()
    UCaveEnvironmentPhysics* CavePhysics;

    UPROPERTY()
    class UEnvironmentalHazardManager* EnvironmentalManager;

    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;
};