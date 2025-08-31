#include "ClimbingPlatformOptimizer.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformMemory.h"
#include "HAL/ThreadManager.h"
#include "RHI.h"
#include "Engine/GameUserSettings.h"
#include "TimerManager.h"
#include "Misc/ConfigCacheIni.h"

#if PLATFORM_WINDOWS
    #include "Windows/WindowsPlatformMisc.h"
    #include "Windows/AllowWindowsPlatformTypes.h"
    #include <windows.h>
    #include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
    #include "Linux/LinuxPlatformMisc.h"
    #include <sys/utsname.h>
    #include <unistd.h>
#elif PLATFORM_MAC
    #include "Mac/MacPlatformMisc.h"
    #include <sys/sysctl.h>
#endif

void UClimbingPlatformOptimizer::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Detect current platform and capabilities
    CurrentPlatformSettings = DetectPlatformCapabilities();
    CurrentPerformanceProfile = DefaultPerformanceProfile;
    CurrentOptimizationLevel = DefaultOptimizationLevel;

    // Apply automatic optimizations if enabled
    if (bEnableAutomaticOptimization)
    {
        ApplyPlatformOptimizations(DefaultOptimizationLevel);
        OnPlatformDetected.Broadcast();
    }

    // Setup periodic optimization checks
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(OptimizationCheckTimer,
            [this]() {
                if (bEnableAutomaticOptimization)
                {
                    UpdateOptimizationEffectiveness();
                }
            },
            OptimizationCheckInterval, true);
    }

    UE_LOG(LogTemp, Log, TEXT("ClimbingPlatformOptimizer initialized for %s"), 
           *CurrentPlatformSettings.PlatformName);
}

void UClimbingPlatformOptimizer::Deinitialize()
{
    if (bIsMonitoring)
    {
        StopPlatformPerformanceMonitoring();
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(OptimizationCheckTimer);
    }

    AppliedOptimizations.Empty();

    Super::Deinitialize();
}

bool UClimbingPlatformOptimizer::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

EPlatformType UClimbingPlatformOptimizer::DetectCurrentPlatform() const
{
#if PLATFORM_WINDOWS
    return EPlatformType::Windows;
#elif PLATFORM_LINUX
    // Check if running on Steam Deck
    FString OSVersion = FPlatformMisc::GetOSVersion();
    if (OSVersion.Contains(TEXT("SteamOS")) || OSVersion.Contains(TEXT("steamdeck")))
    {
        return EPlatformType::Steam;
    }
    return EPlatformType::Linux;
#elif PLATFORM_MAC
    return EPlatformType::Mac;
#else
    return EPlatformType::Unknown;
#endif
}

FString UClimbingPlatformOptimizer::GetPlatformName() const
{
    return FPlatformMisc::GetPlatformName();
}

FString UClimbingPlatformOptimizer::GetArchitectureName() const
{
    return FPlatformMisc::GetCPUChipset();
}

FPlatformOptimizationSettings UClimbingPlatformOptimizer::DetectPlatformCapabilities()
{
    FPlatformOptimizationSettings Settings;
    
    // Basic platform detection
    Settings.PlatformType = DetectCurrentPlatform();
    Settings.PlatformName = GetPlatformName();
    Settings.ArchitectureName = GetArchitectureName();
    
    // CPU detection
    Settings.CPUCoreCount = FPlatformMisc::NumberOfCores();
    Settings.CPUFrequencyGHz = 3.5f; // Default, would need platform-specific detection
    
    // Memory detection
    const FPlatformMemoryConstants& MemConstants = FPlatformMemory::GetConstants();
    Settings.SystemMemoryGB = static_cast<int32>(MemConstants.TotalPhysical / (1024 * 1024 * 1024));
    
    // GPU detection
    Settings.bHasDedicatedGPU = HasDedicatedGPU();
    Settings.GPUMemoryMB = DetectGPUMemoryMB();
    
    // Storage detection
    Settings.bHasNVMeStorage = DetectNVMeStorage();
    
    // Set platform-specific capabilities
    switch (Settings.PlatformType)
    {
        case EPlatformType::Windows:
            Settings.bEnableDirectX12 = true;
            Settings.bEnableVulkanAPI = true;
            Settings.bEnableRayTracing = Settings.bHasDedicatedGPU;
            Settings.bEnableDLSS = DetectGPUVendor().Contains(TEXT("NVIDIA"));
            Settings.bEnableFSR = DetectGPUVendor().Contains(TEXT("AMD"));
            break;
            
        case EPlatformType::Linux:
            Settings.bEnableVulkanAPI = true;
            Settings.bEnableDirectX12 = false;
            Settings.bEnableFSR = true;
            break;
            
        case EPlatformType::Mac:
            Settings.bEnableMetalAPI = true;
            Settings.bEnableVulkanAPI = false;
            Settings.bEnableDirectX12 = false;
            break;
            
        case EPlatformType::Steam:
            // Steam Deck specific optimizations
            Settings.bEnableVulkanAPI = true;
            Settings.bEnableFSR = true;
            Settings.MaxPhysicsThreads = 2; // Steam Deck has 4 cores, reserve 2 for other tasks
            Settings.TexturePoolSizeMB = 512; // Limited VRAM
            break;
    }
    
    // Set optimal thread counts
    Settings.MaxPhysicsThreads = FMath::Clamp(Settings.CPUCoreCount / 2, 1, 8);
    Settings.MaxNetworkThreads = FMath::Clamp(Settings.CPUCoreCount / 4, 1, 4);
    Settings.MaxIOThreads = FMath::Clamp(Settings.CPUCoreCount / 2, 2, 8);
    
    // Set memory pool sizes based on available memory
    if (Settings.SystemMemoryGB >= 16)
    {
        Settings.TexturePoolSizeMB = 2048;
        Settings.AudioPoolSizeMB = 512;
        Settings.PhysicsPoolSizeMB = 1024;
    }
    else if (Settings.SystemMemoryGB >= 8)
    {
        Settings.TexturePoolSizeMB = 1024;
        Settings.AudioPoolSizeMB = 256;
        Settings.PhysicsPoolSizeMB = 512;
    }
    else
    {
        Settings.TexturePoolSizeMB = 512;
        Settings.AudioPoolSizeMB = 128;
        Settings.PhysicsPoolSizeMB = 256;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Detected platform: %s, %d cores, %dGB RAM, %s GPU"), 
           *Settings.PlatformName, Settings.CPUCoreCount, Settings.SystemMemoryGB,
           Settings.bHasDedicatedGPU ? TEXT("Dedicated") : TEXT("Integrated"));
    
    return Settings;
}

bool UClimbingPlatformOptimizer::IsRunningOnHighEndHardware() const
{
    return CurrentPlatformSettings.SystemMemoryGB >= 16 && 
           CurrentPlatformSettings.CPUCoreCount >= 8 && 
           CurrentPlatformSettings.bHasDedicatedGPU &&
           CurrentPlatformSettings.GPUMemoryMB >= 8192;
}

bool UClimbingPlatformOptimizer::IsRunningOnLowEndHardware() const
{
    return CurrentPlatformSettings.SystemMemoryGB <= 4 || 
           CurrentPlatformSettings.CPUCoreCount <= 2 || 
           !CurrentPlatformSettings.bHasDedicatedGPU;
}

bool UClimbingPlatformOptimizer::HasDedicatedGPU() const
{
    FString AdapterName = GRHIAdapterName;
    
    // Check for integrated graphics keywords
    return !AdapterName.Contains(TEXT("Intel HD")) &&
           !AdapterName.Contains(TEXT("Intel UHD")) &&
           !AdapterName.Contains(TEXT("Intel Iris")) &&
           !AdapterName.Contains(TEXT("AMD Radeon Graphics")) &&
           !AdapterName.Contains(TEXT("Integrated"));
}

int32 UClimbingPlatformOptimizer::GetAvailableSystemMemoryGB() const
{
    return CurrentPlatformSettings.SystemMemoryGB;
}

int32 UClimbingPlatformOptimizer::GetCPUCoreCount() const
{
    return CurrentPlatformSettings.CPUCoreCount;
}

TArray<FOptimizationResult> UClimbingPlatformOptimizer::ApplyPlatformOptimizations(EOptimizationLevel Level)
{
    UE_LOG(LogTemp, Log, TEXT("Applying %s level optimizations for %s"), 
           *UEnum::GetValueAsString(Level), *CurrentPlatformSettings.PlatformName);

    TArray<FOptimizationResult> Results;
    CurrentOptimizationLevel = Level;

    // Apply platform-specific optimizations
    switch (CurrentPlatformSettings.PlatformType)
    {
        case EPlatformType::Windows:
            ApplyWindowsSpecificOptimizations();
            break;
        case EPlatformType::Linux:
        case EPlatformType::Steam:
            ApplyLinuxSpecificOptimizations();
            if (CurrentPlatformSettings.PlatformType == EPlatformType::Steam)
            {
                ApplySteamDeckOptimizations();
            }
            break;
        case EPlatformType::Mac:
            ApplyMacSpecificOptimizations();
            break;
    }

    // Apply universal optimizations
    OptimizeThreadingForPlatform();
    OptimizeMemoryForPlatform();
    OptimizeGraphicsAPIForPlatform();
    OptimizeRenderingForPlatform();
    OptimizePhysicsForPlatform();
    OptimizeIOForPlatform();
    OptimizeNetworkForPlatform();

    bOptimizationsApplied = true;
    OnOptimizationsApplied.Broadcast();

    return Results;
}

void UClimbingPlatformOptimizer::SetPerformanceProfile(EPerformanceProfile Profile)
{
    if (CurrentPerformanceProfile == Profile)
        return;

    UE_LOG(LogTemp, Log, TEXT("Changing performance profile from %s to %s"),
           *UEnum::GetValueAsString(CurrentPerformanceProfile),
           *UEnum::GetValueAsString(Profile));

    CurrentPerformanceProfile = Profile;
    ApplyPerformanceProfileOptimizations();
    OnPerformanceProfileChanged.Broadcast();
}

EPerformanceProfile UClimbingPlatformOptimizer::GetCurrentPerformanceProfile() const
{
    return CurrentPerformanceProfile;
}

void UClimbingPlatformOptimizer::ApplyPerformanceProfileOptimizations()
{
    if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
    {
        switch (CurrentPerformanceProfile)
        {
            case EPerformanceProfile::PowerSaver:
                UserSettings->SetFrameRateLimit(30.0f);
                UserSettings->SetOverallScalabilityLevel(1);
                UserSettings->SetResolutionScaleNormalized(0.75f);
                UpdateCVar(TEXT("r.VSync"), TEXT("1"));
                UpdateCVar(TEXT("r.MaxFPS"), TEXT("30"));
                break;

            case EPerformanceProfile::Balanced:
                UserSettings->SetFrameRateLimit(60.0f);
                UserSettings->SetOverallScalabilityLevel(2);
                UserSettings->SetResolutionScaleNormalized(1.0f);
                UpdateCVar(TEXT("r.MaxFPS"), TEXT("60"));
                break;

            case EPerformanceProfile::Performance:
                UserSettings->SetFrameRateLimit(120.0f);
                UserSettings->SetOverallScalabilityLevel(3);
                UserSettings->SetResolutionScaleNormalized(0.9f);
                UpdateCVar(TEXT("r.MaxFPS"), TEXT("120"));
                UpdateCVar(TEXT("r.VSync"), TEXT("0"));
                break;

            case EPerformanceProfile::MaxPerformance:
                UserSettings->SetFrameRateLimit(0.0f); // Unlimited
                UserSettings->SetOverallScalabilityLevel(4);
                UserSettings->SetResolutionScaleNormalized(0.8f);
                UpdateCVar(TEXT("r.MaxFPS"), TEXT("0"));
                UpdateCVar(TEXT("r.VSync"), TEXT("0"));
                break;
        }

        UserSettings->ApplySettings(false);
    }

    // Apply performance profile to physics system
    if (UClimbingPerformanceManager* PerfManager = GetWorld()->GetSubsystem<UClimbingPerformanceManager>())
    {
        switch (CurrentPerformanceProfile)
        {
            case EPerformanceProfile::PowerSaver:
                PerfManager->PerformanceTargets.TargetFPS = 30.0f;
                PerfManager->PerformanceTargets.MaxActiveRopes = 10;
                break;
            case EPerformanceProfile::Balanced:
                PerfManager->PerformanceTargets.TargetFPS = 60.0f;
                PerfManager->PerformanceTargets.MaxActiveRopes = 30;
                break;
            case EPerformanceProfile::Performance:
                PerfManager->PerformanceTargets.TargetFPS = 90.0f;
                PerfManager->PerformanceTargets.MaxActiveRopes = 50;
                break;
            case EPerformanceProfile::MaxPerformance:
                PerfManager->PerformanceTargets.TargetFPS = 120.0f;
                PerfManager->PerformanceTargets.MaxActiveRopes = 80;
                break;
        }
    }
}

void UClimbingPlatformOptimizer::OptimizeThreadingForPlatform()
{
    // Configure task graph threads
    int32 WorkerThreadCount = FMath::Max(1, CurrentPlatformSettings.CPUCoreCount - 2); // Reserve main + render thread
    
    // Limit based on platform
    switch (CurrentPlatformSettings.PlatformType)
    {
        case EPlatformType::Steam:
            WorkerThreadCount = FMath::Min(WorkerThreadCount, 2); // Steam Deck limitation
            break;
        case EPlatformType::Mac:
            // On Apple Silicon, we might want different threading
            if (CurrentPlatformSettings.ArchitectureName.Contains(TEXT("Apple")))
            {
                WorkerThreadCount = FMath::Min(WorkerThreadCount, 6);
            }
            break;
    }

    // Apply threading optimizations via CVars
    UpdateCVar(TEXT("TaskGraph.NumBackgroundThreads"), FString::FromInt(WorkerThreadCount));
    UpdateCVar(TEXT("r.RHICmdBypass"), CurrentPlatformSettings.CPUCoreCount >= 6 ? TEXT("0") : TEXT("1"));

    // Physics threading
    if (CurrentPlatformSettings.bEnablePhysicsMultithreading)
    {
        UpdateCVar(TEXT("p.EnableAsyncScene"), TEXT("1"));
        UpdateCVar(TEXT("p.DefaultAsyncBudgetMS"), TEXT("5.0"));
    }

    UE_LOG(LogTemp, Log, TEXT("Threading optimized: %d worker threads"), WorkerThreadCount);
}

void UClimbingPlatformOptimizer::OptimizeMemoryForPlatform()
{
    // Set texture streaming pool size
    int32 TexturePoolSizeMB = CurrentPlatformSettings.TexturePoolSizeMB;
    
    // Adjust based on performance profile
    switch (CurrentPerformanceProfile)
    {
        case EPerformanceProfile::PowerSaver:
            TexturePoolSizeMB = static_cast<int32>(TexturePoolSizeMB * 0.5f);
            break;
        case EPerformanceProfile::MaxPerformance:
            TexturePoolSizeMB = static_cast<int32>(TexturePoolSizeMB * 1.5f);
            break;
    }

    UpdateCVar(TEXT("r.Streaming.PoolSize"), FString::FromInt(TexturePoolSizeMB));
    UpdateCVar(TEXT("r.Streaming.MaxTempMemoryAllowed"), FString::FromInt(TexturePoolSizeMB / 2));

    // Audio memory
    UpdateCVar(TEXT("au.StreamCacheSizeMB"), FString::FromInt(CurrentPlatformSettings.AudioPoolSizeMB));

    // Enable memory prefetching on high-end systems
    if (IsRunningOnHighEndHardware())
    {
        UpdateCVar(TEXT("r.Streaming.UseAsyncRequestsForDDC"), TEXT("1"));
        UpdateCVar(TEXT("r.Streaming.UseBackgroundLevelStreaming"), TEXT("1"));
    }

    UE_LOG(LogTemp, Log, TEXT("Memory pools configured: Texture=%dMB, Audio=%dMB"), 
           TexturePoolSizeMB, CurrentPlatformSettings.AudioPoolSizeMB);
}

void UClimbingPlatformOptimizer::OptimizeGraphicsAPIForPlatform()
{
    // This would typically be set during engine initialization
    // For runtime, we can only set preferences for next launch

    FString PreferredRHI;
    
    switch (CurrentPlatformSettings.PlatformType)
    {
        case EPlatformType::Windows:
            PreferredRHI = CurrentPlatformSettings.bEnableDirectX12 ? TEXT("DirectX12") : TEXT("DirectX11");
            break;
        case EPlatformType::Linux:
        case EPlatformType::Steam:
            PreferredRHI = TEXT("Vulkan");
            break;
        case EPlatformType::Mac:
            PreferredRHI = TEXT("Metal");
            break;
    }

    // Set RHI preference in config for next launch
    GConfig->SetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), 
                      TEXT("DefaultGraphicsRHI"), *PreferredRHI, GEngineIni);

    UE_LOG(LogTemp, Log, TEXT("Graphics API optimized for %s: %s"), 
           *CurrentPlatformSettings.PlatformName, *PreferredRHI);
}

void UClimbingPlatformOptimizer::OptimizeRenderingForPlatform()
{
    // Enable upscaling technologies
    if (CurrentPlatformSettings.bEnableDLSS)
    {
        UpdateCVar(TEXT("r.NGX.DLSS.Enable"), TEXT("1"));
        UpdateCVar(TEXT("r.NGX.DLSS.Quality"), TEXT("2")); // Balanced
    }
    else if (CurrentPlatformSettings.bEnableFSR)
    {
        UpdateCVar(TEXT("r.FidelityFX.FSR.Enabled"), TEXT("1"));
        UpdateCVar(TEXT("r.FidelityFX.FSR.Quality"), TEXT("2")); // Balanced
    }

    // Platform-specific rendering optimizations
    switch (CurrentPlatformSettings.PlatformType)
    {
        case EPlatformType::Steam:
            // Steam Deck specific optimizations
            UpdateCVar(TEXT("r.Mobile.DisableVertexFog"), TEXT("1"));
            UpdateCVar(TEXT("r.InstancedStereo"), TEXT("0"));
            UpdateCVar(TEXT("r.MobileHDR"), TEXT("0"));
            break;
            
        case EPlatformType::Mac:
            // macOS specific optimizations
            if (CurrentPlatformSettings.bOptimizeForAppleSilicon)
            {
                UpdateCVar(TEXT("r.Metal.UseParallelRenderEncoder"), TEXT("1"));
                UpdateCVar(TEXT("r.Metal.SeparatePresentThread"), TEXT("1"));
            }
            break;
    }

    // Set optimal shadow settings based on hardware
    if (IsRunningOnHighEndHardware())
    {
        UpdateCVar(TEXT("r.ShadowQuality"), TEXT("4"));
        UpdateCVar(TEXT("r.Shadow.MaxResolution"), TEXT("4096"));
    }
    else if (IsRunningOnLowEndHardware())
    {
        UpdateCVar(TEXT("r.ShadowQuality"), TEXT("1"));
        UpdateCVar(TEXT("r.Shadow.MaxResolution"), TEXT("1024"));
    }

    UE_LOG(LogTemp, Log, TEXT("Rendering optimized for platform capabilities"));
}

void UClimbingPlatformOptimizer::OptimizePhysicsForPlatform()
{
    // Set physics timestep based on performance profile
    float PhysicsTimeStep = 1.0f / 60.0f; // Default 60Hz
    
    switch (CurrentPerformanceProfile)
    {
        case EPerformanceProfile::PowerSaver:
            PhysicsTimeStep = 1.0f / 30.0f; // 30Hz
            break;
        case EPerformanceProfile::MaxPerformance:
            PhysicsTimeStep = 1.0f / 120.0f; // 120Hz
            break;
    }

    UpdateCVar(TEXT("p.MaxSubstepDeltaTime"), FString::SanitizeFloat(PhysicsTimeStep));
    
    // Enable async physics on multi-core systems
    if (CurrentPlatformSettings.CPUCoreCount >= 4)
    {
        UpdateCVar(TEXT("p.EnableAsyncScene"), TEXT("1"));
        UpdateCVar(TEXT("p.AsyncSceneEnabled"), TEXT("1"));
    }

    // Configure physics thread count
    int32 PhysicsThreads = FMath::Min(CurrentPlatformSettings.MaxPhysicsThreads, 
                                     CurrentPlatformSettings.CPUCoreCount / 2);
    UpdateCVar(TEXT("p.NumThreads"), FString::FromInt(PhysicsThreads));

    UE_LOG(LogTemp, Log, TEXT("Physics optimized: %dHz timestep, %d threads"), 
           static_cast<int32>(1.0f / PhysicsTimeStep), PhysicsThreads);
}

void UClimbingPlatformOptimizer::OptimizeIOForPlatform()
{
    // Configure async loading
    UpdateCVar(TEXT("s.AsyncLoadingThreadEnabled"), TEXT("1"));
    UpdateCVar(TEXT("s.EventDrivenLoaderEnabled"), TEXT("1"));

    // Set IO thread count
    int32 IOThreads = CurrentPlatformSettings.MaxIOThreads;
    UpdateCVar(TEXT("s.IOWorkerThreadCount"), FString::FromInt(IOThreads));

    // Enable precaching on systems with fast storage
    if (CurrentPlatformSettings.bHasNVMeStorage)
    {
        UpdateCVar(TEXT("s.UsePreloadingThread"), TEXT("1"));
        UpdateCVar(TEXT("r.Streaming.UseAsyncRequestsForDDC"), TEXT("1"));
    }

    UE_LOG(LogTemp, Log, TEXT("I/O optimized: %d threads, precaching %s"), 
           IOThreads, CurrentPlatformSettings.bHasNVMeStorage ? TEXT("enabled") : TEXT("disabled"));
}

void UClimbingPlatformOptimizer::OptimizeNetworkForPlatform()
{
    // Configure network threading
    UpdateCVar(TEXT("net.UseAdaptiveNetUpdateFrequency"), TEXT("1"));
    UpdateCVar(TEXT("net.NetClientTicksPerSecond"), TEXT("30"));

    // Enable compression on slower connections
    if (CurrentPlatformSettings.bEnableNetworkCompression)
    {
        UpdateCVar(TEXT("net.UseCompression"), TEXT("1"));
        UpdateCVar(TEXT("net.CompressionLevel"), TEXT("3")); // Moderate compression
    }

    UE_LOG(LogTemp, Log, TEXT("Network optimized for platform"));
}

// Platform-specific optimization implementations
void UClimbingPlatformOptimizer::ApplyWindowsSpecificOptimizations()
{
#if PLATFORM_WINDOWS
    // Enable Windows Game Mode
    UpdateCVar(TEXT("r.Windows.UseGameMode"), TEXT("1"));
    
    // Set high priority for game process
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
    // Enable DirectStorage if available
    if (CurrentPlatformSettings.bEnableDirectStorage)
    {
        UpdateCVar(TEXT("r.DirectStorage.Enable"), TEXT("1"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("Windows-specific optimizations applied"));
#endif
}

void UClimbingPlatformOptimizer::ApplyLinuxSpecificOptimizations()
{
#if PLATFORM_LINUX
    // Enable Linux-specific optimizations
    UpdateCVar(TEXT("r.Linux.UseVulkan"), TEXT("1"));
    
    // Optimize for specific distributions
    FString OSVersion = FPlatformMisc::GetOSVersion();
    if (OSVersion.Contains(TEXT("Ubuntu")) || OSVersion.Contains(TEXT("Debian")))
    {
        // Ubuntu/Debian specific optimizations
        UpdateCVar(TEXT("r.Linux.OptimizeForUbuntu"), TEXT("1"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("Linux-specific optimizations applied"));
#endif
}

void UClimbingPlatformOptimizer::ApplyMacSpecificOptimizations()
{
#if PLATFORM_MAC
    // Enable Metal Performance Shaders
    UpdateCVar(TEXT("r.Metal.UseMetalPerformanceShaders"), TEXT("1"));
    
    // Optimize for Apple Silicon vs Intel
    if (CurrentPlatformSettings.bOptimizeForAppleSilicon)
    {
        UpdateCVar(TEXT("r.Metal.OptimizeForAppleSilicon"), TEXT("1"));
        UpdateCVar(TEXT("r.Metal.UseUnifiedMemory"), TEXT("1"));
    }
    
    // Enable thermal management
    if (CurrentPlatformSettings.bEnableThermalManagement)
    {
        UpdateCVar(TEXT("r.Mac.EnableThermalThrottling"), TEXT("1"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("macOS-specific optimizations applied"));
#endif
}

void UClimbingPlatformOptimizer::ApplySteamDeckOptimizations()
{
    // Steam Deck specific settings
    UpdateCVar(TEXT("r.SteamDeck.OptimizeForDeck"), TEXT("1"));
    UpdateCVar(TEXT("r.TemporalAA.Quality"), TEXT("2")); // Lower TAA quality
    UpdateCVar(TEXT("r.PostProcessAAQuality"), TEXT("2"));
    UpdateCVar(TEXT("r.Tonemapper.Quality"), TEXT("0")); // Fast tonemapper
    
    // Limit texture quality for VRAM constraints
    UpdateCVar(TEXT("r.MaxTextureSize"), TEXT("2048"));
    UpdateCVar(TEXT("r.Streaming.LimitPoolSizeToVRAM"), TEXT("1"));
    
    UE_LOG(LogTemp, Log, TEXT("Steam Deck optimizations applied"));
}

// Helper functions
FString UClimbingPlatformOptimizer::DetectGPUVendor() const
{
    FString AdapterName = GRHIAdapterName;
    if (AdapterName.Contains(TEXT("NVIDIA")) || AdapterName.Contains(TEXT("GeForce")) || AdapterName.Contains(TEXT("Quadro")))
    {
        return TEXT("NVIDIA");
    }
    else if (AdapterName.Contains(TEXT("AMD")) || AdapterName.Contains(TEXT("Radeon")))
    {
        return TEXT("AMD");
    }
    else if (AdapterName.Contains(TEXT("Intel")))
    {
        return TEXT("Intel");
    }
    else if (AdapterName.Contains(TEXT("Apple")))
    {
        return TEXT("Apple");
    }
    return TEXT("Unknown");
}

FString UClimbingPlatformOptimizer::DetectCPUVendor() const
{
    FString CPUBrand = FPlatformMisc::GetCPUBrand();
    if (CPUBrand.Contains(TEXT("Intel")))
    {
        return TEXT("Intel");
    }
    else if (CPUBrand.Contains(TEXT("AMD")))
    {
        return TEXT("AMD");
    }
    else if (CPUBrand.Contains(TEXT("Apple")))
    {
        return TEXT("Apple");
    }
    return TEXT("Unknown");
}

bool UClimbingPlatformOptimizer::DetectNVMeStorage() const
{
    // This would require platform-specific implementation
    // For now, assume NVMe on high-end systems
    return IsRunningOnHighEndHardware();
}

int32 UClimbingPlatformOptimizer::DetectGPUMemoryMB() const
{
    // This would require platform-specific GPU memory detection
    // For now, estimate based on GPU type
    FString GPUVendor = DetectGPUVendor();
    FString AdapterName = GRHIAdapterName;
    
    if (AdapterName.Contains(TEXT("RTX 4090"))) return 24576;
    if (AdapterName.Contains(TEXT("RTX 4080"))) return 16384;
    if (AdapterName.Contains(TEXT("RTX 4070"))) return 12288;
    if (AdapterName.Contains(TEXT("RTX 3080"))) return 10240;
    if (AdapterName.Contains(TEXT("RTX 3070"))) return 8192;
    if (AdapterName.Contains(TEXT("RTX 3060"))) return 8192;
    if (AdapterName.Contains(TEXT("GTX 1660"))) return 6144;
    
    return 4096; // Default assumption
}

void UClimbingPlatformOptimizer::UpdateCVar(const FString& CVarName, const FString& Value)
{
    if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName))
    {
        CVar->Set(*Value, ECVF_SetByGameSetting);
        
        if (bLogOptimizationChanges)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("Updated CVar %s = %s"), *CVarName, *Value);
        }
    }
    else if (bLogOptimizationChanges)
    {
        UE_LOG(LogTemp, Warning, TEXT("CVar not found: %s"), *CVarName);
    }
}

// Monitoring functions
void UClimbingPlatformOptimizer::StartPlatformPerformanceMonitoring()
{
    if (bIsMonitoring)
        return;
        
    bIsMonitoring = true;
    UE_LOG(LogTemp, Log, TEXT("Platform performance monitoring started"));
}

void UClimbingPlatformOptimizer::StopPlatformPerformanceMonitoring()
{
    if (!bIsMonitoring)
        return;
        
    bIsMonitoring = false;
    UE_LOG(LogTemp, Log, TEXT("Platform performance monitoring stopped"));
}

TArray<FString> UClimbingPlatformOptimizer::GetOptimizationRecommendations() const
{
    return GenerateOptimizationRecommendations();
}

TArray<FString> UClimbingPlatformOptimizer::GenerateOptimizationRecommendations() const
{
    TArray<FString> Recommendations;
    
    // Analyze current system and recommend optimizations
    if (IsRunningOnLowEndHardware())
    {
        Recommendations.Add(TEXT("Consider using PowerSaver performance profile"));
        Recommendations.Add(TEXT("Reduce texture quality settings"));
        Recommendations.Add(TEXT("Disable advanced lighting features"));
    }
    
    if (!CurrentPlatformSettings.bHasDedicatedGPU)
    {
        Recommendations.Add(TEXT("Consider upgrading to dedicated GPU for better performance"));
        Recommendations.Add(TEXT("Enable FSR/DLSS upscaling if available"));
    }
    
    if (CurrentPlatformSettings.SystemMemoryGB < 8)
    {
        Recommendations.Add(TEXT("Consider adding more system memory"));
        Recommendations.Add(TEXT("Enable aggressive memory management"));
    }
    
    // Platform-specific recommendations
    switch (CurrentPlatformSettings.PlatformType)
    {
        case EPlatformType::Steam:
            Recommendations.Add(TEXT("Enable Steam Deck specific optimizations"));
            Recommendations.Add(TEXT("Use FSR for better performance on Steam Deck"));
            break;
        case EPlatformType::Mac:
            Recommendations.Add(TEXT("Enable Metal Performance Shaders"));
            if (DetectCPUVendor() == TEXT("Apple"))
            {
                Recommendations.Add(TEXT("Enable Apple Silicon optimizations"));
            }
            break;
    }
    
    return Recommendations;
}

void UClimbingPlatformOptimizer::UpdateOptimizationEffectiveness()
{
    // Monitor performance and adjust optimizations if needed
    if (UClimbingPerformanceManager* PerfManager = GetWorld()->GetSubsystem<UClimbingPerformanceManager>())
    {
        FPerformanceMetrics Metrics = PerfManager->GetCurrentMetrics();
        
        // Check if we're meeting performance targets
        if (Metrics.CurrentFPS < 30.0f && CurrentPerformanceProfile != EPerformanceProfile::PowerSaver)
        {
            UE_LOG(LogTemp, Warning, TEXT("Performance below 30 FPS, consider reducing quality"));
        }
        
        // Automatic profile adjustment based on performance
        if (bEnableAutomaticOptimization && Metrics.CurrentFPS < 25.0f)
        {
            switch (CurrentPerformanceProfile)
            {
                case EPerformanceProfile::MaxPerformance:
                    SetPerformanceProfile(EPerformanceProfile::Performance);
                    break;
                case EPerformanceProfile::Performance:
                    SetPerformanceProfile(EPerformanceProfile::Balanced);
                    break;
                case EPerformanceProfile::Balanced:
                    SetPerformanceProfile(EPerformanceProfile::PowerSaver);
                    break;
            }
        }
    }
}

// Placeholder implementations for remaining functions
FOptimizationResult UClimbingPlatformOptimizer::ApplyWindowsOptimizations(const FWindowsOptimizationSettings& Settings) { return FOptimizationResult(); }
FOptimizationResult UClimbingPlatformOptimizer::ApplyLinuxOptimizations(const FLinuxOptimizationSettings& Settings) { return FOptimizationResult(); }
FOptimizationResult UClimbingPlatformOptimizer::ApplyMacOptimizations(const FMacOptimizationSettings& Settings) { return FOptimizationResult(); }
void UClimbingPlatformOptimizer::SetCPUAffinityOptimization(bool bEnable) {}
void UClimbingPlatformOptimizer::SetThreadPoolSize(int32 ThreadCount) {}
void UClimbingPlatformOptimizer::SetMemoryPoolSizes(int32 TexturePoolMB, int32 AudioPoolMB, int32 PhysicsPoolMB) {}
void UClimbingPlatformOptimizer::EnableMemoryPrefetching(bool bEnable) {}
bool UClimbingPlatformOptimizer::ShouldUseVulkan() const { return CurrentPlatformSettings.bEnableVulkanAPI; }
bool UClimbingPlatformOptimizer::ShouldUseDirectX12() const { return CurrentPlatformSettings.bEnableDirectX12; }
bool UClimbingPlatformOptimizer::ShouldUseMetal() const { return CurrentPlatformSettings.bEnableMetalAPI; }
void UClimbingPlatformOptimizer::EnableUpscalingTechnology() {}
void UClimbingPlatformOptimizer::SetOptimalRenderScale() {}
void UClimbingPlatformOptimizer::EnableAssetPrecaching(bool bEnable) {}
void UClimbingPlatformOptimizer::OptimizeAssetStreaming() {}
void UClimbingPlatformOptimizer::SetOptimalPhysicsSettings() {}
void UClimbingPlatformOptimizer::EnableAsyncPhysics(bool bEnable) {}
void UClimbingPlatformOptimizer::SetNetworkCompressionLevel(int32 Level) {}
void UClimbingPlatformOptimizer::EnableNetworkPrediction(bool bEnable) {}
void UClimbingPlatformOptimizer::CheckForPerformanceRegression() {}