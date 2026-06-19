#include "Characters/LCPlayerCharacter.h"
#include "Core/LCGameInstance.h"
#include "Core/LCGameMode.h"
#include "Core/LCPlayerState.h"
#include "Core/LCPlayerController.h"
#include "Weapons/LCWeaponTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "World/LCEnvironmentActor.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameViewportClient.h"
#include "Sound/SoundWaveProcedural.h"

ALCPlayerCharacter::ALCPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Don't rotate character to camera direction
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;

    // FPS Camera
    FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
    FPSCamera->SetupAttachment(GetCapsuleComponent());
    FPSCamera->SetRelativeLocation(FVector(0.f, 0.f, 70.f)); // Eye level
    FPSCamera->bUsePawnControlRotation = true;

    // Weapon mesh (first person)
    WeaponMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(FPSCamera);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Parachute mesh
    ParachuteMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ParachuteMesh"));
    ParachuteMesh->SetupAttachment(GetCapsuleComponent());
    ParachuteMesh->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    ParachuteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ParachuteMesh->SetVisibility(false);
}

void ALCPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("=== LCPlayerCharacter BeginPlay ==="));
    UE_LOG(LogTemp, Warning, TEXT("Player location: %s"), *GetActorLocation().ToString());
    UE_LOG(LogTemp, Warning, TEXT("Controller: %s"), Controller ? *Controller->GetName() : TEXT("NULL"));

    FVector MyLoc = GetActorLocation();

    ALCEnvironmentActor* Env = Cast<ALCEnvironmentActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ALCEnvironmentActor::StaticClass()));
    if (Env)
    {
        float TH = Env->GetHeightAt(MyLoc.X, MyLoc.Y);
        MyLoc.Z = TH + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f;
        SetActorLocation(MyLoc, false, nullptr, ETeleportType::TeleportPhysics);
    }

    VertexColorWeaponMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
    if (!VertexColorWeaponMat)
    {
        VertexColorWeaponMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial"));
    }

    // Slate UI - create overlay with text widgets pushed to game viewport
    InitSlateUI();

    LastPosition = GetActorLocation();

    // Set initial total ammo to 300 BEFORE EquipWeapon so HUD picks it up
    if (ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState()))
    {
        PS->AddAmmo(300);
    }

    EquipWeapon(FName("AKM"));

    if (WeaponMesh)
    {
        WeaponRestPosition = WeaponMesh->GetRelativeLocation();
        WeaponRestRotation = WeaponMesh->GetRelativeRotation();
    }

    UE_LOG(LogTemp, Warning, TEXT("=== LCPlayerCharacter BeginPlay DONE (Z=%f) ==="), GetActorLocation().Z);
}

void ALCPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsDead) return;

    UpdateSlateCrosshair();

    // Update wave info display
    if (SlateWaveText.IsValid())
    {
        if (ALCGameMode* GM = Cast<ALCGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            int32 Wave = GM->GetCurrentWave();
            FString PhaseStr;
            switch (GM->GetCurrentPhase())
            {
            case EGamePhase::WaveCombat: PhaseStr = TEXT("COMBAT"); break;
            case EGamePhase::RestPeriod: PhaseStr = TEXT("REST"); break;
            case EGamePhase::BossFight: PhaseStr = TEXT("BOSS"); break;
            case EGamePhase::Victory: PhaseStr = TEXT("VICTORY"); break;
            default: PhaseStr = TEXT(""); break;
            }
            SlateWaveText->SetText(FText::FromString(FString::Printf(TEXT("WAVE %d/20 - %s"), Wave, *PhaseStr)));
        }
    }

    UpdateParachute(DeltaSeconds);
    UpdateShooting(DeltaSeconds);
    UpdateADS(DeltaSeconds);
    UpdateRecoil(DeltaSeconds);
    UpdateAntiCamping(DeltaSeconds);

    // Recovery spread
    if (!bIsShooting)
    {
        CurrentSpread = FMath::Max(0.f, CurrentSpread - 5.f * DeltaSeconds);
    }

    // Weapon recoil animation
    if (RecoilAnimTime > 0.f)
    {
        RecoilAnimTime -= DeltaSeconds;
        float T = 1.f - FMath::Clamp(RecoilAnimTime / 0.1f, 0.f, 1.f);
        float Kick = FMath::Sin(T * PI) * 3.f;
        if (WeaponMesh)
        {
            WeaponMesh->SetRelativeLocation(WeaponRestPosition + FVector(-Kick, 0.f, Kick * 0.5f));
        }
    }
    else if (WeaponMesh && !bIsReloading)
    {
        WeaponMesh->SetRelativeLocation(WeaponRestPosition);
    }

    // Reload animation
    if (bIsReloading && WeaponMesh)
    {
        ReloadAnimTime += DeltaSeconds;
        float P = FMath::Clamp(ReloadAnimTime / ReloadAnimDuration, 0.f, 1.f);
        float Tilt = FMath::Sin(P * PI * 2.f) * 25.f;
        WeaponMesh->SetRelativeRotation(FRotator(Tilt, 0.f, 0.f));
        WeaponMesh->SetRelativeLocation(WeaponRestPosition + FVector(-5.f, 0.f, -5.f * Tilt / 25.f));
    }
    else if (WeaponMesh && !bIsReloading && RecoilAnimTime <= 0.f)
    {
        WeaponMesh->SetRelativeRotation(WeaponRestRotation);
    }

    // Camera recoil recovery
    if (CameraRecoilPitch != 0.f || CameraRecoilYaw != 0.f)
    {
        CameraRecoilPitch = FMath::FInterpTo(CameraRecoilPitch, 0.f, DeltaSeconds, 8.f);
        CameraRecoilYaw = FMath::FInterpTo(CameraRecoilYaw, 0.f, DeltaSeconds, 8.f);
        AddControllerPitchInput(CameraRecoilPitch * DeltaSeconds);
        AddControllerYawInput(CameraRecoilYaw * DeltaSeconds);
    }
}

void ALCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Input bindings are handled via Enhanced Input in LCPlayerController
}

float ALCPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (!PS) return 0.f;

    bool bIsHeadshot = DamageEvent.DamageTypeClass &&
        DamageEvent.DamageTypeClass->GetName().Contains("Headshot");

    float ActualDamage = DamageAmount;

    if (bIsHeadshot)
    {
        float HelmetRed = PS->GetHelmetReduction();
        ActualDamage *= 2.5f * (1.f - HelmetRed);
    }
    else
    {
        float ArmorRed = PS->GetArmorReduction();
        ActualDamage *= (1.f - ArmorRed);
    }

    PS->SetHealth(PS->GetHealth() - ActualDamage);
    FlashRed();

    if (PS->GetHealth() <= 0.f)
    {
        Die(DamageCauser);
    }

    return ActualDamage;
}

void ALCPlayerCharacter::Die(AActor* Killer)
{
    if (bIsDead) return;
    bIsDead = true;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (PS) PS->SetHealth(0.f);

    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (ALCGameMode* GM = Cast<ALCGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->SetGamePhase(EGamePhase::GameOver);
        GM->OnEntityKilled(Killer, this, false);
    }

    // Hide weapon
    if (WeaponMesh) WeaponMesh->SetVisibility(false);
}

bool ALCPlayerCharacter::CanJumpInternal_Implementation() const
{
    return !bIsDead && !bIsParachuting;
}

// --- Movement ---

void ALCPlayerCharacter::MoveForward(float Value)
{
    InputForward = Value;
    if (Value != 0.f && !bIsParachuting)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void ALCPlayerCharacter::MoveRight(float Value)
{
    InputRight = Value;
    if (Value != 0.f && !bIsParachuting)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
    }
}

void ALCPlayerCharacter::LookUp(float Value)
{
    AddControllerPitchInput(-Value * 0.5f);
}

void ALCPlayerCharacter::LookRight(float Value)
{
    AddControllerYawInput(Value * 0.5f);
}

void ALCPlayerCharacter::SetSprinting(bool bSprint)
{
    bIsSprinting = bSprint;
    float Speed = BaseSpeed;
    if (bSprint) Speed *= 1.5f;
    GetCharacterMovement()->MaxWalkSpeed = Speed * HealthMultiplier;
}

void ALCPlayerCharacter::ToggleCrouch()
{
    if (GetCharacterMovement()->IsCrouching())
    {
        UnCrouch();
    }
    else
    {
        Crouch();
    }
}

// --- Shooting ---

void ALCPlayerCharacter::StartShooting()
{
    if (bIsDead || bIsParachuting || bIsInVehicle) return;
    bIsShooting = true;
    Fire();
}

void ALCPlayerCharacter::StopShooting()
{
    bIsShooting = false;
    GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ALCPlayerCharacter::Fire()
{
    if (bIsDead || bIsReloading || bIsParachuting) return;
    if (CurrentWeaponName.IsNone()) return;

    ULCGameInstance* GI = Cast<ULCGameInstance>(GetGameInstance());
    if (!GI) return;

    const FWeaponData* WeaponData = GI->GetWeaponData(CurrentWeaponName);
    if (!WeaponData) return;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (!PS) return;

    // Check fire rate
    float Now = GetWorld()->GetTimeSeconds();
    float FireInterval = WeaponData->FireRate / 1000.f;
    if (Now - LastShotTime < FireInterval) return;

    // Check ammo
    if (CurrentAmmoInMag <= 0)
    {
        Reload();
        return;
    }

    CurrentAmmoInMag--;
    LastShotTime = Now;

    // Spread
    float Spread = WeaponData->SpreadAngle;
    if (bIsADS) Spread *= 0.3f;
    if (bIsSprinting) Spread *= 2.f;
    Spread += CurrentSpread;
    CurrentSpread = FMath::Min(CurrentSpread + 0.5f, 5.f);

    // Fire pellets (for shotguns)
    for (int32 i = 0; i < WeaponData->PelletCount; ++i)
    {
        PerformRaycast();
    }

    // Apply recoil
    float RecoilV = WeaponData->RecoilVertical * (bIsADS ? 0.5f : 1.f);
    float RecoilH = WeaponData->RecoilHorizontal * (bIsADS ? 0.5f : 1.f);
    CameraRecoilPitch += RecoilV * (0.8f + FMath::FRandRange(0.f, 0.4f));
    CameraRecoilYaw += FMath::FRandRange(-RecoilH, RecoilH);

    // Weapon recoil (visual offset)
    RecoilAnimTime = 0.1f;

    SpawnMuzzleFlash();
    SpawnShellEjection();
    PlayGunshotSound();

    // Update HUD ammo via Slate
    FString AmmoStr = FString::Printf(TEXT("%d / %d"), CurrentAmmoInMag, (PS) ? PS->GetAmmoPool() : 0);
    if (SlateAmmoText.IsValid()) SlateAmmoText->SetText(FText::FromString(AmmoStr));

    // Auto fire for automatic weapons
    if (WeaponData->bIsAutomatic && bIsShooting)
    {
        GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ALCPlayerCharacter::Fire, FireInterval, false);
    }
}

void ALCPlayerCharacter::Reload()
{
    if (bIsReloading || bIsDead) return;
    if (CurrentWeaponName.IsNone()) return;

    ULCGameInstance* GI = Cast<ULCGameInstance>(GetGameInstance());
    if (!GI) return;
    const FWeaponData* WeaponData = GI->GetWeaponData(CurrentWeaponName);
    if (!WeaponData || WeaponData->MagSize <= 0) return;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (!PS || PS->GetAmmoPool() <= 0) return;

    if (CurrentAmmoInMag >= WeaponData->MagSize) return;

    bIsReloading = true;
    ReloadProgress = 0.f;
    ReloadAnimTime = 0.f;
    ReloadAnimDuration = WeaponData->ReloadTime;

    if (SlateReloadText.IsValid()) { SlateReloadText->SetVisibility(EVisibility::Visible); SlateReloadText->SetText(FText::FromString(TEXT("RELOADING..."))); }

    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ALCPlayerCharacter::OnReloadComplete, WeaponData->ReloadTime, false);
}

void ALCPlayerCharacter::OnReloadComplete()
{
    if (!bIsReloading) return;
    bIsReloading = false;

    ULCGameInstance* GI = Cast<ULCGameInstance>(GetGameInstance());
    if (!GI) return;
    const FWeaponData* WeaponData = GI->GetWeaponData(CurrentWeaponName);
    if (!WeaponData) return;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (!PS) return;

    int32 Needed = WeaponData->MagSize - CurrentAmmoInMag;
    int32 Available = FMath::Min(Needed, PS->GetAmmoPool());
    CurrentAmmoInMag += Available;
    PS->ConsumeAmmo(Available);
    ReloadProgress = 0.f;

    if (SlateReloadText.IsValid()) SlateReloadText->SetVisibility(EVisibility::Hidden);
    FString AmmoStr = FString::Printf(TEXT("%d / %d"), CurrentAmmoInMag, PS->GetAmmoPool());
    if (SlateAmmoText.IsValid()) SlateAmmoText->SetText(FText::FromString(AmmoStr));
}

void ALCPlayerCharacter::PerformRaycast()
{
    if (!FPSCamera) return;

    ULCGameInstance* GI = Cast<ULCGameInstance>(GetGameInstance());
    if (!GI) return;
    const FWeaponData* WeaponData = GI->GetWeaponData(CurrentWeaponName);
    if (!WeaponData) return;

    FVector CameraLoc = FPSCamera->GetComponentLocation();
    FRotator CameraRot = FPSCamera->GetComponentRotation();

    // Apply spread
    float Spread = CurrentSpread + (bIsADS ? WeaponData->SpreadAngle * 0.3f : WeaponData->SpreadAngle);
    CameraRot.Pitch += FMath::FRandRange(-Spread, Spread);
    CameraRot.Yaw += FMath::FRandRange(-Spread, Spread);

    FVector Direction = CameraRot.Vector();
    FVector End = CameraLoc + Direction * WeaponData->Range;

    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (CurrentVehicle) QueryParams.AddIgnoredActor(CurrentVehicle);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CameraLoc, End, ECC_Pawn, QueryParams);

    FVector TracerStart = WeaponMesh ? WeaponMesh->GetComponentLocation() : (FPSCamera->GetComponentLocation() + Direction * 40.f);
    FVector TracerEnd = bHit ? Hit.Location : End;
    DrawDebugLine(GetWorld(), TracerStart, TracerEnd, FColor(255, 220, 0), false, 0.12f, 0, 1.0f);
    if (bHit)
    {
        DrawDebugSphere(GetWorld(), Hit.Location, 3.f, 6, FColor(255, 200, 0), false, 0.10f);
    }

    if (bHit && Hit.GetActor())
    {
        AActor* HitActor = Hit.GetActor();
        bool bIsHeadshot = Hit.BoneName == "head" || Hit.BoneName == "Head";

        // Check for cover (simplified: if hit something that's not the target, skip)
        if (ALCBaseCharacter* Target = Cast<ALCBaseCharacter>(HitActor))
        {
            float Damage = WeaponData->Damage * DamageMultiplier;
            ApplyDamage(Target, Damage, bIsHeadshot, Hit.Location);

            // Headshot confirmation
            if (ALCPlayerController* PC = Cast<ALCPlayerController>(GetController()))
            {
                PC->OnHitConfirmed(bIsHeadshot);
            }
        }
    }
}

void ALCPlayerCharacter::ApplyDamage(AActor* HitActor, float Damage, bool bIsHeadshot, const FVector& HitLocation)
{
    if (!HitActor) return;

    FDamageEvent DmgEvent;
    HitActor->TakeDamage(Damage, DmgEvent, GetController(), this);

    SpawnBloodEffect(HitLocation);
}

void ALCPlayerCharacter::SpawnMuzzleFlash()
{
    if (!FPSCamera || !WeaponMesh) return;
    FVector BarrelTip = WeaponMesh->GetComponentTransform().TransformPosition(FVector(35.f, 0.f, 0.f));

    UProceduralMeshComponent* Flash = NewObject<UProceduralMeshComponent>(this);
    Flash->RegisterComponent();
    Flash->SetWorldLocation(BarrelTip);
    Flash->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
    TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

    float OuterR = 10.f;
    float InnerR = 3.f;
    int32 Pts = 6;
    V.Add(FVector::ZeroVector);
    C.Add(FColor(255, 255, 200)); UV.Add(FVector2D::ZeroVector);
    for (int32 i = 0; i < Pts; ++i)
    {
        float A = 2.f * PI * i / Pts + PI / Pts;
        V.Add(FVector(OuterR * FMath::Cos(A), OuterR * FMath::Sin(A), 0.f));
        C.Add(FColor(255, 220, 50)); UV.Add(FVector2D::ZeroVector);
        float Ai = 2.f * PI * (i + 0.5f) / Pts + PI / Pts;
        V.Add(FVector(InnerR * FMath::Cos(Ai), InnerR * FMath::Sin(Ai), 0.f));
        C.Add(FColor(255, 240, 150)); UV.Add(FVector2D::ZeroVector);
    }
    for (int32 i = 0; i < Pts; ++i)
    {
        int32 Tip = 1 + i * 2;
        int32 In = 1 + i * 2 + 1;
        int32 NextTip = 1 + ((i + 1) % Pts) * 2;
        T.Add(0); T.Add(Tip); T.Add(In);
        T.Add(0); T.Add(In); T.Add(NextTip);
    }

    Flash->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    if (VertexColorWeaponMat) Flash->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorWeaponMat, this));

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, [Flash]()
    {
        if (IsValid(Flash)) Flash->DestroyComponent();
    }, 0.05f, false);
}

void ALCPlayerCharacter::SpawnBloodEffect(const FVector& Location)
{
    // Create 12 small red cubes as blood particles
    for (int32 i = 0; i < 12; ++i)
    {
        UProceduralMeshComponent* Blood = NewObject<UProceduralMeshComponent>(this);
        Blood->RegisterComponent();
        Blood->SetWorldLocation(Location);
        Blood->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
        TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

        // Small cube
        float S = 1.f;
        Verts.Add(FVector(-S, -S, -S)); Verts.Add(FVector(S, -S, -S));
        Verts.Add(FVector(S, S, -S)); Verts.Add(FVector(-S, S, -S));
        Verts.Add(FVector(-S, -S, S)); Verts.Add(FVector(S, -S, S));
        Verts.Add(FVector(S, S, S)); Verts.Add(FVector(-S, S, S));

        for (int32 j = 0; j < 8; ++j)
        {
            Colors.Add(FColor(180, 0, 0, 255));
            UVs.Add(FVector2D::ZeroVector);
        }

        Tris.Add(0); Tris.Add(1); Tris.Add(2); Tris.Add(0); Tris.Add(2); Tris.Add(3);
        Tris.Add(4); Tris.Add(6); Tris.Add(5); Tris.Add(4); Tris.Add(7); Tris.Add(6);

        Blood->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);

        // Animate with velocity
        FVector Velocity(FMath::FRandRange(-100.f, 100.f), FMath::FRandRange(-100.f, 100.f), FMath::FRandRange(50.f, 150.f));

        FTimerHandle Handle;
        float Lifetime = FMath::FRandRange(0.3f, 0.8f);
        GetWorldTimerManager().SetTimer(Handle, [Blood, Velocity, Lifetime]()
        {
            if (IsValid(Blood))
            {
                Blood->SetWorldLocation(Blood->GetComponentLocation() + Velocity * 0.016f);
                Blood->DestroyComponent();
            }
        }, Lifetime, false);
    }
}

void ALCPlayerCharacter::SpawnBulletHole(const FVector& Location, const FVector& Normal)
{
    UProceduralMeshComponent* Hole = NewObject<UProceduralMeshComponent>(this);
    Hole->RegisterComponent();
    Hole->SetWorldLocation(Location + Normal * 0.1f);
    Hole->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Simple dark circle approximation
    TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
    TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

    int32 Segs = 8;
    float R = 2.f;
    Verts.Add(FVector::ZeroVector);
    Colors.Add(FColor(30, 30, 30, 255));
    UVs.Add(FVector2D::ZeroVector);

    for (int32 i = 0; i < Segs; ++i)
    {
        float Angle = 2.f * PI * i / Segs;
        Verts.Add(FVector(R * FMath::Cos(Angle), R * FMath::Sin(Angle), 0.f));
        Colors.Add(FColor(30, 30, 30, 255));
        UVs.Add(FVector2D::ZeroVector);
    }

    for (int32 i = 0; i < Segs; ++i)
    {
        Tris.Add(0);
        Tris.Add(1 + i);
        Tris.Add(1 + ((i + 1) % Segs));
    }

    Hole->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);

    // Remove after 10 seconds
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, [Hole]()
    {
        if (IsValid(Hole)) Hole->DestroyComponent();
    }, 10.f, false);
}

void ALCPlayerCharacter::SpawnShellEjection()
{
    UProceduralMeshComponent* Shell = NewObject<UProceduralMeshComponent>(this);
    Shell->RegisterComponent();

    FVector CameraRight = FPSCamera->GetRightVector();
    Shell->SetWorldLocation(FPSCamera->GetComponentLocation() + CameraRight * 20.f);
    Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Small gold cylinder
    TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
    TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

    float R = 0.5f, H = 2.f;
    int32 Segs = 6;
    for (int32 i = 0; i < Segs; ++i)
    {
        float A = 2.f * PI * i / Segs;
        float NA = 2.f * PI * ((i + 1) % Segs) / Segs;
        int32 Base = Verts.Num();
        Verts.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f));
        Verts.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), H));
        Verts.Add(FVector(R * FMath::Cos(NA), R * FMath::Sin(NA), 0.f));
        Verts.Add(FVector(R * FMath::Cos(NA), R * FMath::Sin(NA), H));
        for (int32 j = 0; j < 4; ++j) { Colors.Add(FColor(200, 180, 50, 255)); UVs.Add(FVector2D::ZeroVector); }
        Tris.Add(Base); Tris.Add(Base + 1); Tris.Add(Base + 2);
        Tris.Add(Base + 2); Tris.Add(Base + 1); Tris.Add(Base + 3);
    }

    Shell->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);

    FVector EjectDir = CameraRight * 200.f + FVector(0.f, 0.f, 100.f);

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, [Shell, EjectDir]()
    {
        if (IsValid(Shell))
        {
            Shell->SetWorldLocation(Shell->GetComponentLocation() + EjectDir * 0.016f);
            Shell->DestroyComponent();
        }
    }, 0.5f, false);
}

void ALCPlayerCharacter::PlayGunshotSound()
{
    const int32 SampleRate = 44100;
    const float DurationSec = 0.1f;
    const int32 NumSamples = FMath::CeilToInt(SampleRate * DurationSec);
    const int32 NumBytes = NumSamples * sizeof(int16);

    TArray<uint8> PCM;
    PCM.SetNumZeroed(NumBytes);
    int16* Samples = reinterpret_cast<int16*>(PCM.GetData());

    FRandomStream Rng(FMath::Rand());
    for (int32 i = 0; i < NumSamples; ++i)
    {
        float t = (float)i / NumSamples;
        float Env = FMath::Exp(-t * 30.f);
        float Noise = Rng.FRandRange(-1.f, 1.f);
        Samples[i] = static_cast<int16>(Noise * 32767.f * Env * 0.6f);
    }

    USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
    Sound->SetSampleRate(SampleRate);
    Sound->NumChannels = 1;
    Sound->Duration = DurationSec;
    Sound->bLooping = false;
    Sound->QueueAudio(PCM.GetData(), NumBytes);

    UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.f, 0.9f + FMath::FRandRange(0.f, 0.2f));
}

// --- ADS ---

void ALCPlayerCharacter::StartADS()
{
    if (bIsDead || bIsParachuting) return;
    bIsADS = true;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (!PS) return;

    switch (PS->GetScopeType())
    {
    case EScopeType::RedDot:  TargetFOV = 55.f; break;
    case EScopeType::Scope2x: TargetFOV = 40.f; break;
    case EScopeType::Scope4x: TargetFOV = 25.f; break;
    case EScopeType::Scope8x: TargetFOV = 15.f; break;
    default:                  TargetFOV = 65.f; break;
    }
}

void ALCPlayerCharacter::StopADS()
{
    bIsADS = false;
    TargetFOV = DefaultFOV;
}

void ALCPlayerCharacter::UpdateADS(float DeltaTime)
{
    if (bIsADS && DefaultFOV > 0.f)
    {
        TargetFOV = DefaultFOV * 0.55f;
    }
    else
    {
        TargetFOV = DefaultFOV;
    }

    CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, 10.f);

    if (FPSCamera)
    {
        FPSCamera->SetFieldOfView(CurrentFOV);
    }

    WeaponRestPosition = bIsADS ? FVector(0.f, 0.f, -10.f) : FVector(20.f, 10.f, -15.f);
}

void ALCPlayerCharacter::UpdateRecoil(float DeltaTime)
{
    WeaponRestPosition = bIsADS ? FVector(0.f, 0.f, -10.f) : FVector(20.f, 10.f, -15.f);
}

// --- Parachute ---

void ALCPlayerCharacter::UpdateParachute(float DeltaTime)
{
    if (!bIsParachuting) return;

    // Drift based on input
    if (Controller)
    {
        FRotator YawRot(0, Controller->GetControlRotation().Yaw, 0);
        FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

        AddMovementInput(Forward, InputForward * 200.f * DeltaTime);
        AddMovementInput(Right, InputRight * 200.f * DeltaTime);
    }

    // Gravity descent
    FVector Loc = GetActorLocation();
    Loc.Z -= 150.f * DeltaTime; // Descent speed
    SetActorLocation(Loc);

    // Check if landed (simplified - check ground height)
    if (Loc.Z <= 100.f) // Assume ground is around Z=0-100
    {
        bIsParachuting = false;
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        if (ParachuteMesh) ParachuteMesh->SetVisibility(false);
    }
}

// --- Anti-Camping ---

void ALCPlayerCharacter::UpdateAntiCamping(float DeltaTime)
{
    FVector CurrentPos = GetActorLocation();
    float DistMoved = FVector::Distance(CurrentPos, LastPosition);

    if (DistMoved < 10.f)
    {
        StationaryTime += DeltaTime;
        if (StationaryTime > 30.f)
        {
            // Notify game mode to spawn extra enemies
            if (ALCGameMode* GM = Cast<ALCGameMode>(UGameplayStatics::GetGameMode(this)))
            {
                // GM handles anti-camping spawning
            }
            StationaryTime = 0.f;
        }
    }
    else
    {
        StationaryTime = 0.f;
    }
    LastPosition = CurrentPos;
}

void ALCPlayerCharacter::UpdateShooting(float DeltaTime)
{
    // Auto-fire for automatic weapons is handled via timer in Fire()
}

// --- Interaction ---

void ALCPlayerCharacter::Interact()
{
    if (bIsDead || bIsParachuting) return;

    // Line trace for interactable objects
    FVector Start = FPSCamera->GetComponentLocation();
    FVector End = Start + FPSCamera->GetForwardVector() * 200.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (Hit.GetActor())
        {
            // Check if it implements interactable interface or is a pickup
            // For now, try to pick up
            if (Hit.GetActor()->ActorHasTag(FName("Pickup")))
            {
                // Pick up logic handled by pickup system
            }
        }
    }
}

void ALCPlayerCharacter::SwapWeapon()
{
    if (bIsDead) return;
    CurrentSlotIndex = (CurrentSlotIndex + 1) % 2;
    FName NewWeapon = WeaponSlotNames[CurrentSlotIndex];
    if (!NewWeapon.IsNone())
    {
        EquipWeapon(NewWeapon);
    }
}

void ALCPlayerCharacter::EquipWeapon(FName WeaponName)
{
    ULCGameInstance* GI = Cast<ULCGameInstance>(GetGameInstance());
    if (!GI) return;

    const FWeaponData* WeaponData = GI->GetWeaponData(WeaponName);
    if (!WeaponData) return;

    // Store in slot
    WeaponSlotNames[CurrentSlotIndex] = WeaponName;
    CurrentWeaponName = WeaponName;
    CurrentAmmoInMag = WeaponData->MagSize;

    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (PS)
    {
        FWeaponSlotData SlotData;
        SlotData.WeaponName = WeaponName;
        SlotData.CurrentAmmo = CurrentAmmoInMag;
        SlotData.MagSize = WeaponData->MagSize;
        SlotData.bIsEmpty = false;
        PS->SetWeaponSlot(CurrentSlotIndex, SlotData);
    }

    BuildWeaponModel(WeaponName);

    if (VertexColorWeaponMat && WeaponMesh)
    {
        for (int32 s = 0; s < WeaponMesh->GetNumSections(); ++s)
        {
            WeaponMesh->SetMaterial(s, UMaterialInstanceDynamic::Create(VertexColorWeaponMat, this));
        }
    }

    if (WeaponMesh)
    {
        WeaponRestPosition = WeaponMesh->GetRelativeLocation();
        WeaponRestRotation = WeaponMesh->GetRelativeRotation();
    }

    if (SlateWeaponText.IsValid()) SlateWeaponText->SetText(FText::FromString(WeaponName.ToString()));
    int32 TotalAmmo = 0;
    ALCPlayerState* PS2 = Cast<ALCPlayerState>(GetPlayerState());
    if (PS2) TotalAmmo = PS2->GetAmmoPool();
    FString AmmoStr = FString::Printf(TEXT("%d / %d"), CurrentAmmoInMag, TotalAmmo);
    if (SlateAmmoText.IsValid()) SlateAmmoText->SetText(FText::FromString(AmmoStr));
}

void ALCPlayerCharacter::GiveSpecialWeapon(FName WeaponName)
{
    EquipWeapon(WeaponName);
    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (PS) PS->AddAmmo(120);
}

void ALCPlayerCharacter::UseMedkit()
{
    if (bIsDead) return;
    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (PS)
    {
        if (PS->UseMedkit())
        {
            // Visual feedback
            if (ALCPlayerController* PC = Cast<ALCPlayerController>(GetController()))
            {
                PC->ShowNotification(TEXT("Used Medkit (+150 HP)"));
            }
        }
    }
}

void ALCPlayerCharacter::ThrowGrenade()
{
    if (bIsDead) return;
    ALCPlayerState* PS = Cast<ALCPlayerState>(GetPlayerState());
    if (!PS || !PS->UseGrenade()) return;

    // Spawn grenade projectile
    FVector Start = FPSCamera->GetComponentLocation();
    FVector Dir = FPSCamera->GetForwardVector();

    UProceduralMeshComponent* Grenade = NewObject<UProceduralMeshComponent>(this);
    Grenade->RegisterComponent();
    Grenade->SetWorldLocation(Start + Dir * 50.f);

    // Simple sphere
    TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
    TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

    float R = 5.f;
    for (int32 i = 0; i < 8; ++i)
    {
        float A = 2.f * PI * i / 8;
        Verts.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f));
        Colors.Add(FColor(50, 80, 50, 255));
        UVs.Add(FVector2D::ZeroVector);
    }
    for (int32 i = 0; i < 6; ++i)
    {
        Tris.Add(0); Tris.Add(1 + i); Tris.Add(1 + ((i + 1) % 8));
    }

    Grenade->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);

    // Explode after 3 seconds
    FVector Velocity = Dir * 1500.f + FVector(0.f, 0.f, 400.f);
    FTimerHandle ExplodeTimer;
    GetWorldTimerManager().SetTimer(ExplodeTimer, [this, Grenade]()
    {
        if (!IsValid(Grenade)) return;
        FVector ExplodeLoc = Grenade->GetComponentLocation();
        Grenade->DestroyComponent();

        // AOE damage
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(100.f);
        GetWorld()->SweepMultiByChannel(Hits, ExplodeLoc, ExplodeLoc, FQuat::Identity, ECC_Visibility, Sphere);

        for (const FHitResult& Hit : Hits)
        {
            if (ALCBaseCharacter* Target = Cast<ALCBaseCharacter>(Hit.GetActor()))
            {
                if (Target != this)
                {
                    FDamageEvent DmgEvent;
                    Target->TakeDamage(100.f, DmgEvent, GetController(), this);
                }
            }
        }
    }, 3.f, false);
}

void ALCPlayerCharacter::TryEnterExitVehicle()
{
    if (bIsInVehicle && CurrentVehicle)
    {
        // Exit vehicle
        bIsInVehicle = false;
        PossessedBy(GetController());
        SetActorHiddenInGame(false);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);

        FVector VehicleLoc = CurrentVehicle->GetActorLocation();
        SetActorLocation(VehicleLoc + FVector(100.f, 0.f, 50.f));
        CurrentVehicle = nullptr;
    }
    else
    {
        // Try to find nearby vehicle
        FVector Start = GetActorLocation();
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(200.f);
        GetWorld()->SweepMultiByChannel(Hits, Start, Start, FQuat::Identity, ECC_Visibility, Sphere);

        for (const FHitResult& Hit : Hits)
        {
            if (Hit.GetActor() && Hit.GetActor()->ActorHasTag(FName("Vehicle")))
            {
                CurrentVehicle = Hit.GetActor();
                bIsInVehicle = true;
                // Simplified: just possess the vehicle
                GetController()->Possess(Cast<APawn>(CurrentVehicle));
                break;
            }
        }
    }
}

void ALCPlayerCharacter::BuildWeaponModel(FName WeaponName)
{
    if (!WeaponMesh) return;
    WeaponMesh->ClearAllMeshSections();

    ULCGameInstance* GI = Cast<ULCGameInstance>(GetGameInstance());
    if (!GI) return;
    const FWeaponData* Data = GI->GetWeaponData(WeaponName);
    if (!Data) return;

    FColor GunColor(60, 60, 60, 255);
    if (Data->Category == EWeaponCategory::Melee)
    {
        GunColor = FColor(120, 80, 40, 255);
    }
    else if (Data->Category == EWeaponCategory::Special)
    {
        GunColor = Data->SpecialColor;
    }

    // Gun body (box)
    float BodyLength = 40.f;
    if (Data->Category == EWeaponCategory::Pistol) BodyLength = 20.f;
    if (Data->Category == EWeaponCategory::Sniper) BodyLength = 55.f;
    if (Data->Category == EWeaponCategory::Melee) BodyLength = 35.f;

    // Body
    TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
    TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

    float HX = BodyLength * 0.5f, HY = 3.f, HZ = 4.f;
    // Simple box for gun body
    Verts.Add(FVector(-HX, -HY, -HZ)); Verts.Add(FVector(HX, -HY, -HZ));
    Verts.Add(FVector(HX, HY, -HZ)); Verts.Add(FVector(-HX, HY, -HZ));
    Verts.Add(FVector(-HX, -HY, HZ)); Verts.Add(FVector(HX, -HY, HZ));
    Verts.Add(FVector(HX, HY, HZ)); Verts.Add(FVector(-HX, HY, HZ));

    for (int32 i = 0; i < 8; ++i) { Colors.Add(GunColor); UVs.Add(FVector2D::ZeroVector); }

    Tris.Add(0); Tris.Add(1); Tris.Add(5); Tris.Add(0); Tris.Add(5); Tris.Add(4);
    Tris.Add(2); Tris.Add(3); Tris.Add(7); Tris.Add(2); Tris.Add(7); Tris.Add(6);
    Tris.Add(4); Tris.Add(5); Tris.Add(6); Tris.Add(4); Tris.Add(6); Tris.Add(7);
    Tris.Add(0); Tris.Add(3); Tris.Add(2); Tris.Add(0); Tris.Add(2); Tris.Add(1);
    Tris.Add(3); Tris.Add(0); Tris.Add(4); Tris.Add(3); Tris.Add(4); Tris.Add(7);
    Tris.Add(1); Tris.Add(2); Tris.Add(6); Tris.Add(1); Tris.Add(6); Tris.Add(5);

    WeaponMesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, true);

    // Barrel (cylinder approximation) for guns
    if (Data->Category != EWeaponCategory::Melee)
    {
        TArray<FVector> BVerts; TArray<int32> BTris; TArray<FVector> BNormals;
        TArray<FColor> BColors; TArray<FVector2D> BUvs; TArray<FProcMeshTangent> BTangents;

        float BR = 1.5f, BLen = 15.f;
        int32 Segs = 8;
        for (int32 i = 0; i < Segs; ++i)
        {
            float A = 2.f * PI * i / Segs;
            float NA = 2.f * PI * ((i + 1) % Segs) / Segs;
            int32 Base = BVerts.Num();
            BVerts.Add(FVector(HX, BR * FMath::Cos(A), BR * FMath::Sin(A)));
            BVerts.Add(FVector(HX + BLen, BR * FMath::Cos(A), BR * FMath::Sin(A)));
            BVerts.Add(FVector(HX, BR * FMath::Cos(NA), BR * FMath::Sin(NA)));
            BVerts.Add(FVector(HX + BLen, BR * FMath::Cos(NA), BR * FMath::Sin(NA)));
            for (int32 j = 0; j < 4; ++j) { BColors.Add(FColor(40, 40, 40, 255)); BUvs.Add(FVector2D::ZeroVector); }
            BTris.Add(Base); BTris.Add(Base + 1); BTris.Add(Base + 2);
            BTris.Add(Base + 2); BTris.Add(Base + 1); BTris.Add(Base + 3);
        }

        WeaponMesh->CreateMeshSection(1, BVerts, BTris, BNormals, BUvs, BColors, BTangents, true);
    }

    // Magazine (small box under body)
    if (Data->MagSize > 0 && Data->Category != EWeaponCategory::Melee)
    {
        TArray<FVector> MVerts; TArray<int32> MTris; TArray<FVector> MNormals;
        TArray<FColor> MColors; TArray<FVector2D> MUvs; TArray<FProcMeshTangent> MTangents;

        float MX = 5.f, MY = 2.f, MZ = 8.f;
        MVerts.Add(FVector(-MX, -MY, -HZ - MZ)); MVerts.Add(FVector(MX, -MY, -HZ - MZ));
        MVerts.Add(FVector(MX, MY, -HZ - MZ)); MVerts.Add(FVector(-MX, MY, -HZ - MZ));
        MVerts.Add(FVector(-MX, -MY, -HZ)); MVerts.Add(FVector(MX, -MY, -HZ));
        MVerts.Add(FVector(MX, MY, -HZ)); MVerts.Add(FVector(-MX, MY, -HZ));

        for (int32 i = 0; i < 8; ++i) { MColors.Add(FColor(50, 50, 50, 255)); MUvs.Add(FVector2D::ZeroVector); }

        MTris.Add(0); MTris.Add(1); MTris.Add(5); MTris.Add(0); MTris.Add(5); MTris.Add(4);
        MTris.Add(2); MTris.Add(3); MTris.Add(7); MTris.Add(2); MTris.Add(7); MTris.Add(6);
        MTris.Add(4); MTris.Add(5); MTris.Add(6); MTris.Add(4); MTris.Add(6); MTris.Add(7);
        MTris.Add(0); MTris.Add(3); MTris.Add(2); MTris.Add(0); MTris.Add(2); MTris.Add(1);

        WeaponMesh->CreateMeshSection(2, MVerts, MTris, MNormals, MUvs, MColors, MTangents, true);
    }
}

void ALCPlayerCharacter::InitSlateUI()
{
    if (bUIInitialized) return;
    if (!GEngine || !GEngine->GameViewport) return;

    UObject* FontObj = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    FSlateFontInfo BigFont(FontObj, 30);
    FSlateFontInfo MedFont(FontObj, 14);

    SAssignNew(SlateAmmoText, STextBlock)
        .Font(BigFont)
        .ColorAndOpacity(FLinearColor::White)
        .Text(FText::FromString(TEXT("30 / 300")))
        .Justification(ETextJustify::Right);

    SAssignNew(SlateWeaponText, STextBlock)
        .Font(MedFont)
        .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
        .Text(FText::FromString(TEXT("AKM")))
        .Justification(ETextJustify::Right);

    SAssignNew(SlateReloadText, STextBlock)
        .Font(MedFont)
        .ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.f))
        .Visibility(EVisibility::Hidden);

    SAssignNew(SlateWaveText, STextBlock)
        .Font(BigFont)
        .ColorAndOpacity(FLinearColor(1.f, 0.9f, 0.3f))
        .Text(FText::FromString(TEXT("WAVE 1 / 20")));

    SAssignNew(SlateCrossTop, SBox)
        .WidthOverride(2.f).HeightOverride(16.f)
        [SNew(SBorder).BorderBackgroundColor(FLinearColor(0.f, 1.f, 0.f, 0.85f))];
    SAssignNew(SlateCrossBot, SBox)
        .WidthOverride(2.f).HeightOverride(16.f)
        [SNew(SBorder).BorderBackgroundColor(FLinearColor(0.f, 1.f, 0.f, 0.85f))];
    SAssignNew(SlateCrossLeft, SBox)
        .WidthOverride(16.f).HeightOverride(2.f)
        [SNew(SBorder).BorderBackgroundColor(FLinearColor(0.f, 1.f, 0.f, 0.85f))];
    SAssignNew(SlateCrossRight, SBox)
        .WidthOverride(16.f).HeightOverride(2.f)
        [SNew(SBorder).BorderBackgroundColor(FLinearColor(0.f, 1.f, 0.f, 0.85f))];

    SAssignNew(SlateHUD, SOverlay)
    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
    [
        SNew(SCanvas)
        + SCanvas::Slot()
          .Position(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(-1.f, -(CrosshairGap + CrosshairLen)); }))
          .Size(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(2.f, CrosshairLen); }))
          [SlateCrossTop.ToSharedRef()]
        + SCanvas::Slot()
          .Position(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(-1.f, CrosshairGap); }))
          .Size(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(2.f, CrosshairLen); }))
          [SlateCrossBot.ToSharedRef()]
        + SCanvas::Slot()
          .Position(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(-(CrosshairGap + CrosshairLen), -1.f); }))
          .Size(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(CrosshairLen, 2.f); }))
          [SlateCrossLeft.ToSharedRef()]
        + SCanvas::Slot()
          .Position(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(CrosshairGap, -1.f); }))
          .Size(TAttribute<FVector2D>::CreateLambda([this]() { return FVector2D(CrosshairLen, 2.f); }))
          [SlateCrossRight.ToSharedRef()]
    ]
    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, 20, 0, 0)
    [SlateWaveText.ToSharedRef()]
    + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0, 0, 30, 30)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SlateWeaponText.ToSharedRef()]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SlateAmmoText.ToSharedRef()]
    ]
    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 120)
    [SlateReloadText.ToSharedRef()];

    GEngine->GameViewport->AddViewportWidgetContent(SlateHUD.ToSharedRef());
    bUIInitialized = true;
}

void ALCPlayerCharacter::UpdateSlateCrosshair()
{
    if (!bUIInitialized) { InitSlateUI(); if (!bUIInitialized) return; }

    float TargetGap = (bIsADS ? 5.f : 10.f) + CurrentSpread * (bIsADS ? 1.5f : 4.f);
    float TargetLen = bIsADS ? 8.f : 16.f;

    CrosshairGap = FMath::FInterpTo(CrosshairGap, TargetGap, 0.016f, 12.f);
    CrosshairLen = FMath::FInterpTo(CrosshairLen, TargetLen, 0.016f, 12.f);
}