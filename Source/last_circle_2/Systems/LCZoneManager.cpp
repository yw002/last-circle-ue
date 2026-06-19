#include "Systems/LCZoneManager.h"
#include "ProceduralMeshComponent.h"
#include "Core/LCGameState.h"
#include "Characters/LCBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void ALCZoneManager::BeginPlay()
{
    Super::BeginPlay();
    ZoneRadius = 3000.f;
    ZoneCenter = FVector::ZeroVector;

    ZoneMesh = NewObject<UProceduralMeshComponent>(this, TEXT("ZoneMesh"));
    ZoneMesh->RegisterComponent();
    ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UpdateZoneVisual();
}

void ALCZoneManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ShrinkTimer -= DeltaSeconds;
    if (ShrinkTimer <= 0.f)
    {
        ShrinkZone();
        ShrinkTimer = 60.f;
    }

    // Damage entities outside zone
    for (TActorIterator<ALCBaseCharacter> It(GetWorld()); It; ++It)
    {
        if (It->IsAlive() && IsOutsideZone(It->GetActorLocation()))
        {
            It->SetHealth(It->GetHealth() - GetZoneDamage() * DeltaSeconds);
            if (It->GetHealth() <= 0.f) It->Die(nullptr);
        }
    }
}

bool ALCZoneManager::IsOutsideZone(const FVector& Location) const
{
    float Dist2D = FVector::Dist2D(Location, ZoneCenter);
    return Dist2D > ZoneRadius;
}

void ALCZoneManager::ShrinkZone()
{
    ZoneRadius *= 0.6f;
    // Random offset
    float Offset = FMath::RandRange(0.f, ZoneRadius * 0.3f);
    float Angle = FMath::RandRange(0.f, 360.f);
    ZoneCenter += FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Offset,
                          FMath::Sin(FMath::DegreesToRadians(Angle)) * Offset, 0.f);
    CurrentPhase++;
    UpdateZoneVisual();

    if (ALCGameState* GS = Cast<ALCGameState>(UGameplayStatics::GetGameState(this)))
    {
        GS->SetZoneRadius(ZoneRadius);
        GS->SetZoneCenter(ZoneCenter);
    }
}

void ALCZoneManager::UpdateZoneVisual()
{
    if (!ZoneMesh) return;
    ZoneMesh->ClearAllMeshSections();

    // Create cylinder approximation for zone boundary
    int32 Segments = 64;
    float Height = 500.f;
    TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Normals;
    TArray<FColor> Colors; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;

    for (int32 i = 0; i < Segments; ++i)
    {
        float A1 = 2.f * PI * i / Segments;
        float A2 = 2.f * PI * ((i + 1) % Segments) / Segments;
        int32 Base = Verts.Num();
        Verts.Add(ZoneCenter + FVector(ZoneRadius * FMath::Cos(A1), ZoneRadius * FMath::Sin(A1), 0.f));
        Verts.Add(ZoneCenter + FVector(ZoneRadius * FMath::Cos(A1), ZoneRadius * FMath::Sin(A1), Height));
        Verts.Add(ZoneCenter + FVector(ZoneRadius * FMath::Cos(A2), ZoneRadius * FMath::Sin(A2), 0.f));
        Verts.Add(ZoneCenter + FVector(ZoneRadius * FMath::Cos(A2), ZoneRadius * FMath::Sin(A2), Height));
        for (int32 j = 0; j < 4; ++j)
        {
            Colors.Add(FColor(0, 100, 255, 80));
            UVs.Add(FVector2D::ZeroVector);
        }
        Tris.Add(Base); Tris.Add(Base + 1); Tris.Add(Base + 2);
        Tris.Add(Base + 2); Tris.Add(Base + 1); Tris.Add(Base + 3);
    }
    ZoneMesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);
}
