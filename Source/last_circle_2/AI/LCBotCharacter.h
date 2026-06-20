#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "Characters/LCBaseCharacter.h"
#include "Characters/LCPlayerCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "DrawDebugHelpers.h"
#include "LCBotCharacter.generated.h"

UENUM() enum class EBotState : uint8 { Wander, Attack, Dance };

UCLASS()
class LAST_CIRCLE_2_API ALCBotCharacter : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCBotCharacter()
    {
        PrimaryActorTick.bCanEverTick = true;
        AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
        Health = 300.f; MaxHealth = 300.f;
        BaseSpeed = 400.f;
        GetCapsuleComponent()->SetCapsuleSize(50.f, 110.f);
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        FColor Skin(FMath::RandRange(100,220), FMath::RandRange(100,220), FMath::RandRange(100,220));
        FColor Cloth(FMath::RandRange(40,200), FMath::RandRange(40,200), FMath::RandRange(40,200));
        BuildProceduralBody(Skin, Cloth);
        ApplyVertexColorMaterial();
        GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
        BodyMesh->SetWorldScale3D(FVector(2.2f));
        HeadMesh->SetWorldScale3D(FVector(2.2f));
        HelmetLevel = static_cast<EEquipmentLevel>(FMath::RandRange(0, 3));
        ArmorLevel = static_cast<EEquipmentLevel>(FMath::RandRange(0, 3));
        CurrentState = EBotState::Wander;
        WanderTarget = GetActorLocation() + FVector(FMath::RandRange(-500.f,500.f), FMath::RandRange(-500.f,500.f), 0.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsDead) return;
        switch (CurrentState)
        {
        case EBotState::Wander: UpdateWander(DeltaSeconds); break;
        case EBotState::Attack: UpdateAttack(DeltaSeconds); break;
        case EBotState::Dance: break;
        }
    }
    UFUNCTION(BlueprintCallable) EBotState GetBotState() const { return CurrentState; }
    UFUNCTION(BlueprintCallable) void SetBotState(EBotState State) { CurrentState = State; }
protected:
    UPROPERTY() EBotState CurrentState = EBotState::Wander;
    UPROPERTY() FVector WanderTarget = FVector::ZeroVector;
    UPROPERTY() float AttackCooldown = 0.f;
    UPROPERTY() AActor* TargetActor = nullptr;

    ALCPlayerCharacter* FindPlayer() const
    {
        for (TActorIterator<ALCPlayerCharacter> It(GetWorld()); It; ++It)
        {
            if (It->IsAlive()) return *It;
        }
        return nullptr;
    }

    void UpdateWander(float DT)
    {
        FVector Dir = (WanderTarget - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, 0.5f);
        float Dist = FVector::Dist2D(GetActorLocation(), WanderTarget);
        if (Dist < 80.f)
        {
            WanderTarget = GetActorLocation() + FVector(FMath::RandRange(-800.f,800.f), FMath::RandRange(-800.f,800.f), 0.f);
        }
        if (ALCPlayerCharacter* Player = FindPlayer())
        {
            float DistToPlayer = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
            if (DistToPlayer < 3000.f)
            {
                TargetActor = Player;
                CurrentState = EBotState::Attack;
            }
        }
    }

    void UpdateAttack(float DT)
    {
        if (!TargetActor || !Cast<ALCBaseCharacter>(TargetActor)->IsAlive())
        {
            CurrentState = EBotState::Wander;
            TargetActor = nullptr;
            return;
        }
        FRotator LookAt = (TargetActor->GetActorLocation() - GetActorLocation()).Rotation();
        SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));
        float Dist = FVector::Dist2D(GetActorLocation(), TargetActor->GetActorLocation());

        // Only approach if too far, STOP when in attack range (don't come close!)
        if (Dist > 2500.f)
        {
            FVector Dir = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            AddMovementInput(Dir, 1.f);
        }

        // Attack from distance (2500 range)
        AttackCooldown -= DT;
        if (Dist < 2500.f && AttackCooldown <= 0.f)
        {
            float Dmg = 15.f;
            TargetActor->TakeDamage(Dmg, FDamageEvent(), nullptr, this);
            AttackCooldown = 0.6f;

            // Bullet tracer line from enemy to player
            FVector Start = GetActorLocation() + FVector(0.f, 0.f, 80.f);
            FVector End = TargetActor->GetActorLocation() + FVector(0.f, 0.f, 60.f);
            DrawDebugLine(GetWorld(), Start, End, FColor(255, 80, 0), false, 0.15f, 0, 2.0f);
            DrawDebugSphere(GetWorld(), End, 5.f, 6, FColor(255, 50, 0), false, 0.15f);

            // Gunshot sound
            PlayGunshotSound();
        }
    }

    void PlayGunshotSound()
    {
        // Must create fresh sound wave each time - procedural audio data is consumed on play
        USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(this);
        Sound->SetSampleRate(44100);
        Sound->NumChannels = 1;
        Sound->Duration = 0.18f;
        int32 NumSamples = FMath::CeilToInt(44100.f * 0.18f);
        TArray<uint8> AudioData;
        AudioData.SetNumUninitialized(NumSamples * 2);
        int16* Samples = reinterpret_cast<int16*>(AudioData.GetData());
        for (int32 i = 0; i < NumSamples; ++i)
        {
            float T = (float)i / 44100.f;
            float Env = FMath::Exp(-T * 25.f);
            float Noise = FMath::FRandRange(-1.f, 1.f);
            float Bass = FMath::Sin(T * 150.f * 2.f * PI) * 0.5f;
            float Val = (Noise * 0.4f + Bass) * Env;
            Samples[i] = (int16)(FMath::Clamp(Val, -1.f, 1.f) * 30000.f);
        }
        Sound->QueueAudio(AudioData.GetData(), AudioData.Num());
        UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 3.f);
    }
};
