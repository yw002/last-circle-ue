#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "LCZombieCharacter.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCZombieCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCZombieCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;
        Health = 80.f; MaxHealth = 80.f;
        BaseSpeed = 250.f;
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BuildProceduralBody(FColor(100, 140, 80), FColor(60, 80, 40));
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;
        // Find nearest target
        AttackCooldown -= DeltaSeconds;
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(400.f);
        GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere);
        ALCBaseCharacter* Nearest = nullptr; float MinDist = MAX_FLT;
        for (const FHitResult& H : Hits)
        {
            if (ALCBaseCharacter* C = Cast<ALCBaseCharacter>(H.GetActor()))
            {
                if (C != this && C->IsAlive())
                {
                    float D = FVector::Dist(GetActorLocation(), C->GetActorLocation());
                    if (D < MinDist) { MinDist = D; Nearest = C; }
                }
            }
        }
        if (Nearest)
        {
            FVector Dir = (Nearest->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            AddMovementInput(Dir, BaseSpeed * DeltaSeconds);
            if (MinDist < 80.f && AttackCooldown <= 0.f)
            {
                float Dmg = 12.f * DamageMultiplier;
                // Reduce damage vs bots
                if (Cast<ALCBaseCharacter>(Nearest) && !Nearest->IsA<ALCPlayerCharacter>())
                    Dmg *= 0.05f; // 20x less vs bots
                FDamageEvent DmgEvent;
                Nearest->TakeDamage(Dmg, DmgEvent, GetController(), this);
                AttackCooldown = 1.4f;
            }
        }
    }
protected:
    UPROPERTY() float AttackCooldown = 0.f;
};
