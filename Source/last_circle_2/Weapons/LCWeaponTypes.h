#pragma once

#include "CoreMinimal.h"
#include "LCWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
    AssaultRifle  UMETA(DisplayName = "AR"),
    SMG           UMETA(DisplayName = "SMG"),
    Sniper        UMETA(DisplayName = "Sniper"),
    Shotgun       UMETA(DisplayName = "Shotgun"),
    Pistol        UMETA(DisplayName = "Pistol"),
    Melee         UMETA(DisplayName = "Melee"),
    Throwable     UMETA(DisplayName = "Throwable"),
    Special       UMETA(DisplayName = "Special")
};

UENUM(BlueprintType)
enum class EWeaponSpecialType : uint8
{
    None              UMETA(DisplayName = "None"),
    CorrosiveSprayer  UMETA(DisplayName = "Corrosive Sprayer"),
    ArcChainGun       UMETA(DisplayName = "Arc Chain Gun"),
    GravityHammer     UMETA(DisplayName = "Gravity Hammer Launcher"),
    BloodMistHarvester UMETA(DisplayName = "Blood Mist Harvester"),
    RiftRifle         UMETA(DisplayName = "Rift Rifle"),
    InfectionMarker   UMETA(DisplayName = "Infection Marker")
};

USTRUCT(BlueprintType)
struct FWeaponData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName WeaponName = NAME_None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EWeaponCategory Category = EWeaponCategory::AssaultRifle;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Damage = 30.f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float FireRate = 100.f; // ms between shots

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 MagSize = 30;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float ReloadTime = 2.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float SpreadAngle = 1.5f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float RecoilVertical = 0.8f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float RecoilHorizontal = 0.2f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Range = 1000.f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsAutomatic = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 PelletCount = 1; // for shotguns

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EWeaponSpecialType SpecialType = EWeaponSpecialType::None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FColor SpecialColor = FColor::White;

    // Damage over time (for special weapons)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float DOTDamage = 0.f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float DOTDuration = 0.f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float DOTInterval = 0.f;

    // AOE
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float AOERadius = 0.f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float AOEDamage = 0.f;

    // Penetration (Rift Rifle)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 MaxTargets = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float SecondaryTargetDamageMult = 0.f;

    // Knockback (Gravity Hammer)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float KnockbackForce = 0.f;
};
