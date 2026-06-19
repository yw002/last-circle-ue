#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LCBotCharacter.generated.h"

UENUM() enum class EBotState : uint8 { Wander, Attack, Dance };

UCLASS()
class LAST_CIRCLE_2_API ALCBotCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCBotCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;
        Health = 200.f; MaxHealth = 200.f;
        BaseSpeed = 400.f;
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        FColor Skin(FMath::RandRange(100,220), FMath::RandRange(100,220), FMath::RandRange(100,220));
        FColor Cloth(FMath::RandRange(40,200), FMath::RandRange(40,200), FMath::RandRange(40,200));
        BuildProceduralBody(Skin, Cloth);
        ApplyVertexColorMaterial();
        GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
        // Random equipment
        HelmetLevel = static_cast<EEquipmentLevel>(FMath::RandRange(0, 3));
        ArmorLevel = static_cast<EEquipmentLevel>(FMath::RandRange(0, 3));
        CurrentState = EBotState::Wander;
        WanderTarget = GetActorLocation() + FVector(FMath::RandRange(-300.f,300.f), FMath::RandRange(-300.f,300.f), 0.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;

        switch (CurrentState)
        {
        case EBotState::Wander: UpdateWander(DeltaSeconds); break;
        case EBotState::Attack: UpdateAttack(DeltaSeconds); break;
        case EBotState::Dance: break;
        }
    }
    UFUNCTION(BlueprintCallable) EBotState GetBotState() const { return CurrentState; }
    UFUNCTION(BlueprintCallable) void SetBotState(EBotState State) { CurrentState = State; }
protected:
    UPROPERTY() EBotState CurrentState = EBotState::Wander;
    UPROPERTY() FVector WanderTarget = FVector::ZeroVector;
    UPROPERTY() float AttackCooldown = 0.f;
    UPROPERTY() AActor* TargetActor = nullptr;
    UPROPERTY() float UpdateCounter = 0.f;

    void UpdateWander(float DT)
    {
        FVector Dir = (WanderTarget - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, 1.f);

        float Dist = FVector::Dist2D(GetActorLocation(), WanderTarget);
        if (Dist < 50.f)
        {
            WanderTarget = GetActorLocation() + FVector(FMath::RandRange(-500.f,500.f), FMath::RandRange(-500.f,500.f), 0.f);
        }

        // Check for nearby player/bot to attack
        UpdateCounter += DT;
        if (UpdateCounter > 1.f)
        {
            UpdateCounter = 0.f;
            TArray<FHitResult> Hits;
            FCollisionShape Sphere = FCollisionShape::MakeSphere(500.f);
            GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere);
            for (const FHitResult& H : Hits)
            {
                if (ALCBaseCharacter* C = Cast<ALCBaseCharacter>(H.GetActor()))
                {
                    if (C != this && C->IsAlive())
                    {
                        TargetActor = C;
                        CurrentState = EBotState::Attack;
                        break;
                    }
                }
            }
        }
    }

    void UpdateAttack(float DT)
    {
        if (!TargetActor || !Cast<ALCBaseCharacter>(TargetActor)->IsAlive())
        {
            CurrentState = EBotState::Wander;
            TargetActor = nullptr;
            return;
        }

        FVector Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, 1.f);

        float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
        if (Dist < 500.f)
        {
            AttackCooldown -= DT;
            if (AttackCooldown <= 0.f)
            {
                float Dmg = 15.f * DamageMultiplier;
                FDamageEvent DmgEvent;
                TargetActor->TakeDamage(Dmg, DmgEvent, GetController(), this);
                AttackCooldown = 0.5f;
            }
        }
    }
};
