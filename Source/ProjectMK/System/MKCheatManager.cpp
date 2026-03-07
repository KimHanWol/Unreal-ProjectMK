// LINK

#include "ProjectMK/System/MKCheatManager.h"

#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

void UMKCheatManager::DamagePlayer(float Damage)
{
	if (Damage <= 0.f)
	{
		return;
	}

	AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(PlayerCharacter) == false)
	{
		return;
	}

	FDamageableUtil::ApplyDamage(PlayerCharacter->GetAbilitySystemComponent(), nullptr, Damage);
}

void UMKCheatManager::EnableBlockDebugNumbers()
{
	UWorld* World = GetWorld();
	if (::IsValid(World) == false)
	{
		return;
	}

	ULevelManagerSubsystem* LevelManagerSubsystem = World->GetSubsystem<ULevelManagerSubsystem>();
	if (::IsValid(LevelManagerSubsystem) == false)
	{
		return;
	}

	LevelManagerSubsystem->SetBlockDebugNumbersEnabled(true);
}
