#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "../Tools/ClimbingToolBase.h"
#include "AdvancedRopeComponent.h"
#include "AdvancedToolPhysics.generated.h"

UENUM(BlueprintType)
enum class EPulleySystemType : uint8
{
    Simple          UMETA(DisplayName = "Simple Pulley"),
    Compound        UMETA(DisplayName = "Compound Pulley"),
    Fixed           UMETA(DisplayName = "Fixed Pulley"),
    Movable         UMETA(DisplayName = "Movable Pulley"),
    BlockAndTackle  UMETA(DisplayName = "Block and Tackle"),
    ZPulley         UMETA(DisplayName = "Z-Pulley System"),
    Haul            UMETA(DisplayName = "Haul System")
};

UENUM(BlueprintType)
enum class EGrapplingHookState : uint8
{
    Stowed          UMETA(DisplayName = "Stowed"),
    Thrown          UMETA(DisplayName = "In Flight"),
    Hooked          UMETA(DisplayName = "Hooked"),
    Loaded          UMETA(DisplayName = "Under Load"),
    Slipping        UMETA(DisplayName = "Slipping"),
    Failed          UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EToolInteractionType : uint8
{
    Direct          UMETA(DisplayName = "Direct Contact"),
    Rope            UMETA(DisplayName = "Rope Connection"),
    Carabiner       UMETA(DisplayName = "Carabiner Link"),
    Sling           UMETA(DisplayName = "Sling Connection"),
    Chain           UMETA(DisplayName = "Chain Link"),
    Magnetic        UMETA(DisplayName = "Magnetic Connection")
};

USTRUCT(BlueprintType)
struct FPulleySystemState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EPulleySystemType SystemType = EPulleySystemType::Simple;

    UPROPERTY(BlueprintReadOnly)
    float MechanicalAdvantage = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    float SystemEfficiency = 0.95f;

    UPROPERTY(BlueprintReadOnly)
    float InputForce = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly)
    float OutputForce = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly)
    float InputVelocity = 0.0f; // m/s

    UPROPERTY(BlueprintReadOnly)
    float OutputVelocity = 0.0f; // m/s

    UPROPERTY(BlueprintReadOnly)
    float RopeLength = 0.0f; // meters

    UPROPERTY(BlueprintReadOnly)
    int32 NumberOfPulleys = 1;

    UPROPERTY(BlueprintReadOnly)
    TArray<APulleyTool*> ConnectedPulleys;

    UPROPERTY(BlueprintReadOnly)
    bool bSystemIntact = true;

    UPROPERTY(BlueprintReadOnly)
    float FrictionLoss = 0.0f; // Energy lost to friction
};

USTRUCT(BlueprintType)
struct FGrapplingHookData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EGrapplingHookState State = EGrapplingHookState::Stowed;

    UPROPERTY(BlueprintReadOnly)
    FVector ThrowVelocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector HookLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector HoldDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float HoldStrength = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly)
    float SlipThreshold = 0.0f; // Force at which hook starts to slip

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> ContactPoints;

    UPROPERTY(BlueprintReadOnly)
    AActor* HookedSurface = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float TimeHooked = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsSecure = false;
};

USTRUCT(BlueprintType)
struct FToolInteraction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingToolBase* ToolA = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AClimbingToolBase* ToolB = nullptr;

    UPROPERTY(BlueprintReadOnly)
    EToolInteractionType InteractionType = EToolInteractionType::Direct;

    UPROPERTY(BlueprintReadOnly)
    float ConnectionStrength = 0.0f; // Newtons

    UPROPERTY(BlueprintReadOnly)
    FVector ContactPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector LoadDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float TransmittedForce = 0.0f; // Force transmitted through connection

    UPROPERTY(BlueprintReadOnly)
    bool bConnectionActive = true;

    UPROPERTY(BlueprintReadOnly)
    float ConnectionAge = 0.0f; // Time since connection established
};

USTRUCT(BlueprintType)
struct FAdvancedToolSettings
{
    GENERATED_BODY()

    // Pulley system settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Systems")
    float PulleyEfficiency = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Systems")
    float BearingFriction = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Systems")
    float RopeFriction = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulley Systems")
    float MaxPulleyLoad = 30000.0f; // Newtons

    // Grappling hook settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Hook")
    float ThrowForceMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Hook")
    float HookPenetration = 2.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Hook")
    float SlipFactorRock = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Hook")
    float SlipFactorIce = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling Hook")
    float SlipFactorDirt = 0.5f;

    // Tool interaction settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Interactions")
    float ConnectionWearRate = 0.0001f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Interactions")
    float MaxConnectionDistance = 5.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Interactions")
    bool bEnableToolMagnetism = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Interactions")
    float MagneticForceStrength = 100.0f; // Newtons

    // Physics simulation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float SimulationFrequency = 60.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    int32 MaxIterations = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float DampingFactor = 0.98f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    bool bEnableAdvancedPhysics = true;
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UAdvancedToolPhysics : public UActorComponent
{
    GENERATED_BODY()

public:
    UAdvancedToolPhysics();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Tool Physics")
    FAdvancedToolSettings Settings;

    // System states
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Pulley System")
    FPulleySystemState PulleySystem;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Grappling Hook")
    FGrapplingHookData GrapplingHook;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Tool Interactions")
    TArray<FToolInteraction> ActiveInteractions;

    // Pulley system management
    UFUNCTION(BlueprintCallable, Category = "Pulley System")
    void SetupPulleySystem(EPulleySystemType SystemType, const TArray<APulleyTool*>& Pulleys);

    UFUNCTION(BlueprintCallable, Category = "Pulley System")
    float CalculatePulleyMechanicalAdvantage() const;

    UFUNCTION(BlueprintCallable, Category = "Pulley System")
    void UpdatePulleyPhysics(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Pulley System")
    FVector CalculatePulleyForces(const FVector& InputForce) const;

    UFUNCTION(BlueprintCallable, Category = "Pulley System")
    void OptimizePulleyConfiguration();

    // Grappling hook physics
    UFUNCTION(BlueprintCallable, Category = "Grappling Hook")
    void ThrowGrapplingHook(const FVector& TargetLocation, float ThrowForce);

    UFUNCTION(BlueprintCallable, Category = "Grappling Hook")
    void UpdateGrapplingHookPhysics(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Grappling Hook")
    bool TestHookHold(float AppliedForce) const;

    UFUNCTION(BlueprintCallable, Category = "Grappling Hook")
    void SimulateHookPenetration(const FHitResult& HitResult);

    UFUNCTION(BlueprintCallable, Category = "Grappling Hook")
    float CalculateHookSlipThreshold(const AActor* Surface) const;

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerThrowGrapplingHook(const FVector& TargetLocation, float ThrowForce);

    // Tool interactions
    UFUNCTION(BlueprintCallable, Category = "Tool Interactions")
    bool EstablishToolConnection(AClimbingToolBase* ToolA, AClimbingToolBase* ToolB, EToolInteractionType InteractionType);

    UFUNCTION(BlueprintCallable, Category = "Tool Interactions")
    void BreakToolConnection(AClimbingToolBase* ToolA, AClimbingToolBase* ToolB);

    UFUNCTION(BlueprintCallable, Category = "Tool Interactions")
    void UpdateToolInteractions(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Tool Interactions")
    TArray<AClimbingToolBase*> FindNearbyTools(float SearchRadius) const;

    UFUNCTION(BlueprintCallable, Category = "Tool Interactions")
    float CalculateConnectionStrength(const FToolInteraction& Interaction) const;

    // Advanced physics simulations
    UFUNCTION(BlueprintCallable, Category = "Advanced Physics")
    void SimulateToolChainPhysics(const TArray<AClimbingToolBase*>& ToolChain);

    UFUNCTION(BlueprintCallable, Category = "Advanced Physics")
    void ApplyMagneticForces(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Advanced Physics")
    void CalculateComplexLoadDistribution();

    UFUNCTION(BlueprintCallable, Category = "Advanced Physics")
    FVector SimulateToolFailurePropagation(AClimbingToolBase* FailedTool) const;

    // Specialized tool physics
    UFUNCTION(BlueprintCallable, Category = "Specialized Tools")
    void UpdateAscenderPhysics(AClimbingToolBase* Ascender, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Specialized Tools")
    void UpdateDescenderPhysics(AClimbingToolBase* Descender, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Specialized Tools")
    void UpdateBelayDevicePhysics(AClimbingToolBase* BelayDevice, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Specialized Tools")
    void SimulateQuickdrawPhysics(AClimbingToolBase* Quickdraw, float DeltaTime);

    // Analysis and optimization
    UFUNCTION(BlueprintCallable, Category = "Analysis")
    float AnalyzeSystemEfficiency() const;

    UFUNCTION(BlueprintCallable, Category = "Analysis")
    TArray<FVector> PredictToolFailureEffects() const;

    UFUNCTION(BlueprintCallable, Category = "Analysis")
    float CalculateSystemComplexity() const;

    UFUNCTION(BlueprintCallable, Category = "Analysis")
    void GeneratePhysicsReport(FString& ReportText) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnPulleySystemEstablished;

    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnGrapplingHookThrown;

    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnGrapplingHookEngaged;

    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnGrapplingHookSlipped;

    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnToolConnectionEstablished;

    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnToolConnectionBroken;

    UPROPERTY(BlueprintAssignable, Category = "Tool Physics Events")
    FSimpleMulticastDelegate OnSystemOverloaded;

protected:
    // Internal physics calculations
    void CalculatePulleySystemForces();
    void UpdateGrapplingHookTrajectory(float DeltaTime);
    void ProcessToolInteractionForces(float DeltaTime);
    void SimulateSystemDynamics(float DeltaTime);

    // Pulley system specifics
    float CalculateCompoundPulleyAdvantage(const TArray<APulleyTool*>& Pulleys) const;
    float CalculateBlockAndTackleAdvantage(int32 NumberOfPulleys) const;
    float CalculateZPulleyAdvantage() const;
    void UpdatePulleyRotations(float DeltaTime);

    // Grappling hook specifics
    void UpdateHookFlightPhysics(float DeltaTime);
    bool CheckHookContact(const FVector& CurrentLocation);
    void EstablishHookConnection(const FHitResult& HitResult);
    void UpdateHookLoadPhysics(float DeltaTime);

    // Tool interaction specifics
    bool ValidateToolConnection(const FToolInteraction& Interaction) const;
    void ApplyConnectionWear(FToolInteraction& Interaction, float DeltaTime);
    void TransmitForceThroughConnection(FToolInteraction& Interaction);

    // Advanced calculations
    void SolveConstraintSystem();
    void ApplyVirtualWorkPrinciple();
    FVector CalculateSystemCenterOfMass() const;
    float CalculateSystemKineticEnergy() const;

private:
    // Performance optimization
    float LastPhysicsUpdate = 0.0f;
    float PhysicsUpdateInterval = 0.0167f; // 60 FPS
    
    // Cached calculations
    TMap<APulleyTool*, float> PulleyRotationCache;
    TArray<FVector> ForceVectorCache;
    
    // System tracking
    TArray<AClimbingToolBase*> TrackedTools;
    TMap<AClimbingToolBase*, float> ToolLoadHistory;
    
    // Grappling hook trajectory cache
    TArray<FVector> HookTrajectory;
    int32 TrajectoryIndex = 0;
    
    // Physics solver state
    TArray<FVector> ConstraintForces;
    TArray<FVector> VelocityCorrections;
    int32 SolverIterations = 0;
};