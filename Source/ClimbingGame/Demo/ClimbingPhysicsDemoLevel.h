#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "../Physics/AnchorSystem.h"
#include "../Physics/FallMechanicsSystem.h"
#include "../Multiplayer/NetworkedRopeComponent.h"
#include "ClimbingPhysicsDemoLevel.generated.h"

UENUM(BlueprintType)
enum class EDemoScenario : uint8
{
    BasicClimbing       UMETA(DisplayName = "Basic Climbing"),
    RopePhysics         UMETA(DisplayName = "Rope Physics"),
    AnchorSystems       UMETA(DisplayName = "Anchor Systems"),
    FallMechanics       UMETA(DisplayName = "Fall Mechanics"),
    CooperativeClimbing UMETA(DisplayName = "Cooperative Climbing"),
    PerformanceTest     UMETA(DisplayName = "Performance Testing"),
    AllSystems          UMETA(DisplayName = "All Systems Demo")
};

USTRUCT(BlueprintType)
struct FDemoConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    EDemoScenario Scenario = EDemoScenario::AllSystems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    int32 NumberOfClimbers = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    bool bEnablePhysicsVisualization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    bool bEnablePerformanceHUD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    bool bAutoStartDemo = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    float DemoTimeLimit = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Setup")
    bool bEnableAI = true;
};

/**
 * Demonstration level that showcases all climbing physics systems
 * Creates interactive scenarios to test and display physics capabilities
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AClimbingPhysicsDemoLevel : public AActor
{
    GENERATED_BODY()

public:
    AClimbingPhysicsDemoLevel();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // Demo configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Configuration")
    FDemoConfiguration Configuration;

    // Demo actors and components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Actors")
    TArray<ACharacter*> DemoClimbers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Actors")
    TArray<UAdvancedRopeComponent*> DemoRopes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Actors")
    TArray<UAnchorSystem*> DemoAnchorSystems;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Actors")
    TArray<class AClimbingToolBase*> DemoTools;

    // Demo scenarios
    UFUNCTION(BlueprintCallable, Category = "Demo Control", CallInEditor = true)
    void StartDemo(EDemoScenario Scenario = EDemoScenario::AllSystems);

    UFUNCTION(BlueprintCallable, Category = "Demo Control", CallInEditor = true)
    void StopDemo();

    UFUNCTION(BlueprintCallable, Category = "Demo Control", CallInEditor = true)
    void ResetDemo();

    UFUNCTION(BlueprintCallable, Category = "Demo Control")
    void NextScenario();

    UFUNCTION(BlueprintCallable, Category = "Demo Control")
    void PreviousScenario();

    // Individual demo scenarios
    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoBasicClimbing();

    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoRopePhysics();

    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoAnchorSystems();

    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoFallMechanics();

    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoCooperativeClimbing();

    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoPerformanceTest();

    UFUNCTION(BlueprintCallable, Category = "Demo Scenarios", CallInEditor = true)
    void DemoAllSystems();

    // Demo setup functions
    UFUNCTION(BlueprintCallable, Category = "Demo Setup")
    void SpawnDemoClimbers(int32 Count = 2);

    UFUNCTION(BlueprintCallable, Category = "Demo Setup")
    void CreateDemoRoute(float Height = 2000.0f, float Difficulty = 0.5f);

    UFUNCTION(BlueprintCallable, Category = "Demo Setup")
    void SetupDemoRopes();

    UFUNCTION(BlueprintCallable, Category = "Demo Setup")
    void SetupDemoAnchors();

    UFUNCTION(BlueprintCallable, Category = "Demo Setup")
    void SetupDemoTools();

    UFUNCTION(BlueprintCallable, Category = "Demo Setup")
    void ConfigureDemoEnvironment();

    // Visualization and HUD
    UFUNCTION(BlueprintCallable, Category = "Demo Visualization")
    void EnablePhysicsVisualization(bool bEnable = true);

    UFUNCTION(BlueprintCallable, Category = "Demo Visualization")
    void EnablePerformanceHUD(bool bEnable = true);

    UFUNCTION(BlueprintCallable, Category = "Demo Visualization")
    void UpdateVisualization(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Demo Visualization")
    void DrawPhysicsDebugInfo();

    // AI control for automated demonstrations
    UFUNCTION(BlueprintCallable, Category = "AI Demo")
    void EnableAIControl(bool bEnable = true);

    UFUNCTION(BlueprintCallable, Category = "AI Demo")
    void SetAIClimbingPath(const TArray<FVector>& PathPoints);

    UFUNCTION(BlueprintCallable, Category = "AI Demo")
    void MakeAIPerformAction(int32 ClimberIndex, const FString& Action);

    // Demo state management
    UFUNCTION(BlueprintPure, Category = "Demo State")
    bool IsDemoRunning() const { return bDemoRunning; }

    UFUNCTION(BlueprintPure, Category = "Demo State")
    EDemoScenario GetCurrentScenario() const { return CurrentScenario; }

    UFUNCTION(BlueprintPure, Category = "Demo State")
    float GetDemoTimeElapsed() const { return DemoTimeElapsed; }

    UFUNCTION(BlueprintPure, Category = "Demo State")
    float GetDemoTimeRemaining() const;

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance")
    void StartPerformanceMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void StopPerformanceMonitoring();

    UFUNCTION(BlueprintPure, Category = "Performance")
    struct FPerformanceMetrics GetCurrentPerformanceMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "Performance")
    void LogPerformanceResults();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Demo Events")
    FSimpleMulticastDelegate OnDemoStarted;

    UPROPERTY(BlueprintAssignable, Category = "Demo Events")
    FSimpleMulticastDelegate OnDemoStopped;

    UPROPERTY(BlueprintAssignable, Category = "Demo Events")
    FSimpleMulticastDelegate OnScenarioChanged;

    UPROPERTY(BlueprintAssignable, Category = "Demo Events")
    FSimpleMulticastDelegate OnDemoComplete;

protected:
    // Internal state
    bool bDemoRunning = false;
    EDemoScenario CurrentScenario = EDemoScenario::AllSystems;
    float DemoTimeElapsed = 0.0f;
    float ScenarioStartTime = 0.0f;

    // Demo level geometry
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Geometry")
    class UStaticMeshComponent* ClimbingWall;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Geometry")
    TArray<class UStaticMeshComponent*> ClimbingHolds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Geometry")
    TArray<AActor*> AnchorPoints;

    // Physics visualization components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    class USceneComponent* VisualizationRoot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowRopeTension = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowGripPoints = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowAnchorLoads = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowPerformanceMetrics = true;

    // AI control
    bool bAIControlEnabled = false;
    TArray<FVector> AIClimbingPath;
    TArray<int32> AICurrentPathIndex;

    // Performance monitoring
    bool bPerformanceMonitoringActive = false;
    TArray<struct FPerformanceMetrics> PerformanceHistory;
    float PerformanceLogInterval = 1.0f;
    float LastPerformanceLogTime = 0.0f;

    // Demo scenario management
    void InitializeScenario(EDemoScenario Scenario);
    void UpdateScenario(float DeltaTime);
    void CleanupScenario();
    void TransitionToNextScenario();

    // Geometry creation
    void CreateClimbingWall(float Height, float Width, float Angle = 90.0f);
    void CreateClimbingHolds(int32 Count, float Difficulty);
    void CreateAnchorPoints(int32 Count);

    // AI behavior
    void UpdateAIClimbers(float DeltaTime);
    void ProcessAIClimberMovement(ACharacter* Climber, int32 ClimberIndex, float DeltaTime);
    FVector GetNextAITarget(int32 ClimberIndex);

    // Visualization helpers
    void DrawRopeTensionVisualization();
    void DrawGripPointVisualization();
    void DrawAnchorLoadVisualization();
    void DrawPerformanceHUD();

    // Demo scenario implementations
    void SetupBasicClimbingScenario();
    void SetupRopePhysicsScenario();
    void SetupAnchorSystemsScenario();
    void SetupFallMechanicsScenario();
    void SetupCooperativeClimbingScenario();
    void SetupPerformanceTestScenario();
    void SetupAllSystemsScenario();

    // Scenario-specific updates
    void UpdateBasicClimbing(float DeltaTime);
    void UpdateRopePhysics(float DeltaTime);
    void UpdateAnchorSystems(float DeltaTime);
    void UpdateFallMechanics(float DeltaTime);
    void UpdateCooperativeClimbing(float DeltaTime);
    void UpdatePerformanceTest(float DeltaTime);
    void UpdateAllSystems(float DeltaTime);

    // Utility functions
    ACharacter* SpawnClimberAtLocation(const FVector& Location);
    UAdvancedRopeComponent* CreateRopeBetweenPoints(const FVector& Start, const FVector& End);
    UAnchorSystem* CreateAnchorSystemAtLocation(const FVector& Location);
    void SetupClimberPhysics(ACharacter* Climber);
    void CleanupDemoActors();

    // Configuration validation
    bool ValidateConfiguration() const;
    void ApplyConfiguration();

private:
    // Cached references
    class UClimbingPerformanceManager* PerformanceManager;
    class UWorld* CachedWorld;
    
    // Demo constants
    static constexpr float DEMO_WALL_HEIGHT = 3000.0f; // 30 meters
    static constexpr float DEMO_WALL_WIDTH = 2000.0f;  // 20 meters
    static constexpr int32 MAX_DEMO_CLIMBERS = 8;
    static constexpr int32 MAX_CLIMBING_HOLDS = 100;
    static constexpr float SCENARIO_TRANSITION_TIME = 30.0f; // 30 seconds per scenario
};