#include "Core/LCPlayerController.h"
#include "Core/LCGameInstance.h"
#include "Characters/LCPlayerCharacter.h"
#include "UI/LCHUDWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ALCPlayerController::ALCPlayerController()
{
    bShowMouseCursor = false;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = false;
}

void ALCPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Register Enhanced Input mapping context
    if (ULCGameInstance* GI = GetGameInstance<ULCGameInstance>())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        {
            if (GI->IMC_Default)
            {
                Subsystem->AddMappingContext(GI->IMC_Default, 0);
            }
        }
    }

    // Lock mouse to viewport
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    SetShowMouseCursor(false);

    CreateHUD();
}

void ALCPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (ULCGameInstance* GI = GetGameInstance<ULCGameInstance>())
        {
            if (GI->IA_Move)         EIC->BindAction(GI->IA_Move,         ETriggerEvent::Triggered, this, &ALCPlayerController::OnMove);
            if (GI->IA_Look)         EIC->BindAction(GI->IA_Look,         ETriggerEvent::Triggered, this, &ALCPlayerController::OnLook);
            if (GI->IA_Jump)         EIC->BindAction(GI->IA_Jump,         ETriggerEvent::Started,   this, &ALCPlayerController::OnJumpStarted);
            if (GI->IA_Sprint)       EIC->BindAction(GI->IA_Sprint,       ETriggerEvent::Started,   this, &ALCPlayerController::OnSprintStarted);
            if (GI->IA_Sprint)       EIC->BindAction(GI->IA_Sprint,       ETriggerEvent::Completed, this, &ALCPlayerController::OnSprintEnded);
            if (GI->IA_Shoot)        EIC->BindAction(GI->IA_Shoot,        ETriggerEvent::Started,   this, &ALCPlayerController::OnShootStarted);
            if (GI->IA_Shoot)        EIC->BindAction(GI->IA_Shoot,        ETriggerEvent::Completed, this, &ALCPlayerController::OnShootReleased);
            if (GI->IA_ADS)          EIC->BindAction(GI->IA_ADS,          ETriggerEvent::Started,   this, &ALCPlayerController::OnADSStarted);
            if (GI->IA_ADS)          EIC->BindAction(GI->IA_ADS,          ETriggerEvent::Completed, this, &ALCPlayerController::OnADSEnded);
            if (GI->IA_Reload)       EIC->BindAction(GI->IA_Reload,       ETriggerEvent::Started,   this, &ALCPlayerController::OnReload);
            if (GI->IA_Interact)     EIC->BindAction(GI->IA_Interact,     ETriggerEvent::Started,   this, &ALCPlayerController::OnInteract);
            if (GI->IA_SwapWeapon)   EIC->BindAction(GI->IA_SwapWeapon,   ETriggerEvent::Started,   this, &ALCPlayerController::OnSwapWeapon);
            if (GI->IA_MouseWheel)   EIC->BindAction(GI->IA_MouseWheel,   ETriggerEvent::Triggered, this, &ALCPlayerController::OnMouseWheel);
            if (GI->IA_UseMedkit)    EIC->BindAction(GI->IA_UseMedkit,    ETriggerEvent::Started,   this, &ALCPlayerController::OnUseMedkit);
            if (GI->IA_ThrowGrenade) EIC->BindAction(GI->IA_ThrowGrenade, ETriggerEvent::Started,   this, &ALCPlayerController::OnThrowGrenade);
            if (GI->IA_EnterVehicle) EIC->BindAction(GI->IA_EnterVehicle, ETriggerEvent::Started,   this, &ALCPlayerController::OnEnterVehicle);
            if (GI->IA_Crouch)       EIC->BindAction(GI->IA_Crouch,       ETriggerEvent::Started,   this, &ALCPlayerController::OnCrouchToggle);
        }
    }
}

void ALCPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void ALCPlayerController::OnMove(const FInputActionValue& Value)
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        FVector2D Movement = Value.Get<FVector2D>();
        Char->MoveForward(Movement.Y);
        Char->MoveRight(Movement.X);
    }
}

void ALCPlayerController::OnLook(const FInputActionValue& Value)
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        FVector2D Look = Value.Get<FVector2D>();
        Char->LookUp(Look.Y);
        Char->LookRight(Look.X);
    }
}

void ALCPlayerController::OnJumpStarted()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->Jump();
    }
}

void ALCPlayerController::OnSprintStarted()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->SetSprinting(true);
    }
}

void ALCPlayerController::OnSprintEnded()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->SetSprinting(false);
    }
}

void ALCPlayerController::OnShootStarted()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->StartShooting();
    }
}

void ALCPlayerController::OnShootReleased()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->StopShooting();
    }
}

void ALCPlayerController::OnADSStarted()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->StartADS();
    }
}

void ALCPlayerController::OnADSEnded()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->StopADS();
    }
}

void ALCPlayerController::OnReload()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->Reload();
    }
}

void ALCPlayerController::OnInteract()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->Interact();
    }
}

void ALCPlayerController::OnSwapWeapon()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->SwapWeapon();
    }
}

void ALCPlayerController::OnMouseWheel(const FInputActionValue& Value)
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        float WheelValue = Value.Get<float>();
        if (WheelValue > 0.f)
            Char->SwapWeapon();
        else if (WheelValue < 0.f)
            Char->SwapWeapon();
    }
}

void ALCPlayerController::OnUseMedkit()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->UseMedkit();
    }
}

void ALCPlayerController::OnThrowGrenade()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->ThrowGrenade();
    }
}

void ALCPlayerController::OnEnterVehicle()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->TryEnterExitVehicle();
    }
}

void ALCPlayerController::OnCrouchToggle()
{
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        Char->ToggleCrouch();
    }
}

void ALCPlayerController::CreateHUD()
{
    if (IsLocalController())
    {
        HUDWidget = CreateWidget<ULCHUDWidget>(this, ULCHUDWidget::StaticClass());
        if (HUDWidget)
        {
            HUDWidget->AddToViewport(0);
        }
    }
}

void ALCPlayerController::ShowNotification(const FString& Message, float Duration)
{
    if (HUDWidget)
    {
        HUDWidget->ShowNotification(Message, Duration);
    }
}

void ALCPlayerController::AddKillFeedEntry(const FString& KillerName, const FString& VictimName, const FString& WeaponName, bool bHeadshot)
{
    if (HUDWidget)
    {
        HUDWidget->AddKillFeedEntry(KillerName, VictimName, WeaponName, bHeadshot);
    }
}

void ALCPlayerController::OnHitConfirmed(bool bIsHeadshot)
{
    if (HUDWidget)
    {
        HUDWidget->OnHitConfirmed(bIsHeadshot);
    }
}

void ALCPlayerController::HandleCheatInput(const FString& Text)
{
    // Cheat codes: weapon1-weapon6 for special weapons
    if (ALCPlayerCharacter* Char = Cast<ALCPlayerCharacter>(GetPawn()))
    {
        if (Text == "weapon1") Char->GiveSpecialWeapon(FName("CorrosiveSprayer"));
        else if (Text == "weapon2") Char->GiveSpecialWeapon(FName("ArcChainGun"));
        else if (Text == "weapon3") Char->GiveSpecialWeapon(FName("GravityHammer"));
        else if (Text == "weapon4") Char->GiveSpecialWeapon(FName("BloodMistHarvester"));
        else if (Text == "weapon5") Char->GiveSpecialWeapon(FName("RiftRifle"));
        else if (Text == "weapon6") Char->GiveSpecialWeapon(FName("InfectionMarker"));
    }
}
