// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/InteractComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"

AMKCharacter::AMKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetRootComponent(GetCapsuleComponent());

	InteractComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractComponent"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetupAttachment(GetRootComponent());

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
}

UAbilitySystemComponent* AMKCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMKCharacter::BeginPlay()
{
	Super::BeginPlay();
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

	GiveAbilities();
	InitializeCharacterAttribute();
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

	AbilitySystemComponent->AddAttributeSetSubobject(NewObject<UAttributeSet_Character>());

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

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).AddUObject(this, &::AMKCharacter::OnItemCollectRangeChanged);
}

void AMKCharacter::UnbindEvents()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).RemoveAll(this);
}

bool AMKCharacter::IsInteracting()
{
	if (::IsValid(InteractComponent) == false)
	{
		return false;
	}

	return InteractComponent->IsInteracting();
}

void AMKCharacter::MoveRight(float Value)
{
	float Speed = GetCharacterMovement()->IsFlying() ? FlyingSpeed : MoveSpeed;
	AddMovementInput(FVector::ForwardVector, Value * Speed);
}

void AMKCharacter::LookRight(float Value)
{
	CharacterDir.X = Value;

	if (::IsValid(InteractComponent))
	{
		InteractComponent->UpdateCharacterDirection(CharacterDir);
	}
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

	if (::IsValid(InteractComponent))
	{
		InteractComponent->UpdateCharacterDirection(CharacterDir);
	}
}

void AMKCharacter::Fly()
{
	UCharacterMovementComponent* CharacterMovementComp = GetCharacterMovement();
	if (::IsValid(CharacterMovementComp) == false)
	{
		return;
	}

	CharacterMovementComp->SetMovementMode(MOVE_Flying);
	CharacterMovementComp->MaxFlySpeed = 100.f;
	CharacterMovementComp->GravityScale = 1.f;
	AddMovementInput(FVector::UpVector, 100.0f);
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

void AMKCharacter::OnItemCollectRangeChanged(const FOnAttributeChangeData& Data)
{
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->SetGainRadius(Data.NewValue);
	}
}

