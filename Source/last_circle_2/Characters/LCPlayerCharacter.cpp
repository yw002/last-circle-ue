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
#include "UI/LCHUDWidget.h"

ALCPlayerCharacter::ALCPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Don't rotate character to camera direction
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
    GetCharacterMovement()->JumpZVelocity = 420.f;
    GetCharacterMovement()->AirControl = 0.3f;

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

    // Create HUD widget - guaranteed to work
    if (!HUDWidget)
    {
        HUDWidget = CreateWidget<ULCHUDWidget>(GetWorld(), ULCHUDWidget::StaticClass());
        if (HUDWidget)
        {
            HUDWidget->AddToViewport(0);
            FString AmmoText = FString::Printf(TEXT("HUD created OK"));
            UE_LOG(LogTemp, Warning, TEXT("HUD: %s"), *AmmoText);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("HUD: FAILED to create widget!"));
        }
    }

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

    // HUD crosshair
    if (HUDWidget)
    {
        HUDWidget->SetCrosshairSpread(CurrentSpread);
        HUDWidget->SetCrosshairADS(bIsADS);
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

    // Update HUD ammo
    if (HUDWidget)
    {
        int32 Total = (PS) ? PS->GetAmmoPool() : 0;
        HUDWidget->UpdateWeaponInfo(CurrentWeaponName.ToString(), CurrentAmmoInMag, Total);
    }

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

    if (HUDWidget) HUDWidget->SetReloading(true, 0.f);

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

    if (HUDWidget)
    {
        HUDWidget->SetReloading(false, 0.f);
        HUDWidget->UpdateWeaponInfo(CurrentWeaponName.ToString(), CurrentAmmoInMag, PS->GetAmmoPool());
    }
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

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CameraLoc, End, ECC_Visibility, QueryParams);

    FVector TracerStart = FPSCamera->GetComponentLocation() + Direction * 40.f;
    FVector TracerEnd = bHit ? Hit.Location : End;
    DrawDebugLine(GetWorld(), TracerStart, TracerEnd, FColor(255, 200, 0), false, 0.10f, 0, 3.0f);
    if (bHit)
    {
        SpawnHitSpark(Hit.Location);
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
    FVector FlashLoc = BarrelTip;

    UProceduralMeshComponent* FlashMesh = NewObject<UProceduralMeshComponent>(this);
    FlashMesh->RegisterComponent();
    FlashMesh->SetWorldLocation(FlashLoc);
    FlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
    TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

    float R = 8.f;
    Verts.Add(FVector(0, 0, R));
    Verts.Add(FVector(0, 0, -R));
    for (int32 i = 0; i < 8; ++i)
    {
        float A = 2.f * PI * i / 8;
        Verts.Add(FVector(R * 0.6f * FMath::Cos(A), R * 0.6f * FMath::Sin(A), R * 0.3f));
        Colors.Add(FColor(255, 255, 100, 255));
        UVs.Add(FVector2D::ZeroVector);
    }
    Colors.Add(FColor(255, 255, 80, 255)); UVs.Add(FVector2D::ZeroVector);
    Colors.Add(FColor(255, 180, 30, 255)); UVs.Add(FVector2D::ZeroVector);
    for (int32 i = 0; i < 8; ++i)
    {
        Tris.Add(0); Tris.Add(2 + i); Tris.Add(2 + ((i + 1) % 8));
        Tris.Add(1); Tris.Add(2 + ((i + 1) % 8)); Tris.Add(2 + i);
    }

    FlashMesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);
    if (VertexColorWeaponMat) FlashMesh->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorWeaponMat, this));

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, [FlashMesh]()
    {
        if (IsValid(FlashMesh)) FlashMesh->DestroyComponent();
    }, 0.05f, false);
}

void ALCPlayerCharacter::SpawnHitSpark(const FVector& Location)
{
    UProceduralMeshComponent* Spark = NewObject<UProceduralMeshComponent>(this);
    Spark->RegisterComponent();
    Spark->SetWorldLocation(Location);
    Spark->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
    TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

    float R = 5.f;
    float HalfR = R * 0.5f;
    V.Add(FVector(0, 0, R));
    V.Add(FVector(0, 0, -HalfR));
    for (int32 i = 0; i < 8; ++i)
    {
        float A = 2.f * PI * i / 8;
        V.Add(FVector(R * 0.5f * FMath::Cos(A), R * 0.5f * FMath::Sin(A), R * 0.3f));
        C.Add(FColor(255, 220, 50, 255));
        UV.Add(FVector2D::ZeroVector);
    }
    C.Add(FColor(255, 240, 150, 255)); UV.Add(FVector2D::ZeroVector);
    C.Add(FColor(255, 160, 20, 255)); UV.Add(FVector2D::ZeroVector);
    for (int32 i = 0; i < 8; ++i)
    {
        T.Add(0); T.Add(2 + i); T.Add(2 + ((i + 1) % 8));
        T.Add(1); T.Add(2 + ((i + 1) % 8)); T.Add(2 + i);
    }

    Spark->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    if (VertexColorWeaponMat) Spark->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorWeaponMat, this));

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, [Spark]()
    {
        if (IsValid(Spark)) Spark->DestroyComponent();
    }, 0.06f, false);
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

    if (HUDWidget)
    {
        int32 TotalAmmo = 0;
        ALCPlayerState* PS2 = Cast<ALCPlayerState>(GetPlayerState());
        if (PS2) TotalAmmo = PS2->GetAmmoPool();
        HUDWidget->UpdateWeaponInfo(WeaponName.ToString(), CurrentAmmoInMag, TotalAmmo);
    }
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