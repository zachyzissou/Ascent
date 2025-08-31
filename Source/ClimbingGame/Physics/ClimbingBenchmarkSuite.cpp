#include "ClimbingBenchmarkSuite.h"
#include "ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/CableComponent.h"
#include "Stats/Stats.h"
#include "RHI.h"
#include "TimerManager.h"

DECLARE_STATS_GROUP(TEXT("ClimbingBenchmarks"), STATGROUP_ClimbingBenchmarks, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Rope Physics Benchmark"), STAT_RopePhysicsBenchmark, STATGROUP_ClimbingBenchmarks);
DECLARE_CYCLE_STAT(TEXT("Multiplayer Benchmark"), STAT_MultiplayerBenchmark, STATGROUP_ClimbingBenchmarks);
DECLARE_CYCLE_STAT(TEXT("Memory Benchmark"), STAT_MemoryBenchmark, STATGROUP_ClimbingBenchmarks);

void UClimbingBenchmarkSuite::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Initialize default configurations
    DefaultRopePhysicsConfig.NumRopesToTest = 25;
    DefaultRopePhysicsConfig.RopeLength = 1000.0f;
    DefaultRopePhysicsConfig.RopeSegments = 32;
    DefaultRopePhysicsConfig.TestDuration = 30.0f;
    DefaultRopePhysicsConfig.TargetFPS = 60.0f;
    DefaultRopePhysicsConfig.MinimumFPS = 30.0f;
    DefaultRopePhysicsConfig.MaxPhysicsTimeMs = 5.0f;

    DefaultMultiplayerConfig.NumPlayersToSimulate = 4;
    DefaultMultiplayerConfig.TestDuration = 60.0f;
    DefaultMultiplayerConfig.RopesPerPlayer = 3;
    DefaultMultiplayerConfig.UpdateRate = 30.0f;
    DefaultMultiplayerConfig.MaxBandwidthKBps = 256.0f;
    DefaultMultiplayerConfig.MaxLatencyMs = 100.0f;

    DefaultMemoryConfig.TestDuration = 45.0f;
    DefaultMemoryConfig.AssetLoadCycles = 10;
    DefaultMemoryConfig.MaxMemoryMB = 2048.0f;

    // Initialize hardware tier specifications
    FHardwareTierSpecs MinimumSpecs;
    MinimumSpecs.TierName = TEXT("Minimum");
    MinimumSpecs.TargetFPS = 30.0f;
    MinimumSpecs.MinimumFPS = 20.0f;
    MinimumSpecs.MaxFrameTimeMs = 33.33f;
    MinimumSpecs.MaxPhysicsTimeMs = 10.0f;
    MinimumSpecs.MaxMemoryMB = 1024.0f;
    MinimumSpecs.MaxActiveRopes = 20;
    MinimumSpecs.TextureQuality = 1;
    MinimumSpecs.ShadowQuality = 1;
    MinimumSpecs.RenderScale = 0.75f;
    HardwareTierSpecs.Add(EHardwareTier::Minimum, MinimumSpecs);

    FHardwareTierSpecs RecommendedSpecs;
    RecommendedSpecs.TierName = TEXT("Recommended");
    RecommendedSpecs.TargetFPS = 60.0f;
    RecommendedSpecs.MinimumFPS = 30.0f;
    RecommendedSpecs.MaxFrameTimeMs = 16.67f;
    RecommendedSpecs.MaxPhysicsTimeMs = 5.0f;
    RecommendedSpecs.MaxMemoryMB = 2048.0f;
    RecommendedSpecs.MaxActiveRopes = 40;
    RecommendedSpecs.TextureQuality = 2;
    RecommendedSpecs.ShadowQuality = 2;
    RecommendedSpecs.RenderScale = 1.0f;
    HardwareTierSpecs.Add(EHardwareTier::Recommended, RecommendedSpecs);

    FHardwareTierSpecs HighSpecs;
    HighSpecs.TierName = TEXT("High");
    HighSpecs.TargetFPS = 90.0f;
    HighSpecs.MinimumFPS = 60.0f;
    HighSpecs.MaxFrameTimeMs = 11.11f;
    HighSpecs.MaxPhysicsTimeMs = 3.0f;
    HighSpecs.MaxMemoryMB = 4096.0f;
    HighSpecs.MaxActiveRopes = 60;
    HighSpecs.TextureQuality = 3;
    HighSpecs.ShadowQuality = 3;
    HighSpecs.RenderScale = 1.2f;
    HardwareTierSpecs.Add(EHardwareTier::High, HighSpecs);

    FHardwareTierSpecs UltraSpecs;
    UltraSpecs.TierName = TEXT("Ultra");
    UltraSpecs.TargetFPS = 120.0f;
    UltraSpecs.MinimumFPS = 90.0f;
    UltraSpecs.MaxFrameTimeMs = 8.33f;
    UltraSpecs.MaxPhysicsTimeMs = 2.0f;
    UltraSpecs.MaxMemoryMB = 8192.0f;
    UltraSpecs.MaxActiveRopes = 100;
    UltraSpecs.TextureQuality = 4;
    UltraSpecs.ShadowQuality = 4;
    UltraSpecs.RenderScale = 1.5f;
    HardwareTierSpecs.Add(EHardwareTier::Ultra, UltraSpecs);

    // Initialize performance buffers
    FrameTimeBuffer.Reserve(3600); // 1 minute at 60 FPS
    PhysicsTimeBuffer.Reserve(3600);
    MemoryUsageBuffer.Reserve(600); // 1 minute at 10 Hz
    NetworkBandwidthBuffer.Reserve(600);

    // Create benchmark results directory
    FString BenchmarkDir = FPaths::ProjectSavedDir() / BenchmarkResultsPath;
    IFileManager::Get().MakeDirectory(*BenchmarkDir, true);

    UE_LOG(LogTemp, Log, TEXT("ClimbingBenchmarkSuite initialized"));
}

void UClimbingBenchmarkSuite::Deinitialize()
{
    if (bIsMonitoringActive)
    {
        StopRealtimeMonitoring();
    }

    CleanupTestObjects();

    FrameTimeBuffer.Empty();
    PhysicsTimeBuffer.Empty();
    MemoryUsageBuffer.Empty();
    NetworkBandwidthBuffer.Empty();
    BaselineResults.Empty();

    Super::Deinitialize();
}

void UClimbingBenchmarkSuite::RunFullBenchmarkSuite()
{
    if (bIsBenchmarkRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("Benchmark already running"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Starting full benchmark suite"));
    OnBenchmarkStarted.Broadcast();

    TArray<FBenchmarkResult> Results;

    // Run rope physics benchmark
    UE_LOG(LogTemp, Log, TEXT("Running rope physics benchmark..."));
    FBenchmarkResult RopeResult = RunRopePhysicsBenchmark(DefaultRopePhysicsConfig);
    Results.Add(RopeResult);

    // Run multiplayer benchmark
    UE_LOG(LogTemp, Log, TEXT("Running multiplayer benchmark..."));
    FBenchmarkResult MultiplayerResult = RunMultiplayerBenchmark(DefaultMultiplayerConfig);
    Results.Add(MultiplayerResult);

    // Run memory benchmark
    UE_LOG(LogTemp, Log, TEXT("Running memory benchmark..."));
    FBenchmarkResult MemoryResult = RunMemoryBenchmark(DefaultMemoryConfig);
    Results.Add(MemoryResult);

    // Run frame rate benchmark for detected hardware tier
    EHardwareTier DetectedTier = DetectOptimalHardwareTier();
    UE_LOG(LogTemp, Log, TEXT("Running frame rate benchmark for %s tier..."), 
           *HardwareTierSpecs[DetectedTier].TierName);
    FBenchmarkResult FrameRateResult = RunFrameRateBenchmark(60.0f, DetectedTier);
    Results.Add(FrameRateResult);

    // Run loading time benchmark
    UE_LOG(LogTemp, Log, TEXT("Running loading time benchmark..."));
    FBenchmarkResult LoadingResult = RunLoadingTimeBenchmark();
    Results.Add(LoadingResult);

    // Save results
    if (bAutoSaveResults)
    {
        FString TestSuiteName = FString::Printf(TEXT("FullSuite_%s"), 
                                              *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
        SaveBenchmarkResults(Results, TestSuiteName);
    }

    // Generate comprehensive report
    if (bGenerateDetailedReports)
    {
        FString ReportName = FString::Printf(TEXT("BenchmarkReport_%s"), 
                                           *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
        GenerateBenchmarkReport(Results, ReportName);
    }

    OnBenchmarkCompleted.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Full benchmark suite completed"));
}

FBenchmarkResult UClimbingBenchmarkSuite::RunRopePhysicsBenchmark(const FRopePhysicsBenchmark& BenchmarkConfig)
{
    SCOPE_CYCLE_COUNTER(STAT_RopePhysicsBenchmark);

    FString TestName = FString::Printf(TEXT("RopePhysics_%dRopes_%dSegs"), 
                                     BenchmarkConfig.NumRopesToTest, 
                                     BenchmarkConfig.RopeSegments);

    auto TestLogic = [this, &BenchmarkConfig](float DeltaTime) {
        // Simulate rope physics under various conditions
        if (BenchmarkConfig.bTestWithMovement)
        {
            RunRopeMovementTest(DeltaTime);
        }
        
        if (BenchmarkConfig.bTestWithTension)
        {
            RunRopeTensionTest(DeltaTime);
        }
        
        if (BenchmarkConfig.bTestWithCollision)
        {
            RunRopeCollisionTest(DeltaTime);
        }
    };

    // Spawn test ropes
    SpawnTestRopes(BenchmarkConfig.NumRopesToTest, 
                   BenchmarkConfig.RopeLength, 
                   BenchmarkConfig.RopeSegments);

    FBenchmarkResult Result = ExecuteBenchmark(EBenchmarkType::RopePhysics, 
                                              TestName, 
                                              BenchmarkConfig.TestDuration, 
                                              TestLogic);

    // Evaluate pass/fail criteria
    Result.bPassedBenchmark = (Result.AverageFPS >= BenchmarkConfig.MinimumFPS) &&
                              (Result.AveragePhysicsTime <= BenchmarkConfig.MaxPhysicsTimeMs);

    if (!Result.bPassedBenchmark)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Failed: FPS=%.1f (min=%.1f), PhysicsTime=%.1fms (max=%.1fms)"),
            Result.AverageFPS, BenchmarkConfig.MinimumFPS,
            Result.AveragePhysicsTime, BenchmarkConfig.MaxPhysicsTimeMs);
    }

    CleanupTestObjects();
    return Result;
}

FBenchmarkResult UClimbingBenchmarkSuite::RunMultiplayerBenchmark(const FMultiplayerBenchmark& BenchmarkConfig)
{
    SCOPE_CYCLE_COUNTER(STAT_MultiplayerBenchmark);

    FString TestName = FString::Printf(TEXT("Multiplayer_%dPlayers_%dRopes"), 
                                     BenchmarkConfig.NumPlayersToSimulate,
                                     BenchmarkConfig.RopesPerPlayer);

    auto TestLogic = [this, &BenchmarkConfig](float DeltaTime) {
        SimulateMultiplayerActivity(DeltaTime);
    };

    // Setup multiplayer test scenario
    SpawnTestPlayers(BenchmarkConfig.NumPlayersToSimulate);
    SpawnTestRopes(BenchmarkConfig.NumPlayersToSimulate * BenchmarkConfig.RopesPerPlayer, 
                   1000.0f, 16);

    FBenchmarkResult Result = ExecuteBenchmark(EBenchmarkType::MultiplayerSync, 
                                              TestName, 
                                              BenchmarkConfig.TestDuration, 
                                              TestLogic);

    // Evaluate multiplayer-specific criteria
    Result.bPassedBenchmark = (Result.AverageNetworkBandwidth <= BenchmarkConfig.MaxBandwidthKBps) &&
                              (Result.AverageFPS >= 30.0f);

    if (!Result.bPassedBenchmark)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Failed: Bandwidth=%.1fKBps (max=%.1f), FPS=%.1f"),
            Result.AverageNetworkBandwidth, BenchmarkConfig.MaxBandwidthKBps,
            Result.AverageFPS);
    }

    CleanupTestObjects();
    return Result;
}

FBenchmarkResult UClimbingBenchmarkSuite::RunMemoryBenchmark(const FMemoryBenchmark& BenchmarkConfig)
{
    SCOPE_CYCLE_COUNTER(STAT_MemoryBenchmark);

    FString TestName = TEXT("MemoryUsage_LoadCycles");

    auto TestLogic = [this, &BenchmarkConfig](float DeltaTime) {
        TrackMemoryUsageDuringTest();
        
        if (BenchmarkConfig.bTestAssetStreaming)
        {
            TestAssetLoadingUnloading();
        }
        
        if (BenchmarkConfig.bTestGarbageCollection)
        {
            ForceMemoryOperations();
        }
    };

    FBenchmarkResult Result = ExecuteBenchmark(EBenchmarkType::MemoryUsage, 
                                              TestName, 
                                              BenchmarkConfig.TestDuration, 
                                              TestLogic);

    // Evaluate memory usage criteria
    Result.bPassedBenchmark = (Result.PeakMemoryMB <= BenchmarkConfig.MaxMemoryMB);

    if (!Result.bPassedBenchmark)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Failed: PeakMemory=%.1fMB (max=%.1fMB)"),
            Result.PeakMemoryMB, BenchmarkConfig.MaxMemoryMB);
    }

    return Result;
}

FBenchmarkResult UClimbingBenchmarkSuite::RunFrameRateBenchmark(float TestDuration, EHardwareTier TargetTier)
{
    const FHardwareTierSpecs& TierSpecs = HardwareTierSpecs[TargetTier];
    
    FString TestName = FString::Printf(TEXT("FrameRate_%s"), *TierSpecs.TierName);

    auto TestLogic = [this, &TierSpecs](float DeltaTime) {
        // Create a realistic climbing scenario
        SpawnTestRopes(TierSpecs.MaxActiveRopes / 2, 1000.0f, 16);
        SpawnTestPlayers(4);
        
        // Simulate typical gameplay
        SimulateMultiplayerActivity(DeltaTime);
    };

    FBenchmarkResult Result = ExecuteBenchmark(EBenchmarkType::FrameRate, 
                                              TestName, 
                                              TestDuration, 
                                              TestLogic);

    // Evaluate frame rate criteria for this tier
    Result.bPassedBenchmark = (Result.AverageFPS >= TierSpecs.MinimumFPS) &&
                              (Result.MinFPS >= TierSpecs.MinimumFPS * 0.8f);

    if (!Result.bPassedBenchmark)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Failed for %s tier: AvgFPS=%.1f (min=%.1f), MinFPS=%.1f"),
            *TierSpecs.TierName, Result.AverageFPS, TierSpecs.MinimumFPS, Result.MinFPS);
    }

    CleanupTestObjects();
    return Result;
}

FBenchmarkResult UClimbingBenchmarkSuite::RunLoadingTimeBenchmark()
{
    FString TestName = TEXT("LoadingTimes");

    double LoadStartTime = FPlatformTime::Seconds();
    
    // Simulate level loading
    // In a real implementation, this would trigger level streaming
    FPlatformProcess::Sleep(2.0f); // Simulated loading time

    double LoadEndTime = FPlatformTime::Seconds();
    double LoadingTime = LoadEndTime - LoadStartTime;

    FBenchmarkResult Result;
    Result.TestName = TestName;
    Result.BenchmarkType = EBenchmarkType::LoadingTimes;
    Result.TestDurationSeconds = static_cast<float>(LoadingTime);
    Result.TestTimestamp = FDateTime::Now();

    // For loading tests, we measure the loading time directly
    Result.AverageFrameTime = static_cast<float>(LoadingTime * 1000.0f); // Convert to ms
    Result.bPassedBenchmark = (LoadingTime < 5.0); // 5 second loading target

    if (!Result.bPassedBenchmark)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Loading time too long: %.2fs (target: <5.0s)"), LoadingTime);
    }

    return Result;
}

EHardwareTier UClimbingBenchmarkSuite::DetectOptimalHardwareTier()
{
    // Simple hardware classification based on system specs
    float SystemMemoryGB = GetSystemMemoryGB();
    FString GPUInfo = GetGPUInfo();

    if (SystemMemoryGB >= 16.0f && GPUInfo.Contains(TEXT("RTX")))
    {
        return EHardwareTier::Ultra;
    }
    else if (SystemMemoryGB >= 8.0f)
    {
        return EHardwareTier::High;
    }
    else if (SystemMemoryGB >= 4.0f)
    {
        return EHardwareTier::Recommended;
    }
    else
    {
        return EHardwareTier::Minimum;
    }
}

void UClimbingBenchmarkSuite::EstablishPerformanceBaselines()
{
    UE_LOG(LogTemp, Log, TEXT("Establishing performance baselines..."));

    // Run baseline tests for each hardware tier
    for (auto& TierPair : HardwareTierSpecs)
    {
        EHardwareTier Tier = TierPair.Key;
        const FHardwareTierSpecs& Specs = TierPair.Value;

        FString BaselineName = FString::Printf(TEXT("Baseline_%s"), *Specs.TierName);
        
        // Run a representative test for this tier
        FBenchmarkResult Baseline = RunFrameRateBenchmark(30.0f, Tier);
        Baseline.TestName = BaselineName;

        BaselineResults.Add(BaselineName, Baseline);

        // Save baseline to disk
        SaveResultToFile(Baseline, BaselineName + TEXT(".json"));
    }

    UE_LOG(LogTemp, Log, TEXT("Performance baselines established"));
}

void UClimbingBenchmarkSuite::SaveBenchmarkResults(const TArray<FBenchmarkResult>& Results, const FString& TestSuiteName)
{
    // Save individual results
    for (const FBenchmarkResult& Result : Results)
    {
        FString Filename = FString::Printf(TEXT("%s_%s.json"), *TestSuiteName, *Result.TestName);
        SaveResultToFile(Result, Filename);
    }

    // Save summary report
    ExportResultsToJSON(Results, TestSuiteName + TEXT("_Summary.json"));
    ExportResultsToCSV(Results, TestSuiteName + TEXT("_Summary.csv"));

    UE_LOG(LogTemp, Log, TEXT("Benchmark results saved: %s"), *TestSuiteName);
}

FBenchmarkResult UClimbingBenchmarkSuite::ExecuteBenchmark(EBenchmarkType Type, const FString& TestName, float Duration, TFunction<void(float)> TestLogic)
{
    bIsBenchmarkRunning = true;
    CurrentBenchmarkType = Type;
    BenchmarkStartTime = FDateTime::Now();

    // Clear buffers
    FrameTimeBuffer.Empty();
    PhysicsTimeBuffer.Empty();
    MemoryUsageBuffer.Empty();
    NetworkBandwidthBuffer.Empty();

    StartPerformanceMeasurement();

    double TestStartTime = FPlatformTime::Seconds();
    double TestEndTime = TestStartTime + Duration;

    // Main benchmark loop
    while (FPlatformTime::Seconds() < TestEndTime)
    {
        double FrameStartTime = FPlatformTime::Seconds();

        // Execute test-specific logic
        float DeltaTime = 1.0f / 60.0f; // Assume 60 FPS target
        TestLogic(DeltaTime);

        // Update performance measurements
        UpdatePerformanceMeasurement(DeltaTime);

        double FrameEndTime = FPlatformTime::Seconds();
        float FrameTime = static_cast<float>((FrameEndTime - FrameStartTime) * 1000.0f);
        FrameTimeBuffer.Add(FrameTime);

        // Limit frame rate to prevent runaway loop
        FPlatformProcess::Sleep(FMath::Max(0.0f, 0.016f - static_cast<float>(FrameEndTime - FrameStartTime)));
    }

    EndPerformanceMeasurement();

    // Calculate results
    FBenchmarkResult Result;
    Result.TestName = TestName;
    Result.BenchmarkType = Type;
    Result.TestDurationSeconds = Duration;
    Result.TestTimestamp = BenchmarkStartTime;

    if (FrameTimeBuffer.Num() > 0)
    {
        FrameTimeBuffer.Sort();
        Result.AverageFrameTime = FrameTimeBuffer.GetData()[FrameTimeBuffer.Num() / 2]; // Median
        Result.MinFrameTime = FrameTimeBuffer[0];
        Result.MaxFrameTime = FrameTimeBuffer.Last();
        Result.AverageFPS = 1000.0f / Result.AverageFrameTime;
        Result.MinFPS = 1000.0f / Result.MaxFrameTime;
        Result.MaxFPS = 1000.0f / Result.MinFrameTime;
    }

    if (MemoryUsageBuffer.Num() > 0)
    {
        Result.AverageMemoryMB = MemoryUsageBuffer.GetData()[MemoryUsageBuffer.Num() / 2];
        MemoryUsageBuffer.Sort();
        Result.PeakMemoryMB = MemoryUsageBuffer.Last();
    }

    if (PhysicsTimeBuffer.Num() > 0)
    {
        PhysicsTimeBuffer.Sort();
        Result.AveragePhysicsTime = PhysicsTimeBuffer.GetData()[PhysicsTimeBuffer.Num() / 2];
        Result.PeakPhysicsTime = PhysicsTimeBuffer.Last();
    }

    Result.MaxActiveRopes = TestRopes.Num();
    Result.AverageActiveRopes = TestRopes.Num();

    bIsBenchmarkRunning = false;
    return Result;
}

void UClimbingBenchmarkSuite::StartRealtimeMonitoring()
{
    if (bIsMonitoringActive)
    {
        return;
    }

    bIsMonitoringActive = true;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(MonitoringTimer, [this]() {
            if (UClimbingPerformanceManager* PerfManager = GetWorld()->GetSubsystem<UClimbingPerformanceManager>())
            {
                FPerformanceMetrics Metrics = PerfManager->GetCurrentMetrics();
                
                // Log performance data
                UE_LOG(LogTemp, Verbose, TEXT("Realtime Monitor: FPS=%.1f, Memory=%.1fMB, Physics=%.2fms, Ropes=%d"),
                       Metrics.CurrentFPS, Metrics.MemoryUsageMB, Metrics.PhysicsTime, Metrics.ActiveRopes);
                
                // Check for performance regressions
                // This could trigger alerts or automated responses
            }
        }, MonitoringInterval, true);
    }

    UE_LOG(LogTemp, Log, TEXT("Realtime performance monitoring started"));
}

void UClimbingBenchmarkSuite::StopRealtimeMonitoring()
{
    if (!bIsMonitoringActive)
    {
        return;
    }

    bIsMonitoringActive = false;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MonitoringTimer);
    }

    UE_LOG(LogTemp, Log, TEXT("Realtime performance monitoring stopped"));
}

void UClimbingBenchmarkSuite::SpawnTestRopes(int32 NumRopes, float RopeLength, int32 Segments)
{
    if (!GetWorld())
    {
        return;
    }

    CleanupTestObjects();

    for (int32 i = 0; i < NumRopes; ++i)
    {
        // Create rope actor
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = FName(*FString::Printf(TEXT("TestRope_%d"), i));
        
        AActor* RopeActor = GetWorld()->SpawnActor<AActor>(SpawnParams);
        if (!RopeActor)
        {
            continue;
        }

        // Add rope component
        UAdvancedRopeComponent* RopeComp = NewObject<UAdvancedRopeComponent>(RopeActor);
        RopeActor->AddOwnedComponent(RopeComp);

        if (RopeComp->CableComponent)
        {
            RopeComp->CableComponent->CableLength = RopeLength;
            RopeComp->CableComponent->NumSegments = Segments;
            RopeComp->CableComponent->SetSimulatePhysics(true);
        }

        // Position ropes in a grid
        int32 GridSize = FMath::CeilToInt(FMath::Sqrt(NumRopes));
        int32 Row = i / GridSize;
        int32 Col = i % GridSize;
        FVector Position(Col * 500.0f, Row * 500.0f, 1000.0f);
        RopeActor->SetActorLocation(Position);

        TestRopes.Add(RopeComp);
        TestObjects.Add(RopeActor);
    }

    UE_LOG(LogTemp, Log, TEXT("Spawned %d test ropes"), NumRopes);
}

void UClimbingBenchmarkSuite::CleanupTestObjects()
{
    for (AActor* Actor : TestObjects)
    {
        if (IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    TestObjects.Empty();
    TestRopes.Empty();
    TestPlayers.Empty();
}

FString UClimbingBenchmarkSuite::GetCPUInfo() const
{
    return FPlatformMisc::GetCPUBrand();
}

FString UClimbingBenchmarkSuite::GetGPUInfo() const
{
    return GRHIAdapterName;
}

float UClimbingBenchmarkSuite::GetSystemMemoryGB() const
{
    const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
    return static_cast<float>(MemoryConstants.TotalPhysical) / (1024.0f * 1024.0f * 1024.0f);
}

void UClimbingBenchmarkSuite::GenerateBenchmarkReport(const TArray<FBenchmarkResult>& Results, const FString& ReportName)
{
    FString ReportContent;
    ReportContent += FString::Printf(TEXT("# Climbing Game Benchmark Report\n"));
    ReportContent += FString::Printf(TEXT("Generated: %s\n\n"), *FDateTime::Now().ToString());
    
    // System information
    ReportContent += FString::Printf(TEXT("## System Information\n"));
    ReportContent += FString::Printf(TEXT("CPU: %s\n"), *GetCPUInfo());
    ReportContent += FString::Printf(TEXT("GPU: %s\n"), *GetGPUInfo());
    ReportContent += FString::Printf(TEXT("Memory: %.1f GB\n\n"), GetSystemMemoryGB());
    
    // Results summary
    ReportContent += FString::Printf(TEXT("## Benchmark Results\n\n"));
    
    for (const FBenchmarkResult& Result : Results)
    {
        ReportContent += FString::Printf(TEXT("### %s\n"), *Result.TestName);
        ReportContent += FString::Printf(TEXT("- **Status**: %s\n"), Result.bPassedBenchmark ? TEXT("PASSED") : TEXT("FAILED"));
        ReportContent += FString::Printf(TEXT("- **Duration**: %.1f seconds\n"), Result.TestDurationSeconds);
        ReportContent += FString::Printf(TEXT("- **Average FPS**: %.1f\n"), Result.AverageFPS);
        ReportContent += FString::Printf(TEXT("- **Min FPS**: %.1f\n"), Result.MinFPS);
        ReportContent += FString::Printf(TEXT("- **Max FPS**: %.1f\n"), Result.MaxFPS);
        ReportContent += FString::Printf(TEXT("- **Average Frame Time**: %.2f ms\n"), Result.AverageFrameTime);
        ReportContent += FString::Printf(TEXT("- **Average Memory**: %.1f MB\n"), Result.AverageMemoryMB);
        ReportContent += FString::Printf(TEXT("- **Peak Memory**: %.1f MB\n"), Result.PeakMemoryMB);
        ReportContent += FString::Printf(TEXT("- **Average Physics Time**: %.2f ms\n"), Result.AveragePhysicsTime);
        
        if (!Result.bPassedBenchmark)
        {
            ReportContent += FString::Printf(TEXT("- **Failure Reason**: %s\n"), *Result.FailureReason);
        }
        
        ReportContent += TEXT("\n");
    }

    // Save report
    FString ReportPath = GetBenchmarkFilePath(ReportName + TEXT(".md"));
    FFileHelper::SaveStringToFile(ReportContent, *ReportPath);
    
    UE_LOG(LogTemp, Log, TEXT("Benchmark report generated: %s"), *ReportPath);
}

bool UClimbingBenchmarkSuite::SaveResultToFile(const FBenchmarkResult& Result, const FString& Filename)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    
    JsonObject->SetStringField(TEXT("TestName"), Result.TestName);
    JsonObject->SetStringField(TEXT("BenchmarkType"), UEnum::GetValueAsString(Result.BenchmarkType));
    JsonObject->SetNumberField(TEXT("TestDurationSeconds"), Result.TestDurationSeconds);
    JsonObject->SetNumberField(TEXT("AverageFrameTime"), Result.AverageFrameTime);
    JsonObject->SetNumberField(TEXT("MinFrameTime"), Result.MinFrameTime);
    JsonObject->SetNumberField(TEXT("MaxFrameTime"), Result.MaxFrameTime);
    JsonObject->SetNumberField(TEXT("AverageFPS"), Result.AverageFPS);
    JsonObject->SetNumberField(TEXT("MinFPS"), Result.MinFPS);
    JsonObject->SetNumberField(TEXT("MaxFPS"), Result.MaxFPS);
    JsonObject->SetNumberField(TEXT("AverageMemoryMB"), Result.AverageMemoryMB);
    JsonObject->SetNumberField(TEXT("PeakMemoryMB"), Result.PeakMemoryMB);
    JsonObject->SetBoolField(TEXT("PassedBenchmark"), Result.bPassedBenchmark);
    JsonObject->SetStringField(TEXT("FailureReason"), Result.FailureReason);
    JsonObject->SetStringField(TEXT("Timestamp"), Result.TestTimestamp.ToString());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    FString FilePath = GetBenchmarkFilePath(Filename);
    return FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

FString UClimbingBenchmarkSuite::GetBenchmarkFilePath(const FString& Filename) const
{
    return FPaths::ProjectSavedDir() / BenchmarkResultsPath / Filename;
}

void UClimbingBenchmarkSuite::ExportResultsToCSV(const TArray<FBenchmarkResult>& Results, const FString& Filename)
{
    FString CSVContent;
    CSVContent += TEXT("TestName,BenchmarkType,Duration,AvgFPS,MinFPS,MaxFPS,AvgFrameTime,AvgMemory,PeakMemory,Passed,FailureReason\n");
    
    for (const FBenchmarkResult& Result : Results)
    {
        CSVContent += FString::Printf(TEXT("%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,\"%s\"\n"),
            *Result.TestName,
            *UEnum::GetValueAsString(Result.BenchmarkType),
            Result.TestDurationSeconds,
            Result.AverageFPS,
            Result.MinFPS,
            Result.MaxFPS,
            Result.AverageFrameTime,
            Result.AverageMemoryMB,
            Result.PeakMemoryMB,
            Result.bPassedBenchmark ? TEXT("TRUE") : TEXT("FALSE"),
            *Result.FailureReason
        );
    }
    
    FString FilePath = GetBenchmarkFilePath(Filename);
    FFileHelper::SaveStringToFile(CSVContent, *FilePath);
}

void UClimbingBenchmarkSuite::ExportResultsToJSON(const TArray<FBenchmarkResult>& Results, const FString& Filename)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    
    for (const FBenchmarkResult& Result : Results)
    {
        TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
        ResultObject->SetStringField(TEXT("TestName"), Result.TestName);
        ResultObject->SetNumberField(TEXT("AverageFPS"), Result.AverageFPS);
        ResultObject->SetNumberField(TEXT("AverageMemoryMB"), Result.AverageMemoryMB);
        ResultObject->SetBoolField(TEXT("PassedBenchmark"), Result.bPassedBenchmark);
        
        ResultsArray.Add(MakeShareable(new FJsonValueObject(ResultObject)));
    }
    
    RootObject->SetArrayField(TEXT("Results"), ResultsArray);
    RootObject->SetStringField(TEXT("GeneratedAt"), FDateTime::Now().ToString());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    FString FilePath = GetBenchmarkFilePath(Filename);
    FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

// Placeholder implementations for test logic functions
void UClimbingBenchmarkSuite::RunRopeMovementTest(float DeltaTime) 
{
    // Simulate rope movement by applying forces
    for (UAdvancedRopeComponent* Rope : TestRopes)
    {
        if (IsValid(Rope) && IsValid(Rope->CableComponent))
        {
            // Apply oscillating force to simulate wind or movement
            float Force = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0f) * 100.0f;
            // This would apply force to the rope physics
        }
    }
}

void UClimbingBenchmarkSuite::RunRopeTensionTest(float DeltaTime) 
{
    // Apply tension to ropes by constraining endpoints
    for (UAdvancedRopeComponent* Rope : TestRopes)
    {
        if (IsValid(Rope) && IsValid(Rope->CableComponent))
        {
            // Simulate tension by pulling endpoints
        }
    }
}

void UClimbingBenchmarkSuite::RunRopeCollisionTest(float DeltaTime) 
{
    // Enable collision testing between ropes and environment
}

void UClimbingBenchmarkSuite::SimulateMultiplayerActivity(float DeltaTime) 
{
    // Simulate network activity and player movement
}

void UClimbingBenchmarkSuite::SpawnTestPlayers(int32 NumPlayers) 
{
    // Spawn test player characters
}

void UClimbingBenchmarkSuite::TrackMemoryUsageDuringTest() 
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    float MemoryUsageMB = static_cast<float>(MemStats.UsedPhysical) / (1024.0f * 1024.0f);
    MemoryUsageBuffer.Add(MemoryUsageMB);
}

void UClimbingBenchmarkSuite::TestAssetLoadingUnloading() 
{
    // Test asset streaming performance
}

void UClimbingBenchmarkSuite::ForceMemoryOperations() 
{
    // Force garbage collection and memory operations
    GEngine->ForceGarbageCollection(true);
}

void UClimbingBenchmarkSuite::StartPerformanceMeasurement() 
{
    // Initialize performance measurement
}

void UClimbingBenchmarkSuite::UpdatePerformanceMeasurement(float DeltaTime) 
{
    // Update performance metrics during test
    if (UClimbingPerformanceManager* PerfManager = GetWorld()->GetSubsystem<UClimbingPerformanceManager>())
    {
        FPerformanceMetrics Metrics = PerfManager->GetCurrentMetrics();
        PhysicsTimeBuffer.Add(Metrics.PhysicsTime);
        
        // Simulate network bandwidth measurement
        NetworkBandwidthBuffer.Add(32.0f); // Placeholder
    }
}

void UClimbingBenchmarkSuite::EndPerformanceMeasurement() 
{
    // Finalize performance measurement
}