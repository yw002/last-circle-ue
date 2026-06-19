#pragma once

#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "Core/LCPlayerState.h"
#include "LCBaseCharacter.generated.h"

class UProceduralMeshComponent;

UCLASS()
class LAST_CIRCLE_2_API ALCBaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALCBaseCharacter();

    virtual void BeginPlay() override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable)
    float GetHealth() const { return Health; }
    UFUNCTION(BlueprintCallable)
    void SetHealth(float NewHealth) { Health = FMath::Clamp(NewHealth, 0.f, MaxHealth); }
    UFUNCTION(BlueprintCallable)
    float GetMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintCallable)
    void AddHealth(float Amount) { SetHealth(Health + Amount); }

    UFUNCTION(BlueprintCallable)
    bool IsAlive() const { return Health > 0.f; }

    UFUNCTION(BlueprintCallable)
    void SetHealthMultiplier(float Mult) { HealthMultiplier = Mult; }
    UFUNCTION(BlueprintCallable)
    void SetDamageMultiplier(float Mult) { DamageMultiplier = Mult; }
    UFUNCTION(BlueprintCallable)
    void SetSpeedMultiplier(float Mult);

    UFUNCTION(BlueprintCallable)
    float GetDamageMultiplier() const { return DamageMultiplier; }

    UFUNCTION(BlueprintCallable)
    virtual void Die(AActor* Killer);

    UFUNCTION(BlueprintCallable)
    EEquipmentLevel GetHelmetLevel() const { return HelmetLevel; }
    UFUNCTION(BlueprintCallable)
    void SetHelmetLevel(EEquipmentLevel Level) { HelmetLevel = Level; }

    UFUNCTION(BlueprintCallable)
    EEquipmentLevel GetArmorLevel() const { return ArmorLevel; }
    UFUNCTION(BlueprintCallable)
    void SetArmorLevel(EEquipmentLevel Level) { ArmorLevel = Level; }

    UFUNCTION(BlueprintCallable)
    float GetHelmetReduction() const;
    UFUNCTION(BlueprintCallable)
    float GetArmorReduction() const;

    UFUNCTION(BlueprintCallable)
    void BuildProceduralBody(const FColor& SkinColor, const FColor& ClothColor);

    UFUNCTION(BlueprintCallable)
    void ApplyVertexColorMaterial();

    UFUNCTION(BlueprintCallable)
    void FlashRed(float Duration = 0.15f);

    static UMaterial* GetOrCreateVertexColorMaterial();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Health = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MaxHealth = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float HealthMultiplier = 1.f;

    UPROPERTY(BlueprintReadOnly)
    float DamageMultiplier = 1.f;

    UPROPERTY(BlueprintReadOnly)
    float BaseSpeed = 600.f;

    UPROPERTY(BlueprintReadOnly)
    EEquipmentLevel HelmetLevel = EEquipmentLevel::None;

    UPROPERTY(BlueprintReadOnly)
    EEquipmentLevel ArmorLevel = EEquipmentLevel::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent* BodyMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent* HeadMesh = nullptr;

    UPROPERTY()
    UMaterial* VertexColorMaterial = nullptr;

    UPROPERTY()
    bool bIsDead = false;

    void BuildBodyPart(UProceduralMeshComponent* Mesh, const FVector& Offset, const FVector& Size, const FColor& Color);
    void BuildHead(UProceduralMeshComponent* Mesh, const FVector& Offset, float Radius, const FColor& Color);
};
