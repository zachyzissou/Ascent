#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/GameStateBase.h"
#include "EnvironmentalHazardManager.h"
#include "WindPhysicsSystem.h"
#include "WeatherEffectsSystem.h"
#include "GeologicalPhysicsSystem.h"
#include "ThermalPhysicsSystem.h"
#include "VisibilitySystem.h"
#include "EnvironmentalNetworkManager.generated.h"

UENUM(BlueprintType)
enum class ENetworkUpdatePriority : uint8
{
    Critical        UMETA(DisplayName = "Critical"),      // Life-threatening hazards
    High            UMETA(DisplayName = "High"),         // Significant gameplay impact
    Medium          UMETA(DisplayName = "Medium"),       // Moderate impact
    Low             UMETA(DisplayName = "Low"),          // Minor/cosmetic effects
    Background      UMETA(DisplayName = "Background")    // Ambient effects
};

UENUM(BlueprintType)
enum class ENetworkCompressionLevel : uint8
{
    None            UMETA(DisplayName = "None"),         // No compression
    Light           UMETA(DisplayName = "Light"),        // Minimal compression
    Medium          UMETA(DisplayName = "Medium"),       // Balanced compression
    Heavy           UMETA(DisplayName = "Heavy"),        // Maximum compression
    Adaptive        UMETA(DisplayName = "Adaptive")      // Adaptive based on bandwidth
};

USTRUCT(BlueprintType)
struct FNetworkHazardUpdate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    FString HazardID;

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    EGeologicalHazard HazardType;

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    FVector_NetQuantize100 Location; // Quantized to 1cm precision

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    float Intensity = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    float Duration = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    uint32 Timestamp = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Network Update")
    ENetworkUpdatePriority Priority = ENetworkUpdatePriority::Medium;
};

USTRUCT(BlueprintType)
struct FNetworkWeatherUpdate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    EWeatherType WeatherType = EWeatherType::Clear;

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    uint8 IntensityByte = 0; // Compressed intensity (0-255)

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    int16 Temperature = 200; // Temperature * 10 (20.0°C = 200)

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    uint8 Humidity = 128; // Humidity * 255

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    uint16 Precipitation = 0; // Precipitation * 100

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    uint8 VisibilityByte = 255; // Visibility * 255

    UPROPERTY(BlueprintReadWrite, Category = "Weather Update")
    uint32 Timestamp = 0;
};

USTRUCT(BlueprintType)
struct FNetworkWindUpdate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Wind Update")
    FVector_NetQuantizeNormal Direction = FVector(1.0f, 0.0f, 0.0f);

    UPROPERTY(BlueprintReadWrite, Category = "Wind Update")
    uint8 SpeedByte = 0; // Speed * 2.55 (0-100 m/s range)

    UPROPERTY(BlueprintReadWrite, Category = "Wind Update")
    uint8 GustsByte = 0; // Gusts * 5.1 (0-50 m/s range)

    UPROPERTY(BlueprintReadWrite, Category = "Wind Update")
    uint8 TurbulenceByte = 0; // Turbulence * 255

    UPROPERTY(BlueprintReadWrite, Category = "Wind Update")
    uint32 Timestamp = 0;
};

USTRUCT(BlueprintType)
struct FNetworkThermalUpdate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Update")
    FString ZoneID;

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Update")
    int16 Temperature = 200; // Temperature * 10

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Update")
    uint8 HeatGeneration = 0; // Heat generation scaled

    UPROPERTY(BlueprintReadWrite, Category = "Thermal Update")
    uint32 Timestamp = 0;
};

USTRUCT(BlueprintType)
struct FNetworkVisibilityUpdate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Update")
    EVisibilityHazardType HazardType = EVisibilityHazardType::None;

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Update")
    uint8 IntensityByte = 0; // Intensity * 255

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Update")
    uint16 VisualRange = 50000; // Visual range in cm

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Update")
    uint8 DensityByte = 0; // Density * 255

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Update")
    FColor Tint = FColor::White;

    UPROPERTY(BlueprintReadWrite, Category = "Visibility Update")
    uint32 Timestamp = 0;
};

USTRUCT(BlueprintType)
struct FNetworkBandwidthStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bandwidth Stats")
    float AvailableBandwidth = 0.0f; // bytes/second

    UPROPERTY(BlueprintReadOnly, Category = "Bandwidth Stats")
    float UsedBandwidth = 0.0f; // bytes/second

    UPROPERTY(BlueprintReadOnly, Category = "Bandwidth Stats")
    float PacketLoss = 0.0f; // 0-1

    UPROPERTY(BlueprintReadOnly, Category = "Bandwidth Stats")
    float Latency = 0.0f; // milliseconds

    UPROPERTY(BlueprintReadOnly, Category = "Bandwidth Stats")
    int32 QueuedUpdates = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bandwidth Stats")
    float CompressionRatio = 1.0f; // Actual size / original size
};

USTRUCT(BlueprintType)
struct FNetworkClientState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    FString ClientID;

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    FVector_NetQuantize100 ClientLocation;

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    ENetworkCompressionLevel PreferredCompression = ENetworkCompressionLevel::Medium;

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    float MaxUpdateFrequency = 10.0f; // Hz

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    float PriorityRadius = 5000.0f; // cm - radius for high priority updates

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    FNetworkBandwidthStats BandwidthStats;

    UPROPERTY(BlueprintReadWrite, Category = "Client State")
    uint32 LastUpdateTimestamp = 0;
};

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class CLIMBINGGAME_API UEnvironmentalNetworkManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnvironmentalNetworkManager();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // Environmental system references
    UPROPERTY(BlueprintReadWrite, Category = "System References")
    UEnvironmentalHazardManager* HazardManager;

    UPROPERTY(BlueprintReadWrite, Category = "System References")
    UWindPhysicsSystem* WindSystem;

    UPROPERTY(BlueprintReadWrite, Category = "System References")
    UWeatherEffectsSystem* WeatherSystem;

    UPROPERTY(BlueprintReadWrite, Category = "System References")
    UGeologicalPhysicsSystem* GeologicalSystem;

    UPROPERTY(BlueprintReadWrite, Category = "System References")
    UThermalPhysicsSystem* ThermalSystem;

    UPROPERTY(BlueprintReadWrite, Category = "System References")
    UVisibilitySystem* VisibilitySystem;

    // Network state
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Network State")
    TArray<FNetworkClientState> ConnectedClients;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Network State")
    FNetworkBandwidthStats ServerBandwidthStats;

    // Network configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Configuration")
    float MaxBandwidthUsage = 10000.0f; // bytes/second per client

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Configuration")
    float CriticalUpdateFrequency = 30.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Configuration")
    float HighUpdateFrequency = 10.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Configuration")
    float MediumUpdateFrequency = 2.0f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Configuration")
    float LowUpdateFrequency = 0.5f; // Hz

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Configuration")
    float BackgroundUpdateFrequency = 0.1f; // Hz

    // Client management
    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void RegisterClient(const FString& ClientID, const FVector& ClientLocation);

    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void UnregisterClient(const FString& ClientID);

    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void UpdateClientLocation(const FString& ClientID, const FVector& NewLocation);

    UFUNCTION(BlueprintCallable, Category = "Client Management")
    void UpdateClientNetworkSettings(const FString& ClientID, ENetworkCompressionLevel Compression, float MaxFrequency);

    // Hazard network updates
    UFUNCTION(NetMulticast, Reliable)
    void MulticastHazardUpdate(const FNetworkHazardUpdate& HazardUpdate);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastWeatherUpdate(const FNetworkWeatherUpdate& WeatherUpdate);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastWindUpdate(const FNetworkWindUpdate& WindUpdate);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastThermalUpdate(const FNetworkThermalUpdate& ThermalUpdate);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastVisibilityUpdate(const FNetworkVisibilityUpdate& VisibilityUpdate);

    // Targeted updates for specific clients
    UFUNCTION(Client, Reliable)
    void ClientReceiveHazardUpdate(const FNetworkHazardUpdate& HazardUpdate);

    UFUNCTION(Client, Reliable)
    void ClientReceiveWeatherUpdate(const FNetworkWeatherUpdate& WeatherUpdate);

    UFUNCTION(Client, Reliable)
    void ClientReceiveWindUpdate(const FNetworkWindUpdate& WindUpdate);

    UFUNCTION(Client, Reliable)
    void ClientReceiveThermalUpdate(const FNetworkThermalUpdate& ThermalUpdate);

    UFUNCTION(Client, Reliable)
    void ClientReceiveVisibilityUpdate(const FNetworkVisibilityUpdate& VisibilityUpdate);

    // Batch updates for efficiency
    UFUNCTION(NetMulticast, Reliable)
    void MulticastBatchedEnvironmentalUpdate(const TArray<FNetworkHazardUpdate>& HazardUpdates,
                                           const TArray<FNetworkWeatherUpdate>& WeatherUpdates,
                                           const TArray<FNetworkWindUpdate>& WindUpdates);

    // Delta compression updates
    UFUNCTION(Client, Reliable)
    void ClientReceiveDeltaUpdate(const TArray<uint8>& CompressedDelta);

    // Network optimization
    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void OptimizeNetworkUpdates();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    ENetworkUpdatePriority CalculateUpdatePriority(const FString& HazardID, const FVector& HazardLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void AdjustUpdateFrequencies();

    UFUNCTION(BlueprintCallable, Category = "Network Optimization")
    void EnableAdaptiveCompression(bool bEnable);

    // Bandwidth management
    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void UpdateBandwidthStats();

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    bool CanSendUpdate(ENetworkUpdatePriority Priority, int32 UpdateSize) const;

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void QueueUpdate(const FString& UpdateType, const TArray<uint8>& UpdateData, ENetworkUpdatePriority Priority);

    UFUNCTION(BlueprintCallable, Category = "Bandwidth Management")
    void ProcessUpdateQueue();

    // Compression and serialization
    UFUNCTION(BlueprintCallable, Category = "Compression")
    TArray<uint8> CompressEnvironmentalData(const TArray<uint8>& OriginalData, ENetworkCompressionLevel CompressionLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Compression")
    TArray<uint8> DecompressEnvironmentalData(const TArray<uint8>& CompressedData, ENetworkCompressionLevel CompressionLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Compression")
    int32 CalculateUpdateSize(const FNetworkHazardUpdate& Update) const;

    // Delta compression
    UFUNCTION(BlueprintCallable, Category = "Delta Compression")
    TArray<uint8> CreateDeltaUpdate(const TArray<uint8>& PreviousState, const TArray<uint8>& CurrentState) const;

    UFUNCTION(BlueprintCallable, Category = "Delta Compression")
    TArray<uint8> ApplyDeltaUpdate(const TArray<uint8>& BaseState, const TArray<uint8>& DeltaData) const;

    // Interest management
    UFUNCTION(BlueprintCallable, Category = "Interest Management")
    bool IsClientInterestedInHazard(const FString& ClientID, const FVector& HazardLocation, ENetworkUpdatePriority Priority) const;

    UFUNCTION(BlueprintCallable, Category = "Interest Management")
    TArray<FString> GetInterestedClients(const FVector& HazardLocation, ENetworkUpdatePriority Priority) const;

    UFUNCTION(BlueprintCallable, Category = "Interest Management")
    void UpdateClientInterestRegions();

    // Prediction and interpolation
    UFUNCTION(BlueprintCallable, Category = "Prediction")
    FNetworkHazardUpdate PredictHazardUpdate(const FNetworkHazardUpdate& LastUpdate, float DeltaTime) const;

    UFUNCTION(BlueprintCallable, Category = "Prediction")
    void EnableClientPrediction(bool bEnable);

    // Synchronization validation
    UFUNCTION(BlueprintCallable, Category = "Synchronization")
    bool ValidateClientState(const FString& ClientID);

    UFUNCTION(BlueprintCallable, Category = "Synchronization")
    void RequestFullStateSync(const FString& ClientID);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestStateSync();

    // Performance monitoring
    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    FNetworkBandwidthStats GetClientBandwidthStats(const FString& ClientID) const;

    UFUNCTION(BlueprintCallable, Category = "Performance Monitoring")
    void LogNetworkPerformance();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnClientConnected;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnClientDisconnected;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnBandwidthLimitReached;

    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FSimpleMulticastDelegate OnNetworkOptimized;

protected:
    // Internal network management
    void CollectEnvironmentalUpdates(float DeltaTime);
    void ProcessPendingUpdates();
    void UpdateNetworkStatistics(float DeltaTime);
    void ManageClientConnections();

    // Update creation and conversion
    FNetworkHazardUpdate CreateHazardUpdate(const FGeologicalData& GeologicalData) const;
    FNetworkWeatherUpdate CreateWeatherUpdate(const FWeatherData& WeatherData) const;
    FNetworkWindUpdate CreateWindUpdate(const FWindData& WindData) const;
    FNetworkThermalUpdate CreateThermalUpdate(const FThermalZone& ThermalZone) const;
    FNetworkVisibilityUpdate CreateVisibilityUpdate(const FVisibilityCondition& VisibilityCondition) const;

    // Update application on clients
    void ApplyHazardUpdate(const FNetworkHazardUpdate& Update);
    void ApplyWeatherUpdate(const FNetworkWeatherUpdate& Update);
    void ApplyWindUpdate(const FNetworkWindUpdate& Update);
    void ApplyThermalUpdate(const FNetworkThermalUpdate& Update);
    void ApplyVisibilityUpdate(const FNetworkVisibilityUpdate& Update);

    // Compression algorithms
    TArray<uint8> CompressLZ4(const TArray<uint8>& Data) const;
    TArray<uint8> DecompressLZ4(const TArray<uint8>& CompressedData) const;
    TArray<uint8> CompressQuantized(const TArray<uint8>& Data) const;
    TArray<uint8> DecompressQuantized(const TArray<uint8>& CompressedData) const;

    // Interest region calculations
    float CalculateClientDistance(const FString& ClientID, const FVector& Location) const;
    bool IsLocationInClientInterest(const FString& ClientID, const FVector& Location) const;
    float GetPriorityRadius(ENetworkUpdatePriority Priority) const;

private:
    // Network update queues
    struct FPendingUpdate
    {
        FString UpdateType;
        TArray<uint8> UpdateData;
        ENetworkUpdatePriority Priority;
        uint32 Timestamp;
        TArray<FString> TargetClients;
    };
    TArray<FPendingUpdate> UpdateQueue;

    // Update frequency tracking
    TMap<ENetworkUpdatePriority, float> LastUpdateTime;
    TMap<ENetworkUpdatePriority, float> UpdateIntervals;

    // Client state tracking
    TMap<FString, TArray<uint8>> ClientStates; // For delta compression
    TMap<FString, uint32> ClientLastSync;

    // Performance tracking
    float TotalBandwidthUsed = 0.0f;
    int32 TotalPacketsSent = 0;
    float AveragePacketSize = 0.0f;
    float NetworkLatency = 0.0f;

    // Compression settings
    bool bUseAdaptiveCompression = true;
    ENetworkCompressionLevel DefaultCompressionLevel = ENetworkCompressionLevel::Medium;

    // Interest management
    TMap<FString, TArray<FVector>> ClientInterestRegions;
    float InterestUpdateInterval = 1.0f;
    float LastInterestUpdate = 0.0f;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableDeltaCompression = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableClientPrediction = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    bool bEnableInterestManagement = true;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float MaxInterestRadius = 20000.0f; // cm

    UPROPERTY(EditAnywhere, Category = "Configuration")
    int32 MaxQueuedUpdates = 100;

    UPROPERTY(EditAnywhere, Category = "Configuration")
    float SyncValidationInterval = 10.0f; // seconds

public:
    // Static utility functions
    UFUNCTION(BlueprintCallable, Category = "Network Utilities", CallInEditor = true)
    static int32 EstimateUpdateBandwidth(ENetworkUpdatePriority Priority, int32 ClientCount);

    UFUNCTION(BlueprintCallable, Category = "Network Utilities", CallInEditor = true)
    static float CalculateCompressionRatio(int32 OriginalSize, int32 CompressedSize);

    UFUNCTION(BlueprintCallable, Category = "Network Utilities", CallInEditor = true)
    static ENetworkCompressionLevel GetOptimalCompressionLevel(float AvailableBandwidth, float RequiredBandwidth);

    UFUNCTION(BlueprintCallable, Category = "Network Utilities", CallInEditor = true)
    static FString GetNetworkStatsReport(const FNetworkBandwidthStats& Stats);
};