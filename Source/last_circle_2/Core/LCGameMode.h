#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LCGameMode.generated.h"

class ALCGameState;
class ALCPlayerCharacter;
class ALCWaveManager;
class ALCZoneManager;
class ALCWeatherManager;
class ALCDayNightManager;
class ALCDisasterSystem;
class ALCFunnyEventSystem;
class ALCPickupItem;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Parachuting   UMETA(DisplayName = "Parachuting"),
    Looting       UMETA(DisplayName = "Looting"),
    WaveCombat    UMETA(DisplayName = "WaveCombat"),
    RestPeriod    UMETA(DisplayName = "RestPeriod"),
    BossFight     UMETA(DisplayName = "BossFight"),
    Victory       UMETA(DisplayName = "Victory"),
    GameOver      UMETA(DisplayName = "GameOver")
};

UCLASS()
class LAST_CIRCLE_2_API ALCGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALCGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

    UFUNCTION(BlueprintCallable)
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintCallable)
    void SetGamePhase(EGamePhase NewPhase);

    UFUNCTION(BlueprintCallable)
    int32 GetCurrentWave() const { return CurrentWave; }

    UFUNCTION(BlueprintCallable)
    int32 GetAliveCount() const;

    UFUNCTION(BlueprintCallable)
    void OnEntityKilled(AActor* Killer, AActor* Victim, bool bIsHeadshot);

    UFUNCTION(BlueprintCallable)
    void OnWaveCleared();

    UFUNCTION(BlueprintCallable)
    void OnBossKilled();

    UFUNCTION(BlueprintCallable)
    void SpawnPickupAtLocation(const FVector& Location, int32 Count);

    UFUNCTION(BlueprintCallable)
    void SpawnWaveEnemies();

    UFUNCTION(BlueprintCallable)
    void SpawnBossGiant();

    UFUNCTION(BlueprintCallable)
    bool GetBossAlive() const { return bBossAlive; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EGamePhase, NewPhase);
    UPROPERTY(BlueprintAssignable)
    FOnGamePhaseChanged OnGamePhaseChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEntityKilledEvent, AActor*, Killer, AActor*, Victim);
    UPROPERTY(BlueprintAssignable)
    FOnEntityKilledEvent OnEntityKilledEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, WaveNumber);
    UPROPERTY(BlueprintAssignable)
    FOnWaveStarted OnWaveStarted;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossSpawned, AActor*, BossActor);
    UPROPERTY(BlueprintAssignable)
    FOnBossSpawned OnBossSpawned;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGamePhase CurrentPhase = EGamePhase::Parachuting;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentWave = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalKills = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bBossKilledThisRun = false;

    UPROPERTY(BlueprintReadOnly)
    bool bBossAlive = false;

    UPROPERTY()
    float RestTimer = 0.f;

    UPROPERTY()
    float PhaseTimer = 0.f;

    static constexpr int32 TOTAL_WAVES = 20;
    static constexpr float REST_DURATION = 15.f;
    static constexpr int32 BASE_ENEMY_COUNT = 5;
    static constexpr int32 ENEMIES_PER_WAVE = 3;
    static constexpr int32 BOSS_EVERY_N_WAVES = 5;
    static constexpr float KILL_THRESHOLD = 0.80f;

    void HandleParachuting(float DeltaSeconds);
    void HandleWaveCombat(float DeltaSeconds);
    void HandleRestPeriod(float DeltaSeconds);
    void HandleBossFight(float DeltaSeconds);
    void CheckVictoryConditions();

    FVector GetRandomSpawnLocation(const FVector& PlayerPos) const;

    TArray<TSubclassOf<AActor>> GetEnemyClassesForWave(int32 Wave) const;
};
