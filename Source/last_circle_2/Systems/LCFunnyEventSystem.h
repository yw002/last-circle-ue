#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/LCBaseCharacter.h"
#include "LCFunnyEventSystem.generated.h"

UENUM(BlueprintType)
enum class EFunnyEventType : uint8
{
    DanceParty UMETA(DisplayName = "Dance Party"),
    FallingFish UMETA(DisplayName = "Falling Fish"),
    GiantFart   UMETA(DisplayName = "Giant Fart"),
    Delivery    UMETA(DisplayName = "Delivery")
};

UCLASS()
class LAST_CIRCLE_2_API ALCFunnyEventSystem : public AActor
{
    GENERATED_BODY()
public:
    ALCFunnyEventSystem() { PrimaryActorTick.bCanEverTick = true; }
    virtual void BeginPlay() override { Super::BeginPlay(); EventTimer = FMath::RandRange(30.f, 60.f); }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        EventTimer -= DeltaSeconds;
        if (EventTimer <= 0.f)
        {
            TriggerRandomEvent();
            EventTimer = FMath::RandRange(45.f, 90.f);
        }
        // Active event updates
        if (ActiveEvent == EFunnyEventType::DanceParty && EventDuration > 0.f)
        {
            EventDuration -= DeltaSeconds;
            UpdateDanceParty(DeltaSeconds);
        }
        if (ActiveEvent == EFunnyEventType::GiantFart && EventDuration > 0.f)
        {
            EventDuration -= DeltaSeconds;
            UpdateGiantFart(DeltaSeconds);
        }
    }
    UFUNCTION(BlueprintCallable) EFunnyEventType GetLastEvent() const { return LastEvent; }
    UFUNCTION(BlueprintCallable) FString GetEventName() const
    {
        switch (LastEvent)
        {
        case EFunnyEventType::DanceParty: return TEXT("Dance Party!");
        case EFunnyEventType::FallingFish: return TEXT("It's Raining Fish!");
        case EFunnyEventType::GiantFart: return TEXT("Giant Fart!");
        case EFunnyEventType::Delivery: return TEXT("Pizza Delivery!");
        default: return TEXT("");
        }
    }
    UFUNCTION(BlueprintCallable) float GetTimeToNextEvent() const { return EventTimer; }
protected:
    UPROPERTY() float EventTimer = 45.f;
    UPROPERTY() EFunnyEventType LastEvent = EFunnyEventType::DanceParty;
    UPROPERTY() EFunnyEventType ActiveEvent = EFunnyEventType::DanceParty;
    UPROPERTY() float EventDuration = 0.f;
    UPROPERTY() FVector FartLocation = FVector::ZeroVector;

    void TriggerRandomEvent()
    {
        int32 EventIdx = FMath::RandRange(0, 3);
        LastEvent = static_cast<EFunnyEventType>(EventIdx);
        ActiveEvent = LastEvent;

        switch (LastEvent)
        {
        case EFunnyEventType::DanceParty: StartDanceParty(); break;
        case EFunnyEventType::FallingFish: SpawnFallingFish(); break;
        case EFunnyEventType::GiantFart: StartGiantFart(); break;
        case EFunnyEventType::Delivery: SpawnDelivery(); break;
        }
    }

    void StartDanceParty()
    {
        EventDuration = 8.f;
        // All nearby bots start dancing (handled in UpdateDanceParty)
    }
    void UpdateDanceParty(float DT)
    {
        // Make nearby bots spin/dance
        TArray<FHitResult> Hits;
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;
        FVector Center = PC->GetPawn()->GetActorLocation();
        FCollisionShape Sphere = FCollisionShape::MakeSphere(300.f);
        GetWorld()->SweepMultiByChannel(Hits, Center, Center, FQuat::Identity, ECC_Pawn, Sphere);
        for (const FHitResult& H : Hits)
        {
            if (ALCBaseCharacter* Char = Cast<ALCBaseCharacter>(H.GetActor()))
            {
                // Spin the dancing characters
                Char->AddActorWorldRotation(FRotator(0.f, 200.f * DT, 0.f));
            }
        }
    }

    void SpawnFallingFish()
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;
        FVector Center = PC->GetPawn()->GetActorLocation();

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Spawn 10-20 fish that fall from the sky
        for (int32 i = 0; i < FMath::RandRange(10, 20); ++i)
        {
            FVector SpawnLoc = Center + FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 200.f + FMath::RandRange(0.f, 100.f));
            // Create a simple fish mesh actor that falls
            AActor* Fish = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);
            if (Fish)
            {
                // Add procedural mesh for fish
                UProceduralMeshComponent* FishMesh = NewObject<UProceduralMeshComponent>(Fish);
                FishMesh->RegisterComponent();
                Fish->SetRootComponent(FishMesh);
                FishMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                // Simple fish shape
                TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
                TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
                V.Add(FVector(5,0,0)); V.Add(FVector(-5,0,0)); V.Add(FVector(0,2,0)); V.Add(FVector(0,-2,0));
                V.Add(FVector(0,0,1)); V.Add(FVector(0,0,-1));
                FColor FishCol(100, 150, 200);
                for (int32 j = 0; j < 6; ++j) { C.Add(FishCol); UV.Add(FVector2D::ZeroVector); }
                T.Add(0);T.Add(2);T.Add(4); T.Add(0);T.Add(4);T.Add(3);
                T.Add(0);T.Add(3);T.Add(5); T.Add(0);T.Add(5);T.Add(2);
                T.Add(1);T.Add(4);T.Add(2); T.Add(1);T.Add(3);T.Add(4);
                T.Add(1);T.Add(5);T.Add(3); T.Add(1);T.Add(2);T.Add(5);
                FishMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
                Fish->SetLifeSpan(3.f);
            }
        }
    }

    void StartGiantFart()
    {
        EventDuration = 4.f;
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC && PC->GetPawn())
        {
            FartLocation = PC->GetPawn()->GetActorLocation();
        }
    }
    void UpdateGiantFart(float DT)
    {
        // Push everything away from the fart location
        TArray<FHitResult> Hits;
        float Radius = 200.f * (4.f - EventDuration);
        FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
        GetWorld()->SweepMultiByChannel(Hits, FartLocation, FartLocation, FQuat::Identity, ECC_Pawn, Sphere);
        for (const FHitResult& H : Hits)
        {
            if (APawn* P = Cast<APawn>(H.GetActor()))
            {
                FVector PushDir = (P->GetActorLocation() - FartLocation).GetSafeNormal();
                PushDir.Z = 0.3f;
                P->AddMovementInput(PushDir, 500.f * DT);
            }
        }
    }

    void SpawnDelivery()
    {
        // Drop a pizza box near the player that heals
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;
        FVector DropLoc = PC->GetPawn()->GetActorLocation() + FVector(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), 0.f);
        // Heal the player as a "delivery" bonus
        if (ALCBaseCharacter* Char = Cast<ALCBaseCharacter>(PC->GetPawn()))
        {
            Char->AddHealth(100.f);
        }
    }
};
