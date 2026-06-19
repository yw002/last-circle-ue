#include "Core/LCPlayerState.h"
#include "Net/UnrealNetwork.h"

ALCPlayerState::ALCPlayerState()
{
}

bool ALCPlayerState::UseMedkit()
{
    if (Medkits > 0)
    {
        Medkits--;
        AddHealth(150.f);
        return true;
    }
    return false;
}

float ALCPlayerState::GetHelmetReduction() const
{
    switch (HelmetLevel)
    {
    case EEquipmentLevel::L1: return 0.15f;
    case EEquipmentLevel::L2: return 0.25f;
    case EEquipmentLevel::L3: return 0.35f;
    default: return 0.f;
    }
}

float ALCPlayerState::GetArmorReduction() const
{
    switch (ArmorLevel)
    {
    case EEquipmentLevel::L1: return 0.20f;
    case EEquipmentLevel::L2: return 0.35f;
    case EEquipmentLevel::L3: return 0.50f;
    default: return 0.f;
    }
}

FWeaponSlotData ALCPlayerState::GetWeaponSlot(int32 SlotIndex) const
{
    if (SlotIndex >= 0 && SlotIndex < 2)
        return WeaponSlots[SlotIndex];
    return FWeaponSlotData();
}

void ALCPlayerState::SetWeaponSlot(int32 SlotIndex, const FWeaponSlotData& Data)
{
    if (SlotIndex >= 0 && SlotIndex < 2)
        WeaponSlots[SlotIndex] = Data;
}

bool ALCPlayerState::ConsumeAmmo(int32 Amount)
{
    if (AmmoPool >= Amount)
    {
        AmmoPool -= Amount;
        return true;
    }
    return false;
}

void ALCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALCPlayerState, Health);
    DOREPLIFETIME(ALCPlayerState, Kills);
    DOREPLIFETIME(ALCPlayerState, HelmetLevel);
    DOREPLIFETIME(ALCPlayerState, ArmorLevel);
    DOREPLIFETIME(ALCPlayerState, ScopeType);
    DOREPLIFETIME(ALCPlayerState, Medkits);
    DOREPLIFETIME(ALCPlayerState, GrenadeCount);
    DOREPLIFETIME(ALCPlayerState, ActiveWeaponSlot);
    DOREPLIFETIME(ALCPlayerState, AmmoPool);
}
