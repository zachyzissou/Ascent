#include "AnchorSystem.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

// Load Distribution Algorithm Implementations

void UAnchorSystem::DistributeLoad_Equal(float TotalLoad)
{
    if (SystemState.ActiveAnchors == 0)
        return;
    
    float LoadPerAnchor = TotalLoad / SystemState.ActiveAnchors;
    float MaxLoad = 0.0f;
    
    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && !Anchor.bIsBackup)
        {
            Anchor.LoadShare = 1.0f / SystemState.ActiveAnchors;
            Anchor.ActualLoad = LoadPerAnchor * Anchor.Quality; // Adjust for quality
            MaxLoad = FMath::Max(MaxLoad, Anchor.ActualLoad);
        }
        else
        {
            Anchor.LoadShare = 0.0f;
            Anchor.ActualLoad = 0.0f;
        }
    }
    
    SystemState.MaxSinglePointLoad = MaxLoad;
}

void UAnchorSystem::DistributeLoad_Weighted(float TotalLoad)
{
    if (SystemState.ActiveAnchors == 0)
        return;
    
    // Calculate total weighted strength
    float TotalWeightedStrength = 0.0f;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && !Anchor.bIsBackup)
        {
            float WeightedStrength = Anchor.Strength * Anchor.Quality;
            TotalWeightedStrength += WeightedStrength;
        }
    }
    
    if (TotalWeightedStrength <= 0.0f)
        return;
    
    float MaxLoad = 0.0f;
    
    for (FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && !Anchor.bIsBackup)
        {
            float WeightedStrength = Anchor.Strength * Anchor.Quality;
            Anchor.LoadShare = WeightedStrength / TotalWeightedStrength;
            Anchor.ActualLoad = TotalLoad * Anchor.LoadShare * Settings.LoadSharingEfficiency;
            MaxLoad = FMath::Max(MaxLoad, Anchor.ActualLoad);
        }
        else
        {
            Anchor.LoadShare = 0.0f;
            Anchor.ActualLoad = 0.0f;
        }
    }
    
    SystemState.MaxSinglePointLoad = MaxLoad;
}

void UAnchorSystem::DistributeLoad_Angular(float TotalLoad)
{
    if (SystemState.ActiveAnchors == 0 || SystemState.LoadDirection.IsZero())
        return;
    
    float TotalAngularFactor = 0.0f;
    
    // Calculate angular factors for each anchor
    TArray<float> AngularFactors;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && !Anchor.bIsBackup)
        {
            float AngleFactor = CalculateAngleFactor(Anchor, SystemState.LoadDirection);
            AngularFactors.Add(AngleFactor);
            TotalAngularFactor += AngleFactor;
        }
        else
        {
            AngularFactors.Add(0.0f);
        }
    }
    
    if (TotalAngularFactor <= 0.0f)
        return;
    
    float MaxLoad = 0.0f;
    
    for (int32 i = 0; i < AnchorPoints.Num(); ++i)
    {
        FAnchorPoint& Anchor = AnchorPoints[i];
        if (Anchor.bIsActive && !Anchor.bIsBackup && i < AngularFactors.Num())
        {
            Anchor.LoadShare = AngularFactors[i] / TotalAngularFactor;
            Anchor.ActualLoad = TotalLoad * Anchor.LoadShare * Settings.LoadSharingEfficiency;
            MaxLoad = FMath::Max(MaxLoad, Anchor.ActualLoad);
        }
        else
        {
            Anchor.LoadShare = 0.0f;
            Anchor.ActualLoad = 0.0f;
        }
    }
    
    SystemState.MaxSinglePointLoad = MaxLoad;
}

void UAnchorSystem::DistributeLoad_Dynamic(float TotalLoad)
{
    if (SystemState.ActiveAnchors == 0)
        return;
    
    // Dynamic distribution considers strength, quality, angle, and stretch
    float TotalDynamicFactor = 0.0f;
    
    TArray<float> DynamicFactors;
    for (const FAnchorPoint& Anchor : AnchorPoints)
    {
        if (Anchor.bIsActive && !Anchor.bIsBackup)
        {
            float StrengthFactor = CalculateQualityFactor(Anchor);\n            float AngleFactor = CalculateAngleFactor(Anchor, SystemState.LoadDirection);\n            float StretchFactor = CalculateStretchFactor(Anchor);\n            \n            float DynamicFactor = StrengthFactor * AngleFactor * StretchFactor;\n            DynamicFactors.Add(DynamicFactor);\n            TotalDynamicFactor += DynamicFactor;\n        }\n        else\n        {\n            DynamicFactors.Add(0.0f);\n        }\n    }\n    \n    if (TotalDynamicFactor <= 0.0f)\n        return;\n    \n    float MaxLoad = 0.0f;\n    \n    for (int32 i = 0; i < AnchorPoints.Num(); ++i)\n    {\n        FAnchorPoint& Anchor = AnchorPoints[i];\n        if (Anchor.bIsActive && !Anchor.bIsBackup && i < DynamicFactors.Num())\n        {\n            Anchor.LoadShare = DynamicFactors[i] / TotalDynamicFactor;\n            Anchor.ActualLoad = TotalLoad * Anchor.LoadShare * Settings.LoadSharingEfficiency;\n            MaxLoad = FMath::Max(MaxLoad, Anchor.ActualLoad);\n        }\n        else\n        {\n            Anchor.LoadShare = 0.0f;\n            Anchor.ActualLoad = 0.0f;\n        }\n    }\n    \n    SystemState.MaxSinglePointLoad = MaxLoad;\n}\n\nfloat UAnchorSystem::CalculateAnchorLoadShare(const FAnchorPoint& Anchor, const FVector& LoadDirection) const\n{\n    if (!Anchor.bIsActive || Anchor.bIsBackup)\n        return 0.0f;\n    \n    float QualityFactor = CalculateQualityFactor(Anchor);\n    float AngleFactor = CalculateAngleFactor(Anchor, LoadDirection);\n    float StretchFactor = CalculateStretchFactor(Anchor);\n    \n    return QualityFactor * AngleFactor * StretchFactor;\n}\n\nfloat UAnchorSystem::CalculateAngleFactor(const FAnchorPoint& Anchor, const FVector& LoadDirection) const\n{\n    if (LoadDirection.IsZero() || Anchor.Direction.IsZero())\n        return 1.0f;\n    \n    float DotProduct = FVector::DotProduct(Anchor.Direction, LoadDirection);\n    float Angle = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));\n    float AngleDegrees = FMath::RadiansToDegrees(Angle);\n    \n    // Anchors aligned with load direction are more effective\n    float EfficiencyFactor = FMath::Cos(Angle);\n    \n    // Penalty for large angles\n    if (AngleDegrees > Settings.OptimalAnchorAngle)\n    {\n        float AnglePenalty = (AngleDegrees - Settings.OptimalAnchorAngle) / (Settings.MaxAnchorAngle - Settings.OptimalAnchorAngle);\n        EfficiencyFactor *= (1.0f - AnglePenalty * 0.5f);\n    }\n    \n    return FMath::Max(0.1f, EfficiencyFactor);\n}\n\nfloat UAnchorSystem::CalculateStretchFactor(const FAnchorPoint& Anchor) const\n{\n    // Simplified stretch calculation - in real implementation would consider\n    // anchor type, material properties, and current load\n    if (!Anchor.AnchorTool)\n        return 1.0f;\n    \n    // Different anchor types have different stretch characteristics\n    // This affects load distribution as stiffer anchors take more load initially\n    return 1.0f; // Placeholder - would be implemented based on anchor tool properties\n}\n\nfloat UAnchorSystem::CalculateQualityFactor(const FAnchorPoint& Anchor) const\n{\n    float StrengthFactor = Anchor.Strength / 25000.0f; // Normalize to typical anchor strength\n    float QualityFactor = Anchor.Quality;\n    \n    return FMath::Clamp(StrengthFactor * QualityFactor, 0.1f, 2.0f);\n}\n\nfloat UAnchorSystem::CalculateSystemExtension() const\n{\n    if (AnchorPoints.Num() < 2)\n        return 0.0f;\n    \n    // Calculate how much the system would extend under current load\n    // This is a simplified calculation - real implementation would consider\n    // rope stretch, anchor deformation, etc.\n    \n    float AverageExtension = 0.0f;\n    int32 ActiveCount = 0;\n    \n    for (const FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.bIsActive && !Anchor.bIsBackup)\n        {\n            // Simplified extension calculation based on load and anchor properties\n            float Extension = (Anchor.ActualLoad / Anchor.Strength) * 10.0f; // mm\n            AverageExtension += Extension;\n            ActiveCount++;\n        }\n    }\n    \n    if (ActiveCount > 0)\n    {\n        AverageExtension /= ActiveCount;\n    }\n    \n    return AverageExtension;\n}\n\nfloat UAnchorSystem::AnalyzeAnchorAngles() const\n{\n    if (AnchorPoints.Num() < 2)\n        return 0.0f;\n    \n    float MaxAngle = 0.0f;\n    \n    // Check angles between adjacent anchors\n    for (int32 i = 0; i < AnchorPoints.Num(); ++i)\n    {\n        for (int32 j = i + 1; j < AnchorPoints.Num(); ++j)\n        {\n            if (AnchorPoints[i].bIsActive && AnchorPoints[j].bIsActive)\n            {\n                FVector Dir1 = AnchorPoints[i].Direction;\n                FVector Dir2 = AnchorPoints[j].Direction;\n                \n                float DotProduct = FVector::DotProduct(Dir1, Dir2);\n                float Angle = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));\n                float AngleDegrees = FMath::RadiansToDegrees(Angle);\n                \n                MaxAngle = FMath::Max(MaxAngle, AngleDegrees);\n            }\n        }\n    }\n    \n    return MaxAngle;\n}\n\nfloat UAnchorSystem::CalculateSystemEfficiency() const\n{\n    if (SystemState.ActiveAnchors == 0)\n        return 0.0f;\n    \n    // Efficiency based on load distribution quality\n    float LoadVariance = 0.0f;\n    float AverageLoad = SystemState.TotalSystemLoad / SystemState.ActiveAnchors;\n    \n    for (const FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.bIsActive && !Anchor.bIsBackup)\n        {\n            float LoadDifference = Anchor.ActualLoad - AverageLoad;\n            LoadVariance += LoadDifference * LoadDifference;\n        }\n    }\n    \n    LoadVariance /= SystemState.ActiveAnchors;\n    float LoadStandardDeviation = FMath::Sqrt(LoadVariance);\n    \n    // Lower variance = higher efficiency\n    float VarianceRatio = LoadStandardDeviation / (AverageLoad + 1.0f);\n    float Efficiency = FMath::Clamp(1.0f - VarianceRatio, 0.1f, 1.0f);\n    \n    // Apply other efficiency factors\n    Efficiency *= Settings.LoadSharingEfficiency;\n    \n    // Angle penalty\n    float MaxAngle = AnalyzeAnchorAngles();\n    if (MaxAngle > Settings.OptimalAnchorAngle)\n    {\n        float AnglePenalty = (MaxAngle - Settings.OptimalAnchorAngle) / (Settings.MaxAnchorAngle - Settings.OptimalAnchorAngle);\n        Efficiency *= (1.0f - AnglePenalty * 0.3f);\n    }\n    \n    return FMath::Clamp(Efficiency, 0.1f, 1.0f);\n}\n\nbool UAnchorSystem::CheckLoadImbalance() const\n{\n    if (SystemState.ActiveAnchors <= 1)\n        return true;\n    \n    float MaxLoad = 0.0f;\n    float MinLoad = FLT_MAX;\n    \n    for (const FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.bIsActive && !Anchor.bIsBackup)\n        {\n            MaxLoad = FMath::Max(MaxLoad, Anchor.ActualLoad);\n            MinLoad = FMath::Min(MinLoad, Anchor.ActualLoad);\n        }\n    }\n    \n    if (MaxLoad <= 0.0f)\n        return true;\n    \n    float LoadImbalance = (MaxLoad - MinLoad) / MaxLoad;\n    return LoadImbalance <= Settings.MaxLoadImbalance;\n}\n\nvoid UAnchorSystem::UpdateSafetyFactor()\n{\n    if (SystemState.MaxSinglePointLoad <= 0.0f)\n    {\n        SystemState.SafetyFactor = 0.0f;\n        return;\n    }\n    \n    // Find the weakest anchor under current load\n    float WeakestAnchorCapacity = FLT_MAX;\n    \n    for (const FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.bIsActive && !Anchor.bIsBackup && Anchor.ActualLoad > 0.0f)\n        {\n            float EffectiveStrength = Anchor.Strength * Anchor.Quality * 1000.0f; // Convert kN to N\n            WeakestAnchorCapacity = FMath::Min(WeakestAnchorCapacity, EffectiveStrength);\n        }\n    }\n    \n    if (WeakestAnchorCapacity < FLT_MAX)\n    {\n        SystemState.SafetyFactor = WeakestAnchorCapacity / SystemState.MaxSinglePointLoad;\n    }\n    else\n    {\n        SystemState.SafetyFactor = 0.0f;\n    }\n}\n\nvoid UAnchorSystem::UpdateSystemState(float DeltaTime)\n{\n    // Update system integrity\n    SystemState.bSystemIntact = ValidateSystemIntegrity();\n    \n    // Check for warnings\n    if (SystemState.SafetyFactor < Settings.MinSafetyFactor && SystemState.SafetyFactor > 0.0f)\n    {\n        OnLoadImbalanceWarning.Broadcast();\n    }\n    \n    if (SystemState.MaxSinglePointLoad > 15000.0f) // 15kN warning threshold\n    {\n        OnSystemOverloaded.Broadcast();\n    }\n    \n    // Update failure state\n    if (bInFailureRedistribution)\n    {\n        LastFailureTime += DeltaTime;\n        if (LastFailureTime > Settings.FailureRedistributionTime)\n        {\n            bInFailureRedistribution = false;\n        }\n    }\n}\n\nvoid UAnchorSystem::HandleAnchorFailure(AClimbingToolBase* FailedAnchor)\n{\n    for (int32 i = 0; i < AnchorPoints.Num(); ++i)\n    {\n        if (AnchorPoints[i].AnchorTool == FailedAnchor)\n        {\n            AnchorPoints[i].bIsActive = false;\n            SystemState.ActiveAnchors--;\n            SystemState.FailedAnchors++;\n            FailedAnchorIndices.Add(i);\n            \n            OnAnchorFailure.Broadcast();\n            \n            // Start failure redistribution\n            bInFailureRedistribution = true;\n            LastFailureTime = 0.0f;\n            bNeedsRecalculation = true;\n            \n            // Activate backup anchors if available\n            if (Settings.bEnableProgressiveFailure)\n            {\n                ActivateBackupAnchors();\n            }\n            \n            // Check if system is still viable\n            if (SystemState.ActiveAnchors == 0)\n            {\n                OnSystemFailure.Broadcast();\n            }\n            \n            break;\n        }\n    }\n}\n\nvoid UAnchorSystem::ActivateBackupAnchors()\n{\n    for (FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.bIsBackup && !Anchor.bIsActive)\n        {\n            Anchor.bIsActive = true;\n            Anchor.bIsBackup = false; // Now primary\n            SystemState.ActiveAnchors++;\n            \n            UE_LOG(LogTemp, Warning, TEXT(\"Activated backup anchor due to system failure\"));\n        }\n    }\n}\n\nvoid UAnchorSystem::SimulateProgressiveFailure()\n{\n    // Redistribute load after anchor failure\n    // This simulates the dynamic load redistribution that occurs\n    // when an anchor fails in a real climbing system\n    \n    if (FailedAnchorIndices.Num() > 0)\n    {\n        RedistributeLoadAfterFailure(FailedAnchorIndices.Last());\n    }\n}\n\nvoid UAnchorSystem::RedistributeLoadAfterFailure(int32 FailedAnchorIndex)\n{\n    if (FailedAnchorIndex < 0 || FailedAnchorIndex >= AnchorPoints.Num())\n        return;\n    \n    // The load that was on the failed anchor needs to be redistributed\n    float FailedAnchorLoad = AnchorPoints[FailedAnchorIndex].ActualLoad;\n    AnchorPoints[FailedAnchorIndex].ActualLoad = 0.0f;\n    AnchorPoints[FailedAnchorIndex].LoadShare = 0.0f;\n    \n    // Redistribute the failed anchor's load to remaining active anchors\n    if (SystemState.ActiveAnchors > 0)\n    {\n        float AdditionalLoadPerAnchor = FailedAnchorLoad / SystemState.ActiveAnchors;\n        \n        for (FAnchorPoint& Anchor : AnchorPoints)\n        {\n            if (Anchor.bIsActive && !Anchor.bIsBackup)\n            {\n                Anchor.ActualLoad += AdditionalLoadPerAnchor;\n            }\n        }\n        \n        // Recalculate max single point load\n        float NewMaxLoad = 0.0f;\n        for (const FAnchorPoint& Anchor : AnchorPoints)\n        {\n            if (Anchor.bIsActive)\n            {\n                NewMaxLoad = FMath::Max(NewMaxLoad, Anchor.ActualLoad);\n            }\n        }\n        SystemState.MaxSinglePointLoad = NewMaxLoad;\n    }\n    \n    // Update safety factor\n    UpdateSafetyFactor();\n}\n\nbool UAnchorSystem::CanSystemSurviveFailure(AClimbingToolBase* AnchorTool) const\n{\n    // Simulate what would happen if this anchor failed\n    int32 RemainingAnchors = SystemState.ActiveAnchors;\n    float FailedAnchorLoad = 0.0f;\n    \n    // Find the anchor and its current load\n    for (const FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.AnchorTool == AnchorTool && Anchor.bIsActive)\n        {\n            FailedAnchorLoad = Anchor.ActualLoad;\n            RemainingAnchors--;\n            break;\n        }\n    }\n    \n    if (RemainingAnchors <= 0)\n        return false;\n    \n    // Check if remaining anchors can handle the redistributed load\n    float AdditionalLoadPerAnchor = FailedAnchorLoad / RemainingAnchors;\n    \n    for (const FAnchorPoint& Anchor : AnchorPoints)\n    {\n        if (Anchor.bIsActive && Anchor.AnchorTool != AnchorTool)\n        {\n            float NewLoad = Anchor.ActualLoad + AdditionalLoadPerAnchor;\n            float EffectiveStrength = Anchor.Strength * Anchor.Quality * 1000.0f; // Convert to N\n            \n            if (NewLoad > EffectiveStrength / Settings.MinSafetyFactor)\n            {\n                return false; // This anchor would be overloaded\n            }\n        }\n    }\n    \n    return true;\n}"