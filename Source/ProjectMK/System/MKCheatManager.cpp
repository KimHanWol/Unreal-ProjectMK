// LINK

#include "ProjectMK/System/MKCheatManager.h"

#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
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
