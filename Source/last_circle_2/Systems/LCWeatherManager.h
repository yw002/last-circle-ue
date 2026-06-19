#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LCGameState.h"
#include "ProceduralMeshComponent.h"
#include "LCWeatherManager.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCWeatherManager : public AActor
{
    GENERATED_BODY()
public:
    ALCWeatherManager() { PrimaryActorTick.bCanEverTick = true; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        CurrentWeather = EWeatherType::Clear;
        WeatherTimer = FMath::RandRange(60.f, 120.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        WeatherTimer -= DeltaSeconds;
        if (WeatherTimer <= 0.f)
        {
            switch (CurrentWeather)
            {
            case EWeatherType::Clear:    CurrentWeather = EWeatherType::Storm; WeatherTimer = 50.f; break;
            case EWeatherType::Storm:    CurrentWeather = EWeatherType::Blizzard; WeatherTimer = 120.f; break;
            case EWeatherType::Blizzard: CurrentWeather = EWeatherType::Clear; WeatherTimer = FMath::RandRange(60.f, 120.f); break;
            }
            if (ALCGameState* GS = Cast<ALCGameState>(UGameplayStatics::GetGameState(this)))
                GS->SetCurrentWeather(CurrentWeather);
        }
    }
    UFUNCTION(BlueprintCallable) EWeatherType GetWeather() const { return CurrentWeather; }
protected:
    UPROPERTY() EWeatherType CurrentWeather = EWeatherType::Clear;
    UPROPERTY() float WeatherTimer = 0.f;
};
