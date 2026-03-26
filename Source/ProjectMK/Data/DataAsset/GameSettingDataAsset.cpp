#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"

int32 UGameSettingDataAsset::GetEquipmentOverlayZOrder(EEuipmentType EquipmentType) const
{
    for (const FEquipmentOverlayZOrderEntry& Entry : EquipmentOverlayZOrders)
    {
        if (Entry.EquipmentType == EquipmentType)
        {
            return Entry.ZOrder;
        }
    }

    return 0;
}
