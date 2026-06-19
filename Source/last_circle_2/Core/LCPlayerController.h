#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LCPlayerController.generated.h"

class ULCHUDWidget;
class UInputMappingContext;
class UInputAction;

UCLASS()
class LAST_CIRCLE_2_API ALCPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALCPlayerController();

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable)
    ULCHUDWidget* GetHUDWidget() const { return HUDWidget; }

    UFUNCTION(BlueprintCallable)
    void ShowNotification(const FString& Message, float Duration = 3.f);

    UFUNCTION(BlueprintCallable)
    void AddKillFeedEntry(const FString& KillerName, const FString& VictimName, const FString& WeaponName, bool bHeadshot);

    UFUNCTION(BlueprintCallable)
    void OnHitConfirmed(bool bIsHeadshot);

    // Cheat code input
    void HandleCheatInput(const FString& Text);

protected:
    UPROPERTY()
    ULCHUDWidget* HUDWidget = nullptr;

    // Input bindings
    void OnMove(const struct FInputActionValue& Value);
    void OnLook(const struct FInputActionValue& Value);
    void OnJumpStarted();
    void OnSprintStarted();
    void OnSprintEnded();
    void OnShootStarted();
    void OnShootReleased();
    void OnADSStarted();
    void OnADSEnded();
    void OnReload();
    void OnInteract();
    void OnSwapWeapon();
    void OnMouseWheel(const struct FInputActionValue& Value);
    void OnUseMedkit();
    void OnThrowGrenade();
    void OnEnterVehicle();
    void OnCrouchToggle();

    void CreateHUD();
};
