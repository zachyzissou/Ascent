#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "../Tools/ClimbingToolBase.h"
#include "AdvancedClimbingComponent.generated.h"

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
    CMOVE_None      UMETA(DisplayName = "None"),
    CMOVE_Climbing  UMETA(DisplayName = "Climbing"),
    CMOVE_Roped     UMETA(DisplayName = "On Rope"),
    CMOVE_Anchored  UMETA(DisplayName = "Anchored"),
    CMOVE_Falling   UMETA(DisplayName = "Falling"),
    CMOVE_Swinging  UMETA(DisplayName = "Swinging")
};

UENUM(BlueprintType)
enum class EGripType : uint8
{
    Jug         UMETA(DisplayName = "Jug Hold"),
    Crimp       UMETA(DisplayName = "Crimp Hold"),
    Sloper      UMETA(DisplayName = "Sloper"),
    Pinch       UMETA(DisplayName = "Pinch"),
    Pocket      UMETA(DisplayName = "Pocket"),
    Mantle      UMETA(DisplayName = "Mantle")
};

UENUM(BlueprintType)
enum class EClimbingDifficulty : uint8
{
    Grade_5_0   UMETA(DisplayName = "5.0"),
    Grade_5_1   UMETA(DisplayName = "5.1"),
    Grade_5_2   UMETA(DisplayName = "5.2"),
    Grade_5_3   UMETA(DisplayName = "5.3"),
    Grade_5_4   UMETA(DisplayName = "5.4"),
    Grade_5_5   UMETA(DisplayName = "5.5"),
    Grade_5_6   UMETA(DisplayName = "5.6"),
    Grade_5_7   UMETA(DisplayName = "5.7"),
    Grade_5_8   UMETA(DisplayName = "5.8"),
    Grade_5_9   UMETA(DisplayName = "5.9"),
    Grade_5_10a UMETA(DisplayName = "5.10a"),
    Grade_5_10b UMETA(DisplayName = "5.10b"),
    Grade_5_10c UMETA(DisplayName = "5.10c"),
    Grade_5_10d UMETA(DisplayName = "5.10d"),
    Grade_5_11a UMETA(DisplayName = "5.11a"),
    Grade_5_11b UMETA(DisplayName = "5.11b"),
    Grade_5_11c UMETA(DisplayName = "5.11c"),
    Grade_5_11d UMETA(DisplayName = "5.11d"),
    Grade_5_12a UMETA(DisplayName = "5.12a"),
    Grade_5_12b UMETA(DisplayName = "5.12b"),
    Grade_5_12c UMETA(DisplayName = "5.12c"),
    Grade_5_12d UMETA(DisplayName = "5.12d"),
    Grade_5_13a UMETA(DisplayName = "5.13a"),
    Grade_5_13b UMETA(DisplayName = "5.13b"),
    Grade_5_13c UMETA(DisplayName = "5.13c"),
    Grade_5_13d UMETA(DisplayName = "5.13d"),
    Grade_5_14a UMETA(DisplayName = "5.14a"),
    Grade_5_14b UMETA(DisplayName = "5.14b"),
    Grade_5_14c UMETA(DisplayName = "5.14c"),
    Grade_5_14d UMETA(DisplayName = "5.14d"),
    Grade_5_15a UMETA(DisplayName = "5.15a"),
    Grade_5_15b UMETA(DisplayName = "5.15b"),
    Grade_5_15c UMETA(DisplayName = "5.15c"),
    Grade_5_15d UMETA(DisplayName = "5.15d")
};

USTRUCT(BlueprintType)
struct FGripPoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Normal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGripType Type = EGripType::Jug;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Quality = 1.0f; // 0.0 to 1.0 - grip quality

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Size = 1.0f; // 0.0 to 1.0 - grip size

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EClimbingDifficulty RequiredGrade = EClimbingDifficulty::Grade_5_0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* Surface = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StaminaDrain = 1.0f; // Stamina drain multiplier for this grip

    // Calculate grip strength requirement based on type and size
    UFUNCTION(BlueprintCallable, Category = "Climbing")
    float GetRequiredGripStrength() const;

    // Calculate stamina drain rate for this grip
    UFUNCTION(BlueprintCallable, Category = "Climbing")
    float GetStaminaDrainRate() const;
};

USTRUCT(BlueprintType)
struct FClimbingState
{
    GENERATED_BODY()

    // Current grips
    UPROPERTY(Replicated, BlueprintReadOnly)
    FGripPoint LeftHandGrip;

    UPROPERTY(Replicated, BlueprintReadOnly)
    FGripPoint RightHandGrip;

    UPROPERTY(Replicated, BlueprintReadOnly)
    FGripPoint LeftFootGrip;

    UPROPERTY(Replicated, BlueprintReadOnly)
    FGripPoint RightFootGrip;

    // Stamina
    UPROPERTY(Replicated, BlueprintReadOnly)
    float CurrentStamina = 100.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MaxStamina = 100.0f;

    // Grip strength
    UPROPERTY(Replicated, BlueprintReadOnly)
    float CurrentGripStrength = 100.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MaxGripStrength = 100.0f;

    // Movement state
    UPROPERTY(Replicated, BlueprintReadOnly)
    ECustomMovementMode CustomMovementMode = ECustomMovementMode::CMOVE_None;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsResting = false;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float TimeSinceLastMove = 0.0f;

    // Rope integration
    UPROPERTY(Replicated, BlueprintReadOnly)
    UAdvancedRopeComponent* AttachedRope = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float RopeSlackLength = 0.0f;

    // Fall tracking
    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsFalling = false;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float FallDistance = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly)
    FVector FallStartLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FClimbingSettings
{
    GENERATED_BODY()

    // Stamina settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float BaseStaminaRegenRate = 5.0f; // Per second when resting

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float BaseStaminaDrainRate = 2.0f; // Per second when climbing

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float CrimpDrainMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float SloperDrainMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float JugDrainMultiplier = 0.8f;

    // Grip strength settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip")
    float GripRecoveryRate = 10.0f; // Per second when resting

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip")
    float GripFatigueRate = 3.0f; // Per second under load

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip")
    float MinGripThreshold = 20.0f; // Minimum grip strength to hold

    // Movement settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float ClimbingSpeed = 200.0f; // cm/s

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float ReachDistance = 120.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float DynoForce = 1000.0f; // Newtons for dynamic moves

    // Fall settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float FallDamageThreshold = 5.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float MaxSurvivableFall = 20.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Physics")
    float SwingDamping = 0.95f; // Swing motion damping
};

UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UAdvancedClimbingComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    UAdvancedClimbingComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Override movement functions
    virtual void PhysCustom(float deltaTime, int32 Iterations) override;
    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode) override;

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing Configuration")
    FClimbingSettings Settings;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Climbing State")
    FClimbingState ClimbingState;

    // Climbing surface detection
    UFUNCTION(BlueprintCallable, Category = "Climbing")
    bool TryStartClimbing();

    UFUNCTION(BlueprintCallable, Category = "Climbing")
    void StopClimbing();

    UFUNCTION(BlueprintCallable, Category = "Climbing")
    TArray<FGripPoint> FindNearbyGrips(float SearchRadius = 150.0f) const;

    UFUNCTION(BlueprintCallable, Category = "Climbing")
    bool IsValidClimbingSurface(const FHitResult& Hit) const;

    // Grip management
    UFUNCTION(BlueprintCallable, Category = "Grip Management")
    bool TryGrabHold(const FGripPoint& GripPoint, bool bIsLeftHand = true);

    UFUNCTION(BlueprintCallable, Category = "Grip Management")
    void ReleaseGrip(bool bIsLeftHand = true, bool bIsHand = true);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTryGrabHold(const FGripPoint& GripPoint, bool bIsLeftHand, bool bIsHand);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerReleaseGrip(bool bIsLeftHand, bool bIsHand);

    // Stamina and grip strength
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void ConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void RegenerateStamina(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Grip Strength")
    void ConsumeGripStrength(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Grip Strength")
    void RegenerateGripStrength(float DeltaTime);

    // Movement modes
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetCustomMovementMode(ECustomMovementMode NewMode);

    // Specialized climbing moves
    UFUNCTION(BlueprintCallable, Category = "Advanced Moves")
    void PerformDyno(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "Advanced Moves")
    void StartRest();

    UFUNCTION(BlueprintCallable, Category = "Advanced Moves")
    void EndRest();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPerformDyno(const FVector& TargetLocation);

    // Rope integration
    UFUNCTION(BlueprintCallable, Category = "Rope Integration")
    bool AttachToRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Integration")
    void DetachFromRope();

    UFUNCTION(BlueprintCallable, Category = "Rope Integration")
    void AdjustRopeSlack(float SlackAmount);

    // Fall mechanics
    UFUNCTION(BlueprintCallable, Category = "Fall Mechanics")
    void StartFall();

    UFUNCTION(BlueprintCallable, Category = "Fall Mechanics")
    void HandleRopeCatch(float FallDistance);

    UFUNCTION(BlueprintCallable, Category = "Fall Mechanics")
    void HandleGroundImpact(float FallDistance, float ImpactVelocity);

    UFUNCTION(BlueprintCallable, Category = "Fall Mechanics")
    float CalculateFallDamage(float FallDistance, float ImpactVelocity) const;

    // Tool integration
    UFUNCTION(BlueprintCallable, Category = "Tool Integration")
    bool PlaceTool(AClimbingToolBase* Tool, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Tool Integration")
    AClimbingToolBase* GetNearestTool(float SearchRadius = 200.0f) const;

    UFUNCTION(BlueprintCallable, Category = "Tool Integration")
    bool CanReachTool(const AClimbingToolBase* Tool) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnStartClimbing;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnStopClimbing;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnGripAcquired;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnGripLost;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnStaminaLow;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnGripStrengthLow;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnFallStart;

    UPROPERTY(BlueprintAssignable, Category = "Climbing Events")
    FSimpleMulticastDelegate OnRopeCatch;

protected:
    // Custom physics modes
    void PhysClimbing(float deltaTime, int32 Iterations);
    void PhysRoped(float deltaTime, int32 Iterations);
    void PhysAnchored(float deltaTime, int32 Iterations);
    void PhysFalling(float deltaTime, int32 Iterations);
    void PhysSwinging(float deltaTime, int32 Iterations);

    // Collision handling for different physics modes
    void HandleClimbingCollision(const FHitResult& Hit, float DeltaTime);
    void HandleRopeCollision(const FHitResult& Hit, float DeltaTime);
    void HandleSwingCollision(const FHitResult& Hit, float DeltaTime);
    void HandleRopeCatch();
    void HandleFallImpact(const FHitResult& Hit);
    
    // Movement state updates
    void UpdateGripStatesFromMovement(float DeltaTime);

    // Internal calculations
    void UpdateClimbingPhysics(float DeltaTime);
    void UpdateStaminaAndGrip(float DeltaTime);
    void CheckFallCondition();
    void UpdateRopePhysics(float DeltaTime);

    // Grip calculations
    float CalculateRequiredGripStrength(const FGripPoint& Grip) const;
    float CalculateGripStaminaDrain(const FGripPoint& Grip) const;
    bool CanMaintainGrip(const FGripPoint& Grip) const;

    // Movement calculations
    FVector CalculateClimbingVelocity(const FVector& InputVector) const;
    FVector CalculateGravityEffect() const;
    bool ValidateMovement(const FVector& ProposedLocation) const;

    // Surface analysis
    float AnalyzeSurfaceAngle(const FVector& SurfaceNormal) const;
    float AnalyzeSurfaceFriction(const UPhysicalMaterial* Material) const;
    EGripType DetermineGripType(const FHitResult& Hit) const;

    // Rope physics integration
    void ApplyRopeConstraints(FVector& Velocity, float DeltaTime);
    float CalculateRopeTension() const;
    void HandleRopeSwing(float DeltaTime);

private:
    // Cached calculations for performance
    float LastGripUpdate = 0.0f;
    float GripUpdateInterval = 0.1f; // 10Hz grip updates

    float LastStaminaUpdate = 0.0f;
    float StaminaUpdateInterval = 0.2f; // 5Hz stamina updates

    // Internal state
    TArray<FGripPoint> CachedNearbyGrips;
    float LastGripScan = 0.0f;
    float GripScanInterval = 0.5f; // 2Hz grip scanning

    // Movement assistance
    FVector LastValidClimbingPosition = FVector::ZeroVector;
    float TimeSinceLastValidPosition = 0.0f;

    // Performance optimization flags
    bool bOptimizeDistantUpdates = true;
    float PlayerViewDistance = 0.0f;
};