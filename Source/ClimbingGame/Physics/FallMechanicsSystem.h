#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "AdvancedRopeComponent.h"
#include "FallMechanicsSystem.generated.h"

UENUM(BlueprintType)
enum class EFallType : uint8
{
    Free            UMETA(DisplayName = "Free Fall"),
    Rope            UMETA(DisplayName = "Rope Fall"), 
    Pendulum        UMETA(DisplayName = "Pendulum Fall"),
    Factor2         UMETA(DisplayName = "Factor 2 Fall"),
    Groundfall      UMETA(DisplayName = "Ground Fall"),
    Slide           UMETA(DisplayName = "Sliding Fall"),
    Tumble          UMETA(DisplayName = "Tumbling Fall")
};

UENUM(BlueprintType)
enum class EInjurySeverity : uint8
{
    None            UMETA(DisplayName = "No Injury"),
    Minor           UMETA(DisplayName = "Minor Injury"),
    Moderate        UMETA(DisplayName = "Moderate Injury"),
    Severe          UMETA(DisplayName = "Severe Injury"),
    Critical        UMETA(DisplayName = "Critical Injury"),
    Fatal           UMETA(DisplayName = "Fatal Injury")
};

UENUM(BlueprintType)
enum class EInjuryType : uint8
{
    Bruising        UMETA(DisplayName = "Bruising"),
    Abrasion        UMETA(DisplayName = "Abrasion"),
    Sprain          UMETA(DisplayName = "Sprain"),
    Strain          UMETA(DisplayName = "Muscle Strain"),
    Fracture        UMETA(DisplayName = "Fracture"),
    Dislocation     UMETA(DisplayName = "Dislocation"),
    Concussion      UMETA(DisplayName = "Concussion"),
    InternalBleeding UMETA(DisplayName = "Internal Bleeding"),
    SpinalInjury    UMETA(DisplayName = "Spinal Injury")
};

UENUM(BlueprintType)
enum class EBodyPart : uint8
{
    Head            UMETA(DisplayName = "Head"),
    Neck            UMETA(DisplayName = "Neck"),
    LeftShoulder    UMETA(DisplayName = "Left Shoulder"),
    RightShoulder   UMETA(DisplayName = "Right Shoulder"),
    LeftArm         UMETA(DisplayName = "Left Arm"),
    RightArm        UMETA(DisplayName = "Right Arm"),
    LeftHand        UMETA(DisplayName = "Left Hand"),
    RightHand       UMETA(DisplayName = "Right Hand"),
    Chest           UMETA(DisplayName = "Chest"),
    Back            UMETA(DisplayName = "Back"),
    Abdomen         UMETA(DisplayName = "Abdomen"),
    Pelvis          UMETA(DisplayName = "Pelvis"),
    LeftLeg         UMETA(DisplayName = "Left Leg"),
    RightLeg        UMETA(DisplayName = "Right Leg"),
    LeftFoot        UMETA(DisplayName = "Left Foot"),
    RightFoot       UMETA(DisplayName = "Right Foot")
};

USTRUCT(BlueprintType)
struct FFallData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EFallType FallType = EFallType::Free;

    UPROPERTY(BlueprintReadOnly)
    float FallDistance = 0.0f; // meters

    UPROPERTY(BlueprintReadOnly)
    float FallFactor = 0.0f; // Fall factor calculation

    UPROPERTY(BlueprintReadOnly)
    float ImpactVelocity = 0.0f; // m/s at impact

    UPROPERTY(BlueprintReadOnly)
    float ImpactForce = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector ImpactLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float PendulumEnergy = 0.0f; // Joules for pendulum falls

    UPROPERTY(BlueprintReadOnly)
    float RotationalVelocity = 0.0f; // rad/s for tumbling falls

    UPROPERTY(BlueprintReadOnly)
    bool bRopeCaught = false;

    UPROPERTY(BlueprintReadOnly)
    float TimeInAir = 0.0f; // seconds

    UPROPERTY(BlueprintReadOnly)
    AActor* ImpactSurface = nullptr;
};

USTRUCT(BlueprintType)
struct FInjury
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EInjuryType Type = EInjuryType::Bruising;

    UPROPERTY(BlueprintReadOnly)
    EInjurySeverity Severity = EInjurySeverity::None;

    UPROPERTY(BlueprintReadOnly)
    EBodyPart BodyPart = EBodyPart::Head;

    UPROPERTY(BlueprintReadOnly)
    float Damage = 0.0f; // 0-100 scale

    UPROPERTY(BlueprintReadOnly)
    float HealingTime = 0.0f; // seconds to heal

    UPROPERTY(BlueprintReadOnly)
    float RemainingHealTime = 0.0f; // time left to heal

    UPROPERTY(BlueprintReadOnly)
    bool bAffectsMovement = false;

    UPROPERTY(BlueprintReadOnly)
    bool bAffectsGrip = false;

    UPROPERTY(BlueprintReadOnly)
    bool bAffectsStamina = false;

    UPROPERTY(BlueprintReadOnly)
    float MovementPenalty = 0.0f; // 0.0 to 1.0 multiplier

    UPROPERTY(BlueprintReadOnly)
    float GripPenalty = 0.0f; // 0.0 to 1.0 multiplier

    UPROPERTY(BlueprintReadOnly)
    float StaminaPenalty = 0.0f; // 0.0 to 1.0 multiplier

    UPROPERTY(BlueprintReadOnly)
    FString Description = TEXT("");
};

USTRUCT(BlueprintType)
struct FInjurySystemState
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<FInjury> ActiveInjuries;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float OverallHealth = 100.0f; // 0-100

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MovementMultiplier = 1.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float GripMultiplier = 1.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float StaminaMultiplier = 1.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsConscious = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bCanClimb = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bRequiresMedicalAttention = false;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 TotalFalls = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float ShockLevel = 0.0f; // 0-1, affects all performance
};

USTRUCT(BlueprintType)
struct FFallMechanicsSettings
{
    GENERATED_BODY()

    // Fall physics settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float GravityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float AirResistance = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float TerminalVelocity = 56.0f; // m/s (200 km/h)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float ClimberMass = 70.0f; // kg

    // Injury calculation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Injury Calculation")
    float MinInjuryVelocity = 3.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Injury Calculation")
    float SevereInjuryVelocity = 8.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Injury Calculation")
    float FatalInjuryVelocity = 15.0f; // m/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Injury Calculation")
    float InjuryRandomness = 0.2f; // Random factor in injury calculation

    // Rope fall settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Falls")
    float RopeStretchFactor = 0.08f; // Dynamic rope stretch percentage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Falls")
    float RopeDampingFactor = 0.3f; // Energy absorption

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope Falls")
    float MaxRopeForce = 12000.0f; // Newtons - typical dynamic rope limit

    // Ground impact settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Impact")
    float GroundStiffness = 100000.0f; // N/m

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Impact")
    float GroundDamping = 5000.0f; // Ns/m

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Impact")
    float SurfaceHardnessMultiplier = 1.0f; // Rock=1.0, snow=0.3, water=0.1

    // Healing settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
    float HealingRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
    float RestHealingBonus = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
    bool bEnableNaturalHealing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
    float ShockRecoveryRate = 0.1f; // per second
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UFallMechanicsSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UFallMechanicsSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Mechanics")
    FFallMechanicsSettings Settings;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Injury System")
    FInjurySystemState InjuryState;

    // Fall detection and analysis
    UFUNCTION(BlueprintCallable, Category = "Fall Detection")
    void StartFall(EFallType FallType);

    UFUNCTION(BlueprintCallable, Category = "Fall Detection") 
    void EndFall(const FVector& ImpactLocation, const FVector& ImpactVelocity, AActor* ImpactSurface = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Fall Analysis")
    FFallData AnalyzeFall(const FVector& StartLocation, const FVector& EndLocation, 
                         const FVector& ImpactVelocity, EFallType FallType) const;

    // Rope fall specific
    UFUNCTION(BlueprintCallable, Category = "Rope Falls")
    void HandleRopeFall(UAdvancedRopeComponent* Rope, float FallDistance);

    UFUNCTION(BlueprintCallable, Category = "Rope Falls")
    float CalculateRopeFallForce(UAdvancedRopeComponent* Rope, float FallDistance, float ClimberVelocity) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Falls")
    float CalculateFallFactor(float FallDistance, float RopeLength) const;

    // Ground impact
    UFUNCTION(BlueprintCallable, Category = "Ground Impact")
    void HandleGroundImpact(const FVector& ImpactVelocity, const FVector& ImpactLocation, AActor* Surface);

    UFUNCTION(BlueprintCallable, Category = "Ground Impact")
    float CalculateGroundImpactForce(float ImpactVelocity, float SurfaceHardness) const;

    // Injury system
    UFUNCTION(BlueprintCallable, Category = "Injury System")
    void ApplyInjury(const FInjury& Injury);

    UFUNCTION(BlueprintCallable, Category = "Injury System")
    void GenerateInjuriesFromImpact(const FFallData& FallData);

    UFUNCTION(BlueprintCallable, Category = "Injury System")
    void UpdateInjuryHealing(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Injury System")
    void RemoveHeatedInjuries();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerApplyInjury(const FInjury& Injury);

    // Injury analysis
    UFUNCTION(BlueprintCallable, Category = "Injury Analysis")
    EInjurySeverity DetermineInjurySeverity(float ImpactForce, EBodyPart BodyPart) const;

    UFUNCTION(BlueprintCallable, Category = "Injury Analysis")
    EInjuryType DetermineInjuryType(const FFallData& FallData, EBodyPart BodyPart) const;

    UFUNCTION(BlueprintCallable, Category = "Injury Analysis")
    EBodyPart DetermineImpactBodyPart(const FVector& ImpactDirection, const FVector& CharacterRotation) const;

    UFUNCTION(BlueprintCallable, Category = "Injury Analysis")
    float CalculateInjuryHealingTime(const FInjury& Injury) const;

    // Performance effects
    UFUNCTION(BlueprintCallable, Category = "Performance Effects")
    void UpdatePerformanceMultipliers();

    UFUNCTION(BlueprintCallable, Category = "Performance Effects")
    float GetMovementMultiplier() const { return InjuryState.MovementMultiplier; }

    UFUNCTION(BlueprintCallable, Category = "Performance Effects")
    float GetGripMultiplier() const { return InjuryState.GripMultiplier; }

    UFUNCTION(BlueprintCallable, Category = "Performance Effects")
    float GetStaminaMultiplier() const { return InjuryState.StaminaMultiplier; }

    // Medical treatment
    UFUNCTION(BlueprintCallable, Category = "Medical Treatment")
    void ApplyMedicalTreatment(EInjuryType TreatmentType, float Effectiveness);

    UFUNCTION(BlueprintCallable, Category = "Medical Treatment")
    void ApplyPainkillers(float Effectiveness);

    UFUNCTION(BlueprintCallable, Category = "Medical Treatment")
    void ApplyFirstAid(EBodyPart BodyPart, float Effectiveness);

    // Status queries
    UFUNCTION(BlueprintCallable, Category = "Status")
    bool CanContinueClimbing() const;

    UFUNCTION(BlueprintCallable, Category = "Status")
    bool RequiresEvacuation() const;

    UFUNCTION(BlueprintCallable, Category = "Status")
    float GetOverallHealthPercentage() const { return InjuryState.OverallHealth; }

    UFUNCTION(BlueprintCallable, Category = "Status")
    TArray<FInjury> GetActiveInjuries() const { return InjuryState.ActiveInjuries; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Fall Events")
    FSimpleMulticastDelegate OnFallStarted;

    UPROPERTY(BlueprintAssignable, Category = "Fall Events")
    FSimpleMulticastDelegate OnFallEnded;

    UPROPERTY(BlueprintAssignable, Category = "Fall Events")
    FSimpleMulticastDelegate OnRopeCatch;

    UPROPERTY(BlueprintAssignable, Category = "Fall Events")
    FSimpleMulticastDelegate OnGroundImpact;

    UPROPERTY(BlueprintAssignable, Category = "Injury Events")
    FSimpleMulticastDelegate OnInjuryInflicted;

    UPROPERTY(BlueprintAssignable, Category = "Injury Events")
    FSimpleMulticastDelegate OnInjuryHealed;

    UPROPERTY(BlueprintAssignable, Category = "Injury Events")
    FSimpleMulticastDelegate OnBecameIncapacitated;

    UPROPERTY(BlueprintAssignable, Category = "Injury Events")
    FSimpleMulticastDelegate OnRequireEvacuation;

protected:
    // Internal fall tracking
    FFallData CurrentFall;
    bool bIsFalling = false;
    float FallStartTime = 0.0f;
    FVector FallStartLocation = FVector::ZeroVector;
    
    // Physics calculations
    FVector CalculateFallTrajectory(float DeltaTime, const FVector& CurrentVelocity) const;
    float CalculateAirResistance(float Velocity) const;
    void ApplyGravityAndAirResistance(FVector& Velocity, float DeltaTime) const;

    // Injury generation algorithms
    TArray<FInjury> GenerateTraumaticInjuries(float ImpactForce, const FVector& ImpactDirection) const;
    TArray<FInjury> GenerateAbrasionInjuries(float SlideDistance, const AActor* Surface) const;
    TArray<FInjury> GenerateFractureRisk(float ImpactForce, EBodyPart BodyPart) const;

    // Body part vulnerability data
    float GetBodyPartVulnerability(EBodyPart BodyPart) const;
    float GetBodyPartProtection(EBodyPart BodyPart) const; // From gear
    bool IsBodyPartCritical(EBodyPart BodyPart) const;

    // Shock and trauma effects
    void ApplyShockFromInjury(const FInjury& Injury);
    void UpdateShockEffects(float DeltaTime);
    void CheckForIncapacitation();

private:
    // Performance optimization
    float LastInjuryUpdate = 0.0f;
    float InjuryUpdateInterval = 1.0f; // Update injuries once per second

    // Medical state tracking
    TMap<EInjuryType, float> TreatmentEffectiveness; // Track applied treatments
    float LastTreatmentTime = 0.0f;
    bool bHasReceivedFirstAid = false;

    // Fall statistics for analytics
    TArray<FFallData> FallHistory;
    int32 MaxFallHistorySize = 50;

    // Critical thresholds
    static constexpr float UNCONSCIOUSNESS_THRESHOLD = 30.0f;
    static constexpr float EVACUATION_THRESHOLD = 50.0f;
    static constexpr float SHOCK_INCAPACITATION_THRESHOLD = 0.8f;
};