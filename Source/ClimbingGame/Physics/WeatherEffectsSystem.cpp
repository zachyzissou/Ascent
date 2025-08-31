#include "WeatherEffectsSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "../Tools/ClimbingToolBase.h"

UWeatherEffectsSystem::UWeatherEffectsSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    SetIsReplicatedByDefault(true);

    // Initialize default precipitation data
    CurrentPrecipitation.Type = EPrecipitationType::None;
    CurrentPrecipitation.Intensity = 0.0f;
    CurrentPrecipitation.Rate = 0.0f;
    CurrentPrecipitation.DropletSize = 2.0f;
    CurrentPrecipitation.WindDrift = FVector::ZeroVector;
    CurrentPrecipitation.TemperatureThreshold = 2.0f;

    // Initialize default thermal data
    CurrentThermal.AmbientTemperature = 20.0f;
    CurrentThermal.SurfaceTemperature = 20.0f;
    CurrentThermal.ThermalConductivity = 1.0f;
    CurrentThermal.SpecificHeat = 1.0f;
    CurrentThermal.SolarHeating = 0.0f;
    CurrentThermal.RadiativeCooling = 0.0f;
    CurrentThermal.ConvectiveCooling = 0.0f;
}

void UWeatherEffectsSystem::BeginPlay()
{
    Super::BeginPlay();

    // Load material properties
    LoadMaterialProperties();

    // Initialize thermal nodes if detailed thermal simulation is enabled
    if (bSimulateDetailedThermals)
    {
        // This would typically load thermal nodes from level data
        // For now, we'll create a simple grid
    }

    // Try to find environmental hazard manager
    if (!HazardManager)
    {
        HazardManager = GetOwner()->FindComponentByClass<UEnvironmentalHazardManager>();
    }
}

void UWeatherEffectsSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return; // Only simulate on server
    }

    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    // Update weather systems based on LOD and timing
    if (CurrentTime - LastSurfaceUpdate > SurfaceUpdateInterval)
    {
        UpdateSurfaceWetness(DeltaTime);
        ProcessEvaporation(DeltaTime);
        LastSurfaceUpdate = CurrentTime;
    }

    if (CurrentTime - LastThermalUpdate > ThermalUpdateInterval)
    {
        UpdateSurfaceTemperatures(DeltaTime);
        if (bEnableThermalExpansion)
        {
            ProcessThermalExpansion(DeltaTime);
        }
        LastThermalUpdate = CurrentTime;
    }

    if (bEnableIcePhysics && CurrentTime - LastIceUpdate > IceUpdateInterval)
    {
        ProcessFreezingEffects(DeltaTime);
        LastIceUpdate = CurrentTime;
    }

    // Always update precipitation effects
    if (CurrentPrecipitation.Type != EPrecipitationType::None)
    {
        UpdatePrecipitationEffects(DeltaTime);
        SimulatePrecipitationPhysics(DeltaTime);
    }

    // Clear old cache entries
    if (bUseWeatherCache && CurrentTime - LastCacheUpdate > CacheTimeout)
    {
        TemperatureCache.Empty();
        WetnessCache.Empty();
        LastCacheUpdate = CurrentTime;
    }

    // Update LOD if needed
    if (CurrentTime - LastLODUpdate > LODUpdateInterval)
    {
        float ClosestPlayerDistance = MaxWeatherSimulationDistance;
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
        
        OptimizeWeatherSimulation(ClosestPlayerDistance);
        LastLODUpdate = CurrentTime;
    }
}

void UWeatherEffectsSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UWeatherEffectsSystem, CurrentPrecipitation);
    DOREPLIFETIME(UWeatherEffectsSystem, CurrentThermal);
}

void UWeatherEffectsSystem::SetPrecipitation(EPrecipitationType Type, float Intensity, float Rate)
{
    EPrecipitationType OldType = CurrentPrecipitation.Type;
    
    CurrentPrecipitation.Type = Type;
    CurrentPrecipitation.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    CurrentPrecipitation.Rate = FMath::Max(0.0f, Rate);

    // Set droplet size based on precipitation type
    switch (Type)
    {
    case EPrecipitationType::Drizzle:
        CurrentPrecipitation.DropletSize = 0.5f;
        break;
    case EPrecipitationType::Rain:
        CurrentPrecipitation.DropletSize = 2.0f;
        break;
    case EPrecipitationType::HeavyRain:
        CurrentPrecipitation.DropletSize = 4.0f;
        break;
    case EPrecipitationType::Snow:
    case EPrecipitationType::HeavySnow:
        CurrentPrecipitation.DropletSize = 8.0f; // Snowflake size
        break;
    case EPrecipitationType::Sleet:
        CurrentPrecipitation.DropletSize = 3.0f;
        break;
    case EPrecipitationType::Hail:
        CurrentPrecipitation.DropletSize = 10.0f;
        break;
    default:
        CurrentPrecipitation.DropletSize = 2.0f;
        break;
    }

    // Broadcast events
    if (OldType == EPrecipitationType::None && Type != EPrecipitationType::None)
    {
        OnPrecipitationStart.Broadcast();
    }
    else if (OldType != EPrecipitationType::None && Type == EPrecipitationType::None)
    {
        OnPrecipitationEnd.Broadcast();
    }
}

void UWeatherEffectsSystem::UpdatePrecipitationEffects(float DeltaTime)
{
    if (CurrentPrecipitation.Type == EPrecipitationType::None)
    {
        return;
    }

    // Apply precipitation to all registered surfaces
    int32 SurfacesProcessed = 0;
    for (auto& SurfacePair : SurfaceWeatherMap)
    {
        if (SurfacesProcessed >= MaxSurfaceUpdatesPerFrame)
        {
            break; // Limit processing per frame for performance
        }

        ApplyPrecipitationToSurface(SurfacePair.Key, DeltaTime);
        SurfacesProcessed++;
    }

    // Update wind drift based on current wind conditions
    if (HazardManager)
    {
        FVector AverageWindVelocity = FVector::ZeroVector;
        // Sample wind at a few different altitudes for precipitation drift
        for (float Altitude = 0.0f; Altitude <= 2000.0f; Altitude += 500.0f)
        {
            FVector OwnerLocation = GetOwner()->GetActorLocation();
            AverageWindVelocity += HazardManager->GetWindVelocityAtLocation(OwnerLocation + FVector(0, 0, Altitude));
        }
        AverageWindVelocity /= 5.0f; // Average of 5 samples

        CurrentPrecipitation.WindDrift = AverageWindVelocity * 0.1f; // Scale for precipitation drift
    }
}

float UWeatherEffectsSystem::GetWetnessAtLocation(const FVector& Location) const
{
    // Use cache if available
    if (bUseWeatherCache)
    {
        if (const float* CachedWetness = WetnessCache.Find(Location))
        {
            return *CachedWetness;
        }
    }

    float BaseWetness = 0.0f;

    // Calculate wetness from current precipitation
    if (CurrentPrecipitation.Type != EPrecipitationType::None)
    {
        float PrecipitationWetness = CurrentPrecipitation.Intensity;
        
        // Different precipitation types contribute differently to wetness
        switch (CurrentPrecipitation.Type)
        {
        case EPrecipitationType::Drizzle:
            PrecipitationWetness *= 0.3f;
            break;
        case EPrecipitationType::Rain:
            PrecipitationWetness *= 0.8f;
            break;
        case EPrecipitationType::HeavyRain:
            PrecipitationWetness *= 1.0f;
            break;
        case EPrecipitationType::Snow:
            PrecipitationWetness *= 0.2f; // Snow creates less immediate wetness
            break;
        case EPrecipitationType::HeavySnow:
            PrecipitationWetness *= 0.4f;
            break;
        case EPrecipitationType::Sleet:
            PrecipitationWetness *= 0.6f;
            break;
        case EPrecipitationType::Hail:
            PrecipitationWetness *= 0.5f;
            break;
        }

        BaseWetness += PrecipitationWetness;
    }

    // Find nearest registered surface for surface-specific wetness
    float NearestDistance = FLT_MAX;
    FString NearestSurface;
    for (const auto& SurfaceLocationPair : SurfaceLocations)
    {
        float Distance = FVector::Dist(Location, SurfaceLocationPair.Value);
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestSurface = SurfaceLocationPair.Key;
        }
    }

    if (!NearestSurface.IsEmpty() && NearestDistance < 100.0f) // Within 1m
    {
        if (const FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(NearestSurface))
        {
            BaseWetness = FMath::Max(BaseWetness, SurfaceData->WetnessAccumulation);
        }
    }

    BaseWetness = FMath::Clamp(BaseWetness, 0.0f, 1.0f);

    // Cache the result
    if (bUseWeatherCache)
    {
        WetnessCache.Add(Location, BaseWetness);
    }

    return BaseWetness;
}

void UWeatherEffectsSystem::ApplyPrecipitationToSurface(const FString& SurfaceID, float DeltaTime)
{
    FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(SurfaceID);
    if (!SurfaceData)
    {
        return;
    }

    float PrecipitationContribution = 0.0f;

    switch (CurrentPrecipitation.Type)
    {
    case EPrecipitationType::Drizzle:
        PrecipitationContribution = CurrentPrecipitation.Rate * 0.001f * DeltaTime; // mm/h to wetness/s
        break;
    case EPrecipitationType::Rain:
        PrecipitationContribution = CurrentPrecipitation.Rate * 0.002f * DeltaTime;
        break;
    case EPrecipitationType::HeavyRain:
        PrecipitationContribution = CurrentPrecipitation.Rate * 0.003f * DeltaTime;
        break;
    case EPrecipitationType::Snow:
        // Snow accumulates depth rather than immediate wetness
        if (SurfaceData->Temperature <= 0.0f)
        {
            SurfaceData->SnowDepth += CurrentPrecipitation.Rate * 0.1f * DeltaTime; // Rough snow accumulation
        }
        else
        {
            // Snow melts on warm surfaces
            PrecipitationContribution = CurrentPrecipitation.Rate * 0.001f * DeltaTime;
        }
        break;
    case EPrecipitationType::HeavySnow:
        if (SurfaceData->Temperature <= 0.0f)
        {
            SurfaceData->SnowDepth += CurrentPrecipitation.Rate * 0.2f * DeltaTime;
        }
        else
        {
            PrecipitationContribution = CurrentPrecipitation.Rate * 0.002f * DeltaTime;
        }
        break;
    case EPrecipitationType::Sleet:
        PrecipitationContribution = CurrentPrecipitation.Rate * 0.0015f * DeltaTime;
        if (SurfaceData->Temperature <= 0.0f)
        {
            SurfaceData->SnowDepth += CurrentPrecipitation.Rate * 0.05f * DeltaTime;
        }
        break;
    case EPrecipitationType::Hail:
        PrecipitationContribution = CurrentPrecipitation.Rate * 0.001f * DeltaTime;
        break;
    }

    // Apply precipitation to surface wetness
    SurfaceData->WetnessAccumulation += PrecipitationContribution * GlobalWetnessMultiplier;
    SurfaceData->WetnessAccumulation = FMath::Clamp(SurfaceData->WetnessAccumulation, 0.0f, 1.0f);

    // Update wetness level based on accumulation
    if (SurfaceData->WetnessAccumulation <= 0.1f)
    {
        SurfaceData->WetnessLevel = ESurfaceWetness::Dry;
    }
    else if (SurfaceData->WetnessAccumulation <= 0.3f)
    {
        SurfaceData->WetnessLevel = ESurfaceWetness::Damp;
    }
    else if (SurfaceData->WetnessAccumulation <= 0.6f)
    {
        SurfaceData->WetnessLevel = ESurfaceWetness::Wet;
    }
    else if (SurfaceData->WetnessAccumulation <= 0.9f)
    {
        SurfaceData->WetnessLevel = ESurfaceWetness::Soaked;
    }
    else
    {
        SurfaceData->WetnessLevel = ESurfaceWetness::Flooded;
    }

    // Update friction and grip multipliers
    SurfaceData->FrictionMultiplier = CalculateFrictionReduction(*SurfaceData);
    SurfaceData->GripStrengthMultiplier = CalculateGripReduction(*SurfaceData);
}

FSurfaceWeatherData UWeatherEffectsSystem::GetSurfaceWeatherData(const FString& SurfaceID) const
{
    if (const FSurfaceWeatherData* Data = SurfaceWeatherMap.Find(SurfaceID))
    {
        return *Data;
    }

    // Return default surface data
    FSurfaceWeatherData DefaultData;
    DefaultData.Temperature = CurrentThermal.AmbientTemperature;
    return DefaultData;
}

void UWeatherEffectsSystem::UpdateSurfaceWeather(const FString& SurfaceID, float DeltaTime)
{
    FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(SurfaceID);
    if (!SurfaceData)
    {
        return;
    }

    // Process drainage
    CalculateRunoff(SurfaceID, DeltaTime);

    // Update ice formation
    if (bEnableIcePhysics)
    {
        UpdateIceFormation(SurfaceID, DeltaTime);
    }

    // Update thermal effects
    if (const FVector* SurfaceLocation = SurfaceLocations.Find(SurfaceID))
    {
        CalculateHeatExchange(SurfaceID, DeltaTime);
    }
}

float UWeatherEffectsSystem::CalculateFrictionReduction(const FSurfaceWeatherData& SurfaceData) const
{
    float FrictionMultiplier = 1.0f;

    // Wetness effects on friction
    switch (SurfaceData.WetnessLevel)
    {
    case ESurfaceWetness::Dry:
        FrictionMultiplier = 1.0f;
        break;
    case ESurfaceWetness::Damp:
        FrictionMultiplier = 0.9f;
        break;
    case ESurfaceWetness::Wet:
        FrictionMultiplier = 0.7f;
        break;
    case ESurfaceWetness::Soaked:
        FrictionMultiplier = 0.5f;
        break;
    case ESurfaceWetness::Flooded:
        FrictionMultiplier = 0.3f;
        break;
    }

    // Ice effects on friction (overrides wetness if present)
    switch (SurfaceData.IceLevel)
    {
    case EIceFormation::None:
        break; // Use wetness-based friction
    case EIceFormation::Frost:
        FrictionMultiplier = FMath::Min(FrictionMultiplier, 0.8f);
        break;
    case EIceFormation::ThinIce:
        FrictionMultiplier = 0.3f;
        break;
    case EIceFormation::ThickIce:
        FrictionMultiplier = 0.2f;
        break;
    case EIceFormation::BlackIce:
        FrictionMultiplier = 0.1f;
        break;
    case EIceFormation::Verglas:
        FrictionMultiplier = 0.15f;
        break;
    }

    return FrictionMultiplier;
}

float UWeatherEffectsSystem::CalculateGripReduction(const FSurfaceWeatherData& SurfaceData) const
{
    float GripMultiplier = 1.0f;

    // Wetness effects on grip
    switch (SurfaceData.WetnessLevel)
    {
    case ESurfaceWetness::Dry:
        GripMultiplier = 1.0f;
        break;
    case ESurfaceWetness::Damp:
        GripMultiplier = 0.95f;
        break;
    case ESurfaceWetness::Wet:
        GripMultiplier = 0.8f;
        break;
    case ESurfaceWetness::Soaked:
        GripMultiplier = 0.6f;
        break;
    case ESurfaceWetness::Flooded:
        GripMultiplier = 0.4f;
        break;
    }

    // Ice effects on grip
    switch (SurfaceData.IceLevel)
    {
    case EIceFormation::None:
        break;
    case EIceFormation::Frost:
        GripMultiplier = FMath::Min(GripMultiplier, 0.9f);
        break;
    case EIceFormation::ThinIce:
        GripMultiplier = 0.5f;
        break;
    case EIceFormation::ThickIce:
        GripMultiplier = 0.3f;
        break;
    case EIceFormation::BlackIce:
        GripMultiplier = 0.2f;
        break;
    case EIceFormation::Verglas:
        GripMultiplier = 0.25f;
        break;
    }

    // Temperature effects
    if (SurfaceData.Temperature < -10.0f)
    {
        // Very cold surfaces can cause skin to stick momentarily, then slip
        GripMultiplier *= 0.8f;
    }
    else if (SurfaceData.Temperature > 50.0f)
    {
        // Very hot surfaces are uncomfortable and reduce grip effectiveness
        GripMultiplier *= 0.9f;
    }

    return GripMultiplier;
}

void UWeatherEffectsSystem::UpdateThermalConditions(float AmbientTemp, float SolarHeating, float WindSpeed)
{
    CurrentThermal.AmbientTemperature = AmbientTemp;
    CurrentThermal.SolarHeating = SolarHeating;

    // Calculate convective cooling based on wind speed
    CurrentThermal.ConvectiveCooling = WindSpeed * 10.0f; // Simplified convective cooling

    // Update radiative cooling (simplified Stefan-Boltzmann)
    float TempKelvin = AmbientTemp + 273.15f;
    CurrentThermal.RadiativeCooling = 5.67e-8f * FMath::Pow(TempKelvin, 4.0f) * 0.1f; // Simplified
}

float UWeatherEffectsSystem::GetSurfaceTemperature(const FVector& Location) const
{
    // Use cache if available
    if (bUseWeatherCache)
    {
        if (const float* CachedTemp = TemperatureCache.Find(Location))
        {
            return *CachedTemp;
        }
    }

    float SurfaceTemp = CurrentThermal.AmbientTemperature;

    // Find nearest registered surface
    float NearestDistance = FLT_MAX;
    FString NearestSurface;
    for (const auto& SurfaceLocationPair : SurfaceLocations)
    {
        float Distance = FVector::Dist(Location, SurfaceLocationPair.Value);
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestSurface = SurfaceLocationPair.Key;
        }
    }

    if (!NearestSurface.IsEmpty() && NearestDistance < 100.0f)
    {
        if (const FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(NearestSurface))
        {
            SurfaceTemp = SurfaceData->Temperature;
        }
    }

    // Cache the result
    if (bUseWeatherCache)
    {
        TemperatureCache.Add(Location, SurfaceTemp);
    }

    return SurfaceTemp;
}

void UWeatherEffectsSystem::ProcessThermalExpansion(float DeltaTime)
{
    // Process thermal expansion for all registered surfaces
    for (auto& SurfaceDataPair : SurfaceWeatherMap)
    {
        const FString& SurfaceID = SurfaceDataPair.Key;
        FSurfaceWeatherData& SurfaceData = SurfaceDataPair.Value;

        float TemperatureChange = SurfaceData.Temperature - 20.0f; // Reference temperature
        
        if (FMath::Abs(TemperatureChange) > 1.0f) // Only process significant temperature changes
        {
            const FString* MaterialType = SurfaceMaterialTypes.Find(SurfaceID);
            if (MaterialType)
            {
                float ThermalStress = CalculateThermalStress(*MaterialType, TemperatureChange);
                
                // This would interface with the material system to apply stress
                // For climbing surfaces, this could affect hold placement or grip reliability
            }
        }
    }
}

float UWeatherEffectsSystem::CalculateThermalStress(const FString& MaterialType, float TempChange) const
{
    // Linear thermal expansion coefficient (per °C)
    float ExpansionCoefficient = 0.000012f; // Default for steel

    if (MaterialType == TEXT("Rock"))
    {
        ExpansionCoefficient = 0.000008f; // Granite
    }
    else if (MaterialType == TEXT("Aluminum"))
    {
        ExpansionCoefficient = 0.000023f;
    }
    else if (MaterialType == TEXT("Steel"))
    {
        ExpansionCoefficient = 0.000012f;
    }
    else if (MaterialType == TEXT("Concrete"))
    {
        ExpansionCoefficient = 0.000010f;
    }

    // Calculate stress (simplified - would need Young's modulus for proper calculation)
    return ExpansionCoefficient * TempChange * 200000.0f; // Approximate Young's modulus for steel (MPa)
}

void UWeatherEffectsSystem::UpdateIceFormation(const FString& SurfaceID, float DeltaTime)
{
    FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(SurfaceID);
    if (!SurfaceData)
    {
        return;
    }

    if (SurfaceData->Temperature > IceFormationThreshold)
    {
        // Process ice melting
        ProcessIceMelting(SurfaceID, SurfaceData->Temperature, DeltaTime);
    }
    else
    {
        // Process ice formation
        float WindSpeed = 0.0f;
        if (HazardManager)
        {
            const FVector* SurfaceLocation = SurfaceLocations.Find(SurfaceID);
            if (SurfaceLocation)
            {
                WindSpeed = HazardManager->GetWindVelocityAtLocation(*SurfaceLocation).Size();
            }
        }

        EIceFormation NewIceType = DetermineIceType(SurfaceData->Temperature, 
                                                   static_cast<float>(SurfaceData->WetnessLevel), 
                                                   WindSpeed);

        // Ice formation is gradual
        if (NewIceType != SurfaceData->IceLevel)
        {
            // Calculate ice formation rate
            float FormationRate = FMath::Abs(SurfaceData->Temperature) * 0.1f * DeltaTime;
            FormationRate *= (SurfaceData->WetnessAccumulation + 0.1f); // More wetness = faster ice formation

            if (FMath::RandRange(0.0f, 1.0f) < FormationRate)
            {
                // Advance ice formation by one level
                if (SurfaceData->IceLevel == EIceFormation::None && NewIceType != EIceFormation::None)
                {
                    SurfaceData->IceLevel = EIceFormation::Frost;
                    OnIceFormation.Broadcast();
                }
                else if (static_cast<int32>(SurfaceData->IceLevel) < static_cast<int32>(NewIceType))
                {
                    SurfaceData->IceLevel = static_cast<EIceFormation>(static_cast<int32>(SurfaceData->IceLevel) + 1);
                }
            }
        }
    }
}

EIceFormation UWeatherEffectsSystem::DetermineIceType(float Temperature, float WetnessLevel, float WindSpeed) const
{
    if (Temperature > 0.0f)
    {
        return EIceFormation::None;
    }

    if (Temperature > -2.0f)
    {
        if (WetnessLevel < 0.2f)
        {
            return EIceFormation::Frost;
        }
        else
        {
            return EIceFormation::ThinIce;
        }
    }
    else if (Temperature > -10.0f)
    {
        if (WindSpeed > 10.0f && WetnessLevel > 0.3f)
        {
            return EIceFormation::Verglas; // Freezing rain in wind
        }
        else if (WetnessLevel > 0.5f)
        {
            return EIceFormation::ThickIce;
        }
        else
        {
            return EIceFormation::ThinIce;
        }
    }
    else // Very cold
    {
        if (WetnessLevel > 0.1f && WindSpeed < 5.0f)
        {
            return EIceFormation::BlackIce;
        }
        else
        {
            return EIceFormation::ThickIce;
        }
    }
}

float UWeatherEffectsSystem::CalculateIceFriction(EIceFormation IceType) const
{
    switch (IceType)
    {
    case EIceFormation::None:
        return 1.0f;
    case EIceFormation::Frost:
        return 0.8f;
    case EIceFormation::ThinIce:
        return 0.3f;
    case EIceFormation::ThickIce:
        return 0.2f;
    case EIceFormation::BlackIce:
        return 0.1f;
    case EIceFormation::Verglas:
        return 0.15f;
    default:
        return 1.0f;
    }
}

void UWeatherEffectsSystem::ProcessIceMelting(const FString& SurfaceID, float Temperature, float DeltaTime)
{
    FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(SurfaceID);
    if (!SurfaceData || SurfaceData->IceLevel == EIceFormation::None)
    {
        return;
    }

    // Calculate melting rate based on temperature above freezing
    float MeltingRate = (Temperature - IceFormationThreshold) * 0.2f * DeltaTime;

    if (MeltingRate > 0.0f && FMath::RandRange(0.0f, 1.0f) < MeltingRate)
    {
        // Reduce ice level by one step
        if (SurfaceData->IceLevel != EIceFormation::None)
        {
            SurfaceData->IceLevel = static_cast<EIceFormation>(static_cast<int32>(SurfaceData->IceLevel) - 1);

            // Melting ice adds to surface wetness
            SurfaceData->WetnessAccumulation += 0.1f;
            SurfaceData->WetnessAccumulation = FMath::Clamp(SurfaceData->WetnessAccumulation, 0.0f, 1.0f);
        }
    }

    // Melt snow depth
    if (Temperature > 0.0f && SurfaceData->SnowDepth > 0.0f)
    {
        float SnowMeltRate = Temperature * 0.5f * DeltaTime; // cm per second per degree
        SurfaceData->SnowDepth = FMath::Max(0.0f, SurfaceData->SnowDepth - SnowMeltRate);

        // Melting snow adds to wetness
        SurfaceData->WetnessAccumulation += SnowMeltRate * 0.01f;
        SurfaceData->WetnessAccumulation = FMath::Clamp(SurfaceData->WetnessAccumulation, 0.0f, 1.0f);
    }
}

void UWeatherEffectsSystem::ApplyWeatherToEquipment(AClimbingToolBase* Tool, float DeltaTime)
{
    if (!Tool)
    {
        return;
    }

    FVector ToolLocation = Tool->GetActorLocation();
    float SurfaceTemp = GetSurfaceTemperature(ToolLocation);
    float Wetness = GetWetnessAtLocation(ToolLocation);

    // Temperature effects on metal equipment
    if (FMath::Abs(SurfaceTemp - 20.0f) > 10.0f)
    {
        float TempChange = SurfaceTemp - 20.0f;
        float ThermalExpansion = CalculateMetalExpansion(TempChange, 100.0f); // Assume 1m tool length
        
        // This would interface with the tool's physics to modify dimensions
        // Tool->ModifySize(ThermalExpansion);
    }

    // Wetness effects on equipment durability and performance
    if (Wetness > 0.5f)
    {
        // Wet equipment is more prone to corrosion and less reliable
        float CorrosionRate = Wetness * DeltaTime * 0.01f;
        // Tool->ModifyDurability(-CorrosionRate);

        // Reduced grip on wet tools
        // Tool->ModifyGripQuality(1.0f - Wetness * 0.3f);
    }

    // Ice formation on equipment
    if (SurfaceTemp < 0.0f && Wetness > 0.2f)
    {
        // Equipment can become difficult to use when iced up
        float IcingEffect = FMath::Abs(SurfaceTemp) * Wetness * 0.1f;
        // Tool->ModifyUsabilityDue to ice formation
    }
}

void UWeatherEffectsSystem::ApplyWeatherToRope(UAdvancedRopeComponent* Rope, float DeltaTime)
{
    if (!Rope)
    {
        return;
    }

    // Get average conditions along the rope
    TArray<FVector> SegmentPositions = Rope->GetRopeSegmentPositions();
    float AverageTemp = 0.0f;
    float AverageWetness = 0.0f;
    
    for (const FVector& Position : SegmentPositions)
    {
        AverageTemp += GetSurfaceTemperature(Position);
        AverageWetness += GetWetnessAtLocation(Position);
    }
    
    if (SegmentPositions.Num() > 0)
    {
        AverageTemp /= SegmentPositions.Num();
        AverageWetness /= SegmentPositions.Num();
    }

    // Wet rope effects
    if (AverageWetness > 0.3f)
    {
        // Wet ropes are heavier and have different stretch characteristics
        float WetWeightIncrease = AverageWetness * 0.3f; // Up to 30% weight increase
        // Rope->ModifyWeight(Rope->Properties.Weight * (1.0f + WetWeightIncrease));

        // Wet ropes have reduced breaking strength
        float StrengthReduction = AverageWetness * 0.2f; // Up to 20% reduction
        // Rope->ModifyBreakingStrength(Rope->Properties.BreakingStrength * (1.0f - StrengthReduction));

        // Increased dynamic elongation when wet
        float ElongationIncrease = AverageWetness * 0.15f;
        // Rope->ModifyElongation(Rope->Properties.DynamicElongation * (1.0f + ElongationIncrease));
    }

    // Freezing effects on rope
    if (AverageTemp < 0.0f && AverageWetness > 0.2f)
    {
        // Frozen ropes become stiff and brittle
        float FreezeEffect = FMath::Abs(AverageTemp) * AverageWetness * 0.05f;
        
        // Reduced elongation (rope becomes stiffer)
        // Rope->ModifyElongation(Rope->Properties.DynamicElongation * (1.0f - FreezeEffect));
        
        // Increased breaking risk
        // Rope->ModifyBreakingStrength(Rope->Properties.BreakingStrength * (1.0f - FreezeEffect * 0.5f));

        // Visual effect - rope segments might appear frosted
        if (Rope->CableComponent)
        {
            Rope->CableComponent->CableWidth *= (1.0f + FreezeEffect * 0.1f);
        }
    }

    // UV degradation (would typically be calculated over longer time periods)
    if (CurrentThermal.SolarHeating > 100.0f) // Bright sunlight
    {
        float UVDegradation = CurrentThermal.SolarHeating * DeltaTime * 0.00001f;
        // Rope->ProcessUVDegradation(1.0f, UVDegradation);
    }
}

float UWeatherEffectsSystem::CalculateMetalExpansion(float TemperatureChange, float ObjectLength) const
{
    // Linear thermal expansion coefficient for steel (per °C)
    float ExpansionCoefficient = 0.000012f;
    
    // ΔL = α × L₀ × ΔT
    return ExpansionCoefficient * ObjectLength * TemperatureChange;
}

void UWeatherEffectsSystem::UpdateClimberComfort(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime)
{
    if (!ClimbingComponent)
    {
        return;
    }

    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    float ActivityLevel = 1.0f; // Would be based on current climbing intensity
    
    FClimberComfort Comfort = CalculateClimberComfort(ClimberLocation, ActivityLevel);
    ApplyTemperatureEffectsToClimber(ClimbingComponent, Comfort);
}

FClimberComfort UWeatherEffectsSystem::CalculateClimberComfort(const FVector& ClimberLocation, float ActivityLevel) const
{
    FClimberComfort Comfort;
    
    float AmbientTemp = GetSurfaceTemperature(ClimberLocation);
    float Wetness = GetWetnessAtLocation(ClimberLocation);
    
    // Calculate wind chill if wind is present
    float EffectiveTemp = AmbientTemp;
    if (HazardManager)
    {
        float WindSpeed = HazardManager->GetWindVelocityAtLocation(ClimberLocation).Size();
        if (WindSpeed > 2.0f && AmbientTemp < 10.0f)
        {
            // Simplified wind chill calculation
            EffectiveTemp = 13.12f + 0.6215f * AmbientTemp - 11.37f * FMath::Pow(WindSpeed, 0.16f) 
                           + 0.3965f * AmbientTemp * FMath::Pow(WindSpeed, 0.16f);
        }
    }
    
    // Calculate comfort level (optimal around 20°C)
    float TempDeviation = FMath::Abs(EffectiveTemp - 20.0f);
    Comfort.ComfortLevel = FMath::Max(0.0f, 1.0f - TempDeviation * 0.05f);
    
    // Cold stress
    if (EffectiveTemp < 10.0f)
    {
        Comfort.HypothermiaRisk = (10.0f - EffectiveTemp) * 0.1f;
        Comfort.ShiveringIntensity = FMath::Max(0.0f, (5.0f - EffectiveTemp) * 0.2f);
    }
    
    // Heat stress
    if (EffectiveTemp > 30.0f)
    {
        Comfort.HeatStressRisk = (EffectiveTemp - 30.0f) * 0.1f;
        Comfort.DehydrationLevel = (EffectiveTemp - 30.0f) * 0.05f * ActivityLevel;
    }
    
    // Wetness effects on comfort
    if (Wetness > 0.5f)
    {
        Comfort.ComfortLevel *= (1.0f - Wetness * 0.3f);
        if (EffectiveTemp < 15.0f)
        {
            Comfort.HypothermiaRisk += Wetness * 0.5f; // Being wet in cold is dangerous
        }
    }
    
    // Clamp all values
    Comfort.ComfortLevel = FMath::Clamp(Comfort.ComfortLevel, 0.0f, 1.0f);
    Comfort.HypothermiaRisk = FMath::Clamp(Comfort.HypothermiaRisk, 0.0f, 1.0f);
    Comfort.HeatStressRisk = FMath::Clamp(Comfort.HeatStressRisk, 0.0f, 1.0f);
    Comfort.DehydrationLevel = FMath::Clamp(Comfort.DehydrationLevel, 0.0f, 1.0f);
    Comfort.ShiveringIntensity = FMath::Clamp(Comfort.ShiveringIntensity, 0.0f, 1.0f);
    
    return Comfort;
}

void UWeatherEffectsSystem::ApplyTemperatureEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, const FClimberComfort& Comfort)
{
    if (!ClimbingComponent)
    {
        return;
    }

    // Cold effects
    if (Comfort.HypothermiaRisk > 0.1f)
    {
        // Increased stamina drain due to shivering and maintaining body heat
        float ColdStaminaDrain = Comfort.HypothermiaRisk * 5.0f;
        ClimbingComponent->ConsumeStamina(ColdStaminaDrain);
        
        // Reduced grip precision due to cold and shivering
        float GripPrecisionReduction = (Comfort.HypothermiaRisk + Comfort.ShiveringIntensity) * 0.5f;
        ClimbingComponent->Settings.GripRecoveryRate *= (1.0f - GripPrecisionReduction * 0.3f);
        
        // Slower movement due to stiffness
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - Comfort.HypothermiaRisk * 0.4f);
    }

    // Heat effects
    if (Comfort.HeatStressRisk > 0.1f)
    {
        // Increased stamina drain due to overheating
        float HeatStaminaDrain = Comfort.HeatStressRisk * 8.0f;
        ClimbingComponent->ConsumeStamina(HeatStaminaDrain);
        
        // Reduced grip strength due to sweaty hands
        float SweatGripReduction = Comfort.HeatStressRisk * 0.3f;
        ClimbingComponent->ConsumeGripStrength(SweatGripReduction * 10.0f);
    }

    // Dehydration effects
    if (Comfort.DehydrationLevel > 0.2f)
    {
        // Significantly increased stamina drain
        float DehydrationStaminaDrain = Comfort.DehydrationLevel * 10.0f;
        ClimbingComponent->ConsumeStamina(DehydrationStaminaDrain);
        
        // Reduced overall performance
        ClimbingComponent->Settings.MaxStamina *= (1.0f - Comfort.DehydrationLevel * 0.3f);
    }

    // Overall comfort effects on performance
    if (Comfort.ComfortLevel < 0.5f)
    {
        float DiscomfortPenalty = (0.5f - Comfort.ComfortLevel) * 2.0f; // 0-1 scale
        
        // Reduced climbing efficiency
        ClimbingComponent->Settings.BaseStaminaDrainRate *= (1.0f + DiscomfortPenalty * 0.5f);
        
        // Increased likelihood of mistakes (grip failures, etc.)
        // This would be implemented in the climbing system's error calculation
    }
}

float UWeatherEffectsSystem::CalculateVisibilityReduction(const FVector& Location) const
{
    float VisibilityReduction = 0.0f;

    // Precipitation-based visibility reduction
    VisibilityReduction += GetPrecipitationVisibilityReduction();

    // Temperature-based fog calculation
    if (FMath::Abs(CurrentThermal.AmbientTemperature - 15.0f) < 5.0f) // Fog likely around 10-20°C
    {
        float FogIntensity = 1.0f - FMath::Abs(CurrentThermal.AmbientTemperature - 15.0f) / 5.0f;
        VisibilityReduction += FogIntensity * 0.4f;
    }

    return FMath::Clamp(VisibilityReduction, 0.0f, 0.9f); // Never completely zero visibility
}

float UWeatherEffectsSystem::GetPrecipitationVisibilityReduction() const
{
    switch (CurrentPrecipitation.Type)
    {
    case EPrecipitationType::None:
        return 0.0f;
    case EPrecipitationType::Drizzle:
        return CurrentPrecipitation.Intensity * 0.1f;
    case EPrecipitationType::Rain:
        return CurrentPrecipitation.Intensity * 0.3f;
    case EPrecipitationType::HeavyRain:
        return CurrentPrecipitation.Intensity * 0.6f;
    case EPrecipitationType::Snow:
        return CurrentPrecipitation.Intensity * 0.4f;
    case EPrecipitationType::HeavySnow:
        return CurrentPrecipitation.Intensity * 0.7f;
    case EPrecipitationType::Sleet:
        return CurrentPrecipitation.Intensity * 0.5f;
    case EPrecipitationType::Hail:
        return CurrentPrecipitation.Intensity * 0.6f;
    default:
        return 0.0f;
    }
}

void UWeatherEffectsSystem::RegisterSurface(const FString& SurfaceID, const FString& MaterialType, const FVector& Location)
{
    // Initialize surface weather data
    FSurfaceWeatherData SurfaceData;
    SurfaceData.Temperature = CurrentThermal.AmbientTemperature;
    SurfaceData.WetnessLevel = ESurfaceWetness::Dry;
    SurfaceData.IceLevel = EIceFormation::None;
    SurfaceData.WetnessAccumulation = 0.0f;
    SurfaceData.SnowDepth = 0.0f;
    SurfaceData.FrictionMultiplier = 1.0f;
    SurfaceData.GripStrengthMultiplier = 1.0f;
    
    // Set material-specific properties
    float WeatherResistance = GetMaterialWeatherResistance(MaterialType);
    SurfaceData.DrainageRate = WeatherResistance * 0.1f;
    SurfaceData.AbsorptionRate = (1.0f - WeatherResistance) * 0.1f;
    
    SurfaceWeatherMap.Add(SurfaceID, SurfaceData);
    SurfaceMaterialTypes.Add(SurfaceID, MaterialType);
    SurfaceLocations.Add(SurfaceID, Location);
}

void UWeatherEffectsSystem::UnregisterSurface(const FString& SurfaceID)
{
    SurfaceWeatherMap.Remove(SurfaceID);
    SurfaceMaterialTypes.Remove(SurfaceID);
    SurfaceLocations.Remove(SurfaceID);
}

float UWeatherEffectsSystem::GetMaterialWeatherResistance(const FString& MaterialType) const
{
    if (const float* Resistance = MaterialWeatherResistance.Find(MaterialType))
    {
        return *Resistance;
    }
    
    return 0.5f; // Default weather resistance
}

void UWeatherEffectsSystem::LoadMaterialProperties()
{
    // Initialize thermal conductivity table (W/m·K)
    MaterialThermalConductivity.Add(TEXT("Rock"), 2.8f);
    MaterialThermalConductivity.Add(TEXT("Steel"), 50.0f);
    MaterialThermalConductivity.Add(TEXT("Aluminum"), 237.0f);
    MaterialThermalConductivity.Add(TEXT("Concrete"), 1.4f);
    MaterialThermalConductivity.Add(TEXT("Wood"), 0.12f);
    MaterialThermalConductivity.Add(TEXT("Ice"), 2.2f);

    // Initialize specific heat table (J/kg·K)
    MaterialSpecificHeat.Add(TEXT("Rock"), 840.0f);
    MaterialSpecificHeat.Add(TEXT("Steel"), 460.0f);
    MaterialSpecificHeat.Add(TEXT("Aluminum"), 900.0f);
    MaterialSpecificHeat.Add(TEXT("Concrete"), 880.0f);
    MaterialSpecificHeat.Add(TEXT("Wood"), 1700.0f);
    MaterialSpecificHeat.Add(TEXT("Ice"), 2100.0f);

    // Initialize porosity table (0-1)
    MaterialPorosity.Add(TEXT("Rock"), 0.05f);
    MaterialPorosity.Add(TEXT("Steel"), 0.0f);
    MaterialPorosity.Add(TEXT("Aluminum"), 0.0f);
    MaterialPorosity.Add(TEXT("Concrete"), 0.15f);
    MaterialPorosity.Add(TEXT("Wood"), 0.60f);
    MaterialPorosity.Add(TEXT("Ice"), 0.0f);

    // Initialize weather resistance table (0-1)
    MaterialWeatherResistance.Add(TEXT("Rock"), 0.8f);
    MaterialWeatherResistance.Add(TEXT("Steel"), 0.3f); // Rusts easily
    MaterialWeatherResistance.Add(TEXT("Aluminum"), 0.9f); // Corrosion resistant
    MaterialWeatherResistance.Add(TEXT("Concrete"), 0.6f);
    MaterialWeatherResistance.Add(TEXT("Wood"), 0.2f); // Absorbs water readily
    MaterialWeatherResistance.Add(TEXT("Ice"), 0.0f); // Melts
}

void UWeatherEffectsSystem::SimulatePrecipitationPhysics(float DeltaTime)
{
    // This would simulate individual precipitation particles
    // For performance, we use statistical methods instead of individual particle simulation
    
    if (CurrentLOD > 1)
    {
        return; // Skip detailed precipitation physics in lower LOD
    }

    // Update existing precipitation particles (if any)
    for (int32 i = ActivePrecipitation.Num() - 1; i >= 0; --i)
    {
        FPrecipitationParticle& Particle = ActivePrecipitation[i];
        
        // Apply gravity and wind drift
        FVector Acceleration = FVector(0.0f, 0.0f, -980.0f); // Gravity in cm/s²
        Acceleration += CurrentPrecipitation.WindDrift * 100.0f; // Convert to cm/s²
        
        Particle.Velocity += Acceleration * DeltaTime;
        Particle.Position += Particle.Velocity * DeltaTime;
        
        // Remove particles that hit the ground or go out of bounds
        if (Particle.Position.Z < 0.0f || 
            FVector::Dist(Particle.Position, GetOwner()->GetActorLocation()) > MaxWeatherSimulationDistance)
        {
            ActivePrecipitation.RemoveAt(i);
        }
    }
}

void UWeatherEffectsSystem::UpdateSurfaceWetness(float DeltaTime)
{
    int32 SurfacesProcessed = 0;
    
    for (auto& SurfacePair : SurfaceWeatherMap)
    {
        if (SurfacesProcessed >= MaxSurfaceUpdatesPerFrame)
        {
            break;
        }
        
        UpdateSurfaceWeather(SurfacePair.Key, DeltaTime);
        SurfacesProcessed++;
    }
}

void UWeatherEffectsSystem::ProcessEvaporation(float DeltaTime)
{
    float BaseEvaporation = EvaporationRate * DeltaTime / 3600.0f; // Convert hourly rate to per-second
    
    for (auto& SurfacePair : SurfaceWeatherMap)
    {
        FSurfaceWeatherData& SurfaceData = SurfacePair.Value;
        
        if (SurfaceData.WetnessAccumulation > 0.0f)
        {
            // Calculate evaporation rate based on temperature, wind, and humidity
            float TempFactor = FMath::Max(0.1f, SurfaceData.Temperature / 20.0f);
            float WindFactor = 1.0f; // Would get from wind system if available
            
            float EvaporationAmount = BaseEvaporation * TempFactor * WindFactor;
            SurfaceData.WetnessAccumulation = FMath::Max(0.0f, SurfaceData.WetnessAccumulation - EvaporationAmount);
        }
    }
}

void UWeatherEffectsSystem::CalculateRunoff(const FString& SurfaceID, float DeltaTime)
{
    FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(SurfaceID);
    if (!SurfaceData)
    {
        return;
    }

    // Calculate runoff based on surface slope and drainage characteristics
    if (SurfaceData->WetnessAccumulation > 0.5f) // Water starts running off when surface is wet enough
    {
        const FString* MaterialType = SurfaceMaterialTypes.Find(SurfaceID);
        float DrainageEfficiency = MaterialType ? GetMaterialPorosity(*MaterialType) : 0.1f;
        
        float RunoffAmount = (SurfaceData->WetnessAccumulation - 0.5f) * DrainageEfficiency * DeltaTime;
        SurfaceData->WetnessAccumulation = FMath::Max(0.0f, SurfaceData->WetnessAccumulation - RunoffAmount);
    }
}

void UWeatherEffectsSystem::UpdateSurfaceTemperatures(float DeltaTime)
{
    for (auto& SurfacePair : SurfaceWeatherMap)
    {
        CalculateHeatExchange(SurfacePair.Key, DeltaTime);
    }
}

void UWeatherEffectsSystem::CalculateHeatExchange(const FString& SurfaceID, float DeltaTime)
{
    FSurfaceWeatherData* SurfaceData = SurfaceWeatherMap.Find(SurfaceID);
    if (!SurfaceData)
    {
        return;
    }

    const FString* MaterialType = SurfaceMaterialTypes.Find(SurfaceID);
    if (!MaterialType)
    {
        return;
    }

    float ThermalConductivity = GetMaterialThermalConductivity(*MaterialType);
    float SpecificHeat = GetMaterialSpecificHeat(*MaterialType);
    
    // Calculate heat transfer to ambient temperature
    float TemperatureDifference = CurrentThermal.AmbientTemperature - SurfaceData->Temperature;
    float HeatTransferRate = ThermalConductivity * TemperatureDifference * DeltaTime * GlobalThermalMultiplier;
    
    // Apply heat transfer (simplified thermal mass calculation)
    float ThermalMass = SpecificHeat * 1000.0f; // Assume 1000kg mass for simplification
    float TemperatureChange = HeatTransferRate / ThermalMass;
    
    SurfaceData->Temperature += TemperatureChange;
    
    // Add solar heating and radiative cooling
    float NetRadiation = CurrentThermal.SolarHeating - CurrentThermal.RadiativeCooling - CurrentThermal.ConvectiveCooling;
    float RadiationTemperatureChange = NetRadiation * DeltaTime / ThermalMass;
    
    SurfaceData->Temperature += RadiationTemperatureChange;
}

float UWeatherEffectsSystem::GetMaterialThermalConductivity(const FString& MaterialType) const
{
    if (const float* Conductivity = MaterialThermalConductivity.Find(MaterialType))
    {
        return *Conductivity;
    }
    return 1.0f; // Default thermal conductivity
}

float UWeatherEffectsSystem::GetMaterialSpecificHeat(const FString& MaterialType) const
{
    if (const float* SpecificHeat = MaterialSpecificHeat.Find(MaterialType))
    {
        return *SpecificHeat;
    }
    return 1000.0f; // Default specific heat
}

float UWeatherEffectsSystem::GetMaterialPorosity(const FString& MaterialType) const
{
    if (const float* Porosity = MaterialPorosity.Find(MaterialType))
    {
        return *Porosity;
    }
    return 0.1f; // Default porosity
}

void UWeatherEffectsSystem::ProcessFreezingEffects(float DeltaTime)
{
    for (auto& SurfacePair : SurfaceWeatherMap)
    {
        if (SurfacePair.Value.Temperature <= IceFormationThreshold)
        {
            UpdateIceFormation(SurfacePair.Key, DeltaTime);
        }
    }
}

void UWeatherEffectsSystem::SetWeatherLOD(int32 LODLevel)
{
    CurrentLOD = FMath::Clamp(LODLevel, 0, 3);
    
    // Adjust update frequencies based on LOD
    switch (CurrentLOD)
    {
    case 0: // High detail
        SurfaceUpdateInterval = 0.2f; // 5Hz
        ThermalUpdateInterval = 0.5f; // 2Hz
        IceUpdateInterval = 1.0f;     // 1Hz
        MaxSurfaceUpdatesPerFrame = 20;
        break;
    case 1: // Medium detail
        SurfaceUpdateInterval = 0.5f; // 2Hz
        ThermalUpdateInterval = 1.0f; // 1Hz
        IceUpdateInterval = 2.0f;     // 0.5Hz
        MaxSurfaceUpdatesPerFrame = 10;
        break;
    case 2: // Low detail
        SurfaceUpdateInterval = 1.0f; // 1Hz
        ThermalUpdateInterval = 2.0f; // 0.5Hz
        IceUpdateInterval = 5.0f;     // 0.2Hz
        MaxSurfaceUpdatesPerFrame = 5;
        break;
    case 3: // Minimal detail
        SurfaceUpdateInterval = 2.0f; // 0.5Hz
        ThermalUpdateInterval = 5.0f; // 0.2Hz
        IceUpdateInterval = 10.0f;    // 0.1Hz
        MaxSurfaceUpdatesPerFrame = 2;
        break;
    }
}

void UWeatherEffectsSystem::OptimizeWeatherSimulation(float ViewerDistance)
{
    if (ViewerDistance < 1000.0f) // 10m
    {
        SetWeatherLOD(0);
    }
    else if (ViewerDistance < 2500.0f) // 25m
    {
        SetWeatherLOD(1);
    }
    else if (ViewerDistance < 5000.0f) // 50m
    {
        SetWeatherLOD(2);
    }
    else
    {
        SetWeatherLOD(3);
    }
}

// Static utility functions
float UWeatherEffectsSystem::ConvertFahrenheitToCelsius(float Fahrenheit)
{
    return (Fahrenheit - 32.0f) * 5.0f / 9.0f;
}

float UWeatherEffectsSystem::ConvertCelsiusToFahrenheit(float Celsius)
{
    return (Celsius * 9.0f / 5.0f) + 32.0f;
}

float UWeatherEffectsSystem::CalculateDewPoint(float Temperature, float RelativeHumidity)
{
    float A = 17.27f;
    float B = 237.7f;
    float Alpha = ((A * Temperature) / (B + Temperature)) + FMath::Loge(RelativeHumidity / 100.0f);
    return (B * Alpha) / (A - Alpha);
}

float UWeatherEffectsSystem::CalculateHeatIndex(float Temperature, float RelativeHumidity)
{
    if (Temperature < 27.0f)
    {
        return Temperature; // Heat index not applicable below 80°F (27°C)
    }
    
    float T = Temperature;
    float RH = RelativeHumidity;
    
    // Simplified heat index calculation
    float HI = -8.78469475556f + 1.61139411f * T + 2.33854883889f * RH
               - 0.14611605f * T * RH - 0.012308094f * T * T
               - 0.0164248277778f * RH * RH + 0.002211732f * T * T * RH
               + 0.00072546f * T * RH * RH - 0.000003582f * T * T * RH * RH;
    
    return HI;
}

FString UWeatherEffectsSystem::GetTemperatureDescription(float Temperature)
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

float UWeatherEffectsSystem::CalculateWaterVaporPressure(float Temperature, float RelativeHumidity)
{
    // Saturation vapor pressure using Magnus formula
    float SaturationPressure = 6.112f * FMath::Exp((17.67f * Temperature) / (Temperature + 243.5f));
    
    // Actual vapor pressure
    return SaturationPressure * (RelativeHumidity / 100.0f);
}