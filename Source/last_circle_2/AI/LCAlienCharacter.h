#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "LCAlienCharacter.generated.h"

UENUM() enum class EAlienState : uint8 { Idle, Patrol, Attack, Teleport };

UCLASS()
class LAST_CIRCLE_2_API ALCAlienCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCAlienCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;
        Health = 150.f; MaxHealth = 150.f;
        BaseSpeed = 400.f;
        GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
        GetCapsuleComponent()->SetCapsuleSize(50.f, 110.f);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BuildAlienBody();
        BodyMesh->SetWorldScale3D(FVector(1.5f));
        CurrentState = EAlienState::Patrol;
        PatrolTarget = GetActorLocation() + FVector(FMath::RandRange(-400.f, 400.f), FMath::RandRange(-400.f, 400.f), 0.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;

        StateTimer -= DeltaSeconds;

        switch (CurrentState)
        {
        case EAlienState::Idle:
            if (StateTimer <= 0.f)
            {
                CurrentState = EAlienState::Patrol;
                PatrolTarget = GetActorLocation() + FVector(FMath::RandRange(-400.f, 400.f), FMath::RandRange(-400.f, 400.f), 0.f);
            }
            break;
        case EAlienState::Patrol:
            UpdatePatrol(DeltaSeconds);
            break;
        case EAlienState::Attack:
            UpdateAttack(DeltaSeconds);
            break;
        case EAlienState::Teleport:
            UpdateTeleport(DeltaSeconds);
            break;
        }

        // Glow effect
        GlowTimer += DeltaSeconds;
        if (BodyMesh)
        {
            float Glow = 0.5f + 0.5f * FMath::Sin(GlowTimer * 3.f);
            BodyMesh->SetWorldScale3D(FVector(1.f + Glow * 0.02f));
        }
    }

    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override
    {
        float ActualDmg = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
        // When hit, may teleport
        if (FMath::FRand() < 0.3f)
        {
            CurrentState = EAlienState::Teleport;
            StateTimer = 0.5f;
        }
        return ActualDmg;
    }

protected:
    UPROPERTY() EAlienState CurrentState = EAlienState::Patrol;
    UPROPERTY() FVector PatrolTarget = FVector::ZeroVector;
    UPROPERTY() float StateTimer = 3.f;
    UPROPERTY() float AttackCooldown = 0.f;
    UPROPERTY() float GlowTimer = 0.f;
    UPROPERTY() AActor* TargetActor = nullptr;
    UPROPERTY() float TeleportCooldown = 0.f;

    void UpdatePatrol(float DT)
    {
        FVector Dir = (PatrolTarget - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, 1.f);

        float Dist = FVector::Dist2D(GetActorLocation(), PatrolTarget);
        if (Dist < 50.f)
        {
            CurrentState = EAlienState::Idle;
            StateTimer = FMath::RandRange(1.f, 3.f);
        }

        // Detect player
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC && PC->GetPawn())
        {
            float PlayerDist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());
            if (PlayerDist < 3000.f)
            {
                TargetActor = PC->GetPawn();
                CurrentState = EAlienState::Attack;
            }
        }
    }

    void UpdateAttack(float DT)
    {
        if (!TargetActor || !TargetActor->IsValidLowLevel())
        {
            CurrentState = EAlienState::Patrol;
            TargetActor = nullptr;
            return;
        }

        FVector Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, 1.f);

        // Face target
        FRotator LookAt = (TargetActor->GetActorLocation() - GetActorLocation()).Rotation();
        SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));

        float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
        if (Dist > 800.f)
        {
            CurrentState = EAlienState::Patrol;
            TargetActor = nullptr;
            return;
        }

        // Attack with energy blast (20 damage)
        AttackCooldown -= DT;
        if (Dist < 400.f && AttackCooldown <= 0.f)
        {
            FDamageEvent DmgEvent;
            float Dmg = 20.f * DamageMultiplier;
            TargetActor->TakeDamage(Dmg, DmgEvent, GetController(), this);
            AttackCooldown = 1.2f;
        }

        // Occasionally teleport behind target
        TeleportCooldown -= DT;
        if (TeleportCooldown <= 0.f && FMath::FRand() < 0.1f)
        {
            CurrentState = EAlienState::Teleport;
            StateTimer = 0.5f;
        }
    }

    void UpdateTeleport(float DT)
    {
        if (StateTimer <= 0.f)
        {
            // Teleport to random nearby location
            FVector TeleportOffset(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 0.f);
            FVector NewLoc = GetActorLocation() + TeleportOffset;
            SetActorLocation(NewLoc);

            if (TargetActor)
                CurrentState = EAlienState::Attack;
            else
                CurrentState = EAlienState::Patrol;
            TeleportCooldown = 5.f;
        }
    }

    void BuildAlienBody()
    {
        if (!BodyMesh) return;
        BodyMesh->ClearAllMeshSections();

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        // Slim alien torso
        float HX = 6.f, HY = 8.f, HZ = 25.f;
        V.Add(FVector(-HX,-HY,0)); V.Add(FVector(HX,-HY,0)); V.Add(FVector(HX,HY,0)); V.Add(FVector(-HX,HY,0));
        V.Add(FVector(-HX*0.7f,-HY*0.7f,HZ)); V.Add(FVector(HX*0.7f,-HY*0.7f,HZ));
        V.Add(FVector(HX*0.7f,HY*0.7f,HZ)); V.Add(FVector(-HX*0.7f,HY*0.7f,HZ));
        FColor AlienGreen(50, 200, 80);
        for (int32 i = 0; i < 8; ++i) { C.Add(AlienGreen); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);

        // Big head (oval)
        float HeadR = 10.f;
        FVector HeadCenter(0.f, 0.f, HZ + HeadR);
        int32 HeadBase = V.Num();
        int32 Segs = 8;
        for (int32 ring = 0; ring < 4; ++ring)
        {
            float Phi = PI * ring / 4;
            for (int32 seg = 0; seg < Segs; ++seg)
            {
                float Theta = 2.f * PI * seg / Segs;
                FVector P = HeadCenter + FVector(HeadR * 0.8f * FMath::Sin(Phi) * FMath::Cos(Theta),
                                                  HeadR * FMath::Sin(Phi) * FMath::Sin(Theta),
                                                  HeadR * 1.2f * FMath::Cos(Phi));
                V.Add(P);
                // Glowing eyes
                FColor Col = (ring == 1 && (seg == 2 || seg == 6)) ? FColor(255, 255, 0) : FColor(80, 220, 100);
                C.Add(Col); UV.Add(FVector2D::ZeroVector);
            }
        }
        for (int32 ring = 0; ring < 3; ++ring)
        {
            for (int32 seg = 0; seg < Segs; ++seg)
            {
                int32 Curr = HeadBase + ring * Segs + seg;
                int32 Next = HeadBase + ring * Segs + (seg + 1) % Segs;
                int32 Below = HeadBase + (ring + 1) * Segs + seg;
                int32 BelowNext = HeadBase + (ring + 1) * Segs + (seg + 1) % Segs;
                T.Add(Curr); T.Add(Below); T.Add(Next);
                T.Add(Next); T.Add(Below); T.Add(BelowNext);
            }
        }

        // Long arms
        for (int32 side = -1; side <= 1; side += 2)
        {
            float ArmX = side * (HX + 2.f);
            float ArmLen = 30.f;
            int32 ArmBase = V.Num();
            V.Add(FVector(ArmX-1.5f, -1.5f, HZ*0.8f)); V.Add(FVector(ArmX+1.5f, -1.5f, HZ*0.8f));
            V.Add(FVector(ArmX+1.5f, 1.5f, HZ*0.8f)); V.Add(FVector(ArmX-1.5f, 1.5f, HZ*0.8f));
            V.Add(FVector(ArmX-1.f, -1.f, HZ*0.8f - ArmLen)); V.Add(FVector(ArmX+1.f, -1.f, HZ*0.8f - ArmLen));
            V.Add(FVector(ArmX+1.f, 1.f, HZ*0.8f - ArmLen)); V.Add(FVector(ArmX-1.f, 1.f, HZ*0.8f - ArmLen));
            FColor ArmCol(40, 180, 60);
            for (int32 i = 0; i < 8; ++i) { C.Add(ArmCol); UV.Add(FVector2D::ZeroVector); }
            T.Add(ArmBase);T.Add(ArmBase+1);T.Add(ArmBase+5);T.Add(ArmBase);T.Add(ArmBase+5);T.Add(ArmBase+4);
            T.Add(ArmBase+2);T.Add(ArmBase+3);T.Add(ArmBase+7);T.Add(ArmBase+2);T.Add(ArmBase+7);T.Add(ArmBase+6);
            T.Add(ArmBase+4);T.Add(ArmBase+5);T.Add(ArmBase+6);T.Add(ArmBase+4);T.Add(ArmBase+6);T.Add(ArmBase+7);
            T.Add(ArmBase);T.Add(ArmBase+3);T.Add(ArmBase+2);T.Add(ArmBase);T.Add(ArmBase+2);T.Add(ArmBase+1);
            T.Add(ArmBase+3);T.Add(ArmBase);T.Add(ArmBase+4);T.Add(ArmBase+3);T.Add(ArmBase+4);T.Add(ArmBase+7);
            T.Add(ArmBase+1);T.Add(ArmBase+2);T.Add(ArmBase+6);T.Add(ArmBase+1);T.Add(ArmBase+6);T.Add(ArmBase+5);
        }

        BodyMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};
