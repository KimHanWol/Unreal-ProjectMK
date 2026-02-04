// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"

AMKCharacter::AMKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetRootComponent(GetCapsuleComponent());

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetupAttachment(GetRootComponent());

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet_Character = CreateDefaultSubobject<UAttributeSet_Character>(TEXT("AttributeSet_Character"));
}

UAbilitySystemComponent* AMKCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMKCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	GiveAbilities();
	InitializeCharacterAttribute();
}

void AMKCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsUpDoublePressing)
	{
		Fly();
	}
	else
	{
		FinishFly();
	}
}

void AMKCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveRight", this, &AMKCharacter::MoveRight);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMKCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMKCharacter::StopJumping);

	PlayerInputComponent->BindAxis("LookRight", this, &AMKCharacter::LookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AMKCharacter::LookUp);
}

void AMKCharacter::GiveAbilities()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	for (const auto& InitialGameplayAbility : InitialGameplayAbilities)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(InitialGameplayAbility));
	}
}

void AMKCharacter::InitializeCharacterAttribute()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet_Character);

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Character_Init);
	if (::IsValid(EffectClass) == false)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystemComponent->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AMKCharacter::BindEvents()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).AddUObject(this, &AMKCharacter::OnItemCollectRangeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &AMKCharacter::OnCurrentHealthChanged);
}

void AMKCharacter::UnbindEvents()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).RemoveAll(this);
}

UAbilitySystemComponent* AMKCharacter::GetOwnerASC()
{
	return GetAbilitySystemComponent();
}

bool AMKCharacter::CheckIsDestroyed()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return false;
	}

	const UAttributeSet_Character* CharacterAttributeSet =Cast<UAttributeSet_Character>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Character::StaticClass()));
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return false;
	}

	return CharacterAttributeSet->GetCurrentHealth() <= 0.f;
}

void AMKCharacter::OnDestroyed()
{
	
}

void AMKCharacter::MoveRight(float Value)
{
	if (::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	float Speed = GetCharacterMovement()->IsFlying() ? AttributeSet_Character->GetFlyingSpeed() : AttributeSet_Character->GetMoveSpeed() / 10.f;
	if (Speed > 0.f)
	{
		AddMovementInput(FVector::ForwardVector, Value * Speed);
	}
}

void AMKCharacter::LookRight(float Value)
{
	CharacterDir.X = Value;

	TryDrill();
}

void AMKCharacter::LookUp(float Value)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (Value > 0.f)
	{
		if (!bIsUpPressing)
		{
			if (CurrentTime - LastUpPressedTime <= DoublePressDuration)
			{
				bIsUpDoublePressing = true;
			}
			LastUpPressedTime = CurrentTime;
			bIsUpPressing = true;
		}
	}
	else
	{
		bIsUpPressing = false;
		bIsUpDoublePressing = false;
	}

	CharacterDir.Z = Value;

	TryDrill();
}

void AMKCharacter::Fly()
{
	UCharacterMovementComponent* CharacterMovementComp = GetCharacterMovement();
	if (::IsValid(CharacterMovementComp) == false)
	{
		return;
	}

	if (::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	CharacterMovementComp->SetMovementMode(MOVE_Flying);
	CharacterMovementComp->MaxFlySpeed = AttributeSet_Character->GetFlyingSpeed();
	CharacterMovementComp->GravityScale = 1.f;
	AddMovementInput(FVector::UpVector, AttributeSet_Character->GetFlyingSpeed());
}

void AMKCharacter::FinishFly()
{
	UCharacterMovementComponent* CharacterMovementComp = GetCharacterMovement();
	if (::IsValid(CharacterMovementComp) == false)
	{
		return;
	}

	if (CharacterMovementComp->MovementMode != MOVE_Flying)
	{
		return;
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.f;
}

void AMKCharacter::TryDrill()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	if (CharacterDir != FVector::ZeroVector)
	{
		//ㅁㄻㄴㄻㄴㄹ 여기 Ability 확인
		AbilitySystemComponent->TryActivateAbilityByClass(InitialGameplayAbilities[0]);
	}
	else
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(InitialGameplayAbilities[0]);
		if (AbilitySpec)
		{
			AbilitySystemComponent->CancelAbility(AbilitySpec->Ability);
		}
	}
}

void AMKCharacter::OnItemCollectRangeChanged(const FOnAttributeChangeData& Data)
{
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->SetGainRadius(Data.NewValue);
	}
}

void AMKCharacter::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	if (CheckIsDestroyed())
	{
		OnDestroyed();
	}
}
