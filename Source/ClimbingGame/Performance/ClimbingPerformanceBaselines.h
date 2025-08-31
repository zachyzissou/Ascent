#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "../Physics/ClimbingBenchmarkSuite.h"
#include "ClimbingPerformanceBaselines.generated.h"

UENUM(BlueprintType)
enum class EPerformanceCategory : uint8
{
    RopePhysics      UMETA(DisplayName = "Rope Physics"),
    MultiplayerSync  UMETA(DisplayName = "Multiplayer Synchronization"),
    Rendering        UMETA(DisplayName = "Rendering Pipeline"),
    Memory           UMETA(DisplayName = "Memory Management"),
    Network          UMETA(DisplayName = "Network Performance"),
    Loading          UMETA(DisplayName = "Asset Loading"),
    Overall          UMETA(DisplayName = "Overall System")
};

UENUM(BlueprintType)
enum class EPerformanceTier : uint8
{
    Minimum      UMETA(DisplayName = "Minimum Specification"),
    Recommended  UMETA(DisplayName = "Recommended Specification"),
    High         UMETA(DisplayName = "High Performance"),
    Ultra        UMETA(DisplayName = "Ultra Performance")
};

USTRUCT(BlueprintType)
struct FPerformanceBaseline
{
    GENERATED_BODY()

    // Identification
    UPROPERTY(BlueprintReadOnly)
    FString BaselineName;

    UPROPERTY(BlueprintReadOnly)
    EPerformanceCategory Category;

    UPROPERTY(BlueprintReadOnly)
    EPerformanceTier Tier;

    UPROPERTY(BlueprintReadOnly)
    FDateTime EstablishedDate;

    // Target metrics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Targets")
    float TargetFPS = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Targets")
    float MinimumFPS = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frame Rate Targets")
    float MaxFrameTimeMs = 16.67f;

    // Physics targets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Targets")
    float MaxPhysicsTimeMs = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Targets")
    int32 MaxActiveRopes = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Targets")
    int32 MaxPhysicsConstraints = 200;

    // Memory targets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Targets")
    float MaxMemoryUsageMB = 2048.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Targets")
    float MaxGPUMemoryMB = 4096.0f;

    // Network targets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Targets")
    float MaxLatencyMs = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Targets")
    float MaxBandwidthKBps = 256.0f;

    // Loading targets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading Targets")
    float MaxLevelLoadTimeSeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading Targets")
    float MaxAssetStreamingTimeMs = 50.0f;

    // Tolerance settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolerance", meta = (ClampMin = "1.0", ClampMax = "50.0"))
    float TolerancePercentage = 10.0f;

    // Platform configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    FString PlatformName = TEXT("Windows");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    FString HardwareConfiguration;

    // Test configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Configuration")
    int32 TestPlayerCount = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Configuration")
    int32 TestRopeCount = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Configuration")
    float TestDurationSeconds = 60.0f;

    // Quality settings used for this baseline
    UPROPERTY(BlueprintReadOnly, Category = "Quality Settings")
    int32 TextureQuality = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Quality Settings")
    int32 ShadowQuality = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Quality Settings")
    int32 EffectsQuality = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Quality Settings")
    float RenderScale = 1.0f;

    // Validation methods
    bool IsWithinTolerance(const FBenchmarkResult& Result) const;
    float GetDeviationPercentage(const FBenchmarkResult& Result) const;
    FString GetComparisonReport(const FBenchmarkResult& Result) const;
};

USTRUCT(BlueprintType)
struct FPerformanceTarget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinAcceptable = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxAcceptable = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TolerancePercent = 10.0f;

    bool IsWithinRange(float TestValue) const
    {
        if (MaxAcceptable > 0.0f && TestValue > MaxAcceptable)
            return false;
        if (MinAcceptable > 0.0f && TestValue < MinAcceptable)
            return false;
        
        float Tolerance = Value * (TolerancePercent / 100.0f);
        return FMath::Abs(TestValue - Value) <= Tolerance;
    }

    float GetDeviationPercent(float TestValue) const
    {
        if (Value <= 0.0f) return 0.0f;
        return ((TestValue - Value) / Value) * 100.0f;
    }
};

USTRUCT(BlueprintType)
struct FPlatformBaselines
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PlatformName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baselines")
    TMap<EPerformanceTier, FPerformanceBaseline> TierBaselines;

    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    int32 BaselineCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    FDateTime LastUpdated;
};

USTRUCT(BlueprintType)
struct FBaselineValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bPassed = false;

    UPROPERTY(BlueprintReadOnly)
    FString TestName;

    UPROPERTY(BlueprintReadOnly)
    FString BaselineName;

    UPROPERTY(BlueprintReadOnly)
    EPerformanceCategory Category;

    UPROPERTY(BlueprintReadOnly)
    float DeviationPercentage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> FailureReasons;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> WarningMessages;

    UPROPERTY(BlueprintReadOnly)
    FDateTime ValidationTime;
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingPerformanceBaselines : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Baseline establishment
    UFUNCTION(BlueprintCallable, Category = "Performance Baselines", CallInEditor = true)
    void EstablishAllBaselines();

    UFUNCTION(BlueprintCallable, Category = "Performance Baselines")
    FPerformanceBaseline EstablishBaselineForCategory(EPerformanceCategory Category, EPerformanceTier Tier);

    UFUNCTION(BlueprintCallable, Category = "Performance Baselines")
    void EstablishCustomBaseline(const FString& BaselineName, const FPerformanceBaseline& Configuration);

    // Baseline validation
    UFUNCTION(BlueprintCallable, Category = "Performance Validation")
    FBaselineValidationResult ValidateAgainstBaseline(const FBenchmarkResult& TestResult, const FString& BaselineName);

    UFUNCTION(BlueprintCallable, Category = "Performance Validation")
    TArray<FBaselineValidationResult> ValidateFullSuite(const TArray<FBenchmarkResult>& TestResults);

    UFUNCTION(BlueprintCallable, Category = "Performance Validation")
    bool IsPerformanceRegression(const FBenchmarkResult& NewResult, const FBenchmarkResult& BaselineResult, float RegressionThreshold = 5.0f);

    // Baseline management
    UFUNCTION(BlueprintCallable, Category = "Baseline Management")
    void SaveBaselines();

    UFUNCTION(BlueprintCallable, Category = "Baseline Management")
    void LoadBaselines();

    UFUNCTION(BlueprintCallable, Category = "Baseline Management")
    void UpdateBaseline(const FString& BaselineName, const FPerformanceBaseline& UpdatedBaseline);

    UFUNCTION(BlueprintCallable, Category = "Baseline Management")
    bool RemoveBaseline(const FString& BaselineName);

    // Baseline queries
    UFUNCTION(BlueprintCallable, Category = "Baseline Queries", BlueprintPure)
    TArray<FString> GetAvailableBaselines() const;

    UFUNCTION(BlueprintCallable, Category = "Baseline Queries", BlueprintPure)
    TArray<FString> GetBaselinesForCategory(EPerformanceCategory Category) const;

    UFUNCTION(BlueprintCallable, Category = "Baseline Queries", BlueprintPure)
    TArray<FString> GetBaselinesForTier(EPerformanceTier Tier) const;

    UFUNCTION(BlueprintCallable, Category = "Baseline Queries", BlueprintPure)
    FPerformanceBaseline GetBaseline(const FString& BaselineName) const;

    UFUNCTION(BlueprintCallable, Category = "Baseline Queries", BlueprintPure)
    FPlatformBaselines GetPlatformBaselines(const FString& PlatformName) const;

    // Platform-specific operations
    UFUNCTION(BlueprintCallable, Category = "Platform Baselines")
    void EstablishPlatformBaselines(const FString& PlatformName);

    UFUNCTION(BlueprintCallable, Category = "Platform Baselines")
    void ComparePlatformPerformance(const FString& Platform1, const FString& Platform2, TArray<FString>& ComparisonReport);

    // Hardware tier detection and baseline recommendation
    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    EPerformanceTier DetectRecommendedTier() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    FPerformanceBaseline GetRecommendedBaseline() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    void CalibrateForCurrentHardware();

    // Reporting and analytics
    UFUNCTION(BlueprintCallable, Category = "Reporting")
    void GenerateBaselineReport(const FString& ReportName) const;

    UFUNCTION(BlueprintCallable, Category = "Reporting")
    void ExportBaselinesJSON(const FString& FilePath) const;

    UFUNCTION(BlueprintCallable, Category = "Reporting")
    void ImportBaselinesJSON(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "Reporting")
    FString GetBaselineComparisonReport(const FString& Baseline1, const FString& Baseline2) const;

    // CI/CD Integration
    UFUNCTION(BlueprintCallable, Category = "CI/CD Integration")
    bool ValidateForContinuousIntegration(const TArray<FBenchmarkResult>& Results, float FailureThreshold = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "CI/CD Integration")
    void SetCIBaseline(const FString& BaselineName);

    UFUNCTION(BlueprintCallable, Category = "CI/CD Integration")
    TArray<FBaselineValidationResult> GetCIValidationResults() const;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bAutoEstablishBaselines = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bAutoSaveBaselines = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FString BaselinesDirectory = TEXT("Saved/PerformanceBaselines/");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float DefaultRegressionThreshold = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 MaxBaselinesPerCategory = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnablePlatformSpecificBaselines = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Baseline Events")
    FSimpleMulticastDelegate OnBaselineEstablished;

    UPROPERTY(BlueprintAssignable, Category = "Baseline Events")
    FSimpleMulticastDelegate OnRegressionDetected;

    UPROPERTY(BlueprintAssignable, Category = "Baseline Events")
    FSimpleMulticastDelegate OnValidationCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Baseline Events")
    FSimpleMulticastDelegate OnBaselinesUpdated;

protected:
    // Internal data storage
    UPROPERTY()
    TMap<FString, FPerformanceBaseline> StoredBaselines;

    UPROPERTY()
    TMap<FString, FPlatformBaselines> PlatformSpecificBaselines;

    // Current validation state
    TArray<FBaselineValidationResult> LastValidationResults;
    FString CurrentCIBaseline;

    // Hardware detection cache
    mutable EPerformanceTier CachedRecommendedTier = EPerformanceTier::Recommended;
    mutable bool bTierCacheValid = false;

private:
    // Internal baseline operations
    void EstablishDefaultBaselines();
    FPerformanceBaseline CreateBaselineFromBenchmark(const FBenchmarkResult& Benchmark, EPerformanceCategory Category, EPerformanceTier Tier);
    void UpdateBaselineStatistics();

    // Hardware detection helpers
    EPerformanceTier DetectHardwareTier() const;
    FString GetSystemConfigurationString() const;
    float GetSystemPerformanceScore() const;

    // File operations
    FString GetBaselineFilePath(const FString& BaselineName) const;
    bool SaveBaselineToFile(const FString& BaselineName, const FPerformanceBaseline& Baseline);
    bool LoadBaselineFromFile(const FString& BaselineName, FPerformanceBaseline& OutBaseline);

    // Validation helpers
    FBaselineValidationResult ValidateSingleMetric(float TestValue, float BaselineValue, float Tolerance, const FString& MetricName) const;
    void AddValidationFailure(FBaselineValidationResult& Result, const FString& Reason) const;
    void AddValidationWarning(FBaselineValidationResult& Result, const FString& Warning) const;

    // Platform detection
    FString GetCurrentPlatformName() const;
    bool IsPlatformSupported(const FString& PlatformName) const;

    // Statistical analysis
    void CalculateBaselineStatistics();
    float CalculatePerformanceVariance(const TArray<FBenchmarkResult>& Results) const;
    void IdentifyPerformanceOutliers(TArray<FBenchmarkResult>& Results) const;
};