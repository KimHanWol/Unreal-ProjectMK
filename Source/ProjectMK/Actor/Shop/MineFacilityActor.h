#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Actor/Shop/ShopActor.h"
#include "ProjectMK/Data/MineTownData.h"
#include "MineFacilityActor.generated.h"

UCLASS()
class PROJECTMK_API AMineFacilityActor : public AShopActor
{
	GENERATED_BODY()
public:
	virtual bool CanInteract(AActor* Interactor) override;
protected:
	virtual bool OnInteract(AActor* Interactor) override;
public:
	void InitializeFacility(EMineFacilityKind Kind, int32 Depth, const FVector& FloorLocation);
	EMineFacilityKind GetFacilityKind() const;
	int32 GetStationDepth() const;
	FText GetFacilityName() const;
	FName GetArtKey() const;
private:
	EMineFacilityKind FacilityKind = EMineFacilityKind::Elevator;
	int32 StationDepth = 0;
};
