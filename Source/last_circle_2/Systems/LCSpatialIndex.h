#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "LCSpatialIndex.generated.h"

// Grid-based spatial indexing for performance optimization
// Divides the 6000x6000 map into cells for efficient entity queries

USTRUCT()
struct FSpatialCell
{
    GENERATED_BODY()
    UPROPERTY() TArray<AActor*> Entities;
    int32 EntityCount = 0;
};

UCLASS()
class LAST_CIRCLE_2_API ALCSpatialIndex : public AActor
{
    GENERATED_BODY()
public:
    ALCSpatialIndex()
    {
        PrimaryActorTick.bCanEverTick = true;
        PrimaryActorTick.TickInterval = TickInterval;
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        InitializeGrid();
    }

    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        UpdateIndex();
    }

    // Query entities within radius
    UFUNCTION(BlueprintCallable)
    TArray<AActor*> QueryRadius(const FVector& Center, float Radius) const
    {
        TArray<AActor*> Results;
        int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
        FIntPoint CenterCell = WorldToCell(Center);

        for (int32 dx = -CellRadius; dx <= CellRadius; ++dx)
        {
            for (int32 dy = -CellRadius; dy <= CellRadius; ++dy)
            {
                int32 CX = CenterCell.X + dx;
                int32 CY = CenterCell.Y + dy;
                if (CX < 0 || CX >= GridSize || CY < 0 || CY >= GridSize) continue;

                const FSpatialCell& Cell = Grid[CX * GridSize + CY];
                for (AActor* Entity : Cell.Entities)
                {
                    if (Entity && Entity->IsValidLowLevel())
                    {
                        float Dist = FVector::Dist(Center, Entity->GetActorLocation());
                        if (Dist <= Radius)
                        {
                            Results.Add(Entity);
                        }
                    }
                }
            }
        }
        return Results;
    }

    // Query entities by tag within radius
    UFUNCTION(BlueprintCallable)
    TArray<AActor*> QueryRadiusWithTag(const FVector& Center, float Radius, FName Tag) const
    {
        TArray<AActor*> Results;
        TArray<AActor*> All = QueryRadius(Center, Radius);
        for (AActor* A : All)
        {
            if (A && A->ActorHasTag(Tag)) Results.Add(A);
        }
        return Results;
    }

    // Get nearest entity of type
    UFUNCTION(BlueprintCallable)
    AActor* FindNearest(const FVector& Location, FName Tag, float MaxRange = 5000.f) const
    {
        TArray<AActor*> Candidates = QueryRadiusWithTag(Location, MaxRange, Tag);
        AActor* Nearest = nullptr;
        float MinDist = MAX_FLT;
        for (AActor* A : Candidates)
        {
            float D = FVector::Dist(Location, A->GetActorLocation());
            if (D < MinDist) { MinDist = D; Nearest = A; }
        }
        return Nearest;
    }

    // Performance: Get entities visible to player (within view distance)
    UFUNCTION(BlueprintCallable)
    TArray<AActor*> GetVisibleEntities(float ViewDistance = 600.f) const
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return TArray<AActor*>();
        return QueryRadius(PC->GetPawn()->GetActorLocation(), ViewDistance);
    }

    // Get count for LOD decisions
    UFUNCTION(BlueprintCallable)
    int32 GetEntityCountInRadius(const FVector& Center, float Radius) const
    {
        return QueryRadius(Center, Radius).Num();
    }

    // Register an entity
    UFUNCTION(BlueprintCallable)
    void RegisterEntity(AActor* Entity)
    {
        if (!Entity) return;
        FIntPoint Cell = WorldToCell(Entity->GetActorLocation());
        if (Cell.X >= 0 && Cell.X < GridSize && Cell.Y >= 0 && Cell.Y < GridSize)
        {
            Grid[Cell.X * GridSize + Cell.Y].Entities.AddUnique(Entity);
        }
    }

    // Unregister an entity
    UFUNCTION(BlueprintCallable)
    void UnregisterEntity(AActor* Entity)
    {
        if (!Entity) return;
        for (int32 i = 0; i < GridSize * GridSize; ++i)
        {
            Grid[i].Entities.Remove(Entity);
        }
    }

    UFUNCTION(BlueprintCallable)
    int32 GetTotalTrackedEntities() const
    {
        int32 Count = 0;
        for (int32 i = 0; i < GridSize * GridSize; ++i)
        {
            Count += Grid[i].Entities.Num();
        }
        return Count;
    }

protected:
    static constexpr int32 GridSize = 30; // 30x30 grid = 200 units per cell
    static constexpr float CellSize = 200.f;
    static constexpr float TickInterval = 0.5f;

    UPROPERTY() TArray<FSpatialCell> Grid;

    void InitializeGrid()
    {
        Grid.SetNum(GridSize * GridSize);
        for (int32 i = 0; i < GridSize * GridSize; ++i)
        {
            Grid[i].Entities.Empty();
        }
    }

    FIntPoint WorldToCell(const FVector& WorldPos) const
    {
        int32 CX = FMath::Clamp(FMath::FloorToInt((WorldPos.X + 3000.f) / CellSize), 0, GridSize - 1);
        int32 CY = FMath::Clamp(FMath::FloorToInt((WorldPos.Y + 3000.f) / CellSize), 0, GridSize - 1);
        return FIntPoint(CX, CY);
    }

    void UpdateIndex()
    {
        // Clear all cells
        for (int32 i = 0; i < GridSize * GridSize; ++i)
        {
            // Remove invalid actors
            Grid[i].Entities.RemoveAll([](AActor* A) { return !A || !A->IsValidLowLevel(); });
        }

        // Re-index all tracked entities
        TArray<AActor*> AllEntities;
        for (int32 i = 0; i < GridSize * GridSize; ++i)
        {
            AllEntities.Append(Grid[i].Entities);
        }
        // Clear
        for (int32 i = 0; i < GridSize * GridSize; ++i)
        {
            Grid[i].Entities.Empty();
        }
        // Re-add
        for (AActor* Entity : AllEntities)
        {
            if (Entity && Entity->IsValidLowLevel())
            {
                FIntPoint Cell = WorldToCell(Entity->GetActorLocation());
                Grid[Cell.X * GridSize + Cell.Y].Entities.Add(Entity);
            }
        }
    }
};

// ===================== LOD Manager for Performance =====================
UCLASS()
class LAST_CIRCLE_2_API ALCLODManager : public AActor
{
    GENERATED_BODY()
public:
    ALCLODManager()
    {
        PrimaryActorTick.bCanEverTick = true;
        PrimaryActorTick.TickInterval = 1.f;
    }
    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;
        FVector PlayerLoc = PC->GetPawn()->GetActorLocation();

        // Apply LOD rules to all actors
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            AActor* A = *It;
            if (!A || A == PC->GetPawn()) continue;

            float Dist = FVector::Dist(PlayerLoc, A->GetActorLocation());

            // >600: Hide actors
            if (Dist > HideDistance)
            {
                A->SetActorHiddenInGame(true);
                A->SetActorTickEnabled(false);
            }
            // >400: Reduce tick rate
            else if (Dist > ThrottleDistance)
            {
                A->SetActorHiddenInGame(false);
                A->SetActorTickEnabled(true);
                A->PrimaryActorTick.TickInterval = 0.5f;
            }
            // Normal
            else
            {
                A->SetActorHiddenInGame(false);
                A->SetActorTickEnabled(true);
                A->PrimaryActorTick.TickInterval = 0.f;
            }
        }
    }

protected:
    UPROPERTY() float HideDistance = 600.f;
    UPROPERTY() float ThrottleDistance = 400.f;
};
