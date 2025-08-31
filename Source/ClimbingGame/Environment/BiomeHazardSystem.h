#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/World.h"
#include "DynamicWeatherSystem.h"
#include "Net/UnrealNetwork.h"
#include "BiomeHazardSystem.generated.h"

UENUM(BlueprintType)
enum class EBiomeType : uint8
{
    Alpine          UMETA(DisplayName = "Alpine"),
    Desert          UMETA(DisplayName = "Desert"),
    Coastal         UMETA(DisplayName = "Coastal"),
    Temperate       UMETA(DisplayName = "Temperate"),
    Arctic          UMETA(DisplayName = "Arctic"),
    Tropical        UMETA(DisplayName = "Tropical"),
    Volcanic        UMETA(DisplayName = "Volcanic"),
    Cave            UMETA(DisplayName = "Cave/Underground")
};

UENUM(BlueprintType)
enum class EBiomeHazardType : uint8
{
    // Desert Hazards
    HeatStroke      UMETA(DisplayName = "Heat Stroke"),
    Dehydration     UMETA(DisplayName = "Dehydration"),
    Sandstorm       UMETA(DisplayName = "Sandstorm"),
    FlashFlood      UMETA(DisplayName = "Flash Flood"),
    VenomousWildlife UMETA(DisplayName = "Venomous Wildlife"),
    
    // Alpine Hazards
    AltitudeSickness UMETA(DisplayName = "Altitude Sickness"),
    Hypothermia     UMETA(DisplayName = "Hypothermia"),
    Frostbite       UMETA(DisplayName = "Frostbite"),
    Crevasse        UMETA(DisplayName = "Crevasse"),
    SeraccFall      UMETA(DisplayName = "Serac Fall"),
    WhiteOut        UMETA(DisplayName = "White Out"),
    
    // Coastal Hazards
    TidalSurge      UMETA(DisplayName = "Tidal Surge"),
    WaveAction      UMETA(DisplayName = "Wave Action"),
    SaltSpray       UMETA(DisplayName = "Salt Spray"),
    MarineFog       UMETA(DisplayName = "Marine Fog"),
    StormSurge      UMETA(DisplayName = "Storm Surge"),
    
    // General Environmental
    Lightning       UMETA(DisplayName = "Lightning Strike"),
    WildFire        UMETA(DisplayName = "Wild Fire"),
    Earthquake      UMETA(DisplayName = "Earthquake"),
    VolcanicActivity UMETA(DisplayName = "Volcanic Activity"),
    ToxicGas        UMETA(DisplayName = "Toxic Gas"),
    FloodingRain    UMETA(DisplayName = "Flooding Rain")
};

UENUM(BlueprintType)
enum class ESeason : uint8
{
    Spring      UMETA(DisplayName = "Spring"),
    Summer      UMETA(DisplayName = "Summer"),
    Fall        UMETA(DisplayName = "Fall/Autumn"),
    Winter      UMETA(DisplayName = "Winter")
};

USTRUCT(BlueprintType)
struct FBiomeConditions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "-50.0", ClampMax = "60.0"))
    float BaseTemperature = 15.0f; // Celsius

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "0.0", ClampMax = "5000.0"))
    float Elevation = 1000.0f; // meters above sea level

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float Humidity = 50.0f; // percentage

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "0.0", ClampMax = "3000.0"))
    float AnnualPrecipitation = 800.0f; // mm per year

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float AverageWindSpeed = 15.0f; // km/h

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions")
    TArray<EBiomeHazardType> CommonHazards;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions")
    TArray<EBiomeHazardType> SeasonalHazards;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "0.0", ClampMax = "24.0"))
    float DaylightHours = 12.0f; // hours of daylight

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions", Meta = (ClampMin = "1.0", ClampMax = "100.0"))
    float UVIndex = 5.0f; // UV radiation intensity

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions")
    bool bHasWildlife = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Conditions")
    bool bRequiresSpecialEquipment = false;
};

USTRUCT(BlueprintType)
struct FSeasonalModifiers
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Modifiers")
    float TemperatureModifier = 0.0f; // Added to base temperature

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Modifiers", Meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float PrecipitationMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Modifiers", Meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float WindMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Modifiers", Meta = (ClampMin = "0.0", ClampMax = "24.0"))
    float DaylightModifier = 0.0f; // Hours added/subtracted

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Modifiers")
    TArray<EBiomeHazardType> ActiveHazards; // Hazards active in this season

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Modifiers")
    TArray<EBiomeHazardType> InactiveHazards; // Hazards inactive in this season
};

USTRUCT(BlueprintType)
struct FHazardEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event")
    EBiomeHazardType HazardType = EBiomeHazardType::HeatStroke;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Severity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event", Meta = (ClampMin = "0.0", ClampMax = "10000.0"))
    float EffectRadius = 1000.0f; // Unreal units (cm)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event", Meta = (ClampMin = "0.0", ClampMax = "3600.0"))
    float Duration = 300.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event", Meta = (ClampMin = "0.0", ClampMax = "600.0"))
    float WarningTime = 60.0f; // seconds advance warning

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event")
    bool bRequiresEvacuation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Event")
    TArray<FString> SafetyInstructions;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBiomeHazardTriggered, FHazardEvent, HazardEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonChanged, ESeason, NewSeason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnvironmentalDamage, AActor*, AffectedActor, EBiomeHazardType, HazardType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBiomeConditionsChanged, EBiomeType, BiomeType, FBiomeConditions, NewConditions);

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API ABiomeHazardSystem : public AActor
{
    GENERATED_BODY()

public:
    ABiomeHazardSystem();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    virtual void Tick(float DeltaTime) override;

    // Biome Management Functions
    UFUNCTION(BlueprintCallable, Category = "Biome System")
    void SetBiomeType(EBiomeType NewBiomeType);

    UFUNCTION(BlueprintCallable, Category = "Biome System")
    EBiomeType GetCurrentBiome() const { return CurrentBiomeType; }

    UFUNCTION(BlueprintCallable, Category = "Biome System")
    FBiomeConditions GetBiomeConditions() const { return CurrentBiomeConditions; }

    UFUNCTION(BlueprintCallable, Category = "Biome System")
    void UpdateBiomeConditions(const FBiomeConditions& NewConditions);

    // Seasonal System Functions
    UFUNCTION(BlueprintCallable, Category = "Seasonal System")
    void SetSeason(ESeason NewSeason);

    UFUNCTION(BlueprintCallable, Category = "Seasonal System")
    ESeason GetCurrentSeason() const { return CurrentSeason; }

    UFUNCTION(BlueprintCallable, Category = "Seasonal System")
    void AdvanceSeason();

    UFUNCTION(BlueprintCallable, Category = "Seasonal System")
    float GetSeasonProgress() const { return SeasonProgress; }

    UFUNCTION(BlueprintCallable, Category = "Seasonal System")
    void SetSeasonalProgression(bool bEnable, float SeasonDuration = 7200.0f); // 2 hours default

    // Hazard Management Functions
    UFUNCTION(BlueprintCallable, Category = "Hazard System")
    void TriggerHazardEvent(const FHazardEvent& HazardEvent);

    UFUNCTION(BlueprintCallable, Category = "Hazard System")
    bool IsHazardActiveInSeason(EBiomeHazardType HazardType, ESeason Season) const;

    UFUNCTION(BlueprintCallable, Category = "Hazard System")
    TArray<EBiomeHazardType> GetActiveHazards() const;

    UFUNCTION(BlueprintCallable, Category = "Hazard System")
    float GetHazardProbability(EBiomeHazardType HazardType) const;

    UFUNCTION(BlueprintCallable, Category = "Hazard System")
    void SetHazardProbability(EBiomeHazardType HazardType, float Probability);

    // Environmental Effect Functions
    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    bool IsActorAffectedByHazard(AActor* Actor, EBiomeHazardType HazardType) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    float CalculateEnvironmentalStress(AActor* Actor) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Effects")
    TArray<FString> GetSurvivalRecommendations() const;

    // Educational Functions
    UFUNCTION(BlueprintCallable, Category = "Educational")
    FString GetBiomeDescription() const;

    UFUNCTION(BlueprintCallable, Category = "Educational")
    TArray<FString> GetSeasonalSafetyTips() const;

    UFUNCTION(BlueprintCallable, Category = "Educational")
    FString GetHazardEducationalInfo(EBiomeHazardType HazardType) const;

    // Specialized Biome Functions
    UFUNCTION(BlueprintCallable, Category = "Desert Hazards")
    void ProcessHeatStroke(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Alpine Hazards")
    void ProcessAltitudeSickness(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Coastal Hazards")
    void ProcessTidalEffects(const FVector& Location);

    // Event Delegates
    UPROPERTY(BlueprintAssignable, Category = "Biome System")
    FOnBiomeHazardTriggered OnBiomeHazardTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Seasonal System")
    FOnSeasonChanged OnSeasonChanged;

    UPROPERTY(BlueprintAssignable, Category = "Environmental Effects")
    FOnEnvironmentalDamage OnEnvironmentalDamage;

    UPROPERTY(BlueprintAssignable, Category = "Biome System")
    FOnBiomeConditionsChanged OnBiomeConditionsChanged;

protected:
    // Core Biome Properties
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Biome System")
    EBiomeType CurrentBiomeType = EBiomeType::Temperate;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Biome System")
    FBiomeConditions CurrentBiomeConditions;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Seasonal System")
    ESeason CurrentSeason = ESeason::Summer;

    UPROPERTY(BlueprintReadOnly, Category = "Seasonal System")
    float SeasonProgress = 0.0f; // 0.0 to 1.0 through season

    UPROPERTY(BlueprintReadOnly, Category = "Seasonal System")
    float SeasonDuration = 7200.0f; // 2 hours in seconds

    UPROPERTY(BlueprintReadOnly, Category = "Seasonal System")
    bool bSeasonalProgressionEnabled = true;

    // Biome Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Configuration")
    TMap<EBiomeType, FBiomeConditions> BiomePresets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seasonal Configuration")
    TMap<ESeason, FSeasonalModifiers> SeasonalModifiers;

    // Hazard Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration")
    TMap<EBiomeHazardType, float> HazardProbabilities;

    UPROPERTY(BlueprintReadOnly, Category = "Hazard System")
    TArray<FHazardEvent> ActiveHazardEvents;

    // Environmental Effects
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UParticleSystemComponent* BiomeParticleSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio Effects")
    UAudioComponent* BiomeAudioComponent;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* BiomeZone;

    // Audio Assets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TMap<EBiomeType, USoundCue*> BiomeAmbientSounds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TMap<EBiomeHazardType, USoundCue*> HazardSounds;

    // Weather System Integration
    UPROPERTY(BlueprintReadOnly, Category = "Weather Integration")
    ADynamicWeatherSystem* WeatherSystem;

private:
    // Internal update functions
    void UpdateSeasonalProgression(float DeltaTime);
    void UpdateHazardEvents(float DeltaTime);
    void UpdateBiomeEffects();
    void ApplySeasonalModifiers();
    
    // Hazard processing functions
    void ProcessActiveHazards(float DeltaTime);
    void CheckForNewHazards(float DeltaTime);
    void CleanupExpiredHazards();
    
    // Biome-specific implementations
    void InitializeDesertBiome();
    void InitializeAlpineBiome();
    void InitializeCoastalBiome();
    void InitializeTemperate Biome();
    void InitializeArcticBiome();
    void InitializeTropicalBiome();
    void InitializeVolcanicBiome();
    void InitializeCaveBiome();
    
    // Utility functions
    FBiomeConditions ApplySeasonalModifiers(const FBiomeConditions& BaseConditions, const FSeasonalModifiers& Modifiers) const;
    float CalculateHazardSeverity(EBiomeHazardType HazardType, const FBiomeConditions& Conditions) const;
    
    UPROPERTY()
    float HazardCheckTimer = 0.0f;
    
    UPROPERTY()
    float SeasonTimer = 0.0f;
    
    // Network replication handlers
    UFUNCTION()
    void OnRep_CurrentBiomeType();
    
    UFUNCTION()
    void OnRep_CurrentSeason();
};

// Biome hazard calculation library
UCLASS()
class CLIMBINGGAME_API UBiomeHazardLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float CalculateHeatIndex(float Temperature, float Humidity);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float CalculateWindChill(float Temperature, float WindSpeed);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float CalculateAltitudePressure(float Elevation);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float CalculateOxygenLevel(float Elevation);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static bool IsHypothermiaThreat(float Temperature, float WindSpeed, float Humidity, float Exposure Time);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static bool IsHeatStrokeThreat(float Temperature, float Humidity, float ExertionLevel);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float CalculateDehydrationRate(float Temperature, float Humidity, float Activity Level);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float GetSeasonalDaylightHours(float Latitude, ESeason Season);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static float CalculateUVExposure(float Elevation, float Latitude, ESeason Season, float CloudCover);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static TArray<EBiomeHazardType> GetBiomeTypicalHazards(EBiomeType BiomeType);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static FString GetHazardPreventionAdvice(EBiomeHazardType HazardType);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Biome Calculations")
    static bool RequiresSpecializedEquipment(EBiomeType BiomeType, ESeason Season);
};