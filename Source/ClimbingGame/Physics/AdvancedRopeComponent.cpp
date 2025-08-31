#include "AdvancedRopeComponent.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

UAdvancedRopeComponent::UAdvancedRopeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0167f; // 60 FPS
    SetIsReplicatedByDefault(true);

    // Create cable component for visual representation and physics
    CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
    if (CableComponent)
    {
        CableComponent->SetupAttachment(GetAttachmentRootComponent());
        CableComponent->bAttachStart = true;
        CableComponent->bAttachEnd = true;
        CableComponent->NumSegments = HighDetailSegments;
        CableComponent->SubstepTime = 0.0167f; // 60Hz physics substep
        CableComponent->SolverIterations = 4;
        CableComponent->EndLocation = FVector(0, 0, -300);
        CableComponent->CableLength = 300.0f;
        CableComponent->CableWidth = 2.0f;
        CableComponent->NumSides = 6;
        CableComponent->bEnableStiffness = true;
        CableComponent->bUseSubstepping = true;
        CableComponent->bEnableCollision = true;
        CableComponent->CollisionFriction = 0.2f;
        CableComponent->CableGravityScale = 1.0f;
        CableComponent->CableForce = 1000.0f;
        CableComponent->bSkipCableUpdateWhenNotVisible = true; // Performance optimization
        CableComponent->bSkipCableUpdateWhenNotOwnerRecentlyRendered = true;
    }

    // Initialize physics constraint components
    ConstraintA = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("ConstraintA"));
    ConstraintB = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("ConstraintB"));
}

void UAdvancedRopeComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize rope state
    PhysicsState.State = ERopeState::Coiled;
    PhysicsState.CurrentTension = 0.0f;
    PhysicsState.MaxTensionExperienced = 0.0f;
    PhysicsState.CurrentElongation = 0.0f;
    
    UpdateCableProperties();
}

void UAdvancedRopeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwnerRole() == ROLE_Authority)
    {
        UpdatePhysicsSimulation(DeltaTime);
        CheckBreakCondition();
        UpdateRopeState();
    }

    // Performance optimization based on distance
    if (GetWorld() && GetWorld()->GetFirstPlayerController())
    {
        APawn* Player = GetWorld()->GetFirstPlayerController()->GetPawn();
        if (Player)
        {
            float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Player->GetActorLocation());
            OptimizeForDistance(Distance);
        }
    }
}

void UAdvancedRopeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAdvancedRopeComponent, PhysicsState);
    DOREPLIFETIME(UAdvancedRopeComponent, AnchorPointA);
    DOREPLIFETIME(UAdvancedRopeComponent, AnchorPointB);
    DOREPLIFETIME(UAdvancedRopeComponent, AttachmentLocationA);
    DOREPLIFETIME(UAdvancedRopeComponent, AttachmentLocationB);
}

void UAdvancedRopeComponent::DeployRope(AActor* StartAnchor, AActor* EndAnchor)
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerDeployRope(StartAnchor, EndAnchor);
        return;
    }

    if (!StartAnchor || !EndAnchor)
    {
        UE_LOG(LogTemp, Warning, TEXT("UAdvancedRopeComponent: Cannot deploy rope with null anchors"));
        return;
    }

    AnchorPointA = StartAnchor;
    AnchorPointB = EndAnchor;
    
    AttachmentLocationA = StartAnchor->GetActorLocation();
    AttachmentLocationB = EndAnchor->GetActorLocation();

    // Calculate rope length needed
    float DistanceBetweenAnchors = FVector::Dist(AttachmentLocationA, AttachmentLocationB);
    
    if (DistanceBetweenAnchors > Properties.Length)
    {
        UE_LOG(LogTemp, Warning, TEXT("UAdvancedRopeComponent: Distance between anchors exceeds rope length"));
        return;
    }

    // Set up cable component
    if (CableComponent)
    {
        CableComponent->SetAttachEndToComponent(EndAnchor->GetRootComponent());
        CableComponent->CableLength = DistanceBetweenAnchors;
        CableComponent->EndLocation = FVector::ZeroVector;
        CableComponent->SetSimulatePhysics(true);
        
        // Configure cable for rope physics simulation
        UpdateCablePhysicsProperties();
    }

    // Create physics constraints
    CreatePhysicsConstraints();

    PhysicsState.State = ERopeState::Deployed;
    OnRopeDeployed.Broadcast();
}

void UAdvancedRopeComponent::ServerDeployRope_Implementation(AActor* StartAnchor, AActor* EndAnchor)
{
    DeployRope(StartAnchor, EndAnchor);
}

bool UAdvancedRopeComponent::ServerDeployRope_Validate(AActor* StartAnchor, AActor* EndAnchor)
{
    return StartAnchor && EndAnchor;
}

void UAdvancedRopeComponent::CoilRope()
{
    AnchorPointA = nullptr;
    AnchorPointB = nullptr;
    
    if (CableComponent)
    {
        CableComponent->SetAttachEndToComponent(nullptr);
        CableComponent->CableLength = 0.0f;
    }

    // Remove physics constraints
    if (ConstraintA)
    {
        ConstraintA->BreakConstraint();
    }
    if (ConstraintB)
    {
        ConstraintB->BreakConstraint();
    }

    PhysicsState.State = ERopeState::Coiled;
    PhysicsState.CurrentTension = 0.0f;
    PhysicsState.CurrentElongation = 0.0f;
    
    OnRopeCoiled.Broadcast();
}

bool UAdvancedRopeComponent::AttachToAnchor(AActor* Anchor, bool bIsFirstPoint)
{
    if (!Anchor)
        return false;

    if (bIsFirstPoint)
    {
        AnchorPointA = Anchor;
        AttachmentLocationA = Anchor->GetActorLocation();
    }
    else
    {
        AnchorPointB = Anchor;
        AttachmentLocationB = Anchor->GetActorLocation();
    }

    return true;
}

void UAdvancedRopeComponent::DetachFromAnchor(bool bIsFirstPoint)
{
    if (bIsFirstPoint)
    {
        AnchorPointA = nullptr;
        if (ConstraintA)
        {
            ConstraintA->BreakConstraint();
        }
    }
    else
    {
        AnchorPointB = nullptr;
        if (ConstraintB)
        {
            ConstraintB->BreakConstraint();
        }
    }
}

float UAdvancedRopeComponent::CalculateCurrentTension() const
{
    if (!CableComponent || PhysicsState.State != ERopeState::Deployed)
        return 0.0f;

    // Get cable physics data
    const TArray<FVector>& CablePoints = CableComponent->GetCableParticleLocations();
    if (CablePoints.Num() < 2)
        return 0.0f;

    // Calculate tension based on cable deformation
    float CurrentLength = 0.0f;
    for (int32 i = 1; i < CablePoints.Num(); ++i)
    {
        CurrentLength += FVector::Dist(CablePoints[i-1], CablePoints[i]);
    }

    float RestLength = Properties.Length;
    float Elongation = (CurrentLength - RestLength) / RestLength;

    // Apply rope type specific elasticity
    float Tension = 0.0f;
    
    switch (RopeType)
    {
        case ERopeType::Dynamic:
            // Dynamic rope has high elasticity
            Tension = CalculateDynamicRopeTension(Elongation);
            break;
        case ERopeType::Static:
            // Static rope has low elasticity
            Tension = CalculateStaticRopeTension(Elongation);
            break;
        case ERopeType::Accessory:
            // Accessory cord has minimal stretch
            Tension = CalculateAccessoryRopeTension(Elongation);
            break;
        case ERopeType::Steel:
            // Steel cable has virtually no stretch
            Tension = CalculateSteelCableTension(Elongation);
            break;
    }

    return FMath::Max(0.0f, Tension);
}

float UAdvancedRopeComponent::CalculateElongation() const
{
    if (!CableComponent || PhysicsState.State != ERopeState::Deployed)
        return 0.0f;

    float CurrentLength = CableComponent->CableLength;
    float RestLength = Properties.Length;
    
    return ((CurrentLength - RestLength) / RestLength) * 100.0f; // Return as percentage
}

float UAdvancedRopeComponent::CalculateFallFactor(float FallDistance) const
{
    if (Properties.Length <= 0.0f)
        return 0.0f;
    
    return FallDistance / Properties.Length;
}

float UAdvancedRopeComponent::CalculateImpactForce(float FallDistance, float ClimberMass) const
{
    if (FallDistance <= 0.0f || Properties.Length <= 0.0f)
        return 0.0f;

    float FallFactor = CalculateFallFactor(FallDistance);
    
    // Use dynamic rope formula: F = sqrt(2 * m * g * h * k)
    // where k is rope stiffness derived from elongation properties
    float Gravity = 9.81f; // m/s^2
    float RopeStiffness = CalculateRopeStiffness();
    
    float ImpactForce = FMath::Sqrt(2.0f * ClimberMass * Gravity * FallDistance * RopeStiffness);
    
    // Factor 2 falls are the worst case
    if (FallFactor >= 1.8f)
    {
        ImpactForce *= 1.2f; // 20% increase for severe falls
    }
    
    return ImpactForce;
}

bool UAdvancedRopeComponent::WillRopeBreak(float AppliedForce) const
{
    float EffectiveStrength = Properties.BreakingStrength * 1000.0f; // Convert kN to N
    
    // Account for degradation
    EffectiveStrength *= GetRemainingStrength();
    
    // Safety factor for dynamic loading
    EffectiveStrength *= 0.8f;
    
    return AppliedForce > EffectiveStrength;
}

void UAdvancedRopeComponent::ApplyWeatherEffects(float Temperature, float Humidity, bool bIsWet)
{
    float PerformanceModifier = 1.0f;
    
    // Cold weather makes rope stiffer
    if (Temperature < 0.0f)
    {
        PerformanceModifier *= (1.0f + FMath::Abs(Temperature) * 0.002f);
    }
    
    // Wet conditions reduce strength
    if (bIsWet)
    {
        PerformanceModifier *= 0.95f;
        
        // Ice formation
        if (Temperature < 0.0f)
        {
            PerformanceModifier *= 0.9f;
        }
    }
    
    // High humidity accelerates degradation
    float DegradationRate = Humidity * 0.0001f; // Very slow degradation
    ProcessEnvironmentalDegradation(DegradationRate);
}

void UAdvancedRopeComponent::ProcessAbrasion(const FVector& ContactPoint, float AbrasionAmount)
{
    AbrasionWear += AbrasionAmount * (1.0f - Properties.Abrasion_Resistance);
    
    // Localized wear affects overall rope strength
    TotalWear += AbrasionAmount * 0.1f;
    
    if (AbrasionWear > 0.3f) // 30% abrasion wear
    {
        UE_LOG(LogTemp, Warning, TEXT("UAdvancedRopeComponent: Rope showing significant abrasion wear"));
    }
}

void UAdvancedRopeComponent::ProcessUVDegradation(float UVIntensity, float ExposureTime)
{
    float DegradationAmount = UVIntensity * ExposureTime * (1.0f - Properties.UV_Resistance) * 0.0001f;
    UVDegradation += DegradationAmount;
    TotalWear += DegradationAmount;
}

void UAdvancedRopeComponent::RecordFall(float FallDistance, float ImpactForce)
{
    float FallFactor = CalculateFallFactor(FallDistance);
    
    // Factor 1+ falls cause significant wear
    if (FallFactor >= 1.0f)
    {
        PhysicsState.FallsCount++;
        
        // Calculate fatigue damage
        float FatigueDamageAmount = FallFactor * ImpactForce * 0.0001f;
        FatigueDamage += FatigueDamageAmount;
        TotalWear += FatigueDamageAmount;
        
        PhysicsState.TotalEnergyAbsorbed += (0.5f * ImpactForce * ImpactForce) / CalculateRopeStiffness();
    }
    
    // Update maximum tension experienced
    if (ImpactForce > PhysicsState.MaxTensionExperienced)
    {
        PhysicsState.MaxTensionExperienced = ImpactForce;
    }
}

float UAdvancedRopeComponent::GetRemainingStrength() const
{
    // Strength reduction based on various wear factors
    float StrengthReduction = 0.0f;
    
    // UV degradation reduces strength significantly
    StrengthReduction += UVDegradation * 2.0f;
    
    // Abrasion wear
    StrengthReduction += AbrasionWear * 1.5f;
    
    // Fatigue from falls
    StrengthReduction += FatigueDamage * 3.0f;
    
    // General wear
    StrengthReduction += TotalWear;
    
    return FMath::Max(0.1f, 1.0f - StrengthReduction); // Minimum 10% strength retained
}

bool UAdvancedRopeComponent::ShouldRetireRope() const
{
    // UIAA/CE standards for rope retirement
    
    // More than 5 factor 2 falls (for dynamic ropes)
    if (RopeType == ERopeType::Dynamic && PhysicsState.FallsCount > 5)
        return true;
    
    // Strength below 70% of original
    if (GetRemainingStrength() < 0.7f)
        return true;
    
    // Excessive abrasion
    if (AbrasionWear > 0.25f)
        return true;
    
    // High UV degradation
    if (UVDegradation > 0.15f)
        return true;
    
    return false;
}

void UAdvancedRopeComponent::MulticastOnRopeBreak_Implementation(const FVector& BreakLocation)
{
    OnRopeBroken.Broadcast();
    
    // Visual/audio feedback for rope break
    UE_LOG(LogTemp, Error, TEXT("UAdvancedRopeComponent: ROPE BROKEN at location %s"), *BreakLocation.ToString());
    
    // Disable rope physics
    if (CableComponent)
    {
        CableComponent->SetSimulatePhysics(false);
    }
    
    PhysicsState.State = ERopeState::Broken;
}

void UAdvancedRopeComponent::MulticastOnHighTension_Implementation(float TensionValue)
{
    if (TensionValue > Properties.BreakingStrength * 800.0f) // 80% of breaking strength
    {
        OnOverloaded.Broadcast();
        UE_LOG(LogTemp, Warning, TEXT("UAdvancedRopeComponent: High tension warning: %f N"), TensionValue);
    }
}

void UAdvancedRopeComponent::UpdatePhysicsSimulation(float DeltaTime)
{
    if (!ShouldSimulatePhysics())
        return;

    // Update tension calculation (throttled for performance)
    LastTensionUpdateTime += DeltaTime;
    if (LastTensionUpdateTime >= TensionUpdateInterval)
    {
        CachedTension = CalculateCurrentTension();
        PhysicsState.CurrentTension = CachedTension;
        PhysicsState.CurrentElongation = CalculateElongation();
        LastTensionUpdateTime = 0.0f;
        
        // Broadcast high tension warning
        if (CachedTension > Properties.BreakingStrength * 800.0f)
        {
            MulticastOnHighTension(CachedTension);
        }
    }
    
    // Apply wear from current tension
    if (PhysicsState.CurrentTension > 0.0f)
    {
        float WearAmount = CalculateWearFromTension(PhysicsState.CurrentTension, DeltaTime);
        TotalWear += WearAmount;
    }
}

void UAdvancedRopeComponent::UpdateCableProperties()
{
    if (!CableComponent)
        return;
    
    // Set cable properties based on rope type
    switch (RopeType)
    {
        case ERopeType::Dynamic:
            CableComponent->CableForce = 1000.0f;
            CableComponent->CableGravityScale = 1.0f;
            CableComponent->bEnableStiffness = true;
            break;
        case ERopeType::Static:
            CableComponent->CableForce = 2000.0f;
            CableComponent->CableGravityScale = 1.2f;
            CableComponent->bEnableStiffness = true;
            break;
        case ERopeType::Accessory:
            CableComponent->CableForce = 1500.0f;
            CableComponent->CableGravityScale = 0.8f;
            break;
        case ERopeType::Steel:
            CableComponent->CableForce = 5000.0f;
            CableComponent->CableGravityScale = 2.0f;
            CableComponent->bEnableStiffness = false; // Steel doesn't stretch
            break;
    }
    
    // Update visual properties
    CableComponent->CableWidth = Properties.Diameter * 0.1f; // Convert mm to cm for display
}

void UAdvancedRopeComponent::CheckBreakCondition()
{
    if (PhysicsState.State == ERopeState::Broken)
        return;
    
    if (WillRopeBreak(PhysicsState.CurrentTension))
    {
        FVector BreakLocation = GetOwner()->GetActorLocation();
        if (CableComponent)
        {
            const TArray<FVector>& Points = CableComponent->GetCableParticleLocations();
            if (Points.Num() > 0)
            {
                // Find the point of highest stress (usually near middle or anchor points)
                BreakLocation = Points[Points.Num() / 2];
            }
        }
        
        MulticastOnRopeBreak(BreakLocation);
    }
}

void UAdvancedRopeComponent::UpdateRopeState()
{
    if (PhysicsState.State == ERopeState::Broken)
        return;
    
    if (PhysicsState.CurrentTension > Properties.BreakingStrength * 700.0f) // 70% of breaking strength
    {
        PhysicsState.State = ERopeState::Overloaded;
    }
    else if (PhysicsState.CurrentTension > 100.0f) // More than 100N tension
    {
        PhysicsState.State = ERopeState::Tensioned;
    }
    else if (AnchorPointA && AnchorPointB)
    {
        PhysicsState.State = ERopeState::Deployed;
    }
    else
    {
        PhysicsState.State = ERopeState::Coiled;
    }
}

void UAdvancedRopeComponent::OptimizeForDistance(float ViewerDistance)
{
    if (!CableComponent)
        return;
    
    if (ViewerDistance > MaxSimulationDistance)
    {
        // Disable physics simulation for distant ropes
        CableComponent->SetSimulatePhysics(false);
        CableComponent->NumSegments = 2; // Minimum segments for line rendering
    }
    else if (ViewerDistance > MaxSimulationDistance * 0.5f)
    {
        // Low detail for medium distance
        CableComponent->SetSimulatePhysics(true);
        CableComponent->NumSegments = LowDetailSegments;
    }
    else
    {
        // High detail for close distance
        CableComponent->SetSimulatePhysics(true);
        CableComponent->NumSegments = HighDetailSegments;
    }
}

bool UAdvancedRopeComponent::ShouldSimulatePhysics() const
{
    return PhysicsState.State != ERopeState::Coiled && PhysicsState.State != ERopeState::Broken;
}

// Private calculation methods
float UAdvancedRopeComponent::CalculateDynamicRopeTension(float Elongation) const
{
    // Dynamic rope formula based on elongation percentage
    float MaxElongation = Properties.DynamicElongation / 100.0f;
    float ElongationRatio = FMath::Clamp(Elongation / MaxElongation, 0.0f, 1.0f);
    
    // Non-linear response curve for dynamic rope
    float TensionRatio = FMath::Pow(ElongationRatio, 1.8f);
    return TensionRatio * Properties.BreakingStrength * 1000.0f; // Convert kN to N
}

float UAdvancedRopeComponent::CalculateStaticRopeTension(float Elongation) const
{
    float MaxElongation = Properties.StaticElongation / 100.0f;
    float ElongationRatio = FMath::Clamp(Elongation / MaxElongation, 0.0f, 1.0f);
    
    // More linear response for static rope
    return ElongationRatio * Properties.BreakingStrength * 1000.0f;
}

float UAdvancedRopeComponent::CalculateAccessoryRopeTension(float Elongation) const
{
    // Accessory cord has minimal stretch - mostly linear
    float MaxElongation = 0.02f; // 2% maximum elongation
    float ElongationRatio = FMath::Clamp(Elongation / MaxElongation, 0.0f, 1.0f);
    
    return ElongationRatio * Properties.BreakingStrength * 1000.0f;
}

float UAdvancedRopeComponent::CalculateSteelCableTension(float Elongation) const
{
    // Steel cable - virtually no stretch, very high modulus
    float MaxElongation = 0.005f; // 0.5% maximum elongation
    float ElongationRatio = FMath::Clamp(Elongation / MaxElongation, 0.0f, 1.0f);
    
    return ElongationRatio * Properties.BreakingStrength * 1000.0f;
}

float UAdvancedRopeComponent::CalculateRopeStiffness() const
{
    // Stiffness varies by rope type
    switch (RopeType)
    {
        case ERopeType::Dynamic:
            return 5000.0f; // N/m
        case ERopeType::Static:
            return 15000.0f;
        case ERopeType::Accessory:
            return 25000.0f;
        case ERopeType::Steel:
            return 200000.0f;
        default:
            return 10000.0f;
    }
}

void UAdvancedRopeComponent::CreatePhysicsConstraints()
{
    if (!ConstraintA || !ConstraintB)
        return;

    // Set up constraint A
    if (AnchorPointA && AnchorPointA->GetRootComponent())
    {
        ConstraintA->SetConstrainedComponents(
            GetOwner()->GetRootComponent(), NAME_None,
            AnchorPointA->GetRootComponent(), NAME_None
        );
        ConstraintA->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
        ConstraintA->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
        ConstraintA->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    }

    // Set up constraint B  
    if (AnchorPointB && AnchorPointB->GetRootComponent())
    {
        ConstraintB->SetConstrainedComponents(
            GetOwner()->GetRootComponent(), NAME_None,
            AnchorPointB->GetRootComponent(), NAME_None
        );
        ConstraintB->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
        ConstraintB->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
        ConstraintB->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    }
}

float UAdvancedRopeComponent::CalculateWearFromTension(float Tension, float DeltaTime) const
{
    // Higher tension causes more wear
    float NormalizedTension = Tension / (Properties.BreakingStrength * 1000.0f);
    return NormalizedTension * NormalizedTension * DeltaTime * 0.0001f;
}

float UAdvancedRopeComponent::CalculateWearFromAbrasion(float AbrasionForce) const
{
    return AbrasionForce * (1.0f - Properties.Abrasion_Resistance) * 0.001f;
}

float UAdvancedRopeComponent::CalculateWearFromEnvironment(float Temperature, float UV, bool bWet) const
{
    float EnvironmentalWear = 0.0f;
    
    // UV damage
    EnvironmentalWear += UV * (1.0f - Properties.UV_Resistance) * 0.0001f;
    
    // Temperature extremes
    if (Temperature < -10.0f || Temperature > 40.0f)
    {
        EnvironmentalWear += FMath::Abs(Temperature - 20.0f) * 0.00001f;
    }
    
    // Wet conditions
    if (bWet)
    {
        EnvironmentalWear += 0.00001f;
    }
    
    return EnvironmentalWear;
}

void UAdvancedRopeComponent::ProcessEnvironmentalDegradation(float DegradationRate)
{
    TotalWear += DegradationRate;
}

void UAdvancedRopeComponent::UpdateCablePhysicsProperties()
{
    if (!CableComponent)
        return;
    
    // Update physics properties based on rope type and current state
    float MassDensity = Properties.Weight / 100.0f; // kg per meter
    CableComponent->CableMass = MassDensity * Properties.Length;
    
    // Update cable stiffness based on rope properties
    float StiffnessValue = CalculateRopeStiffness() / 10000.0f; // Normalize for UE5
    CableComponent->bEnableStiffness = (RopeType != ERopeType::Dynamic);
    
    // Dynamic simulation parameters
    CableComponent->SolverIterations = FMath::Clamp(PhysicsSettings.HighSolverIterations, 1, 16);
    
    // Damping for realistic rope behavior
    float DampingFactor = 0.1f;
    switch (RopeType)
    {
        case ERopeType::Dynamic:
            DampingFactor = 0.05f; // Less damping for dynamic movement
            break;
        case ERopeType::Static:
            DampingFactor = 0.1f;
            break;
        case ERopeType::Accessory:
            DampingFactor = 0.15f;
            break;
        case ERopeType::Steel:
            DampingFactor = 0.2f; // High damping for steel cables
            break;
    }
    
    // Apply environmental effects to physics
    ApplyEnvironmentalPhysicsModifiers();
}

void UAdvancedRopeComponent::ApplyEnvironmentalPhysicsModifiers()
{
    if (!CableComponent)
        return;
    
    // Temperature effects on rope stiffness
    float TemperatureModifier = 1.0f;
    // Cold makes ropes stiffer, heat makes them more flexible
    // This would be set by environmental system
    
    CableComponent->CableForce *= TemperatureModifier;
}

void UAdvancedRopeComponent::EnableDynamicPhysics(bool bEnable)
{
    if (!CableComponent)
        return;
    
    CableComponent->SetSimulatePhysics(bEnable);
    
    if (bEnable)
    {
        // Ensure proper physics setup
        UpdateCablePhysicsProperties();
        
        // Enable continuous collision detection for fast-moving rope
        CableComponent->bEnableCollision = true;
    }
    else
    {
        // Disable physics for performance
        CableComponent->bEnableCollision = false;
    }
}

TArray<FVector> UAdvancedRopeComponent::GetRopeSegmentPositions() const
{
    TArray<FVector> Positions;
    
    if (CableComponent)
    {
        Positions = CableComponent->GetCableParticleLocations();
    }
    
    return Positions;
}

float UAdvancedRopeComponent::GetDistanceAlongRope(const FVector& WorldLocation) const
{
    if (!CableComponent)
        return 0.0f;
    
    TArray<FVector> RopePoints = CableComponent->GetCableParticleLocations();
    if (RopePoints.Num() < 2)
        return 0.0f;
    
    float MinDistance = FLT_MAX;
    float DistanceAlongRope = 0.0f;
    float BestDistanceAlongRope = 0.0f;
    
    for (int32 i = 0; i < RopePoints.Num() - 1; ++i)
    {
        // Find closest point on this rope segment
        FVector ClosestPoint = FMath::ClosestPointOnSegment(WorldLocation, RopePoints[i], RopePoints[i + 1]);
        float Distance = FVector::Dist(WorldLocation, ClosestPoint);
        
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            
            // Calculate distance along rope to this point
            BestDistanceAlongRope = DistanceAlongRope + FVector::Dist(RopePoints[i], ClosestPoint);
        }
        
        DistanceAlongRope += FVector::Dist(RopePoints[i], RopePoints[i + 1]);
    }
    
    return BestDistanceAlongRope;
}

bool UAdvancedRopeComponent::IsPointOnRope(const FVector& WorldLocation, float Tolerance) const
{
    if (!CableComponent)
        return false;
    
    TArray<FVector> RopePoints = CableComponent->GetCableParticleLocations();
    
    for (int32 i = 0; i < RopePoints.Num() - 1; ++i)
    {
        FVector ClosestPoint = FMath::ClosestPointOnSegment(WorldLocation, RopePoints[i], RopePoints[i + 1]);
        if (FVector::Dist(WorldLocation, ClosestPoint) <= Tolerance)
        {
            return true;
        }
    }
    
    return false;
}

FVector UAdvancedRopeComponent::GetRopeDirectionAtPoint(const FVector& WorldLocation) const
{
    if (!CableComponent)
        return FVector::ZeroVector;
    
    TArray<FVector> RopePoints = CableComponent->GetCableParticleLocations();
    if (RopePoints.Num() < 2)
        return FVector::ZeroVector;
    
    float MinDistance = FLT_MAX;
    FVector BestDirection = FVector::ZeroVector;
    
    for (int32 i = 0; i < RopePoints.Num() - 1; ++i)
    {
        FVector ClosestPoint = FMath::ClosestPointOnSegment(WorldLocation, RopePoints[i], RopePoints[i + 1]);
        float Distance = FVector::Dist(WorldLocation, ClosestPoint);
        
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            BestDirection = (RopePoints[i + 1] - RopePoints[i]).GetSafeNormal();
        }
    }
    
    return BestDirection;
}