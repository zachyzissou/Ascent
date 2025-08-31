#include "NetworkedRopeComponent.h"
#include "ClimbingPlayerState.h"
#include "ClimbingGameState.h"
#include "CooperativeSystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UNetworkedRopeComponent::UNetworkedRopeComponent()
{
    // Ensure this component replicates
    SetIsReplicatedByDefault(true);
    
    // Initialize networking settings
    NetworkPriority = ERopeNetworkPriority::Medium;
    NetworkUpdateRate = 10.0f;
    NetworkCullDistance = 10000.0f;
    bUseAdaptiveNetworking = true;
    
    // Initialize sharing state
    PrimaryUser = nullptr;
    bIsSharedRope = false;
    SharedRopeSlack = 0.0f;
    LastSynchronizationTime = 0.0f;
    
    // Initialize network state
    LastNetworkUpdateTime = 0.0f;
    NetworkUpdateInterval = 0.1f;
    NetworkImportanceScore = 0.5f;
    CurrentLODLevel = 0;
    bNetworkStateDirty = false;
    
    // Initialize tension monitoring
    bTensionMonitoringActive = false;
    TensionAlertThreshold = 0.8f;
    TensionCriticalThreshold = 0.95f;
    LastTensionCheck = 0.0f;
    TensionCheckInterval = 0.5f;
    
    // Initialize interpolation
    InterpolationAlpha = 0.0f;
    InterpolationSpeed = 5.0f;
    
    // Initialize cleanup settings
    InteractionHistoryDuration = 60.0f;
    MaxInteractionHistory = 20;
    
    // Initialize cache
    CachedNetworkImportance = 0.0f;
    LastImportanceCalculationTime = 0.0f;
}

void UNetworkedRopeComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize network settings based on configuration
    CalculateOptimalNetworkSettings();
    
    UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Initialized with priority %d"), (int32)NetworkPriority);
}

void UNetworkedRopeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (GetOwner()->HasAuthority())
    {
        UpdateNetworkState();
        UpdateTensionMonitoring(DeltaTime);
        CleanupExpiredInteractions();
        
        // Send network updates if needed
        if (ShouldSendNetworkUpdate())
        {
            MulticastSyncRopeState(CurrentNetworkState);
            LastNetworkUpdateTime = GetWorld()->GetTimeSeconds();
            bNetworkStateDirty = false;
        }
    }
    else
    {
        // Client-side interpolation
        InterpolateNetworkState(DeltaTime);
    }
    
    // Update shared rope physics on all machines
    if (bIsSharedRope)
    {
        UpdateSharedRopePhysics();
    }
}

void UNetworkedRopeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UNetworkedRopeComponent, SharedWithPlayers);
    DOREPLIFETIME(UNetworkedRopeComponent, PrimaryUser);
    DOREPLIFETIME(UNetworkedRopeComponent, bIsSharedRope);
    DOREPLIFETIME(UNetworkedRopeComponent, SharedRopeSlack);
    DOREPLIFETIME(UNetworkedRopeComponent, CurrentNetworkState);
    DOREPLIFETIME(UNetworkedRopeComponent, RecentInteractions);
    DOREPLIFETIME(UNetworkedRopeComponent, LastSynchronizationTime);
}

bool UNetworkedRopeComponent::ShareRope(AClimbingPlayerState* TargetPlayer)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerShareRope(TargetPlayer);
        return false;
    }
    
    if (!ValidateRopeShare(TargetPlayer))
        return false;
    
    if (SharedWithPlayers.Contains(TargetPlayer))
        return false; // Already shared
    
    SharedWithPlayers.Add(TargetPlayer);
    bIsSharedRope = true;
    
    // Set primary user if not already set
    if (!PrimaryUser)
    {
        // Find the owning player state
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
        {
            if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
            {
                PrimaryUser = Cast<AClimbingPlayerState>(PC->PlayerState);
            }
        }
    }
    
    // Notify cooperative system
    NotifyPlayersOfShare(TargetPlayer);
    
    // Register with game state
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        GameState->RegisterActiveRope(Cast<AAdvancedRopeComponent>(this));
    }
    
    MulticastRopeShared(TargetPlayer, PrimaryUser);
    
    UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Rope shared with %s"), *TargetPlayer->GetPlayerName());
    return true;
}

bool UNetworkedRopeComponent::AcceptRopeShare(AClimbingPlayerState* SharingPlayer)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerAcceptRopeShare(SharingPlayer);
        return false;
    }
    
    if (!SharingPlayer || PrimaryUser != SharingPlayer)
        return false;
    
    // This is called on the receiving player's rope component
    // The actual sharing happens on the primary user's rope component
    if (UNetworkedRopeComponent* SharingRope = SharingPlayer->GetPawn()->FindComponentByClass<UNetworkedRopeComponent>())
    {
        return SharingRope->ShareRope(Cast<AClimbingPlayerState>(GetOwner()->GetOwner()->GetComponentByClass<APlayerController>()->PlayerState));
    }
    
    return false;
}

bool UNetworkedRopeComponent::StopSharingRope(AClimbingPlayerState* Player)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerStopSharingRope(Player);
        return false;
    }
    
    bool bRemovedPlayer = false;
    
    if (Player)
    {
        bRemovedPlayer = SharedWithPlayers.Remove(Player) > 0;
        CleanupRopeShare(Player);
    }
    else
    {
        // Remove all shared players
        for (AClimbingPlayerState* SharedPlayer : SharedWithPlayers)
        {
            CleanupRopeShare(SharedPlayer);
        }
        SharedWithPlayers.Empty();
        bRemovedPlayer = true;
    }
    
    if (bRemovedPlayer)
    {
        bIsSharedRope = SharedWithPlayers.Num() > 0;
        
        if (!bIsSharedRope)
        {
            SharedRopeSlack = 0.0f;
        }
        
        UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Stopped sharing rope with %s"), 
               Player ? *Player->GetPlayerName() : TEXT("all players"));
    }
    
    return bRemovedPlayer;
}

bool UNetworkedRopeComponent::IsRopeSharedWith(AClimbingPlayerState* Player) const
{
    return Player && SharedWithPlayers.Contains(Player);
}

bool UNetworkedRopeComponent::AdjustSlack(float SlackDelta)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerAdjustSlack(SlackDelta);
        return false;
    }
    
    float NewSlack = FMath::Clamp(SharedRopeSlack + SlackDelta, 0.0f, Properties.Length * 0.1f);
    
    if (FMath::IsNearlyEqual(NewSlack, SharedRopeSlack, 0.01f))
        return false; // No significant change
    
    SharedRopeSlack = NewSlack;
    bNetworkStateDirty = true;
    
    // Update cable component
    if (CableComponent)
    {
        float CurrentLength = FVector::Dist(AttachmentLocationA, AttachmentLocationB);
        CableComponent->SetCableLength(CurrentLength + SharedRopeSlack);
    }
    
    // Find the player who made this adjustment
    AClimbingPlayerState* AdjustingPlayer = nullptr;
    if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
        {
            AdjustingPlayer = Cast<AClimbingPlayerState>(PC->PlayerState);
        }
    }
    
    MulticastSlackChanged(SharedRopeSlack, AdjustingPlayer);
    
    return true;
}

bool UNetworkedRopeComponent::SetSlack(float NewSlack)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerSetSlack(NewSlack);
        return false;
    }
    
    float ClampedSlack = FMath::Clamp(NewSlack, 0.0f, Properties.Length * 0.1f);
    
    if (FMath::IsNearlyEqual(ClampedSlack, SharedRopeSlack, 0.01f))
        return false;
    
    SharedRopeSlack = ClampedSlack;
    bNetworkStateDirty = true;
    
    // Update cable component
    if (CableComponent)
    {
        float CurrentLength = FVector::Dist(AttachmentLocationA, AttachmentLocationB);
        CableComponent->SetCableLength(CurrentLength + SharedRopeSlack);
    }
    
    return true;
}

bool UNetworkedRopeComponent::CanAdjustSlack(AClimbingPlayerState* Player) const
{
    if (!Player)
        return false;
    
    // Primary user can always adjust
    if (Player == PrimaryUser)
        return true;
    
    // Shared players can adjust if they're in range and trusted
    if (IsRopeSharedWith(Player))
    {
        float Distance = GetDistanceToPlayer(Player);
        return Distance <= 1000.0f; // 10 meter range for slack adjustment
    }
    
    return false;
}

void UNetworkedRopeComponent::StartTensionMonitoring(float AlertThreshold, float CriticalThreshold)
{
    if (!GetOwner()->HasAuthority())
        return;
    
    bTensionMonitoringActive = true;
    TensionAlertThreshold = FMath::Clamp(AlertThreshold, 0.1f, 1.0f);
    TensionCriticalThreshold = FMath::Clamp(CriticalThreshold, TensionAlertThreshold, 1.0f);
    
    UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Tension monitoring started (Alert: %.2f, Critical: %.2f)"), 
           TensionAlertThreshold, TensionCriticalThreshold);
}

void UNetworkedRopeComponent::StopTensionMonitoring()
{
    if (!GetOwner()->HasAuthority())
        return;
    
    bTensionMonitoringActive = false;
    UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Tension monitoring stopped"));
}

bool UNetworkedRopeComponent::IsTensionCritical() const
{
    return GetTensionAsPercentage() >= TensionCriticalThreshold;
}

float UNetworkedRopeComponent::GetTensionAsPercentage() const
{
    if (Properties.BreakingStrength <= 0.0f)
        return 0.0f;
    
    // Convert breaking strength from kN to Newtons
    float BreakingStrengthN = Properties.BreakingStrength * 1000.0f;
    return FMath::Clamp(PhysicsState.CurrentTension / BreakingStrengthN, 0.0f, 1.0f);
}

void UNetworkedRopeComponent::RequestFullSynchronization()
{
    if (!GetOwner()->HasAuthority())
    {
        ServerRequestFullSync();
        return;
    }
    
    UpdateNetworkState();
    MulticastSyncRopeState(CurrentNetworkState);
    LastSynchronizationTime = GetWorld()->GetTimeSeconds();
    
    UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Full synchronization requested"));
}

void UNetworkedRopeComponent::SetNetworkPriority(ERopeNetworkPriority Priority)
{
    NetworkPriority = Priority;
    UpdateNetworkPriority();
}

void UNetworkedRopeComponent::ForceNetworkUpdate()
{
    if (GetOwner()->HasAuthority())
    {
        UpdateNetworkState();
        MulticastSyncRopeState(CurrentNetworkState);
        LastNetworkUpdateTime = GetWorld()->GetTimeSeconds();
        bNetworkStateDirty = false;
    }
}

bool UNetworkedRopeComponent::IsNetworkDirty() const
{
    return bNetworkStateDirty;
}

void UNetworkedRopeComponent::RecordPlayerInteraction(AClimbingPlayerState* Player, const FString& InteractionType, const FVector& InteractionPoint)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerRecordInteraction(Player, InteractionType, InteractionPoint);
        return;
    }
    
    if (!Player || !IsValidInteractionType(InteractionType))
        return;
    
    FRopeInteraction NewInteraction;
    NewInteraction.Player = Player;
    NewInteraction.InteractionType = InteractionType;
    NewInteraction.InteractionPoint = InteractionPoint;
    NewInteraction.Timestamp = GetWorld()->GetTimeSeconds();
    
    RecentInteractions.Add(NewInteraction);
    
    // Keep history manageable
    if (RecentInteractions.Num() > MaxInteractionHistory)
    {
        RecentInteractions.RemoveAt(0);
    }
    
    UE_LOG(LogTemp, Log, TEXT("NetworkedRopeComponent: Recorded %s interaction by %s"), 
           *InteractionType, *Player->GetPlayerName());
}

FRopeInteraction UNetworkedRopeComponent::GetLastInteractionByPlayer(AClimbingPlayerState* Player) const
{
    if (!Player)
        return FRopeInteraction();
    
    // Search from most recent to oldest
    for (int32 i = RecentInteractions.Num() - 1; i >= 0; --i)
    {
        if (RecentInteractions[i].Player == Player)
        {
            return RecentInteractions[i];
        }
    }
    
    return FRopeInteraction();
}

void UNetworkedRopeComponent::OptimizeForViewers(const TArray<FVector>& ViewerLocations)
{
    if (ViewerLocations.Num() == 0)
    {
        SetLODLevel(3); // Lowest detail if no viewers
        return;
    }
    
    // Find closest viewer
    float ClosestDistance = FLT_MAX;
    FVector RopeCenter = (AttachmentLocationA + AttachmentLocationB) * 0.5f;
    
    for (const FVector& ViewerLocation : ViewerLocations)
    {
        float Distance = FVector::Dist(ViewerLocation, RopeCenter);
        ClosestDistance = FMath::Min(ClosestDistance, Distance);
    }
    
    // Set LOD based on distance
    if (ClosestDistance < 1000.0f) // < 10m
    {
        SetLODLevel(0); // Highest detail
    }
    else if (ClosestDistance < 3000.0f) // < 30m
    {
        SetLODLevel(1); // Medium detail
    }
    else if (ClosestDistance < 5000.0f) // < 50m
    {
        SetLODLevel(2); // Low detail
    }
    else
    {
        SetLODLevel(3); // Lowest detail
    }
}

void UNetworkedRopeComponent::SetLODLevel(int32 LODLevel)
{
    CurrentLODLevel = FMath::Clamp(LODLevel, 0, 3);
    
    if (CableComponent)
    {
        switch (CurrentLODLevel)
        {
            case 0: // High detail
                CableComponent->NumSegments = HighDetailSegments;
                CableComponent->NumSides = 8;
                NetworkUpdateInterval = 0.05f; // 20 fps
                break;
            case 1: // Medium detail
                CableComponent->NumSegments = HighDetailSegments / 2;
                CableComponent->NumSides = 6;
                NetworkUpdateInterval = 0.1f; // 10 fps
                break;
            case 2: // Low detail
                CableComponent->NumSegments = LowDetailSegments;
                CableComponent->NumSides = 4;
                NetworkUpdateInterval = 0.2f; // 5 fps
                break;
            case 3: // Lowest detail
                CableComponent->NumSegments = LowDetailSegments / 2;
                CableComponent->NumSides = 3;
                NetworkUpdateInterval = 0.5f; // 2 fps
                break;
        }
    }
}

int32 UNetworkedRopeComponent::GetCurrentLODLevel() const
{
    return CurrentLODLevel;
}

// Network RPC implementations
void UNetworkedRopeComponent::ServerShareRope_Implementation(AClimbingPlayerState* TargetPlayer)
{
    ShareRope(TargetPlayer);
}

bool UNetworkedRopeComponent::ServerShareRope_Validate(AClimbingPlayerState* TargetPlayer)
{
    return TargetPlayer != nullptr;
}

void UNetworkedRopeComponent::ServerAcceptRopeShare_Implementation(AClimbingPlayerState* SharingPlayer)
{
    AcceptRopeShare(SharingPlayer);
}

bool UNetworkedRopeComponent::ServerAcceptRopeShare_Validate(AClimbingPlayerState* SharingPlayer)
{
    return SharingPlayer != nullptr;
}

void UNetworkedRopeComponent::ServerStopSharingRope_Implementation(AClimbingPlayerState* Player)
{
    StopSharingRope(Player);
}

bool UNetworkedRopeComponent::ServerStopSharingRope_Validate(AClimbingPlayerState* Player)
{
    return true; // Player can be null to stop all sharing
}

void UNetworkedRopeComponent::ServerAdjustSlack_Implementation(float SlackDelta)
{
    AdjustSlack(SlackDelta);
}

bool UNetworkedRopeComponent::ServerAdjustSlack_Validate(float SlackDelta)
{
    return FMath::Abs(SlackDelta) <= 10.0f; // Max 10m adjustment per call
}

void UNetworkedRopeComponent::ServerSetSlack_Implementation(float NewSlack)
{
    SetSlack(NewSlack);
}

bool UNetworkedRopeComponent::ServerSetSlack_Validate(float NewSlack)
{
    return NewSlack >= 0.0f && NewSlack <= Properties.Length;
}

void UNetworkedRopeComponent::ServerRecordInteraction_Implementation(AClimbingPlayerState* Player, const FString& InteractionType, const FVector& InteractionPoint)
{
    RecordPlayerInteraction(Player, InteractionType, InteractionPoint);
}

bool UNetworkedRopeComponent::ServerRecordInteraction_Validate(AClimbingPlayerState* Player, const FString& InteractionType, const FVector& InteractionPoint)
{
    return Player != nullptr && !InteractionType.IsEmpty() && InteractionType.Len() <= 32;
}

void UNetworkedRopeComponent::ServerRequestFullSync_Implementation()
{
    RequestFullSynchronization();
}

bool UNetworkedRopeComponent::ServerRequestFullSync_Validate()
{
    return true;
}

void UNetworkedRopeComponent::MulticastRopeShared_Implementation(AClimbingPlayerState* SharedWith, AClimbingPlayerState* SharedBy)
{
    OnRopeShared.Broadcast(this, SharedWith, SharedBy);
}

void UNetworkedRopeComponent::MulticastSlackChanged_Implementation(float NewSlack, AClimbingPlayerState* AdjustedBy)
{
    OnRopeSlackChanged.Broadcast(this, NewSlack);
}

void UNetworkedRopeComponent::MulticastTensionAlert_Implementation(float TensionLevel, bool bCritical)
{
    OnRopeTensionAlert.Broadcast(this, TensionLevel, bCritical);
}

void UNetworkedRopeComponent::MulticastSyncRopeState_Implementation(const FRopeNetworkState& NetworkState)
{
    if (!GetOwner()->HasAuthority())
    {
        ProcessNetworkUpdate(NetworkState);
    }
}

void UNetworkedRopeComponent::ClientReceiveRopeUpdate_Implementation(const FRopeNetworkState& NetworkState)
{
    ProcessNetworkUpdate(NetworkState);
}

// Protected helper methods
void UNetworkedRopeComponent::UpdateNetworkState()
{
    CurrentNetworkState.StartPosition = AttachmentLocationA;
    CurrentNetworkState.EndPosition = AttachmentLocationB;
    CurrentNetworkState.Tension = PhysicsState.CurrentTension;
    CurrentNetworkState.Slack = SharedRopeSlack;
    CurrentNetworkState.State = PhysicsState.State;
    CurrentNetworkState.Timestamp = GetWorld()->GetTimeSeconds();
    
    // Check if state has changed significantly
    static const float PositionThreshold = 10.0f; // 10cm
    static const float TensionThreshold = 100.0f; // 100N
    static const float SlackThreshold = 0.1f; // 10cm
    
    if (!FVector::PointsAreNear(CurrentNetworkState.StartPosition, PreviousNetworkState.StartPosition, PositionThreshold) ||
        !FVector::PointsAreNear(CurrentNetworkState.EndPosition, PreviousNetworkState.EndPosition, PositionThreshold) ||
        FMath::Abs(CurrentNetworkState.Tension - PreviousNetworkState.Tension) > TensionThreshold ||
        FMath::Abs(CurrentNetworkState.Slack - PreviousNetworkState.Slack) > SlackThreshold ||
        CurrentNetworkState.State != PreviousNetworkState.State)
    {
        bNetworkStateDirty = true;
    }
}

void UNetworkedRopeComponent::ProcessNetworkUpdate(const FRopeNetworkState& NewState)
{
    TargetNetworkState = NewState;
    PreviousNetworkState = CurrentNetworkState;
    InterpolationAlpha = 0.0f;
}

bool UNetworkedRopeComponent::ShouldSendNetworkUpdate() const
{
    if (!bNetworkStateDirty)
        return false;
    
    float TimeSinceLastUpdate = GetWorld()->GetTimeSeconds() - LastNetworkUpdateTime;
    return TimeSinceLastUpdate >= NetworkUpdateInterval;
}

void UNetworkedRopeComponent::InterpolateNetworkState(float DeltaTime)
{
    if (InterpolationAlpha >= 1.0f)
        return;
    
    InterpolationAlpha = FMath::Min(1.0f, InterpolationAlpha + (DeltaTime * InterpolationSpeed));
    
    InterpolatePositions(InterpolationAlpha);
    InterpolateTension(InterpolationAlpha);
    InterpolateSlack(InterpolationAlpha);
}

bool UNetworkedRopeComponent::ValidateRopeShare(AClimbingPlayerState* TargetPlayer) const
{
    if (!TargetPlayer)
        return false;
    
    // Check distance
    float Distance = GetDistanceToPlayer(TargetPlayer);
    if (Distance > 2000.0f) // 20 meter max sharing distance
        return false;
    
    // Check if rope is in valid state for sharing
    if (PhysicsState.State == ERopeState::Broken || PhysicsState.State == ERopeState::Overloaded)
        return false;
    
    return true;
}

void UNetworkedRopeComponent::NotifyPlayersOfShare(AClimbingPlayerState* NewSharedPlayer)
{
    // Notify cooperative systems
    if (UCooperativeSystem* CoopSystem = NewSharedPlayer->GetPawn()->FindComponentByClass<UCooperativeSystem>())
    {
        CoopSystem->RecordCooperativeAction(PrimaryUser, NewSharedPlayer, TEXT("Rope Share"));
    }
}

void UNetworkedRopeComponent::CleanupRopeShare(AClimbingPlayerState* Player)
{
    // Any cleanup needed when a player stops sharing the rope
    if (Player && Player->GetPawn())
    {
        if (UCooperativeSystem* CoopSystem = Player->GetPawn()->FindComponentByClass<UCooperativeSystem>())
        {
            // Cleanup cooperative action records
        }
    }
}

void UNetworkedRopeComponent::UpdateSharedRopePhysics()
{
    if (!CableComponent)
        return;
    
    // Apply shared slack to cable length
    float BaseLength = FVector::Dist(AttachmentLocationA, AttachmentLocationB);
    CableComponent->SetCableLength(BaseLength + SharedRopeSlack);
    
    // Distribute tension among shared users if needed
    if (SharedWithPlayers.Num() > 0)
    {
        // Reduce individual load per user
        float LoadDistributionFactor = 1.0f / (SharedWithPlayers.Num() + 1);
        // Apply load distribution to physics calculations
    }
}

void UNetworkedRopeComponent::UpdateTensionMonitoring(float DeltaTime)
{
    if (!bTensionMonitoringActive)
        return;
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastTensionCheck < TensionCheckInterval)
        return;
    
    LastTensionCheck = CurrentTime;
    CheckTensionThresholds();
}

void UNetworkedRopeComponent::CheckTensionThresholds()
{
    float TensionPercentage = GetTensionAsPercentage();
    
    if (TensionPercentage >= TensionCriticalThreshold)
    {
        BroadcastTensionAlert(TensionPercentage, true);
    }
    else if (TensionPercentage >= TensionAlertThreshold)
    {
        BroadcastTensionAlert(TensionPercentage, false);
    }
}

void UNetworkedRopeComponent::BroadcastTensionAlert(float TensionLevel, bool bCritical)
{
    MulticastTensionAlert(TensionLevel, bCritical);
    
    UE_LOG(LogTemp, Warning, TEXT("NetworkedRopeComponent: %s tension alert - %.1f%% of breaking strength"), 
           bCritical ? TEXT("CRITICAL") : TEXT("High"), TensionLevel * 100.0f);
}

void UNetworkedRopeComponent::CleanupExpiredInteractions()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    RecentInteractions.RemoveAll([CurrentTime, this](const FRopeInteraction& Interaction)
    {
        return (CurrentTime - Interaction.Timestamp) > InteractionHistoryDuration ||
               !IsValid(Interaction.Player);
    });
}

bool UNetworkedRopeComponent::IsValidInteractionType(const FString& InteractionType) const
{
    static const TArray<FString> ValidTypes = {
        TEXT("Grab"), TEXT("Release"), TEXT("Adjust"), TEXT("Anchor"), 
        TEXT("Climb"), TEXT("Belay"), TEXT("Rappel")
    };
    
    return ValidTypes.Contains(InteractionType);
}

void UNetworkedRopeComponent::InterpolatePositions(float Alpha)
{
    AttachmentLocationA = FMath::Lerp(PreviousNetworkState.StartPosition, TargetNetworkState.StartPosition, Alpha);
    AttachmentLocationB = FMath::Lerp(PreviousNetworkState.EndPosition, TargetNetworkState.EndPosition, Alpha);
    
    // Update cable component positions
    if (CableComponent)
    {
        CableComponent->SetWorldLocation(AttachmentLocationA);
        CableComponent->SetAttachEndTo(nullptr, AttachmentLocationB);
    }
}

void UNetworkedRopeComponent::InterpolateTension(float Alpha)
{
    PhysicsState.CurrentTension = FMath::Lerp(PreviousNetworkState.Tension, TargetNetworkState.Tension, Alpha);
}

void UNetworkedRopeComponent::InterpolateSlack(float Alpha)
{
    SharedRopeSlack = FMath::Lerp(PreviousNetworkState.Slack, TargetNetworkState.Slack, Alpha);
}

void UNetworkedRopeComponent::CalculateOptimalNetworkSettings()
{
    AdjustUpdateRateBasedOnImportance();
}

bool UNetworkedRopeComponent::IsPlayerInRange(AClimbingPlayerState* Player, float Range) const
{
    return GetDistanceToPlayer(Player) <= Range;
}

float UNetworkedRopeComponent::GetDistanceToPlayer(AClimbingPlayerState* Player) const
{
    if (!Player || !Player->GetPawn())
        return FLT_MAX;
    
    FVector RopeCenter = (AttachmentLocationA + AttachmentLocationB) * 0.5f;
    return FVector::Dist(Player->GetPawn()->GetActorLocation(), RopeCenter);
}

void UNetworkedRopeComponent::UpdateNetworkPriority()
{
    switch (NetworkPriority)
    {
        case ERopeNetworkPriority::Low:
            NetworkUpdateInterval = 0.5f; // 2 fps
            break;
        case ERopeNetworkPriority::Medium:
            NetworkUpdateInterval = 0.1f; // 10 fps
            break;
        case ERopeNetworkPriority::High:
            NetworkUpdateInterval = 0.05f; // 20 fps
            break;
        case ERopeNetworkPriority::Critical:
            NetworkUpdateInterval = 0.033f; // 30 fps
            break;
    }
}

float UNetworkedRopeComponent::CalculateNetworkImportance() const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastImportanceCalculationTime < ImportanceCalculationInterval)
    {
        return CachedNetworkImportance;
    }
    
    float Importance = 0.0f;
    
    // Base importance from rope state
    switch (PhysicsState.State)
    {
        case ERopeState::Broken:
        case ERopeState::Overloaded:
            Importance += 1.0f;
            break;
        case ERopeState::Tensioned:
            Importance += 0.7f;
            break;
        case ERopeState::Deployed:
            Importance += 0.4f;
            break;
        case ERopeState::Coiled:
            Importance += 0.1f;
            break;
    }
    
    // Importance from tension level
    Importance += GetTensionAsPercentage();
    
    // Importance from sharing
    if (bIsSharedRope)
    {
        Importance += 0.3f + (SharedWithPlayers.Num() * 0.2f);
    }
    
    // Cache the result
    CachedNetworkImportance = FMath::Clamp(Importance, 0.0f, 2.0f);
    LastImportanceCalculationTime = CurrentTime;
    
    return CachedNetworkImportance;
}

void UNetworkedRopeComponent::AdjustUpdateRateBasedOnImportance()
{
    if (!bUseAdaptiveNetworking)
        return;
    
    float Importance = CalculateNetworkImportance();
    float BaseInterval = 0.1f; // 10 fps base
    
    NetworkUpdateInterval = BaseInterval / FMath::Max(0.5f, Importance);
    NetworkUpdateInterval = FMath::Clamp(NetworkUpdateInterval, 0.033f, 1.0f); // 30 fps max, 1 fps min
}

// Replication callbacks
void UNetworkedRopeComponent::OnRep_SharedWithPlayers()
{
    // Handle UI updates for sharing status
}

void UNetworkedRopeComponent::OnRep_CurrentNetworkState()
{
    ProcessNetworkUpdate(CurrentNetworkState);
}

void UNetworkedRopeComponent::OnRep_SharedRopeSlack()
{
    if (CableComponent)
    {
        float BaseLength = FVector::Dist(AttachmentLocationA, AttachmentLocationB);
        CableComponent->SetCableLength(BaseLength + SharedRopeSlack);
    }
}

void UNetworkedRopeComponent::OnRep_PhysicsState()
{
    // Handle physics state changes on clients
    UpdateRopeState();
}