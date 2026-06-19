#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "LCDeathScreenWidget.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ULCDeathScreenWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override
    {
        Super::NativeConstruct();
        CanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DeathCanvas"));
        if (CanvasPanel) WidgetTree->RootWidget = CanvasPanel;

        // Dark overlay
        Overlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Overlay"));
        if (Overlay)
        {
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(Overlay));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); CPSlot->SetAutoSize(false); }
        }

        // "YOU DIED" text
        DeathText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeathText"));
        if (DeathText)
        {
            DeathText->SetText(FText::FromString(TEXT("YOU DIED")));
            DeathText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(DeathText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.3f, 0.35f, 0.7f, 0.45f)); CPSlot->SetAutoSize(true); }
        }

        // Stats
        StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
        if (StatsText)
        {
            StatsText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(StatsText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.35f, 0.5f, 0.65f, 0.65f)); CPSlot->SetAutoSize(true); }
        }

        // Killer info
        KillerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KillerText"));
        if (KillerText)
        {
            KillerText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(KillerText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.35f, 0.45f, 0.65f, 0.5f)); CPSlot->SetAutoSize(true); }
        }

        SetVisibility(ESlateVisibility::Collapsed);
    }

    UFUNCTION(BlueprintCallable)
    void ShowDeathScreen(const FString& KillerName, int32 WaveReached, int32 Kills, float SurvivalTime)
    {
        SetVisibility(ESlateVisibility::Visible);
        if (KillerText)
            KillerText->SetText(FText::FromString(FString::Printf(TEXT("Killed by: %s"), *KillerName)));
        if (StatsText)
            StatsText->SetText(FText::FromString(FString::Printf(
                TEXT("Wave: %d | Kills: %d | Time: %.0fs"), WaveReached, Kills, SurvivalTime)));
    }

    UFUNCTION(BlueprintCallable)
    void HideDeathScreen() { SetVisibility(ESlateVisibility::Collapsed); }

protected:
    UPROPERTY() UCanvasPanel* CanvasPanel = nullptr;
    UPROPERTY() UImage* Overlay = nullptr;
    UPROPERTY() UTextBlock* DeathText = nullptr;
    UPROPERTY() UTextBlock* StatsText = nullptr;
    UPROPERTY() UTextBlock* KillerText = nullptr;
};
