#include "FallMechanicsSystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

UFallMechanicsSystem::UFallMechanicsSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0167f; // 60 FPS
    SetIsReplicatedByDefault(true);

    // Initialize settings
    Settings.GravityMultiplier = 1.0f;
    Settings.AirResistance = 0.02f;
    Settings.TerminalVelocity = 56.0f;
    Settings.ClimberMass = 70.0f;
    Settings.MinInjuryVelocity = 3.0f;
    Settings.SevereInjuryVelocity = 8.0f;
    Settings.FatalInjuryVelocity = 15.0f;
    Settings.InjuryRandomness = 0.2f;
    Settings.RopeStretchFactor = 0.08f;
    Settings.RopeDampingFactor = 0.3f;
    Settings.MaxRopeForce = 12000.0f;
    Settings.GroundStiffness = 100000.0f;
    Settings.GroundDamping = 5000.0f;
    Settings.SurfaceHardnessMultiplier = 1.0f;
    Settings.HealingRateMultiplier = 1.0f;
    Settings.RestHealingBonus = 2.0f;
    Settings.bEnableNaturalHealing = true;
    Settings.ShockRecoveryRate = 0.1f;

    // Initialize injury state
    InjuryState.ActiveInjuries.Empty();
    InjuryState.OverallHealth = 100.0f;
    InjuryState.MovementMultiplier = 1.0f;
    InjuryState.GripMultiplier = 1.0f;
    InjuryState.StaminaMultiplier = 1.0f;
    InjuryState.bIsConscious = true;
    InjuryState.bCanClimb = true;
    InjuryState.bRequiresMedicalAttention = false;
    InjuryState.TotalFalls = 0;
    InjuryState.ShockLevel = 0.0f;

    // Initialize fall tracking
    bIsFalling = false;
    FallStartTime = 0.0f;
    FallStartLocation = FVector::ZeroVector;
    
    FallHistory.Reserve(MaxFallHistorySize);
    TreatmentEffectiveness.Empty();
}

void UFallMechanicsSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize treatment effectiveness tracking
    TreatmentEffectiveness.Add(EInjuryType::Bruising, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::Abrasion, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::Sprain, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::Strain, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::Fracture, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::Dislocation, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::Concussion, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::InternalBleeding, 0.0f);
    TreatmentEffectiveness.Add(EInjuryType::SpinalInjury, 0.0f);
}

void UFallMechanicsSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwnerRole() == ROLE_Authority)
    {
        // Update injury healing
        LastInjuryUpdate += DeltaTime;
        if (LastInjuryUpdate >= InjuryUpdateInterval)
        {
            UpdateInjuryHealing(LastInjuryUpdate);
            UpdateShockEffects(LastInjuryUpdate);
            UpdatePerformanceMultipliers();
            CheckForIncapacitation();
            RemoveHeatedInjuries();
            LastInjuryUpdate = 0.0f;
        }

        // Track fall progress if falling
        if (bIsFalling)
        {
            CurrentFall.TimeInAir += DeltaTime;
            
            // Update fall distance
            FVector CurrentLocation = GetOwner()->GetActorLocation();
            CurrentFall.FallDistance = FVector::Dist(FallStartLocation, CurrentLocation) * 0.01f; // Convert to meters
        }
    }
}

void UFallMechanicsSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UFallMechanicsSystem, InjuryState);
}

void UFallMechanicsSystem::StartFall(EFallType FallType)
{
    if (bIsFalling)
        return;

    bIsFalling = true;
    FallStartTime = GetWorld()->GetTimeSeconds();
    FallStartLocation = GetOwner()->GetActorLocation();
    
    CurrentFall = FFallData();
    CurrentFall.FallType = FallType;
    CurrentFall.TimeInAir = 0.0f;
    CurrentFall.FallDistance = 0.0f;
    CurrentFall.bRopeCaught = false;
    
    InjuryState.TotalFalls++;
    
    OnFallStarted.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Fall started: Type=%d"), static_cast<int32>(FallType));
}

void UFallMechanicsSystem::EndFall(const FVector& ImpactLocation, const FVector& ImpactVelocity, AActor* ImpactSurface)
{
    if (!bIsFalling)
        return;

    bIsFalling = false;
    
    // Complete fall data
    CurrentFall.ImpactLocation = ImpactLocation;
    CurrentFall.ImpactVelocity = ImpactVelocity.Size() * 0.01f; // Convert cm/s to m/s
    CurrentFall.ImpactDirection = ImpactVelocity.GetSafeNormal();
    CurrentFall.ImpactSurface = ImpactSurface;
    CurrentFall.FallDistance = FVector::Dist(FallStartLocation, ImpactLocation) * 0.01f; // Convert to meters
    
    // Calculate impact force
    if (CurrentFall.FallType == EFallType::Rope || CurrentFall.FallType == EFallType::Factor2)
    {
        // Rope fall - use rope dynamics
        CurrentFall.ImpactForce = Settings.MaxRopeForce * FMath::Clamp(CurrentFall.ImpactVelocity / 10.0f, 0.1f, 1.0f);
    }
    else
    {
        // Ground fall - use ground impact calculation
        float SurfaceHardness = DetermineSurfaceHardness(ImpactSurface);
        CurrentFall.ImpactForce = CalculateGroundImpactForce(CurrentFall.ImpactVelocity, SurfaceHardness);
    }

    // Generate injuries from the fall
    GenerateInjuriesFromImpact(CurrentFall);
    
    // Store in fall history
    if (FallHistory.Num() >= MaxFallHistorySize)
    {
        FallHistory.RemoveAt(0);
    }
    FallHistory.Add(CurrentFall);
    
    OnFallEnded.Broadcast();
    
    UE_LOG(LogTemp, Warning, TEXT("Fall ended: Distance=%.1fm, Velocity=%.1fm/s, Force=%.0fN"), 
           CurrentFall.FallDistance, CurrentFall.ImpactVelocity, CurrentFall.ImpactForce);
}

FFallData UFallMechanicsSystem::AnalyzeFall(const FVector& StartLocation, const FVector& EndLocation, 
                                           const FVector& ImpactVelocity, EFallType FallType) const
{
    FFallData Analysis;
    Analysis.FallType = FallType;
    Analysis.FallDistance = FVector::Dist(StartLocation, EndLocation) * 0.01f; // Convert to meters
    Analysis.ImpactVelocity = ImpactVelocity.Size() * 0.01f; // Convert to m/s
    Analysis.ImpactDirection = ImpactVelocity.GetSafeNormal();
    Analysis.ImpactLocation = EndLocation;
    
    // Calculate fall factor for rope falls
    if (FallType == EFallType::Rope || FallType == EFallType::Factor2)
    {
        // Estimate rope length from fall distance (simplified)
        float EstimatedRopeLength = Analysis.FallDistance * 0.5f; // Assume 50% rope stretch
        Analysis.FallFactor = CalculateFallFactor(Analysis.FallDistance, EstimatedRopeLength);
    }
    
    return Analysis;
}

void UFallMechanicsSystem::HandleRopeFall(UAdvancedRopeComponent* Rope, float FallDistance)
{
    if (!Rope)
        return;

    CurrentFall.bRopeCaught = true;
    
    // Calculate rope fall characteristics
    float RopeLength = Rope->Properties.Length;
    CurrentFall.FallFactor = CalculateFallFactor(FallDistance, RopeLength);
    
    // Determine if this is a factor 2 fall
    if (CurrentFall.FallFactor >= 1.8f)
    {
        CurrentFall.FallType = EFallType::Factor2;
    }
    
    // Calculate rope fall force
    float ClimberVelocity = FMath::Sqrt(2.0f * 9.81f * FallDistance); // v = sqrt(2gh)
    CurrentFall.ImpactForce = CalculateRopeFallForce(Rope, FallDistance, ClimberVelocity);
    
    // Apply rope stretch energy absorption
    float EnergyAbsorbed = CurrentFall.ImpactForce * RopeLength * Settings.RopeStretchFactor;
    CurrentFall.ImpactForce *= (1.0f - Settings.RopeDampingFactor);
    
    OnRopeCatch.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Rope fall caught: Factor=%.2f, Force=%.0fN"), 
           CurrentFall.FallFactor, CurrentFall.ImpactForce);
}

float UFallMechanicsSystem::CalculateRopeFallForce(UAdvancedRopeComponent* Rope, float FallDistance, float ClimberVelocity) const
{
    if (!Rope)
        return 0.0f;

    // Use simplified rope dynamics formula: F = sqrt(2 * m * g * h * k)
    // where k is rope stiffness derived from elongation properties
    
    float RopeStiffness = 1.0f / (Rope->Properties.DynamicElongation * 0.01f); // Convert percentage to decimal
    float Force = FMath::Sqrt(2.0f * Settings.ClimberMass * 9.81f * FallDistance * RopeStiffness);
    
    // Apply rope type modifiers
    switch (Rope->RopeType)
    {
        case ERopeType::Dynamic:
            Force *= 0.8f; // Dynamic ropes absorb more energy
            break;
        case ERopeType::Static:
            Force *= 1.2f; // Static ropes are stiffer
            break;
        case ERopeType::Steel:
            Force *= 2.0f; // Steel cables are very stiff
            break;
        default:
            break;
    }
    
    return FMath::Min(Force, Settings.MaxRopeForce);
}

float UFallMechanicsSystem::CalculateFallFactor(float FallDistance, float RopeLength) const
{
    if (RopeLength <= 0.0f)
        return 0.0f;
    
    return FallDistance / RopeLength;
}

void UFallMechanicsSystem::HandleGroundImpact(const FVector& ImpactVelocity, const FVector& ImpactLocation, AActor* Surface)
{
    float ImpactSpeed = ImpactVelocity.Size() * 0.01f; // Convert to m/s
    float SurfaceHardness = DetermineSurfaceHardness(Surface);
    
    CurrentFall.ImpactForce = CalculateGroundImpactForce(ImpactSpeed, SurfaceHardness);
    CurrentFall.ImpactVelocity = ImpactSpeed;
    CurrentFall.ImpactLocation = ImpactLocation;
    CurrentFall.ImpactDirection = ImpactVelocity.GetSafeNormal();
    CurrentFall.ImpactSurface = Surface;
    
    OnGroundImpact.Broadcast();
    
    UE_LOG(LogTemp, Error, TEXT("Ground impact: Velocity=%.1fm/s, Force=%.0fN, Surface=%s"), 
           ImpactSpeed, CurrentFall.ImpactForce, Surface ? *Surface->GetName() : TEXT("Unknown"));
}

float UFallMechanicsSystem::CalculateGroundImpactForce(float ImpactVelocity, float SurfaceHardness) const
{
    // F = m * a, where a is deceleration over impact duration
    // Impact duration depends on surface deformation and body compression
    
    float ImpactTime = 0.1f / SurfaceHardness; // Harder surfaces = shorter impact time
    float Deceleration = ImpactVelocity / ImpactTime;
    float Force = Settings.ClimberMass * Deceleration;
    
    return Force * Settings.SurfaceHardnessMultiplier;
}

void UFallMechanicsSystem::ApplyInjury(const FInjury& Injury)
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerApplyInjury(Injury);
        return;
    }

    // Add injury to active list
    InjuryState.ActiveInjuries.Add(Injury);
    
    // Update overall health
    InjuryState.OverallHealth = FMath::Max(0.0f, InjuryState.OverallHealth - Injury.Damage);
    
    // Apply shock from severe injuries
    ApplyShockFromInjury(Injury);
    
    // Update performance multipliers
    UpdatePerformanceMultipliers();
    
    OnInjuryInflicted.Broadcast();
    
    UE_LOG(LogTemp, Warning, TEXT("Injury applied: %s to %s (Severity=%d, Damage=%.1f)"), 
           *UEnum::GetValueAsString(Injury.Type),
           *UEnum::GetValueAsString(Injury.BodyPart),
           static_cast<int32>(Injury.Severity),
           Injury.Damage);
}

void UFallMechanicsSystem::ServerApplyInjury_Implementation(const FInjury& Injury)
{
    ApplyInjury(Injury);
}

bool UFallMechanicsSystem::ServerApplyInjury_Validate(const FInjury& Injury)
{
    return Injury.Damage >= 0.0f && Injury.Damage <= 100.0f;
}

void UFallMechanicsSystem::GenerateInjuriesFromImpact(const FFallData& FallData)
{
    if (FallData.ImpactVelocity < Settings.MinInjuryVelocity)
        return; // No injury from low-velocity impacts

    // Determine primary impact body part
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    FVector CharacterRotation = Character ? Character->GetActorForwardVector() : FVector::ForwardVector;
    EBodyPart PrimaryBodyPart = DetermineImpactBodyPart(FallData.ImpactDirection, CharacterRotation);
    
    // Generate primary injury
    EInjurySeverity Severity = DetermineInjurySeverity(FallData.ImpactForce, PrimaryBodyPart);
    EInjuryType Type = DetermineInjuryType(FallData, PrimaryBodyPart);
    
    FInjury PrimaryInjury;
    PrimaryInjury.Type = Type;
    PrimaryInjury.Severity = Severity;
    PrimaryInjury.BodyPart = PrimaryBodyPart;
    PrimaryInjury.Damage = CalculateInjuryDamage(FallData.ImpactForce, Severity);
    PrimaryInjury.HealingTime = CalculateInjuryHealingTime(PrimaryInjury);
    PrimaryInjury.RemainingHealTime = PrimaryInjury.HealingTime;
    PrimaryInjury.Description = GenerateInjuryDescription(PrimaryInjury);
    
    SetInjuryEffects(PrimaryInjury);
    ApplyInjury(PrimaryInjury);
    
    // Generate secondary injuries for severe impacts
    if (FallData.ImpactVelocity > Settings.SevereInjuryVelocity)
    {
        TArray<FInjury> SecondaryInjuries = GenerateTraumaticInjuries(FallData.ImpactForce, FallData.ImpactDirection);
        for (const FInjury& SecondaryInjury : SecondaryInjuries)
        {
            ApplyInjury(SecondaryInjury);
        }
    }
    
    // Generate abrasion injuries for sliding falls
    if (FallData.FallType == EFallType::Slide)
    {
        TArray<FInjury> AbrasionInjuries = GenerateAbrasionInjuries(FallData.FallDistance, FallData.ImpactSurface);
        for (const FInjury& AbrasionInjury : AbrasionInjuries)
        {
            ApplyInjury(AbrasionInjury);
        }
    }
}

EInjurySeverity UFallMechanicsSystem::DetermineInjurySeverity(float ImpactForce, EBodyPart BodyPart) const
{
    // Base severity on impact force
    float VulnerabilityMultiplier = GetBodyPartVulnerability(BodyPart);
    float ProtectionMultiplier = GetBodyPartProtection(BodyPart);
    float EffectiveForce = ImpactForce * VulnerabilityMultiplier * (1.0f - ProtectionMultiplier);
    
    // Add randomness
    float RandomFactor = FMath::RandRange(1.0f - Settings.InjuryRandomness, 1.0f + Settings.InjuryRandomness);
    EffectiveForce *= RandomFactor;
    
    // Determine severity thresholds
    if (EffectiveForce < 1000.0f) return EInjurySeverity::Minor;
    if (EffectiveForce < 3000.0f) return EInjurySeverity::Moderate;
    if (EffectiveForce < 6000.0f) return EInjurySeverity::Severe;
    if (EffectiveForce < 12000.0f) return EInjurySeverity::Critical;
    return EInjurySeverity::Fatal;
}

EInjuryType UFallMechanicsSystem::DetermineInjuryType(const FFallData& FallData, EBodyPart BodyPart) const
{
    // Determine injury type based on fall characteristics and body part
    
    if (FallData.FallType == EFallType::Slide)
    {
        return EInjuryType::Abrasion;
    }
    
    if (BodyPart == EBodyPart::Head)
    {
        if (FallData.ImpactForce > 3000.0f)
            return EInjuryType::Concussion;
        else
            return EInjuryType::Bruising;
    }
    
    if (BodyPart == EBodyPart::Neck || BodyPart == EBodyPart::Back)
    {
        if (FallData.ImpactForce > 5000.0f)
            return EInjuryType::SpinalInjury;
        else
            return EInjuryType::Strain;
    }
    
    // Limbs and extremities
    if (BodyPart == EBodyPart::LeftArm || BodyPart == EBodyPart::RightArm ||
        BodyPart == EBodyPart::LeftLeg || BodyPart == EBodyPart::RightLeg)
    {
        if (FallData.ImpactForce > 4000.0f)
            return EInjuryType::Fracture;
        else if (FallData.ImpactForce > 2000.0f)
            return EInjuryType::Sprain;
        else
            return EInjuryType::Strain;
    }
    
    // Joints
    if (BodyPart == EBodyPart::LeftShoulder || BodyPart == EBodyPart::RightShoulder)
    {
        if (FallData.ImpactForce > 3000.0f)
            return EInjuryType::Dislocation;
        else
            return EInjuryType::Sprain;
    }
    
    // Torso
    if (BodyPart == EBodyPart::Chest || BodyPart == EBodyPart::Abdomen)
    {
        if (FallData.ImpactForce > 6000.0f)
            return EInjuryType::InternalBleeding;
        else
            return EInjuryType::Bruising;
    }
    
    return EInjuryType::Bruising; // Default
}

EBodyPart UFallMechanicsSystem::DetermineImpactBodyPart(const FVector& ImpactDirection, const FVector& CharacterRotation) const
{
    // Determine which body part takes the primary impact based on impact direction
    
    FVector NormalizedImpact = ImpactDirection.GetSafeNormal();
    
    // Check for head-first impacts
    if (NormalizedImpact.Z < -0.7f) // Vertical downward
    {
        return EBodyPart::Head;
    }
    
    // Check for feet-first impacts
    if (NormalizedImpact.Z > 0.7f) // Vertical upward
    {
        return FMath::RandBool() ? EBodyPart::LeftFoot : EBodyPart::RightFoot;
    }
    
    // Side impacts
    FVector RightVector = FVector::CrossProduct(CharacterRotation, FVector::UpVector);
    float SideDot = FVector::DotProduct(NormalizedImpact, RightVector);
    
    if (SideDot > 0.5f) // Right side impact
    {
        float RandomValue = FMath::RandRange(0.0f, 1.0f);
        if (RandomValue < 0.3f) return EBodyPart::RightShoulder;
        if (RandomValue < 0.6f) return EBodyPart::RightArm;
        return EBodyPart::RightLeg;
    }
    else if (SideDot < -0.5f) // Left side impact
    {
        float RandomValue = FMath::RandRange(0.0f, 1.0f);
        if (RandomValue < 0.3f) return EBodyPart::LeftShoulder;
        if (RandomValue < 0.6f) return EBodyPart::LeftArm;
        return EBodyPart::LeftLeg;
    }
    
    // Front/back impacts
    float ForwardDot = FVector::DotProduct(NormalizedImpact, CharacterRotation);
    
    if (ForwardDot > 0.5f) // Front impact
    {
        float RandomValue = FMath::RandRange(0.0f, 1.0f);
        if (RandomValue < 0.4f) return EBodyPart::Chest;
        return EBodyPart::Abdomen;
    }
    else // Back impact
    {
        return EBodyPart::Back;
    }
}

float UFallMechanicsSystem::CalculateInjuryHealingTime(const FInjury& Injury) const
{
    float BaseTime = 0.0f;
    
    switch (Injury.Type)
    {
        case EInjuryType::Bruising:
            BaseTime = 3600.0f; // 1 hour
            break;
        case EInjuryType::Abrasion:
            BaseTime = 1800.0f; // 30 minutes
            break;
        case EInjuryType::Sprain:
            BaseTime = 7200.0f; // 2 hours
            break;
        case EInjuryType::Strain:
            BaseTime = 3600.0f; // 1 hour
            break;
        case EInjuryType::Fracture:
            BaseTime = 21600.0f; // 6 hours (simplified)
            break;
        case EInjuryType::Dislocation:
            BaseTime = 10800.0f; // 3 hours
            break;
        case EInjuryType::Concussion:
            BaseTime = 14400.0f; // 4 hours
            break;
        case EInjuryType::InternalBleeding:
            BaseTime = 1800.0f; // 30 minutes (critical - needs immediate attention)
            break;
        case EInjuryType::SpinalInjury:
            BaseTime = 43200.0f; // 12 hours (very serious)
            break;
    }
    
    // Severity multiplier
    float SeverityMultiplier = 1.0f;
    switch (Injury.Severity)
    {
        case EInjurySeverity::Minor:
            SeverityMultiplier = 0.5f;
            break;
        case EInjurySeverity::Moderate:
            SeverityMultiplier = 1.0f;
            break;
        case EInjurySeverity::Severe:
            SeverityMultiplier = 2.0f;
            break;
        case EInjurySeverity::Critical:
            SeverityMultiplier = 4.0f;
            break;
        case EInjurySeverity::Fatal:
            SeverityMultiplier = -1.0f; // Permanent
            break;
        default:
            break;
    }
    
    if (SeverityMultiplier < 0.0f)
        return -1.0f; // Permanent injury
    
    return BaseTime * SeverityMultiplier / Settings.HealingRateMultiplier;
}

void UFallMechanicsSystem::UpdateInjuryHealing(float DeltaTime)
{
    if (!Settings.bEnableNaturalHealing)
        return;

    bool AnyHealed = false;
    
    for (FInjury& Injury : InjuryState.ActiveInjuries)
    {
        if (Injury.RemainingHealTime > 0.0f)
        {
            float HealingRate = DeltaTime;
            
            // Bonus healing when resting
            if (IsOwnerResting())
            {
                HealingRate *= Settings.RestHealingBonus;
            }
            
            // Apply treatment effectiveness
            if (TreatmentEffectiveness.Contains(Injury.Type))
            {
                HealingRate *= (1.0f + TreatmentEffectiveness[Injury.Type]);
            }
            
            Injury.RemainingHealTime -= HealingRate;
            
            if (Injury.RemainingHealTime <= 0.0f)
            {
                Injury.RemainingHealTime = 0.0f;
                AnyHealed = true;
                
                // Restore some health when injury heals
                InjuryState.OverallHealth = FMath::Min(100.0f, InjuryState.OverallHealth + Injury.Damage * 0.5f);
            }
        }
    }
    
    if (AnyHealed)
    {
        OnInjuryHealed.Broadcast();
    }
}

void UFallMechanicsSystem::RemoveHeatedInjuries()
{
    InjuryState.ActiveInjuries.RemoveAll([](const FInjury& Injury) {
        return Injury.RemainingHealTime <= 0.0f && Injury.HealingTime > 0.0f;
    });
}

void UFallMechanicsSystem::UpdatePerformanceMultipliers()
{
    float MovementPenalty = 0.0f;
    float GripPenalty = 0.0f;
    float StaminaPenalty = 0.0f;
    
    for (const FInjury& Injury : InjuryState.ActiveInjuries)
    {
        if (Injury.RemainingHealTime > 0.0f) // Only active injuries affect performance
        {
            MovementPenalty += Injury.MovementPenalty;
            GripPenalty += Injury.GripPenalty;
            StaminaPenalty += Injury.StaminaPenalty;
        }
    }
    
    // Add shock effects
    float ShockMultiplier = 1.0f - InjuryState.ShockLevel;
    
    InjuryState.MovementMultiplier = FMath::Max(0.1f, (1.0f - MovementPenalty) * ShockMultiplier);
    InjuryState.GripMultiplier = FMath::Max(0.1f, (1.0f - GripPenalty) * ShockMultiplier);
    InjuryState.StaminaMultiplier = FMath::Max(0.1f, (1.0f - StaminaPenalty) * ShockMultiplier);
    
    // Update capability flags
    InjuryState.bCanClimb = (InjuryState.MovementMultiplier > 0.3f && InjuryState.GripMultiplier > 0.3f);
    InjuryState.bRequiresMedicalAttention = (InjuryState.OverallHealth < 50.0f || HasCriticalInjuries());
}

void UFallMechanicsSystem::ApplyShockFromInjury(const FInjury& Injury)
{
    float ShockAmount = 0.0f;
    
    switch (Injury.Severity)
    {
        case EInjurySeverity::Minor:
            ShockAmount = 0.02f;
            break;
        case EInjurySeverity::Moderate:
            ShockAmount = 0.05f;
            break;
        case EInjurySeverity::Severe:
            ShockAmount = 0.15f;
            break;
        case EInjurySeverity::Critical:
            ShockAmount = 0.3f;
            break;
        case EInjurySeverity::Fatal:
            ShockAmount = 0.8f;
            break;
        default:
            break;
    }
    
    // Critical body parts cause more shock
    if (IsBodyPartCritical(Injury.BodyPart))
    {
        ShockAmount *= 1.5f;
    }
    
    InjuryState.ShockLevel = FMath::Min(1.0f, InjuryState.ShockLevel + ShockAmount);
}

void UFallMechanicsSystem::UpdateShockEffects(float DeltaTime)
{
    // Natural shock recovery
    if (InjuryState.ShockLevel > 0.0f)
    {
        float RecoveryRate = Settings.ShockRecoveryRate;
        
        if (IsOwnerResting())
        {
            RecoveryRate *= 2.0f; // Faster recovery when resting
        }
        
        InjuryState.ShockLevel = FMath::Max(0.0f, InjuryState.ShockLevel - RecoveryRate * DeltaTime);
    }
}

void UFallMechanicsSystem::CheckForIncapacitation()
{
    bool WasConscious = InjuryState.bIsConscious;
    
    // Check consciousness based on health and shock
    InjuryState.bIsConscious = (InjuryState.OverallHealth > UNCONSCIOUSNESS_THRESHOLD && 
                               InjuryState.ShockLevel < SHOCK_INCAPACITATION_THRESHOLD);
    
    if (WasConscious && !InjuryState.bIsConscious)
    {
        OnBecameIncapacitated.Broadcast();
        UE_LOG(LogTemp, Error, TEXT("Character became unconscious: Health=%.1f, Shock=%.2f"), 
               InjuryState.OverallHealth, InjuryState.ShockLevel);
    }
    
    // Check if evacuation is required
    bool RequiresEvac = (InjuryState.OverallHealth < EVACUATION_THRESHOLD || 
                        HasCriticalInjuries() || 
                        !InjuryState.bIsConscious);
    
    if (RequiresEvac && !InjuryState.bRequiresMedicalAttention)
    {
        OnRequireEvacuation.Broadcast();
    }
}

bool UFallMechanicsSystem::CanContinueClimbing() const
{
    return InjuryState.bCanClimb && InjuryState.bIsConscious && 
           InjuryState.MovementMultiplier > 0.5f && InjuryState.GripMultiplier > 0.5f;
}

bool UFallMechanicsSystem::RequiresEvacuation() const
{
    return InjuryState.OverallHealth < EVACUATION_THRESHOLD || 
           HasCriticalInjuries() || 
           !InjuryState.bIsConscious ||
           InjuryState.ShockLevel > 0.7f;
}

// Helper functions implementation

float UFallMechanicsSystem::GetBodyPartVulnerability(EBodyPart BodyPart) const
{
    switch (BodyPart)
    {
        case EBodyPart::Head:
        case EBodyPart::Neck:
            return 2.0f; // Very vulnerable
        case EBodyPart::Chest:
        case EBodyPart::Back:
        case EBodyPart::Abdomen:
            return 1.5f; // Moderately vulnerable
        case EBodyPart::LeftArm:
        case EBodyPart::RightArm:
        case EBodyPart::LeftLeg:
        case EBodyPart::RightLeg:
            return 1.0f; // Average vulnerability
        case EBodyPart::LeftHand:
        case EBodyPart::RightHand:
        case EBodyPart::LeftFoot:
        case EBodyPart::RightFoot:
            return 0.8f; // Less vulnerable extremities
        case EBodyPart::LeftShoulder:
        case EBodyPart::RightShoulder:
        case EBodyPart::Pelvis:
            return 1.2f; // Joint areas more vulnerable
        default:
            return 1.0f;
    }
}

float UFallMechanicsSystem::GetBodyPartProtection(EBodyPart BodyPart) const
{
    // This would be modified based on protective gear worn
    // For now, assume minimal protection
    
    switch (BodyPart)
    {
        case EBodyPart::Head:
            return 0.3f; // Helmet protection
        case EBodyPart::LeftHand:
        case EBodyPart::RightHand:
            return 0.2f; // Glove protection
        default:
            return 0.0f; // No protection
    }
}

bool UFallMechanicsSystem::IsBodyPartCritical(EBodyPart BodyPart) const
{
    return (BodyPart == EBodyPart::Head || 
            BodyPart == EBodyPart::Neck || 
            BodyPart == EBodyPart::Back ||
            BodyPart == EBodyPart::Chest);
}

bool UFallMechanicsSystem::IsOwnerResting() const
{
    // Check if the climbing component indicates the character is resting
    if (UAdvancedClimbingComponent* ClimbingComp = GetOwner()->FindComponentByClass<UAdvancedClimbingComponent>())
    {
        return ClimbingComp->ClimbingState.bIsResting;
    }
    return false;
}

bool UFallMechanicsSystem::HasCriticalInjuries() const
{
    for (const FInjury& Injury : InjuryState.ActiveInjuries)
    {
        if (Injury.Severity >= EInjurySeverity::Critical && Injury.RemainingHealTime > 0.0f)
        {
            return true;
        }
    }
    return false;
}

float UFallMechanicsSystem::DetermineSurfaceHardness(AActor* Surface) const
{
    if (!Surface)
        return 1.0f; // Default hardness

    // Check surface tags for hardness
    if (Surface->ActorHasTag("Rock") || Surface->ActorHasTag("Concrete"))
        return 1.0f; // Very hard
    if (Surface->ActorHasTag("Dirt") || Surface->ActorHasTag("Grass"))
        return 0.7f; // Moderate
    if (Surface->ActorHasTag("Sand") || Surface->ActorHasTag("Snow"))
        return 0.4f; // Soft
    if (Surface->ActorHasTag("Water"))
        return 0.1f; // Very soft (but dangerous due to other factors)
    
    return 1.0f; // Default to hard surface
}

TArray<FInjury> UFallMechanicsSystem::GenerateTraumaticInjuries(float ImpactForce, const FVector& ImpactDirection) const
{
    TArray<FInjury> Injuries;
    
    // Generate additional injuries for high-energy impacts
    if (ImpactForce > 8000.0f)
    {
        // Multiple body system involvement
        TArray<EBodyPart> PossibleBodyParts = {
            EBodyPart::Chest, EBodyPart::Abdomen, EBodyPart::Back,
            EBodyPart::LeftArm, EBodyPart::RightArm, EBodyPart::LeftLeg, EBodyPart::RightLeg
        };
        
        for (EBodyPart BodyPart : PossibleBodyParts)
        {
            if (FMath::RandRange(0.0f, 1.0f) < 0.3f) // 30% chance for each body part
            {
                FInjury SecondaryInjury;
                SecondaryInjury.BodyPart = BodyPart;
                SecondaryInjury.Type = EInjuryType::Bruising;
                SecondaryInjury.Severity = EInjurySeverity::Minor;
                SecondaryInjury.Damage = FMath::RandRange(5.0f, 15.0f);
                SecondaryInjury.HealingTime = CalculateInjuryHealingTime(SecondaryInjury);
                SecondaryInjury.RemainingHealTime = SecondaryInjury.HealingTime;
                
                SetInjuryEffects(SecondaryInjury);
                Injuries.Add(SecondaryInjury);
            }
        }
    }
    
    return Injuries;
}

TArray<FInjury> UFallMechanicsSystem::GenerateAbrasionInjuries(float SlideDistance, const AActor* Surface) const
{
    TArray<FInjury> Injuries;
    
    if (SlideDistance > 2.0f) // More than 2 meters of sliding
    {
        TArray<EBodyPart> ExposedParts = {
            EBodyPart::LeftHand, EBodyPart::RightHand,
            EBodyPart::LeftArm, EBodyPart::RightArm,
            EBodyPart::LeftLeg, EBodyPart::RightLeg
        };
        
        for (EBodyPart BodyPart : ExposedParts)
        {
            FInjury Abrasion;
            Abrasion.BodyPart = BodyPart;
            Abrasion.Type = EInjuryType::Abrasion;
            Abrasion.Severity = SlideDistance > 5.0f ? EInjurySeverity::Moderate : EInjurySeverity::Minor;
            Abrasion.Damage = SlideDistance * 2.0f; // 2 damage per meter of sliding
            Abrasion.HealingTime = CalculateInjuryHealingTime(Abrasion);
            Abrasion.RemainingHealTime = Abrasion.HealingTime;
            
            SetInjuryEffects(Abrasion);
            Injuries.Add(Abrasion);
        }
    }
    
    return Injuries;
}

void UFallMechanicsSystem::SetInjuryEffects(FInjury& Injury) const
{
    // Set performance effects based on injury type and location
    
    switch (Injury.Type)
    {
        case EInjuryType::Fracture:
        case EInjuryType::Dislocation:
            Injury.bAffectsMovement = true;
            Injury.bAffectsGrip = (Injury.BodyPart == EBodyPart::LeftArm || 
                                  Injury.BodyPart == EBodyPart::RightArm ||
                                  Injury.BodyPart == EBodyPart::LeftHand ||
                                  Injury.BodyPart == EBodyPart::RightHand);
            break;
            
        case EInjuryType::Sprain:
        case EInjuryType::Strain:
            Injury.bAffectsMovement = true;
            Injury.bAffectsStamina = true;
            break;
            
        case EInjuryType::Concussion:
            Injury.bAffectsMovement = true;
            Injury.bAffectsGrip = true;
            Injury.bAffectsStamina = true;
            break;
            
        default:
            break;
    }
    
    // Set penalty amounts based on severity
    float SeverityMultiplier = 1.0f;
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
        default:
            break;
    }
    
    if (Injury.bAffectsMovement)
        Injury.MovementPenalty = 0.3f * SeverityMultiplier;
    if (Injury.bAffectsGrip)
        Injury.GripPenalty = 0.4f * SeverityMultiplier;
    if (Injury.bAffectsStamina)
        Injury.StaminaPenalty = 0.2f * SeverityMultiplier;
}

float UFallMechanicsSystem::CalculateInjuryDamage(float ImpactForce, EInjurySeverity Severity) const
{
    float BaseDamage = ImpactForce * 0.01f; // Scale down force to reasonable damage values
    
    switch (Severity)
    {
        case EInjurySeverity::Minor:
            return FMath::Clamp(BaseDamage * 0.2f, 1.0f, 10.0f);
        case EInjurySeverity::Moderate:
            return FMath::Clamp(BaseDamage * 0.5f, 5.0f, 25.0f);
        case EInjurySeverity::Severe:
            return FMath::Clamp(BaseDamage * 1.0f, 15.0f, 45.0f);
        case EInjurySeverity::Critical:
            return FMath::Clamp(BaseDamage * 1.5f, 30.0f, 70.0f);
        case EInjurySeverity::Fatal:
            return FMath::Clamp(BaseDamage * 2.0f, 60.0f, 100.0f);
        default:
            return 0.0f;
    }
}

FString UFallMechanicsSystem::GenerateInjuryDescription(const FInjury& Injury) const
{
    FString SeverityStr;
    switch (Injury.Severity)
    {
        case EInjurySeverity::Minor:
            SeverityStr = "Minor";
            break;
        case EInjurySeverity::Moderate:
            SeverityStr = "Moderate";
            break;
        case EInjurySeverity::Severe:
            SeverityStr = "Severe";
            break;
        case EInjurySeverity::Critical:
            SeverityStr = "Critical";
            break;
        case EInjurySeverity::Fatal:
            SeverityStr = "Fatal";
            break;
        default:
            SeverityStr = "Unknown";
            break;
    }
    
    return FString::Printf(TEXT("%s %s to %s"), 
                          *SeverityStr,
                          *UEnum::GetValueAsString(Injury.Type),
                          *UEnum::GetValueAsString(Injury.BodyPart));
}

void UFallMechanicsSystem::ApplyMedicalTreatment(EInjuryType TreatmentType, float Effectiveness)
{
    if (TreatmentEffectiveness.Contains(TreatmentType))
    {
        TreatmentEffectiveness[TreatmentType] = FMath::Min(1.0f, TreatmentEffectiveness[TreatmentType] + Effectiveness);
        LastTreatmentTime = GetWorld()->GetTimeSeconds();
        
        UE_LOG(LogTemp, Log, TEXT("Medical treatment applied: %s (Effectiveness: %.2f)"),
               *UEnum::GetValueAsString(TreatmentType), Effectiveness);
    }
}

void UFallMechanicsSystem::ApplyPainkillers(float Effectiveness)
{
    // Painkillers reduce shock level and injury penalties temporarily
    InjuryState.ShockLevel = FMath::Max(0.0f, InjuryState.ShockLevel - Effectiveness * 0.3f);
    
    // Temporarily improve performance multipliers
    float TempBonus = Effectiveness * 0.2f;
    InjuryState.MovementMultiplier = FMath::Min(1.0f, InjuryState.MovementMultiplier + TempBonus);
    InjuryState.GripMultiplier = FMath::Min(1.0f, InjuryState.GripMultiplier + TempBonus);
    InjuryState.StaminaMultiplier = FMath::Min(1.0f, InjuryState.StaminaMultiplier + TempBonus);
    
    UE_LOG(LogTemp, Log, TEXT("Painkillers administered (Effectiveness: %.2f)"), Effectiveness);
}

void UFallMechanicsSystem::ApplyFirstAid(EBodyPart BodyPart, float Effectiveness)
{
    // First aid helps with bleeding and reduces shock
    for (FInjury& Injury : InjuryState.ActiveInjuries)
    {
        if (Injury.BodyPart == BodyPart && 
           (Injury.Type == EInjuryType::Abrasion || Injury.Type == EInjuryType::InternalBleeding))
        {
            // Accelerate healing for treatable injuries
            Injury.RemainingHealTime *= (1.0f - Effectiveness * 0.5f);
            
            // Reduce associated shock
            InjuryState.ShockLevel = FMath::Max(0.0f, InjuryState.ShockLevel - Effectiveness * 0.1f);
        }
    }
    
    bHasReceivedFirstAid = true;
    UE_LOG(LogTemp, Log, TEXT("First aid applied to %s (Effectiveness: %.2f)"),
           *UEnum::GetValueAsString(BodyPart), Effectiveness);
}