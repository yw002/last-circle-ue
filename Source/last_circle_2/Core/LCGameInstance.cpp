#include "Core/LCGameInstance.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"

ULCGameInstance::ULCGameInstance()
{
}

void ULCGameInstance::Init()
{
    Super::Init();
    UE_LOG(LogTemp, Warning, TEXT("=== LCGameInstance Init ==="));
    CreateEnhancedInputAssets();
    UE_LOG(LogTemp, Warning, TEXT("Enhanced Input created: IMC=%s"), IMC_Default ? TEXT("OK") : TEXT("FAILED"));
    InitializeWeaponData();
    UE_LOG(LogTemp, Warning, TEXT("Weapon data initialized: %d weapons"), WeaponDataMap.Num());
}

void ULCGameInstance::Shutdown()
{
    if (IA_Move)         { IA_Move->RemoveFromRoot(); }
    if (IA_Look)         { IA_Look->RemoveFromRoot(); }
    if (IA_Jump)         { IA_Jump->RemoveFromRoot(); }
    if (IA_Sprint)       { IA_Sprint->RemoveFromRoot(); }
    if (IA_Shoot)        { IA_Shoot->RemoveFromRoot(); }
    if (IA_ADS)          { IA_ADS->RemoveFromRoot(); }
    if (IA_Reload)       { IA_Reload->RemoveFromRoot(); }
    if (IA_Interact)     { IA_Interact->RemoveFromRoot(); }
    if (IA_SwapWeapon)   { IA_SwapWeapon->RemoveFromRoot(); }
    if (IA_UseMedkit)    { IA_UseMedkit->RemoveFromRoot(); }
    if (IA_ThrowGrenade) { IA_ThrowGrenade->RemoveFromRoot(); }
    if (IA_EnterVehicle) { IA_EnterVehicle->RemoveFromRoot(); }
    if (IA_MouseWheel)   { IA_MouseWheel->RemoveFromRoot(); }
    if (IA_Crouch)       { IA_Crouch->RemoveFromRoot(); }
    if (IMC_Default)     { IMC_Default->RemoveFromRoot(); }
    Super::Shutdown();
}

void ULCGameInstance::CreateEnhancedInputAssets()
{
    // Create Input Actions
    // Button actions need a UInputTriggerPressed trigger to fire Started/Completed events
    auto CreateButtonIA = [this](const FName& Name) -> UInputAction*
    {
        UInputAction* IA = NewObject<UInputAction>(this, Name);
        IA->ValueType = EInputActionValueType::Boolean;
        // CRITICAL: Without triggers, Started/Completed events never fire!
        UInputTriggerPressed* Trigger = NewObject<UInputTriggerPressed>(IA);
        IA->Triggers.Add(Trigger);
        IA->AddToRoot();
        return IA;
    };
    auto CreateAxisIA = [this](const FName& Name, EInputActionValueType VT) -> UInputAction*
    {
        UInputAction* IA = NewObject<UInputAction>(this, Name);
        IA->ValueType = VT;
        IA->AddToRoot();
        return IA;
    };

    IA_Move         = CreateAxisIA(FName("IA_Move"), EInputActionValueType::Axis2D);
    IA_Look         = CreateAxisIA(FName("IA_Look"), EInputActionValueType::Axis2D);
    IA_Jump         = CreateButtonIA(FName("IA_Jump"));
    IA_Sprint       = CreateButtonIA(FName("IA_Sprint"));
    IA_Shoot        = CreateButtonIA(FName("IA_Shoot"));
    IA_ADS          = CreateButtonIA(FName("IA_ADS"));
    IA_Reload       = CreateButtonIA(FName("IA_Reload"));
    IA_Interact     = CreateButtonIA(FName("IA_Interact"));
    IA_SwapWeapon   = CreateButtonIA(FName("IA_SwapWeapon"));
    IA_UseMedkit    = CreateButtonIA(FName("IA_UseMedkit"));
    IA_ThrowGrenade = CreateButtonIA(FName("IA_ThrowGrenade"));
    IA_EnterVehicle = CreateButtonIA(FName("IA_EnterVehicle"));
    IA_MouseWheel   = CreateAxisIA(FName("IA_MouseWheel"), EInputActionValueType::Axis1D);
    IA_Crouch       = CreateButtonIA(FName("IA_Crouch"));

    // Create Input Mapping Context
    IMC_Default = NewObject<UInputMappingContext>(this, FName("IMC_Default"));
    IMC_Default->AddToRoot();

    auto AddMapping = [this](UInputAction* Action, const FKey& Key, ETriggerEvent Trigger = ETriggerEvent::Started)
    {
        FEnhancedActionKeyMapping& Mapping = IMC_Default->MapKey(Action, Key);
        (void)Trigger; // Trigger event handled via modifiers
        (void)Mapping;
    };

    auto AddMappingWithMods = [this](UInputAction* Action, const FKey& Key,
        const TArray<UInputModifier*>& Modifiers, const TArray<UInputTrigger*>& Triggers)
    {
        FEnhancedActionKeyMapping& Mapping = IMC_Default->MapKey(Action, Key);
        for (UInputModifier* Mod : Modifiers)
        {
            if (Mod) Mapping.Modifiers.Add(Mod);
        }
        for (UInputTrigger* Trig : Triggers)
        {
            if (Trig) Mapping.Triggers.Add(Trig);
        }
    };

    // Helper to create modifiers/triggers
    auto CreateNegate = [this]() -> UInputModifierNegate*
    {
        UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(IMC_Default);
        return Neg;
    };
    auto CreateSwizzle = [this]() -> UInputModifierSwizzleAxis*
    {
        UInputModifierSwizzleAxis* Swiz = NewObject<UInputModifierSwizzleAxis>(IMC_Default);
        Swiz->Order = EInputAxisSwizzle::YXZ;
        return Swiz;
    };
    auto CreateTap = [this]() -> UInputTriggerTap*
    {
        UInputTriggerTap* Tap = NewObject<UInputTriggerTap>(IMC_Default);
        Tap->TapReleaseTimeThreshold = 0.2f;
        return Tap;
    };
    auto CreatePressed = [this]() -> UInputTriggerPressed*
    {
        return NewObject<UInputTriggerPressed>(IMC_Default);
    };

    // Move (WASD)
    {
        // W - forward
        TArray<UInputModifier*> Mods; Mods.Add(CreateSwizzle());
        TArray<UInputTrigger*> Trigs;
        AddMappingWithMods(IA_Move, EKeys::W, Mods, Trigs);
        // S - backward
        TArray<UInputModifier*> ModsS; ModsS.Add(CreateSwizzle()); ModsS.Add(CreateNegate());
        AddMappingWithMods(IA_Move, EKeys::S, ModsS, {});
        // A - left
        TArray<UInputModifier*> ModsA; ModsA.Add(CreateNegate());
        AddMappingWithMods(IA_Move, EKeys::A, ModsA, {});
        // D - right
        AddMapping(IA_Move, EKeys::D);
    }

    // Look (Mouse)
    AddMapping(IA_Look, EKeys::Mouse2D);

    // Jump
    AddMapping(IA_Jump, EKeys::SpaceBar);

    // Sprint
    AddMapping(IA_Sprint, EKeys::LeftShift);

    // Shoot
    AddMapping(IA_Shoot, EKeys::LeftMouseButton);

    // ADS
    AddMapping(IA_ADS, EKeys::RightMouseButton);

    // Reload
    AddMapping(IA_Reload, EKeys::R);

    // Interact
    AddMapping(IA_Interact, EKeys::F);

    // Swap Weapon
    AddMapping(IA_SwapWeapon, EKeys::Q);

    // Use Medkit
    AddMapping(IA_UseMedkit, EKeys::Four);

    // Throw Grenade
    AddMapping(IA_ThrowGrenade, EKeys::G);

    // Enter Vehicle
    AddMapping(IA_EnterVehicle, EKeys::E);

    // Mouse Wheel
    AddMapping(IA_MouseWheel, EKeys::MouseWheelAxis);

    // Crouch
    AddMapping(IA_Crouch, EKeys::C);
}

void ULCGameInstance::InitializeWeaponData()
{
    auto AddWeapon = [this](const FName& Name, EWeaponCategory Cat, float Dmg, float Rate, int32 Mag,
        float Reload, float Spread, float RecV, float RecH, float Range, bool bAuto, int32 Pellets = 1)
    {
        FWeaponData Data;
        Data.WeaponName   = Name;
        Data.Category     = Cat;
        Data.Damage       = Dmg;
        Data.FireRate     = Rate;
        Data.MagSize      = Mag;
        Data.ReloadTime   = Reload;
        Data.SpreadAngle  = Spread;
        Data.RecoilVertical   = RecV;
        Data.RecoilHorizontal = RecH;
        Data.Range        = Range;
        Data.bIsAutomatic = bAuto;
        Data.PelletCount  = Pellets;
        WeaponDataMap.Add(Name, Data);
    };

    // Assault Rifles - range 3000-4000
    AddWeapon(FName("AKM"),       EWeaponCategory::AssaultRifle, 49.f, 100.f, 30, 2.3f, 2.0f, 1.2f, 0.4f, 3500.f, true);
    AddWeapon(FName("M416"),      EWeaponCategory::AssaultRifle, 41.f,  85.f, 30, 2.1f, 1.5f, 0.9f, 0.3f, 3500.f, true);
    AddWeapon(FName("SCAR-L"),    EWeaponCategory::AssaultRifle, 41.f,  95.f, 30, 2.2f, 1.6f, 1.0f, 0.3f, 3500.f, true);
    AddWeapon(FName("M16A4"),     EWeaponCategory::AssaultRifle, 43.f,  80.f, 30, 2.0f, 1.4f, 0.8f, 0.25f, 4000.f, false);
    AddWeapon(FName("AUG"),       EWeaponCategory::AssaultRifle, 41.f,  85.f, 30, 2.1f, 1.3f, 0.85f, 0.28f, 3500.f, true);
    AddWeapon(FName("QBZ"),       EWeaponCategory::AssaultRifle, 41.f,  90.f, 30, 2.2f, 1.5f, 0.95f, 0.3f, 3500.f, true);
    AddWeapon(FName("G36C"),      EWeaponCategory::AssaultRifle, 41.f,  88.f, 30, 2.1f, 1.4f, 0.9f, 0.3f, 3500.f, true);
    AddWeapon(FName("FAMAS"),     EWeaponCategory::AssaultRifle, 39.f,  75.f, 25, 2.0f, 1.8f, 1.1f, 0.35f, 3000.f, true);
    AddWeapon(FName("ACE32"),     EWeaponCategory::AssaultRifle, 43.f,  95.f, 30, 2.3f, 1.7f, 1.0f, 0.3f, 3500.f, true);
    AddWeapon(FName("BerylM762"), EWeaponCategory::AssaultRifle, 47.f,  90.f, 30, 2.4f, 2.2f, 1.5f, 0.5f, 3500.f, true);

    // SMGs - range 1500-2000
    AddWeapon(FName("UMP45"),     EWeaponCategory::SMG, 39.f,  90.f, 25, 2.0f, 1.8f, 0.7f, 0.2f, 2000.f, true);
    AddWeapon(FName("Vector"),    EWeaponCategory::SMG, 33.f,  55.f, 19, 1.8f, 2.0f, 0.6f, 0.2f, 1800.f, true);
    AddWeapon(FName("Uzi"),       EWeaponCategory::SMG, 28.f,  50.f, 30, 1.7f, 2.5f, 0.5f, 0.3f, 1500.f, true);
    AddWeapon(FName("MP5K"),      EWeaponCategory::SMG, 33.f,  80.f, 30, 1.9f, 2.0f, 0.65f, 0.2f, 1800.f, true);
    AddWeapon(FName("PP19"),      EWeaponCategory::SMG, 30.f,  75.f, 53, 2.2f, 2.2f, 0.6f, 0.25f, 1800.f, true);
    AddWeapon(FName("Thompson"),  EWeaponCategory::SMG, 36.f,  80.f, 30, 2.0f, 2.0f, 0.8f, 0.3f, 1800.f, true);

    // Snipers - range 6000-10000
    AddWeapon(FName("Kar98k"),    EWeaponCategory::Sniper, 79.f,  1900.f, 5, 3.5f, 0.5f, 2.5f, 0.3f, 8000.f, false);
    AddWeapon(FName("M24"),       EWeaponCategory::Sniper, 79.f,  1800.f, 5, 3.2f, 0.4f, 2.3f, 0.25f, 8000.f, false);
    AddWeapon(FName("AWM"),       EWeaponCategory::Sniper, 120.f, 1850.f, 5, 4.0f, 0.3f, 3.0f, 0.4f, 10000.f, false);
    AddWeapon(FName("SKS"),       EWeaponCategory::Sniper, 55.f,  250.f,  10, 2.8f, 0.8f, 1.8f, 0.4f, 6000.f, false);
    AddWeapon(FName("Mini14"),    EWeaponCategory::Sniper, 46.f,  180.f,  20, 2.5f, 0.9f, 1.5f, 0.35f, 6000.f, false);
    AddWeapon(FName("Mk14"),      EWeaponCategory::Sniper, 60.f,  130.f,  10, 3.0f, 1.0f, 2.0f, 0.5f, 7000.f, true);
    AddWeapon(FName("SLR"),       EWeaponCategory::Sniper, 58.f,  200.f,  10, 2.8f, 0.9f, 1.9f, 0.4f, 6500.f, false);

    // Shotguns - range 800
    AddWeapon(FName("S686"),      EWeaponCategory::Shotgun, 26.f,  300.f, 2, 2.5f, 6.0f, 2.0f, 0.5f, 800.f, false, 9);
    AddWeapon(FName("S1897"),     EWeaponCategory::Shotgun, 26.f,  500.f, 5, 3.0f, 6.0f, 2.0f, 0.5f, 800.f, false, 9);
    AddWeapon(FName("S12K"),      EWeaponCategory::Shotgun, 22.f,  200.f, 5, 2.8f, 6.5f, 1.8f, 0.5f, 800.f, true, 9);
    AddWeapon(FName("DBS"),       EWeaponCategory::Shotgun, 26.f,  250.f, 14, 3.2f, 6.0f, 2.2f, 0.5f, 800.f, false, 9);

    // Pistols - range 1500-2500
    AddWeapon(FName("P92"),       EWeaponCategory::Pistol, 35.f,  150.f, 15, 1.8f, 2.5f, 0.8f, 0.2f, 1500.f, false);
    AddWeapon(FName("P18C"),      EWeaponCategory::Pistol, 28.f,  70.f,  17, 1.7f, 3.0f, 0.7f, 0.25f, 1500.f, true);
    AddWeapon(FName("DesertEagle"), EWeaponCategory::Pistol, 55.f, 300.f, 7, 2.0f, 2.0f, 1.5f, 0.5f, 2500.f, false);

    // Melee
    {
        FWeaponData Data;
        Data.WeaponName = FName("Pan"); Data.Category = EWeaponCategory::Melee;
        Data.Damage = 80.f; Data.FireRate = 500.f; Data.Range = 80.f; Data.MagSize = 0;
        Data.bIsAutomatic = false; Data.SpreadAngle = 0.f;
        WeaponDataMap.Add(FName("Pan"), Data);
    }
    {
        FWeaponData Data;
        Data.WeaponName = FName("Machete"); Data.Category = EWeaponCategory::Melee;
        Data.Damage = 65.f; Data.FireRate = 400.f; Data.Range = 90.f; Data.MagSize = 0;
        Data.bIsAutomatic = false; Data.SpreadAngle = 0.f;
        WeaponDataMap.Add(FName("Machete"), Data);
    }
    {
        FWeaponData Data;
        Data.WeaponName = FName("SaltedFish"); Data.Category = EWeaponCategory::Melee;
        Data.Damage = 50.f; Data.FireRate = 600.f; Data.Range = 70.f; Data.MagSize = 0;
        Data.bIsAutomatic = false; Data.SpreadAngle = 0.f;
        WeaponDataMap.Add(FName("SaltedFish"), Data);
    }

    // Throwables
    {
        FWeaponData Data;
        Data.WeaponName = FName("Grenade"); Data.Category = EWeaponCategory::Throwable;
        Data.Damage = 100.f; Data.Range = 600.f; Data.MagSize = 1; Data.AOERadius = 100.f; Data.AOEDamage = 100.f;
        Data.bIsAutomatic = false;
        WeaponDataMap.Add(FName("Grenade"), Data);
    }
    {
        FWeaponData Data;
        Data.WeaponName = FName("Flashbang"); Data.Category = EWeaponCategory::Throwable;
        Data.Damage = 0.f; Data.Range = 500.f; Data.MagSize = 1; Data.AOERadius = 80.f;
        Data.bIsAutomatic = false;
        WeaponDataMap.Add(FName("Flashbang"), Data);
    }

    // Special Apocalypse Weapons
    {
        FWeaponData D;
        D.WeaponName = FName("CorrosiveSprayer"); D.Category = EWeaponCategory::Special;
        D.SpecialType = EWeaponSpecialType::CorrosiveSprayer; D.SpecialColor = FColor(180, 220, 0);
        D.Damage = 25.f; D.FireRate = 150.f; D.MagSize = 40; D.ReloadTime = 2.5f; D.Range = 500.f;
        D.DOTDamage = 5.f; D.DOTDuration = 2.8f; D.DOTInterval = 0.65f; D.bIsAutomatic = true;
        WeaponDataMap.Add(D.WeaponName, D);
    }
    {
        FWeaponData D;
        D.WeaponName = FName("ArcChainGun"); D.Category = EWeaponCategory::Special;
        D.SpecialType = EWeaponSpecialType::ArcChainGun; D.SpecialColor = FColor(100, 180, 255);
        D.Damage = 35.f; D.FireRate = 200.f; D.MagSize = 20; D.ReloadTime = 2.0f; D.Range = 600.f;
        D.MaxTargets = 3; D.SecondaryTargetDamageMult = 0.55f; D.bIsAutomatic = true;
        WeaponDataMap.Add(D.WeaponName, D);
    }
    {
        FWeaponData D;
        D.WeaponName = FName("GravityHammer"); D.Category = EWeaponCategory::Special;
        D.SpecialType = EWeaponSpecialType::GravityHammer; D.SpecialColor = FColor(150, 0, 200);
        D.Damage = 60.f; D.FireRate = 800.f; D.MagSize = 8; D.ReloadTime = 3.0f; D.Range = 400.f;
        D.AOERadius = 38.f; D.AOEDamage = 60.f; D.KnockbackForce = 42.f; D.bIsAutomatic = false;
        WeaponDataMap.Add(D.WeaponName, D);
    }
    {
        FWeaponData D;
        D.WeaponName = FName("BloodMistHarvester"); D.Category = EWeaponCategory::Special;
        D.SpecialType = EWeaponSpecialType::BloodMistHarvester; D.SpecialColor = FColor(100, 0, 0);
        D.Damage = 40.f; D.FireRate = 180.f; D.MagSize = 25; D.ReloadTime = 2.2f; D.Range = 700.f;
        D.AOERadius = 32.f; D.AOEDamage = 28.f; D.bIsAutomatic = true;
        WeaponDataMap.Add(D.WeaponName, D);
    }
    {
        FWeaponData D;
        D.WeaponName = FName("RiftRifle"); D.Category = EWeaponCategory::Special;
        D.SpecialType = EWeaponSpecialType::RiftRifle; D.SpecialColor = FColor(180, 0, 255);
        D.Damage = 55.f; D.FireRate = 350.f; D.MagSize = 10; D.ReloadTime = 2.5f; D.Range = 1500.f;
        D.MaxTargets = 2; D.SecondaryTargetDamageMult = 0.62f; D.bIsAutomatic = false;
        WeaponDataMap.Add(D.WeaponName, D);
    }
    {
        FWeaponData D;
        D.WeaponName = FName("InfectionMarker"); D.Category = EWeaponCategory::Special;
        D.SpecialType = EWeaponSpecialType::InfectionMarker; D.SpecialColor = FColor(200, 0, 150);
        D.Damage = 20.f; D.FireRate = 400.f; D.MagSize = 15; D.ReloadTime = 2.0f; D.Range = 600.f;
        D.bIsAutomatic = false;
        WeaponDataMap.Add(D.WeaponName, D);
    }
}

const FWeaponData* ULCGameInstance::GetWeaponData(FName WeaponName) const
{
    return WeaponDataMap.Find(WeaponName);
}
