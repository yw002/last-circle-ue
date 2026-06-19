#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Weapons/LCWeaponTypes.h"
#include "LCWeaponBase.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCWeaponBase : public AActor
{
    GENERATED_BODY()
public:
    ALCWeaponBase()
    {
        PrimaryActorTick.bCanEverTick = false;
        WeaponMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WeaponMesh"));
        RootComponent = WeaponMesh;
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UFUNCTION(BlueprintCallable)
    void InitializeWeapon(const FWeaponData& Data, FName InWeaponName)
    {
        WeaponData = Data;
        WeaponName = InWeaponName;
        BuildWeaponModel();
    }

    UFUNCTION(BlueprintCallable)
    FName GetWeaponName() const { return WeaponName; }

    UFUNCTION(BlueprintCallable)
    const FWeaponData& GetWeaponData() const { return WeaponData; }

    UFUNCTION(BlueprintCallable)
    void SetVisible(bool bVisible) { if (WeaponMesh) WeaponMesh->SetVisibility(bVisible); }

protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* WeaponMesh = nullptr;
    UPROPERTY() FWeaponData WeaponData;
    UPROPERTY() FName WeaponName = NAME_None;

    void BuildWeaponModel()
    {
        if (!WeaponMesh) return;
        WeaponMesh->ClearAllMeshSections();

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        FColor GunColor(60, 60, 70);
        FColor MetalColor(80, 80, 90);
        FColor WoodColor(100, 60, 30);

        float BodyLen = 20.f, BodyH = 3.f, BodyW = 2.f;
        float BarrelLen = 15.f, BarrelR = 0.8f;
        float MagH = 8.f, MagW = 2.f;

        // Scale based on weapon category
        switch (WeaponData.Category)
        {
        case EWeaponCategory::Sniper: BodyLen = 28.f; BarrelLen = 22.f; break;
        case EWeaponCategory::Shotgun: BodyLen = 24.f; BarrelLen = 18.f; BodyH = 4.f; break;
        case EWeaponCategory::SMG: BodyLen = 16.f; BarrelLen = 10.f; break;
        case EWeaponCategory::Pistol: BodyLen = 10.f; BarrelLen = 8.f; BodyH = 4.f; break;
        case EWeaponCategory::Melee: BodyLen = 5.f; BarrelLen = 20.f; break;
        default: break;
        }

        // Gun body (receiver)
        int32 BB = V.Num();
        V.Add(FVector(-BodyLen*0.3f, -BodyW, -BodyH)); V.Add(FVector(BodyLen*0.7f, -BodyW, -BodyH));
        V.Add(FVector(BodyLen*0.7f, BodyW, -BodyH)); V.Add(FVector(-BodyLen*0.3f, BodyW, -BodyH));
        V.Add(FVector(-BodyLen*0.3f, -BodyW, BodyH)); V.Add(FVector(BodyLen*0.7f, -BodyW, BodyH));
        V.Add(FVector(BodyLen*0.7f, BodyW, BodyH)); V.Add(FVector(-BodyLen*0.3f, BodyW, BodyH));
        for (int32 i = 0; i < 8; ++i) { C.Add(GunColor); UV.Add(FVector2D::ZeroVector); }
        T.Add(BB);T.Add(BB+1);T.Add(BB+5);T.Add(BB);T.Add(BB+5);T.Add(BB+4);
        T.Add(BB+2);T.Add(BB+3);T.Add(BB+7);T.Add(BB+2);T.Add(BB+7);T.Add(BB+6);
        T.Add(BB+4);T.Add(BB+5);T.Add(BB+6);T.Add(BB+4);T.Add(BB+6);T.Add(BB+7);
        T.Add(BB);T.Add(BB+3);T.Add(BB+2);T.Add(BB);T.Add(BB+2);T.Add(BB+1);
        T.Add(BB+3);T.Add(BB);T.Add(BB+4);T.Add(BB+3);T.Add(BB+4);T.Add(BB+7);
        T.Add(BB+1);T.Add(BB+2);T.Add(BB+6);T.Add(BB+1);T.Add(BB+6);T.Add(BB+5);

        // Barrel
        int32 Segs = 8;
        for (int32 i = 0; i < Segs; ++i)
        {
            float A1 = 2.f * PI * i / Segs;
            float A2 = 2.f * PI * ((i+1)%Segs) / Segs;
            int32 Base = V.Num();
            float BX = BodyLen * 0.7f;
            V.Add(FVector(BX, BarrelR*FMath::Cos(A1), BarrelR*FMath::Sin(A1)));
            V.Add(FVector(BX + BarrelLen, BarrelR*FMath::Cos(A1), BarrelR*FMath::Sin(A1)));
            V.Add(FVector(BX, BarrelR*FMath::Cos(A2), BarrelR*FMath::Sin(A2)));
            V.Add(FVector(BX + BarrelLen, BarrelR*FMath::Cos(A2), BarrelR*FMath::Sin(A2)));
            for (int32 j = 0; j < 4; ++j) { C.Add(MetalColor); UV.Add(FVector2D::ZeroVector); }
            T.Add(Base);T.Add(Base+1);T.Add(Base+2);T.Add(Base+2);T.Add(Base+1);T.Add(Base+3);
        }

        // Magazine (if not melee/throwable)
        if (WeaponData.Category != EWeaponCategory::Melee && WeaponData.Category != EWeaponCategory::Throwable)
        {
            int32 MB = V.Num();
            float MX = 0.f, MW = MagW * 0.5f;
            V.Add(FVector(MX-MW, -MW, -BodyH)); V.Add(FVector(MX+MW, -MW, -BodyH));
            V.Add(FVector(MX+MW, MW, -BodyH)); V.Add(FVector(MX-MW, MW, -BodyH));
            V.Add(FVector(MX-MW, -MW, -BodyH-MagH)); V.Add(FVector(MX+MW, -MW, -BodyH-MagH));
            V.Add(FVector(MX+MW, MW, -BodyH-MagH)); V.Add(FVector(MX-MW, MW, -BodyH-MagH));
            FColor MagCol(50, 50, 55);
            for (int32 i = 0; i < 8; ++i) { C.Add(MagCol); UV.Add(FVector2D::ZeroVector); }
            T.Add(MB);T.Add(MB+1);T.Add(MB+5);T.Add(MB);T.Add(MB+5);T.Add(MB+4);
            T.Add(MB+2);T.Add(MB+3);T.Add(MB+7);T.Add(MB+2);T.Add(MB+7);T.Add(MB+6);
            T.Add(MB+4);T.Add(MB+5);T.Add(MB+6);T.Add(MB+4);T.Add(MB+6);T.Add(MB+7);
            T.Add(MB);T.Add(MB+3);T.Add(MB+2);T.Add(MB);T.Add(MB+2);T.Add(MB+1);
        }

        // Stock (for AR/Sniper/Shotgun)
        if (WeaponData.Category == EWeaponCategory::AssaultRifle || WeaponData.Category == EWeaponCategory::Sniper || WeaponData.Category == EWeaponCategory::Shotgun)
        {
            int32 SB = V.Num();
            float SX = -BodyLen * 0.3f;
            float StockLen = 10.f;
            V.Add(FVector(SX-StockLen, -BodyW*0.8f, -BodyH*0.5f)); V.Add(FVector(SX, -BodyW*0.8f, -BodyH*0.5f));
            V.Add(FVector(SX, BodyW*0.8f, -BodyH*0.5f)); V.Add(FVector(SX-StockLen, BodyW*0.8f, -BodyH*0.5f));
            V.Add(FVector(SX-StockLen, -BodyW*0.8f, BodyH*0.8f)); V.Add(FVector(SX, -BodyW*0.8f, BodyH*0.8f));
            V.Add(FVector(SX, BodyW*0.8f, BodyH*0.8f)); V.Add(FVector(SX-StockLen, BodyW*0.8f, BodyH*0.8f));
            for (int32 i = 0; i < 8; ++i) { C.Add(WoodColor); UV.Add(FVector2D::ZeroVector); }
            T.Add(SB);T.Add(SB+1);T.Add(SB+5);T.Add(SB);T.Add(SB+5);T.Add(SB+4);
            T.Add(SB+2);T.Add(SB+3);T.Add(SB+7);T.Add(SB+2);T.Add(SB+7);T.Add(SB+6);
            T.Add(SB+4);T.Add(SB+5);T.Add(SB+6);T.Add(SB+4);T.Add(SB+6);T.Add(SB+7);
            T.Add(SB);T.Add(SB+3);T.Add(SB+2);T.Add(SB);T.Add(SB+2);T.Add(SB+1);
        }

        WeaponMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
};
