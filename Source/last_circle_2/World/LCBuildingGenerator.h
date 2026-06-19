#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "World/LCEnvironmentActor.h"
#include "LCBuildingGenerator.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCPickupActor : public AActor
{
    GENERATED_BODY()
public:
    ALCPickupActor()
    {
        PrimaryActorTick.bCanEverTick = true;
        Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
        SetRootComponent(Root);

        BubbleMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BubbleMesh"));
        BubbleMesh->SetupAttachment(Root);
        BubbleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        ItemMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ItemMesh"));
        ItemMesh->SetupAttachment(Root);
        ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BuildBubble();
        BuildItemMesh();
        UMaterial* M = ALCBaseCharacter::GetOrCreateVertexColorMaterial();
        if (M)
        {
            BubbleMesh->SetMaterial(0, UMaterialInstanceDynamic::Create(M, this));
            ItemMesh->SetMaterial(0, UMaterialInstanceDynamic::Create(M, this));
        }
        RotSpeed = FMath::FRandRange(60.f, 120.f);
        FloatSpeed = FMath::FRandRange(0.5f, 1.5f);
    }

    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        FRotator R = GetActorRotation();
        R.Yaw += RotSpeed * DeltaSeconds;
        SetActorRotation(R);

        FloatPhase += DeltaSeconds * FloatSpeed;
        SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, FMath::Sin(FloatPhase) * 0.3f));
    }

    void SetPickupType(int32 Type)
    {
        PickupType = Type;
    }

    int32 GetPickupType() const { return PickupType; }

protected:
    UPROPERTY() USceneComponent* Root;
    UPROPERTY() UProceduralMeshComponent* BubbleMesh;
    UPROPERTY() UProceduralMeshComponent* ItemMesh;
    float RotSpeed = 90.f;
    float FloatSpeed = 1.f;
    float FloatPhase = 0.f;
    int32 PickupType = 0; // 0=Ammo,1=AKM,2=M416,3=Medkit,4=Armor,5=Sniper

    void BuildBubble()
    {
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // 3 orthogonal rings (torus-like) so item is visible through gaps
        float R = 30.f, TR = 2.5f;
        int32 RSegs = 24;
        for (int32 plane = 0; plane < 3; ++plane)
        {
            for (int32 i = 0; i < RSegs; ++i)
            {
                float A1 = 2.f * PI * i / RSegs;
                float A2 = 2.f * PI * (i + 1) / RSegs;
                FVector C1, C2;
                if (plane == 0) { C1 = FVector(R * FMath::Cos(A1), R * FMath::Sin(A1), 0); C2 = FVector(R * FMath::Cos(A2), R * FMath::Sin(A2), 0); }
                else if (plane == 1) { C1 = FVector(R * FMath::Cos(A1), 0, R * FMath::Sin(A1)); C2 = FVector(R * FMath::Cos(A2), 0, R * FMath::Sin(A2)); }
                else { C1 = FVector(0, R * FMath::Cos(A1), R * FMath::Sin(A1)); C2 = FVector(0, R * FMath::Cos(A2), R * FMath::Sin(A2)); }
                FVector Fwd = (C2 - C1).GetSafeNormal();
                FVector Up = FVector::CrossProduct(Fwd, C1.GetSafeNormal()).GetSafeNormal();
                int32 B = V.Num();
                V.Add(C1 + Up * TR); V.Add(C1 - Up * TR);
                V.Add(C2 + Up * TR); V.Add(C2 - Up * TR);
                FColor Glow = (i % 2 == 0) ? FColor(255, 255, 100) : FColor(255, 255, 180);
                for (int32 j = 0; j < 4; ++j) { C.Add(Glow); UV.Add(FVector2D::ZeroVector); }
                T.Add(B); T.Add(B + 2); T.Add(B + 1);
                T.Add(B + 1); T.Add(B + 2); T.Add(B + 3);
            }
        }
        BubbleMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }

    void BuildItemMesh()
    {
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        FColor It;
        switch (PickupType)
        {
        case 0: It = FColor(220,180,20); break; // Ammo gold
        case 1: case 2: case 5: It = FColor(60,60,60); break; // Weapon dark
        case 3: It = FColor(220,30,30); break; // Medkit red
        case 4: It = FColor(30,80,200); break; // Armor blue
        default: It = FColor(200,200,200); break;
        }
        float S = 14.f;
        V.Add(FVector(-S,-S,-S)); V.Add(FVector(S,-S,-S)); V.Add(FVector(S,S,-S)); V.Add(FVector(-S,S,-S));
        V.Add(FVector(-S,-S,S)); V.Add(FVector(S,-S,S)); V.Add(FVector(S,S,S)); V.Add(FVector(-S,S,S));
        for (int32 i = 0; i < 8; ++i) { C.Add(It); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);
        ItemMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
};

UCLASS()
class LAST_CIRCLE_2_API ALCBuildingGenerator : public AActor
{
    GENERATED_BODY()
public:
    ALCBuildingGenerator() { PrimaryActorTick.bCanEverTick = false; }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        VertexColorMaterial = ALCBaseCharacter::GetOrCreateVertexColorMaterial();
        EnvActor = Cast<ALCEnvironmentActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ALCEnvironmentActor::StaticClass()));

        GenerateHouses();
        GenerateShops();
        GenerateRuins();
        GenerateFarms();
    }

protected:
    UPROPERTY() UMaterial* VertexColorMaterial = nullptr;
    ALCEnvironmentActor* EnvActor = nullptr;

    FVector SnapToTerrain(FVector Loc) const
    {
        if (!EnvActor) return Loc + FVector(0,0,200.f);
        float H = EnvActor->GetHeightAt(Loc.X, Loc.Y);
        Loc.Z = H;
        return Loc;
    }

    void SpawnPickupsAround(const FVector& Center, int32 Count)
    {
        for (int32 i = 0; i < Count; ++i)
        {
            float Angle = FMath::RandRange(0.f, 360.f);
            float Dist = FMath::RandRange(120.f, 400.f);
            FVector Loc = Center + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
                                           FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, 0.f);
            Loc = SnapToTerrain(Loc);
            Loc.Z += 100.f;
            ALCPickupActor* P = GetWorld()->SpawnActor<ALCPickupActor>(ALCPickupActor::StaticClass(), Loc, FRotator::ZeroRotator);
            if (P) P->SetPickupType(FMath::RandRange(0, 5));
        }
    }

    void ApplyMat(UProceduralMeshComponent* M)
    {
        if (!VertexColorMaterial || !M) return;
        for (int32 s = 0; s < M->GetNumSections(); ++s)
            M->SetMaterial(s, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
    }

    // --- Houses ---
    void GenerateHouses()
    {
        const float HalfMap = 28000.f;
        for (int32 i = 0; i < 60; ++i)
        {
            float X = FMath::RandRange(-HalfMap * 0.9f, HalfMap * 0.9f);
            float Y = FMath::RandRange(-HalfMap * 0.9f, HalfMap * 0.9f);
            FVector Loc = SnapToTerrain(FVector(X, Y, 0.f));
            BuildHouse(Loc);
            SpawnPickupsAround(Loc + FVector(0,0,50.f), FMath::RandRange(3, 6));
        }
    }

    void BuildHouse(const FVector& Location)
    {
        UProceduralMeshComponent* H = NewObject<UProceduralMeshComponent>(this);
        H->RegisterComponent();
        H->SetWorldLocation(Location);
        H->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        H->SetCollisionObjectType(ECC_WorldStatic);
        H->SetCollisionResponseToAllChannels(ECR_Block);

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        float W = FMath::RandRange(400.f, 800.f), D = FMath::RandRange(280.f, 600.f), Ht = FMath::RandRange(200.f, 500.f);

        auto AddWall = [&](const FVector& A, const FVector& B, const FVector& Cv, const FVector& Dv, FColor Col) {
            int32 Base = V.Num();
            V.Add(A); V.Add(B); V.Add(Cv); V.Add(Dv);
            for (int32 j = 0; j < 4; ++j) { C.Add(Col); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base);T.Add(Base+2);T.Add(Base+1);T.Add(Base);T.Add(Base+3);T.Add(Base+2);
        };

        FColor WallCol(200+FMath::RandRange(-30,30), 170+FMath::RandRange(-30,30), 130+FMath::RandRange(-30,30));
        FColor RoofCol(130+FMath::RandRange(-20,20), 35+FMath::RandRange(-10,10), 25+FMath::RandRange(-10,10));
        float hw = W*0.5f, hd = D*0.5f;

        AddWall(FVector(-hw,-hd,0), FVector(hw,-hd,0), FVector(hw,-hd,Ht), FVector(-hw,-hd,Ht), WallCol);
        AddWall(FVector(hw,hd,0), FVector(-hw,hd,0), FVector(-hw,hd,Ht), FVector(hw,hd,Ht), WallCol);
        AddWall(FVector(-hw,hd,0), FVector(-hw,-hd,0), FVector(-hw,-hd,Ht), FVector(-hw,hd,Ht), WallCol);
        AddWall(FVector(hw,-hd,0), FVector(hw,hd,0), FVector(hw,hd,Ht), FVector(hw,-hd,Ht), WallCol);

        // Roof pyramid
        int32 RB = V.Num();
        V.Add(FVector(-hw-10,-hd-10,Ht)); V.Add(FVector(hw+10,-hd-10,Ht));
        V.Add(FVector(hw+10,hd+10,Ht)); V.Add(FVector(-hw+10,hd+10,Ht));
        V.Add(FVector(0,0,Ht+120.f));
        for (int32 j = 0; j < 5; ++j) { C.Add(RoofCol); UV.Add(FVector2D::ZeroVector); }
        T.Add(RB);T.Add(RB+4);T.Add(RB+1);T.Add(RB+1);T.Add(RB+4);T.Add(RB+2);
        T.Add(RB+2);T.Add(RB+4);T.Add(RB+3);T.Add(RB+3);T.Add(RB+4);T.Add(RB);

        H->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        ApplyMat(H);
    }

    // --- Shops (small buildings with glass-like front) ---
    void GenerateShops()
    {
        const float HalfMap = 28000.f;
        for (int32 i = 0; i < 20; ++i)
        {
            float X = FMath::RandRange(-HalfMap * 0.85f, HalfMap * 0.85f);
            float Y = FMath::RandRange(-HalfMap * 0.85f, HalfMap * 0.85f);
            FVector Loc = SnapToTerrain(FVector(X, Y, 0.f));
            BuildShop(Loc);
            SpawnPickupsAround(Loc + FVector(0,0,40.f), FMath::RandRange(2, 5));
        }
    }

    void BuildShop(const FVector& Loc)
    {
        UProceduralMeshComponent* S = NewObject<UProceduralMeshComponent>(this);
        S->RegisterComponent(); S->SetWorldLocation(Loc);
        S->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        S->SetCollisionObjectType(ECC_WorldStatic);
        S->SetCollisionResponseToAllChannels(ECR_Block);

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        float W = 600.f, D = 400.f, Ht = 250.f;
        float hw = W*0.5f, hd = D*0.5f;
        FColor Wall(210,200,180), Glass(120,200,240);

        auto AddQuad = [&](const FVector& A, const FVector& B, const FVector& Cv, const FVector& Dv, FColor Col) {
            int32 Base = V.Num(); V.Add(A); V.Add(B); V.Add(Cv); V.Add(Dv);
            for (int32 j=0;j<4;++j){C.Add(Col);UV.Add(FVector2D::ZeroVector);}
            T.Add(Base);T.Add(Base+2);T.Add(Base+1);T.Add(Base);T.Add(Base+3);T.Add(Base+2);
        };

        AddQuad(FVector(-hw,-hd,0),FVector(hw,-hd,0),FVector(hw,-hd,Ht),FVector(-hw,-hd,Ht), Glass);
        AddQuad(FVector(hw,hd,0),FVector(-hw,hd,0),FVector(-hw,hd,Ht),FVector(hw,hd,Ht), Wall);
        AddQuad(FVector(-hw,hd,0),FVector(-hw,-hd,0),FVector(-hw,-hd,Ht),FVector(-hw,hd,Ht), Wall);
        AddQuad(FVector(hw,-hd,0),FVector(hw,hd,0),FVector(hw,hd,Ht),FVector(hw,-hd,Ht), Wall);

        S->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
        ApplyMat(S);
    }

    // --- Ruins (stone circles) ---
    void GenerateRuins()
    {
        const float HalfMap = 28000.f;
        for (int32 i = 0; i < 10; ++i)
        {
            float X = FMath::RandRange(-HalfMap * 0.88f, HalfMap * 0.88f);
            float Y = FMath::RandRange(-HalfMap * 0.88f, HalfMap * 0.88f);
            FVector Loc = SnapToTerrain(FVector(X, Y, 0.f));

            UProceduralMeshComponent* R = NewObject<UProceduralMeshComponent>(this);
            R->RegisterComponent(); R->SetWorldLocation(Loc);
            R->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            R->SetCollisionObjectType(ECC_WorldStatic);
            R->SetCollisionResponseToAllChannels(ECR_Block);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
            int32 NumPillars = FMath::RandRange(5, 12);
            float CircleR = FMath::RandRange(200.f, 800.f);
            for (int32 p = 0; p < NumPillars; ++p)
            {
                float A = 2.f*PI*p/NumPillars;
                float PX = CircleR * FMath::Cos(A), PY = CircleR * FMath::Sin(A);
                float PH = FMath::RandRange(100.f, 500.f), PR = FMath::RandRange(12.f, 30.f);
                int32 Segs = 6;
                for (int32 s = 0; s < Segs; ++s)
                {
                    float A1=2.f*PI*s/Segs,A2=2.f*PI*((s+1)%Segs)/Segs;
                    int32 B = V.Num();
                    V.Add(FVector(PX+PR*FMath::Cos(A1),PY+PR*FMath::Sin(A1),0));
                    V.Add(FVector(PX+PR*FMath::Cos(A1),PY+PR*FMath::Sin(A1),PH));
                    V.Add(FVector(PX+PR*FMath::Cos(A2),PY+PR*FMath::Sin(A2),0));
                    V.Add(FVector(PX+PR*FMath::Cos(A2),PY+PR*FMath::Sin(A2),PH));
                    for(int32 j=0;j<4;++j){C.Add(FColor(140,135,120));UV.Add(FVector2D::ZeroVector);}
                    T.Add(B);T.Add(B+2);T.Add(B+1);T.Add(B+1);T.Add(B+2);T.Add(B+3);
                }
            }
            R->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
            ApplyMat(R);
            SpawnPickupsAround(Loc + FVector(0,0,50.f), FMath::RandRange(1, 3));
        }
    }

    // --- Farms ---
    void GenerateFarms()
    {
        const float HalfMap = 28000.f;
        for (int32 i = 0; i < 8; ++i)
        {
            float X = FMath::RandRange(-HalfMap * 0.85f, HalfMap * 0.85f);
            float Y = FMath::RandRange(-HalfMap * 0.85f, HalfMap * 0.85f);
            FVector Loc = SnapToTerrain(FVector(X, Y, 0.f));

            UProceduralMeshComponent* Fm = NewObject<UProceduralMeshComponent>(this);
            Fm->RegisterComponent(); Fm->SetWorldLocation(Loc);
            Fm->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Fm->SetCollisionObjectType(ECC_WorldStatic);
            Fm->SetCollisionResponseToAllChannels(ECR_Block);

            TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
            TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

            float BW = 80.f, BH = 60.f, BD = 50.f;
            int32 B = V.Num();
            V.Add(FVector(-BW,-BD,0));V.Add(FVector(BW,-BD,0));V.Add(FVector(BW,BD,0));V.Add(FVector(-BW,BD,0));
            V.Add(FVector(-BW,-BD,BH));V.Add(FVector(BW,-BD,BH));V.Add(FVector(BW,BD,BH));V.Add(FVector(-BW,BD,BH));
            FColor Barn(150+FMath::RandRange(-20,20), 30+FMath::RandRange(-10,10), 20+FMath::RandRange(-10,10));
            for (int32 j=0;j<8;++j){C.Add(Barn);UV.Add(FVector2D::ZeroVector);}
            T.Add(B);T.Add(B+1);T.Add(B+5);T.Add(B);T.Add(B+5);T.Add(B+4);
            T.Add(B+2);T.Add(B+3);T.Add(B+7);T.Add(B+2);T.Add(B+7);T.Add(B+6);
            T.Add(B+4);T.Add(B+5);T.Add(B+6);T.Add(B+4);T.Add(B+6);T.Add(B+7);
            T.Add(B);T.Add(B+3);T.Add(B+2);T.Add(B);T.Add(B+2);T.Add(B+1);
            T.Add(B+3);T.Add(B);T.Add(B+4);T.Add(B+3);T.Add(B+4);T.Add(B+7);
            T.Add(B+1);T.Add(B+2);T.Add(B+6);T.Add(B+1);T.Add(B+6);T.Add(B+5);

            // Fence posts
            int32 NumPosts = 12;
            float FR = 200.f;
            for (int32 p = 0; p < NumPosts; ++p)
            {
                float A = 2.f*PI*p/NumPosts;
                float PX=FR*FMath::Cos(A),PY=FR*FMath::Sin(A),PH=24.f,PS=4.f;
                V.Add(FVector(PX-PS,PY-PS,0));V.Add(FVector(PX+PS,PY-PS,0));
                V.Add(FVector(PX+PS,PY+PS,0));V.Add(FVector(PX-PS,PY+PS,0));
                V.Add(FVector(PX-PS,PY-PS,PH));V.Add(FVector(PX+PS,PY-PS,PH));
                V.Add(FVector(PX+PS,PY+PS,PH));V.Add(FVector(PX-PS,PY+PS,PH));
                FColor Wood(120,80,40);
                for(int32 j=0;j<8;++j){C.Add(Wood);UV.Add(FVector2D::ZeroVector);}
                int32 Fb=V.Num()-8;
                T.Add(Fb);T.Add(Fb+1);T.Add(Fb+5);T.Add(Fb);T.Add(Fb+5);T.Add(Fb+4);
                T.Add(Fb+2);T.Add(Fb+3);T.Add(Fb+7);T.Add(Fb+2);T.Add(Fb+7);T.Add(Fb+6);
                T.Add(Fb+4);T.Add(Fb+5);T.Add(Fb+6);T.Add(Fb+4);T.Add(Fb+6);T.Add(Fb+7);
                T.Add(Fb);T.Add(Fb+3);T.Add(Fb+2);T.Add(Fb);T.Add(Fb+2);T.Add(Fb+1);
            }

            Fm->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
            ApplyMat(Fm);
            SpawnPickupsAround(Loc + FVector(0,0,30.f), FMath::RandRange(2, 4));
        }
    }
};
