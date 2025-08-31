#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Net/UnrealNetwork.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "ClimbingToolBase.generated.h"

UENUM(BlueprintType)
enum class EToolType : uint8
{
    Anchor          UMETA(DisplayName = "Anchor"),
    Carabiner       UMETA(DisplayName = "Carabiner"), 
    Pulley          UMETA(DisplayName = "Pulley"),
    GrapplingHook   UMETA(DisplayName = "Grappling Hook"),
    Ascender        UMETA(DisplayName = "Ascender"),
    Descender       UMETA(DisplayName = "Descender"),
    Belay           UMETA(DisplayName = "Belay Device"),
    Quickdraw       UMETA(DisplayName = "Quickdraw")
};

UENUM(BlueprintType)
enum class EToolState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    InUse           UMETA(DisplayName = "In Use"),
    UnderLoad       UMETA(DisplayName = "Under Load"),
    Overloaded      UMETA(DisplayName = "Overloaded"),
    Damaged         UMETA(DisplayName = "Damaged"),
    Broken          UMETA(DisplayName = "Broken")
};

UENUM(BlueprintType)
enum class EAnchorType : uint8
{
    Bolt            UMETA(DisplayName = "Expansion Bolt"),
    Piton           UMETA(DisplayName = "Piton"),
    Cam             UMETA(DisplayName = "Spring Loaded Cam"),
    Nut             UMETA(DisplayName = "Nut/Stopper"),
    Sling           UMETA(DisplayName = "Sling Anchor"),
    Natural         UMETA(DisplayName = "Natural Anchor")
};

USTRUCT(BlueprintType)
struct FToolProperties
{
    GENERATED_BODY()

    // Strength ratings (in kN)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength")
    float MajorAxisStrength = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength") 
    float MinorAxisStrength = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength")
    float GateOpenStrength = 7.0f; // For carabiners

    // Physical properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
    float Weight = 0.05f; // kg

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
    float MaxLoadAngle = 30.0f; // degrees off-axis before strength reduction

    // Durability
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float MaxCycles = 10000.0f; // Load cycles before retirement

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float CorrosionResistance = 0.95f; // 0.0 to 1.0

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float WearResistance = 0.9f; // 0.0 to 1.0
};

USTRUCT(BlueprintType)
struct FToolLoadData
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    float CurrentLoad = 0.0f; // Newtons

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MaxLoadExperienced = 0.0f; // Newtons

    UPROPERTY(Replicated, BlueprintReadOnly)
    FVector LoadDirection = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 LoadCycles = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float TotalWear = 0.0f; // 0.0 to 1.0
};

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AClimbingToolBase : public AActor
{
    GENERATED_BODY()

public:
    AClimbingToolBase();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Core components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ToolMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* InteractionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPhysicsConstraintComponent* LoadConstraint;

    // Tool configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Configuration")
    EToolType ToolType = EToolType::Anchor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Configuration")
    FToolProperties Properties;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Tool State")
    EToolState CurrentState = EToolState::Idle;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Tool State")
    FToolLoadData LoadData;

    // Rope connections
    UPROPERTY(Replicated)
    TArray<UAdvancedRopeComponent*> ConnectedRopes;

    UPROPERTY(Replicated)
    int32 MaxRopeConnections = 2;

    // Placement validation
    UFUNCTION(BlueprintCallable, Category = "Tool Placement")
    virtual bool CanPlaceAt(const FVector& Location, const FVector& SurfaceNormal) const;

    UFUNCTION(BlueprintCallable, Category = "Tool Placement")
    virtual bool PlaceToolAt(const FVector& Location, const FVector& SurfaceNormal);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPlaceTool(const FVector& Location, const FVector& SurfaceNormal);

    // Rope management
    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    virtual bool CanConnectRope(UAdvancedRopeComponent* Rope) const;

    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    virtual bool ConnectRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    virtual void DisconnectRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerConnectRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerDisconnectRope(UAdvancedRopeComponent* Rope);

    // Load calculations
    UFUNCTION(BlueprintCallable, Category = "Physics")
    virtual void UpdateLoadCalculations(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Physics")
    virtual float GetCurrentLoad() const { return LoadData.CurrentLoad; }

    UFUNCTION(BlueprintCallable, Category = "Physics")
    virtual FVector GetLoadDirection() const { return LoadData.LoadDirection; }

    UFUNCTION(BlueprintCallable, Category = "Physics")
    virtual bool IsOverloaded() const;

    // Durability and wear
    UFUNCTION(BlueprintCallable, Category = "Durability")
    virtual void ApplyWear(float WearAmount);

    UFUNCTION(BlueprintCallable, Category = "Durability")
    virtual float GetRemainingStrength() const;

    UFUNCTION(BlueprintCallable, Category = "Durability")
    virtual bool ShouldReplace() const;

    // Mechanical advantage (for pulleys)
    UFUNCTION(BlueprintCallable, Category = "Mechanics")
    virtual float GetMechanicalAdvantage() const { return 1.0f; }

    // Tool-specific functions (virtual for derived classes)
    UFUNCTION(BlueprintCallable, Category = "Tool Operations")
    virtual void ActivateTool() {}

    UFUNCTION(BlueprintCallable, Category = "Tool Operations")
    virtual void DeactivateTool() {}

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Tool Events")
    FSimpleMulticastDelegate OnToolPlaced;

    UPROPERTY(BlueprintAssignable, Category = "Tool Events")
    FSimpleMulticastDelegate OnRopeConnected;

    UPROPERTY(BlueprintAssignable, Category = "Tool Events")
    FSimpleMulticastDelegate OnRopeDisconnected;

    UPROPERTY(BlueprintAssignable, Category = "Tool Events")
    FSimpleMulticastDelegate OnOverloaded;

    UPROPERTY(BlueprintAssignable, Category = "Tool Events")
    FSimpleMulticastDelegate OnToolFailure;

protected:
    // Internal calculations
    virtual void CalculateCurrentLoad();
    virtual void UpdateToolState();
    virtual void CheckFailureCondition();
    
    // Load distribution
    virtual FVector CalculateLoadVector() const;
    virtual float CalculateAngleDerating(const FVector& LoadDirection) const;
    
    // Environmental effects
    virtual void ProcessCorrosion(float DeltaTime);
    virtual void ProcessWear(float DeltaTime);

private:
    // Cached calculations for performance
    float LastLoadUpdate = 0.0f;
    float LoadUpdateInterval = 0.1f; // 10Hz update rate

    // Internal state tracking
    bool bIsPlaced = false;
    FVector PlacementLocation = FVector::ZeroVector;
    FVector PlacementNormal = FVector::ZeroVector;
};

// Anchor-specific implementation
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AAnchorTool : public AClimbingToolBase
{
    GENERATED_BODY()

public:
    AAnchorTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor Configuration")
    EAnchorType AnchorType = EAnchorType::Bolt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor Configuration")
    float PlacementDepth = 5.0f; // cm into rock

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor Configuration")
    float MinRockHardness = 3.0f; // Mohs scale

    // Anchor-specific validation
    virtual bool CanPlaceAt(const FVector& Location, const FVector& SurfaceNormal) const override;
    virtual bool PlaceToolAt(const FVector& Location, const FVector& SurfaceNormal) override;

protected:
    // Anchor-specific load calculations
    virtual void CalculateCurrentLoad() override;
    
    // Pull-out testing simulation
    UFUNCTION(BlueprintCallable, Category = "Anchor Testing")
    float SimulatePullOutTest() const;

private:
    // Anchor placement quality factors
    float PlacementQuality = 1.0f; // 0.0 to 1.0
    float RockQuality = 1.0f; // 0.0 to 1.0
};

// Pulley system implementation  
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API APulleyTool : public AClimbingToolBase
{
    GENERATED_BODY()

public:
    APulleyTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Configuration")
    float WheelDiameter = 5.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Configuration")
    float BearingEfficiency = 0.95f; // 0.0 to 1.0

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Configuration")
    int32 NumberOfSheaves = 1;

    // Pulley-specific functions
    virtual float GetMechanicalAdvantage() const override;
    virtual bool ConnectRope(UAdvancedRopeComponent* Rope) override;

    UFUNCTION(BlueprintCallable, Category = "Pulley Operations")
    float CalculateFrictionLoss() const;

    UFUNCTION(BlueprintCallable, Category = "Pulley Operations")
    void UpdateRopeDirection();

protected:
    virtual void CalculateCurrentLoad() override;
    
    // Pulley physics
    void CalculatePulleyPhysics();
    float GetEffectiveRadius() const;

private:
    // Pulley state
    float WheelRotation = 0.0f;
    float AngularVelocity = 0.0f;
    FVector RopeInputDirection = FVector::ZeroVector;
    FVector RopeOutputDirection = FVector::ZeroVector;
};

// Grappling Hook implementation
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AGrapplingHook : public AClimbingToolBase
{
    GENERATED_BODY()

public:
    AGrapplingHook();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Configuration")
    float ThrowDistance = 20.0f; // meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Configuration")
    float HookStrength = 15.0f; // kN

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Configuration")
    int32 NumberOfHooks = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Configuration")
    float MinimumHoldStrength = 5.0f; // kN

    // Grappling hook operations
    UFUNCTION(BlueprintCallable, Category = "Grappling Operations")
    void ThrowHook(const FVector& TargetLocation, float ThrowForce);

    UFUNCTION(BlueprintCallable, Category = "Grappling Operations")
    bool TestHookHold() const;

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerThrowHook(const FVector& TargetLocation, float ThrowForce);

    virtual void ActivateTool() override;
    virtual void DeactivateTool() override;

protected:
    virtual void CalculateCurrentLoad() override;
    
    // Hook physics
    void UpdateProjectilePhysics(float DeltaTime);
    bool CheckForHookContact(const FVector& Location);
    void EstablishHookHold(const FHitResult& HitResult);

private:
    // Grappling state
    bool bIsThrown = false;
    bool bIsHooked = false;
    FVector ThrowVelocity = FVector::ZeroVector;
    AActor* HookedSurface = nullptr;
    TArray<FVector> HookContactPoints;
};

UCLASS(BlueprintType, Blueprintable)  
class CLIMBINGGAME_API ACarabinerTool : public AClimbingToolBase
{
    GENERATED_BODY()

public:
    ACarabinerTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carabiner Configuration")
    bool bIsLocking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carabiner Configuration")
    bool bGateOpen = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carabiner Configuration")
    float GateSpringTension = 50.0f; // Newtons

    // Gate operations
    UFUNCTION(BlueprintCallable, Category = "Carabiner Operations")
    void OpenGate();

    UFUNCTION(BlueprintCallable, Category = "Carabiner Operations")
    void CloseGate();

    UFUNCTION(BlueprintCallable, Category = "Carabiner Operations")
    void TogateLock();

    virtual bool ConnectRope(UAdvancedRopeComponent* Rope) override;

protected:
    virtual void CalculateCurrentLoad() override;
    virtual float GetRemainingStrength() const override;

private:
    // Gate wear tracking
    int32 GateOperations = 0;
    float GateWear = 0.0f;
};