#include "ClimbingPerformanceBaselines.h"
#include "../Physics/ClimbingBenchmarkSuite.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HAL/PlatformMisc.h"
#include "RHI.h"

void UClimbingPerformanceBaselines::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Create baselines directory if it doesn't exist
    FString BaselineDir = FPaths::ProjectSavedDir() / BaselinesDirectory;
    IFileManager::Get().MakeDirectory(*BaselineDir, true);

    // Load existing baselines
    LoadBaselines();

    // Establish default baselines if none exist
    if (StoredBaselines.Num() == 0 && bAutoEstablishBaselines)
    {
        EstablishDefaultBaselines();
    }

    UE_LOG(LogTemp, Log, TEXT("ClimbingPerformanceBaselines initialized: %d baselines loaded"), StoredBaselines.Num());
}

void UClimbingPerformanceBaselines::Deinitialize()
{
    if (bAutoSaveBaselines)
    {
        SaveBaselines();
    }

    StoredBaselines.Empty();
    PlatformSpecificBaselines.Empty();
    LastValidationResults.Empty();

    Super::Deinitialize();
}

void UClimbingPerformanceBaselines::EstablishAllBaselines()
{
    UE_LOG(LogTemp, Log, TEXT("Establishing comprehensive performance baselines..."));

    if (UClimbingBenchmarkSuite* BenchmarkSuite = GetGameInstance()->GetSubsystem<UClimbingBenchmarkSuite>())
    {
        // Establish baselines for all categories and tiers
        TArray<EPerformanceCategory> Categories = {
            EPerformanceCategory::RopePhysics,
            EPerformanceCategory::MultiplayerSync,
            EPerformanceCategory::Rendering,
            EPerformanceCategory::Memory,
            EPerformanceCategory::Network,
            EPerformanceCategory::Loading
        };

        TArray<EPerformanceTier> Tiers = {
            EPerformanceTier::Minimum,
            EPerformanceTier::Recommended,
            EPerformanceTier::High,
            EPerformanceTier::Ultra
        };

        int32 BaselinesEstablished = 0;

        for (EPerformanceCategory Category : Categories)
        {
            for (EPerformanceTier Tier : Tiers)
            {
                FPerformanceBaseline NewBaseline = EstablishBaselineForCategory(Category, Tier);
                
                if (NewBaseline.BaselineName.Len() > 0)
                {
                    BaselinesEstablished++;
                }
            }
        }

        // Establish overall system baselines
        for (EPerformanceTier Tier : Tiers)
        {
            FPerformanceBaseline OverallBaseline = EstablishBaselineForCategory(EPerformanceCategory::Overall, Tier);
            if (OverallBaseline.BaselineName.Len() > 0)
            {
                BaselinesEstablished++;
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Established %d performance baselines"), BaselinesEstablished);
        OnBaselineEstablished.Broadcast();
    }
}

FPerformanceBaseline UClimbingPerformanceBaselines::EstablishBaselineForCategory(EPerformanceCategory Category, EPerformanceTier Tier)
{
    UClimbingBenchmarkSuite* BenchmarkSuite = GetGameInstance()->GetSubsystem<UClimbingBenchmarkSuite>();
    if (!BenchmarkSuite)
    {
        UE_LOG(LogTemp, Error, TEXT("BenchmarkSuite not available for baseline establishment"));
        return FPerformanceBaseline();
    }

    FString BaselineName = FString::Printf(TEXT("%s_%s_%s"), 
                                          GetCurrentPlatformName(),
                                          *UEnum::GetValueAsString(Category),
                                          *UEnum::GetValueAsString(Tier));

    UE_LOG(LogTemp, Log, TEXT("Establishing baseline: %s"), *BaselineName);

    FPerformanceBaseline NewBaseline;
    NewBaseline.BaselineName = BaselineName;
    NewBaseline.Category = Category;
    NewBaseline.Tier = Tier;
    NewBaseline.EstablishedDate = FDateTime::Now();
    NewBaseline.PlatformName = GetCurrentPlatformName();
    NewBaseline.HardwareConfiguration = GetSystemConfigurationString();

    // Configure test parameters based on tier
    switch (Tier)
    {
        case EPerformanceTier::Minimum:
            NewBaseline.TargetFPS = 30.0f;
            NewBaseline.MinimumFPS = 20.0f;
            NewBaseline.MaxFrameTimeMs = 33.33f;
            NewBaseline.MaxPhysicsTimeMs = 10.0f;
            NewBaseline.MaxMemoryUsageMB = 1024.0f;
            NewBaseline.MaxActiveRopes = 20;
            NewBaseline.TestRopeCount = 10;
            NewBaseline.TextureQuality = 1;
            NewBaseline.ShadowQuality = 1;
            NewBaseline.EffectsQuality = 1;
            NewBaseline.RenderScale = 0.75f;
            break;
        case EPerformanceTier::Recommended:
            NewBaseline.TargetFPS = 60.0f;
            NewBaseline.MinimumFPS = 30.0f;
            NewBaseline.MaxFrameTimeMs = 16.67f;
            NewBaseline.MaxPhysicsTimeMs = 5.0f;
            NewBaseline.MaxMemoryUsageMB = 2048.0f;
            NewBaseline.MaxActiveRopes = 40;
            NewBaseline.TestRopeCount = 25;
            NewBaseline.TextureQuality = 2;
            NewBaseline.ShadowQuality = 2;
            NewBaseline.EffectsQuality = 2;
            NewBaseline.RenderScale = 1.0f;
            break;
        case EPerformanceTier::High:
            NewBaseline.TargetFPS = 90.0f;
            NewBaseline.MinimumFPS = 60.0f;
            NewBaseline.MaxFrameTimeMs = 11.11f;
            NewBaseline.MaxPhysicsTimeMs = 3.0f;
            NewBaseline.MaxMemoryUsageMB = 4096.0f;
            NewBaseline.MaxActiveRopes = 60;
            NewBaseline.TestRopeCount = 40;
            NewBaseline.TextureQuality = 3;
            NewBaseline.ShadowQuality = 3;
            NewBaseline.EffectsQuality = 3;
            NewBaseline.RenderScale = 1.2f;
            break;
        case EPerformanceTier::Ultra:
            NewBaseline.TargetFPS = 120.0f;
            NewBaseline.MinimumFPS = 90.0f;
            NewBaseline.MaxFrameTimeMs = 8.33f;
            NewBaseline.MaxPhysicsTimeMs = 2.0f;
            NewBaseline.MaxMemoryUsageMB = 8192.0f;
            NewBaseline.MaxActiveRopes = 100;
            NewBaseline.TestRopeCount = 60;
            NewBaseline.TextureQuality = 4;
            NewBaseline.ShadowQuality = 4;
            NewBaseline.EffectsQuality = 4;
            NewBaseline.RenderScale = 1.5f;
            break;
    }

    // Run category-specific benchmarks to validate targets
    FBenchmarkResult ValidationResult;
    
    switch (Category)
    {
        case EPerformanceCategory::RopePhysics:
            {
                FRopePhysicsBenchmark RopeConfig;
                RopeConfig.NumRopesToTest = NewBaseline.TestRopeCount;
                RopeConfig.TargetFPS = NewBaseline.TargetFPS;
                RopeConfig.MinimumFPS = NewBaseline.MinimumFPS;
                RopeConfig.MaxPhysicsTimeMs = NewBaseline.MaxPhysicsTimeMs;
                RopeConfig.TestDuration = 30.0f;
                ValidationResult = BenchmarkSuite->RunRopePhysicsBenchmark(RopeConfig);
            }
            break;
            
        case EPerformanceCategory::MultiplayerSync:
            {
                FMultiplayerBenchmark MultiplayerConfig;
                MultiplayerConfig.NumPlayersToSimulate = NewBaseline.TestPlayerCount;
                MultiplayerConfig.RopesPerPlayer = FMath::Max(1, NewBaseline.TestRopeCount / NewBaseline.TestPlayerCount);
                MultiplayerConfig.MaxBandwidthKBps = NewBaseline.MaxBandwidthKBps;
                MultiplayerConfig.MaxLatencyMs = NewBaseline.MaxLatencyMs;
                ValidationResult = BenchmarkSuite->RunMultiplayerBenchmark(MultiplayerConfig);
            }
            break;
            
        case EPerformanceCategory::Memory:
            {
                FMemoryBenchmark MemoryConfig;
                MemoryConfig.MaxMemoryMB = NewBaseline.MaxMemoryUsageMB;
                MemoryConfig.TestDuration = NewBaseline.TestDurationSeconds;
                ValidationResult = BenchmarkSuite->RunMemoryBenchmark(MemoryConfig);
            }
            break;
            
        case EPerformanceCategory::Overall:
            // Run a comprehensive test for overall performance
            BenchmarkSuite->RunFullBenchmarkSuite();
            // Use the last comprehensive result as validation
            break;
            
        default:
            // For categories without specific benchmarks, create a frame rate test
            ValidationResult = BenchmarkSuite->RunFrameRateBenchmark(NewBaseline.TestDurationSeconds, 
                                                                   static_cast<EHardwareTier>(static_cast<uint8>(Tier)));
            break;
    }

    // Adjust baseline targets based on actual results
    if (ValidationResult.bPassedBenchmark && ValidationResult.AverageFPS > 0.0f)
    {
        // If we exceeded expectations, we can set more ambitious targets
        if (ValidationResult.AverageFPS > NewBaseline.TargetFPS * 1.2f)
        {
            NewBaseline.TargetFPS = ValidationResult.AverageFPS * 0.9f; // Set target to 90% of achieved
        }
        
        // Update other metrics based on test results
        if (ValidationResult.AveragePhysicsTime > 0.0f)
        {
            NewBaseline.MaxPhysicsTimeMs = ValidationResult.AveragePhysicsTime * 1.5f; // Allow 50% headroom
        }
        
        if (ValidationResult.PeakMemoryMB > 0.0f)
        {
            NewBaseline.MaxMemoryUsageMB = ValidationResult.PeakMemoryMB * 1.3f; // Allow 30% headroom
        }
    }
    else if (!ValidationResult.bPassedBenchmark)
    {
        // If we failed, reduce targets to achievable levels
        if (ValidationResult.AverageFPS > 0.0f && ValidationResult.AverageFPS < NewBaseline.TargetFPS)
        {
            NewBaseline.TargetFPS = ValidationResult.AverageFPS * 0.95f; // Set slightly below achieved
            NewBaseline.MinimumFPS = ValidationResult.MinFPS > 0.0f ? ValidationResult.MinFPS * 0.9f : NewBaseline.MinimumFPS * 0.8f;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Baseline validation failed for %s, adjusting targets"), *BaselineName);
    }

    // Store the new baseline
    StoredBaselines.Add(BaselineName, NewBaseline);

    // Update platform-specific baselines
    if (bEnablePlatformSpecificBaselines)
    {
        FPlatformBaselines* PlatformBaselines = PlatformSpecificBaselines.Find(NewBaseline.PlatformName);
        if (!PlatformBaselines)
        {
            FPlatformBaselines NewPlatformBaselines;
            NewPlatformBaselines.PlatformName = NewBaseline.PlatformName;
            NewPlatformBaselines.LastUpdated = FDateTime::Now();
            PlatformSpecificBaselines.Add(NewBaseline.PlatformName, NewPlatformBaselines);
            PlatformBaselines = &PlatformSpecificBaselines[NewBaseline.PlatformName];
        }
        
        PlatformBaselines->TierBaselines.Add(Tier, NewBaseline);
        PlatformBaselines->BaselineCount = PlatformBaselines->TierBaselines.Num();
        PlatformBaselines->LastUpdated = FDateTime::Now();
    }

    UE_LOG(LogTemp, Log, TEXT("Established baseline %s: Target=%.1fFPS, Memory=%.1fMB, Physics=%.1fms"), 
           *BaselineName, NewBaseline.TargetFPS, NewBaseline.MaxMemoryUsageMB, NewBaseline.MaxPhysicsTimeMs);

    return NewBaseline;
}

void UClimbingPerformanceBaselines::EstablishCustomBaseline(const FString& BaselineName, const FPerformanceBaseline& Configuration)
{
    if (BaselineName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot establish baseline with empty name"));
        return;
    }

    FPerformanceBaseline CustomBaseline = Configuration;
    CustomBaseline.BaselineName = BaselineName;
    CustomBaseline.EstablishedDate = FDateTime::Now();
    CustomBaseline.PlatformName = GetCurrentPlatformName();
    CustomBaseline.HardwareConfiguration = GetSystemConfigurationString();

    StoredBaselines.Add(BaselineName, CustomBaseline);

    UE_LOG(LogTemp, Log, TEXT("Established custom baseline: %s"), *BaselineName);
    OnBaselineEstablished.Broadcast();
}

FBaselineValidationResult UClimbingPerformanceBaselines::ValidateAgainstBaseline(const FBenchmarkResult& TestResult, const FString& BaselineName)
{
    FBaselineValidationResult ValidationResult;
    ValidationResult.TestName = TestResult.TestName;
    ValidationResult.BaselineName = BaselineName;
    ValidationResult.ValidationTime = FDateTime::Now();
    ValidationResult.bPassed = false;

    const FPerformanceBaseline* Baseline = StoredBaselines.Find(BaselineName);
    if (!Baseline)
    {
        AddValidationFailure(ValidationResult, FString::Printf(TEXT("Baseline '%s' not found"), *BaselineName));
        return ValidationResult;
    }

    ValidationResult.Category = Baseline->Category;
    float TotalDeviation = 0.0f;
    int32 MetricCount = 0;
    bool bAnyFailures = false;

    // Validate frame rate metrics
    if (TestResult.AverageFPS > 0.0f)
    {
        if (TestResult.AverageFPS < Baseline->MinimumFPS)
        {
            AddValidationFailure(ValidationResult, 
                FString::Printf(TEXT("Average FPS %.1f below minimum %.1f"), 
                               TestResult.AverageFPS, Baseline->MinimumFPS));
            bAnyFailures = true;
        }
        else if (TestResult.AverageFPS < Baseline->TargetFPS * (1.0f - Baseline->TolerancePercentage / 100.0f))
        {
            AddValidationWarning(ValidationResult,
                FString::Printf(TEXT("Average FPS %.1f below target %.1f (within tolerance)"),
                               TestResult.AverageFPS, Baseline->TargetFPS));
        }

        float FPSDeviation = ((TestResult.AverageFPS - Baseline->TargetFPS) / Baseline->TargetFPS) * 100.0f;
        TotalDeviation += FMath::Abs(FPSDeviation);
        MetricCount++;
    }

    // Validate frame time
    if (TestResult.AverageFrameTime > 0.0f && TestResult.AverageFrameTime > Baseline->MaxFrameTimeMs)
    {
        AddValidationFailure(ValidationResult,
            FString::Printf(TEXT("Average frame time %.2fms exceeds maximum %.2fms"),
                           TestResult.AverageFrameTime, Baseline->MaxFrameTimeMs));
        bAnyFailures = true;
    }

    // Validate physics time
    if (TestResult.AveragePhysicsTime > 0.0f && TestResult.AveragePhysicsTime > Baseline->MaxPhysicsTimeMs)
    {
        AddValidationFailure(ValidationResult,
            FString::Printf(TEXT("Average physics time %.2fms exceeds maximum %.2fms"),
                           TestResult.AveragePhysicsTime, Baseline->MaxPhysicsTimeMs));
        bAnyFailures = true;
    }

    // Validate memory usage
    if (TestResult.PeakMemoryMB > 0.0f && TestResult.PeakMemoryMB > Baseline->MaxMemoryUsageMB)
    {
        AddValidationFailure(ValidationResult,
            FString::Printf(TEXT("Peak memory usage %.1fMB exceeds maximum %.1fMB"),
                           TestResult.PeakMemoryMB, Baseline->MaxMemoryUsageMB));
        bAnyFailures = true;
    }

    // Validate network metrics if available
    if (TestResult.AverageNetworkBandwidth > 0.0f && TestResult.AverageNetworkBandwidth > Baseline->MaxBandwidthKBps)
    {
        AddValidationWarning(ValidationResult,
            FString::Printf(TEXT("Network bandwidth %.1fKBps exceeds baseline %.1fKBps"),
                           TestResult.AverageNetworkBandwidth, Baseline->MaxBandwidthKBps));
    }

    // Calculate overall deviation
    ValidationResult.DeviationPercentage = MetricCount > 0 ? TotalDeviation / MetricCount : 0.0f;
    ValidationResult.bPassed = !bAnyFailures;

    if (ValidationResult.bPassed)
    {
        UE_LOG(LogTemp, Log, TEXT("Validation PASSED: %s against baseline %s (Deviation: %.1f%%)"),
               *TestResult.TestName, *BaselineName, ValidationResult.DeviationPercentage);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Validation FAILED: %s against baseline %s (Deviation: %.1f%%)"),
               *TestResult.TestName, *BaselineName, ValidationResult.DeviationPercentage);
        OnRegressionDetected.Broadcast();
    }

    return ValidationResult;
}

TArray<FBaselineValidationResult> UClimbingPerformanceBaselines::ValidateFullSuite(const TArray<FBenchmarkResult>& TestResults)
{
    TArray<FBaselineValidationResult> ValidationResults;

    for (const FBenchmarkResult& TestResult : TestResults)
    {
        // Try to find a matching baseline for this test
        FString MatchingBaselineName;
        
        // First, try to match by exact test name
        for (const auto& BaselinePair : StoredBaselines)
        {
            if (TestResult.TestName.Contains(BaselinePair.Key) || BaselinePair.Key.Contains(TestResult.TestName))
            {
                MatchingBaselineName = BaselinePair.Key;
                break;
            }
        }

        // If no exact match, try to match by benchmark type
        if (MatchingBaselineName.IsEmpty())
        {
            EPerformanceCategory TargetCategory = EPerformanceCategory::Overall;
            
            switch (TestResult.BenchmarkType)
            {
                case EBenchmarkType::RopePhysics:
                    TargetCategory = EPerformanceCategory::RopePhysics;
                    break;
                case EBenchmarkType::MultiplayerSync:
                    TargetCategory = EPerformanceCategory::MultiplayerSync;
                    break;
                case EBenchmarkType::MemoryUsage:
                    TargetCategory = EPerformanceCategory::Memory;
                    break;
                case EBenchmarkType::NetworkBandwidth:
                    TargetCategory = EPerformanceCategory::Network;
                    break;
                case EBenchmarkType::LoadingTimes:
                    TargetCategory = EPerformanceCategory::Loading;
                    break;
                default:
                    TargetCategory = EPerformanceCategory::Overall;
                    break;
            }

            // Find baseline for detected hardware tier
            EPerformanceTier DetectedTier = DetectRecommendedTier();
            FString CategoryBaselineName = FString::Printf(TEXT("%s_%s_%s"), 
                                                          *GetCurrentPlatformName(),
                                                          *UEnum::GetValueAsString(TargetCategory),
                                                          *UEnum::GetValueAsString(DetectedTier));
            
            if (StoredBaselines.Contains(CategoryBaselineName))
            {
                MatchingBaselineName = CategoryBaselineName;
            }
        }

        if (!MatchingBaselineName.IsEmpty())
        {
            FBaselineValidationResult ValidationResult = ValidateAgainstBaseline(TestResult, MatchingBaselineName);
            ValidationResults.Add(ValidationResult);
        }
        else
        {
            FBaselineValidationResult NoBaselineResult;
            NoBaselineResult.TestName = TestResult.TestName;
            NoBaselineResult.BaselineName = TEXT("None Found");
            NoBaselineResult.bPassed = false;
            NoBaselineResult.ValidationTime = FDateTime::Now();
            AddValidationWarning(NoBaselineResult, TEXT("No matching baseline found for test"));
            ValidationResults.Add(NoBaselineResult);
        }
    }

    LastValidationResults = ValidationResults;
    OnValidationCompleted.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Full suite validation completed: %d tests, %d passed"), 
           ValidationResults.Num(), ValidationResults.FilterByPredicate([](const FBaselineValidationResult& Result) { return Result.bPassed; }).Num());

    return ValidationResults;
}

bool UClimbingPerformanceBaselines::IsPerformanceRegression(const FBenchmarkResult& NewResult, const FBenchmarkResult& BaselineResult, float RegressionThreshold)
{
    // Check for significant performance degradation
    if (NewResult.AverageFPS > 0.0f && BaselineResult.AverageFPS > 0.0f)
    {
        float FPSChange = ((NewResult.AverageFPS - BaselineResult.AverageFPS) / BaselineResult.AverageFPS) * 100.0f;
        if (FPSChange < -RegressionThreshold)
        {
            UE_LOG(LogTemp, Warning, TEXT("Performance regression detected: FPS decreased by %.1f%%"), -FPSChange);
            return true;
        }
    }

    // Check frame time regression
    if (NewResult.AverageFrameTime > 0.0f && BaselineResult.AverageFrameTime > 0.0f)
    {
        float FrameTimeChange = ((NewResult.AverageFrameTime - BaselineResult.AverageFrameTime) / BaselineResult.AverageFrameTime) * 100.0f;
        if (FrameTimeChange > RegressionThreshold)
        {
            UE_LOG(LogTemp, Warning, TEXT("Performance regression detected: Frame time increased by %.1f%%"), FrameTimeChange);
            return true;
        }
    }

    // Check memory regression
    if (NewResult.PeakMemoryMB > 0.0f && BaselineResult.PeakMemoryMB > 0.0f)
    {
        float MemoryChange = ((NewResult.PeakMemoryMB - BaselineResult.PeakMemoryMB) / BaselineResult.PeakMemoryMB) * 100.0f;
        if (MemoryChange > RegressionThreshold * 2.0f) // Allow more tolerance for memory
        {
            UE_LOG(LogTemp, Warning, TEXT("Memory regression detected: Usage increased by %.1f%%"), MemoryChange);
            return true;
        }
    }

    return false;
}

void UClimbingPerformanceBaselines::SaveBaselines()
{
    for (const auto& BaselinePair : StoredBaselines)
    {
        SaveBaselineToFile(BaselinePair.Key, BaselinePair.Value);
    }

    UE_LOG(LogTemp, Log, TEXT("Saved %d performance baselines"), StoredBaselines.Num());
}

void UClimbingPerformanceBaselines::LoadBaselines()
{
    FString BaselineDir = FPaths::ProjectSavedDir() / BaselinesDirectory;
    
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *(BaselineDir / TEXT("*.json")), true, false);

    int32 LoadedCount = 0;
    for (const FString& FileName : FoundFiles)
    {
        FString BaselineName = FPaths::GetBaseFilename(FileName);
        FPerformanceBaseline LoadedBaseline;
        
        if (LoadBaselineFromFile(BaselineName, LoadedBaseline))
        {
            StoredBaselines.Add(BaselineName, LoadedBaseline);
            LoadedCount++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Loaded %d performance baselines"), LoadedCount);
}

// Implementation continues with helper methods...

bool UClimbingPerformanceBaselines::ValidateForContinuousIntegration(const TArray<FBenchmarkResult>& Results, float FailureThreshold)
{
    TArray<FBaselineValidationResult> ValidationResults = ValidateFullSuite(Results);
    
    int32 FailedTests = 0;
    int32 TotalTests = ValidationResults.Num();
    
    for (const FBaselineValidationResult& Result : ValidationResults)
    {
        if (!Result.bPassed && Result.DeviationPercentage > FailureThreshold)
        {
            FailedTests++;
            UE_LOG(LogTemp, Error, TEXT("CI FAILURE: Test '%s' failed with %.1f%% deviation"), 
                   *Result.TestName, Result.DeviationPercentage);
        }
    }

    float FailureRate = (float)FailedTests / FMath::Max(1, TotalTests) * 100.0f;
    bool bCIPassed = FailureRate <= (FailureThreshold / 2.0f); // Half the failure threshold for CI pass rate

    UE_LOG(LogTemp, Log, TEXT("CI Validation Result: %s (%d/%d tests passed, %.1f%% failure rate)"),
           bCIPassed ? TEXT("PASSED") : TEXT("FAILED"), 
           TotalTests - FailedTests, TotalTests, FailureRate);

    return bCIPassed;
}

EPerformanceTier UClimbingPerformanceBaselines::DetectRecommendedTier() const
{
    if (bTierCacheValid)
    {
        return CachedRecommendedTier;
    }

    // Detect based on system specifications
    EPerformanceTier RecommendedTier = DetectHardwareTier();
    
    // Run a quick performance test to validate
    if (UClimbingBenchmarkSuite* BenchmarkSuite = GetGameInstance()->GetSubsystem<UClimbingBenchmarkSuite>())
    {
        FBenchmarkResult QuickTest = BenchmarkSuite->RunFrameRateBenchmark(10.0f, static_cast<EHardwareTier>(static_cast<uint8>(RecommendedTier)));
        
        // Adjust recommendation based on actual performance
        if (QuickTest.AverageFPS < 30.0f)
        {
            RecommendedTier = EPerformanceTier::Minimum;
        }
        else if (QuickTest.AverageFPS < 60.0f)
        {
            RecommendedTier = EPerformanceTier::Recommended;
        }
        else if (QuickTest.AverageFPS < 90.0f)
        {
            RecommendedTier = EPerformanceTier::High;
        }
        else
        {
            RecommendedTier = EPerformanceTier::Ultra;
        }
    }

    CachedRecommendedTier = RecommendedTier;
    bTierCacheValid = true;

    UE_LOG(LogTemp, Log, TEXT("Detected recommended performance tier: %s"), *UEnum::GetValueAsString(RecommendedTier));
    
    return RecommendedTier;
}

// Placeholder implementations for remaining methods
void UClimbingPerformanceBaselines::EstablishDefaultBaselines() 
{
    // Create basic default baselines for each tier
    EstablishAllBaselines();
}

EPerformanceTier UClimbingPerformanceBaselines::DetectHardwareTier() const
{
    // Simple hardware detection based on system specs
    const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
    int64 SystemMemoryGB = MemoryConstants.TotalPhysical / (1024 * 1024 * 1024);
    
    FString GPUName = GRHIAdapterName;
    
    if (SystemMemoryGB >= 32 && GPUName.Contains(TEXT("RTX 40")))
        return EPerformanceTier::Ultra;
    else if (SystemMemoryGB >= 16 && (GPUName.Contains(TEXT("RTX 30")) || GPUName.Contains(TEXT("RTX 20"))))
        return EPerformanceTier::High;
    else if (SystemMemoryGB >= 8)
        return EPerformanceTier::Recommended;
    else
        return EPerformanceTier::Minimum;
}

FString UClimbingPerformanceBaselines::GetCurrentPlatformName() const
{
    return FPlatformMisc::GetPlatformName();
}

FString UClimbingPerformanceBaselines::GetSystemConfigurationString() const
{
    const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
    int64 SystemMemoryGB = MemoryConstants.TotalPhysical / (1024 * 1024 * 1024);
    
    return FString::Printf(TEXT("CPU: %s, GPU: %s, RAM: %lldGB"), 
                          *FPlatformMisc::GetCPUBrand(),
                          *GRHIAdapterName,
                          SystemMemoryGB);
}

void UClimbingPerformanceBaselines::AddValidationFailure(FBaselineValidationResult& Result, const FString& Reason) const
{
    Result.FailureReasons.Add(Reason);
}

void UClimbingPerformanceBaselines::AddValidationWarning(FBaselineValidationResult& Result, const FString& Warning) const
{
    Result.WarningMessages.Add(Warning);
}

bool UClimbingPerformanceBaselines::SaveBaselineToFile(const FString& BaselineName, const FPerformanceBaseline& Baseline)
{
    FString FilePath = GetBaselineFilePath(BaselineName + TEXT(".json"));
    
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    
    // Serialize baseline to JSON
    JsonObject->SetStringField(TEXT("BaselineName"), Baseline.BaselineName);
    JsonObject->SetStringField(TEXT("Category"), UEnum::GetValueAsString(Baseline.Category));
    JsonObject->SetStringField(TEXT("Tier"), UEnum::GetValueAsString(Baseline.Tier));
    JsonObject->SetStringField(TEXT("EstablishedDate"), Baseline.EstablishedDate.ToString());
    JsonObject->SetNumberField(TEXT("TargetFPS"), Baseline.TargetFPS);
    JsonObject->SetNumberField(TEXT("MinimumFPS"), Baseline.MinimumFPS);
    JsonObject->SetNumberField(TEXT("MaxFrameTimeMs"), Baseline.MaxFrameTimeMs);
    JsonObject->SetNumberField(TEXT("MaxPhysicsTimeMs"), Baseline.MaxPhysicsTimeMs);
    JsonObject->SetNumberField(TEXT("MaxMemoryUsageMB"), Baseline.MaxMemoryUsageMB);
    JsonObject->SetNumberField(TEXT("MaxActiveRopes"), Baseline.MaxActiveRopes);
    JsonObject->SetStringField(TEXT("PlatformName"), Baseline.PlatformName);
    JsonObject->SetStringField(TEXT("HardwareConfiguration"), Baseline.HardwareConfiguration);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

bool UClimbingPerformanceBaselines::LoadBaselineFromFile(const FString& BaselineName, FPerformanceBaseline& OutBaseline)
{
    FString FilePath = GetBaselineFilePath(BaselineName + TEXT(".json"));
    FString JsonString;
    
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        return false;
    }
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    // Deserialize from JSON
    OutBaseline.BaselineName = JsonObject->GetStringField(TEXT("BaselineName"));
    OutBaseline.TargetFPS = JsonObject->GetNumberField(TEXT("TargetFPS"));
    OutBaseline.MinimumFPS = JsonObject->GetNumberField(TEXT("MinimumFPS"));
    OutBaseline.MaxFrameTimeMs = JsonObject->GetNumberField(TEXT("MaxFrameTimeMs"));
    OutBaseline.MaxPhysicsTimeMs = JsonObject->GetNumberField(TEXT("MaxPhysicsTimeMs"));
    OutBaseline.MaxMemoryUsageMB = JsonObject->GetNumberField(TEXT("MaxMemoryUsageMB"));
    OutBaseline.MaxActiveRopes = JsonObject->GetIntegerField(TEXT("MaxActiveRopes"));
    OutBaseline.PlatformName = JsonObject->GetStringField(TEXT("PlatformName"));
    OutBaseline.HardwareConfiguration = JsonObject->GetStringField(TEXT("HardwareConfiguration"));
    
    // Parse enums
    FString CategoryString = JsonObject->GetStringField(TEXT("Category"));
    UEnum* CategoryEnum = StaticEnum<EPerformanceCategory>();
    OutBaseline.Category = static_cast<EPerformanceCategory>(CategoryEnum->GetValueByNameString(CategoryString));
    
    FString TierString = JsonObject->GetStringField(TEXT("Tier"));
    UEnum* TierEnum = StaticEnum<EPerformanceTier>();
    OutBaseline.Tier = static_cast<EPerformanceTier>(TierEnum->GetValueByNameString(TierString));
    
    // Parse date
    FDateTime::Parse(JsonObject->GetStringField(TEXT("EstablishedDate")), OutBaseline.EstablishedDate);
    
    return true;
}

FString UClimbingPerformanceBaselines::GetBaselineFilePath(const FString& Filename) const
{
    return FPaths::ProjectSavedDir() / BaselinesDirectory / Filename;
}

// Placeholder implementations for remaining methods
TArray<FString> UClimbingPerformanceBaselines::GetAvailableBaselines() const 
{
    TArray<FString> Names;
    StoredBaselines.GetKeys(Names);
    return Names;
}
TArray<FString> UClimbingPerformanceBaselines::GetBaselinesForCategory(EPerformanceCategory Category) const { return TArray<FString>(); }
TArray<FString> UClimbingPerformanceBaselines::GetBaselinesForTier(EPerformanceTier Tier) const { return TArray<FString>(); }
FPerformanceBaseline UClimbingPerformanceBaselines::GetBaseline(const FString& BaselineName) const 
{
    const FPerformanceBaseline* Found = StoredBaselines.Find(BaselineName);
    return Found ? *Found : FPerformanceBaseline();
}
FPlatformBaselines UClimbingPerformanceBaselines::GetPlatformBaselines(const FString& PlatformName) const { return FPlatformBaselines(); }
void UClimbingPerformanceBaselines::UpdateBaseline(const FString& BaselineName, const FPerformanceBaseline& UpdatedBaseline) {}
bool UClimbingPerformanceBaselines::RemoveBaseline(const FString& BaselineName) { return StoredBaselines.Remove(BaselineName) > 0; }
void UClimbingPerformanceBaselines::EstablishPlatformBaselines(const FString& PlatformName) {}
void UClimbingPerformanceBaselines::ComparePlatformPerformance(const FString& Platform1, const FString& Platform2, TArray<FString>& ComparisonReport) {}
FPerformanceBaseline UClimbingPerformanceBaselines::GetRecommendedBaseline() const { return FPerformanceBaseline(); }
void UClimbingPerformanceBaselines::CalibrateForCurrentHardware() {}
void UClimbingPerformanceBaselines::GenerateBaselineReport(const FString& ReportName) const {}
void UClimbingPerformanceBaselines::ExportBaselinesJSON(const FString& FilePath) const {}
void UClimbingPerformanceBaselines::ImportBaselinesJSON(const FString& FilePath) {}
FString UClimbingPerformanceBaselines::GetBaselineComparisonReport(const FString& Baseline1, const FString& Baseline2) const { return FString(); }
void UClimbingPerformanceBaselines::SetCIBaseline(const FString& BaselineName) { CurrentCIBaseline = BaselineName; }
TArray<FBaselineValidationResult> UClimbingPerformanceBaselines::GetCIValidationResults() const { return LastValidationResults; }