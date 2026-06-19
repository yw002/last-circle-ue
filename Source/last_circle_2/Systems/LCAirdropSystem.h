#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/LCPickupSystem.h"
#include "LCAirdropSystem.generated.h"

// ===================== Airdrop Crate =====================
UCLASS()
class LAST_CIRCLE_2_API ALCAirdropCrate : public AActor
{
    GENERATED_BODY()
public:
    ALCAirdropCrate()
    {
        PrimaryActorTick.bCanEverTick = true;
        Tags.Add(FName("Pickup"));
        CrateMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CrateMesh"));
        RootComponent = CrateMesh;
        CrateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        CrateMesh->SetCollisionProfileName(FName("BlockAll"));
        BuildCrateMesh();

        // Parachute for crate
        ParachuteMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ParachuteMesh"));
        ParachuteMesh->SetupAttachment(RootComponent);
        ParachuteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BuildParachuteMesh();

        // Smoke beacon
        BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
        BeaconLight->SetupAttachment(RootComponent);
        BeaconLight->SetIntensity(1000.f);
        BeaconLight->SetLightColor(FLinearColor(1.f, 0.2f, 0.2f));
        BeaconLight->SetAttenuationRadius(300.f);
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        if (bIsFalling)
        {
            FallSpeed = FMath::Min(FallSpeed + 20.f * DT, 200.f);
            AddActorWorldOffset(FVector(0.f, 0.f, -FallSpeed * DT));

            // Check if landed
            FVector Loc = GetActorLocation();
            if (Loc.Z <= TargetZ)
            {
                bIsFalling = false;
                SetActorLocation(FVector(Loc.X, Loc.Y, TargetZ));
                ParachuteMesh->SetVisibility(false);
                bLanded = true;
            }
        }
        // Blink beacon when landed
        if (bLanded)
        {
            BeaconTimer += DT;
            if (BeaconLight)
            {
                float Intensity = 500.f + 500.f * FMath::Sin(BeaconTimer * 4.f);
                BeaconLight->SetIntensity(Intensity);
            }
        }
    }

    UFUNCTION(BlueprintCallable) void SetTargetZ(float Z) { TargetZ = Z; }
    UFUNCTION(BlueprintCallable) bool HasLanded() const { return bLanded; }
    UFUNCTION(BlueprintCallable)
    TArray<FName> GetContents() const { return WeaponContents; }

protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* CrateMesh = nullptr;
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* ParachuteMesh = nullptr;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* BeaconLight = nullptr;
    UPROPERTY() bool bIsFalling = true;
    UPROPERTY() bool bLanded = false;
    UPROPERTY() float FallSpeed = 0.f;
    UPROPERTY() float TargetZ = 0.f;
    UPROPERTY() float BeaconTimer = 0.f;
    UPROPERTY() TArray<FName> WeaponContents;

    void BuildCrateMesh()
    {
        if (!CrateMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        float S = 15.f;
        V.Add(FVector(-S,-S,0)); V.Add(FVector(S,-S,0)); V.Add(FVector(S,S,0)); V.Add(FVector(-S,S,0));
        V.Add(FVector(-S,-S,S*2)); V.Add(FVector(S,-S,S*2)); V.Add(FVector(S,S,S*2)); V.Add(FVector(-S,S,S*2));
        FColor CrateCol(180, 50, 20);
        for (int32 i = 0; i < 8; ++i) { C.Add(CrateCol); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);
        CrateMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }

    void BuildParachuteMesh()
    {
        if (!ParachuteMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        float R = 40.f, H = 30.f;
        int32 Segs = 12;
        // Dome shape
        for (int32 ring = 0; ring < 4; ++ring)
        {
            float Phi = PI * 0.5f * ring / 4;
            for (int32 seg = 0; seg < Segs; ++seg)
            {
                float Theta = 2.f * PI * seg / Segs;
                float Offset_Z = 40.f; // Above crate
                V.Add(FVector(R * FMath::Sin(Phi) * FMath::Cos(Theta),
                              R * FMath::Sin(Phi) * FMath::Sin(Theta),
                              H * FMath::Cos(Phi) + Offset_Z));
                bool bWhite = (seg % 2 == 0);
                C.Add(bWhite ? FColor(240,240,240,200) : FColor(200,50,50,200));
                UV.Add(FVector2D::ZeroVector);
            }
        }
        for (int32 ring = 0; ring < 3; ++ring)
        {
            for (int32 seg = 0; seg < Segs; ++seg)
            {
                int32 Curr = ring * Segs + seg;
                int32 Next = ring * Segs + (seg + 1) % Segs;
                int32 Below = (ring + 1) * Segs + seg;
                int32 BelowNext = (ring + 1) * Segs + (seg + 1) % Segs;
                T.Add(Curr); T.Add(Below); T.Add(Next);
                T.Add(Next); T.Add(Below); T.Add(BelowNext);
            }
        }
        ParachuteMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
};

// ===================== Airdrop Manager =====================
UCLASS()
class LAST_CIRCLE_2_API ALCAirdropManager : public AActor
{
    GENERATED_BODY()
public:
    ALCAirdropManager() { PrimaryActorTick.bCanEverTick = true; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        DropTimer = FMath::RandRange(60.f, 120.f);
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        DropTimer -= DT;
        if (DropTimer <= 0.f)
        {
            SpawnAirdrop();
            DropTimer = FMath::RandRange(90.f, 180.f);
        }
    }

    UFUNCTION(BlueprintCallable)
    void ForceAirdrop() { SpawnAirdrop(); }
    UFUNCTION(BlueprintCallable)
    float GetTimeToNextDrop() const { return DropTimer; }
protected:
    UPROPERTY() float DropTimer = 90.f;
    UPROPERTY() int32 TotalDrops = 0;

    void SpawnAirdrop()
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;

        FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
        FVector DropLoc = PlayerLoc + FVector(FMath::RandRange(-500.f, 500.f), FMath::RandRange(-500.f, 500.f), 0.f);
        DropLoc.X = FMath::Clamp(DropLoc.X, -2800.f, 2800.f);
        DropLoc.Y = FMath::Clamp(DropLoc.Y, -2800.f, 2800.f);

        // Spawn at high altitude
        FVector SpawnPos = DropLoc + FVector(0.f, 0.f, 1000.f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ALCAirdropCrate* Crate = GetWorld()->SpawnActor<ALCAirdropCrate>(ALCAirdropCrate::StaticClass(), SpawnPos, FRotator::ZeroRotator, Params);
        if (Crate)
        {
            Crate->SetTargetZ(0.f); // Will be adjusted by terrain height
            TotalDrops++;
        }
    }
};
