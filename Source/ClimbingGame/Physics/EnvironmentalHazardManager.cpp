#include "EnvironmentalHazardManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Engine/CollisionProfile.h"
#include "../Tools/ClimbingToolBase.h"
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "HAL/ThreadSafeBool.h"

UEnvironmentalHazardManager::UEnvironmentalHazardManager()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics; // Tick before physics
    SetIsReplicatedByDefault(true);

    // Initialize default environmental conditions
    CurrentWind.Direction = FVector(1.0f, 0.0f, 0.0f);
    CurrentWind.Speed = 0.0f;
    CurrentWind.Gusts = 0.0f;
    CurrentWind.Turbulence = 0.0f;

    CurrentWeather.Type = EWeatherType::Clear;
    CurrentWeather.Intensity = 0.0f;
    CurrentWeather.Temperature = 20.0f;
    CurrentWeather.Humidity = 50.0f;
    CurrentWeather.Precipitation = 0.0f;
    CurrentWeather.Visibility = 1.0f;

    CurrentGeological.HazardType = EGeologicalHazard::None;
    CurrentGeological.Severity = EHazardSeverity::Minimal;
    CurrentGeological.Magnitude = 0.0f;
    CurrentGeological.Duration = 0.0f;

    // Initialize combined effects to neutral
    CombinedEffects.GripStrengthMultiplier = 1.0f;
    CombinedEffects.FrictionMultiplier = 1.0f;
    CombinedEffects.StaminaDrainMultiplier = 1.0f;
    CombinedEffects.ToolAccuracyMultiplier = 1.0f;
    CombinedEffects.RopeSwayMultiplier = 1.0f;
    CombinedEffects.VisibilityMultiplier = 1.0f;
    CombinedEffects.EquipmentDurabilityMultiplier = 1.0f;
    CombinedEffects.MovementSpeedMultiplier = 1.0f;
    CombinedEffects.BalanceOffset = FVector::ZeroVector;
}

void UEnvironmentalHazardManager::BeginPlay()
{
    Super::BeginPlay();

    // Initialize base wind direction
    BaseWindDirection = CurrentWind.Direction;
    
    // Set up initial environmental state
    UpdateCombinedEffects();
}

void UEnvironmentalHazardManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return; // Only simulate on server
    }

    // Determine update frequency based on distance to players
    bool bShouldFullUpdate = ShouldSimulateFullDetail();
    bool bShouldLightUpdate = GetWorld()->GetTimeSeconds() - LastLightUpdate > LightUpdateInterval;

    if (bShouldFullUpdate && GetWorld()->GetTimeSeconds() - LastFullUpdate > FullUpdateInterval)
    {
        UpdateWindEffects(DeltaTime);
        UpdateWeatherEffects(DeltaTime);
        UpdateGeologicalEffects(DeltaTime);
        UpdateCombinedEffects();
        LastFullUpdate = GetWorld()->GetTimeSeconds();
    }
    else if (bShouldLightUpdate)
    {
        // Light update - just update critical systems
        UpdateCombinedEffects();
        LastLightUpdate = GetWorld()->GetTimeSeconds();
    }

    // Always update weather transitions
    if (bIsTransitioning)
    {
        UpdateWeatherTransition(DeltaTime);
    }
}

void UEnvironmentalHazardManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UEnvironmentalHazardManager, CurrentWind);
    DOREPLIFETIME(UEnvironmentalHazardManager, CurrentWeather);
    DOREPLIFETIME(UEnvironmentalHazardManager, CurrentGeological);
}

void UEnvironmentalHazardManager::SetWindConditions(const FWindData& NewWind)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        CurrentWind = NewWind;
        BaseWindDirection = NewWind.Direction.GetSafeNormal();
    }
    else
    {
        ServerSetWindConditions(NewWind);
    }
}

bool UEnvironmentalHazardManager::ServerSetWindConditions_Validate(const FWindData& NewWind)
{
    // Validate wind data ranges
    return NewWind.Speed >= 0.0f && NewWind.Speed <= 100.0f && // 0-100 m/s
           NewWind.Gusts >= 0.0f && NewWind.Gusts <= 50.0f &&  // 0-50 m/s additional
           NewWind.Turbulence >= 0.0f && NewWind.Turbulence <= 1.0f; // 0-1 normalized
}

void UEnvironmentalHazardManager::ServerSetWindConditions_Implementation(const FWindData& NewWind)
{
    SetWindConditions(NewWind);
}

void UEnvironmentalHazardManager::SetWeatherConditions(const FWeatherData& NewWeather)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        CurrentWeather = NewWeather;
        OnWeatherChanged.Broadcast();
    }
    else
    {
        ServerSetWeatherConditions(NewWeather);
    }
}

bool UEnvironmentalHazardManager::ServerSetWeatherConditions_Validate(const FWeatherData& NewWeather)
{
    // Validate weather data ranges
    return NewWeather.Intensity >= 0.0f && NewWeather.Intensity <= 1.0f &&
           NewWeather.Temperature >= -50.0f && NewWeather.Temperature <= 60.0f &&
           NewWeather.Humidity >= 0.0f && NewWeather.Humidity <= 100.0f &&
           NewWeather.Precipitation >= 0.0f && NewWeather.Precipitation <= 200.0f &&
           NewWeather.Visibility >= 0.0f && NewWeather.Visibility <= 1.0f;
}

void UEnvironmentalHazardManager::ServerSetWeatherConditions_Implementation(const FWeatherData& NewWeather)
{
    SetWeatherConditions(NewWeather);
}

void UEnvironmentalHazardManager::TriggerGeologicalEvent(const FGeologicalData& EventData)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        CurrentGeological = EventData;
        bGeologicalEventActive = true;
        GeologicalEventTimer = 0.0f;
        OnGeologicalEventStart.Broadcast();

        // Special handling for different geological events
        switch (EventData.HazardType)
        {
        case EGeologicalHazard::Rockfall:
            SimulateRockfall(EventData.EpicenterLocation, EventData.Magnitude, EventData.Duration);
            break;
        case EGeologicalHazard::Earthquake:
            SimulateEarthquake(EventData.EpicenterLocation, EventData.Magnitude, EventData.Duration);
            break;
        case EGeologicalHazard::Avalanche:
            // Direction extracted from fall zones if available
            FVector Direction = EventData.FallZones.Num() > 0 ? 
                (EventData.FallZones[0] - EventData.EpicenterLocation).GetSafeNormal() : 
                FVector(0.0f, 0.0f, -1.0f);
            SimulateAvalanche(EventData.EpicenterLocation, Direction, EventData.Magnitude * 1000.0f);
            break;
        }
    }
    else
    {
        ServerTriggerGeologicalEvent(EventData);
    }
}

bool UEnvironmentalHazardManager::ServerTriggerGeologicalEvent_Validate(const FGeologicalData& EventData)
{
    // Validate geological event data
    return EventData.Magnitude >= 0.0f && EventData.Magnitude <= 10.0f &&
           EventData.Duration >= 0.0f && EventData.Duration <= 3600.0f; // Max 1 hour events
}

void UEnvironmentalHazardManager::ServerTriggerGeologicalEvent_Implementation(const FGeologicalData& EventData)
{
    TriggerGeologicalEvent(EventData);
}

FEnvironmentalEffects UEnvironmentalHazardManager::CalculateEnvironmentalEffects(const FVector& Location) const
{
    FEnvironmentalEffects Effects = CombinedEffects;

    // Apply distance-based modifications
    float DistanceFromEpicenter = FVector::Dist(Location, CurrentGeological.EpicenterLocation);
    float GeologicalInfluence = FMath::Max(0.0f, 1.0f - (DistanceFromEpicenter / CurrentGeological.AffectedRadius));

    // Modify effects based on geological influence
    if (bGeologicalEventActive && GeologicalInfluence > 0.0f)
    {
        Effects.MovementSpeedMultiplier *= FMath::Lerp(1.0f, 0.5f, GeologicalInfluence * CurrentGeological.Magnitude / 10.0f);
        Effects.ToolAccuracyMultiplier *= FMath::Lerp(1.0f, 0.3f, GeologicalInfluence * CurrentGeological.Magnitude / 10.0f);
    }

    // Apply wind-based balance offset at location
    FVector WindVelocity = GetWindVelocityAtLocation(Location);
    Effects.BalanceOffset = WindVelocity * 0.1f; // Convert wind to balance displacement

    return Effects;
}

float UEnvironmentalHazardManager::GetWindForceAtLocation(const FVector& Location) const
{
    // Use cached calculation if recent enough
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (FVector::Dist(Location, CachedWindLocation) < 100.0f && 
        CurrentTime - CachedWindTime < WindCacheTimeout)
    {
        return CachedWindForce;
    }

    // Calculate base wind force
    float BaseForce = FMath::Square(CurrentWind.Speed + CurrentWind.Gusts) * 0.5f; // Simplified drag equation

    // Apply turbulence variation
    float TurbulenceVariation = FMath::RandRange(-CurrentWind.Turbulence, CurrentWind.Turbulence);
    float TotalForce = BaseForce * (1.0f + TurbulenceVariation);

    // Cache the result
    CachedWindForce = TotalForce;
    CachedWindLocation = Location;
    CachedWindTime = CurrentTime;

    return TotalForce;
}

FVector UEnvironmentalHazardManager::GetWindVelocityAtLocation(const FVector& Location) const
{
    float WindForce = GetWindForceAtLocation(Location);
    float WindSpeed = FMath::Sqrt(WindForce * 2.0f); // Reverse of force calculation
    
    // Apply direction with turbulence
    FVector Direction = CurrentWind.Direction;
    if (CurrentWind.Turbulence > 0.0f)
    {
        float TurbulenceAngle = FMath::RandRange(-CurrentWind.DirectionVariation, CurrentWind.DirectionVariation) * CurrentWind.Turbulence;
        Direction = Direction.RotateAngleAxis(TurbulenceAngle, FVector::UpVector);
    }

    return Direction.GetSafeNormal() * WindSpeed;
}

float UEnvironmentalHazardManager::GetTemperatureAtLocation(const FVector& Location) const
{
    // Base temperature with altitude adjustment (-6.5°C per 1000m altitude gain)
    float Altitude = Location.Z / 100.0f; // Convert cm to meters
    float AltitudeAdjustment = -6.5f * (Altitude / 1000.0f);
    
    return CurrentWeather.Temperature + AltitudeAdjustment;
}

float UEnvironmentalHazardManager::GetVisibilityAtLocation(const FVector& Location) const
{
    float BaseVisibility = CurrentWeather.Visibility;
    
    // Reduce visibility in geological events (dust, debris)
    if (bGeologicalEventActive)
    {
        float DistanceFromEpicenter = FVector::Dist(Location, CurrentGeological.EpicenterLocation);
        float GeologicalInfluence = FMath::Max(0.0f, 1.0f - (DistanceFromEpicenter / CurrentGeological.AffectedRadius));
        
        if (CurrentGeological.HazardType == EGeologicalHazard::Earthquake ||
            CurrentGeological.HazardType == EGeologicalHazard::Rockfall)
        {
            // Dust clouds reduce visibility
            BaseVisibility *= FMath::Lerp(1.0f, 0.2f, GeologicalInfluence * CurrentGeological.Magnitude / 10.0f);
        }
    }
    
    return FMath::Clamp(BaseVisibility, 0.0f, 1.0f);
}

void UEnvironmentalHazardManager::ApplyWindToRope(UAdvancedRopeComponent* RopeComponent, float DeltaTime)
{
    if (!RopeComponent || !RopeComponent->CableComponent || !bEnableWindSimulation)
    {
        return;
    }

    // Get rope segment positions
    TArray<FVector> SegmentPositions = RopeComponent->GetRopeSegmentPositions();
    if (SegmentPositions.Num() == 0)
    {
        return;
    }

    // Apply wind force to each segment
    UCableComponent* Cable = RopeComponent->CableComponent;
    
    for (int32 i = 1; i < SegmentPositions.Num() - 1; ++i) // Skip end points (anchored)
    {
        FVector SegmentLocation = SegmentPositions[i];
        FVector WindVelocity = GetWindVelocityAtLocation(SegmentLocation);
        
        // Calculate wind force based on rope diameter and air resistance
        float RopeDiameter = RopeComponent->Properties.Diameter * 0.001f; // mm to meters
        float SegmentLength = RopeComponent->Properties.Length / SegmentPositions.Num();
        float CrossSectionalArea = RopeDiameter * SegmentLength; // Simplified for cylinder
        
        // Drag force: F = 0.5 * ρ * v² * Cd * A
        const float AirDensity = 1.225f; // kg/m³ at sea level
        const float DragCoefficient = 1.2f; // For cylinder
        float WindSpeedSquared = WindVelocity.SizeSquared();
        
        FVector WindForce = WindVelocity.GetSafeNormal() * 
                           (0.5f * AirDensity * WindSpeedSquared * DragCoefficient * CrossSectionalArea) * 
                           WindToRopeSwayMultiplier * GlobalIntensityMultiplier;
        
        // Apply force to cable component (this would typically require direct physics manipulation)
        // For now, we'll modify the cable's physics parameters to simulate wind effects
        Cable->bEnableStiffness = true;
        Cable->SubstepTime = FMath::Max(0.005f, 0.02f - (WindVelocity.Size() * 0.001f)); // Increase substeps in high wind
        
        // Modify cable properties to simulate wind buffeting
        Cable->CableWidth *= (1.0f + CurrentWind.Turbulence * 0.1f); // Visual wind effect
    }
    
    // Update rope sway multiplier for the climbing system
    RopeComponent->PhysicsState.CurrentTension += GetWindForceAtLocation(SegmentPositions[SegmentPositions.Num()/2]) * 0.1f;
}

void UEnvironmentalHazardManager::ApplyEnvironmentalEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime)
{
    if (!ClimbingComponent)
    {
        return;
    }

    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    FEnvironmentalEffects LocalEffects = CalculateEnvironmentalEffects(ClimberLocation);

    // Apply grip strength modifications
    float GripModifier = LocalEffects.GripStrengthMultiplier;
    
    // Weather effects on grip
    if (bEnableWeatherEffects)
    {
        switch (CurrentWeather.Type)
        {
        case EWeatherType::LightRain:
        case EWeatherType::HeavyRain:
            GripModifier *= FMath::Lerp(1.0f, RainToGripMultiplier, CurrentWeather.Intensity);
            break;
        case EWeatherType::Snow:
        case EWeatherType::Blizzard:
            GripModifier *= FMath::Lerp(1.0f, SnowToFrictionMultiplier, CurrentWeather.Intensity);
            break;
        }
    }
    
    // Temperature effects on stamina
    float Temperature = GetTemperatureAtLocation(ClimberLocation);
    float StaminaModifier = LocalEffects.StaminaDrainMultiplier;
    
    if (Temperature < 0.0f) // Cold increases stamina drain
    {
        StaminaModifier *= FMath::Lerp(1.0f, ColdToStaminaMultiplier, FMath::Abs(Temperature) / 20.0f);
    }
    else if (Temperature > 35.0f) // Heat increases stamina drain
    {
        StaminaModifier *= FMath::Lerp(1.0f, 1.5f, (Temperature - 35.0f) / 15.0f);
    }
    
    // Apply wind-induced balance effects
    if (bEnableWindSimulation && CurrentWind.Speed > 5.0f) // Only in significant wind
    {
        FVector WindForce = GetWindVelocityAtLocation(ClimberLocation) * 0.01f; // Scale down for balance
        
        // Add balance challenge to climbing
        if (ClimbingComponent->ClimbingState.CustomMovementMode == ECustomMovementMode::CMOVE_Climbing)
        {
            ClimbingComponent->ConsumeStamina(WindForce.Size() * DeltaTime * 2.0f);
        }
    }
    
    // Modify climbing component settings temporarily
    ClimbingComponent->Settings.BaseStaminaDrainRate *= StaminaModifier;
    ClimbingComponent->Settings.GripRecoveryRate *= GripModifier;
    ClimbingComponent->Settings.ClimbingSpeed *= LocalEffects.MovementSpeedMultiplier;
}

void UEnvironmentalHazardManager::UpdateEquipmentDurability(AClimbingToolBase* Tool, float DeltaTime)
{
    if (!Tool || !bEnableWeatherEffects)
    {
        return;
    }

    FVector ToolLocation = Tool->GetActorLocation();
    float DurabilityLoss = 0.0f;

    // Weather-based durability loss
    switch (CurrentWeather.Type)
    {
    case EWeatherType::HeavyRain:
        DurabilityLoss += CurrentWeather.Intensity * 0.1f * DeltaTime; // Corrosion from rain
        break;
    case EWeatherType::Blizzard:
        DurabilityLoss += CurrentWeather.Intensity * 0.05f * DeltaTime; // Metal fatigue from cold
        break;
    case EWeatherType::Sandstorm:
        DurabilityLoss += CurrentWeather.Intensity * 0.2f * DeltaTime; // Abrasion from sand
        break;
    }

    // Temperature effects
    float Temperature = GetTemperatureAtLocation(ToolLocation);
    if (Temperature < -10.0f || Temperature > 50.0f)
    {
        DurabilityLoss += FMath::Abs(Temperature - 20.0f) * 0.001f * DeltaTime; // Extreme temperature stress
    }

    // Apply durability loss (this would interface with the tool's durability system)
    // Tool->ModifyDurability(-DurabilityLoss * GlobalIntensityMultiplier);
}

void UEnvironmentalHazardManager::StartWeatherTransition(EWeatherType TargetWeather, float TransitionDuration)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    TransitionTarget = TargetWeather;
    this->TransitionDuration = TransitionDuration;
    TransitionProgress = 0.0f;
    TransitionStartWeather = CurrentWeather;
    bIsTransitioning = true;
}

void UEnvironmentalHazardManager::UpdateWeatherTransition(float DeltaTime)
{
    if (!bIsTransitioning)
    {
        return;
    }

    TransitionProgress += DeltaTime / TransitionDuration;
    
    if (TransitionProgress >= 1.0f)
    {
        // Transition complete
        CurrentWeather.Type = TransitionTarget;
        TransitionProgress = 1.0f;
        bIsTransitioning = false;
    }

    // Interpolate weather parameters
    float Alpha = FMath::SmoothStep(0.0f, 1.0f, TransitionProgress);
    
    // Set target weather properties based on type
    FWeatherData TargetWeatherData = TransitionStartWeather;
    TargetWeatherData.Type = TransitionTarget;
    
    switch (TransitionTarget)
    {
    case EWeatherType::Clear:
        TargetWeatherData.Intensity = 0.0f;
        TargetWeatherData.Precipitation = 0.0f;
        TargetWeatherData.Visibility = 1.0f;
        break;
    case EWeatherType::LightRain:
        TargetWeatherData.Intensity = 0.3f;
        TargetWeatherData.Precipitation = 5.0f;
        TargetWeatherData.Visibility = 0.8f;
        TargetWeatherData.Humidity = 90.0f;
        break;
    case EWeatherType::HeavyRain:
        TargetWeatherData.Intensity = 0.8f;
        TargetWeatherData.Precipitation = 25.0f;
        TargetWeatherData.Visibility = 0.4f;
        TargetWeatherData.Humidity = 95.0f;
        break;
    case EWeatherType::Snow:
        TargetWeatherData.Intensity = 0.5f;
        TargetWeatherData.Temperature = -5.0f;
        TargetWeatherData.Precipitation = 2.0f;
        TargetWeatherData.Visibility = 0.6f;
        break;
    case EWeatherType::Blizzard:
        TargetWeatherData.Intensity = 1.0f;
        TargetWeatherData.Temperature = -15.0f;
        TargetWeatherData.Precipitation = 15.0f;
        TargetWeatherData.Visibility = 0.1f;
        break;
    case EWeatherType::Fog:
        TargetWeatherData.Intensity = 0.7f;
        TargetWeatherData.Visibility = 0.2f;
        TargetWeatherData.Humidity = 100.0f;
        break;
    case EWeatherType::Sandstorm:
        TargetWeatherData.Intensity = 0.9f;
        TargetWeatherData.Visibility = 0.1f;
        TargetWeatherData.Temperature = 40.0f;
        break;
    }

    // Interpolate between start and target weather
    CurrentWeather.Intensity = FMath::Lerp(TransitionStartWeather.Intensity, TargetWeatherData.Intensity, Alpha);
    CurrentWeather.Temperature = FMath::Lerp(TransitionStartWeather.Temperature, TargetWeatherData.Temperature, Alpha);
    CurrentWeather.Humidity = FMath::Lerp(TransitionStartWeather.Humidity, TargetWeatherData.Humidity, Alpha);
    CurrentWeather.Precipitation = FMath::Lerp(TransitionStartWeather.Precipitation, TargetWeatherData.Precipitation, Alpha);
    CurrentWeather.Visibility = FMath::Lerp(TransitionStartWeather.Visibility, TargetWeatherData.Visibility, Alpha);
}

void UEnvironmentalHazardManager::SimulateRockfall(const FVector& Origin, float Intensity, float Duration)
{
    if (!bEnableGeologicalHazards)
    {
        return;
    }

    // This would spawn debris actors and simulate physics-based rockfall
    // For now, we'll set up the geological data structure
    CurrentGeological.HazardType = EGeologicalHazard::Rockfall;
    CurrentGeological.EpicenterLocation = Origin;
    CurrentGeological.Magnitude = Intensity;
    CurrentGeological.Duration = Duration;
    CurrentGeological.AffectedRadius = Intensity * 500.0f; // 5m per intensity point
    
    // Calculate fall zones (simplified - would be more complex in real implementation)
    CurrentGeological.FallZones.Empty();
    int32 NumZones = FMath::RoundToInt(Intensity * 3.0f);
    for (int32 i = 0; i < NumZones; ++i)
    {
        FVector RandomOffset = FVector(
            FMath::RandRange(-CurrentGeological.AffectedRadius, CurrentGeological.AffectedRadius),
            FMath::RandRange(-CurrentGeological.AffectedRadius, CurrentGeological.AffectedRadius),
            FMath::RandRange(-200.0f, 0.0f) // Fall downward
        );
        CurrentGeological.FallZones.Add(Origin + RandomOffset);
    }
}

void UEnvironmentalHazardManager::SimulateEarthquake(const FVector& Epicenter, float Magnitude, float Duration)
{
    if (!bEnableGeologicalHazards)
    {
        return;
    }

    CurrentGeological.HazardType = EGeologicalHazard::Earthquake;
    CurrentGeological.EpicenterLocation = Epicenter;
    CurrentGeological.Magnitude = Magnitude;
    CurrentGeological.Duration = Duration;
    CurrentGeological.AffectedRadius = Magnitude * 1000.0f; // 10m per magnitude point
    CurrentGeological.Severity = static_cast<EHazardSeverity>(FMath::RoundToInt(Magnitude / 2.0f));
}

void UEnvironmentalHazardManager::SimulateAvalanche(const FVector& StartLocation, const FVector& Direction, float Volume)
{
    if (!bEnableGeologicalHazards)
    {
        return;
    }

    CurrentGeological.HazardType = EGeologicalHazard::Avalanche;
    CurrentGeological.EpicenterLocation = StartLocation;
    CurrentGeological.Magnitude = Volume / 1000.0f; // Convert volume to magnitude scale
    CurrentGeological.Duration = 60.0f; // Avalanches typically last about a minute
    CurrentGeological.AffectedRadius = FMath::Sqrt(Volume) * 10.0f; // Area grows with square root of volume
    
    // Calculate avalanche path
    CurrentGeological.FallZones.Empty();
    FVector CurrentPos = StartLocation;
    for (int32 i = 0; i < 10; ++i)
    {
        CurrentPos += Direction * (100.0f + i * 50.0f); // Accelerating avalanche
        CurrentGeological.FallZones.Add(CurrentPos);
    }
}

void UEnvironmentalHazardManager::UpdateWindEffects(float DeltaTime)
{
    if (!bEnableWindSimulation)
    {
        return;
    }

    // Update wind direction variation
    WindDirectionTimer += DeltaTime;
    if (WindDirectionTimer >= 5.0f) // Change direction every 5 seconds
    {
        float DirectionChange = FMath::RandRange(-CurrentWind.DirectionVariation, CurrentWind.DirectionVariation);
        FRotator NewRotation = BaseWindDirection.Rotation();
        NewRotation.Yaw += DirectionChange;
        CurrentWind.Direction = NewRotation.Vector();
        WindDirectionTimer = 0.0f;
    }

    // Update wind gusts
    WindGustTimer += DeltaTime;
    float GustInterval = 60.0f / FMath::Max(0.1f, CurrentWind.GustFrequency);
    if (WindGustTimer >= GustInterval)
    {
        CurrentWind.Gusts = FMath::RandRange(0.0f, CurrentWind.Speed * 0.5f);
        WindGustTimer = 0.0f;
    }
    else
    {
        // Decay gusts over time
        CurrentWind.Gusts = FMath::FInterpTo(CurrentWind.Gusts, 0.0f, DeltaTime, 2.0f);
    }
}

void UEnvironmentalHazardManager::UpdateWeatherEffects(float DeltaTime)
{
    if (!bEnableWeatherEffects)
    {
        return;
    }

    // Update weather-dependent wind
    switch (CurrentWeather.Type)
    {
    case EWeatherType::Blizzard:
    case EWeatherType::Sandstorm:
        CurrentWind.Speed = FMath::Max(CurrentWind.Speed, CurrentWeather.Intensity * 20.0f);
        CurrentWind.Turbulence = FMath::Max(CurrentWind.Turbulence, CurrentWeather.Intensity * 0.8f);
        break;
    case EWeatherType::HeavyRain:
        CurrentWind.Speed = FMath::Max(CurrentWind.Speed, CurrentWeather.Intensity * 10.0f);
        break;
    }

    // Process precipitation effects on surfaces
    if (CurrentWeather.Precipitation > 0.0f)
    {
        ProcessPrecipitationEffects(DeltaTime);
    }
}

void UEnvironmentalHazardManager::UpdateGeologicalEffects(float DeltaTime)
{
    if (!bEnableGeologicalHazards || !bGeologicalEventActive)
    {
        return;
    }

    GeologicalEventTimer += DeltaTime;
    
    if (GeologicalEventTimer >= CurrentGeological.Duration)
    {
        // End geological event
        bGeologicalEventActive = false;
        CurrentGeological.HazardType = EGeologicalHazard::None;
        CurrentGeological.Magnitude = 0.0f;
        OnGeologicalEventEnd.Broadcast();
        
        // Clean up spawned debris
        for (AActor* Debris : SpawnedDebris)
        {
            if (IsValid(Debris))
            {
                Debris->Destroy();
            }
        }
        SpawnedDebris.Empty();
    }
    else
    {
        ProcessActiveGeologicalEvent(DeltaTime);
    }
}

void UEnvironmentalHazardManager::UpdateCombinedEffects()
{
    // Reset effects to baseline
    CombinedEffects.GripStrengthMultiplier = 1.0f;
    CombinedEffects.FrictionMultiplier = 1.0f;
    CombinedEffects.StaminaDrainMultiplier = 1.0f;
    CombinedEffects.ToolAccuracyMultiplier = 1.0f;
    CombinedEffects.RopeSwayMultiplier = 1.0f;
    CombinedEffects.VisibilityMultiplier = 1.0f;
    CombinedEffects.EquipmentDurabilityMultiplier = 1.0f;
    CombinedEffects.MovementSpeedMultiplier = 1.0f;
    CombinedEffects.BalanceOffset = FVector::ZeroVector;

    // Apply wind effects
    if (bEnableWindSimulation && CurrentWind.Speed > 0.0f)
    {
        float WindEffect = FMath::Clamp(CurrentWind.Speed / 30.0f, 0.0f, 1.0f); // Normalize to 30 m/s max
        CombinedEffects.ToolAccuracyMultiplier *= (1.0f - WindEffect * 0.3f);
        CombinedEffects.RopeSwayMultiplier *= (1.0f + WindEffect * 2.0f);
        CombinedEffects.MovementSpeedMultiplier *= (1.0f - WindEffect * 0.2f);
    }

    // Apply weather effects
    if (bEnableWeatherEffects)
    {
        float WeatherIntensity = CurrentWeather.Intensity;
        
        switch (CurrentWeather.Type)
        {
        case EWeatherType::LightRain:
        case EWeatherType::HeavyRain:
            CombinedEffects.GripStrengthMultiplier *= FMath::Lerp(1.0f, RainToGripMultiplier, WeatherIntensity);
            CombinedEffects.FrictionMultiplier *= FMath::Lerp(1.0f, 0.8f, WeatherIntensity);
            break;
        case EWeatherType::Snow:
        case EWeatherType::Blizzard:
            CombinedEffects.GripStrengthMultiplier *= FMath::Lerp(1.0f, SnowToFrictionMultiplier, WeatherIntensity);
            CombinedEffects.StaminaDrainMultiplier *= FMath::Lerp(1.0f, ColdToStaminaMultiplier, WeatherIntensity);
            CombinedEffects.MovementSpeedMultiplier *= FMath::Lerp(1.0f, 0.7f, WeatherIntensity);
            break;
        case EWeatherType::Fog:
        case EWeatherType::Sandstorm:
            CombinedEffects.VisibilityMultiplier *= FMath::Lerp(1.0f, FogToVisibilityMultiplier, WeatherIntensity);
            break;
        }
        
        CombinedEffects.VisibilityMultiplier *= CurrentWeather.Visibility;
    }

    // Apply geological effects
    if (bEnableGeologicalHazards && bGeologicalEventActive)
    {
        float GeologicalIntensity = CurrentGeological.Magnitude / 10.0f;
        CombinedEffects.StaminaDrainMultiplier *= (1.0f + GeologicalIntensity * 0.5f);
        CombinedEffects.ToolAccuracyMultiplier *= (1.0f - GeologicalIntensity * 0.4f);
        CombinedEffects.MovementSpeedMultiplier *= (1.0f - GeologicalIntensity * 0.3f);
        
        if (CurrentGeological.HazardType == EGeologicalHazard::Earthquake)
        {
            // Add seismic vibration to balance offset
            float SeismicOffset = FMath::Sin(GetWorld()->GetTimeSeconds() * GeologicalIntensity * 20.0f) * GeologicalIntensity * 10.0f;
            CombinedEffects.BalanceOffset += FVector(SeismicOffset, 0.0f, 0.0f);
        }
    }

    // Apply global intensity multiplier
    CombinedEffects.GripStrengthMultiplier = FMath::Lerp(1.0f, CombinedEffects.GripStrengthMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.FrictionMultiplier = FMath::Lerp(1.0f, CombinedEffects.FrictionMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.StaminaDrainMultiplier = FMath::Lerp(1.0f, CombinedEffects.StaminaDrainMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.ToolAccuracyMultiplier = FMath::Lerp(1.0f, CombinedEffects.ToolAccuracyMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.RopeSwayMultiplier = FMath::Lerp(1.0f, CombinedEffects.RopeSwayMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.VisibilityMultiplier = FMath::Lerp(1.0f, CombinedEffects.VisibilityMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.EquipmentDurabilityMultiplier = FMath::Lerp(1.0f, CombinedEffects.EquipmentDurabilityMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.MovementSpeedMultiplier = FMath::Lerp(1.0f, CombinedEffects.MovementSpeedMultiplier, GlobalIntensityMultiplier);
    CombinedEffects.BalanceOffset *= GlobalIntensityMultiplier;
}

void UEnvironmentalHazardManager::ProcessPrecipitationEffects(float DeltaTime)
{
    // Calculate surface wetness accumulation over time
    // This would typically interface with material wetness systems
    float WetnessAccumulation = CurrentWeather.Precipitation * DeltaTime * 0.01f;
    
    // Find all climbable surfaces in range and apply wetness
    // This is a simplified implementation - would need surface material system
}

void UEnvironmentalHazardManager::ProcessActiveGeologicalEvent(float DeltaTime)
{
    switch (CurrentGeological.HazardType)
    {
    case EGeologicalHazard::Rockfall:
        // Spawn falling rocks periodically
        if (FMath::RandRange(0.0f, 1.0f) < CurrentGeological.Magnitude * 0.1f * DeltaTime)
        {
            SpawnRockfallDebris(CurrentGeological.EpicenterLocation, CurrentGeological.Magnitude);
        }
        break;
        
    case EGeologicalHazard::Earthquake:
        ApplySeismicForces(DeltaTime);
        break;
        
    case EGeologicalHazard::Avalanche:
        // Avalanche physics would be implemented here
        // For now, just create danger zones
        break;
    }
}

void UEnvironmentalHazardManager::SpawnRockfallDebris(const FVector& Location, float Intensity)
{
    // This would spawn actual rock debris actors
    // For now, we'll just add to the fall zones array
    if (CurrentGeological.FallZones.Num() < MaxDebrisObjects)
    {
        FVector RandomOffset = FVector(
            FMath::RandRange(-100.0f, 100.0f),
            FMath::RandRange(-100.0f, 100.0f),
            FMath::RandRange(0.0f, 200.0f)
        );
        CurrentGeological.FallZones.Add(Location + RandomOffset);
    }
}

void UEnvironmentalHazardManager::ApplySeismicForces(float DeltaTime)
{
    if (!GetWorld())
    {
        return;
    }

    // Find all physics objects in the affected area
    TArray<AActor*> AffectedActors = GetAffectedActors(CurrentGeological.EpicenterLocation, CurrentGeological.AffectedRadius);
    
    float SeismicIntensity = CurrentGeological.Magnitude / 10.0f;
    float FrequencyVariation = FMath::Sin(GetWorld()->GetTimeSeconds() * 10.0f * SeismicIntensity);
    
    for (AActor* Actor : AffectedActors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }
        
        UPrimitiveComponent* PrimComp = Actor->FindComponentByClass<UPrimitiveComponent>();
        if (PrimComp && PrimComp->IsSimulatingPhysics())
        {
            // Apply seismic force
            float Distance = FVector::Dist(Actor->GetActorLocation(), CurrentGeological.EpicenterLocation);
            float ForceMultiplier = FMath::Max(0.0f, 1.0f - (Distance / CurrentGeological.AffectedRadius));
            
            FVector SeismicForce = FVector(
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-0.5f, 0.5f)
            ).GetSafeNormal() * SeismicIntensity * ForceMultiplier * 1000.0f * FrequencyVariation;
            
            PrimComp->AddForce(SeismicForce);
        }
    }
}

TArray<AActor*> UEnvironmentalHazardManager::GetAffectedActors(const FVector& Center, float Radius) const
{
    TArray<AActor*> FoundActors;
    
    if (!GetWorld())
    {
        return FoundActors;
    }
    
    // Use sphere trace to find actors
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(GetOwner());
    
    TArray<FOverlapResult> OverlapResults;
    GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        Center,
        FQuat::Identity,
        ECC_WorldDynamic,
        FCollisionShape::MakeSphere(Radius),
        QueryParams
    );
    
    for (const FOverlapResult& Result : OverlapResults)
    {
        if (Result.GetActor())
        {
            FoundActors.AddUnique(Result.GetActor());
        }
    }
    
    return FoundActors;
}

bool UEnvironmentalHazardManager::ShouldSimulateFullDetail() const
{
    // Check if any players are within simulation range
    if (!GetWorld())
    {
        return false;
    }
    
    FVector ManagerLocation = GetOwner()->GetActorLocation();
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), ManagerLocation);
            if (Distance <= MaxSimulationDistance)
            {
                ViewerDistance = Distance;
                return true;
            }
        }
    }
    
    return false;
}

void UEnvironmentalHazardManager::SetEnvironmentalLOD(int32 LODLevel)
{
    CurrentLODLevel = FMath::Clamp(LODLevel, 0, 3);
    
    // Adjust update frequencies based on LOD
    switch (CurrentLODLevel)
    {
    case 0: // High detail
        FullUpdateInterval = 0.05f; // 20Hz
        LightUpdateInterval = 0.1f; // 10Hz
        break;
    case 1: // Medium detail
        FullUpdateInterval = 0.1f; // 10Hz
        LightUpdateInterval = 0.2f; // 5Hz
        break;
    case 2: // Low detail
        FullUpdateInterval = 0.2f; // 5Hz
        LightUpdateInterval = 0.5f; // 2Hz
        break;
    case 3: // Minimal detail
        FullUpdateInterval = 0.5f; // 2Hz
        LightUpdateInterval = 1.0f; // 1Hz
        break;
    }
}

void UEnvironmentalHazardManager::OptimizeForDistance(float ViewerDistance)
{
    // Automatically set LOD based on distance
    if (ViewerDistance < 1000.0f) // 10m
    {
        SetEnvironmentalLOD(0);
    }
    else if (ViewerDistance < 2500.0f) // 25m
    {
        SetEnvironmentalLOD(1);
    }
    else if (ViewerDistance < 5000.0f) // 50m
    {
        SetEnvironmentalLOD(2);
    }
    else
    {
        SetEnvironmentalLOD(3);
    }
    
    this->ViewerDistance = ViewerDistance;
}