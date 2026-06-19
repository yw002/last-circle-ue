#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Characters/LCBaseCharacter.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LCGiantBoss.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCGiantBoss : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCGiantBoss()
    {
        PrimaryActorTick.bCanEverTick = true;
        Health = 5000.f; MaxHealth = 5000.f;
        BaseSpeed = 150.f;
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        GetCapsuleComponent()->SetCapsuleSize(120.f, 200.f);
        BuildGiantBody();
        if (VertexColorMaterial && BodyMesh)
            BodyMesh->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
        bMovingToZone = false;
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;

        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;

        FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
        FVector Dir = (PlayerLoc - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, 1.f);

        // Attack nearby player
        AttackCooldown -= DeltaSeconds;
        if (AttackCooldown <= 0.f)
        {
            float Dist = FVector::Dist(GetActorLocation(), PlayerLoc);
            if (Dist < 1200.f)
            {
                FDamageEvent DmgEvent;
                PC->GetPawn()->TakeDamage(10.f, DmgEvent, GetController(), this);
                AttackCooldown = 1.5f;
            }
        }

        // Drop ammo every 500 HP lost
        int32 CurrentHPLevel = FMath::FloorToInt(Health / 500.f);
        if (CurrentHPLevel < LastHPLevel)
        {
            LastHPLevel = CurrentHPLevel;
            // Drop ammo box
        }
    }
    virtual void Die(AActor* Killer) override
    {
        if (bIsDead) return;
        bIsDead = true;
        // Slow fall animation over 4 seconds
        GetCharacterMovement()->DisableMovement();
        FTimerHandle FallTimer;
        GetWorldTimerManager().SetTimer(FallTimer, [this]()
        {
            if (IsValid(this))
            {
                FRotator FallRot(0.f, 0.f, 90.f);
                SetActorRotation(FallRot);
            }
        }, 4.f, false);
        Super::Die(Killer);
    }

    UFUNCTION(BlueprintCallable) float GetHealthPercent() const { return Health / MaxHealth; }
protected:
    UPROPERTY() bool bMovingToZone = true;
    UPROPERTY() FVector ZoneTarget = FVector::ZeroVector;
    UPROPERTY() float AttackCooldown = 0.f;
    UPROPERTY() int32 LastHPLevel = 10;

    void BuildGiantBody()
    {
        if (!BodyMesh) return;
        BodyMesh->ClearAllMeshSections();

        float Scale = 10.f; // Giant is 10x scale
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        // Massive torso
        float HX = 30.f, HY = 40.f, HZ = 60.f;
        V.Add(FVector(-HX,-HY,0)); V.Add(FVector(HX,-HY,0)); V.Add(FVector(HX,HY,0)); V.Add(FVector(-HX,HY,0));
        V.Add(FVector(-HX,-HY,HZ)); V.Add(FVector(HX,-HY,HZ)); V.Add(FVector(HX,HY,HZ)); V.Add(FVector(-HX,HY,HZ));
        for (int32 i = 0; i < 8; ++i) { C.Add(FColor(80, 40, 20)); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);

        BodyMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        BodyMesh->SetWorldScale3D(FVector(Scale, Scale, Scale));
    }
};
