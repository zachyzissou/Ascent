#include "ClimbingGameState.h"
#include "ClimbingPlayerState.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AClimbingGameState::AClimbingGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    
    // Initialize default values
    SessionState = EClimbingSessionState::WaitingForPlayers;
    SessionTimeRemaining = 0.0f;
    ConnectedPlayerCount = 0;
    MaxPlayers = 4;
    ActiveBelayPartnerships = 0;
    ToolSharesThisSession = 0;
    
    SessionDuration = 3600.0f; // 1 hour
}

void AClimbingGameState::BeginPlay()
{
    Super::BeginPlay();
    
    SessionStartTime = GetWorld()->GetTimeSeconds();
    SessionTimeRemaining = SessionDuration;
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Session started"));
}

void AClimbingGameState::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (HasAuthority())
    {
        UpdateSessionTimer();
        CleanupExpiredActions();
        ValidateActiveRopes();
    }
}

void AClimbingGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AClimbingGameState, SessionState);
    DOREPLIFETIME(AClimbingGameState, SessionTimeRemaining);
    DOREPLIFETIME(AClimbingGameState, ConnectedPlayerCount);
    DOREPLIFETIME(AClimbingGameState, MaxPlayers);
    DOREPLIFETIME(AClimbingGameState, RecentCooperativeActions);
    DOREPLIFETIME(AClimbingGameState, ActiveBelayPartnerships);
    DOREPLIFETIME(AClimbingGameState, ToolSharesThisSession);
    DOREPLIFETIME(AClimbingGameState, SharedRopeAnchors);
    DOREPLIFETIME(AClimbingGameState, ActiveRopes);
}

void AClimbingGameState::SetSessionState(EClimbingSessionState NewState)
{
    if (HasAuthority())
    {
        EClimbingSessionState OldState = SessionState;
        SessionState = NewState;
        
        MulticastSessionStateChanged(NewState);
        
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Session state changed from %d to %d"), 
               (int32)OldState, (int32)NewState);
    }
}

bool AClimbingGameState::CanStartClimbing() const
{
    return SessionState == EClimbingSessionState::PreparationPhase && 
           ConnectedPlayerCount >= 1 && 
           SessionTimeRemaining > 0.0f;
}

float AClimbingGameState::GetSessionProgress() const
{
    if (SessionDuration <= 0.0f)
        return 1.0f;
        
    return FMath::Clamp((SessionDuration - SessionTimeRemaining) / SessionDuration, 0.0f, 1.0f);
}

FString AClimbingGameState::GetSessionStateString() const
{
    switch (SessionState)
    {
        case EClimbingSessionState::WaitingForPlayers:
            return TEXT("Waiting for Players");
        case EClimbingSessionState::PreparationPhase:
            return TEXT("Preparation Phase");
        case EClimbingSessionState::ClimbingActive:
            return TEXT("Climbing Active");
        case EClimbingSessionState::SessionEnding:
            return TEXT("Session Ending");
        case EClimbingSessionState::SessionCompleted:
            return TEXT("Session Completed");
        default:
            return TEXT("Unknown");
    }
}

TArray<AClimbingPlayerState*> AClimbingGameState::GetAllClimbingPlayers() const
{
    TArray<AClimbingPlayerState*> ClimbingPlayers;
    
    for (APlayerState* PlayerState : PlayerArray)
    {
        if (AClimbingPlayerState* ClimbingPlayer = Cast<AClimbingPlayerState>(PlayerState))
        {
            ClimbingPlayers.Add(ClimbingPlayer);
        }
    }
    
    return ClimbingPlayers;
}

AClimbingPlayerState* AClimbingGameState::GetPlayerByName(const FString& PlayerName) const
{
    for (APlayerState* PlayerState : PlayerArray)
    {
        if (AClimbingPlayerState* ClimbingPlayer = Cast<AClimbingPlayerState>(PlayerState))
        {
            if (ClimbingPlayer->GetPlayerName() == PlayerName)
            {
                return ClimbingPlayer;
            }
        }
    }
    
    return nullptr;
}

bool AClimbingGameState::ArePlayersInRange(AClimbingPlayerState* Player1, AClimbingPlayerState* Player2, float Range) const
{
    if (!Player1 || !Player2)
        return false;
        
    APawn* Pawn1 = Player1->GetPawn();
    APawn* Pawn2 = Player2->GetPawn();
    
    if (!Pawn1 || !Pawn2)
        return false;
        
    float Distance = FVector::Dist(Pawn1->GetActorLocation(), Pawn2->GetActorLocation());
    return Distance <= Range;
}

void AClimbingGameState::RecordCooperativeAction(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted, const FString& ActionType)
{
    if (!HasAuthority())
        return;
        
    if (!Helper || !Assisted)
        return;
    
    FCooperativeAction NewAction;
    NewAction.Helper = Helper;
    NewAction.Assisted = Assisted;
    NewAction.ActionType = ActionType;
    NewAction.Timestamp = GetWorld()->GetTimeSeconds();
    
    RecentCooperativeActions.Add(NewAction);
    
    // Keep history manageable
    if (RecentCooperativeActions.Num() > MAX_COOPERATIVE_ACTION_HISTORY)
    {
        RecentCooperativeActions.RemoveAt(0);
    }
    
    MulticastCooperativeActionRecorded(NewAction);
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Cooperative action recorded - %s helped %s (%s)"), 
           *Helper->GetPlayerName(), *Assisted->GetPlayerName(), *ActionType);
}

void AClimbingGameState::IncrementBelayPartnerships()
{
    if (HasAuthority())
    {
        ActiveBelayPartnerships++;
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Active belay partnerships: %d"), ActiveBelayPartnerships);
    }
}

void AClimbingGameState::DecrementBelayPartnerships()
{
    if (HasAuthority())
    {
        ActiveBelayPartnerships = FMath::Max(0, ActiveBelayPartnerships - 1);
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Active belay partnerships: %d"), ActiveBelayPartnerships);
    }
}

void AClimbingGameState::IncrementToolShares()
{
    if (HasAuthority())
    {
        ToolSharesThisSession++;
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Tool shares this session: %d"), ToolSharesThisSession);
    }
}

TArray<FCooperativeAction> AClimbingGameState::GetCooperativeActionsForPlayer(AClimbingPlayerState* Player) const
{
    TArray<FCooperativeAction> PlayerActions;
    
    if (!Player)
        return PlayerActions;
    
    for (const FCooperativeAction& Action : RecentCooperativeActions)
    {
        if (Action.Helper == Player || Action.Assisted == Player)
        {
            PlayerActions.Add(Action);
        }
    }
    
    return PlayerActions;
}

void AClimbingGameState::RegisterSharedRopeAnchor(AActor* Anchor)
{
    if (!HasAuthority() || !Anchor)
        return;
        
    if (!SharedRopeAnchors.Contains(Anchor))
    {
        SharedRopeAnchors.Add(Anchor);
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Registered shared rope anchor"));
    }
}

void AClimbingGameState::UnregisterSharedRopeAnchor(AActor* Anchor)
{
    if (!HasAuthority() || !Anchor)
        return;
        
    SharedRopeAnchors.Remove(Anchor);
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Unregistered shared rope anchor"));
}

void AClimbingGameState::RegisterActiveRope(AAdvancedRopeComponent* Rope)
{
    if (!HasAuthority() || !Rope)
        return;
        
    if (!ActiveRopes.Contains(Rope))
    {
        ActiveRopes.Add(Rope);
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Registered active rope"));
    }
}

void AClimbingGameState::UnregisterActiveRope(AAdvancedRopeComponent* Rope)
{
    if (!HasAuthority() || !Rope)
        return;
        
    ActiveRopes.Remove(Rope);
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Unregistered active rope"));
}

TArray<AActor*> AClimbingGameState::GetNearbyRopeAnchors(const FVector& Location, float Range) const
{
    TArray<AActor*> NearbyAnchors;
    
    for (AActor* Anchor : SharedRopeAnchors)
    {
        if (Anchor && FVector::Dist(Anchor->GetActorLocation(), Location) <= Range)
        {
            NearbyAnchors.Add(Anchor);
        }
    }
    
    return NearbyAnchors;
}

void AClimbingGameState::MulticastSessionStateChanged_Implementation(EClimbingSessionState NewState)
{
    OnRep_SessionState();
}

void AClimbingGameState::MulticastCooperativeActionRecorded_Implementation(const FCooperativeAction& Action)
{
    OnRep_RecentCooperativeActions();
}

void AClimbingGameState::OnRep_SessionState()
{
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameState: Session state replicated - %s"), *GetSessionStateString());
}

void AClimbingGameState::OnRep_SessionTimeRemaining()
{
    // Update UI or other systems that depend on time remaining
}

void AClimbingGameState::OnRep_ConnectedPlayerCount()
{
    // Update UI or trigger events based on player count changes
}

void AClimbingGameState::OnRep_RecentCooperativeActions()
{
    // Handle UI updates for cooperative actions
}

void AClimbingGameState::UpdateSessionTimer()
{
    if (SessionState == EClimbingSessionState::ClimbingActive)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float ElapsedTime = CurrentTime - SessionStartTime;
        SessionTimeRemaining = FMath::Max(0.0f, SessionDuration - ElapsedTime);
        
        if (SessionTimeRemaining <= 0.0f && SessionState != EClimbingSessionState::SessionEnding)
        {
            SetSessionState(EClimbingSessionState::SessionEnding);
        }
    }
}

void AClimbingGameState::CleanupExpiredActions()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    RecentCooperativeActions.RemoveAll([CurrentTime](const FCooperativeAction& Action)
    {
        return (CurrentTime - Action.Timestamp) > COOPERATIVE_ACTION_HISTORY_DURATION;
    });
}

void AClimbingGameState::ValidateActiveRopes()
{
    // Remove null or invalid rope references
    ActiveRopes.RemoveAll([](const AAdvancedRopeComponent* Rope)
    {
        return !IsValid(Rope);
    });
    
    // Remove null or invalid anchor references
    SharedRopeAnchors.RemoveAll([](const AActor* Anchor)
    {
        return !IsValid(Anchor);
    });
}