#include "AdvancedToolPhysics.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Net/UnrealNetwork.h"

UAdvancedToolPhysics::UAdvancedToolPhysics()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0167f; // 60 FPS
    SetIsReplicatedByDefault(true);

    // Initialize settings
    Settings.PulleyEfficiency = 0.95f;
    Settings.BearingFriction = 0.02f;
    Settings.RopeFriction = 0.03f;
    Settings.MaxPulleyLoad = 30000.0f;
    Settings.ThrowForceMultiplier = 1.5f;
    Settings.HookPenetration = 2.0f;
    Settings.SlipFactorRock = 0.8f;
    Settings.SlipFactorIce = 0.3f;
    Settings.SlipFactorDirt = 0.5f;
    Settings.ConnectionWearRate = 0.0001f;
    Settings.MaxConnectionDistance = 5.0f;
    Settings.bEnableToolMagnetism = false;
    Settings.MagneticForceStrength = 100.0f;
    Settings.SimulationFrequency = 60.0f;
    Settings.MaxIterations = 8;
    Settings.DampingFactor = 0.98f;
    Settings.bEnableAdvancedPhysics = true;

    // Initialize system states
    PulleySystem.SystemType = EPulleySystemType::Simple;
    PulleySystem.MechanicalAdvantage = 1.0f;
    PulleySystem.SystemEfficiency = Settings.PulleyEfficiency;
    PulleySystem.InputForce = 0.0f;
    PulleySystem.OutputForce = 0.0f;
    PulleySystem.InputVelocity = 0.0f;
    PulleySystem.OutputVelocity = 0.0f;
    PulleySystem.RopeLength = 0.0f;
    PulleySystem.NumberOfPulleys = 1;
    PulleySystem.bSystemIntact = true;
    PulleySystem.FrictionLoss = 0.0f;

    GrapplingHook.State = EGrapplingHookState::Stowed;
    GrapplingHook.ThrowVelocity = FVector::ZeroVector;
    GrapplingHook.HookLocation = FVector::ZeroVector;
    GrapplingHook.HoldDirection = FVector::ZeroVector;
    GrapplingHook.HoldStrength = 0.0f;
    GrapplingHook.SlipThreshold = 0.0f;
    GrapplingHook.HookedSurface = nullptr;
    GrapplingHook.TimeHooked = 0.0f;
    GrapplingHook.bIsSecure = false;

    PhysicsUpdateInterval = 1.0f / Settings.SimulationFrequency;
}

void UAdvancedToolPhysics::BeginPlay()
{
    Super::BeginPlay();

    // Initialize cached arrays
    ForceVectorCache.Reserve(32);
    HookTrajectory.Reserve(100);
    ConstraintForces.Reserve(16);
    VelocityCorrections.Reserve(16);
}

void UAdvancedToolPhysics::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwnerRole() == ROLE_Authority && Settings.bEnableAdvancedPhysics)
    {
        LastPhysicsUpdate += DeltaTime;
        if (LastPhysicsUpdate >= PhysicsUpdateInterval)
        {
            // Update all physics systems
            UpdatePulleyPhysics(LastPhysicsUpdate);
            UpdateGrapplingHookPhysics(LastPhysicsUpdate);
            UpdateToolInteractions(LastPhysicsUpdate);
            
            if (Settings.bEnableToolMagnetism)
            {
                ApplyMagneticForces(LastPhysicsUpdate);
            }
            
            // Advanced physics calculations
            CalculateComplexLoadDistribution();
            SimulateSystemDynamics(LastPhysicsUpdate);
            
            LastPhysicsUpdate = 0.0f;
        }
    }
}

void UAdvancedToolPhysics::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAdvancedToolPhysics, PulleySystem);
    DOREPLIFETIME(UAdvancedToolPhysics, GrapplingHook);
    DOREPLIFETIME(UAdvancedToolPhysics, ActiveInteractions);
}

void UAdvancedToolPhysics::SetupPulleySystem(EPulleySystemType SystemType, const TArray<APulleyTool*>& Pulleys)
{
    PulleySystem.SystemType = SystemType;
    PulleySystem.ConnectedPulleys = Pulleys;
    PulleySystem.NumberOfPulleys = Pulleys.Num();
    
    // Calculate system properties based on type
    switch (SystemType)
    {
        case EPulleySystemType::Simple:
            PulleySystem.MechanicalAdvantage = 1.0f;
            break;
        case EPulleySystemType::Compound:
            PulleySystem.MechanicalAdvantage = CalculateCompoundPulleyAdvantage(Pulleys);
            break;
        case EPulleySystemType::Fixed:
            PulleySystem.MechanicalAdvantage = 1.0f;
            break;
        case EPulleySystemType::Movable:
            PulleySystem.MechanicalAdvantage = 2.0f;
            break;
        case EPulleySystemType::BlockAndTackle:
            PulleySystem.MechanicalAdvantage = CalculateBlockAndTackleAdvantage(Pulleys.Num());
            break;
        case EPulleySystemType::ZPulley:
            PulleySystem.MechanicalAdvantage = CalculateZPulleyAdvantage();
            break;
        case EPulleySystemType::Haul:
            PulleySystem.MechanicalAdvantage = static_cast<float>(Pulleys.Num()) * 1.5f;
            break;
    }
    
    // Apply friction losses
    float FrictionLoss = PulleySystem.NumberOfPulleys * (Settings.BearingFriction + Settings.RopeFriction);
    PulleySystem.SystemEfficiency = FMath::Max(0.1f, Settings.PulleyEfficiency - FrictionLoss);
    PulleySystem.FrictionLoss = FrictionLoss;
    
    OnPulleySystemEstablished.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Pulley system established: Type=%d, MA=%.2f, Efficiency=%.2f"), 
           static_cast<int32>(SystemType), PulleySystem.MechanicalAdvantage, PulleySystem.SystemEfficiency);
}

float UAdvancedToolPhysics::CalculatePulleyMechanicalAdvantage() const
{
    return PulleySystem.MechanicalAdvantage * PulleySystem.SystemEfficiency;
}

void UAdvancedToolPhysics::UpdatePulleyPhysics(float DeltaTime)
{
    if (PulleySystem.ConnectedPulleys.Num() == 0)
        return;

    CalculatePulleySystemForces();
    UpdatePulleyRotations(DeltaTime);
    
    // Check system integrity
    PulleySystem.bSystemIntact = true;
    for (APulleyTool* Pulley : PulleySystem.ConnectedPulleys)
    {
        if (!Pulley || Pulley->CurrentState == EToolState::Broken)
        {
            PulleySystem.bSystemIntact = false;
            break;
        }
    }
    
    // Update velocities based on mechanical advantage
    if (PulleySystem.MechanicalAdvantage > 0.0f)
    {
        PulleySystem.OutputVelocity = PulleySystem.InputVelocity / PulleySystem.MechanicalAdvantage;
    }
}

FVector UAdvancedToolPhysics::CalculatePulleyForces(const FVector& InputForce) const
{
    float InputMagnitude = InputForce.Size();
    FVector OutputForce = InputForce;
    
    if (InputMagnitude > 0.0f)
    {
        // Apply mechanical advantage
        float OutputMagnitude = InputMagnitude * PulleySystem.MechanicalAdvantage * PulleySystem.SystemEfficiency;
        OutputForce = InputForce.GetSafeNormal() * OutputMagnitude;
        
        // Check for overload
        if (OutputMagnitude > Settings.MaxPulleyLoad)
        {
            OnSystemOverloaded.Broadcast();
            UE_LOG(LogTemp, Warning, TEXT("Pulley system overloaded: %.1fN exceeds limit %.1fN"), 
                   OutputMagnitude, Settings.MaxPulleyLoad);
        }
    }
    
    return OutputForce;
}

void UAdvancedToolPhysics::OptimizePulleyConfiguration()
{
    if (PulleySystem.ConnectedPulleys.Num() < 2)
        return;

    // Find optimal pulley arrangement for maximum efficiency
    float BestEfficiency = PulleySystem.SystemEfficiency;
    EPulleySystemType BestType = PulleySystem.SystemType;
    
    TArray<EPulleySystemType> TestTypes = {
        EPulleySystemType::Simple,
        EPulleySystemType::Compound, 
        EPulleySystemType::BlockAndTackle,
        EPulleySystemType::ZPulley
    };
    
    for (EPulleySystemType TestType : TestTypes)
    {
        float TestEfficiency = CalculateSystemEfficiencyForType(TestType);
        if (TestEfficiency > BestEfficiency)
        {
            BestEfficiency = TestEfficiency;
            BestType = TestType;
        }
    }
    
    if (BestType != PulleySystem.SystemType)
    {
        SetupPulleySystem(BestType, PulleySystem.ConnectedPulleys);
        UE_LOG(LogTemp, Log, TEXT("Pulley system optimized to type %d with %.1f%% efficiency"), 
               static_cast<int32>(BestType), BestEfficiency * 100.0f);
    }
}

void UAdvancedToolPhysics::ThrowGrapplingHook(const FVector& TargetLocation, float ThrowForce)
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerThrowGrapplingHook(TargetLocation, ThrowForce);
        return;
    }

    if (GrapplingHook.State != EGrapplingHookState::Stowed)
        return;

    FVector StartLocation = GetOwner()->GetActorLocation();
    FVector ThrowDirection = (TargetLocation - StartLocation).GetSafeNormal();
    
    GrapplingHook.State = EGrapplingHookState::Thrown;
    GrapplingHook.ThrowVelocity = ThrowDirection * ThrowForce * Settings.ThrowForceMultiplier;
    GrapplingHook.HookLocation = StartLocation;
    GrapplingHook.TimeHooked = 0.0f;
    GrapplingHook.bIsSecure = false;
    
    // Initialize trajectory tracking
    HookTrajectory.Empty();
    HookTrajectory.Add(StartLocation);
    TrajectoryIndex = 0;
    
    OnGrapplingHookThrown.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Grappling hook thrown with force %.1fN toward %s"), 
           ThrowForce, *TargetLocation.ToString());
}

void UAdvancedToolPhysics::ServerThrowGrapplingHook_Implementation(const FVector& TargetLocation, float ThrowForce)
{
    ThrowGrapplingHook(TargetLocation, ThrowForce);
}

bool UAdvancedToolPhysics::ServerThrowGrapplingHook_Validate(const FVector& TargetLocation, float ThrowForce)
{
    return ThrowForce > 0.0f && ThrowForce < 5000.0f; // Reasonable force limits
}

void UAdvancedToolPhysics::UpdateGrapplingHookPhysics(float DeltaTime)
{
    switch (GrapplingHook.State)
    {
        case EGrapplingHookState::Thrown:
            UpdateHookFlightPhysics(DeltaTime);
            break;
        case EGrapplingHookState::Hooked:
        case EGrapplingHookState::Loaded:
            UpdateHookLoadPhysics(DeltaTime);
            break;
        case EGrapplingHookState::Slipping:
            // Handle gradual slip failure
            GrapplingHook.HoldStrength *= 0.95f; // 5% strength loss per update
            if (GrapplingHook.HoldStrength < 100.0f)
            {
                GrapplingHook.State = EGrapplingHookState::Failed;
                OnGrapplingHookSlipped.Broadcast();
            }
            break;
        default:
            break;
    }
    
    GrapplingHook.TimeHooked += DeltaTime;
}

bool UAdvancedToolPhysics::TestHookHold(float AppliedForce) const
{
    if (GrapplingHook.State != EGrapplingHookState::Hooked && 
        GrapplingHook.State != EGrapplingHookState::Loaded)
        return false;

    return AppliedForce <= GrapplingHook.SlipThreshold;
}

void UAdvancedToolPhysics::SimulateHookPenetration(const FHitResult& HitResult)
{
    if (!HitResult.GetActor())
        return;

    // Calculate penetration depth based on hook velocity and surface hardness
    float ImpactVelocity = GrapplingHook.ThrowVelocity.Size();
    float SurfaceHardness = DetermineSurfaceHardness(HitResult.GetActor());
    
    float PenetrationDepth = (ImpactVelocity * 0.01f) / SurfaceHardness; // Convert to cm
    PenetrationDepth = FMath::Clamp(PenetrationDepth, 0.1f, Settings.HookPenetration);
    
    // Calculate hook hold strength based on penetration and surface type
    float BaseHoldStrength = PenetrationDepth * 1000.0f; // Newtons per cm penetration
    float SurfaceMultiplier = CalculateHookSlipThreshold(HitResult.GetActor());
    
    GrapplingHook.HoldStrength = BaseHoldStrength * SurfaceMultiplier;
    GrapplingHook.SlipThreshold = GrapplingHook.HoldStrength * 0.8f; // 80% of hold strength
    GrapplingHook.HookLocation = HitResult.Location;
    GrapplingHook.HoldDirection = -HitResult.Normal;
    GrapplingHook.HookedSurface = HitResult.GetActor();
    GrapplingHook.State = EGrapplingHookState::Hooked;
    GrapplingHook.bIsSecure = (PenetrationDepth >= Settings.HookPenetration * 0.7f);
    
    // Generate contact points for multiple hook prongs
    GrapplingHook.ContactPoints.Empty();
    for (int32 i = 0; i < 3; ++i) // Assume 3-prong grappling hook
    {
        FVector ContactPoint = HitResult.Location + FMath::VRand() * 2.0f; // 2cm spread
        GrapplingHook.ContactPoints.Add(ContactPoint);
    }
    
    OnGrapplingHookEngaged.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("Grappling hook engaged: Penetration=%.1fcm, Hold=%.0fN, Secure=%s"), 
           PenetrationDepth, GrapplingHook.HoldStrength, GrapplingHook.bIsSecure ? TEXT("Yes") : TEXT("No"));
}

float UAdvancedToolPhysics::CalculateHookSlipThreshold(const AActor* Surface) const
{
    if (!Surface)
        return Settings.SlipFactorRock;

    if (Surface->ActorHasTag("Rock"))
        return Settings.SlipFactorRock;
    if (Surface->ActorHasTag("Ice"))
        return Settings.SlipFactorIce;
    if (Surface->ActorHasTag("Dirt"))
        return Settings.SlipFactorDirt;
    if (Surface->ActorHasTag("Metal"))
        return 0.9f;
    if (Surface->ActorHasTag("Wood"))
        return 0.6f;
        
    return Settings.SlipFactorRock; // Default to rock
}

bool UAdvancedToolPhysics::EstablishToolConnection(AClimbingToolBase* ToolA, AClimbingToolBase* ToolB, EToolInteractionType InteractionType)
{
    if (!ToolA || !ToolB || ToolA == ToolB)
        return false;

    // Check if connection already exists
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        if ((Interaction.ToolA == ToolA && Interaction.ToolB == ToolB) ||
            (Interaction.ToolA == ToolB && Interaction.ToolB == ToolA))
        {
            return false; // Connection already exists
        }
    }

    // Check distance constraint
    float Distance = FVector::Dist(ToolA->GetActorLocation(), ToolB->GetActorLocation());
    if (Distance > Settings.MaxConnectionDistance)
        return false;

    // Create new interaction
    FToolInteraction NewInteraction;
    NewInteraction.ToolA = ToolA;
    NewInteraction.ToolB = ToolB;
    NewInteraction.InteractionType = InteractionType;
    NewInteraction.ContactPoint = (ToolA->GetActorLocation() + ToolB->GetActorLocation()) * 0.5f;
    NewInteraction.ConnectionStrength = CalculateConnectionStrength(NewInteraction);
    NewInteraction.bConnectionActive = true;
    NewInteraction.ConnectionAge = 0.0f;

    ActiveInteractions.Add(NewInteraction);
    OnToolConnectionEstablished.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Tool connection established between %s and %s (Type: %d, Strength: %.0fN)"),
           *ToolA->GetName(), *ToolB->GetName(), static_cast<int32>(InteractionType), NewInteraction.ConnectionStrength);

    return true;
}

void UAdvancedToolPhysics::BreakToolConnection(AClimbingToolBase* ToolA, AClimbingToolBase* ToolB)
{
    for (int32 i = ActiveInteractions.Num() - 1; i >= 0; --i)
    {
        const FToolInteraction& Interaction = ActiveInteractions[i];
        if ((Interaction.ToolA == ToolA && Interaction.ToolB == ToolB) ||
            (Interaction.ToolA == ToolB && Interaction.ToolB == ToolA))
        {
            ActiveInteractions.RemoveAt(i);
            OnToolConnectionBroken.Broadcast();
            
            UE_LOG(LogTemp, Log, TEXT("Tool connection broken between %s and %s"),
                   *ToolA->GetName(), *ToolB->GetName());
            break;
        }
    }
}

void UAdvancedToolPhysics::UpdateToolInteractions(float DeltaTime)
{
    for (int32 i = ActiveInteractions.Num() - 1; i >= 0; --i)
    {
        FToolInteraction& Interaction = ActiveInteractions[i];
        
        if (!ValidateToolConnection(Interaction))
        {
            ActiveInteractions.RemoveAt(i);
            OnToolConnectionBroken.Broadcast();
            continue;
        }
        
        // Update interaction age and apply wear
        Interaction.ConnectionAge += DeltaTime;
        ApplyConnectionWear(Interaction, DeltaTime);
        
        // Update load direction and transmitted force
        if (Interaction.ToolA && Interaction.ToolB)
        {
            FVector DirectionAB = (Interaction.ToolB->GetActorLocation() - Interaction.ToolA->GetActorLocation()).GetSafeNormal();
            Interaction.LoadDirection = DirectionAB;
            
            // Calculate transmitted force based on tool loads
            float LoadA = Interaction.ToolA->GetCurrentLoad();
            float LoadB = Interaction.ToolB->GetCurrentLoad();
            Interaction.TransmittedForce = FMath::Min(LoadA, LoadB);
        }
        
        // Transmit forces through the connection
        TransmitForceThroughConnection(Interaction);
    }
}

TArray<AClimbingToolBase*> UAdvancedToolPhysics::FindNearbyTools(float SearchRadius) const
{
    TArray<AActor*> FoundActors;
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        GetOwner()->GetActorLocation(),
        SearchRadius,
        TArray<TEnumAsByte<EObjectTypeQuery>>(),
        AClimbingToolBase::StaticClass(),
        TArray<AActor*>(),
        FoundActors
    );

    TArray<AClimbingToolBase*> NearbyTools;
    for (AActor* Actor : FoundActors)
    {
        if (AClimbingToolBase* Tool = Cast<AClimbingToolBase>(Actor))
        {
            NearbyTools.Add(Tool);
        }
    }

    return NearbyTools;
}

float UAdvancedToolPhysics::CalculateConnectionStrength(const FToolInteraction& Interaction) const
{
    if (!Interaction.ToolA || !Interaction.ToolB)
        return 0.0f;

    float BaseStrength = 0.0f;
    
    switch (Interaction.InteractionType)
    {
        case EToolInteractionType::Direct:
            BaseStrength = FMath::Min(Interaction.ToolA->Properties.MajorAxisStrength, 
                                    Interaction.ToolB->Properties.MajorAxisStrength) * 1000.0f;
            break;
        case EToolInteractionType::Rope:
            BaseStrength = 15000.0f; // Typical rope strength in Newtons
            break;
        case EToolInteractionType::Carabiner:
            BaseStrength = 25000.0f; // Strong carabiner connection
            break;
        case EToolInteractionType::Sling:
            BaseStrength = 22000.0f; // Dyneema sling strength
            break;
        case EToolInteractionType::Chain:
            BaseStrength = 40000.0f; // Steel chain strength
            break;
        case EToolInteractionType::Magnetic:
            BaseStrength = Settings.MagneticForceStrength;
            break;
    }
    
    return BaseStrength;
}

void UAdvancedToolPhysics::SimulateToolChainPhysics(const TArray<AClimbingToolBase*>& ToolChain)
{
    if (ToolChain.Num() < 2)
        return;

    // Calculate forces propagating through the tool chain
    TArray<float> ChainForces;
    ChainForces.SetNum(ToolChain.Num());
    
    // Start from the end of the chain and work backwards
    ChainForces[ToolChain.Num() - 1] = 0.0f; // End of chain has no load
    
    for (int32 i = ToolChain.Num() - 2; i >= 0; --i)
    {
        float ToolLoad = ToolChain[i]->GetCurrentLoad();
        float ChainLoad = ChainForces[i + 1];
        
        // Find connection between tools i and i+1
        float ConnectionStrength = 25000.0f; // Default carabiner strength
        for (const FToolInteraction& Interaction : ActiveInteractions)
        {
            if ((Interaction.ToolA == ToolChain[i] && Interaction.ToolB == ToolChain[i + 1]) ||
                (Interaction.ToolA == ToolChain[i + 1] && Interaction.ToolB == ToolChain[i]))
            {
                ConnectionStrength = Interaction.ConnectionStrength;
                break;
            }
        }
        
        // Calculate transmitted force
        float TransmittedForce = FMath::Min(ToolLoad + ChainLoad, ConnectionStrength);
        ChainForces[i] = TransmittedForce;
        
        // Check for chain failure
        if (TransmittedForce > ConnectionStrength * 0.9f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Tool chain approaching failure at link %d: %.0fN"), 
                   i, TransmittedForce);
        }
    }
}

void UAdvancedToolPhysics::ApplyMagneticForces(float DeltaTime)
{
    if (!Settings.bEnableToolMagnetism)
        return;

    TArray<AClimbingToolBase*> NearbyTools = FindNearbyTools(Settings.MaxConnectionDistance);
    
    for (int32 i = 0; i < NearbyTools.Num(); ++i)
    {
        for (int32 j = i + 1; j < NearbyTools.Num(); ++j)
        {
            AClimbingToolBase* ToolA = NearbyTools[i];
            AClimbingToolBase* ToolB = NearbyTools[j];
            
            // Check if tools have magnetic properties (simplified)
            if (ToolA->ActorHasTag("Magnetic") && ToolB->ActorHasTag("Magnetic"))
            {
                FVector Direction = (ToolB->GetActorLocation() - ToolA->GetActorLocation());
                float Distance = Direction.Size();
                Direction.Normalize();
                
                // Inverse square law for magnetic force
                float MagneticForce = Settings.MagneticForceStrength / (Distance * Distance);
                
                // Apply attractive force
                FVector ForceVector = Direction * MagneticForce;
                
                if (ToolA->ToolMesh)
                    ToolA->ToolMesh->AddForce(ForceVector);
                if (ToolB->ToolMesh)
                    ToolB->ToolMesh->AddForce(-ForceVector);
            }
        }
    }
}

void UAdvancedToolPhysics::CalculateComplexLoadDistribution()
{
    // Advanced load distribution analysis across all connected tools
    
    if (ActiveInteractions.Num() == 0)
        return;

    // Build adjacency matrix for force distribution
    TMap<AClimbingToolBase*, int32> ToolIndices;
    TArray<AClimbingToolBase*> UniqueTool;
    
    // Collect all unique tools
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        if (Interaction.ToolA && !ToolIndices.Contains(Interaction.ToolA))
        {
            ToolIndices.Add(Interaction.ToolA, UniqueTool.Num());
            UniqueTool.Add(Interaction.ToolA);
        }
        if (Interaction.ToolB && !ToolIndices.Contains(Interaction.ToolB))
        {
            ToolIndices.Add(Interaction.ToolB, UniqueTool.Num());
            UniqueTool.Add(Interaction.ToolB);
        }
    }
    
    int32 NumTools = UniqueTool.Num();
    if (NumTools < 2)
        return;

    // Create force distribution matrix
    TArray<TArray<float>> ForceMatrix;
    ForceMatrix.SetNum(NumTools);
    for (int32 i = 0; i < NumTools; ++i)
    {
        ForceMatrix[i].SetNumZeroed(NumTools);
    }
    
    // Populate matrix with interaction forces
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        if (Interaction.ToolA && Interaction.ToolB)
        {
            int32* IndexA = ToolIndices.Find(Interaction.ToolA);
            int32* IndexB = ToolIndices.Find(Interaction.ToolB);
            
            if (IndexA && IndexB)
            {
                ForceMatrix[*IndexA][*IndexB] = Interaction.TransmittedForce;
                ForceMatrix[*IndexB][*IndexA] = Interaction.TransmittedForce;
            }
        }
    }
    
    // Solve for equilibrium forces (simplified iterative method)
    TArray<float> ToolForces;
    ToolForces.SetNumZeroed(NumTools);
    
    for (int32 Iteration = 0; Iteration < Settings.MaxIterations; ++Iteration)
    {
        TArray<float> NewForces = ToolForces;
        
        for (int32 i = 0; i < NumTools; ++i)
        {
            float TotalConnectedForce = 0.0f;
            int32 ConnectionCount = 0;
            
            for (int32 j = 0; j < NumTools; ++j)
            {
                if (i != j && ForceMatrix[i][j] > 0.0f)
                {
                    TotalConnectedForce += ForceMatrix[i][j];
                    ConnectionCount++;
                }
            }
            
            if (ConnectionCount > 0)
            {
                NewForces[i] = TotalConnectedForce / ConnectionCount;
            }
        }
        
        ToolForces = NewForces;
    }
    
    // Update tool loads based on calculated distribution
    for (int32 i = 0; i < NumTools; ++i)
    {
        if (UniqueTool[i])
        {
            // Store calculated force for analysis
            ToolLoadHistory.Add(UniqueTool[i], ToolForces[i]);
        }
    }
}

float UAdvancedToolPhysics::AnalyzeSystemEfficiency() const
{
    float TotalInputEnergy = 0.0f;
    float TotalOutputEnergy = 0.0f;
    
    // Calculate efficiency based on energy conservation
    if (PulleySystem.bSystemIntact)
    {
        TotalInputEnergy += PulleySystem.InputForce * PulleySystem.InputVelocity;
        TotalOutputEnergy += PulleySystem.OutputForce * PulleySystem.OutputVelocity;
    }
    
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        TotalInputEnergy += Interaction.TransmittedForce;
    }
    
    if (TotalInputEnergy > 0.0f)
    {
        return FMath::Clamp(TotalOutputEnergy / TotalInputEnergy, 0.0f, 1.0f);
    }
    
    return 1.0f;
}

FVector UAdvancedToolPhysics::SimulateToolFailurePropagation(AClimbingToolBase* FailedTool) const
{
    if (!FailedTool)
        return FVector::ZeroVector;

    FVector FailureImpact = FVector::ZeroVector;
    float TotalRedistributedForce = FailedTool->GetCurrentLoad();
    
    // Find all tools connected to the failed tool
    TArray<AClimbingToolBase*> ConnectedTools;
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        if (Interaction.ToolA == FailedTool && Interaction.ToolB)
        {
            ConnectedTools.Add(Interaction.ToolB);
        }
        else if (Interaction.ToolB == FailedTool && Interaction.ToolA)
        {
            ConnectedTools.Add(Interaction.ToolA);
        }
    }
    
    // Redistribute force among connected tools
    if (ConnectedTools.Num() > 0)
    {
        float ForcePerTool = TotalRedistributedForce / ConnectedTools.Num();
        
        for (AClimbingToolBase* Tool : ConnectedTools)
        {
            float NewLoad = Tool->GetCurrentLoad() + ForcePerTool;
            
            // Check if redistribution would cause cascade failure
            if (NewLoad > Tool->Properties.MajorAxisStrength * 1000.0f)
            {
                FailureImpact += Tool->GetActorLocation() * (NewLoad - Tool->Properties.MajorAxisStrength * 1000.0f);
            }
        }
    }
    
    return FailureImpact;
}

// Internal implementation functions

void UAdvancedToolPhysics::CalculatePulleySystemForces()
{
    if (PulleySystem.ConnectedPulleys.Num() == 0)
        return;

    float TotalInputForce = 0.0f;
    
    // Sum forces from all connected ropes and tools
    for (APulleyTool* Pulley : PulleySystem.ConnectedPulleys)
    {
        if (Pulley)
        {
            TotalInputForce += Pulley->GetCurrentLoad();
        }
    }
    
    PulleySystem.InputForce = TotalInputForce;
    PulleySystem.OutputForce = TotalInputForce * PulleySystem.MechanicalAdvantage * PulleySystem.SystemEfficiency;
}

void UAdvancedToolPhysics::UpdateGrapplingHookTrajectory(float DeltaTime)
{
    // Update hook trajectory with gravity and air resistance
    FVector Gravity = FVector(0.0f, 0.0f, -981.0f); // cm/s^2
    FVector AirResistance = -GrapplingHook.ThrowVelocity * GrapplingHook.ThrowVelocity.Size() * 0.01f;
    
    GrapplingHook.ThrowVelocity += (Gravity + AirResistance) * DeltaTime;
    GrapplingHook.HookLocation += GrapplingHook.ThrowVelocity * DeltaTime;
    
    // Store trajectory point
    HookTrajectory.Add(GrapplingHook.HookLocation);
    TrajectoryIndex++;
}

void UAdvancedToolPhysics::UpdateHookFlightPhysics(float DeltaTime)
{
    UpdateGrapplingHookTrajectory(DeltaTime);
    
    // Check for contact with surfaces
    if (CheckHookContact(GrapplingHook.HookLocation))
    {
        // Hook has made contact - transition to hooked state handled in CheckHookContact
        return;
    }
    
    // Check for maximum flight time or distance
    if (GrapplingHook.TimeHooked > 10.0f || // 10 second max flight time
        FVector::Dist(GetOwner()->GetActorLocation(), GrapplingHook.HookLocation) > 5000.0f) // 50m max distance
    {
        GrapplingHook.State = EGrapplingHookState::Failed;
        UE_LOG(LogTemp, Warning, TEXT("Grappling hook failed - exceeded limits"));
    }
}

bool UAdvancedToolPhysics::CheckHookContact(const FVector& CurrentLocation)
{
    FHitResult HitResult;
    FVector StartLocation = HookTrajectory.Num() > 1 ? HookTrajectory[TrajectoryIndex - 1] : CurrentLocation;
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, CurrentLocation, ECC_WorldStatic))
    {
        if (HitResult.GetActor() && HitResult.GetActor()->ActorHasTag("Hookable"))
        {
            EstablishHookConnection(HitResult);
            return true;
        }
    }
    
    return false;
}

void UAdvancedToolPhysics::EstablishHookConnection(const FHitResult& HitResult)
{
    SimulateHookPenetration(HitResult);
    // SimulateHookPenetration already sets the hook state and properties
}

void UAdvancedToolPhysics::UpdateHookLoadPhysics(float DeltaTime)
{
    if (!GrapplingHook.HookedSurface)
        return;

    // Calculate current load on the hook
    float CurrentLoad = 0.0f;
    
    // Add load from connected ropes
    // This would need to be integrated with rope system
    // For now, simulate based on distance from anchor
    FVector ToOwner = GetOwner()->GetActorLocation() - GrapplingHook.HookLocation;
    float Distance = ToOwner.Size() * 0.01f; // Convert to meters
    
    // Simulate tension based on hanging weight and angle
    float GravitationalForce = 700.0f; // 70kg climber * 9.81 m/s^2
    float AngleFactor = FMath::Abs(FVector::DotProduct(ToOwner.GetSafeNormal(), FVector::DownVector));
    CurrentLoad = GravitationalForce * AngleFactor;
    
    // Check for slip conditions
    if (CurrentLoad > GrapplingHook.SlipThreshold)
    {
        if (GrapplingHook.State != EGrapplingHookState::Slipping)
        {
            GrapplingHook.State = EGrapplingHookState::Slipping;
            UE_LOG(LogTemp, Warning, TEXT("Grappling hook starting to slip: Load=%.0fN, Threshold=%.0fN"), 
                   CurrentLoad, GrapplingHook.SlipThreshold);
        }
    }
    else if (GrapplingHook.State == EGrapplingHookState::Slipping)
    {
        GrapplingHook.State = EGrapplingHookState::Loaded;
    }
    else if (CurrentLoad > 100.0f) // 100N minimum for loaded state
    {
        GrapplingHook.State = EGrapplingHookState::Loaded;
    }
    else
    {
        GrapplingHook.State = EGrapplingHookState::Hooked;
    }
}

bool UAdvancedToolPhysics::ValidateToolConnection(const FToolInteraction& Interaction) const
{
    if (!Interaction.ToolA || !Interaction.ToolB)
        return false;

    // Check if tools still exist and are functional
    if (Interaction.ToolA->CurrentState == EToolState::Broken ||
        Interaction.ToolB->CurrentState == EToolState::Broken)
        return false;

    // Check distance constraint
    float Distance = FVector::Dist(Interaction.ToolA->GetActorLocation(), Interaction.ToolB->GetActorLocation());
    if (Distance > Settings.MaxConnectionDistance * 2.0f) // Allow some stretch
        return false;

    return true;
}

void UAdvancedToolPhysics::ApplyConnectionWear(FToolInteraction& Interaction, float DeltaTime)
{
    // Apply wear based on transmitted force and time
    float WearAmount = Interaction.TransmittedForce * Settings.ConnectionWearRate * DeltaTime;
    
    // Reduce connection strength over time
    Interaction.ConnectionStrength = FMath::Max(100.0f, Interaction.ConnectionStrength - WearAmount);
    
    // Check for connection failure
    if (Interaction.ConnectionStrength < 1000.0f) // 1kN minimum
    {
        Interaction.bConnectionActive = false;
    }
}

void UAdvancedToolPhysics::TransmitForceThroughConnection(FToolInteraction& Interaction)
{
    if (!Interaction.bConnectionActive)
        return;

    // Simple force transmission - in reality this would be more complex
    float LoadA = Interaction.ToolA ? Interaction.ToolA->GetCurrentLoad() : 0.0f;
    float LoadB = Interaction.ToolB ? Interaction.ToolB->GetCurrentLoad() : 0.0f;
    
    // Transmit the difference in loads, limited by connection strength
    float ForceDifference = FMath::Abs(LoadA - LoadB);
    Interaction.TransmittedForce = FMath::Min(ForceDifference, Interaction.ConnectionStrength);
}

void UAdvancedToolPhysics::SimulateSystemDynamics(float DeltaTime)
{
    // Advanced system dynamics simulation
    if (ActiveInteractions.Num() == 0 && !PulleySystem.bSystemIntact && GrapplingHook.State == EGrapplingHookState::Stowed)
        return;

    // Update system center of mass
    FVector SystemCenterOfMass = CalculateSystemCenterOfMass();
    
    // Calculate total system kinetic energy
    float SystemKineticEnergy = CalculateSystemKineticEnergy();
    
    // Apply damping to system oscillations
    if (SystemKineticEnergy > 0.0f)
    {
        float DampingForce = SystemKineticEnergy * (1.0f - Settings.DampingFactor);
        
        // Apply damping forces to individual tools
        for (const FToolInteraction& Interaction : ActiveInteractions)
        {
            if (Interaction.ToolA && Interaction.ToolA->ToolMesh)
            {
                FVector DampingVector = -Interaction.ToolA->ToolMesh->GetPhysicsLinearVelocity() * DampingForce * 0.01f;
                Interaction.ToolA->ToolMesh->AddForce(DampingVector);
            }
            if (Interaction.ToolB && Interaction.ToolB->ToolMesh)
            {
                FVector DampingVector = -Interaction.ToolB->ToolMesh->GetPhysicsLinearVelocity() * DampingForce * 0.01f;
                Interaction.ToolB->ToolMesh->AddForce(DampingVector);
            }
        }
    }
}

FVector UAdvancedToolPhysics::CalculateSystemCenterOfMass() const
{
    FVector CenterOfMass = FVector::ZeroVector;
    float TotalMass = 0.0f;
    
    // Include all tracked tools
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        if (Interaction.ToolA)
        {
            CenterOfMass += Interaction.ToolA->GetActorLocation() * Interaction.ToolA->Properties.Weight;
            TotalMass += Interaction.ToolA->Properties.Weight;
        }
        if (Interaction.ToolB)
        {
            CenterOfMass += Interaction.ToolB->GetActorLocation() * Interaction.ToolB->Properties.Weight;
            TotalMass += Interaction.ToolB->Properties.Weight;
        }
    }
    
    if (TotalMass > 0.0f)
    {
        return CenterOfMass / TotalMass;
    }
    
    return GetOwner()->GetActorLocation();
}

float UAdvancedToolPhysics::CalculateSystemKineticEnergy() const
{
    float TotalKineticEnergy = 0.0f;
    
    for (const FToolInteraction& Interaction : ActiveInteractions)
    {
        if (Interaction.ToolA && Interaction.ToolA->ToolMesh)
        {
            FVector Velocity = Interaction.ToolA->ToolMesh->GetPhysicsLinearVelocity() * 0.01f; // Convert to m/s
            float Mass = Interaction.ToolA->Properties.Weight;
            TotalKineticEnergy += 0.5f * Mass * Velocity.SizeSquared();
        }
        if (Interaction.ToolB && Interaction.ToolB->ToolMesh)
        {
            FVector Velocity = Interaction.ToolB->ToolMesh->GetPhysicsLinearVelocity() * 0.01f; // Convert to m/s
            float Mass = Interaction.ToolB->Properties.Weight;
            TotalKineticEnergy += 0.5f * Mass * Velocity.SizeSquared();
        }
    }
    
    return TotalKineticEnergy;
}

// Pulley system specific calculations

float UAdvancedToolPhysics::CalculateCompoundPulleyAdvantage(const TArray<APulleyTool*>& Pulleys) const
{
    if (Pulleys.Num() == 0)
        return 1.0f;

    float TotalAdvantage = 1.0f;
    
    for (const APulleyTool* Pulley : Pulleys)
    {
        if (Pulley)
        {
            float PulleyAdvantage = Pulley->GetMechanicalAdvantage();
            TotalAdvantage *= PulleyAdvantage;
        }
    }
    
    return TotalAdvantage;
}

float UAdvancedToolPhysics::CalculateBlockAndTackleAdvantage(int32 NumberOfPulleys) const
{
    // Block and tackle mechanical advantage = number of rope segments supporting the load
    // For n pulleys, advantage ≈ n (simplified)
    return static_cast<float>(NumberOfPulleys);
}

float UAdvancedToolPhysics::CalculateZPulleyAdvantage() const
{
    // Z-pulley (Z-rig) typically provides 3:1 mechanical advantage
    return 3.0f;
}

void UAdvancedToolPhysics::UpdatePulleyRotations(float DeltaTime)
{
    for (APulleyTool* Pulley : PulleySystem.ConnectedPulleys)
    {
        if (Pulley)
        {
            // Calculate rotation based on rope movement
            float RopeSpeed = PulleySystem.InputVelocity; // Simplified
            float WheelRadius = Pulley->GetEffectiveRadius();
            
            if (WheelRadius > 0.0f)
            {
                float AngularVelocity = RopeSpeed / WheelRadius;
                
                // Cache rotation for optimization
                float CurrentRotation = 0.0f;
                if (PulleyRotationCache.Contains(Pulley))
                {
                    CurrentRotation = PulleyRotationCache[Pulley];
                }
                
                CurrentRotation += AngularVelocity * DeltaTime;
                PulleyRotationCache.Add(Pulley, CurrentRotation);
            }
        }
    }
}

float UAdvancedToolPhysics::CalculateSystemEfficiencyForType(EPulleySystemType Type) const
{
    float BaseEfficiency = Settings.PulleyEfficiency;
    int32 NumPulleys = PulleySystem.ConnectedPulleys.Num();
    
    switch (Type)
    {
        case EPulleySystemType::Simple:
            return BaseEfficiency - (Settings.BearingFriction + Settings.RopeFriction);
        case EPulleySystemType::Compound:
            return FMath::Pow(BaseEfficiency, NumPulleys);
        case EPulleySystemType::BlockAndTackle:
            return BaseEfficiency - (NumPulleys * Settings.BearingFriction);
        case EPulleySystemType::ZPulley:
            return BaseEfficiency * 0.9f; // 10% efficiency loss due to complexity
        case EPulleySystemType::Haul:
            return BaseEfficiency - (NumPulleys * Settings.BearingFriction * 0.5f);
        default:
            return BaseEfficiency;
    }
}

float UAdvancedToolPhysics::DetermineSurfaceHardness(const AActor* Surface) const
{
    if (!Surface)
        return 1.0f;

    if (Surface->ActorHasTag("Rock"))
        return 1.0f;
    if (Surface->ActorHasTag("Ice"))
        return 0.7f;
    if (Surface->ActorHasTag("Dirt"))
        return 0.4f;
    if (Surface->ActorHasTag("Wood"))
        return 0.6f;
    if (Surface->ActorHasTag("Metal"))
        return 1.2f;
    
    return 1.0f; // Default to rock hardness
}

void UAdvancedToolPhysics::GeneratePhysicsReport(FString& ReportText) const
{
    ReportText = TEXT("=== Advanced Tool Physics Report ===\n\n");
    
    // Pulley system report
    if (PulleySystem.bSystemIntact)
    {
        ReportText += FString::Printf(TEXT("Pulley System:\n"));
        ReportText += FString::Printf(TEXT("  Type: %s\n"), *UEnum::GetValueAsString(PulleySystem.SystemType));
        ReportText += FString::Printf(TEXT("  Mechanical Advantage: %.2f:1\n"), PulleySystem.MechanicalAdvantage);
        ReportText += FString::Printf(TEXT("  System Efficiency: %.1f%%\n"), PulleySystem.SystemEfficiency * 100.0f);
        ReportText += FString::Printf(TEXT("  Input Force: %.0f N\n"), PulleySystem.InputForce);
        ReportText += FString::Printf(TEXT("  Output Force: %.0f N\n"), PulleySystem.OutputForce);
        ReportText += FString::Printf(TEXT("  Friction Loss: %.1f%%\n"), PulleySystem.FrictionLoss * 100.0f);
        ReportText += TEXT("\n");
    }
    
    // Grappling hook report
    if (GrapplingHook.State != EGrapplingHookState::Stowed)
    {
        ReportText += FString::Printf(TEXT("Grappling Hook:\n"));
        ReportText += FString::Printf(TEXT("  State: %s\n"), *UEnum::GetValueAsString(GrapplingHook.State));
        ReportText += FString::Printf(TEXT("  Hold Strength: %.0f N\n"), GrapplingHook.HoldStrength);
        ReportText += FString::Printf(TEXT("  Slip Threshold: %.0f N\n"), GrapplingHook.SlipThreshold);
        ReportText += FString::Printf(TEXT("  Time Hooked: %.1f s\n"), GrapplingHook.TimeHooked);
        ReportText += FString::Printf(TEXT("  Secure: %s\n"), GrapplingHook.bIsSecure ? TEXT("Yes") : TEXT("No"));
        ReportText += TEXT("\n");
    }
    
    // Tool interactions report
    ReportText += FString::Printf(TEXT("Tool Interactions: %d active\n"), ActiveInteractions.Num());
    for (int32 i = 0; i < ActiveInteractions.Num(); ++i)
    {
        const FToolInteraction& Interaction = ActiveInteractions[i];
        ReportText += FString::Printf(TEXT("  [%d] %s <-> %s\n"), i + 1,
                                    Interaction.ToolA ? *Interaction.ToolA->GetName() : TEXT("Unknown"),
                                    Interaction.ToolB ? *Interaction.ToolB->GetName() : TEXT("Unknown"));
        ReportText += FString::Printf(TEXT("      Type: %s\n"), *UEnum::GetValueAsString(Interaction.InteractionType));
        ReportText += FString::Printf(TEXT("      Strength: %.0f N\n"), Interaction.ConnectionStrength);
        ReportText += FString::Printf(TEXT("      Transmitted Force: %.0f N\n"), Interaction.TransmittedForce);
        ReportText += FString::Printf(TEXT("      Age: %.1f s\n"), Interaction.ConnectionAge);
    }
    ReportText += TEXT("\n");
    
    // System analysis
    float SystemEfficiency = AnalyzeSystemEfficiency();
    float SystemComplexity = CalculateSystemComplexity();
    
    ReportText += FString::Printf(TEXT("System Analysis:\n"));
    ReportText += FString::Printf(TEXT("  Overall Efficiency: %.1f%%\n"), SystemEfficiency * 100.0f);
    ReportText += FString::Printf(TEXT("  System Complexity: %.2f\n"), SystemComplexity);
    ReportText += FString::Printf(TEXT("  Tracked Tools: %d\n"), TrackedTools.Num());
    
    ReportText += TEXT("\n=== End Report ===");
}

float UAdvancedToolPhysics::CalculateSystemComplexity() const
{
    // Calculate system complexity based on number of interactions and types
    float Complexity = 0.0f;
    
    // Base complexity from number of interactions
    Complexity += ActiveInteractions.Num() * 0.1f;
    
    // Add complexity from pulley system
    if (PulleySystem.bSystemIntact)
    {
        Complexity += PulleySystem.NumberOfPulleys * 0.2f;
        
        // More complex pulley types add more complexity
        switch (PulleySystem.SystemType)
        {
            case EPulleySystemType::Simple:
                Complexity += 0.1f;
                break;
            case EPulleySystemType::Compound:
                Complexity += 0.3f;
                break;
            case EPulleySystemType::BlockAndTackle:
                Complexity += 0.4f;
                break;
            case EPulleySystemType::ZPulley:
                Complexity += 0.5f;
                break;
            case EPulleySystemType::Haul:
                Complexity += 0.6f;
                break;
            default:
                break;
        }
    }
    
    // Add complexity from grappling hook if active
    if (GrapplingHook.State != EGrapplingHookState::Stowed)
    {
        Complexity += 0.3f;
    }
    
    return Complexity;
}