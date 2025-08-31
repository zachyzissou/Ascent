#include "InventoryWidget.h"
#include "Components/GridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"

UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentFilter = EToolCategory::Protection; // Default filter
	QuickSelectSlots.SetNum(9); // Initialize 9 quick select slots
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button events
	if (EquipButton)
	{
		EquipButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnEquipButtonClicked);
	}
	if (UseButton)
	{
		UseButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnUseButtonClicked);
	}
	if (DropButton)
	{
		DropButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnDropButtonClicked);
	}

	// Bind category filter buttons
	if (ProtectionFilterButton)
	{
		ProtectionFilterButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnProtectionFilterClicked);
	}
	if (HardwareFilterButton)
	{
		HardwareFilterButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnHardwareFilterClicked);
	}
	if (RopeFilterButton)
	{
		RopeFilterButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnRopeFilterClicked);
	}
	if (AnchoringFilterButton)
	{
		AnchoringFilterButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnAnchoringFilterClicked);
	}
	if (UtilityFilterButton)
	{
		UtilityFilterButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnUtilityFilterClicked);
	}
	if (EmergencyFilterButton)
	{
		EmergencyFilterButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnEmergencyFilterClicked);
	}
	if (AllCategoriesButton)
	{
		AllCategoriesButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnAllCategoriesClicked);
	}

	// Initialize inventory
	RefreshInventory();
}

void UInventoryWidget::RefreshInventory()
{
	// Get current inventory from game state
	CurrentInventory = GetInventoryFromGameState();

	// Apply current filter
	ApplyCategoryFilter();

	// Update UI
	PopulateInventoryGrid();
	UpdateQuickSelectSlots();
	UpdateWeightAndStats();
}

void UInventoryWidget::FilterByCategory(EToolCategory Category)
{
	CurrentFilter = Category;
	ApplyCategoryFilter();
	PopulateInventoryGrid();
}

void UInventoryWidget::ShowAllCategories()
{
	FilteredInventory = CurrentInventory;
	PopulateInventoryGrid();
}

void UInventoryWidget::SetSelectedItem(const FInventoryItem& Item)
{
	SelectedItem = Item;
	UpdateItemDetails();

	// Broadcast selection event
	OnItemSelected.Broadcast(Item);
}

void UInventoryWidget::EquipSelectedItem()
{
	if (SelectedItem.ItemID != NAME_None)
	{
		OnItemAction.Broadcast(SelectedItem, FName("Equip"));
	}
}

void UInventoryWidget::UnequipSelectedItem()
{
	if (SelectedItem.ItemID != NAME_None)
	{
		OnItemAction.Broadcast(SelectedItem, FName("Unequip"));
	}
}

void UInventoryWidget::UseSelectedItem()
{
	if (SelectedItem.ItemID != NAME_None)
	{
		OnItemAction.Broadcast(SelectedItem, FName("Use"));
	}
}

void UInventoryWidget::DropSelectedItem()
{
	if (SelectedItem.ItemID != NAME_None)
	{
		OnItemAction.Broadcast(SelectedItem, FName("Drop"));
	}
}

void UInventoryWidget::SetQuickSelectSlot(int32 SlotIndex, const FInventoryItem& Item)
{
	if (SlotIndex >= 0 && SlotIndex < QuickSelectSlots.Num())
	{
		QuickSelectSlots[SlotIndex] = Item;
		UpdateQuickSelectSlots();
	}
}

void UInventoryWidget::ActivateQuickSelectSlot(int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < QuickSelectSlots.Num())
	{
		const FInventoryItem& Item = QuickSelectSlots[SlotIndex];
		if (Item.ItemID != NAME_None)
		{
			OnItemAction.Broadcast(Item, FName("QuickSelect"));
		}
	}
}

float UInventoryWidget::GetTotalWeight() const
{
	float TotalWeight = 0.0f;
	for (const FInventoryItem& Item : CurrentInventory)
	{
		TotalWeight += Item.Weight * Item.Quantity;
	}
	return TotalWeight;
}

int32 UInventoryWidget::GetItemCount() const
{
	return CurrentInventory.Num();
}

void UInventoryWidget::OnEquipButtonClicked()
{
	if (SelectedItem.bIsEquipped)
	{
		UnequipSelectedItem();
	}
	else
	{
		EquipSelectedItem();
	}
}

void UInventoryWidget::OnUseButtonClicked()
{
	UseSelectedItem();
}

void UInventoryWidget::OnDropButtonClicked()
{
	DropSelectedItem();
}

void UInventoryWidget::OnProtectionFilterClicked()
{
	FilterByCategory(EToolCategory::Protection);
}

void UInventoryWidget::OnHardwareFilterClicked()
{
	FilterByCategory(EToolCategory::Hardware);
}

void UInventoryWidget::OnRopeFilterClicked()
{
	FilterByCategory(EToolCategory::Rope);
}

void UInventoryWidget::OnAnchoringFilterClicked()
{
	FilterByCategory(EToolCategory::Anchoring);
}

void UInventoryWidget::OnUtilityFilterClicked()
{
	FilterByCategory(EToolCategory::Utility);
}

void UInventoryWidget::OnEmergencyFilterClicked()
{
	FilterByCategory(EToolCategory::Emergency);
}

void UInventoryWidget::OnAllCategoriesClicked()
{
	ShowAllCategories();
}

void UInventoryWidget::PopulateInventoryGrid()
{
	if (!InventoryGrid || !InventorySlotWidgetClass) return;

	// Clear existing widgets
	InventoryGrid->ClearChildren();

	// Create widgets for filtered inventory
	for (int32 i = 0; i < FilteredInventory.Num(); i++)
	{
		if (UUserWidget* SlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), InventorySlotWidgetClass))
		{
			int32 Row = i / InventoryGridColumns;
			int32 Column = i % InventoryGridColumns;
			InventoryGrid->AddChildToUniformGrid(SlotWidget, Row, Column);

			// TODO: Set up slot widget with item data
			// This would typically involve calling a function on the slot widget
			// to initialize it with the item data
		}
	}
}

void UInventoryWidget::UpdateItemDetails()
{
	if (ItemNameText)
	{
		ItemNameText->SetText(SelectedItem.ItemName);
	}

	if (ItemDescriptionText)
	{
		ItemDescriptionText->SetText(SelectedItem.ItemDescription);
	}

	if (ItemStatsText)
	{
		FString StatsText = FString::Printf(TEXT("Durability: %.1f/%.1f\nWeight: %.1fg\nQuantity: %d"),
			SelectedItem.Durability, SelectedItem.MaxDurability, SelectedItem.Weight, SelectedItem.Quantity);
		ItemStatsText->SetText(FText::FromString(StatsText));
	}

	if (ItemPreviewImage && SelectedItem.ItemIcon)
	{
		ItemPreviewImage->SetBrushFromTexture(SelectedItem.ItemIcon);
	}

	// Update button states
	if (EquipButton)
	{
		FText ButtonText = SelectedItem.bIsEquipped ? FText::FromString("Unequip") : FText::FromString("Equip");
		// Note: You'll need to access the button's text component in Blueprint
	}

	if (UseButton)
	{
		UseButton->SetIsEnabled(!SelectedItem.bIsConsumable || SelectedItem.Quantity > 0);
	}
}

void UInventoryWidget::UpdateQuickSelectSlots()
{
	if (!QuickSelectGrid || !QuickSelectSlotWidgetClass) return;

	// Clear existing quick select widgets
	QuickSelectGrid->ClearChildren();

	// Create widgets for quick select slots
	for (int32 i = 0; i < QuickSelectSlots.Num(); i++)
	{
		if (UUserWidget* SlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), QuickSelectSlotWidgetClass))
		{
			QuickSelectGrid->AddChildToUniformGrid(SlotWidget, 0, i);

			// TODO: Set up quick select slot widget with item data
		}
	}
}

void UInventoryWidget::UpdateWeightAndStats()
{
	if (TotalWeightText)
	{
		float CurrentWeight = GetTotalWeight();
		FString WeightText = FString::Printf(TEXT("Weight: %.1f/%.1f kg"), 
			CurrentWeight / 1000.0f, MaxCarryWeight / 1000.0f);
		TotalWeightText->SetText(FText::FromString(WeightText));

		// Color code based on weight
		FLinearColor WeightColor = FLinearColor::White;
		float WeightPercentage = CurrentWeight / MaxCarryWeight;
		if (WeightPercentage > 0.9f)
		{
			WeightColor = FLinearColor::Red;
		}
		else if (WeightPercentage > 0.7f)
		{
			WeightColor = FLinearColor::Yellow;
		}
		
		TotalWeightText->SetColorAndOpacity(WeightColor);
	}

	if (ItemCountText)
	{
		FString CountText = FString::Printf(TEXT("Items: %d"), GetItemCount());
		ItemCountText->SetText(FText::FromString(CountText));
	}
}

void UInventoryWidget::ApplyCategoryFilter()
{
	FilteredInventory.Empty();

	for (const FInventoryItem& Item : CurrentInventory)
	{
		if (Item.Category == CurrentFilter)
		{
			FilteredInventory.Add(Item);
		}
	}
}

TArray<FInventoryItem> UInventoryWidget::GetInventoryFromGameState()
{
	// This is a placeholder implementation
	// In a real game, this would interface with your inventory system
	TArray<FInventoryItem> TestInventory;

	// Add some sample items for testing
	FInventoryItem Cam;
	Cam.ItemID = FName("TestCam");
	Cam.ItemName = FText::FromString("Cam #1");
	Cam.ItemDescription = FText::FromString("Small protection cam");
	Cam.Category = EToolCategory::Protection;
	Cam.Quantity = 3;
	Cam.Weight = 45.0f;
	TestInventory.Add(Cam);

	FInventoryItem Rope;
	Rope.ItemID = FName("DynamicRope");
	Rope.ItemName = FText::FromString("Dynamic Rope");
	Rope.ItemDescription = FText::FromString("60m climbing rope");
	Rope.Category = EToolCategory::Rope;
	Rope.Weight = 4200.0f;
	TestInventory.Add(Rope);

	return TestInventory;
}