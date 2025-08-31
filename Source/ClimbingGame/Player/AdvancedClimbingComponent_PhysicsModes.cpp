#include "AdvancedClimbingComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// Physics Mode Implementations for AdvancedClimbingComponent

void UAdvancedClimbingComponent::PhysClimbing(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    // Calculate climbing forces
    FVector InputVector = ConsumeInputVector();
    FVector ClimbingForce = CalculateClimbingVelocity(InputVector);
    FVector GravityEffect = CalculateGravityEffect();
    
    // Apply stamina-based movement scaling
    float StaminaMultiplier = FMath::GetMappedRangeValueClamped(
        FVector2D(0.0f, ClimbingState.MaxStamina),
        FVector2D(0.3f, 1.0f),
        ClimbingState.CurrentStamina
    );
    
    // Apply grip strength scaling
    float GripMultiplier = FMath::GetMappedRangeValueClamped(
        FVector2D(0.0f, ClimbingState.MaxGripStrength),
        FVector2D(0.2f, 1.0f),
        ClimbingState.CurrentGripStrength
    );
    
    // Calculate final velocity
    FVector NewVelocity = ClimbingForce * StaminaMultiplier * GripMultiplier + GravityEffect;
    
    // Apply rope constraints if attached to rope
    if (ClimbingState.AttachedRope)
    {
        ApplyRopeConstraints(NewVelocity, deltaTime);
    }
    
    // Validate movement doesn't exceed grip reach limits
    if (!ValidateMovement(UpdatedComponent->GetComponentLocation() + NewVelocity * deltaTime))
    {
        NewVelocity *= 0.5f; // Reduce movement if it would break grip constraints
    }
    
    Velocity = NewVelocity;
    
    // Perform movement with collision detection
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    // Handle collision
    if (Hit.IsValidBlockingHit())
    {
        HandleClimbingCollision(Hit, deltaTime);
    }
    
    // Update grip states and stamina consumption
    UpdateGripStatesFromMovement(deltaTime);
}

void UAdvancedClimbingComponent::PhysRoped(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME || !ClimbingState.AttachedRope)
    {
        return;
    }

    // Base physics similar to climbing but with rope constraints
    FVector InputVector = ConsumeInputVector();
    FVector MovementForce = InputVector * Settings.ClimbingSpeed * 0.8f; // Reduced speed on rope
    FVector GravityForce = GetGravity();
    
    // Calculate rope swing physics
    HandleRopeSwing(deltaTime);
    
    // Apply rope tension effects
    float RopeTension = CalculateRopeTension();
    if (RopeTension > 1000.0f) // High tension
    {
        // Reduce movement capability under high rope tension
        MovementForce *= 0.5f;
    }
    
    // Combine forces
    FVector NewVelocity = MovementForce + GravityForce + Velocity * Settings.SwingDamping;
    
    // Apply rope length constraints
    ApplyRopeConstraints(NewVelocity, deltaTime);
    
    Velocity = NewVelocity;
    
    // Move with collision
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    if (Hit.IsValidBlockingHit())
    {
        HandleRopeCollision(Hit, deltaTime);
    }
    
    // Check if we should transition to climbing mode
    TArray<FGripPoint> NearbyGrips = FindNearbyGrips(Settings.ReachDistance);
    if (NearbyGrips.Num() > 0)
    {
        // Opportunity to grab onto surface
        for (const FGripPoint& Grip : NearbyGrips)
        {
            if (CanMaintainGrip(Grip))
            {
                // Auto-grab if close enough and player is trying to climb
                if (!InputVector.IsZero() && FVector::Dist(UpdatedComponent->GetComponentLocation(), Grip.Location) < 50.0f)
                {
                    TryGrabHold(Grip, true); // Grab with left hand first
                    SetCustomMovementMode(ECustomMovementMode::CMOVE_Climbing);
                    break;
                }
            }
        }
    }
}

void UAdvancedClimbingComponent::PhysAnchored(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    // Player is anchored (belaying or resting)
    // Very limited movement, mostly stationary with slight sway
    
    FVector InputVector = ConsumeInputVector();
    FVector RestrictedMovement = InputVector * 50.0f; // Very limited movement
    
    // Apply anchor constraints - can only move within anchor range
    float MaxAnchorMovement = 100.0f; // 1 meter radius
    FVector AnchorPosition = LastValidClimbingPosition; // Use anchor position
    FVector CurrentOffset = UpdatedComponent->GetComponentLocation() - AnchorPosition;
    
    if (CurrentOffset.Size() + RestrictedMovement.Size() * deltaTime > MaxAnchorMovement)
    {
        RestrictedMovement = FVector::ZeroVector; // No movement if at anchor limit
    }
    
    // Small gravity effect (mostly supported by anchor)
    FVector AnchorGravity = GetGravity() * 0.1f;
    
    Velocity = RestrictedMovement + AnchorGravity;
    
    // Perform restricted movement
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    // In anchored mode, player recovers stamina quickly
    RegenerateStamina(deltaTime * 3.0f); // Triple stamina recovery while anchored
    RegenerateGripStrength(deltaTime * 2.0f); // Double grip recovery
}

void UAdvancedClimbingComponent::PhysFalling(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    // Enhanced falling physics for climbing context
    ClimbingState.bIsFalling = true;
    
    if (ClimbingState.FallStartLocation.IsZero())
    {
        ClimbingState.FallStartLocation = UpdatedComponent->GetComponentLocation();
    }
    
    // Calculate current fall distance
    ClimbingState.FallDistance = FVector::Dist(ClimbingState.FallStartLocation, UpdatedComponent->GetComponentLocation());
    
    // Apply gravity
    FVector GravityForce = GetGravity();
    
    // Air resistance (simplified)
    FVector AirResistance = -Velocity.GetSafeNormal() * Velocity.SizeSquared() * 0.001f;
    
    // Rope catch physics
    if (ClimbingState.AttachedRope)
    {
        float RopeLength = ClimbingState.AttachedRope->Properties.Length;
        if (ClimbingState.FallDistance >= RopeLength - ClimbingState.RopeSlackLength)
        {
            // Rope catches the fall
            HandleRopeCatch();
            return;
        }
    }
    
    // Update velocity
    Velocity += (GravityForce + AirResistance) * deltaTime;
    
    // Terminal velocity cap
    float TerminalVelocity = 5600.0f; // ~200 km/h terminal velocity
    if (Velocity.Z < -TerminalVelocity)
    {
        Velocity.Z = -TerminalVelocity;
    }
    
    // Perform movement
    FHitResult Hit;
    SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
    
    if (Hit.IsValidBlockingHit())
    {
        // Ground impact or wall collision
        HandleFallImpact(Hit);
    }
    
    // Check for emergency grabs during fall
    if (ClimbingState.FallDistance < 5.0f) // Only within first 5 meters
    {
        TArray<FGripPoint> EmergencyGrips = FindNearbyGrips(Settings.ReachDistance * 2.0f);
        for (const FGripPoint& Grip : EmergencyGrips)
        {
            float GripDistance = FVector::Dist(UpdatedComponent->GetComponentLocation(), Grip.Location);
            if (GripDistance < 80.0f && CanMaintainGrip(Grip)) // Close emergency grab
            {
                // Emergency grab attempt - high stamina cost
                if (ClimbingState.CurrentStamina > 30.0f)
                {
                    ConsumeStamina(30.0f); // High cost for emergency grab
                    TryGrabHold(Grip, true);
                    SetCustomMovementMode(ECustomMovementMode::CMOVE_Climbing);
                    ClimbingState.bIsFalling = false;
                    OnRopeCatch.Broadcast(); // Same event for emergency grabs
                    break;
                }
            }
        }
    }
}

void UAdvancedClimbingComponent::PhysSwinging(float deltaTime, int32 Iterations)
{
    if (!HasValidData() || deltaTime < MIN_TICK_TIME || !ClimbingState.AttachedRope)
    {
        return;
    }

    // Pendulum swing physics
    FVector RopeAnchor = ClimbingState.AttachedRope->AnchorPointA ? 
        ClimbingState.AttachedRope->AnchorPointA->GetActorLocation() : 
        ClimbingState.AttachedRope->GetOwner()->GetActorLocation();
        
    FVector CharacterLocation = UpdatedComponent->GetComponentLocation();
    FVector RopeVector = CharacterLocation - RopeAnchor;
    float RopeLength = RopeVector.Size();
    
    if (RopeLength > 0.0f)
    {
        // Calculate pendulum forces
        FVector RadialDirection = RopeVector.GetSafeNormal();
        FVector TangentialDirection = FVector::CrossProduct(RadialDirection, FVector::UpVector);
        TangentialDirection.Normalize();
        
        // Input affects swing direction
        FVector InputVector = ConsumeInputVector();
        FVector SwingInput = FVector::VectorPlaneProject(InputVector, RadialDirection);
        SwingInput.Normalize();
        
        // Apply swing force
        float SwingForce = 500.0f * FVector::DotProduct(SwingInput, TangentialDirection);
        FVector SwingAcceleration = TangentialDirection * SwingForce;
        
        // Gravity component perpendicular to rope
        FVector GravityForce = GetGravity();
        FVector TangentialGravity = FVector::VectorPlaneProject(GravityForce, RadialDirection);
        
        // Damping
        FVector Damping = -Velocity * 0.1f;
        
        // Update velocity
        Velocity += (SwingAcceleration + TangentialGravity + Damping) * deltaTime;
        
        // Enforce rope constraint
        FVector RadialVelocity = FVector::ProjectOnTo(Velocity, RadialDirection);
        if (RopeLength >= ClimbingState.AttachedRope->Properties.Length)
        {
            // At full extension - remove radial component if extending
            if (FVector::DotProduct(RadialVelocity, RadialDirection) > 0.0f)
            {
                Velocity -= RadialVelocity;
            }
        }
        
        // Move
        FHitResult Hit;
        SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
        
        if (Hit.IsValidBlockingHit())
        {
            HandleSwingCollision(Hit, deltaTime);
        }
        
        // Check for transition to climbing
        TArray<FGripPoint> NearbyGrips = FindNearbyGrips(Settings.ReachDistance);
        if (NearbyGrips.Num() > 0 && !InputVector.IsZero())
        {
            for (const FGripPoint& Grip : NearbyGrips)
            {
                if (CanMaintainGrip(Grip) && FVector::Dist(CharacterLocation, Grip.Location) < 60.0f)
                {
                    TryGrabHold(Grip, true);
                    SetCustomMovementMode(ECustomMovementMode::CMOVE_Climbing);
                    break;
                }
            }
        }
    }
}

// Collision handling methods
void UAdvancedClimbingComponent::HandleClimbingCollision(const FHitResult& Hit, float DeltaTime)
{
    if (Hit.IsValidBlockingHit())
    {
        // Slide along the surface
        FVector Normal = Hit.Normal;
        FVector Deflection = FVector::VectorPlaneProject(Velocity, Normal);
        Velocity = Deflection * 0.8f; // Reduce velocity on collision
        
        // Try to find new grips on the collision surface
        if (IsValidClimbingSurface(Hit))
        {
            FGripPoint CollisionGrip;
            CollisionGrip.Location = Hit.Location;
            CollisionGrip.Normal = Hit.Normal;
            CollisionGrip.Surface = Hit.GetActor();
            CollisionGrip.Type = DetermineGripType(Hit);
            CollisionGrip.Quality = 0.6f; // Moderate quality for collision grips
            CollisionGrip.Size = 0.7f;
            
            // Auto-grab if we don't have enough active grips
            int32 ActiveGrips = 0;
            if (ClimbingState.LeftHandGrip.bIsActive) ActiveGrips++;
            if (ClimbingState.RightHandGrip.bIsActive) ActiveGrips++;
            
            if (ActiveGrips < 2 && CanMaintainGrip(CollisionGrip))
            {
                TryGrabHold(CollisionGrip, !ClimbingState.LeftHandGrip.bIsActive);
            }
        }
    }
}

void UAdvancedClimbingComponent::HandleRopeCollision(const FHitResult& Hit, float DeltaTime)
{
    // Bounce off surfaces while on rope
    FVector Normal = Hit.Normal;
    FVector ReflectedVelocity = Velocity - 2.0f * FVector::DotProduct(Velocity, Normal) * Normal;
    Velocity = ReflectedVelocity * 0.6f; // Energy loss on collision
}

void UAdvancedClimbingComponent::HandleSwingCollision(const FHitResult& Hit, float DeltaTime)
{
    // Similar to rope collision but with different energy loss
    FVector Normal = Hit.Normal;
    FVector ReflectedVelocity = Velocity - 2.0f * FVector::DotProduct(Velocity, Normal) * Normal;
    Velocity = ReflectedVelocity * 0.4f; // More energy loss during swinging
}

void UAdvancedClimbingComponent::HandleRopeCatch()
{
    ClimbingState.bIsFalling = false;
    SetCustomMovementMode(ECustomMovementMode::CMOVE_Roped);
    
    // Calculate fall impact based on fall distance and rope properties
    float FallEnergy = 0.5f * 70.0f * Velocity.SizeSquared(); // Assuming 70kg climber
    float RopeAbsorption = ClimbingState.AttachedRope->Properties.DynamicElongation / 100.0f;
    
    // Rope absorbs most of the energy
    float ImpactForce = FallEnergy * (1.0f - RopeAbsorption);
    
    // Apply some damage/stamina loss based on impact
    float StaminaLoss = FMath::Clamp(ImpactForce * 0.01f, 5.0f, 30.0f);
    ConsumeStamina(StaminaLoss);
    
    // Record fall in rope
    ClimbingState.AttachedRope->RecordFall(ClimbingState.FallDistance, ImpactForce);
    
    OnRopeCatch.Broadcast();
    
    // Reset fall tracking
    ClimbingState.FallStartLocation = FVector::ZeroVector;
    ClimbingState.FallDistance = 0.0f;
}

void UAdvancedClimbingComponent::HandleFallImpact(const FHitResult& Hit)
{
    ClimbingState.bIsFalling = false;
    
    // Calculate impact damage
    float ImpactVelocity = Velocity.Size();
    float FallDamage = CalculateFallDamage(ClimbingState.FallDistance, ImpactVelocity);
    
    // Apply damage through fall mechanics system if available
    if (auto FallSystem = GetOwner()->FindComponentByClass<class UFallMechanicsSystem>())
    {
        FallSystem->HandleGroundImpact(Velocity, Hit.Location, Hit.GetActor());
    }
    
    // Reset to walking mode
    SetMovementMode(MOVE_Walking);
    ClimbingState.CustomMovementMode = ECustomMovementMode::CMOVE_None;
    
    // Reset fall tracking
    ClimbingState.FallStartLocation = FVector::ZeroVector;
    ClimbingState.FallDistance = 0.0f;
}

void UAdvancedClimbingComponent::UpdateGripStatesFromMovement(float DeltaTime)
{
    // Update stamina consumption based on active grips
    float TotalStaminaDrain = 0.0f;
    
    if (ClimbingState.LeftHandGrip.bIsActive)
    {
        TotalStaminaDrain += CalculateGripStaminaDrain(ClimbingState.LeftHandGrip);
    }
    if (ClimbingState.RightHandGrip.bIsActive)
    {
        TotalStaminaDrain += CalculateGripStaminaDrain(ClimbingState.RightHandGrip);
    }
    if (ClimbingState.LeftFootGrip.bIsActive)
    {
        TotalStaminaDrain += CalculateGripStaminaDrain(ClimbingState.LeftFootGrip) * 0.5f; // Feet use less stamina
    }
    if (ClimbingState.RightFootGrip.bIsActive)
    {
        TotalStaminaDrain += CalculateGripStaminaDrain(ClimbingState.RightFootGrip) * 0.5f;
    }
    
    ConsumeStamina(TotalStaminaDrain * DeltaTime);
    
    // Grip strength consumption
    float GripDrain = TotalStaminaDrain * 0.3f * DeltaTime;
    ConsumeGripStrength(GripDrain);
}