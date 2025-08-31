#include "AdvancedClimbingComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Net/UnrealNetwork.h"

// FGripPoint Implementation
float FGripPoint::GetRequiredGripStrength() const
{
    float BaseStrength = 30.0f; // Base grip strength requirement

    // Adjust based on grip type
    switch (Type)
    {
        case EGripType::Jug:
            BaseStrength *= 0.6f; // Jugs are easiest
            break;
        case EGripType::Crimp:
            BaseStrength *= 1.4f; // Crimps are hardest on fingers
            break;
        case EGripType::Sloper:
            BaseStrength *= 1.2f; // Slopers require more strength
            break;
        case EGripType::Pinch:
            BaseStrength *= 1.3f; // Pinches are difficult
            break;
        case EGripType::Pocket:
            BaseStrength *= 1.1f; // Moderate difficulty
            break;
        case EGripType::Mantle:
            BaseStrength *= 0.8f; // More about technique
            break;
    }

    // Adjust based on size (smaller holds require more strength)
    BaseStrength *= (1.5f - (Size * 0.5f));

    // Adjust based on quality
    BaseStrength *= (1.2f - (Quality * 0.2f));

    // Adjust based on grade
    float GradeDifficulty = static_cast<float>(RequiredGrade) / 35.0f; // Normalize to 0-1
    BaseStrength *= (1.0f + GradeDifficulty * 2.0f); // Up to 3x harder for higher grades

    return FMath::Clamp(BaseStrength, 10.0f, 200.0f);
}

float FGripPoint::GetStaminaDrainRate() const
{
    float BaseDrain = StaminaDrain;

    // Grip type affects stamina drain
    switch (Type)
    {
        case EGripType::Jug:
            BaseDrain *= 0.5f; // Rest holds
            break;
        case EGripType::Crimp:
            BaseDrain *= 2.0f; // Very taxing
            break;
        case EGripType::Sloper:
            BaseDrain *= 1.5f; // Require constant tension
            break;
        case EGripType::Pinch:
            BaseDrain *= 1.8f; // Thumb intensive
            break;
        case EGripType::Pocket:
            BaseDrain *= 1.3f; // Finger intensive
            break;
        case EGripType::Mantle:
            BaseDrain *= 1.0f; // Balanced
            break;
    }

    // Size affects drain (smaller holds are more taxing)
    BaseDrain *= (1.5f - (Size * 0.4f));

    // Quality affects drain (poor holds are more taxing)
    BaseDrain *= (1.3f - (Quality * 0.3f));

    return FMath::Max(0.1f, BaseDrain);
}

// UAdvancedClimbingComponent Implementation
UAdvancedClimbingComponent::UAdvancedClimbingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0167f; // 60 FPS
    SetIsReplicatedByDefault(true);

    // Set default movement properties for climbing
    MaxWalkSpeed = 200.0f;
    MaxAcceleration = 1024.0f;
    BrakingDecelerationWalking = 512.0f;
    GroundFriction = 0.5f;
    JumpZVelocity = 420.0f;

    // Initialize climbing settings
    Settings.BaseStaminaRegenRate = 5.0f;
    Settings.BaseStaminaDrainRate = 2.0f;
    Settings.CrimpDrainMultiplier = 2.0f;
    Settings.SloperDrainMultiplier = 1.5f;
    Settings.JugDrainMultiplier = 0.8f;
    Settings.GripRecoveryRate = 10.0f;
    Settings.GripFatigueRate = 3.0f;
    Settings.MinGripThreshold = 20.0f;
    Settings.ClimbingSpeed = 200.0f;
    Settings.ReachDistance = 120.0f;
    Settings.DynoForce = 1000.0f;
    Settings.FallDamageThreshold = 5.0f;
    Settings.MaxSurvivableFall = 20.0f;
    Settings.SwingDamping = 0.95f;

    // Initialize climbing state
    ClimbingState.CurrentStamina = 100.0f;
    ClimbingState.MaxStamina = 100.0f;
    ClimbingState.CurrentGripStrength = 100.0f;
    ClimbingState.MaxGripStrength = 100.0f;
    ClimbingState.CustomMovementMode = ECustomMovementMode::CMOVE_None;
}

void UAdvancedClimbingComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache the starting position as last valid climbing position
    LastValidClimbingPosition = UpdatedComponent->GetComponentLocation();
}

void UAdvancedClimbingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwnerRole() == ROLE_Authority)
    {
        UpdateClimbingPhysics(DeltaTime);
        UpdateStaminaAndGrip(DeltaTime);
        CheckFallCondition();
        
        if (ClimbingState.AttachedRope)
        {
            UpdateRopePhysics(DeltaTime);
        }
    }

    // Update timer
    ClimbingState.TimeSinceLastMove += DeltaTime;
}

void UAdvancedClimbingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAdvancedClimbingComponent, ClimbingState);
}

void UAdvancedClimbingComponent::PhysCustom(float deltaTime, int32 Iterations)
{
    Super::PhysCustom(deltaTime, Iterations);

    switch (static_cast<ECustomMovementMode>(CustomMovementMode))
    {
        case ECustomMovementMode::CMOVE_Climbing:
            PhysClimbing(deltaTime, Iterations);
            break;
        case ECustomMovementMode::CMOVE_Roped:
            PhysRoped(deltaTime, Iterations);
            break;
        case ECustomMovementMode::CMOVE_Anchored:
            PhysAnchored(deltaTime, Iterations);
            break;
        case ECustomMovementMode::CMOVE_Falling:
            PhysFalling(deltaTime, Iterations);
            break;
        case ECustomMovementMode::CMOVE_Swinging:
            PhysSwinging(deltaTime, Iterations);
            break;
        default:
            break;
    }
}

void UAdvancedClimbingComponent::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode)
{
    Super::OnMovementModeChanged(PrevMovementMode, PrevCustomMode);

    if (MovementMode == MOVE_Custom)
    {
        switch (static_cast<ECustomMovementMode>(CustomMovementMode))
        {
            case ECustomMovementMode::CMOVE_Climbing:
                OnStartClimbing.Broadcast();
                break;
            case ECustomMovementMode::CMOVE_Falling:
                OnFallStart.Broadcast();
                break;
        }
    }
    else if (PrevMovementMode == MOVE_Custom)
    {
        switch (static_cast<ECustomMovementMode>(PrevCustomMode))
        {
            case ECustomMovementMode::CMOVE_Climbing:
                OnStopClimbing.Broadcast();
                break;
        }
    }
}

bool UAdvancedClimbingComponent::TryStartClimbing()
{
    // Check if there are climbable surfaces nearby
    TArray<FGripPoint> NearbyGrips = FindNearbyGrips(Settings.ReachDistance);
    
    if (NearbyGrips.Num() == 0)
        return false;

    // Try to find suitable starting grips
    FGripPoint BestGrip;
    float BestScore = 0.0f;

    for (const FGripPoint& Grip : NearbyGrips)
    {
        float Score = Grip.Quality * Grip.Size;
        if (Score > BestScore && CanMaintainGrip(Grip))
        {
            BestScore = Score;
            BestGrip = Grip;
        }
    }

    if (BestScore > 0.0f)
    {
        // Start climbing
        SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::CMOVE_Climbing));
        ClimbingState.CustomMovementMode = ECustomMovementMode::CMOVE_Climbing;
        
        // Grab the best grip with both hands initially
        ClimbingState.LeftHandGrip = BestGrip;
        ClimbingState.RightHandGrip = BestGrip;
        ClimbingState.LeftHandGrip.bIsActive = true;
        ClimbingState.RightHandGrip.bIsActive = true;

        LastValidClimbingPosition = UpdatedComponent->GetComponentLocation();
        return true;
    }

    return false;
}

void UAdvancedClimbingComponent::StopClimbing()
{
    // Release all grips
    ClimbingState.LeftHandGrip.bIsActive = false;
    ClimbingState.RightHandGrip.bIsActive = false;
    ClimbingState.LeftFootGrip.bIsActive = false;
    ClimbingState.RightFootGrip.bIsActive = false;

    // Return to normal movement
    SetMovementMode(MOVE_Walking);
    ClimbingState.CustomMovementMode = ECustomMovementMode::CMOVE_None;
}

TArray<FGripPoint> UAdvancedClimbingComponent::FindNearbyGrips(float SearchRadius) const
{
    TArray<FGripPoint> GripPoints;

    // Use cached grips if recent
    if (GetWorld()->GetTimeSeconds() - LastGripScan < GripScanInterval)
    {
        return CachedNearbyGrips;
    }

    FVector StartLocation = UpdatedComponent->GetComponentLocation();
    
    // Perform sphere sweep to find climbing surfaces
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        StartLocation,
        StartLocation,
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeSphere(SearchRadius),
        QueryParams
    );

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            if (IsValidClimbingSurface(Hit))
            {
                FGripPoint NewGrip;
                NewGrip.Location = Hit.Location;
                NewGrip.Normal = Hit.Normal;
                NewGrip.Surface = Hit.GetActor();
                NewGrip.Type = DetermineGripType(Hit);
                NewGrip.Quality = FMath::RandRange(0.3f, 1.0f); // Randomized for now
                NewGrip.Size = FMath::RandRange(0.2f, 1.0f);
                
                // Determine required grade based on grip properties
                float DifficultyScore = (1.0f - NewGrip.Quality) + (1.0f - NewGrip.Size);
                NewGrip.RequiredGrade = static_cast<EClimbingDifficulty>(FMath::Clamp(
                    static_cast<int32>(DifficultyScore * 20.0f), 
                    0, 
                    static_cast<int32>(EClimbingDifficulty::Grade_5_15d)
                ));

                GripPoints.Add(NewGrip);
            }
        }
    }

    return GripPoints;
}

bool UAdvancedClimbingComponent::IsValidClimbingSurface(const FHitResult& Hit) const
{
    if (!Hit.GetActor())
        return false;

    // Check if tagged as climbable
    if (!Hit.GetActor()->ActorHasTag("Climbable"))
        return false;

    // Check surface angle - should be steep enough to require climbing
    float SurfaceAngle = AnalyzeSurfaceAngle(Hit.Normal);
    if (SurfaceAngle < 45.0f) // Too shallow
        return false;

    return true;
}

bool UAdvancedClimbingComponent::TryGrabHold(const FGripPoint& GripPoint, bool bIsLeftHand)
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerTryGrabHold(GripPoint, bIsLeftHand, true);
        return true;
    }

    // Check if we can maintain this grip
    if (!CanMaintainGrip(GripPoint))
        return false;

    // Check reach distance
    float Distance = FVector::Dist(UpdatedComponent->GetComponentLocation(), GripPoint.Location);
    if (Distance > Settings.ReachDistance)
        return false;

    // Assign the grip
    if (bIsLeftHand)
    {
        ClimbingState.LeftHandGrip = GripPoint;
        ClimbingState.LeftHandGrip.bIsActive = true;
    }
    else
    {
        ClimbingState.RightHandGrip = GripPoint;
        ClimbingState.RightHandGrip.bIsActive = true;
    }

    // Consume stamina for the grab
    float GrabCost = CalculateRequiredGripStrength(GripPoint) * 0.1f;
    ConsumeStamina(GrabCost);
    ConsumeGripStrength(GrabCost * 0.5f);

    OnGripAcquired.Broadcast();
    ClimbingState.TimeSinceLastMove = 0.0f;

    return true;
}

void UAdvancedClimbingComponent::ReleaseGrip(bool bIsLeftHand, bool bIsHand)
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerReleaseGrip(bIsLeftHand, bIsHand);
        return;
    }

    if (bIsHand)
    {
        if (bIsLeftHand)
        {
            ClimbingState.LeftHandGrip.bIsActive = false;
        }
        else
        {
            ClimbingState.RightHandGrip.bIsActive = false;
        }
    }
    else // Foot
    {
        if (bIsLeftHand) // Using same parameter for left/right foot
        {
            ClimbingState.LeftFootGrip.bIsActive = false;
        }
        else
        {
            ClimbingState.RightFootGrip.bIsActive = false;
        }
    }

    OnGripLost.Broadcast();
}

void UAdvancedClimbingComponent::ServerTryGrabHold_Implementation(const FGripPoint& GripPoint, bool bIsLeftHand, bool bIsHand)
{
    TryGrabHold(GripPoint, bIsLeftHand);
}

bool UAdvancedClimbingComponent::ServerTryGrabHold_Validate(const FGripPoint& GripPoint, bool bIsLeftHand, bool bIsHand)
{
    return GripPoint.Surface != nullptr && GripPoint.Quality > 0.0f;
}

void UAdvancedClimbingComponent::ServerReleaseGrip_Implementation(bool bIsLeftHand, bool bIsHand)
{
    ReleaseGrip(bIsLeftHand, bIsHand);
}

bool UAdvancedClimbingComponent::ServerReleaseGrip_Validate(bool bIsLeftHand, bool bIsHand)
{
    return true; // Always allow grip release
}

void UAdvancedClimbingComponent::ConsumeStamina(float Amount)
{
    ClimbingState.CurrentStamina = FMath::Max(0.0f, ClimbingState.CurrentStamina - Amount);
    
    if (ClimbingState.CurrentStamina < 20.0f)
    {
        OnStaminaLow.Broadcast();
    }
}

void UAdvancedClimbingComponent::RegenerateStamina(float DeltaTime)
{
    if (ClimbingState.bIsResting)
    {
        float RegenAmount = Settings.BaseStaminaRegenRate * DeltaTime;
        ClimbingState.CurrentStamina = FMath::Min(ClimbingState.MaxStamina, ClimbingState.CurrentStamina + RegenAmount);
    }
}

void UAdvancedClimbingComponent::ConsumeGripStrength(float Amount)
{
    ClimbingState.CurrentGripStrength = FMath::Max(0.0f, ClimbingState.CurrentGripStrength - Amount);
    
    if (ClimbingState.CurrentGripStrength < Settings.MinGripThreshold)
    {
        OnGripStrengthLow.Broadcast();
    }
}

void UAdvancedClimbingComponent::RegenerateGripStrength(float DeltaTime)
{
    if (ClimbingState.bIsResting)
    {
        float RegenAmount = Settings.GripRecoveryRate * DeltaTime;
        ClimbingState.CurrentGripStrength = FMath::Min(ClimbingState.MaxGripStrength, ClimbingState.CurrentGripStrength + RegenAmount);
    }
}

void UAdvancedClimbingComponent::SetCustomMovementMode(ECustomMovementMode NewMode)
{
    if (GetOwnerRole() < ROLE_Authority)
        return;

    ClimbingState.CustomMovementMode = NewMode;
    SetMovementMode(MOVE_Custom, static_cast<uint8>(NewMode));
}

void UAdvancedClimbingComponent::PerformDyno(const FVector& TargetLocation)
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerPerformDyno(TargetLocation);
        return;
    }

    // Check if we have enough stamina
    float DynoCost = 30.0f;
    if (ClimbingState.CurrentStamina < DynoCost)
        return;

    // Calculate dyno direction and force
    FVector DynoDirection = (TargetLocation - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
    FVector DynoVelocity = DynoDirection * Settings.DynoForce;

    // Apply the impulse
    Velocity = DynoVelocity;
    
    // Release current grips
    ClimbingState.LeftHandGrip.bIsActive = false;
    ClimbingState.RightHandGrip.bIsActive = false;
    ClimbingState.LeftFootGrip.bIsActive = false;
    ClimbingState.RightFootGrip.bIsActive = false;

    // Consume stamina
    ConsumeStamina(DynoCost);
    ConsumeGripStrength(20.0f);

    // Set movement mode to falling temporarily
    SetCustomMovementMode(ECustomMovementMode::CMOVE_Falling);
}

void UAdvancedClimbingComponent::ServerPerformDyno_Implementation(const FVector& TargetLocation)
{
    PerformDyno(TargetLocation);
}

bool UAdvancedClimbingComponent::ServerPerformDyno_Validate(const FVector& TargetLocation)
{
    // Validate reasonable dyno distance
    float Distance = FVector::Dist(UpdatedComponent->GetComponentLocation(), TargetLocation);
    return Distance <= Settings.ReachDistance * 2.0f; // Allow dynos up to 2x reach distance
}

void UAdvancedClimbingComponent::StartRest()
{
    // Can only rest on good holds
    bool CanRest = false;
    
    if (ClimbingState.LeftHandGrip.bIsActive && ClimbingState.LeftHandGrip.Type == EGripType::Jug)
        CanRest = true;
    if (ClimbingState.RightHandGrip.bIsActive && ClimbingState.RightHandGrip.Type == EGripType::Jug)
        CanRest = true;

    if (CanRest)
    {
        ClimbingState.bIsResting = true;
    }
}

void UAdvancedClimbingComponent::EndRest()
{
    ClimbingState.bIsResting = false;
}

bool UAdvancedClimbingComponent::AttachToRope(UAdvancedRopeComponent* Rope)
{
    if (!Rope)
        return false;

    ClimbingState.AttachedRope = Rope;
    return true;
}

void UAdvancedClimbingComponent::DetachFromRope()
{
    ClimbingState.AttachedRope = nullptr;
    ClimbingState.RopeSlackLength = 0.0f;
}

void UAdvancedClimbingComponent::AdjustRopeSlack(float SlackAmount)
{
    ClimbingState.RopeSlackLength += SlackAmount;
    ClimbingState.RopeSlackLength = FMath::Max(0.0f, ClimbingState.RopeSlackLength);
}

void UAdvancedClimbingComponent::StartFall()
{
    if (ClimbingState.bIsFalling)
        return;

    ClimbingState.bIsFalling = true;
    ClimbingState.FallStartLocation = UpdatedComponent->GetComponentLocation();
    ClimbingState.FallDistance = 0.0f;

    // Release all grips
    ClimbingState.LeftHandGrip.bIsActive = false;
    ClimbingState.RightHandGrip.bIsActive = false;
    ClimbingState.LeftFootGrip.bIsActive = false;
    ClimbingState.RightFootGrip.bIsActive = false;

    SetCustomMovementMode(ECustomMovementMode::CMOVE_Falling);
}

void UAdvancedClimbingComponent::HandleRopeCatch(float FallDistance)
{
    ClimbingState.bIsFalling = false;
    OnRopeCatch.Broadcast();

    // Calculate impact from rope catch
    if (ClimbingState.AttachedRope)
    {
        float ImpactForce = ClimbingState.AttachedRope->CalculateImpactForce(FallDistance, 70.0f); // Assume 70kg climber
        
        // Apply damage based on impact force
        float Damage = CalculateFallDamage(FallDistance, ImpactForce);
        
        // Reduce stamina and grip strength from the fall
        ConsumeStamina(FallDistance * 5.0f);
        ConsumeGripStrength(FallDistance * 3.0f);
        
        // Switch to swinging mode
        SetCustomMovementMode(ECustomMovementMode::CMOVE_Swinging);
    }
}

void UAdvancedClimbingComponent::HandleGroundImpact(float FallDistance, float ImpactVelocity)
{
    ClimbingState.bIsFalling = false;
    
    float Damage = CalculateFallDamage(FallDistance, ImpactVelocity);
    
    // Apply significant stamina and health consequences
    ConsumeStamina(FallDistance * 10.0f);
    ClimbingState.CurrentGripStrength = 0.0f; // Zero grip strength from impact
    
    UE_LOG(LogTemp, Warning, TEXT("Ground impact: %f damage from %fm fall"), Damage, FallDistance);
}

float UAdvancedClimbingComponent::CalculateFallDamage(float FallDistance, float ImpactVelocity) const
{
    if (FallDistance < Settings.FallDamageThreshold)
        return 0.0f;

    float BaseDamage = (FallDistance - Settings.FallDamageThreshold) * 10.0f;
    float VelocityMultiplier = ImpactVelocity / 1000.0f; // Normalize velocity
    
    return BaseDamage * VelocityMultiplier;
}

bool UAdvancedClimbingComponent::PlaceTool(AClimbingToolBase* Tool, const FVector& Location)
{
    if (!Tool || !CanReachTool(Tool))
        return false;

    // Check if we can reach the placement location
    float Distance = FVector::Dist(UpdatedComponent->GetComponentLocation(), Location);
    if (Distance > Settings.ReachDistance)
        return false;

    // Calculate surface normal at placement location
    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Location, Location + FVector::DownVector * 100.0f, ECC_WorldStatic))
    {
        return Tool->PlaceToolAt(Location, HitResult.Normal);
    }

    return false;
}

AClimbingToolBase* UAdvancedClimbingComponent::GetNearestTool(float SearchRadius) const
{
    TArray<AActor*> FoundActors;
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        UpdatedComponent->GetComponentLocation(),
        SearchRadius,
        TArray<TEnumAsByte<EObjectTypeQuery>>(),
        AClimbingToolBase::StaticClass(),
        TArray<AActor*>(),
        FoundActors
    );

    AClimbingToolBase* NearestTool = nullptr;
    float NearestDistance = SearchRadius;

    for (AActor* Actor : FoundActors)
    {
        if (AClimbingToolBase* Tool = Cast<AClimbingToolBase>(Actor))
        {
            float Distance = FVector::Dist(UpdatedComponent->GetComponentLocation(), Tool->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestTool = Tool;
            }
        }
    }

    return NearestTool;
}

bool UAdvancedClimbingComponent::CanReachTool(const AClimbingToolBase* Tool) const
{
    if (!Tool)
        return false;

    float Distance = FVector::Dist(UpdatedComponent->GetComponentLocation(), Tool->GetActorLocation());
    return Distance <= Settings.ReachDistance;
}

// Physics implementations
void UAdvancedClimbingComponent::PhysClimbing(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
        return;

    // Apply gravity with grip-based resistance
    FVector GravityEffect = CalculateGravityEffect();
    
    // Calculate movement based on input and grip positions
    FVector InputVector = ConsumeInputVector();
    FVector ClimbingVelocity = CalculateClimbingVelocity(InputVector);
    
    // Combine gravity and climbing movement
    Velocity = ClimbingVelocity + GravityEffect;
    
    // Apply movement
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    // Update last valid position if movement was successful
    if (!Hit.bBlockingHit)
    {
        LastValidClimbingPosition = UpdatedComponent->GetComponentLocation();
        TimeSinceLastValidPosition = 0.0f;
    }
    else
    {
        TimeSinceLastValidPosition += deltaTime;
    }
}

void UAdvancedClimbingComponent::PhysRoped(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
        return;

    // Apply gravity
    Velocity += GetGravity() * deltaTime;
    
    // Apply rope constraints
    ApplyRopeConstraints(Velocity, deltaTime);
    
    // Move with rope constraints
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    if (Hit.bBlockingHit)
    {
        // Handle collision while on rope
        HandleImpact(Hit, deltaTime, Velocity);
    }
}

void UAdvancedClimbingComponent::PhysAnchored(float deltaTime, int32 Iterations)
{
    // Character is anchored - minimal movement allowed
    Velocity = FVector::ZeroVector;
    
    // Allow small adjustments for comfort
    FVector InputVector = ConsumeInputVector();
    if (!InputVector.IsZero())
    {
        FVector LimitedMovement = InputVector * 50.0f; // 50 cm/s max while anchored
        FHitResult Hit;
        SafeMoveUpdatedComponent(LimitedMovement * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    }
}

void UAdvancedClimbingComponent::PhysFalling(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
        return;

    // Update fall distance
    FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
    ClimbingState.FallDistance = FVector::Dist(ClimbingState.FallStartLocation, CurrentLocation);
    
    // Apply gravity
    Velocity += GetGravity() * deltaTime;
    
    // Check for rope catch
    if (ClimbingState.AttachedRope && ClimbingState.FallDistance > ClimbingState.RopeSlackLength)
    {
        HandleRopeCatch(ClimbingState.FallDistance);
        return;
    }
    
    // Move with fall physics
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    if (Hit.bBlockingHit)
    {
        // Ground impact
        float ImpactVelocity = Velocity.Size();
        HandleGroundImpact(ClimbingState.FallDistance, ImpactVelocity);
        SetMovementMode(MOVE_Walking);
    }
}

void UAdvancedClimbingComponent::PhysSwinging(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
        return;

    // Apply pendulum physics
    HandleRopeSwing(deltaTime);
    
    // Apply damping to reduce swing over time
    Velocity *= Settings.SwingDamping;
    
    // Check if swing has damped enough to return to normal rope mode
    if (Velocity.Size() < 100.0f) // Less than 1 m/s
    {
        SetCustomMovementMode(ECustomMovementMode::CMOVE_Roped);
    }
}

void UAdvancedClimbingComponent::UpdateClimbingPhysics(float DeltaTime)
{
    // Performance optimization - reduce updates for distant players
    if (bOptimizeDistantUpdates && PlayerViewDistance > 2000.0f)
    {
        return;
    }

    // Update grip scanning
    LastGripScan += DeltaTime;
    if (LastGripScan >= GripScanInterval)
    {
        const_cast<TArray<FGripPoint>&>(CachedNearbyGrips) = FindNearbyGrips(Settings.ReachDistance);
        LastGripScan = 0.0f;
    }
}

void UAdvancedClimbingComponent::UpdateStaminaAndGrip(float DeltaTime)
{
    LastStaminaUpdate += DeltaTime;
    if (LastStaminaUpdate >= StaminaUpdateInterval)
    {
        RegenerateStamina(LastStaminaUpdate);
        RegenerateGripStrength(LastStaminaUpdate);

        // Calculate stamina drain from active grips
        float TotalDrain = 0.0f;
        if (ClimbingState.LeftHandGrip.bIsActive)
        {
            TotalDrain += CalculateGripStaminaDrain(ClimbingState.LeftHandGrip);
        }
        if (ClimbingState.RightHandGrip.bIsActive)
        {
            TotalDrain += CalculateGripStaminaDrain(ClimbingState.RightHandGrip);
        }
        if (ClimbingState.LeftFootGrip.bIsActive)
        {
            TotalDrain += CalculateGripStaminaDrain(ClimbingState.LeftFootGrip) * 0.5f; // Feet use less stamina
        }
        if (ClimbingState.RightFootGrip.bIsActive)
        {
            TotalDrain += CalculateGripStaminaDrain(ClimbingState.RightFootGrip) * 0.5f;
        }

        // Apply stamina drain
        ConsumeStamina(TotalDrain * LastStaminaUpdate);
        
        // Apply grip strength fatigue
        if (!ClimbingState.bIsResting)
        {
            ConsumeGripStrength(Settings.GripFatigueRate * LastStaminaUpdate);
        }

        LastStaminaUpdate = 0.0f;
    }
}

void UAdvancedClimbingComponent::CheckFallCondition()
{
    if (ClimbingState.CustomMovementMode != ECustomMovementMode::CMOVE_Climbing)
        return;

    // Check if grip strength is too low
    if (ClimbingState.CurrentGripStrength < Settings.MinGripThreshold)
    {
        StartFall();
        return;
    }

    // Check if no active grips
    bool HasActiveGrip = ClimbingState.LeftHandGrip.bIsActive || 
                        ClimbingState.RightHandGrip.bIsActive ||
                        ClimbingState.LeftFootGrip.bIsActive || 
                        ClimbingState.RightFootGrip.bIsActive;

    if (!HasActiveGrip)
    {
        StartFall();
        return;
    }

    // Check if can maintain current grips
    if (ClimbingState.LeftHandGrip.bIsActive && !CanMaintainGrip(ClimbingState.LeftHandGrip))
    {
        ReleaseGrip(true, true);
    }
    if (ClimbingState.RightHandGrip.bIsActive && !CanMaintainGrip(ClimbingState.RightHandGrip))
    {
        ReleaseGrip(false, true);
    }
}

void UAdvancedClimbingComponent::UpdateRopePhysics(float DeltaTime)
{
    if (!ClimbingState.AttachedRope)
        return;

    // Update rope slack based on movement
    float RopeTension = CalculateRopeTension();
    
    // Adjust movement based on rope constraints
    if (RopeTension > 0.0f)
    {
        // Rope is under tension - apply constraints
        FVector RopeDirection = (ClimbingState.AttachedRope->GetOwner()->GetActorLocation() - 
                                UpdatedComponent->GetComponentLocation()).GetSafeNormal();
        
        // Prevent movement that would increase rope tension beyond limits
        FVector ProjectedVelocity = FVector::VectorPlaneProject(Velocity, RopeDirection);
        Velocity = ProjectedVelocity;
    }
}

float UAdvancedClimbingComponent::CalculateRequiredGripStrength(const FGripPoint& Grip) const
{
    return Grip.GetRequiredGripStrength();
}

float UAdvancedClimbingComponent::CalculateGripStaminaDrain(const FGripPoint& Grip) const
{
    return Grip.GetStaminaDrainRate() * Settings.BaseStaminaDrainRate;
}

bool UAdvancedClimbingComponent::CanMaintainGrip(const FGripPoint& Grip) const
{
    float RequiredStrength = CalculateRequiredGripStrength(Grip);
    return ClimbingState.CurrentGripStrength >= RequiredStrength;
}

FVector UAdvancedClimbingComponent::CalculateClimbingVelocity(const FVector& InputVector) const
{
    if (InputVector.IsZero())
        return FVector::ZeroVector;

    FVector ClimbingDirection = InputVector.GetSafeNormal();
    float SpeedMultiplier = 1.0f;

    // Adjust speed based on stamina
    float StaminaRatio = ClimbingState.CurrentStamina / ClimbingState.MaxStamina;
    SpeedMultiplier *= FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.3f, 1.0f), StaminaRatio);

    // Adjust speed based on grip strength
    float GripRatio = ClimbingState.CurrentGripStrength / ClimbingState.MaxGripStrength;
    SpeedMultiplier *= FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.2f, 1.0f), GripRatio);

    return ClimbingDirection * Settings.ClimbingSpeed * SpeedMultiplier;
}

FVector UAdvancedClimbingComponent::CalculateGravityEffect() const
{
    FVector Gravity = GetGravity();
    
    // Reduce gravity effect based on active grips
    int32 ActiveGrips = 0;
    if (ClimbingState.LeftHandGrip.bIsActive) ActiveGrips++;
    if (ClimbingState.RightHandGrip.bIsActive) ActiveGrips++;
    if (ClimbingState.LeftFootGrip.bIsActive) ActiveGrips++;
    if (ClimbingState.RightFootGrip.bIsActive) ActiveGrips++;

    float GripSupport = FMath::Min(1.0f, ActiveGrips * 0.4f); // Each grip supports 40% of weight
    return Gravity * (1.0f - GripSupport);
}

bool UAdvancedClimbingComponent::ValidateMovement(const FVector& ProposedLocation) const
{
    // Ensure proposed location doesn't exceed reach from current grips
    // This is a simplified validation - real implementation would be more complex
    return true;
}

float UAdvancedClimbingComponent::AnalyzeSurfaceAngle(const FVector& SurfaceNormal) const
{
    return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(SurfaceNormal, FVector::UpVector)));
}

float UAdvancedClimbingComponent::AnalyzeSurfaceFriction(const UPhysicalMaterial* Material) const
{
    if (!Material)
        return 0.7f; // Default friction

    return Material->Friction;
}

EGripType UAdvancedClimbingComponent::DetermineGripType(const FHitResult& Hit) const
{
    // Simplified grip type determination - in practice would analyze surface geometry
    float RandomValue = FMath::RandRange(0.0f, 1.0f);
    
    if (RandomValue < 0.2f) return EGripType::Jug;
    if (RandomValue < 0.4f) return EGripType::Crimp;
    if (RandomValue < 0.6f) return EGripType::Sloper;
    if (RandomValue < 0.8f) return EGripType::Pinch;
    if (RandomValue < 0.9f) return EGripType::Pocket;
    return EGripType::Mantle;
}

void UAdvancedClimbingComponent::ApplyRopeConstraints(FVector& Velocity, float DeltaTime)
{
    if (!ClimbingState.AttachedRope)
        return;

    // Calculate rope constraint based on current rope length and position
    FVector RopeAnchor = ClimbingState.AttachedRope->GetOwner()->GetActorLocation();
    FVector CharacterLocation = UpdatedComponent->GetComponentLocation();
    FVector RopeVector = CharacterLocation - RopeAnchor;
    float RopeLength = RopeVector.Size();
    float MaxRopeLength = ClimbingState.AttachedRope->Properties.Length;

    if (RopeLength >= MaxRopeLength)
    {
        // Rope is at full extension - constrain movement
        FVector RopeDirection = RopeVector.GetSafeNormal();
        FVector ConstrainedVelocity = FVector::VectorPlaneProject(Velocity, RopeDirection);
        
        // Only allow movement that doesn't increase rope tension
        if (FVector::DotProduct(Velocity, RopeDirection) > 0.0f)
        {
            Velocity = ConstrainedVelocity;
        }
    }
}

float UAdvancedClimbingComponent::CalculateRopeTension() const
{
    if (!ClimbingState.AttachedRope)
        return 0.0f;

    return ClimbingState.AttachedRope->CalculateCurrentTension();
}

void UAdvancedClimbingComponent::HandleRopeSwing(float DeltaTime)
{
    if (!ClimbingState.AttachedRope)
        return;

    // Apply pendulum physics
    FVector RopeAnchor = ClimbingState.AttachedRope->GetOwner()->GetActorLocation();
    FVector CharacterLocation = UpdatedComponent->GetComponentLocation();
    FVector RopeVector = CharacterLocation - RopeAnchor;
    float RopeLength = RopeVector.Size();
    
    if (RopeLength > 0.0f)
    {
        FVector RadialDirection = RopeVector.GetSafeNormal();
        FVector TangentialDirection = FVector::CrossProduct(RadialDirection, FVector::UpVector);
        
        // Apply centripetal force to maintain swing
        float TangentialVelocity = FVector::DotProduct(Velocity, TangentialDirection);
        FVector CentripetalForce = RadialDirection * (TangentialVelocity * TangentialVelocity / RopeLength);
        
        Velocity += CentripetalForce * DeltaTime;
    }
}