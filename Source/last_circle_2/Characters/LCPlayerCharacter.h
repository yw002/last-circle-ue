#pragma once

#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "LCPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ULCWeaponBase;
class UInputAction;
class UInputMappingContext;
class SOverlay;
class STextBlock;
class SBox;
class SBorder;

UCLASS()
class LAST_CIRCLE_2_API ALCPlayerCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()

public:
    ALCPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    virtual void Die(AActor* Killer) override;

    // Movement
    void MoveForward(float Value);
    void MoveRight(float Value);
    void LookUp(float Value);
    void LookRight(float Value);
    void SetSprinting(bool bSprint);
    void ToggleCrouch();

    // Shooting
    void StartShooting();
    void StopShooting();
    void Fire();
    void Reload();

    // ADS
    void StartADS();
    void StopADS();

    // Interaction
    void Interact();
    void SwapWeapon();
    void UseMedkit();
    void ThrowGrenade();
    void TryEnterExitVehicle();

    // Weapon management
    UFUNCTION(BlueprintCallable)
    void EquipWeapon(FName WeaponName);
    UFUNCTION(BlueprintCallable)
    void GiveSpecialWeapon(FName WeaponName);
    UFUNCTION(BlueprintCallable)
    FName GetCurrentWeaponName() const { return CurrentWeaponName; }

    // Parachute
    UFUNCTION(BlueprintCallable)
    bool IsParachuting() const { return bIsParachuting; }

    // Vehicle
    UFUNCTION(BlueprintCallable)
    bool IsInVehicle() const { return bIsInVehicle; }

    UFUNCTION(BlueprintCallable)
    AActor* GetCurrentVehicle() const { return CurrentVehicle; }

    // Camera access
    UFUNCTION(BlueprintCallable)
    UCameraComponent* GetFPSCamera() const { return FPSCamera; }

    // Spread tracking
    UFUNCTION(BlueprintCallable)
    float GetCurrentSpread() const { return CurrentSpread; }

    // Crosshair / HUD
    UFUNCTION(BlueprintCallable)
    float GetAmmoInMag() const { return CurrentAmmoInMag; }
    UFUNCTION(BlueprintCallable)
    bool IsReloading() const { return bIsReloading; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UCameraComponent* FPSCamera = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent* WeaponMesh = nullptr;

    // Parachute
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent* ParachuteMesh = nullptr;

    // Weapon state
    UPROPERTY()
    FName CurrentWeaponName = NAME_None;
    UPROPERTY()
    FName WeaponSlotNames[2];
    UPROPERTY()
    int32 CurrentSlotIndex = 0;
    UPROPERTY()
    int32 CurrentAmmoInMag = 0;
    UPROPERTY()
    int32 CurrentTotalAmmo = 0;

    // Shooting state
    UPROPERTY()
    bool bIsShooting = false;
    UPROPERTY()
    bool bIsReloading = false;
    UPROPERTY()
    bool bIsADS = false;
    UPROPERTY()
    float CurrentSpread = 0.f;
    UPROPERTY()
    float LastShotTime = 0.f;
    UPROPERTY()
    float ReloadProgress = 0.f;

    // Movement state
    UPROPERTY()
    bool bIsSprinting = false;
    UPROPERTY()
    float InputForward = 0.f;
    UPROPERTY()
    float InputRight = 0.f;

    // Parachute state
    UPROPERTY()
    bool bIsParachuting = false;
    UPROPERTY()
    float ParachuteStartHeight = 5000.f;

    // Vehicle state
    UPROPERTY()
    bool bIsInVehicle = false;
    UPROPERTY()
    AActor* CurrentVehicle = nullptr;

    // Timers
    FTimerHandle FireTimerHandle;
    FTimerHandle ReloadTimerHandle;

    // ADS interpolation
    UPROPERTY()
    float CurrentFOV = 90.f;
    UPROPERTY()
    float TargetFOV = 90.f;
    UPROPERTY()
    float DefaultFOV = 90.f;

    // Anti-camping
    UPROPERTY()
    float StationaryTime = 0.f;
    UPROPERTY()
    FVector LastPosition = FVector::ZeroVector;

    // Camera recoil
    UPROPERTY()
    float CameraRecoilPitch = 0.f;
    UPROPERTY()
    float CameraRecoilYaw = 0.f;

    UPROPERTY()
    UMaterial* VertexColorWeaponMat = nullptr;

    TSharedPtr<SOverlay> SlateHUD;
    TSharedPtr<STextBlock> SlateAmmoText;
    TSharedPtr<STextBlock> SlateWeaponText;
    TSharedPtr<STextBlock> SlateReloadText;
    TSharedPtr<SBox> SlateCrossTop, SlateCrossBot, SlateCrossLeft, SlateCrossRight;
    float CrosshairGap = 8.f;
    float CrosshairLen = 14.f;
    bool bUIInitialized = false;

    UPROPERTY()
    float RecoilAnimTime = 0.f;

    UPROPERTY()
    float ReloadAnimTime = 0.f;

    UPROPERTY()
    float ReloadAnimDuration = 0.f;

    UPROPERTY()
    FVector WeaponRestPosition = FVector::ZeroVector;

    UPROPERTY()
    FRotator WeaponRestRotation = FRotator::ZeroRotator;

    void UpdateShooting(float DeltaTime);
    void UpdateADS(float DeltaTime);
    void UpdateRecoil(float DeltaTime);
    void UpdateParachute(float DeltaTime);
    void UpdateAntiCamping(float DeltaTime);
    void BuildWeaponModel(FName WeaponName);
    void PerformRaycast();
    void ApplyDamage(AActor* HitActor, float Damage, bool bIsHeadshot, const FVector& HitLocation);
    void SpawnMuzzleFlash();
    void SpawnBloodEffect(const FVector& Location);
    void SpawnBulletHole(const FVector& Location, const FVector& Normal);
    void SpawnShellEjection();

    void OnReloadComplete();
    void InitSlateUI();
    void UpdateSlateCrosshair();
};
