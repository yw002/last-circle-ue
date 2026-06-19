#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Weapons/LCWeaponTypes.h"
#include "LCGameInstance.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class LAST_CIRCLE_2_API ULCGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    ULCGameInstance();

    virtual void Init() override;
    virtual void Shutdown() override;

    // Enhanced Input Actions
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Move = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Look = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Jump = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Sprint = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Shoot = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_ADS = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Reload = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Interact = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_SwapWeapon = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_UseMedkit = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_ThrowGrenade = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_EnterVehicle = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_MouseWheel = nullptr;
    UPROPERTY(BlueprintReadOnly)
    UInputAction* IA_Crouch = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UInputMappingContext* IMC_Default = nullptr;

    // Weapon data
    const FWeaponData* GetWeaponData(FName WeaponName) const;

    const TMap<FName, FWeaponData>& GetAllWeaponData() const { return WeaponDataMap; }

protected:
    UPROPERTY()
    TMap<FName, FWeaponData> WeaponDataMap;

    void CreateEnhancedInputAssets();
    void InitializeWeaponData();
};
