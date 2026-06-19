#include "Characters/LCBaseCharacter.h"
#include "Core/LCGameMode.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#if WITH_EDITOR
#include "Materials/MaterialExpressionVertexColor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

ALCBaseCharacter::ALCBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(40.f, 100.f);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    BodyMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetMesh());
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HeadMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(GetMesh());
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
    GetCharacterMovement()->JumpZVelocity = 520.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxStepHeight = 150.f;
    GetCharacterMovement()->GroundFriction = 32.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 4096.f;
    GetCharacterMovement()->bUseSeparateBrakingFriction = false;
    GetCharacterMovement()->BrakingFrictionFactor = 2.f;
    GetCharacterMovement()->PerchRadiusThreshold = 10.f;
    GetCharacterMovement()->PerchAdditionalHeight = 30.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched = BaseSpeed * 0.6f;
    GetCharacterMovement()->SetWalkableFloorAngle(70.f);
    GetCharacterMovement()->bAlwaysCheckFloor = true;
    GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
    JumpMaxCount = 2;
}

void ALCBaseCharacter::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth * HealthMultiplier;

    VertexColorMaterial = ALCBaseCharacter::GetOrCreateVertexColorMaterial();
}

float ALCBaseCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    // Check if headshot via damage type
    bool bIsHeadshot = false;
    if (DamageEvent.DamageTypeClass)
    {
        // Use a custom damage type or check tag
        bIsHeadshot = DamageEvent.DamageTypeClass->GetName().Contains("Headshot");
    }

    float ActualDamage = DamageAmount * DamageMultiplier;

    if (bIsHeadshot)
    {
        float HelmetRed = GetHelmetReduction();
        ActualDamage *= 2.5f * (1.f - HelmetRed);
    }
    else
    {
        float ArmorRed = GetArmorReduction();
        ActualDamage *= (1.f - ArmorRed);
    }

    float AppliedDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
    SetHealth(Health - ActualDamage);

    FlashRed();

    if (Health <= 0.f)
    {
        Die(DamageCauser);
    }

    return ActualDamage;
}

void ALCBaseCharacter::Die(AActor* Killer)
{
    if (bIsDead) return;
    bIsDead = true;
    Health = 0.f;

    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Notify game mode
    if (ALCGameMode* GM = Cast<ALCGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->OnEntityKilled(Killer, this, false);
    }

    // Ragdoll-like: tilt the mesh
    SetLifeSpan(5.f);
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
    {
        if (IsValid(this))
        {
            FRotator Tilt(0.f, 0.f, 90.f);
            SetActorRotation(Tilt);
        }
    }, 0.1f, false);
}

void ALCBaseCharacter::SetSpeedMultiplier(float Mult)
{
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * Mult;
}

float ALCBaseCharacter::GetHelmetReduction() const
{
    switch (HelmetLevel)
    {
    case EEquipmentLevel::L1: return 0.15f;
    case EEquipmentLevel::L2: return 0.25f;
    case EEquipmentLevel::L3: return 0.35f;
    default: return 0.f;
    }
}

float ALCBaseCharacter::GetArmorReduction() const
{
    switch (ArmorLevel)
    {
    case EEquipmentLevel::L1: return 0.20f;
    case EEquipmentLevel::L2: return 0.35f;
    case EEquipmentLevel::L3: return 0.50f;
    default: return 0.f;
    }
}

void ALCBaseCharacter::BuildProceduralBody(const FColor& SkinColor, const FColor& ClothColor)
{
    if (!BodyMesh || !HeadMesh) return;

    // Clear existing sections
    BodyMesh->ClearAllMeshSections();
    HeadMesh->ClearAllMeshSections();

    // Torso (box) - enlarged for 40-radius capsule
    BuildBodyPart(BodyMesh, FVector(0.f, 0.f, 0.f), FVector(16.f, 22.f, 32.f), ClothColor);

    // Left arm
    BuildBodyPart(BodyMesh, FVector(0.f, -18.f, 6.f), FVector(8.f, 8.f, 28.f), SkinColor);
    // Right arm
    BuildBodyPart(BodyMesh, FVector(0.f, 18.f, 6.f), FVector(8.f, 8.f, 28.f), SkinColor);

    // Left leg
    BuildBodyPart(BodyMesh, FVector(0.f, -7.f, -24.f), FVector(9.f, 9.f, 32.f), ClothColor);
    // Right leg
    BuildBodyPart(BodyMesh, FVector(0.f, 7.f, -24.f), FVector(9.f, 9.f, 32.f), ClothColor);

    // Head (sphere approximation)
    BuildHead(HeadMesh, FVector(0.f, 0.f, 28.f), 12.f, SkinColor);
}

void ALCBaseCharacter::ApplyVertexColorMaterial()
{
    if (!VertexColorMaterial) return;
    if (BodyMesh) for (int32 i = 0; i < BodyMesh->GetNumSections(); ++i)
        BodyMesh->SetMaterial(i, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
    if (HeadMesh) for (int32 i = 0; i < HeadMesh->GetNumSections(); ++i)
        HeadMesh->SetMaterial(i, UMaterialInstanceDynamic::Create(VertexColorMaterial, this));
}

void ALCBaseCharacter::BuildBodyPart(UProceduralMeshComponent* InMesh, const FVector& Offset,
    const FVector& Size, const FColor& Color)
{
    int32 SectionIndex = InMesh->GetNumSections();

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FColor> VertexColors;
    TArray<FVector2D> UVs;

    float HX = Size.X * 0.5f;
    float HY = Size.Y * 0.5f;
    float HZ = Size.Z * 0.5f;

    // 8 vertices of a box
    Vertices.Add(Offset + FVector(-HX, -HY, -HZ));
    Vertices.Add(Offset + FVector( HX, -HY, -HZ));
    Vertices.Add(Offset + FVector( HX,  HY, -HZ));
    Vertices.Add(Offset + FVector(-HX,  HY, -HZ));
    Vertices.Add(Offset + FVector(-HX, -HY,  HZ));
    Vertices.Add(Offset + FVector( HX, -HY,  HZ));
    Vertices.Add(Offset + FVector( HX,  HY,  HZ));
    Vertices.Add(Offset + FVector(-HX,  HY,  HZ));

    // 6 faces (2 triangles each = 36 indices)
    // Front
    Triangles.Add(0); Triangles.Add(1); Triangles.Add(5);
    Triangles.Add(0); Triangles.Add(5); Triangles.Add(4);
    // Back
    Triangles.Add(2); Triangles.Add(3); Triangles.Add(7);
    Triangles.Add(2); Triangles.Add(7); Triangles.Add(6);
    // Top
    Triangles.Add(4); Triangles.Add(5); Triangles.Add(6);
    Triangles.Add(4); Triangles.Add(6); Triangles.Add(7);
    // Bottom
    Triangles.Add(0); Triangles.Add(3); Triangles.Add(2);
    Triangles.Add(0); Triangles.Add(2); Triangles.Add(1);
    // Left
    Triangles.Add(3); Triangles.Add(0); Triangles.Add(4);
    Triangles.Add(3); Triangles.Add(4); Triangles.Add(7);
    // Right
    Triangles.Add(1); Triangles.Add(2); Triangles.Add(6);
    Triangles.Add(1); Triangles.Add(6); Triangles.Add(5);

    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        VertexColors.Add(Color);
        UVs.Add(FVector2D(0.f, 0.f));
    }

    InMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), true);
    InMesh->SetMeshSectionVisible(SectionIndex, true);
}

void ALCBaseCharacter::BuildHead(UProceduralMeshComponent* InMesh, const FVector& Offset, float Radius, const FColor& Color)
{
    // Simple sphere approximation using octahedron
    int32 SectionIndex = InMesh->GetNumSections();

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FColor> VertexColors;
    TArray<FVector2D> UVs;

    int32 Segments = 8;
    int32 Rings = 6;

    // Top vertex
    Vertices.Add(Offset + FVector(0.f, 0.f, Radius));
    // Bottom vertex
    Vertices.Add(Offset + FVector(0.f, 0.f, -Radius));

    for (int32 Ring = 1; Ring < Rings; ++Ring)
    {
        float Phi = PI * Ring / Rings;
        float SinPhi = FMath::Sin(Phi);
        float CosPhi = FMath::Cos(Phi);

        for (int32 Seg = 0; Seg < Segments; ++Seg)
        {
            float Theta = 2.f * PI * Seg / Segments;
            FVector V(
                Radius * SinPhi * FMath::Cos(Theta),
                Radius * SinPhi * FMath::Sin(Theta),
                Radius * CosPhi
            );
            Vertices.Add(Offset + V);
        }
    }

    // Top cap triangles
    for (int32 Seg = 0; Seg < Segments; ++Seg)
    {
        int32 Next = (Seg + 1) % Segments;
        Triangles.Add(0);
        Triangles.Add(2 + Seg);
        Triangles.Add(2 + Next);
    }

    // Bottom cap triangles
    for (int32 Seg = 0; Seg < Segments; ++Seg)
    {
        int32 Next = (Seg + 1) % Segments;
        int32 BaseIdx = 2 + (Rings - 2) * Segments;
        Triangles.Add(1);
        Triangles.Add(BaseIdx + Next);
        Triangles.Add(BaseIdx + Seg);
    }

    // Middle rings
    for (int32 Ring = 0; Ring < Rings - 2; ++Ring)
    {
        for (int32 Seg = 0; Seg < Segments; ++Seg)
        {
            int32 Next = (Seg + 1) % Segments;
            int32 Curr = 2 + Ring * Segments + Seg;
            int32 NextCurr = 2 + Ring * Segments + Next;
            int32 Below = 2 + (Ring + 1) * Segments + Seg;
            int32 BelowNext = 2 + (Ring + 1) * Segments + Next;

            Triangles.Add(Curr);
            Triangles.Add(Below);
            Triangles.Add(NextCurr);

            Triangles.Add(NextCurr);
            Triangles.Add(Below);
            Triangles.Add(BelowNext);
        }
    }

    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        VertexColors.Add(Color);
        UVs.Add(FVector2D(0.f, 0.f));
    }

    InMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), true);
    InMesh->SetMeshSectionVisible(SectionIndex, true);
}

void ALCBaseCharacter::FlashRed(float Duration)
{
    if (BodyMesh)
    {
        // Simple flash: set emissive color to red, then restore after timer
        UMaterialInterface* Mat = BodyMesh->GetMaterial(0);
        if (Mat)
        {
            UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Mat, this);
            if (DynMat)
            {
                DynMat->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor::Red);
                BodyMesh->SetMaterial(0, DynMat);

                FTimerHandle Handle;
                GetWorldTimerManager().SetTimer(Handle, [this]()
                {
                    if (IsValid(this) && BodyMesh)
                    {
                        UMaterialInterface* OriginalMat = BodyMesh->GetMaterial(0);
                        if (OriginalMat)
                        {
                            UMaterialInstanceDynamic* RestoreMat = UMaterialInstanceDynamic::Create(OriginalMat, this);
                            if (RestoreMat)
                            {
                                RestoreMat->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor::Black);
                                BodyMesh->SetMaterial(0, RestoreMat);
                            }
                        }
                    }
                }, Duration, false);
            }
        }
    }
}

UMaterial* ALCBaseCharacter::GetOrCreateVertexColorMaterial()
{
    static UMaterial* Cached = nullptr;
    if (Cached) return Cached;

    Cached = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
    if (Cached) return Cached;

    Cached = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_VertexColor.M_VertexColor"));
    if (Cached) return Cached;

#if WITH_EDITOR
    UPackage* Pkg = CreatePackage(*FString("/Game/Materials/M_VertexColor"));
    Cached = NewObject<UMaterial>(Pkg, UMaterial::StaticClass(), FName("M_VertexColor"), RF_Public | RF_Standalone);
    Cached->MaterialDomain = MD_Surface;
    Cached->BlendMode = BLEND_Opaque;

    UMaterialExpressionVertexColor* VtxNode = NewObject<UMaterialExpressionVertexColor>(Cached);
    Cached->GetExpressionCollection().AddExpression(VtxNode);
    Cached->GetEditorOnlyData()->EmissiveColor.Expression = VtxNode;

    Cached->PostLoad();
    FAssetRegistryModule::AssetCreated(Cached);
    Pkg->MarkPackageDirty();
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Pkg, Cached, *FPackageName::LongPackageNameToFilename(TEXT("/Game/Materials/M_VertexColor"), FPackageName::GetAssetPackageExtension()), SaveArgs);
#endif

    if (!Cached)
    {
        Cached = UMaterial::GetDefaultMaterial(MD_Surface);
    }

    return Cached;
}
