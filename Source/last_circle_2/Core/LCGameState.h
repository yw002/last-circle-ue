#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/LCGameMode.h"
#include "LCGameState.generated.h"

UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear   UMETA(DisplayName = "Clear"),
    Storm   UMETA(DisplayName = "Storm"),
    Blizzard UMETA(DisplayName = "Blizzard")
};

UCLASS()
class LAST_CIRCLE_2_API ALCGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALCGameState();

    UFUNCTION(BlueprintCallable)
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }
    UFUNCTION(BlueprintCallable)
    void SetCurrentPhase(EGamePhase Phase) { CurrentPhase = Phase; }

    UFUNCTION(BlueprintCallable)
    int32 GetCurrentWave() const { return CurrentWave; }
    UFUNCTION(BlueprintCallable)
    void SetCurrentWave(int32 Wave) { CurrentWave = Wave; }

    UFUNCTION(BlueprintCallable)
    int32 GetAliveCount() const { return AliveCount; }
    UFUNCTION(BlueprintCallable)
    void SetAliveCount(int32 Count) { AliveCount = Count; }

    UFUNCTION(BlueprintCallable)
    float GetZoneRadius() const { return ZoneRadius; }
    UFUNCTION(BlueprintCallable)
    void SetZoneRadius(float Radius) { ZoneRadius = Radius; }

    UFUNCTION(BlueprintCallable)
    FVector GetZoneCenter() const { return ZoneCenter; }
    UFUNCTION(BlueprintCallable)
    void SetZoneCenter(const FVector& Center) { ZoneCenter = Center; }

    UFUNCTION(BlueprintCallable)
    float GetRestTimeRemaining() const { return RestTimeRemaining; }
    UFUNCTION(BlueprintCallable)
    void SetRestTimeRemaining(float Time) { RestTimeRemaining = Time; }

    UFUNCTION(BlueprintCallable)
    EWeatherType GetCurrentWeather() const { return CurrentWeather; }
    UFUNCTION(BlueprintCallable)
    void SetCurrentWeather(EWeatherType Weather) { CurrentWeather = Weather; }

    UFUNCTION(BlueprintCallable)
    float GetDayNightTime() const { return DayNightTime; }
    UFUNCTION(BlueprintCallable)
    void SetDayNightTime(float Time) { DayNightTime = Time; }

    UFUNCTION(BlueprintCallable)
    int32 GetTotalKills() const { return TotalKills; }
    UFUNCTION(BlueprintCallable)
    void AddKill() { TotalKills++; }

protected:
    UPROPERTY(BlueprintReadOnly, Replicated)
    EGamePhase CurrentPhase = EGamePhase::Parachuting;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 CurrentWave = 0;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 AliveCount = 101;

    UPROPERTY(BlueprintReadOnly, Replicated)
    float ZoneRadius = 3000.f;

    UPROPERTY(BlueprintReadOnly, Replicated)
    FVector ZoneCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Replicated)
    float RestTimeRemaining = 0.f;

    UPROPERTY(BlueprintReadOnly, Replicated)
    EWeatherType CurrentWeather = EWeatherType::Clear;

    UPROPERTY(BlueprintReadOnly, Replicated)
    float DayNightTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 TotalKills = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
