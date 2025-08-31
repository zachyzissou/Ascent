#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "RHI.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "ClimbingFrameRateManager.generated.h"

UENUM(BlueprintType)
enum class EFrameRateProfile : uint8
{
    VSync_30     UMETA(DisplayName = "VSync 30 FPS"),
    VSync_60     UMETA(DisplayName = "VSync 60 FPS"),
    VSync_120    UMETA(DisplayName = "VSync 120 FPS"),
    VSync_144    UMETA(DisplayName = "VSync 144 FPS"),
    Unlimited    UMETA(DisplayName = "Unlimited"),
    Custom       UMETA(DisplayName = "Custom Target")
};

UENUM(BlueprintType)
enum class EPerformanceMode : uint8
{
    PowerSaver      UMETA(DisplayName = "Power Saver"),
    Balanced        UMETA(DisplayName = "Balanced"),
    Performance     UMETA(DisplayName = "Performance"),
    MaxPerformance  UMETA(DisplayName = "Max Performance")
};

UENUM(BlueprintType)
enum class EQualityAdjustment : uint8
{
    Aggressive      UMETA(DisplayName = "Aggressive"),
    Moderate        UMETA(DisplayName = "Moderate"),
    Conservative    UMETA(DisplayName = "Conservative"),
    Disabled        UMETA(DisplayName = "Disabled")
};

USTRUCT(BlueprintType)
struct FHardwareTierSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate")
    float TargetFrameRate = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate")
    float MinimumFrameRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate")
    float MaximumFrameRate = 144.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    int32 TextureQuality = 2; // 0-4 scale

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    int32 ShadowQuality = 2; // 0-4 scale

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    int32 EffectsQuality = 2; // 0-4 scale

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    float RenderScale = 1.0f; // 0.5-2.0 range

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    bool bEnableDynamicResolution = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    float DynamicResolutionMin = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    float DynamicResolutionMax = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    int32 MaxRopeSegments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float PhysicsUpdateRate = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    int32 MaxActiveRopes = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    float NetworkUpdateRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    bool bEnableNetworkCompression = true;

    FString GetDisplayName() const
    {
        return FString::Printf(TEXT("Target: %.0f FPS, Min: %.0f FPS, Scale: %.2f"),
                              TargetFrameRate, MinimumFrameRate, RenderScale);
    }
};

USTRUCT(BlueprintType)
struct FFrameRateMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float CurrentFPS = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageFrameTime = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly)
    float MinFrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MaxFrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float FrameTimeVariance = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float TargetFrameTime = 16.67f; // 60 FPS

    UPROPERTY(BlueprintReadOnly)
    float FrameTimeBudgetUsed = 0.0f; // Percentage

    UPROPERTY(BlueprintReadOnly)
    int32 DroppedFrames = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalFrames = 0;

    UPROPERTY(BlueprintReadOnly)
    float PerformanceScore = 100.0f; // 0-100 scale

    float GetFrameRateStability() const
    {
        if (MaxFrameTime <= MinFrameTime)
            return 100.0f;
        return (1.0f - (FrameTimeVariance / AverageFrameTime)) * 100.0f;
    }
};

USTRUCT(BlueprintType)
struct FAdaptiveQualityState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float CurrentRenderScale = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentTextureQuality = 2;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentShadowQuality = 2;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentEffectsQuality = 2;

    UPROPERTY(BlueprintReadOnly)
    int32 AdjustmentsMadeThisSession = 0;

    UPROPERTY(BlueprintReadOnly)
    float LastAdjustmentTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bIsOptimizingForPerformance = false;

    UPROPERTY(BlueprintReadOnly)
    float PerformanceDebt = 0.0f; // Accumulated performance deficit
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingFrameRateManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // Frame rate management
    UFUNCTION(BlueprintCallable, Category = "Frame Rate Management")
    void SetFrameRateProfile(EFrameRateProfile Profile);

    UFUNCTION(BlueprintCallable, Category = "Frame Rate Management")
    void SetCustomFrameRateTarget(float TargetFPS);

    UFUNCTION(BlueprintCallable, Category = "Frame Rate Management")
    void SetPerformanceMode(EPerformanceMode Mode);

    UFUNCTION(BlueprintCallable, Category = "Frame Rate Management")
    EFrameRateProfile GetCurrentFrameRateProfile() const { return CurrentFrameRateProfile; }

    UFUNCTION(BlueprintCallable, Category = "Frame Rate Management")
    float GetCurrentFrameRateTarget() const { return CurrentTarget; }

    // Hardware tier detection and application
    UFUNCTION(BlueprintCallable, Category = "Hardware Tiers")
    EHardwareTier DetectHardwareTier() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Tiers")
    void ApplyHardwareTierSettings(EHardwareTier Tier);

    UFUNCTION(BlueprintCallable, Category = "Hardware Tiers")
    FHardwareTierSettings GetHardwareTierSettings(EHardwareTier Tier) const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Tiers")
    void SetHardwareTierSettings(EHardwareTier Tier, const FHardwareTierSettings& Settings);

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    FFrameRateMetrics GetCurrentMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    bool IsTargetFrameRateMet() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    float GetPerformanceScore() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void ResetPerformanceMetrics();

    // Adaptive quality
    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void EnableAdaptiveQuality(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void SetQualityAdjustmentMode(EQualityAdjustment Mode);

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    FAdaptiveQualityState GetAdaptiveQualityState() const;

    UFUNCTION(BlueprintCallable, Category = "Adaptive Quality")
    void ForceQualityAdjustment(bool bIncreaseQuality);

    // Dynamic resolution scaling
    UFUNCTION(BlueprintCallable, Category = "Dynamic Resolution")
    void EnableDynamicResolution(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Resolution")
    void SetDynamicResolutionRange(float MinScale, float MaxScale);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Resolution")
    float GetCurrentResolutionScale() const;

    // VSync and display management
    UFUNCTION(BlueprintCallable, Category = "Display Management")
    void SetVSyncEnabled(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Display Management")
    bool IsVSyncEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "Display Management")
    void SetDisplayRefreshRate(int32 RefreshRate);

    UFUNCTION(BlueprintCallable, Category = "Display Management")
    int32 GetDisplayRefreshRate() const;

    // Platform-specific optimizations
    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void ApplyPlatformOptimizations();

    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void OptimizeForPowerUsage(bool bOptimizeForBattery);

    UFUNCTION(BlueprintCallable, Category = "Platform Optimization")
    void SetThermalThrottling(bool bEnable);

    // Benchmarking and calibration
    UFUNCTION(BlueprintCallable, Category = "Calibration")
    void RunFrameRateCalibration(float CalibrationDuration = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "Calibration")
    void CalibrateForHardware();

    UFUNCTION(BlueprintCallable, Category = "Calibration")
    bool IsCalibrationRunning() const { return bIsCalibrating; }

    // Settings profiles
    UFUNCTION(BlueprintCallable, Category = "Settings Profiles")
    void SaveCurrentSettingsProfile(const FString& ProfileName);

    UFUNCTION(BlueprintCallable, Category = "Settings Profiles")
    void LoadSettingsProfile(const FString& ProfileName);

    UFUNCTION(BlueprintCallable, Category = "Settings Profiles")
    TArray<FString> GetAvailableSettingsProfiles() const;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    TMap<EHardwareTier, FHardwareTierSettings> HardwareTierSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    bool bEnableAdaptiveQuality = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    EQualityAdjustment QualityAdjustmentMode = EQualityAdjustment::Moderate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    float AdaptiveQualityInterval = 2.0f; // Seconds between adjustments

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    float FrameRateTolerancePercent = 10.0f; // Allow 10% variance

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    int32 MetricsHistorySize = 300; // 5 seconds at 60 FPS

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Config")
    bool bAutoDetectHardwareTier = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Frame Rate Events")
    FSimpleMulticastDelegate OnFrameRateTargetChanged;

    UPROPERTY(BlueprintAssignable, Category = "Frame Rate Events")
    FSimpleMulticastDelegate OnPerformanceModeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Frame Rate Events")
    FSimpleMulticastDelegate OnQualityAdjusted;

    UPROPERTY(BlueprintAssignable, Category = "Frame Rate Events")
    FSimpleMulticastDelegate OnHardwareTierDetected;

    UPROPERTY(BlueprintAssignable, Category = "Frame Rate Events")
    FSimpleMulticastDelegate OnCalibrationComplete;

protected:
    // Internal state
    EFrameRateProfile CurrentFrameRateProfile = EFrameRateProfile::VSync_60;
    EPerformanceMode CurrentPerformanceMode = EPerformanceMode::Balanced;
    EHardwareTier DetectedHardwareTier = EHardwareTier::Recommended;
    float CurrentTarget = 60.0f;

    // Metrics tracking
    FFrameRateMetrics CurrentMetrics;
    FAdaptiveQualityState AdaptiveState;
    TArray<float> FrameTimeHistory;
    float LastAdaptiveAdjustment = 0.0f;
    int32 FrameTimeHistoryIndex = 0;

    // Calibration state
    bool bIsCalibrating = false;
    float CalibrationStartTime = 0.0f;
    float CalibrationDuration = 0.0f;
    TArray<float> CalibrationFrameTimes;

    // Settings management
    TMap<FString, FHardwareTierSettings> SavedSettingsProfiles;

private:
    // Performance monitoring
    void UpdateFrameRateMetrics(float DeltaTime);
    void AnalyzeFrameRatePerformance();
    void DetectPerformanceIssues();
    
    // Adaptive quality adjustments
    void UpdateAdaptiveQuality(float DeltaTime);
    void AdjustQualityForPerformance(bool bIncreaseQuality);
    void AdjustRenderScale(float DeltaAdjustment);
    void AdjustTextureQuality(int32 DeltaLevels);
    void AdjustShadowQuality(int32 DeltaLevels);
    void AdjustEffectsQuality(int32 DeltaLevels);
    
    // Hardware detection
    void AnalyzeHardwareCapabilities();
    FString GetCPUBrandString() const;
    FString GetGPUBrandString() const;
    int32 GetSystemRAMGB() const;
    int32 GetVRAMGB() const;
    
    // Quality application
    void ApplyQualitySettings();
    void ApplyRenderingSettings(const FHardwareTierSettings& Settings);
    void ApplyPhysicsSettings(const FHardwareTierSettings& Settings);
    void ApplyNetworkSettings(const FHardwareTierSettings& Settings);
    
    // Platform-specific implementations
    void SetPlatformVSync(bool bEnable);
    void SetPlatformFrameRateLimit(float FrameRate);
    void ApplyWindowsPlatformOptimizations();
    void ApplyLinuxPlatformOptimizations();
    void ApplyMacPlatformOptimizations();
    
    // Calibration helpers
    void StartCalibrationProcess(float Duration);
    void UpdateCalibrationMetrics(float DeltaTime);
    void CompleteCalibration();
    void AnalyzeCalibrationResults();
    
    // Utility functions
    bool ShouldAdjustQuality() const;
    float CalculatePerformanceScore() const;
    float GetFrameRateVariance() const;
    void LogPerformanceState() const;
    
    // Settings persistence
    FString GetSettingsProfilePath(const FString& ProfileName) const;
    void SaveSettingsToFile(const FString& Filename, const FHardwareTierSettings& Settings);
    bool LoadSettingsFromFile(const FString& Filename, FHardwareTierSettings& OutSettings);
};