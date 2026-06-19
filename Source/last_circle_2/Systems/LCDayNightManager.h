#pragma once
#include "CoreMinimal.h"
#include "Components/DirectionalLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "LCDayNightManager.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCDayNightManager : public AActor
{
    GENERATED_BODY()
public:
    ALCDayNightManager()
    {
        PrimaryActorTick.bCanEverTick = true;
        SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
        SunLight->SetMobility(EComponentMobility::Movable);
        SunLight->SetIntensity(5.f);
        SunLight->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
        RootComponent = SunLight;

        MoonMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MoonMesh"));
        MoonMesh->SetupAttachment(RootComponent);
        MoonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        TimeOfDay += DeltaSeconds;
        if (TimeOfDay >= CycleDuration) TimeOfDay -= CycleDuration;

        float SunAngle = (TimeOfDay / CycleDuration) * 360.f;
        float SunRad = FMath::DegreesToRadians(SunAngle);
        float TiltRad = FMath::DegreesToRadians(30.f);

        FVector SunDir(FMath::Cos(SunRad), FMath::Sin(SunRad) * FMath::Sin(TiltRad), FMath::Sin(SunRad) * FMath::Cos(TiltRad));
        SunDir.Normalize();

        if (SunLight)
        {
            SunLight->SetWorldRotation(SunDir.Rotation());
            float Intensity = FMath::Clamp(SunDir.Z * 6.f, 1.5f, 8.f);
            SunLight->SetIntensity(Intensity);

            FLinearColor SunColor;
            if (SunDir.Z > 0.3f) SunColor = FLinearColor(1.f, 0.95f, 0.85f);
            else if (SunDir.Z > 0.f) SunColor = FLinearColor(1.f, 0.6f, 0.3f);
            else SunColor = FLinearColor(0.1f, 0.1f, 0.3f);
            SunLight->SetLightColor(SunColor);
        }
    }

    UPROPERTY(VisibleAnywhere) UDirectionalLightComponent* SunLight = nullptr;
    UPROPERTY() UProceduralMeshComponent* MoonMesh = nullptr;
    UFUNCTION(BlueprintCallable) float GetTimeOfDay() const { return TimeOfDay; }
protected:
    UPROPERTY() float TimeOfDay = 0.f;
    UPROPERTY(EditAnywhere) float CycleDuration = 600.f;
};
