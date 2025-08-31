#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "../Tools/ClimbingToolBase.h"
#include "AdvancedRopeComponent.h"
#include "AnchorSystem.generated.h"

UENUM(BlueprintType)
enum class EAnchorConfiguration : uint8
{
    Single          UMETA(DisplayName = "Single Point"),
    Equalized       UMETA(DisplayName = "Equalized Multi-Point"),
    Redundant       UMETA(DisplayName = "Redundant System"),
    Directional     UMETA(DisplayName = "Directional Anchor"),
    FloatingX       UMETA(DisplayName = "Floating X Configuration"),
    Quad            UMETA(DisplayName = "Quad Anchor"),
    Gear            UMETA(DisplayName = "Gear Anchor")
};

UENUM(BlueprintType)
enum class ELoadDistribution : uint8
{
    Equal           UMETA(DisplayName = "Equal Distribution"),
    Weighted        UMETA(DisplayName = "Weighted by Strength"),
    Angular         UMETA(DisplayName = "Angular Compensation"),
    Dynamic         UMETA(DisplayName = "Dynamic Adjustment")
};

USTRUCT(BlueprintType)
struct FAnchorPoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AClimbingToolBase* AnchorTool = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Direction = FVector::ZeroVector; // Load direction from this anchor

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Strength = 25.0f; // kN

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LoadShare = 0.0f; // Current load percentage (0.0 to 1.0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ActualLoad = 0.0f; // Newtons

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Angle = 0.0f; // Angle to load direction

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsBackup = false; // Backup anchor that only engages if primary fails

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Quality = 1.0f; // Placement quality factor (0.0 to 1.0)
};

USTRUCT(BlueprintType)
struct FAnchorSystemState
{
    GENERATED_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    float TotalSystemLoad = 0.0f; // Newtons

    UPROPERTY(Replicated, BlueprintReadOnly)
    FVector LoadDirection = FVector::ZeroVector;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float SystemEfficiency = 1.0f; // 0.0 to 1.0

    UPROPERTY(Replicated, BlueprintReadOnly)
    float SafetyFactor = 1.0f; // Current safety margin

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bSystemIntact = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 ActiveAnchors = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 FailedAnchors = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float MaxSinglePointLoad = 0.0f; // Highest load on any single anchor
};

USTRUCT(BlueprintType)
struct FAnchorSystemSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Settings")
    EAnchorConfiguration Configuration = EAnchorConfiguration::Equalized;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Settings")
    ELoadDistribution LoadDistribution = ELoadDistribution::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
    float MinSafetyFactor = 3.0f; // Minimum acceptable safety factor

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
    float RedundancyFactor = 2.0f; // System should handle this multiple of expected load

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Load Sharing")
    float LoadSharingEfficiency = 0.85f; // How efficiently load is shared (imperfect due to stretch differences)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Load Sharing")
    float MaxLoadImbalance = 0.3f; // Maximum acceptable load imbalance between anchors

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angles")
    float MaxAnchorAngle = 60.0f; // Maximum angle between anchors for optimal load sharing

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angles")
    float OptimalAnchorAngle = 30.0f; // Optimal angle between anchors

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extension")
    float MaxSystemExtension = 15.0f; // Maximum acceptable system extension in cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Failure")
    bool bEnableProgressiveFailure = true; // Allow system to continue functioning with failed anchors

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Failure")
    float FailureRedistributionTime = 0.1f; // Time in seconds to redistribute load after anchor failure
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UAnchorSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UAnchorSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // System configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor System")
    FAnchorSystemSettings Settings;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "System State")
    FAnchorSystemState SystemState;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Anchor Points")
    TArray<FAnchorPoint> AnchorPoints;

    // Connected ropes
    UPROPERTY(Replicated)
    TArray<UAdvancedRopeComponent*> ConnectedRopes;

    // Anchor management
    UFUNCTION(BlueprintCallable, Category = "Anchor Management")
    bool AddAnchorPoint(AClimbingToolBase* AnchorTool, bool bIsBackup = false);

    UFUNCTION(BlueprintCallable, Category = "Anchor Management")
    void RemoveAnchorPoint(AClimbingToolBase* AnchorTool);

    UFUNCTION(BlueprintCallable, Category = "Anchor Management")
    void ClearAllAnchors();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAddAnchorPoint(AClimbingToolBase* AnchorTool, bool bIsBackup);

    // Rope connections
    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    bool ConnectRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Management")
    void DisconnectRope(UAdvancedRopeComponent* Rope);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerConnectRope(UAdvancedRopeComponent* Rope);

    // Load calculations
    UFUNCTION(BlueprintCallable, Category = "Load Analysis")
    void CalculateLoadDistribution();

    UFUNCTION(BlueprintCallable, Category = "Load Analysis")
    float GetSystemSafetyFactor() const;

    UFUNCTION(BlueprintCallable, Category = "Load Analysis")
    float GetMaxSinglePointLoad() const;

    UFUNCTION(BlueprintCallable, Category = "Load Analysis")
    FVector GetSystemCenterOfLoad() const;

    UFUNCTION(BlueprintCallable, Category = "Load Analysis")
    bool ValidateSystemIntegrity() const;

    // Advanced analysis
    UFUNCTION(BlueprintCallable, Category = "Advanced Analysis")
    float CalculateSystemExtension() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Analysis")
    float AnalyzeAnchorAngles() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Analysis")
    TArray<FVector> SimulateFailureScenarios() const;

    UFUNCTION(BlueprintCallable, Category = "Advanced Analysis")
    float PredictSystemLifetime() const;

    // Configuration methods
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetConfiguration(EAnchorConfiguration NewConfig);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void OptimizeLoadDistribution();

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void ReconfigureForLoadDirection(const FVector& LoadDirection);

    // Failure handling
    UFUNCTION(BlueprintCallable, Category = "Failure Management")
    void HandleAnchorFailure(AClimbingToolBase* FailedAnchor);

    UFUNCTION(BlueprintCallable, Category = "Failure Management")
    void ActivateBackupAnchors();

    UFUNCTION(BlueprintCallable, Category = "Failure Management")
    bool CanSystemSurviveFailure(AClimbingToolBase* AnchorTool) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Anchor Events")
    FSimpleMulticastDelegate OnSystemEstablished;

    UPROPERTY(BlueprintAssignable, Category = "Anchor Events")
    FSimpleMulticastDelegate OnLoadImbalanceWarning;

    UPROPERTY(BlueprintAssignable, Category = "Anchor Events")
    FSimpleMulticastDelegate OnSystemOverloaded;

    UPROPERTY(BlueprintAssignable, Category = "Anchor Events")
    FSimpleMulticastDelegate OnAnchorFailure;

    UPROPERTY(BlueprintAssignable, Category = "Anchor Events")
    FSimpleMulticastDelegate OnSystemFailure;

protected:
    // Internal calculations
    void UpdateSystemState(float DeltaTime);
    void CalculateEqualizedLoads();
    void CalculateWeightedLoads();
    void CalculateAngularCompensation();
    void CalculateDynamicDistribution();

    // Load distribution algorithms
    void DistributeLoad_Equal(float TotalLoad);
    void DistributeLoad_Weighted(float TotalLoad);
    void DistributeLoad_Angular(float TotalLoad);
    void DistributeLoad_Dynamic(float TotalLoad);

    // Analysis functions
    float CalculateAnchorLoadShare(const FAnchorPoint& Anchor, const FVector& LoadDirection) const;
    float CalculateAngleFactor(const FAnchorPoint& Anchor, const FVector& LoadDirection) const;
    float CalculateStretchFactor(const FAnchorPoint& Anchor) const;
    float CalculateQualityFactor(const FAnchorPoint& Anchor) const;

    // System validation
    bool ValidateConfiguration() const;
    bool CheckLoadImbalance() const;
    void UpdateSafetyFactor();

    // Failure simulation
    void SimulateProgressiveFailure();
    void RedistributeLoadAfterFailure(int32 FailedAnchorIndex);

private:
    // Performance optimization
    float LastCalculationTime = 0.0f;
    float CalculationInterval = 0.1f; // Update loads 10 times per second

    // Cached calculations
    FVector CachedLoadDirection = FVector::ZeroVector;
    float CachedTotalLoad = 0.0f;
    bool bNeedsRecalculation = true;

    // Internal tracking
    TArray<float> LoadHistory; // For trend analysis
    float SystemAge = 0.0f; // Time since system establishment
    int32 LoadCycles = 0; // Number of significant load events

    // Failure tracking
    TArray<int32> FailedAnchorIndices;
    float LastFailureTime = 0.0f;
    bool bInFailureRedistribution = false;
};