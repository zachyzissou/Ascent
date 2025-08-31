#include "FallMechanicsSystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// Advanced injury calculation and treatment methods

void UFallMechanicsSystem::GenerateInjuriesFromImpact(const FFallData& FallData)
{
    if (FallData.ImpactVelocity < Settings.MinInjuryVelocity)
        return;

    // Determine which body parts are affected based on impact direction
    TArray<EBodyPart> AffectedBodyParts = DetermineImpactBodyParts(FallData.ImpactDirection);
    
    for (EBodyPart BodyPart : AffectedBodyParts)
    {
        // Generate injuries for each affected body part
        TArray<FInjury> BodyPartInjuries = GenerateInjuriesForBodyPart(FallData, BodyPart);
        
        for (const FInjury& Injury : BodyPartInjuries)
        {
            ApplyInjury(Injury);
        }
    }
    
    // Apply shock effect from traumatic fall
    float ShockAmount = FMath::Clamp(FallData.ImpactVelocity / Settings.FatalInjuryVelocity, 0.0f, 1.0f);
    ApplyShockFromFall(ShockAmount);
}

TArray<EBodyPart> UFallMechanicsSystem::DetermineImpactBodyParts(const FVector& ImpactDirection) const
{
    TArray<EBodyPart> AffectedParts;
    
    // Analyze impact direction to determine which body parts hit first
    FVector NormalizedDirection = ImpactDirection.GetSafeNormal();
    
    // Vertical impact (feet first or head first)
    if (FMath::Abs(NormalizedDirection.Z) > 0.7f)
    {
        if (NormalizedDirection.Z > 0.0f)
        {
            // Feet first landing
            AffectedParts.Add(EBodyPart::LeftFoot);
            AffectedParts.Add(EBodyPart::RightFoot);
            AffectedParts.Add(EBodyPart::LeftLeg);
            AffectedParts.Add(EBodyPart::RightLeg);
            
            // Secondary impact - torso and head
            AffectedParts.Add(EBodyPart::Pelvis);
            AffectedParts.Add(EBodyPart::Back);
        }
        else
        {
            // Head first impact (very dangerous)
            AffectedParts.Add(EBodyPart::Head);
            AffectedParts.Add(EBodyPart::Neck);
            AffectedParts.Add(EBodyPart::LeftShoulder);
            AffectedParts.Add(EBodyPart::RightShoulder);
        }
    }
    // Side impact
    else if (FMath::Abs(NormalizedDirection.X) > 0.7f || FMath::Abs(NormalizedDirection.Y) > 0.7f)
    {
        // Side impact - arms and torso
        AffectedParts.Add(EBodyPart::LeftArm);
        AffectedParts.Add(EBodyPart::RightArm);
        AffectedParts.Add(EBodyPart::Chest);
        AffectedParts.Add(EBodyPart::Back);
        AffectedParts.Add(EBodyPart::LeftShoulder);
        AffectedParts.Add(EBodyPart::RightShoulder);
    }
    // Tumbling fall - affects everything
    else
    {
        // Multiple contact points - full body impact
        for (int32 i = 0; i < static_cast<int32>(EBodyPart::RightFoot) + 1; ++i)
        {
            AffectedParts.Add(static_cast<EBodyPart>(i));
        }
    }
    
    return AffectedParts;
}

TArray<FInjury> UFallMechanicsSystem::GenerateInjuriesForBodyPart(const FFallData& FallData, EBodyPart BodyPart) const
{
    TArray<FInjury> Injuries;
    
    float ImpactSeverity = FMath::Clamp(FallData.ImpactVelocity / Settings.SevereInjuryVelocity, 0.0f, 2.0f);
    float BodyPartVulnerability = GetBodyPartVulnerability(BodyPart);
    float BodyPartProtection = GetBodyPartProtection(BodyPart);
    
    // Adjust severity based on vulnerability and protection
    float AdjustedSeverity = ImpactSeverity * BodyPartVulnerability * (1.0f - BodyPartProtection);
    
    // Add randomness factor
    float RandomFactor = FMath::RandRange(1.0f - Settings.InjuryRandomness, 1.0f + Settings.InjuryRandomness);
    AdjustedSeverity *= RandomFactor;
    
    // Generate appropriate injuries based on severity and body part
    if (AdjustedSeverity > 0.3f)
    {
        // Minor injury guaranteed
        FInjury MinorInjury = CreateInjury(EInjuryType::Bruising, EInjurySeverity::Minor, BodyPart, AdjustedSeverity);
        Injuries.Add(MinorInjury);
    }
    
    if (AdjustedSeverity > 0.6f)
    {
        // Moderate injury possible
        EInjuryType ModerateType = DetermineInjuryType(FallData, BodyPart);
        if (ModerateType != EInjuryType::Bruising) // Don't duplicate bruising
        {
            FInjury ModerateInjury = CreateInjury(ModerateType, EInjurySeverity::Moderate, BodyPart, AdjustedSeverity);
            Injuries.Add(ModerateInjury);
        }
    }
    
    if (AdjustedSeverity > 1.0f)
    {
        // Severe injury possible
        EInjuryType SevereType = DetermineSevereInjuryType(FallData, BodyPart);
        FInjury SevereInjury = CreateInjury(SevereType, EInjurySeverity::Severe, BodyPart, AdjustedSeverity);
        Injuries.Add(SevereInjury);
    }
    
    if (AdjustedSeverity > 1.5f && IsBodyPartCritical(BodyPart))
    {
        // Critical injury for critical body parts only
        EInjuryType CriticalType = EInjuryType::InternalBleeding;
        if (BodyPart == EBodyPart::Head)
            CriticalType = EInjuryType::Concussion;
        else if (BodyPart == EBodyPart::Neck || BodyPart == EBodyPart::Back)
            CriticalType = EInjuryType::SpinalInjury;
        
        FInjury CriticalInjury = CreateInjury(CriticalType, EInjurySeverity::Critical, BodyPart, AdjustedSeverity);
        Injuries.Add(CriticalInjury);
    }
    
    return Injuries;
}

FInjury UFallMechanicsSystem::CreateInjury(EInjuryType Type, EInjurySeverity Severity, EBodyPart BodyPart, float ImpactSeverity) const
{
    FInjury NewInjury;
    NewInjury.Type = Type;
    NewInjury.Severity = Severity;
    NewInjury.BodyPart = BodyPart;
    
    // Calculate damage based on severity
    NewInjury.Damage = CalculateInjuryDamage(Type, Severity, ImpactSeverity);
    
    // Calculate healing time
    NewInjury.HealingTime = CalculateInjuryHealingTime(NewInjury);
    NewInjury.RemainingHealTime = NewInjury.HealingTime;
    
    // Determine performance impacts
    CalculateInjuryEffects(NewInjury);
    
    // Generate description
    NewInjury.Description = GenerateInjuryDescription(NewInjury);
    
    return NewInjury;
}

float UFallMechanicsSystem::CalculateInjuryDamage(EInjuryType Type, EInjurySeverity Severity, float ImpactSeverity) const
{
    float BaseDamage = 10.0f;
    
    // Base damage by severity
    switch (Severity)
    {
        case EInjurySeverity::Minor:
            BaseDamage = 10.0f;
            break;
        case EInjurySeverity::Moderate:
            BaseDamage = 25.0f;
            break;
        case EInjurySeverity::Severe:
            BaseDamage = 50.0f;
            break;
        case EInjurySeverity::Critical:
            BaseDamage = 75.0f;
            break;
        case EInjurySeverity::Fatal:
            BaseDamage = 100.0f;
            break;
    }
    
    // Modify by injury type
    switch (Type)
    {
        case EInjuryType::Bruising:
            BaseDamage *= 0.5f;
            break;
        case EInjuryType::Abrasion:
            BaseDamage *= 0.7f;
            break;
        case EInjuryType::Sprain:
        case EInjuryType::Strain:
            BaseDamage *= 0.8f;
            break;
        case EInjuryType::Fracture:
            BaseDamage *= 1.5f;
            break;
        case EInjuryType::Dislocation:
            BaseDamage *= 1.3f;
            break;
        case EInjuryType::Concussion:
            BaseDamage *= 2.0f;
            break;
        case EInjuryType::InternalBleeding:
        case EInjuryType::SpinalInjury:
            BaseDamage *= 3.0f;
            break;
    }
    
    return FMath::Clamp(BaseDamage * ImpactSeverity, 1.0f, 100.0f);
}

void UFallMechanicsSystem::CalculateInjuryEffects(FInjury& Injury) const
{
    // Reset effects
    Injury.bAffectsMovement = false;
    Injury.bAffectsGrip = false;
    Injury.bAffectsStamina = false;
    Injury.MovementPenalty = 0.0f;
    Injury.GripPenalty = 0.0f;
    Injury.StaminaPenalty = 0.0f;
    
    float SeverityMultiplier = 0.1f;
    switch (Injury.Severity)
    {
        case EInjurySeverity::Minor:
            SeverityMultiplier = 0.1f;
            break;
        case EInjurySeverity::Moderate:
            SeverityMultiplier = 0.25f;
            break;
        case EInjurySeverity::Severe:
            SeverityMultiplier = 0.5f;
            break;
        case EInjurySeverity::Critical:
            SeverityMultiplier = 0.75f;
            break;
        case EInjurySeverity::Fatal:
            SeverityMultiplier = 1.0f;
            break;
    }
    
    // Body part specific effects
    switch (Injury.BodyPart)
    {
        case EBodyPart::LeftLeg:
        case EBodyPart::RightLeg:
        case EBodyPart::LeftFoot:
        case EBodyPart::RightFoot:
        case EBodyPart::Pelvis:
            Injury.bAffectsMovement = true;
            Injury.MovementPenalty = SeverityMultiplier * 0.8f;
            Injury.bAffectsStamina = true;
            Injury.StaminaPenalty = SeverityMultiplier * 0.3f;
            break;
            
        case EBodyPart::LeftArm:
        case EBodyPart::RightArm:
        case EBodyPart::LeftHand:
        case EBodyPart::RightHand:
        case EBodyPart::LeftShoulder:
        case EBodyPart::RightShoulder:
            Injury.bAffectsGrip = true;
            Injury.GripPenalty = SeverityMultiplier;
            if (Injury.BodyPart == EBodyPart::LeftHand || Injury.BodyPart == EBodyPart::RightHand)
            {
                Injury.GripPenalty *= 1.5f; // Hands are more critical for grip
            }
            break;
            
        case EBodyPart::Head:
        case EBodyPart::Neck:
            Injury.bAffectsMovement = true;
            Injury.bAffectsGrip = true;
            Injury.bAffectsStamina = true;
            Injury.MovementPenalty = SeverityMultiplier * 0.6f;
            Injury.GripPenalty = SeverityMultiplier * 0.4f;
            Injury.StaminaPenalty = SeverityMultiplier * 0.5f;
            break;
            \n        case EBodyPart::Chest:\n        case EBodyPart::Back:\n        case EBodyPart::Abdomen:\n            Injury.bAffectsStamina = true;\n            Injury.StaminaPenalty = SeverityMultiplier * 0.6f;\n            if (Injury.Type == EInjuryType::InternalBleeding)\n            {\n                Injury.bAffectsMovement = true;\n                Injury.MovementPenalty = SeverityMultiplier * 0.4f;\n            }\n            break;\n    }\n    \n    // Injury type specific effects\n    switch (Injury.Type)\n    {\n        case EInjuryType::Fracture:\n            // Fractures severely limit movement\n            if (IsLimbBodyPart(Injury.BodyPart))\n            {\n                Injury.bAffectsMovement = true;\n                Injury.MovementPenalty = FMath::Max(Injury.MovementPenalty, SeverityMultiplier * 0.8f);\n            }\n            break;\n            \n        case EInjuryType::Dislocation:\n            // Dislocations affect specific joint function\n            Injury.bAffectsMovement = true;\n            Injury.MovementPenalty = FMath::Max(Injury.MovementPenalty, SeverityMultiplier * 0.6f);\n            break;\n            \n        case EInjuryType::Concussion:\n            // Concussions affect everything\n            Injury.bAffectsMovement = true;\n            Injury.bAffectsGrip = true;\n            Injury.bAffectsStamina = true;\n            Injury.MovementPenalty = SeverityMultiplier * 0.5f;\n            Injury.GripPenalty = SeverityMultiplier * 0.3f;\n            Injury.StaminaPenalty = SeverityMultiplier * 0.4f;\n            break;\n    }\n    \n    // Clamp all penalties\n    Injury.MovementPenalty = FMath::Clamp(Injury.MovementPenalty, 0.0f, 0.9f);\n    Injury.GripPenalty = FMath::Clamp(Injury.GripPenalty, 0.0f, 0.9f);\n    Injury.StaminaPenalty = FMath::Clamp(Injury.StaminaPenalty, 0.0f, 0.9f);\n}\n\nbool UFallMechanicsSystem::IsLimbBodyPart(EBodyPart BodyPart) const\n{\n    return BodyPart == EBodyPart::LeftArm || BodyPart == EBodyPart::RightArm ||\n           BodyPart == EBodyPart::LeftLeg || BodyPart == EBodyPart::RightLeg ||\n           BodyPart == EBodyPart::LeftHand || BodyPart == EBodyPart::RightHand ||\n           BodyPart == EBodyPart::LeftFoot || BodyPart == EBodyPart::RightFoot;\n}\n\nFString UFallMechanicsSystem::GenerateInjuryDescription(const FInjury& Injury) const\n{\n    FString SeverityText;\n    switch (Injury.Severity)\n    {\n        case EInjurySeverity::Minor:\n            SeverityText = TEXT(\"Minor\");\n            break;\n        case EInjurySeverity::Moderate:\n            SeverityText = TEXT(\"Moderate\");\n            break;\n        case EInjurySeverity::Severe:\n            SeverityText = TEXT(\"Severe\");\n            break;\n        case EInjurySeverity::Critical:\n            SeverityText = TEXT(\"Critical\");\n            break;\n        case EInjurySeverity::Fatal:\n            SeverityText = TEXT(\"Fatal\");\n            break;\n    }\n    \n    FString InjuryText;\n    switch (Injury.Type)\n    {\n        case EInjuryType::Bruising:\n            InjuryText = TEXT(\"bruising\");\n            break;\n        case EInjuryType::Abrasion:\n            InjuryText = TEXT(\"abrasion\");\n            break;\n        case EInjuryType::Sprain:\n            InjuryText = TEXT(\"sprain\");\n            break;\n        case EInjuryType::Strain:\n            InjuryText = TEXT(\"muscle strain\");\n            break;\n        case EInjuryType::Fracture:\n            InjuryText = TEXT(\"fracture\");\n            break;\n        case EInjuryType::Dislocation:\n            InjuryText = TEXT(\"dislocation\");\n            break;\n        case EInjuryType::Concussion:\n            InjuryText = TEXT(\"concussion\");\n            break;\n        case EInjuryType::InternalBleeding:\n            InjuryText = TEXT(\"internal bleeding\");\n            break;\n        case EInjuryType::SpinalInjury:\n            InjuryText = TEXT(\"spinal injury\");\n            break;\n    }\n    \n    FString BodyPartText = GetBodyPartName(Injury.BodyPart);\n    \n    return FString::Printf(TEXT(\"%s %s to %s\"), *SeverityText, *InjuryText, *BodyPartText);\n}\n\nFString UFallMechanicsSystem::GetBodyPartName(EBodyPart BodyPart) const\n{\n    switch (BodyPart)\n    {\n        case EBodyPart::Head: return TEXT(\"head\");\n        case EBodyPart::Neck: return TEXT(\"neck\");\n        case EBodyPart::LeftShoulder: return TEXT(\"left shoulder\");\n        case EBodyPart::RightShoulder: return TEXT(\"right shoulder\");\n        case EBodyPart::LeftArm: return TEXT(\"left arm\");\n        case EBodyPart::RightArm: return TEXT(\"right arm\");\n        case EBodyPart::LeftHand: return TEXT(\"left hand\");\n        case EBodyPart::RightHand: return TEXT(\"right hand\");\n        case EBodyPart::Chest: return TEXT(\"chest\");\n        case EBodyPart::Back: return TEXT(\"back\");\n        case EBodyPart::Abdomen: return TEXT(\"abdomen\");\n        case EBodyPart::Pelvis: return TEXT(\"pelvis\");\n        case EBodyPart::LeftLeg: return TEXT(\"left leg\");\n        case EBodyPart::RightLeg: return TEXT(\"right leg\");\n        case EBodyPart::LeftFoot: return TEXT(\"left foot\");\n        case EBodyPart::RightFoot: return TEXT(\"right foot\");\n        default: return TEXT(\"unknown\");\n    }\n}\n\nfloat UFallMechanicsSystem::GetBodyPartVulnerability(EBodyPart BodyPart) const\n{\n    switch (BodyPart)\n    {\n        case EBodyPart::Head:\n        case EBodyPart::Neck:\n            return 2.0f; // Very vulnerable\n        case EBodyPart::LeftHand:\n        case EBodyPart::RightHand:\n        case EBodyPart::LeftFoot:\n        case EBodyPart::RightFoot:\n            return 1.5f; // Vulnerable extremities\n        case EBodyPart::LeftArm:\n        case EBodyPart::RightArm:\n        case EBodyPart::LeftLeg:\n        case EBodyPart::RightLeg:\n            return 1.2f; // Moderately vulnerable\n        case EBodyPart::Chest:\n        case EBodyPart::Back:\n        case EBodyPart::Abdomen:\n        case EBodyPart::Pelvis:\n            return 1.0f; // Standard vulnerability\n        case EBodyPart::LeftShoulder:\n        case EBodyPart::RightShoulder:\n            return 1.3f; // Joint vulnerability\n        default:\n            return 1.0f;\n    }\n}\n\nfloat UFallMechanicsSystem::GetBodyPartProtection(EBodyPart BodyPart) const\n{\n    // This would be enhanced to check for protective gear\n    // For now, assume minimal protection\n    return 0.1f; // 10% base protection from clothing/natural padding\n}\n\nbool UFallMechanicsSystem::IsBodyPartCritical(EBodyPart BodyPart) const\n{\n    return BodyPart == EBodyPart::Head || \n           BodyPart == EBodyPart::Neck || \n           BodyPart == EBodyPart::Chest ||\n           BodyPart == EBodyPart::Abdomen ||\n           BodyPart == EBodyPart::Back;\n}\n\nvoid UFallMechanicsSystem::ApplyShockFromFall(float ShockAmount)\n{\n    InjuryState.ShockLevel = FMath::Clamp(InjuryState.ShockLevel + ShockAmount, 0.0f, 1.0f);\n    \n    if (InjuryState.ShockLevel > SHOCK_INCAPACITATION_THRESHOLD)\n    {\n        InjuryState.bIsConscious = false;\n        OnBecameIncapacitated.Broadcast();\n    }\n}\n\nvoid UFallMechanicsSystem::ApplyShockFromInjury(const FInjury& Injury)\n{\n    float ShockAmount = 0.0f;\n    \n    switch (Injury.Severity)\n    {\n        case EInjurySeverity::Minor:\n            ShockAmount = 0.05f;\n            break;\n        case EInjurySeverity::Moderate:\n            ShockAmount = 0.15f;\n            break;\n        case EInjurySeverity::Severe:\n            ShockAmount = 0.3f;\n            break;\n        case EInjurySeverity::Critical:\n            ShockAmount = 0.6f;\n            break;\n        case EInjurySeverity::Fatal:\n            ShockAmount = 1.0f;\n            break;\n    }\n    \n    // Critical body parts cause more shock\n    if (IsBodyPartCritical(Injury.BodyPart))\n    {\n        ShockAmount *= 1.5f;\n    }\n    \n    ApplyShockFromFall(ShockAmount);\n}\n\nvoid UFallMechanicsSystem::UpdateShockEffects(float DeltaTime)\n{\n    if (InjuryState.ShockLevel > 0.0f)\n    {\n        // Shock recovers over time\n        float RecoveryAmount = Settings.ShockRecoveryRate * DeltaTime;\n        InjuryState.ShockLevel = FMath::Max(0.0f, InjuryState.ShockLevel - RecoveryAmount);\n        \n        // Check if consciousness is regained\n        if (!InjuryState.bIsConscious && InjuryState.ShockLevel < SHOCK_INCAPACITATION_THRESHOLD)\n        {\n            InjuryState.bIsConscious = true;\n            UE_LOG(LogTemp, Log, TEXT(\"Regained consciousness after shock recovery\"));\n        }\n    }\n}\n\nvoid UFallMechanicsSystem::CheckForIncapacitation()\n{\n    bool bWasCapacitated = InjuryState.bCanClimb;\n    \n    // Check overall health\n    if (InjuryState.OverallHealth < UNCONSCIOUSNESS_THRESHOLD)\n    {\n        InjuryState.bIsConscious = false;\n        InjuryState.bCanClimb = false;\n    }\n    else if (InjuryState.OverallHealth < EVACUATION_THRESHOLD)\n    {\n        InjuryState.bRequiresMedicalAttention = true;\n        InjuryState.bCanClimb = false;\n    }\n    else\n    {\n        InjuryState.bCanClimb = true;\n        InjuryState.bRequiresMedicalAttention = false;\n    }\n    \n    // Check for severe injuries that prevent climbing\n    for (const FInjury& Injury : InjuryState.ActiveInjuries)\n    {\n        if (Injury.Severity == EInjurySeverity::Critical || Injury.Severity == EInjurySeverity::Fatal)\n        {\n            if (Injury.Type == EInjuryType::Fracture || Injury.Type == EInjuryType::SpinalInjury)\n            {\n                InjuryState.bCanClimb = false;\n                InjuryState.bRequiresMedicalAttention = true;\n            }\n        }\n    }\n    \n    // Broadcast events for state changes\n    if (bWasCapacitated && !InjuryState.bCanClimb)\n    {\n        OnBecameIncapacitated.Broadcast();\n    }\n    \n    if (InjuryState.bRequiresMedicalAttention)\n    {\n        OnRequireEvacuation.Broadcast();\n    }\n}\n\nfloat UFallMechanicsSystem::DetermineSurfaceHardness(AActor* Surface) const\n{\n    if (!Surface)\n        return Settings.SurfaceHardnessMultiplier;\n    \n    // Check for surface tags\n    if (Surface->ActorHasTag(\"Snow\"))\n        return 0.3f;\n    if (Surface->ActorHasTag(\"Water\"))\n        return 0.1f;\n    if (Surface->ActorHasTag(\"Sand\"))\n        return 0.4f;\n    if (Surface->ActorHasTag(\"Grass\"))\n        return 0.5f;\n    if (Surface->ActorHasTag(\"Rock\"))\n        return 1.0f;\n    if (Surface->ActorHasTag(\"Ice\"))\n        return 1.2f;\n    \n    // Default to rock hardness\n    return Settings.SurfaceHardnessMultiplier;\n}"