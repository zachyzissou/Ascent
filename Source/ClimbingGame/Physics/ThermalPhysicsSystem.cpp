#include "ThermalPhysicsSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "../Tools/ClimbingToolBase.h"

UThermalPhysicsSystem::UThermalPhysicsSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    SetIsReplicatedByDefault(true);

    // Initialize material properties
    InitializeMaterialProperties();
}

void UThermalPhysicsSystem::BeginPlay()
{
    Super::BeginPlay();

    // Try to find environmental hazard manager
    if (!HazardManager)
    {
        HazardManager = GetOwner()->FindComponentByClass<UEnvironmentalHazardManager>();
    }

    // Load thermal properties
    LoadThermalPropertiesTable();

    // Initialize thermal grid if enabled
    if (bUseThermalGrid)
    {
        // This would initialize a 3D grid for thermal simulation
        // For now, we'll use a simplified zone-based approach
    }

    // Set initial ground and ambient temperatures
    CalculateGroundTemperature();
}

void UThermalPhysicsSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return; // Only simulate on server
    }

    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    // Update thermal systems based on LOD and timing
    if (CurrentTime - LastZoneUpdate > ZoneUpdateInterval)
    {
        UpdateThermalZones(DeltaTime);
        LastZoneUpdate = CurrentTime;
    }

    if (CurrentTime - LastObjectUpdate > ObjectUpdateInterval)
    {
        UpdateThermalObjects(DeltaTime);
        LastObjectUpdate = CurrentTime;
    }

    if (CurrentTime - LastHeatTransferUpdate > HeatTransferInterval)
    {
        ProcessHeatTransfer(DeltaTime);
        LastHeatTransferUpdate = CurrentTime;
    }

    if (bSimulateThermalExpansion && CurrentTime - LastExpansionUpdate > ExpansionUpdateInterval)
    {
        UpdateThermalExpansion(DeltaTime);
        LastExpansionUpdate = CurrentTime;
    }

    // Process environmental effects
    ProcessEnvironmentalHeating(DeltaTime);
    ApplyWeatherThermalEffects(DeltaTime);

    // Clear old cache entries
    if (bUseThermalCache && CurrentTime - LastCacheUpdate > CacheTimeout)
    {
        TemperatureCache.Empty();
        TemperatureGradientCache.Empty();
        LastCacheUpdate = CurrentTime;
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
        
        OptimizeThermalSimulation(ClosestPlayerDistance);
        LastLODUpdate = CurrentTime;
    }
}

void UThermalPhysicsSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UThermalPhysicsSystem, ThermalZones);
    DOREPLIFETIME(UThermalPhysicsSystem, ThermalObjects);
}

void UThermalPhysicsSystem::RegisterThermalZone(const FThermalZone& Zone)
{
    // Check if zone already exists
    for (int32 i = 0; i < ThermalZones.Num(); ++i)
    {
        if (ThermalZones[i].ZoneID == Zone.ZoneID)
        {
            ThermalZones[i] = Zone; // Update existing
            return;
        }
    }
    
    ThermalZones.Add(Zone);
}

void UThermalPhysicsSystem::UnregisterThermalZone(const FString& ZoneID)
{
    ThermalZones.RemoveAll([&ZoneID](const FThermalZone& Zone)
    {
        return Zone.ZoneID == ZoneID;
    });
}

FThermalZone UThermalPhysicsSystem::GetThermalZone(const FString& ZoneID) const
{
    for (const FThermalZone& Zone : ThermalZones)
    {
        if (Zone.ZoneID == ZoneID)
        {
            return Zone;
        }
    }
    
    // Return default zone if not found
    FThermalZone DefaultZone;
    DefaultZone.ZoneID = TEXT("Default");
    DefaultZone.BaseTemperature = AmbientTemperature;
    DefaultZone.CurrentTemperature = AmbientTemperature;
    return DefaultZone;
}

TArray<FString> UThermalPhysicsSystem::GetThermalZonesAtLocation(const FVector& Location) const
{
    TArray<FString> ZoneIDs;
    
    for (const FThermalZone& Zone : ThermalZones)
    {
        // Check if location is within zone bounds
        FVector ZoneMin = Zone.Center - Zone.Extent;
        FVector ZoneMax = Zone.Center + Zone.Extent;
        
        if (Location.X >= ZoneMin.X && Location.X <= ZoneMax.X &&
            Location.Y >= ZoneMin.Y && Location.Y <= ZoneMax.Y &&
            Location.Z >= ZoneMin.Z && Location.Z <= ZoneMax.Z)
        {
            ZoneIDs.Add(Zone.ZoneID);
        }
    }
    
    return ZoneIDs;
}

void UThermalPhysicsSystem::RegisterThermalObject(const FThermalObject& Object)
{
    if (ThermalObjects.Num() >= MaxThermalObjects)
    {
        // Remove oldest object to make room
        ThermalObjects.RemoveAt(0);
    }

    // Check if object already exists
    for (int32 i = 0; i < ThermalObjects.Num(); ++i)
    {
        if (ThermalObjects[i].ObjectID == Object.ObjectID)
        {
            ThermalObjects[i] = Object; // Update existing
            return;
        }
    }
    
    ThermalObjects.Add(Object);
}

void UThermalPhysicsSystem::UnregisterThermalObject(const FString& ObjectID)
{
    ThermalObjects.RemoveAll([&ObjectID](const FThermalObject& Object)
    {
        return Object.ObjectID == ObjectID;
    });
}

FThermalObject UThermalPhysicsSystem::GetThermalObject(const FString& ObjectID) const
{
    for (const FThermalObject& Object : ThermalObjects)
    {
        if (Object.ObjectID == ObjectID)
        {
            return Object;
        }
    }
    
    // Return default object if not found
    FThermalObject DefaultObject;
    DefaultObject.ObjectID = TEXT("Default");
    DefaultObject.CurrentTemperature = AmbientTemperature;
    DefaultObject.ThermalProps = GetMaterialThermalProperties(EThermalMaterial::Rock);
    return DefaultObject;
}

float UThermalPhysicsSystem::GetTemperatureAtLocation(const FVector& Location) const
{
    // Use cache if available
    if (bUseThermalCache)
    {
        if (const float* CachedTemp = TemperatureCache.Find(Location))
        {
            return *CachedTemp;
        }
    }

    float ResultTemperature = AmbientTemperature;
    float TotalWeight = 0.0f;

    // Sample from thermal zones
    TArray<FString> ZoneIDs = GetThermalZonesAtLocation(Location);
    
    if (ZoneIDs.Num() > 0)
    {
        for (const FString& ZoneID : ZoneIDs)
        {
            FThermalZone Zone = GetThermalZone(ZoneID);
            
            // Calculate distance-based weighting within zone
            float Distance = FVector::Dist(Location, Zone.Center);
            float MaxDistance = FMath::Max(Zone.Extent.X, FMath::Max(Zone.Extent.Y, Zone.Extent.Z));
            float Weight = 1.0f - (Distance / MaxDistance);
            Weight = FMath::Max(0.1f, Weight); // Minimum weight
            
            ResultTemperature += Zone.CurrentTemperature * Weight;
            TotalWeight += Weight;
        }
        
        if (TotalWeight > 0.0f)
        {
            ResultTemperature /= TotalWeight;
        }
    }

    // Apply influence from nearby thermal objects
    for (const FThermalObject& Object : ThermalObjects)
    {
        float Distance = FVector::Dist(Location, Object.Location);
        float InfluenceRadius = FMath::Max(Object.Size.X, FMath::Max(Object.Size.Y, Object.Size.Z)) * 2.0f;
        
        if (Distance < InfluenceRadius)
        {
            float ObjectWeight = 1.0f - (Distance / InfluenceRadius);
            ObjectWeight *= 0.1f; // Objects have less influence than zones
            
            ResultTemperature += Object.CurrentTemperature * ObjectWeight;
            TotalWeight += ObjectWeight;
        }
    }

    // Apply altitude-based temperature decrease (lapse rate)
    float Altitude = Location.Z * 0.01f; // Convert cm to meters
    float TemperatureLapse = Altitude * 0.0065f; // Standard atmospheric lapse rate 6.5°C/km
    ResultTemperature -= TemperatureLapse;

    // Cache the result
    if (bUseThermalCache)
    {
        TemperatureCache.Add(Location, ResultTemperature);
    }

    return ResultTemperature;
}

FVector UThermalPhysicsSystem::GetTemperatureGradient(const FVector& Location) const
{
    // Use cache if available
    if (bUseThermalCache)
    {
        if (const FVector* CachedGradient = TemperatureGradientCache.Find(Location))
        {
            return *CachedGradient;
        }
    }

    // Calculate temperature gradient using finite differences
    float GridSpacing = 100.0f; // 1m spacing
    
    float TempX1 = GetTemperatureAtLocation(Location + FVector(GridSpacing, 0, 0));
    float TempX2 = GetTemperatureAtLocation(Location - FVector(GridSpacing, 0, 0));
    float TempY1 = GetTemperatureAtLocation(Location + FVector(0, GridSpacing, 0));
    float TempY2 = GetTemperatureAtLocation(Location - FVector(0, GridSpacing, 0));
    float TempZ1 = GetTemperatureAtLocation(Location + FVector(0, 0, GridSpacing));
    float TempZ2 = GetTemperatureAtLocation(Location - FVector(0, 0, GridSpacing));
    
    FVector Gradient;
    Gradient.X = (TempX1 - TempX2) / (2.0f * GridSpacing);
    Gradient.Y = (TempY1 - TempY2) / (2.0f * GridSpacing);
    Gradient.Z = (TempZ1 - TempZ2) / (2.0f * GridSpacing);
    
    // Cache the result
    if (bUseThermalCache)
    {
        TemperatureGradientCache.Add(Location, Gradient);
    }
    
    return Gradient;
}

float UThermalPhysicsSystem::CalculateApparentTemperature(const FVector& Location, float WindSpeed, float Humidity) const
{
    float AirTemp = GetTemperatureAtLocation(Location);
    
    // Calculate wind chill or heat index based on temperature
    if (AirTemp <= 10.0f && WindSpeed > 1.3f) // Wind chill conditions
    {
        return CalculateWindChillTemperature(AirTemp, WindSpeed);
    }
    else if (AirTemp >= 27.0f) // Heat index conditions
    {
        return CalculateHeatIndexTemperature(AirTemp, Humidity);
    }
    else
    {
        return AirTemp; // No apparent temperature adjustment
    }
}

float UThermalPhysicsSystem::CalculateWindChillTemperature(float AirTemp, float WindSpeed) const
{
    if (AirTemp > 10.0f || WindSpeed <= 1.3f)
    {
        return AirTemp; // Wind chill not applicable
    }
    
    // Convert wind speed from m/s to km/h for the formula
    float WindSpeedKMH = WindSpeed * 3.6f;
    
    // Wind chill formula (Environment Canada)
    float WindChill = 13.12f + 0.6215f * AirTemp - 11.37f * FMath::Pow(WindSpeedKMH, 0.16f) + 0.3965f * AirTemp * FMath::Pow(WindSpeedKMH, 0.16f);
    
    return WindChill;
}

float UThermalPhysicsSystem::CalculateHeatIndexTemperature(float AirTemp, float Humidity) const
{
    if (AirTemp < 27.0f)
    {
        return AirTemp; // Heat index not applicable
    }
    
    // Convert to Fahrenheit for the formula
    float TempF = ConvertCelsiusToFahrenheit(AirTemp);
    float RH = Humidity * 100.0f; // Convert to percentage
    
    // Rothfusz regression (NOAA)
    float HI = -42.379f + 2.04901523f * TempF + 10.14333127f * RH - 0.22475541f * TempF * RH 
               - 0.00683783f * TempF * TempF - 0.05481717f * RH * RH 
               + 0.00122874f * TempF * TempF * RH + 0.00085282f * TempF * RH * RH 
               - 0.00000199f * TempF * TempF * RH * RH;
    
    // Convert back to Celsius
    return ConvertFahrenheitToCelsius(HI);
}

FHeatTransferData UThermalPhysicsSystem::CalculateHeatTransfer(const FString& FromObjectID, const FString& ToObjectID) const
{
    FHeatTransferData HeatTransfer;
    
    FThermalObject FromObject = GetThermalObject(FromObjectID);
    FThermalObject ToObject = GetThermalObject(ToObjectID);
    
    float TempDifference = FromObject.CurrentTemperature - ToObject.CurrentTemperature;
    
    if (FMath::Abs(TempDifference) < 0.1f)
    {
        return HeatTransfer; // No significant temperature difference
    }
    
    float Distance = FVector::Dist(FromObject.Location, ToObject.Location);
    float ContactArea = FMath::Min(FromObject.Size.X * FromObject.Size.Y, ToObject.Size.X * ToObject.Size.Y) * 0.0001f; // cm² to m²
    
    // Conduction (if objects are in contact or very close)
    if (Distance < 50.0f) // Within 50cm
    {
        float AvgConductivity = (FromObject.ThermalProps.ThermalConductivity + ToObject.ThermalProps.ThermalConductivity) * 0.5f;
        HeatTransfer.ConductionRate = CalculateConductiveHeatTransfer(TempDifference, AvgConductivity, ContactArea, Distance * 0.01f);
    }
    
    // Convection (through air)
    if (Distance < 1000.0f) // Within 10m
    {
        float ConvectionCoeff = 10.0f; // W/(m²·K) for natural convection in air
        if (HazardManager)
        {
            float WindSpeed = HazardManager->GetWindVelocityAtLocation(FromObject.Location).Size();
            ConvectionCoeff += WindSpeed * 5.0f; // Forced convection
        }
        
        HeatTransfer.ConvectionRate = CalculateConvectiveHeatTransfer(TempDifference, ConvectionCoeff, ContactArea);
    }
    
    // Radiation (always present for significant temperature differences)
    if (FMath::Abs(TempDifference) > 5.0f)
    {
        float AvgEmissivity = (FromObject.ThermalProps.Emissivity + ToObject.ThermalProps.Emissivity) * 0.5f;
        HeatTransfer.RadiationRate = CalculateRadiativeHeatTransfer(FromObject.CurrentTemperature, ToObject.CurrentTemperature, AvgEmissivity, ContactArea);
    }
    
    // Total heat transfer
    HeatTransfer.TotalHeatFlow = HeatTransfer.ConductionRate + HeatTransfer.ConvectionRate + HeatTransfer.RadiationRate;
    
    // Direction of heat flow (from hot to cold)
    if (TempDifference > 0.0f)
    {
        HeatTransfer.HeatFlowDirection = (ToObject.Location - FromObject.Location).GetSafeNormal();
    }
    else
    {
        HeatTransfer.HeatFlowDirection = (FromObject.Location - ToObject.Location).GetSafeNormal();
    }
    
    return HeatTransfer;
}

float UThermalPhysicsSystem::CalculateConductiveHeatTransfer(float TempDifference, float ThermalConductivity, float Area, float Distance) const
{
    // Fourier's law: q = -kA(dT/dx)
    return ThermalConductivity * Area * TempDifference / FMath::Max(0.01f, Distance);
}

float UThermalPhysicsSystem::CalculateConvectiveHeatTransfer(float TempDifference, float HeatTransferCoeff, float Area) const
{
    // Newton's law of cooling: q = hA(T1 - T2)
    return HeatTransferCoeff * Area * TempDifference;
}

float UThermalPhysicsSystem::CalculateRadiativeHeatTransfer(float TempHot, float TempCold, float Emissivity, float Area) const
{
    // Stefan-Boltzmann law: q = εσA(T1⁴ - T2⁴)
    float T1Kelvin = ConvertCelsiusToKelvin(TempHot);
    float T2Kelvin = ConvertCelsiusToKelvin(TempCold);
    
    return Emissivity * StefanBoltzmannConstant * Area * (FMath::Pow(T1Kelvin, 4.0f) - FMath::Pow(T2Kelvin, 4.0f));
}

FVector UThermalPhysicsSystem::CalculateThermalExpansion(const FString& ObjectID, float TemperatureChange) const
{
    FThermalObject Object = GetThermalObject(ObjectID);
    
    // Linear thermal expansion: ΔL = αL₀ΔT
    FVector Expansion;
    Expansion.X = Object.ThermalProps.LinearExpansionCoeff * Object.Size.X * TemperatureChange;
    Expansion.Y = Object.ThermalProps.LinearExpansionCoeff * Object.Size.Y * TemperatureChange;
    Expansion.Z = Object.ThermalProps.LinearExpansionCoeff * Object.Size.Z * TemperatureChange;
    
    return Expansion * GlobalThermalMultiplier;
}

float UThermalPhysicsSystem::CalculateThermalStress(const FString& ObjectID, float TemperatureChange, bool bConstrained) const
{
    if (!bConstrained)
    {
        return 0.0f; // No stress if free to expand
    }
    
    FThermalObject Object = GetThermalObject(ObjectID);
    
    // Thermal stress: σ = EαΔT (for constrained expansion)
    float YoungsModulus = 200000000000.0f; // Pa - would get from material properties
    
    switch (Object.Material)
    {
    case EThermalMaterial::Rock:
        YoungsModulus = 70000000000.0f; // 70 GPa
        break;
    case EThermalMaterial::Metal:
        YoungsModulus = 200000000000.0f; // 200 GPa for steel
        break;
    case EThermalMaterial::Wood:
        YoungsModulus = 12000000000.0f; // 12 GPa
        break;
    default:
        YoungsModulus = 70000000000.0f;
        break;
    }
    
    float ThermalStress = YoungsModulus * Object.ThermalProps.LinearExpansionCoeff * FMath::Abs(TemperatureChange);
    
    return ThermalStress * GlobalThermalMultiplier;
}

void UThermalPhysicsSystem::ApplyThermalExpansionToObject(const FString& ObjectID, float DeltaTime)
{
    FThermalObject* Object = nullptr;
    for (FThermalObject& ThermalObj : ThermalObjects)
    {
        if (ThermalObj.ObjectID == ObjectID)
        {
            Object = &ThermalObj;
            break;
        }
    }
    
    if (!Object)
    {
        return;
    }
    
    float TemperatureChange = Object->CurrentTemperature - 20.0f; // Reference temperature
    
    if (FMath::Abs(TemperatureChange) < 1.0f)
    {
        return; // Negligible temperature change
    }
    
    // Calculate new thermal expansion
    FVector NewExpansion = CalculateThermalExpansion(ObjectID, TemperatureChange);
    
    // Apply expansion gradually
    float ExpansionRate = 1.0f / ThermalTimeConstant; // Expansion rate per second
    Object->ThermalExpansion = FMath::VInterpTo(Object->ThermalExpansion, NewExpansion, DeltaTime, ExpansionRate);
    
    // Calculate thermal stress if constrained
    if (Object->bHasConstraints)
    {
        Object->ThermalStress = CalculateThermalStress(ObjectID, TemperatureChange, true);
        
        // Check for stress limits
        float MaxAllowableStress = 100000000.0f; // 100 MPa - would vary by material
        if (Object->ThermalStress > MaxAllowableStress)
        {
            OnThermalStressLimit.Broadcast();
            // Could trigger structural failure or cracking
        }
    }
}

float UThermalPhysicsSystem::CalculateSolarHeatGain(const FVector& Location, const FVector& SurfaceNormal, float SolarIntensity) const
{
    // Assume sun direction (would be calculated based on time of day and location)
    FVector SunDirection = FVector(0.5f, 0.5f, -0.7f).GetSafeNormal(); // Arbitrary sun direction
    
    // Calculate angle between surface normal and sun direction
    float CosIncidenceAngle = FVector::DotProduct(SurfaceNormal, -SunDirection);
    CosIncidenceAngle = FMath::Max(0.0f, CosIncidenceAngle); // No heating on surfaces facing away from sun
    
    // Solar heat gain = Solar Intensity × Absorption Coefficient × cos(incidence angle)
    float SolarAbsorption = 0.7f; // Would get from surface material properties
    
    return SolarIntensity * SolarAbsorption * CosIncidenceAngle;
}

void UThermalPhysicsSystem::UpdateSolarHeating(float SolarIntensity, const FVector& SunDirection, float DeltaTime)
{
    if (CurrentLOD > 1) // Skip detailed solar heating in low LOD
    {
        return;
    }
    
    // Apply solar heating to thermal objects
    for (FThermalObject& Object : ThermalObjects)
    {
        // Calculate surface normal (simplified - assume upward facing)
        FVector SurfaceNormal = FVector(0.0f, 0.0f, 1.0f);
        
        // Calculate solar heat gain
        float SolarHeatGain = CalculateSolarHeatGain(Object.Location, SurfaceNormal, SolarIntensity);
        
        // Apply heat to object temperature
        if (SolarHeatGain > 0.0f)
        {
            float SurfaceArea = Object.Size.X * Object.Size.Y * 0.0001f; // cm² to m²
            float HeatInput = SolarHeatGain * SurfaceArea * DeltaTime; // Joules
            float TemperatureIncrease = HeatInput / (Object.Mass * Object.ThermalProps.SpecificHeat);
            
            Object.CurrentTemperature += TemperatureIncrease * GlobalThermalMultiplier;
        }
    }
    
    // Apply solar heating to thermal zones
    for (FThermalZone& Zone : ThermalZones)
    {
        if (Zone.ZoneType == EThermalZoneType::Solar)
        {
            // Direct solar heating for solar zones
            float TemperatureIncrease = SolarIntensity * 0.001f * DeltaTime * GlobalThermalMultiplier;
            Zone.CurrentTemperature += TemperatureIncrease;
        }
        else if (Zone.ZoneType == EThermalZoneType::Shade)
        {
            // Cooling effect in shade zones
            float TemperatureDecrease = SolarIntensity * 0.0002f * DeltaTime * GlobalThermalMultiplier;
            Zone.CurrentTemperature -= TemperatureDecrease;
        }
    }
}

void UThermalPhysicsSystem::ApplyThermalEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime)
{
    if (!ClimbingComponent)
    {
        return;
    }

    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    float ActivityLevel = 2.0f; // Moderate activity for climbing
    float ClothingLevel = 1.0f; // Standard climbing clothing
    
    FClimberThermalState ThermalState = CalculateClimberThermalState(ClimberLocation, ActivityLevel, ClothingLevel);
    
    // Apply thermal effects to climbing performance
    switch (ThermalState.ComfortLevel)
    {
    case EThermalComfortLevel::HypothermiaRisk:
        // Severe cold effects
        ClimbingComponent->ConsumeStamina(50.0f * DeltaTime); // Rapid stamina drain
        ClimbingComponent->ConsumeGripStrength(30.0f * DeltaTime); // Reduced grip from cold
        ClimbingComponent->Settings.ClimbingSpeed *= 0.5f; // Slower movement
        ClimbingComponent->Settings.GripRecoveryRate *= 0.3f; // Much slower recovery
        OnHypothermiaRisk.Broadcast();
        break;
        
    case EThermalComfortLevel::Cold:
        // Moderate cold effects
        ClimbingComponent->ConsumeStamina(20.0f * DeltaTime);
        ClimbingComponent->ConsumeGripStrength(15.0f * DeltaTime);
        ClimbingComponent->Settings.ClimbingSpeed *= 0.8f;
        ClimbingComponent->Settings.GripRecoveryRate *= 0.7f;
        break;
        
    case EThermalComfortLevel::Cool:
        // Slight cold effects
        ClimbingComponent->ConsumeStamina(5.0f * DeltaTime);
        ClimbingComponent->Settings.GripRecoveryRate *= 0.9f;
        break;
        
    case EThermalComfortLevel::Comfortable:
        // Optimal performance - no modifications
        break;
        
    case EThermalComfortLevel::Warm:
        // Slight heat effects
        ClimbingComponent->ConsumeStamina(10.0f * DeltaTime); // Increased sweating
        break;
        
    case EThermalComfortLevel::Hot:
        // Moderate heat effects
        ClimbingComponent->ConsumeStamina(25.0f * DeltaTime); // Heavy sweating
        ClimbingComponent->ConsumeGripStrength(10.0f * DeltaTime); // Sweaty palms
        ClimbingComponent->Settings.ClimbingSpeed *= 0.9f; // Slightly slower
        break;
        
    case EThermalComfortLevel::HeatStrokeRisk:
        // Severe heat effects
        ClimbingComponent->ConsumeStamina(60.0f * DeltaTime); // Extreme dehydration
        ClimbingComponent->ConsumeGripStrength(25.0f * DeltaTime); // Very sweaty palms
        ClimbingComponent->Settings.ClimbingSpeed *= 0.6f; // Much slower movement
        ClimbingComponent->Settings.BaseStaminaDrainRate *= 2.0f; // Double stamina drain
        OnHeatStrokeRisk.Broadcast();
        break;
    }
    
    // Apply shivering effects
    if (ThermalState.ShiveringIntensity > 0.1f)
    {
        // Shivering reduces precision and grip strength
        float ShiveringPenalty = ThermalState.ShiveringIntensity * 0.5f;
        ClimbingComponent->ConsumeGripStrength(ShiveringPenalty * 20.0f * DeltaTime);
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - ShiveringPenalty * 0.3f);
    }
}

void UThermalPhysicsSystem::ApplyThermalEffectsToRope(UAdvancedRopeComponent* Rope, float DeltaTime)
{
    if (!Rope)
    {
        return;
    }

    TArray<FVector> SegmentPositions = Rope->GetRopeSegmentPositions();
    
    // Calculate average temperature along rope
    float AverageTemp = 0.0f;
    for (const FVector& Position : SegmentPositions)
    {
        AverageTemp += GetTemperatureAtLocation(Position);
    }
    
    if (SegmentPositions.Num() > 0)
    {
        AverageTemp /= SegmentPositions.Num();
    }
    
    float TemperatureChange = AverageTemp - 20.0f; // Reference temperature
    
    // Apply thermal effects to rope properties
    if (FMath::Abs(TemperatureChange) > 5.0f)
    {
        // Thermal expansion affects rope length
        float ThermalExpansion = Rope->Properties.Length * 0.00002f * TemperatureChange; // Approximate for nylon
        
        // Cold makes rope stiffer, heat makes it more elastic
        if (TemperatureChange < 0.0f) // Cold
        {
            // Rope becomes stiffer and more brittle
            float ColdFactor = FMath::Min(FMath::Abs(TemperatureChange) / 30.0f, 0.3f);
            
            // Reduced dynamic elongation
            // Rope->ModifyElongation(Rope->Properties.DynamicElongation * (1.0f - ColdFactor));
            
            // Increased breaking risk in extreme cold
            if (AverageTemp < -20.0f)
            {
                // Rope->ModifyBreakingStrength(Rope->Properties.BreakingStrength * 0.8f);
            }
        }
        else // Hot
        {
            // Rope becomes more elastic but potentially weaker
            float HeatFactor = FMath::Min(TemperatureChange / 50.0f, 0.2f);
            
            // Increased dynamic elongation
            // Rope->ModifyElongation(Rope->Properties.DynamicElongation * (1.0f + HeatFactor));
            
            // Potential strength reduction in extreme heat
            if (AverageTemp > 60.0f)
            {
                // Rope->ModifyBreakingStrength(Rope->Properties.BreakingStrength * 0.9f);
            }
        }
        
        // Apply thermal expansion to cable component
        if (Rope->CableComponent)
        {
            // Visual representation of thermal expansion
            float ExpansionFactor = 1.0f + (ThermalExpansion / Rope->Properties.Length);
            // This would require modifying the cable's end-to-end length
        }
    }
}

void UThermalPhysicsSystem::ApplyThermalEffectsToEquipment(AActor* Equipment, float DeltaTime)
{
    if (!Equipment)
    {
        return;
    }

    FVector EquipmentLocation = Equipment->GetActorLocation();
    float EquipmentTemp = GetTemperatureAtLocation(EquipmentLocation);
    float TemperatureChange = EquipmentTemp - 20.0f;
    
    // Apply thermal expansion to equipment
    if (FMath::Abs(TemperatureChange) > 10.0f)
    {
        UStaticMeshComponent* MeshComp = Equipment->FindComponentByClass<UStaticMeshComponent>();
        if (MeshComp)
        {
            // Calculate thermal expansion
            float LinearExpansionCoeff = 0.000012f; // Steel default
            
            AClimbingToolBase* ClimbingTool = Cast<AClimbingToolBase>(Equipment);
            if (ClimbingTool)
            {
                // Different tools have different materials
                // This would interface with the tool's material system
            }
            
            float ExpansionFactor = 1.0f + LinearExpansionCoeff * TemperatureChange;
            FVector CurrentScale = MeshComp->GetComponentScale();
            FVector TargetScale = CurrentScale * ExpansionFactor;
            
            // Apply gradual thermal expansion
            float ExpansionRate = 1.0f / ThermalTimeConstant;
            FVector NewScale = FMath::VInterpTo(CurrentScale, TargetScale, DeltaTime, ExpansionRate);
            MeshComp->SetWorldScale3D(NewScale);
        }
    }
    
    // Extreme temperature effects on equipment functionality
    if (EquipmentTemp < -30.0f)
    {
        // Metal becomes brittle in extreme cold
        // ClimbingTool->ModifyDurability(-0.01f * DeltaTime);
    }
    else if (EquipmentTemp > 80.0f)
    {
        // Metal softens in extreme heat
        // ClimbingTool->ModifyStrength(-0.005f * DeltaTime);
    }
}

FClimberThermalState UThermalPhysicsSystem::CalculateClimberThermalState(const FVector& ClimberLocation, float ActivityLevel, float ClothingLevel) const
{
    FClimberThermalState ThermalState;
    
    float AmbientTemp = GetTemperatureAtLocation(ClimberLocation);
    float WindSpeed = 0.0f;
    float Humidity = 50.0f; // Default humidity
    
    if (HazardManager)
    {
        WindSpeed = HazardManager->GetWindVelocityAtLocation(ClimberLocation).Size();
        // Would get humidity from weather system
    }
    
    // Calculate apparent temperature
    float ApparentTemp = CalculateApparentTemperature(ClimberLocation, WindSpeed, Humidity * 0.01f);
    
    // Calculate metabolic heat generation
    ThermalState.MetabolicHeatGeneration = CalculateMetabolicHeatGeneration(ActivityLevel);
    
    // Simulate heat balance
    float HeatGeneration = ThermalState.MetabolicHeatGeneration; // W
    float HeatLoss = 0.0f;
    
    // Convective heat loss
    float ConvectiveCoeff = 8.0f + 6.0f * FMath::Sqrt(WindSpeed); // W/(m²·K)
    float BodySurfaceArea = 1.8f; // m² typical adult
    float ConvectiveLoss = ConvectiveCoeff * BodySurfaceArea * (ThermalState.SkinTemperature - ApparentTemp);
    HeatLoss += FMath::Max(0.0f, ConvectiveLoss);
    
    // Radiative heat loss
    float RadiativeLoss = 0.95f * StefanBoltzmannConstant * BodySurfaceArea * 
                         (FMath::Pow(ConvertCelsiusToKelvin(ThermalState.SkinTemperature), 4.0f) - 
                          FMath::Pow(ConvertCelsiusToKelvin(ApparentTemp), 4.0f));
    HeatLoss += FMath::Max(0.0f, RadiativeLoss);
    
    // Evaporative heat loss (sweating)
    if (ApparentTemp > 25.0f || ActivityLevel > 1.5f)
    {
        ThermalState.SweatRate = CalculateSweatRate(ThermalState.CoreTemperature, ThermalState.SkinTemperature, ActivityLevel);
        float EvaporativeLoss = ThermalState.SweatRate * 2430000.0f / 3600.0f; // J/kg latent heat of vaporization
        HeatLoss += EvaporativeLoss;
    }
    
    ThermalState.HeatLossRate = HeatLoss;
    
    // Update body temperatures based on heat balance
    float NetHeat = HeatGeneration - HeatLoss;
    float BodyThermalMass = 70.0f * 3500.0f; // kg * J/(kg·K) typical adult
    float CoreTempChange = NetHeat / BodyThermalMass;
    
    ThermalState.CoreTemperature += CoreTempChange * 0.01f; // Slow response
    ThermalState.CoreTemperature = FMath::Clamp(ThermalState.CoreTemperature, 30.0f, 45.0f);
    
    // Skin temperature responds faster to ambient conditions
    float SkinTempTarget = FMath::Lerp(ApparentTemp, ThermalState.CoreTemperature, 0.3f);
    ThermalState.SkinTemperature = FMath::Lerp(ThermalState.SkinTemperature, SkinTempTarget, 0.1f);
    
    // Calculate shivering response
    if (ThermalState.CoreTemperature < 36.5f)
    {
        ThermalState.ShiveringIntensity = (36.5f - ThermalState.CoreTemperature) / 6.5f; // Normalize to hypothermic range
        ThermalState.ShiveringIntensity = FMath::Clamp(ThermalState.ShiveringIntensity, 0.0f, 1.0f);
    }
    
    // Determine comfort level
    ThermalState.ComfortLevel = DetermineComfortLevel(ApparentTemp);
    
    return ThermalState;
}

EThermalComfortLevel UThermalPhysicsSystem::DetermineComfortLevel(float EffectiveTemp) const
{
    if (EffectiveTemp < -10.0f)
        return EThermalComfortLevel::HypothermiaRisk;
    else if (EffectiveTemp < 5.0f)
        return EThermalComfortLevel::Cold;
    else if (EffectiveTemp < 15.0f)
        return EThermalComfortLevel::Cool;
    else if (EffectiveTemp < 25.0f)
        return EThermalComfortLevel::Comfortable;
    else if (EffectiveTemp < 30.0f)
        return EThermalComfortLevel::Warm;
    else if (EffectiveTemp < 40.0f)
        return EThermalComfortLevel::Hot;
    else
        return EThermalComfortLevel::HeatStrokeRisk;
}

float UThermalPhysicsSystem::CalculateMetabolicHeatGeneration(float ActivityLevel) const
{
    // Base metabolic rate: ~100W for resting adult
    // Activity level: 1.0 = resting, 2.0 = moderate work, 3.0+ = heavy work
    
    float BaseMetabolicRate = 100.0f; // W
    return BaseMetabolicRate * ActivityLevel;
}

float UThermalPhysicsSystem::CalculateSweatRate(float CoreTemp, float SkinTemp, float ActivityLevel) const
{
    // Sweating response based on core and skin temperature
    float SweatThreshold = 37.0f; // Start sweating when core temp exceeds 37°C
    
    if (CoreTemp <= SweatThreshold)
    {
        return 0.0f;
    }
    
    // Sweat rate increases with core temperature and activity
    float TempExcess = CoreTemp - SweatThreshold;
    float BaseSweatRate = TempExcess * 0.5f; // L/hour per degree excess
    
    // Activity multiplier
    BaseSweatRate *= ActivityLevel;
    
    // Maximum sustainable sweat rate
    return FMath::Min(BaseSweatRate, 3.0f); // Max 3 L/hour
}

float UThermalPhysicsSystem::CalculatePMV(float AirTemp, float RadiantTemp, float WindSpeed, float Humidity, float ActivityLevel, float ClothingLevel) const
{
    // Predicted Mean Vote calculation (Fanger's equation)
    // Simplified implementation of the full PMV calculation
    
    float MetabolicRate = CalculateMetabolicHeatGeneration(ActivityLevel);
    
    // Thermal resistance of clothing (simplified)
    float ClothingInsulation = ClothingLevel * 0.155f; // Convert to m²K/W
    
    // Heat transfer coefficients
    float ConvectiveCoeff = 8.0f + 6.0f * FMath::Sqrt(WindSpeed);
    float RadiativeCoeff = 5.0f; // Simplified
    
    // Heat balance calculation (simplified PMV)
    float HeatGeneration = MetabolicRate;
    float HeatLoss = ConvectiveCoeff * (AirTemp - 35.0f) + RadiativeCoeff * (RadiantTemp - 35.0f);
    
    // PMV approximation
    float PMV = (HeatGeneration - HeatLoss) * 0.01f / ClothingInsulation;
    
    return FMath::Clamp(PMV, -3.0f, 3.0f);
}

float UThermalPhysicsSystem::CalculatePPD(float PMV) const
{
    // Predicted Percentage Dissatisfied
    // PPD = 100 - 95 * exp(-(0.03353 * PMV^4 + 0.2179 * PMV^2))
    
    float PMV2 = PMV * PMV;
    float PMV4 = PMV2 * PMV2;
    
    float PPD = 100.0f - 95.0f * FMath::Exp(-(0.03353f * PMV4 + 0.2179f * PMV2));
    
    return FMath::Clamp(PPD, 5.0f, 100.0f); // Minimum 5% always dissatisfied
}

FThermalProperties UThermalPhysicsSystem::GetMaterialThermalProperties(EThermalMaterial Material) const
{
    if (const FThermalProperties* Props = MaterialPropertiesTable.Find(Material))
    {
        return *Props;
    }
    
    // Return default properties if not found
    return MaterialPropertiesTable[EThermalMaterial::Rock];
}

void UThermalPhysicsSystem::SetCustomMaterialProperties(EThermalMaterial Material, const FThermalProperties& Properties)
{
    MaterialPropertiesTable.Add(Material, Properties);
}

void UThermalPhysicsSystem::SetThermalLOD(int32 LODLevel)
{
    CurrentLOD = FMath::Clamp(LODLevel, 0, 3);
    
    // Adjust update frequencies based on LOD
    switch (CurrentLOD)
    {
    case 0: // High detail
        ZoneUpdateInterval = 0.5f;      // 2Hz
        ObjectUpdateInterval = 0.2f;    // 5Hz
        HeatTransferInterval = 0.1f;    // 10Hz
        ExpansionUpdateInterval = 1.0f; // 1Hz
        bUseAdvancedHeatTransfer = true;
        bSimulateThermalExpansion = true;
        break;
    case 1: // Medium detail
        ZoneUpdateInterval = 1.0f;      // 1Hz
        ObjectUpdateInterval = 0.5f;    // 2Hz
        HeatTransferInterval = 0.2f;    // 5Hz
        ExpansionUpdateInterval = 2.0f; // 0.5Hz
        bUseAdvancedHeatTransfer = true;
        bSimulateThermalExpansion = true;
        break;
    case 2: // Low detail
        ZoneUpdateInterval = 2.0f;      // 0.5Hz
        ObjectUpdateInterval = 1.0f;    // 1Hz
        HeatTransferInterval = 0.5f;    // 2Hz
        ExpansionUpdateInterval = 5.0f; // 0.2Hz
        bUseAdvancedHeatTransfer = false;
        bSimulateThermalExpansion = false;
        break;
    case 3: // Minimal detail
        ZoneUpdateInterval = 5.0f;      // 0.2Hz
        ObjectUpdateInterval = 2.0f;    // 0.5Hz
        HeatTransferInterval = 1.0f;    // 1Hz
        ExpansionUpdateInterval = 10.0f;// 0.1Hz
        bUseAdvancedHeatTransfer = false;
        bSimulateThermalExpansion = false;
        break;
    }
}

void UThermalPhysicsSystem::OptimizeThermalSimulation(float ViewerDistance)
{
    if (ViewerDistance < 2000.0f) // 20m
    {
        SetThermalLOD(0);
    }
    else if (ViewerDistance < 5000.0f) // 50m
    {
        SetThermalLOD(1);
    }
    else if (ViewerDistance < 10000.0f) // 100m
    {
        SetThermalLOD(2);
    }
    else
    {
        SetThermalLOD(3);
    }
}

// Protected implementation functions
void UThermalPhysicsSystem::UpdateThermalZones(float DeltaTime)
{
    for (FThermalZone& Zone : ThermalZones)
    {
        // Apply zone-specific thermal effects
        float TemperatureChange = 0.0f;
        
        switch (Zone.ZoneType)
        {
        case EThermalZoneType::Geothermal:
            TemperatureChange += Zone.HeatGeneration * DeltaTime * 0.001f;
            break;
        case EThermalZoneType::Underground:
            // Underground zones tend toward ground temperature
            TemperatureChange = (GroundTemperature - Zone.CurrentTemperature) * DeltaTime * 0.01f;
            break;
        case EThermalZoneType::Water:
            // Water zones moderate temperature changes
            TemperatureChange = (AmbientTemperature - Zone.CurrentTemperature) * DeltaTime * 0.1f;
            break;
        case EThermalZoneType::Artificial:
            // Artificial heating/cooling maintains set temperature
            TemperatureChange = (Zone.BaseTemperature - Zone.CurrentTemperature) * DeltaTime * 0.5f;
            break;
        default:
            // Ambient zones slowly equilibrate with ambient temperature
            TemperatureChange = (AmbientTemperature - Zone.CurrentTemperature) * DeltaTime * 0.01f;
            break;
        }
        
        Zone.CurrentTemperature += TemperatureChange * GlobalThermalMultiplier;
        
        // Apply random temperature variation
        if (Zone.TemperatureVariation > 0.0f)
        {
            float Variation = FMath::RandRange(-Zone.TemperatureVariation, Zone.TemperatureVariation);
            Zone.CurrentTemperature += Variation * DeltaTime * 0.1f;
        }
    }
}

void UThermalPhysicsSystem::UpdateThermalObjects(float DeltaTime)
{
    for (FThermalObject& Object : ThermalObjects)
    {
        // Get ambient temperature at object location
        float AmbientTemp = AmbientTemperature;
        
        // Find thermal zone influence
        TArray<FString> ZoneIDs = GetThermalZonesAtLocation(Object.Location);
        if (ZoneIDs.Num() > 0)
        {
            AmbientTemp = 0.0f;
            for (const FString& ZoneID : ZoneIDs)
            {
                FThermalZone Zone = GetThermalZone(ZoneID);
                AmbientTemp += Zone.CurrentTemperature;
            }
            AmbientTemp /= ZoneIDs.Num();
        }
        
        // Calculate heat transfer to ambient
        float TempDifference = AmbientTemp - Object.CurrentTemperature;
        float HeatTransferRate = Object.ThermalProps.ThermalConductivity * TempDifference * DeltaTime;
        
        // Apply thermal time constant
        float ThermalTimeConstant = Object.Mass * Object.ThermalProps.SpecificHeat / 
                                   (Object.ThermalProps.ThermalConductivity * 6.0f); // Simplified
        
        float TemperatureChange = HeatTransferRate / (Object.Mass * Object.ThermalProps.SpecificHeat) / ThermalTimeConstant;
        Object.CurrentTemperature += TemperatureChange * GlobalThermalMultiplier;
    }
}

void UThermalPhysicsSystem::ProcessHeatTransfer(float DeltaTime)
{
    if (!bUseAdvancedHeatTransfer || CurrentLOD > 1)
    {
        return;
    }
    
    // Calculate heat transfer between thermal objects
    for (int32 i = 0; i < ThermalObjects.Num(); ++i)
    {
        for (int32 j = i + 1; j < ThermalObjects.Num(); ++j)
        {
            CalculateObjectToObjectHeatTransfer(ThermalObjects[i], ThermalObjects[j], DeltaTime);
        }
    }
    
    // Calculate heat transfer between zones and objects
    for (FThermalObject& Object : ThermalObjects)
    {
        TArray<FString> ZoneIDs = GetThermalZonesAtLocation(Object.Location);
        for (const FString& ZoneID : ZoneIDs)
        {
            FThermalZone* Zone = ThermalZones.FindByPredicate([&ZoneID](const FThermalZone& Z)
            {
                return Z.ZoneID == ZoneID;
            });
            
            if (Zone)
            {
                CalculateObjectToZoneHeatTransfer(Object, *Zone, DeltaTime);
            }
        }
    }
}

void UThermalPhysicsSystem::UpdateThermalExpansion(float DeltaTime)
{
    if (!bSimulateThermalExpansion)
    {
        return;
    }
    
    for (const FThermalObject& Object : ThermalObjects)
    {
        ApplyThermalExpansionToObject(Object.ObjectID, DeltaTime);
    }
}

void UThermalPhysicsSystem::CalculateObjectToObjectHeatTransfer(FThermalObject& ObjectA, FThermalObject& ObjectB, float DeltaTime)
{
    float Distance = FVector::Dist(ObjectA.Location, ObjectB.Location);
    
    // Only calculate heat transfer for nearby objects
    if (Distance > 1000.0f) // 10m maximum
    {
        return;
    }
    
    float TempDifference = ObjectA.CurrentTemperature - ObjectB.CurrentTemperature;
    
    if (FMath::Abs(TempDifference) < 0.1f)
    {
        return;
    }
    
    // Calculate effective thermal conductivity and area
    float AvgConductivity = (ObjectA.ThermalProps.ThermalConductivity + ObjectB.ThermalProps.ThermalConductivity) * 0.5f;
    float ContactArea = FMath::Min(ObjectA.Size.X * ObjectA.Size.Y, ObjectB.Size.X * ObjectB.Size.Y) * 0.0001f; // cm² to m²
    
    // Heat transfer rate
    float HeatTransferRate = AvgConductivity * ContactArea * TempDifference / (Distance * 0.01f) * DeltaTime;
    
    // Apply heat transfer to both objects
    float HeatToB = HeatTransferRate * GlobalThermalMultiplier;
    float TempChangeA = -HeatToB / (ObjectA.Mass * ObjectA.ThermalProps.SpecificHeat);
    float TempChangeB = HeatToB / (ObjectB.Mass * ObjectB.ThermalProps.SpecificHeat);
    
    ObjectA.CurrentTemperature += TempChangeA;
    ObjectB.CurrentTemperature += TempChangeB;
}

void UThermalPhysicsSystem::CalculateObjectToZoneHeatTransfer(FThermalObject& Object, FThermalZone& Zone, float DeltaTime)
{
    float TempDifference = Zone.CurrentTemperature - Object.CurrentTemperature;
    
    if (FMath::Abs(TempDifference) < 0.1f)
    {
        return;
    }
    
    // Calculate heat transfer coefficient based on zone type
    float HeatTransferCoeff = 10.0f; // W/(m²·K) default
    
    switch (Zone.ZoneType)
    {
    case EThermalZoneType::Water:
        HeatTransferCoeff = 100.0f; // High heat transfer in water
        break;
    case EThermalZoneType::Air:
        HeatTransferCoeff = 5.0f; // Low heat transfer in air
        break;
    case EThermalZoneType::Underground:
        HeatTransferCoeff = 20.0f; // Moderate heat transfer to ground
        break;
    }
    
    // Object surface area
    float SurfaceArea = (Object.Size.X * Object.Size.Y * 2.0f + Object.Size.X * Object.Size.Z * 2.0f + 
                        Object.Size.Y * Object.Size.Z * 2.0f) * 0.0001f; // cm² to m²
    
    float HeatTransferRate = HeatTransferCoeff * SurfaceArea * TempDifference * DeltaTime;
    
    // Apply heat transfer to object (zone temperature assumed stable)
    float TempChange = HeatTransferRate / (Object.Mass * Object.ThermalProps.SpecificHeat);
    Object.CurrentTemperature += TempChange * GlobalThermalMultiplier;
}

void UThermalPhysicsSystem::ProcessEnvironmentalHeating(float DeltaTime)
{
    // Apply environmental heating effects from weather and hazard systems
    if (HazardManager)
    {
        // Get current weather conditions
        float AmbientTemp = HazardManager->GetTemperatureAtLocation(GetOwner()->GetActorLocation());
        
        // Update global ambient temperature
        AmbientTemperature = FMath::FInterpTo(AmbientTemperature, AmbientTemp, DeltaTime, 0.1f);
    }
}

void UThermalPhysicsSystem::ApplyWeatherThermalEffects(float DeltaTime)
{
    // Apply weather-based thermal effects
    if (HazardManager)
    {
        // This would interface with the weather system to get:
        // - Solar radiation intensity
        // - Cloud cover
        // - Wind speed for convective cooling
        // - Precipitation for evaporative cooling
        
        // For now, apply simplified effects
        if (GetWorld())
        {
            float TimeOfDay = FMath::Fmod(GetWorld()->GetTimeSeconds() / 3600.0f, 24.0f); // Hours
            
            // Simple day/night temperature variation
            float SolarEffect = FMath::Sin((TimeOfDay - 6.0f) * PI / 12.0f); // Peak at noon
            SolarEffect = FMath::Max(0.0f, SolarEffect);
            
            float SolarHeating = SolarEffect * 800.0f; // W/m² maximum solar intensity
            UpdateSolarHeating(SolarHeating, FVector(0.5f, 0.5f, -0.7f), DeltaTime);
        }
    }
}

void UThermalPhysicsSystem::CalculateGroundTemperature()
{
    // Ground temperature is typically more stable than air temperature
    // Usually a few degrees warmer than average air temperature at shallow depths
    GroundTemperature = AmbientTemperature + 3.0f;
}

void UThermalPhysicsSystem::InitializeMaterialProperties()
{
    // This will be populated in LoadThermalPropertiesTable()
}

void UThermalPhysicsSystem::LoadThermalPropertiesTable()
{
    // Rock properties
    FThermalProperties RockProps;
    RockProps.ThermalConductivity = 2.8f;   // W/(m·K)
    RockProps.SpecificHeat = 840.0f;        // J/(kg·K)
    RockProps.Density = 2650.0f;            // kg/m³
    RockProps.LinearExpansionCoeff = 8e-6f; // 1/K
    RockProps.Emissivity = 0.9f;
    RockProps.SolarAbsorption = 0.7f;
    MaterialPropertiesTable.Add(EThermalMaterial::Rock, RockProps);
    
    // Metal properties (Steel)
    FThermalProperties MetalProps;
    MetalProps.ThermalConductivity = 50.0f;
    MetalProps.SpecificHeat = 460.0f;
    MetalProps.Density = 7850.0f;
    MetalProps.LinearExpansionCoeff = 12e-6f;
    MetalProps.Emissivity = 0.7f;
    MetalProps.SolarAbsorption = 0.5f;
    MaterialPropertiesTable.Add(EThermalMaterial::Metal, MetalProps);
    
    // Wood properties
    FThermalProperties WoodProps;
    WoodProps.ThermalConductivity = 0.12f;
    WoodProps.SpecificHeat = 1700.0f;
    WoodProps.Density = 600.0f;
    WoodProps.LinearExpansionCoeff = 5e-6f;
    WoodProps.Emissivity = 0.9f;
    WoodProps.SolarAbsorption = 0.8f;
    MaterialPropertiesTable.Add(EThermalMaterial::Wood, WoodProps);
    
    // Plastic properties
    FThermalProperties PlasticProps;
    PlasticProps.ThermalConductivity = 0.2f;
    PlasticProps.SpecificHeat = 1500.0f;
    PlasticProps.Density = 950.0f;
    PlasticProps.LinearExpansionCoeff = 80e-6f;
    PlasticProps.Emissivity = 0.95f;
    PlasticProps.SolarAbsorption = 0.6f;
    MaterialPropertiesTable.Add(EThermalMaterial::Plastic, PlasticProps);
    
    // Rubber properties
    FThermalProperties RubberProps;
    RubberProps.ThermalConductivity = 0.16f;
    RubberProps.SpecificHeat = 2000.0f;
    RubberProps.Density = 1200.0f;
    RubberProps.LinearExpansionCoeff = 200e-6f;
    RubberProps.Emissivity = 0.92f;
    RubberProps.SolarAbsorption = 0.9f;
    MaterialPropertiesTable.Add(EThermalMaterial::Rubber, RubberProps);
    
    // Fabric properties
    FThermalProperties FabricProps;
    FabricProps.ThermalConductivity = 0.04f;
    FabricProps.SpecificHeat = 1340.0f;
    FabricProps.Density = 300.0f;
    FabricProps.LinearExpansionCoeff = 10e-6f;
    FabricProps.Emissivity = 0.95f;
    FabricProps.SolarAbsorption = 0.7f;
    MaterialPropertiesTable.Add(EThermalMaterial::Fabric, FabricProps);
    
    // Ice properties
    FThermalProperties IceProps;
    IceProps.ThermalConductivity = 2.2f;
    IceProps.SpecificHeat = 2100.0f;
    IceProps.Density = 917.0f;
    IceProps.LinearExpansionCoeff = 51e-6f;
    IceProps.Emissivity = 0.97f;
    IceProps.SolarAbsorption = 0.3f;
    MaterialPropertiesTable.Add(EThermalMaterial::Ice, IceProps);
    
    // Water properties
    FThermalProperties WaterProps;
    WaterProps.ThermalConductivity = 0.6f;
    WaterProps.SpecificHeat = 4180.0f;
    WaterProps.Density = 1000.0f;
    WaterProps.LinearExpansionCoeff = 214e-6f;
    WaterProps.Emissivity = 0.96f;
    WaterProps.SolarAbsorption = 0.9f;
    MaterialPropertiesTable.Add(EThermalMaterial::Water, WaterProps);
    
    // Air properties
    FThermalProperties AirProps;
    AirProps.ThermalConductivity = 0.026f;
    AirProps.SpecificHeat = 1005.0f;
    AirProps.Density = 1.225f;
    AirProps.LinearExpansionCoeff = 3400e-6f;
    AirProps.Emissivity = 0.0f; // Air doesn't emit significantly
    AirProps.SolarAbsorption = 0.0f;
    MaterialPropertiesTable.Add(EThermalMaterial::Air, AirProps);
    
    // Calculate thermal diffusivities
    for (auto& MaterialPair : MaterialPropertiesTable)
    {
        FThermalProperties& Props = MaterialPair.Value;
        Props.ThermalDiffusivity = CalculateThermalDiffusivity(Props.ThermalConductivity, Props.Density, Props.SpecificHeat);
    }
}

// Static utility functions
float UThermalPhysicsSystem::ConvertCelsiusToKelvin(float Celsius)
{
    return Celsius + 273.15f;
}

float UThermalPhysicsSystem::ConvertKelvinToCelsius(float Kelvin)
{
    return Kelvin - 273.15f;
}

float UThermalPhysicsSystem::ConvertCelsiusToFahrenheit(float Celsius)
{
    return (Celsius * 9.0f / 5.0f) + 32.0f;
}

float UThermalPhysicsSystem::ConvertFahrenheitToCelsius(float Fahrenheit)
{
    return (Fahrenheit - 32.0f) * 5.0f / 9.0f;
}

float UThermalPhysicsSystem::CalculateThermalDiffusivity(float Conductivity, float Density, float SpecificHeat)
{
    return Conductivity / (Density * SpecificHeat);
}

FString UThermalPhysicsSystem::GetTemperatureDescription(float Temperature)
{
    if (Temperature < -20.0f)
        return TEXT("Extremely Cold");
    else if (Temperature < -10.0f)
        return TEXT("Very Cold");
    else if (Temperature < 0.0f)
        return TEXT("Freezing");
    else if (Temperature < 10.0f)
        return TEXT("Cold");
    else if (Temperature < 20.0f)
        return TEXT("Cool");
    else if (Temperature < 25.0f)
        return TEXT("Comfortable");
    else if (Temperature < 30.0f)
        return TEXT("Warm");
    else if (Temperature < 35.0f)
        return TEXT("Hot");
    else if (Temperature < 40.0f)
        return TEXT("Very Hot");
    else
        return TEXT("Extremely Hot");
}