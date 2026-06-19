#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LCZoneManager.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCZoneManager : public AActor
{
    GENERATED_BODY()
public:
    ALCZoneManager() { PrimaryActorTick.bCanEverTick = true; }
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable) float GetZoneRadius() const { return ZoneRadius; }
    UFUNCTION(BlueprintCallable) FVector GetZoneCenter() const { return ZoneCenter; }
    UFUNCTION(BlueprintCallable) bool IsOutsideZone(const FVector& Location) const;
    UFUNCTION(BlueprintCallable) float GetZoneDamage() const { return CurrentPhase * 2.f; }

protected:
    UPROPERTY() float ZoneRadius = 3000.f;
    UPROPERTY() FVector ZoneCenter = FVector::ZeroVector;
    UPROPERTY() float ShrinkTimer = 60.f;
    UPROPERTY() int32 CurrentPhase = 1;
    UPROPERTY() UProceduralMeshComponent* ZoneMesh = nullptr;

    void ShrinkZone();
    void UpdateZoneVisual();
};
