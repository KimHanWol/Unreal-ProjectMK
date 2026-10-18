#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "PaperCharacter.h"
#include "ProjectMK/Data/DataTable/MonsterDataTableRow.h"
#include "ProjectMK/Interface/Damageable.h"
#include "MKMonster.generated.h"

class ABlockBase;
class AMKCharacter;
class UAbilitySystemComponent;
class UArrowComponent;
class UAttributeSet_Character;
class UPaperSpriteComponent;
class UWidgetComponent;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EMonsterState : uint8
{
	Idle,
	Wandering,
	Angry,
};

UCLASS()
class PROJECTMK_API AMKMonster : public APaperCharacter, public IAbilitySystemInterface, public IDamageable
{
	GENERATED_BODY()

public:
	AMKMonster();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	void BindEvents();
	void UnbindEvents();

	virtual UAbilitySystemComponent* GetOwnerASC() override;
	virtual bool CheckIsDestroyed() override;
	virtual void OnDestroyed() override;

public:
	// SpawnActorDeferred 호출과 FinishSpawning 사이에 사용할 행을 지정한다.
	void SetMonsterDataRowKey(FName RowKey);
	EMonsterState GetMonsterState() const;

private:
	bool InitializeMonster();
	void UpdateBehavior();
	void UpdateContactDamage();
	void UpdateStatusVisual();
	void ApplyIdle();
	void ApplyWandering();
	void ApplyAngry();
	void ApplyAngryWandering();
	void UpdateAngryDrilling();
	void ClearDrillTarget();
	bool TrySelectDrillTarget();
	bool CanMoveInDirection(float Direction, bool bApproachBlocks = false) const;
	bool CanDrillTarget() const;
	void OnCurrentHealthChanged(const FOnAttributeChangeData& Data);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster", meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetMonsterRowNames"))
	FName MonsterDataRowKey = TEXT("DrillCrawler");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster")
	EMonsterState State = EMonsterState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> BodySprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> StatusWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> DrillIndicator;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster", meta = (AllowPrivateAccess = "true"))
	FMonsterDataTableRow MonsterData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAttributeSet_Character> Attributes;

	TWeakObjectPtr<ABlockBase> DrillTarget;
	int32 RemainingDrillCount = 0;
	TMap<TWeakObjectPtr<AMKCharacter>, double> NextContactDamageTimes;
	FVector WanderOrigin = FVector::ZeroVector;
	float MoveDirection = 1.f;
	double StateEndTime = 0.0;
	double NextDrillTime = 0.0;
	double NextAngryMoveEndTime = 0.0;
	bool bAngryMoving = false;
	bool bInitialized = false;
	bool bIsDying = false;
	FTimerHandle BehaviorTimerHandle;
};
