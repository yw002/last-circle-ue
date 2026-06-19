#include "Core/LCGameMode.h"
#include "Core/LCGameState.h"
#include "Core/LCPlayerState.h"
#include "Core/LCPlayerController.h"
#include "Characters/LCPlayerCharacter.h"
#include "Systems/LCWaveManager.h"
#include "Systems/LCZoneManager.h"
#include "Systems/LCWeatherManager.h"
#include "Systems/LCDayNightManager.h"
#include "Systems/LCDisasterSystem.h"
#include "Systems/LCFunnyEventSystem.h"
#include "Systems/LCAirdropSystem.h"
#include "Systems/LCSpatialIndex.h"
#include "World/LCEnvironmentActor.h"
#include "World/LCBuildingGenerator.h"
#include "World/LCBridgeGenerator.h"
#include "World/LCCaveGenerator.h"
#include "AI/LCBotCharacter.h"
#include "AI/LCZombieCharacter.h"
#include "AI/LCGhostCharacter.h"
#include "AI/LCGiantBoss.h"
#include "AI/LCUFOActor.h"
#include "AI/LCAlienCharacter.h"
#include "AI/LCAnimalSystem.h"
#include "Systems/LCPickupSystem.h"
#include "Effects/LCEffectsSystem.h"
#include "Effects/LCAudioManager.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "GameFramework/PlayerStart.h"

ALCGameMode::ALCGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ALCPlayerCharacter::StaticClass();
    GameStateClass = ALCGameState::StaticClass();
    PlayerStateClass = ALCPlayerState::StaticClass();
    PlayerControllerClass = ALCPlayerController::StaticClass();
    UE_LOG(LogTemp, Warning, TEXT("=== LCGameMode Constructor === DefaultPawnClass=%s"), DefaultPawnClass ? *DefaultPawnClass->GetName() : TEXT("NULL"));
}

void ALCGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    UE_LOG(LogTemp, Warning, TEXT("=== LCGameMode InitGame === MapName=%s"), *MapName);
}

void ALCGameMode::BeginPlay()
{
    Super::BeginPlay();

    // === 禁用 VSM（虚拟阴影贴图）—— 程序化网格太多会溢出 VSM 队列，改回传统级联阴影 ===
    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.Shadow.Virtual.Enable 0"));
        GEngine->Exec(GetWorld(), TEXT("r.Shadow.CSM.MaxCascades 4"));
    }

    UE_LOG(LogTemp, Warning, TEXT("=== LCGameMode BeginPlay START ==="));
    UE_LOG(LogTemp, Warning, TEXT("DefaultPawnClass: %s"), DefaultPawnClass ? *DefaultPawnClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("PlayerController: %s"), UGameplayStatics::GetPlayerController(this, 0) ? *UGameplayStatics::GetPlayerController(this, 0)->GetName() : TEXT("NULL"));

    // === 强制创建必要的光照与天空，保证空场景也不会黑屏 ===
    {
        FActorSpawnParameters SkyParams;
        SkyParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // 太阳光（DirectionalLight）
        ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0, 0, 5000), FRotator(-50.f, 30.f, 0.f), SkyParams);
        if (Sun && Sun->GetLightComponent())
        {
            Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(10);
            Sun->SetBrightness(30.f);
            Sun->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
        }

        // 天空光（SkyLight，让阴影部分也有亮度）
        ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0, 0, 5000), FRotator::ZeroRotator, SkyParams);
        if (Sky && Sky->GetLightComponent())
        {
            Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            Sky->GetLightComponent()->SetIntensity(2.f);
            Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);
        }

        // 天空大气（蓝天 / 落日颜色）
        ASkyAtmosphere* Atmo = GetWorld()->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SkyParams);
        (void)Atmo;

        // 指数高度雾，让远处地形不至于一片漆黑
        AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SkyParams);
        (void)Fog;

        // 在原点放一个 PlayerStart，保证 Pawn 能成功 spawn
        if (!UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
        {
            GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0, 0, 2000), FRotator::ZeroRotator, SkyParams);
        }

        UE_LOG(LogTemp, Warning, TEXT("Sky/Sun/Fog/PlayerStart spawned."));
    }

    // Spawn environment (terrain, biomes, water, trees, rocks)
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    UE_LOG(LogTemp, Warning, TEXT("Spawning EnvironmentActor..."));
    ALCEnvironmentActor* Env = GetWorld()->SpawnActor<ALCEnvironmentActor>(ALCEnvironmentActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    UE_LOG(LogTemp, Warning, TEXT("EnvironmentActor: %s"), Env ? TEXT("OK") : TEXT("FAILED"));

    UE_LOG(LogTemp, Warning, TEXT("Spawning buildings..."));
    GetWorld()->SpawnActor<ALCBuildingGenerator>(ALCBuildingGenerator::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

    UE_LOG(LogTemp, Warning, TEXT("Spawning system managers..."));
    // ALL systems disabled for performance debugging.
    // SpawnWaveEnemies, Airdrop, Disaster, FunnyEvent, Weather, Zone, Spatial, LOD, Audio, Particles, Animal — all off.
    // GetWorld()->SpawnActor<ALCZoneManager>(ALCZoneManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCWeatherManager>(ALCWeatherManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCDisasterSystem>(ALCDisasterSystem::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCFunnyEventSystem>(ALCFunnyEventSystem::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCAirdropManager>(ALCAirdropManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCSpatialIndex>(ALCSpatialIndex::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCLODManager>(ALCLODManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCAudioManager>(ALCAudioManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCWeatherParticles>(ALCWeatherParticles::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    // GetWorld()->SpawnActor<ALCAnimalSpawner>(ALCAnimalSpawner::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

    UE_LOG(LogTemp, Warning, TEXT("All systems disabled for clean baseline test."));

    // Zero initial enemies
    UE_LOG(LogTemp, Warning, TEXT("Spawning initial enemies and bots... SKIPPED (0 enemies)"));


    UE_LOG(LogTemp, Warning, TEXT("Setting game phase to WaveCombat 20-WAVE system"));
    SetGamePhase(EGamePhase::WaveCombat);
    CurrentWave = 1;
    SpawnWaveEnemies();
    UE_LOG(LogTemp, Warning, TEXT("=== LCGameMode BeginPlay COMPLETE ==="));
}

void ALCGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    switch (CurrentPhase)
    {
    case EGamePhase::Parachuting:
        HandleParachuting(DeltaSeconds);
        break;
    case EGamePhase::WaveCombat:
        HandleWaveCombat(DeltaSeconds);
        break;
    case EGamePhase::RestPeriod:
        HandleRestPeriod(DeltaSeconds);
        break;
    case EGamePhase::BossFight:
        HandleBossFight(DeltaSeconds);
        break;
    case EGamePhase::Victory:
    case EGamePhase::GameOver:
        break;
    default:
        break;
    }
}

void ALCGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    UE_LOG(LogTemp, Warning, TEXT("=== LCGameMode PostLogin === Player=%s"), NewPlayer ? *NewPlayer->GetName() : TEXT("NULL"));
}

void ALCGameMode::SetGamePhase(EGamePhase NewPhase)
{
    CurrentPhase = NewPhase;
    PhaseTimer = 0.f;

    if (ALCGameState* GS = Cast<ALCGameState>(GameState))
    {
        GS->SetCurrentPhase(NewPhase);
        GS->SetCurrentWave(CurrentWave);
    }

    OnGamePhaseChanged.Broadcast(NewPhase);
}

int32 ALCGameMode::GetAliveCount() const
{
    int32 Count = 0;
    for (TActorIterator<ALCBotCharacter> It(GetWorld()); It; ++It)
    {
        if (It->GetHealth() > 0.f) Count++;
    }
    // Add player if alive
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            if (ALCBaseCharacter* Char = Cast<ALCBaseCharacter>(Pawn))
            {
                if (Char->GetHealth() > 0.f) Count++;
            }
        }
    }
    return Count;
}

void ALCGameMode::OnEntityKilled(AActor* Killer, AActor* Victim, bool bIsHeadshot)
{
    OnEntityKilledEvent.Broadcast(Killer, Victim);
    TotalKills++;

    if (ALCGameState* GS = Cast<ALCGameState>(GameState))
    {
        GS->SetAliveCount(GetAliveCount());
    }

    // Check if victim is boss
    if (Victim->IsA<ALCGiantBoss>())
    {
        OnBossKilled();
    }

    // Check wave progress
    if (CurrentPhase == EGamePhase::WaveCombat)
    {
        int32 AliveEnemies = 0;
        int32 TotalSpawnedThisWave = BASE_ENEMY_COUNT + CurrentWave * ENEMIES_PER_WAVE;
        for (TActorIterator<ALCBaseCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetHealth() > 0.f && !It->IsA<ALCPlayerCharacter>())
                AliveEnemies++;
        }

        float KillPercent = 1.f - ((float)AliveEnemies / FMath::Max(1, TotalSpawnedThisWave));
        if (KillPercent >= KILL_THRESHOLD)
        {
            OnWaveCleared();
        }
    }
}

void ALCGameMode::OnWaveCleared()
{
    if (CurrentWave % BOSS_EVERY_N_WAVES == 0 && CurrentWave > 0 && !bBossKilledThisRun)
    {
        // Boss wave required
        SpawnBossGiant();
        SetGamePhase(EGamePhase::BossFight);
    }
    else if (CurrentWave >= TOTAL_WAVES)
    {
        CheckVictoryConditions();
    }
    else
    {
        SetGamePhase(EGamePhase::RestPeriod);
        RestTimer = REST_DURATION;
    }
}

void ALCGameMode::OnBossKilled()
{
    bBossAlive = false;
    bBossKilledThisRun = true;

    if (CurrentWave >= TOTAL_WAVES)
    {
        CheckVictoryConditions();
    }
    else
    {
        SetGamePhase(EGamePhase::RestPeriod);
        RestTimer = REST_DURATION;
    }
}

void ALCGameMode::SpawnPickupAtLocation(const FVector& Location, int32 Count)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 i = 0; i < Count; ++i)
    {
        FVector Offset = Location + FVector(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), 10.f);
        GetWorld()->SpawnActor<ALCPickupItem>(ALCPickupItem::StaticClass(), Offset, FRotator::ZeroRotator, Params);
    }
}

void ALCGameMode::SpawnWaveEnemies()
{
    int32 EnemyCount = BASE_ENEMY_COUNT + CurrentWave * ENEMIES_PER_WAVE;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;
    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    float HealthScale = 1.f + CurrentWave * 0.08f;
    float DamageScale = 1.f + CurrentWave * 0.05f;
    float SpeedScale  = 1.f + CurrentWave * 0.02f;

    int32 ZombieCount = 0, BotCount = 0, GhostCount = 0, AlienCount = 0;

    if (CurrentWave >= 1 && CurrentWave <= 3)
    {
        ZombieCount = EnemyCount;
    }
    else if (CurrentWave >= 4 && CurrentWave <= 7)
    {
        ZombieCount = EnemyCount * 0.6f;
        BotCount    = EnemyCount * 0.4f;
    }
    else if (CurrentWave >= 8 && CurrentWave <= 14)
    {
        ZombieCount = EnemyCount * 0.30f;
        BotCount    = EnemyCount * 0.35f;
        GhostCount  = EnemyCount * 0.15f;
        AlienCount  = EnemyCount * 0.20f;
    }
    else
    {
        ZombieCount = EnemyCount * 0.25f;
        BotCount    = EnemyCount * 0.35f;
        GhostCount  = EnemyCount * 0.20f;
        AlienCount  = EnemyCount * 0.20f;
    }

    for (int32 i = 0; i < ZombieCount; ++i)
    {
        FVector Loc = GetRandomSpawnLocation(PlayerPawn->GetActorLocation());
        ALCZombieCharacter* Z = GetWorld()->SpawnActor<ALCZombieCharacter>(ALCZombieCharacter::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParams);
        if (Z) { Z->SetHealthMultiplier(HealthScale); Z->SetDamageMultiplier(DamageScale); Z->SetSpeedMultiplier(SpeedScale); }
    }
    for (int32 i = 0; i < BotCount; ++i)
    {
        FVector Loc = GetRandomSpawnLocation(PlayerPawn->GetActorLocation());
        ALCBotCharacter* B = GetWorld()->SpawnActor<ALCBotCharacter>(ALCBotCharacter::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParams);
        if (B) { B->SetHealthMultiplier(HealthScale); B->SetDamageMultiplier(DamageScale); B->SetSpeedMultiplier(SpeedScale); }
    }
    for (int32 i = 0; i < GhostCount; ++i)
    {
        FVector Loc = GetRandomSpawnLocation(PlayerPawn->GetActorLocation());
        GetWorld()->SpawnActor<ALCGhostCharacter>(ALCGhostCharacter::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParams);
    }
    // Spawn aliens for higher waves
    for (int32 i = 0; i < AlienCount; ++i)
    {
        FVector Loc = GetRandomSpawnLocation(PlayerPawn->GetActorLocation());
        ALCAlienCharacter* Alien = GetWorld()->SpawnActor<ALCAlienCharacter>(ALCAlienCharacter::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParams);
        if (Alien) { Alien->SetHealthMultiplier(HealthScale); Alien->SetDamageMultiplier(DamageScale); Alien->SetSpeedMultiplier(SpeedScale); }
    }

    SetGamePhase(EGamePhase::WaveCombat);
    OnWaveStarted.Broadcast(CurrentWave);
}

void ALCGameMode::SpawnBossGiant()
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;
    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn) return;

    FVector Loc = GetRandomSpawnLocation(PlayerPawn->GetActorLocation());
    ALCGiantBoss* Boss = GetWorld()->SpawnActor<ALCGiantBoss>(ALCGiantBoss::StaticClass(), Loc, FRotator::ZeroRotator, Params);
    if (Boss)
    {
        bBossAlive = true;
        OnBossSpawned.Broadcast(Boss);
    }
}

void ALCGameMode::HandleParachuting(float DeltaSeconds)
{
    // Check if player has landed
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;
    ALCPlayerCharacter* PlayerChar = Cast<ALCPlayerCharacter>(PC->GetPawn());
    if (PlayerChar && !PlayerChar->IsParachuting())
    {
        SetGamePhase(EGamePhase::WaveCombat);
        CurrentWave = 1;
        SpawnWaveEnemies();
    }
}

void ALCGameMode::HandleWaveCombat(float DeltaSeconds)
{
    PhaseTimer += DeltaSeconds;

    // Anti-camping: player stationary > 30s, spawn 3 extra enemies
    // Handled by PlayerCharacter position tracking
}

void ALCGameMode::HandleRestPeriod(float DeltaSeconds)
{
    RestTimer -= DeltaSeconds;
    if (ALCGameState* GS = Cast<ALCGameState>(GameState))
    {
        GS->SetRestTimeRemaining(RestTimer);
    }
    if (RestTimer <= 0.f)
    {
        CurrentWave++;
        if (CurrentWave > TOTAL_WAVES)
        {
            CheckVictoryConditions();
        }
        else
        {
            SpawnWaveEnemies();
        }
    }
}

void ALCGameMode::HandleBossFight(float DeltaSeconds)
{
    if (!bBossAlive)
    {
        OnBossKilled();
    }
}

void ALCGameMode::CheckVictoryConditions()
{
    if (CurrentWave >= TOTAL_WAVES && bBossKilledThisRun)
    {
        SetGamePhase(EGamePhase::Victory);
    }
    else if (CurrentWave >= TOTAL_WAVES && !bBossKilledThisRun)
    {
        // Spawn final boss
        SpawnBossGiant();
        SetGamePhase(EGamePhase::BossFight);
    }
}

FVector ALCGameMode::GetRandomSpawnLocation(const FVector& PlayerPos) const
{
    float Angle = FMath::RandRange(0.f, 360.f);
    float Dist  = FMath::RandRange(3000.f, 15000.f);
    FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
                   FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, 0.f);
    FVector Loc = PlayerPos + Offset;
    Loc.X = FMath::Clamp(Loc.X, -28000.f, 28000.f);
    Loc.Y = FMath::Clamp(Loc.Y, -28000.f, 28000.f);

    // Query terrain height from EnvironmentActor
    ALCEnvironmentActor* Env = Cast<ALCEnvironmentActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ALCEnvironmentActor::StaticClass()));
    if (Env)
    {
        Loc.Z = Env->GetHeightAt(Loc.X, Loc.Y) + 100.f; // Spawn 100 units above terrain
    }
    else
    {
        Loc.Z = 200.f; // Fallback
    }
    return Loc;
}

TArray<TSubclassOf<AActor>> ALCGameMode::GetEnemyClassesForWave(int32 Wave) const
{
    return TArray<TSubclassOf<AActor>>();
}
