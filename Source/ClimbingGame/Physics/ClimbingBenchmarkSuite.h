#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/DateTime.h"
#include "AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "../Tools/ClimbingToolBase.h"
#include "ClimbingBenchmarkSuite.generated.h"

UENUM(BlueprintType)
enum class EBenchmarkType : uint8
{
    RopePhysics         UMETA(DisplayName = "Rope Physics"),
    MultiplayerSync     UMETA(DisplayName = "Multiplayer Sync"),
    MemoryUsage         UMETA(DisplayName = "Memory Usage"),
    FrameRate           UMETA(DisplayName = "Frame Rate"),
    NetworkBandwidth    UMETA(DisplayName = "Network Bandwidth"),
    LoadingTimes        UMETA(DisplayName = "Loading Times"),
    FullSystem          UMETA(DisplayName = "Full System Test")
};

UENUM(BlueprintType)
enum class EHardwareTier : uint8
{
    Minimum     UMETA(DisplayName = "Minimum Spec"),
    Recommended UMETA(DisplayName = "Recommended Spec"),
    High        UMETA(DisplayName = "High End"),
    Ultra       UMETA(DisplayName = "Ultra High End")
};

USTRUCT(BlueprintType)
struct FBenchmarkResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString TestName;

    UPROPERTY(BlueprintReadOnly)
    EBenchmarkType BenchmarkType;

    UPROPERTY(BlueprintReadOnly)
    float TestDurationSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageFrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MinFrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MaxFrameTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageFPS = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MinFPS = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MaxFPS = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PeakMemoryMB = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AveragePhysicsTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PeakPhysicsTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AverageNetworkBandwidth = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PeakNetworkBandwidth = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 AverageActiveRopes = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxActiveRopes = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalPhysicsConstraints = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bPassedBenchmark = false;

    UPROPERTY(BlueprintReadOnly)
    FString FailureReason;

    UPROPERTY(BlueprintReadOnly)
    FDateTime TestTimestamp;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> PerformanceEvents;
};

USTRUCT(BlueprintType)
struct FRopePhysicsBenchmark
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumRopesToTest = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RopeLength = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RopeSegments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TestDuration = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestWithMovement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestWithTension = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestWithCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TargetFPS = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinimumFPS = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxPhysicsTimeMs = 5.0f;
};

USTRUCT(BlueprintType)
struct FMultiplayerBenchmark
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumPlayersToSimulate = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TestDuration = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RopesPerPlayer = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UpdateRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxBandwidthKBps = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxLatencyMs = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSimulatePacketLoss = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PacketLossPercentage = 1.0f;
};

USTRUCT(BlueprintType)
struct FMemoryBenchmark
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TestDuration = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AssetLoadCycles = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxMemoryMB = 2048.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestGarbageCollection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestAssetStreaming = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestMemoryLeaks = true;
};

USTRUCT(BlueprintType)
struct FHardwareTierSpecs
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString TierName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TargetFPS = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinimumFPS = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxFrameTimeMs = 16.67f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxPhysicsTimeMs = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxMemoryMB = 2048.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxActiveRopes = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRenderDistance = 10000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TextureQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ShadowQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RenderScale = 1.0f;
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingBenchmarkSuite : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Main benchmark functions
    UFUNCTION(BlueprintCallable, Category = "Benchmarking", CallInEditor = true)
    void RunFullBenchmarkSuite();

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    FBenchmarkResult RunRopePhysicsBenchmark(const FRopePhysicsBenchmark& BenchmarkConfig);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    FBenchmarkResult RunMultiplayerBenchmark(const FMultiplayerBenchmark& BenchmarkConfig);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    FBenchmarkResult RunMemoryBenchmark(const FMemoryBenchmark& BenchmarkConfig);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    FBenchmarkResult RunFrameRateBenchmark(float TestDuration, EHardwareTier TargetTier);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    FBenchmarkResult RunLoadingTimeBenchmark();

    // Hardware tier testing
    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    void TestHardwareTier(EHardwareTier Tier);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    EHardwareTier DetectOptimalHardwareTier();

    // Baseline establishment
    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    void EstablishPerformanceBaselines();

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    void SaveBenchmarkResults(const TArray<FBenchmarkResult>& Results, const FString& TestSuiteName);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    TArray<FBenchmarkResult> LoadBenchmarkResults(const FString& TestSuiteName);

    // Regression testing
    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    bool CompareWithBaseline(const FBenchmarkResult& NewResult, const FBenchmarkResult& BaselineResult, float TolerancePercent = 5.0f);

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    void RunRegressionTest(const FString& BaselineName);

    // Stress testing
    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    FBenchmarkResult RunStressTest(int32 MaxRopes, int32 MaxPlayers, float TestDuration);

    // Real-time monitoring
    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    void StartRealtimeMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    void StopRealtimeMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Benchmarking")
    bool IsMonitoringActive() const { return bIsMonitoringActive; }

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark Config")
    FRopePhysicsBenchmark DefaultRopePhysicsConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark Config")
    FMultiplayerBenchmark DefaultMultiplayerConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark Config")
    FMemoryBenchmark DefaultMemoryConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark Config")
    TMap<EHardwareTier, FHardwareTierSpecs> HardwareTierSpecs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoSaveResults = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FString BenchmarkResultsPath = TEXT("Saved/Benchmarks/");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MonitoringInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bGenerateDetailedReports = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Benchmark Events")
    FSimpleMulticastDelegate OnBenchmarkStarted;

    UPROPERTY(BlueprintAssignable, Category = "Benchmark Events")
    FSimpleMulticastDelegate OnBenchmarkCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Benchmark Events")
    FSimpleMulticastDelegate OnBenchmarkFailed;

    UPROPERTY(BlueprintAssignable, Category = "Benchmark Events")
    FSimpleMulticastDelegate OnRegressionDetected;

protected:
    // Internal benchmark execution
    FBenchmarkResult ExecuteBenchmark(EBenchmarkType Type, const FString& TestName, float Duration, TFunction<void(float)> TestLogic);

    // Rope physics test scenarios
    void RunRopePhysicsStressTest(float DeltaTime);
    void RunRopeMovementTest(float DeltaTime);
    void RunRopeTensionTest(float DeltaTime);
    void RunRopeCollisionTest(float DeltaTime);

    // Multiplayer simulation
    void SimulateMultiplayerActivity(float DeltaTime);
    void SimulatePlayerMovement(class ACharacter* Player, float DeltaTime);
    void SimulateRopeInteractions(float DeltaTime);

    // Memory tracking
    void TrackMemoryUsageDuringTest();
    void ForceMemoryOperations();
    void TestAssetLoadingUnloading();

    // Performance measurement
    void StartPerformanceMeasurement();
    void UpdatePerformanceMeasurement(float DeltaTime);
    void EndPerformanceMeasurement();

    // Hardware detection
    FString GetCPUInfo() const;
    FString GetGPUInfo() const;
    float GetSystemMemoryGB() const;
    EHardwareTier ClassifyHardware() const;

    // Reporting
    void GenerateBenchmarkReport(const TArray<FBenchmarkResult>& Results, const FString& ReportName);
    void ExportResultsToCSV(const TArray<FBenchmarkResult>& Results, const FString& Filename);
    void ExportResultsToJSON(const TArray<FBenchmarkResult>& Results, const FString& Filename);

    // Test object management
    void SpawnTestRopes(int32 NumRopes, float RopeLength, int32 Segments);
    void SpawnTestPlayers(int32 NumPlayers);
    void CleanupTestObjects();

private:
    // Test state
    bool bIsBenchmarkRunning = false;
    bool bIsMonitoringActive = false;
    EBenchmarkType CurrentBenchmarkType;
    FDateTime BenchmarkStartTime;

    // Performance tracking
    TArray<float> FrameTimeBuffer;
    TArray<float> PhysicsTimeBuffer;
    TArray<float> MemoryUsageBuffer;
    TArray<float> NetworkBandwidthBuffer;
    int32 SampleIndex = 0;

    // Test objects
    UPROPERTY()
    TArray<UAdvancedRopeComponent*> TestRopes;

    UPROPERTY()
    TArray<class ACharacter*> TestPlayers;

    UPROPERTY()
    TArray<class AActor*> TestObjects;

    // Baseline data
    TMap<FString, FBenchmarkResult> BaselineResults;

    // Monitoring timer
    FTimerHandle MonitoringTimer;

    // File I/O helpers
    bool SaveResultToFile(const FBenchmarkResult& Result, const FString& Filename);
    bool LoadResultFromFile(FBenchmarkResult& Result, const FString& Filename);
    FString GetBenchmarkFilePath(const FString& Filename) const;

    // Statistical analysis
    float CalculateStandardDeviation(const TArray<float>& Values) const;
    float CalculatePercentile(const TArray<float>& Values, float Percentile) const;
    void PerformStatisticalAnalysis(FBenchmarkResult& Result) const;
};