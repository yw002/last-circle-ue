#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/DamageEvents.h"
#include "Core/LCGameState.h"
#include "Kismet/GameplayStatics.h"
#include "LCEffectsSystem.generated.h"

// ===================== Blood Effect =====================
UCLASS()
class LAST_CIRCLE_2_API ALCBloodEffect : public AActor
{
    GENERATED_BODY()
public:
    ALCBloodEffect();
    virtual void Tick(float DT) override;
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* BloodMesh = nullptr;
    UPROPERTY() float Lifespan = 1.5f;
    UPROPERTY() float Elapsed = 0.f;
    void BuildBloodParticles();
};

// ===================== Muzzle Flash =====================
UCLASS()
class LAST_CIRCLE_2_API ALCMuzzleFlash : public AActor
{
    GENERATED_BODY()
public:
    ALCMuzzleFlash();
    virtual void Tick(float DT) override;
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* FlashMesh = nullptr;
    UPROPERTY() float Timer = 0.f;
    void BuildFlashMesh();
};

// ===================== Explosion Effect =====================
UCLASS()
class LAST_CIRCLE_2_API ALCExplosionEffect : public AActor
{
    GENERATED_BODY()
public:
    ALCExplosionEffect();
    virtual void Tick(float DT) override;
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* ExplosionMesh = nullptr;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* ExplosionLight = nullptr;
    UPROPERTY() float Timer = 0.f;
    void BuildExplosionMesh();
};

// ===================== Weather Particles =====================
USTRUCT()
struct FParticleData
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY()
    float Lifetime = 2.f;
};

UCLASS()
class LAST_CIRCLE_2_API ALCWeatherParticles : public AActor
{
    GENERATED_BODY()
public:
    ALCWeatherParticles();
    virtual void Tick(float DT) override;

    UFUNCTION(BlueprintCallable)
    void SetWeather(EWeatherType Weather);
    UFUNCTION(BlueprintCallable)
    EWeatherType GetWeather() const { return CurrentWeather; }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* ParticleMesh = nullptr;
    UPROPERTY() bool bActive = false;
    UPROPERTY() EWeatherType CurrentWeather = EWeatherType::Clear;
    UPROPERTY() TArray<FParticleData> ActiveParticles;
    UPROPERTY() float Timer = 0.f;
    UPROPERTY() float SpawnInterval = 0.02f;
    UPROPERTY() FColor ParticleColor = FColor::White;

    void SpawnParticle();
    void RebuildParticles();
};

// ===================== Shell Ejection =====================
UCLASS()
class LAST_CIRCLE_2_API ALCShellEjection : public AActor
{
    GENERATED_BODY()
public:
    ALCShellEjection()
    {
        PrimaryActorTick.bCanEverTick = true;
        ShellMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShellMesh"));
        RootComponent = ShellMesh;
        ShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BuildShellMesh();
        SetLifeSpan(2.f);
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        Velocity.Z -= 400.f * DT; // Gravity
        AddActorWorldOffset(Velocity * DT);
        AddActorWorldRotation(FRotator(500.f * DT, 300.f * DT, 0.f));
    }

    UFUNCTION(BlueprintCallable)
    void SetEjectionVelocity(const FVector& Vel) { Velocity = Vel; }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* ShellMesh = nullptr;
    UPROPERTY() FVector Velocity = FVector(0.f, 50.f, 100.f);

    void BuildShellMesh()
    {
        if (!ShellMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // Small brass cylinder
        float R = 0.8f, H = 3.f;
        int32 Segs = 6;
        for (int32 i = 0; i < Segs; ++i)
        {
            float A1 = 2.f * PI * i / Segs;
            float A2 = 2.f * PI * ((i+1)%Segs) / Segs;
            int32 Base = V.Num();
            V.Add(FVector(R*FMath::Cos(A1), R*FMath::Sin(A1), 0.f));
            V.Add(FVector(R*FMath::Cos(A1), R*FMath::Sin(A1), H));
            V.Add(FVector(R*FMath::Cos(A2), R*FMath::Sin(A2), 0.f));
            V.Add(FVector(R*FMath::Cos(A2), R*FMath::Sin(A2), H));
            FColor Brass(200, 170, 50);
            for (int32 j = 0; j < 4; ++j) { C.Add(Brass); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base);T.Add(Base+1);T.Add(Base+2);T.Add(Base+2);T.Add(Base+1);T.Add(Base+3);
        }
        ShellMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
};

// ===================== Bullet Hole Decal =====================
UCLASS()
class LAST_CIRCLE_2_API ALCBulletHole : public AActor
{
    GENERATED_BODY()
public:
    ALCBulletHole()
    {
        PrimaryActorTick.bCanEverTick = false;
        HoleMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HoleMesh"));
        RootComponent = HoleMesh;
        HoleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BuildHoleMesh();
        SetLifeSpan(30.f); // Decals last 30 seconds
    }

    UFUNCTION(BlueprintCallable)
    void SetHitNormal(const FVector& Normal)
    {
        FRotator Rot = Normal.Rotation();
        SetActorRotation(Rot);
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* HoleMesh = nullptr;

    void BuildHoleMesh()
    {
        if (!HoleMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // Dark circle
        float R = 1.5f;
        int32 Segs = 8;
        V.Add(FVector(0, 0, 0.1f)); // Center
        C.Add(FColor(10, 10, 10, 200)); UV.Add(FVector2D::ZeroVector);
        for (int32 i = 0; i < Segs; ++i)
        {
            float A = 2.f * PI * i / Segs;
            V.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f));
            C.Add(FColor(30, 30, 30, 150));
            UV.Add(FVector2D::ZeroVector);
        }
        for (int32 i = 0; i < Segs; ++i)
        {
            T.Add(0); T.Add(1 + i); T.Add(1 + ((i + 1) % Segs));
        }
        HoleMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
};

// ===================== Meteor Actor =====================
UCLASS()
class LAST_CIRCLE_2_API ALCMeteorActor : public AActor
{
    GENERATED_BODY()
public:
    ALCMeteorActor()
    {
        PrimaryActorTick.bCanEverTick = true;
        MeteorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeteorMesh"));
        RootComponent = MeteorMesh;
        MeteorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BuildMeteorMesh();

        TrailLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TrailLight"));
        TrailLight->SetupAttachment(RootComponent);
        TrailLight->SetIntensity(3000.f);
        TrailLight->SetLightColor(FLinearColor(1.f, 0.3f, 0.05f));
        TrailLight->SetAttenuationRadius(300.f);
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        // Fall toward target
        FVector Dir = (TargetLocation - GetActorLocation()).GetSafeNormal();
        AddActorWorldOffset(Dir * FallSpeed * DT);
        AddActorWorldRotation(FRotator(200.f * DT, 100.f * DT, 0.f));

        float Dist = FVector::Dist(GetActorLocation(), TargetLocation);
        if (Dist < 50.f)
        {
            Impact();
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetTargetLocation(const FVector& Loc) { TargetLocation = Loc; }
    UFUNCTION(BlueprintCallable)
    void SetFallSpeed(float Speed) { FallSpeed = Speed; }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* MeteorMesh = nullptr;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* TrailLight = nullptr;
    UPROPERTY() FVector TargetLocation = FVector::ZeroVector;
    UPROPERTY() float FallSpeed = 800.f;
    UPROPERTY() float DamageRadius = 200.f;
    UPROPERTY() float ImpactDamage = 150.f;

    void BuildMeteorMesh()
    {
        if (!MeteorMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        float Scale = FMath::FRandRange(5.f, 15.f);
        // Irregular rock shape
        for (int32 i = 0; i < 8; ++i)
        {
            float Phi = FMath::FRandRange(0.f, PI);
            float Theta = FMath::FRandRange(0.f, 2.f * PI);
            float R = Scale * FMath::FRandRange(0.5f, 1.f);
            V.Add(FVector(R*FMath::Sin(Phi)*FMath::Cos(Theta), R*FMath::Sin(Phi)*FMath::Sin(Theta), R*FMath::Cos(Phi)));
            C.Add(FColor(80 + FMath::RandRange(0,40), 20, 0));
            UV.Add(FVector2D::ZeroVector);
        }
        // Simple triangulation
        for (int32 i = 0; i < 6; ++i)
        {
            T.Add(0); T.Add(i+1); T.Add(i+2 < 8 ? i+2 : 1);
        }
        MeteorMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }

    void Impact()
    {
        // AOE damage
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageRadius);
        GetWorld()->SweepMultiByChannel(Hits, TargetLocation, TargetLocation, FQuat::Identity, ECC_Pawn, Sphere);
        for (const FHitResult& H : Hits)
        {
            if (AActor* A = H.GetActor())
            {
                float Dist = FVector::Dist(TargetLocation, A->GetActorLocation());
                float Dmg = ImpactDamage * (1.f - Dist / DamageRadius);
                FDamageEvent DmgEvent;
                A->TakeDamage(FMath::Max(0.f, Dmg), DmgEvent, nullptr, this);
            }
        }
        // Spawn explosion effect
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GetWorld()->SpawnActor<ALCExplosionEffect>(ALCExplosionEffect::StaticClass(), TargetLocation, FRotator::ZeroRotator, Params);
        Destroy();
    }
};

// ===================== Tornado Actor =====================
UCLASS()
class LAST_CIRCLE_2_API ALCTornadoActor : public AActor
{
    GENERATED_BODY()
public:
    ALCTornadoActor()
    {
        PrimaryActorTick.bCanEverTick = true;
        TornadoMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TornadoMesh"));
        RootComponent = TornadoMesh;
        TornadoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BuildTornadoMesh();
        SetLifeSpan(30.f);
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        Timer += DT;
        // Move in a direction
        AddActorWorldOffset(MoveDirection * MoveSpeed * DT);
        // Spin
        AddActorWorldRotation(FRotator(0.f, 200.f * DT, 0.f));

        // Push nearby pawns
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(PushRadius);
        GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere);
        for (const FHitResult& H : Hits)
        {
            if (APawn* P = Cast<APawn>(H.GetActor()))
            {
                FVector PushDir = (P->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                PushDir.Z = 0.5f;
                PushDir.Normalize();
                P->AddMovementInput(PushDir, PushForce * DT);
            }
        }
        // Rebuild mesh with wobble
        if (FMath::Fmod(Timer, 0.5f) < DT)
        {
            MoveDirection = FVector(FMath::FRandRange(-1.f,1.f), FMath::FRandRange(-1.f,1.f), 0.f).GetSafeNormal();
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetMoveDirection(const FVector& Dir) { MoveDirection = Dir.GetSafeNormal(); }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* TornadoMesh = nullptr;
    UPROPERTY() float Timer = 0.f;
    UPROPERTY() FVector MoveDirection = FVector(1.f, 0.f, 0.f);
    UPROPERTY() float MoveSpeed = 150.f;
    UPROPERTY() float PushRadius = 200.f;
    UPROPERTY() float PushForce = 2000.f;

    void BuildTornadoMesh()
    {
        if (!TornadoMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // Funnel shape (cone)
        float BottomR = 10.f, TopR = 60.f, Height = 200.f;
        int32 Segs = 12, Rings = 8;
        for (int32 ring = 0; ring < Rings; ++ring)
        {
            float Z = Height * ring / Rings;
            float R = FMath::Lerp(BottomR, TopR, (float)ring / Rings);
            for (int32 seg = 0; seg < Segs; ++seg)
            {
                float A = 2.f * PI * seg / Segs;
                V.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), Z));
                float Alpha = (float)ring / Rings;
                C.Add(FColor(100, 100, 100, FMath::Lerp(255, 80, Alpha)));
                UV.Add(FVector2D::ZeroVector);
            }
        }
        for (int32 ring = 0; ring < Rings - 1; ++ring)
        {
            for (int32 seg = 0; seg < Segs; ++seg)
            {
                int32 Curr = ring * Segs + seg;
                int32 Next = ring * Segs + (seg + 1) % Segs;
                int32 Above = (ring + 1) * Segs + seg;
                int32 AboveNext = (ring + 1) * Segs + (seg + 1) % Segs;
                T.Add(Curr); T.Add(Above); T.Add(Next);
                T.Add(Next); T.Add(Above); T.Add(AboveNext);
            }
        }
        TornadoMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
};
