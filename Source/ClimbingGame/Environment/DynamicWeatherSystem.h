#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DynamicWeatherSystem.generated.h"

UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear       UMETA(DisplayName = "Clear"),
    Cloudy      UMETA(DisplayName = "Cloudy"),
    LightRain   UMETA(DisplayName = "Light Rain"),
    HeavyRain   UMETA(DisplayName = "Heavy Rain"),
    Snow        UMETA(DisplayName = "Snow"),
    Fog         UMETA(DisplayName = "Fog"),
    Storm       UMETA(DisplayName = "Storm"),
    Blizzard    UMETA(DisplayName = "Blizzard"),
    Sandstorm   UMETA(DisplayName = "Sandstorm"),
    HighWind    UMETA(DisplayName = "High Wind")
};

UENUM(BlueprintType)
enum class ESeverityLevel : uint8
{
    Minimal     UMETA(DisplayName = "Minimal"),
    Light       UMETA(DisplayName = "Light"), 
    Moderate    UMETA(DisplayName = "Moderate"),
    Heavy       UMETA(DisplayName = "Heavy"),
    Severe      UMETA(DisplayName = "Severe"),
    Extreme     UMETA(DisplayName = "Extreme")
};

USTRUCT(BlueprintType)
struct FWeatherCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    EWeatherType WeatherType = EWeatherType::Clear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Coverage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    FVector WindDirection = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float WindSpeed = 0.0f; // mph

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "-50.0", ClampMax = "130.0"))
    float Temperature = 70.0f; // Fahrenheit

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float Humidity = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "28.0", ClampMax = "32.0"))
    float BarometricPressure = 30.0f; // inches of mercury

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", Meta = (ClampMin = "0.0", ClampMax = "50000.0"))
    float VisibilityRange = 50000.0f; // Unreal units (cm)
};

USTRUCT(BlueprintType)
struct FWeatherTransition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Transition")
    FWeatherCondition FromCondition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Transition")
    FWeatherCondition ToCondition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Transition", Meta = (ClampMin = "0.0", ClampMax = "3600.0"))
    float TransitionDuration = 300.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Transition")
    UCurveFloat* TransitionCurve = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, FWeatherCondition, OldCondition, FWeatherCondition, NewCondition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHazardousWeatherAlert, FWeatherCondition, IncomingCondition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisibilityChanged, float, NewVisibilityRange);

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API ADynamicWeatherSystem : public AActor
{
    GENERATED_BODY()

public:
    ADynamicWeatherSystem();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    virtual void Tick(float DeltaTime) override;

    // Weather Management Functions
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void SetWeatherCondition(const FWeatherCondition& NewCondition, float TransitionTime = 60.0f);

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void TransitionToWeather(EWeatherType NewWeatherType, float Intensity = 0.5f, float TransitionTime = 300.0f);

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    FWeatherCondition GetCurrentWeatherCondition() const { return CurrentWeatherCondition; }

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    bool IsWeatherHazardous() const;

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    ESeverityLevel GetWeatherSeverity() const;

    // Environmental Impact Functions
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    float GetSurfaceFrictionMultiplier() const;

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    float GetStaminaDrainMultiplier() const;

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    float GetToolPlacementDifficultyMultiplier() const;

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    bool ShouldForceRouteEvacuation() const;

    // Wind System Functions
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    FVector GetWindForceAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    float GetWindGustMultiplier() const;

    // Visibility Functions  
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    float GetVisibilityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Weather System")
    bool IsLocationObscured(const FVector& Location) const;

    // Seasonal System Functions
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void SetSeasonalModifiers(float TemperatureOffset, float PrecipitationModifier, float WindModifier);

    // Event Delegates
    UPROPERTY(BlueprintAssignable, Category = "Weather System")
    FOnWeatherChanged OnWeatherChanged;

    UPROPERTY(BlueprintAssignable, Category = "Weather System")
    FOnHazardousWeatherAlert OnHazardousWeatherAlert;

    UPROPERTY(BlueprintAssignable, Category = "Weather System")
    FOnVisibilityChanged OnVisibilityChanged;

protected:
    // Core Weather Properties
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weather System")
    FWeatherCondition CurrentWeatherCondition;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weather System")
    FWeatherCondition TargetWeatherCondition;

    UPROPERTY(BlueprintReadOnly, Category = "Weather System")
    bool bIsTransitioning = false;

    UPROPERTY(BlueprintReadOnly, Category = "Weather System")
    float TransitionProgress = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Weather System")
    float TransitionDuration = 300.0f;

    // Weather Pattern Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    TArray<FWeatherTransition> WeatherPatterns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration", Meta = (ClampMin = "60.0", ClampMax = "7200.0"))
    float MinWeatherDuration = 600.0f; // 10 minutes minimum

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration", Meta = (ClampMin = "300.0", ClampMax = "14400.0"))
    float MaxWeatherDuration = 3600.0f; // 1 hour maximum

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    bool bEnableAutomaticWeatherProgression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Configuration")
    bool bEnableSeasonalVariation = true;

    // Hazard Thresholds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration", Meta = (ClampMin = "20.0", ClampMax = "80.0"))
    float HazardousWindSpeed = 35.0f; // mph

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration", Meta = (ClampMin = "0.3", ClampMax = "0.8"))
    float HazardousRainIntensity = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration", Meta = (ClampMin = "1000.0", ClampMax = "20000.0"))
    float MinimumSafeVisibility = 5000.0f; // Unreal units

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard Configuration", Meta = (ClampMin = "10.0", ClampMax = "40.0"))
    float CriticalLowTemperature = 25.0f; // Fahrenheit

    // Visual Effects Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UParticleSystemComponent* RainParticleSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UParticleSystemComponent* SnowParticleSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UParticleSystemComponent* FogParticleSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual Effects")
    UParticleSystemComponent* SandstormParticleSystem;

    // Audio Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
    UAudioComponent* WeatherAudioComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TMap<EWeatherType, USoundCue*> WeatherSounds;

    // Wind Gust System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind System", Meta = (ClampMin = "5.0", ClampMax = "60.0"))
    float GustInterval = 15.0f; // seconds between gusts

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind System", Meta = (ClampMin = "1.2", ClampMax = "3.0"))
    float MaxGustMultiplier = 2.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Wind System")
    float CurrentGustMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Wind System")
    float GustTimer = 0.0f;

    // Seasonal Modifiers
    UPROPERTY(BlueprintReadOnly, Category = "Seasonal System")
    float SeasonalTemperatureOffset = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Seasonal System")
    float SeasonalPrecipitationModifier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Seasonal System")
    float SeasonalWindModifier = 1.0f;

private:
    // Internal update functions
    void UpdateWeatherTransition(float DeltaTime);
    void UpdateWindGusts(float DeltaTime);
    void UpdateVisualEffects();
    void UpdateAudioEffects();
    void CheckForHazardousConditions();
    
    // Weather calculation helpers
    FWeatherCondition InterpolateWeatherConditions(const FWeatherCondition& A, const FWeatherCondition& B, float Alpha) const;
    float CalculateFrictionReduction() const;
    float CalculateStaminaPenalty() const;
    float CalculateToolDifficulty() const;
    
    // Automatic weather progression
    UPROPERTY()
    float WeatherProgressionTimer = 0.0f;
    
    UPROPERTY()
    float CurrentWeatherDuration = 0.0f;
    
    void UpdateAutomaticWeatherProgression(float DeltaTime);
    EWeatherType SelectNextWeatherType() const;
    
    // Network replication handlers
    UFUNCTION()
    void OnRep_CurrentWeatherCondition();
    
    UFUNCTION()
    void OnRep_TargetWeatherCondition();
};

// Helper Blueprint Function Library for Weather Calculations
UCLASS()
class CLIMBINGGAME_API UWeatherCalculationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static float CalculateWindChill(float Temperature, float WindSpeed);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static float CalculateHeatIndex(float Temperature, float Humidity);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static bool IsFreezingConditions(float Temperature, float Humidity);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static ESeverityLevel DetermineWeatherSeverity(const FWeatherCondition& Condition);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static float ConvertTemperatureCelsiusToFahrenheit(float Celsius);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static float ConvertTemperatureFahrenheitToCelsius(float Fahrenheit);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weather Calculations")
    static FVector CalculateWindForceOnObject(const FWeatherCondition& Weather, float ObjectArea, float DragCoefficient);
};