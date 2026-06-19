#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "LCHUDWidget.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ULCHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override
    {
        Super::NativeConstruct();
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        if (RootCanvas) WidgetTree->RootWidget = RootCanvas;

        // Health bar (bottom-left)
        HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
        if (HealthBar)
        {
            HealthBar->SetFillColorAndOpacity(FLinearColor(0.1f, 0.9f, 0.1f));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(HealthBar));
            if (S) { S->SetAnchors(FAnchors(0.02f, 0.92f, 0.25f, 0.955f)); S->SetAutoSize(false); }
        }

        // Ammo text (bottom-center-right) - current / total
        AmmoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoText"));
        if (AmmoText)
        {
            AmmoText->SetJustification(ETextJustify::Right);
            AmmoText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            AmmoText->SetText(FText::FromString(TEXT("30 / 120")));
            FSlateFontInfo Font = AmmoText->GetFont();
            Font.Size = 36;
            AmmoText->SetFont(Font);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(AmmoText));
            if (S) { S->SetAnchors(FAnchors(0.78f, 0.88f, 0.98f, 0.97f)); S->SetAutoSize(true); S->SetAlignment(FVector2D(1.f, 1.f)); }
        }

        // Weapon name text (bottom-right, above ammo)
        WeaponNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeaponNameText"));
        if (WeaponNameText)
        {
            WeaponNameText->SetJustification(ETextJustify::Right);
            WeaponNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
            WeaponNameText->SetText(FText::FromString(TEXT("AKM")));
            FSlateFontInfo Font = WeaponNameText->GetFont();
            Font.Size = 20;
            WeaponNameText->SetFont(Font);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(WeaponNameText));
            if (S) { S->SetAnchors(FAnchors(0.78f, 0.85f, 0.98f, 0.88f)); S->SetAutoSize(true); S->SetAlignment(FVector2D(1.f, 1.f)); }
        }

        // Reloading text (center-bottom)
        ReloadText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReloadText"));
        if (ReloadText)
        {
            ReloadText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.f)));
            ReloadText->SetText(FText::FromString(TEXT("")));
            ReloadText->SetVisibility(ESlateVisibility::Hidden);
            FSlateFontInfo Font = ReloadText->GetFont();
            Font.Size = 24;
            ReloadText->SetFont(Font);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(ReloadText));
            if (S) { S->SetAnchors(FAnchors(0.4f, 0.55f, 0.6f, 0.6f)); S->SetAutoSize(true); S->SetAlignment(FVector2D(0.5f, 0.5f)); }
        }

        // Crosshair lines - 4 lines (top, bottom, left, right) around center
        for (int32 i = 0; i < 4; ++i)
        {
            CrosshairLines[i] = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*FString::Printf(TEXT("CrosshairLine%d"), i)));
            if (CrosshairLines[i])
            {
                CrosshairLines[i]->SetColorAndOpacity(FLinearColor(0.1f, 1.f, 0.1f, 0.85f));
                UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(CrosshairLines[i]));
                if (S) { S->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f)); S->SetAutoSize(false); }
            }
        }

        // Alive / Wave / KillFeed / Notifications (existing)
        AliveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AliveText"));
        if (AliveText)
        {
            AliveText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(AliveText));
            if (S) { S->SetAnchors(FAnchors(0.9f, 0.02f, 1.f, 0.05f)); S->SetAutoSize(true); }
        }
        WaveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveText"));
        if (WaveText)
        {
            WaveText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(WaveText));
            if (S) { S->SetAnchors(FAnchors(0.4f, 0.02f, 0.6f, 0.05f)); S->SetAutoSize(true); }
        }
        NotificationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotificationText"));
        if (NotificationText)
        {
            NotificationText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(NotificationText));
            if (S) { S->SetAnchors(FAnchors(0.35f, 0.7f, 0.65f, 0.75f)); S->SetAutoSize(true); }
        }
        KillFeedText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KillFeedText"));
        if (KillFeedText)
        {
            KillFeedText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(KillFeedText));
            if (S) { S->SetAnchors(FAnchors(0.75f, 0.05f, 0.98f, 0.15f)); S->SetAutoSize(true); }
        }
    }

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override
    {
        Super::NativeTick(MyGeometry, InDeltaTime);

        // Update crosshair geometry
        UpdateCrosshair(InDeltaTime);

        if (NotificationText && NotifyDuration > 0.f)
        {
            NotifyDuration -= InDeltaTime;
            if (NotifyDuration <= 0.f) NotificationText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    void UpdateCrosshair(float DT)
    {
        float LineLen = bIsADS ? 14.f : 22.f;
        float Gap = (bIsADS ? 3.f : 6.f) + CurrentCrosshairSpread * (bIsADS ? 0.4f : 1.5f);
        float Thick = bIsADS ? 2.5f : 3.f;

        FLinearColor Col = FLinearColor(0.1f, 1.f, 0.1f, 0.85f);
        if (bHitConfirmed) { Col = FLinearColor(1.f, 0.2f, 0.2f, 0.9f); }

        auto PosLine = [&](int32 Idx, float OX, float OY, float W, float H) {
            if (!CrosshairLines[Idx]) return;
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(CrosshairLines[Idx]->Slot);
            if (!S) return;
            S->SetOffsets(FMargin(OX - W * 0.5f, OY - H * 0.5f, OX + W * 0.5f, OY + H * 0.5f));
            S->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            CrosshairLines[Idx]->SetColorAndOpacity(Col);
        };

        float VCenter = 0.f;
        PosLine(0, 0.f, VCenter - Gap - LineLen, Thick, LineLen);  // top
        PosLine(1, 0.f, VCenter + Gap, Thick, LineLen);             // bottom
        PosLine(2, VCenter - Gap - LineLen, 0.f, LineLen, Thick);   // left
        PosLine(3, VCenter + Gap, 0.f, LineLen, Thick);             // right
    }

    UFUNCTION(BlueprintCallable)
    void UpdateHealth(float Percent)
    {
        if (HealthBar) HealthBar->SetPercent(Percent);
    }

    UFUNCTION(BlueprintCallable)
    void UpdateWeaponInfo(const FString& WeaponName, int32 AmmoInMag, int32 TotalAmmo)
    {
        if (WeaponNameText) WeaponNameText->SetText(FText::FromString(WeaponName));
        if (AmmoText) AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), AmmoInMag, TotalAmmo)));
    }

    UFUNCTION(BlueprintCallable)
    void SetReloading(bool bReloading, float Progress)
    {
        if (ReloadText)
        {
            if (bReloading)
            {
                ReloadText->SetVisibility(ESlateVisibility::Visible);
                ReloadText->SetText(FText::FromString(TEXT("RELOADING...")));
            }
            else
            {
                ReloadText->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetCrosshairSpread(float Spread) { CurrentCrosshairSpread = Spread; }
    UFUNCTION(BlueprintCallable)
    void SetCrosshairADS(bool bADS) { bIsADS = bADS; }
    UFUNCTION(BlueprintCallable)
    void SetCrosshairHit(bool bHit) { bHitConfirmed = bHit; }

    UFUNCTION(BlueprintCallable)
    void UpdateAliveCount(int32 Count)
    {
        if (AliveText) AliveText->SetText(FText::FromString(FString::Printf(TEXT("Alive: %d"), Count)));
    }
    UFUNCTION(BlueprintCallable)
    void UpdateWaveInfo(int32 Wave)
    {
        if (WaveText) WaveText->SetText(FText::FromString(FString::Printf(TEXT("Wave %d / 20"), Wave)));
    }
    UFUNCTION(BlueprintCallable)
    void ShowNotification(const FString& Message, float Duration)
    {
        if (NotificationText)
        {
            NotificationText->SetText(FText::FromString(Message));
            NotificationText->SetVisibility(ESlateVisibility::Visible);
            NotifyDuration = Duration;
        }
    }
    UFUNCTION(BlueprintCallable)
    void AddKillFeedEntry(const FString& Killer, const FString& Victim, const FString& Weapon, bool bHeadshot)
    {
        if (KillFeedText)
        {
            FString Entry = FString::Printf(TEXT("[%s] %s -> %s%s"), *Weapon, *Killer, *Victim, bHeadshot ? TEXT(" HS") : TEXT(""));
            KillFeedEntries.Add(Entry);
            if (KillFeedEntries.Num() > 5) KillFeedEntries.RemoveAt(0);
            FString Display;
            for (const FString& E : KillFeedEntries) Display += E + TEXT("\n");
            KillFeedText->SetText(FText::FromString(Display));
        }
    }
    UFUNCTION(BlueprintCallable)
    void OnHitConfirmed(bool bHeadshot)
    {
        bHitConfirmed = true;
    }

private:
    UPROPERTY() UCanvasPanel* RootCanvas = nullptr;
    UPROPERTY() UProgressBar* HealthBar = nullptr;
    UPROPERTY() UTextBlock* WeaponNameText = nullptr;
    UPROPERTY() UTextBlock* AmmoText = nullptr;
    UPROPERTY() UTextBlock* ReloadText = nullptr;
    UPROPERTY() UTextBlock* AliveText = nullptr;
    UPROPERTY() UTextBlock* WaveText = nullptr;
    UPROPERTY() UTextBlock* NotificationText = nullptr;
    UPROPERTY() UTextBlock* KillFeedText = nullptr;
    UPROPERTY() UImage* CrosshairLines[4] = {};
    UPROPERTY() float NotifyDuration = 0.f;
    UPROPERTY() TArray<FString> KillFeedEntries;
    UPROPERTY() float CurrentCrosshairSpread = 0.f;
    UPROPERTY() bool bIsADS = false;
    UPROPERTY() bool bHitConfirmed = false;
};
