#include "WindPhysicsSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "HAL/ThreadSafeBool.h"
#include "../Tools/ClimbingToolBase.h"

UWindPhysicsSystem::UWindPhysicsSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    
    // Initialize default atmospheric properties
    AirDensity = 1.225f; // kg/m³ at 15°C, sea level
    Temperature = 288.15f; // 15°C in Kelvin
    Pressure = StandardPressure;
    Humidity = 0.5f;
    
    // Initialize default wind field
    FWindField DefaultField;
    DefaultField.Center = FVector::ZeroVector;
    DefaultField.Radius = 50000.0f; // 500m radius
    
    FWindLayer BaseLayer;
    BaseLayer.Altitude = 0.0f;
    BaseLayer.Direction = FVector(1.0f, 0.0f, 0.0f);
    BaseLayer.Speed = 0.0f;
    BaseLayer.Turbulence = 0.1f;
    BaseLayer.Thickness = 10000.0f; // 100m thick
    
    DefaultField.Layers.Add(BaseLayer);
    WindFields.Add(DefaultField);
    
    // Initialize turbulence
    GenerateTurbulenceSeeds();
    
    // Initialize drag coefficient table
    InitializeDragCoefficientTable();
}

void UWindPhysicsSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Update air density based on initial conditions
    AirDensity = CalculateAirDensityAtAltitude(0.0f);
}

void UWindPhysicsSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Update turbulence
    TurbulenceTimer += DeltaTime;
    if (TurbulenceTimer >= 5.0f) // Regenerate turbulence every 5 seconds
    {
        GenerateTurbulenceSeeds();
        TurbulenceTimer = 0.0f;
    }
    
    // Update active gusts
    UpdateActiveGusts(DeltaTime);
    
    // Clear old cache entries
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (bUseWindCache && CurrentTime - LastCacheUpdate > CacheTimeout)
    {
        WindVelocityCache.Empty();
        WindPressureCache.Empty();
        LastCacheUpdate = CurrentTime;
    }
    
    // Update LOD if needed
    if (CurrentTime - LastLODUpdate > LODUpdateInterval)
    {
        // Find closest player for LOD calculation
        float ClosestPlayerDistance = MaxWindSimulationDistance;
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
        
        OptimizeWindSimulation(ClosestPlayerDistance);
        LastLODUpdate = CurrentTime;
    }
}

FVector UWindPhysicsSystem::GetWindVelocityAtLocation(const FVector& Location, float Altitude) const
{
    // Use cache if available and recent
    uint32 LocationHash = GetTypeHash(Location) ^ GetTypeHash(Altitude);
    if (bUseWindCache)
    {
        if (FVector* CachedVelocity = WindVelocityCache.Find(LocationHash))
        {
            return *CachedVelocity;
        }
    }
    
    FVector ResultantWind = FVector::ZeroVector;
    float TotalWeight = 0.0f;
    
    // Sample from all wind fields
    for (const FWindField& Field : WindFields)
    {
        float DistanceToField = FVector::Dist(Location, Field.Center);
        if (DistanceToField > Field.Radius)
        {
            continue; // Outside this field's influence
        }
        
        // Calculate field influence weight (inverse distance)
        float Weight = 1.0f - (DistanceToField / Field.Radius);
        Weight = FMath::Pow(Weight, 2.0f); // Square for smoother falloff
        
        // Sample wind from appropriate layer
        FVector LayerWind = FVector::ZeroVector;
        for (int32 i = 0; i < Field.Layers.Num(); ++i)
        {
            const FWindLayer& Layer = Field.Layers[i];
            float LayerBottom = Layer.Altitude;
            float LayerTop = Layer.Altitude + Layer.Thickness;
            
            if (Altitude >= LayerBottom && Altitude <= LayerTop)
            {
                // Calculate position within layer for interpolation
                float LayerAlpha = (Altitude - LayerBottom) / Layer.Thickness;
                
                // Base wind calculation
                LayerWind = CalculateBaseWind(Location, Altitude);
                LayerWind = Layer.Direction.GetSafeNormal() * Layer.Speed;
                
                // Apply turbulence
                if (Layer.Turbulence > 0.0f)
                {
                    LayerWind = ApplyTurbulence(LayerWind, Location, Layer.Turbulence);
                }
                
                // Apply terrain effects
                LayerWind = ApplyTerrainEffects(LayerWind, Location, Field);
                
                // Apply wind shear if enabled
                if (bSimulateWindShear && i < Field.Layers.Num() - 1)
                {
                    LayerWind = ApplyWindShear(LayerWind, Altitude, 0.1f);
                }
                
                break;
            }
            else if (i < Field.Layers.Num() - 1)
            {
                // Interpolate between layers
                const FWindLayer& NextLayer = Field.Layers[i + 1];
                if (Altitude > LayerTop && Altitude < NextLayer.Altitude)
                {
                    float InterpolationAlpha = (Altitude - LayerTop) / (NextLayer.Altitude - LayerTop);
                    
                    FVector CurrentLayerWind = Layer.Direction.GetSafeNormal() * Layer.Speed;
                    FVector NextLayerWind = NextLayer.Direction.GetSafeNormal() * NextLayer.Speed;
                    
                    LayerWind = FMath::Lerp(CurrentLayerWind, NextLayerWind, InterpolationAlpha);
                    LayerWind = ApplyTurbulence(LayerWind, Location, FMath::Lerp(Layer.Turbulence, NextLayer.Turbulence, InterpolationAlpha));
                    break;
                }
            }
        }
        
        ResultantWind += LayerWind * Weight;
        TotalWeight += Weight;
    }
    
    // Normalize by total weight
    if (TotalWeight > 0.0f)
    {
        ResultantWind /= TotalWeight;
    }
    
    // Apply global scaling
    ResultantWind *= GlobalWindScale;
    
    // Add gust contributions
    ResultantWind += CalculateGustContribution(Location);
    
    // Cache the result
    if (bUseWindCache)
    {
        WindVelocityCache.Add(LocationHash, ResultantWind);
    }
    
    return ResultantWind;
}

FWindForceResult UWindPhysicsSystem::CalculateWindForce(const FVector& Location, const FWindInteractionData& ObjectData) const
{
    FWindForceResult Result;
    
    FVector WindVelocity = GetWindVelocityAtLocation(Location);
    float WindSpeed = WindVelocity.Size();
    
    if (WindSpeed < 0.1f)
    {
        return Result; // No significant wind
    }
    
    FVector WindDirection = WindVelocity.GetSafeNormal();
    
    // Calculate dynamic pressure
    float DynamicPressure = CalculateDynamicPressure(WindSpeed, AirDensity);
    Result.Pressure = DynamicPressure;
    
    // Calculate drag force
    FVector DragForce = CalculateDragForce(WindVelocity, ObjectData.CrossSectionalArea, 
                                          ObjectData.DragCoefficient, AirDensity);
    
    Result.Force = DragForce;
    Result.Magnitude = DragForce.Size();
    Result.ApplicationPoint = Location + ObjectData.CenterOfPressure;
    
    // Calculate torque if center of pressure is offset
    if (!ObjectData.CenterOfPressure.IsZero())
    {
        Result.Torque = FVector::CrossProduct(ObjectData.CenterOfPressure, DragForce);
    }
    
    return Result;
}

float UWindPhysicsSystem::GetWindPressureAtLocation(const FVector& Location, float Altitude) const
{
    // Use cache if available
    uint32 LocationHash = GetTypeHash(Location) ^ GetTypeHash(Altitude);
    if (bUseWindCache)
    {
        if (float* CachedPressure = WindPressureCache.Find(LocationHash))
        {
            return *CachedPressure;
        }
    }
    
    FVector WindVelocity = GetWindVelocityAtLocation(Location, Altitude);
    float WindSpeed = WindVelocity.Size();
    
    float DynamicPressure = CalculateDynamicPressure(WindSpeed, AirDensity);
    
    // Cache the result
    if (bUseWindCache)
    {
        WindPressureCache.Add(LocationHash, DynamicPressure);
    }
    
    return DynamicPressure;
}

FVector UWindPhysicsSystem::GetWindShearAtLocation(const FVector& Location, float AltitudeDelta) const
{
    if (!bSimulateWindShear)
    {
        return FVector::ZeroVector;
    }
    
    FVector BaseWind = GetWindVelocityAtLocation(Location, 0.0f);
    FVector UpperWind = GetWindVelocityAtLocation(Location, AltitudeDelta);
    
    return (UpperWind - BaseWind) / (AltitudeDelta * 0.01f); // Convert cm to m
}

void UWindPhysicsSystem::ApplyWindToRope(UAdvancedRopeComponent* Rope, float DeltaTime)
{
    if (!Rope || !Rope->CableComponent || CurrentLOD > 2)
    {
        return;
    }
    
    TArray<FVector> SegmentPositions = Rope->GetRopeSegmentPositions();
    int32 NumSegments = FMath::Min(SegmentPositions.Num(), MaxRopeSegmentsForWind);
    
    if (NumSegments <= 2)
    {
        return; // Need at least 3 points for meaningful wind simulation
    }
    
    TArray<FVector> SegmentForces;
    CalculateRopeWindForces(Rope, SegmentForces);
    
    // Apply flutter effects
    ApplyRopeFlutter(Rope, GetWindVelocityAtLocation(SegmentPositions[NumSegments/2]).Size(), DeltaTime);
    
    // Apply vortex shedding forces
    ApplyVortexSheddingForces(Rope, DeltaTime);
    
    // Modify cable properties to simulate wind effects
    UCableComponent* Cable = Rope->CableComponent;
    if (Cable)
    {
        float AvgWindSpeed = 0.0f;
        for (int32 i = 1; i < NumSegments - 1; ++i) // Skip anchored ends
        {
            AvgWindSpeed += GetWindVelocityAtLocation(SegmentPositions[i]).Size();
        }
        AvgWindSpeed /= FMath::Max(1, NumSegments - 2);
        
        // Increase simulation fidelity in high wind
        if (AvgWindSpeed > 10.0f)
        {
            Cable->NumSegments = FMath::Min(Cable->NumSegments + 2, 64);
            Cable->SubstepTime = FMath::Max(Cable->SubstepTime * 0.8f, 0.002f);
        }
        
        // Add wind-induced tension
        float WindTension = AvgWindSpeed * AvgWindSpeed * 0.1f; // Simplified wind loading
        Rope->PhysicsState.CurrentTension += WindTension;
    }
}

void UWindPhysicsSystem::ApplyWindToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime)
{
    if (!ClimbingComponent || CurrentLOD > 1)
    {
        return;
    }
    
    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    FVector WindVelocity = GetWindVelocityAtLocation(ClimberLocation);
    
    // Calculate climber wind resistance
    FVector WindResistance = CalculateClimberWindResistance(ClimbingComponent);
    
    // Apply wind-induced balance challenge
    ApplyWindBalance(ClimbingComponent, WindResistance, DeltaTime);
    
    // Calculate additional grip strength requirement
    float AdditionalGripRequired = CalculateGripStrengthRequirement(WindResistance, ClimberLocation);
    
    // Apply effects to climbing component
    if (WindResistance.Size() > 50.0f) // Significant wind force
    {
        float WindEffect = FMath::Clamp(WindResistance.Size() / 500.0f, 0.0f, 1.0f);
        
        // Increase stamina drain due to wind resistance
        ClimbingComponent->ConsumeStamina(WindEffect * 10.0f * DeltaTime);
        
        // Reduce movement precision
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - WindEffect * 0.3f);
        
        // Additional grip strength requirement
        ClimbingComponent->ConsumeGripStrength(AdditionalGripRequired * DeltaTime);
    }
}

void UWindPhysicsSystem::ApplyWindToRigidBody(UPrimitiveComponent* Component, float DeltaTime)
{
    if (!Component || !Component->IsSimulatingPhysics() || CurrentLOD > 2)
    {
        return;
    }
    
    FVector ObjectLocation = Component->GetComponentLocation();
    FVector ObjectVelocity = Component->GetPhysicsLinearVelocity();
    
    // Get relative wind velocity
    FVector WindVelocity = GetWindVelocityAtLocation(ObjectLocation);
    FVector RelativeWind = WindVelocity - ObjectVelocity * 0.01f; // Convert cm/s to m/s
    
    if (RelativeWind.Size() < 0.1f)
    {
        return; // No significant relative wind
    }
    
    // Calculate object interaction data
    FWindInteractionData InteractionData;
    FVector ObjectBounds = Component->Bounds.BoxExtent * 2.0f;
    InteractionData.CrossSectionalArea = (ObjectBounds.X * ObjectBounds.Z) * 0.0001f; // cm² to m²
    InteractionData.DragCoefficient = GetDragCoefficient(0.0f, TEXT("Box")); // Simplified
    InteractionData.Mass = Component->GetMass();
    
    // Calculate wind force
    FWindForceResult WindForce = CalculateWindForce(ObjectLocation, InteractionData);
    
    // Apply force to rigid body
    Component->AddForce(WindForce.Force * 100.0f); // Convert N to UE4 units
    
    if (!WindForce.Torque.IsZero())
    {
        Component->AddTorqueInRadians(WindForce.Torque * 100.0f);
    }
}

void UWindPhysicsSystem::ApplyWindToTool(AActor* Tool, float DeltaTime)
{
    if (!Tool)
    {
        return;
    }
    
    UPrimitiveComponent* ToolComponent = Tool->FindComponentByClass<UPrimitiveComponent>();
    if (ToolComponent)
    {
        ApplyWindToRigidBody(ToolComponent, DeltaTime);
    }
    
    // Special handling for climbing tools
    AClimbingToolBase* ClimbingTool = Cast<AClimbingToolBase>(Tool);
    if (ClimbingTool)
    {
        FVector ToolLocation = Tool->GetActorLocation();
        FVector WindVelocity = GetWindVelocityAtLocation(ToolLocation);
        
        // Wind affects tool placement accuracy
        float WindSpeed = WindVelocity.Size();
        if (WindSpeed > 5.0f)
        {
            float AccuracyReduction = FMath::Clamp(WindSpeed / 30.0f, 0.0f, 0.7f);
            // This would interface with the tool's placement system
            // ClimbingTool->ModifyPlacementAccuracy(1.0f - AccuracyReduction);
        }
    }
}

void UWindPhysicsSystem::CreateWindGust(const FVector& Origin, float Intensity, float Duration, float Radius)
{
    FActiveGust NewGust;
    NewGust.Origin = Origin;
    NewGust.Intensity = Intensity;
    NewGust.Duration = Duration;
    NewGust.ElapsedTime = 0.0f;
    NewGust.Radius = Radius;
    
    ActiveGusts.Add(NewGust);
}

FVector UWindPhysicsSystem::CalculateBaseWind(const FVector& Location, float Altitude) const
{
    // Find the appropriate wind field
    FWindField NearestField = GetNearestWindField(Location);
    
    // Find the appropriate layer for this altitude
    for (const FWindLayer& Layer : NearestField.Layers)
    {
        if (Altitude >= Layer.Altitude && Altitude <= Layer.Altitude + Layer.Thickness)
        {
            return Layer.Direction.GetSafeNormal() * Layer.Speed;
        }
    }
    
    // Default to first layer if no match found
    if (NearestField.Layers.Num() > 0)
    {
        const FWindLayer& DefaultLayer = NearestField.Layers[0];
        return DefaultLayer.Direction.GetSafeNormal() * DefaultLayer.Speed;
    }
    
    return FVector::ZeroVector;
}

FVector UWindPhysicsSystem::ApplyTurbulence(const FVector& BaseWind, const FVector& Location, float Turbulence) const
{
    if (Turbulence <= 0.0f)
    {
        return BaseWind;
    }
    
    // Sample turbulence at location
    FVector TurbulenceWind = SampleTurbulence(Location, Turbulence);
    
    // Apply turbulence as a percentage of base wind
    FVector TurbulentWind = BaseWind + (TurbulenceWind * BaseWind.Size() * Turbulence);
    
    return TurbulentWind;
}

FVector UWindPhysicsSystem::ApplyTerrainEffects(const FVector& Wind, const FVector& Location, const FWindField& Field) const
{
    FVector ModifiedWind = Wind;
    
    switch (Field.TerrainEffect)
    {
    case ETerrainWindEffect::Venturi:
        ModifiedWind += CalculateVenturiEffect(Location, Wind.GetSafeNormal());
        break;
    case ETerrainWindEffect::Rotor:
        ModifiedWind += CalculateRotorEffect(Location, FVector::UpVector);
        break;
    case ETerrainWindEffect::Channeling:
        // Channel wind along terrain features
        ModifiedWind *= (1.0f + Field.TerrainEffectStrength * 0.5f);
        break;
    }
    
    return ModifiedWind;
}

FVector UWindPhysicsSystem::ApplyWindShear(const FVector& Wind, float Altitude, float ShearFactor) const
{
    // Simple logarithmic wind profile
    float ReferenceHeight = 1000.0f; // 10m in cm
    float WindShearMultiplier = FMath::Loge((Altitude + ReferenceHeight) / ReferenceHeight) * ShearFactor;
    
    return Wind * (1.0f + WindShearMultiplier);
}

float UWindPhysicsSystem::CalculateAirDensityAtAltitude(float Altitude) const
{
    // Standard atmosphere model
    float AltitudeMeters = Altitude * 0.01f; // Convert cm to m
    float TemperatureAtAltitude = Temperature - (6.5f * AltitudeMeters / 1000.0f); // Temperature lapse rate
    float PressureRatio = FMath::Pow(TemperatureAtAltitude / Temperature, 5.26f);
    
    return AirDensity * PressureRatio * (Temperature / TemperatureAtAltitude);
}

float UWindPhysicsSystem::CalculateDynamicPressure(float Velocity, float Density) const
{
    return 0.5f * Density * Velocity * Velocity;
}

FVector UWindPhysicsSystem::CalculateDragForce(const FVector& Velocity, float Area, float DragCoeff, float Density) const
{
    float VelocitySquared = Velocity.SizeSquared();
    if (VelocitySquared < 0.01f)
    {
        return FVector::ZeroVector;
    }
    
    float DragMagnitude = 0.5f * Density * VelocitySquared * DragCoeff * Area;
    return -Velocity.GetSafeNormal() * DragMagnitude; // Opposing velocity direction
}

void UWindPhysicsSystem::CalculateRopeWindForces(UAdvancedRopeComponent* Rope, TArray<FVector>& SegmentForces) const
{
    if (!Rope)
    {
        return;
    }
    
    TArray<FVector> SegmentPositions = Rope->GetRopeSegmentPositions();
    SegmentForces.SetNum(SegmentPositions.Num());
    
    float RopeDiameter = Rope->Properties.Diameter * 0.001f; // mm to m
    float SegmentLength = Rope->Properties.Length / FMath::Max(1, SegmentPositions.Num() - 1);
    SegmentLength *= 0.01f; // Convert cm to m
    
    for (int32 i = 0; i < SegmentPositions.Num(); ++i)
    {
        if (i == 0 || i == SegmentPositions.Num() - 1)
        {
            // End points are anchored, no wind force
            SegmentForces[i] = FVector::ZeroVector;
            continue;
        }
        
        FVector SegmentVelocity = FVector::ZeroVector; // TODO: Get actual segment velocity
        FVector WindForce = FVector::ZeroVector;
        
        CalculateRopeSegmentWind(SegmentPositions[i], SegmentVelocity, SegmentLength, RopeDiameter, WindForce);
        SegmentForces[i] = WindForce;
    }
}

void UWindPhysicsSystem::CalculateRopeSegmentWind(const FVector& SegmentPos, const FVector& SegmentVel, 
                                                  float SegmentLength, float RopeDiameter, FVector& OutForce) const
{
    FVector WindVelocity = GetWindVelocityAtLocation(SegmentPos);
    FVector RelativeWind = WindVelocity - SegmentVel * 0.01f; // Convert cm/s to m/s
    
    if (RelativeWind.Size() < 0.1f)
    {
        OutForce = FVector::ZeroVector;
        return;
    }
    
    // Calculate drag force on rope segment (cylinder in cross-flow)
    float CrossSectionalArea = RopeDiameter * SegmentLength;
    float DragCoefficient = 1.2f; // Cylinder drag coefficient
    
    OutForce = CalculateDragForce(RelativeWind, CrossSectionalArea, DragCoefficient, AirDensity);
    OutForce *= 100.0f; // Convert to UE4 units (N to UE4 force)
}

void UWindPhysicsSystem::ApplyRopeFlutter(UAdvancedRopeComponent* Rope, float WindSpeed, float DeltaTime)
{
    if (!Rope || WindSpeed < 5.0f)
    {
        return;
    }
    
    // Calculate rope natural frequency
    float RopeTension = Rope->PhysicsState.CurrentTension;
    float RopeLength = Rope->Properties.Length * 0.01f; // cm to m
    float LinearDensity = Rope->Properties.Weight / 100.0f; // kg per 100m to kg/m
    
    float NaturalFrequency = 0.5f * FMath::Sqrt(RopeTension / LinearDensity) / RopeLength;
    
    // Calculate Strouhal number for vortex shedding frequency
    float StrouhalNumber = CalculateRopeStrouhalNumber(Rope->Properties.Diameter * 0.001f, WindSpeed);
    float VortexFrequency = (StrouhalNumber * WindSpeed) / (Rope->Properties.Diameter * 0.001f);
    
    // Check for resonance condition
    float ResonanceFactor = CalculateRopeResonance(Rope, VortexFrequency);
    
    if (ResonanceFactor > 0.8f)
    {
        // Apply flutter forces
        if (Rope->CableComponent)
        {
            Rope->CableComponent->CableWidth *= (1.0f + ResonanceFactor * 0.2f * FMath::Sin(GetWorld()->GetTimeSeconds() * VortexFrequency * 2.0f * PI));
        }
    }
}

float UWindPhysicsSystem::CalculateRopeStrouhalNumber(float RopeDiameter, float WindSpeed) const
{
    // Strouhal number for cylinder in cross-flow (typically ~0.2 for smooth cylinders)
    float ReynoldsNumber = GetReynoldsNumber(RopeDiameter, WindSpeed);
    
    if (ReynoldsNumber < 40.0f)
    {
        return 0.0f; // No vortex shedding at very low Re
    }
    else if (ReynoldsNumber < 1000.0f)
    {
        return FMath::Lerp(0.0f, 0.2f, (ReynoldsNumber - 40.0f) / 960.0f);
    }
    else if (ReynoldsNumber < 200000.0f)
    {
        return 0.2f; // Typical value for laminar boundary layer
    }
    else
    {
        return 0.27f; // Turbulent boundary layer
    }
}

float UWindPhysicsSystem::CalculateRopeResonance(UAdvancedRopeComponent* Rope, float ExcitationFrequency) const
{
    // Calculate rope natural frequencies (first few modes)
    float RopeTension = Rope->PhysicsState.CurrentTension;
    float RopeLength = Rope->Properties.Length * 0.01f; // cm to m
    float LinearDensity = Rope->Properties.Weight / 100.0f; // kg per 100m to kg/m
    
    float FundamentalFrequency = 0.5f * FMath::Sqrt(RopeTension / LinearDensity) / RopeLength;
    
    // Check resonance with first few modes
    for (int32 Mode = 1; Mode <= 5; ++Mode)
    {
        float ModeFrequency = Mode * FundamentalFrequency;
        float FrequencyRatio = FMath::Abs(ExcitationFrequency - ModeFrequency) / ModeFrequency;
        
        if (FrequencyRatio < 0.1f) // Within 10% of natural frequency
        {
            return 1.0f - (FrequencyRatio / 0.1f); // Linear decay from resonance peak
        }
    }
    
    return 0.0f; // No resonance
}

void UWindPhysicsSystem::ApplyVortexSheddingForces(UAdvancedRopeComponent* Rope, float DeltaTime)
{
    if (!Rope || !Rope->CableComponent)
    {
        return;
    }
    
    TArray<FVector> SegmentPositions = Rope->GetRopeSegmentPositions();
    if (SegmentPositions.Num() < 3)
    {
        return;
    }
    
    float RopeDiameter = Rope->Properties.Diameter * 0.001f; // mm to m
    
    for (int32 i = 1; i < SegmentPositions.Num() - 1; ++i) // Skip end points
    {
        FVector WindVelocity = GetWindVelocityAtLocation(SegmentPositions[i]);
        float WindSpeed = WindVelocity.Size();
        
        if (WindSpeed < 2.0f)
        {
            continue;
        }
        
        // Calculate vortex shedding frequency
        float StrouhalNumber = CalculateRopeStrouhalNumber(RopeDiameter, WindSpeed);
        float ShedFrequency = (StrouhalNumber * WindSpeed) / RopeDiameter;
        
        // Generate alternating vortex forces perpendicular to wind direction
        float VortexPhase = GetWorld()->GetTimeSeconds() * ShedFrequency * 2.0f * PI;
        FVector WindDirection = WindVelocity.GetSafeNormal();
        FVector PerpendicularDirection = FVector::CrossProduct(WindDirection, FVector::UpVector).GetSafeNormal();
        
        float VortexForce = FMath::Sin(VortexPhase + i * PI * 0.1f) * WindSpeed * WindSpeed * 0.1f;
        FVector VortexVector = PerpendicularDirection * VortexForce * 100.0f; // Convert to UE4 units
        
        // Apply force to rope segment (this would typically require direct cable manipulation)
        // For now, we modify the cable's visual properties to simulate the effect
        if (FMath::Abs(VortexForce) > 10.0f)
        {
            float WaveAmplitude = FMath::Abs(VortexForce) * 0.1f;
            Rope->CableComponent->CableWidth *= (1.0f + WaveAmplitude * FMath::Sin(VortexPhase));
        }
    }
}

FVector UWindPhysicsSystem::CalculateClimberWindResistance(UAdvancedClimbingComponent* ClimbingComponent) const
{
    if (!ClimbingComponent)
    {
        return FVector::ZeroVector;
    }
    
    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    FVector ClimberVelocity = ClimbingComponent->Velocity * 0.01f; // Convert cm/s to m/s
    FVector WindVelocity = GetWindVelocityAtLocation(ClimberLocation);
    
    // Calculate relative wind
    FVector RelativeWind = WindVelocity - ClimberVelocity;
    if (RelativeWind.Size() < 0.1f)
    {
        return FVector::ZeroVector;
    }
    
    // Calculate climber orientation (simplified)
    FVector ClimberForward = ClimbingComponent->GetOwner()->GetActorForwardVector();
    FVector ClimberUp = ClimbingComponent->GetOwner()->GetActorUpVector();
    
    // Calculate drag and lift forces
    FVector DragForce, LiftForce;
    CalculateClimberDragAndLift(RelativeWind, ClimberForward, DragForce, LiftForce);
    
    return DragForce + LiftForce;
}

void UWindPhysicsSystem::CalculateClimberDragAndLift(const FVector& WindVelocity, const FVector& ClimberOrientation,
                                                    FVector& OutDrag, FVector& OutLift) const
{
    float WindSpeed = WindVelocity.Size();
    FVector WindDirection = WindVelocity.GetSafeNormal();
    
    // Calculate cross-sectional area presented to wind
    float CrossSectionalArea = CalculateClimberCrossSectionalArea(WindDirection, ClimberOrientation);
    
    // Drag coefficient for human body (varies with orientation)
    float DragCoefficient = 1.0f; // Simplified - would vary based on posture
    
    // Calculate drag force
    OutDrag = CalculateDragForce(WindVelocity, CrossSectionalArea, DragCoefficient, AirDensity);
    
    // Calculate lift (Magnus effect from body rotation, typically minimal)
    OutLift = FVector::ZeroVector; // Simplified - climbers don't generate significant lift
    
    // Convert to UE4 units
    OutDrag *= 100.0f;
    OutLift *= 100.0f;
}

float UWindPhysicsSystem::CalculateClimberCrossSectionalArea(const FVector& WindDirection, const FVector& ClimberOrientation) const
{
    // Simplified calculation - assume climber is roughly cylindrical
    float ClimberHeight = 180.0f; // cm
    float ClimberWidth = 60.0f;   // cm
    float ClimberDepth = 30.0f;   // cm
    
    // Calculate presented area based on wind direction relative to climber orientation
    float DotProduct = FMath::Abs(FVector::DotProduct(WindDirection, ClimberOrientation));
    
    // Interpolate between frontal and side areas
    float FrontalArea = ClimberHeight * ClimberDepth * 0.0001f; // cm² to m²
    float SideArea = ClimberHeight * ClimberWidth * 0.0001f;    // cm² to m²
    
    return FMath::Lerp(SideArea, FrontalArea, DotProduct);
}

void UWindPhysicsSystem::ApplyWindBalance(UAdvancedClimbingComponent* ClimbingComponent, const FVector& WindForce, float DeltaTime)
{
    if (!ClimbingComponent || WindForce.Size() < 10.0f)
    {
        return;
    }
    
    float WindMagnitude = WindForce.Size();
    FVector WindDirection = WindForce.GetSafeNormal();
    
    // Calculate balance challenge based on wind force
    float BalanceChallenge = FMath::Clamp(WindMagnitude / 100.0f, 0.0f, 1.0f);
    
    // Increase stamina drain due to balance effort
    ClimbingComponent->ConsumeStamina(BalanceChallenge * 5.0f * DeltaTime);
    
    // Apply random balance displacement
    if (FMath::RandRange(0.0f, 1.0f) < BalanceChallenge * 0.1f) // Random balance events
    {
        FVector BalanceOffset = WindDirection * BalanceChallenge * 10.0f; // cm displacement
        
        // This would typically apply a small displacement to the character
        // For now, we just increase grip strength requirements
        ClimbingComponent->ConsumeGripStrength(BalanceChallenge * 20.0f);
    }
}

float UWindPhysicsSystem::CalculateGripStrengthRequirement(const FVector& WindForce, const FVector& ClimberPosition) const
{
    float WindMagnitude = WindForce.Size();
    
    // Additional grip strength needed to resist wind (simplified calculation)
    float AdditionalGrip = WindMagnitude * 0.1f; // 10% of wind force as additional grip requirement
    
    return FMath::Clamp(AdditionalGrip, 0.0f, 50.0f); // Cap at 50% additional grip requirement
}

void UWindPhysicsSystem::UpdateAtmosphericProperties(float NewTemperature, float NewPressure, float NewHumidity)
{
    Temperature = NewTemperature;
    Pressure = NewPressure;
    Humidity = FMath::Clamp(NewHumidity, 0.0f, 1.0f);
    
    // Update air density based on new conditions
    AirDensity = (Pressure / (SpecificGasConstant * Temperature)) * 
                 (1.0f - 0.378f * (Humidity * 611.21f * FMath::Exp(17.502f * (Temperature - 273.15f) / (Temperature - 32.19f)) / Pressure));
}

float UWindPhysicsSystem::GetReynoldsNumber(float CharacteristicLength, float Velocity) const
{
    return (AirDensity * Velocity * CharacteristicLength) / (AirDensity * KinematicViscosity);
}

float UWindPhysicsSystem::GetDragCoefficient(float ReynoldsNumber, const FString& ObjectShape) const
{
    if (float* CachedCoeff = DragCoefficientTable.Find(ObjectShape))
    {
        return *CachedCoeff;
    }
    
    // Default drag coefficient if shape not found
    return 1.0f;
}

void UWindPhysicsSystem::GenerateTurbulenceSeeds()
{
    TurbulenceSeeds.Empty();
    
    // Generate random seeds for Perlin noise-like turbulence
    for (int32 i = 0; i < 64; ++i)
    {
        FVector Seed = FVector(
            FMath::RandRange(-1.0f, 1.0f),
            FMath::RandRange(-1.0f, 1.0f),
            FMath::RandRange(-1.0f, 1.0f)
        ).GetSafeNormal();
        
        TurbulenceSeeds.Add(Seed);
    }
}

FVector UWindPhysicsSystem::SampleTurbulence(const FVector& Location, float Scale) const
{
    if (TurbulenceSeeds.Num() == 0)
    {
        return FVector::ZeroVector;
    }
    
    // Simple 3D noise sampling
    FVector ScaledLocation = Location * 0.01f * Scale; // Convert to meters and scale
    
    int32 X = FMath::FloorToInt(ScaledLocation.X) & 63;
    int32 Y = FMath::FloorToInt(ScaledLocation.Y) & 63;
    int32 Z = FMath::FloorToInt(ScaledLocation.Z) & 63;
    
    int32 Index = (X + Y * 8 + Z * 64) % TurbulenceSeeds.Num();
    
    return TurbulenceSeeds[Index] * Scale;
}

void UWindPhysicsSystem::UpdateActiveGusts(float DeltaTime)
{
    for (int32 i = ActiveGusts.Num() - 1; i >= 0; --i)
    {
        ActiveGusts[i].ElapsedTime += DeltaTime;
        
        if (ActiveGusts[i].ElapsedTime >= ActiveGusts[i].Duration)
        {
            ActiveGusts.RemoveAt(i);
        }
    }
}

FVector UWindPhysicsSystem::CalculateGustContribution(const FVector& Location) const
{
    FVector GustContribution = FVector::ZeroVector;
    
    for (const FActiveGust& Gust : ActiveGusts)
    {
        float DistanceToGust = FVector::Dist(Location, Gust.Origin);
        if (DistanceToGust > Gust.Radius)
        {
            continue;
        }
        
        // Calculate gust strength based on distance and time
        float DistanceFactor = 1.0f - (DistanceToGust / Gust.Radius);
        float TimeFactor = FMath::Sin((Gust.ElapsedTime / Gust.Duration) * PI); // Bell curve over time
        
        float GustStrength = Gust.Intensity * DistanceFactor * TimeFactor;
        
        // Gust direction (radial from origin)
        FVector GustDirection = (Location - Gust.Origin).GetSafeNormal();
        if (GustDirection.IsZero())
        {
            GustDirection = FVector(1.0f, 0.0f, 0.0f);
        }
        
        GustContribution += GustDirection * GustStrength;
    }
    
    return GustContribution;
}

FWindField UWindPhysicsSystem::GetNearestWindField(const FVector& Location) const
{
    if (WindFields.Num() == 0)
    {
        // Return default field
        FWindField DefaultField;
        DefaultField.Center = Location;
        DefaultField.Radius = 10000.0f;
        
        FWindLayer DefaultLayer;
        DefaultLayer.Altitude = 0.0f;
        DefaultLayer.Direction = FVector(1.0f, 0.0f, 0.0f);
        DefaultLayer.Speed = 0.0f;
        DefaultLayer.Turbulence = 0.1f;
        DefaultLayer.Thickness = 10000.0f;
        
        DefaultField.Layers.Add(DefaultLayer);
        return DefaultField;
    }
    
    float MinDistance = FLT_MAX;
    int32 NearestIndex = 0;
    
    for (int32 i = 0; i < WindFields.Num(); ++i)
    {
        float Distance = FVector::Dist(Location, WindFields[i].Center);
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestIndex = i;
        }
    }
    
    return WindFields[NearestIndex];
}

void UWindPhysicsSystem::InitializeDragCoefficientTable()
{
    DragCoefficientTable.Add(TEXT("Sphere"), 0.47f);
    DragCoefficientTable.Add(TEXT("Cube"), 1.05f);
    DragCoefficientTable.Add(TEXT("Box"), 0.8f);
    DragCoefficientTable.Add(TEXT("Cylinder"), 1.2f);
    DragCoefficientTable.Add(TEXT("Streamlined"), 0.04f);
    DragCoefficientTable.Add(TEXT("Flat Plate"), 1.28f);
    DragCoefficientTable.Add(TEXT("Human Body"), 1.0f);
    DragCoefficientTable.Add(TEXT("Rope"), 1.2f);
}

void UWindPhysicsSystem::SetWindLOD(int32 LODLevel)
{
    CurrentLOD = FMath::Clamp(LODLevel, 0, 3);
    
    // Adjust simulation complexity based on LOD
    switch (CurrentLOD)
    {
    case 0: // High detail
        MaxRopeSegmentsForWind = 64;
        bUseReynoldsNumber = true;
        bSimulateWindShear = true;
        break;
    case 1: // Medium detail
        MaxRopeSegmentsForWind = 32;
        bUseReynoldsNumber = true;
        bSimulateWindShear = false;
        break;
    case 2: // Low detail
        MaxRopeSegmentsForWind = 16;
        bUseReynoldsNumber = false;
        bSimulateWindShear = false;
        break;
    case 3: // Minimal detail
        MaxRopeSegmentsForWind = 8;
        bUseReynoldsNumber = false;
        bSimulateWindShear = false;
        break;
    }
}

void UWindPhysicsSystem::OptimizeWindSimulation(float ViewerDistance)
{
    // Automatically adjust LOD based on distance
    if (ViewerDistance < 1000.0f) // 10m
    {
        SetWindLOD(0);
    }
    else if (ViewerDistance < 2500.0f) // 25m
    {
        SetWindLOD(1);
    }
    else if (ViewerDistance < 5000.0f) // 50m
    {
        SetWindLOD(2);
    }
    else
    {
        SetWindLOD(3);
    }
}

// Static utility functions
float UWindPhysicsSystem::ConvertBeaufortToMS(int32 BeaufortScale)
{
    BeaufortScale = FMath::Clamp(BeaufortScale, 0, 12);
    
    static const float BeaufortToMS[] = {
        0.0f,   // 0: Calm
        0.8f,   // 1: Light air
        2.4f,   // 2: Light breeze
        4.4f,   // 3: Gentle breeze
        6.7f,   // 4: Moderate breeze
        9.3f,   // 5: Fresh breeze
        12.3f,  // 6: Strong breeze
        15.5f,  // 7: Near gale
        18.9f,  // 8: Gale
        22.6f,  // 9: Strong gale
        26.4f,  // 10: Storm
        30.5f,  // 11: Violent storm
        35.0f   // 12: Hurricane
    };
    
    return BeaufortToMS[BeaufortScale];
}

int32 UWindPhysicsSystem::ConvertMSToBeaufort(float MetersPerSecond)
{
    if (MetersPerSecond < 0.3f) return 0;
    else if (MetersPerSecond < 1.6f) return 1;
    else if (MetersPerSecond < 3.4f) return 2;
    else if (MetersPerSecond < 5.5f) return 3;
    else if (MetersPerSecond < 8.0f) return 4;
    else if (MetersPerSecond < 10.8f) return 5;
    else if (MetersPerSecond < 13.9f) return 6;
    else if (MetersPerSecond < 17.2f) return 7;
    else if (MetersPerSecond < 20.8f) return 8;
    else if (MetersPerSecond < 24.5f) return 9;
    else if (MetersPerSecond < 28.5f) return 10;
    else if (MetersPerSecond < 32.7f) return 11;
    else return 12;
}

FString UWindPhysicsSystem::GetWindDescription(float WindSpeed)
{
    int32 Beaufort = ConvertMSToBeaufort(WindSpeed);
    
    static const FString Descriptions[] = {
        TEXT("Calm"),
        TEXT("Light air"),
        TEXT("Light breeze"),
        TEXT("Gentle breeze"),
        TEXT("Moderate breeze"),
        TEXT("Fresh breeze"),
        TEXT("Strong breeze"),
        TEXT("Near gale"),
        TEXT("Gale"),
        TEXT("Strong gale"),
        TEXT("Storm"),
        TEXT("Violent storm"),
        TEXT("Hurricane")
    };
    
    return Descriptions[FMath::Clamp(Beaufort, 0, 12)];
}

float UWindPhysicsSystem::CalculateWindChill(float AirTemp, float WindSpeed)
{
    // Wind chill formula (valid for temperatures below 10°C and wind speeds above 4.8 km/h)
    float WindSpeedKMH = WindSpeed * 3.6f; // Convert m/s to km/h
    
    if (AirTemp > 10.0f || WindSpeedKMH < 4.8f)
    {
        return AirTemp; // No wind chill calculation
    }
    
    float WindChill = 13.12f + 0.6215f * AirTemp - 11.37f * FMath::Pow(WindSpeedKMH, 0.16f) + 0.3965f * AirTemp * FMath::Pow(WindSpeedKMH, 0.16f);
    
    return WindChill;
}

float UWindPhysicsSystem::CalculateApparentWindSpeed(float TrueWindSpeed, float ObjectSpeed, float AngleBetween)
{
    // Vector addition of true wind and object-induced wind
    float CosAngle = FMath::Cos(FMath::DegreesToRadians(AngleBetween));
    
    return FMath::Sqrt(TrueWindSpeed * TrueWindSpeed + ObjectSpeed * ObjectSpeed + 2.0f * TrueWindSpeed * ObjectSpeed * CosAngle);
}