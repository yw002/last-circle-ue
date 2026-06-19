#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "LCBridgeGenerator.generated.h"

// ===================== Bridge =====================
UCLASS()
class LAST_CIRCLE_2_API ALCBridge : public AActor
{
    GENERATED_BODY()
public:
    ALCBridge()
    {
        PrimaryActorTick.bCanEverTick = false;
        BridgeMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BridgeMesh"));
        RootComponent = BridgeMesh;
        BridgeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        BridgeMesh->SetCollisionProfileName(FName("BlockAll"));
    }

    UFUNCTION(BlueprintCallable)
    void BuildBridge(const FVector& Start, const FVector& End, float Width)
    {
        if (!BridgeMesh) return;
        BridgeMesh->ClearAllMeshSections();

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        FVector Dir = (End - Start).GetSafeNormal();
        FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal() * Width * 0.5f;
        float Length = FVector::Dist(Start, End);
        int32 Segments = FMath::Max(4, FMath::FloorToInt(Length / 30.f));

        // Deck planks
        for (int32 i = 0; i <= Segments; ++i)
        {
            float Alpha = (float)i / Segments;
            FVector Center = FMath::Lerp(Start, End, Alpha);
            // Slight arch
            Center.Z += FMath::Sin(Alpha * PI) * 10.f;

            V.Add(Center - Right);
            V.Add(Center + Right);
            C.Add(FColor(120, 80, 40)); C.Add(FColor(120, 80, 40));
            UV.Add(FVector2D::ZeroVector); UV.Add(FVector2D::ZeroVector);
        }
        for (int32 i = 0; i < Segments; ++i)
        {
            int32 BL = i * 2, BR = i * 2 + 1;
            int32 TL = (i + 1) * 2, TR = (i + 1) * 2 + 1;
            T.Add(BL); T.Add(TL); T.Add(BR);
            T.Add(BR); T.Add(TL); T.Add(TR);
        }

        // Railings
        for (int32 side = -1; side <= 1; side += 2)
        {
            float RailHeight = 8.f;
            for (int32 i = 0; i <= Segments; ++i)
            {
                float Alpha = (float)i / Segments;
                FVector Center = FMath::Lerp(Start, End, Alpha);
                Center.Z += FMath::Sin(Alpha * PI) * 10.f;
                FVector RailPos = Center + Right * side;

                // Post
                int32 PostBase = V.Num();
                V.Add(RailPos + FVector(-0.5f, -0.5f, 0)); V.Add(RailPos + FVector(0.5f, -0.5f, 0));
                V.Add(RailPos + FVector(0.5f, 0.5f, 0)); V.Add(RailPos + FVector(-0.5f, 0.5f, 0));
                V.Add(RailPos + FVector(-0.5f, -0.5f, RailHeight)); V.Add(RailPos + FVector(0.5f, -0.5f, RailHeight));
                V.Add(RailPos + FVector(0.5f, 0.5f, RailHeight)); V.Add(RailPos + FVector(-0.5f, 0.5f, RailHeight));
                for (int32 j = 0; j < 8; ++j) { C.Add(FColor(90, 60, 30)); UV.Add(FVector2D::ZeroVector); }
                T.Add(PostBase);T.Add(PostBase+1);T.Add(PostBase+5);T.Add(PostBase);T.Add(PostBase+5);T.Add(PostBase+4);
                T.Add(PostBase+2);T.Add(PostBase+3);T.Add(PostBase+7);T.Add(PostBase+2);T.Add(PostBase+7);T.Add(PostBase+6);
            }
        }

        // Support pillars underneath
        for (int32 i = 1; i < Segments; i += 2)
        {
            float Alpha = (float)i / Segments;
            FVector Center = FMath::Lerp(Start, End, Alpha);
            Center.Z += FMath::Sin(Alpha * PI) * 10.f;
            float PillarH = Center.Z + 5.f;

            int32 PB = V.Num();
            float PR = 2.f;
            V.Add(Center + FVector(-PR, -PR, -PillarH)); V.Add(Center + FVector(PR, -PR, -PillarH));
            V.Add(Center + FVector(PR, PR, -PillarH)); V.Add(Center + FVector(-PR, PR, -PillarH));
            V.Add(Center + FVector(-PR, -PR, 0)); V.Add(Center + FVector(PR, -PR, 0));
            V.Add(Center + FVector(PR, PR, 0)); V.Add(Center + FVector(-PR, PR, 0));
            for (int32 j = 0; j < 8; ++j) { C.Add(FColor(100, 100, 100)); UV.Add(FVector2D::ZeroVector); }
            T.Add(PB);T.Add(PB+1);T.Add(PB+5);T.Add(PB);T.Add(PB+5);T.Add(PB+4);
            T.Add(PB+2);T.Add(PB+3);T.Add(PB+7);T.Add(PB+2);T.Add(PB+7);T.Add(PB+6);
        }

        BridgeMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* BridgeMesh = nullptr;
};

// ===================== Bridge Generator =====================
UCLASS()
class LAST_CIRCLE_2_API ALCBridgeGenerator : public AActor
{
    GENERATED_BODY()
public:
    ALCBridgeGenerator() { PrimaryActorTick.bCanEverTick = false; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        GenerateBridges();
    }
protected:
    void GenerateBridges()
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Generate 8 bridges across rivers/gaps
        for (int32 i = 0; i < 8; ++i)
        {
            float Angle = 2.f * PI * i / 8.f;
            float R1 = 800.f + FMath::FRandRange(-200.f, 200.f);
            float R2 = R1 + FMath::FRandRange(100.f, 300.f);
            FVector Start(R1 * FMath::Cos(Angle), R1 * FMath::Sin(Angle), 5.f);
            FVector End(R2 * FMath::Cos(Angle), R2 * FMath::Sin(Angle), 5.f);

            ALCBridge* Bridge = GetWorld()->SpawnActor<ALCBridge>(ALCBridge::StaticClass(), Start, FRotator::ZeroRotator, Params);
            if (Bridge)
            {
                Bridge->BuildBridge(Start, End, 12.f);
            }
        }
    }
};
