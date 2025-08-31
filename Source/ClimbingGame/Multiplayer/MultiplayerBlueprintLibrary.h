#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "NetworkedClimbingCharacter.h"
#include "ClimbingSessionManager.h"
#include "CooperativeInventory.h"
#include "MultiplayerNetworkOptimizer.h"
#include "MultiplayerBlueprintLibrary.generated.h"

class UClimbingVoiceChat;
class UNetworkedRopeComponent;

USTRUCT(BlueprintType)
struct FClimbingMultiplayerConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session Config")
    FClimbingSessionSettings SessionSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    ENetworkOptimizationMode OptimizationMode = ENetworkOptimizationMode::Balanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Config")
    float TargetBandwidth = 128.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Config")
    FVoiceChatSettings VoiceSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation Config")
    bool bEnableToolSharing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation Config")
    bool bEnableRopeSharing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooperation Config")
    bool bEnableEmergencyMode = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Config")
    bool bUsePhysicsOptimization = true;

    FClimbingMultiplayerConfig()
    {
        OptimizationMode = ENetworkOptimizationMode::Balanced;
        TargetBandwidth = 128.0f;
        bEnableToolSharing = true;
        bEnableRopeSharing = true;
        bEnableEmergencyMode = true;
        bUsePhysicsOptimization = true;
    }
};

USTRUCT(BlueprintType)
struct FCooperativeClimbingEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Helper = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AClimbingPlayerState* Assisted = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FString ActionType;

    UPROPERTY(BlueprintReadOnly)
    FVector ActionLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float ActionTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bWasSuccessful = false;

    FCooperativeClimbingEvent()
    {
        Helper = nullptr;
        Assisted = nullptr;
        ActionType = TEXT("");
        ActionLocation = FVector::ZeroVector;
        ActionTime = 0.0f;
        bWasSuccessful = false;
    }
};

/**
 * Blueprint function library providing easy access to multiplayer climbing features
 * Simplifies common multiplayer operations for Blueprint developers
 */
UCLASS()
class CLIMBINGGAME_API UMultiplayerBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // === SESSION MANAGEMENT ===

    /** Create a new climbing session with the specified settings */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Session", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static bool CreateClimbingSession(const UObject* WorldContextObject, const FClimbingSessionSettings& Settings);

    /** Join an existing climbing session by ID */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Session", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static bool JoinClimbingSession(const UObject* WorldContextObject, const FString& SessionId, const FString& Password = TEXT(""));

    /** Leave the current climbing session */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Session", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static bool LeaveClimbingSession(const UObject* WorldContextObject);

    /** Search for available climbing sessions */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Session", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static bool SearchForSessions(const UObject* WorldContextObject, const FClimbingSessionSettings& SearchCriteria);

    /** Get the current session status */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Session", meta = (WorldContext = "WorldContextObject"))
    static ESessionStatus GetSessionStatus(const UObject* WorldContextObject);

    /** Get all players in the current session */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Session", meta = (WorldContext = "WorldContextObject"))
    static TArray<AClimbingPlayerState*> GetSessionPlayers(const UObject* WorldContextObject);

    // === COOPERATIVE ACTIONS ===

    /** Request belay assistance from another player */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Cooperation", meta = (WorldContext = "WorldContextObject"))
    static bool RequestBelayAssistance(const UObject* WorldContextObject, AClimbingPlayerState* Helper, AClimbingPlayerState* Climber);

    /** Share a rope with another player */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Cooperation", meta = (WorldContext = "WorldContextObject"))
    static bool ShareRope(const UObject* WorldContextObject, AClimbingPlayerState* TargetPlayer, UNetworkedRopeComponent* Rope);

    /** Share a tool with another player */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Cooperation", meta = (WorldContext = "WorldContextObject"))
    static bool ShareTool(const UObject* WorldContextObject, AClimbingPlayerState* TargetPlayer, UClimbingToolBase* Tool, EToolShareType ShareType = EToolShareType::Temporary);

    /** Call for emergency help */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Cooperation", meta = (WorldContext = "WorldContextObject"))
    static void CallForEmergencyHelp(const UObject* WorldContextObject, const FString& EmergencyType, const FVector& Location);

    /** Perform a cooperative climbing maneuver */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Cooperation", meta = (WorldContext = "WorldContextObject"))
    static bool PerformCooperativeMantle(const UObject* WorldContextObject, AClimbingPlayerState* Partner);

    // === VOICE COMMUNICATION ===

    /** Start voice transmission on a specific channel */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Voice", meta = (WorldContext = "WorldContextObject"))
    static bool StartVoiceTransmission(const UObject* WorldContextObject, EVoiceChatChannel Channel = EVoiceChatChannel::Proximity);

    /** Stop voice transmission */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Voice", meta = (WorldContext = "WorldContextObject"))
    static bool StopVoiceTransmission(const UObject* WorldContextObject);

    /** Send a climbing signal to other players */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Voice", meta = (WorldContext = "WorldContextObject"))
    static bool SendClimbingSignal(const UObject* WorldContextObject, const FString& SignalType, AClimbingPlayerState* TargetPlayer = nullptr);

    /** Get players within voice chat range */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Voice", meta = (WorldContext = "WorldContextObject"))
    static TArray<AClimbingPlayerState*> GetPlayersInVoiceRange(const UObject* WorldContextObject, EVoiceChatChannel Channel = EVoiceChatChannel::Proximity);

    // === NETWORK OPTIMIZATION ===

    /** Set the network optimization mode */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Network", meta = (WorldContext = "WorldContextObject"))
    static void SetNetworkOptimizationMode(const UObject* WorldContextObject, ENetworkOptimizationMode Mode);

    /** Get current network statistics */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Network", meta = (WorldContext = "WorldContextObject"))
    static FNetworkStats GetNetworkStats(const UObject* WorldContextObject);

    /** Optimize network settings for a specific player */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Network", meta = (WorldContext = "WorldContextObject"))
    static void OptimizeForPlayer(const UObject* WorldContextObject, AClimbingPlayerState* Player);

    /** Enable or disable physics prediction */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Network", meta = (WorldContext = "WorldContextObject"))
    static void SetPhysicsPrediction(const UObject* WorldContextObject, bool bEnable);

    // === UTILITY FUNCTIONS ===

    /** Get the networked climbing character for a player */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Utility", meta = (WorldContext = "WorldContextObject"))
    static ANetworkedClimbingCharacter* GetNetworkedClimbingCharacter(const UObject* WorldContextObject, AClimbingPlayerState* PlayerState);

    /** Check if two players are within cooperation range */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Utility", meta = (WorldContext = "WorldContextObject"))
    static bool ArePlayersInCooperationRange(const UObject* WorldContextObject, AClimbingPlayerState* Player1, AClimbingPlayerState* Player2, float Range = 1000.0f);

    /** Get the distance between two players */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Utility", meta = (WorldContext = "WorldContextObject"))
    static float GetDistanceBetweenPlayers(const UObject* WorldContextObject, AClimbingPlayerState* Player1, AClimbingPlayerState* Player2);

    /** Check if a player is currently climbing */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Utility", meta = (WorldContext = "WorldContextObject"))
    static bool IsPlayerClimbing(const UObject* WorldContextObject, AClimbingPlayerState* Player);

    /** Get all shared ropes in the world */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Utility", meta = (WorldContext = "WorldContextObject"))
    static TArray<UNetworkedRopeComponent*> GetSharedRopes(const UObject* WorldContextObject);

    /** Get all tools shared by a player */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Utility", meta = (WorldContext = "WorldContextObject"))
    static TArray<UClimbingToolBase*> GetSharedTools(const UObject* WorldContextObject, AClimbingPlayerState* Player);

    // === ADVANCED FEATURES ===

    /** Setup a complete multiplayer climbing environment */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Advanced", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static bool SetupMultiplayerEnvironment(const UObject* WorldContextObject, const FClimbingMultiplayerConfig& Config);

    /** Create a team inventory pool */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Advanced", meta = (WorldContext = "WorldContextObject"))
    static bool CreateTeamInventoryPool(const UObject* WorldContextObject, const TArray<AClimbingPlayerState*>& TeamMembers, const FString& PoolName = TEXT(""));

    /** Get recommended tools for a climbing route */
    UFUNCTION(BlueprintPure, Category = "Climbing Multiplayer|Advanced", meta = (WorldContext = "WorldContextObject"))
    static TArray<UClimbingToolBase*> GetRecommendedToolsForRoute(const UObject* WorldContextObject, const TArray<FVector>& RoutePoints);

    /** Calculate optimal tool distribution among team members */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Advanced", meta = (WorldContext = "WorldContextObject"))
    static bool CalculateOptimalToolDistribution(const UObject* WorldContextObject, const TArray<AClimbingPlayerState*>& TeamMembers, TArray<FString>& OutSuggestions);

    /** Record a cooperative climbing event for analytics */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Advanced", meta = (WorldContext = "WorldContextObject"))
    static void RecordCooperativeEvent(const UObject* WorldContextObject, const FCooperativeClimbingEvent& Event);

    // === DEBUG AND TESTING ===

    /** Simulate network latency for testing */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Debug", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static void SimulateNetworkLatency(const UObject* WorldContextObject, float Latency, float PacketLoss = 0.0f);

    /** Enable debug visualization for network optimization */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Debug", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static void EnableNetworkDebugVisualization(const UObject* WorldContextObject, bool bEnable);

    /** Log current multiplayer state for debugging */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Debug", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static void LogMultiplayerState(const UObject* WorldContextObject);

    /** Test all cooperative systems */
    UFUNCTION(BlueprintCallable, Category = "Climbing Multiplayer|Debug", CallInEditor = true, meta = (WorldContext = "WorldContextObject"))
    static bool TestCooperativeSystems(const UObject* WorldContextObject, TArray<FString>& OutResults);

protected:
    // Helper functions
    static AClimbingSessionManager* GetSessionManager(const UObject* WorldContextObject);
    static UMultiplayerNetworkOptimizer* GetNetworkOptimizer(const UObject* WorldContextObject);
    static UCooperativeInventory* GetCooperativeInventory(const UObject* WorldContextObject, AClimbingPlayerState* Player);
    static UClimbingVoiceChat* GetVoiceChat(const UObject* WorldContextObject, AClimbingPlayerState* Player);

    // Validation functions
    static bool ValidateWorldContext(const UObject* WorldContextObject);
    static bool ValidatePlayerState(AClimbingPlayerState* PlayerState);
    static bool ValidateNetworkConnection(const UObject* WorldContextObject);

    // Utility functions
    static void LogMultiplayerEvent(const FString& Event, const FString& Details = TEXT(""));
    static FString GetPlayerDisplayName(AClimbingPlayerState* PlayerState);
    static FVector GetPlayerLocation(AClimbingPlayerState* PlayerState);
};