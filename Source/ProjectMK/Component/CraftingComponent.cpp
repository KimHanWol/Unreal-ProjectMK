#include "ProjectMK/Component/CraftingComponent.h"

#include "AbilitySystemComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/Data/DataTable/ShopRecipeDataTableRow.h"

void UCraftingComponent::ApplySavedRecipes(const TArray<FName>& SavedRecipeKeys)
{
	PurchasedRecipeKeySet.Reset();
	for (const FName& SavedRecipeKey : SavedRecipeKeys)
	{
		if (GetRecipeDataTableRow(SavedRecipeKey) != nullptr)
		{
			PurchasedRecipeKeySet.Add(SavedRecipeKey);
		}
	}
}

bool UCraftingComponent::TryUnlockRecipe(FName RecipeKey)
{
	if (CanUnlockRecipe(RecipeKey) == false)
	{
		USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_UseFailed);
		return false;
	}

	const FShopRecipeDataTableRow* RecipeDataTableRow = GetRecipeDataTableRow(RecipeKey);
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	const float NewCoin = GetOwnerCoin() - static_cast<float>(RecipeDataTableRow->UnlockPrice);
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), NewCoin);

	PurchasedRecipeKeySet.Add(RecipeKey);
	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Recipe_Unlock);
	OnRecipeUnlockedDelegate.Broadcast(RecipeKey);
	return true;
}

bool UCraftingComponent::TryCraftRecipe(FName RecipeKey)
{
	const FShopRecipeDataTableRow* RecipeDataTableRow = GetRecipeDataTableRow(RecipeKey);
	UInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (RecipeDataTableRow == nullptr || ::IsValid(InventoryComponent) == false || CanCraftRecipe(RecipeKey) == false
		|| InventoryComponent->CraftShopRecipe(*RecipeDataTableRow) == false)
	{
		USoundManager::TryPlaySFX(GetOwner(), ESFXType::Item_UseFailed);
		return false;
	}

	USoundManager::TryPlaySFX(GetOwner(), ESFXType::Shop_Craft);
	OnRecipeCraftedDelegate.Broadcast(RecipeKey);
	return true;
}

bool UCraftingComponent::IsRecipeUnlocked(FName RecipeKey) const
{
	const FShopRecipeDataTableRow* RecipeDataTableRow = GetRecipeDataTableRow(RecipeKey);
	if (RecipeDataTableRow == nullptr)
	{
		return false;
	}

	return RecipeDataTableRow->UnlockPrice <= 0 || PurchasedRecipeKeySet.Contains(RecipeKey);
}

bool UCraftingComponent::CanUnlockRecipe(FName RecipeKey) const
{
	const FShopRecipeDataTableRow* RecipeDataTableRow = GetRecipeDataTableRow(RecipeKey);
	if (RecipeDataTableRow == nullptr || IsRecipeUnlocked(RecipeKey) || ::IsValid(GetOwnerAbilitySystemComponent()) == false)
	{
		return false;
	}

	return GetOwnerCoin() >= static_cast<float>(RecipeDataTableRow->UnlockPrice);
}

bool UCraftingComponent::CanCraftRecipe(FName RecipeKey) const
{
	const FShopRecipeDataTableRow* RecipeDataTableRow = GetRecipeDataTableRow(RecipeKey);
	const UInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (RecipeDataTableRow == nullptr || ::IsValid(InventoryComponent) == false || IsRecipeUnlocked(RecipeKey) == false)
	{
		return false;
	}

	return InventoryComponent->CanCraftShopRecipe(*RecipeDataTableRow);
}

TArray<FName> UCraftingComponent::GetUnlockedRecipeKeys() const
{
	TArray<FName> UnlockedRecipeKeys;
	const UDataManager* DataManager = UDataManager::Get(const_cast<UCraftingComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return UnlockedRecipeKeys;
	}

	for (const FName& RecipeKey : DataManager->GetShopRecipeRowNames())
	{
		if (IsRecipeUnlocked(RecipeKey))
		{
			UnlockedRecipeKeys.Add(RecipeKey);
		}
	}

	return UnlockedRecipeKeys;
}

TArray<FName> UCraftingComponent::GetLockedRecipeKeys() const
{
	TArray<FName> LockedRecipeKeys;
	const UDataManager* DataManager = UDataManager::Get(const_cast<UCraftingComponent*>(this));
	if (::IsValid(DataManager) == false)
	{
		return LockedRecipeKeys;
	}

	for (const FName& RecipeKey : DataManager->GetShopRecipeRowNames())
	{
		if (IsRecipeUnlocked(RecipeKey) == false)
		{
			LockedRecipeKeys.Add(RecipeKey);
		}
	}

	return LockedRecipeKeys;
}

TArray<FName> UCraftingComponent::GetPurchasedRecipeKeys() const
{
	return PurchasedRecipeKeySet.Array();
}

const FShopRecipeDataTableRow* UCraftingComponent::GetRecipeDataTableRow(FName RecipeKey) const
{
	if (RecipeKey.IsNone())
	{
		return nullptr;
	}

	const UDataManager* DataManager = UDataManager::Get(const_cast<UCraftingComponent*>(this));
	return ::IsValid(DataManager) ? DataManager->GetShopRecipeDataTableRow(RecipeKey) : nullptr;
}

UAbilitySystemComponent* UCraftingComponent::GetOwnerAbilitySystemComponent() const
{
	const AActor* Owner = GetOwner();
	return ::IsValid(Owner) ? Owner->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
}

UInventoryComponent* UCraftingComponent::GetInventoryComponent() const
{
	const AActor* Owner = GetOwner();
	return ::IsValid(Owner) ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

float UCraftingComponent::GetOwnerCoin() const
{
	const UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	const UAttributeSet_Character* AttributeSet = ::IsValid(AbilitySystemComponent) ? AbilitySystemComponent->GetSet<UAttributeSet_Character>() : nullptr;
	return ::IsValid(AttributeSet) ? AttributeSet->GetCoin() : 0.f;
}
