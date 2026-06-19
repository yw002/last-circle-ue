#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
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

        // Health bar (bottom-left) - green bar
        HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
        if (HealthBar)
        {
            HealthBar->SetFillColorAndOpacity(FLinearColor(0.1f, 0.9f, 0.1f));
            HealthBar->SetPercent(1.f);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(HealthBar));
            if (S) { S->SetAnchors(FAnchors(0.02f, 0.92f, 0.25f, 0.955f)); }
        }

        // Weapon name (bottom-right, above ammo) - gray
        WeaponNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeaponNameText"));
        if (WeaponNameText)
        {
            WeaponNameText->SetJustification(ETextJustify::Right);
            WeaponNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
            WeaponNameText->SetText(FText::FromString(TEXT("AKM")));
            FSlateFontInfo Font = WeaponNameText->GetFont(); Font.Size = 20; WeaponNameText->SetFont(Font);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(WeaponNameText));
            if (S) { S->SetAnchors(FAnchors(0.78f, 0.85f, 0.98f, 0.88f)); S->SetAutoSize(true); S->SetAlignment(FVector2D(1.f, 1.f)); }
        }

        // Ammo text (bottom-right) - large white "30 / 300"
        AmmoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoText"));
        if (AmmoText)
        {
            AmmoText->SetJustification(ETextJustify::Right);
            AmmoText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            AmmoText->SetText(FText::FromString(TEXT("30 / 300")));
            FSlateFontInfo Font = AmmoText->GetFont(); Font.Size = 36; AmmoText->SetFont(Font);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(AmmoText));
            if (S) { S->SetAnchors(FAnchors(0.78f, 0.88f, 0.98f, 0.97f)); S->SetAutoSize(true); S->SetAlignment(FVector2D(1.f, 1.f)); }
        }

        // Reloading text (center-bottom) - orange
        ReloadText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReloadText"));
        if (ReloadText)
        {
            ReloadText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.f)));
            ReloadText->SetVisibility(ESlateVisibility::Hidden);
            FSlateFontInfo Font = ReloadText->GetFont(); Font.Size = 24; ReloadText->SetFont(Font);
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(ReloadText));
            if (S) { S->SetAnchors(FAnchors(0.4f, 0.55f, 0.6f, 0.6f)); S->SetAutoSize(true); S->SetAlignment(FVector2D(0.5f, 0.5f)); }
        }

        // Crosshair: 4 UTextBlock with box-drawing chars (always render, no texture needed)
        for (int32 i = 0; i < 4; ++i)
        {
            CrosshairTexts[i] = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("XHair%d"), i)));
            if (CrosshairTexts[i])
            {
                CrosshairTexts[i]->SetColorAndOpacity(FSlateColor(FLinearColor(0.1f, 1.f, 0.1f, 0.9f)));
                FSlateFontInfo Font = CrosshairTexts[i]->GetFont(); Font.Size = 18; CrosshairTexts[i]->SetFont(Font);
                UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(CrosshairTexts[i]));
                if (S) { S->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f)); S->SetAutoSize(true); }
            }
        }
        CrosshairTexts[0]->SetText(FText::FromString(TEXT("\xE2\x94\x82"))); // top "│"
        CrosshairTexts[1]->SetText(FText::FromString(TEXT("\xE2\x94\x82"))); // bottom "│"
        CrosshairTexts[2]->SetText(FText::FromString(TEXT("\xE2\x94\x80\xE2\x94\x80"))); // left "──"
        CrosshairTexts[3]->SetText(FText::FromString(TEXT("\xE2\x94\x80\xE2\x94\x80"))); // right "──"

        // Alive count (top-right)
        AliveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AliveText"));
        if (AliveText)
        {
            AliveText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(AliveText));
            if (S) { S->SetAnchors(FAnchors(0.9f, 0.02f, 1.f, 0.05f)); S->SetAutoSize(true); }
        }
    }

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override
    {
        Super::NativeTick(MyGeometry, InDeltaTime);
        UpdateCrosshair();
        if (HitTimer > 0.f) { HitTimer -= InDeltaTime; if (HitTimer <= 0.f) bHitConfirmed = false; }
    }

    void UpdateCrosshair()
    {
        float LineLen = bIsADS ? 8.f : 14.f;
        float Gap = (bIsADS ? 3.f : 5.f) + CurrentCrosshairSpread * (bIsADS ? 0.4f : 1.5f);
        FLinearColor Col = bHitConfirmed ? FLinearColor(1.f, 0.2f, 0.2f, 0.9f) : FLinearColor(0.1f, 1.f, 0.1f, 0.9f);

        auto SetPos = [&](int32 Idx, float OX, float OY, float W, float H) {
            if (!CrosshairTexts[Idx]) return;
            CrosshairTexts[Idx]->SetColorAndOpacity(FSlateColor(Col));
            UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(CrosshairTexts[Idx]->Slot);
            if (S) S->SetOffsets(FMargin(OX - W, OY, OX + W, OY));
        };

        SetPos(0, 0.f, 0.f - Gap - LineLen, 3.f, 2.f);   // top "│"
        SetPos(1, 0.f, 0.f + Gap, 3.f, 2.f);              // bottom "│"
        SetPos(2, 0.f - Gap - LineLen, 0.f, 4.f, 2.f);    // left "──"
        SetPos(3, 0.f + Gap, 0.f, 4.f, 2.f);              // right "──"
    }

    UFUNCTION(BlueprintCallable) void UpdateHealth(float Percent) { if (HealthBar) HealthBar->SetPercent(Percent); }

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
            if (bReloading) { ReloadText->SetVisibility(ESlateVisibility::Visible); ReloadText->SetText(FText::FromString(TEXT("RELOADING..."))); }
            else { ReloadText->SetVisibility(ESlateVisibility::Hidden); }
        }
    }

    UFUNCTION(BlueprintCallable) void SetCrosshairSpread(float Spread) { CurrentCrosshairSpread = Spread; }
    UFUNCTION(BlueprintCallable) void SetCrosshairADS(bool bADS) { bIsADS = bADS; }
    UFUNCTION(BlueprintCallable) void SetCrosshairHit(bool bHit) { bHitConfirmed = bHit; HitTimer = 0.3f; }

    UFUNCTION(BlueprintCallable)
    void ShowNotification(const FString& Message, float Duration);

    UFUNCTION(BlueprintCallable)
    void AddKillFeedEntry(const FString& Killer, const FString& Victim, const FString& Weapon, bool bHeadshot);

private:
    UPROPERTY() UCanvasPanel* RootCanvas = nullptr;
    UPROPERTY() UProgressBar* HealthBar = nullptr;
    UPROPERTY() UTextBlock* WeaponNameText = nullptr;
    UPROPERTY() UTextBlock* AmmoText = nullptr;
    UPROPERTY() UTextBlock* ReloadText = nullptr;
    UPROPERTY() UTextBlock* AliveText = nullptr;
    UPROPERTY() UTextBlock* CrosshairTexts[4] = {};
    UPROPERTY() float CurrentCrosshairSpread = 0.f;
    UPROPERTY() bool bIsADS = false;
    UPROPERTY() bool bHitConfirmed = false;
    UPROPERTY() float HitTimer = 0.f;
};
