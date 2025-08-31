#include "ClimbingToolBase.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

// Base Tool Implementation
AClimbingToolBase::AClimbingToolBase()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f; // 10 FPS for performance
    bReplicates = true;
    SetReplicateMovement(true);

    // Create mesh component
    ToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolMesh"));
    RootComponent = ToolMesh;
    ToolMesh->SetSimulatePhysics(true);
    ToolMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ToolMesh->SetCollisionResponseToAllChannels(ECR_Block);

    // Create interaction sphere
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(ToolMesh);
    InteractionSphere->SetSphereRadius(50.0f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Create physics constraint for load calculations
    LoadConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LoadConstraint"));
    LoadConstraint->SetupAttachment(ToolMesh);

    // Initialize properties
    Properties.MajorAxisStrength = 25.0f;
    Properties.MinorAxisStrength = 8.0f;
    Properties.GateOpenStrength = 7.0f;
    Properties.Weight = 0.05f;
    Properties.MaxLoadAngle = 30.0f;
    Properties.MaxCycles = 10000.0f;
    Properties.CorrosionResistance = 0.95f;
    Properties.WearResistance = 0.9f;
}

void AClimbingToolBase::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize load data
    LoadData.CurrentLoad = 0.0f;
    LoadData.MaxLoadExperienced = 0.0f;
    LoadData.LoadDirection = FVector::ZeroVector;
    LoadData.LoadCycles = 0;
    LoadData.TotalWear = 0.0f;
}

void AClimbingToolBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() == ROLE_Authority)
    {
        UpdateLoadCalculations(DeltaTime);
        UpdateToolState();
        CheckFailureCondition();
        ProcessCorrosion(DeltaTime);
        ProcessWear(DeltaTime);
    }
}

void AClimbingToolBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AClimbingToolBase, CurrentState);
    DOREPLIFETIME(AClimbingToolBase, LoadData);
    DOREPLIFETIME(AClimbingToolBase, ConnectedRopes);
    DOREPLIFETIME(AClimbingToolBase, MaxRopeConnections);
}

bool AClimbingToolBase::CanPlaceAt(const FVector& Location, const FVector& SurfaceNormal) const
{
    // Basic placement validation
    if (SurfaceNormal.IsZero())
        return false;

    // Check if surface is within valid angle range
    float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(SurfaceNormal, FVector::UpVector)));
    
    // Most tools require relatively vertical surfaces (within 30 degrees of vertical)
    if (SurfaceAngle > 60.0f && SurfaceAngle < 120.0f)
        return true;

    return false;
}

bool AClimbingToolBase::PlaceToolAt(const FVector& Location, const FVector& SurfaceNormal)
{
    if (!CanPlaceAt(Location, SurfaceNormal))
        return false;

    if (GetLocalRole() < ROLE_Authority)
    {
        ServerPlaceTool(Location, SurfaceNormal);
        return true;
    }

    // Set tool position and orientation
    SetActorLocation(Location);
    FRotator SurfaceRotation = UKismetMathLibrary::MakeRotFromZ(SurfaceNormal);
    SetActorRotation(SurfaceRotation);

    // Attach to surface if possible
    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Location, Location - SurfaceNormal * 100.0f, ECC_WorldStatic))
    {
        if (HitResult.GetActor())
        {
            ToolMesh->AttachToComponent(HitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform);
        }
    }

    bIsPlaced = true;
    PlacementLocation = Location;
    PlacementNormal = SurfaceNormal;
    CurrentState = EToolState::Idle;

    OnToolPlaced.Broadcast();
    return true;
}

void AClimbingToolBase::ServerPlaceTool_Implementation(const FVector& Location, const FVector& SurfaceNormal)
{
    PlaceToolAt(Location, SurfaceNormal);
}

bool AClimbingToolBase::ServerPlaceTool_Validate(const FVector& Location, const FVector& SurfaceNormal)
{
    return !Location.IsZero() && !SurfaceNormal.IsZero();
}

bool AClimbingToolBase::CanConnectRope(UAdvancedRopeComponent* Rope) const
{
    if (!Rope || !bIsPlaced)
        return false;

    return ConnectedRopes.Num() < MaxRopeConnections;
}

bool AClimbingToolBase::ConnectRope(UAdvancedRopeComponent* Rope)
{
    if (!CanConnectRope(Rope))
        return false;

    if (GetLocalRole() < ROLE_Authority)
    {
        ServerConnectRope(Rope);
        return true;
    }

    ConnectedRopes.Add(Rope);
    
    // Establish physical connection
    if (Rope->AttachToAnchor(this))
    {
        OnRopeConnected.Broadcast();
        return true;
    }

    // Remove from list if physical connection failed
    ConnectedRopes.Remove(Rope);
    return false;
}

void AClimbingToolBase::DisconnectRope(UAdvancedRopeComponent* Rope)
{
    if (GetLocalRole() < ROLE_Authority)
    {
        ServerDisconnectRope(Rope);
        return;
    }

    ConnectedRopes.Remove(Rope);
    OnRopeDisconnected.Broadcast();
}

void AClimbingToolBase::ServerConnectRope_Implementation(UAdvancedRopeComponent* Rope)
{
    ConnectRope(Rope);
}

bool AClimbingToolBase::ServerConnectRope_Validate(UAdvancedRopeComponent* Rope)
{
    return Rope != nullptr;
}

void AClimbingToolBase::ServerDisconnectRope_Implementation(UAdvancedRopeComponent* Rope)
{
    DisconnectRope(Rope);
}

bool AClimbingToolBase::ServerDisconnectRope_Validate(UAdvancedRopeComponent* Rope)
{
    return Rope != nullptr;
}

void AClimbingToolBase::UpdateLoadCalculations(float DeltaTime)
{
    LastLoadUpdate += DeltaTime;
    if (LastLoadUpdate >= LoadUpdateInterval)
    {
        CalculateCurrentLoad();
        LastLoadUpdate = 0.0f;
    }
}

bool AClimbingToolBase::IsOverloaded() const
{
    float SafeWorkingLoad = Properties.MajorAxisStrength * 1000.0f * 0.25f; // 25% of breaking strength
    return LoadData.CurrentLoad > SafeWorkingLoad;
}

void AClimbingToolBase::ApplyWear(float WearAmount)
{
    LoadData.TotalWear += WearAmount * (1.0f - Properties.WearResistance);
    LoadData.TotalWear = FMath::Clamp(LoadData.TotalWear, 0.0f, 1.0f);
}

float AClimbingToolBase::GetRemainingStrength() const
{
    // Strength reduction from wear
    float WearFactor = 1.0f - (LoadData.TotalWear * 0.5f); // 50% strength loss at max wear
    
    // Strength reduction from load cycles
    float CycleFactor = 1.0f - (LoadData.LoadCycles / Properties.MaxCycles) * 0.3f; // 30% reduction at max cycles
    
    return FMath::Max(0.1f, WearFactor * CycleFactor);
}

bool AClimbingToolBase::ShouldReplace() const
{
    // Replace if strength drops below 70%
    if (GetRemainingStrength() < 0.7f)
        return true;

    // Replace if exceeded maximum load cycles
    if (LoadData.LoadCycles > Properties.MaxCycles)
        return true;

    // Replace if experienced load over 80% of breaking strength
    float CriticalLoad = Properties.MajorAxisStrength * 1000.0f * 0.8f;
    if (LoadData.MaxLoadExperienced > CriticalLoad)
        return true;

    return false;
}

void AClimbingToolBase::CalculateCurrentLoad()
{
    float TotalLoad = 0.0f;
    FVector LoadDirection = FVector::ZeroVector;

    // Calculate load from all connected ropes
    for (UAdvancedRopeComponent* Rope : ConnectedRopes)
    {
        if (Rope)
        {
            float RopeTension = Rope->CalculateCurrentTension();
            TotalLoad += RopeTension;
            
            // TODO: Calculate proper load direction based on rope attachment points
            LoadDirection += GetActorForwardVector(); // Placeholder
        }
    }

    // Apply angle derating if load is off-axis
    if (!LoadDirection.IsZero())
    {
        LoadDirection.Normalize();
        float AngleDeratingFactor = CalculateAngleDerating(LoadDirection);
        TotalLoad *= AngleDeratingFactor;
    }

    // Update load data
    LoadData.CurrentLoad = TotalLoad;
    LoadData.LoadDirection = LoadDirection;

    if (TotalLoad > LoadData.MaxLoadExperienced)
    {
        LoadData.MaxLoadExperienced = TotalLoad;
    }

    // Increment load cycles for significant loads
    if (TotalLoad > Properties.MajorAxisStrength * 1000.0f * 0.1f) // 10% of breaking strength
    {
        LoadData.LoadCycles++;
    }
}

void AClimbingToolBase::UpdateToolState()
{
    if (CurrentState == EToolState::Broken)
        return;

    float SafeWorkingLoad = Properties.MajorAxisStrength * 1000.0f * 0.25f;
    float OverloadThreshold = Properties.MajorAxisStrength * 1000.0f * 0.8f;

    if (LoadData.CurrentLoad > OverloadThreshold)
    {
        CurrentState = EToolState::Overloaded;
        OnOverloaded.Broadcast();
    }
    else if (LoadData.CurrentLoad > SafeWorkingLoad)
    {
        CurrentState = EToolState::UnderLoad;
    }
    else if (ConnectedRopes.Num() > 0)
    {
        CurrentState = EToolState::InUse;
    }
    else
    {
        CurrentState = EToolState::Idle;
    }

    // Check for damage
    if (LoadData.TotalWear > 0.8f)
    {
        CurrentState = EToolState::Damaged;
    }
}

void AClimbingToolBase::CheckFailureCondition()
{
    float BreakingLoad = Properties.MajorAxisStrength * 1000.0f * GetRemainingStrength();

    if (LoadData.CurrentLoad > BreakingLoad)
    {
        CurrentState = EToolState::Broken;
        OnToolFailure.Broadcast();
        
        // Disconnect all ropes
        for (UAdvancedRopeComponent* Rope : ConnectedRopes)
        {
            if (Rope)
            {
                Rope->DetachFromAnchor();
            }
        }
        ConnectedRopes.Empty();

        UE_LOG(LogTemp, Error, TEXT("Tool failure: %s broke under load of %f N"), *GetName(), LoadData.CurrentLoad);
    }
}

FVector AClimbingToolBase::CalculateLoadVector() const
{
    FVector TotalLoadVector = FVector::ZeroVector;

    for (const UAdvancedRopeComponent* Rope : ConnectedRopes)
    {
        if (Rope)
        {
            // Calculate load vector contribution from each rope
            // This is a simplified calculation - in reality would need rope attachment points
            FVector RopeDirection = (Rope->GetOwner()->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            float RopeTension = Rope->CalculateCurrentTension();
            TotalLoadVector += RopeDirection * RopeTension;
        }
    }

    return TotalLoadVector;
}

float AClimbingToolBase::CalculateAngleDerating(const FVector& LoadDirection) const
{
    FVector ToolAxis = GetActorForwardVector(); // Primary strength axis
    float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ToolAxis, LoadDirection)));

    if (Angle <= Properties.MaxLoadAngle)
    {
        return 1.0f; // Full strength
    }
    else if (Angle <= 90.0f)
    {
        // Linear derating from full strength to minor axis strength
        float DeratingFactor = FMath::GetMappedRangeValueClamped(
            FVector2D(Properties.MaxLoadAngle, 90.0f),
            FVector2D(1.0f, Properties.MinorAxisStrength / Properties.MajorAxisStrength),
            Angle
        );
        return DeratingFactor;
    }
    else
    {
        // Severely compromised strength for loads over 90 degrees
        return Properties.MinorAxisStrength / Properties.MajorAxisStrength * 0.5f;
    }
}

void AClimbingToolBase::ProcessCorrosion(float DeltaTime)
{
    // Simplified corrosion model - would be more complex with environmental factors
    float CorrosionRate = (1.0f - Properties.CorrosionResistance) * 0.0001f; // Very slow corrosion
    ApplyWear(CorrosionRate * DeltaTime);
}

void AClimbingToolBase::ProcessWear(float DeltaTime)
{
    // Wear from current load
    if (LoadData.CurrentLoad > 0.0f)
    {
        float LoadNormalized = LoadData.CurrentLoad / (Properties.MajorAxisStrength * 1000.0f);
        float WearRate = LoadNormalized * LoadNormalized * 0.001f; // Quadratic wear with load
        ApplyWear(WearRate * DeltaTime);
    }
}

// Anchor Tool Implementation
AAnchorTool::AAnchorTool()
{
    ToolType = EToolType::Anchor;
    
    // Anchor-specific properties
    Properties.MajorAxisStrength = 25.0f; // 25 kN typical for expansion bolt
    Properties.MinorAxisStrength = 8.0f;  // Cross-loading strength
    Properties.Weight = 0.1f; // 100g
    Properties.MaxLoadAngle = 15.0f; // Anchors are sensitive to angle loading
    MaxRopeConnections = 2; // Can belay and have backup
}

bool AAnchorTool::CanPlaceAt(const FVector& Location, const FVector& SurfaceNormal) const
{
    if (!Super::CanPlaceAt(Location, SurfaceNormal))
        return false;

    // Check rock quality at placement location
    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Location, Location - SurfaceNormal * PlacementDepth, ECC_WorldStatic))
    {
        // Simulate rock hardness testing
        if (HitResult.GetActor() && HitResult.GetActor()->ActorHasTag("SolidRock"))
        {
            return true;
        }
    }

    return false;
}

bool AAnchorTool::PlaceToolAt(const FVector& Location, const FVector& SurfaceNormal)
{
    if (!Super::PlaceToolAt(Location, SurfaceNormal))
        return false;

    // Calculate placement quality based on surface characteristics
    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Location, Location - SurfaceNormal * PlacementDepth, ECC_WorldStatic))
    {
        // Simulate rock quality assessment
        PlacementQuality = 1.0f; // Start with perfect placement
        
        if (HitResult.GetActor()->ActorHasTag("WeakRock"))
        {
            PlacementQuality *= 0.7f;
        }
        else if (HitResult.GetActor()->ActorHasTag("CrackedRock"))
        {
            PlacementQuality *= 0.8f;
        }
        
        RockQuality = PlacementQuality;
    }

    return true;
}

void AAnchorTool::CalculateCurrentLoad()
{
    Super::CalculateCurrentLoad();
    
    // Anchors have additional failure modes
    // Check for pull-out based on placement quality
    float PullOutResistance = Properties.MajorAxisStrength * 1000.0f * PlacementQuality;
    
    if (LoadData.CurrentLoad > PullOutResistance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Anchor showing signs of potential pull-out"));
    }
}

float AAnchorTool::SimulatePullOutTest() const
{
    // Simulate pull-out test strength
    return Properties.MajorAxisStrength * PlacementQuality * RockQuality;
}

// Pulley Tool Implementation
APulleyTool::APulleyTool()
{
    ToolType = EToolType::Pulley;
    
    // Pulley-specific properties
    Properties.MajorAxisStrength = 30.0f; // 30 kN
    Properties.MinorAxisStrength = 10.0f;
    Properties.Weight = 0.15f; // 150g
    Properties.MaxLoadAngle = 45.0f; // More forgiving angle loading
    MaxRopeConnections = 2; // Input and output rope segments
}

float APulleyTool::GetMechanicalAdvantage() const
{
    // Simple pulley provides direction change but no mechanical advantage
    if (NumberOfSheaves == 1)
    {
        return 1.0f - CalculateFrictionLoss();
    }
    
    // Multiple sheave systems can provide mechanical advantage
    // This is simplified - real systems are more complex
    float TheoreticalAdvantage = static_cast<float>(NumberOfSheaves);
    float EfficiencyLoss = FMath::Pow(BearingEfficiency, NumberOfSheaves);
    
    return TheoreticalAdvantage * EfficiencyLoss;
}

bool APulleyTool::ConnectRope(UAdvancedRopeComponent* Rope)
{
    if (!Super::ConnectRope(Rope))
        return false;

    // Update rope routing through pulley
    UpdateRopeDirection();
    return true;
}

float APulleyTool::CalculateFrictionLoss() const
{
    // Bearing friction and rope bend radius effects
    float BendRadius = WheelDiameter / 2.0f;
    float FrictionCoefficient = 1.0f - BearingEfficiency;
    
    // Simplified friction calculation
    return FrictionCoefficient * (1.0f / BendRadius) * 0.1f;
}

void APulleyTool::UpdateRopeDirection()
{
    // Update rope direction vectors based on pulley orientation
    FVector PulleyAxis = GetActorRightVector(); // Axis of rotation
    
    // Calculate input and output rope directions
    // This would typically involve more complex geometry
    RopeInputDirection = GetActorForwardVector();
    RopeOutputDirection = -GetActorForwardVector(); // Opposite direction for 180-degree redirect
}

void APulleyTool::CalculateCurrentLoad()
{
    Super::CalculateCurrentLoad();
    
    // Additional calculations for pulley physics
    CalculatePulleyPhysics();
}

void APulleyTool::CalculatePulleyPhysics()
{
    // Calculate wheel rotation based on rope movement
    float TotalRopeTension = 0.0f;
    
    for (const UAdvancedRopeComponent* Rope : ConnectedRopes)
    {
        if (Rope)
        {
            TotalRopeTension += Rope->CalculateCurrentTension();
        }
    }
    
    // Simplified rotational physics
    float WheelTorque = TotalRopeTension * GetEffectiveRadius();
    AngularVelocity = WheelTorque / (Properties.Weight * GetEffectiveRadius() * GetEffectiveRadius());
    WheelRotation += AngularVelocity * GetWorld()->GetDeltaSeconds();
}

float APulleyTool::GetEffectiveRadius() const
{
    return (WheelDiameter / 2.0f) * 0.01f; // Convert cm to meters
}

// Grappling Hook Implementation
AGrapplingHook::AGrapplingHook()
{
    ToolType = EToolType::GrapplingHook;
    
    // Grappling hook properties
    Properties.MajorAxisStrength = 15.0f; // 15 kN
    Properties.MinorAxisStrength = 5.0f;
    Properties.Weight = 0.3f; // 300g
    Properties.MaxLoadAngle = 60.0f; // More flexible loading
    MaxRopeConnections = 1; // Only one rope connection
}

void AGrapplingHook::ThrowHook(const FVector& TargetLocation, float ThrowForce)
{
    if (GetLocalRole() < ROLE_Authority)
    {
        ServerThrowHook(TargetLocation, ThrowForce);
        return;
    }

    FVector ThrowDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();
    ThrowVelocity = ThrowDirection * ThrowForce;
    
    bIsThrown = true;
    bIsHooked = false;
    CurrentState = EToolState::InUse;

    // Detach from any surface
    ToolMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    ToolMesh->SetSimulatePhysics(true);
    ToolMesh->AddImpulse(ThrowVelocity, NAME_None, true);
}

void AGrapplingHook::ServerThrowHook_Implementation(const FVector& TargetLocation, float ThrowForce)
{
    ThrowHook(TargetLocation, ThrowForce);
}

bool AGrapplingHook::ServerThrowHook_Validate(const FVector& TargetLocation, float ThrowForce)
{
    return ThrowForce > 0.0f && ThrowForce < 2000.0f; // Reasonable throw force limits
}

bool AGrapplingHook::TestHookHold() const
{
    if (!bIsHooked || !HookedSurface)
        return false;

    // Test based on hook contact points and surface type
    float HoldStrength = 0.0f;
    
    for (const FVector& ContactPoint : HookContactPoints)
    {
        // Each hook contributes to hold strength
        HoldStrength += MinimumHoldStrength / NumberOfHooks;
    }

    return HoldStrength >= MinimumHoldStrength;
}

void AGrapplingHook::ActivateTool()
{
    // Prepare for throwing
    CurrentState = EToolState::Idle;
}

void AGrapplingHook::DeactivateTool()
{
    // Retract or reset hook
    if (bIsHooked)
    {
        bIsHooked = false;
        HookedSurface = nullptr;
        HookContactPoints.Empty();
    }
    
    if (bIsThrown)
    {
        bIsThrown = false;
        ThrowVelocity = FVector::ZeroVector;
    }

    CurrentState = EToolState::Idle;
}

void AGrapplingHook::CalculateCurrentLoad()
{
    if (!bIsHooked)
    {
        LoadData.CurrentLoad = 0.0f;
        return;
    }

    Super::CalculateCurrentLoad();
    
    // Additional check for hook pull-out
    if (LoadData.CurrentLoad > MinimumHoldStrength * 1000.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grappling hook under excessive load - risk of pull-out"));
        
        if (LoadData.CurrentLoad > HookStrength * 1000.0f)
        {
            // Hook failure - release from surface
            DeactivateTool();
            CurrentState = EToolState::Broken;
        }
    }
}

void AGrapplingHook::UpdateProjectilePhysics(float DeltaTime)
{
    if (!bIsThrown || bIsHooked)
        return;

    // Check for contact with surfaces during flight
    FVector CurrentLocation = GetActorLocation();
    
    if (CheckForHookContact(CurrentLocation))
    {
        bIsThrown = false;
        // Hook establishment is handled in CheckForHookContact
    }
}

bool AGrapplingHook::CheckForHookContact(const FVector& Location)
{
    // Perform sphere trace to check for hookable surfaces
    FHitResult HitResult;
    if (GetWorld()->SweepSingleByChannel(HitResult, GetActorLocation(), Location,
        FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(10.0f)))
    {
        if (HitResult.GetActor() && HitResult.GetActor()->ActorHasTag("Hookable"))
        {
            EstablishHookHold(HitResult);
            return true;
        }
    }
    
    return false;
}

void AGrapplingHook::EstablishHookHold(const FHitResult& HitResult)
{
    bIsHooked = true;
    HookedSurface = HitResult.GetActor();
    
    // Calculate hook contact points
    HookContactPoints.Empty();
    FVector HookLocation = HitResult.Location;
    
    // Simulate multiple hook points
    for (int32 i = 0; i < NumberOfHooks; ++i)
    {
        FVector Offset = FMath::VRand() * 5.0f; // 5cm random spread
        HookContactPoints.Add(HookLocation + Offset);
    }

    // Attach to surface
    ToolMesh->SetSimulatePhysics(false);
    SetActorLocation(HookLocation);
    
    if (HitResult.GetComponent())
    {
        ToolMesh->AttachToComponent(HitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform);
    }

    CurrentState = EToolState::InUse;
    OnToolPlaced.Broadcast();
}

// Carabiner Tool Implementation
ACarabinerTool::ACarabinerTool()
{
    ToolType = EToolType::Carabiner;
    
    // Carabiner properties
    Properties.MajorAxisStrength = 25.0f; // 25 kN
    Properties.MinorAxisStrength = 8.0f;
    Properties.GateOpenStrength = 7.0f; // Strength when gate is open
    Properties.Weight = 0.05f; // 50g
    Properties.MaxLoadAngle = 30.0f;
    MaxRopeConnections = 2; // Two rope segments through carabiner
}

void ACarabinerTool::OpenGate()
{
    if (!bIsLocking || !bGateOpen)
    {
        bGateOpen = true;
        GateOperations++;
        
        // Gate operations cause wear
        GateWear += 0.0001f; // Small wear per operation
    }
}

void ACarabinerTool::CloseGate()
{
    bGateOpen = false;
    GateOperations++;
    GateWear += 0.0001f;
}

void ACarabinerTool::TogateLock()
{
    if (bIsLocking)
    {
        // Toggle lock mechanism
        if (bGateOpen)
        {
            CloseGate();
        }
        else
        {
            OpenGate();
        }
    }
}

bool ACarabinerTool::ConnectRope(UAdvancedRopeComponent* Rope)
{
    // Can only connect rope if gate is open (for realistic operation)
    if (bGateOpen || !bIsLocking)
    {
        return Super::ConnectRope(Rope);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Cannot connect rope to carabiner - gate is closed"));
    return false;
}

void ACarabinerTool::CalculateCurrentLoad()
{
    Super::CalculateCurrentLoad();
    
    // Reduced strength when gate is open
    if (bGateOpen)
    {
        LoadData.CurrentLoad *= Properties.GateOpenStrength / Properties.MajorAxisStrength;
    }
}

float ACarabinerTool::GetRemainingStrength() const
{
    float BaseStrength = Super::GetRemainingStrength();
    
    // Reduce strength based on gate wear
    float GateWearFactor = 1.0f - (GateWear * 0.3f); // Up to 30% strength loss from gate wear
    
    // Reduce strength from excessive gate operations
    float OperationsFactor = 1.0f - (GateOperations / 50000.0f) * 0.2f; // 20% loss after 50k operations
    
    return BaseStrength * GateWearFactor * OperationsFactor;
}