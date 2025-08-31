#include "ClimbingFrameRateManager.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameUserSettings.h"
#include "RenderCore/Public/RenderingThread.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformApplicationMisc.h"
#include "RHI.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("ClimbingFrameRate"), STATGROUP_ClimbingFrameRate, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Frame Rate Management"), STAT_FrameRateManagement, STATGROUP_ClimbingFrameRate);
DECLARE_CYCLE_STAT(TEXT("Adaptive Quality"), STAT_AdaptiveQuality, STATGROUP_ClimbingFrameRate);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Current FPS"), STAT_CurrentFPS, STATGROUP_ClimbingFrameRate);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Frame Time MS"), STAT_FrameTimeMS, STATGROUP_ClimbingFrameRate);

void UClimbingFrameRateManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Initialize hardware tier settings with appropriate defaults
    FHardwareTierSettings MinimumSettings;
    MinimumSettings.TargetFrameRate = 30.0f;
    MinimumSettings.MinimumFrameRate = 20.0f;
    MinimumSettings.MaximumFrameRate = 60.0f;
    MinimumSettings.TextureQuality = 1;
    MinimumSettings.ShadowQuality = 1;
    MinimumSettings.EffectsQuality = 1;
    MinimumSettings.RenderScale = 0.75f;
    MinimumSettings.bEnableDynamicResolution = true;
    MinimumSettings.DynamicResolutionMin = 0.5f;
    MinimumSettings.DynamicResolutionMax = 0.9f;
    MinimumSettings.MaxRopeSegments = 16;
    MinimumSettings.PhysicsUpdateRate = 30.0f;
    MinimumSettings.MaxActiveRopes = 20;
    MinimumSettings.NetworkUpdateRate = 20.0f;
    HardwareTierSettings.Add(EHardwareTier::Minimum, MinimumSettings);

    FHardwareTierSettings RecommendedSettings;
    RecommendedSettings.TargetFrameRate = 60.0f;
    RecommendedSettings.MinimumFrameRate = 30.0f;
    RecommendedSettings.MaximumFrameRate = 90.0f;
    RecommendedSettings.TextureQuality = 2;
    RecommendedSettings.ShadowQuality = 2;
    RecommendedSettings.EffectsQuality = 2;
    RecommendedSettings.RenderScale = 1.0f;
    RecommendedSettings.bEnableDynamicResolution = true;
    RecommendedSettings.DynamicResolutionMin = 0.8f;
    RecommendedSettings.DynamicResolutionMax = 1.1f;
    RecommendedSettings.MaxRopeSegments = 32;
    RecommendedSettings.PhysicsUpdateRate = 60.0f;
    RecommendedSettings.MaxActiveRopes = 40;
    RecommendedSettings.NetworkUpdateRate = 30.0f;
    HardwareTierSettings.Add(EHardwareTier::Recommended, RecommendedSettings);

    FHardwareTierSettings HighSettings;
    HighSettings.TargetFrameRate = 90.0f;
    HighSettings.MinimumFrameRate = 60.0f;
    HighSettings.MaximumFrameRate = 120.0f;
    HighSettings.TextureQuality = 3;
    HighSettings.ShadowQuality = 3;
    HighSettings.EffectsQuality = 3;
    HighSettings.RenderScale = 1.2f;
    HighSettings.bEnableDynamicResolution = true;
    HighSettings.DynamicResolutionMin = 1.0f;
    HighSettings.DynamicResolutionMax = 1.3f;
    HighSettings.MaxRopeSegments = 48;
    HighSettings.PhysicsUpdateRate = 90.0f;
    HighSettings.MaxActiveRopes = 60;
    HighSettings.NetworkUpdateRate = 60.0f;
    HardwareTierSettings.Add(EHardwareTier::High, HighSettings);

    FHardwareTierSettings UltraSettings;
    UltraSettings.TargetFrameRate = 120.0f;
    UltraSettings.MinimumFrameRate = 90.0f;
    UltraSettings.MaximumFrameRate = 240.0f;
    UltraSettings.TextureQuality = 4;
    UltraSettings.ShadowQuality = 4;
    UltraSettings.EffectsQuality = 4;
    UltraSettings.RenderScale = 1.5f;
    UltraSettings.bEnableDynamicResolution = false; // Ultra settings don't need dynamic scaling
    UltraSettings.DynamicResolutionMin = 1.0f;
    UltraSettings.DynamicResolutionMax = 2.0f;
    UltraSettings.MaxRopeSegments = 64;
    UltraSettings.PhysicsUpdateRate = 120.0f;
    UltraSettings.MaxActiveRopes = 100;
    UltraSettings.NetworkUpdateRate = 60.0f;
    HardwareTierSettings.Add(EHardwareTier::Ultra, UltraSettings);

    // Initialize metrics tracking
    FrameTimeHistory.Reserve(MetricsHistorySize);
    CalibrationFrameTimes.Reserve(1000); // Reserve space for calibration data

    // Initialize adaptive state
    AdaptiveState.CurrentRenderScale = 1.0f;
    AdaptiveState.CurrentTextureQuality = 2;
    AdaptiveState.CurrentShadowQuality = 2;
    AdaptiveState.CurrentEffectsQuality = 2;

    // Auto-detect hardware tier if enabled
    if (bAutoDetectHardwareTier)
    {
        DetectedHardwareTier = DetectHardwareTier();
        ApplyHardwareTierSettings(DetectedHardwareTier);
    }

    // Set initial frame rate profile
    SetFrameRateProfile(EFrameRateProfile::VSync_60);

    UE_LOG(LogTemp, Log, TEXT("ClimbingFrameRateManager initialized: Hardware Tier=%s, Target=%.0f FPS"),
           *UEnum::GetValueAsString(DetectedHardwareTier), CurrentTarget);
}

void UClimbingFrameRateManager::Deinitialize()
{
    if (bIsCalibrating)
    {
        CompleteCalibration();
    }

    FrameTimeHistory.Empty();
    CalibrationFrameTimes.Empty();
    SavedSettingsProfiles.Empty();

    Super::Deinitialize();
}

void UClimbingFrameRateManager::Tick(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_FrameRateManagement);

    // Update metrics
    UpdateFrameRateMetrics(DeltaTime);

    // Handle calibration if running
    if (bIsCalibrating)
    {
        UpdateCalibrationMetrics(DeltaTime);
    }

    // Update adaptive quality if enabled
    if (bEnableAdaptiveQuality)
    {
        UpdateAdaptiveQuality(DeltaTime);
    }

    // Analyze performance periodically
    static float AnalysisTimer = 0.0f;
    AnalysisTimer += DeltaTime;
    if (AnalysisTimer >= 5.0f) // Every 5 seconds
    {
        AnalyzeFrameRatePerformance();
        AnalysisTimer = 0.0f;
    }
}

void UClimbingFrameRateManager::SetFrameRateProfile(EFrameRateProfile Profile)
{
    CurrentFrameRateProfile = Profile;

    switch (Profile)
    {
        case EFrameRateProfile::VSync_30:
            CurrentTarget = 30.0f;
            SetPlatformFrameRateLimit(30.0f);
            SetVSyncEnabled(true);
            break;
        case EFrameRateProfile::VSync_60:
            CurrentTarget = 60.0f;
            SetPlatformFrameRateLimit(60.0f);
            SetVSyncEnabled(true);
            break;
        case EFrameRateProfile::VSync_120:
            CurrentTarget = 120.0f;
            SetPlatformFrameRateLimit(120.0f);
            SetVSyncEnabled(true);
            break;
        case EFrameRateProfile::VSync_144:
            CurrentTarget = 144.0f;
            SetPlatformFrameRateLimit(144.0f);
            SetVSyncEnabled(true);
            break;
        case EFrameRateProfile::Unlimited:
            CurrentTarget = 300.0f; // High target for unlimited
            SetPlatformFrameRateLimit(0.0f); // 0 = unlimited
            SetVSyncEnabled(false);
            break;
        case EFrameRateProfile::Custom:
            // Custom target should be set via SetCustomFrameRateTarget
            SetVSyncEnabled(false);
            break;
    }

    // Update target frame time for metrics
    CurrentMetrics.TargetFrameTime = 1000.0f / CurrentTarget;

    OnFrameRateTargetChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Frame rate profile changed to %s (Target: %.0f FPS)"),
           *UEnum::GetValueAsString(Profile), CurrentTarget);
}

void UClimbingFrameRateManager::SetCustomFrameRateTarget(float TargetFPS)
{
    CurrentTarget = FMath::Clamp(TargetFPS, 15.0f, 300.0f);
    CurrentFrameRateProfile = EFrameRateProfile::Custom;
    
    SetPlatformFrameRateLimit(CurrentTarget);
    CurrentMetrics.TargetFrameTime = 1000.0f / CurrentTarget;

    OnFrameRateTargetChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Custom frame rate target set: %.0f FPS"), CurrentTarget);
}

void UClimbingFrameRateManager::SetPerformanceMode(EPerformanceMode Mode)
{
    if (CurrentPerformanceMode == Mode)
        return;

    CurrentPerformanceMode = Mode;

    // Adjust settings based on performance mode
    FHardwareTierSettings CurrentSettings = GetHardwareTierSettings(DetectedHardwareTier);

    switch (Mode)
    {
        case EPerformanceMode::PowerSaver:
            // Reduce all quality settings for battery life
            CurrentSettings.TargetFrameRate *= 0.5f; // Half the target frame rate
            CurrentSettings.RenderScale *= 0.8f;
            CurrentSettings.TextureQuality = FMath::Max(0, CurrentSettings.TextureQuality - 2);
            CurrentSettings.ShadowQuality = FMath::Max(0, CurrentSettings.ShadowQuality - 2);
            CurrentSettings.EffectsQuality = FMath::Max(0, CurrentSettings.EffectsQuality - 2);
            CurrentSettings.PhysicsUpdateRate *= 0.5f;
            break;

        case EPerformanceMode::Balanced:
            // Use tier defaults
            break;

        case EPerformanceMode::Performance:
            // Optimize for frame rate over quality
            CurrentSettings.TargetFrameRate *= 1.2f;
            CurrentSettings.RenderScale *= 0.9f;
            CurrentSettings.bEnableDynamicResolution = true;
            break;

        case EPerformanceMode::MaxPerformance:
            // Maximum frame rate, minimum quality
            CurrentSettings.TargetFrameRate *= 1.5f;
            CurrentSettings.RenderScale *= 0.75f;
            CurrentSettings.TextureQuality = FMath::Max(0, CurrentSettings.TextureQuality - 1);
            CurrentSettings.ShadowQuality = FMath::Max(0, CurrentSettings.ShadowQuality - 1);
            CurrentSettings.bEnableDynamicResolution = true;
            CurrentSettings.DynamicResolutionMin *= 0.8f;
            break;
    }

    // Apply the modified settings
    ApplyRenderingSettings(CurrentSettings);
    ApplyPhysicsSettings(CurrentSettings);

    // Update current target
    CurrentTarget = CurrentSettings.TargetFrameRate;
    CurrentMetrics.TargetFrameTime = 1000.0f / CurrentTarget;

    OnPerformanceModeChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Performance mode changed to %s"), *UEnum::GetValueAsString(Mode));
}

EHardwareTier UClimbingFrameRateManager::DetectHardwareTier() const
{
    // Get system specifications
    int32 SystemRAM = GetSystemRAMGB();
    int32 VRAM = GetVRAMGB();
    FString CPUBrand = GetCPUBrandString();
    FString GPUBrand = GetGPUBrandString();

    UE_LOG(LogTemp, Log, TEXT("Hardware Detection: RAM=%dGB, VRAM=%dGB, CPU=%s, GPU=%s"),
           SystemRAM, VRAM, *CPUBrand, *GPUBrand);

    // Ultra tier detection
    if (SystemRAM >= 32 && VRAM >= 12)
    {
        if (GPUBrand.Contains(TEXT("RTX 40")) || GPUBrand.Contains(TEXT("RX 7900")))
        {
            return EHardwareTier::Ultra;
        }
    }

    // High tier detection
    if (SystemRAM >= 16 && VRAM >= 8)
    {
        if (GPUBrand.Contains(TEXT("RTX 30")) || GPUBrand.Contains(TEXT("RTX 20")) ||
            GPUBrand.Contains(TEXT("RX 6800")) || GPUBrand.Contains(TEXT("GTX 1080")))
        {
            return EHardwareTier::High;
        }
    }

    // Recommended tier detection
    if (SystemRAM >= 8 && VRAM >= 4)
    {
        if (GPUBrand.Contains(TEXT("GTX 1060")) || GPUBrand.Contains(TEXT("RX 580")) ||
            GPUBrand.Contains(TEXT("GTX 1660")) || GPUBrand.Contains(TEXT("RTX")))
        {
            return EHardwareTier::Recommended;
        }
    }

    // Default to minimum tier
    return EHardwareTier::Minimum;
}

void UClimbingFrameRateManager::ApplyHardwareTierSettings(EHardwareTier Tier)
{
    const FHardwareTierSettings* Settings = HardwareTierSettings.Find(Tier);
    if (!Settings)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hardware tier settings not found for tier: %s"), 
               *UEnum::GetValueAsString(Tier));
        return;
    }

    DetectedHardwareTier = Tier;

    // Apply all settings categories
    ApplyRenderingSettings(*Settings);
    ApplyPhysicsSettings(*Settings);
    ApplyNetworkSettings(*Settings);

    // Update current targets
    CurrentTarget = Settings->TargetFrameRate;
    CurrentMetrics.TargetFrameTime = 1000.0f / CurrentTarget;

    // Update adaptive state
    AdaptiveState.CurrentRenderScale = Settings->RenderScale;
    AdaptiveState.CurrentTextureQuality = Settings->TextureQuality;
    AdaptiveState.CurrentShadowQuality = Settings->ShadowQuality;
    AdaptiveState.CurrentEffectsQuality = Settings->EffectsQuality;

    OnHardwareTierDetected.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Applied hardware tier settings: %s (%s)"),
           *UEnum::GetValueAsString(Tier), *Settings->GetDisplayName());
}

FHardwareTierSettings UClimbingFrameRateManager::GetHardwareTierSettings(EHardwareTier Tier) const
{
    const FHardwareTierSettings* Settings = HardwareTierSettings.Find(Tier);
    return Settings ? *Settings : FHardwareTierSettings();
}

void UClimbingFrameRateManager::SetHardwareTierSettings(EHardwareTier Tier, const FHardwareTierSettings& Settings)
{
    HardwareTierSettings.Add(Tier, Settings);
    
    // If this is the currently active tier, apply the settings immediately
    if (Tier == DetectedHardwareTier)
    {
        ApplyHardwareTierSettings(Tier);
    }
}

FFrameRateMetrics UClimbingFrameRateManager::GetCurrentMetrics() const
{
    return CurrentMetrics;
}

bool UClimbingFrameRateManager::IsTargetFrameRateMet() const
{
    float Tolerance = CurrentTarget * (FrameRateTolerancePercent / 100.0f);
    return CurrentMetrics.CurrentFPS >= (CurrentTarget - Tolerance);
}

float UClimbingFrameRateManager::GetPerformanceScore() const
{
    return CalculatePerformanceScore();
}

void UClimbingFrameRateManager::ResetPerformanceMetrics()
{
    CurrentMetrics = FFrameRateMetrics();
    CurrentMetrics.TargetFrameTime = 1000.0f / CurrentTarget;
    FrameTimeHistory.Empty();
    FrameTimeHistoryIndex = 0;

    UE_LOG(LogTemp, Log, TEXT("Performance metrics reset"));
}

void UClimbingFrameRateManager::EnableAdaptiveQuality(bool bEnable)
{
    bEnableAdaptiveQuality = bEnable;
    
    if (bEnable)
    {
        UE_LOG(LogTemp, Log, TEXT("Adaptive quality enabled: Mode=%s, Interval=%.1fs"),
               *UEnum::GetValueAsString(QualityAdjustmentMode), AdaptiveQualityInterval);
    }
    else
    {
        AdaptiveState.bIsOptimizingForPerformance = false;
        AdaptiveState.PerformanceDebt = 0.0f;
        UE_LOG(LogTemp, Log, TEXT("Adaptive quality disabled"));
    }
}

void UClimbingFrameRateManager::SetQualityAdjustmentMode(EQualityAdjustment Mode)
{
    QualityAdjustmentMode = Mode;
    UE_LOG(LogTemp, Log, TEXT("Quality adjustment mode set to: %s"), *UEnum::GetValueAsString(Mode));
}

FAdaptiveQualityState UClimbingFrameRateManager::GetAdaptiveQualityState() const
{
    return AdaptiveState;
}

void UClimbingFrameRateManager::ForceQualityAdjustment(bool bIncreaseQuality)
{
    AdjustQualityForPerformance(bIncreaseQuality);
    
    UE_LOG(LogTemp, Log, TEXT("Forced quality adjustment: %s"), 
           bIncreaseQuality ? TEXT("Increase") : TEXT("Decrease"));
}

void UClimbingFrameRateManager::RunFrameRateCalibration(float CalibrationDuration)
{
    if (bIsCalibrating)
    {
        UE_LOG(LogTemp, Warning, TEXT("Calibration already in progress"));
        return;
    }

    StartCalibrationProcess(CalibrationDuration);
}

void UClimbingFrameRateManager::CalibrateForHardware()
{
    UE_LOG(LogTemp, Log, TEXT("Starting hardware calibration..."));

    // Run a comprehensive calibration that tests different quality levels
    RunFrameRateCalibration(30.0f); // 30 second calibration
}

void UClimbingFrameRateManager::UpdateFrameRateMetrics(float DeltaTime)
{
    float FrameTimeMs = DeltaTime * 1000.0f;

    // Add to history
    FrameTimeHistory.Add(FrameTimeMs);
    if (FrameTimeHistory.Num() > MetricsHistorySize)
    {
        FrameTimeHistory.RemoveAt(0);
    }

    // Calculate current FPS
    CurrentMetrics.CurrentFPS = 1000.0f / FrameTimeMs;

    // Calculate statistics from history
    if (FrameTimeHistory.Num() > 0)
    {
        float TotalTime = 0.0f;
        float MinTime = FrameTimeHistory[0];
        float MaxTime = FrameTimeHistory[0];

        for (float Time : FrameTimeHistory)
        {
            TotalTime += Time;
            MinTime = FMath::Min(MinTime, Time);
            MaxTime = FMath::Max(MaxTime, Time);
        }

        CurrentMetrics.AverageFrameTime = TotalTime / FrameTimeHistory.Num();
        CurrentMetrics.MinFrameTime = MinTime;
        CurrentMetrics.MaxFrameTime = MaxTime;

        // Calculate variance
        float VarianceSum = 0.0f;
        for (float Time : FrameTimeHistory)
        {
            VarianceSum += FMath::Pow(Time - CurrentMetrics.AverageFrameTime, 2.0f);
        }
        CurrentMetrics.FrameTimeVariance = FMath::Sqrt(VarianceSum / FrameTimeHistory.Num());
    }

    // Calculate budget usage
    CurrentMetrics.FrameTimeBudgetUsed = (CurrentMetrics.AverageFrameTime / CurrentMetrics.TargetFrameTime) * 100.0f;

    // Count dropped frames
    if (FrameTimeMs > CurrentMetrics.TargetFrameTime * 1.5f)
    {
        CurrentMetrics.DroppedFrames++;
    }

    CurrentMetrics.TotalFrames++;

    // Calculate performance score
    CurrentMetrics.PerformanceScore = CalculatePerformanceScore();

    // Update stats
    SET_FLOAT_STAT(STAT_CurrentFPS, CurrentMetrics.CurrentFPS);
    SET_FLOAT_STAT(STAT_FrameTimeMS, FrameTimeMs);
}

void UClimbingFrameRateManager::UpdateAdaptiveQuality(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_AdaptiveQuality);

    if (QualityAdjustmentMode == EQualityAdjustment::Disabled)
        return;

    LastAdaptiveAdjustment += DeltaTime;

    if (LastAdaptiveAdjustment >= AdaptiveQualityInterval)
    {
        bool bNeedsAdjustment = ShouldAdjustQuality();
        
        if (bNeedsAdjustment)
        {
            bool bCurrentlyUnderperforming = CurrentMetrics.CurrentFPS < (CurrentTarget * 0.9f);
            
            if (bCurrentlyUnderperforming)
            {
                // Reduce quality to improve performance
                AdjustQualityForPerformance(false);
                AdaptiveState.bIsOptimizingForPerformance = true;
                AdaptiveState.PerformanceDebt += (CurrentTarget - CurrentMetrics.CurrentFPS);
            }
            else if (AdaptiveState.PerformanceDebt > 0.0f && CurrentMetrics.CurrentFPS > (CurrentTarget * 1.1f))
            {
                // We have performance headroom and debt to pay back
                AdjustQualityForPerformance(true);
                AdaptiveState.PerformanceDebt -= (CurrentMetrics.CurrentFPS - CurrentTarget);
                AdaptiveState.PerformanceDebt = FMath::Max(0.0f, AdaptiveState.PerformanceDebt);
                
                if (AdaptiveState.PerformanceDebt <= 0.0f)
                {
                    AdaptiveState.bIsOptimizingForPerformance = false;
                }
            }
        }

        LastAdaptiveAdjustment = 0.0f;
    }
}

void UClimbingFrameRateManager::AdjustQualityForPerformance(bool bIncreaseQuality)
{
    float AdjustmentStrength = 1.0f;
    
    switch (QualityAdjustmentMode)
    {
        case EQualityAdjustment::Conservative:
            AdjustmentStrength = 0.5f;
            break;
        case EQualityAdjustment::Moderate:
            AdjustmentStrength = 1.0f;
            break;
        case EQualityAdjustment::Aggressive:
            AdjustmentStrength = 2.0f;
            break;
    }

    if (bIncreaseQuality)
    {
        // Gradually increase quality
        if (AdaptiveState.CurrentRenderScale < 1.5f)
        {
            AdjustRenderScale(0.05f * AdjustmentStrength);
        }
        else if (AdaptiveState.CurrentTextureQuality < 4)
        {
            AdjustTextureQuality(1);
        }
        else if (AdaptiveState.CurrentShadowQuality < 4)
        {
            AdjustShadowQuality(1);
        }
        else if (AdaptiveState.CurrentEffectsQuality < 4)
        {
            AdjustEffectsQuality(1);
        }
    }
    else
    {
        // Gradually decrease quality
        if (AdaptiveState.CurrentRenderScale > 0.5f)
        {
            AdjustRenderScale(-0.1f * AdjustmentStrength);
        }
        else if (AdaptiveState.CurrentEffectsQuality > 0)
        {
            AdjustEffectsQuality(-1);
        }
        else if (AdaptiveState.CurrentShadowQuality > 0)
        {
            AdjustShadowQuality(-1);
        }
        else if (AdaptiveState.CurrentTextureQuality > 0)
        {
            AdjustTextureQuality(-1);
        }
    }

    AdaptiveState.AdjustmentsMadeThisSession++;
    AdaptiveState.LastAdjustmentTime = GetWorld()->GetTimeSeconds();
    
    OnQualityAdjusted.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Quality adjusted: %s (Scale=%.2f, Texture=%d, Shadow=%d, Effects=%d)"),
           bIncreaseQuality ? TEXT("Increased") : TEXT("Decreased"),
           AdaptiveState.CurrentRenderScale,
           AdaptiveState.CurrentTextureQuality,
           AdaptiveState.CurrentShadowQuality,
           AdaptiveState.CurrentEffectsQuality);
}

void UClimbingFrameRateManager::ApplyRenderingSettings(const FHardwareTierSettings& Settings)
{
    if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
    {
        // Apply quality settings
        UserSettings->SetTextureQuality(Settings.TextureQuality);
        UserSettings->SetShadowQuality(Settings.ShadowQuality);
        UserSettings->SetPostProcessingQuality(Settings.EffectsQuality);
        UserSettings->SetResolutionScaleNormalized(Settings.RenderScale);

        // Apply dynamic resolution settings
        if (Settings.bEnableDynamicResolution)
        {
            // Enable dynamic resolution scaling
            // This would be implemented using Unreal's dynamic resolution system
        }

        UserSettings->ApplySettings(false);
    }
}

void UClimbingFrameRateManager::ApplyPhysicsSettings(const FHardwareTierSettings& Settings)
{
    // Apply to performance manager if available
    if (UClimbingPerformanceManager* PerfManager = GetWorld()->GetSubsystem<UClimbingPerformanceManager>())
    {
        PerfManager->PhysicsSettings.UltraRopeSegments = Settings.MaxRopeSegments;
        PerfManager->PerformanceTargets.MaxActiveRopes = Settings.MaxActiveRopes;
        
        // Update physics timestep based on update rate
        if (UWorld* World = GetWorld())
        {
            World->GetPhysicsScene()->SetFixedTimeStep(1.0f / Settings.PhysicsUpdateRate);
        }
    }
}

void UClimbingFrameRateManager::SetVSyncEnabled(bool bEnable)
{
    SetPlatformVSync(bEnable);
}

bool UClimbingFrameRateManager::IsVSyncEnabled() const
{
    if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
    {
        return UserSettings->IsVSyncEnabled();
    }
    return false;
}

float UClimbingFrameRateManager::CalculatePerformanceScore() const
{
    if (FrameTimeHistory.Num() < 10)
        return 100.0f;

    float TargetScore = (CurrentMetrics.CurrentFPS / CurrentTarget) * 100.0f;
    float StabilityScore = GetFrameRateStability();
    float EfficiencyScore = FMath::Clamp((100.0f - CurrentMetrics.FrameTimeBudgetUsed), 0.0f, 100.0f);

    // Weighted average: 50% target achievement, 30% stability, 20% efficiency
    return (TargetScore * 0.5f) + (StabilityScore * 0.3f) + (EfficiencyScore * 0.2f);
}

float UClimbingFrameRateManager::GetFrameRateStability() const
{
    return CurrentMetrics.GetFrameRateStability();
}

bool UClimbingFrameRateManager::ShouldAdjustQuality() const
{
    if (FrameTimeHistory.Num() < 30) // Need some history
        return false;

    // Check if we're consistently over or under target
    float PerformanceRatio = CurrentMetrics.CurrentFPS / CurrentTarget;
    float Tolerance = FrameRateTolerancePercent / 100.0f;

    return (PerformanceRatio < (1.0f - Tolerance)) || 
           (PerformanceRatio > (1.0f + Tolerance * 2.0f)); // Allow more headroom before increasing quality
}

void UClimbingFrameRateManager::StartCalibrationProcess(float Duration)
{
    bIsCalibrating = true;
    CalibrationStartTime = GetWorld()->GetTimeSeconds();
    CalibrationDuration = Duration;
    CalibrationFrameTimes.Empty();

    UE_LOG(LogTemp, Log, TEXT("Started frame rate calibration: Duration=%.1fs"), Duration);
}

void UClimbingFrameRateManager::UpdateCalibrationMetrics(float DeltaTime)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    CalibrationFrameTimes.Add(DeltaTime * 1000.0f);

    if (CurrentTime - CalibrationStartTime >= CalibrationDuration)
    {
        CompleteCalibration();
    }
}

void UClimbingFrameRateManager::CompleteCalibration()
{
    if (!bIsCalibrating)
        return;

    bIsCalibrating = false;
    AnalyzeCalibrationResults();
    OnCalibrationComplete.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Frame rate calibration completed: %d frames analyzed"), 
           CalibrationFrameTimes.Num());
}

void UClimbingFrameRateManager::AnalyzeCalibrationResults()
{
    if (CalibrationFrameTimes.Num() == 0)
        return;

    // Calculate calibration statistics
    float TotalTime = 0.0f;
    float MinTime = CalibrationFrameTimes[0];
    float MaxTime = CalibrationFrameTimes[0];

    for (float Time : CalibrationFrameTimes)
    {
        TotalTime += Time;
        MinTime = FMath::Min(MinTime, Time);
        MaxTime = FMath::Max(MaxTime, Time);
    }

    float AverageFrameTime = TotalTime / CalibrationFrameTimes.Num();
    float AverageFPS = 1000.0f / AverageFrameTime;

    UE_LOG(LogTemp, Log, TEXT("Calibration Results: Avg FPS=%.1f, Min FPS=%.1f, Max FPS=%.1f, Stability=%.1f%%"),
           AverageFPS, 1000.0f / MaxTime, 1000.0f / MinTime, 
           (1.0f - ((MaxTime - MinTime) / AverageFrameTime)) * 100.0f);

    // Recommend optimal settings based on results
    EHardwareTier RecommendedTier = EHardwareTier::Minimum;
    
    if (AverageFPS >= 120.0f)
        RecommendedTier = EHardwareTier::Ultra;
    else if (AverageFPS >= 90.0f)
        RecommendedTier = EHardwareTier::High;
    else if (AverageFPS >= 60.0f)
        RecommendedTier = EHardwareTier::Recommended;

    if (RecommendedTier != DetectedHardwareTier)
    {
        UE_LOG(LogTemp, Log, TEXT("Calibration recommends tier change: %s -> %s"),
               *UEnum::GetValueAsString(DetectedHardwareTier),
               *UEnum::GetValueAsString(RecommendedTier));
        
        // Optionally auto-apply the recommended tier
        // ApplyHardwareTierSettings(RecommendedTier);
    }
}

// Platform-specific implementations
void UClimbingFrameRateManager::SetPlatformVSync(bool bEnable)
{
    if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
    {
        UserSettings->SetVSyncEnabled(bEnable);
        UserSettings->ApplySettings(false);
    }
}

void UClimbingFrameRateManager::SetPlatformFrameRateLimit(float FrameRate)
{
    if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
    {
        UserSettings->SetFrameRateLimit(FrameRate);
        UserSettings->ApplySettings(false);
    }
}

// Hardware detection implementations
FString UClimbingFrameRateManager::GetCPUBrandString() const
{
    return FPlatformMisc::GetCPUBrand();
}

FString UClimbingFrameRateManager::GetGPUBrandString() const
{
    return GRHIAdapterName;
}

int32 UClimbingFrameRateManager::GetSystemRAMGB() const
{
    const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
    return static_cast<int32>(MemoryConstants.TotalPhysical / (1024 * 1024 * 1024));
}

int32 UClimbingFrameRateManager::GetVRAMGB() const
{
    // This would require platform-specific implementation
    // For now, return a reasonable estimate based on GPU name
    FString GPUName = GetGPUBrandString();
    
    if (GPUName.Contains(TEXT("RTX 4090"))) return 24;
    if (GPUName.Contains(TEXT("RTX 4080"))) return 16;
    if (GPUName.Contains(TEXT("RTX 4070"))) return 12;
    if (GPUName.Contains(TEXT("RTX 3080"))) return 10;
    if (GPUName.Contains(TEXT("RTX 3070"))) return 8;
    if (GPUName.Contains(TEXT("RTX 3060"))) return 8;
    if (GPUName.Contains(TEXT("GTX 1660"))) return 6;
    if (GPUName.Contains(TEXT("GTX 1060"))) return 6;
    
    return 4; // Default assumption
}

// Placeholder implementations for remaining functions
void UClimbingFrameRateManager::AnalyzeFrameRatePerformance() {}
void UClimbingFrameRateManager::DetectPerformanceIssues() {}
void UClimbingFrameRateManager::AdjustRenderScale(float DeltaAdjustment) 
{
    AdaptiveState.CurrentRenderScale = FMath::Clamp(AdaptiveState.CurrentRenderScale + DeltaAdjustment, 0.5f, 2.0f);
}
void UClimbingFrameRateManager::AdjustTextureQuality(int32 DeltaLevels) 
{
    AdaptiveState.CurrentTextureQuality = FMath::Clamp(AdaptiveState.CurrentTextureQuality + DeltaLevels, 0, 4);
}
void UClimbingFrameRateManager::AdjustShadowQuality(int32 DeltaLevels) 
{
    AdaptiveState.CurrentShadowQuality = FMath::Clamp(AdaptiveState.CurrentShadowQuality + DeltaLevels, 0, 4);
}
void UClimbingFrameRateManager::AdjustEffectsQuality(int32 DeltaLevels) 
{
    AdaptiveState.CurrentEffectsQuality = FMath::Clamp(AdaptiveState.CurrentEffectsQuality + DeltaLevels, 0, 4);
}
void UClimbingFrameRateManager::ApplyNetworkSettings(const FHardwareTierSettings& Settings) {}
void UClimbingFrameRateManager::EnableDynamicResolution(bool bEnable) {}
void UClimbingFrameRateManager::SetDynamicResolutionRange(float MinScale, float MaxScale) {}
float UClimbingFrameRateManager::GetCurrentResolutionScale() const { return AdaptiveState.CurrentRenderScale; }
void UClimbingFrameRateManager::SetDisplayRefreshRate(int32 RefreshRate) {}
int32 UClimbingFrameRateManager::GetDisplayRefreshRate() const { return 60; }
void UClimbingFrameRateManager::ApplyPlatformOptimizations() {}
void UClimbingFrameRateManager::OptimizeForPowerUsage(bool bOptimizeForBattery) {}
void UClimbingFrameRateManager::SetThermalThrottling(bool bEnable) {}
void UClimbingFrameRateManager::SaveCurrentSettingsProfile(const FString& ProfileName) {}
void UClimbingFrameRateManager::LoadSettingsProfile(const FString& ProfileName) {}
TArray<FString> UClimbingFrameRateManager::GetAvailableSettingsProfiles() const { return TArray<FString>(); }