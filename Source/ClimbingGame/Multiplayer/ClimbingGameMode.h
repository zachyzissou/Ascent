#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "ClimbingGameMode.generated.h"

class AClimbingPlayerState;
class AClimbingGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerJoined, AClimbingPlayerState*, NewPlayer, int32, PlayerCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerLeft, AClimbingPlayerState*, LeavingPlayer, int32, PlayerCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCooperativeAction, AClimbingPlayerState*, Helper, AClimbingPlayerState*, Assisted);

/**
 * Game mode for ClimbingGame's cooperative multiplayer sessions
 * Handles up to 4 players, session management, and cooperative mechanics
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AClimbingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AClimbingGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual bool ReadyToEndMatch_Implementation() override;

public:
	// Session settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	int32 MinPlayersToStart = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	bool bAllowSpectators = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	float SessionTimeLimit = 3600.0f; // 1 hour

	// Cooperative mechanics settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative")
	float BelayAssistanceRange = 1000.0f; // 10 meters

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative")
	float ToolSharingRange = 500.0f; // 5 meters

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooperative")
	bool bAllowRopeSharing = true;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerJoined OnPlayerJoined;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerLeft OnPlayerLeft;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCooperativeAction OnCooperativeAction;

	// Player management
	UFUNCTION(BlueprintCallable, Category = "Players")
	int32 GetCurrentPlayerCount() const;

	UFUNCTION(BlueprintCallable, Category = "Players")
	TArray<AClimbingPlayerState*> GetAllClimbingPlayerStates() const;

	UFUNCTION(BlueprintCallable, Category = "Players")
	bool CanAcceptNewPlayer() const;

	// Cooperative actions
	UFUNCTION(BlueprintCallable, Category = "Cooperative")
	bool RequestBelayAssistance(AClimbingPlayerState* Helper, AClimbingPlayerState* NeedingHelp);

	UFUNCTION(BlueprintCallable, Category = "Cooperative")
	bool RequestToolShare(AClimbingPlayerState* Giver, AClimbingPlayerState* Receiver, class UClimbingToolBase* Tool);

	UFUNCTION(BlueprintCallable, Category = "Cooperative")
	void NotifyCooperativeAction(AClimbingPlayerState* Helper, AClimbingPlayerState* Assisted);

	// Session management
	UFUNCTION(BlueprintCallable, Category = "Session")
	void StartClimbingSession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void EndClimbingSession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	float GetSessionTimeRemaining() const;

protected:
	// Internal state
	UPROPERTY()
	float SessionStartTime = 0.0f;

	UPROPERTY()
	bool bSessionActive = false;

	// Player tracking
	UPROPERTY()
	TArray<AClimbingPlayerState*> ConnectedPlayers;

	// Session validation
	bool ValidatePlayerConnection(APlayerController* NewPlayer);
	void SetupPlayerForSession(APlayerController* Player);
	void CleanupPlayerFromSession(AController* LeavingPlayer);

	// Cooperative mechanics validation
	bool ValidateBelayRequest(AClimbingPlayerState* Helper, AClimbingPlayerState* NeedingHelp) const;
	bool ValidateToolShare(AClimbingPlayerState* Giver, AClimbingPlayerState* Receiver, class UClimbingToolBase* Tool) const;
};