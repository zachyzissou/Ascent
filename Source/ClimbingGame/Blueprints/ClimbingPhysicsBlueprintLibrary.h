#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "../Physics/AdvancedRopeComponent.h"
#include "../Player/AdvancedClimbingComponent.h"
#include "../Physics/AnchorSystem.h"
#include "../Physics/FallMechanicsSystem.h"
#include "../Physics/ClimbingPerformanceManager.h"
#include "ClimbingPhysicsBlueprintLibrary.generated.h"

/**
 * Blueprint Function Library for ClimbingGame Physics Systems
 * Provides Blueprint-accessible functions for all major physics components
 */
UCLASS()
class CLIMBINGGAME_API UClimbingPhysicsBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ===== ROPE PHYSICS FUNCTIONS =====
    
    /** Create and deploy a rope between two anchor points */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Rope", CallInEditor = true, 
              meta = (DisplayName = "Deploy Rope Between Anchors"))
    static bool DeployRopeBetweenAnchors(UAdvancedRopeComponent* RopeComponent, 
                                       AActor* StartAnchor, AActor* EndAnchor);
    
    /** Get current tension on a rope in Newtons */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Get Rope Tension"))
    static float GetRopeTension(UAdvancedRopeComponent* RopeComponent);
    
    /** Get rope elongation as percentage */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Get Rope Elongation"))
    static float GetRopeElongation(UAdvancedRopeComponent* RopeComponent);
    
    /** Check if rope will break under given force */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Will Rope Break"))
    static bool WillRopeBreak(UAdvancedRopeComponent* RopeComponent, float AppliedForce);
    
    /** Calculate fall factor for a given fall distance and rope length */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Calculate Fall Factor"))
    static float CalculateFallFactor(float FallDistance, float RopeLength);
    
    /** Get all segment positions along the rope */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Get Rope Segment Positions"))
    static TArray<FVector> GetRopeSegmentPositions(UAdvancedRopeComponent* RopeComponent);
    
    /** Check if a world location is on the rope within tolerance */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Is Point On Rope"))
    static bool IsPointOnRope(UAdvancedRopeComponent* RopeComponent, 
                             const FVector& WorldLocation, float Tolerance = 10.0f);

    // ===== CLIMBING PHYSICS FUNCTIONS =====
    
    /** Try to start climbing on nearby surfaces */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Movement", 
              meta = (DisplayName = "Start Climbing"))
    static bool StartClimbing(UAdvancedClimbingComponent* ClimbingComponent);
    
    /** Stop climbing and return to normal movement */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Movement", 
              meta = (DisplayName = "Stop Climbing"))
    static void StopClimbing(UAdvancedClimbingComponent* ClimbingComponent);
    
    /** Find all available grip points within range */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Grips", 
              meta = (DisplayName = "Find Nearby Grips"))
    static TArray<FGripPoint> FindNearbyGrips(UAdvancedClimbingComponent* ClimbingComponent, 
                                            float SearchRadius = 150.0f);
    
    /** Try to grab a specific grip point */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Grips", 
              meta = (DisplayName = "Grab Hold"))
    static bool GrabHold(UAdvancedClimbingComponent* ClimbingComponent, 
                        const FGripPoint& GripPoint, bool bIsLeftHand = true);
    
    /** Release a grip */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Grips", 
              meta = (DisplayName = "Release Grip"))
    static void ReleaseGrip(UAdvancedClimbingComponent* ClimbingComponent, 
                          bool bIsLeftHand = true, bool bIsHand = true);
    
    /** Get current stamina level */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Stamina", 
              meta = (DisplayName = "Get Current Stamina"))
    static float GetCurrentStamina(UAdvancedClimbingComponent* ClimbingComponent);
    
    /** Get current grip strength */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Stamina", 
              meta = (DisplayName = "Get Current Grip Strength"))
    static float GetCurrentGripStrength(UAdvancedClimbingComponent* ClimbingComponent);
    
    /** Perform a dynamic move (dyno) to target location */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Advanced", 
              meta = (DisplayName = "Perform Dyno"))
    static void PerformDyno(UAdvancedClimbingComponent* ClimbingComponent, 
                           const FVector& TargetLocation);
    
    /** Attach climber to a rope */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Rope", 
              meta = (DisplayName = "Attach To Rope"))
    static bool AttachToRope(UAdvancedClimbingComponent* ClimbingComponent, 
                           UAdvancedRopeComponent* Rope);

    // ===== ANCHOR SYSTEM FUNCTIONS =====
    
    /** Add an anchor point to the anchor system */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Add Anchor Point"))
    static bool AddAnchorPoint(UAnchorSystem* AnchorSystem, 
                             AClimbingToolBase* AnchorTool, bool bIsBackup = false);
    
    /** Remove an anchor point from the system */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Remove Anchor Point"))
    static void RemoveAnchorPoint(UAnchorSystem* AnchorSystem, AClimbingToolBase* AnchorTool);
    
    /** Connect a rope to the anchor system */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Connect Rope To Anchors"))
    static bool ConnectRopeToAnchors(UAnchorSystem* AnchorSystem, UAdvancedRopeComponent* Rope);
    
    /** Get the system safety factor */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Get System Safety Factor"))
    static float GetSystemSafetyFactor(UAnchorSystem* AnchorSystem);
    
    /** Get maximum load on any single anchor */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Get Max Single Point Load"))
    static float GetMaxSinglePointLoad(UAnchorSystem* AnchorSystem);
    
    /** Check if the anchor system is intact and safe */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Validate System Integrity"))
    static bool ValidateSystemIntegrity(UAnchorSystem* AnchorSystem);
    
    /** Calculate system extension under current load */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Calculate System Extension"))
    static float CalculateSystemExtension(UAnchorSystem* AnchorSystem);
    
    /** Test if system can survive failure of specific anchor */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Anchors", 
              meta = (DisplayName = "Can Survive Anchor Failure"))
    static bool CanSurviveAnchorFailure(UAnchorSystem* AnchorSystem, AClimbingToolBase* AnchorTool);

    // ===== FALL MECHANICS FUNCTIONS =====
    
    /** Start tracking a fall */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Falls", 
              meta = (DisplayName = "Start Fall"))
    static void StartFall(UFallMechanicsSystem* FallSystem, EFallType FallType);
    
    /** End fall and calculate injuries */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Falls", 
              meta = (DisplayName = "End Fall"))
    static void EndFall(UFallMechanicsSystem* FallSystem, const FVector& ImpactLocation, 
                       const FVector& ImpactVelocity, AActor* ImpactSurface = nullptr);
    
    /** Handle a rope fall specifically */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Falls", 
              meta = (DisplayName = "Handle Rope Fall"))
    static void HandleRopeFall(UFallMechanicsSystem* FallSystem, 
                              UAdvancedRopeComponent* Rope, float FallDistance);
    
    /** Get current health percentage */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Injury", 
              meta = (DisplayName = "Get Health Percentage"))
    static float GetHealthPercentage(UFallMechanicsSystem* FallSystem);
    
    /** Get all active injuries */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Injury", 
              meta = (DisplayName = "Get Active Injuries"))
    static TArray<FInjury> GetActiveInjuries(UFallMechanicsSystem* FallSystem);
    
    /** Check if character can continue climbing */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Injury", 
              meta = (DisplayName = "Can Continue Climbing"))
    static bool CanContinueClimbing(UFallMechanicsSystem* FallSystem);
    
    /** Check if character requires evacuation */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Injury", 
              meta = (DisplayName = "Requires Evacuation"))
    static bool RequiresEvacuation(UFallMechanicsSystem* FallSystem);
    
    /** Apply first aid treatment */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Treatment", 
              meta = (DisplayName = "Apply First Aid"))
    static void ApplyFirstAid(UFallMechanicsSystem* FallSystem, 
                             EBodyPart BodyPart, float Effectiveness = 0.5f);
    
    /** Apply medical treatment for specific injury type */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Treatment", 
              meta = (DisplayName = "Apply Medical Treatment"))
    static void ApplyMedicalTreatment(UFallMechanicsSystem* FallSystem, 
                                    EInjuryType TreatmentType, float Effectiveness = 0.7f);

    // ===== PERFORMANCE MANAGEMENT FUNCTIONS =====
    
    /** Get current performance metrics */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Performance", 
              meta = (DisplayName = "Get Performance Metrics"))
    static FPerformanceMetrics GetPerformanceMetrics(const UObject* WorldContext);
    
    /** Manually adjust quality for performance */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Performance", 
              meta = (DisplayName = "Adjust Quality For Performance"))
    static void AdjustQualityForPerformance(const UObject* WorldContext);
    
    /** Set global LOD bias */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Performance", 
              meta = (DisplayName = "Set Global LOD Bias"))
    static void SetGlobalLODBias(const UObject* WorldContext, float LODBias);
    
    /** Force garbage collection */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Performance", 
              meta = (DisplayName = "Run Garbage Collection"))
    static void RunGarbageCollection(const UObject* WorldContext);
    
    /** Get LOD level for specific actor at distance */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Performance", 
              meta = (DisplayName = "Get Object LOD"))
    static EPerformanceLOD GetObjectLOD(const UObject* WorldContext, 
                                       AActor* Actor, const FVector& ViewerLocation);
    
    /** Enable/disable adaptive quality */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Performance", 
              meta = (DisplayName = "Set Adaptive Quality Enabled"))
    static void SetAdaptiveQualityEnabled(const UObject* WorldContext, bool bEnabled);

    // ===== UTILITY FUNCTIONS =====
    
    /** Convert climb grade enum to display string */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Utility", 
              meta = (DisplayName = "Grade To String"))
    static FString ClimbingGradeToString(EClimbingDifficulty Grade);
    
    /** Convert rope type enum to display string */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Utility", 
              meta = (DisplayName = "Rope Type To String"))
    static FString RopeTypeToString(ERopeType RopeType);
    
    /** Convert injury severity to color for UI display */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Utility", 
              meta = (DisplayName = "Injury Severity To Color"))
    static FLinearColor InjurySeverityToColor(EInjurySeverity Severity);
    
    /** Calculate estimated climbing time for route */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Utility", 
              meta = (DisplayName = "Estimate Climbing Time"))
    static float EstimateClimbingTime(float RouteLength, EClimbingDifficulty Difficulty, 
                                    float ClimberSkillLevel = 1.0f);
    
    /** Calculate gear weight for inventory management */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Utility", 
              meta = (DisplayName = "Calculate Total Gear Weight"))
    static float CalculateTotalGearWeight(const TArray<AClimbingToolBase*>& GearList);
    
    /** Check if weather conditions are safe for climbing */
    UFUNCTION(BlueprintPure, Category = "Climbing Physics|Utility", 
              meta = (DisplayName = "Are Weather Conditions Safe"))
    static bool AreWeatherConditionsSafe(float Temperature, float WindSpeed, 
                                       float Humidity, bool bIsRaining);

    // ===== SYSTEM INTEGRATION FUNCTIONS =====
    
    /** Initialize all physics systems for a character */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Integration", 
              CallInEditor = true, meta = (DisplayName = "Initialize Climbing Physics"))
    static bool InitializeClimbingPhysics(ACharacter* Character);
    
    /** Connect climbing component to rope and anchor systems */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Integration", 
              meta = (DisplayName = "Connect Physics Systems"))
    static void ConnectPhysicsSystems(UAdvancedClimbingComponent* ClimbingComponent,
                                    UAdvancedRopeComponent* RopeComponent,
                                    UAnchorSystem* AnchorSystem,
                                    UFallMechanicsSystem* FallSystem);
    
    /** Setup performance monitoring for climbing physics */
    UFUNCTION(BlueprintCallable, Category = "Climbing Physics|Integration", 
              meta = (DisplayName = "Setup Performance Monitoring"))
    static void SetupPerformanceMonitoring(const UObject* WorldContext, bool bAutoOptimize = true);

private:
    /** Helper to get performance manager from world */
    static UClimbingPerformanceManager* GetPerformanceManager(const UObject* WorldContext);
};