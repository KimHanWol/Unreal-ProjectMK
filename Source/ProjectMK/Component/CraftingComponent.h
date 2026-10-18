#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "CraftingComponent.generated.h"

class UAbilitySystemComponent;
class UInventoryComponent;
struct FShopRecipeDataTableRow;

/** 상점에서 산 레시피(해금)와 제작을 관리한다 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTMK_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void ApplySavedRecipes(const TArray<FName>& SavedRecipeKeys);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool TryUnlockRecipe(FName RecipeKey);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool TryCraftRecipe(FName RecipeKey);

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsRecipeUnlocked(FName RecipeKey) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanUnlockRecipe(FName RecipeKey) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraftRecipe(FName RecipeKey) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FName> GetUnlockedRecipeKeys() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FName> GetLockedRecipeKeys() const;

	/** 저장용. 가격이 0 이라 처음부터 해금된 레시피는 포함하지 않는다 */
	TArray<FName> GetPurchasedRecipeKeys() const;

private:
	const FShopRecipeDataTableRow* GetRecipeDataTableRow(FName RecipeKey) const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	UInventoryComponent* GetInventoryComponent() const;
	float GetOwnerCoin() const;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecipeChanged, FName);
	FOnRecipeChanged OnRecipeUnlockedDelegate;
	FOnRecipeChanged OnRecipeCraftedDelegate;

private:
	UPROPERTY(Transient)
	TSet<FName> PurchasedRecipeKeySet;
};
