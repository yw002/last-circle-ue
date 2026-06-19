#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Characters/LCBaseCharacter.h"
#include "Systems/LCInteractableInterface.h"
#include "Components/PointLightComponent.h"
#include "LCInteractable.generated.h"

// ===================== Campfire =====================
UCLASS()
class LAST_CIRCLE_2_API ALCCampfire : public AActor
{
    GENERATED_BODY()
public:
    ALCCampfire()
    {
        PrimaryActorTick.bCanEverTick = true;
        CampfireMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CampfireMesh"));
        RootComponent = CampfireMesh;
        BuildCampfire();

        PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
        PointLight->SetupAttachment(RootComponent);
        PointLight->SetIntensity(500.f);
        PointLight->SetLightColor(FLinearColor(1.f, 0.5f, 0.1f));
        PointLight->SetAttenuationRadius(200.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        // Heal nearby player
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC && PC->GetPawn())
        {
            float Dist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());
            if (Dist < 200.f)
            {
                ALCBaseCharacter* Char = Cast<ALCBaseCharacter>(PC->GetPawn());
                if (Char && Char->IsAlive())
                {
                    Char->AddHealth(10.f * DeltaSeconds);
                }
            }
        }
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* CampfireMesh = nullptr;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* PointLight = nullptr;

    void BuildCampfire()
    {
        if (!CampfireMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // 5 sticks arranged in a circle
        for (int32 i = 0; i < 5; ++i)
        {
            float A = 2.f * PI * i / 5;
            float R = 8.f;
            int32 Base = V.Num();
            FVector Offset(R * FMath::Cos(A), R * FMath::Sin(A), 0.f);
            V.Add(Offset + FVector(-1, -1, 0)); V.Add(Offset + FVector(1, -1, 0));
            V.Add(Offset + FVector(1, 1, 0)); V.Add(Offset + FVector(-1, 1, 0));
            V.Add(Offset + FVector(-1, -1, 12)); V.Add(Offset + FVector(1, -1, 12));
            V.Add(Offset + FVector(1, 1, 12)); V.Add(Offset + FVector(-1, 1, 12));
            for (int32 j = 0; j < 8; ++j) { C.Add(FColor(80, 40, 10)); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base);T.Add(Base+1);T.Add(Base+5);T.Add(Base);T.Add(Base+5);T.Add(Base+4);
            T.Add(Base+2);T.Add(Base+3);T.Add(Base+7);T.Add(Base+2);T.Add(Base+7);T.Add(Base+6);
            T.Add(Base+4);T.Add(Base+5);T.Add(Base+6);T.Add(Base+4);T.Add(Base+6);T.Add(Base+7);
        }
        CampfireMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};

// ===================== Explosive Barrel =====================
UCLASS()
class LAST_CIRCLE_2_API ALCExplosiveBarrel : public AActor, public ILCInteractableInterface
{
    GENERATED_BODY()
public:
    ALCExplosiveBarrel()
    {
        PrimaryActorTick.bCanEverTick = false;
        Tags.Add(FName("Pickup"));
        BarrelMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BarrelMesh"));
        RootComponent = BarrelMesh;
        BarrelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        BuildBarrel();
    }
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override
    {
        Health -= DamageAmount;
        if (Health <= 0.f && !bExploded)
        {
            Explode(DamageCauser);
        }
        return DamageAmount;
    }
    void Explode(AActor* DamageCauser)
    {
        bExploded = true;
        FVector Loc = GetActorLocation();
        // AOE damage
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(250.f);
        GetWorld()->SweepMultiByChannel(Hits, Loc, Loc, FQuat::Identity, ECC_Visibility, Sphere);
        for (const FHitResult& H : Hits)
        {
            if (ALCBaseCharacter* C = Cast<ALCBaseCharacter>(H.GetActor()))
            {
                float Dist = FVector::Dist(Loc, C->GetActorLocation());
                float Dmg = 80.f * (1.f - Dist / 250.f);
                FDamageEvent DmgEvent;
                C->TakeDamage(FMath::Max(0.f, Dmg), DmgEvent, nullptr, this);
            }
            if (ALCExplosiveBarrel* B = Cast<ALCExplosiveBarrel>(H.GetActor()))
            {
                if (B != this && !B->bExploded)
                {
                    // Chain reaction with slight delay
                    FTimerHandle Handle;
                    GetWorldTimerManager().SetTimer(Handle, [B, DamageCauser]()
                    {
                        if (IsValid(B)) B->Explode(DamageCauser);
                    }, 0.1f, false);
                }
            }
        }
        // Destroy self
        Destroy();
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* BarrelMesh = nullptr;
    UPROPERTY() float Health = 50.f;
    UPROPERTY() bool bExploded = false;

    void BuildBarrel()
    {
        if (!BarrelMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        float R = 8.f, H = 20.f;
        int32 Segs = 12;
        for (int32 i = 0; i < Segs; ++i)
        {
            float A1 = 2.f * PI * i / Segs;
            float A2 = 2.f * PI * ((i+1)%Segs) / Segs;
            int32 Base = V.Num();
            V.Add(FVector(R*FMath::Cos(A1), R*FMath::Sin(A1), 0.f));
            V.Add(FVector(R*FMath::Cos(A1), R*FMath::Sin(A1), H));
            V.Add(FVector(R*FMath::Cos(A2), R*FMath::Sin(A2), 0.f));
            V.Add(FVector(R*FMath::Cos(A2), R*FMath::Sin(A2), H));
            for (int32 j = 0; j < 4; ++j) { C.Add(FColor(150, 30, 20)); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base);T.Add(Base+1);T.Add(Base+2);T.Add(Base+2);T.Add(Base+1);T.Add(Base+3);
        }
        BarrelMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};

// ===================== Destructible Crate =====================
UCLASS()
class LAST_CIRCLE_2_API ALCDestructibleCrate : public AActor
{
    GENERATED_BODY()
public:
    ALCDestructibleCrate()
    {
        PrimaryActorTick.bCanEverTick = false;
        CrateMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CrateMesh"));
        RootComponent = CrateMesh;
        CrateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        BuildCrate();
    }
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override
    {
        Health -= DamageAmount;
        if (Health <= 0.f && !bDestroyed)
        {
            bDestroyed = true;
            // Drop loot
            Destroy();
        }
        return DamageAmount;
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* CrateMesh = nullptr;
    UPROPERTY() float Health = 30.f;
    UPROPERTY() bool bDestroyed = false;

    void BuildCrate()
    {
        if (!CrateMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        float S = 10.f;
        V.Add(FVector(-S,-S,0)); V.Add(FVector(S,-S,0)); V.Add(FVector(S,S,0)); V.Add(FVector(-S,S,0));
        V.Add(FVector(-S,-S,S*2)); V.Add(FVector(S,-S,S*2)); V.Add(FVector(S,S,S*2)); V.Add(FVector(-S,S,S*2));
        for (int32 i = 0; i < 8; ++i) { C.Add(FColor(120, 80, 40)); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);
        CrateMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};

// ===================== Fishing Spot =====================
UCLASS()
class LAST_CIRCLE_2_API ALCFishingSpot : public AActor, public ILCInteractableInterface
{
    GENERATED_BODY()
public:
    ALCFishingSpot() { PrimaryActorTick.bCanEverTick = false; }
    virtual void OnInteract(AActor* Interactor) override
    {
        if (bOnCooldown) return;
        bOnCooldown = true;
        // Wait 2 seconds then give reward
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle, [this, Interactor]()
        {
            if (ALCBaseCharacter* Char = Cast<ALCBaseCharacter>(Interactor))
            {
                Char->AddHealth(50.f);
            }
            FTimerHandle CooldownHandle;
            GetWorldTimerManager().SetTimer(CooldownHandle, [this]() { bOnCooldown = false; }, 8.f, false);
        }, 2.f, false);
    }
protected:
    UPROPERTY() bool bOnCooldown = false;
};
