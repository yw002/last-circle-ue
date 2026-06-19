#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LCWaveManager.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCWaveManager : public AActor
{
    GENERATED_BODY()
public:
    ALCWaveManager() { PrimaryActorTick.bCanEverTick = true; }
    virtual void BeginPlay() override { Super::BeginPlay(); }
    virtual void Tick(float DeltaSeconds) override { Super::Tick(DeltaSeconds); }

    UFUNCTION(BlueprintCallable)
    int32 GetCurrentWave() const { return CurrentWave; }
    UFUNCTION(BlueprintCallable)
    bool IsResting() const { return bIsResting; }
    UFUNCTION(BlueprintCallable)
    float GetRestTimer() const { return RestTimer; }

protected:
    UPROPERTY(BlueprintReadOnly) int32 CurrentWave = 0;
    UPROPERTY(BlueprintReadOnly) bool bIsResting = false;
    UPROPERTY() float RestTimer = 0.f;
};
