#pragma once
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Actor.h"
#include "Characters/LCBaseCharacter.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LCAnimalSystem.generated.h"

UENUM(BlueprintType)
enum class EAnimalSpecies : uint8
{
    // Passive (8)
    Deer, Rabbit, Chicken, Cow, Sheep, Pig, Duck, Horse,
    // Aggressive (5)
    Wolf, Bear, Boar, Crocodile, Snake,
    // Flying (3)
    Eagle, Crow, Parrot,
    // Aquatic (3)
    Fish, Shark, Turtle
};

UENUM() enum class EAnimalBehavior : uint8 { Idle, Wander, Flee, Attack, Swim, Fly };

UCLASS()
class LAST_CIRCLE_2_API ALCAnimal : public ALCBaseCharacter
{
    GENERATED_BODY()
public:
    ALCAnimal()
    {
        PrimaryActorTick.bCanEverTick = true;
        Health = 50.f; MaxHealth = 50.f;
    }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BuildAnimalMesh();
        CurrentBehavior = bIsAggressive ? EAnimalBehavior::Wander : EAnimalBehavior::Idle;
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        if (bIsDead) return;
        BehaviorTimer -= DT;

        switch (CurrentBehavior)
        {
        case EAnimalBehavior::Idle:
            if (BehaviorTimer <= 0.f) { CurrentBehavior = EAnimalBehavior::Wander; BehaviorTimer = FMath::RandRange(3.f, 8.f); }
            DetectThreats(DT);
            break;
        case EAnimalBehavior::Wander:
            UpdateWander(DT);
            DetectThreats(DT);
            break;
        case EAnimalBehavior::Flee:
            UpdateFlee(DT);
            if (BehaviorTimer <= 0.f) { CurrentBehavior = EAnimalBehavior::Wander; }
            break;
        case EAnimalBehavior::Attack:
            UpdateAttack(DT);
            break;
        case EAnimalBehavior::Swim:
            UpdateSwim(DT);
            break;
        case EAnimalBehavior::Fly:
            UpdateFly(DT);
            break;
        }
    }

    UFUNCTION(BlueprintCallable) void SetSpecies(EAnimalSpecies S) { Species = S; ConfigureSpecies(); }
    UFUNCTION(BlueprintCallable) EAnimalSpecies GetSpecies() const { return Species; }

protected:
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* AnimalMesh = nullptr;
    UPROPERTY() EAnimalSpecies Species = EAnimalSpecies::Deer;
    UPROPERTY() EAnimalBehavior CurrentBehavior = EAnimalBehavior::Idle;
    UPROPERTY() bool bIsAggressive = false;
    UPROPERTY() bool bIsFlying = false;
    UPROPERTY() bool bIsAquatic = false;
    UPROPERTY() float BehaviorTimer = 5.f;
    UPROPERTY() FVector WanderTarget = FVector::ZeroVector;
    UPROPERTY() AActor* ThreatActor = nullptr;
    UPROPERTY() float AttackCooldown = 0.f;
    UPROPERTY() float AnimalDamage = 10.f;
    UPROPERTY() float DetectionRange = 300.f;
    UPROPERTY() float FleeSpeed = 500.f;

    void ConfigureSpecies()
    {
        switch (Species)
        {
        // Passive
        case EAnimalSpecies::Deer:    Health=80; MaxHealth=80; BaseSpeed=400; bIsAggressive=false; FleeSpeed=600; DetectionRange=400; break;
        case EAnimalSpecies::Rabbit:  Health=20; MaxHealth=20; BaseSpeed=350; bIsAggressive=false; FleeSpeed=700; DetectionRange=200; break;
        case EAnimalSpecies::Chicken: Health=15; MaxHealth=15; BaseSpeed=200; bIsAggressive=false; FleeSpeed=300; DetectionRange=150; break;
        case EAnimalSpecies::Cow:     Health=200;MaxHealth=200;BaseSpeed=250; bIsAggressive=false; FleeSpeed=350; DetectionRange=250; break;
        case EAnimalSpecies::Sheep:   Health=60; MaxHealth=60; BaseSpeed=300; bIsAggressive=false; FleeSpeed=400; DetectionRange=200; break;
        case EAnimalSpecies::Pig:     Health=70; MaxHealth=70; BaseSpeed=280; bIsAggressive=false; FleeSpeed=350; DetectionRange=180; break;
        case EAnimalSpecies::Duck:    Health=15; MaxHealth=15; BaseSpeed=200; bIsAggressive=false; bIsFlying=true; FleeSpeed=500; break;
        case EAnimalSpecies::Horse:   Health=250;MaxHealth=250;BaseSpeed=600; bIsAggressive=false; FleeSpeed=800; DetectionRange=350; break;
        // Aggressive
        case EAnimalSpecies::Wolf:     Health=100;MaxHealth=100;BaseSpeed=500; bIsAggressive=true; AnimalDamage=15; DetectionRange=500; break;
        case EAnimalSpecies::Bear:     Health=400;MaxHealth=400;BaseSpeed=350; bIsAggressive=true; AnimalDamage=40; DetectionRange=400; break;
        case EAnimalSpecies::Boar:     Health=120;MaxHealth=120;BaseSpeed=400; bIsAggressive=true; AnimalDamage=20; DetectionRange=300; break;
        case EAnimalSpecies::Crocodile:Health=300;MaxHealth=300;BaseSpeed=200; bIsAggressive=true; bIsAquatic=true; AnimalDamage=50; break;
        case EAnimalSpecies::Snake:    Health=30; MaxHealth=30; BaseSpeed=300; bIsAggressive=true; AnimalDamage=25; DetectionRange=150; break;
        // Flying
        case EAnimalSpecies::Eagle:  Health=40; MaxHealth=40; BaseSpeed=600; bIsFlying=true; bIsAggressive=false; break;
        case EAnimalSpecies::Crow:   Health=15; MaxHealth=15; BaseSpeed=500; bIsFlying=true; bIsAggressive=false; break;
        case EAnimalSpecies::Parrot: Health=20; MaxHealth=20; BaseSpeed=450; bIsFlying=true; bIsAggressive=false; break;
        // Aquatic
        case EAnimalSpecies::Fish:   Health=10; MaxHealth=10; BaseSpeed=400; bIsAquatic=true; bIsAggressive=false; break;
        case EAnimalSpecies::Shark:  Health=500;MaxHealth=500;BaseSpeed=500; bIsAquatic=true; bIsAggressive=true; AnimalDamage=60; break;
        case EAnimalSpecies::Turtle: Health=150;MaxHealth=150;BaseSpeed=100; bIsAquatic=true; bIsAggressive=false; break;
        }
        SetHealth(Health);
    }

    void DetectThreats(float DT)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;
        float PlayerDist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());

        if (PlayerDist < DetectionRange)
        {
            if (bIsAggressive)
            {
                ThreatActor = PC->GetPawn();
                CurrentBehavior = EAnimalBehavior::Attack;
            }
            else
            {
                ThreatActor = PC->GetPawn();
                CurrentBehavior = EAnimalBehavior::Flee;
                BehaviorTimer = FMath::RandRange(3.f, 6.f);
            }
        }
    }

    void UpdateWander(float DT)
    {
        if (WanderTarget == FVector::ZeroVector || FVector::Dist2D(GetActorLocation(), WanderTarget) < 50.f)
        {
            WanderTarget = GetActorLocation() + FVector(FMath::RandRange(-300.f, 300.f), FMath::RandRange(-300.f, 300.f), 0.f);
        }
        FVector Dir = (WanderTarget - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, BaseSpeed * 0.5f * DT);
        if (BehaviorTimer <= 0.f) { CurrentBehavior = EAnimalBehavior::Idle; BehaviorTimer = FMath::RandRange(2.f, 5.f); }
    }

    void UpdateFlee(float DT)
    {
        if (!ThreatActor) { CurrentBehavior = EAnimalBehavior::Wander; return; }
        FVector Dir = (GetActorLocation() - ThreatActor->GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, FleeSpeed * DT);
        if (FVector::Dist(GetActorLocation(), ThreatActor->GetActorLocation()) > DetectionRange * 2.f)
        {
            CurrentBehavior = EAnimalBehavior::Wander;
            ThreatActor = nullptr;
        }
    }

    void UpdateAttack(float DT)
    {
        if (!ThreatActor || !ThreatActor->IsValidLowLevel())
        {
            CurrentBehavior = EAnimalBehavior::Wander;
            ThreatActor = nullptr;
            return;
        }
        FVector Dir = (ThreatActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Dir, BaseSpeed * DT);
        FRotator LookAt = (ThreatActor->GetActorLocation() - GetActorLocation()).Rotation();
        SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));

        float Dist = FVector::Dist(GetActorLocation(), ThreatActor->GetActorLocation());
        AttackCooldown -= DT;
        if (Dist < 80.f && AttackCooldown <= 0.f)
        {
            FDamageEvent DmgEvent;
            ThreatActor->TakeDamage(AnimalDamage * DamageMultiplier, DmgEvent, GetController(), this);
            AttackCooldown = 1.5f;
        }
        if (Dist > DetectionRange * 2.f)
        {
            CurrentBehavior = EAnimalBehavior::Wander;
            ThreatActor = nullptr;
        }
    }

    void UpdateSwim(float DT)
    {
        // Simple circular swimming pattern
        float Time = GetWorld()->GetTimeSeconds();
        FVector Dir(FMath::Cos(Time * 0.5f), FMath::Sin(Time * 0.5f), 0.f);
        AddMovementInput(Dir, BaseSpeed * DT);
    }

    void UpdateFly(float DT)
    {
        float Time = GetWorld()->GetTimeSeconds();
        FVector Dir(FMath::Cos(Time * 0.3f), FMath::Sin(Time * 0.4f), FMath::Sin(Time * 0.2f) * 0.3f);
        Dir.Normalize();
        AddMovementInput(Dir, BaseSpeed * DT);
        // Keep altitude
        FVector Loc = GetActorLocation();
        if (Loc.Z < 100.f) Loc.Z += 50.f * DT;
        if (Loc.Z > 300.f) Loc.Z -= 50.f * DT;
        SetActorLocation(Loc);
    }

    void BuildAnimalMesh()
    {
        if (!BodyMesh) return;
        BodyMesh->ClearAllMeshSections();

        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

        // Generic quadruped body
        float Scale = 1.f;
        FColor BodyColor = FColor(150, 100, 60);
        switch (Species)
        {
        case EAnimalSpecies::Deer: Scale=1.2f; BodyColor=FColor(160,120,60); break;
        case EAnimalSpecies::Rabbit: Scale=0.3f; BodyColor=FColor(180,160,140); break;
        case EAnimalSpecies::Bear: Scale=2.f; BodyColor=FColor(80,50,20); break;
        case EAnimalSpecies::Wolf: Scale=1.f; BodyColor=FColor(100,100,100); break;
        case EAnimalSpecies::Chicken: Scale=0.3f; BodyColor=FColor(230,220,200); break;
        case EAnimalSpecies::Cow: Scale=1.8f; BodyColor=FColor(200,200,200); break;
        case EAnimalSpecies::Shark: Scale=2.5f; BodyColor=FColor(80,80,100); break;
        case EAnimalSpecies::Eagle: Scale=0.5f; BodyColor=FColor(60,40,20); break;
        default: break;
        }

        // Body (horizontal box)
        float BX = 12.f * Scale, BY = 6.f * Scale, BZ = 8.f * Scale;
        V.Add(FVector(-BX,-BY,BZ)); V.Add(FVector(BX,-BY,BZ)); V.Add(FVector(BX,BY,BZ)); V.Add(FVector(-BX,BY,BZ));
        V.Add(FVector(-BX,-BY,BZ*2)); V.Add(FVector(BX,-BY,BZ*2)); V.Add(FVector(BX,BY,BZ*2)); V.Add(FVector(-BX,BY,BZ*2));
        for (int32 i = 0; i < 8; ++i) { C.Add(BodyColor); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        T.Add(3);T.Add(0);T.Add(4);T.Add(3);T.Add(4);T.Add(7);
        T.Add(1);T.Add(2);T.Add(6);T.Add(1);T.Add(6);T.Add(5);

        // 4 Legs
        for (int32 leg = 0; leg < 4; ++leg)
        {
            float LX = (leg < 2 ? -1.f : 1.f) * BX * 0.7f;
            float LY = (leg % 2 == 0 ? -1.f : 1.f) * BY * 0.7f;
            float LegH = BZ;
            int32 LB = V.Num();
            V.Add(FVector(LX-1.5f*Scale, LY-1.5f*Scale, 0)); V.Add(FVector(LX+1.5f*Scale, LY-1.5f*Scale, 0));
            V.Add(FVector(LX+1.5f*Scale, LY+1.5f*Scale, 0)); V.Add(FVector(LX-1.5f*Scale, LY+1.5f*Scale, 0));
            V.Add(FVector(LX-1.5f*Scale, LY-1.5f*Scale, LegH)); V.Add(FVector(LX+1.5f*Scale, LY-1.5f*Scale, LegH));
            V.Add(FVector(LX+1.5f*Scale, LY+1.5f*Scale, LegH)); V.Add(FVector(LX-1.5f*Scale, LY+1.5f*Scale, LegH));
            FColor LegCol = (FLinearColor(BodyColor) * 0.8f).ToFColor(false);
            for (int32 i = 0; i < 8; ++i) { C.Add(LegCol); UV.Add(FVector2D::ZeroVector); }
            T.Add(LB);T.Add(LB+1);T.Add(LB+5);T.Add(LB);T.Add(LB+5);T.Add(LB+4);
            T.Add(LB+2);T.Add(LB+3);T.Add(LB+7);T.Add(LB+2);T.Add(LB+7);T.Add(LB+6);
            T.Add(LB+4);T.Add(LB+5);T.Add(LB+6);T.Add(LB+4);T.Add(LB+6);T.Add(LB+7);
            T.Add(LB);T.Add(LB+3);T.Add(LB+2);T.Add(LB);T.Add(LB+2);T.Add(LB+1);
        }

        // Head
        float HR = 4.f * Scale;
        FVector HeadCenter(BX + HR, 0.f, BZ * 1.5f);
        int32 HB = V.Num();
        for (int32 ring = 0; ring < 4; ++ring)
        {
            float Phi = PI * ring / 4;
            for (int32 seg = 0; seg < 6; ++seg)
            {
                float Theta = 2.f * PI * seg / 6;
                V.Add(HeadCenter + FVector(HR * FMath::Sin(Phi) * FMath::Cos(Theta), HR * FMath::Sin(Phi) * FMath::Sin(Theta), HR * FMath::Cos(Phi)));
                C.Add(BodyColor); UV.Add(FVector2D::ZeroVector);
            }
        }
        for (int32 ring = 0; ring < 3; ++ring)
        {
            for (int32 seg = 0; seg < 6; ++seg)
            {
                int32 Curr = HB + ring * 6 + seg;
                int32 Next = HB + ring * 6 + (seg + 1) % 6;
                int32 Below = HB + (ring + 1) * 6 + seg;
                int32 BelowNext = HB + (ring + 1) * 6 + (seg + 1) % 6;
                T.Add(Curr); T.Add(Below); T.Add(Next);
                T.Add(Next); T.Add(Below); T.Add(BelowNext);
            }
        }

        // Wings for flying animals
        if (bIsFlying)
        {
            for (int32 side = -1; side <= 1; side += 2)
            {
                int32 WB = V.Num();
                float WingSpan = BX * 3.f;
                V.Add(FVector(-BX*0.3f, side*BY, BZ*1.8f)); V.Add(FVector(BX*0.3f, side*BY, BZ*1.8f));
                V.Add(FVector(0.f, side*WingSpan, BZ*2.f));
                FColor WingCol = (FLinearColor(BodyColor) * 0.9f).ToFColor(false);
                for (int32 i = 0; i < 3; ++i) { C.Add(WingCol); UV.Add(FVector2D::ZeroVector); }
                T.Add(WB); T.Add(WB+1); T.Add(WB+2);
            }
        }

        BodyMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};

// ===================== Animal Spawner Manager =====================
UCLASS()
class LAST_CIRCLE_2_API ALCAnimalSpawner : public AActor
{
    GENERATED_BODY()
public:
    ALCAnimalSpawner() { PrimaryActorTick.bCanEverTick = false; }
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        SpawnAllAnimals();
    }

    UFUNCTION(BlueprintCallable) int32 GetTotalAnimalCount() const { return SpawnedAnimals.Num(); }
protected:
    UPROPERTY() TArray<ALCAnimal*> SpawnedAnimals;

    void SpawnAllAnimals()
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Passive animals (various counts)
        SpawnSpecies(EAnimalSpecies::Deer, 12, Params);
        SpawnSpecies(EAnimalSpecies::Rabbit, 20, Params);
        SpawnSpecies(EAnimalSpecies::Chicken, 15, Params);
        SpawnSpecies(EAnimalSpecies::Cow, 8, Params);
        SpawnSpecies(EAnimalSpecies::Sheep, 10, Params);
        SpawnSpecies(EAnimalSpecies::Pig, 8, Params);
        SpawnSpecies(EAnimalSpecies::Duck, 12, Params);
        SpawnSpecies(EAnimalSpecies::Horse, 6, Params);

        // Aggressive animals
        SpawnSpecies(EAnimalSpecies::Wolf, 8, Params);
        SpawnSpecies(EAnimalSpecies::Bear, 4, Params);
        SpawnSpecies(EAnimalSpecies::Boar, 6, Params);
        SpawnSpecies(EAnimalSpecies::Crocodile, 3, Params);
        SpawnSpecies(EAnimalSpecies::Snake, 10, Params);

        // Flying
        SpawnSpecies(EAnimalSpecies::Eagle, 5, Params);
        SpawnSpecies(EAnimalSpecies::Crow, 15, Params);
        SpawnSpecies(EAnimalSpecies::Parrot, 8, Params);

        // Aquatic
        SpawnSpecies(EAnimalSpecies::Fish, 20, Params);
        SpawnSpecies(EAnimalSpecies::Shark, 3, Params);
        SpawnSpecies(EAnimalSpecies::Turtle, 5, Params);
    }

    void SpawnSpecies(EAnimalSpecies Species, int32 Count, const FActorSpawnParameters& Params)
    {
        for (int32 i = 0; i < Count; ++i)
        {
            FVector SpawnLoc(FMath::RandRange(-2800.f, 2800.f), FMath::RandRange(-2800.f, 2800.f), 0.f);
            // Aquatic animals spawn near water (low Z)
            if (Species == EAnimalSpecies::Fish || Species == EAnimalSpecies::Shark || Species == EAnimalSpecies::Turtle || Species == EAnimalSpecies::Crocodile)
            {
                SpawnLoc.Z = -5.f;
            }
            // Flying animals spawn higher
            if (Species == EAnimalSpecies::Eagle || Species == EAnimalSpecies::Crow || Species == EAnimalSpecies::Parrot || Species == EAnimalSpecies::Duck)
            {
                SpawnLoc.Z = FMath::RandRange(80.f, 250.f);
            }

            ALCAnimal* Animal = GetWorld()->SpawnActor<ALCAnimal>(ALCAnimal::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);
            if (Animal)
            {
                Animal->SetSpecies(Species);
                SpawnedAnimals.Add(Animal);
            }
        }
    }
};
