#include "ClimbingNetworkOptimizer.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetConnection.h"
#include "Components/PrimitiveComponent.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

DECLARE_STATS_GROUP(TEXT("ClimbingNetwork"), STATGROUP_ClimbingNetwork, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Network Optimization"), STAT_NetworkOptimization, STATGROUP_ClimbingNetwork);
DECLARE_CYCLE_STAT(TEXT("Player Replication"), STAT_PlayerReplication, STATGROUP_ClimbingNetwork);
DECLARE_CYCLE_STAT(TEXT("Rope Replication"), STAT_RopeReplication, STATGROUP_ClimbingNetwork);
DECLARE_DWORD_COUNTER_STAT(TEXT("Bytes Per Second"), STAT_BytesPerSecond, STATGROUP_ClimbingNetwork);
DECLARE_DWORD_COUNTER_STAT(TEXT("Packets Per Second"), STAT_PacketsPerSecond, STATGROUP_ClimbingNetwork);

void UClimbingNetworkOptimizer::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Initialize bandwidth history
    BandwidthHistory.Reserve(MaxHistorySize);
    LatencyHistory.Reserve(MaxHistorySize);

    // Initialize compression ratios
    CompressionRatios.Add(ENetworkCompressionLevel::None, 1.0f);
    CompressionRatios.Add(ENetworkCompressionLevel::Light, 0.8f);
    CompressionRatios.Add(ENetworkCompressionLevel::Medium, 0.6f);
    CompressionRatios.Add(ENetworkCompressionLevel::Heavy, 0.4f);
    CompressionRatios.Add(ENetworkCompressionLevel::Maximum, 0.25f);

    // Initialize update policy defaults
    UpdatePolicy.CriticalUpdateRate = 60.0f;
    UpdatePolicy.HighUpdateRate = 30.0f;
    UpdatePolicy.MediumUpdateRate = 15.0f;
    UpdatePolicy.LowUpdateRate = 10.0f;
    UpdatePolicy.DeferredUpdateRate = 5.0f;

    // Initialize bandwidth budget defaults
    BandwidthBudget.TotalBudgetKBps = 256.0f;
    BandwidthBudget.PlayerMovementBudget = 0.4f;
    BandwidthBudget.RopePhysicsBudget = 0.3f;
    BandwidthBudget.ToolInteractionBudget = 0.2f;
    BandwidthBudget.ChatAndUIBudget = 0.1f;

    UE_LOG(LogNet, Log, TEXT("ClimbingNetworkOptimizer initialized"));
}

void UClimbingNetworkOptimizer::Deinitialize()
{
    ConnectionProfiles.Empty();
    TrackedRopes.Empty();
    BandwidthHistory.Empty();
    LatencyHistory.Empty();
    CompressionBuffers.Empty();

    Super::Deinitialize();
}

void UClimbingNetworkOptimizer::Tick(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_NetworkOptimization);

    NetworkUpdateAccumulator += DeltaTime;

    if (NetworkUpdateAccumulator >= NetworkUpdateInterval)
    {
        OptimizeNetworkTraffic(NetworkUpdateAccumulator);
        UpdateConnectionProfiles();
        UpdateNetworkStatistics(NetworkUpdateAccumulator);
        NetworkUpdateAccumulator = 0.0f;
    }
}

void UClimbingNetworkOptimizer::OptimizeNetworkTraffic(float DeltaTime)
{
    if (!GetWorld() || !GetWorld()->GetAuthGameMode())
        return;

    // Update network priorities first
    UpdateNetworkPriorities();

    // Adapt to current network conditions
    AdaptToNetworkConditions();

    // Optimize replication for each category
    for (auto& ConnectionPair : ConnectionProfiles)
    {
        OptimizePlayerReplication(ConnectionPair.Key, DeltaTime);
    }

    OptimizeRopeReplication(DeltaTime);
    OptimizeToolReplication(DeltaTime);

    // Enforce bandwidth limits
    EnforceBandwidthLimits();

    // Update replication priorities based on current frame
    UpdateReplicationPriorities();
}

void UClimbingNetworkOptimizer::UpdateNetworkPriorities()
{
    if (!bEnablePrioritySystem)
        return;

    for (auto& ConnectionPair : ConnectionProfiles)
    {
        APlayerController* PC = ConnectionPair.Key;
        if (!IsValid(PC) || !PC->GetPawn())
            continue;

        FVector ViewerLocation = PC->GetPawn()->GetActorLocation();

        // Update priorities for all relevant actors
        for (TActorIterator<AActor> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
        {
            AActor* Actor = *ActorIterator;
            if (!IsValid(Actor) || !IsActorRelevantToPlayer(Actor, PC))
                continue;

            ENetworkPriority Priority = CalculateActorPriority(Actor, PC);
            float UpdateRate = CalculateUpdateRate(Actor, PC);

            // Apply priority-based optimizations
            if (Actor->GetNetUpdateFrequency() != UpdateRate)
            {
                Actor->SetNetUpdateTime(UpdateRate > 0 ? 1.0f / UpdateRate : 0.0f);
            }
        }
    }
}

void UClimbingNetworkOptimizer::AdaptToNetworkConditions()
{
    if (!bEnableAdaptiveCompression)
        return;

    for (auto& ConnectionPair : ConnectionProfiles)
    {
        FConnectionProfile& Profile = ConnectionPair.Value;
        AnalyzeConnectionQuality(Profile);

        // Adjust compression level based on connection quality
        ENetworkCompressionLevel OptimalLevel = GetOptimalCompressionLevel(Profile);
        if (Profile.CompressionLevel != OptimalLevel)
        {
            SetCompressionLevel(ConnectionPair.Key, OptimalLevel);
            Profile.CompressionLevel = OptimalLevel;
            
            UE_LOG(LogNet, Log, TEXT("Adjusted compression level for %s to %s"), 
                   *ConnectionPair.Key->GetName(),
                   *UEnum::GetValueAsString(OptimalLevel));
        }
    }
}

float UClimbingNetworkOptimizer::GetCurrentBandwidthUsage() const
{
    return CurrentMetrics.BandwidthUsageKBps;
}

bool UClimbingNetworkOptimizer::IsWithinBandwidthBudget() const
{
    return CurrentMetrics.BandwidthUsageKBps <= BandwidthBudget.TotalBudgetKBps;
}

void UClimbingNetworkOptimizer::EnforceBandwidthLimits()
{
    if (IsWithinBandwidthBudget())
        return;

    float OveragePercent = (CurrentMetrics.BandwidthUsageKBps - BandwidthBudget.TotalBudgetKBps) / BandwidthBudget.TotalBudgetKBps;
    
    if (OveragePercent > 0.1f) // 10% overage threshold
    {
        OnBandwidthExceeded.Broadcast();
        
        // Implement bandwidth reduction strategies
        
        // 1. Reduce update rates for low-priority actors
        for (auto& ConnectionPair : ConnectionProfiles)
        {
            APlayerController* PC = ConnectionPair.Key;
            if (!IsValid(PC))
                continue;

            // Reduce update rates for distant actors
            for (TActorIterator<AActor> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
            {
                AActor* Actor = *ActorIterator;
                if (!IsValid(Actor))
                    continue;

                float Distance = GetNetworkDistance(Actor, PC);
                if (Distance > UpdatePolicy.MediumDistance)
                {
                    float ReducedRate = Actor->GetNetUpdateFrequency() * 0.7f;
                    Actor->SetNetUpdateTime(ReducedRate > 0 ? 1.0f / ReducedRate : 0.0f);
                }
            }
        }

        // 2. Increase compression for all connections
        for (auto& ConnectionPair : ConnectionProfiles)
        {
            FConnectionProfile& Profile = ConnectionPair.Value;
            if (Profile.CompressionLevel < ENetworkCompressionLevel::Heavy)
            {
                ENetworkCompressionLevel NewLevel = static_cast<ENetworkCompressionLevel>(
                    static_cast<uint8>(Profile.CompressionLevel) + 1);
                SetCompressionLevel(ConnectionPair.Key, NewLevel);
                Profile.CompressionLevel = NewLevel;
            }
        }

        UE_LOG(LogNet, Warning, TEXT("Bandwidth overage detected: %.1f%%. Applying reduction strategies."), 
               OveragePercent * 100.0f);
    }
}

FQuantizedPlayerState UClimbingNetworkOptimizer::QuantizePlayerState(ACharacter* Player)
{
    if (!IsValid(Player))
        return FQuantizedPlayerState();

    return FQuantizedPlayerState::FromPlayerState(
        Player->GetActorLocation(),
        Player->GetActorRotation(),
        Player->GetVelocity(),
        0, // Grip state - would come from climbing component
        100 // Stamina - would come from climbing component
    );
}

FQuantizedRopeState UClimbingNetworkOptimizer::QuantizeRopeState(UAdvancedRopeComponent* Rope, uint8 RopeID, uint8 OwnerID)
{
    if (!IsValid(Rope))
        return FQuantizedRopeState();

    return FQuantizedRopeState::FromRopeComponent(Rope, RopeID, OwnerID);
}

void UClimbingNetworkOptimizer::SetCompressionLevel(APlayerController* PC, ENetworkCompressionLevel Level)
{
    if (!IsValid(PC) || !PC->GetNetConnection())
        return;

    // Apply compression settings to the connection
    UNetConnection* Connection = PC->GetNetConnection();
    
    switch (Level)
    {
        case ENetworkCompressionLevel::None:
            // No special compression
            break;
        case ENetworkCompressionLevel::Light:
            // Light compression settings
            break;
        case ENetworkCompressionLevel::Medium:
            // Medium compression settings
            break;
        case ENetworkCompressionLevel::Heavy:
            // Heavy compression settings
            break;
        case ENetworkCompressionLevel::Maximum:
            // Maximum compression settings
            break;
    }

    if (FConnectionProfile* Profile = ConnectionProfiles.Find(PC))
    {
        Profile->CompressionLevel = Level;
    }
}

ENetworkPriority UClimbingNetworkOptimizer::CalculateActorPriority(AActor* Actor, APlayerController* Viewer) const
{
    if (!IsValid(Actor) || !IsValid(Viewer))
        return ENetworkPriority::Deferred;

    FVector ViewerLocation = Viewer->GetPawn() ? Viewer->GetPawn()->GetActorLocation() : FVector::ZeroVector;
    float Distance = FVector::Dist(Actor->GetActorLocation(), ViewerLocation);

    // Distance-based priority
    ENetworkPriority DistancePriority;
    if (Distance <= UpdatePolicy.CriticalDistance)
        DistancePriority = ENetworkPriority::Critical;
    else if (Distance <= UpdatePolicy.HighDistance)
        DistancePriority = ENetworkPriority::High;
    else if (Distance <= UpdatePolicy.MediumDistance)
        DistancePriority = ENetworkPriority::Medium;
    else if (Distance <= UpdatePolicy.LowDistance)
        DistancePriority = ENetworkPriority::Low;
    else
        DistancePriority = ENetworkPriority::Deferred;

    // Adjust priority based on actor type and gameplay importance
    if (Actor == Viewer->GetPawn())
    {
        return ENetworkPriority::Critical; // Player's own character
    }
    else if (Cast<ACharacter>(Actor))
    {
        // Other players are high priority when close
        return DistancePriority >= ENetworkPriority::Medium ? ENetworkPriority::High : DistancePriority;
    }
    else if (Actor->FindComponentByClass<UAdvancedRopeComponent>())
    {
        // Ropes are critical for climbing gameplay
        return DistancePriority >= ENetworkPriority::Low ? ENetworkPriority::High : DistancePriority;
    }

    return DistancePriority;
}

float UClimbingNetworkOptimizer::CalculateUpdateRate(AActor* Actor, APlayerController* Viewer) const
{
    ENetworkPriority Priority = CalculateActorPriority(Actor, Viewer);

    switch (Priority)
    {
        case ENetworkPriority::Critical:
            return UpdatePolicy.CriticalUpdateRate;
        case ENetworkPriority::High:
            return UpdatePolicy.HighUpdateRate;
        case ENetworkPriority::Medium:
            return UpdatePolicy.MediumUpdateRate;
        case ENetworkPriority::Low:
            return UpdatePolicy.LowUpdateRate;
        default:
            return UpdatePolicy.DeferredUpdateRate;
    }
}

void UClimbingNetworkOptimizer::UpdateReplicationPriorities()
{
    // This would typically integrate with Unreal's replication priority system
    for (auto& ConnectionPair : ConnectionProfiles)
    {
        APlayerController* PC = ConnectionPair.Key;
        if (!IsValid(PC))
            continue;

        // Update actor priorities for this connection
        for (TActorIterator<AActor> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
        {
            AActor* Actor = *ActorIterator;
            if (!IsValid(Actor))
                continue;

            ENetworkPriority Priority = CalculateActorPriority(Actor, PC);
            
            // Set replication priority based on our calculated priority
            float ReplicationPriority = 1.0f;
            switch (Priority)
            {
                case ENetworkPriority::Critical:
                    ReplicationPriority = 10.0f;
                    break;
                case ENetworkPriority::High:
                    ReplicationPriority = 5.0f;
                    break;
                case ENetworkPriority::Medium:
                    ReplicationPriority = 2.0f;
                    break;
                case ENetworkPriority::Low:
                    ReplicationPriority = 1.0f;
                    break;
                case ENetworkPriority::Deferred:
                    ReplicationPriority = 0.5f;
                    break;
            }

            // Apply the priority (this would use Unreal's priority system)
            Actor->SetNetPriority(ReplicationPriority);
        }
    }
}

void UClimbingNetworkOptimizer::RegisterPlayerConnection(APlayerController* PC)
{
    if (!IsValid(PC))
        return;

    FConnectionProfile Profile;
    Profile.PlayerController = PC;
    Profile.CurrentBandwidthKBps = 0.0f;
    Profile.AverageLatencyMs = 50.0f;
    Profile.PacketLossRate = 0.0f;
    Profile.CompressionLevel = ENetworkCompressionLevel::Light;
    Profile.LastUpdateTime = FDateTime::Now();
    Profile.DroppedUpdates = 0;
    Profile.bIsUnreliableConnection = false;

    ConnectionProfiles.Add(PC, Profile);

    // Initialize compression buffer
    CompressionBuffers.Add(PC, TArray<uint8>());
    CompressionBuffers[PC].Reserve(1024); // 1KB initial buffer

    UE_LOG(LogNet, Log, TEXT("Registered player connection: %s"), *PC->GetName());
}

void UClimbingNetworkOptimizer::UnregisterPlayerConnection(APlayerController* PC)
{
    if (ConnectionProfiles.Remove(PC) > 0)
    {
        CompressionBuffers.Remove(PC);
        UE_LOG(LogNet, Log, TEXT("Unregistered player connection: %s"), *PC->GetName());
    }
}

void UClimbingNetworkOptimizer::UpdateConnectionProfiles()
{
    for (auto& ConnectionPair : ConnectionProfiles)
    {
        APlayerController* PC = ConnectionPair.Key;
        FConnectionProfile& Profile = ConnectionPair.Value;

        if (!IsValid(PC) || !PC->GetNetConnection())
        {
            Profile.bIsUnreliableConnection = true;
            continue;
        }

        UNetConnection* Connection = PC->GetNetConnection();
        
        // Update connection metrics
        Profile.AverageLatencyMs = Connection->AvgLag * 1000.0f; // Convert to milliseconds
        Profile.CurrentBandwidthKBps = Connection->OutBytesPerSecond / 1024.0f;
        Profile.PacketLossRate = Connection->OutPacketsLost / FMath::Max(1.0f, static_cast<float>(Connection->OutPackets));

        // Update history
        Profile.LatencyHistory.Add(Profile.AverageLatencyMs);
        Profile.BandwidthHistory.Add(Profile.CurrentBandwidthKBps);

        // Keep history size manageable
        if (Profile.LatencyHistory.Num() > 100)
        {
            Profile.LatencyHistory.RemoveAt(0);
        }
        if (Profile.BandwidthHistory.Num() > 100)
        {
            Profile.BandwidthHistory.RemoveAt(0);
        }

        // Detect connection quality issues
        if (Profile.AverageLatencyMs > MaxAcceptableLatency)
        {
            OnHighLatencyDetected.Broadcast();
        }

        if (Profile.PacketLossRate > MaxAcceptablePacketLoss)
        {
            OnPacketLossDetected.Broadcast();
        }

        Profile.LastUpdateTime = FDateTime::Now();
    }
}

FVector UClimbingNetworkOptimizer::PredictPlayerPosition(ACharacter* Player, float PredictionTime) const
{
    if (!IsValid(Player))
        return FVector::ZeroVector;

    FVector CurrentLocation = Player->GetActorLocation();
    FVector CurrentVelocity = Player->GetVelocity();
    
    // Simple linear prediction - could be enhanced with physics simulation
    return CurrentLocation + CurrentVelocity * PredictionTime;
}

void UClimbingNetworkOptimizer::CompensateForLatency(APlayerController* PC, float ClientTimeStamp)
{
    if (!bEnableLatencyCompensation || !IsValid(PC))
        return;

    const FConnectionProfile* Profile = ConnectionProfiles.Find(PC);
    if (!Profile)
        return;

    float LatencySeconds = Profile->AverageLatencyMs / 1000.0f;
    float ServerTime = GetWorld()->GetTimeSeconds();
    float TimeDifference = ServerTime - ClientTimeStamp;

    // If the time difference is significantly different from expected latency,
    // apply compensation
    if (FMath::Abs(TimeDifference - LatencySeconds) > 0.1f) // 100ms tolerance
    {
        // Apply lag compensation by rewinding world state
        // This is a simplified version - full implementation would store world snapshots
        UE_LOG(LogNet, VeryVerbose, TEXT("Applying latency compensation for %s: %.1fms"), 
               *PC->GetName(), Profile->AverageLatencyMs);
    }
}

bool UClimbingNetworkOptimizer::ValidatePlayerAction(ACharacter* Player, const FVector& ActionLocation, float Timestamp) const
{
    if (!IsValid(Player))
        return false;

    // Basic validation - check if action location is reasonable given player position and movement
    FVector PlayerLocation = Player->GetActorLocation();
    float Distance = FVector::Dist(PlayerLocation, ActionLocation);
    float MaxReachDistance = 200.0f; // 2 meters reach

    if (Distance > MaxReachDistance)
    {
        UE_LOG(LogNet, Warning, TEXT("Invalid player action: distance %.1f exceeds max reach %.1f"), 
               Distance, MaxReachDistance);
        return false;
    }

    return true;
}

FNetworkMetrics UClimbingNetworkOptimizer::GetCurrentNetworkMetrics() const
{
    return CurrentMetrics;
}

void UClimbingNetworkOptimizer::LogNetworkPerformance()
{
    UE_LOG(LogNet, Log, TEXT("Network Performance: Bandwidth=%.1fKBps, Latency=%.1fms, Connections=%d, PacketLoss=%.2f%%"),
           CurrentMetrics.BandwidthUsageKBps,
           CurrentMetrics.LatencyMs,
           CurrentMetrics.ActiveConnections,
           CurrentMetrics.PacketLossPercent * 100.0f);
}

void UClimbingNetworkOptimizer::OptimizeRopeReplication(UAdvancedRopeComponent* Rope)
{
    if (!IsValid(Rope))
        return;

    // Register rope for tracking if not already tracked
    if (!TrackedRopes.Contains(Rope))
    {
        TrackedRopes.Add(Rope);
    }

    // Determine which clients should receive rope updates
    for (auto& ConnectionPair : ConnectionProfiles)
    {
        APlayerController* PC = ConnectionPair.Key;
        if (!ShouldReplicateRopeToClient(Rope, PC))
        {
            // Skip replication for this client
            continue;
        }

        // Calculate appropriate update rate for this rope and client
        float UpdateRate = CalculateUpdateRate(Rope->GetOwner(), PC);
        
        // Apply quantized rope state for bandwidth efficiency
        uint8 RopeID = static_cast<uint8>(TrackedRopes.IndexOfByKey(Rope));
        uint8 OwnerID = 0; // Would be determined from rope owner
        FQuantizedRopeState QuantizedState = QuantizeRopeState(Rope, RopeID, OwnerID);

        // Queue for transmission (in real implementation, this would use Unreal's replication system)
    }
}

bool UClimbingNetworkOptimizer::ShouldReplicateRopeToClient(UAdvancedRopeComponent* Rope, APlayerController* Client) const
{
    if (!IsValid(Rope) || !IsValid(Client) || !Client->GetPawn())
        return false;

    // Don't replicate if client is too far away
    float Distance = GetNetworkDistance(Rope->GetOwner(), Client);
    if (Distance > UpdatePolicy.LowDistance * 1.5f)
        return false;

    // Always replicate if client is interacting with the rope
    if (UAdvancedClimbingComponent* ClimbingComp = Client->GetPawn()->FindComponentByClass<UAdvancedClimbingComponent>())
    {
        // Check if player is using this rope
        // This would require specific implementation in the climbing component
    }

    return true;
}

bool UClimbingNetworkOptimizer::ValidateMovement(ACharacter* Player, const FVector& OldPos, const FVector& NewPos, float DeltaTime) const
{
    if (!IsValid(Player) || DeltaTime <= 0.0f)
        return false;

    float Distance = FVector::Dist(OldPos, NewPos);
    float MaxSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
    float MaxDistance = MaxSpeed * DeltaTime * 1.1f; // 10% tolerance

    if (Distance > MaxDistance)
    {
        UE_LOG(LogNet, Warning, TEXT("Suspicious movement detected for %s: %.1f units in %.3fs (max: %.1f)"),
               *Player->GetName(), Distance, DeltaTime, MaxDistance);
        return false;
    }

    return true;
}

// Static helper function implementations
FQuantizedPlayerState FQuantizedPlayerState::FromPlayerState(const FVector& Location, const FRotator& Rotation, 
                                                           const FVector& Velocity, uint8 Grip, uint8 Stamina)
{
    FQuantizedPlayerState State;
    
    // Quantize location to centimeter precision
    State.LocationX = static_cast<uint16>(FMath::Clamp(Location.X + 32767.0f, 0.0f, 65535.0f));
    State.LocationY = static_cast<uint16>(FMath::Clamp(Location.Y + 32767.0f, 0.0f, 65535.0f));
    State.LocationZ = static_cast<uint16>(FMath::Clamp(Location.Z, 0.0f, 65535.0f));
    
    // Quantize rotation to ~1.4 degree precision
    State.RotationYaw = static_cast<uint8>(Rotation.Yaw / 1.4f);
    State.RotationPitch = static_cast<uint8>((Rotation.Pitch + 90.0f) / 1.4f);
    
    // Quantize velocity magnitude
    State.VelocityMagnitude = static_cast<uint8>(FMath::Clamp(Velocity.Size() / 10.0f, 0.0f, 255.0f));
    
    State.GripState = Grip;
    State.StaminaLevel = Stamina;
    
    return State;
}

FQuantizedRopeState FQuantizedRopeState::FromRopeComponent(const UAdvancedRopeComponent* Rope, uint8 RopeID, uint8 OwnerID)
{
    FQuantizedRopeState State;
    State.RopeID = RopeID;
    State.OwnerPlayerID = OwnerID;
    
    if (!IsValid(Rope) || !IsValid(Rope->CableComponent))
        return State;
    
    State.NumSegments = static_cast<uint8>(Rope->CableComponent->NumSegments);
    State.TensionLevel = 128; // Placeholder for tension calculation
    
    // Delta compress rope segment positions
    // This is a simplified version - full implementation would store previous state
    State.SegmentPositions.Reserve(State.NumSegments * 3);
    
    return State;
}

void UClimbingNetworkOptimizer::UpdateNetworkStatistics(float DeltaTime)
{
    // Update current metrics
    CurrentMetrics.ActiveConnections = ConnectionProfiles.Num();
    CurrentMetrics.BandwidthUsageKBps = TotalBytesThisSecond / 1024.0f;
    CurrentMetrics.PacketsPerSecond = PacketsThisSecond;

    // Calculate average latency across all connections
    float TotalLatency = 0.0f;
    float TotalPacketLoss = 0.0f;
    int32 ValidConnections = 0;

    for (const auto& ConnectionPair : ConnectionProfiles)
    {
        const FConnectionProfile& Profile = ConnectionPair.Value;
        TotalLatency += Profile.AverageLatencyMs;
        TotalPacketLoss += Profile.PacketLossRate;
        ValidConnections++;
    }

    if (ValidConnections > 0)
    {
        CurrentMetrics.LatencyMs = TotalLatency / ValidConnections;
        CurrentMetrics.PacketLossPercent = TotalPacketLoss / ValidConnections;
    }

    // Update stats
    SET_DWORD_STAT(STAT_BytesPerSecond, static_cast<uint32>(TotalBytesThisSecond));
    SET_DWORD_STAT(STAT_PacketsPerSecond, PacketsThisSecond);

    // Reset per-second counters
    TotalBytesThisSecond = 0.0f;
    PacketsThisSecond = 0;

    // Update history
    BandwidthHistory.Add(CurrentMetrics.BandwidthUsageKBps);
    LatencyHistory.Add(CurrentMetrics.LatencyMs);

    if (BandwidthHistory.Num() > MaxHistorySize)
    {
        BandwidthHistory.RemoveAt(0);
    }
    if (LatencyHistory.Num() > MaxHistorySize)
    {
        LatencyHistory.RemoveAt(0);
    }
}

bool UClimbingNetworkOptimizer::IsActorRelevantToPlayer(AActor* Actor, APlayerController* Player) const
{
    if (!IsValid(Actor) || !IsValid(Player))
        return false;

    // Basic relevance check - distance and visibility
    float Distance = GetNetworkDistance(Actor, Player);
    return Distance <= UpdatePolicy.LowDistance * 2.0f; // Extended range for relevance
}

float UClimbingNetworkOptimizer::GetNetworkDistance(AActor* Actor, APlayerController* Player) const
{
    if (!IsValid(Actor) || !IsValid(Player) || !Player->GetPawn())
        return MAX_FLT;

    return FVector::Dist(Actor->GetActorLocation(), Player->GetPawn()->GetActorLocation());
}

void UClimbingNetworkOptimizer::AnalyzeConnectionQuality(FConnectionProfile& Profile)
{
    // Analyze recent performance to classify connection quality
    if (Profile.LatencyHistory.Num() < 5)
        return;

    // Calculate latency variance
    float AvgLatency = 0.0f;
    for (float Latency : Profile.LatencyHistory)
    {
        AvgLatency += Latency;
    }
    AvgLatency /= Profile.LatencyHistory.Num();

    float Variance = 0.0f;
    for (float Latency : Profile.LatencyHistory)
    {
        Variance += FMath::Pow(Latency - AvgLatency, 2.0f);
    }
    Variance /= Profile.LatencyHistory.Num();

    // Classify connection stability
    Profile.bIsUnreliableConnection = (AvgLatency > MaxAcceptableLatency) || 
                                     (Variance > 1000.0f) || // High jitter
                                     (Profile.PacketLossRate > MaxAcceptablePacketLoss);
}

ENetworkCompressionLevel UClimbingNetworkOptimizer::GetOptimalCompressionLevel(const FConnectionProfile& Profile) const
{
    if (Profile.bIsUnreliableConnection)
    {
        return ENetworkCompressionLevel::Maximum;
    }
    else if (Profile.AverageLatencyMs > 100.0f)
    {
        return ENetworkCompressionLevel::Heavy;
    }
    else if (Profile.CurrentBandwidthKBps > BandwidthBudget.TotalBudgetKBps * 0.8f)
    {
        return ENetworkCompressionLevel::Medium;
    }
    else
    {
        return ENetworkCompressionLevel::Light;
    }
}

// Placeholder implementations for remaining functions
void UClimbingNetworkOptimizer::OptimizePlayerReplication(APlayerController* PC, float DeltaTime) {}
void UClimbingNetworkOptimizer::OptimizeRopeReplication(float DeltaTime) {}
void UClimbingNetworkOptimizer::OptimizeToolReplication(float DeltaTime) {}
void UClimbingNetworkOptimizer::BatchUpdateRopeStates(float DeltaTime) {}
void UClimbingNetworkOptimizer::SaveNetworkTelemetry(const FString& Filename) {}
bool UClimbingNetworkOptimizer::ValidateRopeInteraction(UAdvancedRopeComponent* Rope, ACharacter* Player, const FVector& InteractionPoint) const { return true; }