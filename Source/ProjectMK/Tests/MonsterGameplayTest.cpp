#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Monster/MKMonster.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"

namespace
{
	class FMonsterDrillInputCommand : public IAutomationLatentCommand
	{
	public:
		explicit FMonsterDrillInputCommand(FAutomationTestBase* InTest)
			: Test(InTest), SetupStartTime(FPlatformTime::Seconds())
		{
		}

		virtual bool Update() override
		{
			if (bStarted == false)
			{
				UWorld* World = GEditor->PlayWorld;
				APlayerController* LocalController = ::IsValid(World) ? World->GetFirstPlayerController() : nullptr;
				AMKCharacter* Character = ::IsValid(LocalController) ? Cast<AMKCharacter>(LocalController->GetPawn()) : nullptr;
				if (::IsValid(Character) == false)
				{
					if (FPlatformTime::Seconds() - SetupStartTime < 15.0)
					{
						return false;
					}
					Test->AddError(TEXT("Main PIE did not create the player."));
					return true;
				}
				USaveGameSubsystem* Saves = USaveGameSubsystem::Get(World);
				if (::IsValid(Saves) == false || Saves->StartPlaytestSession() == false)
				{
					Test->AddError(TEXT("Could not protect save files during the gameplay test."));
					return true;
				}
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AMKMonster* SpawnedMonster = World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Character->GetActorLocation() + FVector(13.f, 0.f, 0.f),
					FRotator::ZeroRotator, SpawnParameters);
				if (::IsValid(SpawnedMonster) == false)
				{
					Test->AddError(TEXT("Could not spawn DrillCrawler. Check the Monster DataTable connection."));
					return true;
				}
				SpawnedMonster->GetCharacterMovement()->MaxWalkSpeed = 0.f;
				Controller = LocalController;
				Monster = SpawnedMonster;
				Player = Character;
				PlayerTransform = Character->GetActorTransform();
				UAbilitySystemComponent* PlayerASC = Character->GetAbilitySystemComponent();
				OriginalMoveSpeed = PlayerASC->GetNumericAttributeBase(UAttributeSet_Character::GetMoveSpeedAttribute());
				PlayerASC->SetNumericAttributeBase(UAttributeSet_Character::GetMoveSpeedAttribute(), 0.f);
				StartTime = World->GetTimeSeconds();
				const UAttributeSet_Character* Attributes = SpawnedMonster->GetAbilitySystemComponent()->GetSet<UAttributeSet_Character>();
				InitialHealth = Attributes->GetCurrentHealth();
				Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Pressed, 1.f));
				bStarted = true;
				return false;
			}
			if (Controller.IsValid() && Monster.IsValid() && Controller->GetWorld()->GetTimeSeconds() - StartTime < 1.5)
			{
				return false;
			}
			if (Controller.IsValid())
			{
				Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Released, 0.f));
			}
			Test->TestTrue(TEXT("Monster survives the non-lethal drill test"), Monster.IsValid());
			if (Monster.IsValid())
			{
				const UAttributeSet_Character* Attributes = Monster->GetAbilitySystemComponent()->GetSet<UAttributeSet_Character>();
				Test->TestTrue(TEXT("Player's held drill input reduces monster health"), Attributes->GetCurrentHealth() < InitialHealth);
				Test->TestTrue(TEXT("Player drill damage triggers anger"), Monster->GetMonsterState() == EMonsterState::Angry);
				Test->AddInfo(FString::Printf(TEXT("Player drill: monster health %.0f -> %.0f"), InitialHealth, Attributes->GetCurrentHealth()));
				Monster->Destroy();
			}
			if (Player.IsValid())
			{
				if (AMKCharacter* Character = Cast<AMKCharacter>(Player.Get()))
				{
					Character->GetCharacterMovement()->StopMovementImmediately();
					Character->GetAbilitySystemComponent()->SetNumericAttributeBase(UAttributeSet_Character::GetMoveSpeedAttribute(), OriginalMoveSpeed);
				}
				Player->SetActorTransform(PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
			}
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TWeakObjectPtr<APlayerController> Controller;
		TWeakObjectPtr<AMKMonster> Monster;
		TWeakObjectPtr<APawn> Player;
		FTransform PlayerTransform;
		double SetupStartTime;
		double StartTime = 0.0;
		float InitialHealth = 0.f;
		float OriginalMoveSpeed = 0.f;
		bool bStarted = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterPlayerDrillTest, "ProjectMK.PIE.Monster.PlayerDrillTriggersAnger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterPlayerDrillTest::RunTest(const FString& Parameters)
{
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (::IsValid(EditorWorld) == false || EditorWorld->GetOutermost()->GetName() != TEXT("/Game/Level/Main"))
	{
		AddError(TEXT("Open the Main map before running this gameplay test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FMonsterDrillInputCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
