#include "AnchorSystem.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "../Tools/ClimbingToolBase.h"
#include "AdvancedRopeComponent.h"

UAnchorSystem::UAnchorSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0167f; // 60 FPS for responsive load calculations
    SetIsReplicatedByDefault(true);

    // Initialize default settings
    Settings.Configuration = EAnchorConfiguration::Equalized;
    Settings.LoadDistribution = ELoadDistribution::Equal;
    Settings.MinSafetyFactor = 3.0f;
    Settings.RedundancyFactor = 2.0f;
    Settings.LoadSharingEfficiency = 0.85f;
    Settings.MaxLoadImbalance = 0.3f;
    Settings.MaxAnchorAngle = 60.0f;
    Settings.OptimalAnchorAngle = 30.0f;
    Settings.MaxSystemExtension = 15.0f;
    Settings.bEnableProgressiveFailure = true;
    Settings.FailureRedistributionTime = 0.1f;

    // Initialize system state
    SystemState.TotalSystemLoad = 0.0f;
    SystemState.LoadDirection = FVector::ZeroVector;
    SystemState.SystemEfficiency = 1.0f;
    SystemState.SafetyFactor = 0.0f;
    SystemState.bSystemIntact = true;
    SystemState.ActiveAnchors = 0;
    SystemState.FailedAnchors = 0;
    SystemState.MaxSinglePointLoad = 0.0f;
}

void UAnchorSystem::BeginPlay()
{
    Super::BeginPlay();
    SystemAge = 0.0f;
    LoadHistory.Reserve(1000); // Reserve space for load history
}

void UAnchorSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwnerRole() == ROLE_Authority)
    {
        UpdateSystemState(DeltaTime);
        
        // Perform load calculations at regular intervals
        LastCalculationTime += DeltaTime;
        if (LastCalculationTime >= CalculationInterval || bNeedsRecalculation)
        {
            CalculateLoadDistribution();
            UpdateSafetyFactor();
            LastCalculationTime = 0.0f;
            bNeedsRecalculation = false;
        }

        // Handle progressive failure if enabled
        if (Settings.bEnableProgressiveFailure && bInFailureRedistribution)
        {
            SimulateProgressiveFailure();
        }
    }

    SystemAge += DeltaTime;
}

void UAnchorSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAnchorSystem, SystemState);
    DOREPLIFETIME(UAnchorSystem, AnchorPoints);
    DOREPLIFETIME(UAnchorSystem, ConnectedRopes);
}

bool UAnchorSystem::AddAnchorPoint(AClimbingToolBase* AnchorTool, bool bIsBackup)
{
    if (!AnchorTool)
        return false;

    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerAddAnchorPoint(AnchorTool, bIsBackup);
        return true;
    }

    // Create new anchor point
    FAnchorPoint NewAnchor;
    NewAnchor.AnchorTool = AnchorTool;
    NewAnchor.Location = AnchorTool->GetActorLocation();
    NewAnchor.Direction = AnchorTool->GetActorForwardVector();
    NewAnchor.Strength = AnchorTool->Properties.MajorAxisStrength;
    NewAnchor.bIsActive = !bIsBackup; // Backup anchors start inactive
    NewAnchor.bIsBackup = bIsBackup;
    NewAnchor.Quality = 1.0f; // Default quality, could be set by anchor tool

    // For anchor tools with placement quality, use that
    if (AAnchorTool* AnchorCast = Cast<AAnchorTool>(AnchorTool))
    {
        NewAnchor.Quality = AnchorCast->PlacementQuality;
    }

    AnchorPoints.Add(NewAnchor);
    
    if (!bIsBackup)
    {
        SystemState.ActiveAnchors++;
    }

    bNeedsRecalculation = true;
    
    // Validate system configuration
    if (AnchorPoints.Num() >= 2)
    {
        OnSystemEstablished.Broadcast();
    }

    UE_LOG(LogTemp, Log, TEXT("Added anchor point. Total anchors: %d, Active: %d"), 
           AnchorPoints.Num(), SystemState.ActiveAnchors);

    return true;
}

void UAnchorSystem::ServerAddAnchorPoint_Implementation(AClimbingToolBase* AnchorTool, bool bIsBackup)
{
    AddAnchorPoint(AnchorTool, bIsBackup);
}

bool UAnchorSystem::ServerAddAnchorPoint_Validate(AClimbingToolBase* AnchorTool, bool bIsBackup)
{
    return AnchorTool != nullptr;
}

void UAnchorSystem::RemoveAnchorPoint(AClimbingToolBase* AnchorTool)
{
    if (!AnchorTool)
        return;

    for (int32 i = AnchorPoints.Num() - 1; i >= 0; --i)
    {
        if (AnchorPoints[i].AnchorTool == AnchorTool)
        {
            if (AnchorPoints[i].bIsActive)
            {
                SystemState.ActiveAnchors--;
            }
            AnchorPoints.RemoveAt(i);
            bNeedsRecalculation = true;
            break;
        }
    }
}

void UAnchorSystem::ClearAllAnchors()
{
    AnchorPoints.Empty();
    SystemState.ActiveAnchors = 0;
    SystemState.FailedAnchors = 0;
    SystemState.bSystemIntact = AnchorPoints.Num() > 0;
    bNeedsRecalculation = true;
}

bool UAnchorSystem::ConnectRope(UAdvancedRopeComponent* Rope)
{
    if (!Rope)
        return false;

    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerConnectRope(Rope);
        return true;
    }

    if (!ConnectedRopes.Contains(Rope))
    {
        ConnectedRopes.Add(Rope);
        bNeedsRecalculation = true;
        return true;
    }

    return false;
}

void UAnchorSystem::ServerConnectRope_Implementation(UAdvancedRopeComponent* Rope)
{
    ConnectRope(Rope);
}

bool UAnchorSystem::ServerConnectRope_Validate(UAdvancedRopeComponent* Rope)
{
    return Rope != nullptr;
}

void UAnchorSystem::DisconnectRope(UAdvancedRopeComponent* Rope)
{
    if (!Rope)
        return;

    ConnectedRopes.Remove(Rope);
    bNeedsRecalculation = true;
}

void UAnchorSystem::CalculateLoadDistribution()
{
    if (AnchorPoints.Num() == 0)
        return;

    // Calculate total load from connected ropes
    float TotalLoad = 0.0f;
    FVector AverageLoadDirection = FVector::ZeroVector;
    
    for (UAdvancedRopeComponent* Rope : ConnectedRopes)
    {
        if (Rope && IsValid(Rope))
        {
            float RopeTension = Rope->CalculateCurrentTension();
            TotalLoad += RopeTension;
            
            // Calculate load direction from rope
            FVector RopeDirection = FVector::ZeroVector;
            if (Rope->AnchorPointA && Rope->AnchorPointB)
            {
                RopeDirection = (Rope->AnchorPointB->GetActorLocation() - 
                               Rope->AnchorPointA->GetActorLocation()).GetSafeNormal();
            }
            AverageLoadDirection += RopeDirection * RopeTension;
        }
    }
    
    if (TotalLoad > 0.0f)
    {
        AverageLoadDirection.Normalize();
    }
    
    SystemState.TotalSystemLoad = TotalLoad;
    SystemState.LoadDirection = AverageLoadDirection;
    CachedTotalLoad = TotalLoad;
    CachedLoadDirection = AverageLoadDirection;
    
    // Apply load distribution based on configuration
    switch (Settings.LoadDistribution)
    {
        case ELoadDistribution::Equal:
            DistributeLoad_Equal(TotalLoad);
            break;
        case ELoadDistribution::Weighted:
            DistributeLoad_Weighted(TotalLoad);
            break;
        case ELoadDistribution::Angular:
            DistributeLoad_Angular(TotalLoad);
            break;
        case ELoadDistribution::Dynamic:
            DistributeLoad_Dynamic(TotalLoad);
            break;
    }
    
    // Update system efficiency
    SystemState.SystemEfficiency = CalculateSystemEfficiency();
    
    // Track load history
    LoadHistory.Add(TotalLoad);
    if (LoadHistory.Num() > 1000)
    {
        LoadHistory.RemoveAt(0);
    }
    
    LoadCycles++;
}

float UAnchorSystem::GetSystemSafetyFactor() const
{
    return SystemState.SafetyFactor;
}

float UAnchorSystem::GetMaxSinglePointLoad() const
{
    return SystemState.MaxSinglePointLoad;
}

FVector UAnchorSystem::GetSystemCenterOfLoad() const
{
    if (AnchorPoints.Num() == 0)
        return FVector::ZeroVector;
    
    FVector CenterOfLoad = FVector::ZeroVector;
    float TotalLoad = 0.0f;
    
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && Anchor.ActualLoad > 0.0f)
        {
            CenterOfLoad += Anchor.Location * Anchor.ActualLoad;
            TotalLoad += Anchor.ActualLoad;
        }
    }
    
    if (TotalLoad > 0.0f)
    {
        CenterOfLoad /= TotalLoad;
    }
    
    return CenterOfLoad;
}

bool UAnchorSystem::ValidateSystemIntegrity() const
{
    // Check minimum number of active anchors
    if (SystemState.ActiveAnchors < 1)
        return false;
    
    // Check safety factor
    if (SystemState.SafetyFactor < Settings.MinSafetyFactor)
        return false;
    
    // Check load imbalance
    if (!CheckLoadImbalance())
        return false;
    
    // Check anchor angles
    if (AnalyzeAnchorAngles() > Settings.MaxAnchorAngle)
        return false;
    
    return true;
}
    ConnectedRopes.Remove(Rope);
    bNeedsRecalculation = true;
}

void UAnchorSystem::CalculateLoadDistribution()
{
    if (AnchorPoints.Num() == 0)
    {
        SystemState.TotalSystemLoad = 0.0f;
        return;
    }

    // Calculate total system load from connected ropes
    float TotalLoad = 0.0f;
    FVector LoadDirection = FVector::ZeroVector;

    for (UAdvancedRopeComponent* Rope : ConnectedRopes)
    {
        if (Rope)
        {
            float RopeTension = Rope->CalculateCurrentTension();
            TotalLoad += RopeTension;
            
            // Calculate average load direction
            FVector RopeDirection = (GetOwner()->GetActorLocation() - Rope->GetOwner()->GetActorLocation()).GetSafeNormal();
            LoadDirection += RopeDirection * RopeTension;
        }
    }

    if (TotalLoad > 0.0f)
    {
        LoadDirection.Normalize();
    }

    SystemState.TotalSystemLoad = TotalLoad;
    SystemState.LoadDirection = LoadDirection;

    // Distribute load based on configuration
    switch (Settings.LoadDistribution)
    {
        case ELoadDistribution::Equal:
            DistributeLoad_Equal(TotalLoad);
            break;
        case ELoadDistribution::Weighted:
            DistributeLoad_Weighted(TotalLoad);
            break;
        case ELoadDistribution::Angular:
            DistributeLoad_Angular(TotalLoad);
            break;
        case ELoadDistribution::Dynamic:
            DistributeLoad_Dynamic(TotalLoad);
            break;
    }

    // Update system efficiency and maximum single point load
    UpdateSystemEfficiency();
    
    // Record load history for analysis
    if (LoadHistory.Num() > 999)
    {
        LoadHistory.RemoveAt(0);
    }
    LoadHistory.Add(TotalLoad);

    // Count significant load events
    if (TotalLoad > 500.0f) // More than 500N
    {
        LoadCycles++;
    }
}

void UAnchorSystem::DistributeLoad_Equal(float TotalLoad)
{
    int32 ActiveAnchorCount = 0;
    
    // Count active anchors
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            ActiveAnchorCount++;
        }
    }

    if (ActiveAnchorCount == 0)
        return;

    float LoadPerAnchor = TotalLoad / ActiveAnchorCount;
    
    // Apply load sharing efficiency
    LoadPerAnchor /= Settings.LoadSharingEfficiency;

    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            Anchor.ActualLoad = LoadPerAnchor;
            Anchor.LoadShare = 1.0f / ActiveAnchorCount;
        }
        else
        {
            Anchor.ActualLoad = 0.0f;
            Anchor.LoadShare = 0.0f;
        }
    }
}

void UAnchorSystem::DistributeLoad_Weighted(float TotalLoad)
{
    float TotalWeightedStrength = 0.0f;
    
    // Calculate total weighted strength
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            TotalWeightedStrength += Anchor.Strength * Anchor.Quality;
        }
    }

    if (TotalWeightedStrength <= 0.0f)
        return;

    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            float WeightedStrength = Anchor.Strength * Anchor.Quality;
            Anchor.LoadShare = WeightedStrength / TotalWeightedStrength;
            Anchor.ActualLoad = TotalLoad * Anchor.LoadShare / Settings.LoadSharingEfficiency;
        }
        else
        {
            Anchor.ActualLoad = 0.0f;
            Anchor.LoadShare = 0.0f;
        }
    }
}

void UAnchorSystem::DistributeLoad_Angular(float TotalLoad)
{
    FVector CenterOfSystem = GetSystemCenterOfLoad();
    
    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            // Calculate angle factor
            float AngleFactor = CalculateAngleFactor(Anchor, SystemState.LoadDirection);
            
            // Calculate distance factor
            float Distance = FVector::Dist(Anchor.Location, CenterOfSystem);
            float DistanceFactor = FMath::Clamp(1.0f / (Distance * 0.01f + 1.0f), 0.1f, 1.0f);
            
            // Combine factors
            float CombinedFactor = AngleFactor * DistanceFactor * Anchor.Quality;
            Anchor.LoadShare = CombinedFactor;
        }
        else
        {
            Anchor.LoadShare = 0.0f;
        }
    }

    // Normalize load shares
    float TotalShares = 0.0f;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            TotalShares += Anchor.LoadShare;
        }
    }

    if (TotalShares > 0.0f)
    {
        for (FAnchorPoint& Anchor : AnchorPoints)
        {
            if (Anchor.bIsActive)
            {
                Anchor.LoadShare /= TotalShares;
                Anchor.ActualLoad = TotalLoad * Anchor.LoadShare / Settings.LoadSharingEfficiency;
            }
        }
    }
}

void UAnchorSystem::DistributeLoad_Dynamic(float TotalLoad)
{
    // Dynamic distribution considers stretch characteristics and real-time feedback
    
    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            float StretchFactor = CalculateStretchFactor(Anchor);
            float QualityFactor = CalculateQualityFactor(Anchor);
            float AngleFactor = CalculateAngleFactor(Anchor, SystemState.LoadDirection);
            
            // Combine all factors for dynamic distribution
            float DynamicFactor = StretchFactor * QualityFactor * AngleFactor;
            Anchor.LoadShare = DynamicFactor;
        }
        else
        {
            Anchor.LoadShare = 0.0f;
        }
    }

    // Normalize and apply loads
    float TotalShares = 0.0f;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        TotalShares += Anchor.LoadShare;
    }

    if (TotalShares > 0.0f)
    {
        for (FAnchorPoint& Anchor : AnchorPoints)
        {
            if (Anchor.bIsActive)
            {
                Anchor.LoadShare /= TotalShares;
                Anchor.ActualLoad = TotalLoad * Anchor.LoadShare / Settings.LoadSharingEfficiency;
            }
        }
    }
}

float UAnchorSystem::CalculateAngleFactor(const FAnchorPoint& Anchor, const FVector& LoadDirection) const
{
    if (LoadDirection.IsZero())
        return 1.0f;

    FVector AnchorDirection = (Anchor.Location - GetOwner()->GetActorLocation()).GetSafeNormal();
    float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(AnchorDirection, LoadDirection)));
    
    // Optimal angle provides best load sharing
    if (Angle <= Settings.OptimalAnchorAngle)
    {
        return 1.0f;
    }
    else if (Angle <= Settings.MaxAnchorAngle)
    {
        // Linear decrease from optimal to maximum angle
        return FMath::GetMappedRangeValueClamped(
            FVector2D(Settings.OptimalAnchorAngle, Settings.MaxAnchorAngle),
            FVector2D(1.0f, 0.5f),
            Angle
        );
    }
    else
    {
        // Poor geometry - significantly reduced effectiveness
        return 0.3f;
    }
}

float UAnchorSystem::CalculateStretchFactor(const FAnchorPoint& Anchor) const
{
    // Simulate anchor stretch characteristics
    // Different anchor types have different stretch properties
    if (Anchor.AnchorTool)
    {
        switch (Anchor.AnchorTool->ToolType)
        {
            case EToolType::Anchor:
                return 0.95f; // Bolts have minimal stretch
            case EToolType::Carabiner:
                return 0.98f; // Very minimal stretch
            default:
                return 1.0f;
        }
    }
    return 1.0f;
}

float UAnchorSystem::CalculateQualityFactor(const FAnchorPoint& Anchor) const
{
    return Anchor.Quality;
}

void UAnchorSystem::UpdateSystemState(float DeltaTime)
{
    // Update anchor states
    SystemState.ActiveAnchors = 0;
    SystemState.FailedAnchors = 0;
    SystemState.MaxSinglePointLoad = 0.0f;

    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            SystemState.ActiveAnchors++;
            if (Anchor.ActualLoad > SystemState.MaxSinglePointLoad)
            {
                SystemState.MaxSinglePointLoad = Anchor.ActualLoad;
            }
        }
        else if (Anchor.AnchorTool && Anchor.AnchorTool->CurrentState == EToolState::Broken)
        {
            SystemState.FailedAnchors++;
        }
    }

    SystemState.bSystemIntact = (SystemState.ActiveAnchors > 0) && (SystemState.FailedAnchors < AnchorPoints.Num());

    // Check for warnings and failures
    if (CheckLoadImbalance())
    {
        OnLoadImbalanceWarning.Broadcast();
    }

    if (SystemState.SafetyFactor < Settings.MinSafetyFactor)
    {
        OnSystemOverloaded.Broadcast();
    }

    if (!SystemState.bSystemIntact)
    {
        OnSystemFailure.Broadcast();
    }
}

void UAnchorSystem::UpdateSafetyFactor()
{
    if (SystemState.TotalSystemLoad <= 0.0f)
    {
        SystemState.SafetyFactor = 999.0f; // Effectively infinite when no load
        return;
    }

    // Find the weakest link in the system
    float MinimumStrength = FLT_MAX;
    
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            float EffectiveStrength = Anchor.Strength * Anchor.Quality * 1000.0f; // Convert kN to N
            
            // Apply angle derating
            float AngleFactor = CalculateAngleFactor(Anchor, SystemState.LoadDirection);
            EffectiveStrength *= AngleFactor;
            
            if (EffectiveStrength < MinimumStrength)
            {
                MinimumStrength = EffectiveStrength;
            }
        }
    }

    if (MinimumStrength < FLT_MAX)
    {
        SystemState.SafetyFactor = MinimumStrength / SystemState.MaxSinglePointLoad;
    }
    else
    {
        SystemState.SafetyFactor = 0.0f;
    }
}

void UAnchorSystem::UpdateSystemEfficiency()
{
    if (SystemState.ActiveAnchors <= 1)
    {
        SystemState.SystemEfficiency = 1.0f;
        return;
    }

    // Calculate efficiency based on load distribution
    float IdealLoadPerAnchor = SystemState.TotalSystemLoad / SystemState.ActiveAnchors;
    float LoadVariation = 0.0f;

    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            float LoadDifference = FMath::Abs(Anchor.ActualLoad - IdealLoadPerAnchor);
            LoadVariation += LoadDifference;
        }
    }

    // Normalize variation and calculate efficiency
    if (SystemState.ActiveAnchors > 0)
    {
        LoadVariation /= (SystemState.ActiveAnchors * IdealLoadPerAnchor);
        SystemState.SystemEfficiency = FMath::Clamp(1.0f - LoadVariation, 0.1f, 1.0f);
    }
}

bool UAnchorSystem::CheckLoadImbalance() const
{
    if (SystemState.ActiveAnchors <= 1)
        return false;

    float AverageLoad = SystemState.TotalSystemLoad / SystemState.ActiveAnchors;
    
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive)
        {
            float LoadRatio = Anchor.ActualLoad / AverageLoad;
            if (LoadRatio > (1.0f + Settings.MaxLoadImbalance))
            {
                return true;
            }
        }
    }
    
    return false;
}

float UAnchorSystem::GetSystemSafetyFactor() const
{
    return SystemState.SafetyFactor;
}

float UAnchorSystem::GetMaxSinglePointLoad() const
{
    return SystemState.MaxSinglePointLoad;
}

FVector UAnchorSystem::GetSystemCenterOfLoad() const
{
    FVector Center = FVector::ZeroVector;
    float TotalLoad = 0.0f;

    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && Anchor.ActualLoad > 0.0f)
        {
            Center += Anchor.Location * Anchor.ActualLoad;
            TotalLoad += Anchor.ActualLoad;
        }
    }

    if (TotalLoad > 0.0f)
    {
        return Center / TotalLoad;
    }

    return GetOwner()->GetActorLocation();
}

bool UAnchorSystem::ValidateSystemIntegrity() const
{
    // Check minimum number of anchors
    if (SystemState.ActiveAnchors < 1)
        return false;

    // Check safety factor
    if (SystemState.SafetyFactor < Settings.MinSafetyFactor)
        return false;

    // Check load imbalance
    if (CheckLoadImbalance())
        return false;

    // Check anchor angles
    float MaxAngle = AnalyzeAnchorAngles();
    if (MaxAngle > Settings.MaxAnchorAngle)
        return false;

    return true;
}

float UAnchorSystem::AnalyzeAnchorAngles() const
{
    float MaxAngle = 0.0f;
    
    for (int32 i = 0; i < AnchorPoints.Num(); ++i)
    {
        if (!AnchorPoints[i].bIsActive)
            continue;

        for (int32 j = i + 1; j < AnchorPoints.Num(); ++j)
        {
            if (!AnchorPoints[j].bIsActive)
                continue;

            FVector Dir1 = (AnchorPoints[i].Location - GetOwner()->GetActorLocation()).GetSafeNormal();
            FVector Dir2 = (AnchorPoints[j].Location - GetOwner()->GetActorLocation()).GetSafeNormal();
            
            float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Dir1, Dir2)));
            
            if (Angle > MaxAngle)
            {
                MaxAngle = Angle;
            }
        }
    }
    
    return MaxAngle;
}

void UAnchorSystem::HandleAnchorFailure(AClimbingToolBase* FailedAnchor)
{
    // Find and deactivate failed anchor
    for (int32 i = 0; i < AnchorPoints.Num(); ++i)
    {
        if (AnchorPoints[i].AnchorTool == FailedAnchor)
        {
            AnchorPoints[i].bIsActive = false;
            FailedAnchorIndices.Add(i);
            LastFailureTime = SystemAge;
            bInFailureRedistribution = true;
            
            OnAnchorFailure.Broadcast();
            
            // Activate backup anchors if available
            if (Settings.bEnableProgressiveFailure)
            {
                ActivateBackupAnchors();
            }
            
            bNeedsRecalculation = true;
            break;
        }
    }
}

void UAnchorSystem::ActivateBackupAnchors()
{
    // Activate backup anchors to maintain system integrity
    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsBackup && !Anchor.bIsActive)
        {
            Anchor.bIsActive = true;
            Anchor.bIsBackup = false; // No longer backup once activated
            UE_LOG(LogTemp, Warning, TEXT("Backup anchor activated due to primary anchor failure"));
            break; // Activate one backup at a time
        }
    }
}

void UAnchorSystem::SimulateProgressiveFailure()
{
    if (SystemAge - LastFailureTime > Settings.FailureRedistributionTime)
    {
        bInFailureRedistribution = false;
        
        // Check if system can still function
        if (SystemState.ActiveAnchors < 1)
        {
            SystemState.bSystemIntact = false;
            OnSystemFailure.Broadcast();
        }
    }
}

bool UAnchorSystem::CanSystemSurviveFailure(AClimbingToolBase* AnchorTool) const
{
    // Simulate what happens if this anchor fails
    int32 ActiveCount = SystemState.ActiveAnchors;
    
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.AnchorTool == AnchorTool && Anchor.bIsActive)
        {
            ActiveCount--;
            break;
        }
    }

    // Check if backup anchors are available
    int32 BackupCount = 0;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsBackup)
        {
            BackupCount++;
        }
    }

    return (ActiveCount + BackupCount) >= 1;
}

float UAnchorSystem::PredictSystemLifetime() const
{
    // Predict system lifetime based on load history and anchor degradation
    
    if (LoadHistory.Num() < 10)
        return -1.0f; // Insufficient data

    // Calculate average load and load variance
    float AverageLoad = 0.0f;
    for (float Load : LoadHistory)
    {
        AverageLoad += Load;
    }
    AverageLoad /= LoadHistory.Num();

    // Estimate wear rate based on load cycles and intensity
    float EstimatedWearRate = (LoadCycles * AverageLoad) / (SystemAge * 10000.0f);
    
    // Factor in anchor quality degradation
    float AverageQuality = 0.0f;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        AverageQuality += Anchor.Quality;
    }
    AverageQuality /= FMath::Max(1, AnchorPoints.Num());

    // Predict remaining lifetime (simplified model)
    float RemainingLifetime = (0.7f - (1.0f - AverageQuality)) / EstimatedWearRate;
    
    return FMath::Max(0.0f, RemainingLifetime);
}

void UAnchorSystem::SetConfiguration(EAnchorConfiguration NewConfig)
{
    Settings.Configuration = NewConfig;
    
    // Adjust settings based on configuration
    switch (NewConfig)
    {
        case EAnchorConfiguration::Single:
            Settings.LoadDistribution = ELoadDistribution::Equal;
            break;
        case EAnchorConfiguration::Equalized:
            Settings.LoadDistribution = ELoadDistribution::Equal;
            Settings.LoadSharingEfficiency = 0.85f;
            break;
        case EAnchorConfiguration::Redundant:
            Settings.LoadDistribution = ELoadDistribution::Weighted;
            Settings.bEnableProgressiveFailure = true;
            break;
        case EAnchorConfiguration::Directional:
            Settings.LoadDistribution = ELoadDistribution::Angular;
            Settings.OptimalAnchorAngle = 45.0f;
            break;
        case EAnchorConfiguration::FloatingX:
            Settings.LoadDistribution = ELoadDistribution::Dynamic;
            Settings.MaxSystemExtension = 30.0f;
            break;
        case EAnchorConfiguration::Quad:
            Settings.LoadDistribution = ELoadDistribution::Weighted;
            Settings.OptimalAnchorAngle = 90.0f;
            break;
        case EAnchorConfiguration::Gear:
            Settings.LoadDistribution = ELoadDistribution::Dynamic;
            Settings.LoadSharingEfficiency = 0.75f; // Lower efficiency due to gear placement
            break;
    }
    
    bNeedsRecalculation = true;
}

void UAnchorSystem::OptimizeLoadDistribution()
{
    // Analyze current load distribution and suggest optimizations
    float CurrentEfficiency = SystemState.SystemEfficiency;
    
    // Try different distribution methods and pick the best
    ELoadDistribution OriginalMethod = Settings.LoadDistribution;
    float BestEfficiency = CurrentEfficiency;
    ELoadDistribution BestMethod = OriginalMethod;
    
    TArray<ELoadDistribution> Methods = {
        ELoadDistribution::Equal,
        ELoadDistribution::Weighted, 
        ELoadDistribution::Angular,
        ELoadDistribution::Dynamic
    };
    
    for (ELoadDistribution Method : Methods)
    {
        Settings.LoadDistribution = Method;
        CalculateLoadDistribution();
        
        if (SystemState.SystemEfficiency > BestEfficiency)
        {
            BestEfficiency = SystemState.SystemEfficiency;
            BestMethod = Method;
        }
    }
    
    Settings.LoadDistribution = BestMethod;
    CalculateLoadDistribution(); // Apply the best method
    
    UE_LOG(LogTemp, Log, TEXT("Load distribution optimized. Method: %d, Efficiency: %f"), 
           static_cast<int32>(BestMethod), BestEfficiency);
}