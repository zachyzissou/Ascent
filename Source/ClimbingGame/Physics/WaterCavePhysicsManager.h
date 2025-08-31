#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "WaterPhysicsComponent.h"
#include "CaveEnvironmentPhysics.h"
#include "WaterfallRappellingPhysics.h"
#include "CaveDivingPhysics.h"
#include "CaveLightingSystem.h"
#include "AdvancedRopeComponent.h"
#include "../Performance/FluidDynamicsOptimizer.h"
#include "../Performance/ClimbingPerformanceManager.h"
#include "EnvironmentalHazardManager.h"
#include "WaterCavePhysicsManager.generated.h"

UENUM(BlueprintType)
enum class EWaterCaveEnvironment : uint8
{
    DryLand         UMETA(DisplayName = "Dry Land"),
    ShallowWater    UMETA(DisplayName = "Shallow Water"),
    DeepWater       UMETA(DisplayName = "Deep Water"),
    Waterfall       UMETA(DisplayName = "Waterfall Area"),
    DryCave         UMETA(DisplayName = "Dry Cave"),
    FloodedCave     UMETA(DisplayName = "Flooded Cave"),
    UnderwaterCave  UMETA(DisplayName = "Underwater Cave"),
    CaveWithWater   UMETA(DisplayName = "Cave with Water Features"),
    ThermalSprings  UMETA(DisplayName = "Thermal Springs"),
    UndergroundRiver UMETA(DisplayName = "Underground River")
};

USTRUCT(BlueprintType)
struct FEnvironmentTransition
{
    GENERATED_BODY()

    // Environment transition data
    UPROPERTY(BlueprintReadWrite, Category = "Environment Transition")
    EWaterCaveEnvironment FromEnvironment = EWaterCaveEnvironment::DryLand;

    UPROPERTY(BlueprintReadWrite, Category = "Environment Transition")
    EWaterCaveEnvironment ToEnvironment = EWaterCaveEnvironment::DryLand;

    UPROPERTY(BlueprintReadWrite, Category = "Environment Transition")
    float TransitionProgress = 0.0f; // 0-1

    UPROPERTY(BlueprintReadWrite, Category = "Environment Transition")
    float TransitionDuration = 5.0f; // seconds

    UPROPERTY(BlueprintReadWrite, Category = "Environment Transition")
    bool bIsTransitioning = false;

    UPROPERTY(BlueprintReadWrite, Category = "Environment Transition")
    AActor* TransitioningActor = nullptr;
};

USTRUCT(BlueprintType)
struct FIntegratedPhysicsState
{
    GENERATED_BODY()

    // Combined state from all physics systems
    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    EWaterCaveEnvironment CurrentEnvironment = EWaterCaveEnvironment::DryLand;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    bool bInWater = false;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    bool bInCave = false;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    bool bNearWaterfall = false;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    bool bCaveDiving = false;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    float WaterDepth = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    float CaveDepth = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    float LightLevel = 0.0f; // Lux

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    float EnvironmentalStress = 0.0f; // Combined stress from all sources

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    TArray<FString> ActiveHazards;

    UPROPERTY(BlueprintReadOnly, Category = "Integrated Physics")
    float OverallSafetyRating = 1.0f; // 0-1 safety assessment
};

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AWaterCavePhysicsManager : public AActor
{
    GENERATED_BODY()

public:
    AWaterCavePhysicsManager();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Core physics components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Components")
    UWaterPhysicsComponent* WaterPhysics;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Components")
    UCaveEnvironmentPhysics* CavePhysics;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Components")
    UWaterfallRappellingPhysics* WaterfallPhysics;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Components")
    UCaveDivingPhysics* CaveDivingPhysics;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Components")
    UCaveLightingSystem* LightingSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics Components")
    UEnvironmentalHazardManager* EnvironmentalManager;

    // Performance optimization
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
    UFluidDynamicsOptimizer* FluidOptimizer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
    UClimbingPerformanceManager* PerformanceManager;

    // Environment state management
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Environment State")
    TMap<AActor*, FIntegratedPhysicsState> ActorEnvironmentStates;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment State")
    TArray<FEnvironmentTransition> ActiveTransitions;

    // Environment detection and management
    UFUNCTION(BlueprintCallable, Category = "Environment Management")
    EWaterCaveEnvironment DetectEnvironment(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Environment Management")
    void UpdateActorEnvironment(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Environment Management")
    void ProcessEnvironmentTransition(AActor* Actor, EWaterCaveEnvironment NewEnvironment);

    UFUNCTION(BlueprintCallable, Category = "Environment Management")
    FIntegratedPhysicsState GetIntegratedPhysicsState(AActor* Actor) const;

    // System coordination
    UFUNCTION(BlueprintCallable, Category = "System Coordination")
    void CoordinatePhysicsSystems(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "System Coordination")
    void ProcessCrossSystemInteractions(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "System Coordination")
    void UpdateEnvironmentalEffects(AActor* Actor, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "System Coordination")
    void SynchronizeSystemStates();

    // Rope integration across environments
    UFUNCTION(BlueprintCallable, Category = "Rope Integration")
    void ProcessRopeInEnvironment(UAdvancedRopeComponent* Rope, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Rope Integration")
    void CalculateEnvironmentalRopeEffects(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Integration")
    void ProcessRopeEnvironmentTransition(UAdvancedRopeComponent* Rope, EWaterCaveEnvironment NewEnvironment);

    // Safety and emergency coordination
    UFUNCTION(BlueprintCallable, Category = "Safety Coordination")
    void AssessOverallSafety(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Safety Coordination")
    void CoordinateEmergencyProcedures(AActor* Actor, const FString& EmergencyType);

    UFUNCTION(BlueprintCallable, Category = "Safety Coordination")
    bool IsEnvironmentSafeForActor(AActor* Actor, EWaterCaveEnvironment Environment) const;

    UFUNCTION(BlueprintCallable, Category = "Safety Coordination")
    TArray<FVector> GetEmergencyEvacuationRoutes(const FVector& CurrentLocation) const;

    // Performance coordination
    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void OptimizeAllSystems(float TargetFrameRate);

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void SetGlobalSimulationQuality(EFluidDynamicsLOD QualityLevel);

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void ProcessPerformanceBasedCulling(const FVector& ViewerLocation);

    UFUNCTION(BlueprintCallable, Category = "Performance Coordination")
    void BalanceSystemPriorities();

    // Network synchronization
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUpdateActorEnvironment(AActor* Actor, EWaterCaveEnvironment NewEnvironment);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnEnvironmentTransition(AActor* Actor, EWaterCaveEnvironment FromEnv, EWaterCaveEnvironment ToEnv);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnCombinedEmergency(AActor* Actor, const TArray<FString>& EmergencyTypes);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Environment Events")
    FSimpleMulticastDelegate OnActorEnvironmentChanged;

    UPROPERTY(BlueprintAssignable, Category = "Environment Events")
    FSimpleMulticastDelegate OnCombinedEnvironmentHazard;

    UPROPERTY(BlueprintAssignable, Category = "Environment Events")
    FSimpleMulticastDelegate OnSystemsCoordinated;

    UPROPERTY(BlueprintAssignable, Category = "Environment Events")
    FSimpleMulticastDelegate OnPerformanceOptimized;

protected:
    // Internal coordination functions
    void UpdateSystemCoordination(float DeltaTime);
    void ProcessInterSystemEffects(float DeltaTime);
    void UpdatePerformanceOptimization(float DeltaTime);
    void UpdateEnvironmentTransitions(float DeltaTime);

    // Cross-system effect helpers
    void ProcessWaterCaveInteraction(AActor* Actor);
    void ProcessWaterfallCaveInteraction(AActor* Actor);
    void ProcessLightingWaterInteraction(AActor* Actor);
    void ProcessRopeMultiEnvironmentEffects(UAdvancedRopeComponent* Rope);

    // Environment detection helpers
    bool IsLocationInWater(const FVector& Location) const;
    bool IsLocationInCave(const FVector& Location) const;
    bool IsLocationNearWaterfall(const FVector& Location) const;
    float GetEnvironmentInfluenceStrength(const FVector& Location, EWaterCaveEnvironment Environment) const;

    // Safety assessment helpers
    float CalculateCombinedHazardLevel(AActor* Actor) const;
    void ProcessMultiSystemEmergencies(AActor* Actor);
    bool IsEvacuationPathViable(const FVector& Start, const FVector& End) const;

    // Performance coordination helpers
    void DistributePerformanceBudget();
    void AdjustSystemPriorities(float FrameTimeOverhead);
    void ProcessSystemLODCoordination();

private:
    // System coordination state
    float LastCoordinationUpdate = 0.0f;
    float CoordinationUpdateInterval = 0.2f; // 5Hz coordination
    float LastPerformanceUpdate = 0.0f;
    float PerformanceUpdateInterval = 1.0f; // 1Hz performance optimization

    // Environment detection cache
    TMap<FVector, EWaterCaveEnvironment> EnvironmentCache;
    float LastEnvironmentCacheUpdate = 0.0f;
    float EnvironmentCacheTimeout = 2.0f;

    // Performance tracking
    TArray<float> SystemFrameTimes;
    float TotalSystemOverhead = 0.0f;

    // Emergency coordination state
    TMap<AActor*, TArray<FString>> ActiveActorEmergencies;
    bool bGlobalEmergencyActive = false;

    // System integration weights
    float WaterPhysicsWeight = 1.0f;
    float CavePhysicsWeight = 1.0f;
    float WaterfallPhysicsWeight = 1.0f;
    float CaveDivingWeight = 1.0f;
    float LightingWeight = 0.8f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MaxCombinedSystemTime = 8.0f; // ms per frame for all systems

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableSystemCoordination = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnablePerformanceCoordination = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableEmergencyCoordination = true;

public:
    // Blueprint-accessible configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager Configuration")
    bool bAutoDetectEnvironments = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager Configuration")
    bool bCoordinateSystemOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager Configuration")
    bool bEnableCrossSystemEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager Configuration")
    float GlobalPhysicsIntensity = 1.0f;

    // Performance targets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    float TargetFrameRate = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    float AcceptableFrameRate = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Targets")
    bool bMaintainMinimumFrameRate = true;
};