#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "PaperSprite.h"
#include "Tests/AutomationEditorCommon.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Monster/MKMonster.h"
#include "ProjectMK/Component/MKCharacterVisualComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/System/GlobalConstants.h"

namespace
{
	// Main의 빈 공중에 검증용 지형을 만들고 테스트 PIE 종료 시 함께 폐기한다.
	struct FGeometryFixture
	{
		UWorld* World = nullptr;
		ULevelManagerSubsystem* Level = nullptr;
		AMKCharacter* Player = nullptr;
		FVector Origin = FVector::ZeroVector;
		FTransform PlayerTransform;
		TArray<TWeakObjectPtr<ABlockBase>> Blocks;

		bool Initialize()
		{
			World = GEditor->PlayWorld;
			APlayerController* Controller = ::IsValid(World) ? World->GetFirstPlayerController() : nullptr;
			Player = ::IsValid(Controller) ? Cast<AMKCharacter>(Controller->GetPawn()) : nullptr;
			Level = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
			if (::IsValid(Player) == false || ::IsValid(Level) == false || Level->GetBlocks().IsEmpty())
			{
				return false;
			}
			USaveGameSubsystem* Saves = USaveGameSubsystem::Get(World);
			if (::IsValid(Saves) == false || Saves->StartPlaytestSession() == false)
			{
				return false;
			}
			ABlockBase* ReferenceBlock = Level->GetBlocks()[0];
			if (::IsValid(ReferenceBlock) == false)
			{
				return false;
			}
			const FVector ReferenceLocation = ReferenceBlock->GetActorLocation();
			const FVector2D ReferenceCell = UMKBlueprintFunctionLibrary::GetBlockPosition(ReferenceBlock);
			const FVector2D Cell = UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(Player->GetActorLocation()) + FVector2D(0.f, -8.f);
			Origin = FVector(ReferenceLocation.X + (Cell.X - ReferenceCell.X) * BLOCK_SIZE, Player->GetActorLocation().Y,
				ReferenceLocation.Z - (Cell.Y - ReferenceCell.Y) * BLOCK_SIZE);
			PlayerTransform = Player->GetActorTransform();
			return true;
		}

		ABlockBase* Place(FName Row, int32 X, int32 Y)
		{
			const FVector Position = Origin + FVector(X * BLOCK_SIZE, 0.f, -Y * BLOCK_SIZE);
			if (Level->TryPlaceBlock(Row, Position) == false)
			{
				return nullptr;
			}
			ABlockBase* Block = Level->GetBlockAtWorldLocation(Position);
			if (::IsValid(Block))
			{
				Block->SetItemDropEnabled(false);
				Blocks.Add(Block);
			}
			return Block;
		}

		void Clear()
		{
			if (::IsValid(Player))
			{
				Player->GetCharacterMovement()->StopMovementImmediately();
				Player->SetActorTransform(PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
			}
			for (const TWeakObjectPtr<ABlockBase>& Block : Blocks)
			{
				if (Block.IsValid())
				{
					Block->Destroy();
				}
			}
		}
	};

	class FSequentialMonsterDrillCommand : public IAutomationLatentCommand
	{
	public:
		explicit FSequentialMonsterDrillCommand(FAutomationTestBase* InTest)
			: Test(InTest), SetupTime(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (bStarted == false)
			{
				if (Fixture.Initialize() == false)
				{
					if (FPlatformTime::Seconds() - SetupTime < 15.0) { return false; }
					Test->AddError(TEXT("Main PIE geometry fixture did not initialize."));
					return true;
				}
				for (int32 X = -5; X <= 5; ++X)
				{
					if (Fixture.Place(TEXT("Rock"), X, 1) == nullptr)
					{
						Test->AddError(TEXT("Could not place the drill fixture floor."));
						Fixture.Clear();
						return true;
					}
					if (FMath::Abs(X) >= 2 && FMath::Abs(X) <= 4)
					{
						ABlockBase* Block = Fixture.Place(TEXT("Dirt"), X, 0);
						if (::IsValid(Block) == false)
						{
							Test->AddError(TEXT("Could not place a drill target."));
							Fixture.Clear();
							return true;
						}
						Block->GetAbilitySystemComponent()->SetNumericAttributeBase(UAttributeSet_Block::GetDurabilityAttribute(), 90.f);
						Targets.Add(Block);
						PreviousDurabilities.Add(Block, 90.f);
					}
				}
				Monster = Fixture.World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Fixture.Origin, FRotator::ZeroRotator);
				if (Monster.IsValid() == false)
				{
					Test->AddError(TEXT("Could not spawn the drill test monster."));
					Fixture.Clear();
					return true;
				}
				InitialLocation = Monster->GetActorLocation();
				AngryDuration = UDataManager::Get(Fixture.World)->GetMonsterDataTableRow(TEXT("DrillCrawler"))->AngryDuration;
				Monster->GetAbilitySystemComponent()->SetNumericAttributeBase(UAttributeSet_Character::GetDrillingPowerAttribute(), 30.f);
				FGameplayAbilityUtils::ApplyDamage(Monster->GetAbilitySystemComponent(), Fixture.Player->GetAbilitySystemComponent(), 1.f);
				StartTime = Fixture.World->GetTimeSeconds();
				bStarted = true;
				return false;
			}
			if (Monster.IsValid()) { MaximumTravel = FMath::Max(MaximumTravel, FMath::Abs(Monster->GetActorLocation().X - InitialLocation.X)); }
			int32 PartialTargets = 0;
			int32 DestroyedTargets = 0;
			for (const TWeakObjectPtr<ABlockBase>& Target : Targets)
			{
				if (Target.IsValid() == false || Target->IsBreaking()) { ++DestroyedTargets; }
				else
				{
					const float Durability = Target->GetAbilitySystemComponent()->GetNumericAttribute(UAttributeSet_Block::GetDurabilityAttribute());
					const float Previous = PreviousDurabilities.FindRef(Target);
					if (Durability < 90.f) { ++PartialTargets; }
					if (Durability < Previous && Monster.IsValid())
					{
						const float DistanceToFace = FMath::Abs(Target->GetActorLocation().X - Monster->GetActorLocation().X) - BLOCK_SIZE * 0.5f;
						Test->TestTrue(TEXT("Every damaged block is within capsule drill reach"), DistanceToFace <= Monster->GetCapsuleComponent()->GetScaledCapsuleRadius() + 2.6f);
					}
					PreviousDurabilities.Add(Target, Durability);
				}
			}
			Test->TestTrue(TEXT("Only one block has partial drill damage at a time"), PartialTargets <= 1);
			const bool bFinished = Monster.IsValid() && Monster->GetMonsterState() != EMonsterState::Angry;
			if (bFinished || Fixture.World->GetTimeSeconds() - StartTime > 12.0 || Monster.IsValid() == false)
			{
				Test->TestTrue(TEXT("Anger ends after the finite drill sequence"), bFinished);
				Test->TestTrue(TEXT("Anger mines nearby blocks up to the two-or-three block limit"), DestroyedTargets >= 1 && DestroyedTargets <= 3);
				Test->TestTrue(TEXT("Anger moves before mining"), MaximumTravel > BLOCK_SIZE * 0.5f);
				Test->TestTrue(TEXT("Drilling does not extend the anger deadline"), Fixture.World->GetTimeSeconds() - StartTime <= AngryDuration + 0.3f);

				Test->AddInfo(FString::Printf(TEXT("Sequential monster drilling: %d blocks completed, %d partial."), DestroyedTargets, PartialTargets));
				if (Monster.IsValid()) { Monster->Destroy(); }
				Fixture.Clear();
				return true;
			}
			return false;
		}

	private:
		FAutomationTestBase* Test;
		FGeometryFixture Fixture;
		TWeakObjectPtr<AMKMonster> Monster;
		TArray<TWeakObjectPtr<ABlockBase>> Targets;
		TMap<TWeakObjectPtr<ABlockBase>, float> PreviousDurabilities;
		FVector InitialLocation;
		double MaximumTravel = 0.0;
		float AngryDuration = 0.f;
		double SetupTime;
		double StartTime = 0.0;
		bool bStarted = false;
	};

	class FAngerTimeoutCommand : public IAutomationLatentCommand
	{
	public:
		FAngerTimeoutCommand(FAutomationTestBase* InTest, bool bInRestart)
			: Test(InTest), bRestart(bInRestart), SetupTime(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (bStarted == false)
			{
				if (Fixture.Initialize() == false)
				{
					if (FPlatformTime::Seconds() - SetupTime < 15.0) { return false; }
					Test->AddError(TEXT("Main PIE anger fixture did not initialize."));
					return true;
				}
				for (int32 X = -5; X <= 5; ++X)
				{
					if (Fixture.Place(TEXT("Rock"), X, 1) == nullptr)
					{
						Test->AddError(TEXT("Could not place the anger fixture floor."));
						Fixture.Clear();
						return true;
					}
				}
				if (bRestart)
				{
					for (int32 X : { -1, 1 })
					{
						ABlockBase* Block = Fixture.Place(TEXT("Dirt"), X, 0);
						if (::IsValid(Block) == false)
						{
							Test->AddError(TEXT("Could not place the restart drill targets."));
							Fixture.Clear();
							return true;
						}
						Block->GetAbilitySystemComponent()->SetNumericAttributeBase(UAttributeSet_Block::GetDurabilityAttribute(), 10000.f);
						Targets.Add(Block);
					}
				}
				Monster = Fixture.World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Fixture.Origin, FRotator::ZeroRotator);
				if (Monster.IsValid() == false)
				{
					Test->AddError(TEXT("Could not spawn the anger test monster."));
					Fixture.Clear();
					return true;
				}
				AngryDuration = UDataManager::Get(Fixture.World)->GetMonsterDataTableRow(TEXT("DrillCrawler"))->AngryDuration;
				FGameplayAbilityUtils::ApplyDamage(Monster->GetAbilitySystemComponent(), Fixture.Player->GetAbilitySystemComponent(), 1.f);
				StartTime = LastHitTime = Fixture.World->GetTimeSeconds();
				bStarted = true;
				return false;
			}
			const double Now = Fixture.World->GetTimeSeconds();
			if (Monster.IsValid() == false)
			{
				Test->AddError(TEXT("Anger test monster disappeared."));
				Fixture.Clear();
				return true;
			}
			if (bRestart && bRestarted == false && Now - StartTime >= AngryDuration * 0.65f)
			{
				bool bWasDrilling = false;
				for (const TWeakObjectPtr<ABlockBase>& Target : Targets)
				{
					bWasDrilling |= Target.IsValid() && Target->GetAbilitySystemComponent()->GetNumericAttribute(UAttributeSet_Block::GetDurabilityAttribute()) < 10000.f;
				}
				Test->TestTrue(TEXT("Repeated hit happens while mining a block"), bWasDrilling);
				FGameplayAbilityUtils::ApplyDamage(Monster->GetAbilitySystemComponent(), Fixture.Player->GetAbilitySystemComponent(), 1.f);
				LastHitTime = Now;
				bRestarted = true;
			}
			if (bRestarted && bCheckedOriginalDeadline == false && Now - StartTime > AngryDuration + 0.2f)
			{
				Test->TestTrue(TEXT("Repeated hit restarts anger past the original deadline"), Monster->GetMonsterState() == EMonsterState::Angry);
				bCheckedOriginalDeadline = true;
			}
			if (Monster->GetMonsterState() != EMonsterState::Angry || Now - LastHitTime > AngryDuration + 0.5f)
			{
				Test->TestTrue(TEXT("Anger ends at the last-hit deadline even without breakable targets or while still mining"), Monster->GetMonsterState() != EMonsterState::Angry);
				Test->TestTrue(TEXT("Anger lasts for its configured duration"), Now - LastHitTime >= AngryDuration - 0.15f);
				Test->TestTrue(TEXT("Anger has a finite duration"), Now - LastHitTime <= AngryDuration + 0.3f);
				if (bRestart) { Test->TestTrue(TEXT("Restart was verified across the original deadline"), bCheckedOriginalDeadline); }
				Test->AddInfo(FString::Printf(TEXT("Anger timeout: %.2f seconds after last hit; restarted=%d."), Now - LastHitTime, bRestarted));
				Monster->Destroy();
				Fixture.Clear();
				return true;
			}
			return false;
		}

	private:
		FAutomationTestBase* Test;
		FGeometryFixture Fixture;
		TWeakObjectPtr<AMKMonster> Monster;
		TArray<TWeakObjectPtr<ABlockBase>> Targets;
		bool bRestart;
		bool bStarted = false;
		bool bRestarted = false;
		bool bCheckedOriginalDeadline = false;
		double SetupTime;
		double StartTime = 0.0;
		double LastHitTime = 0.0;
		float AngryDuration = 0.f;
	};

	class FMonsterContactKnockbackCommand : public IAutomationLatentCommand
	{
	public:
		explicit FMonsterContactKnockbackCommand(FAutomationTestBase* InTest)
			: Test(InTest), SetupTime(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (bInitialized == false)
			{
				if (Fixture.Initialize() == false)
				{
					if (FPlatformTime::Seconds() - SetupTime < 15.0) { return false; }
					Test->AddError(TEXT("Main PIE knockback fixture did not initialize."));
					return true;
				}
				for (int32 X = -5; X <= 5; ++X)
				{
					if (Fixture.Place(TEXT("Rock"), X, 1) == nullptr)
					{
						Test->AddError(TEXT("Could not place the knockback fixture floor."));
						Fixture.Clear();
						return true;
					}
				}
				PlayerASC = Fixture.Player->GetAbilitySystemComponent();
				Controller = Fixture.World->GetFirstPlayerController();
				InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"));
				OriginalFlyingSpeed = PlayerASC->GetNumericAttributeBase(UAttributeSet_Character::GetFlyingSpeedAttribute());
				ReadyTime = Fixture.World->GetTimeSeconds() + 0.3;
				bInitialized = true;
			}
			if (Fixture.World->GetTimeSeconds() < ReadyTime) { return false; }
			if (bCaseStarted == false)
			{
				Fixture.Player->RespawnAtSpawnLocation();
				PlayerASC->CancelAllAbilities();
				PlayerASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(InvincibleTag));
				PlayerASC->SetNumericAttributeBase(UAttributeSet_Character::GetCurrentHealthAttribute(), 100.f);
				PlayerASC->SetNumericAttributeBase(UAttributeSet_Character::GetFlyingSpeedAttribute(), 0.f);
				UCharacterMovementComponent* Movement = Fixture.Player->GetCharacterMovement();
				Movement->StopMovementImmediately();
				Movement->SetMovementMode(MOVE_Walking);
				Fixture.Player->SetActorLocation(Fixture.Origin + FVector(0.f, 0.f, 1.15f), false, nullptr, ETeleportType::TeleportPhysics);
				StartX = Fixture.Player->GetActorLocation().X;
				Direction = CaseIndex == 1 ? -1.f : 1.f;
				Fixture.Player->SetDrillingVector(FVector(-Direction, 0.f, 0.f));
				StartZ = Fixture.Player->GetActorLocation().Z;
				if (CaseIndex == 3) { PlayerASC->AddLooseGameplayTag(InvincibleTag); }
				if (CaseIndex == 4 && Fixture.Place(TEXT("Rock"), 1, 0) == nullptr)
				{
					Test->AddError(TEXT("Could not place the knockback wall."));
					Cleanup();
					return true;
				}
				FActorSpawnParameters Parameters;
				Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Monster = Fixture.World->SpawnActor<AMKMonster>(AMKMonster::StaticClass(), Fixture.Player->GetActorLocation() - FVector(Direction * (CaseIndex == 5 ? 13.f : 8.f), 0.f, 0.f), FRotator::ZeroRotator, Parameters);
				if (Monster.IsValid() == false)
				{
					Test->AddError(TEXT("Could not spawn the contact test monster."));
					Cleanup();
					return true;
				}
				Monster->GetCharacterMovement()->MaxWalkSpeed = 0.f;
				if (CaseIndex == 5)
				{
					Test->TestFalse(TEXT("Separated monster does not touch the player's body"), Monster->GetCapsuleComponent()->IsOverlappingComponent(Fixture.Player->GetCapsuleComponent()));
				}
				if (CaseIndex == 2) { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Pressed, 1.f)); }
				StartTime = Fixture.World->GetTimeSeconds();
				HitTime = -1.0;
				MaximumTravel = 0.0;
				MaximumRise = 0.0;
				bSawUpwardVelocity = false;
				bSawDownwardVelocity = false;
				bVerifiedPush = false;
				bCaseStarted = true;
				return false;
			}
			const double Now = Fixture.World->GetTimeSeconds();
			const double Travel = (Fixture.Player->GetActorLocation().X - StartX) * Direction;
			MaximumTravel = FMath::Max(MaximumTravel, Travel);
			MaximumRise = FMath::Max(MaximumRise, Fixture.Player->GetActorLocation().Z - StartZ);
			if (HitTime >= 0.0)
			{
				bSawUpwardVelocity |= Fixture.Player->GetVelocity().Z > 1.f;
				bSawDownwardVelocity |= Fixture.Player->GetVelocity().Z < -1.f;
			}
			const float Health = PlayerASC->GetNumericAttribute(UAttributeSet_Character::GetCurrentHealthAttribute());
			if (Health < 99.f && HitTime < 0.0)
			{
				HitTime = Now;
				Fixture.Player->SetDrillingVector(FVector::ZeroVector);
				// 정지, 방향 전환 입력, 비행 중 반대 입력을 각각 검증한다.
				if (CaseIndex == 1) { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Pressed, 1.f)); }
				if (CaseIndex == 2) { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Pressed, 1.f)); }
			}
			if (CaseIndex < 3 && HitTime >= 0.0 && Now - HitTime >= 0.1 && bVerifiedPush == false)
			{
				Test->TestTrue(TEXT("Contact knockback bounces away from the monster"), Travel > 3.f);
				Test->TestTrue(TEXT("Knockback lifts the player off the ground"), MaximumRise > 1.f);
				Test->TestTrue(TEXT("Knockback uses gravity even during flying input"), Fixture.Player->GetCharacterMovement()->IsFalling());
				Test->TestTrue(TEXT("Recovery is still active during the bounce"), Fixture.Player->IsKnockedBack());
				Test->TestFalse(TEXT("Bounce does not apply a constant movement force"), Fixture.Player->GetCharacterMovement()->HasRootMotionSources());
				UMKCharacterVisualComponent* Visual = Fixture.Player->FindComponentByClass<UMKCharacterVisualComponent>();
				Test->TestTrue(TEXT("Sprite keeps its pre-hit facing while moving backward"), IsValid(Visual) && IsValid(Visual->GetSprite())
					&& Visual->GetSprite()->GetPathName().Contains(Direction > 0.f ? TEXT("/Idle_Left/") : TEXT("/Idle_Right/")));
				bVerifiedPush = true;
			}
			if (CaseIndex != 3 && CaseIndex != 5)
			{
				if (HitTime < 0.0 && Now - StartTime < 2.0) { return false; }
				if (HitTime >= 0.0 && Now - HitTime < 1.5
					&& (Now - HitTime < 0.4 || Fixture.Player->IsKnockedBack())) { return false; }
			}
			else if (Now - StartTime < 0.55) { return false; }
			if (CaseIndex == 3 || CaseIndex == 5)
			{
				Test->TestTrue(TEXT("Invincibility or lack of body contact blocks damage"), Health >= 99.f);
				Test->TestTrue(TEXT("Invincibility or lack of body contact blocks knockback"), FMath::Abs(Travel) < 0.5 && MaximumRise < 0.5);
				if (CaseIndex == 3) { PlayerASC->RemoveLooseGameplayTag(InvincibleTag); }
			}
			else
			{
				Test->TestTrue(TEXT("Overlap applies contact damage"), HitTime >= 0.0);
				Test->TestTrue(TEXT("Contact damage pushes the player"), MaximumTravel > (CaseIndex == 4 ? 1.f : 3.f));
				if (CaseIndex == 4) { Test->TestTrue(TEXT("Knockback stops at the wall instead of tunneling"), MaximumTravel <= 3.3); }
				else
				{
					Test->TestTrue(TEXT("Push was observed during the knockback"), bVerifiedPush);
					Test->TestTrue(TEXT("Bounce rises and then falls with gravity"), bSawUpwardVelocity && bSawDownwardVelocity);
					if (CaseIndex == 2) { Test->TestTrue(TEXT("Held flight resumes after recovery"), Fixture.Player->GetCharacterMovement()->IsFlying()); }
					else { Test->TestTrue(TEXT("Grounded bounce lands normally"), Fixture.Player->GetCharacterMovement()->IsMovingOnGround()); }
				}
			}
			Test->TestFalse(TEXT("Knockback recovery ends"), Fixture.Player->IsKnockedBack());
			Test->AddInfo(FString::Printf(TEXT("Contact bounce case %d: max travel %.2f, rise %.2f, health %.1f."), CaseIndex, MaximumTravel, MaximumRise, Health));
			ReleaseInput();
			if (Monster.IsValid()) { Monster->Destroy(); }
			bCaseStarted = false;
			if (++CaseIndex < 6) { return false; }
			Cleanup();
			return true;
		}

	private:
		void ReleaseInput()
		{
			Fixture.Player->SetDrillingVector(FVector::ZeroVector);
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Released, 0.f));
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Released, 0.f));
			Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Released, 0.f));
		}

		void Cleanup()
		{
			ReleaseInput();
			if (Monster.IsValid()) { Monster->Destroy(); }
			PlayerASC->SetNumericAttributeBase(UAttributeSet_Character::GetFlyingSpeedAttribute(), OriginalFlyingSpeed);
			Fixture.Clear();
		}

		FAutomationTestBase* Test;
		FGeometryFixture Fixture;
		UAbilitySystemComponent* PlayerASC = nullptr;
		APlayerController* Controller = nullptr;
		TWeakObjectPtr<AMKMonster> Monster;
		FGameplayTag InvincibleTag;
		double SetupTime;
		double StartTime = 0.0;
		double HitTime = -1.0;
		double ReadyTime = 0.0;
		double StartZ = 0.0;
		double MaximumRise = 0.0;
		bool bSawUpwardVelocity = false;
		bool bSawDownwardVelocity = false;
		double StartX = 0.0;
		double MaximumTravel = 0.0;
		float OriginalFlyingSpeed = 0.f;
		float Direction = 1.f;
		int32 CaseIndex = 0;
		bool bInitialized = false;
		bool bCaseStarted = false;
		bool bVerifiedPush = false;
	};

	class FOneCellShaftCommand : public IAutomationLatentCommand
	{
	public:
		explicit FOneCellShaftCommand(FAutomationTestBase* InTest)
			: Test(InTest), SetupTime(FPlatformTime::Seconds()) {}

		virtual bool Update() override
		{
			if (bStarted == false)
			{
				if (Fixture.Initialize() == false)
				{
					if (FPlatformTime::Seconds() - SetupTime < 15.0) { return false; }
					Test->AddError(TEXT("Main PIE collision fixture did not initialize."));
					return true;
				}
				for (int32 Y = 0; Y < 3; ++Y)
				{
					if (Fixture.Place(TEXT("Rock"), -1, Y) == nullptr || Fixture.Place(TEXT("Rock"), 1, Y) == nullptr)
					{
						Test->AddError(TEXT("Could not place the shaft walls."));
						Fixture.Clear();
						return true;
					}
				}
				if (Fixture.Place(TEXT("Rock"), 0, 3) == nullptr)
				{
					Test->AddError(TEXT("Could not place the shaft floor."));
					Fixture.Clear();
					return true;
				}
				UCapsuleComponent* Capsule = Fixture.Player->GetCapsuleComponent();
				Test->TestEqual(TEXT("Player collision radius is reduced"), Capsule->GetUnscaledCapsuleRadius(), 5.f);
				StartZ = Fixture.Origin.Z + BLOCK_SIZE * 0.5f + Capsule->GetScaledCapsuleHalfHeight() + 2.15f;
				Fixture.Player->GetCharacterMovement()->StopMovementImmediately();
				Fixture.Player->SetActorLocation(FVector(Fixture.Origin.X - 1.7f, Fixture.Origin.Y, StartZ), false, nullptr, ETeleportType::TeleportPhysics);
				StartTime = Fixture.World->GetTimeSeconds();
				bStarted = true;
				return false;
			}
			if (Fixture.World->GetTimeSeconds() - StartTime < 1.2) { return false; }
			const double Z = Fixture.Player->GetActorLocation().Z;
			Test->TestTrue(TEXT("Off-center player falls through a one-cell shaft"), StartZ - Z > BLOCK_SIZE * 2.f);
			Test->TestTrue(TEXT("Shaft floor still blocks the player"), Z >= Fixture.Origin.Z - BLOCK_SIZE * 3.f + BLOCK_SIZE * 0.5f);
			Test->AddInfo(FString::Printf(TEXT("One-cell shaft: player fell %.2f units."), StartZ - Z));
			Fixture.Clear();
			return true;
		}

	private:
		FAutomationTestBase* Test;
		FGeometryFixture Fixture;
		double SetupTime;
		double StartTime = 0.0;
		double StartZ = 0.0;
		bool bStarted = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterSequentialDrillTest, "ProjectMK.PIE.Monster.SequentialSingleBlockDrilling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterSequentialDrillTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FSequentialMonsterDrillCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterOneCellShaftTest, "ProjectMK.PIE.Character.OneCellShaftCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterOneCellShaftTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FOneCellShaftCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterEmptyAngerTimeoutTest, "ProjectMK.PIE.Monster.EmptyAreaAngerTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterEmptyAngerTimeoutTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FAngerTimeoutCommand(this, false));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterAngerRestartTest, "ProjectMK.PIE.Monster.HitWhileDrillingRestartsAnger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterAngerRestartTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FAngerTimeoutCommand(this, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMonsterContactKnockbackTest, "ProjectMK.PIE.Monster.ContactKnockback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterContactKnockbackTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FMonsterContactKnockbackCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
