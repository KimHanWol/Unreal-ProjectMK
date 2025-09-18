// LINK

#include "ProjectMK/Interface/Interactable.h"

bool IInteractable::TryInteract(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return false;
	}

	return Interact(Interactor);
}

bool IInteractable::CanInteract(AActor* Interactor)
{
	if (::IsValid(Interactor) == false)
	{
		return false;
	}

	return true;
}
