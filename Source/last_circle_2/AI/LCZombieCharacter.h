#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "Characters/LCPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
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
        BaseSpeed = 350.f;
        GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
        GetCapsuleComponent()->SetCapsuleSize(50.f, 110.f);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BuildProceduralBody(FColor(100, 140, 80), FColor(60, 80, 40));
        ApplyVertexColorMaterial();
        GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
        BodyMesh->SetWorldScale3D(FVector(1.5f));
        HeadMesh->SetWorldScale3D(FVector(1.5f));
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;
        AttackCooldown -= DeltaSeconds;
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(3000.f);
        GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere);
        ALCBaseCharacter* Nearest = nullptr; float MinDist = MAX_FLT;
        for (const FHitResult& H : Hits)
        {
            if (ALCBaseCharacter* C = Cast<ALCBaseCharacter>(H.GetActor()))
            {
                if (C != this && C->IsAlive() && C->IsA<ALCPlayerCharacter>())
                {
                    float D = FVector::Dist(GetActorLocation(), C->GetActorLocation());
                    if (D < MinDist) { MinDist = D; Nearest = C; }
                }
            }
        }
        if (Nearest)
        {
            FVector Dir = (Nearest->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            AddMovementInput(Dir, 1.f);
            if (MinDist < 150.f && AttackCooldown <= 0.f)
            {
                float Dmg = 12.f * DamageMultiplier;
                FDamageEvent DmgEvent;
                Nearest->TakeDamage(Dmg, DmgEvent, GetController(), this);
                AttackCooldown = 1.0f;
            }
        }
    }
protected:
    UPROPERTY() float AttackCooldown = 0.f;
};
