#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/GridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Engine/DataTable.h"
#include "InventoryWidget.generated.h"

UENUM(BlueprintType)
enum class EToolCategory : uint8
{
	Protection,      // Cams, nuts, pitons
	Hardware,        // Carabiners, quickdraws
	Rope,           // Dynamic rope, static rope
	Anchoring,      // Anchors, belay devices
	Utility,        // Knife, tape, headlamp
	Emergency       // First aid, emergency shelter
};

USTRUCT(BlueprintType)
struct FInventoryItem : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ItemID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText ItemName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText ItemDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EToolCategory Category;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* ItemIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Quantity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxQuantity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Durability;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxDurability;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Weight; // In grams

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsEquipped;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsConsumable;

	FInventoryItem()
	{
		ItemID = NAME_None;
		ItemName = FText::GetEmpty();
		ItemDescription = FText::GetEmpty();
		Category = EToolCategory::Utility;
		ItemIcon = nullptr;
		Quantity = 1;
		MaxQuantity = 1;
		Durability = 100.0f;
		MaxDurability = 100.0f;
		Weight = 0.0f;
		bIsEquipped = false;
		bIsConsumable = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemSelected, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemAction, const FInventoryItem&, Item, FName, Action);

/**
 * Advanced inventory widget for climbing tools and equipment
 * Features quick-selection, category filtering, and durability tracking
 */
UCLASS(BlueprintType, Blueprintable)
class CLIMBINGGAME_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UInventoryWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

public:
	// Inventory management functions
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void FilterByCategory(EToolCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ShowAllCategories();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSelectedItem(const FInventoryItem& Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnequipSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropSelectedItem();

	// Quick selection (1-9 keys)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetQuickSelectSlot(int32 SlotIndex, const FInventoryItem& Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ActivateQuickSelectSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemSelected OnItemSelected;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemAction OnItemAction;

protected:
	// Widget components - bound in Blueprint
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> InventoryScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemStatsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemPreviewImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EquipButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DropButton;

	// Category filter buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ProtectionFilterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HardwareFilterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RopeFilterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AnchoringFilterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UtilityFilterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EmergencyFilterButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AllCategoriesButton;

	// Quick select slots (1-9)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> QuickSelectGrid;

	// Weight and stats display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalWeightText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCountText;

	// Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Config")
	TSubclassOf<UUserWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Config")
	TSubclassOf<UUserWidget> QuickSelectSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Config")
	int32 InventoryGridColumns = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Config")
	float MaxCarryWeight = 15000.0f; // 15kg in grams

private:
	// Current inventory state
	UPROPERTY()
	TArray<FInventoryItem> CurrentInventory;

	UPROPERTY()
	TArray<FInventoryItem> FilteredInventory;

	UPROPERTY()
	FInventoryItem SelectedItem;

	UPROPERTY()
	TArray<FInventoryItem> QuickSelectSlots; // 9 slots

	EToolCategory CurrentFilter;

	// Button event handlers
	UFUNCTION()
	void OnEquipButtonClicked();

	UFUNCTION()
	void OnUseButtonClicked();

	UFUNCTION()
	void OnDropButtonClicked();

	UFUNCTION()
	void OnProtectionFilterClicked();

	UFUNCTION()
	void OnHardwareFilterClicked();

	UFUNCTION()
	void OnRopeFilterClicked();

	UFUNCTION()
	void OnAnchoringFilterClicked();

	UFUNCTION()
	void OnUtilityFilterClicked();

	UFUNCTION()
	void OnEmergencyFilterClicked();

	UFUNCTION()
	void OnAllCategoriesClicked();

	// Helper functions
	void PopulateInventoryGrid();
	void UpdateItemDetails();
	void UpdateQuickSelectSlots();
	void UpdateWeightAndStats();
	void ApplyCategoryFilter();

	// Get inventory data from game systems
	TArray<FInventoryItem> GetInventoryFromGameState();
};