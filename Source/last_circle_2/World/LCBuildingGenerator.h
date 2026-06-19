#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "LCBuildingGenerator.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCBuildingGenerator : public AActor
{
    GENERATED_BODY()
public:
    ALCBuildingGenerator() { PrimaryActorTick.bCanEverTick = false; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        GenerateHouses();
        GenerateMilitaryBases();
        GenerateRuins();
        GenerateLighthouses();
        GenerateFarms();
    }

protected:
    UPROPERTY() TArray<FVector> HouseLocations;

    void GenerateHouses()
    {
        for (int32 i = 0; i < 200; ++i)
        {
            float X = FMath::RandRange(-2800.f, 2800.f);
            float Y = FMath::RandRange(-2800.f, 2800.f);
            FVector Loc(X, Y, 0.f);
            HouseLocations.Add(Loc);
            BuildHouse(Loc);
        }
    }

    void BuildHouse(const FVector& Location)
    {
        UProceduralMeshComponent* House = NewObject<UProceduralMeshComponent>(this);
        House->RegisterComponent();
        House->SetWorldLocation(Location);
        House->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        House->SetCollisionProfileName(FName("BlockAll"));

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        float W = 15.f, H = 12.f, D = 15.f;
        // Floor
        V.Add(FVector(-W,-D,0)); V.Add(FVector(W,-D,0)); V.Add(FVector(W,D,0)); V.Add(FVector(-W,D,0));
        // Walls
        for (int32 i = 0; i < 4; ++i)
        {
            int32 Base = V.Num();
            // Front wall
            V.Add(FVector(-W,-D,i*H)); V.Add(FVector(W,-D,i*H));
            V.Add(FVector(W,-D,(i+1)*H)); V.Add(FVector(-W,-D,(i+1)*H));
            for (int32 j = 0; j < 4; ++j) { C.Add(FColor(200, 180, 150)); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base); T.Add(Base+1); T.Add(Base+2); T.Add(Base); T.Add(Base+2); T.Add(Base+3);
        }

        // Simple box house (4 walls + roof)
        // 4 wall quads
        auto AddWall = [&](const FVector& A, const FVector& B, const FVector& C2, const FVector& D2, FColor Col)
        {
            int32 Base = V.Num();
            V.Add(A); V.Add(B); V.Add(C2); V.Add(D2);
            for (int32 j = 0; j < 4; ++j) { C.Add(Col); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base); T.Add(Base+1); T.Add(Base+2); T.Add(Base); T.Add(Base+2); T.Add(Base+3);
        };

        // Front, Back, Left, Right walls
        AddWall(FVector(-W,-D,0), FVector(W,-D,0), FVector(W,-D,H), FVector(-W,-D,H), FColor(200,180,150));
        AddWall(FVector(W,D,0), FVector(-W,D,0), FVector(-W,D,H), FVector(W,D,H), FColor(200,180,150));
        AddWall(FVector(-W,D,0), FVector(-W,-D,0), FVector(-W,-D,H), FVector(-W,D,H), FColor(190,170,140));
        AddWall(FVector(W,-D,0), FVector(W,D,0), FVector(W,D,H), FVector(W,-D,H), FColor(190,170,140));

        // Roof (pyramid)
        int32 RBase = V.Num();
        V.Add(FVector(-W,-D,H)); V.Add(FVector(W,-D,H)); V.Add(FVector(W,D,H)); V.Add(FVector(-W,D,H));
        V.Add(FVector(0,0,H+8.f));
        for (int32 j = 0; j < 5; ++j) { C.Add(FColor(150, 30, 30)); UV.Add(FVector2D::ZeroVector); }
        T.Add(RBase); T.Add(RBase+1); T.Add(RBase+4);
        T.Add(RBase+1); T.Add(RBase+2); T.Add(RBase+4);
        T.Add(RBase+2); T.Add(RBase+3); T.Add(RBase+4);
        T.Add(RBase+3); T.Add(RBase); T.Add(RBase+4);

        House->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }

    void GenerateMilitaryBases()
    {
        for (int32 i = 0; i < 3; ++i)
        {
            FVector Loc(FMath::RandRange(-2000.f, 2000.f), FMath::RandRange(-2000.f, 2000.f), 0.f);
            UProceduralMeshComponent* Base = NewObject<UProceduralMeshComponent>(this);
            Base->RegisterComponent();
            Base->SetWorldLocation(Loc);
            Base->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

            // Containers (3-5)
            int32 NumContainers = FMath::RandRange(3, 5);
            for (int32 c = 0; c < NumContainers; ++c)
            {
                float OX = c * 25.f;
                float HX = 10.f, HY = 4.f, HZ = 4.f;
                int32 B = V.Num();
                V.Add(FVector(OX-HX,-HY,0)); V.Add(FVector(OX+HX,-HY,0));
                V.Add(FVector(OX+HX,HY,0)); V.Add(FVector(OX-HX,HY,0));
                V.Add(FVector(OX-HX,-HY,HZ*2)); V.Add(FVector(OX+HX,-HY,HZ*2));
                V.Add(FVector(OX+HX,HY,HZ*2)); V.Add(FVector(OX-HX,HY,HZ*2));
                for (int32 j = 0; j < 8; ++j) { C.Add(FColor(50,80,50)); UV.Add(FVector2D::ZeroVector); }
                T.Add(B);T.Add(B+1);T.Add(B+5);T.Add(B);T.Add(B+5);T.Add(B+4);
                T.Add(B+2);T.Add(B+3);T.Add(B+7);T.Add(B+2);T.Add(B+7);T.Add(B+6);
                T.Add(B+4);T.Add(B+5);T.Add(B+6);T.Add(B+4);T.Add(B+6);T.Add(B+7);
                T.Add(B);T.Add(B+3);T.Add(B+2);T.Add(B);T.Add(B+2);T.Add(B+1);
            }
            Base->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        }
    }

    void GenerateRuins()
    {
        for (int32 i = 0; i < 5; ++i)
        {
            FVector Loc(FMath::RandRange(-2000.f, 2000.f), FMath::RandRange(-2000.f, 2000.f), 0.f);
            UProceduralMeshComponent* Ruin = NewObject<UProceduralMeshComponent>(this);
            Ruin->RegisterComponent();
            Ruin->SetWorldLocation(Loc);
            Ruin->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

            // Stone pillars (4-9)
            int32 NumPillars = FMath::RandRange(4, 9);
            float CircleR = 30.f;
            for (int32 p = 0; p < NumPillars; ++p)
            {
                float A = 2.f * PI * p / NumPillars;
                float PX = CircleR * FMath::Cos(A);
                float PY = CircleR * FMath::Sin(A);
                float PH = FMath::RandRange(10.f, 20.f);
                int32 B = V.Num();
                int32 Segs = 6;
                float PR = 2.f;
                for (int32 s = 0; s < Segs; ++s)
                {
                    float A1 = 2.f * PI * s / Segs;
                    float A2 = 2.f * PI * ((s+1)%Segs) / Segs;
                    int32 Base2 = V.Num();
                    V.Add(FVector(PX+PR*FMath::Cos(A1), PY+PR*FMath::Sin(A1), 0));
                    V.Add(FVector(PX+PR*FMath::Cos(A1), PY+PR*FMath::Sin(A1), PH));
                    V.Add(FVector(PX+PR*FMath::Cos(A2), PY+PR*FMath::Sin(A2), 0));
                    V.Add(FVector(PX+PR*FMath::Cos(A2), PY+PR*FMath::Sin(A2), PH));
                    for (int32 j = 0; j < 4; ++j) { C.Add(FColor(150,140,130)); UV.Add(FVector2D::ZeroVector); }
                    T.Add(Base2);T.Add(Base2+1);T.Add(Base2+2);T.Add(Base2+2);T.Add(Base2+1);T.Add(Base2+3);
                }
            }
            Ruin->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        }
    }

    void GenerateLighthouses()
    {
        for (int32 i = 0; i < 2; ++i)
        {
            FVector Loc(FMath::RandRange(-2500.f, 2500.f), FMath::RandRange(-2500.f, 2500.f), 0.f);
            UProceduralMeshComponent* LH = NewObject<UProceduralMeshComponent>(this);
            LH->RegisterComponent();
            LH->SetWorldLocation(Loc);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

            float LH_R = 6.f, LH_H = 60.f;
            int32 Segs = 12;
            for (int32 s = 0; s < Segs; ++s)
            {
                float A1 = 2.f * PI * s / Segs;
                float A2 = 2.f * PI * ((s+1)%Segs) / Segs;
                int32 Base = V.Num();
                V.Add(FVector(LH_R*FMath::Cos(A1), LH_R*FMath::Sin(A1), 0));
                V.Add(FVector(LH_R*FMath::Cos(A1), LH_R*FMath::Sin(A1), LH_H));
                V.Add(FVector(LH_R*FMath::Cos(A2), LH_R*FMath::Sin(A2), 0));
                V.Add(FVector(LH_R*FMath::Cos(A2), LH_R*FMath::Sin(A2), LH_H));
                bool bIsRed = (s % 2 == 0);
                FColor Col = bIsRed ? FColor(180, 30, 30) : FColor(230, 230, 230);
                for (int32 j = 0; j < 4; ++j) { C.Add(Col); UV.Add(FVector2D::ZeroVector); }
                T.Add(Base);T.Add(Base+1);T.Add(Base+2);T.Add(Base+2);T.Add(Base+1);T.Add(Base+3);
            }
            LH->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        }
    }

    void GenerateFarms()
    {
        for (int32 i = 0; i < 5; ++i)
        {
            FVector Loc(FMath::RandRange(-2000.f, 2000.f), FMath::RandRange(-2000.f, 2000.f), 0.f);
            UProceduralMeshComponent* Farm = NewObject<UProceduralMeshComponent>(this);
            Farm->RegisterComponent();
            Farm->SetWorldLocation(Loc);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

            // Barn (red box)
            float BW = 20.f, BH = 15.f, BD = 12.f;
            int32 B = V.Num();
            V.Add(FVector(-BW,-BD,0)); V.Add(FVector(BW,-BD,0)); V.Add(FVector(BW,BD,0)); V.Add(FVector(-BW,BD,0));
            V.Add(FVector(-BW,-BD,BH)); V.Add(FVector(BW,-BD,BH)); V.Add(FVector(BW,BD,BH)); V.Add(FVector(-BW,BD,BH));
            for (int32 j = 0; j < 8; ++j) { C.Add(FColor(150, 30, 20)); UV.Add(FVector2D::ZeroVector); }
            T.Add(B);T.Add(B+1);T.Add(B+5);T.Add(B);T.Add(B+5);T.Add(B+4);
            T.Add(B+2);T.Add(B+3);T.Add(B+7);T.Add(B+2);T.Add(B+7);T.Add(B+6);
            T.Add(B+4);T.Add(B+5);T.Add(B+6);T.Add(B+4);T.Add(B+6);T.Add(B+7);
            T.Add(B);T.Add(B+3);T.Add(B+2);T.Add(B);T.Add(B+2);T.Add(B+1);
            T.Add(B+3);T.Add(B);T.Add(B+4);T.Add(B+3);T.Add(B+4);T.Add(B+7);
            T.Add(B+1);T.Add(B+2);T.Add(B+6);T.Add(B+1);T.Add(B+6);T.Add(B+5);

            Farm->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        }
    }
};
