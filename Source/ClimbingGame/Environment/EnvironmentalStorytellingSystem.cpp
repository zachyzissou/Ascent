#include "EnvironmentalStorytellingSystem.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AEnvironmentalStorytellingSystem::AEnvironmentalStorytellingSystem()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;

    // Create story anchor point
    StoryAnchorPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StoryAnchorPoint"));
    RootComponent = StoryAnchorPoint;

    // Create trigger volume for proximity-based stories
    StoryTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("StoryTriggerVolume"));
    StoryTriggerVolume->SetupAttachment(RootComponent);
    StoryTriggerVolume->SetBoxExtent(FVector(2000.0f, 2000.0f, 1000.0f)); // Large coverage area
    StoryTriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    StoryTriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    StoryTriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    StoryTriggerVolume->SetVisibility(false);

    // Initialize default configuration
    bEnableContextualStories = true;
    bEnableDynamicGeneration = true;
    StoryUpdateInterval = 10.0f;
    MaxActiveStories = 5;
    bTrackEducationalProgress = true;
    EducationalMomentCooldown = 300.0f;
}

void AEnvironmentalStorytellingSystem::BeginPlay()
{
    Super::BeginPlay();

    // Register preloaded narratives
    for (const FEnvironmentalNarrative& Narrative : PreloadedNarratives)
    {
        RegisterStoryLocation(Narrative);
    }

    // Start story update timer
    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_StoryUpdate,
            [this]() { ProcessStoryTriggers(StoryUpdateInterval); },
            StoryUpdateInterval,
            true
        );
    }

    UE_LOG(LogTemp, Log, TEXT("Environmental Storytelling System initialized with %d narratives"), 
           RegisteredNarratives.Num());
}

void AEnvironmentalStorytellingSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    Super::EndPlay(EndPlayReason);
}

void AEnvironmentalStorytellingSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AEnvironmentalStorytellingSystem, RegisteredNarratives);
}

void AEnvironmentalStorytellingSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority())
    {
        UpdateEnvironmentalClues(DeltaTime);
    }
}

void AEnvironmentalStorytellingSystem::RegisterStoryLocation(const FEnvironmentalNarrative& Narrative)
{
    if (!HasAuthority()) return;

    // Check if story already exists
    bool bAlreadyExists = RegisteredNarratives.ContainsByPredicate([&Narrative](const FEnvironmentalNarrative& ExistingNarrative)
    {
        return ExistingNarrative.MainStory.StoryTitle == Narrative.MainStory.StoryTitle;
    });

    if (!bAlreadyExists)
    {
        RegisteredNarratives.Add(Narrative);
        UE_LOG(LogTemp, Log, TEXT("Registered story: %s"), *Narrative.MainStory.StoryTitle);
    }
}

void AEnvironmentalStorytellingSystem::TriggerStory(const FString& StoryTitle, AActor* TriggeringActor)
{
    if (!HasAuthority()) return;

    // Find the narrative
    FEnvironmentalNarrative* FoundNarrative = RegisteredNarratives.FindByPredicate([&StoryTitle](const FEnvironmentalNarrative& Narrative)
    {
        return Narrative.MainStory.StoryTitle == StoryTitle;
    });

    if (!FoundNarrative || !FoundNarrative->bIsActive)
    {
        return;
    }

    // Check if already triggered and set to only trigger once
    if (FoundNarrative->TriggerCondition.bOnlyTriggerOnce && TriggeredStories.Contains(StoryTitle))
    {
        return;
    }

    // Evaluate trigger conditions
    if (!EvaluateTriggerConditions(FoundNarrative->TriggerCondition, TriggeringActor))
    {
        return;
    }

    // Check probability
    if (FMath::RandRange(0.0f, 1.0f) > FoundNarrative->TriggerCondition.TriggerProbability)
    {
        return;
    }

    // Deliver the story
    DeliverStory(FoundNarrative->MainStory, FoundNarrative->WorldLocation, TriggeringActor);

    // Mark as triggered
    TriggeredStories.AddUnique(StoryTitle);

    // Broadcast event
    OnStoryTriggered.Broadcast(StoryTitle, FoundNarrative->MainStory);

    // Track educational progress if applicable
    if (bTrackEducationalProgress && TriggeringActor)
    {
        TrackLearningProgress(TriggeringActor, StoryTitle);
    }

    // Trigger educational moment if story has takeaways
    if (FoundNarrative->MainStory.EducationalTakeaways.Num() > 0)
    {
        CreateEducationalMoment(
            FoundNarrative->MainStory.StoryTitle, 
            FoundNarrative->MainStory.EducationalTakeaways, 
            FoundNarrative->WorldLocation
        );
    }

    UE_LOG(LogTemp, Log, TEXT("Story triggered: %s"), *StoryTitle);
}

void AEnvironmentalStorytellingSystem::ActivateStoryByLocation(const FVector& Location, float SearchRadius)
{
    TArray<FString> NearbyStories;

    for (const FEnvironmentalNarrative& Narrative : RegisteredNarratives)
    {
        float Distance = FVector::Dist(Location, Narrative.WorldLocation);
        if (Distance <= SearchRadius && Narrative.bIsActive)
        {
            NearbyStories.Add(Narrative.MainStory.StoryTitle);
        }
    }

    // Trigger the most relevant nearby story
    if (NearbyStories.Num() > 0)
    {
        // Find the closest story
        FString BestStory;
        float ClosestDistance = SearchRadius + 1.0f;

        for (const FString& StoryTitle : NearbyStories)
        {
            FEnvironmentalNarrative* Narrative = RegisteredNarratives.FindByPredicate([&StoryTitle](const FEnvironmentalNarrative& N)
            {
                return N.MainStory.StoryTitle == StoryTitle;
            });

            if (Narrative)
            {
                float Distance = FVector::Dist(Location, Narrative->WorldLocation);
                if (Distance < ClosestDistance)
                {
                    ClosestDistance = Distance;
                    BestStory = StoryTitle;
                }
            }
        }

        if (!BestStory.IsEmpty())
        {
            TriggerStory(BestStory, nullptr);
        }
    }
}

bool AEnvironmentalStorytellingSystem::IsStoryAvailable(const FString& StoryTitle) const
{
    const FEnvironmentalNarrative* FoundNarrative = RegisteredNarratives.FindByPredicate([&StoryTitle](const FEnvironmentalNarrative& Narrative)
    {
        return Narrative.MainStory.StoryTitle == StoryTitle;
    });

    if (!FoundNarrative || !FoundNarrative->bIsActive)
    {
        return false;
    }

    // Check if already triggered and only triggers once
    if (FoundNarrative->TriggerCondition.bOnlyTriggerOnce && TriggeredStories.Contains(StoryTitle))
    {
        return false;
    }

    return true;
}

TArray<FString> AEnvironmentalStorytellingSystem::GetAvailableStories(const FVector& Location, float SearchRadius) const
{
    TArray<FString> AvailableStories;

    for (const FEnvironmentalNarrative& Narrative : RegisteredNarratives)
    {
        if (!Narrative.bIsActive) continue;

        float Distance = FVector::Dist(Location, Narrative.WorldLocation);
        if (Distance <= SearchRadius)
        {
            if (IsStoryAvailable(Narrative.MainStory.StoryTitle))
            {
                AvailableStories.Add(Narrative.MainStory.StoryTitle);
            }
        }
    }

    return AvailableStories;
}

void AEnvironmentalStorytellingSystem::UpdateContextualStories(const FVector& PlayerLocation, const FString& CurrentWeather, int32 PlayerSkillLevel)
{
    if (!bEnableContextualStories) return;

    CheckProximityTriggers(PlayerLocation);
    CheckContextualTriggers();

    // Generate dynamic stories based on context
    if (bEnableDynamicGeneration)
    {
        // Weather-based story generation
        if (!CurrentWeather.IsEmpty() && CurrentWeather != TEXT("Clear"))
        {
            FStoryElement WeatherStory = CreateWeatherBasedStory(CurrentWeather, PlayerLocation);
            if (!WeatherStory.StoryTitle.IsEmpty())
            {
                FEnvironmentalNarrative NewNarrative;
                NewNarrative.MainStory = WeatherStory;
                NewNarrative.WorldLocation = PlayerLocation;
                NewNarrative.TriggerCondition.TriggerType = EStoryTriggerType::WeatherCondition;
                
                RegisterStoryLocation(NewNarrative);
            }
        }
    }
}

TArray<FStoryElement> AEnvironmentalStorytellingSystem::GetContextualStories(EStoryEventType EventType) const
{
    TArray<FStoryElement> ContextualStories;

    for (const FEnvironmentalNarrative& Narrative : RegisteredNarratives)
    {
        if (Narrative.MainStory.EventType == EventType && Narrative.bIsActive)
        {
            ContextualStories.Add(Narrative.MainStory);
        }
    }

    return ContextualStories;
}

void AEnvironmentalStorytellingSystem::TriggerHazardStory(const FString& HazardType, const FVector& Location)
{
    if (!bEnableDynamicGeneration) return;

    FStoryElement HazardStory = CreateHazardBasedStory(HazardType, Location);
    if (!HazardStory.StoryTitle.IsEmpty())
    {
        FEnvironmentalNarrative HazardNarrative;
        HazardNarrative.MainStory = HazardStory;
        HazardNarrative.WorldLocation = Location;
        HazardNarrative.TriggerCondition.TriggerType = EStoryTriggerType::HazardEncounter;
        HazardNarrative.TriggerCondition.TriggerRadius = 1000.0f;

        RegisterStoryLocation(HazardNarrative);
        TriggerStory(HazardStory.StoryTitle, nullptr);
    }
}

void AEnvironmentalStorytellingSystem::TriggerSeasonalStory(const FString& Season, const FVector& Location)
{
    if (!bEnableDynamicGeneration) return;

    FStoryElement SeasonalStory = CreateSeasonalStory(Season, Location);
    if (!SeasonalStory.StoryTitle.IsEmpty())
    {
        FEnvironmentalNarrative SeasonalNarrative;
        SeasonalNarrative.MainStory = SeasonalStory;
        SeasonalNarrative.WorldLocation = Location;
        SeasonalNarrative.TriggerCondition.TriggerType = EStoryTriggerType::SeasonalEvent;

        RegisterStoryLocation(SeasonalNarrative);
        TriggerStory(SeasonalStory.StoryTitle, nullptr);
    }
}

void AEnvironmentalStorytellingSystem::CreateEducationalMoment(const FString& LessonTitle, const TArray<FString>& KeyPoints, const FVector& Location)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Check cooldown
    if (CurrentTime - LastEducationalMomentTime < EducationalMomentCooldown)
    {
        return;
    }

    LastEducationalMomentTime = CurrentTime;
    OnEducationalMoment.Broadcast(LessonTitle, KeyPoints);

    UE_LOG(LogTemp, Log, TEXT("Educational moment created: %s"), *LessonTitle);
}

TArray<FString> AEnvironmentalStorytellingSystem::GetEducationalTakeaways(const FString& StoryTitle) const
{
    const FEnvironmentalNarrative* FoundNarrative = RegisteredNarratives.FindByPredicate([&StoryTitle](const FEnvironmentalNarrative& Narrative)
    {
        return Narrative.MainStory.StoryTitle == StoryTitle;
    });

    if (FoundNarrative)
    {
        return FoundNarrative->MainStory.EducationalTakeaways;
    }

    return TArray<FString>();
}

void AEnvironmentalStorytellingSystem::TrackLearningProgress(AActor* Player, const FString& StoryTitle)
{
    if (!Player || !bTrackEducationalProgress) return;

    if (!PlayerLearningProgress.Contains(Player))
    {
        PlayerLearningProgress.Add(Player, TArray<FString>());
    }

    PlayerLearningProgress[Player].AddUnique(StoryTitle);

    UE_LOG(LogTemp, Log, TEXT("Tracked learning progress for %s: %s"), 
           *Player->GetName(), *StoryTitle);
}

void AEnvironmentalStorytellingSystem::PlaceWeatheredEquipment(const FVector& Location, const FString& StoryContext)
{
    // Spawn weathered equipment actor
    AEnvironmentalEvidence* Equipment = GetWorld()->SpawnActor<AEnvironmentalEvidence>(
        AEnvironmentalEvidence::StaticClass(), 
        Location, 
        FRotator::ZeroRotator
    );

    if (Equipment)
    {
        Equipment->EvidenceType = EStoryDeliveryMethod::WeatheredEquipment;
        Equipment->AssociatedStory.StoryDescription = StoryContext;
        Equipment->WeatheringLevel = FMath::RandRange(0.6f, 0.9f); // Heavily weathered

        UE_LOG(LogTemp, Log, TEXT("Placed weathered equipment at %s"), *Location.ToString());
    }
}

void AEnvironmentalStorytellingSystem::CreateChalkMarks(const FVector& Location, const FString& RouteInformation)
{
    AEnvironmentalEvidence* ChalkMarks = GetWorld()->SpawnActor<AEnvironmentalEvidence>(
        AEnvironmentalEvidence::StaticClass(), 
        Location, 
        FRotator::ZeroRotator
    );

    if (ChalkMarks)
    {
        ChalkMarks->EvidenceType = EStoryDeliveryMethod::EnvironmentalClues;
        ChalkMarks->AssociatedStory.StoryDescription = RouteInformation;
        ChalkMarks->AssociatedStory.EventType = EStoryEventType::RouteHistory;
        ChalkMarks->WeatheringLevel = 0.3f; // Recent marks

        UE_LOG(LogTemp, Log, TEXT("Created chalk marks at %s"), *Location.ToString());
    }
}

void AEnvironmentalStorytellingSystem::PlaceMemorialMarker(const FVector& Location, const FStoryElement& MemorialStory)
{
    AEnvironmentalEvidence* Memorial = GetWorld()->SpawnActor<AEnvironmentalEvidence>(
        AEnvironmentalEvidence::StaticClass(), 
        Location, 
        FRotator::ZeroRotator
    );

    if (Memorial)
    {
        Memorial->EvidenceType = EStoryDeliveryMethod::InteractiveObject;
        Memorial->AssociatedStory = MemorialStory;
        Memorial->WeatheringLevel = 0.5f; // Moderately weathered
        Memorial->bRequiresInteraction = true;

        UE_LOG(LogTemp, Log, TEXT("Placed memorial marker: %s"), *MemorialStory.StoryTitle);
    }
}

void AEnvironmentalStorytellingSystem::UpdateEnvironmentalClues(float DeltaTime)
{
    // Update weathering and degradation of environmental evidence
    TArray<AActor*> EvidenceActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnvironmentalEvidence::StaticClass(), EvidenceActors);

    for (AActor* Actor : EvidenceActors)
    {
        AEnvironmentalEvidence* Evidence = Cast<AEnvironmentalEvidence>(Actor);
        if (Evidence)
        {
            Evidence->UpdateWeathering(DeltaTime);
        }
    }
}

FStoryElement AEnvironmentalStorytellingSystem::GenerateWeatherStory(const FString& WeatherEvent, const FVector& Location)
{
    return CreateWeatherBasedStory(WeatherEvent, Location);
}

FStoryElement AEnvironmentalStorytellingSystem::GenerateRescueStory(const FVector& Location, const TArray<FString>& InvolvedClimbers)
{
    FStoryElement RescueStory;
    RescueStory.StoryTitle = TEXT("Emergency Rescue Operation");
    RescueStory.EventType = EStoryEventType::RescueStory;
    RescueStory.DeliveryMethod = EStoryDeliveryMethod::AudioNarration;
    RescueStory.EmotionalImpact = 0.8f;
    RescueStory.HistoricalDate = FDateTime::Now();

    FString ClimberList = FString::Join(InvolvedClimbers, TEXT(", "));
    RescueStory.StoryDescription = FString::Printf(
        TEXT("A complex rescue operation took place at this location involving climbers: %s. The challenging terrain and weather conditions required coordinated efforts from multiple rescue teams."),
        *ClimberList
    );

    RescueStory.EducationalTakeaways.Add(TEXT("Always carry emergency communication devices"));
    RescueStory.EducationalTakeaways.Add(TEXT("Inform others of your climbing plans and expected return"));
    RescueStory.EducationalTakeaways.Add(TEXT("Know basic first aid and rescue techniques"));
    RescueStory.LocationSignificance = TEXT("Site of successful emergency rescue demonstration");

    return RescueStory;
}

void AEnvironmentalStorytellingSystem::UpdateSeasonalNarratives()
{
    // Update all narratives with seasonal variations
    for (FEnvironmentalNarrative& Narrative : RegisteredNarratives)
    {
        FString SeasonalVariation = GetSeasonalStoryVariation(Narrative);
        if (!SeasonalVariation.IsEmpty())
        {
            // Update story description with seasonal context
            Narrative.MainStory.StoryDescription = SeasonalVariation;
        }
    }
}

void AEnvironmentalStorytellingSystem::ProcessStoryTriggers(float DeltaTime)
{
    StoryUpdateTimer += DeltaTime;

    if (StoryUpdateTimer >= StoryUpdateInterval)
    {
        UpdateActiveNarratives();
        UpdateSeasonalNarratives();
        StoryUpdateTimer = 0.0f;
    }

    // Get all players in the world
    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), Players);

    for (AActor* Player : Players)
    {
        APawn* Pawn = Cast<APawn>(Player);
        if (Pawn && Pawn->IsPlayerControlled())
        {
            CheckProximityTriggers(Player->GetActorLocation());
        }
    }
}

void AEnvironmentalStorytellingSystem::UpdateActiveNarratives()
{
    // Ensure we don't have too many active stories
    if (RegisteredNarratives.Num() > MaxActiveStories)
    {
        // Remove least relevant stories (simplified logic)
        RegisteredNarratives.RemoveAt(RegisteredNarratives.Num() - 1);
    }
}

void AEnvironmentalStorytellingSystem::CheckProximityTriggers(const FVector& PlayerLocation)
{
    for (const FEnvironmentalNarrative& Narrative : RegisteredNarratives)
    {
        if (!Narrative.bIsActive) continue;
        if (Narrative.TriggerCondition.TriggerType != EStoryTriggerType::Proximity) continue;

        float Distance = FVector::Dist(PlayerLocation, Narrative.WorldLocation);
        if (Distance <= Narrative.TriggerCondition.TriggerRadius)
        {
            TriggerStory(Narrative.MainStory.StoryTitle, nullptr);
        }
    }
}

void AEnvironmentalStorytellingSystem::CheckContextualTriggers()
{
    // Check for contextual triggers like weather conditions, time of day, etc.
    // This would integrate with weather and time systems
}

void AEnvironmentalStorytellingSystem::DeliverStory(const FStoryElement& Story, const FVector& Location, AActor* TriggeringActor)
{
    switch (Story.DeliveryMethod)
    {
    case EStoryDeliveryMethod::AudioNarration:
        PlayNarrationAudio(Story, Location);
        break;
    case EStoryDeliveryMethod::VisualCutscene:
        DisplayVisualStory(Story, TriggeringActor);
        break;
    case EStoryDeliveryMethod::EnvironmentalClues:
        CreateEnvironmentalEvidence(Story, Location);
        break;
    case EStoryDeliveryMethod::InteractiveObject:
        PlaceMemorialMarker(Location, Story);
        break;
    case EStoryDeliveryMethod::WeatheredEquipment:
        PlaceWeatheredEquipment(Location, Story.StoryDescription);
        break;
    default:
        // Default text display
        UE_LOG(LogTemp, Warning, TEXT("Story: %s\n%s"), *Story.StoryTitle, *Story.StoryDescription);
        break;
    }
}

void AEnvironmentalStorytellingSystem::PlayNarrationAudio(const FStoryElement& Story, const FVector& Location)
{
    if (Story.NarrationAudio)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Story.NarrationAudio, Location);
    }
    else if (StoryAudioCues.Contains(Story.EventType))
    {
        USoundCue* AudioCue = StoryAudioCues[Story.EventType];
        if (AudioCue)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), AudioCue, Location);
        }
    }
}

void AEnvironmentalStorytellingSystem::DisplayVisualStory(const FStoryElement& Story, AActor* TriggeringActor)
{
    // This would trigger UI elements or cutscenes
    // Implementation depends on UI system
    UE_LOG(LogTemp, Log, TEXT("Displaying visual story: %s"), *Story.StoryTitle);
}

void AEnvironmentalStorytellingSystem::CreateEnvironmentalEvidence(const FStoryElement& Story, const FVector& Location)
{
    AEnvironmentalEvidence* Evidence = GetWorld()->SpawnActor<AEnvironmentalEvidence>(
        AEnvironmentalEvidence::StaticClass(), 
        Location, 
        FRotator::ZeroRotator
    );

    if (Evidence)
    {
        Evidence->AssociatedStory = Story;
        Evidence->EvidenceType = Story.DeliveryMethod;
    }
}

bool AEnvironmentalStorytellingSystem::EvaluateTriggerConditions(const FStoryTriggerCondition& Conditions, AActor* TriggeringActor)
{
    switch (Conditions.TriggerType)
    {
    case EStoryTriggerType::Proximity:
        return true; // Proximity already checked in calling function
        
    case EStoryTriggerType::TeamSize:
        {
            TArray<AActor*> Players;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), Players);
            int32 PlayerCount = 0;
            for (AActor* Player : Players)
            {
                APawn* Pawn = Cast<APawn>(Player);
                if (Pawn && Pawn->IsPlayerControlled())
                {
                    PlayerCount++;
                }
            }
            return PlayerCount >= Conditions.RequiredTeamSize;
        }
        
    case EStoryTriggerType::SkillLevel:
        // Would need to query player skill system
        return true; // Simplified - always true for now
        
    default:
        return true;
    }
}

FStoryElement AEnvironmentalStorytellingSystem::CreateWeatherBasedStory(const FString& WeatherCondition, const FVector& Location)
{
    FStoryElement WeatherStory;
    WeatherStory.EventType = EStoryEventType::WeatherEvent;
    WeatherStory.DeliveryMethod = EStoryDeliveryMethod::AudioNarration;
    WeatherStory.HistoricalDate = FDateTime::Now();
    WeatherStory.EmotionalImpact = 0.6f;

    if (WeatherCondition == TEXT("Storm") || WeatherCondition == TEXT("HeavyRain"))
    {
        WeatherStory.StoryTitle = TEXT("The Storm of '98");
        WeatherStory.StoryDescription = TEXT("In 1998, a sudden storm caught a climbing party unprepared at this location. Their quick thinking and emergency shelter construction saved their lives, but the experience taught valuable lessons about weather monitoring and emergency preparedness.");
        WeatherStory.EducationalTakeaways.Add(TEXT("Always check weather forecasts before climbing"));
        WeatherStory.EducationalTakeaways.Add(TEXT("Carry emergency shelter materials"));
        WeatherStory.EducationalTakeaways.Add(TEXT("Know how to recognize approaching storms"));
        WeatherStory.LocationSignificance = TEXT("Site where emergency weather procedures were successfully employed");
    }
    else if (WeatherCondition == TEXT("Snow") || WeatherCondition == TEXT("Blizzard"))
    {
        WeatherStory.StoryTitle = TEXT("Winter Survival");
        WeatherStory.StoryDescription = TEXT("During an unexpected winter storm, climbers were forced to dig emergency snow caves and wait out dangerous conditions. Their survival demonstrated the importance of winter climbing skills and proper equipment.");
        WeatherStory.EducationalTakeaways.Add(TEXT("Learn snow shelter construction techniques"));
        WeatherStory.EducationalTakeaways.Add(TEXT("Carry winter emergency equipment"));
        WeatherStory.EducationalTakeaways.Add(TEXT("Understand hypothermia prevention"));
    }
    else if (WeatherCondition == TEXT("Fog"))
    {
        WeatherStory.StoryTitle = TEXT("Lost in the Mist");
        WeatherStory.StoryDescription = TEXT("Dense fog descended rapidly, reducing visibility to mere meters. Climbers had to use compass navigation and rope team techniques to safely find their way back to known terrain."));
        WeatherStory.EducationalTakeaways.Add(TEXT("Learn compass and GPS navigation"));
        WeatherStory.EducationalTakeaways.Add(TEXT("Practice rope team travel in low visibility"));
        WeatherStory.EducationalTakeaways.Add(TEXT("Carry backup navigation tools"));
    }

    return WeatherStory;
}

FStoryElement AEnvironmentalStorytellingSystem::CreateHazardBasedStory(const FString& HazardType, const FVector& Location)
{
    FStoryElement HazardStory;
    HazardStory.EventType = EStoryEventType::AccidentSite;
    HazardStory.DeliveryMethod = EStoryDeliveryMethod::InteractiveObject;
    HazardStory.EmotionalImpact = 0.7f;
    HazardStory.HistoricalDate = FDateTime::Now();

    if (HazardType == TEXT("Rockfall"))
    {
        HazardStory.StoryTitle = TEXT("The Rockfall Warning");
        HazardStory.StoryDescription = TEXT("A rockfall event at this location led to improved safety protocols and hazard assessment techniques. The incident highlighted the importance of helmet use and hazard recognition skills.");
        HazardStory.EducationalTakeaways.Add(TEXT("Always wear helmets in rockfall zones"));
        HazardStory.EducationalTakeaways.Add(TEXT("Learn to recognize unstable rock"));
        HazardStory.EducationalTakeaways.Add(TEXT("Use proper communication calls"));
    }
    else if (HazardType == TEXT("Avalanche"))
    {
        HazardStory.StoryTitle = TEXT("Avalanche Education");
        HazardStory.StoryDescription = TEXT("This area experienced an avalanche that demonstrated the importance of snow safety education, proper equipment, and rescue techniques. The event led to improved avalanche awareness programs.");
        HazardStory.EducationalTakeaways.Add(TEXT("Learn avalanche safety basics"));
        HazardStory.EducationalTakeaways.Add(TEXT("Carry beacon, probe, and shovel"));
        HazardStory.EducationalTakeaways.Add(TEXT("Practice rescue scenarios"));
    }

    return HazardStory;
}

FStoryElement AEnvironmentalStorytellingSystem::CreateSeasonalStory(const FString& Season, const FVector& Location)
{
    FStoryElement SeasonalStory;
    SeasonalStory.EventType = EStoryEventType::HistoricalAscent;
    SeasonalStory.DeliveryMethod = EStoryDeliveryMethod::EnvironmentalClues;
    SeasonalStory.EmotionalImpact = 0.4f;

    if (Season == TEXT("Spring"))
    {
        SeasonalStory.StoryTitle = TEXT("Spring Awakening");
        SeasonalStory.StoryDescription = TEXT("Spring brings unique challenges and opportunities to this area. Melting snow creates temporary waterfalls, while freeze-thaw cycles can destabilize rock formations.");
        SeasonalStory.EducationalTakeaways.Add(TEXT("Monitor freeze-thaw conditions"));
        SeasonalStory.EducationalTakeaways.Add(TEXT("Be aware of spring snowpack instability"));
    }
    else if (Season == TEXT("Winter"))
    {
        SeasonalStory.StoryTitle = TEXT("Winter Transformation");
        SeasonalStory.StoryDescription = TEXT("Winter transforms this landscape into a different climbing environment entirely. Ice formation, snow accumulation, and extreme cold create unique challenges and opportunities."));
        SeasonalStory.EducationalTakeaways.Add(TEXT("Learn ice climbing techniques"));
        SeasonalStory.EducationalTakeaways.Add(TEXT("Understand cold weather physiology"));
    }

    return SeasonalStory;
}

FString AEnvironmentalStorytellingSystem::GetSeasonalStoryVariation(const FEnvironmentalNarrative& Narrative)
{
    // Return seasonal variation if available
    if (Narrative.SeasonalVariations.Num() > 0)
    {
        // Simple logic - could be expanded to check actual season
        return Narrative.SeasonalVariations[0];
    }
    
    return FString();
}

void AEnvironmentalStorytellingSystem::OnRep_RegisteredNarratives()
{
    // Handle replication of narratives to clients
    UE_LOG(LogTemp, Log, TEXT("Received %d narratives from server"), RegisteredNarratives.Num());
}

// Environmental Evidence Implementation

AEnvironmentalEvidence::AEnvironmentalEvidence()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create evidence mesh
    EvidenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EvidenceMesh"));
    RootComponent = EvidenceMesh;
    EvidenceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Create interaction sphere
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(200.0f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Create audio component for story delivery
    StoryAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("StoryAudioComponent"));
    StoryAudioComponent->SetupAttachment(RootComponent);
    StoryAudioComponent->bAutoActivate = false;

    // Default settings
    bRequiresInteraction = true;
    WeatheringLevel = 0.5f;
    bDegradesTooTime = true;
}

void AEnvironmentalEvidence::BeginPlay()
{
    Super::BeginPlay();

    // Bind interaction events
    if (InteractionSphere)
    {
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnvironmentalEvidence::OnInteractionSphereBeginOverlap);
    }
}

void AEnvironmentalEvidence::InteractWithEvidence(AActor* InteractingActor)
{
    if (bHasBeenInteractedWith && bRequiresInteraction) return;

    RevealStoryClue();
    bHasBeenInteractedWith = true;

    UE_LOG(LogTemp, Log, TEXT("Evidence interacted with: %s"), *AssociatedStory.StoryTitle);
}

void AEnvironmentalEvidence::RevealStoryClue()
{
    // Play associated audio if available
    if (AssociatedStory.NarrationAudio && StoryAudioComponent)
    {
        StoryAudioComponent->SetSound(AssociatedStory.NarrationAudio);
        StoryAudioComponent->Play();
    }

    // Could trigger UI elements, visual effects, etc.
    UE_LOG(LogTemp, Warning, TEXT("Story Clue: %s\n%s"), 
           *AssociatedStory.StoryTitle, *AssociatedStory.StoryDescription);
}

void AEnvironmentalEvidence::UpdateWeathering(float DeltaTime)
{
    if (!bDegradesTooTime) return;

    WeatheringTimer += DeltaTime;

    // Degrade over time (simplified)
    float DegradationRate = 0.00001f; // Very slow degradation
    WeatheringLevel = FMath::Min(WeatheringLevel + DegradationRate * DeltaTime, 1.0f);

    // Apply weathering effects to mesh (would need material parameters in full implementation)
}

void AEnvironmentalEvidence::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (Pawn && Pawn->IsPlayerControlled())
    {
        if (!bRequiresInteraction)
        {
            // Automatically reveal story for non-interactive evidence
            InteractWithEvidence(OtherActor);
        }
        else
        {
            // Show interaction prompt (would be handled by UI system)
            UE_LOG(LogTemp, Log, TEXT("Evidence available for interaction"));
        }
    }
}