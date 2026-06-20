#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Characters/LCBaseCharacter.h"
#include "ProceduralMeshComponent.h"
#include "LCPickupSystem.generated.h"

UENUM(BlueprintType)
enum class EPickupType : uint8
{
    Weapon, Ammo, Medkit, Scope, Helmet, Armor, SpecialWeapon
};

UCLASS()
class LAST_CIRCLE_2_API ALCPickupItem : public AActor
{
    GENERATED_BODY()
public:
    ALCPickupItem()
    {
        PrimaryActorTick.bCanEverTick = true;
        Tags.Add(FName("Pickup"));
        PickupMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PickupMesh"));
        RootComponent = PickupMesh;
        PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        PickupMesh->SetCollisionProfileName(FName("OverlapAllDynamic"));

        PickupType = static_cast<EPickupType>(FMath::RandRange(0, 5));
        BuildPickupMesh();
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BaseZ = GetActorLocation().Z;
        // Apply vertex color material so pickup shows color
        if (UMaterial* Mat = ALCBaseCharacter::GetOrCreateVertexColorMaterial())
        {
            for (int32 i = 0; i < PickupMesh->GetNumSections(); ++i)
                PickupMesh->SetMaterial(i, UMaterialInstanceDynamic::Create(Mat, this));
        }
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        AddActorWorldRotation(FRotator(0.f, 90.f * DT, 0.f));
        float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 8.f;
        FVector Loc = GetActorLocation();
        Loc.Z = BaseZ + Bob;
        SetActorLocation(Loc);
    }

    UFUNCTION(BlueprintCallable) EPickupType GetPickupType() const { return PickupType; }
    UFUNCTION(BlueprintCallable) FName GetWeaponName() const { return WeaponName; }
    UFUNCTION(BlueprintCallable) void SetWeaponName(FName Name) { WeaponName = Name; PickupType = EPickupType::Weapon; }
    UFUNCTION(BlueprintCallable) int32 GetAmmoAmount() const { return AmmoAmount; }

    UFUNCTION(BlueprintCallable)
    FString GetDisplayName() const
    {
        switch (PickupType)
        {
        case EPickupType::Weapon: return WeaponName.IsNone() ? TEXT("Weapon") : WeaponName.ToString();
        case EPickupType::Ammo: return FString::Printf(TEXT("Ammo x%d"), AmmoAmount);
        case EPickupType::Medkit: return TEXT("Medkit");
        case EPickupType::Scope: return TEXT("Scope");
        case EPickupType::Helmet: return TEXT("Helmet");
        case EPickupType::Armor: return TEXT("Armor");
        case EPickupType::SpecialWeapon: return TEXT("Special Weapon");
        default: return TEXT("Pickup");
        }
    }

protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* PickupMesh = nullptr;
    UPROPERTY() EPickupType PickupType = EPickupType::Ammo;
    UPROPERTY() FName WeaponName = NAME_None;
    UPROPERTY() int32 AmmoAmount = 60;
    UPROPERTY() float BaseZ = 0.f;

    void BuildPickupMesh()
    {
        if (!PickupMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        FColor Col = FColor::White;
        switch (PickupType)
        {
        case EPickupType::Weapon: Col = FColor(255, 100, 0); break;
        case EPickupType::Ammo: Col = FColor(255, 200, 0); break;
        case EPickupType::Medkit: Col = FColor(0, 255, 0); break;
        case EPickupType::Scope: Col = FColor(0, 150, 255); break;
        case EPickupType::Helmet: Col = FColor(100, 100, 255); break;
        case EPickupType::Armor: Col = FColor(150, 150, 150); break;
        default: Col = FColor(255, 0, 255); break;
        }
        float S = 15.f;
        V.Add(FVector(-S,-S,-S)); V.Add(FVector(S,-S,-S)); V.Add(FVector(S,S,-S)); V.Add(FVector(-S,S,-S));
        V.Add(FVector(-S,-S,S)); V.Add(FVector(S,-S,S)); V.Add(FVector(S,S,S)); V.Add(FVector(-S,S,S));
        for (int32 i = 0; i < 8; ++i) { C.Add(Col); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);
        PickupMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};
