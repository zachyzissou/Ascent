#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/World.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "Net/UnrealNetwork.h"
#include "GeologicalHazardSystem.generated.h"

UENUM(BlueprintType)
enum class ERockfallTrigger : uint8
{
    Manual          UMETA(DisplayName = "Manual"),
    Proximity       UMETA(DisplayName = "Proximity"),
    TimeBased       UMETA(DisplayName = "Time Based"),
    WeatherBased    UMETA(DisplayName = "Weather Based"),
    Seismic         UMETA(DisplayName = "Seismic"),
    PlayerAction    UMETA(DisplayName = "Player Action")
};

UENUM(BlueprintType)
enum class ERockSize : uint8
{
    Pebble      UMETA(DisplayName = "Pebble"),     // < 64mm
    Cobble      UMETA(DisplayName = "Cobble"),     // 64-256mm
    Boulder     UMETA(DisplayName = "Boulder"),    // 256mm-2m
    LargeBoulder UMETA(DisplayName = "Large Boulder"), // > 2m
    Slab        UMETA(DisplayName = "Slab")        // Flat rock formation
};

UENUM(BlueprintType)
enum class EAvalancheType : uint8
{
    SlabAvalanche   UMETA(DisplayName = "Slab Avalanche"),
    LooseSnow       UMETA(DisplayName = "Loose Snow"),
    WetAvalanche    UMETA(DisplayName = "Wet Avalanche"),
    WindSlab        UMETA(DisplayName = "Wind Slab"),
    IceAvalanche    UMETA(DisplayName = "Ice Avalanche"),
    Cornice         UMETA(DisplayName = "Cornice Collapse")
};

UENUM(BlueprintType)
enum class ESnowStability : uint8
{
    VeryStable      UMETA(DisplayName = "Very Stable"),
    Stable          UMETA(DisplayName = "Stable"),
    FairlyStable    UMETA(DisplayName = "Fairly Stable"),
    Unstable        UMETA(DisplayName = "Unstable"),
    VeryUnstable    UMETA(DisplayName = "Very Unstable")
};

USTRUCT(BlueprintType)
struct FRockfallEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall")
    ERockSize RockSize = ERockSize::Cobble;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall", Meta = (ClampMin = "1", ClampMax = "50"))
    int32 RockCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall")
    FVector InitialVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall")
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall", Meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float Duration = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall")
    bool bPlayWarningSound = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rockfall", Meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float WarningTime = 2.0f; // seconds before rocks start falling
};

USTRUCT(BlueprintType)
struct FSnowConditions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "0.0", ClampMax = "500.0"))
    float SnowDepth = 0.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float NewSnowDepth = 0.0f; // cm in last 24 hours

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "-40.0", ClampMax = "40.0"))
    float SnowTemperature = -5.0f; // Celsius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float WindLoading = 0.0f; // Wind-transported snow

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "100.0", ClampMax = "800.0"))
    float SnowDensity = 300.0f; // kg/m³

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions")
    ESnowStability Stability = ESnowStability::FairlyStable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float SlopeAngle = 35.0f; // degrees

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions")
    bool bHasWeakLayer = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions", Meta = (ClampMin = "0.0", ClampMax = "200.0"))
    float WeakLayerDepth = 0.0f; // cm from surface
};

USTRUCT(BlueprintType)
struct FAvalancheEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche")
    EAvalancheType AvalancheType = EAvalancheType::SlabAvalanche;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche")
    FVector StartLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche")
    TArray<FVector> AvalanchePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche", Meta = (ClampMin = "10.0", ClampMax = "2000.0"))
    float Width = 100.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche", Meta = (ClampMin = "10.0", ClampMax = "5000.0"))
    float Length = 500.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche", Meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float AverageDepth = 2.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche", Meta = (ClampMin = "5.0", ClampMax = "200.0"))
    float MaxSpeed = 60.0f; // km/h

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche", Meta = (ClampMin = "5.0", ClampMax = "300.0"))
    float Duration = 30.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche")
    FSnowConditions SnowConditions;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRockfallWarning, FRockfallEvent, RockfallEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRockfallImpact, AActor*, HitActor, float, ImpactForce);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAvalancheWarning, FAvalancheEvent, AvalancheEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAvalancheImpact, AActor*, Victim, float, BurialDepth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSnowStabilityChanged, ESnowStability, NewStability);

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API ARockfallHazard : public AActor
{
    GENERATED_BODY()

public:
    ARockfallHazard();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    virtual void Tick(float DeltaTime) override;

    // Rockfall Control Functions
    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    void TriggerRockfall(const FRockfallEvent& RockfallEvent);

    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard") 
    void SetRockfallProbability(float NewProbability);

    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    void EnableProximityTrigger(bool bEnable, float TriggerRadius = 1000.0f);

    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    void SetWeatherInfluence(float TemperatureThreshold, float PrecipitationMultiplier);

    // Rock Quality Assessment
    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    float GetRockQualityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    bool IsLocationInRockfallZone(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    TArray<FVector> GetSafeZones() const;

    // Educational Functions
    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    FString GetRockfallRiskAssessment() const;

    UFUNCTION(BlueprintCallable, Category = "Rockfall Hazard")
    TArray<FString> GetSafetyRecommendations() const;

    // Event Delegates
    UPROPERTY(BlueprintAssignable, Category = "Rockfall Hazard")
    FOnRockfallWarning OnRockfallWarning;

    UPROPERTY(BlueprintAssignable, Category = "Rockfall Hazard")
    FOnRockfallImpact OnRockfallImpact;

protected:
    // Core Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* HazardZoneMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* ProximityTrigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* DustEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* RockfallAudioComponent;

    // Hazard Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseProbability = 0.1f; // Base chance per hour

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration")
    ERockfallTrigger TriggerType = ERockfallTrigger::TimeBased;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration")
    TArray<FRockfallEvent> PredefinedRockfallEvents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration")
    TArray<UStaticMesh*> RockMeshes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration")
    TMap<ERockSize, float> RockSizeMasses; // kg

    // Environmental Influence
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Influence", Meta = (ClampMin = "-20.0", ClampMax = "50.0"))
    float FreezeTh‍awTemperatureThreshold = 0.0f; // Celsius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Influence", Meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float RainRockfallMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Influence", Meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float SeismicMultiplier = 3.0f;

    // Safety Zones
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
    TArray<FVector> SafeZoneLocations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety", Meta = (ClampMin = "100.0", ClampMax = "1000.0"))
    float SafeZoneRadius = 300.0f;

    // Audio Assets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* RockfallWarningSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* RockImpactSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* RockBounceSound;

private:
    UPROPERTY()
    TArray<AActor*> ActiveRocks;

    UPROPERTY()
    float RockfallTimer = 0.0f;

    UPROPERTY()
    float CurrentProbabilityModifier = 1.0f;

    void SpawnRock(ERockSize RockSize, const FVector& SpawnLocation, const FVector& InitialVelocity);
    void UpdateRockfallProbability(float DeltaTime);
    float CalculateEnvironmentalModifier() const;
    
    UFUNCTION()
    void OnProximityTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnRockImpact(AActor* Rock, const FHitResult& HitResult);
};

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AAvalancheSystem : public AActor
{
    GENERATED_BODY()

public:
    AAvalancheSystem();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    virtual void Tick(float DeltaTime) override;

    // Avalanche Control Functions
    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    void TriggerAvalanche(const FAvalancheEvent& AvalancheEvent);

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    void UpdateSnowConditions(const FSnowConditions& NewConditions);

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    ESnowStability AssessSnowStability(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    float CalculateAvalancheRisk(const FVector& Location) const;

    // Rescue and Safety Functions
    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    bool IsActorBuriedInAvalanche(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    float GetBurialDepth(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    TArray<FVector> LocateTransceiverSignals(const FVector& SearchCenter, float SearchRadius) const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    void StartRescueOperation(const FVector& BurialLocation);

    // Educational Functions
    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    FString GenerateAvalancheBulletin() const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    TArray<FString> GetAvalancheSafetyTips() const;

    UFUNCTION(BlueprintCallable, Category = "Avalanche System")
    bool PerformSnowPitTest(const FVector& Location, FSnowConditions& OutResults) const;

    // Event Delegates
    UPROPERTY(BlueprintAssignable, Category = "Avalanche System")
    FOnAvalancheWarning OnAvalancheWarning;

    UPROPERTY(BlueprintAssignable, Category = "Avalanche System")
    FOnAvalancheImpact OnAvalancheImpact;

    UPROPERTY(BlueprintAssignable, Category = "Avalanche System")
    FOnSnowStabilityChanged OnSnowStabilityChanged;

protected:
    // Core Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* AvalancheZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UParticleSystemComponent* SnowEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* AvalancheAudioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* SnowPackMesh;

    // Snow and Avalanche Configuration
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Snow Conditions")
    FSnowConditions CurrentSnowConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Configuration")
    TArray<FAvalancheEvent> PredefinedAvalanches;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Configuration", Meta = (ClampMin = "15.0", ClampMax = "60.0"))
    float CriticalSlopeAngle = 38.0f; // degrees

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Configuration", Meta = (ClampMin = "20.0", ClampMax = "200.0"))
    float CriticalSnowDepth = 60.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avalanche Configuration", Meta = (ClampMin = "10.0", ClampMax = "100.0"))
    float CriticalWindLoading = 25.0f; // cm

    // Rescue Equipment Simulation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue Equipment")
    float TransceiverRange = 5000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue Equipment")
    float ProbeLength = 300.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rescue Equipment", Meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float RescueTimeLimit = 15.0f; // minutes for viable rescue

    // Environmental Factors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Factors")
    TArray<FVector> WindDirectionHistory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Factors")
    TArray<float> TemperatureHistory; // Last 7 days

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Factors")
    TArray<float> PrecipitationHistory; // Last 7 days

    // Audio Assets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* AvalancheRumbleSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio") 
    USoundCue* TransceiverBeepSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundCue* RescueDiggingSound;

private:
    UPROPERTY()
    TArray<AActor*> BuriedActors;

    UPROPERTY()
    TMap<AActor*, float> BurialDepths;

    UPROPERTY()
    TMap<AActor*, float> BurialTimes;

    UPROPERTY()
    bool bAvalancheActive = false;

    UPROPERTY()
    float AvalancheStartTime = 0.0f;

    void UpdateSnowStability();
    void ProcessAvalancheFlow(const FAvalancheEvent& Event, float DeltaTime);
    void CheckForBuriedActors(const FVector& AvalancheCenter, float Radius);
    float CalculateSnowPackStress(const FVector& Location) const;
    EAvalancheType DetermineAvalancheType(const FSnowConditions& Conditions) const;
    
    UFUNCTION()
    void OnAvalancheZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

// Geological Hazard Calculation Library
UCLASS()
class CLIMBINGGAME_API UGeologicalHazardLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static float CalculateRockfallTrajectory(const FVector& StartLocation, const FVector& InitialVelocity, float Mass, float DragCoefficient, float TimeStep = 0.1f);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static FVector PredictRockImpactLocation(const FVector& StartLocation, const FVector& InitialVelocity, float Mass);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static float CalculateAvalancheRunoutDistance(const FSnowConditions& SnowConditions, float SlopeAngle, float VerticalDrop);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static ESnowStability CalculateSnowStability(const FSnowConditions& Conditions);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static float CalculateAvalancheSpeed(EAvalancheType Type, float SlopeAngle, const FSnowConditions& Conditions);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static bool IsRockfallLikelyFromWeather(float Temperature, float Precipitation, float WindSpeed);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static float CalculateRockQualityRating(float WeatherExposure, float RockAge, float JointDensity);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Geological Hazards")
    static TArray<FVector> GenerateEscapeRoutes(const FVector& HazardCenter, float HazardRadius, const TArray<FVector>& SafeZones);
};