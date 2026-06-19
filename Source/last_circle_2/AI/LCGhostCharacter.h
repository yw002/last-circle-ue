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
        Health = 1.f; MaxHealth = 1.f;
        BaseSpeed = 100.f;
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BuildProceduralBody(FColor(180, 210, 255), FColor(140, 180, 240));
        ApplyVertexColorMaterial();
        if (BodyMesh)
        {
            for (int32 i = 0; i < BodyMesh->GetNumSections(); ++i)
            {
                UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BodyMesh->GetMaterial(i), this);
                if (DynMat) { DynMat->SetScalarParameterValue(FName("Opacity"), 0.35f); BodyMesh->SetMaterial(i, DynMat); }
            }
        }
        if (HeadMesh)
        {
            for (int32 i = 0; i < HeadMesh->GetNumSections(); ++i)
            {
                UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(HeadMesh->GetMaterial(i), this);
                if (DynMat) { DynMat->SetScalarParameterValue(FName("Opacity"), 0.35f); HeadMesh->SetMaterial(i, DynMat); }
            }
        }
        bIsVisible = true;
        AppearTimer = FMath::RandRange(3.f, 8.f);
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
                if (Dist < 400.f && Dist > 20.f)
                {
                    AddMovementInput(Dir, 0.5f);
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
