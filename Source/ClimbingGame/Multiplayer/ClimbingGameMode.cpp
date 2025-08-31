#include "ClimbingGameMode.h"
#include "ClimbingGameState.h"
#include "ClimbingPlayerState.h"
#include "../Tools/ClimbingToolBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AClimbingGameMode::AClimbingGameMode()
{
    // Set default classes
    GameStateClass = AClimbingGameState::StaticClass();
    PlayerStateClass = AClimbingPlayerState::StaticClass();
    
    // Session settings
    MaxPlayers = 4;
    MinPlayersToStart = 1;
    bAllowSpectators = true;
    SessionTimeLimit = 3600.0f; // 1 hour
    
    // Cooperative settings
    BelayAssistanceRange = 1000.0f;
    ToolSharingRange = 500.0f;
    bAllowRopeSharing = true;
}

void AClimbingGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    SessionStartTime = GetWorld()->GetTimeSeconds();
    bSessionActive = true;
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Session started with max %d players"), MaxPlayers);
}

void AClimbingGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    if (!ValidatePlayerConnection(NewPlayer))
    {
        UE_LOG(LogTemp, Warning, TEXT("ClimbingGameMode: Player connection validation failed"));
        return;
    }
    
    AClimbingPlayerState* ClimbingPlayerState = Cast<AClimbingPlayerState>(NewPlayer->PlayerState);
    if (ClimbingPlayerState)
    {
        ConnectedPlayers.Add(ClimbingPlayerState);
        SetupPlayerForSession(NewPlayer);
        
        // Broadcast player joined event
        OnPlayerJoined.Broadcast(ClimbingPlayerState, GetCurrentPlayerCount());
        
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Player %s joined (%d/%d players)"), 
               *ClimbingPlayerState->GetPlayerName(), GetCurrentPlayerCount(), MaxPlayers);
    }
}

void AClimbingGameMode::Logout(AController* Exiting)
{
    if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
    {
        if (AClimbingPlayerState* ClimbingPlayerState = Cast<AClimbingPlayerState>(PlayerController->PlayerState))
        {
            ConnectedPlayers.Remove(ClimbingPlayerState);
            CleanupPlayerFromSession(Exiting);
            
            // Broadcast player left event
            OnPlayerLeft.Broadcast(ClimbingPlayerState, GetCurrentPlayerCount());
            
            UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Player %s left (%d/%d players)"), 
                   *ClimbingPlayerState->GetPlayerName(), GetCurrentPlayerCount(), MaxPlayers);
        }
    }
    
    Super::Logout(Exiting);
}

bool AClimbingGameMode::ReadyToStartMatch_Implementation()
{
    int32 CurrentPlayers = GetCurrentPlayerCount();
    return CurrentPlayers >= MinPlayersToStart && bSessionActive;
}

bool AClimbingGameMode::ReadyToEndMatch_Implementation()
{
    if (!bSessionActive)
        return true;
        
    // End if time limit reached
    if (GetSessionTimeRemaining() <= 0.0f)
        return true;
        
    // End if no players remain
    if (GetCurrentPlayerCount() == 0)
        return true;
        
    return false;
}

int32 AClimbingGameMode::GetCurrentPlayerCount() const
{
    return ConnectedPlayers.Num();
}

TArray<AClimbingPlayerState*> AClimbingGameMode::GetAllClimbingPlayerStates() const
{
    return ConnectedPlayers;
}

bool AClimbingGameMode::CanAcceptNewPlayer() const
{
    return GetCurrentPlayerCount() < MaxPlayers && bSessionActive;
}

bool AClimbingGameMode::RequestBelayAssistance(AClimbingPlayerState* Helper, AClimbingPlayerState* NeedingHelp)
{
    if (!ValidateBelayRequest(Helper, NeedingHelp))
    {
        return false;
    }
    
    // Set up belay partnership
    Helper->SetBelayPartner(NeedingHelp);
    NeedingHelp->SetBelayPartner(Helper);
    
    NotifyCooperativeAction(Helper, NeedingHelp);
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Belay assistance established between %s and %s"), 
           *Helper->GetPlayerName(), *NeedingHelp->GetPlayerName());
    
    return true;
}

bool AClimbingGameMode::RequestToolShare(AClimbingPlayerState* Giver, AClimbingPlayerState* Receiver, UClimbingToolBase* Tool)
{
    if (!ValidateToolShare(Giver, Receiver, Tool))
    {
        return false;
    }
    
    // Transfer tool ownership
    if (Giver->TransferTool(Tool, Receiver))
    {
        NotifyCooperativeAction(Giver, Receiver);
        
        UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Tool %s shared from %s to %s"), 
               *Tool->GetToolName(), *Giver->GetPlayerName(), *Receiver->GetPlayerName());
        
        return true;
    }
    
    return false;
}

void AClimbingGameMode::NotifyCooperativeAction(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted)
{
    OnCooperativeAction.Broadcast(Helper, Assisted);
}

void AClimbingGameMode::StartClimbingSession()
{
    SessionStartTime = GetWorld()->GetTimeSeconds();
    bSessionActive = true;
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Climbing session started"));
}

void AClimbingGameMode::EndClimbingSession()
{
    bSessionActive = false;
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingGameMode: Climbing session ended"));
}

float AClimbingGameMode::GetSessionTimeRemaining() const
{
    if (!bSessionActive)
        return 0.0f;
        
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - SessionStartTime;
    return FMath::Max(0.0f, SessionTimeLimit - ElapsedTime);
}

bool AClimbingGameMode::ValidatePlayerConnection(APlayerController* NewPlayer)
{
    if (!NewPlayer)
        return false;
        
    if (!CanAcceptNewPlayer())
    {
        UE_LOG(LogTemp, Warning, TEXT("ClimbingGameMode: Cannot accept new player - session full or inactive"));
        return false;
    }
    
    return true;
}

void AClimbingGameMode::SetupPlayerForSession(APlayerController* Player)
{
    if (!Player)
        return;
        
    // Find suitable spawn point
    AActor* SpawnPoint = FindPlayerStart(Player);
    if (SpawnPoint)
    {
        FVector SpawnLocation = SpawnPoint->GetActorLocation();
        FRotator SpawnRotation = SpawnPoint->GetActorRotation();
        
        // Set player spawn location
        if (APawn* PlayerPawn = Player->GetPawn())
        {
            PlayerPawn->SetActorLocation(SpawnLocation);
            PlayerPawn->SetActorRotation(SpawnRotation);
        }
    }
}

void AClimbingGameMode::CleanupPlayerFromSession(AController* LeavingPlayer)
{
    if (APlayerController* PlayerController = Cast<APlayerController>(LeavingPlayer))
    {
        if (AClimbingPlayerState* PlayerState = Cast<AClimbingPlayerState>(PlayerController->PlayerState))
        {
            // Clean up belay partnerships
            if (AClimbingPlayerState* Partner = PlayerState->GetBelayPartner())
            {
                Partner->SetBelayPartner(nullptr);
                PlayerState->SetBelayPartner(nullptr);
            }
            
            // Return any shared tools
            PlayerState->ReturnAllSharedTools();
        }
    }
}

bool AClimbingGameMode::ValidateBelayRequest(AClimbingPlayerState* Helper, AClimbingPlayerState* NeedingHelp) const
{
    if (!Helper || !NeedingHelp)
        return false;
        
    if (Helper == NeedingHelp)
        return false;
        
    // Check if both players are in range
    if (APawn* HelperPawn = Helper->GetPawn())
    {
        if (APawn* HelpPawn = NeedingHelp->GetPawn())
        {
            float Distance = FVector::Dist(HelperPawn->GetActorLocation(), HelpPawn->GetActorLocation());
            if (Distance > BelayAssistanceRange)
            {
                return false;
            }
        }
    }
    
    // Check if helper is already belaying someone
    if (Helper->GetBelayPartner() != nullptr)
        return false;
        
    // Check if the person needing help already has a belay partner
    if (NeedingHelp->GetBelayPartner() != nullptr)
        return false;
    
    return true;
}

bool AClimbingGameMode::ValidateToolShare(AClimbingPlayerState* Giver, AClimbingPlayerState* Receiver, UClimbingToolBase* Tool) const
{
    if (!Giver || !Receiver || !Tool)
        return false;
        
    if (Giver == Receiver)
        return false;
        
    // Check if giver owns the tool
    if (!Giver->OwnsItem(Tool))
        return false;
        
    // Check range
    if (APawn* GiverPawn = Giver->GetPawn())
    {
        if (APawn* ReceiverPawn = Receiver->GetPawn())
        {
            float Distance = FVector::Dist(GiverPawn->GetActorLocation(), ReceiverPawn->GetActorLocation());
            if (Distance > ToolSharingRange)
            {
                return false;
            }
        }
    }
    
    // Check if receiver has inventory space
    if (!Receiver->CanAcceptTool(Tool))
        return false;
    
    return true;
}