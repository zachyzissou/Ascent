#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "ClimbingCompassWidget.generated.h"

UENUM(BlueprintType)
enum class ENavigationMarkerType : uint8
{
	RouteStart,         // Beginning of climbing route
	RouteEnd,          // Top of route / completion point
	Belay,             // Belay station
	Anchor,            // Anchor point
	RestLedge,         // Good resting spot
	CruxSection,       // Most difficult part of route
	Hazard,            // Dangerous area (rockfall, loose rock)
	Exit,              // Route exit/descent
	Water,             // Water source
	Shelter,           // Emergency shelter
	Teammate,          // Other team members
	Equipment,         // Dropped or cached equipment
	Custom             // User-defined marker
};

USTRUCT(BlueprintType)
struct FNavigationMarker
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName MarkerID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ENavigationMarkerType MarkerType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector WorldLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText MarkerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText MarkerDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DistanceFromPlayer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AltitudeDifference;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsVisible;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowOnCompass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor MarkerColor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* MarkerIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CreationTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString CreatedBy; // For multiplayer

	FNavigationMarker()
	{
		MarkerID = NAME_None;
		MarkerType = ENavigationMarkerType::Custom;
		WorldLocation = FVector::ZeroVector;
		MarkerName = FText::GetEmpty();
		MarkerDescription = FText::GetEmpty();
		DistanceFromPlayer = 0.0f;
		AltitudeDifference = 0.0f;
		bIsVisible = true;
		bShowOnCompass = true;
		MarkerColor = FLinearColor::White;
		MarkerIcon = nullptr;
		CreationTime = 0.0f;
		CreatedBy = TEXT("");
	}
};

USTRUCT(BlueprintType)
struct FCompassConfiguration
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CompassRadius;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowCardinalDirections;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowIntercardinalDirections;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowDegreeMarkers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowDistanceRings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxDisplayDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowAltitudeMarkers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowPlayerTrail;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxTrailPoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CompassOpacity;

	FCompassConfiguration()
	{
		CompassRadius = 150.0f;
		bShowCardinalDirections = true;
		bShowIntercardinalDirections = true;
		bShowDegreeMarkers = false;
		bShowDistanceRings = true;
		MaxDisplayDistance = 500.0f;
		bShowAltitudeMarkers = true;
		bShowPlayerTrail = false;
		MaxTrailPoints = 50;
		CompassOpacity = 0.8f;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerSelected, const FNavigationMarker&, Marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarkerAction, const FNavigationMarker&, Marker, FName, Action);

/**
 * Advanced climbing compass and navigation widget
 * Provides directional guidance, marker system, and route navigation
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UClimbingCompassWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UClimbingCompassWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// Compass functionality
	UFUNCTION(BlueprintCallable, Category = "Compass")
	void UpdatePlayerRotation(float YawRotation);

	UFUNCTION(BlueprintCallable, Category = "Compass")
	void UpdatePlayerLocation(const FVector& NewLocation);

	UFUNCTION(BlueprintCallable, Category = "Compass")
	void SetCompassVisibility(bool bVisible);

	// Marker management
	UFUNCTION(BlueprintCallable, Category = "Compass|Markers")
	void AddNavigationMarker(const FNavigationMarker& Marker);

	UFUNCTION(BlueprintCallable, Category = "Compass|Markers")
	void RemoveNavigationMarker(const FName& MarkerID);

	UFUNCTION(BlueprintCallable, Category = "Compass|Markers")
	void UpdateNavigationMarker(const FName& MarkerID, const FNavigationMarker& UpdatedMarker);

	UFUNCTION(BlueprintCallable, Category = "Compass|Markers")
	void ClearAllMarkers();

	UFUNCTION(BlueprintCallable, Category = "Compass|Markers")
	void SetMarkerVisibility(const FName& MarkerID, bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Compass|Markers")
	void SetMarkerTypeVisibility(ENavigationMarkerType MarkerType, bool bVisible);

	// Route guidance
	UFUNCTION(BlueprintCallable, Category = "Compass|Route")
	void SetDestination(const FVector& DestinationLocation, const FText& DestinationName = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category = "Compass|Route")
	void ClearDestination();

	UFUNCTION(BlueprintCallable, Category = "Compass|Route")
	void SetRouteMarkers(const TArray<FNavigationMarker>& RoutePoints);

	UFUNCTION(BlueprintCallable, Category = "Compass|Route")
	void HighlightRouteSection(int32 StartIndex, int32 EndIndex);

	// Configuration
	UFUNCTION(BlueprintCallable, Category = "Compass|Configuration")
	void ApplyCompassConfiguration(const FCompassConfiguration& Config);

	UFUNCTION(BlueprintCallable, Category = "Compass|Configuration")
	void SetCompassSize(float NewSize);

	UFUNCTION(BlueprintCallable, Category = "Compass|Configuration")
	void SetMaxDisplayDistance(float Distance);

	UFUNCTION(BlueprintCallable, Category = "Compass|Configuration")
	void ToggleCompassMode(); // Switch between full compass and mini compass

	// Utility functions
	UFUNCTION(BlueprintPure, Category = "Compass|Utility")
	float GetDistanceToMarker(const FName& MarkerID) const;

	UFUNCTION(BlueprintPure, Category = "Compass|Utility")
	float GetBearingToMarker(const FName& MarkerID) const;

	UFUNCTION(BlueprintPure, Category = "Compass|Utility")
	TArray<FNavigationMarker> GetNearbyMarkers(float SearchRadius) const;

	UFUNCTION(BlueprintPure, Category = "Compass|Utility")
	FNavigationMarker GetClosestMarker(ENavigationMarkerType MarkerType) const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Compass")
	FOnMarkerSelected OnMarkerSelected;

	UPROPERTY(BlueprintAssignable, Category = "Compass")
	FOnMarkerAction OnMarkerAction;

protected:
	// Main compass components
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CompassContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CompassRose;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CompassNeedle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MarkersContainer;

	// Directional indicators
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NorthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SouthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EastText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WestText;

	// Distance rings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DistanceRing100;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DistanceRing250;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DistanceRing500;

	// Information display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentBearingText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentAltitudeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DestinationInfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> DestinationProgressBar;

	// Mini compass mode
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MiniCompassContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> MiniCompassRose;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compass Config")
	TSubclassOf<UUserWidget> MarkerWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compass Config")
	TMap<ENavigationMarkerType, UTexture2D*> MarkerIcons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compass Config")
	TMap<ENavigationMarkerType, FLinearColor> MarkerColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compass Config")
	float UpdateFrequency = 10.0f; // Updates per second

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compass Config")
	bool bStartInMiniMode = false;

private:
	// Current state
	FVector PlayerLocation;
	float PlayerYaw;
	FVector DestinationLocation;
	FText DestinationName;
	bool bHasDestination;
	TMap<FName, FNavigationMarker> NavigationMarkers;
	TArray<FNavigationMarker> RouteMarkers;
	FCompassConfiguration CurrentConfig;
	bool bIsMiniMode;
	float LastUpdateTime;

	// Marker visibility settings
	TMap<ENavigationMarkerType, bool> MarkerTypeVisibility;
	
	// Trail tracking
	TArray<FVector> PlayerTrail;
	int32 TrailWriteIndex;

	// Helper functions
	void UpdateCompassRotation();
	void UpdateMarkerPositions();
	void CreateMarkerWidget(const FNavigationMarker& Marker);
	void DestroyMarkerWidget(const FName& MarkerID);
	void UpdateDestinationInfo();
	void UpdatePlayerTrail();
	void RefreshCompassDisplay();
	
	// Coordinate conversion
	FVector2D WorldToCompassPosition(const FVector& WorldLocation) const;
	float CalculateBearing(const FVector& FromLocation, const FVector& ToLocation) const;
	float WrapAngle(float Angle) const;
	
	// Marker management
	bool IsMarkerInRange(const FNavigationMarker& Marker) const;
	bool ShouldDisplayMarker(const FNavigationMarker& Marker) const;
	void SortMarkersByDistance();
	
	// UI state management
	void SwitchToFullCompass();
	void SwitchToMiniCompass();
	void UpdateCompassOpacity();
};