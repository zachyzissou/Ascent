#include "DynamicWeatherSystem.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystem.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Curves/CurveFloat.h"
#include "Math/UnrealMathUtility.h"

ADynamicWeatherSystem::ADynamicWeatherSystem()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;

    // Initialize weather condition
    CurrentWeatherCondition.WeatherType = EWeatherType::Clear;
    CurrentWeatherCondition.Intensity = 0.0f;
    CurrentWeatherCondition.WindSpeed = 5.0f;
    CurrentWeatherCondition.Temperature = 65.0f;
    CurrentWeatherCondition.Humidity = 45.0f;
    CurrentWeatherCondition.BarometricPressure = 30.0f;
    CurrentWeatherCondition.VisibilityRange = 50000.0f;

    TargetWeatherCondition = CurrentWeatherCondition;

    // Create particle system components
    RainParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("RainParticleSystem"));
    RootComponent = RainParticleSystem;
    RainParticleSystem->SetAutoActivate(false);

    SnowParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SnowParticleSystem"));
    SnowParticleSystem->SetupAttachment(RootComponent);
    SnowParticleSystem->SetAutoActivate(false);

    FogParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FogParticleSystem"));
    FogParticleSystem->SetupAttachment(RootComponent);
    FogParticleSystem->SetAutoActivate(false);

    SandstormParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SandstormParticleSystem"));
    SandstormParticleSystem->SetupAttachment(RootComponent);
    SandstormParticleSystem->SetAutoActivate(false);

    // Create audio component
    WeatherAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("WeatherAudioComponent"));
    WeatherAudioComponent->SetupAttachment(RootComponent);
    WeatherAudioComponent->bAutoActivate = false;

    // Initialize timers and modifiers
    WeatherProgressionTimer = 0.0f;
    CurrentWeatherDuration = MinWeatherDuration + FMath::RandRange(0.0f, MaxWeatherDuration - MinWeatherDuration);
    GustTimer = 0.0f;
    CurrentGustMultiplier = 1.0f;
}

void ADynamicWeatherSystem::BeginPlay()
{
    Super::BeginPlay();

    // Initialize visual effects based on current weather
    UpdateVisualEffects();
    UpdateAudioEffects();

    // Start automatic weather progression if enabled
    if (bEnableAutomaticWeatherProgression && HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_WeatherProgression,
            [this]() { UpdateAutomaticWeatherProgression(0.1f); },
            0.1f,
            true
        );
    }
}

void ADynamicWeatherSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    Super::EndPlay(EndPlayReason);
}

void ADynamicWeatherSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ADynamicWeatherSystem, CurrentWeatherCondition);
    DOREPLIFETIME(ADynamicWeatherSystem, TargetWeatherCondition);
}

void ADynamicWeatherSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsTransitioning)
    {
        UpdateWeatherTransition(DeltaTime);
    }

    UpdateWindGusts(DeltaTime);
    CheckForHazardousConditions();

    // Update visual effects every tick for smooth transitions
    UpdateVisualEffects();
}

void ADynamicWeatherSystem::SetWeatherCondition(const FWeatherCondition& NewCondition, float TransitionTime)
{
    if (!HasAuthority()) return;

    FWeatherCondition OldCondition = CurrentWeatherCondition;
    
    TargetWeatherCondition = NewCondition;
    TransitionDuration = FMath::Max(TransitionTime, 1.0f);
    TransitionProgress = 0.0f;
    bIsTransitioning = true;

    // Broadcast weather change event
    OnWeatherChanged.Broadcast(OldCondition, NewCondition);

    // Check if incoming weather is hazardous
    if (IsWeatherHazardous())
    {
        OnHazardousWeatherAlert.Broadcast(NewCondition);
    }
}

void ADynamicWeatherSystem::TransitionToWeather(EWeatherType NewWeatherType, float Intensity, float TransitionTime)
{
    FWeatherCondition NewCondition = CurrentWeatherCondition;
    NewCondition.WeatherType = NewWeatherType;
    NewCondition.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);

    // Set appropriate defaults based on weather type
    switch (NewWeatherType)
    {
    case EWeatherType::Clear:
        NewCondition.WindSpeed = FMath::RandRange(3.0f, 8.0f);
        NewCondition.VisibilityRange = 50000.0f;
        break;
    case EWeatherType::LightRain:
        NewCondition.WindSpeed = FMath::RandRange(8.0f, 15.0f);
        NewCondition.VisibilityRange = FMath::RandRange(20000.0f, 35000.0f);
        NewCondition.Humidity = FMath::RandRange(80.0f, 95.0f);
        break;
    case EWeatherType::HeavyRain:
        NewCondition.WindSpeed = FMath::RandRange(15.0f, 30.0f);
        NewCondition.VisibilityRange = FMath::RandRange(8000.0f, 20000.0f);
        NewCondition.Humidity = FMath::RandRange(90.0f, 100.0f);
        break;
    case EWeatherType::Snow:
        NewCondition.WindSpeed = FMath::RandRange(10.0f, 25.0f);
        NewCondition.VisibilityRange = FMath::RandRange(10000.0f, 25000.0f);
        NewCondition.Temperature = FMath::RandRange(15.0f, 32.0f);
        break;
    case EWeatherType::Fog:
        NewCondition.WindSpeed = FMath::RandRange(2.0f, 8.0f);
        NewCondition.VisibilityRange = FMath::RandRange(1000.0f * Intensity, 10000.0f);
        NewCondition.Humidity = FMath::RandRange(95.0f, 100.0f);
        break;
    case EWeatherType::Storm:
        NewCondition.WindSpeed = FMath::RandRange(35.0f, 60.0f);
        NewCondition.VisibilityRange = FMath::RandRange(3000.0f, 10000.0f);
        NewCondition.BarometricPressure = FMath::RandRange(28.5f, 29.5f);
        break;
    case EWeatherType::HighWind:
        NewCondition.WindSpeed = FMath::RandRange(30.0f, 70.0f);
        break;
    case EWeatherType::Sandstorm:
        NewCondition.WindSpeed = FMath::RandRange(25.0f, 50.0f);
        NewCondition.VisibilityRange = FMath::RandRange(500.0f, 5000.0f);
        NewCondition.Temperature = FMath::RandRange(85.0f, 120.0f);
        break;
    }

    SetWeatherCondition(NewCondition, TransitionTime);
}

bool ADynamicWeatherSystem::IsWeatherHazardous() const
{
    return (CurrentWeatherCondition.WindSpeed >= HazardousWindSpeed) ||
           (CurrentWeatherCondition.Intensity >= HazardousRainIntensity && 
            (CurrentWeatherCondition.WeatherType == EWeatherType::HeavyRain || 
             CurrentWeatherCondition.WeatherType == EWeatherType::Storm)) ||
           (CurrentWeatherCondition.VisibilityRange <= MinimumSafeVisibility) ||
           (CurrentWeatherCondition.Temperature <= CriticalLowTemperature) ||
           (CurrentWeatherCondition.WeatherType == EWeatherType::Storm) ||
           (CurrentWeatherCondition.WeatherType == EWeatherType::Blizzard) ||
           (CurrentWeatherCondition.WeatherType == EWeatherType::Sandstorm);
}

ESeverityLevel ADynamicWeatherSystem::GetWeatherSeverity() const
{
    return UWeatherCalculationLibrary::DetermineWeatherSeverity(CurrentWeatherCondition);
}

float ADynamicWeatherSystem::GetSurfaceFrictionMultiplier() const
{
    return 1.0f - CalculateFrictionReduction();
}

float ADynamicWeatherSystem::GetStaminaDrainMultiplier() const
{
    return 1.0f + CalculateStaminaPenalty();
}

float ADynamicWeatherSystem::GetToolPlacementDifficultyMultiplier() const
{
    return 1.0f + CalculateToolDifficulty();
}

bool ADynamicWeatherSystem::ShouldForceRouteEvacuation() const
{
    ESeverityLevel Severity = GetWeatherSeverity();
    return (Severity == ESeverityLevel::Severe || Severity == ESeverityLevel::Extreme) ||
           (CurrentWeatherCondition.WeatherType == EWeatherType::Storm) ||
           (CurrentWeatherCondition.WeatherType == EWeatherType::Blizzard) ||
           (CurrentWeatherCondition.WindSpeed >= 50.0f);
}

FVector ADynamicWeatherSystem::GetWindForceAtLocation(const FVector& Location) const
{
    FVector BaseWindForce = CurrentWeatherCondition.WindDirection.GetSafeNormal() * 
                           CurrentWeatherCondition.WindSpeed * CurrentGustMultiplier;
    
    // Add some variation based on location (simplified terrain interaction)
    float LocationNoise = FMath::PerlinNoise1D(Location.X * 0.001f + Location.Y * 0.001f) * 0.3f;
    BaseWindForce *= (1.0f + LocationNoise);

    return BaseWindForce;
}

float ADynamicWeatherSystem::GetWindGustMultiplier() const
{
    return CurrentGustMultiplier;
}

float ADynamicWeatherSystem::GetVisibilityAtLocation(const FVector& Location) const
{
    float BaseVisibility = CurrentWeatherCondition.VisibilityRange;
    
    // Reduce visibility in precipitation
    if (CurrentWeatherCondition.WeatherType == EWeatherType::LightRain ||
        CurrentWeatherCondition.WeatherType == EWeatherType::HeavyRain ||
        CurrentWeatherCondition.WeatherType == EWeatherType::Snow)
    {
        BaseVisibility *= (1.0f - CurrentWeatherCondition.Intensity * 0.7f);
    }

    return FMath::Max(BaseVisibility, 100.0f); // Minimum 1 meter visibility
}

bool ADynamicWeatherSystem::IsLocationObscured(const FVector& Location) const
{
    return GetVisibilityAtLocation(Location) < MinimumSafeVisibility;
}

void ADynamicWeatherSystem::SetSeasonalModifiers(float TemperatureOffset, float PrecipitationModifier, float WindModifier)
{
    SeasonalTemperatureOffset = TemperatureOffset;
    SeasonalPrecipitationModifier = FMath::Max(PrecipitationModifier, 0.1f);
    SeasonalWindModifier = FMath::Max(WindModifier, 0.1f);

    // Apply seasonal modifiers to current weather
    CurrentWeatherCondition.Temperature += SeasonalTemperatureOffset;
    CurrentWeatherCondition.WindSpeed *= SeasonalWindModifier;
}

void ADynamicWeatherSystem::UpdateWeatherTransition(float DeltaTime)
{
    TransitionProgress += DeltaTime / TransitionDuration;
    
    if (TransitionProgress >= 1.0f)
    {
        TransitionProgress = 1.0f;
        bIsTransitioning = false;
        CurrentWeatherCondition = TargetWeatherCondition;
    }
    else
    {
        CurrentWeatherCondition = InterpolateWeatherConditions(
            CurrentWeatherCondition, TargetWeatherCondition, TransitionProgress);
    }

    UpdateAudioEffects();
    
    // Broadcast visibility changes
    static float LastVisibility = -1.0f;
    float CurrentVisibility = CurrentWeatherCondition.VisibilityRange;
    if (FMath::Abs(CurrentVisibility - LastVisibility) > 1000.0f)
    {
        OnVisibilityChanged.Broadcast(CurrentVisibility);
        LastVisibility = CurrentVisibility;
    }
}

void ADynamicWeatherSystem::UpdateWindGusts(float DeltaTime)
{
    GustTimer += DeltaTime;
    
    if (GustTimer >= GustInterval)
    {
        // Generate new gust
        float GustStrength = FMath::RandRange(1.0f, MaxGustMultiplier);
        float GustDuration = FMath::RandRange(2.0f, 8.0f);
        
        CurrentGustMultiplier = GustStrength;
        
        // Reset timer with some variation
        GustTimer = 0.0f;
        GustInterval = FMath::RandRange(10.0f, 30.0f);
        
        // Set timer to reduce gust after duration
        GetWorldTimerManager().SetTimer(
            TimerHandle_GustReduction,
            [this]() { CurrentGustMultiplier = FMath::FInterpTo(CurrentGustMultiplier, 1.0f, 0.1f, 2.0f); },
            GustDuration,
            false
        );
    }
}

void ADynamicWeatherSystem::UpdateVisualEffects()
{
    // Update particle systems based on current weather
    switch (CurrentWeatherCondition.WeatherType)
    {
    case EWeatherType::LightRain:
    case EWeatherType::HeavyRain:
    case EWeatherType::Storm:
        RainParticleSystem->SetActive(true);
        RainParticleSystem->SetFloatParameter(FName("Intensity"), CurrentWeatherCondition.Intensity);
        SnowParticleSystem->SetActive(false);
        FogParticleSystem->SetActive(false);
        SandstormParticleSystem->SetActive(false);
        break;
        
    case EWeatherType::Snow:
    case EWeatherType::Blizzard:
        SnowParticleSystem->SetActive(true);
        SnowParticleSystem->SetFloatParameter(FName("Intensity"), CurrentWeatherCondition.Intensity);
        RainParticleSystem->SetActive(false);
        FogParticleSystem->SetActive(false);
        SandstormParticleSystem->SetActive(false);
        break;
        
    case EWeatherType::Fog:
        FogParticleSystem->SetActive(true);
        FogParticleSystem->SetFloatParameter(FName("Density"), 1.0f - (CurrentWeatherCondition.VisibilityRange / 50000.0f));
        RainParticleSystem->SetActive(false);
        SnowParticleSystem->SetActive(false);
        SandstormParticleSystem->SetActive(false);
        break;
        
    case EWeatherType::Sandstorm:
        SandstormParticleSystem->SetActive(true);
        SandstormParticleSystem->SetFloatParameter(FName("Intensity"), CurrentWeatherCondition.Intensity);
        RainParticleSystem->SetActive(false);
        SnowParticleSystem->SetActive(false);
        FogParticleSystem->SetActive(false);
        break;
        
    default:
        // Clear weather - disable all effects
        RainParticleSystem->SetActive(false);
        SnowParticleSystem->SetActive(false);
        FogParticleSystem->SetActive(false);
        SandstormParticleSystem->SetActive(false);
        break;
    }
}

void ADynamicWeatherSystem::UpdateAudioEffects()
{
    if (WeatherSounds.Contains(CurrentWeatherCondition.WeatherType))
    {
        USoundCue* WeatherSound = WeatherSounds[CurrentWeatherCondition.WeatherType];
        if (WeatherSound)
        {
            WeatherAudioComponent->SetSound(WeatherSound);
            WeatherAudioComponent->SetVolumeMultiplier(CurrentWeatherCondition.Intensity);
            
            if (!WeatherAudioComponent->IsPlaying())
            {
                WeatherAudioComponent->Play();
            }
        }
    }
    else
    {
        WeatherAudioComponent->Stop();
    }
}

void ADynamicWeatherSystem::CheckForHazardousConditions()
{
    static bool bWasHazardous = false;
    bool bIsCurrentlyHazardous = IsWeatherHazardous();
    
    if (bIsCurrentlyHazardous && !bWasHazardous)
    {
        OnHazardousWeatherAlert.Broadcast(CurrentWeatherCondition);
    }
    
    bWasHazardous = bIsCurrentlyHazardous;
}

FWeatherCondition ADynamicWeatherSystem::InterpolateWeatherConditions(const FWeatherCondition& A, const FWeatherCondition& B, float Alpha) const
{
    FWeatherCondition Result;
    
    Result.WeatherType = (Alpha < 0.5f) ? A.WeatherType : B.WeatherType;
    Result.Intensity = FMath::Lerp(A.Intensity, B.Intensity, Alpha);
    Result.Coverage = FMath::Lerp(A.Coverage, B.Coverage, Alpha);
    Result.WindDirection = FMath::Lerp(A.WindDirection, B.WindDirection, Alpha);
    Result.WindSpeed = FMath::Lerp(A.WindSpeed, B.WindSpeed, Alpha);
    Result.Temperature = FMath::Lerp(A.Temperature, B.Temperature, Alpha);
    Result.Humidity = FMath::Lerp(A.Humidity, B.Humidity, Alpha);
    Result.BarometricPressure = FMath::Lerp(A.BarometricPressure, B.BarometricPressure, Alpha);
    Result.VisibilityRange = FMath::Lerp(A.VisibilityRange, B.VisibilityRange, Alpha);
    
    return Result;
}

float ADynamicWeatherSystem::CalculateFrictionReduction() const
{
    float FrictionReduction = 0.0f;
    
    // Rain reduces friction
    if (CurrentWeatherCondition.WeatherType == EWeatherType::LightRain)
    {
        FrictionReduction += CurrentWeatherCondition.Intensity * 0.3f;
    }
    else if (CurrentWeatherCondition.WeatherType == EWeatherType::HeavyRain || 
             CurrentWeatherCondition.WeatherType == EWeatherType::Storm)
    {
        FrictionReduction += CurrentWeatherCondition.Intensity * 0.6f;
    }
    
    // Ice/snow reduces friction significantly
    if (CurrentWeatherCondition.WeatherType == EWeatherType::Snow ||
        CurrentWeatherCondition.WeatherType == EWeatherType::Blizzard)
    {
        FrictionReduction += CurrentWeatherCondition.Intensity * 0.7f;
    }
    
    // Freezing conditions create ice
    if (UWeatherCalculationLibrary::IsFreezingConditions(CurrentWeatherCondition.Temperature, CurrentWeatherCondition.Humidity))
    {
        FrictionReduction += 0.4f;
    }
    
    return FMath::Clamp(FrictionReduction, 0.0f, 0.8f);
}

float ADynamicWeatherSystem::CalculateStaminaPenalty() const
{
    float StaminaPenalty = 0.0f;
    
    // Wind increases energy expenditure
    StaminaPenalty += (CurrentWeatherCondition.WindSpeed - 10.0f) * 0.01f * CurrentGustMultiplier;
    
    // Extreme temperatures
    float ComfortableTemp = 70.0f;
    float TempDifference = FMath::Abs(CurrentWeatherCondition.Temperature - ComfortableTemp);
    if (TempDifference > 20.0f)
    {
        StaminaPenalty += (TempDifference - 20.0f) * 0.02f;
    }
    
    // Precipitation requires more energy
    if (CurrentWeatherCondition.WeatherType == EWeatherType::LightRain ||
        CurrentWeatherCondition.WeatherType == EWeatherType::HeavyRain ||
        CurrentWeatherCondition.WeatherType == EWeatherType::Snow)
    {
        StaminaPenalty += CurrentWeatherCondition.Intensity * 0.3f;
    }
    
    return FMath::Clamp(StaminaPenalty, 0.0f, 2.0f);
}

float ADynamicWeatherSystem::CalculateToolDifficulty() const
{
    float ToolDifficulty = 0.0f;
    
    // Wet conditions make tools harder to place
    if (CurrentWeatherCondition.WeatherType == EWeatherType::LightRain ||
        CurrentWeatherCondition.WeatherType == EWeatherType::HeavyRain)
    {
        ToolDifficulty += CurrentWeatherCondition.Intensity * 0.4f;
    }
    
    // Cold reduces dexterity
    if (CurrentWeatherCondition.Temperature < 40.0f)
    {
        ToolDifficulty += (40.0f - CurrentWeatherCondition.Temperature) * 0.02f;
    }
    
    // High winds make tools harder to manipulate
    if (CurrentWeatherCondition.WindSpeed > 20.0f)
    {
        ToolDifficulty += (CurrentWeatherCondition.WindSpeed - 20.0f) * 0.02f * CurrentGustMultiplier;
    }
    
    return FMath::Clamp(ToolDifficulty, 0.0f, 1.5f);
}

void ADynamicWeatherSystem::UpdateAutomaticWeatherProgression(float DeltaTime)
{
    WeatherProgressionTimer += DeltaTime;
    
    if (WeatherProgressionTimer >= CurrentWeatherDuration && !bIsTransitioning)
    {
        // Select next weather type
        EWeatherType NextWeather = SelectNextWeatherType();
        float NextIntensity = FMath::RandRange(0.3f, 0.8f);
        float TransitionTime = FMath::RandRange(120.0f, 600.0f); // 2-10 minutes
        
        TransitionToWeather(NextWeather, NextIntensity, TransitionTime);
        
        // Reset timer
        WeatherProgressionTimer = 0.0f;
        CurrentWeatherDuration = MinWeatherDuration + FMath::RandRange(0.0f, MaxWeatherDuration - MinWeatherDuration);
    }
}

EWeatherType ADynamicWeatherSystem::SelectNextWeatherType() const
{
    // Simplified weather progression logic
    // In a full implementation, this would consider current season, location, and weather patterns
    
    TArray<EWeatherType> PossibleWeatherTypes;
    
    // Always possible
    PossibleWeatherTypes.Add(EWeatherType::Clear);
    PossibleWeatherTypes.Add(EWeatherType::Cloudy);
    
    // Season and temperature dependent
    if (CurrentWeatherCondition.Temperature > 35.0f)
    {
        PossibleWeatherTypes.Add(EWeatherType::LightRain);
        if (CurrentWeatherCondition.Temperature > 45.0f)
        {
            PossibleWeatherTypes.Add(EWeatherType::HeavyRain);
        }
    }
    
    if (CurrentWeatherCondition.Temperature < 35.0f)
    {
        PossibleWeatherTypes.Add(EWeatherType::Snow);
    }
    
    // Wind and fog are always possibilities
    PossibleWeatherTypes.Add(EWeatherType::HighWind);
    PossibleWeatherTypes.Add(EWeatherType::Fog);
    
    // Storms are rare
    if (FMath::RandRange(0.0f, 1.0f) < 0.1f)
    {
        PossibleWeatherTypes.Add(EWeatherType::Storm);
    }
    
    return PossibleWeatherTypes[FMath::RandRange(0, PossibleWeatherTypes.Num() - 1)];
}

void ADynamicWeatherSystem::OnRep_CurrentWeatherCondition()
{
    UpdateVisualEffects();
    UpdateAudioEffects();
}

void ADynamicWeatherSystem::OnRep_TargetWeatherCondition()
{
    // Handle target weather condition replication if needed
}

// Weather Calculation Library Implementation

float UWeatherCalculationLibrary::CalculateWindChill(float Temperature, float WindSpeed)
{
    if (Temperature > 50.0f || WindSpeed < 3.0f)
    {
        return Temperature; // Wind chill formula not applicable
    }
    
    // Simplified wind chill formula (Fahrenheit)
    float WindChill = 35.74f + (0.6215f * Temperature) - (35.75f * FMath::Pow(WindSpeed, 0.16f)) + 
                      (0.4275f * Temperature * FMath::Pow(WindSpeed, 0.16f));
                      
    return FMath::Min(WindChill, Temperature);
}

float UWeatherCalculationLibrary::CalculateHeatIndex(float Temperature, float Humidity)
{
    if (Temperature < 80.0f)
    {
        return Temperature; // Heat index not applicable below 80°F
    }
    
    // Simplified heat index calculation
    float HI = -42.379f + (2.04901523f * Temperature) + (10.14333127f * Humidity) -
               (0.22475541f * Temperature * Humidity) - (0.00683783f * Temperature * Temperature) -
               (0.05481717f * Humidity * Humidity) + (0.00122874f * Temperature * Temperature * Humidity) +
               (0.00085282f * Temperature * Humidity * Humidity) - 
               (0.00000199f * Temperature * Temperature * Humidity * Humidity);
               
    return FMath::Max(HI, Temperature);
}

bool UWeatherCalculationLibrary::IsFreezingConditions(float Temperature, float Humidity)
{
    return Temperature <= 32.0f && Humidity > 30.0f;
}

ESeverityLevel UWeatherCalculationLibrary::DetermineWeatherSeverity(const FWeatherCondition& Condition)
{
    int32 SeverityPoints = 0;
    
    // Wind severity
    if (Condition.WindSpeed > 60.0f) SeverityPoints += 4;
    else if (Condition.WindSpeed > 40.0f) SeverityPoints += 3;
    else if (Condition.WindSpeed > 25.0f) SeverityPoints += 2;
    else if (Condition.WindSpeed > 15.0f) SeverityPoints += 1;
    
    // Precipitation severity
    if (Condition.Intensity > 0.8f) SeverityPoints += 3;
    else if (Condition.Intensity > 0.6f) SeverityPoints += 2;
    else if (Condition.Intensity > 0.3f) SeverityPoints += 1;
    
    // Visibility severity
    if (Condition.VisibilityRange < 1000.0f) SeverityPoints += 4;
    else if (Condition.VisibilityRange < 5000.0f) SeverityPoints += 3;
    else if (Condition.VisibilityRange < 15000.0f) SeverityPoints += 2;
    else if (Condition.VisibilityRange < 30000.0f) SeverityPoints += 1;
    
    // Temperature extremes
    if (Condition.Temperature < 10.0f || Condition.Temperature > 100.0f) SeverityPoints += 3;
    else if (Condition.Temperature < 25.0f || Condition.Temperature > 85.0f) SeverityPoints += 2;
    else if (Condition.Temperature < 35.0f || Condition.Temperature > 75.0f) SeverityPoints += 1;
    
    // Weather type severity
    switch (Condition.WeatherType)
    {
    case EWeatherType::Storm:
    case EWeatherType::Blizzard:
        SeverityPoints += 4;
        break;
    case EWeatherType::Sandstorm:
        SeverityPoints += 3;
        break;
    case EWeatherType::HeavyRain:
    case EWeatherType::Snow:
        SeverityPoints += 2;
        break;
    case EWeatherType::LightRain:
    case EWeatherType::Fog:
    case EWeatherType::HighWind:
        SeverityPoints += 1;
        break;
    default:
        break;
    }
    
    // Convert points to severity level
    if (SeverityPoints >= 12) return ESeverityLevel::Extreme;
    else if (SeverityPoints >= 9) return ESeverityLevel::Severe;
    else if (SeverityPoints >= 6) return ESeverityLevel::Heavy;
    else if (SeverityPoints >= 4) return ESeverityLevel::Moderate;
    else if (SeverityPoints >= 2) return ESeverityLevel::Light;
    else return ESeverityLevel::Minimal;
}

float UWeatherCalculationLibrary::ConvertTemperatureCelsiusToFahrenheit(float Celsius)
{
    return (Celsius * 9.0f / 5.0f) + 32.0f;
}

float UWeatherCalculationLibrary::ConvertTemperatureFahrenheitToCelsius(float Fahrenheit)
{
    return (Fahrenheit - 32.0f) * 5.0f / 9.0f;
}

FVector UWeatherCalculationLibrary::CalculateWindForceOnObject(const FWeatherCondition& Weather, float ObjectArea, float DragCoefficient)
{
    // Simplified wind force calculation: F = 0.5 * ρ * v² * Cd * A
    // Using air density ρ = 1.225 kg/m³ at sea level
    float AirDensity = 1.225f;
    float WindSpeedMS = Weather.WindSpeed * 0.44704f; // Convert mph to m/s
    
    float Force = 0.5f * AirDensity * WindSpeedMS * WindSpeedMS * DragCoefficient * ObjectArea;
    
    return Weather.WindDirection.GetSafeNormal() * Force;
}