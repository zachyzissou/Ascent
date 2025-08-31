#include "ClimbingPhysicsDemoLevel.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "../Blueprints/ClimbingPhysicsBlueprintLibrary.h"

AClimbingPhysicsDemoLevel::AClimbingPhysicsDemoLevel()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0167f; // 60 FPS

    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    
    // Create visualization root
    VisualizationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualizationRoot"));
    VisualizationRoot->SetupAttachment(RootComponent);

    // Initialize default configuration
    Configuration.Scenario = EDemoScenario::AllSystems;
    Configuration.NumberOfClimbers = 2;
    Configuration.bEnablePhysicsVisualization = true;
    Configuration.bEnablePerformanceHUD = true;
    Configuration.bAutoStartDemo = true;
    Configuration.DemoTimeLimit = 300.0f;
    Configuration.bEnableAI = true;

    // Initialize arrays
    DemoClimbers.Reserve(MAX_DEMO_CLIMBERS);
    DemoRopes.Reserve(10);
    DemoAnchorSystems.Reserve(5);
    DemoTools.Reserve(20);
    ClimbingHolds.Reserve(MAX_CLIMBING_HOLDS);
    AnchorPoints.Reserve(10);
    PerformanceHistory.Reserve(1000);
}

void AClimbingPhysicsDemoLevel::BeginPlay()
{
    Super::BeginPlay();

    CachedWorld = GetWorld();
    PerformanceManager = CachedWorld ? CachedWorld->GetSubsystem<UClimbingPerformanceManager>() : nullptr;

    if (ValidateConfiguration())
    {
        ApplyConfiguration();
        
        if (Configuration.bAutoStartDemo)
        {
            StartDemo(Configuration.Scenario);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ClimbingPhysicsDemoLevel: Invalid configuration, demo not started"));
    }
}

void AClimbingPhysicsDemoLevel::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bDemoRunning)
    {
        DemoTimeElapsed += DeltaTime;

        // Update current scenario
        UpdateScenario(DeltaTime);

        // Update AI climbers if enabled
        if (bAIControlEnabled)
        {
            UpdateAIClimbers(DeltaTime);
        }

        // Update visualization
        if (Configuration.bEnablePhysicsVisualization)
        {
            UpdateVisualization(DeltaTime);
        }

        // Performance monitoring
        if (bPerformanceMonitoringActive)
        {
            LastPerformanceLogTime += DeltaTime;
            if (LastPerformanceLogTime >= PerformanceLogInterval)
            {
                PerformanceHistory.Add(GetCurrentPerformanceMetrics());
                LastPerformanceLogTime = 0.0f;
            }
        }

        // Check for demo completion
        if (Configuration.DemoTimeLimit > 0.0f && DemoTimeElapsed >= Configuration.DemoTimeLimit)
        {
            StopDemo();
            OnDemoComplete.Broadcast();
        }
        
        // Auto-transition scenarios in AllSystems mode
        if (CurrentScenario == EDemoScenario::AllSystems)
        {
            if (DemoTimeElapsed - ScenarioStartTime >= SCENARIO_TRANSITION_TIME)
            {
                TransitionToNextScenario();
            }
        }
    }
}

void AClimbingPhysicsDemoLevel::StartDemo(EDemoScenario Scenario)
{
    if (bDemoRunning)
    {
        StopDemo();
    }

    CurrentScenario = Scenario;
    bDemoRunning = true;
    DemoTimeElapsed = 0.0f;
    ScenarioStartTime = 0.0f;

    // Configure demo environment
    ConfigureDemoEnvironment();

    // Initialize the scenario
    InitializeScenario(Scenario);

    // Enable performance monitoring
    if (Configuration.bEnablePerformanceHUD)
    {
        StartPerformanceMonitoring();
    }

    // Enable physics visualization
    if (Configuration.bEnablePhysicsVisualization)
    {
        EnablePhysicsVisualization(true);
    }

    // Enable AI if configured
    if (Configuration.bEnableAI)
    {
        EnableAIControl(true);
    }

    OnDemoStarted.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Started climbing physics demo: %s"), 
           *UEnum::GetValueAsString(Scenario));
}

void AClimbingPhysicsDemoLevel::StopDemo()
{
    if (!bDemoRunning)
        return;

    bDemoRunning = false;
    
    CleanupScenario();
    StopPerformanceMonitoring();
    EnablePhysicsVisualization(false);
    EnableAIControl(false);

    OnDemoStopped.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Stopped climbing physics demo"));
}

void AClimbingPhysicsDemoLevel::ResetDemo()
{
    EDemoScenario PreviousScenario = CurrentScenario;
    StopDemo();
    CleanupDemoActors();
    StartDemo(PreviousScenario);
}

void AClimbingPhysicsDemoLevel::NextScenario()
{
    int32 CurrentIndex = static_cast<int32>(CurrentScenario);
    int32 MaxIndex = static_cast<int32>(EDemoScenario::AllSystems);
    
    EDemoScenario NewScenario = static_cast<EDemoScenario>((CurrentIndex + 1) % (MaxIndex + 1));
    
    if (bDemoRunning)
    {
        StartDemo(NewScenario);
    }
    else
    {
        CurrentScenario = NewScenario;
        OnScenarioChanged.Broadcast();
    }
}

void AClimbingPhysicsDemoLevel::PreviousScenario()
{
    int32 CurrentIndex = static_cast<int32>(CurrentScenario);
    int32 MaxIndex = static_cast<int32>(EDemoScenario::AllSystems);
    
    int32 NewIndex = (CurrentIndex - 1 + MaxIndex + 1) % (MaxIndex + 1);
    EDemoScenario NewScenario = static_cast<EDemoScenario>(NewIndex);
    
    if (bDemoRunning)
    {
        StartDemo(NewScenario);
    }
    else
    {
        CurrentScenario = NewScenario;
        OnScenarioChanged.Broadcast();
    }
}

// Individual demo scenarios
void AClimbingPhysicsDemoLevel::DemoBasicClimbing()
{
    StartDemo(EDemoScenario::BasicClimbing);
}

void AClimbingPhysicsDemoLevel::DemoRopePhysics()
{
    StartDemo(EDemoScenario::RopePhysics);
}

void AClimbingPhysicsDemoLevel::DemoAnchorSystems()
{
    StartDemo(EDemoScenario::AnchorSystems);
}

void AClimbingPhysicsDemoLevel::DemoFallMechanics()
{
    StartDemo(EDemoScenario::FallMechanics);
}

void AClimbingPhysicsDemoLevel::DemoCooperativeClimbing()
{
    StartDemo(EDemoScenario::CooperativeClimbing);
}

void AClimbingPhysicsDemoLevel::DemoPerformanceTest()
{
    StartDemo(EDemoScenario::PerformanceTest);
}

void AClimbingPhysicsDemoLevel::DemoAllSystems()
{
    StartDemo(EDemoScenario::AllSystems);
}

void AClimbingPhysicsDemoLevel::ConfigureDemoEnvironment()
{
    // Create the main climbing wall
    CreateClimbingWall(DEMO_WALL_HEIGHT, DEMO_WALL_WIDTH, 95.0f); // Slightly overhanging

    // Create climbing holds
    CreateClimbingHolds(50, 0.6f); // Medium difficulty route

    // Create anchor points
    CreateAnchorPoints(8);

    UE_LOG(LogTemp, Log, TEXT("Demo environment configured"));
}

void AClimbingPhysicsDemoLevel::SpawnDemoClimbers(int32 Count)
{
    Count = FMath::Clamp(Count, 1, MAX_DEMO_CLIMBERS);
    
    // Clean up existing climbers
    CleanupDemoActors();

    for (int32 i = 0; i < Count; ++i)
    {
        FVector SpawnLocation = GetActorLocation() + FVector(0, i * 200.0f, 100.0f);
        ACharacter* Climber = SpawnClimberAtLocation(SpawnLocation);
        
        if (Climber)
        {
            DemoClimbers.Add(Climber);
            SetupClimberPhysics(Climber);
            
            UE_LOG(LogTemp, Log, TEXT("Spawned demo climber %d at location %s"), 
                   i + 1, *SpawnLocation.ToString());
        }
    }
}

void AClimbingPhysicsDemoLevel::SetupDemoRopes()
{
    // Create demonstration ropes
    for (int32 i = 0; i < FMath::Min(DemoClimbers.Num(), AnchorPoints.Num() - 1); ++i)
    {
        if (i < AnchorPoints.Num() - 1)
        {
            FVector StartPoint = AnchorPoints[i]->GetActorLocation();
            FVector EndPoint = AnchorPoints[i + 1]->GetActorLocation();
            
            UAdvancedRopeComponent* Rope = CreateRopeBetweenPoints(StartPoint, EndPoint);
            if (Rope)
            {
                DemoRopes.Add(Rope);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Setup %d demo ropes"), DemoRopes.Num());
}

void AClimbingPhysicsDemoLevel::SetupDemoAnchors()
{
    // Create anchor systems at strategic points
    for (int32 i = 0; i < AnchorPoints.Num(); i += 2)
    {
        if (AnchorPoints.IsValidIndex(i))
        {
            UAnchorSystem* AnchorSys = CreateAnchorSystemAtLocation(AnchorPoints[i]->GetActorLocation());
            if (AnchorSys)
            {
                DemoAnchorSystems.Add(AnchorSys);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Setup %d demo anchor systems"), DemoAnchorSystems.Num());
}

void AClimbingPhysicsDemoLevel::EnablePhysicsVisualization(bool bEnable)
{
    Configuration.bEnablePhysicsVisualization = bEnable;
    
    if (bEnable)
    {
        // Enable debug drawing
        bShowRopeTension = true;
        bShowGripPoints = true;
        bShowAnchorLoads = true;
        bShowPerformanceMetrics = true;
    }
    else
    {
        // Disable all visualization
        bShowRopeTension = false;
        bShowGripPoints = false;
        bShowAnchorLoads = false;
        bShowPerformanceMetrics = false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Physics visualization %s"), bEnable ? TEXT("enabled") : TEXT("disabled"));
}

void AClimbingPhysicsDemoLevel::UpdateVisualization(float DeltaTime)
{
    if (!CachedWorld)
        return;

    // Clear previous debug draws (they persist for one frame)
    
    if (bShowRopeTension)
    {
        DrawRopeTensionVisualization();
    }
    
    if (bShowGripPoints)
    {
        DrawGripPointVisualization();
    }
    
    if (bShowAnchorLoads)
    {
        DrawAnchorLoadVisualization();
    }
    
    if (bShowPerformanceMetrics)
    {
        DrawPerformanceHUD();
    }
}

void AClimbingPhysicsDemoLevel::EnableAIControl(bool bEnable)
{
    bAIControlEnabled = bEnable;
    
    if (bEnable && AIClimbingPath.Num() == 0)
    {
        // Generate a simple climbing path
        TArray<FVector> Path;
        FVector StartPoint = GetActorLocation() + FVector(0, 0, 200);
        
        for (int32 i = 0; i < 10; ++i)
        {
            FVector Point = StartPoint + FVector(0, FMath::RandRange(-100, 100), i * 200);
            Path.Add(Point);
        }
        
        SetAIClimbingPath(Path);
    }
    
    // Initialize AI path indices for each climber
    AICurrentPathIndex.SetNum(DemoClimbers.Num());
    for (int32& Index : AICurrentPathIndex)
    {
        Index = 0;
    }
    
    UE_LOG(LogTemp, Log, TEXT("AI control %s"), bEnable ? TEXT("enabled") : TEXT("disabled"));
}

void AClimbingPhysicsDemoLevel::SetAIClimbingPath(const TArray<FVector>& PathPoints)
{
    AIClimbingPath = PathPoints;
    UE_LOG(LogTemp, Log, TEXT("Set AI climbing path with %d points"), PathPoints.Num());
}

void AClimbingPhysicsDemoLevel::StartPerformanceMonitoring()
{
    bPerformanceMonitoringActive = true;
    PerformanceHistory.Empty();
    LastPerformanceLogTime = 0.0f;
    
    if (PerformanceManager)
    {
        // Configure performance manager for demo
        PerformanceManager->bEnableAdaptiveQuality = false; // Disable for consistent results
    }
    
    UE_LOG(LogTemp, Log, TEXT("Started performance monitoring"));
}

void AClimbingPhysicsDemoLevel::StopPerformanceMonitoring()
{
    bPerformanceMonitoringActive = false;
    
    if (PerformanceHistory.Num() > 0)
    {
        LogPerformanceResults();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Stopped performance monitoring"));
}

FPerformanceMetrics AClimbingPhysicsDemoLevel::GetCurrentPerformanceMetrics() const
{
    if (PerformanceManager)
    {
        return PerformanceManager->GetCurrentMetrics();
    }
    return FPerformanceMetrics();
}

void AClimbingPhysicsDemoLevel::LogPerformanceResults()
{
    if (PerformanceHistory.Num() == 0)
        return;

    // Calculate averages
    float AvgFPS = 0.0f;
    float AvgFrameTime = 0.0f;
    float AvgPhysicsTime = 0.0f;
    
    for (const FPerformanceMetrics& Metrics : PerformanceHistory)
    {
        AvgFPS += Metrics.CurrentFPS;
        AvgFrameTime += Metrics.AverageFrameTime;
        AvgPhysicsTime += Metrics.PhysicsTime;
    }
    
    int32 SampleCount = PerformanceHistory.Num();
    AvgFPS /= SampleCount;
    AvgFrameTime /= SampleCount;
    AvgPhysicsTime /= SampleCount;

    UE_LOG(LogTemp, Warning, TEXT("=== CLIMBING PHYSICS DEMO PERFORMANCE RESULTS ==="));
    UE_LOG(LogTemp, Warning, TEXT("Demo Duration: %.1f seconds"), DemoTimeElapsed);
    UE_LOG(LogTemp, Warning, TEXT("Average FPS: %.1f"), AvgFPS);
    UE_LOG(LogTemp, Warning, TEXT("Average Frame Time: %.2f ms"), AvgFrameTime);
    UE_LOG(LogTemp, Warning, TEXT("Average Physics Time: %.2f ms"), AvgPhysicsTime);
    UE_LOG(LogTemp, Warning, TEXT("Active Ropes: %d"), DemoRopes.Num());
    UE_LOG(LogTemp, Warning, TEXT("Active Climbers: %d"), DemoClimbers.Num());
    UE_LOG(LogTemp, Warning, TEXT("Performance Samples: %d"), SampleCount);
    UE_LOG(LogTemp, Warning, TEXT("==============================================="));
}

float AClimbingPhysicsDemoLevel::GetDemoTimeRemaining() const
{
    if (Configuration.DemoTimeLimit <= 0.0f)
        return -1.0f; // No time limit
    
    return FMath::Max(0.0f, Configuration.DemoTimeLimit - DemoTimeElapsed);
}

// Implementation of internal methods

void AClimbingPhysicsDemoLevel::InitializeScenario(EDemoScenario Scenario)
{
    ScenarioStartTime = DemoTimeElapsed;
    
    switch (Scenario)
    {
        case EDemoScenario::BasicClimbing:
            SetupBasicClimbingScenario();
            break;
        case EDemoScenario::RopePhysics:
            SetupRopePhysicsScenario();
            break;
        case EDemoScenario::AnchorSystems:
            SetupAnchorSystemsScenario();
            break;
        case EDemoScenario::FallMechanics:
            SetupFallMechanicsScenario();
            break;
        case EDemoScenario::CooperativeClimbing:
            SetupCooperativeClimbingScenario();
            break;
        case EDemoScenario::PerformanceTest:
            SetupPerformanceTestScenario();
            break;
        case EDemoScenario::AllSystems:
            SetupAllSystemsScenario();
            break;
    }
}

void AClimbingPhysicsDemoLevel::UpdateScenario(float DeltaTime)
{
    switch (CurrentScenario)
    {
        case EDemoScenario::BasicClimbing:
            UpdateBasicClimbing(DeltaTime);
            break;
        case EDemoScenario::RopePhysics:
            UpdateRopePhysics(DeltaTime);
            break;
        case EDemoScenario::AnchorSystems:
            UpdateAnchorSystems(DeltaTime);
            break;
        case EDemoScenario::FallMechanics:
            UpdateFallMechanics(DeltaTime);
            break;
        case EDemoScenario::CooperativeClimbing:
            UpdateCooperativeClimbing(DeltaTime);
            break;
        case EDemoScenario::PerformanceTest:
            UpdatePerformanceTest(DeltaTime);
            break;
        case EDemoScenario::AllSystems:
            UpdateAllSystems(DeltaTime);
            break;
    }
}

void AClimbingPhysicsDemoLevel::CleanupScenario()
{
    // Scenario-specific cleanup would go here
    // For now, just general cleanup
}

bool AClimbingPhysicsDemoLevel::ValidateConfiguration() const
{
    if (Configuration.NumberOfClimbers < 1 || Configuration.NumberOfClimbers > MAX_DEMO_CLIMBERS)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid number of climbers: %d"), Configuration.NumberOfClimbers);
        return false;
    }
    
    if (Configuration.DemoTimeLimit < 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid demo time limit: %f"), Configuration.DemoTimeLimit);
        return false;
    }
    
    return true;
}

void AClimbingPhysicsDemoLevel::ApplyConfiguration()
{
    // Apply configuration settings
    SpawnDemoClimbers(Configuration.NumberOfClimbers);
    
    if (Configuration.bEnablePhysicsVisualization)
    {
        EnablePhysicsVisualization(true);
    }
    
    if (Configuration.bEnableAI)
    {
        EnableAIControl(true);
    }
}

// Placeholder implementations for demonstration scenarios
void AClimbingPhysicsDemoLevel::SetupBasicClimbingScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up Basic Climbing scenario"));
    // Basic climbing setup - single climber, simple route
}

void AClimbingPhysicsDemoLevel::SetupRopePhysicsScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up Rope Physics scenario"));
    SetupDemoRopes();
    // Focus on rope physics demonstration
}

void AClimbingPhysicsDemoLevel::SetupAnchorSystemsScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up Anchor Systems scenario"));
    SetupDemoAnchors();
    // Demonstrate anchor system load sharing
}

void AClimbingPhysicsDemoLevel::SetupFallMechanicsScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up Fall Mechanics scenario"));
    // Set up controlled fall scenarios
}

void AClimbingPhysicsDemoLevel::SetupCooperativeClimbingScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up Cooperative Climbing scenario"));
    SetupDemoRopes();
    SetupDemoAnchors();
    // Multi-player cooperative setup
}

void AClimbingPhysicsDemoLevel::SetupPerformanceTestScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up Performance Test scenario"));
    // Stress test with maximum objects
    SpawnDemoClimbers(MAX_DEMO_CLIMBERS);
    SetupDemoRopes();
    SetupDemoAnchors();
}

void AClimbingPhysicsDemoLevel::SetupAllSystemsScenario()
{
    UE_LOG(LogTemp, Log, TEXT("Setting up All Systems scenario"));
    SetupDemoRopes();
    SetupDemoAnchors();
    SetupDemoTools();
    // Comprehensive demonstration
}

// Simplified implementations for demo purposes
void AClimbingPhysicsDemoLevel::UpdateBasicClimbing(float DeltaTime) { /* Implementation */ }
void AClimbingPhysicsDemoLevel::UpdateRopePhysics(float DeltaTime) { /* Implementation */ }
void AClimbingPhysicsDemoLevel::UpdateAnchorSystems(float DeltaTime) { /* Implementation */ }
void AClimbingPhysicsDemoLevel::UpdateFallMechanics(float DeltaTime) { /* Implementation */ }
void AClimbingPhysicsDemoLevel::UpdateCooperativeClimbing(float DeltaTime) { /* Implementation */ }
void AClimbingPhysicsDemoLevel::UpdatePerformanceTest(float DeltaTime) { /* Implementation */ }
void AClimbingPhysicsDemoLevel::UpdateAllSystems(float DeltaTime) { /* Implementation */ }

ACharacter* AClimbingPhysicsDemoLevel::SpawnClimberAtLocation(const FVector& Location)
{
    // Simplified climber spawning - would use proper character class in full implementation
    return nullptr; // Placeholder
}

UAdvancedRopeComponent* AClimbingPhysicsDemoLevel::CreateRopeBetweenPoints(const FVector& Start, const FVector& End)
{
    // Simplified rope creation - placeholder
    return nullptr;
}

UAnchorSystem* AClimbingPhysicsDemoLevel::CreateAnchorSystemAtLocation(const FVector& Location)
{
    // Simplified anchor system creation - placeholder
    return nullptr;
}

void AClimbingPhysicsDemoLevel::SetupClimberPhysics(ACharacter* Climber)
{
    if (Climber)
    {
        UClimbingPhysicsBlueprintLibrary::InitializeClimbingPhysics(Climber);
    }
}

void AClimbingPhysicsDemoLevel::CleanupDemoActors()
{
    // Clean up spawned actors
    for (ACharacter* Climber : DemoClimbers)
    {
        if (IsValid(Climber))
        {
            Climber->Destroy();
        }
    }
    DemoClimbers.Empty();
    
    DemoRopes.Empty();
    DemoAnchorSystems.Empty();
    DemoTools.Empty();
}

void AClimbingPhysicsDemoLevel::CreateClimbingWall(float Height, float Width, float Angle)
{
    // Create climbing wall geometry - simplified implementation
    UE_LOG(LogTemp, Log, TEXT("Created climbing wall: %.0fx%.0f at %.0f degrees"), Width, Height, Angle);
}

void AClimbingPhysicsDemoLevel::CreateClimbingHolds(int32 Count, float Difficulty)
{
    UE_LOG(LogTemp, Log, TEXT("Created %d climbing holds at difficulty %.1f"), Count, Difficulty);
}

void AClimbingPhysicsDemoLevel::CreateAnchorPoints(int32 Count)
{
    UE_LOG(LogTemp, Log, TEXT("Created %d anchor points"), Count);
}

void AClimbingPhysicsDemoLevel::DrawRopeTensionVisualization()
{
    if (!CachedWorld)
        return;

    for (UAdvancedRopeComponent* Rope : DemoRopes)
    {
        if (IsValid(Rope))
        {
            float Tension = Rope->CalculateCurrentTension();
            FColor TensionColor = FColor::Green;
            
            if (Tension > 5000.0f)
                TensionColor = FColor::Yellow;
            if (Tension > 10000.0f)
                TensionColor = FColor::Red;
                
            // Draw rope tension visualization
            TArray<FVector> RopePoints = Rope->GetRopeSegmentPositions();
            for (int32 i = 0; i < RopePoints.Num() - 1; ++i)
            {
                DrawDebugLine(CachedWorld, RopePoints[i], RopePoints[i + 1], 
                             TensionColor, false, -1.0f, 0, 2.0f);
            }
        }
    }
}

void AClimbingPhysicsDemoLevel::DrawGripPointVisualization()
{
    // Draw grip point visualization for climbers
    for (ACharacter* Climber : DemoClimbers)
    {
        if (IsValid(Climber))
        {
            UAdvancedClimbingComponent* ClimbingComp = Climber->FindComponentByClass<UAdvancedClimbingComponent>();
            if (ClimbingComp)
            {
                TArray<FGripPoint> Grips = ClimbingComp->FindNearbyGrips(200.0f);
                for (const FGripPoint& Grip : Grips)
                {
                    FColor GripColor = FColor::Blue;
                    if (Grip.bIsActive)
                        GripColor = FColor::Green;
                        
                    DrawDebugSphere(CachedWorld, Grip.Location, 5.0f, 8, GripColor, false, -1.0f, 0, 1.0f);
                }
            }
        }
    }
}

void AClimbingPhysicsDemoLevel::DrawAnchorLoadVisualization()
{
    // Draw anchor load visualization
    for (UAnchorSystem* AnchorSys : DemoAnchorSystems)
    {
        if (IsValid(AnchorSys))
        {
            float SafetyFactor = AnchorSys->GetSystemSafetyFactor();
            FColor LoadColor = FColor::Green;
            
            if (SafetyFactor < 3.0f)
                LoadColor = FColor::Yellow;
            if (SafetyFactor < 2.0f)
                LoadColor = FColor::Red;
                
            // Draw anchor system visualization
            FVector SystemLocation = AnchorSys->GetSystemCenterOfLoad();
            DrawDebugSphere(CachedWorld, SystemLocation, 10.0f, 12, LoadColor, false, -1.0f, 0, 2.0f);
        }
    }
}

void AClimbingPhysicsDemoLevel::DrawPerformanceHUD()
{
    if (!bShowPerformanceMetrics)
        return;

    FPerformanceMetrics Metrics = GetCurrentPerformanceMetrics();
    
    // Draw performance metrics on screen
    FString PerformanceText = FString::Printf(
        TEXT("FPS: %.1f | Frame: %.1fms | Physics: %.1fms | Ropes: %d | Climbers: %d"),
        Metrics.CurrentFPS, Metrics.AverageFrameTime, Metrics.PhysicsTime,
        DemoRopes.Num(), DemoClimbers.Num()
    );
    
    // This would be implemented with proper HUD rendering in a full implementation
    UE_LOG(LogTemp, VeryVerbose, TEXT("Performance: %s"), *PerformanceText);
}

void AClimbingPhysicsDemoLevel::UpdateAIClimbers(float DeltaTime)
{
    for (int32 i = 0; i < DemoClimbers.Num(); ++i)
    {
        if (DemoClimbers.IsValidIndex(i) && IsValid(DemoClimbers[i]))
        {
            ProcessAIClimberMovement(DemoClimbers[i], i, DeltaTime);
        }
    }
}

void AClimbingPhysicsDemoLevel::ProcessAIClimberMovement(ACharacter* Climber, int32 ClimberIndex, float DeltaTime)
{
    if (!Climber || !AIClimbingPath.IsValidIndex(ClimberIndex))
        return;

    FVector TargetLocation = GetNextAITarget(ClimberIndex);
    FVector CurrentLocation = Climber->GetActorLocation();
    
    // Simple AI movement towards target
    if (FVector::Dist(CurrentLocation, TargetLocation) < 50.0f)
    {
        // Move to next target
        if (AICurrentPathIndex.IsValidIndex(ClimberIndex))
        {
            AICurrentPathIndex[ClimberIndex] = (AICurrentPathIndex[ClimberIndex] + 1) % AIClimbingPath.Num();
        }
    }
    
    // AI climbing logic would be implemented here
}

FVector AClimbingPhysicsDemoLevel::GetNextAITarget(int32 ClimberIndex)
{
    if (AICurrentPathIndex.IsValidIndex(ClimberIndex) && 
        AIClimbingPath.IsValidIndex(AICurrentPathIndex[ClimberIndex]))
    {
        return AIClimbingPath[AICurrentPathIndex[ClimberIndex]];
    }
    
    return GetActorLocation();
}

void AClimbingPhysicsDemoLevel::TransitionToNextScenario()
{
    // In AllSystems mode, cycle through individual scenarios
    static TArray<EDemoScenario> ScenarioOrder = {
        EDemoScenario::BasicClimbing,
        EDemoScenario::RopePhysics,
        EDemoScenario::AnchorSystems,
        EDemoScenario::FallMechanics,
        EDemoScenario::CooperativeClimbing,
        EDemoScenario::PerformanceTest
    };
    
    static int32 CurrentScenarioIndex = 0;
    
    if (ScenarioOrder.IsValidIndex(CurrentScenarioIndex))
    {
        InitializeScenario(ScenarioOrder[CurrentScenarioIndex]);
        CurrentScenarioIndex = (CurrentScenarioIndex + 1) % ScenarioOrder.Num();
        ScenarioStartTime = DemoTimeElapsed;
        
        UE_LOG(LogTemp, Log, TEXT("Transitioned to scenario: %s"), 
               *UEnum::GetValueAsString(ScenarioOrder[(CurrentScenarioIndex - 1 + ScenarioOrder.Num()) % ScenarioOrder.Num()]));
    }
}

void AClimbingPhysicsDemoLevel::SetupDemoTools()
{
    UE_LOG(LogTemp, Log, TEXT("Setup demo tools"));
    // Tool setup implementation would go here
}

void AClimbingPhysicsDemoLevel::CreateDemoRoute(float Height, float Difficulty)
{
    UE_LOG(LogTemp, Log, TEXT("Created demo route: %.0fm height, %.1f difficulty"), Height * 0.01f, Difficulty);
    // Route creation implementation
}

void AClimbingPhysicsDemoLevel::MakeAIPerformAction(int32 ClimberIndex, const FString& Action)
{
    UE_LOG(LogTemp, Log, TEXT("AI Climber %d performing action: %s"), ClimberIndex, *Action);
    // AI action implementation
}

void AClimbingPhysicsDemoLevel::DrawPhysicsDebugInfo()
{
    UpdateVisualization(0.0f);
}

void AClimbingPhysicsDemoLevel::EnablePerformanceHUD(bool bEnable)
{
    Configuration.bEnablePerformanceHUD = bEnable;
    bShowPerformanceMetrics = bEnable;
}