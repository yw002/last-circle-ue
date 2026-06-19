#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "LCEnvironmentActor.generated.h"

UENUM(BlueprintType)
enum class EBiomeType : uint8
{
    Desert UMETA(DisplayName = "Desert"),
    Snow   UMETA(DisplayName = "Snow"),
    Jungle UMETA(DisplayName = "Jungle"),
    Swamp  UMETA(DisplayName = "Swamp"),
    Lava   UMETA(DisplayName = "Lava")
};

UCLASS()
class LAST_CIRCLE_2_API ALCEnvironmentActor : public AActor
{
    GENERATED_BODY()
public:
    static constexpr float MapHalfExtent = 30000.f;
    static constexpr float MapSize = MapHalfExtent * 2.f;
    static constexpr int32 BiomeGridSize = 300;
    static constexpr float BiomeCellSize = MapSize / BiomeGridSize;

    ALCEnvironmentActor()
    {
        PrimaryActorTick.bCanEverTick = false;
        TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
        RootComponent = TerrainMesh;
        TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        TerrainMesh->SetCollisionResponseToAllChannels(ECR_Block);
        TerrainMesh->SetCollisionObjectType(ECC_WorldStatic);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();

        VertexColorMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
        if (!VertexColorMaterial)
        {
            VertexColorMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial"));
        }

        GenerateBiomeLookup();
        GenerateTerrain();
        if (VertexColorMaterial)
        {
            for (int32 s = 0; s < TerrainMesh->GetNumSections(); ++s)
            {
                TerrainMesh->SetMaterial(s, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
            }
        }
        GenerateTrees();
        GenerateRocks();
        GenerateWater();
        BuildBoundaryWalls();
    }

    void BuildBoundaryWalls()
    {
        const float HalfMap = MapHalfExtent + 50.f;
        const float HalfThick = 100.f;
        const float WallHeight = 2000.f;

        auto MakeWall = [&](FVector Loc, FVector Extent) {
            UBoxComponent* Wall = NewObject<UBoxComponent>(this);
            Wall->RegisterComponent();
            Wall->SetWorldLocation(Loc);
            Wall->SetBoxExtent(Extent);
            Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Wall->SetCollisionResponseToAllChannels(ECR_Block);
            Wall->SetCollisionObjectType(ECC_WorldStatic);
            Wall->SetVisibility(false);
            Wall->SetHiddenInGame(true);
        };

        MakeWall(FVector( HalfMap, 0.f, WallHeight * 0.5f), FVector(HalfThick, HalfMap + HalfThick, WallHeight));
        MakeWall(FVector(-HalfMap, 0.f, WallHeight * 0.5f), FVector(HalfThick, HalfMap + HalfThick, WallHeight));
        MakeWall(FVector(0.f,  HalfMap, WallHeight * 0.5f), FVector(HalfMap + HalfThick, HalfThick, WallHeight));
        MakeWall(FVector(0.f, -HalfMap, WallHeight * 0.5f), FVector(HalfMap + HalfThick, HalfThick, WallHeight));
    }

    UFUNCTION(BlueprintCallable)
    EBiomeType GetBiomeAt(float X, float Y) const
    {
        int32 LX = FMath::Clamp(FMath::FloorToInt((X + MapHalfExtent) / BiomeCellSize), 0, BiomeGridSize - 1);
        int32 LY = FMath::Clamp(FMath::FloorToInt((Y + MapHalfExtent) / BiomeCellSize), 0, BiomeGridSize - 1);
        return BiomeLookup[LX][LY];
    }

    UFUNCTION(BlueprintCallable)
    float GetHeightAt(float X, float Y) const
    {
        float Dist = FMath::Sqrt(X * X + Y * Y);
        float Base = FMath::Max(10.f, 2000.f - Dist * 0.05f);

        auto GaussPeak = [](float X, float Y, float PX, float PY, float H) -> float {
            float D2 = (X - PX) * (X - PX) + (Y - PY) * (Y - PY);
            return H * FMath::Exp(-D2 / (2.f * 3000.f * 3000.f));
        };
        Base += GaussPeak(X, Y, 15000.f, 15000.f, 1200.f);
        Base += GaussPeak(X, Y, -18000.f, 5000.f, 1000.f);
        Base += GaussPeak(X, Y, 5000.f, -15000.f, 1100.f);

        Base += FMath::Sin(X * 0.001f) * FMath::Cos(Y * 0.001f) * 200.f;
        Base += FMath::Sin(X * 0.0005f + 1.f) * FMath::Cos(Y * 0.0007f) * 150.f;

        EBiomeType Biome = GetBiomeAt(X, Y);
        if (Biome == EBiomeType::Desert) Base *= 0.7f;
        if (Biome == EBiomeType::Swamp) Base = FMath::Max(15.f, FMath::Min(Base, 200.f));
        if (Biome == EBiomeType::Lava) Base += FMath::Abs(FMath::Sin(X * 0.005f) * FMath::Cos(Y * 0.005f)) * 100.f;

        return FMath::Max(10.f, Base);
    }

protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* TerrainMesh = nullptr;
    UPROPERTY() UMaterial* VertexColorMaterial = nullptr;
    EBiomeType BiomeLookup[300][300];
    FVector2D BiomeSeedPoints[5] = {
        FVector2D(-15000.f, -15000.f), FVector2D(15000.f, -15000.f),
        FVector2D(0.f, 0.f), FVector2D(-15000.f, 15000.f), FVector2D(15000.f, 15000.f)
    };

    FColor GetTerrainColor(float X, float Y, float Z, EBiomeType Biome) const
    {
        switch (Biome)
        {
        case EBiomeType::Desert:
        {
            float n = FMath::Sin(X * 0.0012f + Y * 0.0018f) * FMath::Cos(Y * 0.0009f);
            uint8 R = 190 + (int32)(n * 30.f);
            uint8 G = 160 + (int32)(n * 25.f);
            uint8 B = 110 + (int32)(n * 20.f);
            return FColor(R, G, B);
        }
        case EBiomeType::Snow:
        {
            float n = FMath::Sin(X * 0.0008f) * FMath::Cos(Y * 0.0008f);
            uint8 v = 230 + (int32)(n * 20.f);
            return FColor(v, v, v + 10);
        }
        case EBiomeType::Jungle:
        {
            float n1 = FMath::Sin(X * 0.0015f + Y * 0.0019f);
            float n2 = FMath::Cos(X * 0.0031f + 2.7f) * FMath::Sin(Y * 0.0027f + 1.3f);
            float n3 = FMath::Sin(X * 0.0007f - Y * 0.0011f) * FMath::Cos(Y * 0.0013f);

            float grassBlend = n1 * 25.f + n2 * 15.f;
            uint8 R = 30 + (int32)(n2 * 20.f + n3 * 10.f);
            uint8 G = 100 + (int32)(grassBlend);
            uint8 B = 15 + (int32)(n1 * 10.f + n3 * 8.f);

            float path = FMath::Abs(FMath::Sin(X * 0.0005f + Y * 0.0008f) + FMath::Cos(Y * 0.0004f - X * 0.0006f));
            if (path > 1.3f)
            {
                R = FMath::Min(255, R + 40);
                G = FMath::Min(255, G + 30);
                B = FMath::Min(255, B + 15);
            }

            if (Z < 200.f)
            {
                uint8 D = (uint8)FMath::Clamp((int32)((200.f - Z) * 0.2f), 0, 40);
                R = FMath::Max(0, (int32)R - D / 2);
                G = FMath::Max(0, (int32)G - D);
                B = FMath::Max(0, (int32)B - D);
            }
            else if (Z > 800.f)
            {
                uint8 L = (uint8)FMath::Clamp((int32)((Z - 800.f) * 0.05f), 0, 30);
                R = FMath::Min(255, R + L);
                G = FMath::Min(255, G + L);
                B = FMath::Min(255, B + L / 2);
            }

            float dirt = FMath::Abs(FMath::Sin(X * 0.0022f + 3.f) * FMath::Cos(Y * 0.0025f - 1.f));
            if (dirt > 0.65f)
            {
                R = 120 + (int32)(dirt * 50.f);
                G = 80 + (int32)(dirt * 30.f);
                B = 40;
            }

            return FColor(
                FMath::Clamp(R, 0, 255),
                FMath::Clamp(G, 0, 255),
                FMath::Clamp(B, 0, 255)
            );
        }
        case EBiomeType::Swamp:
        {
            float n = FMath::Sin(X * 0.001f) * FMath::Cos(Y * 0.001f);
            return FColor(70 + (int32)(n * 20.f), 90 + (int32)(n * 15.f), 30 + (int32)(n * 10.f));
        }
        case EBiomeType::Lava:
        {
            float n = FMath::Abs(FMath::Sin(X * 0.004f) * FMath::Cos(Y * 0.004f));
            return FColor(140 + (int32)(n * 80.f), 40 + (int32)(n * 50.f), 10);
        }
        default: return FColor(100, 100, 100);
        }
    }

    void GenerateBiomeLookup()
    {
        for (int32 i = 0; i < BiomeGridSize; ++i)
        {
            for (int32 j = 0; j < BiomeGridSize; ++j)
            {
                float WX = -MapHalfExtent + i * BiomeCellSize;
                float WY = -MapHalfExtent + j * BiomeCellSize;
                float MinDist = MAX_FLT; int32 Closest = 0;
                for (int32 k = 0; k < 5; ++k)
                {
                    float D = FVector2D::Distance(FVector2D(WX, WY), BiomeSeedPoints[k]);
                    D += FMath::Sin(WX * 0.0005f + k) * FMath::Cos(WY * 0.0005f + k) * 2000.f;
                    if (D < MinDist) { MinDist = D; Closest = k; }
                }
                BiomeLookup[i][j] = static_cast<EBiomeType>(Closest);
            }
        }
    }

    void GenerateTerrain()
    {
        if (!TerrainMesh) return;
        const int32 GridRes = 60;
        const float CellSize = MapSize / GridRes;

        TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
        TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

        for (int32 i = 0; i <= GridRes; ++i)
        {
            for (int32 j = 0; j <= GridRes; ++j)
            {
                float X = -MapHalfExtent + i * CellSize;
                float Y = -MapHalfExtent + j * CellSize;
                float Z = GetHeightAt(X, Y);
                Verts.Add(FVector(X, Y, Z));
                EBiomeType B = GetBiomeAt(X, Y);
                FColor Col = GetTerrainColor(X, Y, Z, B);
                Colors.Add(Col);
                UVs.Add(FVector2D((float)i / GridRes, (float)j / GridRes));
            }
        }

        for (int32 i = 0; i < GridRes; ++i)
        {
            for (int32 j = 0; j < GridRes; ++j)
            {
                int32 TL = i * (GridRes + 1) + j;
                int32 TR = TL + 1;
                int32 BL = (i + 1) * (GridRes + 1) + j;
                int32 BR = BL + 1;
                Tris.Add(TL); Tris.Add(BL); Tris.Add(TR);
                Tris.Add(TR); Tris.Add(BL); Tris.Add(BR);
            }
        }

        const int32 NumVerts = Verts.Num();
        Normals.SetNum(NumVerts);
        for (int32 k = 0; k < NumVerts; ++k) Normals[k] = FVector::ZeroVector;
        for (int32 t = 0; t < Tris.Num(); t += 3)
        {
            int32 I0 = Tris[t], I1 = Tris[t+1], I2 = Tris[t+2];
            FVector A = Verts[I1] - Verts[I0];
            FVector B = Verts[I2] - Verts[I0];
            FVector FN = FVector::CrossProduct(A, B).GetSafeNormal();
            Normals[I0] += FN; Normals[I1] += FN; Normals[I2] += FN;
        }
        for (int32 k = 0; k < NumVerts; ++k) Normals[k].Normalize();

        TerrainMesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, true);
        TerrainMesh->SetMeshSectionVisible(0, true);
    }

    void GenerateTrees()
    {
        const int32 GridSize = 16;
        const float CellSize = MapSize / GridSize;
        for (int32 i = 0; i < GridSize; ++i)
        {
            for (int32 j = 0; j < GridSize; ++j)
            {
                float X = -MapHalfExtent + i * CellSize + FMath::RandRange(0.f, CellSize);
                float Y = -MapHalfExtent + j * CellSize + FMath::RandRange(0.f, CellSize);
                float MathZ = GetHeightAt(X, Y);
                if (MathZ < 10.f) continue;

                // Use line trace to find exact terrain surface (avoids tree floating on slopes)
                FVector TraceStart(X, Y, MathZ + 1000.f);
                FVector TraceEnd(X, Y, MathZ - 500.f);
                FHitResult Hit;
                FCollisionQueryParams Params;
                if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
                {
                    MathZ = Hit.Location.Z;
                }

                UProceduralMeshComponent* Tree = NewObject<UProceduralMeshComponent>(this);
                Tree->RegisterComponent();
                Tree->SetWorldLocation(FVector(X, Y, MathZ));
                Tree->SetCollisionEnabled(ECollisionEnabled::NoCollision);

                TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
                TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

                float TH = 800.f + FMath::RandRange(0.f, 400.f);
                float TR = 80.f;
                int32 Segs = 6;
                for (int32 s = 0; s < Segs; ++s)
                {
                    float A1 = 2.f * PI * s / Segs;
                    float A2 = 2.f * PI * ((s + 1) % Segs) / Segs;
                    int32 Base = V.Num();
                    V.Add(FVector(TR * FMath::Cos(A1), TR * FMath::Sin(A1), 0.f));
                    V.Add(FVector(TR * FMath::Cos(A1), TR * FMath::Sin(A1), TH));
                    V.Add(FVector(TR * FMath::Cos(A2), TR * FMath::Sin(A2), 0.f));
                    V.Add(FVector(TR * FMath::Cos(A2), TR * FMath::Sin(A2), TH));
                    for (int32 k = 0; k < 4; ++k) { C.Add(FColor(80, 50, 20)); UV.Add(FVector2D::ZeroVector); }
                    T.Add(Base); T.Add(Base+1); T.Add(Base+2);
                    T.Add(Base+2); T.Add(Base+1); T.Add(Base+3);
                }

                float CR = 400.f + FMath::RandRange(0.f, 200.f);
                FVector CrownCenter(0.f, 0.f, TH + CR * 0.5f);
                EBiomeType Biome = GetBiomeAt(X, Y);
                FColor CrownColor = FColor(0, 120, 0);
                if (Biome == EBiomeType::Snow) CrownColor = FColor(20, 60, 20);
                if (Biome == EBiomeType::Desert) CrownColor = FColor(100, 80, 20);

                int32 CrownBase = V.Num();
                const int32 CrownRings = 8;
                const int32 CrownSegs = 10;
                for (int32 ring = 0; ring < CrownRings; ++ring)
                {
                    float Phi = PI * ring / (CrownRings - 1);
                    for (int32 seg = 0; seg < CrownSegs; ++seg)
                    {
                        float Theta = 2.f * PI * seg / CrownSegs;
                        V.Add(CrownCenter + FVector(CR * FMath::Sin(Phi) * FMath::Cos(Theta),
                                                     CR * FMath::Sin(Phi) * FMath::Sin(Theta),
                                                     CR * FMath::Cos(Phi)));
                        C.Add(CrownColor); UV.Add(FVector2D::ZeroVector);
                    }
                }
                for (int32 ring = 0; ring < CrownRings - 1; ++ring)
                {
                    for (int32 seg = 0; seg < CrownSegs; ++seg)
                    {
                        int32 Curr = CrownBase + ring * CrownSegs + seg;
                        int32 Next = CrownBase + ring * CrownSegs + (seg + 1) % CrownSegs;
                        int32 Below = CrownBase + (ring + 1) * CrownSegs + seg;
                        int32 BelowNext = CrownBase + (ring + 1) * CrownSegs + (seg + 1) % CrownSegs;
                        T.Add(Curr); T.Add(Below); T.Add(Next);
                        T.Add(Next); T.Add(Below); T.Add(BelowNext);
                    }
                }

                Tree->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
                if (VertexColorMaterial) Tree->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
            }
        }
    }

    void GenerateRocks()
    {
        for (int32 i = 0; i < 60; ++i)
        {
            float X = FMath::RandRange(-MapHalfExtent * 0.93f, MapHalfExtent * 0.93f);
            float Y = FMath::RandRange(-MapHalfExtent * 0.93f, MapHalfExtent * 0.93f);
            float MathZ = GetHeightAt(X, Y);

            FVector TraceStart(X, Y, MathZ + 500.f);
            FVector TraceEnd(X, Y, MathZ - 200.f);
            FHitResult RockHit;
            FCollisionQueryParams RockParams;
            if (GetWorld()->LineTraceSingleByChannel(RockHit, TraceStart, TraceEnd, ECC_WorldStatic, RockParams))
            {
                MathZ = RockHit.Location.Z;
            }

            UProceduralMeshComponent* Rock = NewObject<UProceduralMeshComponent>(this);
            Rock->RegisterComponent();
            Rock->SetWorldLocation(FVector(X, Y, MathZ));
            Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

            float Scale = FMath::RandRange(150.f, 500.f);
            V.Add(FVector(0, 0, Scale));
            V.Add(FVector(0, 0, -Scale * 0.3f));
            for (int32 j = 0; j < 6; ++j)
            {
                float A = 2.f * PI * j / 6;
                float R = Scale * (0.6f + FMath::FRandRange(-0.2f, 0.2f));
                V.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), Scale * 0.3f));
                C.Add(FColor(120, 120, 110)); UV.Add(FVector2D::ZeroVector);
            }
            C.Add(FColor(120, 120, 110)); UV.Add(FVector2D::ZeroVector);
            C.Add(FColor(120, 120, 110)); UV.Add(FVector2D::ZeroVector);

            for (int32 j = 0; j < 6; ++j)
            {
                T.Add(0); T.Add(2 + j); T.Add(2 + ((j + 1) % 6));
                T.Add(1); T.Add(2 + ((j + 1) % 6)); T.Add(2 + j);
            }

            Rock->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
            if (VertexColorMaterial) Rock->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
        }
    }

    void GenerateWater()
    {
        UProceduralMeshComponent* Water = NewObject<UProceduralMeshComponent>(this, TEXT("WaterPlane"));
        Water->RegisterComponent();
        Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        float S = MapHalfExtent + 1000.f;
        float WZ = 250.f;
        V.Add(FVector(-S, -S, WZ)); V.Add(FVector(S, -S, WZ));
        V.Add(FVector(S, S, WZ)); V.Add(FVector(-S, S, WZ));
        for (int32 i = 0; i < 4; ++i) { C.Add(FColor(20, 80, 180, 150)); UV.Add(FVector2D::ZeroVector); }
        T.Add(0); T.Add(1); T.Add(2); T.Add(0); T.Add(2); T.Add(3);

        Water->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
        if (VertexColorMaterial) Water->SetMaterial(0, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
    }
};
