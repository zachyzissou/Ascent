#include "BiomeHazardSystem.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

ABiomeHazardSystem::ABiomeHazardSystem()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;

    // Create biome zone component
    BiomeZone = CreateDefaultSubobject<UBoxComponent>(TEXT("BiomeZone"));
    RootComponent = BiomeZone;
    BiomeZone->SetBoxExtent(FVector(10000.0f, 10000.0f, 5000.0f)); // Large area coverage
    BiomeZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BiomeZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    BiomeZone->SetVisibility(false); // Only visible in editor

    // Create particle system for biome effects
    BiomeParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BiomeParticleSystem"));
    BiomeParticleSystem->SetupAttachment(RootComponent);
    BiomeParticleSystem->SetAutoActivate(false);

    // Create audio component for ambient sounds
    BiomeAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BiomeAudioComponent"));
    BiomeAudioComponent->SetupAttachment(RootComponent);
    BiomeAudioComponent->bAutoActivate = false);

    // Initialize default biome conditions (Temperate)
    CurrentBiomeConditions.BaseTemperature = 15.0f;
    CurrentBiomeConditions.Elevation = 1000.0f;
    CurrentBiomeConditions.Humidity = 60.0f;
    CurrentBiomeConditions.AnnualPrecipitation = 800.0f;
    CurrentBiomeConditions.AverageWindSpeed = 15.0f;
    CurrentBiomeConditions.DaylightHours = 12.0f;
    CurrentBiomeConditions.UVIndex = 5.0f;

    // Initialize biome presets
    InitializeBiomePresets();
    
    // Initialize seasonal modifiers
    InitializeSeasonalModifiers();
    
    // Initialize hazard probabilities
    InitializeHazardProbabilities();

    // Set default season duration (2 hours)
    SeasonDuration = 7200.0f;
    SeasonProgress = 0.0f;
    CurrentSeason = ESeason::Summer;
}

void ABiomeHazardSystem::BeginPlay()
{
    Super::BeginPlay();

    // Apply initial biome configuration
    SetBiomeType(CurrentBiomeType);
    
    // Update biome effects
    UpdateBiomeEffects();

    // Find weather system in world
    TArray<AActor*> WeatherSystems;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADynamicWeatherSystem::StaticClass(), WeatherSystems);
    if (WeatherSystems.Num() > 0)
    {
        WeatherSystem = Cast<ADynamicWeatherSystem>(WeatherSystems[0]);
    }

    // Start seasonal progression timer if enabled
    if (bSeasonalProgressionEnabled && HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_SeasonalProgression,
            [this]() { UpdateSeasonalProgression(0.1f); },
            0.1f,
            true
        );
    }

    // Start hazard monitoring timer
    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_HazardCheck,
            [this]() { UpdateHazardEvents(1.0f); },
            1.0f,
            true
        );
    }
}

void ABiomeHazardSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    Super::EndPlay(EndPlayReason);
}

void ABiomeHazardSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABiomeHazardSystem, CurrentBiomeType);
    DOREPLIFETIME(ABiomeHazardSystem, CurrentBiomeConditions);
    DOREPLIFETIME(ABiomeHazardSystem, CurrentSeason);
}

void ABiomeHazardSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ProcessActiveHazards(DeltaTime);
}

void ABiomeHazardSystem::SetBiomeType(EBiomeType NewBiomeType)
{
    if (!HasAuthority()) return;

    EBiomeType OldBiomeType = CurrentBiomeType;
    CurrentBiomeType = NewBiomeType;

    // Apply biome preset conditions
    if (BiomePresets.Contains(NewBiomeType))
    {
        CurrentBiomeConditions = BiomePresets[NewBiomeType];
        ApplySeasonalModifiers();
        
        OnBiomeConditionsChanged.Broadcast(CurrentBiomeType, CurrentBiomeConditions);
    }

    UpdateBiomeEffects();
    
    // Initialize biome-specific settings
    switch (NewBiomeType)
    {
    case EBiomeType::Desert:
        InitializeDesertBiome();
        break;
    case EBiomeType::Alpine:
        InitializeAlpineBiome();
        break;
    case EBiomeType::Coastal:
        InitializeCoastalBiome();
        break;
    case EBiomeType::Arctic:
        InitializeArcticBiome();
        break;
    case EBiomeType::Tropical:
        InitializeTropicalBiome();
        break;
    case EBiomeType::Volcanic:
        InitializeVolcanicBiome();
        break;
    case EBiomeType::Cave:
        InitializeCaveBiome();
        break;
    default:
        InitializeTemperateBiome();
        break;
    }
}

void ABiomeHazardSystem::UpdateBiomeConditions(const FBiomeConditions& NewConditions)
{
    if (!HasAuthority()) return;

    CurrentBiomeConditions = NewConditions;
    OnBiomeConditionsChanged.Broadcast(CurrentBiomeType, CurrentBiomeConditions);
}

void ABiomeHazardSystem::SetSeason(ESeason NewSeason)
{
    if (!HasAuthority()) return;

    ESeason OldSeason = CurrentSeason;
    CurrentSeason = NewSeason;
    SeasonProgress = 0.0f;

    ApplySeasonalModifiers();
    OnSeasonChanged.Broadcast(NewSeason);

    UE_LOG(LogTemp, Log, TEXT("Season changed to %d"), (int32)NewSeason);
}

void ABiomeHazardSystem::AdvanceSeason()
{
    ESeason NextSeason;
    switch (CurrentSeason)
    {
    case ESeason::Spring:
        NextSeason = ESeason::Summer;
        break;
    case ESeason::Summer:
        NextSeason = ESeason::Fall;
        break;
    case ESeason::Fall:
        NextSeason = ESeason::Winter;
        break;
    case ESeason::Winter:
    default:
        NextSeason = ESeason::Spring;
        break;
    }
    
    SetSeason(NextSeason);
}

void ABiomeHazardSystem::SetSeasonalProgression(bool bEnable, float NewSeasonDuration)
{
    bSeasonalProgressionEnabled = bEnable;
    SeasonDuration = FMath::Max(NewSeasonDuration, 60.0f); // Minimum 1 minute
    
    if (bEnable && HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_SeasonalProgression,
            [this]() { UpdateSeasonalProgression(0.1f); },
            0.1f,
            true
        );
    }
    else
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_SeasonalProgression);
    }
}

void ABiomeHazardSystem::TriggerHazardEvent(const FHazardEvent& HazardEvent)
{
    if (!HasAuthority()) return;

    // Add to active hazards
    ActiveHazardEvents.Add(HazardEvent);

    // Broadcast hazard event
    OnBiomeHazardTriggered.Broadcast(HazardEvent);

    // Play hazard sound if available
    if (HazardSounds.Contains(HazardEvent.HazardType))
    {
        USoundCue* HazardSound = HazardSounds[HazardEvent.HazardType];
        if (HazardSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), HazardSound, HazardEvent.Location);
        }
    }

    // Set timer to remove hazard after duration
    FTimerHandle HazardTimer;
    GetWorldTimerManager().SetTimer(
        HazardTimer,
        [this, HazardEvent]()
        {
            ActiveHazardEvents.RemoveAll([HazardEvent](const FHazardEvent& Event)
            {
                return Event.HazardType == HazardEvent.HazardType && 
                       FVector::Dist(Event.Location, HazardEvent.Location) < 100.0f;
            });
        },
        HazardEvent.Duration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Hazard triggered: %d at location %s"), 
           (int32)HazardEvent.HazardType, *HazardEvent.Location.ToString());
}

bool ABiomeHazardSystem::IsHazardActiveInSeason(EBiomeHazardType HazardType, ESeason Season) const
{
    if (!SeasonalModifiers.Contains(Season)) return false;

    const FSeasonalModifiers& Modifiers = SeasonalModifiers[Season];
    return Modifiers.ActiveHazards.Contains(HazardType) && 
           !Modifiers.InactiveHazards.Contains(HazardType);
}

TArray<EBiomeHazardType> ABiomeHazardSystem::GetActiveHazards() const
{
    TArray<EBiomeHazardType> ActiveHazards;

    // Get base biome hazards
    ActiveHazards.Append(CurrentBiomeConditions.CommonHazards);

    // Add seasonal hazards
    if (SeasonalModifiers.Contains(CurrentSeason))
    {
        const FSeasonalModifiers& Modifiers = SeasonalModifiers[CurrentSeason];
        ActiveHazards.Append(Modifiers.ActiveHazards);
        
        // Remove inactive hazards
        for (EBiomeHazardType InactiveHazard : Modifiers.InactiveHazards)
        {
            ActiveHazards.Remove(InactiveHazard);
        }
    }

    return ActiveHazards;
}

float ABiomeHazardSystem::GetHazardProbability(EBiomeHazardType HazardType) const
{
    if (HazardProbabilities.Contains(HazardType))
    {
        return HazardProbabilities[HazardType];
    }
    return 0.0f;
}

void ABiomeHazardSystem::SetHazardProbability(EBiomeHazardType HazardType, float Probability)
{
    HazardProbabilities.Add(HazardType, FMath::Clamp(Probability, 0.0f, 1.0f));
}

bool ABiomeHazardSystem::IsActorAffectedByHazard(AActor* Actor, EBiomeHazardType HazardType) const
{
    if (!Actor) return false;

    // Check if actor is within range of any active hazard of this type
    for (const FHazardEvent& HazardEvent : ActiveHazardEvents)
    {
        if (HazardEvent.HazardType == HazardType)
        {
            float Distance = FVector::Dist(Actor->GetActorLocation(), HazardEvent.Location);
            if (Distance <= HazardEvent.EffectRadius)
            {
                return true;
            }
        }
    }

    return false;
}

float ABiomeHazardSystem::CalculateEnvironmentalStress(AActor* Actor) const
{
    if (!Actor) return 0.0f;

    float StressLevel = 0.0f;
    FVector ActorLocation = Actor->GetActorLocation();

    // Temperature stress
    float EffectiveTemp = CurrentBiomeConditions.BaseTemperature;
    if (WeatherSystem)
    {
        FWeatherCondition Weather = WeatherSystem->GetCurrentWeatherCondition();
        EffectiveTemp = UWeatherCalculationLibrary::ConvertTemperatureFahrenheitToCelsius(Weather.Temperature);
    }

    // Heat stress
    if (EffectiveTemp > 35.0f)
    {
        StressLevel += (EffectiveTemp - 35.0f) * 0.1f;
    }
    // Cold stress
    else if (EffectiveTemp < 0.0f)
    {
        StressLevel += FMath::Abs(EffectiveTemp) * 0.05f;
    }

    // Altitude stress
    if (CurrentBiomeConditions.Elevation > 2500.0f)
    {
        float AltitudeStress = (CurrentBiomeConditions.Elevation - 2500.0f) / 1000.0f;
        StressLevel += AltitudeStress * 0.2f;
    }

    // Hazard-specific stress
    for (const FHazardEvent& HazardEvent : ActiveHazardEvents)
    {
        float Distance = FVector::Dist(ActorLocation, HazardEvent.Location);
        if (Distance <= HazardEvent.EffectRadius)
        {
            float ProximityFactor = 1.0f - (Distance / HazardEvent.EffectRadius);
            StressLevel += HazardEvent.Severity * ProximityFactor * 0.5f;
        }
    }

    return FMath::Clamp(StressLevel, 0.0f, 2.0f);
}

TArray<FString> ABiomeHazardSystem::GetSurvivalRecommendations() const
{
    TArray<FString> Recommendations;

    // Biome-specific recommendations
    switch (CurrentBiomeType)
    {
    case EBiomeType::Desert:
        Recommendations.Add(TEXT("Carry extra water - minimum 4L per day"));
        Recommendations.Add(TEXT("Wear light-colored, loose-fitting clothing"));
        Recommendations.Add(TEXT("Climb during cooler morning/evening hours"));
        Recommendations.Add(TEXT("Seek shade during midday heat"));
        break;

    case EBiomeType::Alpine:
        Recommendations.Add(TEXT("Acclimatize gradually to prevent altitude sickness"));
        Recommendations.Add(TEXT("Layer clothing for temperature regulation"));
        Recommendations.Add(TEXT("Carry emergency shelter and insulation"));
        Recommendations.Add(TEXT("Monitor weather conditions constantly"));
        break;

    case EBiomeType::Coastal:
        Recommendations.Add(TEXT("Check tide tables before climbing"));
        Recommendations.Add(TEXT("Have escape routes above high tide"));
        Recommendations.Add(TEXT("Protect gear from salt spray corrosion"));
        Recommendations.Add(TEXT("Be aware of marine weather patterns"));
        break;

    case EBiomeType::Arctic:
        Recommendations.Add(TEXT("Prevent hypothermia with proper insulation"));
        Recommendations.Add(TEXT("Stay dry to maintain body heat"));
        Recommendations.Add(TEXT("Carry emergency heat sources"));
        Recommendations.Add(TEXT("Plan for extremely short daylight hours"));
        break;
    }

    // Seasonal recommendations
    switch (CurrentSeason)
    {
    case ESeason::Winter:
        Recommendations.Add(TEXT("Check avalanche bulletins in snow country"));
        Recommendations.Add(TEXT("Carry winter emergency equipment"));
        break;
    case ESeason::Summer:
        Recommendations.Add(TEXT("Start early to avoid afternoon heat"));
        Recommendations.Add(TEXT("Increase UV protection at altitude"));
        break;
    case ESeason::Spring:
        Recommendations.Add(TEXT("Be aware of increased rockfall from freeze-thaw"));
        Recommendations.Add(TEXT("Check for snowpack instability"));
        break;
    case ESeason::Fall:
        Recommendations.Add(TEXT("Prepare for rapid weather changes"));
        Recommendations.Add(TEXT("Plan for shorter daylight hours"));
        break;
    }

    return Recommendations;
}

FString ABiomeHazardSystem::GetBiomeDescription() const
{
    FString Description;

    switch (CurrentBiomeType)
    {
    case EBiomeType::Desert:
        Description = TEXT("Arid desert environment with extreme temperature variations, limited water sources, and specialized flora adapted to harsh conditions. Climbing challenges include heat stress, dehydration, and flash flood risks.");
        break;
    case EBiomeType::Alpine:
        Description = TEXT("High-altitude mountain environment characterized by thin air, extreme weather, and challenging terrain. Altitude sickness, hypothermia, and avalanche risks are primary concerns.");
        break;
    case EBiomeType::Coastal:
        Description = TEXT("Maritime cliff environment influenced by tidal action, salt spray, and marine weather patterns. Unique challenges include tide timing, corrosive conditions, and wave action.");
        break;
    case EBiomeType::Temperate:
        Description = TEXT("Moderate climate environment with seasonal variations. Generally stable conditions but subject to typical weather patterns and seasonal hazards.");
        break;
    case EBiomeType::Arctic:
        Description = TEXT("Extreme cold environment with permafrost, limited daylight, and severe weather conditions. Hypothermia and frostbite are constant threats.");
        break;
    case EBiomeType::Tropical:
        Description = TEXT("Hot, humid environment with intense rainfall patterns and diverse wildlife. Heat stress, tropical diseases, and sudden weather changes are key concerns.");
        break;
    case EBiomeType::Volcanic:
        Description = TEXT("Geologically active environment with potential for seismic activity, volcanic gases, and unstable terrain. Unique rock formations but elevated geological risks.");
        break;
    case EBiomeType::Cave:
        Description = TEXT("Underground environment with constant temperature, high humidity, and unique navigation challenges. Hypothermia, getting lost, and air quality are primary concerns.");
        break;
    }

    return Description;
}

TArray<FString> ABiomeHazardSystem::GetSeasonalSafetyTips() const
{
    TArray<FString> SafetyTips = UBiomeHazardLibrary::GetSeasonalSafetyTips(CurrentSeason, CurrentBiomeType);

    // Add current hazard-specific tips
    TArray<EBiomeHazardType> ActiveHazards = GetActiveHazards();
    for (EBiomeHazardType HazardType : ActiveHazards)
    {
        FString HazardTip = UBiomeHazardLibrary::GetHazardPreventionAdvice(HazardType);
        if (!HazardTip.IsEmpty())
        {
            SafetyTips.Add(HazardTip);
        }
    }

    return SafetyTips;
}

FString ABiomeHazardSystem::GetHazardEducationalInfo(EBiomeHazardType HazardType) const
{
    FString EducationalInfo;

    switch (HazardType)
    {
    case EBiomeHazardType::HeatStroke:
        EducationalInfo = TEXT("Heat stroke occurs when the body's temperature regulation fails. Symptoms include confusion, hot dry skin, and rapid pulse. Prevention: stay hydrated, take breaks in shade, wear appropriate clothing.");
        break;
    case EBiomeHazardType::AltitudeSickness:
        EducationalInfo = TEXT("Altitude sickness results from reduced oxygen at elevation. Symptoms include headache, nausea, and fatigue. Prevention: gradual ascent, adequate hydration, descent if symptoms worsen.");
        break;
    case EBiomeHazardType::Hypothermia:
        EducationalInfo = TEXT("Hypothermia occurs when core body temperature drops dangerously low. Symptoms progress from shivering to confusion to unconsciousness. Prevention: stay dry, layer clothing, maintain caloric intake.");
        break;
    case EBiomeHazardType::Dehydration:
        EducationalInfo = TEXT("Dehydration occurs when fluid loss exceeds intake. Symptoms include thirst, dark urine, and decreased performance. Prevention: drink regularly before thirst, monitor urine color.");
        break;
    case EBiomeHazardType::Lightning:
        EducationalInfo = TEXT("Lightning strikes are a serious threat in exposed terrain. Risk increases with elevation and metal objects. Prevention: monitor weather, seek shelter, avoid peaks during storms.");
        break;
    default:
        EducationalInfo = TEXT("Consult survival guides and local expertise for detailed information about this environmental hazard.");
        break;
    }

    return EducationalInfo;
}

void ABiomeHazardSystem::ProcessHeatStroke(AActor* Actor)
{
    if (!Actor) return;

    // Check environmental conditions
    float EffectiveTemp = CurrentBiomeConditions.BaseTemperature;
    if (WeatherSystem)
    {
        FWeatherCondition Weather = WeatherSystem->GetCurrentWeatherCondition();
        EffectiveTemp = UWeatherCalculationLibrary::ConvertTemperatureFahrenheitToCelsius(Weather.Temperature);
    }

    float HeatIndex = UBiomeHazardLibrary::CalculateHeatIndex(EffectiveTemp, CurrentBiomeConditions.Humidity);
    
    if (UBiomeHazardLibrary::IsHeatStrokeThreat(EffectiveTemp, CurrentBiomeConditions.Humidity, 1.0f))
    {
        // Apply heat stroke effects
        float Damage = FMath::Clamp((HeatIndex - 35.0f) * 2.0f, 0.0f, 50.0f);
        
        UGameplayStatics::ApplyDamage(
            Actor,
            Damage,
            nullptr,
            this,
            UDamageType::StaticClass()
        );

        OnEnvironmentalDamage.Broadcast(Actor, EBiomeHazardType::HeatStroke);
        
        UE_LOG(LogTemp, Warning, TEXT("Heat stroke damage applied to %s: %.1f"), 
               *Actor->GetName(), Damage);
    }
}

void ABiomeHazardSystem::ProcessAltitudeSickness(AActor* Actor)
{
    if (!Actor) return;

    float OxygenLevel = UBiomeHazardLibrary::CalculateOxygenLevel(CurrentBiomeConditions.Elevation);
    
    if (OxygenLevel < 0.7f) // Below 70% oxygen
    {
        float Severity = 1.0f - OxygenLevel;
        float Damage = Severity * 20.0f; // Max 20 damage at sea level equivalent
        
        UGameplayStatics::ApplyDamage(
            Actor,
            Damage,
            nullptr,
            this,
            UDamageType::StaticClass()
        );

        OnEnvironmentalDamage.Broadcast(Actor, EBiomeHazardType::AltitudeSickness);
        
        UE_LOG(LogTemp, Warning, TEXT("Altitude sickness affecting %s at %.0fm elevation"), 
               *Actor->GetName(), CurrentBiomeConditions.Elevation);
    }
}

void ABiomeHazardSystem::ProcessTidalEffects(const FVector& Location)
{
    if (CurrentBiomeType != EBiomeType::Coastal) return;

    // Simplified tidal simulation
    float GameTime = GetWorld()->GetTimeSeconds();
    float TidalCycle = 12.42f * 3600.0f; // 12.42 hours in seconds (simplified)
    float TidalPhase = FMath::Fmod(GameTime, TidalCycle) / TidalCycle;
    
    // Calculate tide height (-2m to +2m range)
    float TideHeight = FMath::Sin(TidalPhase * 2 * PI) * 200.0f; // ±2m in Unreal units
    
    // Check if location is affected by current tide
    if (Location.Z < TideHeight + 100.0f) // 1m safety margin
    {
        FHazardEvent TidalHazard;
        TidalHazard.HazardType = EBiomeHazardType::TidalSurge;
        TidalHazard.Location = Location;
        TidalHazard.EffectRadius = 1000.0f;
        TidalHazard.Severity = FMath::Clamp((TideHeight + 100.0f - Location.Z) / 300.0f, 0.0f, 1.0f);
        TidalHazard.Duration = 30.0f;
        TidalHazard.bRequiresEvacuation = TidalHazard.Severity > 0.7f;

        TriggerHazardEvent(TidalHazard);
    }
}

void ABiomeHazardSystem::UpdateSeasonalProgression(float DeltaTime)
{
    if (!bSeasonalProgressionEnabled) return;

    SeasonTimer += DeltaTime;
    SeasonProgress = SeasonTimer / SeasonDuration;

    if (SeasonProgress >= 1.0f)
    {
        AdvanceSeason();
        SeasonTimer = 0.0f;
        SeasonProgress = 0.0f;
    }
}

void ABiomeHazardSystem::UpdateHazardEvents(float DeltaTime)
{
    HazardCheckTimer += DeltaTime;

    if (HazardCheckTimer >= 1.0f) // Check every second
    {
        CheckForNewHazards(DeltaTime);
        CleanupExpiredHazards();
        HazardCheckTimer = 0.0f;
    }
}

void ABiomeHazardSystem::UpdateBiomeEffects()
{
    // Update particle effects based on biome
    if (BiomeParticleSystem)
    {
        // Set appropriate particle system for biome type
        // Implementation would depend on available particle assets
    }

    // Update ambient audio
    if (BiomeAmbientSounds.Contains(CurrentBiomeType) && BiomeAudioComponent)
    {
        USoundCue* AmbientSound = BiomeAmbientSounds[CurrentBiomeType];
        if (AmbientSound)
        {
            BiomeAudioComponent->SetSound(AmbientSound);
            if (!BiomeAudioComponent->IsPlaying())
            {
                BiomeAudioComponent->Play();
            }
        }
    }
}

void ABiomeHazardSystem::ApplySeasonalModifiers()
{
    if (!SeasonalModifiers.Contains(CurrentSeason)) return;

    const FSeasonalModifiers& Modifiers = SeasonalModifiers[CurrentSeason];
    
    // Apply modifiers to current conditions
    FBiomeConditions ModifiedConditions = ApplySeasonalModifiers(
        BiomePresets.Contains(CurrentBiomeType) ? BiomePresets[CurrentBiomeType] : CurrentBiomeConditions,
        Modifiers
    );

    CurrentBiomeConditions = ModifiedConditions;

    // Update weather system if available
    if (WeatherSystem)
    {
        WeatherSystem->SetSeasonalModifiers(
            Modifiers.TemperatureModifier,
            Modifiers.PrecipitationMultiplier,
            Modifiers.WindMultiplier
        );
    }
}

void ABiomeHazardSystem::ProcessActiveHazards(float DeltaTime)
{
    // Process ongoing hazard effects
    TArray<AActor*> AffectedActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), AffectedActors);

    for (AActor* Actor : AffectedActors)
    {
        if (!Actor) continue;

        // Check for heat stroke in hot conditions
        if (CurrentBiomeType == EBiomeType::Desert || 
            (CurrentBiomeConditions.BaseTemperature > 35.0f && CurrentSeason == ESeason::Summer))
        {
            ProcessHeatStroke(Actor);
        }

        // Check for altitude sickness in high elevations
        if (CurrentBiomeType == EBiomeType::Alpine && CurrentBiomeConditions.Elevation > 2500.0f)
        {
            ProcessAltitudeSickness(Actor);
        }
    }
}

void ABiomeHazardSystem::CheckForNewHazards(float DeltaTime)
{
    TArray<EBiomeHazardType> ActiveHazards = GetActiveHazards();
    
    for (EBiomeHazardType HazardType : ActiveHazards)
    {
        float Probability = GetHazardProbability(HazardType);
        
        // Convert hourly probability to per-second
        float SecondlyProbability = Probability / 3600.0f;
        
        if (FMath::RandRange(0.0f, 1.0f) < SecondlyProbability)
        {
            // Trigger random hazard event
            FHazardEvent RandomHazard;
            RandomHazard.HazardType = HazardType;
            RandomHazard.Severity = FMath::RandRange(0.3f, 0.8f);
            RandomHazard.Location = GetActorLocation() + FVector(
                FMath::RandRange(-5000.0f, 5000.0f),
                FMath::RandRange(-5000.0f, 5000.0f),
                0.0f
            );
            RandomHazard.EffectRadius = FMath::RandRange(500.0f, 2000.0f);
            RandomHazard.Duration = FMath::RandRange(60.0f, 300.0f);
            RandomHazard.WarningTime = 30.0f;

            TriggerHazardEvent(RandomHazard);
        }
    }
}

void ABiomeHazardSystem::CleanupExpiredHazards()
{
    // Remove hazards that have exceeded their duration
    // This is handled by the individual hazard timers set in TriggerHazardEvent
    // Additional cleanup could be added here if needed
}

// Initialize biome-specific configurations
void ABiomeHazardSystem::InitializeDesertBiome()
{
    // Set desert-specific hazard probabilities
    SetHazardProbability(EBiomeHazardType::HeatStroke, 0.1f);
    SetHazardProbability(EBiomeHazardType::Dehydration, 0.2f);
    SetHazardProbability(EBiomeHazardType::Sandstorm, 0.05f);
    SetHazardProbability(EBiomeHazardType::FlashFlood, 0.02f);
}

void ABiomeHazardSystem::InitializeAlpineBiome()
{
    SetHazardProbability(EBiomeHazardType::AltitudeSickness, 0.15f);
    SetHazardProbability(EBiomeHazardType::Hypothermia, 0.1f);
    SetHazardProbability(EBiomeHazardType::Frostbite, 0.08f);
    SetHazardProbability(EBiomeHazardType::WhiteOut, 0.05f);
}

void ABiomeHazardSystem::InitializeCoastalBiome()
{
    SetHazardProbability(EBiomeHazardType::TidalSurge, 0.3f); // High frequency due to tidal cycles
    SetHazardProbability(EBiomeHazardType::WaveAction, 0.2f);
    SetHazardProbability(EBiomeHazardType::MarineFog, 0.1f);
    SetHazardProbability(EBiomeHazardType::StormSurge, 0.03f);
}

void ABiomeHazardSystem::InitializeTemperateBiome()
{
    // Moderate hazard probabilities for temperate climate
    SetHazardProbability(EBiomeHazardType::Lightning, 0.05f);
    SetHazardProbability(EBiomeHazardType::FloodingRain, 0.03f);
}

void ABiomeHazardSystem::InitializeArcticBiome()
{
    SetHazardProbability(EBiomeHazardType::Hypothermia, 0.25f);
    SetHazardProbability(EBiomeHazardType::Frostbite, 0.2f);
    SetHazardProbability(EBiomeHazardType::WhiteOut, 0.15f);
}

void ABiomeHazardSystem::InitializeTropicalBiome()
{
    SetHazardProbability(EBiomeHazardType::HeatStroke, 0.08f);
    SetHazardProbability(EBiomeHazardType::FloodingRain, 0.1f);
    SetHazardProbability(EBiomeHazardType::Lightning, 0.08f);
}

void ABiomeHazardSystem::InitializeVolcanicBiome()
{
    SetHazardProbability(EBiomeHazardType::VolcanicActivity, 0.02f);
    SetHazardProbability(EBiomeHazardType::ToxicGas, 0.05f);
    SetHazardProbability(EBiomeHazardType::Earthquake, 0.03f);
}

void ABiomeHazardSystem::InitializeCaveBiome()
{
    SetHazardProbability(EBiomeHazardType::Hypothermia, 0.1f);
    SetHazardProbability(EBiomeHazardType::ToxicGas, 0.03f);
}

FBiomeConditions ABiomeHazardSystem::ApplySeasonalModifiers(const FBiomeConditions& BaseConditions, const FSeasonalModifiers& Modifiers) const
{
    FBiomeConditions ModifiedConditions = BaseConditions;
    
    ModifiedConditions.BaseTemperature += Modifiers.TemperatureModifier;
    ModifiedConditions.AnnualPrecipitation *= Modifiers.PrecipitationMultiplier;
    ModifiedConditions.AverageWindSpeed *= Modifiers.WindMultiplier;
    ModifiedConditions.DaylightHours += Modifiers.DaylightModifier;
    
    // Update hazard lists
    ModifiedConditions.SeasonalHazards = Modifiers.ActiveHazards;
    
    return ModifiedConditions;
}

void ABiomeHazardSystem::OnRep_CurrentBiomeType()
{
    UpdateBiomeEffects();
}

void ABiomeHazardSystem::OnRep_CurrentSeason()
{
    ApplySeasonalModifiers();
    UpdateBiomeEffects();
}

// Biome Hazard Library Implementation

float UBiomeHazardLibrary::CalculateHeatIndex(float Temperature, float Humidity)
{
    if (Temperature < 26.7f) return Temperature; // Heat index not applicable below 80°F

    // Heat index calculation for Celsius
    float T = Temperature;
    float H = Humidity;
    
    float HI = -8.78469475556f + 1.61139411f * T + 2.33854883889f * H + 
               (-0.14611605f * T * H) + (-0.012308094f * T * T) + 
               (-0.0164248277778f * H * H) + (0.002211732f * T * T * H) + 
               (0.00072546f * T * H * H) + (-0.000003582f * T * T * H * H);
               
    return FMath::Max(HI, Temperature);
}

float UBiomeHazardLibrary::CalculateWindChill(float Temperature, float WindSpeed)
{
    if (Temperature > 10.0f || WindSpeed < 1.3f) return Temperature;
    
    // Wind chill calculation for Celsius and km/h
    float WC = 13.12f + 0.6215f * Temperature - 11.37f * FMath::Pow(WindSpeed, 0.16f) + 
               0.3965f * Temperature * FMath::Pow(WindSpeed, 0.16f);
               
    return FMath::Min(WC, Temperature);
}

float UBiomeHazardLibrary::CalculateAltitudePressure(float Elevation)
{
    // Barometric pressure decreases with altitude
    float SeaLevelPressure = 1013.25f; // hPa
    float PressureRatio = FMath::Pow(1.0f - (0.0065f * Elevation / 288.15f), 5.255f);
    return SeaLevelPressure * PressureRatio;
}

float UBiomeHazardLibrary::CalculateOxygenLevel(float Elevation)
{
    // Oxygen availability decreases with altitude
    float PressureRatio = CalculateAltitudePressure(Elevation) / 1013.25f;
    return PressureRatio; // Simplified - oxygen percentage follows pressure
}

bool UBiomeHazardLibrary::IsHypothermiaThreat(float Temperature, float WindSpeed, float Humidity, float ExposureTime)
{
    float WindChill = CalculateWindChill(Temperature, WindSpeed);
    
    // Risk increases with lower temperature, higher wind, and longer exposure
    float RiskScore = 0.0f;
    
    if (WindChill < 0.0f) RiskScore += 2.0f;
    else if (WindChill < 10.0f) RiskScore += 1.0f;
    
    if (Humidity > 80.0f) RiskScore += 0.5f; // Wet conditions increase risk
    
    if (ExposureTime > 3600.0f) RiskScore += 1.0f; // >1 hour exposure
    
    return RiskScore >= 2.0f;
}

bool UBiomeHazardLibrary::IsHeatStrokeThreat(float Temperature, float Humidity, float ExertionLevel)
{
    float HeatIndex = CalculateHeatIndex(Temperature, Humidity);
    
    // Adjust for exertion level
    float AdjustedHeatIndex = HeatIndex + (ExertionLevel * 5.0f);
    
    return AdjustedHeatIndex > 40.0f; // 104°F danger threshold
}

float UBiomeHazardLibrary::CalculateDehydrationRate(float Temperature, float Humidity, float ActivityLevel)
{
    // Base fluid loss rate in L/hour
    float BaseRate = 0.5f;
    
    // Temperature adjustment
    if (Temperature > 25.0f)
    {
        BaseRate += (Temperature - 25.0f) * 0.1f;
    }
    
    // Activity level adjustment (0.0 = rest, 1.0 = heavy exercise)
    BaseRate += ActivityLevel * 1.0f;
    
    // Humidity adjustment (lower humidity increases rate)
    float HumidityFactor = 1.0f + (1.0f - Humidity / 100.0f) * 0.5f;
    BaseRate *= HumidityFactor;
    
    return FMath::Max(BaseRate, 0.2f);
}

float UBiomeHazardLibrary::GetSeasonalDaylightHours(float Latitude, ESeason Season)
{
    // Simplified daylight calculation
    float BaseHours = 12.0f;
    float LatitudeEffect = FMath::Abs(Latitude) / 90.0f * 6.0f;
    
    switch (Season)
    {
    case ESeason::Summer:
        return Latitude > 0 ? BaseHours + LatitudeEffect : BaseHours - LatitudeEffect;
    case ESeason::Winter:
        return Latitude > 0 ? BaseHours - LatitudeEffect : BaseHours + LatitudeEffect;
    case ESeason::Spring:
    case ESeason::Fall:
    default:
        return BaseHours;
    }
}

float UBiomeHazardLibrary::CalculateUVExposure(float Elevation, float Latitude, ESeason Season, float CloudCover)
{
    float BaseUV = 5.0f;
    
    // Elevation increases UV (4-5% per 300m)
    BaseUV += (Elevation / 300.0f) * 0.25f;
    
    // Latitude effect
    BaseUV *= (1.0f - FMath::Abs(Latitude) / 90.0f * 0.3f);
    
    // Seasonal variation
    switch (Season)
    {
    case ESeason::Summer:
        BaseUV *= 1.3f;
        break;
    case ESeason::Winter:
        BaseUV *= 0.7f;
        break;
    case ESeason::Spring:
        BaseUV *= 1.1f;
        break;
    case ESeason::Fall:
        BaseUV *= 0.9f;
        break;
    }
    
    // Cloud cover reduction
    BaseUV *= (1.0f - CloudCover * 0.6f);
    
    return FMath::Max(BaseUV, 1.0f);
}

TArray<EBiomeHazardType> UBiomeHazardLibrary::GetBiomeTypicalHazards(EBiomeType BiomeType)
{
    TArray<EBiomeHazardType> TypicalHazards;
    
    switch (BiomeType)
    {
    case EBiomeType::Desert:
        TypicalHazards.Add(EBiomeHazardType::HeatStroke);
        TypicalHazards.Add(EBiomeHazardType::Dehydration);
        TypicalHazards.Add(EBiomeHazardType::Sandstorm);
        TypicalHazards.Add(EBiomeHazardType::FlashFlood);
        break;
    case EBiomeType::Alpine:
        TypicalHazards.Add(EBiomeHazardType::AltitudeSickness);
        TypicalHazards.Add(EBiomeHazardType::Hypothermia);
        TypicalHazards.Add(EBiomeHazardType::Frostbite);
        TypicalHazards.Add(EBiomeHazardType::WhiteOut);
        break;
    case EBiomeType::Coastal:
        TypicalHazards.Add(EBiomeHazardType::TidalSurge);
        TypicalHazards.Add(EBiomeHazardType::WaveAction);
        TypicalHazards.Add(EBiomeHazardType::MarineFog);
        break;
    case EBiomeType::Arctic:
        TypicalHazards.Add(EBiomeHazardType::Hypothermia);
        TypicalHazards.Add(EBiomeHazardType::Frostbite);
        TypicalHazards.Add(EBiomeHazardType::WhiteOut);
        break;
    case EBiomeType::Tropical:
        TypicalHazards.Add(EBiomeHazardType::HeatStroke);
        TypicalHazards.Add(EBiomeHazardType::FloodingRain);
        TypicalHazards.Add(EBiomeHazardType::Lightning);
        break;
    }
    
    return TypicalHazards;
}

FString UBiomeHazardLibrary::GetHazardPreventionAdvice(EBiomeHazardType HazardType)
{
    switch (HazardType)
    {
    case EBiomeHazardType::HeatStroke:
        return TEXT("Stay hydrated, wear light colors, climb during cooler hours, take frequent shade breaks");
    case EBiomeHazardType::AltitudeSickness:
        return TEXT("Ascend gradually, stay hydrated, descend if symptoms worsen, consider medication");
    case EBiomeHazardType::Hypothermia:
        return TEXT("Stay dry, layer clothing, maintain caloric intake, recognize early symptoms");
    case EBiomeHazardType::Dehydration:
        return TEXT("Drink regularly before thirst, monitor urine color, carry extra water");
    case EBiomeHazardType::Lightning:
        return TEXT("Monitor weather, avoid peaks during storms, seek low ground with good drainage");
    case EBiomeHazardType::TidalSurge:
        return TEXT("Check tide tables, plan escape routes above high tide, monitor wave conditions");
    default:
        return TEXT("Consult local guides and current conditions for specific prevention strategies");
    }
}

bool UBiomeHazardLibrary::RequiresSpecializedEquipment(EBiomeType BiomeType, ESeason Season)
{
    switch (BiomeType)
    {
    case EBiomeType::Alpine:
    case EBiomeType::Arctic:
        return true; // Always requires specialized cold weather gear
    case EBiomeType::Desert:
        return Season == ESeason::Summer; // Extra cooling/sun protection needed
    case EBiomeType::Coastal:
        return true; // Corrosion-resistant gear needed
    case EBiomeType::Volcanic:
    case EBiomeType::Cave:
        return true; // Specialized safety equipment required
    default:
        return Season == ESeason::Winter; // Winter conditions may require special gear
    }
}