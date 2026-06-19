#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ProceduralMeshComponent.h"
#include "LCVehicleBase.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ALCVehicleBase : public APawn
{
    GENERATED_BODY()
public:
    ALCVehicleBase()
    {
        PrimaryActorTick.bCanEverTick = true;
        Tags.Add(FName("Vehicle"));
        VehicleMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VehicleMesh"));
        RootComponent = VehicleMesh;
        VehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    virtual void Tick(float DeltaSeconds) override
    {
        Super::Tick(DeltaSeconds);
        if (bIsBeingDriven)
        {
            // Simple vehicle physics
            Speed += ThrottleInput * Acceleration * DeltaSeconds;
            Speed *= (1.f - 0.5f * DeltaSeconds); // Friction
            Speed = FMath::Clamp(Speed, -MaxSpeed * 0.3f, MaxSpeed);

            FRotator YawRot(0.f, GetActorRotation().Yaw + SteeringInput * SteeringSpeed * DeltaSeconds, 0.f);
            SetActorRotation(YawRot);

            FVector Forward = GetActorForwardVector();
            AddActorWorldOffset(Forward * Speed * DeltaSeconds);
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetDriverInput(float Throttle, float Steering)
    {
        ThrottleInput = Throttle;
        SteeringInput = Steering;
    }

    UFUNCTION(BlueprintCallable)
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintCallable)
    void TakeVehicleDamage(float Damage)
    {
        Health -= Damage;
        if (Health <= 0.f && !bDestroyed)
        {
            bDestroyed = true;
            // Explode and eject driver
        }
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxSpeed = 1300.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Acceleration = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SteeringSpeed = 100.f;
    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* VehicleMesh = nullptr;
    UPROPERTY() bool bIsBeingDriven = false;
    UPROPERTY() float Health = 300.f;
    UPROPERTY() bool bDestroyed = false;
protected:
    UPROPERTY() float Speed = 0.f;
    UPROPERTY() float ThrottleInput = 0.f;
    UPROPERTY() float SteeringInput = 0.f;
};

// ===================== Jeep =====================
UCLASS()
class LAST_CIRCLE_2_API ALCJeep : public ALCVehicleBase
{
    GENERATED_BODY()
public:
    ALCJeep()
    {
        MaxSpeed = 1300.f; Acceleration = 220.f; SteeringSpeed = 80.f; Health = 300.f;
        BuildJeepMesh();
    }
protected:
    void BuildJeepMesh()
    {
        if (!VehicleMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // Body
        float HX = 80.f, HY = 40.f, HZ = 25.f;
        V.Add(FVector(-HX,-HY,10)); V.Add(FVector(HX,-HY,10)); V.Add(FVector(HX,HY,10)); V.Add(FVector(-HX,HY,10));
        V.Add(FVector(-HX,-HY,10+HZ)); V.Add(FVector(HX,-HY,10+HZ)); V.Add(FVector(HX,HY,10+HZ)); V.Add(FVector(-HX,HY,10+HZ));
        for (int32 i = 0; i < 8; ++i) { C.Add(FColor(60, 80, 60)); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        VehicleMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};

// ===================== Motorcycle =====================
UCLASS()
class LAST_CIRCLE_2_API ALCMotorcycle : public ALCVehicleBase
{
    GENERATED_BODY()
public:
    ALCMotorcycle()
    {
        MaxSpeed = 1700.f; Acceleration = 320.f; SteeringSpeed = 130.f; Health = 150.f;
        BuildBikeMesh();
    }
protected:
    void BuildBikeMesh()
    {
        if (!VehicleMesh) return;
        TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
        TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
        // Slim body
        float HX = 50.f, HY = 10.f, HZ = 15.f;
        V.Add(FVector(-HX,-HY,10)); V.Add(FVector(HX,-HY,10)); V.Add(FVector(HX,HY,10)); V.Add(FVector(-HX,HY,10));
        V.Add(FVector(-HX,-HY,10+HZ)); V.Add(FVector(HX,-HY,10+HZ)); V.Add(FVector(HX,HY,10+HZ)); V.Add(FVector(-HX,HY,10+HZ));
        for (int32 i = 0; i < 8; ++i) { C.Add(FColor(40, 40, 50)); UV.Add(FVector2D::ZeroVector); }
        T.Add(0);T.Add(1);T.Add(5);T.Add(0);T.Add(5);T.Add(4);
        T.Add(2);T.Add(3);T.Add(7);T.Add(2);T.Add(7);T.Add(6);
        T.Add(4);T.Add(5);T.Add(6);T.Add(4);T.Add(6);T.Add(7);
        T.Add(0);T.Add(3);T.Add(2);T.Add(0);T.Add(2);T.Add(1);
        VehicleMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
    }
};
