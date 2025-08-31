#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CableComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "AdvancedRopeComponent.generated.h"

// Forward declarations
class UWaterPhysicsComponent;
class UCaveEnvironmentPhysics;

UENUM(BlueprintType)
enum class ERopeType : uint8
{
    Dynamic     UMETA(DisplayName = "Dynamic Rope"),
    Static      UMETA(DisplayName = "Static Rope"), 
    Accessory   UMETA(DisplayName = "Accessory Cord"),
    Steel       UMETA(DisplayName = "Steel Cable")
};

UENUM(BlueprintType)
enum class ERopeState : uint8
{
    Coiled      UMETA(DisplayName = "Coiled"),
    Deployed    UMETA(DisplayName = "Deployed"),
    Tensioned   UMETA(DisplayName = "Under Tension"),
    Overloaded  UMETA(DisplayName = "Overloaded"),
    Broken      UMETA(DisplayName = "Broken")
};

USTRUCT(BlueprintType)
struct FRopeProperties
{
    GENERATED_BODY()

    // Physical properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float Diameter = 10.5f; // mm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float Length = 60.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float Weight = 4.2f; // kg per 100m

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float BreakingStrength = 22.0f; // kN

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float DynamicElongation = 8.5f; // % at 80kg

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float StaticElongation = 2.3f; // % at 150kg

    // Environmental resistance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float UV_Resistance = 1.0f; // 0.0 to 1.0

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability") 
    float Water_Resistance = 0.8f; // 0.0 to 1.0

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float Abrasion_Resistance = 0.9f; // 0.0 to 1.0
};

USTRUCT(BlueprintType)
struct FRopePhysicsState
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    float CurrentTension = 0.0f; // Newtons

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MaxTensionExperienced = 0.0f; // Newtons

    UPROPERTY(Replicated, BlueprintReadOnly)
    float CurrentElongation = 0.0f; // % of original length

    UPROPERTY(Replicated, BlueprintReadOnly)
    ERopeState State = ERopeState::Coiled;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 FallsCount = 0; // Number of factor 2 falls taken

    UPROPERTY(Replicated, BlueprintReadOnly)
    float TotalEnergyAbsorbed = 0.0f; // Joules over rope lifetime
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UAdvancedRopeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAdvancedRopeComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Core rope properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Configuration")
    ERopeType RopeType = ERopeType::Dynamic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Configuration")
    FRopeProperties Properties;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rope State")
    FRopePhysicsState PhysicsState;

    // Visual representation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCableComponent* CableComponent;

    // Anchor points
    UPROPERTY(Replicated)
    class AActor* AnchorPointA;

    UPROPERTY(Replicated)
    class AActor* AnchorPointB;

    UPROPERTY(Replicated)
    FVector AttachmentLocationA;

    UPROPERTY(Replicated)
    FVector AttachmentLocationB;

    // Physics constraints
    UPROPERTY()
    UPhysicsConstraintComponent* ConstraintA;

    UPROPERTY()
    UPhysicsConstraintComponent* ConstraintB;

    // Rope deployment and management
    UFUNCTION(BlueprintCallable, Category = "Rope Operations")
    void DeployRope(AActor* StartAnchor, AActor* EndAnchor);

    UFUNCTION(BlueprintCallable, Category = "Rope Operations")
    void CoilRope();

    UFUNCTION(BlueprintCallable, Category = "Rope Operations")
    bool AttachToAnchor(AActor* Anchor, bool bIsFirstPoint = true);

    UFUNCTION(BlueprintCallable, Category = "Rope Operations")
    void DetachFromAnchor(bool bIsFirstPoint = true);

    UFUNCTION(Server, Reliable, WithValidation, CallInEditor = true)
    void ServerDeployRope(AActor* StartAnchor, AActor* EndAnchor);

    // Physics calculations
    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    float CalculateCurrentTension() const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    float CalculateElongation() const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    float CalculateFallFactor(float FallDistance) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    float CalculateImpactForce(float FallDistance, float ClimberMass) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    bool WillRopeBreak(float AppliedForce) const;

    // Environmental effects
    UFUNCTION(BlueprintCallable, Category = "Environmental")
    void ApplyWeatherEffects(float Temperature, float Humidity, bool bIsWet);

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    void ProcessAbrasion(const FVector& ContactPoint, float AbrasionAmount);

    UFUNCTION(BlueprintCallable, Category = "Environmental")
    void ProcessUVDegradation(float UVIntensity, float ExposureTime);

    // Underwater physics
    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    void ProcessUnderwaterPhysics(UWaterPhysicsComponent* WaterPhysics, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    float CalculateUnderwaterDrag(const FVector& WaterVelocity, float WaterDensity) const;

    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    void ApplyBuoyancyForces(UWaterPhysicsComponent* WaterPhysics, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    void ProcessWaterAbsorption(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    void CalculateSubmersionLevel(UWaterPhysicsComponent* WaterPhysics);

    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    FVector GetUnderwaterCurrent(const FVector& RopeSegmentLocation, UWaterPhysicsComponent* WaterPhysics) const;

    UFUNCTION(BlueprintCallable, Category = "Underwater Physics")
    void ProcessEmergencyAscentForces(float AscentRate, float WaterPressure);

    // Advanced rope physics
    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    void EnableDynamicPhysics(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    TArray<FVector> GetRopeSegmentPositions() const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    float GetDistanceAlongRope(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    bool IsPointOnRope(const FVector& WorldLocation, float Tolerance = 10.0f) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Physics")
    FVector GetRopeDirectionAtPoint(const FVector& WorldLocation) const;

    // Durability and wear
    UFUNCTION(BlueprintCallable, Category = "Durability")
    void RecordFall(float FallDistance, float ImpactForce);

    UFUNCTION(BlueprintCallable, Category = "Durability")
    float GetRemainingStrength() const;

    UFUNCTION(BlueprintCallable, Category = "Durability")
    bool ShouldRetireRope() const;

    // Multiplayer events
    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnRopeBreak(const FVector& BreakLocation);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnHighTension(float TensionValue);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnRopeSubmerged(float SubmersionPercentage);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnWaterloggedRope();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnRopeDeployed;

    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnRopeCoiled;

    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnRopeBroken;

    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnOverloaded;

    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnRopeSubmerged;

    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnRopeWaterlogged;

    UPROPERTY(BlueprintAssignable, Category = "Rope Events")
    FSimpleMulticastDelegate OnUnderwaterTensionCritical;

private:
    // Internal calculations
    void UpdatePhysicsSimulation(float DeltaTime);
    void UpdateCableProperties();
    void UpdateCablePhysicsProperties();
    void ApplyEnvironmentalPhysicsModifiers();
    void CheckBreakCondition();
    void UpdateRopeState();
    
    // Underwater physics helpers
    void UpdateUnderwaterPhysics(float DeltaTime);
    void CalculateUnderwaterForces(float DeltaTime);
    void ProcessWaterDamage(float DeltaTime);
    void UpdateBuoyancyDistribution();
    bool IsRopeSegmentSubmerged(int32 SegmentIndex) const;
    float CalculateSegmentBuoyancy(int32 SegmentIndex) const;
    void ApplyCurrentForces(const TArray<FVector>& CurrentField, float DeltaTime);
    
    // Performance optimization
    void OptimizeForDistance(float ViewerDistance);
    bool ShouldSimulatePhysics() const;

    // Wear calculation
    float CalculateWearFromTension(float Tension, float DeltaTime);
    float CalculateWearFromAbrasion(float AbrasionForce);
    float CalculateWearFromEnvironment(float Temperature, float UV, bool bWet);

    // Cached values for performance
    float CachedTension = 0.0f;
    float LastTensionUpdateTime = 0.0f;
    float TensionUpdateInterval = 0.1f; // Update tension 10 times per second

    // Degradation tracking
    float TotalWear = 0.0f;
    float UVDegradation = 0.0f;
    float AbrasionWear = 0.0f;
    float FatigueDamage = 0.0f;
    
    // Performance settings
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxSimulationDistance = 5000.0f; // 50m

    UPROPERTY(EditAnywhere, Category = "Performance") 
    int32 HighDetailSegments = 32;

    UPROPERTY(EditAnywhere, Category = "Performance")
    int32 LowDetailSegments = 8;

    // Physics optimization reference
    UPROPERTY()
    class UClimbingPerformanceManager* PerformanceManager;

public:
    // Water and cave physics integration
    UPROPERTY()
    UWaterPhysicsComponent* AssociatedWaterPhysics;

    UPROPERTY()
    UCaveEnvironmentPhysics* AssociatedCavePhysics;

    // Underwater physics state
    UPROPERTY(Replicated)
    TArray<bool> SegmentSubmersionStates; // Per-segment submersion tracking

    UPROPERTY()
    TArray<float> SegmentBuoyancies; // Per-segment buoyancy forces

    UPROPERTY()
    TArray<FVector> SegmentCurrentForces; // Per-segment water current forces

    // Water damage tracking
    float WaterDamageAccumulation = 0.0f;
    float LastWaterContactTime = 0.0f;
    bool bWasRecentlySubmerged = false;

    // Emergency ascent physics
    bool bEmergencyAscentMode = false;
    float EmergencyAscentStartDepth = 0.0f;
    float EmergencyAscentStartTime = 0.0f;

    // Performance optimization for underwater
    float UnderwaterPhysicsUpdateInterval = 0.05f; // 20Hz for underwater physics
    float LastUnderwaterUpdate = 0.0f;

    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Configuration")
    bool bEnableUnderwaterPhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Configuration")
    bool bEnableBuoyancySimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Configuration")
    bool bEnableWaterDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Configuration")
    float UnderwaterPhysicsIntensity = 1.0f;

    // Additional physics properties for integration
    struct FPhysicsOptimizationSettings
    {
        int32 HighSolverIterations = 6;
        int32 MediumSolverIterations = 4;
        int32 LowSolverIterations = 2;
        
        // Underwater physics settings
        int32 UnderwaterSolverIterations = 8; // Higher precision for underwater
        float UnderwaterStabilityDamping = 0.8f;
        float UnderwaterSubstepCount = 2;
    } PhysicsSettings;

    // Underwater physics configuration
    struct FUnderwaterPhysicsSettings
    {
        float MinimumBuoyancyForce = 1.0f; // Newtons
        float MaximumDragForce = 1000.0f; // Newtons
        float WaterloggedThreshold = 300.0f; // Seconds of submersion
        float BuoyancyDamping = 0.9f; // Reduce oscillation
        float CurrentResponseTime = 2.0f; // Seconds to reach current speed
    } UnderwaterSettings;
};