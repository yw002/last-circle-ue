#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "Characters/LCPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "LCZombieCharacter.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCZombieCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCZombieCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;
        AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
        Health = 250.f; MaxHealth = 250.f;
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
        BodyMesh->SetWorldScale3D(FVector(2.2f));
        HeadMesh->SetWorldScale3D(FVector(2.2f));
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;
        AttackCooldown -= DeltaSeconds;
        ALCPlayerCharacter* Player = nullptr;
        float MinDist = MAX_FLT;
        for (TActorIterator<ALCPlayerCharacter> It(GetWorld()); It; ++It)
        {
            if (It->IsAlive())
            {
                float D = FVector::Dist(GetActorLocation(), It->GetActorLocation());
                if (D < MinDist) { MinDist = D; Player = *It; }
            }
        }
        if (Player)
        {
            if (MinDist < 4000.f)
            {
                float Dist2D = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
                FRotator LookAt = (Player->GetActorLocation() - GetActorLocation()).Rotation();
                SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));

                // Approach but stop at melee range (don't push into player)
                if (Dist2D > 350.f)
                {
                    FVector Dir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                    AddMovementInput(Dir, 1.f);
                }

                // Melee attack at 350 range
                if (Dist2D < 350.f && AttackCooldown <= 0.f)
                {
                    float Dmg = 18.f;
                    Player->TakeDamage(Dmg, FDamageEvent(), nullptr, this);
                    AttackCooldown = 0.8f;

                    // Visual feedback: red slash line
                    FVector Start = GetActorLocation() + FVector(0.f, 0.f, 80.f);
                    FVector End = Player->GetActorLocation() + FVector(0.f, 0.f, 60.f);
                    DrawDebugLine(GetWorld(), Start, End, FColor(255, 0, 0), false, 0.2f, 0, 3.0f);
                    DrawDebugSphere(GetWorld(), End, 8.f, 6, FColor(255, 0, 0), false, 0.25f);
                }
            }
        }
    }
protected:
    UPROPERTY() float AttackCooldown = 0.f;
};
