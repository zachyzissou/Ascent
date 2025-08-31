#include "GeologicalHazardSystem.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodyInstance.h"

// Rockfall Hazard Implementation

ARockfallHazard::ARockfallHazard()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    // Create root component
    HazardZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardZoneMesh"));
    RootComponent = HazardZoneMesh;
    HazardZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HazardZoneMesh->SetVisibility(false); // Only visible in editor

    // Create proximity trigger
    ProximityTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ProximityTrigger"));
    ProximityTrigger->SetupAttachment(RootComponent);
    ProximityTrigger->SetSphereRadius(1000.0f);
    ProximityTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProximityTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProximityTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Create particle effect
    DustEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("DustEffect"));
    DustEffect->SetupAttachment(RootComponent);
    DustEffect->SetAutoActivate(false);

    // Create audio component
    RockfallAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("RockfallAudioComponent"));
    RockfallAudioComponent->SetupAttachment(RootComponent);
    RockfallAudioComponent->bAutoActivate = false;

    // Initialize rock size masses (realistic values in kg)
    RockSizeMasses.Add(ERockSize::Pebble, 0.1f);
    RockSizeMasses.Add(ERockSize::Cobble, 5.0f);
    RockSizeMasses.Add(ERockSize::Boulder, 100.0f);
    RockSizeMasses.Add(ERockSize::LargeBoulder, 1000.0f);
    RockSizeMasses.Add(ERockSize::Slab, 500.0f);

    // Default safety zones
    SafeZoneLocations.Add(FVector(0, 0, 0)); // Will be set relative to actor location
}

void ARockfallHazard::BeginPlay()
{
    Super::BeginPlay();

    // Bind proximity trigger
    if (ProximityTrigger)
    {
        ProximityTrigger->OnComponentBeginOverlap.AddDynamic(this, &ARockfallHazard::OnProximityTriggerBeginOverlap);
    }

    // Convert relative safe zone locations to world coordinates
    for (int32 i = 0; i < SafeZoneLocations.Num(); i++)
    {
        SafeZoneLocations[i] = GetActorLocation() + SafeZoneLocations[i];
    }

    // Start automatic rockfall timer if time-based
    if (TriggerType == ERockfallTrigger::TimeBased && HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_RockfallCheck,
            [this]() { UpdateRockfallProbability(1.0f); },
            1.0f, // Check every second
            true
        );
    }
}

void ARockfallHazard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    
    // Clean up any active rocks
    for (AActor* Rock : ActiveRocks)
    {
        if (IsValid(Rock))
        {
            Rock->Destroy();
        }
    }
    ActiveRocks.Empty();

    Super::EndPlay(EndPlayReason);
}

void ARockfallHazard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Add replication for any networked properties if needed
}

void ARockfallHazard::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Clean up rocks that have stopped moving or left the area
    for (int32 i = ActiveRocks.Num() - 1; i >= 0; i--)
    {
        AActor* Rock = ActiveRocks[i];
        if (!IsValid(Rock))
        {
            ActiveRocks.RemoveAt(i);
            continue;
        }

        // Remove rocks that are too far away or stationary for too long
        float DistanceFromOrigin = FVector::Dist(Rock->GetActorLocation(), GetActorLocation());
        if (DistanceFromOrigin > 10000.0f) // 100 meters
        {
            Rock->Destroy();
            ActiveRocks.RemoveAt(i);
        }
    }
}

void ARockfallHazard::TriggerRockfall(const FRockfallEvent& RockfallEvent)
{
    if (!HasAuthority()) return;

    // Broadcast warning
    OnRockfallWarning.Broadcast(RockfallEvent);

    // Play warning sound if enabled
    if (RockfallEvent.bPlayWarningSound && RockfallWarningSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), RockfallWarningSound, RockfallEvent.SpawnLocation);
    }

    // Start dust effect
    if (DustEffect && !DustEffect->IsActive())
    {
        DustEffect->SetWorldLocation(RockfallEvent.SpawnLocation);
        DustEffect->Activate();
    }

    // Delayed rock spawn after warning time
    FTimerHandle TimerHandle_DelayedSpawn;
    GetWorldTimerManager().SetTimer(
        TimerHandle_DelayedSpawn,
        [this, RockfallEvent]()
        {
            for (int32 i = 0; i < RockfallEvent.RockCount; i++)
            {
                FVector SpawnOffset = FVector(
                    FMath::RandRange(-200.0f, 200.0f),
                    FMath::RandRange(-200.0f, 200.0f),
                    FMath::RandRange(0.0f, 100.0f)
                );
                
                FVector ActualSpawnLocation = RockfallEvent.SpawnLocation + SpawnOffset;
                FVector RandomizedVelocity = RockfallEvent.InitialVelocity + FVector(
                    FMath::RandRange(-100.0f, 100.0f),
                    FMath::RandRange(-100.0f, 100.0f),
                    FMath::RandRange(0.0f, 200.0f)
                );

                SpawnRock(RockfallEvent.RockSize, ActualSpawnLocation, RandomizedVelocity);
            }
        },
        RockfallEvent.WarningTime,
        false
    );
}

void ARockfallHazard::SetRockfallProbability(float NewProbability)
{
    BaseProbability = FMath::Clamp(NewProbability, 0.0f, 1.0f);
}

void ARockfallHazard::EnableProximityTrigger(bool bEnable, float TriggerRadius)
{
    if (ProximityTrigger)
    {
        ProximityTrigger->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        ProximityTrigger->SetSphereRadius(TriggerRadius);
    }
}

void ARockfallHazard::SetWeatherInfluence(float TemperatureThreshold, float PrecipitationMultiplier)
{
    FreezeTh‍awTemperatureThreshold = TemperatureThreshold;
    RainRockfallMultiplier = PrecipitationMultiplier;
}

float ARockfallHazard::GetRockQualityAtLocation(const FVector& Location) const
{
    // Simplified rock quality calculation based on distance from hazard center
    float DistanceFromHazard = FVector::Dist(Location, GetActorLocation());
    float MaxDistance = 2000.0f; // 20 meters
    
    // Closer to hazard center = lower quality
    float BaseQuality = FMath::Clamp(DistanceFromHazard / MaxDistance, 0.1f, 1.0f);
    
    // Add environmental factors
    float EnvironmentalFactor = 1.0f - (CalculateEnvironmentalModifier() * 0.3f);
    
    return BaseQuality * EnvironmentalFactor;
}

bool ARockfallHazard::IsLocationInRockfallZone(const FVector& Location) const
{
    if (!HazardZoneMesh) return false;
    
    FBoxSphereBounds Bounds = HazardZoneMesh->GetLocalBounds();
    FVector LocalLocation = GetActorTransform().InverseTransformPosition(Location);
    
    return Bounds.BoxExtent.ContainsPoint(LocalLocation);
}

TArray<FVector> ARockfallHazard::GetSafeZones() const
{
    return SafeZoneLocations;
}

FString ARockfallHazard::GetRockfallRiskAssessment() const
{
    float CurrentRisk = BaseProbability * CalculateEnvironmentalModifier();
    
    FString RiskLevel;
    if (CurrentRisk < 0.2f) RiskLevel = TEXT("Low");
    else if (CurrentRisk < 0.5f) RiskLevel = TEXT("Moderate");
    else if (CurrentRisk < 0.8f) RiskLevel = TEXT("High");
    else RiskLevel = TEXT("Extreme");
    
    return FString::Printf(TEXT("Rockfall Risk: %s (%.1f%% probability)"), *RiskLevel, CurrentRisk * 100.0f);
}

TArray<FString> ARockfallHazard::GetSafetyRecommendations() const
{
    TArray<FString> Recommendations;
    
    float CurrentRisk = BaseProbability * CalculateEnvironmentalModifier();
    
    Recommendations.Add(TEXT("Always wear a helmet when in rockfall zones"));
    Recommendations.Add(TEXT("Move quickly through high-risk areas"));
    Recommendations.Add(TEXT("Listen for warning sounds (cracking, grinding)"));
    
    if (CurrentRisk > 0.5f)
    {
        Recommendations.Add(TEXT("Consider alternative routes due to high risk"));
        Recommendations.Add(TEXT("Travel in small groups to minimize exposure"));
    }
    
    if (GetWorld())
    {
        // Get current time of day
        float TimeOfDay = GetWorld()->GetTimeSeconds();
        if (FMath::Fmod(TimeOfDay, 86400.0f) < 21600.0f) // Early morning (0-6 AM game time)
        {
            Recommendations.Add(TEXT("Early morning has increased freeze-thaw rockfall risk"));
        }
    }
    
    return Recommendations;
}

void ARockfallHazard::SpawnRock(ERockSize RockSize, const FVector& SpawnLocation, const FVector& InitialVelocity)
{
    if (!GetWorld() || RockMeshes.Num() == 0) return;

    // Create rock actor
    AStaticMeshActor* RockActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator);
    if (!RockActor) return;

    // Set random mesh from available rock meshes
    UStaticMesh* SelectedMesh = RockMeshes[FMath::RandRange(0, RockMeshes.Num() - 1)];
    RockActor->GetStaticMeshComponent()->SetStaticMesh(SelectedMesh);

    // Configure physics
    UStaticMeshComponent* MeshComp = RockActor->GetStaticMeshComponent();
    MeshComp->SetSimulatePhysics(true);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
    
    // Set mass based on rock size
    float Mass = RockSizeMasses.Contains(RockSize) ? RockSizeMasses[RockSize] : 10.0f;
    MeshComp->SetMassOverrideInKg(NAME_None, Mass, true);

    // Apply initial velocity
    MeshComp->SetPhysicsLinearVelocity(InitialVelocity);
    
    // Add random angular velocity for realistic tumbling
    FVector AngularVelocity = FVector(
        FMath::RandRange(-10.0f, 10.0f),
        FMath::RandRange(-10.0f, 10.0f),
        FMath::RandRange(-10.0f, 10.0f)
    );
    MeshComp->SetPhysicsAngularVelocityInRadians(AngularVelocity);

    // Bind impact events
    MeshComp->OnComponentHit.AddDynamic(this, &ARockfallHazard::OnRockImpact);

    // Add to active rocks list
    ActiveRocks.Add(RockActor);

    // Set destruction timer (cleanup after 60 seconds)
    FTimerHandle DestroyTimer;
    GetWorldTimerManager().SetTimer(
        DestroyTimer,
        [RockActor]()
        {
            if (IsValid(RockActor))
            {
                RockActor->Destroy();
            }
        },
        60.0f,
        false
    );
}

void ARockfallHazard::UpdateRockfallProbability(float DeltaTime)
{
    RockfallTimer += DeltaTime;
    
    // Check for rockfall every second
    if (RockfallTimer >= 1.0f)
    {
        float CurrentProbability = BaseProbability * CalculateEnvironmentalModifier();
        float RockfallChance = CurrentProbability / 3600.0f; // Convert hourly probability to per-second
        
        if (FMath::RandRange(0.0f, 1.0f) < RockfallChance)
        {
            // Trigger random rockfall event
            FRockfallEvent RandomEvent;
            RandomEvent.RockSize = static_cast<ERockSize>(FMath::RandRange(0, 4));
            RandomEvent.RockCount = FMath::RandRange(1, 3);
            RandomEvent.SpawnLocation = GetActorLocation() + FVector(0, 0, 1000); // 10m above
            RandomEvent.InitialVelocity = FVector(0, 0, -500); // Downward
            RandomEvent.Duration = 10.0f;
            RandomEvent.bPlayWarningSound = true;
            RandomEvent.WarningTime = 2.0f;
            
            TriggerRockfall(RandomEvent);
        }
        
        RockfallTimer = 0.0f;
    }
}

float ARockfallHazard::CalculateEnvironmentalModifier() const
{
    float Modifier = 1.0f;
    
    // Weather-based modifications would be implemented here
    // This is a simplified version - full implementation would query weather system
    
    // Simulate freeze-thaw cycles
    if (GetWorld())
    {
        float GameTime = GetWorld()->GetTimeSeconds();
        float TimeOfDay = FMath::Fmod(GameTime, 86400.0f);
        
        // Increased activity during dawn (freeze-thaw transition)
        if (TimeOfDay > 18000.0f && TimeOfDay < 25200.0f) // 5-7 AM
        {
            Modifier *= 2.0f;
        }
    }
    
    return Modifier;
}

void ARockfallHazard::OnProximityTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (TriggerType != ERockfallTrigger::Proximity) return;
    
    // Check if it's a player character
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (Pawn && Pawn->IsPlayerControlled())
    {
        // Trigger proximity-based rockfall
        FRockfallEvent ProximityEvent;
        ProximityEvent.RockSize = ERockSize::Cobble;
        ProximityEvent.RockCount = FMath::RandRange(2, 5);
        ProximityEvent.SpawnLocation = GetActorLocation() + FVector(
            FMath::RandRange(-500.0f, 500.0f),
            FMath::RandRange(-500.0f, 500.0f),
            1000.0f
        );
        ProximityEvent.InitialVelocity = FVector(0, 0, -300);
        ProximityEvent.bPlayWarningSound = true;
        ProximityEvent.WarningTime = 1.5f;
        
        TriggerRockfall(ProximityEvent);
        
        // Disable proximity trigger temporarily to prevent spam
        EnableProximityTrigger(false, 0.0f);
        
        FTimerHandle ReenableTimer;
        GetWorldTimerManager().SetTimer(
            ReenableTimer,
            [this]() { EnableProximityTrigger(true, ProximityTrigger->GetScaledSphereRadius()); },
            30.0f, // 30 second cooldown
            false
        );
    }
}

void ARockfallHazard::OnRockImpact(AActor* Rock, const FHitResult& HitResult)
{
    if (!HitResult.GetActor()) return;
    
    // Calculate impact force based on rock velocity and mass
    UStaticMeshComponent* RockMesh = Rock->FindComponentByClass<UStaticMeshComponent>();
    if (!RockMesh) return;
    
    FVector Velocity = RockMesh->GetPhysicsLinearVelocity();
    float Mass = RockMesh->GetMass();
    float ImpactForce = Velocity.Size() * Mass * 0.01f; // Simplified force calculation
    
    // Play impact sound
    if (RockImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), RockImpactSound, HitResult.Location);
    }
    
    // Broadcast impact event
    OnRockfallImpact.Broadcast(HitResult.GetActor(), ImpactForce);
    
    // Apply damage to hit actor if it's a character
    APawn* HitPawn = Cast<APawn>(HitResult.GetActor());
    if (HitPawn)
    {
        // Damage calculation based on impact force
        float Damage = FMath::Clamp(ImpactForce * 0.1f, 10.0f, 100.0f);
        
        // Apply damage through gameplay framework
        UGameplayStatics::ApplyDamage(
            HitPawn,
            Damage,
            nullptr, // No instigator controller for environmental damage
            Rock,
            UDamageType::StaticClass()
        );
    }
}

// Avalanche System Implementation

AAvalancheSystem::AAvalancheSystem()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    // Create avalanche zone
    AvalancheZone = CreateDefaultSubobject<UBoxComponent>(TEXT("AvalancheZone"));
    RootComponent = AvalancheZone;
    AvalancheZone->SetBoxExtent(FVector(5000.0f, 5000.0f, 1000.0f)); // 100m x 100m x 20m
    AvalancheZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AvalancheZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    AvalancheZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Create snow effect
    SnowEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SnowEffect"));
    SnowEffect->SetupAttachment(RootComponent);
    SnowEffect->SetAutoActivate(false);

    // Create audio component
    AvalancheAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AvalancheAudioComponent"));
    AvalancheAudioComponent->SetupAttachment(RootComponent);
    AvalancheAudioComponent->bAutoActivate = false;

    // Create snow pack mesh
    SnowPackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SnowPackMesh"));
    SnowPackMesh->SetupAttachment(RootComponent);
    SnowPackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Initialize default snow conditions
    CurrentSnowConditions.SnowDepth = 80.0f; // cm
    CurrentSnowConditions.NewSnowDepth = 5.0f;
    CurrentSnowConditions.SnowTemperature = -8.0f;
    CurrentSnowConditions.WindLoading = 15.0f;
    CurrentSnowConditions.SnowDensity = 350.0f;
    CurrentSnowConditions.Stability = ESnowStability::FairlyStable;
    CurrentSnowConditions.SlopeAngle = 35.0f;
    CurrentSnowConditions.bHasWeakLayer = false;
}

void AAvalancheSystem::BeginPlay()
{
    Super::BeginPlay();

    // Bind avalanche zone overlap
    if (AvalancheZone)
    {
        AvalancheZone->OnComponentBeginOverlap.AddDynamic(this, &AAvalancheSystem::OnAvalancheZoneBeginOverlap);
    }

    // Initialize snow stability assessment
    UpdateSnowStability();
}

void AAvalancheSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    Super::EndPlay(EndPlayReason);
}

void AAvalancheSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAvalancheSystem, CurrentSnowConditions);
}

void AAvalancheSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bAvalancheActive)
    {
        // Update avalanche simulation
        float ElapsedTime = GetWorld()->GetTimeSeconds() - AvalancheStartTime;
        
        // Check burial status for all actors in avalanche zone
        TArray<AActor*> OverlappingActors;
        AvalancheZone->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
        
        for (AActor* Actor : OverlappingActors)
        {
            if (!BurialDepths.Contains(Actor))
            {
                CheckForBuriedActors(GetActorLocation(), AvalancheZone->GetScaledBoxExtent().Size());
            }
        }
    }

    // Update snow stability based on environmental conditions
    static float StabilityUpdateTimer = 0.0f;
    StabilityUpdateTimer += DeltaTime;
    if (StabilityUpdateTimer >= 60.0f) // Update every minute
    {
        UpdateSnowStability();
        StabilityUpdateTimer = 0.0f;
    }
}

void AAvalancheSystem::TriggerAvalanche(const FAvalancheEvent& AvalancheEvent)
{
    if (!HasAuthority()) return;

    // Broadcast avalanche warning
    OnAvalancheWarning.Broadcast(AvalancheEvent);

    // Start avalanche audio
    if (AvalancheRumbleSound && AvalancheAudioComponent)
    {
        AvalancheAudioComponent->SetSound(AvalancheRumbleSound);
        AvalancheAudioComponent->Play();
    }

    // Activate snow effect
    if (SnowEffect)
    {
        SnowEffect->Activate();
        SnowEffect->SetFloatParameter(FName("Intensity"), 1.0f);
    }

    // Mark avalanche as active
    bAvalancheActive = true;
    AvalancheStartTime = GetWorld()->GetTimeSeconds();

    // Process avalanche flow
    ProcessAvalancheFlow(AvalancheEvent, 0.0f);

    // Set timer to end avalanche
    FTimerHandle AvalancheEndTimer;
    GetWorldTimerManager().SetTimer(
        AvalancheEndTimer,
        [this]()
        {
            bAvalancheActive = false;
            if (SnowEffect) SnowEffect->Deactivate();
            if (AvalancheAudioComponent) AvalancheAudioComponent->Stop();
        },
        AvalancheEvent.Duration,
        false
    );
}

void AAvalancheSystem::UpdateSnowConditions(const FSnowConditions& NewConditions)
{
    ESnowStability OldStability = CurrentSnowConditions.Stability;
    CurrentSnowConditions = NewConditions;
    UpdateSnowStability();
    
    if (CurrentSnowConditions.Stability != OldStability)
    {
        OnSnowStabilityChanged.Broadcast(CurrentSnowConditions.Stability);
    }
}

ESnowStability AAvalancheSystem::AssessSnowStability(const FVector& Location) const
{
    return UGeologicalHazardLibrary::CalculateSnowStability(CurrentSnowConditions);
}

float AAvalancheSystem::CalculateAvalancheRisk(const FVector& Location) const
{
    float RiskScore = 0.0f;

    // Slope angle risk
    if (CurrentSnowConditions.SlopeAngle > 30.0f && CurrentSnowConditions.SlopeAngle < 45.0f)
    {
        RiskScore += 0.3f; // Highest risk slope angles
    }
    else if (CurrentSnowConditions.SlopeAngle > 25.0f)
    {
        RiskScore += 0.2f;
    }

    // Snow depth risk
    if (CurrentSnowConditions.SnowDepth > CriticalSnowDepth)
    {
        RiskScore += 0.2f;
    }

    // New snow risk
    if (CurrentSnowConditions.NewSnowDepth > 25.0f)
    {
        RiskScore += 0.2f;
    }

    // Wind loading risk
    if (CurrentSnowConditions.WindLoading > CriticalWindLoading)
    {
        RiskScore += 0.1f;
    }

    // Temperature risk (warming conditions)
    if (CurrentSnowConditions.SnowTemperature > -2.0f)
    {
        RiskScore += 0.1f;
    }

    // Weak layer risk
    if (CurrentSnowConditions.bHasWeakLayer)
    {
        RiskScore += 0.1f;
    }

    return FMath::Clamp(RiskScore, 0.0f, 1.0f);
}

bool AAvalancheSystem::IsActorBuriedInAvalanche(AActor* Actor) const
{
    return BuriedActors.Contains(Actor);
}

float AAvalancheSystem::GetBurialDepth(AActor* Actor) const
{
    return BurialDepths.Contains(Actor) ? BurialDepths[Actor] : 0.0f;
}

TArray<FVector> AAvalancheSystem::LocateTransceiverSignals(const FVector& SearchCenter, float SearchRadius) const
{
    TArray<FVector> SignalLocations;
    
    // Simulate transceiver signals from buried actors
    for (const auto& BurialPair : BuriedActors)
    {
        AActor* BuriedActor = BurialPair.Key;
        if (!IsValid(BuriedActor)) continue;
        
        FVector BurialLocation = BuriedActor->GetActorLocation();
        float Distance = FVector::Dist(SearchCenter, BurialLocation);
        
        if (Distance <= SearchRadius && Distance <= TransceiverRange)
        {
            // Add some signal error/noise
            FVector NoiseOffset = FVector(
                FMath::RandRange(-50.0f, 50.0f),
                FMath::RandRange(-50.0f, 50.0f),
                0.0f
            );
            SignalLocations.Add(BurialLocation + NoiseOffset);
        }
    }
    
    return SignalLocations;
}

void AAvalancheSystem::StartRescueOperation(const FVector& BurialLocation)
{
    // Find buried actor at location
    AActor* NearestBuriedActor = nullptr;
    float NearestDistance = 200.0f; // 2 meter search radius
    
    for (AActor* BuriedActor : BuriedActors)
    {
        if (!IsValid(BuriedActor)) continue;
        
        float Distance = FVector::Dist(BurialLocation, BuriedActor->GetActorLocation());
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestBuriedActor = BuriedActor;
        }
    }
    
    if (NearestBuriedActor)
    {
        // Start rescue timer
        float BurialTime = BurialTimes.Contains(NearestBuriedActor) ? 
            (GetWorld()->GetTimeSeconds() - BurialTimes[NearestBuriedActor]) : 0.0f;
        
        float RescueTime = FMath::RandRange(300.0f, 900.0f); // 5-15 minutes
        float Depth = GetBurialDepth(NearestBuriedActor);
        
        // Play rescue sound
        if (RescueDiggingSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), RescueDiggingSound, BurialLocation);
        }
        
        // Set rescue completion timer
        FTimerHandle RescueTimer;
        GetWorldTimerManager().SetTimer(
            RescueTimer,
            [this, NearestBuriedActor, BurialTime]()
            {
                if (IsValid(NearestBuriedActor))
                {
                    // Successful rescue
                    BuriedActors.Remove(NearestBuriedActor);
                    BurialDepths.Remove(NearestBuriedActor);
                    BurialTimes.Remove(NearestBuriedActor);
                    
                    // Calculate survival chance based on burial time
                    bool bSurvived = BurialTime < (RescueTimeLimit * 60.0f);
                    
                    if (bSurvived)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Successful avalanche rescue!"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Avalanche rescue too late..."));
                    }
                }
            },
            RescueTime,
            false
        );
    }
}

FString AAvalancheSystem::GenerateAvalancheBulletin() const
{
    FString Bulletin = TEXT("AVALANCHE BULLETIN\n\n");
    
    FString RiskLevel;
    float Risk = CalculateAvalancheRisk(GetActorLocation());
    if (Risk < 0.2f) RiskLevel = TEXT("LOW (Green)");
    else if (Risk < 0.4f) RiskLevel = TEXT("MODERATE (Yellow)");
    else if (Risk < 0.6f) RiskLevel = TEXT("CONSIDERABLE (Orange)");
    else if (Risk < 0.8f) RiskLevel = TEXT("HIGH (Red)");
    else RiskLevel = TEXT("EXTREME (Black)");
    
    Bulletin += FString::Printf(TEXT("Avalanche Danger: %s\n\n"), *RiskLevel);
    Bulletin += FString::Printf(TEXT("Snow Depth: %.0f cm\n"), CurrentSnowConditions.SnowDepth);
    Bulletin += FString::Printf(TEXT("New Snow: %.0f cm (24hr)\n"), CurrentSnowConditions.NewSnowDepth);
    Bulletin += FString::Printf(TEXT("Snow Temperature: %.1f°C\n"), CurrentSnowConditions.SnowTemperature);
    Bulletin += FString::Printf(TEXT("Wind Loading: %.0f cm\n"), CurrentSnowConditions.WindLoading);
    Bulletin += FString::Printf(TEXT("Slope Angle: %.1f°\n"), CurrentSnowConditions.SlopeAngle);
    
    if (CurrentSnowConditions.bHasWeakLayer)
    {
        Bulletin += FString::Printf(TEXT("Weak Layer Detected at %.0f cm depth\n"), CurrentSnowConditions.WeakLayerDepth);
    }
    
    return Bulletin;
}

TArray<FString> AAvalancheSystem::GetAvalancheSafetyTips() const
{
    TArray<FString> SafetyTips;
    
    SafetyTips.Add(TEXT("Always carry avalanche safety equipment: beacon, probe, shovel"));
    SafetyTips.Add(TEXT("Check avalanche bulletins before entering backcountry"));
    SafetyTips.Add(TEXT("Travel one at a time in avalanche terrain"));
    SafetyTips.Add(TEXT("Identify safe zones and escape routes"));
    SafetyTips.Add(TEXT("Avoid slopes 30-45 degrees during unstable conditions"));
    
    float Risk = CalculateAvalancheRisk(GetActorLocation());
    if (Risk > 0.6f)
    {
        SafetyTips.Add(TEXT("HIGH RISK: Consider avoiding avalanche terrain entirely"));
        SafetyTips.Add(TEXT("Stick to lower angle slopes and ridgetops"));
    }
    
    if (CurrentSnowConditions.NewSnowDepth > 25.0f)
    {
        SafetyTips.Add(TEXT("Recent snowfall increases avalanche risk - allow time to settle"));
    }
    
    if (CurrentSnowConditions.WindLoading > 25.0f)
    {
        SafetyTips.Add(TEXT("Wind loading present - avoid leeward slopes"));
    }
    
    return SafetyTips;
}

bool AAvalancheSystem::PerformSnowPitTest(const FVector& Location, FSnowConditions& OutResults) const
{
    // Simulate snow pit analysis
    OutResults = CurrentSnowConditions;
    
    // Add some variation based on location
    float LocationVariation = FMath::PerlinNoise1D(Location.X * 0.001f + Location.Y * 0.001f);
    OutResults.SnowDepth += LocationVariation * 20.0f;
    OutResults.SnowDensity += LocationVariation * 50.0f;
    
    // Determine if test reveals instability
    return (OutResults.Stability == ESnowStability::Unstable || OutResults.Stability == ESnowStability::VeryUnstable);
}

void AAvalancheSystem::UpdateSnowStability()
{
    ESnowStability OldStability = CurrentSnowConditions.Stability;
    CurrentSnowConditions.Stability = UGeologicalHazardLibrary::CalculateSnowStability(CurrentSnowConditions);
    
    if (CurrentSnowConditions.Stability != OldStability)
    {
        OnSnowStabilityChanged.Broadcast(CurrentSnowConditions.Stability);
    }
}

void AAvalancheSystem::ProcessAvalancheFlow(const FAvalancheEvent& Event, float DeltaTime)
{
    // Check for actors in avalanche path
    TArray<AActor*> OverlappingActors;
    AvalancheZone->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
    
    for (AActor* Actor : OverlappingActors)
    {
        if (!BuriedActors.Contains(Actor))
        {
            // Bury the actor
            BuriedActors.Add(Actor);
            BurialDepths.Add(Actor, FMath::RandRange(0.5f, 3.0f)); // 0.5-3 meter burial depth
            BurialTimes.Add(Actor, GetWorld()->GetTimeSeconds());
            
            // Broadcast burial event
            OnAvalancheImpact.Broadcast(Actor, BurialDepths[Actor]);
            
            // Disable actor movement (simplified burial simulation)
            APawn* Pawn = Cast<APawn>(Actor);
            if (Pawn)
            {
                Pawn->DisableInput(nullptr);
            }
        }
    }
}

void AAvalancheSystem::CheckForBuriedActors(const FVector& AvalancheCenter, float Radius)
{
    TArray<AActor*> NearbyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), NearbyActors);
    
    for (AActor* Actor : NearbyActors)
    {
        float Distance = FVector::Dist(Actor->GetActorLocation(), AvalancheCenter);
        if (Distance <= Radius && !BuriedActors.Contains(Actor))
        {
            // Probability of burial based on distance from center
            float BurialProbability = 1.0f - (Distance / Radius);
            if (FMath::RandRange(0.0f, 1.0f) < BurialProbability)
            {
                BuriedActors.Add(Actor);
                BurialDepths.Add(Actor, FMath::RandRange(0.3f, 2.5f));
                BurialTimes.Add(Actor, GetWorld()->GetTimeSeconds());
                
                OnAvalancheImpact.Broadcast(Actor, BurialDepths[Actor]);
            }
        }
    }
}

void AAvalancheSystem::OnAvalancheZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // This could be used to trigger human-triggered avalanches
    // For now, it's primarily used for tracking actors in avalanche zone
}

// Geological Hazard Library Implementation

float UGeologicalHazardLibrary::CalculateRockfallTrajectory(const FVector& StartLocation, const FVector& InitialVelocity, float Mass, float DragCoefficient, float TimeStep)
{
    // Simplified physics calculation for rockfall trajectory
    FVector Position = StartLocation;
    FVector Velocity = InitialVelocity;
    
    float Gravity = 980.0f; // cm/s²
    float AirDensity = 0.00125f; // g/cm³
    float Time = 0.0f;
    
    while (Position.Z > 0.0f && Time < 30.0f) // Max 30 seconds
    {
        // Apply gravity
        Velocity.Z -= Gravity * TimeStep;
        
        // Apply air resistance
        float Speed = Velocity.Size();
        if (Speed > 0.0f)
        {
            FVector DragForce = -Velocity.GetSafeNormal() * 0.5f * AirDensity * Speed * Speed * DragCoefficient;
            FVector Acceleration = DragForce / Mass;
            Velocity += Acceleration * TimeStep;
        }
        
        // Update position
        Position += Velocity * TimeStep;
        Time += TimeStep;
        
        // Simple ground collision
        if (Position.Z <= 0.0f)
        {
            break;
        }
    }
    
    return Time;
}

FVector UGeologicalHazardLibrary::PredictRockImpactLocation(const FVector& StartLocation, const FVector& InitialVelocity, float Mass)
{
    FVector Position = StartLocation;
    FVector Velocity = InitialVelocity;
    
    float Gravity = 980.0f; // cm/s²
    float TimeStep = 0.1f;
    
    while (Position.Z > 0.0f)
    {
        Velocity.Z -= Gravity * TimeStep;
        Position += Velocity * TimeStep;
        
        if (Position.Z <= 0.0f)
        {
            break;
        }
    }
    
    return Position;
}

float UGeologicalHazardLibrary::CalculateAvalancheRunoutDistance(const FSnowConditions& SnowConditions, float SlopeAngle, float VerticalDrop)
{
    // Simplified runout calculation based on alpha-beta model
    float AlphaAngle = 0.96f * SlopeAngle - 1.4f; // Empirical formula
    float BetaAngle = SlopeAngle * 0.5f; // Simplified
    
    float RunoutDistance = VerticalDrop / FMath::Tan(FMath::DegreesToRadians(AlphaAngle));
    
    // Adjust for snow density
    float DensityFactor = SnowConditions.SnowDensity / 300.0f; // Relative to average density
    RunoutDistance *= DensityFactor;
    
    return FMath::Max(RunoutDistance, 0.0f);
}

ESnowStability UGeologicalHazardLibrary::CalculateSnowStability(const FSnowConditions& Conditions)
{
    int32 StabilityScore = 0;
    
    // Slope angle factor
    if (Conditions.SlopeAngle > 35.0f && Conditions.SlopeAngle < 45.0f)
        StabilityScore += 3; // Most dangerous slope angles
    else if (Conditions.SlopeAngle > 30.0f)
        StabilityScore += 2;
    else if (Conditions.SlopeAngle > 25.0f)
        StabilityScore += 1;
    
    // New snow factor
    if (Conditions.NewSnowDepth > 30.0f)
        StabilityScore += 3;
    else if (Conditions.NewSnowDepth > 20.0f)
        StabilityScore += 2;
    else if (Conditions.NewSnowDepth > 10.0f)
        StabilityScore += 1;
    
    // Wind loading factor
    if (Conditions.WindLoading > 25.0f)
        StabilityScore += 2;
    else if (Conditions.WindLoading > 15.0f)
        StabilityScore += 1;
    
    // Temperature factor
    if (Conditions.SnowTemperature > -1.0f)
        StabilityScore += 2; // Warming conditions
    else if (Conditions.SnowTemperature < -15.0f)
        StabilityScore += 1; // Very cold, brittle snow
    
    // Weak layer factor
    if (Conditions.bHasWeakLayer)
        StabilityScore += 3;
    
    // Convert score to stability rating
    if (StabilityScore >= 10) return ESnowStability::VeryUnstable;
    else if (StabilityScore >= 7) return ESnowStability::Unstable;
    else if (StabilityScore >= 4) return ESnowStability::FairlyStable;
    else if (StabilityScore >= 2) return ESnowStability::Stable;
    else return ESnowStability::VeryStable;
}

float UGeologicalHazardLibrary::CalculateAvalancheSpeed(EAvalancheType Type, float SlopeAngle, const FSnowConditions& Conditions)
{
    float BaseSpeed = 0.0f;
    
    // Base speeds by avalanche type (km/h)
    switch (Type)
    {
    case EAvalancheType::SlabAvalanche:
        BaseSpeed = 80.0f;
        break;
    case EAvalancheType::LooseSnow:
        BaseSpeed = 30.0f;
        break;
    case EAvalancheType::WetAvalanche:
        BaseSpeed = 20.0f;
        break;
    case EAvalancheType::WindSlab:
        BaseSpeed = 60.0f;
        break;
    case EAvalancheType::IceAvalanche:
        BaseSpeed = 100.0f;
        break;
    case EAvalancheType::Cornice:
        BaseSpeed = 40.0f;
        break;
    }
    
    // Modify by slope angle
    float SlopeFactor = FMath::Sin(FMath::DegreesToRadians(SlopeAngle));
    BaseSpeed *= SlopeFactor;
    
    // Modify by snow density
    float DensityFactor = Conditions.SnowDensity / 400.0f;
    BaseSpeed *= DensityFactor;
    
    return FMath::Max(BaseSpeed, 5.0f);
}

bool UGeologicalHazardLibrary::IsRockfallLikelyFromWeather(float Temperature, float Precipitation, float WindSpeed)
{
    // Freeze-thaw conditions
    bool FreezeThaw = (Temperature > -2.0f && Temperature < 5.0f);
    
    // Heavy precipitation
    bool HeavyRain = (Precipitation > 10.0f);
    
    // High winds
    bool StrongWinds = (WindSpeed > 30.0f);
    
    return FreezeThaw || HeavyRain || StrongWinds;
}

float UGeologicalHazardLibrary::CalculateRockQualityRating(float WeatherExposure, float RockAge, float JointDensity)
{
    float Quality = 1.0f;
    
    // Weather exposure reduces quality
    Quality *= (1.0f - FMath::Clamp(WeatherExposure, 0.0f, 0.8f));
    
    // Age reduces quality
    float AgeFactor = FMath::Exp(-RockAge * 0.001f);
    Quality *= AgeFactor;
    
    // Joint density reduces quality
    Quality *= (1.0f - FMath::Clamp(JointDensity, 0.0f, 0.7f));
    
    return FMath::Clamp(Quality, 0.1f, 1.0f);
}

TArray<FVector> UGeologicalHazardLibrary::GenerateEscapeRoutes(const FVector& HazardCenter, float HazardRadius, const TArray<FVector>& SafeZones)
{
    TArray<FVector> EscapeRoutes;
    
    for (const FVector& SafeZone : SafeZones)
    {
        FVector Direction = (SafeZone - HazardCenter).GetSafeNormal();
        FVector EscapePoint = HazardCenter + Direction * (HazardRadius + 500.0f);
        EscapeRoutes.Add(EscapePoint);
    }
    
    return EscapeRoutes;
}