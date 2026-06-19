#pragma once
#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "LCCaveGenerator.generated.h"

// ===================== Single Cave =====================
UCLASS()
class LAST_CIRCLE_2_API ALCCave : public AActor
{
    GENERATED_BODY()
public:
    ALCCave()
    {
        PrimaryActorTick.bCanEverTick = false;
        CaveMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CaveMesh"));
        RootComponent = CaveMesh;
        CaveMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        CaveMesh->SetCollisionProfileName(FName("BlockAll"));

        // Interior light
        CaveLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CaveLight"));
        CaveLight->SetupAttachment(RootComponent);
        CaveLight->SetIntensity(200.f);
        CaveLight->SetLightColor(FLinearColor(0.8f, 0.6f, 0.3f));
        CaveLight->SetAttenuationRadius(150.f);
    }

    UFUNCTION(BlueprintCallable)
    void BuildCave(float CaveRadius, float CaveDepth, int32 Segments)
    {
        if (!CaveMesh) return;
        CaveMesh->ClearAllMeshSections();

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        // Tunnel entrance (half-cylinder)
        for (int32 i = 0; i <= Segments; ++i)
        {
            float Angle = PI * i / Segments;
            float R = CaveRadius * (0.8f + FMath::FRandRange(-0.15f, 0.15f));

            // Outer ring (entrance)
            V.Add(FVector(0.f, R * FMath::Cos(Angle), R * FMath::Sin(Angle)));
            C.Add(FColor(60, 55, 50)); UV.Add(FVector2D::ZeroVector);

            // Inner ring (deep inside)
            float InnerR = R * 0.9f;
            V.Add(FVector(-CaveDepth, InnerR * FMath::Cos(Angle), InnerR * FMath::Sin(Angle)));
            C.Add(FColor(40, 35, 30)); UV.Add(FVector2D::ZeroVector);
        }

        for (int32 i = 0; i < Segments; ++i)
        {
            int32 O1 = i * 2, I1 = i * 2 + 1;
            int32 O2 = (i + 1) * 2, I2 = (i + 1) * 2 + 1;
            T.Add(O1); T.Add(O2); T.Add(I1);
            T.Add(I1); T.Add(O2); T.Add(I2);
        }

        // Back wall (cap)
        int32 CapCenter = V.Num();
        V.Add(FVector(-CaveDepth, 0.f, CaveRadius * 0.5f));
        C.Add(FColor(30, 25, 20)); UV.Add(FVector2D::ZeroVector);
        for (int32 i = 0; i < Segments; ++i)
        {
            T.Add(CapCenter); T.Add(i * 2 + 1); T.Add(((i + 1) % Segments) * 2 + 1);
        }

        // Stalactites (hanging from ceiling)
        for (int32 s = 0; s < 5; ++s)
        {
            float DepthOffset = FMath::FRandRange(-CaveDepth * 0.8f, 0.f);
            float SideOffset = FMath::FRandRange(-CaveRadius * 0.5f, CaveRadius * 0.5f);
            float StalH = FMath::FRandRange(5.f, 15.f);
            float StalR = FMath::FRandRange(1.f, 3.f);

            int32 SB = V.Num();
            V.Add(FVector(DepthOffset, SideOffset - StalR, CaveRadius * 0.8f));
            V.Add(FVector(DepthOffset, SideOffset + StalR, CaveRadius * 0.8f));
            V.Add(FVector(DepthOffset, SideOffset, CaveRadius * 0.8f - StalH));
            FColor StalCol(80, 75, 65);
            for (int32 j = 0; j < 3; ++j) { C.Add(StalCol); UV.Add(FVector2D::ZeroVector); }
            T.Add(SB); T.Add(SB+1); T.Add(SB+2);
        }

        // Ore deposits (glowing spots on walls)
        for (int32 o = 0; o < 3; ++o)
        {
            float DepthOffset = FMath::FRandRange(-CaveDepth * 0.7f, -5.f);
            float Angle = FMath::FRandRange(0.3f, PI - 0.3f);
            float R = CaveRadius * 0.95f;
            FVector OrePos(DepthOffset, R * FMath::Cos(Angle), R * FMath::Sin(Angle));

            int32 OB = V.Num();
            float OreR = 2.f;
            V.Add(OrePos + FVector(-OreR, -OreR, 0)); V.Add(OrePos + FVector(OreR, -OreR, 0));
            V.Add(OrePos + FVector(OreR, OreR, 0)); V.Add(OrePos + FVector(-OreR, OreR, 0));
            FColor OreCol(100, 200, 255); // Blue-ish ore glow
            for (int32 j = 0; j < 4; ++j) { C.Add(OreCol); UV.Add(FVector2D::ZeroVector); }
            T.Add(OB); T.Add(OB+1); T.Add(OB+2); T.Add(OB); T.Add(OB+2); T.Add(OB+3);
        }

        CaveMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);

        // Position cave light inside
        if (CaveLight)
        {
            CaveLight->SetRelativeLocation(FVector(-CaveDepth * 0.4f, 0.f, CaveRadius * 0.5f));
        }
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* CaveMesh = nullptr;
    UPROPERTY(VisibleAnywhere) UPointLightComponent* CaveLight = nullptr;
};

// ===================== Cave Generator (spawns 3 mine caves) =====================
UCLASS()
class LAST_CIRCLE_2_API ALCCaveGenerator : public AActor
{
    GENERATED_BODY()
public:
    ALCCaveGenerator() { PrimaryActorTick.bCanEverTick = false; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        GenerateCaves();
    }
protected:
    void GenerateCaves()
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // 3 mine caves at different locations
        struct FCaveConfig
        {
            FVector Location;
            float Radius;
            float Depth;
            FRotator Rotation;
        };

        TArray<FCaveConfig> Configs;
        Configs.Add({FVector(-2000.f, -1500.f, 10.f), 25.f, 80.f, FRotator(0.f, 45.f, 0.f)});
        Configs.Add({FVector(1800.f, -2000.f, 15.f), 30.f, 100.f, FRotator(0.f, -30.f, 0.f)});
        Configs.Add({FVector(500.f, 2200.f, 5.f), 20.f, 60.f, FRotator(0.f, 120.f, 0.f)});

        for (const FCaveConfig& Config : Configs)
        {
            ALCCave* Cave = GetWorld()->SpawnActor<ALCCave>(ALCCave::StaticClass(), Config.Location, Config.Rotation, Params);
            if (Cave)
            {
                Cave->BuildCave(Config.Radius, Config.Depth, 12);
            }
        }
    }
};
