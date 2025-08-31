#include "CooperativeSystem.h"
#include "ClimbingPlayerState.h"
#include "ClimbingGameState.h"
#include "../Tools/ClimbingToolBase.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UCooperativeSystem::UCooperativeSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    // Initialize default values
    MaxCooperationRange = 1000.0f;
    BelayAssistanceRange = 1500.0f;
    SpottingRange = 500.0f;
    RequestTimeout = 30.0f;
    
    CurrentSpotter = nullptr;
    bIsReceivingSpot = false;
    OwningPlayerState = nullptr;
}

void UCooperativeSystem::BeginPlay()
{
    Super::BeginPlay();
    
    OwningPlayerState = GetOwningClimbingPlayerState();
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: Initialized for player %s"), 
           OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"));
}

void UCooperativeSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (GetOwner()->HasAuthority())
    {
        CleanupExpiredRequests();
        UpdateBelayAssistances(DeltaTime);
    }
}

void UCooperativeSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UCooperativeSystem, ActiveRequests);
    DOREPLIFETIME(UCooperativeSystem, ActiveBelayAssistances);
    DOREPLIFETIME(UCooperativeSystem, CurrentSpotter);
    DOREPLIFETIME(UCooperativeSystem, bIsReceivingSpot);
}

bool UCooperativeSystem::RequestCooperativeAction(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerRequestCooperativeAction(Target, ActionType, Location);
        return false;
    }
    
    if (!ValidateCooperativeAction(Target, ActionType, Location))
        return false;
    
    // Check if request already exists
    int32 ExistingRequestIndex = FindRequestIndex(OwningPlayerState, Target, ActionType);
    if (ExistingRequestIndex != INDEX_NONE)
    {
        return false; // Request already pending
    }
    
    FCooperativeRequest NewRequest;
    NewRequest.Requester = OwningPlayerState;
    NewRequest.Target = Target;
    NewRequest.ActionType = ActionType;
    NewRequest.ActionLocation = Location;
    NewRequest.RequestTime = GetWorld()->GetTimeSeconds();
    NewRequest.ExpiryTime = RequestTimeout;
    NewRequest.bIsActive = true;
    
    ActiveRequests.Add(NewRequest);
    
    // Notify target player
    if (UCooperativeSystem* TargetCoopSystem = Target->GetPawn()->FindComponentByClass<UCooperativeSystem>())
    {
        TargetCoopSystem->OnCooperativeRequestReceived.Broadcast(OwningPlayerState, ActionType, Location);
    }
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s requested %s from %s"), 
           *OwningPlayerState->GetPlayerName(), 
           *UEnum::GetValueAsString(ActionType),
           *Target->GetPlayerName());
    
    return true;
}

bool UCooperativeSystem::RespondToCooperativeRequest(int32 RequestIndex, bool bAccept)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerRespondToRequest(RequestIndex, bAccept);
        return false;
    }
    
    if (!ActiveRequests.IsValidIndex(RequestIndex))
        return false;
    
    FCooperativeRequest& Request = ActiveRequests[RequestIndex];
    if (!Request.bIsActive || Request.Target != OwningPlayerState)
        return false;
    
    if (bAccept)
    {
        // Start the cooperative action
        bool bActionStarted = false;
        
        switch (Request.ActionType)
        {
            case ECooperativeActionType::Belay:
                bActionStarted = StartBelayAssistance(Request.Requester);
                break;
            case ECooperativeActionType::SpotClimb:
                bActionStarted = StartSpotting(Request.Requester);
                break;
            case ECooperativeActionType::ToolShare:
                // Tool sharing logic would go here
                bActionStarted = true;
                break;
            case ECooperativeActionType::EmergencyRescue:
                bActionStarted = RespondToEmergency(Request.Requester);
                break;
            default:
                bActionStarted = false;
                break;
        }
        
        if (bActionStarted)
        {
            MulticastCooperativeActionStarted(OwningPlayerState, Request.Requester, Request.ActionType);
            
            // Record in game state
            if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
            {
                GameState->RecordCooperativeAction(OwningPlayerState, Request.Requester, UEnum::GetValueAsString(Request.ActionType));
            }
        }
    }
    
    // Remove the request
    ActiveRequests.RemoveAt(RequestIndex);
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s %s request from %s"), 
           *OwningPlayerState->GetPlayerName(),
           bAccept ? TEXT("accepted") : TEXT("declined"),
           *Request.Requester->GetPlayerName());
    
    return true;
}

void UCooperativeSystem::CancelCooperativeRequest(int32 RequestIndex)
{
    if (!GetOwner()->HasAuthority())
        return;
    
    if (ActiveRequests.IsValidIndex(RequestIndex))
    {
        ActiveRequests.RemoveAt(RequestIndex);
    }
}

TArray<FCooperativeRequest> UCooperativeSystem::GetPendingRequests() const
{
    TArray<FCooperativeRequest> PendingRequests;
    
    for (const FCooperativeRequest& Request : ActiveRequests)
    {
        if (Request.bIsActive && Request.Target == OwningPlayerState)
        {
            PendingRequests.Add(Request);
        }
    }
    
    return PendingRequests;
}

TArray<FCooperativeRequest> UCooperativeSystem::GetRequestsForPlayer(AClimbingPlayerState* Player) const
{
    TArray<FCooperativeRequest> PlayerRequests;
    
    if (!Player)
        return PlayerRequests;
    
    for (const FCooperativeRequest& Request : ActiveRequests)
    {
        if (Request.bIsActive && (Request.Requester == Player || Request.Target == Player))
        {
            PlayerRequests.Add(Request);
        }
    }
    
    return PlayerRequests;
}

bool UCooperativeSystem::StartBelayAssistance(AClimbingPlayerState* Climber, AAdvancedRopeComponent* Rope)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerStartBelayAssistance(Climber, Rope);
        return false;
    }
    
    if (!Climber || Climber == OwningPlayerState)
        return false;
    
    // Check if already providing belay to this climber
    int32 ExistingIndex = FindBelayAssistanceIndex(Climber);
    if (ExistingIndex != INDEX_NONE)
        return false;
    
    // Validate range
    if (APawn* OwnerPawn = GetOwner()->GetOwner<APawn>())
    {
        if (APawn* ClimberPawn = Climber->GetPawn())
        {
            float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), ClimberPawn->GetActorLocation());
            if (Distance > BelayAssistanceRange)
                return false;
        }
    }
    
    FBelayAssistance NewAssistance;
    NewAssistance.Belayer = OwningPlayerState;
    NewAssistance.Climber = Climber;
    NewAssistance.SharedRope = Rope;
    NewAssistance.SlackAmount = 0.0f;
    NewAssistance.bIsLocked = false;
    NewAssistance.StartTime = GetWorld()->GetTimeSeconds();
    
    ActiveBelayAssistances.Add(NewAssistance);
    
    // Update player states
    if (OwningPlayerState)
    {
        OwningPlayerState->SetBelayPartner(Climber);
    }
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s started belay assistance for %s"), 
           *OwningPlayerState->GetPlayerName(), *Climber->GetPlayerName());
    
    return true;
}

bool UCooperativeSystem::StopBelayAssistance(AClimbingPlayerState* Climber)
{
    if (!GetOwner()->HasAuthority())
        return false;
    
    int32 AssistanceIndex = FindBelayAssistanceIndex(Climber);
    if (AssistanceIndex == INDEX_NONE)
        return false;
    
    FBelayAssistance& Assistance = ActiveBelayAssistances[AssistanceIndex];
    
    // Update player states
    if (Assistance.Belayer)
    {
        Assistance.Belayer->SetBelayPartner(nullptr);
    }
    
    if (Assistance.Climber)
    {
        Assistance.Climber->SetBelayPartner(nullptr);
    }
    
    ActiveBelayAssistances.RemoveAt(AssistanceIndex);
    
    MulticastCooperativeActionCompleted(OwningPlayerState, Climber, ECooperativeActionType::Belay);
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s stopped belay assistance for %s"), 
           *OwningPlayerState->GetPlayerName(), *Climber->GetPlayerName());
    
    return true;
}

bool UCooperativeSystem::AdjustRopeSlack(AClimbingPlayerState* Climber, float SlackDelta)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerAdjustRopeSlack(Climber, SlackDelta);
        return false;
    }
    
    int32 AssistanceIndex = FindBelayAssistanceIndex(Climber);
    if (AssistanceIndex == INDEX_NONE)
        return false;
    
    FBelayAssistance& Assistance = ActiveBelayAssistances[AssistanceIndex];
    if (Assistance.bIsLocked)
        return false;
    
    Assistance.SlackAmount = FMath::Clamp(Assistance.SlackAmount + SlackDelta, 0.0f, 10.0f);
    
    // Apply slack adjustment to rope
    if (Assistance.SharedRope)
    {
        Assistance.SharedRope->AdjustSlack(SlackDelta);
    }
    
    return true;
}

bool UCooperativeSystem::LockBelay(AClimbingPlayerState* Climber, bool bLock)
{
    if (!GetOwner()->HasAuthority())
        return false;
    
    int32 AssistanceIndex = FindBelayAssistanceIndex(Climber);
    if (AssistanceIndex == INDEX_NONE)
        return false;
    
    ActiveBelayAssistances[AssistanceIndex].bIsLocked = bLock;
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: Belay %s for %s"), 
           bLock ? TEXT("locked") : TEXT("unlocked"), *Climber->GetPlayerName());
    
    return true;
}

FBelayAssistance UCooperativeSystem::GetBelayAssistanceForClimber(AClimbingPlayerState* Climber) const
{
    int32 AssistanceIndex = FindBelayAssistanceIndex(Climber);
    if (AssistanceIndex != INDEX_NONE)
    {
        return ActiveBelayAssistances[AssistanceIndex];
    }
    
    return FBelayAssistance();
}

bool UCooperativeSystem::IsProvidingBelay() const
{
    for (const FBelayAssistance& Assistance : ActiveBelayAssistances)
    {
        if (Assistance.Belayer == OwningPlayerState)
            return true;
    }
    return false;
}

bool UCooperativeSystem::IsReceivingBelay() const
{
    for (const FBelayAssistance& Assistance : ActiveBelayAssistances)
    {
        if (Assistance.Climber == OwningPlayerState)
            return true;
    }
    return false;
}

bool UCooperativeSystem::StartSpotting(AClimbingPlayerState* Climber)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerStartSpotting(Climber);
        return false;
    }
    
    if (!CanProvideSpot(Climber))
        return false;
    
    CurrentSpotter = OwningPlayerState;
    
    // Set spotter on climber's system
    if (UCooperativeSystem* ClimberCoopSystem = Climber->GetPawn()->FindComponentByClass<UCooperativeSystem>())
    {
        ClimberCoopSystem->CurrentSpotter = OwningPlayerState;
        ClimberCoopSystem->bIsReceivingSpot = true;
    }
    
    MulticastCooperativeActionStarted(OwningPlayerState, Climber, ECooperativeActionType::SpotClimb);
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s started spotting %s"), 
           *OwningPlayerState->GetPlayerName(), *Climber->GetPlayerName());
    
    return true;
}

bool UCooperativeSystem::StopSpotting()
{
    if (!GetOwner()->HasAuthority())
        return false;
    
    if (!CurrentSpotter)
        return false;
    
    AClimbingPlayerState* SpottedClimber = nullptr;
    
    // Find the climber being spotted
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        for (AClimbingPlayerState* Player : GameState->GetAllClimbingPlayers())
        {
            if (UCooperativeSystem* PlayerCoopSystem = Player->GetPawn()->FindComponentByClass<UCooperativeSystem>())
            {
                if (PlayerCoopSystem->CurrentSpotter == OwningPlayerState)
                {
                    SpottedClimber = Player;
                    PlayerCoopSystem->CurrentSpotter = nullptr;
                    PlayerCoopSystem->bIsReceivingSpot = false;
                    break;
                }
            }
        }
    }
    
    CurrentSpotter = nullptr;
    
    if (SpottedClimber)
    {
        MulticastCooperativeActionCompleted(OwningPlayerState, SpottedClimber, ECooperativeActionType::SpotClimb);
    }
    
    return true;
}

bool UCooperativeSystem::ProvideCatchAssistance(AClimbingPlayerState* FallingClimber, const FVector& CatchLocation)
{
    if (!FallingClimber || !CurrentSpotter)
        return false;
    
    // Check if we're the spotter for this climber
    if (UCooperativeSystem* ClimberCoopSystem = FallingClimber->GetPawn()->FindComponentByClass<UCooperativeSystem>())
    {
        if (ClimberCoopSystem->CurrentSpotter != OwningPlayerState)
            return false;
    }
    
    // Validate catch location is within range
    if (APawn* OwnerPawn = GetOwner()->GetOwner<APawn>())
    {
        float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), CatchLocation);
        if (Distance > SpottingRange)
            return false;
    }
    
    // Record successful spot
    if (OwningPlayerState)
    {
        OwningPlayerState->IncrementBelayAssists(); // Using belay assists counter for spots too
    }
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s provided catch assistance for %s"), 
           *OwningPlayerState->GetPlayerName(), *FallingClimber->GetPlayerName());
    
    return true;
}

bool UCooperativeSystem::CanProvideSpot(AClimbingPlayerState* Climber) const
{
    if (!Climber || Climber == OwningPlayerState)
        return false;
    
    if (CurrentSpotter != nullptr)
        return false; // Already spotting someone
    
    // Check range
    if (APawn* OwnerPawn = GetOwner()->GetOwner<APawn>())
    {
        if (APawn* ClimberPawn = Climber->GetPawn())
        {
            float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), ClimberPawn->GetActorLocation());
            return Distance <= SpottingRange;
        }
    }
    
    return false;
}

bool UCooperativeSystem::CallForEmergencyAssistance(ECooperativeActionType EmergencyType, const FVector& Location)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerCallEmergency(EmergencyType, Location);
        return false;
    }
    
    // Broadcast emergency to all players
    MulticastEmergencyCall(OwningPlayerState, EmergencyType, Location);
    
    UE_LOG(LogTemp, Warning, TEXT("CooperativeSystem: %s called for emergency assistance (%s)"), 
           *OwningPlayerState->GetPlayerName(), *UEnum::GetValueAsString(EmergencyType));
    
    return true;
}

bool UCooperativeSystem::RespondToEmergency(AClimbingPlayerState* PlayerInNeed)
{
    if (!PlayerInNeed || PlayerInNeed == OwningPlayerState)
        return false;
    
    // Start emergency response - this could involve moving to the player's location
    // and providing assistance based on the emergency type
    
    UE_LOG(LogTemp, Log, TEXT("CooperativeSystem: %s responding to emergency for %s"), 
           *OwningPlayerState->GetPlayerName(), *PlayerInNeed->GetPlayerName());
    
    return true;
}

// Network RPC implementations
void UCooperativeSystem::ServerRequestCooperativeAction_Implementation(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location)
{
    RequestCooperativeAction(Target, ActionType, Location);
}

bool UCooperativeSystem::ServerRequestCooperativeAction_Validate(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location)
{
    return Target != nullptr && ActionType != ECooperativeActionType::None;
}

void UCooperativeSystem::ServerRespondToRequest_Implementation(int32 RequestIndex, bool bAccept)
{
    RespondToCooperativeRequest(RequestIndex, bAccept);
}

bool UCooperativeSystem::ServerRespondToRequest_Validate(int32 RequestIndex, bool bAccept)
{
    return RequestIndex >= 0;
}

void UCooperativeSystem::ServerStartBelayAssistance_Implementation(AClimbingPlayerState* Climber, AAdvancedRopeComponent* Rope)
{
    StartBelayAssistance(Climber, Rope);
}

bool UCooperativeSystem::ServerStartBelayAssistance_Validate(AClimbingPlayerState* Climber, AAdvancedRopeComponent* Rope)
{
    return Climber != nullptr;
}

void UCooperativeSystem::ServerAdjustRopeSlack_Implementation(AClimbingPlayerState* Climber, float SlackDelta)
{
    AdjustRopeSlack(Climber, SlackDelta);
}

bool UCooperativeSystem::ServerAdjustRopeSlack_Validate(AClimbingPlayerState* Climber, float SlackDelta)
{
    return Climber != nullptr && FMath::Abs(SlackDelta) <= 5.0f; // Max 5m adjustment per call
}

void UCooperativeSystem::ServerStartSpotting_Implementation(AClimbingPlayerState* Climber)
{
    StartSpotting(Climber);
}

bool UCooperativeSystem::ServerStartSpotting_Validate(AClimbingPlayerState* Climber)
{
    return Climber != nullptr;
}

void UCooperativeSystem::ServerCallEmergency_Implementation(ECooperativeActionType EmergencyType, const FVector& Location)
{
    CallForEmergencyAssistance(EmergencyType, Location);
}

bool UCooperativeSystem::ServerCallEmergency_Validate(ECooperativeActionType EmergencyType, const FVector& Location)
{
    return EmergencyType == ECooperativeActionType::EmergencyRescue;
}

void UCooperativeSystem::MulticastCooperativeActionStarted_Implementation(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted, ECooperativeActionType ActionType)
{
    OnCooperativeActionStarted.Broadcast(Helper, Assisted, ActionType);
}

void UCooperativeSystem::MulticastCooperativeActionCompleted_Implementation(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted, ECooperativeActionType ActionType)
{
    OnCooperativeActionCompleted.Broadcast(Helper, Assisted, ActionType);
}

void UCooperativeSystem::MulticastEmergencyCall_Implementation(AClimbingPlayerState* PlayerInNeed, ECooperativeActionType EmergencyType, const FVector& Location)
{
    // Handle emergency UI/notifications on all clients
    UE_LOG(LogTemp, Warning, TEXT("CooperativeSystem: Emergency call from %s"), *PlayerInNeed->GetPlayerName());
}

// Helper functions
AClimbingPlayerState* UCooperativeSystem::GetOwningClimbingPlayerState() const
{
    if (APawn* OwnerPawn = GetOwner()->GetOwner<APawn>())
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
        {
            return Cast<AClimbingPlayerState>(PC->PlayerState);
        }
    }
    return nullptr;
}

bool UCooperativeSystem::ValidateCooperativeAction(AClimbingPlayerState* Target, ECooperativeActionType ActionType, const FVector& Location) const
{
    if (!Target || !OwningPlayerState || Target == OwningPlayerState)
        return false;
    
    // Check range based on action type
    float RequiredRange = MaxCooperationRange;
    
    switch (ActionType)
    {
        case ECooperativeActionType::Belay:
            RequiredRange = BelayAssistanceRange;
            break;
        case ECooperativeActionType::SpotClimb:
            RequiredRange = SpottingRange;
            break;
        case ECooperativeActionType::ToolShare:
            RequiredRange = MaxCooperationRange;
            break;
        default:
            RequiredRange = MaxCooperationRange;
            break;
    }
    
    if (APawn* OwnerPawn = GetOwner()->GetOwner<APawn>())
    {
        if (APawn* TargetPawn = Target->GetPawn())
        {
            float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), TargetPawn->GetActorLocation());
            return Distance <= RequiredRange;
        }
    }
    
    return false;
}

bool UCooperativeSystem::IsInRange(const FVector& Location1, const FVector& Location2, float Range) const
{
    return FVector::Dist(Location1, Location2) <= Range;
}

void UCooperativeSystem::CleanupExpiredRequests()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    ActiveRequests.RemoveAll([CurrentTime](const FCooperativeRequest& Request)
    {
        return !Request.bIsActive || (CurrentTime - Request.RequestTime) > Request.ExpiryTime;
    });
}

void UCooperativeSystem::UpdateBelayAssistances(float DeltaTime)
{
    // Update any belay-specific logic here
    // For example, monitoring rope tension, distance between partners, etc.
}

int32 UCooperativeSystem::FindRequestIndex(AClimbingPlayerState* Requester, AClimbingPlayerState* Target, ECooperativeActionType ActionType) const
{
    for (int32 i = 0; i < ActiveRequests.Num(); ++i)
    {
        const FCooperativeRequest& Request = ActiveRequests[i];
        if (Request.Requester == Requester && Request.Target == Target && Request.ActionType == ActionType && Request.bIsActive)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UCooperativeSystem::FindBelayAssistanceIndex(AClimbingPlayerState* Climber) const
{
    for (int32 i = 0; i < ActiveBelayAssistances.Num(); ++i)
    {
        if (ActiveBelayAssistances[i].Climber == Climber)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void UCooperativeSystem::OnRep_ActiveRequests()
{
    // Handle UI updates for active requests
}

void UCooperativeSystem::OnRep_ActiveBelayAssistances()
{
    // Handle UI updates for belay assistances
}

void UCooperativeSystem::OnRep_CurrentSpotter()
{
    // Handle UI updates for spotting status
}