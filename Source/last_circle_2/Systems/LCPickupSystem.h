#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

        // Random type
        PickupType = static_cast<EPickupType>(FMath::RandRange(0, 5));
        BuildPickupMesh();
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        GetWorldTimerManager().SetTimer(RotateTimer, [this]()
        {
            if (IsValid(this)) AddActorWorldRotation(FRotator(0.f, 5.f, 0.f));
        }, 0.05f, true);
    }
    UFUNCTION(BlueprintCallable) EPickupType GetPickupType() const { return PickupType; }
    UFUNCTION(BlueprintCallable) FName GetWeaponName() const { return WeaponName; }
    UFUNCTION(BlueprintCallable) void SetWeaponName(FName Name) { WeaponName = Name; PickupType = EPickupType::Weapon; }
    UFUNCTION(BlueprintCallable) int32 GetAmmoAmount() const { return AmmoAmount; }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* PickupMesh = nullptr;
    UPROPERTY() EPickupType PickupType = EPickupType::Ammo;
    UPROPERTY() FName WeaponName = NAME_None;
    UPROPERTY() int32 AmmoAmount = 60;
    UPROPERTY() FTimerHandle RotateTimer;

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
        float S = 8.f;
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
