#pragma once
#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "LCAudioManager.generated.h"

UENUM(BlueprintType)
enum class EAudioCategory : uint8
{
    Gunshot, Footstep, Explosion, Engine, Ambient, UI, Weather
};

USTRUCT(BlueprintType)
struct FAudioEntry
{
    GENERATED_BODY()
    UPROPERTY() FName SoundName = NAME_None;
    UPROPERTY() float Volume = 1.f;
    UPROPERTY() float Pitch = 1.f;
    UPROPERTY() float Duration = 0.5f;
    UPROPERTY() EAudioCategory Category = EAudioCategory::Gunshot;
};

USTRUCT()
struct FActiveSound
{
    GENERATED_BODY()
    UPROPERTY() FName SoundName = NAME_None;
    UPROPERTY() EAudioCategory Category = EAudioCategory::Gunshot;
    UPROPERTY() float Volume = 1.f;
    UPROPERTY() float Pitch = 1.f;
    UPROPERTY() float RemainingTime = 1.f;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() bool bIsSpatial = true;
};

UCLASS()
class LAST_CIRCLE_2_API ALCAudioManager : public AActor
{
    GENERATED_BODY()
public:
    ALCAudioManager()
    {
        PrimaryActorTick.bCanEverTick = true;
        AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
        RootComponent = AudioComponent;
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        InitializeSoundLibrary();
    }

    virtual void Tick(float DT) override
    {
        Super::Tick(DT);
        // Update spatial audio based on player position
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC && PC->GetPawn())
        {
            AudioListenerLocation = PC->GetPawn()->GetActorLocation();
        }

        // Clean up expired entries
        for (int32 i = ActiveSounds.Num() - 1; i >= 0; --i)
        {
            ActiveSounds[i].RemainingTime -= DT;
            if (ActiveSounds[i].RemainingTime <= 0.f)
            {
                ActiveSounds.RemoveAt(i);
            }
        }
    }

    UFUNCTION(BlueprintCallable)
    void PlayGunshot(FName WeaponName, const FVector& Location)
    {
        PlaySoundAtLocation(WeaponName, Location, EAudioCategory::Gunshot, 1.f, 1.f);
    }

    UFUNCTION(BlueprintCallable)
    void PlayFootstep(const FVector& Location, float SurfaceType)
    {
        float Volume = FMath::Clamp(0.3f + SurfaceType * 0.1f, 0.1f, 0.5f);
        PlaySoundAtLocation(FName("Footstep"), Location, EAudioCategory::Footstep, Volume, FMath::FRandRange(0.9f, 1.1f));
    }

    UFUNCTION(BlueprintCallable)
    void PlayExplosion(const FVector& Location, float Size)
    {
        float Volume = FMath::Clamp(Size / 100.f, 0.5f, 2.f);
        PlaySoundAtLocation(FName("Explosion"), Location, EAudioCategory::Explosion, Volume, FMath::FRandRange(0.8f, 1.2f));
    }

    UFUNCTION(BlueprintCallable)
    void PlayEngine(const FVector& Location, float RPM)
    {
        float Volume = FMath::Clamp(RPM / 3000.f, 0.1f, 1.f);
        float Pitch = 0.5f + RPM / 6000.f;
        PlaySoundAtLocation(FName("Engine"), Location, EAudioCategory::Engine, Volume, Pitch);
    }

    UFUNCTION(BlueprintCallable)
    void PlayWeatherAmbient(EAudioCategory Category)
    {
        FActiveSound AS;
        AS.SoundName = FName("WeatherAmbient");
        AS.Category = Category;
        AS.Volume = 0.4f;
        AS.Pitch = 1.f;
        AS.RemainingTime = 5.f;
        AS.Location = AudioListenerLocation;
        AS.bIsSpatial = false;
        ActiveSounds.Add(AS);
    }

    UFUNCTION(BlueprintCallable)
    void PlayUISound(FName SoundName)
    {
        FActiveSound AS;
        AS.SoundName = SoundName;
        AS.Category = EAudioCategory::UI;
        AS.Volume = 0.8f;
        AS.Pitch = 1.f;
        AS.RemainingTime = 0.5f;
        AS.Location = FVector::ZeroVector;
        AS.bIsSpatial = false;
        ActiveSounds.Add(AS);
    }

    UFUNCTION(BlueprintCallable)
    float GetDistanceAttenuation(const FVector& SoundLocation) const
    {
        float Dist = FVector::Dist(AudioListenerLocation, SoundLocation);
        return FMath::Clamp(1.f - Dist / MaxHearingDistance, 0.f, 1.f);
    }

    UFUNCTION(BlueprintCallable)
    int32 GetActiveSoundCount() const { return ActiveSounds.Num(); }

protected:
    UPROPERTY(VisibleAnywhere) UAudioComponent* AudioComponent = nullptr;
    UPROPERTY() FVector AudioListenerLocation = FVector::ZeroVector;
    UPROPERTY() float MaxHearingDistance = 5000.f;

    UPROPERTY() TArray<FActiveSound> ActiveSounds;
    UPROPERTY() TMap<FName, FAudioEntry> SoundLibrary;

    void InitializeSoundLibrary()
    {
        // Register all game sounds (procedural audio - no actual audio files)
        auto RegisterSound = [this](const FName& Name, EAudioCategory Cat, float Vol, float Dur) {
            FAudioEntry Entry;
            Entry.SoundName = Name;
            Entry.Category = Cat;
            Entry.Volume = Vol;
            Entry.Duration = Dur;
            SoundLibrary.Add(Name, Entry);
        };

        // Gunshots
        RegisterSound(FName("AKM"), EAudioCategory::Gunshot, 1.f, 0.15f);
        RegisterSound(FName("M416"), EAudioCategory::Gunshot, 0.9f, 0.12f);
        RegisterSound(FName("AWM"), EAudioCategory::Gunshot, 1.5f, 0.3f);
        RegisterSound(FName("Shotgun"), EAudioCategory::Gunshot, 1.3f, 0.2f);
        RegisterSound(FName("Pistol"), EAudioCategory::Gunshot, 0.6f, 0.1f);

        // Effects
        RegisterSound(FName("Explosion"), EAudioCategory::Explosion, 2.f, 1.f);
        RegisterSound(FName("Footstep"), EAudioCategory::Footstep, 0.3f, 0.1f);
        RegisterSound(FName("Engine"), EAudioCategory::Engine, 0.5f, 0.5f);
        RegisterSound(FName("HitMarker"), EAudioCategory::UI, 0.8f, 0.05f);
        RegisterSound(FName("KillConfirm"), EAudioCategory::UI, 0.9f, 0.2f);
        RegisterSound(FName("Pickup"), EAudioCategory::UI, 0.5f, 0.15f);
        RegisterSound(FName("Reload"), EAudioCategory::Gunshot, 0.4f, 0.5f);
    }

    void PlaySoundAtLocation(FName SoundName, const FVector& Location, EAudioCategory Category, float Volume, float Pitch)
    {
        FActiveSound AS;
        AS.SoundName = SoundName;
        AS.Category = Category;
        AS.Volume = Volume;
        AS.Pitch = Pitch;
        AS.Location = Location;
        AS.bIsSpatial = true;

        // Apply distance attenuation
        float Attenuation = GetDistanceAttenuation(Location);
        AS.Volume *= Attenuation;

        // Get duration from library
        if (FAudioEntry* Entry = SoundLibrary.Find(SoundName))
        {
            AS.RemainingTime = Entry->Duration;
        }
        else
        {
            AS.RemainingTime = 0.5f;
        }

        if (AS.Volume > 0.01f)
        {
            ActiveSounds.Add(AS);
        }
    }
};
