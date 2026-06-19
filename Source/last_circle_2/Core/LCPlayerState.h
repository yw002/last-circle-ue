#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LCPlayerState.generated.h"

UENUM(BlueprintType)
enum class EEquipmentLevel : uint8
{
    None  UMETA(DisplayName = "None"),
    L1    UMETA(DisplayName = "Level 1"),
    L2    UMETA(DisplayName = "Level 2"),
    L3    UMETA(DisplayName = "Level 3")
};

UENUM(BlueprintType)
enum class EScopeType : uint8
{
    None     UMETA(DisplayName = "None"),
    RedDot   UMETA(DisplayName = "Red Dot"),
    Scope2x  UMETA(DisplayName = "2x"),
    Scope4x  UMETA(DisplayName = "4x"),
    Scope8x  UMETA(DisplayName = "8x")
};

USTRUCT(BlueprintType)
struct FWeaponSlotData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName WeaponName = NAME_None;

    UPROPERTY(BlueprintReadWrite)
    int32 CurrentAmmo = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 MagSize = 0;

    UPROPERTY(BlueprintReadWrite)
    bool bIsEmpty = true;
};

UCLASS()
class LAST_CIRCLE_2_API ALCPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ALCPlayerState();

    // Health
    UFUNCTION(BlueprintCallable)
    float GetHealth() const { return Health; }
    UFUNCTION(BlueprintCallable)
    void SetHealth(float NewHealth) { Health = FMath::Clamp(NewHealth, 0.f, MaxHealth); }
    UFUNCTION(BlueprintCallable)
    float GetMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintCallable)
    void AddHealth(float Amount) { SetHealth(Health + Amount); }

    // Kills
    UFUNCTION(BlueprintCallable)
    int32 GetKills() const { return Kills; }
    UFUNCTION(BlueprintCallable)
    void AddKill() { Kills++; }

    // Equipment
    UFUNCTION(BlueprintCallable)
    EEquipmentLevel GetHelmetLevel() const { return HelmetLevel; }
    UFUNCTION(BlueprintCallable)
    void SetHelmetLevel(EEquipmentLevel Level) { HelmetLevel = Level; }

    UFUNCTION(BlueprintCallable)
    EEquipmentLevel GetArmorLevel() const { return ArmorLevel; }
    UFUNCTION(BlueprintCallable)
    void SetArmorLevel(EEquipmentLevel Level) { ArmorLevel = Level; }

    UFUNCTION(BlueprintCallable)
    EScopeType GetScopeType() const { return ScopeType; }
    UFUNCTION(BlueprintCallable)
    void SetScopeType(EScopeType Scope) { ScopeType = Scope; }

    UFUNCTION(BlueprintCallable)
    int32 GetMedkits() const { return Medkits; }
    UFUNCTION(BlueprintCallable)
    void SetMedkits(int32 Count) { Medkits = Count; }
    UFUNCTION(BlueprintCallable)
    void AddMedkit() { Medkits++; }
    UFUNCTION(BlueprintCallable)
    bool UseMedkit();

    // Damage reduction
    UFUNCTION(BlueprintCallable)
    float GetHelmetReduction() const;
    UFUNCTION(BlueprintCallable)
    float GetArmorReduction() const;

    // Weapons
    UFUNCTION(BlueprintCallable)
    FWeaponSlotData GetWeaponSlot(int32 SlotIndex) const;
    UFUNCTION(BlueprintCallable)
    void SetWeaponSlot(int32 SlotIndex, const FWeaponSlotData& Data);
    UFUNCTION(BlueprintCallable)
    int32 GetActiveSlot() const { return ActiveWeaponSlot; }
    UFUNCTION(BlueprintCallable)
    void SetActiveSlot(int32 Slot) { ActiveWeaponSlot = FMath::Clamp(Slot, 0, 1); }
    UFUNCTION(BlueprintCallable)
    void SwapActiveSlot() { ActiveWeaponSlot = (ActiveWeaponSlot == 0) ? 1 : 0; }

    // Ammo pool
    UFUNCTION(BlueprintCallable)
    int32 GetAmmoPool() const { return AmmoPool; }
    UFUNCTION(BlueprintCallable)
    void AddAmmo(int32 Amount) { AmmoPool += Amount; }
    UFUNCTION(BlueprintCallable)
    bool ConsumeAmmo(int32 Amount);

    // Grenades
    UFUNCTION(BlueprintCallable)
    int32 GetGrenadeCount() const { return GrenadeCount; }
    UFUNCTION(BlueprintCallable)
    void AddGrenade() { GrenadeCount++; }
    UFUNCTION(BlueprintCallable)
    bool UseGrenade() { if (GrenadeCount > 0) { GrenadeCount--; return true; } return false; }

protected:
    UPROPERTY(BlueprintReadOnly, Replicated)
    float Health = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float MaxHealth = 500.f;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 Kills = 0;

    UPROPERTY(BlueprintReadOnly, Replicated)
    EEquipmentLevel HelmetLevel = EEquipmentLevel::None;

    UPROPERTY(BlueprintReadOnly, Replicated)
    EEquipmentLevel ArmorLevel = EEquipmentLevel::None;

    UPROPERTY(BlueprintReadOnly, Replicated)
    EScopeType ScopeType = EScopeType::None;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 Medkits = 0;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 GrenadeCount = 0;

    UPROPERTY(Replicated)
    FWeaponSlotData WeaponSlots[2];

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 ActiveWeaponSlot = 0;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 AmmoPool = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
