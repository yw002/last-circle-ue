#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Effects/LCEffectsSystem.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "LCDisasterSystem.generated.h"

UENUM(BlueprintType)
enum class EDisasterType : uint8
{
    None, Meteor, Tornado, VolcanoEarthquake
};

UCLASS()
class LAST_CIRCLE_2_API ALCDisasterSystem : public AActor
{
    GENERATED_BODY()
public:
    ALCDisasterSystem() { PrimaryActorTick.bCanEverTick = true; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        MeteorTimer = FMath::RandRange(15.f, 30.f);
        TornadoTimer = FMath::RandRange(60.f, 120.f);
        EarthquakeTimer = FMath::RandRange(90.f, 180.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);

        // Meteor shower
        MeteorTimer -= DeltaSeconds;
        if (MeteorTimer <= 0.f)
        {
            SpawnMeteorShower();
            MeteorTimer = FMath::RandRange(20.f, 40.f);
        }

        // Tornado
        TornadoTimer -= DeltaSeconds;
        if (TornadoTimer <= 0.f && ActiveTornadoes < 2)
        {
            SpawnTornado();
            TornadoTimer = FMath::RandRange(90.f, 180.f);
        }

        // Earthquake
        EarthquakeTimer -= DeltaSeconds;
        if (EarthquakeTimer <= 0.f)
        {
            TriggerEarthquake();
            EarthquakeTimer = FMath::RandRange(120.f, 240.f);
        }
    }

    UFUNCTION(BlueprintCallable) EDisasterType GetLastDisaster() const { return LastDisaster; }
    UFUNCTION(BlueprintCallable) float GetMeteorTimer() const { return MeteorTimer; }
    UFUNCTION(BlueprintCallable) float GetTornadoTimer() const { return TornadoTimer; }

protected:
    UPROPERTY() float MeteorTimer = 20.f;
    UPROPERTY() float TornadoTimer = 90.f;
    UPROPERTY() float EarthquakeTimer = 150.f;
    UPROPERTY() int32 ActiveTornadoes = 0;
    UPROPERTY() EDisasterType LastDisaster = EDisasterType::None;
    UPROPERTY() float EarthquakeDuration = 0.f;
    UPROPERTY() float EarthquakeIntensity = 0.f;

    void SpawnMeteorShower()
    {
        LastDisaster = EDisasterType::Meteor;
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;

        FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
        int32 Count = FMath::RandRange(3, 7);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        for (int32 i = 0; i < Count; ++i)
        {
            FVector TargetLoc = PlayerLoc + FVector(FMath::RandRange(-400.f, 400.f), FMath::RandRange(-400.f, 400.f), 0.f);
            FVector SpawnLoc = TargetLoc + FVector(FMath::RandRange(-100.f, 100.f), FMath::RandRange(-100.f, 100.f), 500.f + FMath::RandRange(0.f, 200.f));

            ALCMeteorActor* Meteor = GetWorld()->SpawnActor<ALCMeteorActor>(ALCMeteorActor::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);
            if (Meteor)
            {
                Meteor->SetTargetLocation(TargetLoc);
                Meteor->SetFallSpeed(FMath::RandRange(600.f, 1200.f));
            }
        }
    }

    void SpawnTornado()
    {
        LastDisaster = EDisasterType::Tornado;
        ActiveTornadoes++;

        FVector SpawnLoc(FMath::RandRange(-2500.f, 2500.f), FMath::RandRange(-2500.f, 2500.f), 0.f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ALCTornadoActor* Tornado = GetWorld()->SpawnActor<ALCTornadoActor>(ALCTornadoActor::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);
        if (Tornado)
        {
            Tornado->SetMoveDirection(FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal());
            // Decrement count when tornado expires
            FTimerHandle Handle;
            GetWorldTimerManager().SetTimer(Handle, [this]() { ActiveTornadoes = FMath::Max(0, ActiveTornadoes - 1); }, 30.f, false);
        }
    }

    void TriggerEarthquake()
    {
        LastDisaster = EDisasterType::VolcanoEarthquake;
        EarthquakeDuration = 5.f;
        EarthquakeIntensity = FMath::FRandRange(0.5f, 1.5f);

        // Damage all entities in range
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;

        FVector Center = PC->GetPawn()->GetActorLocation();
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(500.f);
        GetWorld()->SweepMultiByChannel(Hits, Center, Center, FQuat::Identity, ECC_Pawn, Sphere);
        for (const FHitResult& H : Hits)
        {
            if (AActor* A = H.GetActor())
            {
                float Dmg = 20.f * EarthquakeIntensity;
                FDamageEvent DmgEvent;
                A->TakeDamage(Dmg, DmgEvent, nullptr, this);
            }
        }
    }
};
