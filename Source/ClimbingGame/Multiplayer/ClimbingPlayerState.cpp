#include "ClimbingPlayerState.h"
#include "../Tools/ClimbingToolBase.h"
#include "ClimbingGameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AClimbingPlayerState::AClimbingPlayerState()
{
    bReplicates = true;
    
    // Initialize default values
    BelayPartner = nullptr;
    MaxInventorySlots = 20;
    MaxCarryWeight = 25.0f;
    CurrentCarryWeight = 0.0f;
    
    bIsClimbing = false;
    bIsProvingBelay = false;
    bIsReceivingBelay = false;
    CurrentStamina = 100.0f;
    MaxStamina = 100.0f;
    
    bVoiceChatEnabled = true;
    bRadioCommunicationActive = false;
    
    // Initialize inventory slots
    InventorySlots.SetNum(MaxInventorySlots);
}

void AClimbingPlayerState::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: Player %s initialized"), *GetPlayerName());
}

void AClimbingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AClimbingPlayerState, SessionStats);
    DOREPLIFETIME(AClimbingPlayerState, CareerStats);
    DOREPLIFETIME(AClimbingPlayerState, BelayPartner);
    DOREPLIFETIME(AClimbingPlayerState, TrustedPartners);
    DOREPLIFETIME(AClimbingPlayerState, InventorySlots);
    DOREPLIFETIME(AClimbingPlayerState, MaxInventorySlots);
    DOREPLIFETIME(AClimbingPlayerState, MaxCarryWeight);
    DOREPLIFETIME(AClimbingPlayerState, CurrentCarryWeight);
    DOREPLIFETIME(AClimbingPlayerState, bIsClimbing);
    DOREPLIFETIME(AClimbingPlayerState, bIsProvingBelay);
    DOREPLIFETIME(AClimbingPlayerState, bIsReceivingBelay);
    DOREPLIFETIME(AClimbingPlayerState, CurrentStamina);
    DOREPLIFETIME(AClimbingPlayerState, MaxStamina);
    DOREPLIFETIME(AClimbingPlayerState, bVoiceChatEnabled);
    DOREPLIFETIME(AClimbingPlayerState, bRadioCommunicationActive);
}

void AClimbingPlayerState::SetBelayPartner(AClimbingPlayerState* Partner)
{
    if (!HasAuthority())
        return;
    
    AClimbingPlayerState* OldPartner = BelayPartner;
    BelayPartner = Partner;
    
    // Update game state statistics
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        if (OldPartner && !Partner)
        {
            GameState->DecrementBelayPartnerships();
        }
        else if (!OldPartner && Partner)
        {
            GameState->IncrementBelayPartnerships();
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: %s set belay partner to %s"), 
           *GetPlayerName(), Partner ? *Partner->GetPlayerName() : TEXT("None"));
}

bool AClimbingPlayerState::CanProvideBelay() const
{
    return !bIsClimbing && !bIsProvingBelay && BelayPartner == nullptr;
}

bool AClimbingPlayerState::StartBelaying(AClimbingPlayerState* Climber)
{
    if (!HasAuthority())
    {
        ServerStartBelaying(Climber);
        return false;
    }
    
    if (!CanProvideBelay() || !Climber || Climber == this)
        return false;
    
    // Check if climber is available for belay
    if (Climber->BelayPartner != nullptr)
        return false;
    
    SetBelayPartner(Climber);
    Climber->SetBelayPartner(this);
    
    bIsProvingBelay = true;
    Climber->bIsReceivingBelay = true;
    
    // Record cooperative action
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        GameState->RecordCooperativeAction(this, Climber, TEXT("Belay"));
    }
    
    return true;
}

void AClimbingPlayerState::StopBelaying()
{
    if (!HasAuthority())
    {
        ServerStopBelaying();
        return;
    }
    
    if (BelayPartner)
    {
        BelayPartner->bIsReceivingBelay = false;
        BelayPartner->SetBelayPartner(nullptr);
    }
    
    bIsProvingBelay = false;
    SetBelayPartner(nullptr);
}

void AClimbingPlayerState::ServerStartBelaying_Implementation(AClimbingPlayerState* Climber)
{
    StartBelaying(Climber);
}

bool AClimbingPlayerState::ServerStartBelaying_Validate(AClimbingPlayerState* Climber)
{
    return Climber != nullptr && Climber != this;
}

void AClimbingPlayerState::ServerStopBelaying_Implementation()
{
    StopBelaying();
}

bool AClimbingPlayerState::ServerStopBelaying_Validate()
{
    return true;
}

void AClimbingPlayerState::AddTrustedPartner(AClimbingPlayerState* Partner)
{
    if (!HasAuthority() || !Partner || Partner == this)
        return;
    
    if (!TrustedPartners.Contains(Partner))
    {
        TrustedPartners.Add(Partner);
        UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: %s added %s as trusted partner"), 
               *GetPlayerName(), *Partner->GetPlayerName());
    }
}

void AClimbingPlayerState::RemoveTrustedPartner(AClimbingPlayerState* Partner)
{
    if (!HasAuthority() || !Partner)
        return;
    
    TrustedPartners.Remove(Partner);
    UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: %s removed %s as trusted partner"), 
           *GetPlayerName(), *Partner->GetPlayerName());
}

bool AClimbingPlayerState::IsTrustedPartner(AClimbingPlayerState* Partner) const
{
    return Partner && TrustedPartners.Contains(Partner);
}

bool AClimbingPlayerState::AddItem(UClimbingToolBase* Tool, int32 Quantity)
{
    if (!HasAuthority() || !Tool || Quantity <= 0)
        return false;
    
    if (!CanAcceptTool(Tool))
        return false;
    
    int32 ExistingSlot = FindInventorySlotWithTool(Tool);
    if (ExistingSlot != INDEX_NONE)
    {
        InventorySlots[ExistingSlot].Quantity += Quantity;
    }
    else
    {
        int32 EmptySlot = FindEmptyInventorySlot();
        if (EmptySlot != INDEX_NONE)
        {
            InventorySlots[EmptySlot].Tool = Tool;
            InventorySlots[EmptySlot].Quantity = Quantity;
            InventorySlots[EmptySlot].bIsShared = false;
            InventorySlots[EmptySlot].SharedFrom = nullptr;
        }
        else
        {
            return false; // No space
        }
    }
    
    UpdateCarryWeight();
    return true;
}

bool AClimbingPlayerState::RemoveItem(UClimbingToolBase* Tool, int32 Quantity)
{
    if (!HasAuthority() || !Tool || Quantity <= 0)
        return false;
    
    int32 SlotIndex = FindInventorySlotWithTool(Tool);
    if (SlotIndex == INDEX_NONE)
        return false;
    
    FInventorySlot& Slot = InventorySlots[SlotIndex];
    if (Slot.Quantity < Quantity)
        return false;
    
    Slot.Quantity -= Quantity;
    if (Slot.Quantity <= 0)
    {
        // Clear the slot
        Slot.Tool = nullptr;
        Slot.Quantity = 0;
        Slot.bIsShared = false;
        Slot.SharedFrom = nullptr;
    }
    
    UpdateCarryWeight();
    return true;
}

bool AClimbingPlayerState::OwnsItem(UClimbingToolBase* Tool) const
{
    return GetItemQuantity(Tool) > 0;
}

int32 AClimbingPlayerState::GetItemQuantity(UClimbingToolBase* Tool) const
{
    if (!Tool)
        return 0;
    
    int32 SlotIndex = FindInventorySlotWithTool(Tool);
    if (SlotIndex != INDEX_NONE)
    {
        return InventorySlots[SlotIndex].Quantity;
    }
    
    return 0;
}

bool AClimbingPlayerState::CanAcceptTool(UClimbingToolBase* Tool) const
{
    if (!Tool)
        return false;
    
    // Check weight capacity
    float ToolWeight = Tool->GetWeight();
    if (CurrentCarryWeight + ToolWeight > MaxCarryWeight)
        return false;
    
    // Check if we have an existing slot or an empty slot
    int32 ExistingSlot = FindInventorySlotWithTool(Tool);
    if (ExistingSlot != INDEX_NONE)
        return true;
    
    int32 EmptySlot = FindEmptyInventorySlot();
    return EmptySlot != INDEX_NONE;
}

float AClimbingPlayerState::GetRemainingCarryCapacity() const
{
    return FMath::Max(0.0f, MaxCarryWeight - CurrentCarryWeight);
}

bool AClimbingPlayerState::TransferTool(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient)
{
    if (!HasAuthority())
    {
        ServerTransferTool(Tool, Recipient);
        return false;
    }
    
    if (!ValidateToolTransfer(Tool, Recipient))
        return false;
    
    // Remove from this player
    if (!RemoveItem(Tool, 1))
        return false;
    
    // Add to recipient
    if (!Recipient->AddItem(Tool, 1))
    {
        // Failed to add to recipient, return to original owner
        AddItem(Tool, 1);
        return false;
    }
    
    // Update statistics
    IncrementToolsShared();
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        GameState->IncrementToolShares();
        GameState->RecordCooperativeAction(this, Recipient, TEXT("Tool Transfer"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: %s transferred %s to %s"), 
           *GetPlayerName(), *Tool->GetToolName(), *Recipient->GetPlayerName());
    
    return true;
}

bool AClimbingPlayerState::ShareTool(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient)
{
    if (!HasAuthority())
    {
        ServerShareTool(Tool, Recipient);
        return false;
    }
    
    if (!ValidateToolTransfer(Tool, Recipient))
        return false;
    
    // Find the tool in our inventory
    int32 SlotIndex = FindInventorySlotWithTool(Tool);
    if (SlotIndex == INDEX_NONE || InventorySlots[SlotIndex].Quantity <= 0)
        return false;
    
    // Add shared copy to recipient
    if (!Recipient->CanAcceptTool(Tool))
        return false;
    
    int32 RecipientSlot = Recipient->FindEmptyInventorySlot();
    if (RecipientSlot == INDEX_NONE)
        return false;
    
    Recipient->InventorySlots[RecipientSlot].Tool = Tool;
    Recipient->InventorySlots[RecipientSlot].Quantity = 1;
    Recipient->InventorySlots[RecipientSlot].bIsShared = true;
    Recipient->InventorySlots[RecipientSlot].SharedFrom = this;
    
    Recipient->UpdateCarryWeight();
    
    // Update statistics
    IncrementToolsShared();
    if (AClimbingGameState* GameState = GetWorld()->GetGameState<AClimbingGameState>())
    {
        GameState->IncrementToolShares();
        GameState->RecordCooperativeAction(this, Recipient, TEXT("Tool Share"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: %s shared %s with %s"), 
           *GetPlayerName(), *Tool->GetToolName(), *Recipient->GetPlayerName());
    
    return true;
}

void AClimbingPlayerState::ReturnSharedTool(UClimbingToolBase* Tool)
{
    if (!HasAuthority() || !Tool)
        return;
    
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        FInventorySlot& Slot = InventorySlots[i];
        if (Slot.Tool == Tool && Slot.bIsShared && Slot.SharedFrom)
        {
            // Remove from our inventory
            Slot.Tool = nullptr;
            Slot.Quantity = 0;
            Slot.bIsShared = false;
            Slot.SharedFrom = nullptr;
            
            UpdateCarryWeight();
            break;
        }
    }
}

void AClimbingPlayerState::ReturnAllSharedTools()
{
    if (!HasAuthority())
        return;
    
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        FInventorySlot& Slot = InventorySlots[i];
        if (Slot.bIsShared && Slot.SharedFrom)
        {
            Slot.Tool = nullptr;
            Slot.Quantity = 0;
            Slot.bIsShared = false;
            Slot.SharedFrom = nullptr;
        }
    }
    
    UpdateCarryWeight();
}

void AClimbingPlayerState::ServerTransferTool_Implementation(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient)
{
    TransferTool(Tool, Recipient);
}

bool AClimbingPlayerState::ServerTransferTool_Validate(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient)
{
    return Tool != nullptr && Recipient != nullptr && Recipient != this;
}

void AClimbingPlayerState::ServerShareTool_Implementation(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient)
{
    ShareTool(Tool, Recipient);
}

bool AClimbingPlayerState::ServerShareTool_Validate(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient)
{
    return Tool != nullptr && Recipient != nullptr && Recipient != this;
}

void AClimbingPlayerState::IncrementBelayAssists()
{
    if (HasAuthority())
    {
        SessionStats.SuccessfulBelayAssists++;
        CareerStats.SuccessfulBelayAssists++;
    }
}

void AClimbingPlayerState::IncrementToolsShared()
{
    if (HasAuthority())
    {
        SessionStats.ToolsShared++;
        CareerStats.ToolsShared++;
    }
}

void AClimbingPlayerState::AddClimbingTime(float DeltaTime)
{
    if (HasAuthority() && bIsClimbing)
    {
        SessionStats.TotalClimbingTime += DeltaTime;
        CareerStats.TotalClimbingTime += DeltaTime;
    }
}

void AClimbingPlayerState::AddDistanceClimbed(float Distance)
{
    if (HasAuthority())
    {
        SessionStats.DistanceClimbed += Distance;
        CareerStats.DistanceClimbed += Distance;
    }
}

void AClimbingPlayerState::IncrementRopesDeployed()
{
    if (HasAuthority())
    {
        SessionStats.RopesDeployed++;
        CareerStats.RopesDeployed++;
    }
}

void AClimbingPlayerState::SetClimbingStatus(bool bClimbing)
{
    if (HasAuthority())
    {
        bIsClimbing = bClimbing;
    }
}

void AClimbingPlayerState::SetStamina(float NewStamina)
{
    if (HasAuthority())
    {
        CurrentStamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
    }
}

float AClimbingPlayerState::GetStaminaPercentage() const
{
    return MaxStamina > 0.0f ? (CurrentStamina / MaxStamina) : 0.0f;
}

void AClimbingPlayerState::SetVoiceChatEnabled(bool bEnabled)
{
    if (!HasAuthority())
    {
        ServerSetVoiceChatEnabled(bEnabled);
        return;
    }
    
    bVoiceChatEnabled = bEnabled;
}

void AClimbingPlayerState::ToggleRadioCommunication()
{
    if (!HasAuthority())
    {
        ServerToggleRadioCommunication();
        return;
    }
    
    bRadioCommunicationActive = !bRadioCommunicationActive;
}

void AClimbingPlayerState::ServerSetVoiceChatEnabled_Implementation(bool bEnabled)
{
    SetVoiceChatEnabled(bEnabled);
}

bool AClimbingPlayerState::ServerSetVoiceChatEnabled_Validate(bool bEnabled)
{
    return true;
}

void AClimbingPlayerState::ServerToggleRadioCommunication_Implementation()
{
    ToggleRadioCommunication();
}

bool AClimbingPlayerState::ServerToggleRadioCommunication_Validate()
{
    return true;
}

void AClimbingPlayerState::OnRep_BelayPartner()
{
    UE_LOG(LogTemp, Log, TEXT("ClimbingPlayerState: Belay partner replicated for %s"), *GetPlayerName());
}

void AClimbingPlayerState::OnRep_InventorySlots()
{
    UpdateCarryWeight();
}

void AClimbingPlayerState::OnRep_SessionStats()
{
    // Handle UI updates for statistics
}

void AClimbingPlayerState::OnRep_PlayerStatus()
{
    // Handle status change effects
}

int32 AClimbingPlayerState::FindEmptyInventorySlot() const
{
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i].Tool == nullptr)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 AClimbingPlayerState::FindInventorySlotWithTool(UClimbingToolBase* Tool) const
{
    if (!Tool)
        return INDEX_NONE;
    
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i].Tool == Tool)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void AClimbingPlayerState::UpdateCarryWeight()
{
    float TotalWeight = 0.0f;
    
    for (const FInventorySlot& Slot : InventorySlots)
    {
        if (Slot.Tool && Slot.Quantity > 0)
        {
            TotalWeight += Slot.Tool->GetWeight() * Slot.Quantity;
        }
    }
    
    CurrentCarryWeight = TotalWeight;
}

bool AClimbingPlayerState::ValidateToolTransfer(UClimbingToolBase* Tool, AClimbingPlayerState* Recipient) const
{
    if (!Tool || !Recipient || Recipient == this)
        return false;
    
    // Check if we own the tool
    if (!OwnsItem(Tool))
        return false;
    
    // Check if recipient can accept the tool
    if (!Recipient->CanAcceptTool(Tool))
        return false;
    
    return true;
}