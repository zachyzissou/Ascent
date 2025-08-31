#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "ClimbingPlatformOptimizer.generated.h"

UENUM(BlueprintType)
enum class EPlatformType : uint8
{
    Windows     UMETA(DisplayName = "Windows"),
    Linux       UMETA(DisplayName = "Linux"),
    Mac         UMETA(DisplayName = "macOS"),
    Steam       UMETA(DisplayName = "Steam Deck"),
    Unknown     UMETA(DisplayName = "Unknown Platform")
};

UENUM(BlueprintType)
enum class EOptimizationLevel : uint8
{
    Disabled    UMETA(DisplayName = "Disabled"),
    Light       UMETA(DisplayName = "Light Optimizations"),
    Moderate    UMETA(DisplayName = "Moderate Optimizations"),
    Aggressive  UMETA(DisplayName = "Aggressive Optimizations"),
    Maximum     UMETA(DisplayName = "Maximum Optimizations")
};

UENUM(BlueprintType)
enum class EPerformanceProfile : uint8
{
    PowerSaver    UMETA(DisplayName = "Power Saver"),
    Balanced      UMETA(DisplayName = "Balanced"),
    Performance   UMETA(DisplayName = "Performance"),
    MaxPerformance UMETA(DisplayName = "Maximum Performance")
};

USTRUCT(BlueprintType)
struct FPlatformOptimizationSettings
{
    GENERATED_BODY()

    // Platform identification
    UPROPERTY(BlueprintReadOnly)
    EPlatformType PlatformType = EPlatformType::Windows;

    UPROPERTY(BlueprintReadOnly)
    FString PlatformName;

    UPROPERTY(BlueprintReadOnly)
    FString ArchitectureName;

    // Hardware capabilities
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardware")
    int32 CPUCoreCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardware")
    float CPUFrequencyGHz = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardware")
    int32 SystemMemoryGB = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardware")
    int32 GPUMemoryMB = 8192;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardware")
    bool bHasDedicatedGPU = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hardware")
    bool bHasNVMeStorage = true;

    // Platform-specific optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimizations")
    bool bEnableThreadPoolOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimizations")
    bool bEnableCPUAffinityOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimizations")
    bool bEnableMemoryPooling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optimizations")
    bool bEnableAsyncLoadingOptimization = true;

    // Rendering optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableVulkanAPI = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableDirectX12 = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableMetalAPI = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableRayTracing = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableDLSS = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    bool bEnableFSR = false;

    // Physics optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    int32 MaxPhysicsThreads = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    bool bEnablePhysicsMultithreading = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    bool bEnableAsyncPhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float PhysicsTimeStep = 0.0166f; // 60Hz

    // Network optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    bool bEnableNetworkCompression = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    int32 MaxNetworkThreads = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    bool bEnableNetworkPrediction = true;

    // Memory optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    int32 TexturePoolSizeMB = 1024;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    int32 AudioPoolSizeMB = 256;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    int32 PhysicsPoolSizeMB = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    bool bEnableMemoryPrefetching = true;

    // I/O optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IO")
    bool bEnableIOBatching = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IO")
    int32 MaxIOThreads = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IO")
    bool bEnableAssetPrecaching = true;
};

USTRUCT(BlueprintType)
struct FWindowsOptimizationSettings : public FPlatformOptimizationSettings
{
    GENERATED_BODY()

    // Windows-specific optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bEnableWindowsGameMode = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bDisableWindowsDefender = false; // For development builds only

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bEnableHighPrecisionTimer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bSetHighPriorityClass = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bEnableDirectStorage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bOptimizeForIntel = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bOptimizeForAMD = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Windows")
    bool bOptimizeForNVIDIA = false;
};

USTRUCT(BlueprintType)
struct FLinuxOptimizationSettings : public FPlatformOptimizationSettings
{
    GENERATED_BODY()

    // Linux-specific optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    bool bEnableLinuxGameMode = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    bool bOptimizeForProton = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    bool bEnableVulkanValidation = false; // Debug builds only

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    bool bUseLowLatencyKernel = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    FString CPUGovernor = TEXT("performance");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    bool bDisableSwapping = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linux")
    bool bOptimizeForSteamOS = false;
};

USTRUCT(BlueprintType)
struct FMacOptimizationSettings : public FPlatformOptimizationSettings
{
    GENERATED_BODY()

    // macOS-specific optimizations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mac")
    bool bOptimizeForAppleSilicon = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mac")
    bool bOptimizeForIntelMac = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mac")
    bool bEnableMetalPerformanceShaders = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mac")
    bool bUseUnifiedMemoryOptimization = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mac")
    bool bEnableThermalManagement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mac")
    bool bOptimizeForRetina = true;
};

USTRUCT(BlueprintType)
struct FOptimizationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bOptimizationApplied = false;

    UPROPERTY(BlueprintReadOnly)
    FString OptimizationName;

    UPROPERTY(BlueprintReadOnly)
    float PerformanceImpactPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FString Description;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> AppliedSettings;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Warnings;
};

UCLASS(BlueprintType)
class CLIMBINGGAME_API UClimbingPlatformOptimizer : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Platform detection
    UFUNCTION(BlueprintCallable, Category = "Platform Detection")
    EPlatformType DetectCurrentPlatform() const;

    UFUNCTION(BlueprintCallable, Category = "Platform Detection")
    FString GetPlatformName() const;

    UFUNCTION(BlueprintCallable, Category = "Platform Detection")
    FString GetArchitectureName() const;

    UFUNCTION(BlueprintCallable, Category = "Platform Detection")
    FPlatformOptimizationSettings DetectPlatformCapabilities();

    // Hardware detection
    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    bool IsRunningOnHighEndHardware() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    bool IsRunningOnLowEndHardware() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    bool HasDedicatedGPU() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    int32 GetAvailableSystemMemoryGB() const;

    UFUNCTION(BlueprintCallable, Category = "Hardware Detection")
    int32 GetCPUCoreCount() const;

    // Optimization application
    UFUNCTION(BlueprintCallable, Category = "Optimization")
    TArray<FOptimizationResult> ApplyPlatformOptimizations(EOptimizationLevel Level);

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    FOptimizationResult ApplyWindowsOptimizations(const FWindowsOptimizationSettings& Settings);

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    FOptimizationResult ApplyLinuxOptimizations(const FLinuxOptimizationSettings& Settings);

    UFUNCTION(BlueprintCallable, Category = "Optimization")
    FOptimizationResult ApplyMacOptimizations(const FMacOptimizationSettings& Settings);

    // Performance profiling
    UFUNCTION(BlueprintCallable, Category = "Performance Profiling")
    void SetPerformanceProfile(EPerformanceProfile Profile);

    UFUNCTION(BlueprintCallable, Category = "Performance Profiling")
    EPerformanceProfile GetCurrentPerformanceProfile() const;

    UFUNCTION(BlueprintCallable, Category = "Performance Profiling")
    void ApplyPerformanceProfileOptimizations();

    // Threading optimizations
    UFUNCTION(BlueprintCallable, Category = "Threading")
    void OptimizeThreadingForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Threading")
    void SetCPUAffinityOptimization(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "Threading")
    void SetThreadPoolSize(int32 ThreadCount);

    // Memory optimizations
    UFUNCTION(BlueprintCallable, Category = "Memory")
    void OptimizeMemoryForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Memory")
    void SetMemoryPoolSizes(int32 TexturePoolMB, int32 AudioPoolMB, int32 PhysicsPoolMB);

    UFUNCTION(BlueprintCallable, Category = "Memory")
    void EnableMemoryPrefetching(bool bEnable);

    // Graphics API optimizations
    UFUNCTION(BlueprintCallable, Category = "Graphics API")
    void OptimizeGraphicsAPIForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Graphics API")
    bool ShouldUseVulkan() const;

    UFUNCTION(BlueprintCallable, Category = "Graphics API")
    bool ShouldUseDirectX12() const;

    UFUNCTION(BlueprintCallable, Category = "Graphics API")
    bool ShouldUseMetal() const;

    // Rendering optimizations
    UFUNCTION(BlueprintCallable, Category = "Rendering")
    void OptimizeRenderingForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Rendering")
    void EnableUpscalingTechnology();

    UFUNCTION(BlueprintCallable, Category = "Rendering")
    void SetOptimalRenderScale();

    // I/O optimizations
    UFUNCTION(BlueprintCallable, Category = "IO")
    void OptimizeIOForPlatform();

    UFUNCTION(BlueprintCallable, Category = "IO")
    void EnableAssetPrecaching(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "IO")
    void OptimizeAssetStreaming();

    // Physics optimizations
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void OptimizePhysicsForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Physics")
    void SetOptimalPhysicsSettings();

    UFUNCTION(BlueprintCallable, Category = "Physics")
    void EnableAsyncPhysics(bool bEnable);

    // Network optimizations
    UFUNCTION(BlueprintCallable, Category = "Network")
    void OptimizeNetworkForPlatform();

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetNetworkCompressionLevel(int32 Level);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void EnableNetworkPrediction(bool bEnable);

    // Monitoring and analysis
    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    void StartPlatformPerformanceMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    void StopPlatformPerformanceMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Monitoring")
    TArray<FString> GetOptimizationRecommendations() const;

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableAutomaticOptimization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    EOptimizationLevel DefaultOptimizationLevel = EOptimizationLevel::Moderate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    EPerformanceProfile DefaultPerformanceProfile = EPerformanceProfile::Balanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bLogOptimizationChanges = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float OptimizationCheckInterval = 60.0f; // seconds

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Optimization Events")
    FSimpleMulticastDelegate OnOptimizationsApplied;

    UPROPERTY(BlueprintAssignable, Category = "Optimization Events")
    FSimpleMulticastDelegate OnPerformanceProfileChanged;

    UPROPERTY(BlueprintAssignable, Category = "Optimization Events")
    FSimpleMulticastDelegate OnPlatformDetected;

protected:
    // Current platform settings
    UPROPERTY()
    FPlatformOptimizationSettings CurrentPlatformSettings;

    EPerformanceProfile CurrentPerformanceProfile = EPerformanceProfile::Balanced;
    EOptimizationLevel CurrentOptimizationLevel = EOptimizationLevel::Moderate;

    // Optimization state
    TArray<FOptimizationResult> AppliedOptimizations;
    bool bOptimizationsApplied = false;
    bool bIsMonitoring = false;

    // Timing
    float LastOptimizationCheck = 0.0f;
    FTimerHandle OptimizationCheckTimer;

private:
    // Internal optimization functions
    void ApplyWindowsSpecificOptimizations();
    void ApplyLinuxSpecificOptimizations();
    void ApplyMacSpecificOptimizations();
    void ApplySteamDeckOptimizations();

    // Hardware detection helpers
    FString DetectGPUVendor() const;
    FString DetectCPUVendor() const;
    bool DetectNVMeStorage() const;
    int32 DetectGPUMemoryMB() const;

    // Optimization helpers
    void SetEngineSettings(const TMap<FString, FString>& Settings);
    void ApplyRegistryChanges(const TMap<FString, FString>& Changes); // Windows only
    void ApplySystemOptimizations();
    void UpdateCVar(const FString& CVarName, const FString& Value);

    // Performance monitoring
    void UpdateOptimizationEffectiveness();
    void CheckForPerformanceRegression();
    TArray<FString> GenerateOptimizationRecommendations() const;

    // Platform-specific implementations
    void InitializeWindowsOptimizations();
    void InitializeLinuxOptimizations();
    void InitializeMacOptimizations();

    // Configuration validation
    bool ValidateOptimizationSettings(const FPlatformOptimizationSettings& Settings) const;
    void SanitizeOptimizationSettings(FPlatformOptimizationSettings& Settings) const;
};