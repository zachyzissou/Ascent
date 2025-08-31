#include "VisibilitySystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/PostProcessComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"

UVisibilitySystem::UVisibilitySystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    SetIsReplicatedByDefault(true);

    // Initialize global visibility condition
    GlobalCondition.HazardType = EVisibilityHazardType::None;
    GlobalCondition.Intensity = 0.0f;
    GlobalCondition.VisualRange = StandardVisibility;
    GlobalCondition.Density = 0.0f;
    GlobalCondition.Tint = FColor::White;
    GlobalCondition.ParticleSize = 1.0f;
    GlobalCondition.Pattern = EVisibilityPattern::Uniform;
    GlobalCondition.MovementDirection = FVector::ZeroVector;
    GlobalCondition.MovementSpeed = 0.0f;

    // Create post-process component for visibility effects
    VisibilityPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("VisibilityPostProcess"));
    VisibilityPostProcess->bEnabled = true;
    VisibilityPostProcess->BlendWeight = 0.0f;

    // Create dynamic fog component
    DynamicFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("DynamicFog"));
    DynamicFog->bVisible = false;

    // Initialize extinction coefficients lookup table
    ExtinctionCoefficients.Add(EVisibilityHazardType::Fog, 0.05f);     // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Mist, 0.02f);    // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Dust, 0.1f);     // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Sand, 0.5f);     // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Smoke, 1.0f);    // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Steam, 0.3f);    // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Rain, 0.15f);    // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Snow, 0.4f);     // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Hail, 0.6f);     // 1/km
    ExtinctionCoefficients.Add(EVisibilityHazardType::Blizzard, 2.0f); // 1/km

    // Initialize scattering colors
    ScatteringColors.Add(EVisibilityHazardType::Fog, FLinearColor(0.9f, 0.95f, 1.0f, 1.0f));      // Blue-white
    ScatteringColors.Add(EVisibilityHazardType::Mist, FLinearColor(0.95f, 0.98f, 1.0f, 1.0f));    // Light blue-white
    ScatteringColors.Add(EVisibilityHazardType::Dust, FLinearColor(0.8f, 0.7f, 0.5f, 1.0f));      // Brown-yellow
    ScatteringColors.Add(EVisibilityHazardType::Sand, FLinearColor(0.9f, 0.8f, 0.6f, 1.0f));      // Yellow-brown
    ScatteringColors.Add(EVisibilityHazardType::Smoke, FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));     // Dark gray
    ScatteringColors.Add(EVisibilityHazardType::Steam, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));     // White
    ScatteringColors.Add(EVisibilityHazardType::Rain, FLinearColor(0.8f, 0.85f, 0.9f, 1.0f));     // Gray-blue
    ScatteringColors.Add(EVisibilityHazardType::Snow, FLinearColor(0.95f, 0.95f, 1.0f, 1.0f));    // Blue-white
    ScatteringColors.Add(EVisibilityHazardType::Hail, FLinearColor(0.9f, 0.9f, 0.95f, 1.0f));     // Gray-white
    ScatteringColors.Add(EVisibilityHazardType::Blizzard, FLinearColor(0.8f, 0.85f, 0.9f, 1.0f)); // Gray-blue
}

void UVisibilitySystem::BeginPlay()
{
    Super::BeginPlay();

    // Try to find environmental hazard manager
    if (!HazardManager)
    {
        HazardManager = GetOwner()->FindComponentByClass<UEnvironmentalHazardManager>();
    }

    // Find or create main post-process volume
    MainPostProcessVolume = nullptr;
    TArray<AActor*> PostProcessVolumes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), PostProcessVolumes);
    
    for (AActor* Actor : PostProcessVolumes)
    {
        APostProcessVolume* PPV = Cast<APostProcessVolume>(Actor);
        if (PPV && PPV->bIsUnbound) // Find the global post-process volume
        {
            MainPostProcessVolume = PPV;
            break;
        }
    }

    // Initialize fog effects if enabled
    if (bEnableDynamicFog && DynamicFog)
    {
        DynamicFog->SetVisibility(false); // Start invisible
        UpdateFogParameters();
    }

    // Initialize material parameter collection
    if (VisibilityMPC)
    {
        UpdateMaterialParameterCollection();
    }
}

void UVisibilitySystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return; // Only simulate on server
    }

    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    // Update visibility systems based on timing
    if (CurrentTime - LastZoneUpdate > ZoneUpdateInterval)
    {
        UpdateVisibilityZones(DeltaTime);
        LastZoneUpdate = CurrentTime;
    }

    if (CurrentTime - LastEffectUpdate > EffectUpdateInterval)
    {
        UpdateGlobalVisibilityCondition(DeltaTime);
        ProcessVisibilityMovement(DeltaTime);
        CalculateVisibilityInteractions(DeltaTime);
        UpdateAtmosphericScattering(DeltaTime);
        LastEffectUpdate = CurrentTime;
    }

    if (bUpdateVisualEffects && CurrentTime - LastVisualUpdate > VisualUpdateInterval)
    {
        UpdatePostProcessEffects(DeltaTime);
        UpdateFogEffects(DeltaTime);
        UpdateMaterialParameterCollection();
        LastVisualUpdate = CurrentTime;
    }

    // Process visibility blending
    if (bIsBlendingVisibility)
    {
        BlendProgress += DeltaTime / BlendDuration;
        
        if (BlendProgress >= 1.0f)
        {
            GlobalCondition = BlendTargetCondition;
            bIsBlendingVisibility = false;
            BlendProgress = 1.0f;
        }
        else
        {
            // Interpolate visibility conditions
            float Alpha = FMath::SmoothStep(0.0f, 1.0f, BlendProgress);
            GlobalCondition.Intensity = FMath::Lerp(BlendStartCondition.Intensity, BlendTargetCondition.Intensity, Alpha);
            GlobalCondition.VisualRange = FMath::Lerp(BlendStartCondition.VisualRange, BlendTargetCondition.VisualRange, Alpha);
            GlobalCondition.Density = FMath::Lerp(BlendStartCondition.Density, BlendTargetCondition.Density, Alpha);
        }
    }

    // Process weather integration
    ProcessWeatherVisibilityEffects(DeltaTime);

    // Clear old cache entries
    if (bUseVisibilityCache && CurrentTime - LastCacheUpdate > CacheTimeout)
    {
        VisibilityCache.Empty();
        EffectsCache.Empty();
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
        
        OptimizeVisibilitySimulation(ClosestPlayerDistance);
        LastLODUpdate = CurrentTime;
    }
}

void UVisibilitySystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UVisibilitySystem, VisibilityZones);
    DOREPLIFETIME(UVisibilitySystem, GlobalCondition);
}

void UVisibilitySystem::RegisterVisibilityZone(const FVisibilityZone& Zone)
{
    if (VisibilityZones.Num() >= MaxVisibilityZones)
    {
        // Remove oldest zone to make room
        VisibilityZones.RemoveAt(0);
    }

    // Check if zone already exists
    for (int32 i = 0; i < VisibilityZones.Num(); ++i)
    {
        if (VisibilityZones[i].ZoneID == Zone.ZoneID)
        {
            VisibilityZones[i] = Zone; // Update existing
            return;
        }
    }
    
    VisibilityZones.Add(Zone);
}

void UVisibilitySystem::UnregisterVisibilityZone(const FString& ZoneID)
{
    VisibilityZones.RemoveAll([&ZoneID](const FVisibilityZone& Zone)
    {
        return Zone.ZoneID == ZoneID;
    });
}

FVisibilityZone UVisibilitySystem::GetVisibilityZone(const FString& ZoneID) const
{
    for (const FVisibilityZone& Zone : VisibilityZones)
    {
        if (Zone.ZoneID == ZoneID)
        {
            return Zone;
        }
    }
    
    // Return default zone if not found
    FVisibilityZone DefaultZone;
    DefaultZone.ZoneID = TEXT("Default");
    DefaultZone.Condition.VisualRange = StandardVisibility;
    return DefaultZone;
}

TArray<FString> UVisibilitySystem::GetVisibilityZonesAtLocation(const FVector& Location) const
{
    TArray<FString> ZoneIDs;
    
    for (const FVisibilityZone& Zone : VisibilityZones)
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

float UVisibilitySystem::GetVisibilityAtLocation(const FVector& Location) const
{
    // Use cache if available
    if (bUseVisibilityCache)
    {
        if (const float* CachedVisibility = VisibilityCache.Find(Location))
        {
            return *CachedVisibility;
        }
    }

    float ResultVisibility = 1.0f; // Start with perfect visibility
    
    // Apply global visibility condition
    if (GlobalCondition.HazardType != EVisibilityHazardType::None)
    {
        float GlobalVisibilityFactor = CalculateDistanceVisibility(0.0f, GlobalCondition);
        ResultVisibility *= GlobalVisibilityFactor;
    }

    // Apply local visibility zones
    TArray<FString> ZoneIDs = GetVisibilityZonesAtLocation(Location);
    
    for (const FString& ZoneID : ZoneIDs)
    {
        FVisibilityZone Zone = GetVisibilityZone(ZoneID);
        
        // Calculate distance-based weighting within zone
        float Distance = FVector::Dist(Location, Zone.Center);
        float MaxDistance = FMath::Max(Zone.Extent.X, FMath::Max(Zone.Extent.Y, Zone.Extent.Z));
        float Weight = 1.0f - (Distance / MaxDistance);
        Weight = FMath::Clamp(Weight, 0.1f, 1.0f);
        
        // Apply altitude gradient
        if (Zone.AltitudeGradient != 0.0f)
        {
            float AltitudeDifference = Location.Z - Zone.Center.Z;
            float AltitudeEffect = Zone.AltitudeGradient * AltitudeDifference * 0.001f; // Convert cm to m
            Zone.Condition.Intensity = FMath::Clamp(Zone.Condition.Intensity + AltitudeEffect, 0.0f, 1.0f);
        }
        
        float ZoneVisibilityFactor = CalculateDistanceVisibility(0.0f, Zone.Condition);
        ResultVisibility *= FMath::Lerp(1.0f, ZoneVisibilityFactor, Weight);
    }

    // Apply atmospheric visibility based on weather
    if (HazardManager)
    {
        float Humidity = 0.5f; // Would get from weather system
        float Temperature = HazardManager->GetTemperatureAtLocation(Location);
        float Pollution = 0.1f; // Would get from environmental system
        
        float AtmosphericVisibility = CalculateAtmosphericVisibility(Humidity, Temperature, Pollution);
        ResultVisibility *= AtmosphericVisibility;
    }

    // Apply distance-based atmospheric haze
    float HorizonDistance = GetHorizonDistance(Location);
    if (HorizonDistance < StandardVisibility)
    {
        float HazeEffect = HorizonDistance / StandardVisibility;
        ResultVisibility *= HazeEffect;
    }

    ResultVisibility = FMath::Clamp(ResultVisibility * GlobalVisibilityMultiplier, 0.0f, 1.0f);

    // Cache the result
    if (bUseVisibilityCache)
    {
        VisibilityCache.Add(Location, ResultVisibility);
    }

    return ResultVisibility;
}

float UVisibilitySystem::GetVisualRangeAtLocation(const FVector& Location) const
{
    float Visibility = GetVisibilityAtLocation(Location);
    return ConvertVisibilityToVisualRange(Visibility);
}

FVisibilityEffects UVisibilitySystem::CalculateVisibilityEffects(const FVector& Location) const
{
    // Use cache if available
    if (bUseVisibilityCache)
    {
        if (const FVisibilityEffects* CachedEffects = EffectsCache.Find(Location))
        {
            return *CachedEffects;
        }
    }

    FVisibilityEffects Effects;
    
    float Visibility = GetVisibilityAtLocation(Location);
    Effects.VisualRangeMultiplier = Visibility;
    
    // Calculate contrast reduction based on visibility
    Effects.ContrastReduction = (1.0f - Visibility) * 0.8f;
    
    // Calculate color desaturation
    Effects.ColorDesaturation = (1.0f - Visibility) * 0.6f;
    
    // Determine tint color from active hazards
    FLinearColor TotalTint = FLinearColor::White;
    float TintWeight = 0.0f;
    
    if (GlobalCondition.HazardType != EVisibilityHazardType::None)
    {
        if (const FLinearColor* ScatteringColor = ScatteringColors.Find(GlobalCondition.HazardType))
        {
            TotalTint += *ScatteringColor * GlobalCondition.Intensity;
            TintWeight += GlobalCondition.Intensity;
        }
    }
    
    // Apply zone-based tinting
    TArray<FString> ZoneIDs = GetVisibilityZonesAtLocation(Location);
    for (const FString& ZoneID : ZoneIDs)
    {
        FVisibilityZone Zone = GetVisibilityZone(ZoneID);
        if (const FLinearColor* ScatteringColor = ScatteringColors.Find(Zone.Condition.HazardType))
        {
            float ZoneWeight = Zone.Condition.Intensity * 0.5f; // Zones have less influence than global
            TotalTint += *ScatteringColor * ZoneWeight;
            TintWeight += ZoneWeight;
        }
    }
    
    if (TintWeight > 0.0f)
    {
        Effects.TintColor = TotalTint / (1.0f + TintWeight);
    }
    
    // Calculate depth blur
    Effects.DepthBlur = (1.0f - Visibility) * 0.7f;
    
    // Calculate light halo effect
    Effects.LightHalo = FMath::Min((1.0f - Visibility) * 2.0f, 1.0f);
    
    // Calculate sound attenuation (fog and precipitation attenuate sound)
    Effects.SoundAttenuation = FMath::Lerp(1.0f, 0.3f, 1.0f - Visibility);
    
    // Calculate navigation difficulty
    Effects.NavigationDifficulty = FMath::Pow(1.0f - Visibility, 1.5f);
    
    // Cache the result
    if (bUseVisibilityCache)
    {
        EffectsCache.Add(Location, Effects);
    }
    
    return Effects;
}

bool UVisibilitySystem::IsLocationVisible(const FVector& ViewerLocation, const FVector& TargetLocation) const
{
    float Distance = FVector::Dist(ViewerLocation, TargetLocation);
    
    // Get visibility at both locations
    float ViewerVisibility = GetVisibilityAtLocation(ViewerLocation);
    float TargetVisibility = GetVisibilityAtLocation(TargetLocation);
    
    // Use the worse visibility of the two
    float EffectiveVisibility = FMath::Min(ViewerVisibility, TargetVisibility);
    
    // Calculate visual range
    float VisualRange = ConvertVisibilityToVisualRange(EffectiveVisibility);
    
    // Check if target is within visual range
    return Distance <= VisualRange;
}

FLightScatteringData UVisibilitySystem::CalculateLightScattering(const FVector& Location, const FVisibilityCondition& Condition) const
{
    FLightScatteringData ScatteringData;
    
    if (Condition.HazardType == EVisibilityHazardType::None)
    {
        return ScatteringData;
    }
    
    // Determine scattering type based on particle size
    if (Condition.ParticleSize < 0.1f) // Very small particles
    {
        ScatteringData.ScatteringType = ELightScattering::Rayleigh;
        ScatteringData.ScatteringCoefficient = RayleighScatteringCoeff * Condition.Density;
    }
    else if (Condition.ParticleSize > 10.0f) // Large particles
    {
        ScatteringData.ScatteringType = ELightScattering::Mie;
        ScatteringData.ScatteringCoefficient = MieScatteringBase * Condition.Density * Condition.ParticleSize;
    }
    else // Mixed scattering
    {
        ScatteringData.ScatteringType = ELightScattering::Mixed;
        float RayleighComponent = RayleighScatteringCoeff * Condition.Density * (1.0f - Condition.ParticleSize);
        float MieComponent = MieScatteringBase * Condition.Density * Condition.ParticleSize;
        ScatteringData.ScatteringCoefficient = RayleighComponent + MieComponent;
    }
    
    // Calculate absorption coefficient
    ScatteringData.AbsorptionCoefficient = CalculateExtinctionCoefficient(Condition.HazardType, Condition.Intensity) * 0.1f;
    
    // Total extinction = scattering + absorption
    ScatteringData.ExtinctionCoefficient = ScatteringData.ScatteringCoefficient + ScatteringData.AbsorptionCoefficient;
    
    // Get color shift from lookup table
    if (const FLinearColor* Color = ScatteringColors.Find(Condition.HazardType))
    {
        ScatteringData.ColorShift = *Color;
    }
    
    // Calculate halo and glare effects
    ScatteringData.HaloIntensity = Condition.Intensity * 0.5f;
    ScatteringData.GlareIntensity = Condition.Intensity * ScatteringData.ScatteringCoefficient * 100.0f;
    
    return ScatteringData;
}

float UVisibilitySystem::CalculateExtinctionCoefficient(EVisibilityHazardType HazardType, float Intensity) const
{
    if (const float* BaseCoeff = ExtinctionCoefficients.Find(HazardType))
    {
        return *BaseCoeff * Intensity;
    }
    return 0.0f;
}

FLinearColor UVisibilitySystem::CalculateScatteredLightColor(const FLinearColor& IncomingLight, const FLightScatteringData& Scattering) const
{
    FLinearColor ScatteredLight = IncomingLight;
    
    switch (Scattering.ScatteringType)
    {
    case ELightScattering::Rayleigh:
        // Rayleigh scattering favors blue light (1/λ⁴ dependence)
        ScatteredLight.R *= 0.3f;
        ScatteredLight.G *= 0.6f;
        ScatteredLight.B *= 1.0f;
        break;
        
    case ELightScattering::Mie:
        // Mie scattering is more wavelength-neutral but forward-directed
        ScatteredLight *= 0.8f;
        break;
        
    case ELightScattering::Mixed:
        // Combination of both effects
        ScatteredLight.R *= 0.6f;
        ScatteredLight.G *= 0.8f;
        ScatteredLight.B *= 1.0f;
        break;
    }
    
    // Apply color shift from particles
    ScatteredLight *= Scattering.ColorShift;
    
    // Apply extinction
    float ExtinctionFactor = FMath::Exp(-Scattering.ExtinctionCoefficient);
    ScatteredLight *= ExtinctionFactor;
    
    return ScatteredLight;
}

float UVisibilitySystem::CalculatePrecipitationVisibility(EVisibilityHazardType PrecipitationType, float Intensity, float DropletSize) const
{
    float BaseVisibility = 1.0f;
    
    switch (PrecipitationType)
    {
    case EVisibilityHazardType::Rain:
        // Marshall-Palmer drop size distribution for rain
        BaseVisibility = FMath::Exp(-0.01f * Intensity * FMath::Sqrt(DropletSize));
        break;
        
    case EVisibilityHazardType::Snow:
        // Snow visibility depends on flake size and density
        BaseVisibility = FMath::Exp(-0.02f * Intensity * DropletSize);
        break;
        
    case EVisibilityHazardType::Hail:
        // Hail creates severe visibility reduction
        BaseVisibility = FMath::Exp(-0.05f * Intensity * DropletSize);
        break;
        
    case EVisibilityHazardType::Blizzard:
        // Blizzard combines snow with wind-driven particles
        BaseVisibility = FMath::Exp(-0.08f * Intensity * DropletSize);
        break;
        
    default:
        break;
    }
    
    return FMath::Clamp(BaseVisibility, 0.01f, 1.0f);
}

void UVisibilitySystem::UpdatePrecipitationEffects(EVisibilityHazardType Type, float Intensity, float DeltaTime)
{
    if (Type == EVisibilityHazardType::None || Intensity <= 0.0f)
    {
        return;
    }
    
    // Update global condition based on precipitation
    FVisibilityCondition PrecipCondition;
    PrecipCondition.HazardType = Type;
    PrecipCondition.Intensity = Intensity;
    PrecipCondition.Density = Intensity;
    
    switch (Type)
    {
    case EVisibilityHazardType::Rain:
        PrecipCondition.VisualRange = FMath::Lerp(StandardVisibility, 1000.0f, Intensity); // 10m to 500m
        PrecipCondition.ParticleSize = 2.0f; // mm
        PrecipCondition.Tint = FColor(200, 210, 220, 255);
        break;
        
    case EVisibilityHazardType::Snow:
        PrecipCondition.VisualRange = FMath::Lerp(StandardVisibility, 500.0f, Intensity); // 5m to 500m
        PrecipCondition.ParticleSize = 5.0f; // mm
        PrecipCondition.Tint = FColor(240, 240, 255, 255);
        break;
        
    case EVisibilityHazardType::Hail:
        PrecipCondition.VisualRange = FMath::Lerp(StandardVisibility, 200.0f, Intensity); // 2m to 500m
        PrecipCondition.ParticleSize = 10.0f; // mm
        PrecipCondition.Tint = FColor(220, 220, 240, 255);
        break;
        
    case EVisibilityHazardType::Blizzard:
        PrecipCondition.VisualRange = FMath::Lerp(StandardVisibility, 100.0f, Intensity); // 1m to 500m
        PrecipCondition.ParticleSize = 8.0f; // mm
        PrecipCondition.Tint = FColor(200, 210, 230, 255);
        PrecipCondition.Pattern = EVisibilityPattern::Moving;
        
        // Add wind-driven movement
        if (HazardManager)
        {
            FVector WindVelocity = HazardManager->GetWindVelocityAtLocation(GetOwner()->GetActorLocation());
            PrecipCondition.MovementDirection = WindVelocity.GetSafeNormal();
            PrecipCondition.MovementSpeed = WindVelocity.Size() * 100.0f; // Convert to cm/s
        }
        break;
        
    default:
        break;
    }
    
    // Blend with existing global condition
    if (GlobalCondition.HazardType == EVisibilityHazardType::None)
    {
        GlobalCondition = PrecipCondition;
    }
    else
    {
        // Combine effects (use worst visibility)
        GlobalCondition.VisualRange = FMath::Min(GlobalCondition.VisualRange, PrecipCondition.VisualRange);
        GlobalCondition.Intensity = FMath::Max(GlobalCondition.Intensity, PrecipCondition.Intensity);
        GlobalCondition.Density = FMath::Max(GlobalCondition.Density, PrecipCondition.Density);
    }
}

void UVisibilitySystem::UpdateFogConditions(float Humidity, float Temperature, float Pressure, float DeltaTime)
{
    // Calculate fog formation potential
    float DewPoint = Temperature - ((100.0f - Humidity * 100.0f) / 5.0f); // Simplified dew point
    float TemperatureDewPointDiff = Temperature - DewPoint;
    
    // Fog forms when temperature approaches dew point
    if (TemperatureDewPointDiff < 3.0f && Humidity > 0.85f)
    {
        // Calculate fog intensity
        float FogIntensity = FMath::Clamp((3.0f - TemperatureDewPointDiff) / 3.0f * (Humidity - 0.85f) / 0.15f, 0.0f, 1.0f);
        
        FVisibilityCondition FogCondition;
        FogCondition.HazardType = EVisibilityHazardType::Fog;
        FogCondition.Intensity = FogIntensity;
        FogCondition.VisualRange = FMath::Lerp(StandardVisibility, 100.0f, FogIntensity);
        FogCondition.Density = FogIntensity * 0.8f;
        FogCondition.ParticleSize = 0.01f; // Very small water droplets
        FogCondition.Tint = FColor(230, 240, 255, 255);
        FogCondition.Pattern = EVisibilityPattern::Uniform;
        
        // Apply fog condition gradually
        float BlendRate = 0.1f; // Fog forms/dissipates slowly
        
        if (GlobalCondition.HazardType == EVisibilityHazardType::Fog)
        {
            GlobalCondition.Intensity = FMath::FInterpTo(GlobalCondition.Intensity, FogCondition.Intensity, DeltaTime, BlendRate);
            GlobalCondition.VisualRange = FMath::FInterpTo(GlobalCondition.VisualRange, FogCondition.VisualRange, DeltaTime, BlendRate);
            GlobalCondition.Density = FMath::FInterpTo(GlobalCondition.Density, FogCondition.Density, DeltaTime, BlendRate);
        }
        else if (GlobalCondition.HazardType == EVisibilityHazardType::None)
        {
            BlendGlobalVisibility(FogCondition, 30.0f); // 30 second formation time
        }
    }
    else if (GlobalCondition.HazardType == EVisibilityHazardType::Fog)
    {
        // Clear fog gradually
        FVisibilityCondition ClearCondition;
        ClearCondition.HazardType = EVisibilityHazardType::None;
        BlendGlobalVisibility(ClearCondition, 60.0f); // 60 second dissipation time
    }
}

void UVisibilitySystem::UpdateDustConditions(float WindSpeed, float AridityLevel, float DeltaTime)
{
    // Dust storms form in arid conditions with high winds
    if (WindSpeed > 8.0f && AridityLevel > 0.7f) // >8 m/s wind in arid conditions
    {
        float DustIntensity = FMath::Clamp((WindSpeed - 8.0f) / 15.0f * AridityLevel, 0.0f, 1.0f);
        
        FVisibilityCondition DustCondition;
        DustCondition.HazardType = (WindSpeed > 15.0f) ? EVisibilityHazardType::Sand : EVisibilityHazardType::Dust;
        DustCondition.Intensity = DustIntensity;
        DustCondition.VisualRange = FMath::Lerp(StandardVisibility, 50.0f, DustIntensity);
        DustCondition.Density = DustIntensity;
        DustCondition.ParticleSize = (DustCondition.HazardType == EVisibilityHazardType::Sand) ? 0.5f : 0.1f;
        DustCondition.Tint = FColor(200, 180, 120, 255); // Brown-yellow
        DustCondition.Pattern = EVisibilityPattern::Moving;
        
        // Add wind-driven movement
        if (HazardManager)
        {
            FVector WindDirection = HazardManager->GetWindVelocityAtLocation(GetOwner()->GetActorLocation()).GetSafeNormal();
            DustCondition.MovementDirection = WindDirection;
            DustCondition.MovementSpeed = WindSpeed * 100.0f; // Convert to cm/s
        }
        
        if (GlobalCondition.HazardType == EVisibilityHazardType::None)
        {
            BlendGlobalVisibility(DustCondition, 10.0f); // Quick formation
        }
    }
}

float UVisibilitySystem::CalculateAtmosphericVisibility(float Humidity, float Temperature, float Pollution) const
{
    // Base atmospheric visibility calculation
    float BaseVisibility = 1.0f;
    
    // Humidity effect (higher humidity reduces visibility)
    BaseVisibility *= (1.0f - Humidity * 0.3f);
    
    // Temperature inversion effect (can trap pollutants)
    if (Temperature > 25.0f) // Hot conditions can create haze
    {
        BaseVisibility *= (1.0f - (Temperature - 25.0f) * 0.01f);
    }
    
    // Pollution effect
    BaseVisibility *= (1.0f - Pollution * 0.5f);
    
    return FMath::Clamp(BaseVisibility, 0.1f, 1.0f);
}

void UVisibilitySystem::SetGlobalVisibilityCondition(const FVisibilityCondition& Condition)
{
    FVisibilityCondition OldCondition = GlobalCondition;
    GlobalCondition = Condition;
    
    // Trigger visibility change event
    if (OldCondition.Intensity != Condition.Intensity || OldCondition.HazardType != Condition.HazardType)
    {
        OnVisibilityChanged.Broadcast();
        
        // Check for low visibility warning
        if (Condition.Intensity > LowVisibilityThreshold && OldCondition.Intensity <= LowVisibilityThreshold)
        {
            OnLowVisibilityEntered.Broadcast();
        }
        else if (Condition.Intensity <= LowVisibilityThreshold && OldCondition.Intensity > LowVisibilityThreshold)
        {
            OnVisibilityCleared.Broadcast();
        }
    }
}

void UVisibilitySystem::ClearGlobalVisibilityCondition()
{
    FVisibilityCondition ClearCondition;
    SetGlobalVisibilityCondition(ClearCondition);
}

void UVisibilitySystem::BlendGlobalVisibility(const FVisibilityCondition& TargetCondition, float BlendTime)
{
    BlendStartCondition = GlobalCondition;
    BlendTargetCondition = TargetCondition;
    BlendDuration = FMath::Max(BlendTime, 0.1f);
    BlendProgress = 0.0f;
    bIsBlendingVisibility = true;
}

void UVisibilitySystem::ApplyVisibilityEffectsToClimber(UAdvancedClimbingComponent* ClimbingComponent, float DeltaTime)
{
    if (!ClimbingComponent)
    {
        return;
    }

    FVector ClimberLocation = ClimbingComponent->GetOwner()->GetActorLocation();
    FVisibilityEffects Effects = CalculateVisibilityEffects(ClimberLocation);
    
    // Apply navigation difficulty
    if (Effects.NavigationDifficulty > 0.1f)
    {
        // Reduced climbing precision due to poor visibility
        float PrecisionReduction = Effects.NavigationDifficulty * 0.3f;
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - PrecisionReduction);
        
        // Increased stamina drain from concentration effort
        float ConcentrationStrain = Effects.NavigationDifficulty * 10.0f;
        ClimbingComponent->ConsumeStamina(ConcentrationStrain * DeltaTime);
        
        // Reduced grip precision from visibility challenges
        if (Effects.NavigationDifficulty > 0.5f)
        {
            float GripPenalty = (Effects.NavigationDifficulty - 0.5f) * 20.0f;
            ClimbingComponent->ConsumeGripStrength(GripPenalty * DeltaTime);
        }
    }
    
    // Apply visual range limitation to route finding
    float VisualRange = GetVisualRangeAtLocation(ClimberLocation);
    if (VisualRange < 1000.0f) // Less than 10m visibility
    {
        // Severely impaired route finding
        float RouteFindingImpairment = 1.0f - (VisualRange / 1000.0f);
        ClimbingComponent->Settings.ClimbingSpeed *= (1.0f - RouteFindingImpairment * 0.5f);
        
        // Additional stamina drain from cautious movement
        ClimbingComponent->ConsumeStamina(RouteFindingImpairment * 15.0f * DeltaTime);
    }
}

float UVisibilitySystem::CalculateNavigationDifficulty(const FVector& Location) const
{
    FVisibilityEffects Effects = CalculateVisibilityEffects(Location);
    return Effects.NavigationDifficulty;
}

bool UVisibilitySystem::CanSeeClimbingRoute(const FVector& ViewerLocation, const TArray<FVector>& RoutePoints) const
{
    if (RoutePoints.Num() == 0)
    {
        return false;
    }
    
    // Check visibility to each route point
    for (const FVector& Point : RoutePoints)
    {
        if (!IsLocationVisible(ViewerLocation, Point))
        {
            return false; // If any point is not visible, route is not visible
        }
    }
    
    return true;
}

void UVisibilitySystem::UpdatePostProcessEffects(float DeltaTime)
{
    if (!bEnablePostProcessEffects || !VisibilityPostProcess)
    {
        return;
    }

    FVector ViewerLocation = GetOwner()->GetActorLocation();
    FVisibilityEffects Effects = CalculateVisibilityEffects(ViewerLocation);
    
    // Calculate post-process weight based on visibility effects
    float PostProcessWeight = FMath::Max({Effects.ContrastReduction, Effects.ColorDesaturation, Effects.DepthBlur}) * 0.8f;
    SetVisibilityPostProcessWeight(PostProcessWeight);
    
    // Update post-process parameters
    UpdatePostProcessParameters();
}

void UVisibilitySystem::UpdateFogEffects(float DeltaTime)
{
    if (!bEnableDynamicFog || !DynamicFog)
    {
        return;
    }

    // Update fog visibility based on global condition
    bool bShouldShowFog = (GlobalCondition.HazardType == EVisibilityHazardType::Fog || 
                          GlobalCondition.HazardType == EVisibilityHazardType::Mist) &&
                          GlobalCondition.Intensity > 0.1f;
    
    DynamicFog->SetVisibility(bShouldShowFog);
    
    if (bShouldShowFog)
    {
        UpdateFogParameters();
    }
}

void UVisibilitySystem::UpdateMaterialParameterCollection()
{
    if (!VisibilityMPC)
    {
        return;
    }

    UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(VisibilityMPC);
    if (!MPCInstance)
    {
        return;
    }

    FVector ViewerLocation = GetOwner()->GetActorLocation();
    FVisibilityEffects Effects = CalculateVisibilityEffects(ViewerLocation);
    
    // Update material parameters for visibility effects
    MPCInstance->SetScalarParameterValue(TEXT("VisibilityRange"), GetVisualRangeAtLocation(ViewerLocation));
    MPCInstance->SetScalarParameterValue(TEXT("FogDensity"), GlobalCondition.Density);
    MPCInstance->SetScalarParameterValue(TEXT("ContrastReduction"), Effects.ContrastReduction);
    MPCInstance->SetScalarParameterValue(TEXT("ColorDesaturation"), Effects.ColorDesaturation);
    MPCInstance->SetVectorParameterValue(TEXT("TintColor"), Effects.TintColor);
    MPCInstance->SetScalarParameterValue(TEXT("LightHaloIntensity"), Effects.LightHalo);
}

void UVisibilitySystem::SetVisibilityPostProcessWeight(float Weight)
{
    if (VisibilityPostProcess)
    {
        VisibilityPostProcess->BlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
    }
}

float UVisibilitySystem::CalculateDistanceVisibility(float Distance, const FVisibilityCondition& Condition) const
{
    if (Condition.HazardType == EVisibilityHazardType::None || Condition.Intensity <= 0.0f)
    {
        return 1.0f;
    }
    
    // Use Koschmieder equation for extinction
    float ExtinctionCoeff = CalculateExtinctionCoefficient(Condition.HazardType, Condition.Intensity);
    float Visibility = CalculateKoschmiederEquation(Distance * 0.01f, ExtinctionCoeff); // Convert cm to m
    
    return FMath::Clamp(Visibility, 0.0f, 1.0f);
}

float UVisibilitySystem::GetHorizonDistance(const FVector& ViewerLocation) const
{
    // Calculate geometric horizon based on Earth curvature and viewer height
    float HeightMeters = ViewerLocation.Z * 0.01f; // Convert cm to meters
    float EarthRadius = 6371000.0f; // meters
    
    if (HeightMeters <= 0.0f)
    {
        return StandardVisibility; // At ground level, use standard visibility
    }
    
    // Geometric horizon distance = sqrt(2 * R * h)
    float HorizonDistance = FMath::Sqrt(2.0f * EarthRadius * HeightMeters) * 100.0f; // Convert to cm
    
    return FMath::Min(HorizonDistance, StandardVisibility);
}

float UVisibilitySystem::CalculateAngularSize(float ObjectSize, float Distance) const
{
    // Calculate angular size in radians
    return 2.0f * FMath::Atan(ObjectSize / (2.0f * Distance));
}

void UVisibilitySystem::SetVisibilityLOD(int32 LODLevel)
{
    CurrentLOD = FMath::Clamp(LODLevel, 0, 3);
    
    // Adjust update frequencies based on LOD
    switch (CurrentLOD)
    {
    case 0: // High detail
        ZoneUpdateInterval = 0.2f;     // 5Hz
        EffectUpdateInterval = 0.05f;  // 20Hz
        VisualUpdateInterval = 0.033f; // 30Hz
        bUpdateVisualEffects = true;
        break;
    case 1: // Medium detail
        ZoneUpdateInterval = 0.5f;     // 2Hz
        EffectUpdateInterval = 0.1f;   // 10Hz
        VisualUpdateInterval = 0.066f; // 15Hz
        bUpdateVisualEffects = true;
        break;
    case 2: // Low detail
        ZoneUpdateInterval = 1.0f;     // 1Hz
        EffectUpdateInterval = 0.2f;   // 5Hz
        VisualUpdateInterval = 0.1f;   // 10Hz
        bUpdateVisualEffects = false;
        break;
    case 3: // Minimal detail
        ZoneUpdateInterval = 2.0f;     // 0.5Hz
        EffectUpdateInterval = 0.5f;   // 2Hz
        VisualUpdateInterval = 0.2f;   // 5Hz
        bUpdateVisualEffects = false;
        break;
    }
}

void UVisibilitySystem::OptimizeVisibilitySimulation(float ViewerDistance)
{
    if (ViewerDistance < 2000.0f) // 20m
    {
        SetVisibilityLOD(0);
    }
    else if (ViewerDistance < 5000.0f) // 50m
    {
        SetVisibilityLOD(1);
    }
    else if (ViewerDistance < 10000.0f) // 100m
    {
        SetVisibilityLOD(2);
    }
    else
    {
        SetVisibilityLOD(3);
    }
}

// Protected implementation functions
void UVisibilitySystem::UpdateVisibilityZones(float DeltaTime)
{
    for (FVisibilityZone& Zone : VisibilityZones)
    {
        // Update zone movement if applicable
        if (Zone.Condition.Pattern == EVisibilityPattern::Moving && Zone.Condition.MovementSpeed > 0.0f)
        {
            FVector Movement = Zone.Condition.MovementDirection * Zone.Condition.MovementSpeed * DeltaTime;
            Zone.Center += Movement;
        }
        
        // Update zone intensity based on environmental factors
        if (HazardManager)
        {
            float Temperature = HazardManager->GetTemperatureAtLocation(Zone.Center);
            if (Zone.TemperatureEffect != 0.0f)
            {
                float TempModifier = Zone.TemperatureEffect * (Temperature - 20.0f) * 0.01f;
                Zone.Condition.Intensity = FMath::Clamp(Zone.Condition.Intensity + TempModifier, 0.0f, 1.0f);
            }
        }
    }
}

void UVisibilitySystem::UpdateGlobalVisibilityCondition(float DeltaTime)
{
    // Natural dissipation of visibility conditions over time
    if (GlobalCondition.HazardType != EVisibilityHazardType::None)
    {
        float DissipationRate = 0.01f; // 1% per second natural dissipation
        
        switch (GlobalCondition.HazardType)
        {
        case EVisibilityHazardType::Fog:
        case EVisibilityHazardType::Mist:
            DissipationRate = 0.005f; // Fog dissipates slowly
            break;
        case EVisibilityHazardType::Dust:
        case EVisibilityHazardType::Sand:
            DissipationRate = 0.02f; // Dust settles relatively quickly
            break;
        case EVisibilityHazardType::Smoke:
            DissipationRate = 0.05f; // Smoke disperses quickly
            break;
        }
        
        GlobalCondition.Intensity = FMath::Max(0.0f, GlobalCondition.Intensity - DissipationRate * DeltaTime);
        
        // Clear condition if intensity becomes negligible
        if (GlobalCondition.Intensity < 0.01f)
        {
            ClearGlobalVisibilityCondition();
        }
    }
}

void UVisibilitySystem::ProcessVisibilityMovement(float DeltaTime)
{
    // Process movement patterns for global condition
    if (GlobalCondition.Pattern == EVisibilityPattern::Moving && GlobalCondition.MovementSpeed > 0.0f)
    {
        // This would update the global visibility field movement
        // For now, we apply wind-based movement to the condition
        if (HazardManager)
        {
            FVector WindVelocity = HazardManager->GetWindVelocityAtLocation(GetOwner()->GetActorLocation());
            if (WindVelocity.Size() > 1.0f)
            {
                GlobalCondition.MovementDirection = WindVelocity.GetSafeNormal();
                GlobalCondition.MovementSpeed = WindVelocity.Size() * 100.0f; // Convert to cm/s
            }
        }
    }
}

void UVisibilitySystem::CalculateVisibilityInteractions(float DeltaTime)
{
    // Calculate interactions between different visibility zones
    for (int32 i = 0; i < VisibilityZones.Num(); ++i)
    {
        for (int32 j = i + 1; j < VisibilityZones.Num(); ++j)
        {
            FVisibilityZone& ZoneA = VisibilityZones[i];
            FVisibilityZone& ZoneB = VisibilityZones[j];
            
            float Distance = FVector::Dist(ZoneA.Center, ZoneB.Center);
            float CombinedExtent = FMath::Max(ZoneA.Extent.Size(), ZoneB.Extent.Size());
            
            if (Distance < CombinedExtent)
            {
                // Zones are interacting - blend their effects
                float OverlapFactor = 1.0f - (Distance / CombinedExtent);
                
                // Mix intensities based on overlap
                float MixedIntensity = FMath::Max(ZoneA.Condition.Intensity, ZoneB.Condition.Intensity) * OverlapFactor;
                ZoneA.Condition.Intensity = FMath::Max(ZoneA.Condition.Intensity, MixedIntensity * 0.5f);
                ZoneB.Condition.Intensity = FMath::Max(ZoneB.Condition.Intensity, MixedIntensity * 0.5f);
            }
        }
    }
}

void UVisibilitySystem::UpdateAtmosphericScattering(float DeltaTime)
{
    // Update atmospheric scattering effects based on time of day and weather
    if (GetWorld())
    {
        float TimeOfDay = FMath::Fmod(GetWorld()->GetTimeSeconds() / 3600.0f, 24.0f); // Hours
        
        // Calculate sun angle for atmospheric scattering
        float SunAngle = (TimeOfDay - 12.0f) * 15.0f; // Degrees from zenith
        float AtmosphericScattering = FMath::Max(0.0f, FMath::Cos(FMath::DegreesToRadians(FMath::Abs(SunAngle))));
        
        // Apply to global visibility (atmospheric haze increases during day)
        if (GlobalCondition.HazardType == EVisibilityHazardType::None && AtmosphericScattering > 0.5f)
        {
            // Create slight atmospheric haze during bright daylight
            GlobalCondition.Intensity = FMath::Min(GlobalCondition.Intensity + AtmosphericScattering * 0.1f * DeltaTime, 0.3f);
        }
    }
}

void UVisibilitySystem::ProcessWeatherVisibilityEffects(float DeltaTime)
{
    // Integration with weather system through hazard manager
    if (HazardManager)
    {
        // Get current weather conditions
        float Temperature = HazardManager->GetTemperatureAtLocation(GetOwner()->GetActorLocation());
        float Humidity = 0.5f; // Would get from weather system
        float Pressure = 101325.0f; // Would get from weather system
        
        // Update fog conditions based on weather
        UpdateFogConditions(Humidity, Temperature, Pressure, DeltaTime);
        
        // Update dust conditions based on wind
        FVector WindVelocity = HazardManager->GetWindVelocityAtLocation(GetOwner()->GetActorLocation());
        float WindSpeed = WindVelocity.Size();
        float AridityLevel = FMath::Max(0.0f, (Temperature - 15.0f) / 35.0f); // Simple aridity calculation
        
        UpdateDustConditions(WindSpeed, AridityLevel, DeltaTime);
    }
}

void UVisibilitySystem::UpdateFogParameters()
{
    if (!DynamicFog)
    {
        return;
    }
    
    // Update fog properties based on global condition
    float FogDensity = GlobalCondition.Density * 0.01f; // Scale for UE4 fog
    float FogHeight = StandardVisibility * GlobalCondition.Intensity; // Height affected by fog
    
    DynamicFog->SetFogDensity(FogDensity);
    DynamicFog->SetFogHeightFalloff(1.0f / FMath::Max(FogHeight, 100.0f));
    
    // Set fog color based on condition tint
    FLinearColor FogColor = FLinearColor(GlobalCondition.Tint);
    DynamicFog->SetFogInscatteringColor(FogColor);
}

void UVisibilitySystem::UpdatePostProcessParameters()
{
    if (!VisibilityPostProcess)
    {
        return;
    }
    
    FVector ViewerLocation = GetOwner()->GetActorLocation();
    FVisibilityEffects Effects = CalculateVisibilityEffects(ViewerLocation);
    
    // Update post-process settings based on visibility effects
    // This would typically modify post-process settings like:
    // - Desaturation
    // - Contrast
    // - Color tinting
    // - Depth of field blur
    // - Light bloom/halo effects
}

bool UVisibilitySystem::ShouldSimulateDetailedVisibility() const
{
    // Check if any players are within simulation range
    if (!GetWorld())
    {
        return false;
    }
    
    FVector SystemLocation = GetOwner()->GetActorLocation();
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (PC && PC->GetPawn())
        {
            float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), SystemLocation);
            if (Distance <= MaxSimulationDistance)
            {
                return true;
            }
        }
    }
    
    return false;
}

// Static utility functions
float UVisibilitySystem::ConvertVisualRangeToVisibility(float VisualRange)
{
    return FMath::Clamp(VisualRange / StandardVisibility, 0.0f, 1.0f);
}

float UVisibilitySystem::ConvertVisibilityToVisualRange(float Visibility)
{
    return FMath::Clamp(Visibility * StandardVisibility, MinimumVisibility, StandardVisibility);
}

FString UVisibilitySystem::GetVisibilityDescription(float Visibility)
{
    if (Visibility > 0.9f)
        return TEXT("Excellent");
    else if (Visibility > 0.7f)
        return TEXT("Good");
    else if (Visibility > 0.5f)
        return TEXT("Moderate");
    else if (Visibility > 0.3f)
        return TEXT("Poor");
    else if (Visibility > 0.1f)
        return TEXT("Very Poor");
    else
        return TEXT("Near Zero");
}

float UVisibilitySystem::CalculateKoschmiederEquation(float Distance, float ExtinctionCoeff)
{
    // Koschmieder's law: V = -ln(0.05) / β
    // For contrast threshold of 5%, visibility is distance where contrast = 0.05
    return FMath::Exp(-ExtinctionCoeff * Distance);
}

float UVisibilitySystem::CalculateMeteorologicalRange(float ExtinctionCoeff)
{
    // Meteorological range is distance at which visibility drops to 5%
    return -FMath::Loge(0.05f) / FMath::Max(ExtinctionCoeff, 0.001f);
}