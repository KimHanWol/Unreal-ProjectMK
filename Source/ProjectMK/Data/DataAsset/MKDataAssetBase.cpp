// LINK

#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"

void UMKDataAssetBase::PostLoad()
{
	Super::PostLoad();

	AsyncLoadAsset();
}