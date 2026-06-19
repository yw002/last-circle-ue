#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "LCUFOActor.generated.h"

UENUM() enum class EUFOState : uint8 { Hovering, Descending, Releasing, Ascending, Moving };

UCLASS()
class LAST_CIRCLE_2_API ALCUFOActor : public AActor
{
    GENERATED_BODY()
public:
    ALCUFOActor()
    {
        PrimaryActorTick.bCanEverTick = true;
        UFOMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("UFOMesh"));
        RootComponent = UFOMesh;
        BuildUFOMesh();
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        CurrentState = EUFOState::Hovering;
        StateTimer = FMath::RandRange(5.f, 15.f);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        StateTimer -= DeltaSeconds;
        if (StateTimer <= 0.f)
        {
            switch (CurrentState)
            {
            case EUFOState::Hovering:
                CurrentState = EUFOState::Descending; StateTimer = 5.f; break;
            case EUFOState::Descending:
                CurrentState = EUFOState::Releasing; StateTimer = 3.f; break;
            case EUFOState::Releasing:
                CurrentState = EUFOState::Ascending; StateTimer = 5.f; break;
            case EUFOState::Ascending:
                CurrentState = EUFOState::Moving; StateTimer = FMath::RandRange(10.f, 20.f); break;
            case EUFOState::Moving:
                CurrentState = EUFOState::Hovering; StateTimer = FMath::RandRange(5.f, 15.f); break;
            }
        }

        switch (CurrentState)
        {
        case EUFOState::Hovering:
            // Bob in place
            break;
        case EUFOState::Descending:
            AddActorWorldOffset(FVector(0, 0, -50.f * DeltaSeconds)); break;
        case EUFOState::Releasing:
            // Spawn aliens (handled elsewhere)
            break;
        case EUFOState::Ascending:
            AddActorWorldOffset(FVector(0, 0, 50.f * DeltaSeconds)); break;
        case EUFOState::Moving:
            {
                FVector MoveDir(FMath::RandRange(-1.f,1.f), FMath::RandRange(-1.f,1.f), 0.f);
                MoveDir.Normalize();
                AddActorWorldOffset(MoveDir * 200.f * DeltaSeconds);
            }
            break;
        }

        // Always spin
        AddActorWorldRotation(FRotator(0.f, 30.f * DeltaSeconds, 0.f));
    }
protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* UFOMesh = nullptr;
    UPROPERTY() EUFOState CurrentState = EUFOState::Hovering;
    UPROPERTY() float StateTimer = 10.f;
    UPROPERTY() int32 AliensReleased = 0;

    void BuildUFOMesh()
    {
        if (!UFOMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        // Disk shape
        float R = 30.f, H = 8.f;
        int32 Segs = 16;
        // Top ring
        for (int32 i = 0; i < Segs; ++i)
        {
            float A = 2.f * PI * i / Segs;
            V.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f));
            V.Add(FVector(R * 0.3f * FMath::Cos(A), R * 0.3f * FMath::Sin(A), H));
            C.Add(FColor(150, 150, 180)); C.Add(FColor(200, 200, 220));
            UV.Add(FVector2D::ZeroVector); UV.Add(FVector2D::ZeroVector);
        }
        for (int32 i = 0; i < Segs; ++i)
        {
            int32 B = i * 2;
            int32 NB = ((i + 1) % Segs) * 2;
            T.Add(B); T.Add(NB); T.Add(B + 1);
            T.Add(B + 1); T.Add(NB); T.Add(NB + 1);
        }

        UFOMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};
