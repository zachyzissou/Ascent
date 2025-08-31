#include "GeologicalPhysicsSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "DrawDebugHelpers.h"
#include "../Tools/ClimbingToolBase.h"

UGeologicalPhysicsSystem::UGeologicalPhysicsSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    SetIsReplicatedByDefault(true);

    // Initialize default seismic data
    CurrentSeismicActivity.Epicenter = FVector::ZeroVector;
    CurrentSeismicActivity.Magnitude = 0.0f;
    CurrentSeismicActivity.Depth = 1000.0f;
    CurrentSeismicActivity.Duration = 0.0f;
    CurrentSeismicActivity.PrimaryWaveType = ESeismicWaveType::Primary;
    CurrentSeismicActivity.PWaveVelocity = 6000.0f;
    CurrentSeismicActivity.SWaveVelocity = 3500.0f;
    CurrentSeismicActivity.AttenuationFactor = 0.1f;
}

void UGeologicalPhysicsSystem::BeginPlay()
{
    Super::BeginPlay();

    // Try to find environmental hazard manager
    if (!HazardManager)
    {
        HazardManager = GetOwner()->FindComponentByClass<UEnvironmentalHazardManager>();
    }

    // Initialize structural analysis
    CalculateStructuralLoads();
}

void UGeologicalPhysicsSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return; // Only simulate on server
    }

    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    // Update geological events based on LOD and timing
    if (CurrentTime - LastSeismicUpdate > SeismicUpdateInterval)
    {
        UpdateSeismicActivity(DeltaTime);
        LastSeismicUpdate = CurrentTime;
    }

    if (CurrentTime - LastDebrisUpdate > DebrisUpdateInterval)
    {
        UpdateAllGeologicalEvents(DeltaTime);
        UpdateDebrisPhysics(DeltaTime);
        LastDebrisUpdate = CurrentTime;
    }

    if (CurrentTime - LastStructuralUpdate > StructuralUpdateInterval)
    {
        ProcessStructuralStress(DeltaTime);
        ProcessWeatheringEffects(DeltaTime);
        LastStructuralUpdate = CurrentTime;
    }

    // Always check hazard zones
    CheckHazardZoneProximity();

    // Clear old terrain cache
    if (bUseTerrainCache && CurrentTime - LastTerrainCacheUpdate > TerrainCacheTimeout)
    {
        TerrainHardnessCache.Empty();
        SlopeAngleCache.Empty();
        LastTerrainCacheUpdate = CurrentTime;
    }

    // Update LOD if needed
    if (CurrentTime - LastLODUpdate > LODUpdateInterval)
    {
        float ClosestPlayerDistance = MaxSimulationDistance;
        if (GetWorld())
        {
            for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
            {
                APlayerController* PC = Iterator->Get();
                if (PC && PC->GetPawn())
                {
                    float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), GetOwner()->GetActorLocation());
                    ClosestPlayerDistance = FMath::Min(ClosestPlayerDistance, Distance);
                }
            }
        }
        
        OptimizeGeologicalSimulation(ClosestPlayerDistance);
        LastLODUpdate = CurrentTime;
    }
}

void UGeologicalPhysicsSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UGeologicalPhysicsSystem, ActiveRockfalls);
    DOREPLIFETIME(UGeologicalPhysicsSystem, ActiveAvalanches);
    DOREPLIFETIME(UGeologicalPhysicsSystem, CurrentSeismicActivity);
}

void UGeologicalPhysicsSystem::RegisterGeologicalStructure(const FGeologicalStructure& Structure)
{
    // Check if structure already exists
    for (int32 i = 0; i < GeologicalStructures.Num(); ++i)
    {
        if (GeologicalStructures[i].StructureID == Structure.StructureID)
        {
            GeologicalStructures[i] = Structure; // Update existing
            return;
        }
    }
    
    GeologicalStructures.Add(Structure);
    
    // Recalculate structural loads when new structure is added
    CalculateStructuralLoads();
}

void UGeologicalPhysicsSystem::UnregisterGeologicalStructure(const FString& StructureID)
{
    GeologicalStructures.RemoveAll([&StructureID](const FGeologicalStructure& Structure)
    {
        return Structure.StructureID == StructureID;
    });
    
    // Recalculate structural loads when structure is removed
    CalculateStructuralLoads();
}

EStructuralStability UGeologicalPhysicsSystem::AnalyzeStructuralStability(const FString& StructureID)
{
    FGeologicalStructure* Structure = GeologicalStructures.FindByPredicate([&StructureID](const FGeologicalStructure& S)
    {
        return S.StructureID == StructureID;
    });

    if (!Structure)
    {
        return EStructuralStability::Stable; // Default if not found
    }

    // Calculate stability based on accumulated stress and rock properties
    float StabilityFactor = 1.0f - Structure->StressAccumulation;
    
    // Adjust for rock type strength
    float RockStrengthFactor = Structure->RockProperties.CompressiveStrength / 200.0f; // Normalized to granite
    StabilityFactor *= RockStrengthFactor;
    
    // Adjust for fracture lines
    float FractureImpact = FMath::Min(Structure->FractureLines.Num() * 0.1f, 0.5f);
    StabilityFactor -= FractureImpact;
    
    // Determine stability level
    if (StabilityFactor > 0.8f)
    {
        Structure->Stability = EStructuralStability::Stable;
    }
    else if (StabilityFactor > 0.6f)
    {
        Structure->Stability = EStructuralStability::SlightlyUnstable;
    }
    else if (StabilityFactor > 0.4f)
    {
        Structure->Stability = EStructuralStability::Unstable;
    }
    else if (StabilityFactor > 0.2f)
    {
        Structure->Stability = EStructuralStability::VeryUnstable;
    }
    else
    {
        Structure->Stability = EStructuralStability::Critical;
    }

    return Structure->Stability;
}

void UGeologicalPhysicsSystem::ApplyStressToStructure(const FString& StructureID, float StressAmount)
{
    FGeologicalStructure* Structure = GeologicalStructures.FindByPredicate([&StructureID](const FGeologicalStructure& S)
    {
        return S.StructureID == StructureID;
    });

    if (!Structure)
    {
        return;
    }

    // Apply stress with global multiplier
    float AdjustedStress = StressAmount * GlobalStressMultiplier;
    Structure->StressAccumulation += AdjustedStress;
    Structure->StressAccumulation = FMath::Clamp(Structure->StressAccumulation, 0.0f, 1.0f);

    // Check if stress causes fracturing
    if (Structure->StressAccumulation > Structure->RockProperties.FractureResistance)
    {
        // Add new fracture line
        FVector FractureLocation = Structure->CenterLocation + 
                                  FVector(FMath::RandRange(-Structure->Bounds.X * 0.5f, Structure->Bounds.X * 0.5f),
                                         FMath::RandRange(-Structure->Bounds.Y * 0.5f, Structure->Bounds.Y * 0.5f),
                                         FMath::RandRange(-Structure->Bounds.Z * 0.5f, Structure->Bounds.Z * 0.5f));
        
        Structure->FractureLines.Add(FractureLocation);
        
        // Reduce stress accumulation after fracturing (stress relief)
        Structure->StressAccumulation *= 0.7f;
    }

    // Update stability analysis
    EStructuralStability NewStability = AnalyzeStructuralStability(StructureID);
    
    // Check for structural failure
    if (NewStability == EStructuralStability::Critical && FMath::RandRange(0.0f, 1.0f) < 0.1f)
    {
        OnStructuralFailure.Broadcast();
        
        if (bEnableStructuralFailurePropagation)
        {
            PropagateStructuralFailure(StructureID);
        }
    }
}

TArray<FString> UGeologicalPhysicsSystem::GetStructuresInRadius(const FVector& Center, float Radius) const
{
    TArray<FString> NearbyStructures;
    
    for (const FGeologicalStructure& Structure : GeologicalStructures)
    {
        float Distance = FVector::Dist(Center, Structure.CenterLocation);
        if (Distance <= Radius)
        {
            NearbyStructures.Add(Structure.StructureID);
        }
    }
    
    return NearbyStructures;
}

void UGeologicalPhysicsSystem::InitiateRockfall(const FRockfallData& RockfallData)
{
    if (ActiveRockfalls.Num() >= MaxActiveRockfalls)
    {
        // Remove oldest rockfall to make room
        ActiveRockfalls.RemoveAt(0);
    }

    ActiveRockfalls.Add(RockfallData);
    OnRockfallInitiated.Broadcast();

    // Spawn visual debris if enabled
    if (bUseDetailedPhysics)
    {
        SpawnDebrisActor(RockfallData.Origin, RockfallData.Volume * 2650.0f, // Assume rock density
                        RockfallData.Direction * RockfallData.InitialVelocity);
    }
}

void UGeologicalPhysicsSystem::UpdateRockfallPhysics(int32 RockfallIndex, float DeltaTime)
{
    if (RockfallIndex < 0 || RockfallIndex >= ActiveRockfalls.Num())
    {
        return;
    }

    FRockfallData& Rockfall = ActiveRockfalls[RockfallIndex];
    
    // Calculate current position based on physics
    FVector CurrentVelocity = Rockfall.Direction * Rockfall.InitialVelocity;
    
    // Apply gravity
    CurrentVelocity += FVector(0.0f, 0.0f, -980.0f) * DeltaTime; // cm/s²
    
    // Apply air resistance
    if (bUseDetailedPhysics)
    {
        FVector DragForce = CalculateRockfallDrag(CurrentVelocity, 
                                                 FMath::Pow(Rockfall.Volume, 0.67f), // Approximate cross-sectional area
                                                 Rockfall.AirResistance);
        float Mass = Rockfall.Volume * 2650.0f * 0.001f; // kg (rock density)
        CurrentVelocity += (DragForce / Mass) * DeltaTime * 100.0f; // Convert to cm/s²
    }
    
    // Update position
    FVector NewPosition = Rockfall.Origin + CurrentVelocity * DeltaTime;
    
    // Check for collision
    FVector CollisionPosition, CollisionVelocity;
    if (CheckRockfallCollision(NewPosition, CurrentVelocity, CollisionPosition, CollisionVelocity))
    {
        // Handle bounce
        Rockfall.Origin = CollisionPosition;
        Rockfall.InitialVelocity = CollisionVelocity.Size();
        Rockfall.Direction = CollisionVelocity.GetSafeNormal();
        
        // Add bounce point for trajectory tracking
        Rockfall.BouncePoints.Add(CollisionPosition);
        
        // Reduce energy due to impact
        Rockfall.InitialVelocity *= Rockfall.CoefficientOfRestitution;
        
        // Stop if velocity becomes too low
        if (Rockfall.InitialVelocity < 100.0f) // 1 m/s
        {
            // Rockfall has stopped - remove from active list
            ActiveRockfalls.RemoveAt(RockfallIndex);
            return;
        }
    }
    else
    {
        Rockfall.Origin = NewPosition;
        Rockfall.Direction = CurrentVelocity.GetSafeNormal();
        Rockfall.InitialVelocity = CurrentVelocity.Size();
    }
    
    // Remove rockfall if it goes too far from origin
    FVector OriginalOrigin = Rockfall.BouncePoints.Num() > 0 ? Rockfall.BouncePoints[0] : Rockfall.Origin;
    if (FVector::Dist(Rockfall.Origin, OriginalOrigin) > MaxSimulationDistance)
    {
        ActiveRockfalls.RemoveAt(RockfallIndex);
    }
}

TArray<FVector> UGeologicalPhysicsSystem::CalculateRockfallTrajectory(const FRockfallData& RockfallData) const
{
    TArray<FVector> Trajectory;
    
    FVector Position = RockfallData.Origin;
    FVector Velocity = RockfallData.Direction * RockfallData.InitialVelocity;
    
    float SimulationTime = 0.0f;
    float TimeStep = 0.1f; // 100ms steps
    float MaxSimTime = 60.0f; // 60 seconds max
    
    Trajectory.Add(Position);
    
    while (SimulationTime < MaxSimTime && Position.Z > -10000.0f) // Stop if goes too deep
    {
        // Apply gravity
        Velocity += FVector(0.0f, 0.0f, -980.0f) * TimeStep;
        
        // Apply air resistance (simplified)
        if (bUseDetailedPhysics)
        {
            float DragMagnitude = Velocity.SizeSquared() * 0.0001f; // Simplified drag
            Velocity -= Velocity.GetSafeNormal() * DragMagnitude * TimeStep;
        }
        
        // Update position
        Position += Velocity * TimeStep;
        Trajectory.Add(Position);
        
        // Check for collision (simplified - would use proper collision detection)
        if (GetTerrainHardness(Position) > 0.5f) // Hit solid ground
        {
            break;
        }
        
        SimulationTime += TimeStep;
    }
    
    return Trajectory;
}

bool UGeologicalPhysicsSystem::CheckRockfallCollision(const FVector& Position, const FVector& Velocity, 
                                                     FVector& OutNewPosition, FVector& OutNewVelocity) const
{
    // Simplified collision detection - in a real implementation this would use proper physics collision
    float TerrainHardness = GetTerrainHardness(Position);
    
    if (TerrainHardness > 0.3f) // Hit terrain
    {
        FVector TerrainNormal = GetTerrainNormal(Position);
        
        // Calculate bounce velocity
        FVector ReflectedVelocity = Velocity - 2.0f * FVector::DotProduct(Velocity, TerrainNormal) * TerrainNormal;
        
        // Apply coefficient of restitution and surface hardness
        float BounceIntensity = CalculateBounceVelocity(Velocity.Size(), 0.3f, TerrainHardness);
        
        OutNewPosition = Position + TerrainNormal * 10.0f; // Move slightly away from surface
        OutNewVelocity = ReflectedVelocity.GetSafeNormal() * BounceIntensity;
        
        return true;
    }
    
    return false;
}

void UGeologicalPhysicsSystem::InitiateAvalanche(const FAvalancheData& AvalancheData)
{
    if (ActiveAvalanches.Num() >= MaxActiveAvalanches)
    {
        // Remove oldest avalanche
        ActiveAvalanches.RemoveAt(0);
    }

    ActiveAvalanches.Add(AvalancheData);
    OnAvalancheInitiated.Broadcast();
}

void UGeologicalPhysicsSystem::UpdateAvalanchePhysics(int32 AvalancheIndex, float DeltaTime)
{
    if (AvalancheIndex < 0 || AvalancheIndex >= ActiveAvalanches.Num())
    {
        return;
    }

    FAvalancheData& Avalanche = ActiveAvalanches[AvalancheIndex];
    
    // Calculate flow dynamics
    CalculateAvalancheFlowDynamics(Avalanche, DeltaTime);
    
    // Update position along flow path
    if (Avalanche.FlowPath.Num() > 1)
    {
        // Move to next point in path based on speed
        float DistanceToTravel = Avalanche.Speed * DeltaTime * 100.0f; // Convert m/s to cm/s
        
        // This would update the avalanche front position
        // Simplified implementation - real system would track the entire avalanche body
    }
    
    // Check if avalanche has stopped
    if (Avalanche.Speed < 1.0f || Avalanche.Volume < 10.0f)
    {
        ActiveAvalanches.RemoveAt(AvalancheIndex);
    }
}

float UGeologicalPhysicsSystem::CalculateAvalancheHazardLevel(const FVector& Location) const
{
    float HazardLevel = 0.0f;
    
    for (const FAvalancheData& Avalanche : ActiveAvalanches)
    {
        // Calculate distance to avalanche path
        float MinDistance = FLT_MAX;
        for (const FVector& PathPoint : Avalanche.FlowPath)
        {
            float Distance = FVector::Dist(Location, PathPoint);
            MinDistance = FMath::Min(MinDistance, Distance);
        }
        
        // Calculate hazard based on distance, volume, and speed
        if (MinDistance < 5000.0f) // Within 50m
        {
            float DistanceFactor = 1.0f - (MinDistance / 5000.0f);
            float VolumeFactor = FMath::Min(Avalanche.Volume / 10000.0f, 1.0f); // Normalize to 10,000 m³
            float SpeedFactor = FMath::Min(Avalanche.Speed / 80.0f, 1.0f); // Normalize to 80 m/s
            
            float AvalancheHazard = DistanceFactor * VolumeFactor * SpeedFactor;
            HazardLevel = FMath::Max(HazardLevel, AvalancheHazard);
        }
    }
    
    return FMath::Clamp(HazardLevel, 0.0f, 1.0f);
}

TArray<FVector> UGeologicalPhysicsSystem::PredictAvalanchePath(const FAvalancheData& AvalancheData) const
{
    TArray<FVector> Path;
    
    FVector CurrentPos = AvalancheData.StartLocation;
    FVector FlowDirection = AvalancheData.FlowDirection;
    
    Path.Add(CurrentPos);
    
    // Trace path downhill
    for (int32 i = 0; i < 100; ++i) // Max 100 path points
    {
        FVector NextPos = CurrentPos + FlowDirection * 1000.0f; // 10m steps
        
        // Update flow direction based on terrain
        FVector TerrainNormal = GetTerrainNormal(NextPos);
        FlowDirection = (FlowDirection - FVector::DotProduct(FlowDirection, TerrainNormal) * TerrainNormal).GetSafeNormal();
        
        // Add gravity component
        FlowDirection += FVector(0.0f, 0.0f, -1.0f) * 0.1f;
        FlowDirection = FlowDirection.GetSafeNormal();
        
        Path.Add(NextPos);
        CurrentPos = NextPos;
        
        // Stop if slope becomes too gentle
        float SlopeAngle = GetSlopeAngle(NextPos);
        if (SlopeAngle < 15.0f) // Less than 15 degrees
        {
            break;
        }
    }
    
    return Path;
}

void UGeologicalPhysicsSystem::InitiateEarthquake(const FSeismicData& SeismicData)
{
    CurrentSeismicActivity = SeismicData;
    
    // Initialize seismic waves
    ActiveSeismicWaves.Empty();
    
    FSeismicWave PWave;
    PWave.Origin = SeismicData.Epicenter;
    PWave.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    PWave.Magnitude = SeismicData.Magnitude;
    PWave.Type = ESeismicWaveType::Primary;
    PWave.bActive = true;
    ActiveSeismicWaves.Add(PWave);
    
    FSeismicWave SWave;
    SWave.Origin = SeismicData.Epicenter;
    SWave.StartTime = PWave.StartTime;
    SWave.Magnitude = SeismicData.Magnitude;
    SWave.Type = ESeismicWaveType::Secondary;
    SWave.bActive = true;
    ActiveSeismicWaves.Add(SWave);
    
    OnEarthquakeStart.Broadcast();
    
    // Apply immediate stress to nearby geological structures
    TArray<FString> NearbyStructures = GetStructuresInRadius(SeismicData.Epicenter, SeismicData.Magnitude * 10000.0f);
    for (const FString& StructureID : NearbyStructures)
    {
        float Distance = 0.0f;
        const FGeologicalStructure* Structure = GeologicalStructures.FindByPredicate([&StructureID](const FGeologicalStructure& S)
        {
            return S.StructureID == StructureID;
        });
        
        if (Structure)
        {
            Distance = FVector::Dist(SeismicData.Epicenter, Structure->CenterLocation) * 0.01f; // Convert to meters
        }
        
        float SeismicStress = SeismicData.Magnitude * 0.1f / FMath::Max(1.0f, Distance * 0.1f);
        ApplyStressToStructure(StructureID, SeismicStress);
    }
}

void UGeologicalPhysicsSystem::UpdateSeismicActivity(float DeltaTime)
{
    if (CurrentSeismicActivity.Magnitude <= 0.0f)
    {
        return; // No active earthquake
    }

    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    // Update duration and check if earthquake is over
    CurrentSeismicActivity.Duration -= DeltaTime;
    if (CurrentSeismicActivity.Duration <= 0.0f)
    {
        CurrentSeismicActivity.Magnitude = 0.0f;
        ActiveSeismicWaves.Empty();
        return;
    }
    
    // Propagate seismic waves
    PropagateSeismicWaves(DeltaTime);
    
    // Apply ongoing seismic effects to structures and players
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
            float SeismicIntensity = CalculateSeismicIntensity(PlayerLocation);
            
            if (SeismicIntensity > 0.1f)
            {
                UAdvancedClimbingComponent* ClimbingComp = PC->GetPawn()->FindComponentByClass<UAdvancedClimbingComponent>();
                if (ClimbingComp)
                {
                    ApplyGeologicalEffectsToClimber(ClimbingComp, DeltaTime);
                }
            }
        }
    }
}

float UGeologicalPhysicsSystem::CalculateSeismicIntensity(const FVector& Location) const
{
    if (CurrentSeismicActivity.Magnitude <= 0.0f)
    {
        return 0.0f;
    }

    float Distance = FVector::Dist(Location, CurrentSeismicActivity.Epicenter) * 0.01f; // Convert to meters
    
    // Apply attenuation with distance
    float Intensity = CurrentSeismicActivity.Magnitude * FMath::Exp(-CurrentSeismicActivity.AttenuationFactor * Distance);
    
    // Apply depth factor
    float DepthFactor = FMath::Sqrt(CurrentSeismicActivity.Depth * 0.01f) * 0.1f; // Convert depth to meters
    Intensity /= (1.0f + DepthFactor);
    
    // Add wave interference effects
    float WaveIntensity = 0.0f;
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    for (const FSeismicWave& Wave : ActiveSeismicWaves)
    {
        if (!Wave.bActive)
        {
            continue;
        }
        
        float WaveAmplitude = CalculateSeismicWaveAmplitude(Location, Wave.Type, CurrentTime - Wave.StartTime);
        WaveIntensity += WaveAmplitude;
    }
    
    Intensity *= (1.0f + WaveIntensity);
    
    return FMath::Clamp(Intensity, 0.0f, 10.0f); // Clamp to reasonable range
}

FVector UGeologicalPhysicsSystem::CalculateSeismicForce(const FVector& Location, float Mass) const
{
    float Intensity = CalculateSeismicIntensity(Location);
    
    if (Intensity < 0.1f)
    {
        return FVector::ZeroVector;
    }
    
    // Generate pseudo-random seismic motion
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    float FrequencyMultiplier = FMath::Sin(CurrentTime * 10.0f * Intensity) + 
                               FMath::Sin(CurrentTime * 15.0f * Intensity) * 0.5f +
                               FMath::Sin(CurrentTime * 25.0f * Intensity) * 0.25f;
    
    // Create realistic seismic motion (combination of horizontal and vertical components)
    FVector SeismicForce = FVector(
        FMath::Sin(CurrentTime * 8.0f + Location.X * 0.001f) * Intensity,
        FMath::Sin(CurrentTime * 12.0f + Location.Y * 0.001f) * Intensity,
        FMath::Sin(CurrentTime * 6.0f + Location.Z * 0.001f) * Intensity * 0.5f // Less vertical motion
    );
    
    SeismicForce *= FrequencyMultiplier * Mass * 100.0f; // Scale for UE4 units
    
    return SeismicForce;
}

void UGeologicalPhysicsSystem::ApplyGeologicalEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime)
{
    if (!ClimbingComponent)
    {
        return;
    }

    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    
    // Seismic effects
    float SeismicIntensity = CalculateSeismicIntensity(ClimberLocation);
    if (SeismicIntensity > 0.1f)
    {
        // Increased stamina drain due to maintaining balance during shaking
        float SeismicStaminaDrain = SeismicIntensity * 20.0f * DeltaTime;
        ClimbingComponent->ConsumeStamina(SeismicStaminaDrain);
        
        // Reduced grip strength due to vibrations
        float SeismicGripReduction = SeismicIntensity * 15.0f * DeltaTime;
        ClimbingComponent->ConsumeGripStrength(SeismicGripReduction);
        
        // Movement precision reduction
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - SeismicIntensity * 0.3f);
        
        // Apply seismic force to character
        if (ClimbingComponent->GetOwner()->GetRootComponent())
        {
            UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ClimbingComponent->GetOwner()->GetRootComponent());
            if (PrimComp)
            {
                FVector SeismicForce = CalculateSeismicForce(ClimberLocation, PrimComp->GetMass());
                PrimComp->AddForce(SeismicForce);
            }
        }
    }
    
    // Rockfall hazard effects
    float RockfallRisk = PredictRockfallProbability(ClimberLocation);
    if (RockfallRisk > 0.3f)
    {
        // Increased alertness and caution
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - RockfallRisk * 0.2f);
    }
    
    // Avalanche hazard effects
    float AvalancheHazard = CalculateAvalancheHazardLevel(ClimberLocation);
    if (AvalancheHazard > 0.2f)
    {
        // Panic response - increased stamina drain, reduced precision
        ClimbingComponent->ConsumeStamina(AvalancheHazard * 30.0f * DeltaTime);
        ClimbingComponent->Settings.GripRecoveryRate *= (1.0f - AvalancheHazard * 0.5f);
    }
}

void UGeologicalPhysicsSystem::ApplyGeologicalEffectsToRope(UAdvancedRopeComponent* Rope, float DeltaTime)
{
    if (!Rope)
    {
        return;
    }

    TArray<FVector> SegmentPositions = Rope->GetRopeSegmentPositions();
    
    // Apply seismic forces to rope segments
    float MaxSeismicIntensity = 0.0f;
    for (const FVector& Position : SegmentPositions)
    {
        float SeismicIntensity = CalculateSeismicIntensity(Position);
        MaxSeismicIntensity = FMath::Max(MaxSeismicIntensity, SeismicIntensity);
    }
    
    if (MaxSeismicIntensity > 0.1f)
    {
        // Seismic vibrations increase rope tension and wear
        float SeismicTensionIncrease = MaxSeismicIntensity * 1000.0f; // Additional Newtons
        Rope->PhysicsState.CurrentTension += SeismicTensionIncrease;
        
        // Accelerated wear due to vibrations
        float SeismicWear = MaxSeismicIntensity * DeltaTime * 0.01f;
        // Rope->ProcessVibrationWear(SeismicWear);
        
        // Modify rope physics properties during earthquake
        if (Rope->CableComponent)
        {
            // Increase simulation substeps for more accurate physics during shaking
            Rope->CableComponent->SubstepTime = FMath::Max(Rope->CableComponent->SubstepTime * 0.5f, 0.001f);
            
            // Visual vibration effect
            float VibrationAmplitude = MaxSeismicIntensity * 2.0f;
            Rope->CableComponent->CableWidth *= (1.0f + VibrationAmplitude * FMath::Sin(GetWorld()->GetTimeSeconds() * 30.0f));
        }
    }
    
    // Check for rockfall hitting rope
    for (const FRockfallData& Rockfall : ActiveRockfalls)
    {
        for (const FVector& SegmentPos : SegmentPositions)
        {
            if (FVector::Dist(Rockfall.Origin, SegmentPos) < 50.0f) // 50cm collision radius
            {
                // Rockfall hit rope - apply damage and force
                float ImpactForce = Rockfall.Volume * 2650.0f * Rockfall.InitialVelocity * 0.01f; // Simplified impact calculation
                
                // This would damage the rope
                // Rope->ApplyImpactDamage(ImpactForce, SegmentPos);
                
                // Apply force to rope segment
                if (Rope->CableComponent)
                {
                    // Visual effect - rope recoils from impact
                    Rope->CableComponent->bEnableStiffness = false;
                    // Would need custom physics to apply localized force
                }
                
                break;
            }
        }
    }
}

bool UGeologicalPhysicsSystem::IsLocationSafeForClimbing(const FVector& Location) const
{
    // Check various geological hazards
    float RockfallRisk = PredictRockfallProbability(Location);
    float AvalancheRisk = PredictAvalancheProbability(Location);
    float SeismicIntensity = CalculateSeismicIntensity(Location);
    
    // Check structural stability of nearby structures
    TArray<FString> NearbyStructures = GetStructuresInRadius(Location, 1000.0f); // Within 10m
    for (const FString& StructureID : NearbyStructures)
    {
        EStructuralStability Stability = AnalyzeStructuralStability(StructureID);
        if (Stability == EStructuralStability::Critical || Stability == EStructuralStability::VeryUnstable)
        {
            return false;
        }
    }
    
    // Location is unsafe if any hazard is above threshold
    return (RockfallRisk < 0.5f && AvalancheRisk < 0.3f && SeismicIntensity < 3.0f);
}

void UGeologicalPhysicsSystem::ProcessWeatheringEffects(float DeltaTime)
{
    if (CurrentLOD > 1) // Skip detailed weathering in lower LOD
    {
        return;
    }

    for (FGeologicalStructure& Structure : GeologicalStructures)
    {
        // Apply weathering based on rock type and environmental conditions
        float WeatheringAmount = Structure.RockProperties.WeatheringRate * WeatheringRate * DeltaTime / 86400.0f; // Per day to per second
        
        // Environmental factors from weather system
        if (HazardManager)
        {
            float Temperature = HazardManager->GetTemperatureAtLocation(Structure.CenterLocation);
            float Wetness = HazardManager->GetWetnessAtLocation(Structure.CenterLocation);
            
            // Freeze-thaw cycles increase weathering
            if (Temperature < 5.0f && Temperature > -5.0f && Wetness > 0.3f)
            {
                WeatheringAmount *= 2.0f; // Freeze-thaw is very destructive
            }
            
            // Chemical weathering increases with temperature and moisture
            float ChemicalWeatheringFactor = (Temperature + 20.0f) * 0.05f * (Wetness + 0.1f);
            WeatheringAmount *= ChemicalWeatheringFactor;
        }
        
        // Apply weathering as gradual stress accumulation
        ApplyStressToStructure(Structure.StructureID, WeatheringAmount * 0.1f);
        
        // Reduce rock strength over time
        Structure.RockProperties.CompressiveStrength *= (1.0f - WeatheringAmount * 0.001f);
        Structure.RockProperties.TensileStrength *= (1.0f - WeatheringAmount * 0.002f);
        Structure.RockProperties.FractureResistance *= (1.0f - WeatheringAmount * 0.001f);
        
        // Ensure values don't go below minimum thresholds
        Structure.RockProperties.CompressiveStrength = FMath::Max(Structure.RockProperties.CompressiveStrength, 10.0f);
        Structure.RockProperties.TensileStrength = FMath::Max(Structure.RockProperties.TensileStrength, 1.0f);
        Structure.RockProperties.FractureResistance = FMath::Max(Structure.RockProperties.FractureResistance, 0.1f);
    }
}

void UGeologicalPhysicsSystem::ApplyThermalStressToStructures(float TemperatureChange)
{
    if (FMath::Abs(TemperatureChange) < 5.0f)
    {
        return; // Only significant temperature changes cause thermal stress
    }

    for (const FGeologicalStructure& Structure : GeologicalStructures)
    {
        // Calculate thermal stress based on rock type
        float ThermalExpansionCoeff = 0.000008f; // Default for rock
        
        switch (Structure.RockProperties.Type)
        {
        case ERockType::Granite:
            ThermalExpansionCoeff = 0.000008f;
            break;
        case ERockType::Limestone:
            ThermalExpansionCoeff = 0.000006f;
            break;
        case ERockType::Sandstone:
            ThermalExpansionCoeff = 0.000012f;
            break;
        case ERockType::Basalt:
            ThermalExpansionCoeff = 0.000005f;
            break;
        default:
            ThermalExpansionCoeff = 0.000008f;
            break;
        }
        
        // Calculate thermal stress
        float ThermalStrain = ThermalExpansionCoeff * TemperatureChange;
        float ThermalStress = ThermalStrain * Structure.RockProperties.YoungsModulus * 0.000001f; // Convert to stress units
        
        // Apply thermal stress
        ApplyStressToStructure(Structure.StructureID, ThermalStress * 0.01f); // Scale for game units
    }
}

void UGeologicalPhysicsSystem::ProcessFreezeThawCycles(float Temperature)
{
    static float LastTemperature = Temperature;
    
    // Detect freeze-thaw transitions
    bool bFreezingNow = Temperature < 0.0f;
    bool bWasFreezingBefore = LastTemperature < 0.0f;
    
    if (bFreezingNow != bWasFreezingBefore) // Temperature crossed freezing point
    {
        // Apply freeze-thaw stress to all structures
        for (const FGeologicalStructure& Structure : GeologicalStructures)
        {
            // Check if there's moisture present (would come from weather system)
            float MoistureLevel = 0.1f; // Default assumption
            if (HazardManager)
            {
                MoistureLevel = HazardManager->GetWetnessAtLocation(Structure.CenterLocation);
            }
            
            if (MoistureLevel > 0.2f) // Sufficient moisture for freeze-thaw
            {
                // Water expansion during freezing creates significant stress
                float FreezeThawStress = MoistureLevel * 0.1f; // 9% expansion of water when freezing
                ApplyStressToStructure(Structure.StructureID, FreezeThawStress);
            }
        }
    }
    
    LastTemperature = Temperature;
}

float UGeologicalPhysicsSystem::PredictRockfallProbability(const FVector& Location) const
{
    float Probability = 0.0f;
    
    // Check slope angle
    float SlopeAngle = GetSlopeAngle(Location);
    if (SlopeAngle > 30.0f) // Steep slopes are more prone to rockfall
    {
        Probability += (SlopeAngle - 30.0f) * 0.01f;
    }
    
    // Check nearby structural instability
    TArray<FString> NearbyStructures = GetStructuresInRadius(Location, 5000.0f); // Within 50m
    for (const FString& StructureID : NearbyStructures)
    {
        EStructuralStability Stability = const_cast<UGeologicalPhysicsSystem*>(this)->AnalyzeStructuralStability(StructureID);
        
        switch (Stability)
        {
        case EStructuralStability::Critical:
            Probability += 0.8f;
            break;
        case EStructuralStability::VeryUnstable:
            Probability += 0.5f;
            break;
        case EStructuralStability::Unstable:
            Probability += 0.2f;
            break;
        case EStructuralStability::SlightlyUnstable:
            Probability += 0.05f;
            break;
        default:
            break;
        }
    }
    
    // Seismic activity increases rockfall probability
    float SeismicIntensity = CalculateSeismicIntensity(Location);
    Probability += SeismicIntensity * 0.1f;
    
    // Weather factors (if available)
    if (HazardManager)
    {
        float Temperature = HazardManager->GetTemperatureAtLocation(Location);
        float Wetness = HazardManager->GetWetnessAtLocation(Location);
        
        // Freeze-thaw conditions increase rockfall risk
        if (Temperature < 5.0f && Temperature > -5.0f && Wetness > 0.3f)
        {
            Probability += 0.3f;
        }
        
        // Heavy rain can trigger rockfall
        if (Wetness > 0.7f)
        {
            Probability += 0.2f;
        }
    }
    
    return FMath::Clamp(Probability, 0.0f, 1.0f);
}

float UGeologicalPhysicsSystem::PredictAvalancheProbability(const FVector& Location) const
{
    float Probability = 0.0f;
    
    // Avalanches require snow - check for snow conditions
    float SnowProbability = 0.0f; // Would get from weather system
    if (HazardManager)
    {
        float Temperature = HazardManager->GetTemperatureAtLocation(Location);
        if (Temperature < 2.0f) // Snow conditions
        {
            SnowProbability = 0.8f;
        }
    }
    
    if (SnowProbability < 0.3f)
    {
        return 0.0f; // No snow, no avalanche
    }
    
    // Check slope angle (avalanches occur on 30-45 degree slopes typically)
    float SlopeAngle = GetSlopeAngle(Location);
    if (SlopeAngle >= 30.0f && SlopeAngle <= 50.0f)
    {
        float OptimalSlopeRange = 1.0f - FMath::Abs(SlopeAngle - 37.5f) / 12.5f;
        Probability += OptimalSlopeRange * 0.5f;
    }
    
    // Temperature changes can trigger avalanches
    if (HazardManager)
    {
        float Temperature = HazardManager->GetTemperatureAtLocation(Location);
        if (Temperature > -5.0f && Temperature < 2.0f) // Warming conditions
        {
            Probability += 0.3f;
        }
    }
    
    // Seismic activity can trigger avalanches
    float SeismicIntensity = CalculateSeismicIntensity(Location);
    if (SeismicIntensity > 2.0f)
    {
        Probability += 0.6f;
    }
    
    return FMath::Clamp(Probability * SnowProbability, 0.0f, 1.0f);
}

TArray<FVector> UGeologicalPhysicsSystem::IdentifyHazardZones(float HazardThreshold) const
{
    TArray<FVector> HazardZones;
    
    // Sample locations around registered structures
    for (const FGeologicalStructure& Structure : GeologicalStructures)
    {
        // Sample points around each structure
        for (int32 x = -5; x <= 5; ++x)
        {
            for (int32 y = -5; y <= 5; ++y)
            {
                FVector SampleLocation = Structure.CenterLocation + FVector(x * 1000.0f, y * 1000.0f, 0.0f);
                
                float RockfallRisk = PredictRockfallProbability(SampleLocation);
                float AvalancheRisk = PredictAvalancheProbability(SampleLocation);
                float SeismicRisk = CalculateSeismicIntensity(SampleLocation) * 0.1f;
                
                float TotalHazard = FMath::Max({RockfallRisk, AvalancheRisk, SeismicRisk});
                
                if (TotalHazard >= HazardThreshold)
                {
                    HazardZones.Add(SampleLocation);
                }
            }
        }
    }
    
    return HazardZones;
}

void UGeologicalPhysicsSystem::SpawnDebrisActor(const FVector& Location, float Mass, const FVector& InitialVelocity)
{
    if (!GetWorld() || SpawnedDebris.Num() >= MaxDebrisObjects)
    {
        return;
    }

    // Create debris actor
    AStaticMeshActor* DebrisActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass());
    if (!DebrisActor)
    {
        return;
    }

    // Set up debris properties
    DebrisActor->SetActorLocation(Location);
    
    if (UStaticMeshComponent* MeshComp = DebrisActor->GetStaticMeshComponent())
    {
        // Enable physics simulation
        MeshComp->SetSimulatePhysics(true);
        MeshComp->SetMassOverrideInKg(NAME_None, Mass);
        MeshComp->SetLinearVelocity(InitialVelocity);
        
        // Set collision properties
        MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
    }
    
    SpawnedDebris.Add(DebrisActor);
}

void UGeologicalPhysicsSystem::CleanupDebrisActors()
{
    for (int32 i = SpawnedDebris.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(SpawnedDebris[i]))
        {
            SpawnedDebris.RemoveAt(i);
            continue;
        }
        
        // Remove debris that has moved too far or stopped moving
        FVector DebrisLocation = SpawnedDebris[i]->GetActorLocation();
        FVector DebrisVelocity = SpawnedDebris[i]->GetVelocity();
        
        if (FVector::Dist(DebrisLocation, GetOwner()->GetActorLocation()) > MaxSimulationDistance ||
            DebrisVelocity.Size() < 10.0f) // Less than 10 cm/s
        {
            SpawnedDebris[i]->Destroy();
            SpawnedDebris.RemoveAt(i);
        }
    }
}

void UGeologicalPhysicsSystem::SetMaxDebrisCount(int32 MaxCount)
{
    MaxDebrisObjects = FMath::Max(1, MaxCount);
    
    // Remove excess debris if necessary
    while (SpawnedDebris.Num() > MaxDebrisObjects)
    {
        if (IsValid(SpawnedDebris[0]))
        {
            SpawnedDebris[0]->Destroy();
        }
        SpawnedDebris.RemoveAt(0);
    }
}

void UGeologicalPhysicsSystem::SetGeologicalLOD(int32 LODLevel)
{
    CurrentLOD = FMath::Clamp(LODLevel, 0, 3);
    
    // Adjust simulation parameters based on LOD
    switch (CurrentLOD)
    {
    case 0: // High detail
        StructuralUpdateInterval = 0.5f;  // 2Hz
        DebrisUpdateInterval = 0.05f;     // 20Hz
        SeismicUpdateInterval = 0.02f;    // 50Hz
        bUseDetailedPhysics = true;
        MaxActiveRockfalls = 20;
        MaxActiveAvalanches = 5;
        break;
    case 1: // Medium detail
        StructuralUpdateInterval = 1.0f;  // 1Hz
        DebrisUpdateInterval = 0.1f;      // 10Hz
        SeismicUpdateInterval = 0.05f;    // 20Hz
        bUseDetailedPhysics = true;
        MaxActiveRockfalls = 10;
        MaxActiveAvalanches = 3;
        break;
    case 2: // Low detail
        StructuralUpdateInterval = 2.0f;  // 0.5Hz
        DebrisUpdateInterval = 0.2f;      // 5Hz
        SeismicUpdateInterval = 0.1f;     // 10Hz
        bUseDetailedPhysics = false;
        MaxActiveRockfalls = 5;
        MaxActiveAvalanches = 2;
        break;
    case 3: // Minimal detail
        StructuralUpdateInterval = 5.0f;  // 0.2Hz
        DebrisUpdateInterval = 0.5f;      // 2Hz
        SeismicUpdateInterval = 0.2f;     // 5Hz
        bUseDetailedPhysics = false;
        MaxActiveRockfalls = 2;
        MaxActiveAvalanches = 1;
        break;
    }
    
    // Cleanup excess events if needed
    while (ActiveRockfalls.Num() > MaxActiveRockfalls)
    {
        ActiveRockfalls.RemoveAt(0);
    }
    while (ActiveAvalanches.Num() > MaxActiveAvalanches)
    {
        ActiveAvalanches.RemoveAt(0);
    }
}

void UGeologicalPhysicsSystem::OptimizeGeologicalSimulation(float ViewerDistance)
{
    if (ViewerDistance < 2000.0f) // 20m
    {
        SetGeologicalLOD(0);
    }
    else if (ViewerDistance < 5000.0f) // 50m
    {
        SetGeologicalLOD(1);
    }
    else if (ViewerDistance < 10000.0f) // 100m
    {
        SetGeologicalLOD(2);
    }
    else
    {
        SetGeologicalLOD(3);
    }
}

// Protected implementation functions
void UGeologicalPhysicsSystem::UpdateAllGeologicalEvents(float DeltaTime)
{
    // Update rockfalls
    for (int32 i = ActiveRockfalls.Num() - 1; i >= 0; --i)
    {
        UpdateRockfallPhysics(i, DeltaTime);
    }
    
    // Update avalanches
    for (int32 i = ActiveAvalanches.Num() - 1; i >= 0; --i)
    {
        UpdateAvalanchePhysics(i, DeltaTime);
    }
    
    // Cleanup debris
    if (CurrentLOD <= 2)
    {
        CleanupDebrisActors();
    }
}

void UGeologicalPhysicsSystem::ProcessStructuralStress(float DeltaTime)
{
    // Process stress accumulation and structural changes
    for (FGeologicalStructure& Structure : GeologicalStructures)
    {
        // Natural stress relief over time
        Structure.StressAccumulation *= FMath::Max(0.0f, 1.0f - DeltaTime * 0.01f);
        
        // Update stability analysis
        AnalyzeStructuralStability(Structure.StructureID);
    }
}

void UGeologicalPhysicsSystem::UpdateDebrisPhysics(float DeltaTime)
{
    // Update physics for spawned debris actors
    for (int32 i = SpawnedDebris.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(SpawnedDebris[i]))
        {
            SpawnedDebris.RemoveAt(i);
            continue;
        }
        
        UStaticMeshComponent* MeshComp = SpawnedDebris[i]->GetStaticMeshComponent();
        if (MeshComp && MeshComp->IsSimulatingPhysics())
        {
            // Apply additional forces like wind resistance
            FVector Velocity = MeshComp->GetPhysicsLinearVelocity();
            if (Velocity.Size() > 100.0f) // Only apply drag to fast-moving debris
            {
                FVector DragForce = -Velocity.GetSafeNormal() * Velocity.SizeSquared() * 0.0001f;
                MeshComp->AddForce(DragForce);
            }
        }
    }
}

void UGeologicalPhysicsSystem::CheckHazardZoneProximity()
{
    if (!GetWorld())
    {
        return;
    }

    // Check player proximity to hazard zones
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
            
            // Check proximity to active geological events
            for (const FRockfallData& Rockfall : ActiveRockfalls)
            {
                if (FVector::Dist(PlayerLocation, Rockfall.Origin) < HazardZoneProximityDistance)
                {
                    OnHazardZoneEntered.Broadcast();
                    break;
                }
            }
            
            for (const FAvalancheData& Avalanche : ActiveAvalanches)
            {
                if (FVector::Dist(PlayerLocation, Avalanche.StartLocation) < HazardZoneProximityDistance * 3.0f) // Larger radius for avalanches
                {
                    OnHazardZoneEntered.Broadcast();
                    break;
                }
            }
            
            // Check seismic intensity
            if (CalculateSeismicIntensity(PlayerLocation) > 2.0f)
            {
                OnHazardZoneEntered.Broadcast();
            }
        }
    }
}

FVector UGeologicalPhysicsSystem::CalculateRockfallDrag(const FVector& Velocity, float CrossSectionalArea, float DragCoeff) const
{
    if (Velocity.SizeSquared() < 0.01f)
    {
        return FVector::ZeroVector;
    }
    
    float AirDensity = 1.225f; // kg/m³ at sea level
    float DragMagnitude = 0.5f * AirDensity * Velocity.SizeSquared() * DragCoeff * CrossSectionalArea * 0.01f; // Scale for units
    
    return -Velocity.GetSafeNormal() * DragMagnitude;
}

float UGeologicalPhysicsSystem::CalculateBounceVelocity(float IncomingVelocity, float Restitution, float SurfaceHardness) const
{
    return IncomingVelocity * Restitution * SurfaceHardness;
}

FVector UGeologicalPhysicsSystem::CalculateRollingDeceleration(const FVector& Velocity, float Friction, float SlopeAngle) const
{
    if (Velocity.SizeSquared() < 0.01f)
    {
        return FVector::ZeroVector;
    }
    
    float RollingResistance = Friction * 980.0f * FMath::Cos(FMath::DegreesToRadians(SlopeAngle)); // cm/s²
    float GravityComponent = 980.0f * FMath::Sin(FMath::DegreesToRadians(SlopeAngle)); // cm/s²
    
    FVector NetDeceleration = -Velocity.GetSafeNormal() * RollingResistance;
    NetDeceleration.Z -= GravityComponent; // Add gravity component
    
    return NetDeceleration;
}

void UGeologicalPhysicsSystem::CalculateAvalancheFlowDynamics(FAvalancheData& AvalancheData, float DeltaTime)
{
    // Simplified avalanche flow model
    FVector FlowDirection = AvalancheData.FlowDirection;
    
    // Calculate acceleration due to gravity and terrain
    float SlopeAngle = GetSlopeAngle(AvalancheData.StartLocation);
    float GravityAcceleration = 9.8f * FMath::Sin(FMath::DegreesToRadians(SlopeAngle)); // m/s²
    
    // Apply flow resistance based on avalanche type
    float FlowResistance = 0.1f; // Base resistance
    switch (AvalancheData.Type)
    {
    case EAvalancheType::SlabAvalanche:
        FlowResistance = 0.05f; // Low resistance, high speed
        break;
    case EAvalancheType::LooseSnow:
        FlowResistance = 0.15f; // Higher resistance
        break;
    case EAvalancheType::WetAvalanche:
        FlowResistance = 0.2f; // High resistance due to wet snow
        break;
    case EAvalancheType::PowderAvalanche:
        FlowResistance = 0.03f; // Very low resistance
        break;
    case EAvalancheType::IceAvalanche:
        FlowResistance = 0.08f; // Low resistance, hard impacts
        break;
    }
    
    // Calculate net acceleration
    float NetAcceleration = GravityAcceleration - (AvalancheData.Speed * FlowResistance);
    
    // Update speed
    AvalancheData.Speed += NetAcceleration * DeltaTime;
    AvalancheData.Speed = FMath::Clamp(AvalancheData.Speed, 0.0f, AvalancheData.MaxSpeed);
    
    // Entrainment - avalanche picks up more material as it flows
    float EntrainmentAmount = AvalancheData.EntrainmentRate * AvalancheData.Speed * DeltaTime;
    AvalancheData.Volume += EntrainmentAmount;
    
    // Update flow direction based on terrain (simplified)
    FVector TerrainNormal = GetTerrainNormal(AvalancheData.StartLocation);
    AvalancheData.FlowDirection = (FlowDirection + FVector(0.0f, 0.0f, -1.0f) * 0.1f).GetSafeNormal();
}

float UGeologicalPhysicsSystem::CalculateSeismicWaveAmplitude(const FVector& Location, ESeismicWaveType WaveType, float Time) const
{
    float Distance = FVector::Dist(Location, CurrentSeismicActivity.Epicenter) * 0.01f; // Convert to meters
    
    // Calculate wave velocity
    float WaveVelocity = 0.0f;
    switch (WaveType)
    {
    case ESeismicWaveType::Primary:
        WaveVelocity = CurrentSeismicActivity.PWaveVelocity;
        break;
    case ESeismicWaveType::Secondary:
        WaveVelocity = CurrentSeismicActivity.SWaveVelocity;
        break;
    case ESeismicWaveType::Love:
    case ESeismicWaveType::Rayleigh:
        WaveVelocity = CurrentSeismicActivity.SWaveVelocity * 0.9f; // Surface waves are slightly slower
        break;
    }
    
    // Calculate wave arrival time
    float ArrivalTime = Distance / WaveVelocity;
    
    if (Time < ArrivalTime)
    {
        return 0.0f; // Wave hasn't arrived yet
    }
    
    // Calculate amplitude with distance attenuation
    float BaseAmplitude = CurrentSeismicActivity.Magnitude;
    float DistanceAttenuation = FMath::Exp(-CurrentSeismicActivity.AttenuationFactor * Distance);
    
    // Apply time-based amplitude modulation
    float TimeSinceArrival = Time - ArrivalTime;
    float TimeAttenuation = FMath::Exp(-TimeSinceArrival * 0.1f); // Amplitude decays over time
    
    return BaseAmplitude * DistanceAttenuation * TimeAttenuation;
}

void UGeologicalPhysicsSystem::PropagateSeismicWaves(float DeltaTime)
{
    // Update active seismic waves
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    for (FSeismicWave& Wave : ActiveSeismicWaves)
    {
        if (!Wave.bActive)
        {
            continue;
        }
        
        float WaveAge = CurrentTime - Wave.StartTime;
        
        // Deactivate waves that have been active too long
        if (WaveAge > CurrentSeismicActivity.Duration)
        {
            Wave.bActive = false;
        }
    }
}

void UGeologicalPhysicsSystem::CalculateStructuralLoads()
{
    // Calculate load distribution among connected structures
    for (FGeologicalStructure& Structure : GeologicalStructures)
    {
        float TotalLoad = Structure.LoadBearing;
        
        // Distribute load among connected structures
        for (const FString& ConnectedID : Structure.ConnectedStructures)
        {
            FGeologicalStructure* ConnectedStructure = GeologicalStructures.FindByPredicate([&ConnectedID](const FGeologicalStructure& S)
            {
                return S.StructureID == ConnectedID;
            });
            
            if (ConnectedStructure)
            {
                // Simple load sharing - equal distribution
                float SharedLoad = TotalLoad / (Structure.ConnectedStructures.Num() + 1);
                ApplyStressToStructure(ConnectedStructure->StructureID, SharedLoad * 0.1f);
            }
        }
    }
}

void UGeologicalPhysicsSystem::PropagateStructuralFailure(const FString& FailedStructureID)
{
    FGeologicalStructure* FailedStructure = GeologicalStructures.FindByPredicate([&FailedStructureID](const FGeologicalStructure& S)
    {
        return S.StructureID == FailedStructureID;
    });

    if (!FailedStructure)
    {
        return;
    }

    // Redistribute load from failed structure to connected structures
    float RedistributedLoad = FailedStructure->LoadBearing;
    
    for (const FString& ConnectedID : FailedStructure->ConnectedStructures)
    {
        float AdditionalStress = RedistributedLoad / FailedStructure->ConnectedStructures.Num();
        ApplyStressToStructure(ConnectedID, AdditionalStress);
        
        // Potentially trigger cascading failures
        EStructuralStability ConnectedStability = AnalyzeStructuralStability(ConnectedID);
        if (ConnectedStability == EStructuralStability::Critical)
        {
            // Cascading failure with some probability
            if (FMath::RandRange(0.0f, 1.0f) < 0.3f)
            {
                PropagateStructuralFailure(ConnectedID);
            }
        }
    }
    
    // Trigger rockfall from failed structure
    FRockfallData FailureRockfall;
    FailureRockfall.Origin = FailedStructure->CenterLocation;
    FailureRockfall.Volume = FailedStructure->Bounds.X * FailedStructure->Bounds.Y * FailedStructure->Bounds.Z * 0.000001f; // cm³ to m³
    FailureRockfall.InitialVelocity = 5.0f; // 5 m/s initial velocity
    FailureRockfall.Direction = FVector(0.0f, 0.0f, -1.0f);
    
    InitiateRockfall(FailureRockfall);
}

FVector UGeologicalPhysicsSystem::GetTerrainNormal(const FVector& Location) const
{
    // Simplified terrain normal calculation
    // In a real implementation, this would raycast or query the landscape
    return FVector(0.0f, 0.0f, 1.0f); // Assume flat terrain for now
}

float UGeologicalPhysicsSystem::GetSlopeAngle(const FVector& Location) const
{
    // Use cache if available
    if (bUseTerrainCache)
    {
        if (const float* CachedAngle = SlopeAngleCache.Find(Location))
        {
            return *CachedAngle;
        }
    }
    
    // Simplified slope calculation - would use proper terrain queries in real implementation
    float SlopeAngle = FMath::RandRange(0.0f, 45.0f); // Random for demonstration
    
    // Cache the result
    if (bUseTerrainCache)
    {
        SlopeAngleCache.Add(Location, SlopeAngle);
    }
    
    return SlopeAngle;
}

float UGeologicalPhysicsSystem::GetTerrainHardness(const FVector& Location) const
{
    // Use cache if available
    if (bUseTerrainCache)
    {
        if (const float* CachedHardness = TerrainHardnessCache.Find(Location))
        {
            return *CachedHardness;
        }
    }
    
    // Simplified hardness calculation - would query actual terrain material properties
    float Hardness = 0.8f; // Assume hard rock surface
    
    // Cache the result
    if (bUseTerrainCache)
    {
        TerrainHardnessCache.Add(Location, Hardness);
    }
    
    return Hardness;
}

// Static utility functions
FRockProperties UGeologicalPhysicsSystem::GetDefaultRockProperties(ERockType RockType)
{
    FRockProperties Properties;
    Properties.Type = RockType;
    
    switch (RockType)
    {
    case ERockType::Granite:
        Properties.Density = 2650.0f;
        Properties.CompressiveStrength = 200.0f;
        Properties.TensileStrength = 15.0f;
        Properties.ShearStrength = 25.0f;
        Properties.YoungsModulus = 70000.0f;
        Properties.PoissonsRatio = 0.25f;
        Properties.FractureResistance = 0.9f;
        Properties.WeatheringRate = 0.001f;
        break;
        
    case ERockType::Limestone:
        Properties.Density = 2300.0f;
        Properties.CompressiveStrength = 100.0f;
        Properties.TensileStrength = 8.0f;
        Properties.ShearStrength = 15.0f;
        Properties.YoungsModulus = 50000.0f;
        Properties.PoissonsRatio = 0.3f;
        Properties.FractureResistance = 0.6f;
        Properties.WeatheringRate = 0.01f;
        break;
        
    case ERockType::Sandstone:
        Properties.Density = 2200.0f;
        Properties.CompressiveStrength = 80.0f;
        Properties.TensileStrength = 5.0f;
        Properties.ShearStrength = 12.0f;
        Properties.YoungsModulus = 30000.0f;
        Properties.PoissonsRatio = 0.2f;
        Properties.FractureResistance = 0.5f;
        Properties.WeatheringRate = 0.005f;
        break;
        
    case ERockType::Basalt:
        Properties.Density = 2800.0f;
        Properties.CompressiveStrength = 300.0f;
        Properties.TensileStrength = 20.0f;
        Properties.ShearStrength = 35.0f;
        Properties.YoungsModulus = 90000.0f;
        Properties.PoissonsRatio = 0.28f;
        Properties.FractureResistance = 0.95f;
        Properties.WeatheringRate = 0.0005f;
        break;
        
    default:
        // Default to granite properties
        break;
    }
    
    return Properties;
}

float UGeologicalPhysicsSystem::ConvertRichterToMomentMagnitude(float RichterMagnitude)
{
    // Approximate conversion - varies by region and depth
    if (RichterMagnitude < 6.0f)
    {
        return RichterMagnitude;
    }
    else
    {
        return 0.67f * RichterMagnitude + 2.07f;
    }
}

FString UGeologicalPhysicsSystem::GetEarthquakeDescription(float Magnitude)
{
    if (Magnitude < 2.0f)
        return TEXT("Micro earthquake");
    else if (Magnitude < 4.0f)
        return TEXT("Minor earthquake");
    else if (Magnitude < 5.0f)
        return TEXT("Light earthquake");
    else if (Magnitude < 6.0f)
        return TEXT("Moderate earthquake");
    else if (Magnitude < 7.0f)
        return TEXT("Strong earthquake");
    else if (Magnitude < 8.0f)
        return TEXT("Major earthquake");
    else
        return TEXT("Great earthquake");
}

float UGeologicalPhysicsSystem::CalculateRockfallEnergy(float Mass, float Height)
{
    // Potential energy: E = mgh
    return Mass * 9.8f * Height; // Joules
}

float UGeologicalPhysicsSystem::EstimateAvalancheDamage(float Volume, float Speed)
{
    // Simplified damage estimation based on volume and speed
    float KineticEnergy = 0.5f * (Volume * 300.0f) * Speed * Speed; // Assume 300 kg/m³ snow density
    
    // Damage scale (arbitrary units for game purposes)
    return FMath::Sqrt(KineticEnergy) * 0.001f;
}