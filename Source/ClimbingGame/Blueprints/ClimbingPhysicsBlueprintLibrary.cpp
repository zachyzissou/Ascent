#include "ClimbingPhysicsBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../Physics/ClimbingPerformanceManager.h"

// ===== ROPE PHYSICS IMPLEMENTATIONS =====

bool UClimbingPhysicsBlueprintLibrary::DeployRopeBetweenAnchors(UAdvancedRopeComponent* RopeComponent, 
                                                                AActor* StartAnchor, AActor* EndAnchor)
{
    if (!RopeComponent || !StartAnchor || !EndAnchor)
        return false;
    
    RopeComponent->DeployRope(StartAnchor, EndAnchor);
    return true;
}

float UClimbingPhysicsBlueprintLibrary::GetRopeTension(UAdvancedRopeComponent* RopeComponent)
{
    if (!RopeComponent)
        return 0.0f;
    
    return RopeComponent->CalculateCurrentTension();
}

float UClimbingPhysicsBlueprintLibrary::GetRopeElongation(UAdvancedRopeComponent* RopeComponent)
{
    if (!RopeComponent)
        return 0.0f;
    
    return RopeComponent->CalculateElongation();
}

bool UClimbingPhysicsBlueprintLibrary::WillRopeBreak(UAdvancedRopeComponent* RopeComponent, float AppliedForce)
{
    if (!RopeComponent)
        return false;
    
    return RopeComponent->WillRopeBreak(AppliedForce);
}

float UClimbingPhysicsBlueprintLibrary::CalculateFallFactor(float FallDistance, float RopeLength)
{
    if (RopeLength <= 0.0f)
        return 0.0f;
    
    return FallDistance / RopeLength;
}

TArray<FVector> UClimbingPhysicsBlueprintLibrary::GetRopeSegmentPositions(UAdvancedRopeComponent* RopeComponent)
{
    if (!RopeComponent)
        return TArray<FVector>();
    
    return RopeComponent->GetRopeSegmentPositions();
}

bool UClimbingPhysicsBlueprintLibrary::IsPointOnRope(UAdvancedRopeComponent* RopeComponent, 
                                                     const FVector& WorldLocation, float Tolerance)
{
    if (!RopeComponent)
        return false;
    
    return RopeComponent->IsPointOnRope(WorldLocation, Tolerance);
}

// ===== CLIMBING PHYSICS IMPLEMENTATIONS =====

bool UClimbingPhysicsBlueprintLibrary::StartClimbing(UAdvancedClimbingComponent* ClimbingComponent)
{
    if (!ClimbingComponent)
        return false;
    
    return ClimbingComponent->TryStartClimbing();
}

void UClimbingPhysicsBlueprintLibrary::StopClimbing(UAdvancedClimbingComponent* ClimbingComponent)
{
    if (!ClimbingComponent)
        return;
    
    ClimbingComponent->StopClimbing();
}

TArray<FGripPoint> UClimbingPhysicsBlueprintLibrary::FindNearbyGrips(UAdvancedClimbingComponent* ClimbingComponent, 
                                                                     float SearchRadius)
{
    if (!ClimbingComponent)
        return TArray<FGripPoint>();
    
    return ClimbingComponent->FindNearbyGrips(SearchRadius);
}

bool UClimbingPhysicsBlueprintLibrary::GrabHold(UAdvancedClimbingComponent* ClimbingComponent, 
                                                const FGripPoint& GripPoint, bool bIsLeftHand)
{
    if (!ClimbingComponent)
        return false;
    
    return ClimbingComponent->TryGrabHold(GripPoint, bIsLeftHand);
}

void UClimbingPhysicsBlueprintLibrary::ReleaseGrip(UAdvancedClimbingComponent* ClimbingComponent, 
                                                   bool bIsLeftHand, bool bIsHand)
{
    if (!ClimbingComponent)
        return;
    
    ClimbingComponent->ReleaseGrip(bIsLeftHand, bIsHand);
}

float UClimbingPhysicsBlueprintLibrary::GetCurrentStamina(UAdvancedClimbingComponent* ClimbingComponent)
{
    if (!ClimbingComponent)
        return 0.0f;
    
    return ClimbingComponent->ClimbingState.CurrentStamina;
}

float UClimbingPhysicsBlueprintLibrary::GetCurrentGripStrength(UAdvancedClimbingComponent* ClimbingComponent)
{
    if (!ClimbingComponent)
        return 0.0f;
    
    return ClimbingComponent->ClimbingState.CurrentGripStrength;
}

void UClimbingPhysicsBlueprintLibrary::PerformDyno(UAdvancedClimbingComponent* ClimbingComponent, 
                                                   const FVector& TargetLocation)
{
    if (!ClimbingComponent)
        return;
    
    ClimbingComponent->PerformDyno(TargetLocation);
}

bool UClimbingPhysicsBlueprintLibrary::AttachToRope(UAdvancedClimbingComponent* ClimbingComponent, 
                                                    UAdvancedRopeComponent* Rope)
{
    if (!ClimbingComponent || !Rope)
        return false;
    
    return ClimbingComponent->AttachToRope(Rope);
}

// ===== ANCHOR SYSTEM IMPLEMENTATIONS =====

bool UClimbingPhysicsBlueprintLibrary::AddAnchorPoint(UAnchorSystem* AnchorSystem, 
                                                      AClimbingToolBase* AnchorTool, bool bIsBackup)
{
    if (!AnchorSystem || !AnchorTool)
        return false;
    
    return AnchorSystem->AddAnchorPoint(AnchorTool, bIsBackup);
}

void UClimbingPhysicsBlueprintLibrary::RemoveAnchorPoint(UAnchorSystem* AnchorSystem, 
                                                         AClimbingToolBase* AnchorTool)
{
    if (!AnchorSystem || !AnchorTool)
        return;
    
    AnchorSystem->RemoveAnchorPoint(AnchorTool);
}

bool UClimbingPhysicsBlueprintLibrary::ConnectRopeToAnchors(UAnchorSystem* AnchorSystem, 
                                                           UAdvancedRopeComponent* Rope)
{
    if (!AnchorSystem || !Rope)
        return false;
    
    return AnchorSystem->ConnectRope(Rope);
}

float UClimbingPhysicsBlueprintLibrary::GetSystemSafetyFactor(UAnchorSystem* AnchorSystem)
{
    if (!AnchorSystem)
        return 0.0f;
    
    return AnchorSystem->GetSystemSafetyFactor();
}

float UClimbingPhysicsBlueprintLibrary::GetMaxSinglePointLoad(UAnchorSystem* AnchorSystem)
{
    if (!AnchorSystem)
        return 0.0f;
    
    return AnchorSystem->GetMaxSinglePointLoad();
}

bool UClimbingPhysicsBlueprintLibrary::ValidateSystemIntegrity(UAnchorSystem* AnchorSystem)
{
    if (!AnchorSystem)
        return false;
    
    return AnchorSystem->ValidateSystemIntegrity();
}

float UClimbingPhysicsBlueprintLibrary::CalculateSystemExtension(UAnchorSystem* AnchorSystem)
{
    if (!AnchorSystem)
        return 0.0f;
    
    return AnchorSystem->CalculateSystemExtension();
}

bool UClimbingPhysicsBlueprintLibrary::CanSurviveAnchorFailure(UAnchorSystem* AnchorSystem, 
                                                              AClimbingToolBase* AnchorTool)
{
    if (!AnchorSystem || !AnchorTool)
        return false;
    
    return AnchorSystem->CanSystemSurviveFailure(AnchorTool);
}

// ===== FALL MECHANICS IMPLEMENTATIONS =====

void UClimbingPhysicsBlueprintLibrary::StartFall(UFallMechanicsSystem* FallSystem, EFallType FallType)
{
    if (!FallSystem)
        return;
    
    FallSystem->StartFall(FallType);
}

void UClimbingPhysicsBlueprintLibrary::EndFall(UFallMechanicsSystem* FallSystem, 
                                               const FVector& ImpactLocation, 
                                               const FVector& ImpactVelocity, AActor* ImpactSurface)
{
    if (!FallSystem)
        return;
    
    FallSystem->EndFall(ImpactLocation, ImpactVelocity, ImpactSurface);
}

void UClimbingPhysicsBlueprintLibrary::HandleRopeFall(UFallMechanicsSystem* FallSystem, 
                                                     UAdvancedRopeComponent* Rope, float FallDistance)
{
    if (!FallSystem || !Rope)
        return;
    
    FallSystem->HandleRopeFall(Rope, FallDistance);
}

float UClimbingPhysicsBlueprintLibrary::GetHealthPercentage(UFallMechanicsSystem* FallSystem)
{
    if (!FallSystem)
        return 100.0f;
    
    return FallSystem->GetOverallHealthPercentage();
}

TArray<FInjury> UClimbingPhysicsBlueprintLibrary::GetActiveInjuries(UFallMechanicsSystem* FallSystem)
{
    if (!FallSystem)
        return TArray<FInjury>();
    
    return FallSystem->GetActiveInjuries();
}

bool UClimbingPhysicsBlueprintLibrary::CanContinueClimbing(UFallMechanicsSystem* FallSystem)
{
    if (!FallSystem)
        return true;
    
    return FallSystem->CanContinueClimbing();
}

bool UClimbingPhysicsBlueprintLibrary::RequiresEvacuation(UFallMechanicsSystem* FallSystem)
{
    if (!FallSystem)
        return false;
    
    return FallSystem->RequiresEvacuation();
}

void UClimbingPhysicsBlueprintLibrary::ApplyFirstAid(UFallMechanicsSystem* FallSystem, 
                                                     EBodyPart BodyPart, float Effectiveness)
{
    if (!FallSystem)
        return;
    
    FallSystem->ApplyFirstAid(BodyPart, Effectiveness);
}

void UClimbingPhysicsBlueprintLibrary::ApplyMedicalTreatment(UFallMechanicsSystem* FallSystem, 
                                                           EInjuryType TreatmentType, float Effectiveness)
{
    if (!FallSystem)
        return;
    
    FallSystem->ApplyMedicalTreatment(TreatmentType, Effectiveness);
}

// ===== PERFORMANCE MANAGEMENT IMPLEMENTATIONS =====

FPerformanceMetrics UClimbingPhysicsBlueprintLibrary::GetPerformanceMetrics(const UObject* WorldContext)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (!Manager)
        return FPerformanceMetrics();
    
    return Manager->GetCurrentMetrics();
}

void UClimbingPhysicsBlueprintLibrary::AdjustQualityForPerformance(const UObject* WorldContext)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (Manager)
    {
        Manager->AdjustQualityForPerformance();
    }
}

void UClimbingPhysicsBlueprintLibrary::SetGlobalLODBias(const UObject* WorldContext, float LODBias)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (Manager)
    {
        Manager->SetGlobalLODBias(LODBias);
    }
}

void UClimbingPhysicsBlueprintLibrary::RunGarbageCollection(const UObject* WorldContext)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (Manager)
    {
        Manager->RunGarbageCollection();
    }
}

EPerformanceLOD UClimbingPhysicsBlueprintLibrary::GetObjectLOD(const UObject* WorldContext, 
                                                               AActor* Actor, const FVector& ViewerLocation)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (!Manager || !Actor)
        return EPerformanceLOD::Disabled;
    
    return Manager->GetObjectLOD(Actor, ViewerLocation);
}

void UClimbingPhysicsBlueprintLibrary::SetAdaptiveQualityEnabled(const UObject* WorldContext, bool bEnabled)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (Manager)
    {
        Manager->bEnableAdaptiveQuality = bEnabled;
    }
}

// ===== UTILITY IMPLEMENTATIONS =====

FString UClimbingPhysicsBlueprintLibrary::ClimbingGradeToString(EClimbingDifficulty Grade)
{
    switch (Grade)
    {
        case EClimbingDifficulty::Grade_5_0: return TEXT("5.0");
        case EClimbingDifficulty::Grade_5_1: return TEXT("5.1");
        case EClimbingDifficulty::Grade_5_2: return TEXT("5.2");
        case EClimbingDifficulty::Grade_5_3: return TEXT("5.3");
        case EClimbingDifficulty::Grade_5_4: return TEXT("5.4");
        case EClimbingDifficulty::Grade_5_5: return TEXT("5.5");
        case EClimbingDifficulty::Grade_5_6: return TEXT("5.6");
        case EClimbingDifficulty::Grade_5_7: return TEXT("5.7");
        case EClimbingDifficulty::Grade_5_8: return TEXT("5.8");
        case EClimbingDifficulty::Grade_5_9: return TEXT("5.9");
        case EClimbingDifficulty::Grade_5_10a: return TEXT("5.10a");
        case EClimbingDifficulty::Grade_5_10b: return TEXT("5.10b");
        case EClimbingDifficulty::Grade_5_10c: return TEXT("5.10c");
        case EClimbingDifficulty::Grade_5_10d: return TEXT("5.10d");
        case EClimbingDifficulty::Grade_5_11a: return TEXT("5.11a");
        case EClimbingDifficulty::Grade_5_11b: return TEXT("5.11b");
        case EClimbingDifficulty::Grade_5_11c: return TEXT("5.11c");
        case EClimbingDifficulty::Grade_5_11d: return TEXT("5.11d");
        case EClimbingDifficulty::Grade_5_12a: return TEXT("5.12a");
        case EClimbingDifficulty::Grade_5_12b: return TEXT("5.12b");
        case EClimbingDifficulty::Grade_5_12c: return TEXT("5.12c");
        case EClimbingDifficulty::Grade_5_12d: return TEXT("5.12d");
        case EClimbingDifficulty::Grade_5_13a: return TEXT("5.13a");
        case EClimbingDifficulty::Grade_5_13b: return TEXT("5.13b");
        case EClimbingDifficulty::Grade_5_13c: return TEXT("5.13c");
        case EClimbingDifficulty::Grade_5_13d: return TEXT("5.13d");
        case EClimbingDifficulty::Grade_5_14a: return TEXT("5.14a");
        case EClimbingDifficulty::Grade_5_14b: return TEXT("5.14b");
        case EClimbingDifficulty::Grade_5_14c: return TEXT("5.14c");
        case EClimbingDifficulty::Grade_5_14d: return TEXT("5.14d");
        case EClimbingDifficulty::Grade_5_15a: return TEXT("5.15a");
        case EClimbingDifficulty::Grade_5_15b: return TEXT("5.15b");
        case EClimbingDifficulty::Grade_5_15c: return TEXT("5.15c");
        case EClimbingDifficulty::Grade_5_15d: return TEXT("5.15d");
        default: return TEXT("Unknown");
    }
}

FString UClimbingPhysicsBlueprintLibrary::RopeTypeToString(ERopeType RopeType)
{
    switch (RopeType)
    {
        case ERopeType::Dynamic: return TEXT("Dynamic Rope");
        case ERopeType::Static: return TEXT("Static Rope");
        case ERopeType::Accessory: return TEXT("Accessory Cord");
        case ERopeType::Steel: return TEXT("Steel Cable");
        default: return TEXT("Unknown");
    }
}

FLinearColor UClimbingPhysicsBlueprintLibrary::InjurySeverityToColor(EInjurySeverity Severity)
{
    switch (Severity)
    {
        case EInjurySeverity::None: return FLinearColor::Green;
        case EInjurySeverity::Minor: return FLinearColor::Yellow;
        case EInjurySeverity::Moderate: return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
        case EInjurySeverity::Severe: return FLinearColor::Red;
        case EInjurySeverity::Critical: return FLinearColor(0.5f, 0.0f, 0.0f); // Dark Red
        case EInjurySeverity::Fatal: return FLinearColor::Black;
        default: return FLinearColor::White;
    }
}

float UClimbingPhysicsBlueprintLibrary::EstimateClimbingTime(float RouteLength, EClimbingDifficulty Difficulty, 
                                                           float ClimberSkillLevel)
{
    // Base time: 1 minute per meter for grade 5.8
    float BaseTimeMinutes = RouteLength * 0.01f; // Convert cm to meters, then minutes
    
    // Difficulty modifier
    float DifficultyModifier = 1.0f;
    int32 GradeValue = static_cast<int32>(Difficulty);
    
    if (GradeValue <= 8) // Up to 5.8
    {
        DifficultyModifier = 0.5f + (GradeValue * 0.1f);
    }
    else // Above 5.8
    {
        DifficultyModifier = 1.3f + ((GradeValue - 8) * 0.3f);
    }
    
    // Skill level modifier (higher skill = faster climbing)
    float SkillModifier = FMath::Clamp(2.0f - ClimberSkillLevel, 0.5f, 3.0f);
    
    return BaseTimeMinutes * DifficultyModifier * SkillModifier;
}

float UClimbingPhysicsBlueprintLibrary::CalculateTotalGearWeight(const TArray<AClimbingToolBase*>& GearList)
{
    float TotalWeight = 0.0f;
    
    for (AClimbingToolBase* Gear : GearList)
    {
        if (IsValid(Gear))
        {
            TotalWeight += Gear->Properties.Weight;
        }
    }
    
    return TotalWeight; // kg
}

bool UClimbingPhysicsBlueprintLibrary::AreWeatherConditionsSafe(float Temperature, float WindSpeed, 
                                                               float Humidity, bool bIsRaining)
{
    // Temperature check (Celsius)
    if (Temperature < -10.0f || Temperature > 40.0f)
        return false;
    
    // Wind speed check (m/s)
    if (WindSpeed > 15.0f) // ~54 km/h
        return false;
    
    // Rain check
    if (bIsRaining)
        return false;
    
    // Humidity check (ice formation risk)
    if (Temperature < 5.0f && Humidity > 0.8f)
        return false;
    
    return true;
}

// ===== SYSTEM INTEGRATION IMPLEMENTATIONS =====

bool UClimbingPhysicsBlueprintLibrary::InitializeClimbingPhysics(ACharacter* Character)
{
    if (!Character)
        return false;
    
    bool bSuccess = true;
    
    // Add climbing component if not present
    UAdvancedClimbingComponent* ClimbingComp = Character->FindComponentByClass<UAdvancedClimbingComponent>();
    if (!ClimbingComp)
    {
        ClimbingComp = NewObject<UAdvancedClimbingComponent>(Character);
        Character->AddInstanceComponent(ClimbingComp);
    }
    
    // Add fall mechanics system
    UFallMechanicsSystem* FallSystem = Character->FindComponentByClass<UFallMechanicsSystem>();
    if (!FallSystem)
    {
        FallSystem = NewObject<UFallMechanicsSystem>(Character);
        Character->AddInstanceComponent(FallSystem);
    }
    
    // Replace default movement component if needed
    if (Character->GetCharacterMovement() != ClimbingComp)
    {
        // This would require more complex setup in a real implementation
        UE_LOG(LogTemp, Warning, TEXT("Manual replacement of movement component required"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("Initialized climbing physics for %s"), *Character->GetName());
    return bSuccess;
}

void UClimbingPhysicsBlueprintLibrary::ConnectPhysicsSystems(UAdvancedClimbingComponent* ClimbingComponent,
                                                           UAdvancedRopeComponent* RopeComponent,
                                                           UAnchorSystem* AnchorSystem,
                                                           UFallMechanicsSystem* FallSystem)
{
    if (!ClimbingComponent)
        return;
    
    // Connect rope if provided
    if (RopeComponent)
    {
        ClimbingComponent->AttachToRope(RopeComponent);
    }
    
    // Connect anchor system if provided
    if (AnchorSystem && RopeComponent)
    {
        AnchorSystem->ConnectRope(RopeComponent);
    }
    
    // Systems are automatically integrated through component references
    UE_LOG(LogTemp, Log, TEXT("Connected physics systems"));
}

void UClimbingPhysicsBlueprintLibrary::SetupPerformanceMonitoring(const UObject* WorldContext, bool bAutoOptimize)
{
    UClimbingPerformanceManager* Manager = GetPerformanceManager(WorldContext);
    if (Manager)
    {
        Manager->bEnableAdaptiveQuality = bAutoOptimize;
        Manager->bEnableLODOptimization = true;
        Manager->bEnableBatchProcessing = true;
        
        UE_LOG(LogTemp, Log, TEXT("Setup performance monitoring with auto-optimize: %s"), 
               bAutoOptimize ? TEXT("true") : TEXT("false"));
    }
}

// ===== HELPER IMPLEMENTATIONS =====

UClimbingPerformanceManager* UClimbingPhysicsBlueprintLibrary::GetPerformanceManager(const UObject* WorldContext)
{
    if (!WorldContext)
        return nullptr;
    
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
        return nullptr;
    
    return World->GetSubsystem<UClimbingPerformanceManager>();
}