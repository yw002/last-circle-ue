#include "Effects/LCEffectsSystem.h"
#include "Characters/LCBaseCharacter.h"

// ===================== Blood Effect =====================
ALCBloodEffect::ALCBloodEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    BloodMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BloodMesh"));
    RootComponent = BloodMesh;
    BloodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BuildBloodParticles();
    Lifespan = 1.5f;
    SetLifeSpan(Lifespan);
}

void ALCBloodEffect::Tick(float DT)
{
    Super::Tick(DT);
    Elapsed += DT;
    float Alpha = Elapsed / Lifespan;
    // Fade and expand
    if (BloodMesh)
    {
        float Scale = 1.f + Alpha * 2.f;
        BloodMesh->SetWorldScale3D(FVector(Scale));
    }
}

void ALCBloodEffect::BuildBloodParticles()
{
    if (!BloodMesh) return;
    TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
    TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

    // 12 small cubes scattered
    for (int32 i = 0; i < 12; ++i)
    {
        float S = 1.f + FMath::FRandRange(0.f, 1.5f);
        FVector Offset(FMath::FRandRange(-5.f, 5.f), FMath::FRandRange(-5.f, 5.f), FMath::FRandRange(0.f, 8.f));
        int32 Base = V.Num();
        V.Add(Offset + FVector(-S,-S,-S)); V.Add(Offset + FVector(S,-S,-S));
        V.Add(Offset + FVector(S,S,-S)); V.Add(Offset + FVector(-S,S,-S));
        V.Add(Offset + FVector(-S,-S,S)); V.Add(Offset + FVector(S,-S,S));
        V.Add(Offset + FVector(S,S,S)); V.Add(Offset + FVector(-S,S,S));
        FColor BloodCol(180 + FMath::RandRange(0,40), 0, 0, 255);
        for (int32 j = 0; j < 8; ++j) { C.Add(BloodCol); UV.Add(FVector2D::ZeroVector); }
        T.Add(Base);T.Add(Base+1);T.Add(Base+5);T.Add(Base);T.Add(Base+5);T.Add(Base+4);
        T.Add(Base+2);T.Add(Base+3);T.Add(Base+7);T.Add(Base+2);T.Add(Base+7);T.Add(Base+6);
        T.Add(Base+4);T.Add(Base+5);T.Add(Base+6);T.Add(Base+4);T.Add(Base+6);T.Add(Base+7);
        T.Add(Base);T.Add(Base+3);T.Add(Base+2);T.Add(Base);T.Add(Base+2);T.Add(Base+1);
        T.Add(Base+3);T.Add(Base);T.Add(Base+4);T.Add(Base+3);T.Add(Base+4);T.Add(Base+7);
        T.Add(Base+1);T.Add(Base+2);T.Add(Base+6);T.Add(Base+1);T.Add(Base+6);T.Add(Base+5);
    }
    BloodMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
}

// ===================== Muzzle Flash =====================
ALCMuzzleFlash::ALCMuzzleFlash()
{
    PrimaryActorTick.bCanEverTick = true;
    FlashMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FlashMesh"));
    RootComponent = FlashMesh;
    FlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BuildFlashMesh();
    SetLifeSpan(0.08f);
}

void ALCMuzzleFlash::Tick(float DT)
{
    Super::Tick(DT);
    Timer += DT;
    float Alpha = Timer / 0.08f;
    if (FlashMesh)
    {
        float Scale = 1.f + Alpha * 0.5f;
        FlashMesh->SetWorldScale3D(FVector(Scale));
    }
}

void ALCMuzzleFlash::BuildFlashMesh()
{
    if (!FlashMesh) return;
    TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
    TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

    // White sphere + star shape
    float R = 3.f;
    int32 Segs = 8;
    int32 Base = 0;
    // Central sphere approximation
    V.Add(FVector(0, 0, R)); // Top
    V.Add(FVector(0, 0, -R)); // Bottom
    C.Add(FColor(255, 255, 200)); C.Add(FColor(255, 255, 200));
    UV.Add(FVector2D::ZeroVector); UV.Add(FVector2D::ZeroVector);

    for (int32 i = 0; i < Segs; ++i)
    {
        float A = 2.f * PI * i / Segs;
        V.Add(FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f));
        C.Add(FColor(255, 220, 100));
        UV.Add(FVector2D::ZeroVector);
    }
    for (int32 i = 0; i < Segs; ++i)
    {
        T.Add(0); T.Add(2 + i); T.Add(2 + ((i + 1) % Segs));
        T.Add(1); T.Add(2 + ((i + 1) % Segs)); T.Add(2 + i);
    }

    // Star spikes (4 elongated quads)
    for (int32 i = 0; i < 4; ++i)
    {
        float A = PI * 0.25f * (2 * i + 1);
        float SpikeLen = R * 3.f;
        FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
        FVector Perp = FVector(-Dir.Y, Dir.X, 0.f) * 0.5f;
        int32 B = V.Num();
        V.Add(Dir * SpikeLen + Perp);
        V.Add(Dir * SpikeLen - Perp);
        V.Add(-Perp);
        V.Add(Perp);
        for (int32 j = 0; j < 4; ++j) { C.Add(FColor(255, 255, 150, 200)); UV.Add(FVector2D::ZeroVector); }
        T.Add(B); T.Add(B+1); T.Add(B+2); T.Add(B+2); T.Add(B+1); T.Add(B+3);
    }

    FlashMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
}

// ===================== Explosion Effect =====================
ALCExplosionEffect::ALCExplosionEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    ExplosionMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ExplosionMesh"));
    RootComponent = ExplosionMesh;
    ExplosionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BuildExplosionMesh();

    ExplosionLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ExplosionLight"));
    ExplosionLight->SetupAttachment(RootComponent);
    ExplosionLight->SetIntensity(5000.f);
    ExplosionLight->SetLightColor(FLinearColor(1.f, 0.5f, 0.1f));
    ExplosionLight->SetAttenuationRadius(500.f);
    SetLifeSpan(1.f);
}

void ALCExplosionEffect::Tick(float DT)
{
    Super::Tick(DT);
    Timer += DT;
    float Alpha = Timer / 1.f;
    if (ExplosionMesh)
    {
        float Scale = 1.f + Alpha * 5.f;
        ExplosionMesh->SetWorldScale3D(FVector(Scale));
    }
    if (ExplosionLight)
    {
        ExplosionLight->SetIntensity(FMath::Max(0.f, 5000.f * (1.f - Alpha)));
    }
}

void ALCExplosionEffect::BuildExplosionMesh()
{
    if (!ExplosionMesh) return;
    TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
    TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

    // Orange/red sphere
    float R = 20.f;
    int32 Rings = 6, Segs = 8;
    for (int32 ring = 0; ring < Rings; ++ring)
    {
        float Phi = PI * ring / Rings;
        for (int32 seg = 0; seg < Segs; ++seg)
        {
            float Theta = 2.f * PI * seg / Segs;
            V.Add(FVector(R * FMath::Sin(Phi) * FMath::Cos(Theta),
                          R * FMath::Sin(Phi) * FMath::Sin(Theta),
                          R * FMath::Cos(Phi)));
            float T_alpha = (float)ring / Rings;
            C.Add(FColor(255, FMath::Lerp(200, 50, T_alpha), 0, 200));
            UV.Add(FVector2D::ZeroVector);
        }
    }
    for (int32 ring = 0; ring < Rings - 1; ++ring)
    {
        for (int32 seg = 0; seg < Segs; ++seg)
        {
            int32 Curr = ring * Segs + seg;
            int32 Next = ring * Segs + (seg + 1) % Segs;
            int32 Below = (ring + 1) * Segs + seg;
            int32 BelowNext = (ring + 1) * Segs + (seg + 1) % Segs;
            T.Add(Curr); T.Add(Below); T.Add(Next);
            T.Add(Next); T.Add(Below); T.Add(BelowNext);
        }
    }
    ExplosionMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
}

// ===================== Weather Particles =====================
ALCWeatherParticles::ALCWeatherParticles()
{
    PrimaryActorTick.bCanEverTick = true;
    ParticleMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ParticleMesh"));
    RootComponent = ParticleMesh;
    ParticleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALCWeatherParticles::Tick(float DT)
{
    Super::Tick(DT);
    if (!bActive) return;

    Timer += DT;
    if (Timer > SpawnInterval)
    {
        Timer = 0.f;
        SpawnParticle();
    }
}

void ALCWeatherParticles::SetWeather(EWeatherType Weather)
{
    CurrentWeather = Weather;
    bActive = (Weather != EWeatherType::Clear);
    if (ParticleMesh) ParticleMesh->ClearAllMeshSections();
    ActiveParticles.Empty();

    if (Weather == EWeatherType::Storm)
    {
        SpawnInterval = 0.01f;
        ParticleColor = FColor(150, 150, 200, 180);
    }
    else if (Weather == EWeatherType::Blizzard)
    {
        SpawnInterval = 0.02f;
        ParticleColor = FColor(240, 240, 250, 200);
    }
}

void ALCWeatherParticles::SpawnParticle()
{
    if (!ParticleMesh) return;
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC || !PC->GetPawn()) return;

    FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
    FVector SpawnLoc = PlayerLoc + FVector(FMath::FRandRange(-300.f, 300.f),
                                            FMath::FRandRange(-300.f, 300.f),
                                            FMath::FRandRange(100.f, 300.f));

    FParticleData PD;
    PD.Position = SpawnLoc;
    PD.Velocity = FVector(0.f, 0.f, -200.f - FMath::FRandRange(0.f, 100.f));
    if (CurrentWeather == EWeatherType::Blizzard)
    {
        PD.Velocity = FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), -80.f);
    }
    PD.Lifetime = 2.f;
    ActiveParticles.Add(PD);

    // Build mesh section
    RebuildParticles();
}

void ALCWeatherParticles::RebuildParticles()
{
    if (!ParticleMesh) return;
    ParticleMesh->ClearAllMeshSections();

    TArray<FVector> V; TArray<int32> T; TArray<FVector> N;
    TArray<FColor> C; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;

    int32 MaxParticles = FMath::Min(ActiveParticles.Num(), 200);
    for (int32 i = 0; i < MaxParticles; ++i)
    {
        float S = (CurrentWeather == EWeatherType::Blizzard) ? 1.5f : 0.5f;
        FVector P = ActiveParticles[i].Position;
        int32 Base = V.Num();
        V.Add(P + FVector(-S,-S,-S)); V.Add(P + FVector(S,-S,-S));
        V.Add(P + FVector(S,S,-S)); V.Add(P + FVector(-S,S,-S));
        V.Add(P + FVector(-S,-S,S)); V.Add(P + FVector(S,-S,S));
        V.Add(P + FVector(S,S,S)); V.Add(P + FVector(-S,S,S));
        for (int32 j = 0; j < 8; ++j) { C.Add(ParticleColor); UV.Add(FVector2D::ZeroVector); }
        T.Add(Base);T.Add(Base+1);T.Add(Base+5);T.Add(Base);T.Add(Base+5);T.Add(Base+4);
        T.Add(Base+2);T.Add(Base+3);T.Add(Base+7);T.Add(Base+2);T.Add(Base+7);T.Add(Base+6);
        T.Add(Base+4);T.Add(Base+5);T.Add(Base+6);T.Add(Base+4);T.Add(Base+6);T.Add(Base+7);
        T.Add(Base);T.Add(Base+3);T.Add(Base+2);T.Add(Base);T.Add(Base+2);T.Add(Base+1);
        T.Add(Base+3);T.Add(Base);T.Add(Base+4);T.Add(Base+3);T.Add(Base+4);T.Add(Base+7);
        T.Add(Base+1);T.Add(Base+2);T.Add(Base+6);T.Add(Base+1);T.Add(Base+6);T.Add(Base+5);
    }
    if (V.Num() > 0)
    {
        ParticleMesh->CreateMeshSection(0, V, T, N, UV, C, Tan, false);
    }
}
