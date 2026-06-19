#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Characters/LCBaseCharacter.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LCGhostCharacter.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCGhostCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCGhostCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;
        Health = 1.f; MaxHealth = 1.f; // Ghosts are not killable
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BodyMesh->SetVisibility(true);
        // Semi-transparent appearance
        if (BodyMesh)
        {
            UMaterialInterface* Mat = BodyMesh->GetMaterial(0);
            if (Mat)
            {
                UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Mat, this);
                if (DynMat)
                {
                    DynMat->SetScalarParameterValue(FName("Opacity"), 0.3f);
                    BodyMesh->SetMaterial(0, DynMat);
                }
            }
        }
        bIsVisible = false;
        AppearTimer = FMath::RandRange(5.f, 15.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        AppearTimer -= DeltaSeconds;
        if (AppearTimer <= 0.f)
        {
            bIsVisible = !bIsVisible;
            BodyMesh->SetVisibility(bIsVisible);
            HeadMesh->SetVisibility(bIsVisible);
            AppearTimer = bIsVisible ? FMath::RandRange(3.f, 8.f) : FMath::RandRange(5.f, 15.f);
        }
        // Float toward player slowly
        if (bIsVisible && GetWorld())
        {
            APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
            if (PC && PC->GetPawn())
            {
                FVector Dir = (PC->GetPawn()->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                float Dist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());
                if (Dist < 150.f && Dist > 20.f)
                {
                    AddMovementInput(Dir, 0.2f);
                }
                // Always face player
                FRotator LookAt = (PC->GetPawn()->GetActorLocation() - GetActorLocation()).Rotation();
                SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));
            }
        }
        // Bob up and down
        float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 5.f;
        FVector Loc = GetActorLocation();
        Loc.Z += Bob * DeltaSeconds;
        SetActorLocation(Loc);
    }
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override
    {
        return 0.f; // Ghosts cannot be damaged
    }
protected:
    UPROPERTY() bool bIsVisible = false;
    UPROPERTY() float AppearTimer = 10.f;
};
