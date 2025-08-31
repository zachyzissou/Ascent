#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ClimbingSessionManager.generated.h"

class AClimbingGameMode;
class AClimbingPlayerState;
class UClimbingVoiceChat;

UENUM(BlueprintType)
enum class EClimbingSessionType : uint8
{
    Practice,           // Solo or small group practice
    Cooperative,        // 2-4 player cooperation
    Competitive,        // Speed climbing or challenges
    Expedition,         // Long-form climbing with checkpoints
    Training           // Tutorial and skill building
};

UENUM(BlueprintType)
enum class ESessionDifficulty : uint8
{
    Beginner,          // 5.0-5.6 routes
    Intermediate,      // 5.7-5.10 routes
    Advanced,          // 5.11-5.12 routes
    Expert,            // 5.13+ routes
    Mixed             // Various difficulties
};

UENUM(BlueprintType)
enum class ESessionStatus : uint8
{
    Creating,
    WaitingForPlayers,
    PreparationPhase,
    Active,
    Paused,
    Ending,
    Completed,
    Failed
};

USTRUCT(BlueprintType)
struct FClimbingSessionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SessionName = TEXT("Climbing Session");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EClimbingSessionType SessionType = EClimbingSessionType::Cooperative;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESessionDifficulty Difficulty = ESessionDifficulty::Intermediate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxPlayers = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinPlayersToStart = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SessionTimeLimit = 3600.0f; // 1 hour

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowSpectators = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequireVoiceChat = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowLatejoin = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPrivateSession = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Password = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MapName = TEXT("/Game/Maps/ClimbingMap01");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> RequiredSkills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinPlayerRating = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxPlayerRating = 100.0f;

    FClimbingSessionSettings()
    {
        SessionName = TEXT("Climbing Session");
        SessionType = EClimbingSessionType::Cooperative;
        Difficulty = ESessionDifficulty::Intermediate;
        MaxPlayers = 4;
        MinPlayersToStart = 1;
        SessionTimeLimit = 3600.0f;
        bAllowSpectators = true;
        bRequireVoiceChat = false;
        bAllowLatejoin = true;
        bPrivateSession = false;
        Password = TEXT("");
        MapName = TEXT("/Game/Maps/ClimbingMap01");
        MinPlayerRating = 0.0f;
        MaxPlayerRating = 100.0f;
    }
};

USTRUCT(BlueprintType)
struct FSessionSearchResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString SessionId;

    UPROPERTY(BlueprintReadOnly)
    FString SessionName;

    UPROPERTY(BlueprintReadOnly)
    FString HostName;

    UPROPERTY(BlueprintReadOnly)
    EClimbingSessionType SessionType = EClimbingSessionType::Cooperative;

    UPROPERTY(BlueprintReadOnly)
    ESessionDifficulty Difficulty = ESessionDifficulty::Intermediate;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers = 4;

    UPROPERTY(BlueprintReadOnly)
    int32 Ping = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bHasPassword = false;

    UPROPERTY(BlueprintReadOnly)
    bool bAllowsLatejoin = true;

    UPROPERTY(BlueprintReadOnly)
    FString MapName;

    UPROPERTY(BlueprintReadOnly)
    float AveragePlayerRating = 0.0f;

    FSessionSearchResult()
    {
        SessionId = TEXT("");
        SessionName = TEXT("");
        HostName = TEXT("");
        SessionType = EClimbingSessionType::Cooperative;
        Difficulty = ESessionDifficulty::Intermediate;
        CurrentPlayers = 0;
        MaxPlayers = 4;
        Ping = 0;
        bHasPassword = false;
        bAllowsLatejoin = true;
        MapName = TEXT("");
        AveragePlayerRating = 0.0f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreated, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStarted, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionEnded, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionDestroyed, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoined, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionLeft, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionSearchCompleted, bool, bWasSuccessful, const TArray<FSessionSearchResult>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerJoinedSession, AClimbingPlayerState*, Player, int32, PlayerCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerLeftSession, AClimbingPlayerState*, Player, int32, PlayerCount);

/**
 * Manages multiplayer session creation, searching, joining, and lifecycle
 * Handles matchmaking and player coordination for climbing sessions
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AClimbingSessionManager : public AActor
{
    GENERATED_BODY()

public:
    AClimbingSessionManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

public:
    // Session management
    UFUNCTION(BlueprintCallable, Category = "Session Management")
    void CreateSession(const FClimbingSessionSettings& Settings);

    UFUNCTION(BlueprintCallable, Category = "Session Management")
    void DestroyCurrentSession();

    UFUNCTION(BlueprintCallable, Category = "Session Management")
    void StartSession();

    UFUNCTION(BlueprintCallable, Category = "Session Management")
    void EndSession();

    UFUNCTION(BlueprintCallable, Category = "Session Management")
    void LeaveSession();

    // Session search and joining
    UFUNCTION(BlueprintCallable, Category = "Session Search")
    void SearchForSessions(const FClimbingSessionSettings& SearchCriteria);

    UFUNCTION(BlueprintCallable, Category = "Session Search")
    void JoinSessionById(const FString& SessionId, const FString& Password = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Session Search")
    void JoinSession(const FSessionSearchResult& SessionResult, const FString& Password = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Session Search")
    void QuickMatch(const FClimbingSessionSettings& Preferences);

    // Session information
    UFUNCTION(BlueprintPure, Category = "Session Info")
    bool IsSessionActive() const;

    UFUNCTION(BlueprintPure, Category = "Session Info")
    bool IsSessionHost() const;

    UFUNCTION(BlueprintPure, Category = "Session Info")
    ESessionStatus GetSessionStatus() const;

    UFUNCTION(BlueprintPure, Category = "Session Info")
    FClimbingSessionSettings GetCurrentSessionSettings() const;

    UFUNCTION(BlueprintPure, Category = "Session Info")
    int32 GetCurrentPlayerCount() const;

    UFUNCTION(BlueprintPure, Category = "Session Info")
    TArray<AClimbingPlayerState*> GetSessionPlayers() const;

    UFUNCTION(BlueprintPure, Category = "Session Info")
    float GetSessionTimeRemaining() const;

    // Player management
    UFUNCTION(BlueprintCallable, Category = "Player Management")
    void KickPlayer(AClimbingPlayerState* Player, const FString& Reason = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Player Management")
    void BanPlayer(AClimbingPlayerState* Player, const FString& Reason = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Player Management")
    void PromoteToHost(AClimbingPlayerState* Player);

    UFUNCTION(BlueprintCallable, Category = "Player Management")
    bool CanPlayerJoin(AClimbingPlayerState* Player) const;

    // Session configuration
    UFUNCTION(BlueprintCallable, Category = "Session Config")
    void UpdateSessionSettings(const FClimbingSessionSettings& NewSettings);

    UFUNCTION(BlueprintCallable, Category = "Session Config")
    void SetSessionPassword(const FString& NewPassword);

    UFUNCTION(BlueprintCallable, Category = "Session Config")
    void SetMaxPlayers(int32 NewMaxPlayers);

    UFUNCTION(BlueprintCallable, Category = "Session Config")
    void ToggleSpectatorMode(bool bAllowSpectators);

    // Advanced matchmaking
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void CreateSkillBasedMatch(float PlayerRating, ESessionDifficulty PreferredDifficulty);

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void FindPartnersForCooperativeClimbing(const TArray<FString>& RequiredSkills);

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void JoinTrainingSession(ESessionDifficulty MaxDifficulty);

    // Session events
    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionCreated OnSessionCreated;

    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionStarted OnSessionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionEnded OnSessionEnded;

    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionDestroyed OnSessionDestroyed;

    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionJoined OnSessionJoined;

    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionLeft OnSessionLeft;

    UPROPERTY(BlueprintAssignable, Category = "Session Events")
    FOnSessionSearchCompleted OnSessionSearchCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnPlayerJoinedSession OnPlayerJoinedSession;

    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnPlayerLeftSession OnPlayerLeftSession;

protected:
    // Current session state
    UPROPERTY(BlueprintReadOnly, Category = "Session State")
    FClimbingSessionSettings CurrentSettings;

    UPROPERTY(BlueprintReadOnly, Category = "Session State")
    ESessionStatus SessionStatus = ESessionStatus::Creating;

    UPROPERTY(BlueprintReadOnly, Category = "Session State")
    bool bIsHost = false;

    UPROPERTY(BlueprintReadOnly, Category = "Session State")
    float SessionStartTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Session State")
    FString CurrentSessionId;

    // Online subsystem integration
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    FName SessionName = TEXT("ClimbingGameSession");

    // Session delegates
    FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
    FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
    FOnEndSessionCompleteDelegate OnEndSessionCompleteDelegate;
    FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
    FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
    FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;

    FDelegateHandle OnCreateSessionCompleteDelegateHandle;
    FDelegateHandle OnStartSessionCompleteDelegateHandle;
    FDelegateHandle OnEndSessionCompleteDelegateHandle;
    FDelegateHandle OnDestroySessionCompleteDelegateHandle;
    FDelegateHandle OnJoinSessionCompleteDelegateHandle;
    FDelegateHandle OnFindSessionsCompleteDelegateHandle;

    // Session management internals
    void InitializeOnlineSubsystem();
    void CleanupOnlineSubsystem();
    void SetupSessionDelegates();
    void ClearSessionDelegates();

    // Session callbacks
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnFindSessionsComplete(bool bWasSuccessful);

    // Session configuration helpers
    TSharedPtr<FOnlineSessionSettings> CreateSessionSettings(const FClimbingSessionSettings& Settings);
    bool ValidateSessionSettings(const FClimbingSessionSettings& Settings) const;
    FClimbingSessionSettings ExtractSessionSettings(const FOnlineSessionSettings& Settings) const;

    // Search and filtering
    TArray<FSessionSearchResult> FilterSearchResults(const TArray<FOnlineSessionSearchResult>& Results, const FClimbingSessionSettings& Criteria) const;
    bool DoesSessionMatchCriteria(const FOnlineSessionSearchResult& Result, const FClimbingSessionSettings& Criteria) const;
    FSessionSearchResult ConvertToSessionResult(const FOnlineSessionSearchResult& Result) const;

    // Player validation
    bool ValidatePlayerSkills(AClimbingPlayerState* Player, const TArray<FString>& RequiredSkills) const;
    bool ValidatePlayerRating(AClimbingPlayerState* Player, float MinRating, float MaxRating) const;
    float CalculateSessionSkillLevel() const;

    // Session monitoring
    void UpdateSessionStatus();
    void CheckSessionHealth();
    void HandleSessionTimeout();
    void MonitorPlayerConnections();

    // Matchmaking algorithms
    void FindBestMatchingSessions(const FClimbingSessionSettings& Preferences, TArray<FSessionSearchResult>& OutResults) const;
    float CalculateSessionCompatibility(const FSessionSearchResult& Session, const FClimbingSessionSettings& Preferences) const;
    void SortSessionsByCompatibility(TArray<FSessionSearchResult>& Sessions, const FClimbingSessionSettings& Preferences) const;

private:
    // Internal state
    TArray<AClimbingPlayerState*> SessionPlayers;
    TMap<FString, float> PlayerRatings;
    TArray<FString> BannedPlayers;

    // Search state
    bool bIsSearching = false;
    FClimbingSessionSettings LastSearchCriteria;
    float SearchStartTime = 0.0f;
    float MaxSearchTime = 30.0f; // 30 seconds

    // Session monitoring
    float LastStatusUpdate = 0.0f;
    float StatusUpdateInterval = 1.0f; // 1 second
    float LastHealthCheck = 0.0f;
    float HealthCheckInterval = 10.0f; // 10 seconds

    // Performance tracking
    float SessionCreationTime = 0.0f;
    int32 TotalSessionsCreated = 0;
    int32 SuccessfulJoins = 0;
    int32 FailedJoins = 0;

    // Helper functions
    void LogSessionEvent(const FString& Event, const FString& Details = TEXT("")) const;
    void UpdateSessionStatistics();
    void CleanupSessionData();
    bool IsNetworkAvailable() const;
};