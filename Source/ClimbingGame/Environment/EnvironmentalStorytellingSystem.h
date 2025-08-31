#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/TriggerVolume.h"
#include "Sound/SoundCue.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "EnvironmentalStorytellingSystem.generated.h"

UENUM(BlueprintType)
enum class EStoryEventType : uint8
{
    ClimberMemorial     UMETA(DisplayName = "Climber Memorial"),
    HistoricalAscent    UMETA(DisplayName = "Historical Ascent"),
    AccidentSite        UMETA(DisplayName = "Accident Site"),
    WeatherEvent        UMETA(DisplayName = "Weather Event"),
    GeologicalEvent     UMETA(DisplayName = "Geological Event"),
    RouteHistory        UMETA(DisplayName = "Route History"),
    EquipmentEvolution  UMETA(DisplayName = "Equipment Evolution"),
    TechnicalInnovation UMETA(DisplayName = "Technical Innovation"),
    RescueStory         UMETA(DisplayName = "Rescue Story"),
    ConservationEffort  UMETA(DisplayName = "Conservation Effort"),
    CulturalSignificance UMETA(DisplayName = "Cultural Significance"),
    FirstAscent         UMETA(DisplayName = "First Ascent"),
    EnvironmentalChange UMETA(DisplayName = "Environmental Change")
};

UENUM(BlueprintType)
enum class EStoryTriggerType : uint8
{
    Proximity           UMETA(DisplayName = "Proximity"),
    RouteCompletion     UMETA(DisplayName = "Route Completion"),
    HazardEncounter     UMETA(DisplayName = "Hazard Encounter"),
    SeasonalEvent       UMETA(DisplayName = "Seasonal Event"),
    WeatherCondition    UMETA(DisplayName = "Weather Condition"),
    TimeOfDay          UMETA(DisplayName = "Time of Day"),
    SkillLevel         UMETA(DisplayName = "Skill Level"),
    TeamSize           UMETA(DisplayName = "Team Size"),
    EquipmentUsed      UMETA(DisplayName = "Equipment Used"),
    PreviousExperience UMETA(DisplayName = "Previous Experience")
};

UENUM(BlueprintType)
enum class EStoryDeliveryMethod : uint8
{
    EnvironmentalClues  UMETA(DisplayName = "Environmental Clues"),
    InteractiveObject   UMETA(DisplayName = "Interactive Object"),
    AudioNarration      UMETA(DisplayName = "Audio Narration"),
    VisualCutscene      UMETA(DisplayName = "Visual Cutscene"),
    TextDisplay         UMETA(DisplayName = "Text Display"),
    GhostClimber        UMETA(DisplayName = "Ghost Climber"),
    WeatheredEquipment  UMETA(DisplayName = "Weathered Equipment"),
    RockCarving         UMETA(DisplayName = "Rock Carving"),
    PlantGrowth         UMETA(DisplayName = "Plant Growth Pattern"),
    WornPath           UMETA(DisplayName = "Worn Climbing Path")
};

USTRUCT(BlueprintType)
struct FStoryElement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    FString StoryTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element", Meta = (MultiLine = "true"))
    FString StoryDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    EStoryEventType EventType = EStoryEventType::RouteHistory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    EStoryDeliveryMethod DeliveryMethod = EStoryDeliveryMethod::EnvironmentalClues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    FDateTime HistoricalDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    TArray<FString> KeyCharacters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    FString LocationSignificance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EmotionalImpact = 0.5f; // 0 = informational, 1 = highly emotional

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    TArray<FString> EducationalTakeaways;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    bool bRequiresPreviousKnowledge = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    TArray<FString> PrerequisiteStories;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    USoundCue* NarrationAudio = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    UTexture2D* HistoricalImage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Element")
    UStaticMesh* AssociatedObject = nullptr;
};

USTRUCT(BlueprintType)
struct FStoryTriggerCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    EStoryTriggerType TriggerType = EStoryTriggerType::Proximity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    float TriggerRadius = 500.0f; // For proximity triggers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    FString RequiredRoute; // For route completion triggers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    FString RequiredWeatherCondition; // For weather triggers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    int32 MinimumSkillLevel = 1; // For skill level triggers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    int32 RequiredTeamSize = 1; // For team size triggers

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    float TriggerProbability = 1.0f; // 0.0 to 1.0, chance of triggering when conditions are met

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Condition")
    bool bOnlyTriggerOnce = true; // Whether this story can only be experienced once
};

USTRUCT(BlueprintType)
struct FEnvironmentalNarrative
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    FStoryElement MainStory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    TArray<FStoryElement> RelatedStories;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    FStoryTriggerCondition TriggerCondition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    bool bIsActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    TArray<FString> SeasonalVariations; // Different story versions for different seasons

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environmental Narrative")
    TArray<FString> WeatherVariations; // Different story versions for different weather
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStoryTriggered, FString, StoryTitle, FStoryElement, StoryData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryCompleted, FString, StoryTitle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEducationalMoment, FString, LessonTitle, TArray<FString>, KeyPoints);

UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AEnvironmentalStorytellingSystem : public AActor
{
    GENERATED_BODY()

public:
    AEnvironmentalStorytellingSystem();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    virtual void Tick(float DeltaTime) override;

    // Story Management Functions
    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    void RegisterStoryLocation(const FEnvironmentalNarrative& Narrative);

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    void TriggerStory(const FString& StoryTitle, AActor* TriggeringActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    void ActivateStoryByLocation(const FVector& Location, float SearchRadius = 1000.0f);

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    bool IsStoryAvailable(const FString& StoryTitle) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    TArray<FString> GetAvailableStories(const FVector& Location, float SearchRadius = 2000.0f) const;

    // Contextual Story Functions
    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    void UpdateContextualStories(const FVector& PlayerLocation, const FString& CurrentWeather, int32 PlayerSkillLevel);

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    TArray<FStoryElement> GetContextualStories(EStoryEventType EventType) const;

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    void TriggerHazardStory(const FString& HazardType, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Environmental Storytelling")
    void TriggerSeasonalStory(const FString& Season, const FVector& Location);

    // Educational Integration Functions
    UFUNCTION(BlueprintCallable, Category = "Educational Storytelling")
    void CreateEducationalMoment(const FString& LessonTitle, const TArray<FString>& KeyPoints, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Educational Storytelling")
    TArray<FString> GetEducationalTakeaways(const FString& StoryTitle) const;

    UFUNCTION(BlueprintCallable, Category = "Educational Storytelling")
    void TrackLearningProgress(AActor* Player, const FString& StoryTitle);

    // Environmental Placement Functions
    UFUNCTION(BlueprintCallable, Category = "Environmental Placement")
    void PlaceWeatheredEquipment(const FVector& Location, const FString& StoryContext);

    UFUNCTION(BlueprintCallable, Category = "Environmental Placement")
    void CreateChalkMarks(const FVector& Location, const FString& RouteInformation);

    UFUNCTION(BlueprintCallable, Category = "Environmental Placement")
    void PlaceMemorialMarker(const FVector& Location, const FStoryElement& MemorialStory);

    UFUNCTION(BlueprintCallable, Category = "Environmental Placement")
    void UpdateEnvironmentalClues(float DeltaTime);

    // Dynamic Story Generation
    UFUNCTION(BlueprintCallable, Category = "Dynamic Storytelling")
    FStoryElement GenerateWeatherStory(const FString& WeatherEvent, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Storytelling")
    FStoryElement GenerateRescueStory(const FVector& Location, const TArray<FString>& InvolvedClimbers);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Storytelling")
    void UpdateSeasonalNarratives();

    // Event Delegates
    UPROPERTY(BlueprintAssignable, Category = "Environmental Storytelling")
    FOnStoryTriggered OnStoryTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Environmental Storytelling")
    FOnStoryCompleted OnStoryCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Educational Storytelling")
    FOnEducationalMoment OnEducationalMoment;

protected:
    // Core Story Data
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Story System")
    TArray<FEnvironmentalNarrative> RegisteredNarratives;

    UPROPERTY(BlueprintReadOnly, Category = "Story System")
    TArray<FString> TriggeredStories;

    UPROPERTY(BlueprintReadOnly, Category = "Story System")
    TMap<AActor*, TArray<FString>> PlayerLearningProgress;

    // Environmental Elements
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* StoryAnchorPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* StoryTriggerVolume;

    // Story Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Configuration")
    TArray<FEnvironmentalNarrative> PreloadedNarratives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Configuration")
    bool bEnableContextualStories = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Configuration")
    bool bEnableDynamicGeneration = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Configuration")
    float StoryUpdateInterval = 10.0f; // seconds

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Configuration")
    int32 MaxActiveStories = 5;

    // Audio and Visual Assets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Assets")
    TMap<EStoryEventType, USoundCue*> StoryAudioCues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Assets")
    TMap<EStoryEventType, UStaticMesh*> StoryProps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Assets")
    TMap<FString, UTexture2D*> HistoricalImages;

    // Educational Integration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Educational Configuration")
    bool bTrackEducationalProgress = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Educational Configuration")
    float EducationalMomentCooldown = 300.0f; // 5 minutes between educational moments

    UPROPERTY(BlueprintReadOnly, Category = "Educational System")
    float LastEducationalMomentTime = 0.0f;

private:
    // Internal story management
    void ProcessStoryTriggers(float DeltaTime);
    void UpdateActiveNarratives();
    void CheckProximityTriggers(const FVector& PlayerLocation);
    void CheckContextualTriggers();
    
    // Story delivery systems
    void DeliverStory(const FStoryElement& Story, const FVector& Location, AActor* TriggeringActor);
    void PlayNarrationAudio(const FStoryElement& Story, const FVector& Location);
    void DisplayVisualStory(const FStoryElement& Story, AActor* TriggeringActor);
    void CreateEnvironmentalEvidence(const FStoryElement& Story, const FVector& Location);
    
    // Dynamic content generation
    FStoryElement CreateWeatherBasedStory(const FString& WeatherCondition, const FVector& Location);
    FStoryElement CreateHazardBasedStory(const FString& HazardType, const FVector& Location);
    FStoryElement CreateSeasonalStory(const FString& Season, const FVector& Location);
    
    // Utility functions
    bool EvaluateTriggerConditions(const FStoryTriggerCondition& Conditions, AActor* TriggeringActor);
    float CalculateStoryRelevance(const FEnvironmentalNarrative& Narrative, const FVector& PlayerLocation);
    FString GetSeasonalStoryVariation(const FEnvironmentalNarrative& Narrative);
    
    UPROPERTY()
    float StoryUpdateTimer = 0.0f;
    
    UPROPERTY()
    TArray<AActor*> TrackedPlayers;
    
    // Network replication
    UFUNCTION()
    void OnRep_RegisteredNarratives();
};

// Environmental Evidence Actor - represents physical story elements in the world
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API AEnvironmentalEvidence : public AActor
{
    GENERATED_BODY()

public:
    AEnvironmentalEvidence();

protected:
    virtual void BeginPlay() override;

public:
    // Evidence Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
    FStoryElement AssociatedStory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
    EStoryDeliveryMethod EvidenceType = EStoryDeliveryMethod::WeatheredEquipment;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
    bool bRequiresInteraction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WeatheringLevel = 0.5f; // 0 = new, 1 = heavily weathered

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
    bool bDegradesTooTime = true;

    // Interaction Functions
    UFUNCTION(BlueprintCallable, Category = "Evidence")
    void InteractWithEvidence(AActor* InteractingActor);

    UFUNCTION(BlueprintCallable, Category = "Evidence")
    void RevealStoryClue();

    UFUNCTION(BlueprintCallable, Category = "Evidence")
    void UpdateWeathering(float DeltaTime);

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* EvidenceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* InteractionSphere;

    // Audio component for story delivery
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* StoryAudioComponent;

    UPROPERTY()
    bool bHasBeenInteractedWith = false;

    UPROPERTY()
    float WeatheringTimer = 0.0f;

    UFUNCTION()
    void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

// Story Data Table Row Structure for external story content
USTRUCT(BlueprintType)
struct FStoryDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Data")
    FEnvironmentalNarrative Narrative;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Data")
    FString LocationTag; // Used to associate stories with specific locations

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Data")
    bool bIsMainStoryline = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Data")
    int32 ChapterNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Data")
    TArray<FString> UnlockRequirements;
};