#include "WaterPhysicsComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "DrawDebugHelpers.h"
#include "../Tools/ClimbingToolBase.h"
#include "../Performance/ClimbingPerformanceManager.h"
#include "EnvironmentalHazardManager.h"

UWaterPhysicsComponent::UWaterPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10Hz default
    SetIsReplicatedByDefault(true);

    // Initialize water properties with sensible defaults
    WaterProperties.WaterType = EWaterType::Freshwater;
    WaterProperties.Density = 1000.0f;
    WaterProperties.Viscosity = 0.001f;
    WaterProperties.Temperature = 15.0f;
    WaterProperties.FlowType = EWaterFlowType::Still;

    // Create water mesh component
    WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
    
    // Initialize current data grid
    CurrentData.GridOrigin = FVector::ZeroVector;
    CurrentData.GridSize = FVector(1000, 1000, 200);
    CurrentData.GridResolutionX = 20;
    CurrentData.GridResolutionY = 20;
    CurrentData.GridResolutionZ = 10;
}

void UWaterPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize performance manager reference
    if (AActor* Owner = GetOwner())
    {
        PerformanceManager = Owner->FindComponentByClass<UClimbingPerformanceManager>();
        EnvironmentalManager = Owner->FindComponentByClass<UEnvironmentalHazardManager>();
    }

    // Initialize current grid
    InitializeCurrentGrid();
    
    // Setup water surface level if not set
    if (WaterSurfaceLevel == FVector::ZeroVector && WaterMesh)
    {
        WaterSurfaceLevel = WaterMesh->GetComponentLocation();
    }
}

void UWaterPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableBuoyancy && !bEnableCurrents && !bEnableUnderwaterMechanics)
    {
        return;
    }

    // Performance-based update frequency
    float CurrentTime = GetWorld()->GetTimeSeconds();
    bool bShouldFullUpdate = (CurrentTime - LastFullUpdate) >= FullUpdateInterval;
    bool bShouldLightUpdate = (CurrentTime - LastLightUpdate) >= LightUpdateInterval;

    if (bShouldFullUpdate)
    {
        UpdateWaterPhysics(DeltaTime);
        LastFullUpdate = CurrentTime;
    }
    else if (bShouldLightUpdate)
    {
        // Light update - only essential systems
        UpdateCurrentFlow(DeltaTime);
        LastLightUpdate = CurrentTime;
    }

    // Update underwater states for tracked actors
    UpdateUnderwaterStates(DeltaTime);
}

void UWaterPhysicsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UWaterPhysicsComponent, CurrentData);
    DOREPLIFETIME(UWaterPhysicsComponent, UnderwaterStates);
}

void UWaterPhysicsComponent::UpdateWaterPhysics(float DeltaTime)
{
    if (!ShouldSimulateFullWaterPhysics())
    {
        return;
    }

    // Update current flow patterns
    UpdateCurrentFlow(DeltaTime);
    
    // Process all actors in water
    UpdateBuoyancyForces(DeltaTime);
    UpdateCurrentForces(DeltaTime);

    // Apply environmental effects
    for (auto& RopeRef : SubmergedRopes)
    {
        if (IsValid(RopeRef))
        {
            ApplyBuoyancyToRope(RopeRef, DeltaTime);
        }
    }
}

FBuoyancyData UWaterPhysicsComponent::CalculateBuoyancy(UPrimitiveComponent* Component, const FVector& ComponentLocation) const
{
    FBuoyancyData BuoyancyData;
    
    if (!Component || !IsValid(Component))
    {
        return BuoyancyData;
    }

    // Check cache first for performance
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CachedComponent == Component && 
        (CurrentTime - CachedBuoyancyTime) < BuoyancyCacheTimeout &&
        FVector::Dist(ComponentLocation, CachedBuoyancy.CenterOfBuoyancy) < 10.0f)
    {
        return CachedBuoyancy;
    }

    // Calculate submersion
    float SubmersionDepth = GetSubmersionDepth(ComponentLocation);
    if (SubmersionDepth <= 0.0f)
    {
        return BuoyancyData; // Not submerged
    }

    // Get component bounds for volume calculation
    FBoxSphereBounds ComponentBounds = Component->GetLocalBounds();
    float ComponentVolume = ComponentBounds.BoxExtent.X * ComponentBounds.BoxExtent.Y * ComponentBounds.BoxExtent.Z * 8.0f; // Rough box volume
    
    // Calculate submerged volume (simplified)
    float MaxSubmersionDepth = ComponentBounds.BoxExtent.Z * 2.0f;
    float SubmersionRatio = FMath::Clamp(SubmersionDepth / MaxSubmersionDepth, 0.0f, 1.0f);
    
    BuoyancyData.SubmergedVolume = ComponentVolume * SubmersionRatio * 0.000001f; // Convert cm³ to m³
    BuoyancyData.SubmersionPercentage = SubmersionRatio;
    
    // Calculate buoyant force (Archimedes' principle)
    float GravityAcceleration = 9.81f; // m/s²
    BuoyancyData.BuoyantForce = WaterProperties.Density * GravityAcceleration * BuoyancyData.SubmergedVolume;
    BuoyancyData.BuoyancyForceVector = FVector(0, 0, BuoyancyData.BuoyantForce);

    // Calculate center of buoyancy
    BuoyancyData.CenterOfBuoyancy = ComponentLocation + FVector(0, 0, -SubmersionDepth * 0.5f);

    // Calculate hydrodynamic drag
    FVector ComponentVelocity = Component->GetPhysicsLinearVelocity();
    float Speed = ComponentVelocity.Size() * 0.01f; // Convert cm/s to m/s
    float CrossSectionalArea = FMath::PI * FMath::Pow(ComponentBounds.SphereRadius * 0.01f, 2); // Approximate as sphere
    
    BuoyancyData.HydrodynamicDrag = CalculateDragForce(ComponentVelocity, CrossSectionalArea);

    // Cache result
    CachedBuoyancy = BuoyancyData;
    CachedComponent = Component;
    CachedBuoyancyTime = CurrentTime;

    return BuoyancyData;
}

void UWaterPhysicsComponent::ApplyBuoyancyToComponent(UPrimitiveComponent* Component, float DeltaTime)
{
    if (!Component || !Component->IsSimulatingPhysics() || !bEnableBuoyancy)
    {
        return;
    }

    FBuoyancyData Buoyancy = CalculateBuoyancy(Component, Component->GetComponentLocation());
    
    if (Buoyancy.SubmersionPercentage > 0.0f)
    {
        // Apply buoyant force
        Component->AddForceAtLocation(
            Buoyancy.BuoyancyForceVector * BuoyancyStrength * GlobalWaterIntensity,
            Buoyancy.CenterOfBuoyancy
        );

        // Apply drag force
        FVector DragForce = -Component->GetPhysicsLinearVelocity().GetSafeNormal() * Buoyancy.HydrodynamicDrag;
        Component->AddForce(DragForce);

        // Apply current forces if enabled
        if (bEnableCurrents)
        {
            ApplyCurrentForce(Component, DeltaTime);
        }
    }
}

void UWaterPhysicsComponent::ApplyBuoyancyToRope(UAdvancedRopeComponent* RopeComponent, float DeltaTime)
{
    if (!RopeComponent || !bEnableBuoyancy)
    {
        return;
    }

    // Process rope buoyancy through its underwater physics system
    RopeComponent->ProcessUnderwaterPhysics(this, DeltaTime);
}

void UWaterPhysicsComponent::InitializeCurrentGrid()
{
    int32 TotalCells = CurrentData.GridResolutionX * CurrentData.GridResolutionY * CurrentData.GridResolutionZ;
    CurrentData.FlowVectors.Reset(TotalCells);
    CurrentData.FlowSpeeds.Reset(TotalCells);

    // Initialize with base flow pattern
    for (int32 i = 0; i < TotalCells; i++)
    {
        CurrentData.FlowVectors.Add(WaterProperties.FlowDirection);
        CurrentData.FlowSpeeds.Add(WaterProperties.FlowVelocity);
    }
}

void UWaterPhysicsComponent::UpdateCurrentFlow(float DeltaTime)
{
    if (!bEnableCurrents || WaterProperties.FlowType == EWaterFlowType::Still)
    {
        return;
    }

    switch (WaterProperties.FlowType)
    {
        case EWaterFlowType::TurbulentFlow:
            GenerateTurbulentFlow(DeltaTime);
            break;
        case EWaterFlowType::Rapids:
        case EWaterFlowType::Waterfall:
            ProcessWaterfallFlow(DeltaTime);
            break;
        case EWaterFlowType::Whirlpool:
            ProcessWhirlpoolFlow(DeltaTime);
            break;
        default:
            break;
    }
}

void UWaterPhysicsComponent::GenerateTurbulentFlow(float DeltaTime)
{
    TurbulenceTimer += DeltaTime;
    
    // Generate pseudo-random turbulent flow
    for (int32 i = 0; i < CurrentData.FlowVectors.Num(); i++)
    {
        float NoiseX = FMath::PerlinNoise1D(TurbulenceTimer + i * 0.1f) * WaterProperties.Turbulence;
        float NoiseY = FMath::PerlinNoise1D(TurbulenceTimer + i * 0.1f + 100.0f) * WaterProperties.Turbulence;
        
        FVector TurbulentDirection = WaterProperties.FlowDirection + FVector(NoiseX, NoiseY, 0);
        TurbulentDirection.Normalize();
        
        CurrentData.FlowVectors[i] = TurbulentDirection;
        CurrentData.FlowSpeeds[i] = WaterProperties.FlowVelocity * (1.0f + NoiseX * 0.5f);
    }
}

FVector UWaterPhysicsComponent::GetCurrentFlowAtLocation(const FVector& WorldLocation) const
{
    return InterpolateCurrentFlow(WorldLocation);
}

float UWaterPhysicsComponent::CalculateDragForce(const FVector& Velocity, float CrossSectionalArea) const
{
    float Speed = Velocity.Size() * 0.01f; // Convert cm/s to m/s
    float DragCoefficient = 0.47f; // Sphere approximation
    
    // Drag force = 0.5 * ρ * v² * Cd * A
    return 0.5f * WaterProperties.Density * Speed * Speed * DragCoefficient * CrossSectionalArea;
}

FVector UWaterPhysicsComponent::InterpolateCurrentFlow(const FVector& WorldLocation) const
{
    // Convert world location to grid coordinates
    FVector LocalLocation = WorldLocation - CurrentData.GridOrigin;
    FVector GridPosition = LocalLocation / CurrentData.GridSize;
    
    // Clamp to grid bounds
    GridPosition.X = FMath::Clamp(GridPosition.X, 0.0f, 1.0f);
    GridPosition.Y = FMath::Clamp(GridPosition.Y, 0.0f, 1.0f);
    GridPosition.Z = FMath::Clamp(GridPosition.Z, 0.0f, 1.0f);
    
    // Calculate grid indices
    int32 X = FMath::FloorToInt(GridPosition.X * (CurrentData.GridResolutionX - 1));
    int32 Y = FMath::FloorToInt(GridPosition.Y * (CurrentData.GridResolutionY - 1));
    int32 Z = FMath::FloorToInt(GridPosition.Z * (CurrentData.GridResolutionZ - 1));
    
    // Simple trilinear interpolation (simplified for performance)
    int32 Index = X + Y * CurrentData.GridResolutionX + Z * CurrentData.GridResolutionX * CurrentData.GridResolutionY;
    
    if (CurrentData.FlowVectors.IsValidIndex(Index))
    {
        FVector FlowDirection = CurrentData.FlowVectors[Index];
        float FlowSpeed = CurrentData.FlowSpeeds.IsValidIndex(Index) ? CurrentData.FlowSpeeds[Index] : 0.0f;
        
        return FlowDirection * FlowSpeed;
    }
    
    return FVector::ZeroVector;
}

bool UWaterPhysicsComponent::IsActorSubmerged(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    float SubmersionDepth = GetSubmersionDepth(Actor->GetActorLocation());
    return SubmersionDepth > 0.0f;
}

float UWaterPhysicsComponent::GetSubmersionDepth(const FVector& WorldLocation) const
{
    // Simple depth calculation based on water surface level
    float WaterSurfaceZ = WaterSurfaceLevel.Z;
    float LocationZ = WorldLocation.Z;
    
    float Depth = WaterSurfaceZ - LocationZ;
    return FMath::Max(0.0f, Depth * 0.01f); // Convert cm to meters
}

float UWaterPhysicsComponent::CalculateWaterPressure(float Depth) const
{
    // Hydrostatic pressure: P = P₀ + ρgh
    float SurfacePressure = 101325.0f; // Pascals (1 atmosphere)
    float GravityAcceleration = 9.81f; // m/s²
    
    return SurfacePressure + (WaterProperties.Density * GravityAcceleration * Depth);
}

float UWaterPhysicsComponent::CalculateVisibilityRange(float Depth, float Turbidity) const
{
    // Underwater visibility formula
    float BaseVisibility = 50.0f; // meters in perfect conditions
    float TurbidityReduction = FMath::Pow(1.0f - Turbidity, 2.0f);
    float DepthReduction = FMath::Exp(-Depth * 0.1f); // Light attenuation with depth
    
    return BaseVisibility * TurbidityReduction * DepthReduction;
}

void UWaterPhysicsComponent::EnterWater(AActor* Actor)
{
    if (!Actor || UnderwaterStates.Contains(Actor))
    {
        return;
    }

    // Initialize underwater state
    FUnderwaterState NewState;
    NewState.SwimmingState = ESwimmingState::Surface;
    NewState.TimeSubmerged = 0.0f;
    NewState.OxygenLevel = 100.0f;
    NewState.WaterPressure = 101325.0f; // Surface pressure
    
    UnderwaterStates.Add(Actor, NewState);
    
    // Trigger events
    OnActorEnterWater.Broadcast();
    MulticastOnActorEnterWater(Actor);
    
    // Apply immediate temperature effects
    if (bEnableTemperatureEffects)
    {
        ActorTemperatures.Add(Actor, 37.0f); // Normal body temperature
        ExposureTimes.Add(Actor, 0.0f);
    }
}

void UWaterPhysicsComponent::ExitWater(AActor* Actor)
{
    if (!Actor || !UnderwaterStates.Contains(Actor))
    {
        return;
    }

    // Clean up tracking
    UnderwaterStates.Remove(Actor);
    ActorTemperatures.Remove(Actor);
    ExposureTimes.Remove(Actor);
    
    // Trigger events
    OnActorExitWater.Broadcast();
    MulticastOnActorExitWater(Actor);
}

void UWaterPhysicsComponent::UpdateUnderwaterStates(float DeltaTime)
{
    for (auto& StateEntry : UnderwaterStates)
    {
        AActor* Actor = StateEntry.Key;
        FUnderwaterState& State = StateEntry.Value;
        
        if (!IsValid(Actor))
        {
            continue;
        }

        // Update depth and pressure
        float NewDepth = GetSubmersionDepth(Actor->GetActorLocation());
        State.CurrentDepth = NewDepth;
        State.MaxDepthReached = FMath::Max(State.MaxDepthReached, NewDepth);
        State.WaterPressure = CalculateWaterPressure(NewDepth);
        
        // Update swimming state
        if (NewDepth <= 0.5f)
        {
            State.SwimmingState = ESwimmingState::Surface;
        }
        else if (NewDepth <= 5.0f)
        {
            State.SwimmingState = ESwimmingState::Submerged;
        }
        else
        {
            State.SwimmingState = ESwimmingState::Diving;
        }
        
        // Update time submerged
        if (NewDepth > 0.1f)
        {
            State.TimeSubmerged += DeltaTime;
        }
        
        // Process oxygen consumption
        if (bEnableUnderwaterMechanics && State.SwimmingState != ESwimmingState::Surface)
        {
            ProcessOxygenConsumption(Actor, DeltaTime);
        }
        
        // Process decompression if diving deep
        if (NewDepth > DecompressionLimit)
        {
            ProcessDecompression(Actor, DeltaTime);
        }
        
        // Update visibility range
        State.VisibilityRange = CalculateVisibilityRange(NewDepth, WaterProperties.TurbidityLevel);
    }
}

void UWaterPhysicsComponent::ProcessOxygenConsumption(AActor* Actor, float DeltaTime)
{
    if (!UnderwaterStates.Contains(Actor))
    {
        return;
    }

    FUnderwaterState& State = UnderwaterStates[Actor];
    
    // Base oxygen consumption rate
    float ConsumptionRate = OxygenConsumptionRate;
    
    // Increase consumption with activity and stress
    if (State.SwimmingState == ESwimmingState::Emergency)
    {
        ConsumptionRate *= 3.0f; // Panic breathing
    }
    else if (State.CurrentDepth > 10.0f)
    {
        ConsumptionRate *= 1.5f; // Increased work at depth
    }
    
    // Apply consumption
    State.OxygenLevel = FMath::Max(0.0f, State.OxygenLevel - ConsumptionRate * DeltaTime);
    
    // Check for critical oxygen levels
    if (State.OxygenLevel < 20.0f && State.SwimmingState != ESwimmingState::Emergency)
    {
        State.SwimmingState = ESwimmingState::Emergency;
        OnUnderwaterEmergency.Broadcast();
    }
}

void UWaterPhysicsComponent::ProcessDecompression(AActor* Actor, float DeltaTime)
{
    if (!UnderwaterStates.Contains(Actor))
    {
        return;
    }

    FUnderwaterState& State = UnderwaterStates[Actor];
    
    // Simplified nitrogen loading calculation
    float DepthRatio = State.CurrentDepth / 30.0f; // Normalized to 30m reference
    State.NitrogenLoad += DepthRatio * DeltaTime * 0.1f; // Simplified absorption
    
    // Calculate ascent rate
    static float LastDepth = State.CurrentDepth;
    State.AscentRate = (LastDepth - State.CurrentDepth) / DeltaTime * 60.0f; // m/min
    LastDepth = State.CurrentDepth;
    
    // Check for dangerous ascent rate
    if (State.AscentRate > SafeAscentRate && State.NitrogenLoad > 0.5f)
    {
        OnDecompressionWarning.Broadcast();
    }
}

bool UWaterPhysicsComponent::ShouldSimulateFullWaterPhysics() const
{
    if (!bSimulateFullPhysics)
    {
        return false;
    }

    // Check performance constraints
    if (PerformanceManager)
    {
        float CurrentFrameTime = PerformanceManager->GetAverageFrameTime();
        return CurrentFrameTime < 20.0f; // 50+ FPS
    }

    return true;
}

void UWaterPhysicsComponent::OptimizeForDistance(float ViewerDistance)
{
    this->ViewerDistance = ViewerDistance;
    
    // Adjust LOD based on distance
    if (ViewerDistance > MaxSimulationDistance)
    {
        CurrentLODLevel = 3; // Minimal simulation
    }
    else if (ViewerDistance > MaxSimulationDistance * 0.5f)
    {
        CurrentLODLevel = 2; // Reduced simulation
    }
    else if (ViewerDistance > MaxSimulationDistance * 0.25f)
    {
        CurrentLODLevel = 1; // Medium simulation
    }
    else
    {
        CurrentLODLevel = 0; // Full simulation
    }
    
    // Adjust update frequencies based on distance
    float DistanceRatio = ViewerDistance / MaxSimulationDistance;
    FullUpdateInterval = FMath::Lerp(0.1f, 1.0f, DistanceRatio);
    LightUpdateInterval = FMath::Lerp(0.5f, 2.0f, DistanceRatio);
}

// Network function implementations
void UWaterPhysicsComponent::ServerUpdateWaterState_Implementation(const FWaterCurrentData& NewCurrentData)
{
    CurrentData = NewCurrentData;
}

bool UWaterPhysicsComponent::ServerUpdateWaterState_Validate(const FWaterCurrentData& NewCurrentData)
{
    // Basic validation
    return NewCurrentData.GridResolutionX > 0 && 
           NewCurrentData.GridResolutionY > 0 && 
           NewCurrentData.GridResolutionZ > 0;
}

void UWaterPhysicsComponent::MulticastOnActorEnterWater_Implementation(AActor* Actor)
{
    // Visual and audio effects for entering water
    if (Actor && IsValid(Actor))
    {
        // Could trigger splash effects, sound, etc.
    }
}

void UWaterPhysicsComponent::MulticastOnActorExitWater_Implementation(AActor* Actor)
{
    // Visual and audio effects for exiting water
    if (Actor && IsValid(Actor))
    {
        // Could trigger water dripping effects, etc.
    }
}