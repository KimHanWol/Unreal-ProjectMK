// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectMK.h"

#if WITH_EDITOR
#include "DataStorage/Features.h"
#include "Elements/Interfaces/TypedElementDataStorageCompatibilityInterface.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ProjectMK/Actor/Block/BlockBase.h"

namespace
{
	TAutoConsoleVariable<int32> TrackPIEBlocksInDataStorage(
		TEXT("mk.Editor.TrackPIEBlocksInTEDS"), 0,
		TEXT("Include runtime terrain in the editor data hierarchy. Enable only for profiling comparisons."));
}
#endif

void FProjectMKModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
	DataStorageEnabledHandle = UE::Editor::DataStorage::OnEditorDataStorageFeaturesEnabled().AddRaw(this, &FProjectMKModule::RegisterEditorDataStorageFilter);
	RegisterEditorDataStorageFilter();
#endif
}

void FProjectMKModule::ShutdownModule()
{
#if WITH_EDITOR
	UE::Editor::DataStorage::OnEditorDataStorageFeaturesEnabled().Remove(DataStorageEnabledHandle);
#endif
	FDefaultGameModuleImpl::ShutdownModule();
}

#if WITH_EDITOR
void FProjectMKModule::RegisterEditorDataStorageFilter()
{
	using namespace UE::Editor::DataStorage;
	ICompatibilityProvider* Compatibility = GetMutableDataStorageFeature<ICompatibilityProvider>(CompatibilityFeatureName);
	if (Compatibility == nullptr || bRegisteredDataStorageFilter) { return; }
	Compatibility->RegisterRegistrationFilter([](const ICompatibilityProvider&, const UObject* Object)
	{
		if (TrackPIEBlocksInDataStorage.GetValueOnAnyThread() != 0) { return true; }
		const ABlockBase* Block = Cast<ABlockBase>(Object);
		if (Block == nullptr) { Block = Object->GetTypedOuter<ABlockBase>(); }
		// Runtime terrain remains in the world/outliner, but needs no editor data hierarchy or GC observers.
		return Block == nullptr || Block->GetWorld() == nullptr || Block->GetWorld()->IsGameWorld() == false;
	});
	bRegisteredDataStorageFilter = true;
}
#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FProjectMKModule, ProjectMK, "ProjectMK");
