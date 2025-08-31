#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "../Physics/WaterPhysicsComponent.h"
#include "../Physics/CaveEnvironmentPhysics.h"
#include "../Physics/WaterfallRappellingPhysics.h"
#include "../Physics/CaveDivingPhysics.h"
#include "../Physics/CaveLightingSystem.h"
#include "../Physics/WaterCavePhysicsManager.h"
#include "WaterCavePhysicsBlueprintLibrary.generated.h"

UCLASS()
class CLIMBINGGAME_API UWaterCavePhysicsBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Water Physics Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Water Physics", CallInEditor = true)
    static UWaterPhysicsComponent* GetWaterPhysicsAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Water Physics", CallInEditor = true)
    static bool IsLocationUnderwater(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Water Physics", CallInEditor = true)
    static float GetWaterDepthAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Water Physics", CallInEditor = true)
    static FVector GetWaterCurrentAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Water Physics", CallInEditor = true)
    static float CalculateBuoyancyForMass(float ObjectMass, float SubmergedVolume, EWaterType WaterType = EWaterType::Freshwater);

    UFUNCTION(BlueprintCallable, Category = "Water Physics", CallInEditor = true)
    static void ApplyWaterDrag(UPrimitiveComponent* Component, float DragCoefficient = 0.47f);

    // Cave Physics Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Cave Physics", CallInEditor = true)
    static UCaveEnvironmentPhysics* GetCavePhysicsAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Physics", CallInEditor = true)
    static bool IsLocationInCave(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Physics", CallInEditor = true)
    static EAirQuality GetAirQualityAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Physics", CallInEditor = true)
    static float GetCaveTemperatureAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Physics", CallInEditor = true)
    static bool IsCaveLocationSafeForBreathing(UObject* WorldContext, const FVector& Location);

    // Waterfall Physics Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Waterfall Physics", CallInEditor = true)
    static UWaterfallRappellingPhysics* GetWaterfallPhysicsAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Waterfall Physics", CallInEditor = true)
    static float GetWaterPressureAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Waterfall Physics", CallInEditor = true)
    static ERappellingCondition GetRappellingCondition(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Waterfall Physics", CallInEditor = true)
    static bool IsRappellingSafeAtLocation(UObject* WorldContext, const FVector& StartLocation, const FVector& EndLocation);

    // Cave Diving Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Cave Diving", CallInEditor = true)
    static UCaveDivingPhysics* GetCaveDivingPhysicsAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Diving", CallInEditor = true)
    static EDecompressionStatus GetDecompressionStatus(UObject* WorldContext, AActor* Diver);

    UFUNCTION(BlueprintCallable, Category = "Cave Diving", CallInEditor = true)
    static float CalculateDiveTime(float MaxDepth, float BottomTime);

    UFUNCTION(BlueprintCallable, Category = "Cave Diving", CallInEditor = true)
    static bool IsEmergencyAscentRequired(UObject* WorldContext, AActor* Diver);

    UFUNCTION(BlueprintCallable, Category = "Cave Diving", CallInEditor = true)
    static float GetDistanceToExit(UObject* WorldContext, const FVector& DiverLocation);

    // Lighting Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Cave Lighting", CallInEditor = true)
    static UCaveLightingSystem* GetLightingSystemAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting", CallInEditor = true)
    static float GetVisibilityRangeAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting", CallInEditor = true)
    static EVisibilityLevel GetVisibilityLevel(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Cave Lighting", CallInEditor = true)
    static bool CanSeeTarget(UObject* WorldContext, const FVector& ObserverLocation, const FVector& TargetLocation);

    // Rope Physics Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Rope Physics", CallInEditor = true)
    static void ProcessRopeInWater(UAdvancedRopeComponent* Rope, UWaterPhysicsComponent* WaterPhysics);

    UFUNCTION(BlueprintCallable, Category = "Rope Physics", CallInEditor = true)
    static float GetRopeSubmersionPercentage(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Physics", CallInEditor = true)
    static bool IsRopeWaterlogged(UAdvancedRopeComponent* Rope);

    UFUNCTION(BlueprintCallable, Category = "Rope Physics", CallInEditor = true)
    static void SetRopeEmergencyAscentMode(UAdvancedRopeComponent* Rope, bool bEmergencyMode);

    // Environment Management Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Environment Management", CallInEditor = true)
    static EWaterCaveEnvironment GetEnvironmentAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Environment Management", CallInEditor = true)
    static AWaterCavePhysicsManager* GetPhysicsManager(UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "Environment Management", CallInEditor = true)
    static FIntegratedPhysicsState GetActorPhysicsState(UObject* WorldContext, AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Environment Management", CallInEditor = true)
    static void SetGlobalPhysicsQuality(UObject* WorldContext, EFluidDynamicsLOD QualityLevel);

    // Safety Assessment Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Safety Assessment", CallInEditor = true)
    static float GetLocationSafetyRating(UObject* WorldContext, const FVector& Location, AActor* ForActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Safety Assessment", CallInEditor = true)
    static TArray<FString> GetActiveHazardsAtLocation(UObject* WorldContext, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Safety Assessment", CallInEditor = true)
    static bool IsEmergencyEvacuationNeeded(UObject* WorldContext, AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Safety Assessment", CallInEditor = true)
    static TArray<FVector> GetNearestSafeLocations(UObject* WorldContext, const FVector& DangerLocation, int32 MaxLocations = 3);

    // Performance Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Performance", CallInEditor = true)
    static float GetFluidPhysicsFrameTime(UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "Performance", CallInEditor = true)
    static bool IsPerformanceWithinTarget(UObject* WorldContext, float TargetFrameRate = 60.0f);

    UFUNCTION(BlueprintCallable, Category = "Performance", CallInEditor = true)
    static void OptimizeFluidSystemsForFrameRate(UObject* WorldContext, float TargetFrameRate);

    UFUNCTION(BlueprintCallable, Category = "Performance", CallInEditor = true)
    static FString GetPerformanceReport(UObject* WorldContext);

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Utilities", CallInEditor = true)
    static float CalculateHypothermiaRisk(float WaterTemperature, float ExposureTime);

    UFUNCTION(BlueprintCallable, Category = "Utilities", CallInEditor = true)
    static float CalculateDecompressionTime(float MaxDepth, float BottomTime);

    UFUNCTION(BlueprintCallable, Category = "Utilities", CallInEditor = true)
    static bool IsEquipmentSafeForEnvironment(class AClimbingToolBase* Equipment, EWaterCaveEnvironment Environment);

    UFUNCTION(BlueprintCallable, Category = "Utilities", CallInEditor = true)
    static float GetEnvironmentalStressLevel(UObject* WorldContext, AActor* Actor);

protected:
    // Helper functions for Blueprint library
    static AWaterCavePhysicsManager* FindPhysicsManager(UObject* WorldContext);
    static UWaterPhysicsComponent* FindWaterPhysicsAtLocation(UObject* WorldContext, const FVector& Location);
    static UCaveEnvironmentPhysics* FindCavePhysicsAtLocation(UObject* WorldContext, const FVector& Location);
    static UWaterfallRappellingPhysics* FindWaterfallPhysicsAtLocation(UObject* WorldContext, const FVector& Location);
    static UCaveDivingPhysics* FindCaveDivingPhysicsAtLocation(UObject* WorldContext, const FVector& Location);
    static UCaveLightingSystem* FindLightingSystemAtLocation(UObject* WorldContext, const FVector& Location);
};